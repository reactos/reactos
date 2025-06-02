/*
 * WINE RTF file reader
 *
 * Portions Copyright 2004 Mike McCormack for CodeWeavers
 * Portions Copyright 2006 by Phil Krylov
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
 * Derived from RTF Tools by Paul DuBois (dubois@primate.wisc.edu)
 * Homepage: http://www.snake.net/software/RTF/
 * Original license follows:
 */

/*
 * reader.c - RTF file reader.  Release 1.10.
 *
 * ....
 *
 * Author: Paul DuBois	dubois@primate.wisc.edu
 *
 * This software may be redistributed without restriction and used for
 * any purpose whatsoever.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "editor.h"
#include "rtf.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

static int	_RTFGetChar(RTF_Info *);
static void	_RTFGetToken (RTF_Info *);
static void	_RTFGetToken2 (RTF_Info *);
static int	GetChar (RTF_Info *);
static void	ReadFontTbl (RTF_Info *);
static void	ReadColorTbl (RTF_Info *);
static void	ReadStyleSheet (RTF_Info *);
static void	ReadInfoGroup (RTF_Info *);
static void	ReadPictGroup (RTF_Info *);
static void	ReadObjGroup (RTF_Info *);
static void	Lookup (RTF_Info *, char *);
static int	Hash (const char *);

static void	CharAttr(RTF_Info *info);
static void	CharSet(RTF_Info *info);
static void	DocAttr(RTF_Info *info);

static void	RTFFlushCPOutputBuffer(RTF_Info *info);
static void	RTFPutCodePageChar(RTF_Info *info, int c);

/* ---------------------------------------------------------------------- */


int _RTFGetChar(RTF_Info *info)
{
	int ch;
        ME_InStream *stream = info->stream;

	if (stream->dwSize <= stream->dwUsed)
	{
                ME_StreamInFill(stream);
		/* if error, it's EOF */
		if (stream->editstream->dwError)
			return EOF;
		/* if no bytes read, it's EOF */
		if (stream->dwSize == 0)
			return EOF;
	}
	ch = (unsigned char)stream->buffer[stream->dwUsed++];
	if (!ch)
		 return ' ';
	return ch;
}

void RTFSetEditStream(RTF_Info *info, ME_InStream *stream)
{
        info->stream = stream;
}

static void
RTFDestroyAttrs(RTF_Info *info)
{
	RTFColor	*cp;
	RTFFont		*fp;
	RTFStyle	*sp;
	RTFStyleElt	*eltList, *ep;

	while (info->fontList)
	{
		fp = info->fontList->rtfNextFont;
		free (info->fontList->rtfFName);
		free (info->fontList);
		info->fontList = fp;
	}
	while (info->colorList)
	{
		cp = info->colorList->rtfNextColor;
		free (info->colorList);
		info->colorList = cp;
	}
	while (info->styleList)
	{
		sp = info->styleList->rtfNextStyle;
		eltList = info->styleList->rtfSSEList;
		while (eltList)
		{
			ep = eltList->rtfNextSE;
			free (eltList->rtfSEText);
			free (eltList);
			eltList = ep;
		}
		free (info->styleList->rtfSName);
		free (info->styleList);
		info->styleList = sp;
	}
}


void
RTFDestroy(RTF_Info *info)
{
	if (info->rtfTextBuf)
	{
		free(info->rtfTextBuf);
		free(info->pushedTextBuf);
	}
	RTFDestroyAttrs(info);
	free(info->cpOutputBuffer);
        while (info->tableDef)
        {
                RTFTable *tableDef = info->tableDef;
                info->tableDef = tableDef->parent;
                free(tableDef);
        }
}



/* ---------------------------------------------------------------------- */

/*
 * Callback table manipulation routines
 */


/*
 * Install or return a writer callback for a token class
 */

static void RTFSetClassCallback(RTF_Info *info, int class, RTFFuncPtr callback)
{
	if (class >= 0 && class < rtfMaxClass)
		info->ccb[class] = callback;
}


static RTFFuncPtr RTFGetClassCallback(const RTF_Info *info, int class)
{
	if (class >= 0 && class < rtfMaxClass)
		return info->ccb[class];
	return NULL;
}


/*
 * Initialize the reader.  This may be called multiple times,
 * to read multiple files.  The only thing not reset is the input
 * stream; that must be done with RTFSetStream().
 */

void RTFInit(RTF_Info *info)
{
	int	i;

	if (info->rtfTextBuf == NULL)	/* initialize the text buffers */
	{
		info->rtfTextBuf = malloc (rtfBufSiz);
		info->pushedTextBuf = malloc (rtfBufSiz);
		if (info->rtfTextBuf == NULL || info->pushedTextBuf == NULL) {
			ERR ("Cannot allocate text buffers.\n");
			return;
		}
		info->rtfTextBuf[0] = info->pushedTextBuf[0] = '\0';
	}

	for (i = 0; i < rtfMaxClass; i++)
		RTFSetClassCallback (info, i, NULL);
	for (i = 0; i < rtfMaxDestination; i++)
		RTFSetDestinationCallback (info, i, NULL);

	/* install built-in destination readers */
	RTFSetDestinationCallback (info, rtfFontTbl, ReadFontTbl);
	RTFSetDestinationCallback (info, rtfColorTbl, ReadColorTbl);
	RTFSetDestinationCallback (info, rtfStyleSheet, ReadStyleSheet);
	RTFSetDestinationCallback (info, rtfInfo, ReadInfoGroup);
	RTFSetDestinationCallback (info, rtfPict, ReadPictGroup);
	RTFSetDestinationCallback (info, rtfObject, ReadObjGroup);


	RTFSetReadHook (info, NULL);

	/* dump old lists if necessary */

	RTFDestroyAttrs(info);

        info->ansiCodePage = 1252; /* Latin-1; actually unused */
	info->unicodeLength = 1; /* \uc1 is the default */
	info->codePage = info->ansiCodePage;
        info->defFont = 0;

	info->rtfClass = -1;
	info->pushedClass = -1;
	info->pushedChar = EOF;

	info->rtfLineNum = 0;
	info->rtfLinePos = 0;
	info->prevChar = EOF;
        info->bumpLine = FALSE;

	info->dwCPOutputCount = 0;
        if (!info->cpOutputBuffer)
	{
		info->dwMaxCPOutputCount = 0x1000;
		info->cpOutputBuffer = malloc (info->dwMaxCPOutputCount);
	}

        info->tableDef = NULL;
        info->nestingLevel = 0;
        info->canInheritInTbl = FALSE;
        info->borderType = 0;

        memset(&info->fmt, 0, sizeof(info->fmt));
        info->fmt.cbSize = sizeof(info->fmt);
}

/*
 * Install or return a writer callback for a destination type
 */

void RTFSetDestinationCallback(RTF_Info *info, int dest, RTFFuncPtr callback)
{
	if (dest >= 0 && dest < rtfMaxDestination)
		info->dcb[dest] = callback;
}


static RTFFuncPtr RTFGetDestinationCallback(const RTF_Info *info, int dest)
{
	if (dest >= 0 && dest < rtfMaxDestination)
		return info->dcb[dest];
	return NULL;
}


/* ---------------------------------------------------------------------- */

/*
 * Token reading routines
 */


/*
 * Read the input stream, invoking the writer's callbacks
 * where appropriate.
 */

void RTFRead(RTF_Info *info)
{
	while (RTFGetToken (info) != rtfEOF)
		RTFRouteToken (info);
}


/*
 * Route a token.  If it's a destination for which a reader is
 * installed, process the destination internally, otherwise
 * pass the token to the writer's class callback.
 */

void RTFRouteToken(RTF_Info *info)
{
	RTFFuncPtr	p;

	if (info->rtfClass < 0 || info->rtfClass >= rtfMaxClass)	/* watchdog */
	{
		ERR( "Unknown class %d: %s (reader malfunction)\n",
							info->rtfClass, info->rtfTextBuf);
	}
	if (RTFCheckCM (info, rtfControl, rtfDestination))
	{
		/* invoke destination-specific callback if there is one */
		p = RTFGetDestinationCallback (info, info->rtfMinor);
		if (p != NULL)
		{
			(*p) (info);
			return;
		}
	}
	/* invoke class callback if there is one */
	p = RTFGetClassCallback (info, info->rtfClass);
	if (p != NULL)
		(*p) (info);
}


/*
 * Skip to the end of the current group.  When this returns,
 * writers that maintain a state stack may want to call their
 * state unstacker; global vars will still be set to the group's
 * closing brace.
 */

void RTFSkipGroup(RTF_Info *info)
{
	int	level = 1;

	while (RTFGetToken (info) != rtfEOF)
	{
		if (info->rtfClass == rtfGroup)
		{
			if (info->rtfMajor == rtfBeginGroup)
				++level;
			else if (info->rtfMajor == rtfEndGroup)
			{
				if (--level < 1)
					break;	/* end of initial group */
			}
		}
	}
}

/*
 * Do no special processing on the group.
 *
 * This acts as a placeholder for a callback in order to indicate that it
 * shouldn't be ignored.  Instead it will fallback on the loop in RTFRead.
 */
void RTFReadGroup (RTF_Info *info)
{
}


/*
 * Install or return a token reader hook.
 */

void RTFSetReadHook(RTF_Info *info, RTFFuncPtr f)
{
	info->readHook = f;
}


static RTFFuncPtr RTFGetReadHook(const RTF_Info *info)
{
	return (info->readHook);
}


/*
 * Read one token.  Call the read hook if there is one.  The
 * token class is the return value.  Returns rtfEOF when there
 * are no more tokens.
 */

int RTFGetToken(RTF_Info *info)
{
	RTFFuncPtr	p;

	/* don't try to return anything once EOF is reached */
	if (info->rtfClass == rtfEOF) {
		return rtfEOF;
	}

	for (;;)
	{
		_RTFGetToken (info);
		p = RTFGetReadHook (info);
		if (p != NULL)
			(*p) (info);	/* give read hook a look at token */

		/* Silently discard newlines, carriage returns, nulls.  */
		if (!(info->rtfClass == rtfText && info->rtfFormat != SF_TEXT
			&& (info->rtfMajor == '\r' || info->rtfMajor == '\n' || info->rtfMajor == '\0')))
			break;
	}
	return (info->rtfClass);
}


static void RTFUngetToken(RTF_Info *info)
{
	if (info->pushedClass >= 0)	/* there's already an ungotten token */
		ERR ("cannot unget two tokens\n");
	if (info->rtfClass < 0)
		ERR ("no token to unget\n");
	info->pushedClass = info->rtfClass;
	info->pushedMajor = info->rtfMajor;
	info->pushedMinor = info->rtfMinor;
	info->pushedParam = info->rtfParam;
	lstrcpyA (info->pushedTextBuf, info->rtfTextBuf);
	/* The read hook decrements stackTop on rtfEndGroup, so
	 * increment the value to compensate for it being decremented
	 * twice due to the RTFUngetToken. */
	if(RTFCheckCM (info, rtfGroup, rtfEndGroup))
	{
		info->stack[info->stackTop].style = info->style;
		ME_AddRefStyle(info->style);
		info->stackTop++;
	}
}


static void _RTFGetToken(RTF_Info *info)
{
	if (info->rtfFormat == SF_TEXT)
	{
		info->rtfMajor = GetChar (info);
		info->rtfMinor = 0;
		info->rtfParam = rtfNoParam;
		info->rtfTextBuf[info->rtfTextLen = 0] = '\0';
		if (info->rtfMajor == EOF)
			info->rtfClass = rtfEOF;
		else
			info->rtfClass = rtfText;
		return;
	}

	/* first check for pushed token from RTFUngetToken() */

	if (info->pushedClass >= 0)
	{
		info->rtfClass = info->pushedClass;
		info->rtfMajor = info->pushedMajor;
		info->rtfMinor = info->pushedMinor;
		info->rtfParam = info->pushedParam;
		lstrcpyA (info->rtfTextBuf, info->pushedTextBuf);
		info->rtfTextLen = lstrlenA(info->rtfTextBuf);
		info->pushedClass = -1;
		return;
	}

	/*
	 * Beyond this point, no token is ever seen twice, which is
	 * important, e.g., for making sure no "}" pops the font stack twice.
	 */

	_RTFGetToken2 (info);
}


int
RTFCharSetToCodePage(RTF_Info *info, int charset)
{
	switch (charset)
        {
                case ANSI_CHARSET:
                        return 1252;
                case DEFAULT_CHARSET:
                        return CP_ACP;
                case SYMBOL_CHARSET:
                        return CP_SYMBOL;
                case MAC_CHARSET:
                        return CP_MACCP;
                case SHIFTJIS_CHARSET:
                        return 932;
                case HANGEUL_CHARSET:
                        return 949;
                case JOHAB_CHARSET:
                        return 1361;
                case GB2312_CHARSET:
                        return 936;
                case CHINESEBIG5_CHARSET:
                        return 950;
                case GREEK_CHARSET:
                        return 1253;
                case TURKISH_CHARSET:
                        return 1254;
                case VIETNAMESE_CHARSET:
                        return 1258;
                case HEBREW_CHARSET:
                        return 1255;
                case ARABIC_CHARSET:
                        return 1256;
                case BALTIC_CHARSET:
                        return 1257;
                case RUSSIAN_CHARSET:
                        return 1251;
                case THAI_CHARSET:
                        return 874;
                case EASTEUROPE_CHARSET:
                        return 1250;
                case OEM_CHARSET:
                        return CP_OEMCP;
                default:
		{
                        CHARSETINFO csi;
                        DWORD n = charset;

                        /* FIXME: TranslateCharsetInfo does not work as good as it
                         * should, so let's use it only when all else fails */
                        if (!TranslateCharsetInfo(&n, &csi, TCI_SRCCHARSET))
                                ERR("unknown charset %d\n", charset);
			else
                                return csi.ciACP;
		}
	}
        return 0;
}


/* this shouldn't be called anywhere but from _RTFGetToken() */

static void _RTFGetToken2(RTF_Info *info)
{
	int	sign;
	int	c;

	/* initialize token vars */

	info->rtfClass = rtfUnknown;
	info->rtfParam = rtfNoParam;
	info->rtfTextBuf[info->rtfTextLen = 0] = '\0';

	/* get first character, which may be a pushback from previous token */

	if (info->pushedChar != EOF)
	{
		c = info->pushedChar;
		info->rtfTextBuf[info->rtfTextLen++] = c;
		info->rtfTextBuf[info->rtfTextLen] = '\0';
		info->pushedChar = EOF;
	}
	else if ((c = GetChar (info)) == EOF)
	{
		info->rtfClass = rtfEOF;
		return;
	}

	if (c == '{')
	{
		info->rtfClass = rtfGroup;
		info->rtfMajor = rtfBeginGroup;
		return;
	}
	if (c == '}')
	{
		info->rtfClass = rtfGroup;
		info->rtfMajor = rtfEndGroup;
		return;
	}
	if (c != '\\')
	{
		/*
		 * Two possibilities here:
		 * 1) ASCII 9, effectively like \tab control symbol
		 * 2) literal text char
		 */
		if (c == '\t')			/* ASCII 9 */
		{
			info->rtfClass = rtfControl;
			info->rtfMajor = rtfSpecialChar;
			info->rtfMinor = rtfTab;
		}
		else
		{
			info->rtfClass = rtfText;
			info->rtfMajor = c;
		}
		return;
	}
	if ((c = GetChar (info)) == EOF)
	{
		/* early eof, whoops (class is rtfUnknown) */
		return;
	}
	if (!isalpha (c))
	{
		/*
		 * Three possibilities here:
		 * 1) hex encoded text char, e.g., \'d5, \'d3
		 * 2) special escaped text char, e.g., \{, \}
		 * 3) control symbol, e.g., \_, \-, \|, \<10>
		 */
		if (c == '\'')				/* hex char */
		{
		int	c2;

			if ((c = GetChar (info)) != EOF && (c2 = GetChar (info)) != EOF
				&& isxdigit(c) && isxdigit(c2))
			{
				info->rtfClass = rtfText;
				info->rtfMajor = RTFCharToHex (c) * 16 + RTFCharToHex (c2);
				return;
			}
			/* early eof, whoops */
			info->rtfClass = rtfEOF;
			info->stream->editstream->dwError = -14;
			return;
		}

		/* escaped char */
		/*if (index (":{}\\", c) != NULL)*/ /* escaped char */
		if (c == ':' || c == '{' || c == '}' || c == '\\')
		{
			info->rtfClass = rtfText;
			info->rtfMajor = c;
			return;
		}

		/* control symbol */
		Lookup (info, info->rtfTextBuf);	/* sets class, major, minor */
		return;
	}
	/* control word */
	while (isalpha (c))
	{
		if ((c = GetChar (info)) == EOF)
			break;
	}

	/*
	 * At this point, the control word is all collected, so the
	 * major/minor numbers are determined before the parameter
	 * (if any) is scanned.  There will be one too many characters
	 * in the buffer, though, so fix up before and restore after
	 * looking up.
	 */

	if (c != EOF)
		info->rtfTextBuf[info->rtfTextLen-1] = '\0';
	Lookup (info, info->rtfTextBuf);	/* sets class, major, minor */
	if (c != EOF)
		info->rtfTextBuf[info->rtfTextLen-1] = c;

	/*
	 * Should be looking at first digit of parameter if there
	 * is one, unless it's negative.  In that case, next char
	 * is '-', so need to gobble next char, and remember sign.
	 */

	sign = 1;
	if (c == '-')
	{
		sign = -1;
		c = GetChar (info);
	}
	if (c != EOF && isdigit (c))
	{
		info->rtfParam = 0;
		while (isdigit (c))	/* gobble parameter */
		{
			info->rtfParam = info->rtfParam * 10 + c - '0';
			if ((c = GetChar (info)) == EOF)
				break;
		}
		info->rtfParam *= sign;
	}
	/*
	 * If control symbol delimiter was a blank, gobble it.
	 * Otherwise the character is first char of next token, so
	 * push it back for next call.  In either case, delete the
	 * delimiter from the token buffer.
	 */
	if (c != EOF)
	{
		if (c != ' ')
			info->pushedChar = c;
		info->rtfTextBuf[--info->rtfTextLen] = '\0';
	}
}


/*
 * Read the next character from the input.  This handles setting the
 * current line and position-within-line variables.  Those variables are
 * set correctly whether lines end with CR, LF, or CRLF (the last being
 * the tricky case).
 *
 * bumpLine indicates whether the line number should be incremented on
 * the *next* input character.
 */


static int GetChar(RTF_Info *info)
{
	int	c;
        BOOL    oldBumpLine;

	if ((c = _RTFGetChar(info)) != EOF)
	{
		info->rtfTextBuf[info->rtfTextLen++] = c;
		info->rtfTextBuf[info->rtfTextLen] = '\0';
	}
	if (info->prevChar == EOF)
                info->bumpLine = TRUE;
        oldBumpLine = info->bumpLine; /* TRUE if prev char was line ending */
        info->bumpLine = FALSE;
	if (c == '\r')
                info->bumpLine = TRUE;
	else if (c == '\n')
	{
                info->bumpLine = TRUE;
		if (info->prevChar == '\r')		/* oops, previous \r wasn't */
                        oldBumpLine = FALSE;	/* really a line ending */
	}
	++info->rtfLinePos;
	if (oldBumpLine)	/* were we supposed to increment the */
	{			/* line count on this char? */
		++info->rtfLineNum;
		info->rtfLinePos = 1;
	}
	info->prevChar = c;
	return (c);
}


/* ---------------------------------------------------------------------- */

/*
 * Special destination readers.  They gobble the destination so the
 * writer doesn't have to deal with them.  That's wrong for any
 * translator that wants to process any of these itself.  In that
 * case, these readers should be overridden by installing a different
 * destination callback.
 *
 * NOTE: The last token read by each of these reader will be the
 * destination's terminating '}', which will then be the current token.
 * That '}' token is passed to RTFRouteToken() - the writer has already
 * seen the '{' that began the destination group, and may have pushed a
 * state; it also needs to know at the end of the group that a state
 * should be popped.
 *
 * It's important that rtf.h and the control token lookup table list
 * as many symbols as possible, because these destination readers
 * unfortunately make strict assumptions about the input they expect,
 * and a token of class rtfUnknown will throw them off easily.
 */


/*
 * Read { \fonttbl ... } destination.  Old font tables don't have
 * braces around each table entry; try to adjust for that.
 */

static void ReadFontTbl(RTF_Info *info)
{
	RTFFont		*fp = NULL;
	char		buf[rtfBufSiz], *bp;
	int		old = -1;

	for (;;)
	{
		RTFGetToken (info);
		if (info->rtfClass == rtfEOF)
			break;
		if (RTFCheckCM (info, rtfGroup, rtfEndGroup))
			break;
		if (old < 0)		/* first entry - determine tbl type */
		{
			if (RTFCheckCMM (info, rtfControl, rtfCharAttr, rtfFontNum))
				old = 1;	/* no brace */
			else if (RTFCheckCM (info, rtfGroup, rtfBeginGroup))
				old = 0;	/* brace */
			else			/* can't tell! */
				ERR ("cannot determine format\n");
		}
		if (old == 0)		/* need to find "{" here */
		{
			if (!RTFCheckCM (info, rtfGroup, rtfBeginGroup))
				ERR ("missing \"{\"\n");
			RTFGetToken (info);	/* yes, skip to next token */
			if (info->rtfClass == rtfEOF)
				break;
		}
		fp = malloc (sizeof(*fp));
		if (fp == NULL) {
			ERR ("cannot allocate font entry\n");
			break;
		}

		fp->rtfNextFont = info->fontList;
		info->fontList = fp;

		fp->rtfFName = NULL;
		fp->rtfFAltName = NULL;
		fp->rtfFNum = -1;
		fp->rtfFFamily = FF_DONTCARE;
		fp->rtfFCharSet = DEFAULT_CHARSET; /* 1 */
		fp->rtfFPitch = DEFAULT_PITCH;
		fp->rtfFType = 0;
		fp->rtfFCodePage = CP_ACP;

		while (info->rtfClass != rtfEOF
		       && !RTFCheckCM (info, rtfText, ';')
		       && !RTFCheckCM (info, rtfGroup, rtfEndGroup))
		{
			if (info->rtfClass == rtfControl)
			{
				switch (info->rtfMajor)
				{
				default:
					/* ignore token but announce it */
					WARN ("unknown token \"%s\"\n",
						info->rtfTextBuf);
                                        break;
				case rtfFontFamily:
					fp->rtfFFamily = info->rtfMinor;
					break;
				case rtfCharAttr:
					switch (info->rtfMinor)
					{
					default:
						break;	/* ignore unknown? */
					case rtfFontNum:
						fp->rtfFNum = info->rtfParam;
						break;
					}
					break;
				case rtfFontAttr:
					switch (info->rtfMinor)
					{
					default:
						break;	/* ignore unknown? */
					case rtfFontCharSet:
						fp->rtfFCharSet = info->rtfParam;
                                                if (!fp->rtfFCodePage)
                                                        fp->rtfFCodePage = RTFCharSetToCodePage(info, info->rtfParam);
						break;
					case rtfFontPitch:
						fp->rtfFPitch = info->rtfParam;
						break;
					case rtfFontCodePage:
						fp->rtfFCodePage = info->rtfParam;
						break;
					case rtfFTypeNil:
					case rtfFTypeTrueType:
						fp->rtfFType = info->rtfParam;
						break;
					}
					break;
				}
			}
			else if (RTFCheckCM (info, rtfGroup, rtfBeginGroup))	/* dest */
			{
				RTFSkipGroup (info);	/* ignore for now */
			}
			else if (info->rtfClass == rtfText)	/* font name */
			{
				bp = buf;
                                while (info->rtfClass == rtfText
                                        && !RTFCheckCM (info, rtfText, ';'))
				{
					*bp++ = info->rtfMajor;
					RTFGetToken (info);
				}

				/* FIX: in some cases the <fontinfo> isn't finished with a semi-column */
				if(RTFCheckCM (info, rtfGroup, rtfEndGroup))
				{
					RTFUngetToken (info);
				}
				*bp = '\0';
				fp->rtfFName = strdup (buf);
				if (fp->rtfFName == NULL)
					ERR ("cannot allocate font name\n");
				/* already have next token; don't read one */
				/* at bottom of loop */
				continue;
			}
			else
			{
				/* ignore token but announce it */
				WARN ("unknown token \"%s\"\n", info->rtfTextBuf);
			}
			RTFGetToken (info);
			if (info->rtfClass == rtfEOF)
				break;
		}
		if (info->rtfClass == rtfEOF)
			break;
		if (old == 0)	/* need to see "}" here */
		{
			RTFGetToken (info);
			if (!RTFCheckCM (info, rtfGroup, rtfEndGroup))
				ERR ("missing \"}\"\n");
			if (info->rtfClass == rtfEOF)
				break;
		}

                /* Apply the real properties of the default font */
                if (fp->rtfFNum == info->defFont)
                {
                        if (info->ansiCodePage != CP_UTF8)
                                info->codePage = fp->rtfFCodePage;
                        TRACE("default font codepage %d\n", info->codePage);
                }
	}
	if (!fp || (fp->rtfFNum == -1))
		ERR("missing font number\n");
/*
 * Could check other pieces of structure here, too, I suppose.
 */
	RTFRouteToken (info);	/* feed "}" back to router */

        /* Set default font */
	info->rtfClass = rtfControl;
	info->rtfMajor = rtfCharAttr;
	info->rtfMinor = rtfFontNum;
	info->rtfParam = info->defFont;
	lstrcpyA(info->rtfTextBuf, "f");
        RTFUngetToken(info);
}


/*
 * The color table entries have color values of -1 if
 * the default color should be used for the entry (only
 * a semi-colon is given in the definition, no color values).
 * There will be a problem if a partial entry (1 or 2 but
 * not 3 color values) is given.  The possibility is ignored
 * here.
 */

static void ReadColorTbl(RTF_Info *info)
{
	RTFColor	*cp;
	int		cnum = 0;
        int group_level = 1;

	for (;;)
	{
		RTFGetToken (info);
		if (info->rtfClass == rtfEOF)
			break;
		if (RTFCheckCM (info, rtfGroup, rtfEndGroup))
                {
                        group_level--;
                        if (!group_level)
                                break;
                        continue;
                }
                else if (RTFCheckCM(info, rtfGroup, rtfBeginGroup))
                {
                        group_level++;
                        continue;
                }

		cp = malloc (sizeof(*cp));
		if (cp == NULL) {
			ERR ("cannot allocate color entry\n");
			break;
		}
		cp->rtfCNum = cnum++;
		cp->rtfNextColor = info->colorList;
		info->colorList = cp;
		if (!RTFCheckCM (info, rtfControl, rtfColorName))
			cp->rtfCRed = cp->rtfCGreen = cp->rtfCBlue = -1;
		else {
			cp->rtfCRed = cp->rtfCGreen = cp->rtfCBlue = 0;
			do {
				switch (info->rtfMinor)
				{
				case rtfRed:	cp->rtfCRed = info->rtfParam & 0xFF; break;
				case rtfGreen:	cp->rtfCGreen = info->rtfParam & 0xFF; break;
				case rtfBlue:	cp->rtfCBlue = info->rtfParam & 0xFF; break;
				}
				RTFGetToken (info);
			} while (RTFCheckCM (info, rtfControl, rtfColorName));
		}
		if (info->rtfClass == rtfEOF)
			break;
		if (!RTFCheckCM (info, rtfText, ';'))
			ERR ("malformed entry\n");
	}
	RTFRouteToken (info);	/* feed "}" back to router */
}


/*
 * The "Normal" style definition doesn't contain any style number,
 * all others do.  Normal style is given style rtfNormalStyleNum.
 */

static void ReadStyleSheet(RTF_Info *info)
{
	RTFStyle	*sp;
	RTFStyleElt	*sep, *sepLast;
	char		buf[rtfBufSiz], *bp;
	int             real_style;

	for (;;)
	{
		RTFGetToken (info);
		if (info->rtfClass == rtfEOF)
			break;
		if (RTFCheckCM (info, rtfGroup, rtfEndGroup))
			break;
		sp = malloc (sizeof(*sp));
		if (sp == NULL) {
			ERR ("cannot allocate stylesheet entry\n");
			break;
		}
		sp->rtfSName = NULL;
		sp->rtfSNum = -1;
		sp->rtfSType = rtfParStyle;
		sp->rtfSAdditive = 0;
		sp->rtfSBasedOn = rtfNoStyleNum;
		sp->rtfSNextPar = -1;
		sp->rtfSSEList = sepLast = NULL;
		sp->rtfNextStyle = info->styleList;
		sp->rtfExpanding = 0;
		info->styleList = sp;
		if (!RTFCheckCM (info, rtfGroup, rtfBeginGroup))
			ERR ("missing \"{\"\n");
		real_style = TRUE;
		for (;;)
		{
			RTFGetToken (info);
			if (info->rtfClass == rtfEOF
				|| RTFCheckCM (info, rtfText, ';'))
				break;
			if (info->rtfClass == rtfControl)
			{
				if (RTFCheckMM (info, rtfSpecialChar, rtfOptDest)) {
					RTFGetToken(info);
					ERR("skipping optional destination\n");
					RTFSkipGroup(info);
					info->rtfClass = rtfGroup;
					info->rtfMajor = rtfEndGroup;
					real_style = FALSE;
					break; /* ignore "\*" */
				}
				if (RTFCheckMM (info, rtfParAttr, rtfStyleNum))
				{
					sp->rtfSNum = info->rtfParam;
					sp->rtfSType = rtfParStyle;
					continue;
				}
				if (RTFCheckMM (info, rtfCharAttr, rtfCharStyleNum))
				{
					sp->rtfSNum = info->rtfParam;
					sp->rtfSType = rtfCharStyle;
					continue;
				}
				if (RTFCheckMM (info, rtfSectAttr, rtfSectStyleNum))
				{
					sp->rtfSNum = info->rtfParam;
					sp->rtfSType = rtfSectStyle;
					continue;
				}
				if (RTFCheckMM (info, rtfStyleAttr, rtfBasedOn))
				{
					sp->rtfSBasedOn = info->rtfParam;
					continue;
				}
				if (RTFCheckMM (info, rtfStyleAttr, rtfAdditive))
				{
					sp->rtfSAdditive = 1;
					continue;
				}
				if (RTFCheckMM (info, rtfStyleAttr, rtfNext))
				{
					sp->rtfSNextPar = info->rtfParam;
					continue;
				}
				sep = malloc (sizeof(*sep));
				if (sep == NULL)
                                {
					ERR ("cannot allocate style element\n");
					break;
				}
				sep->rtfSEClass = info->rtfClass;
				sep->rtfSEMajor = info->rtfMajor;
				sep->rtfSEMinor = info->rtfMinor;
				sep->rtfSEParam = info->rtfParam;
				sep->rtfSEText = strdup (info->rtfTextBuf);
				if (sep->rtfSEText == NULL)
					ERR ("cannot allocate style element text\n");
				if (sepLast == NULL)
					sp->rtfSSEList = sep;	/* first element */
				else				/* add to end */
					sepLast->rtfNextSE = sep;
				sep->rtfNextSE = NULL;
				sepLast = sep;
			}
			else if (RTFCheckCM (info, rtfGroup, rtfBeginGroup))
			{
				/*
				 * This passes over "{\*\keycode ... }, among
				 * other things. A temporary (perhaps) hack.
				 */
				ERR("skipping begin\n");
				RTFSkipGroup (info);
				continue;
			}
			else if (info->rtfClass == rtfText)	/* style name */
			{
				bp = buf;
				while (info->rtfClass == rtfText)
				{
					if (info->rtfMajor == ';')
					{
						/* put back for "for" loop */
						RTFUngetToken (info);
						break;
					}
					*bp++ = info->rtfMajor;
					RTFGetToken (info);
				}
				*bp = '\0';
				sp->rtfSName = strdup (buf);
				if (sp->rtfSName == NULL)
					ERR ("cannot allocate style name\n");
			}
			else		/* unrecognized */
			{
				/* ignore token but announce it */
				WARN ("unknown token \"%s\"\n", info->rtfTextBuf);
			}
		}
		if (real_style) {
			RTFGetToken (info);
			if (!RTFCheckCM (info, rtfGroup, rtfEndGroup))
				ERR ("missing \"}\"\n");
			/*
			 * Check over the style structure.  A name is a must.
			 * If no style number was specified, check whether it's the
			 * Normal style (in which case it's given style number
			 * rtfNormalStyleNum).  Note that some "normal" style names
			 * just begin with "Normal" and can have other stuff following,
			 * e.g., "Normal,Times 10 point".  Ugh.
			 *
			 * Some German RTF writers use "Standard" instead of "Normal".
			 */
			if (sp->rtfSName == NULL)
				ERR ("missing style name\n");
			if (sp->rtfSNum < 0)
			{
				if (strncmp (buf, "Normal", 6) != 0
					&& strncmp (buf, "Standard", 8) != 0)
					ERR ("missing style number\n");
				sp->rtfSNum = rtfNormalStyleNum;
			}
			if (sp->rtfSNextPar == -1)	/* if \snext not given, */
				sp->rtfSNextPar = sp->rtfSNum;	/* next is itself */
		}
		/* otherwise we're just dealing with fake end group from skipped group */
	}
	RTFRouteToken (info);	/* feed "}" back to router */
}


static void ReadInfoGroup(RTF_Info *info)
{
	RTFSkipGroup (info);
	RTFRouteToken (info);	/* feed "}" back to router */
}


static void ReadPictGroup(RTF_Info *info)
{
	RTFSkipGroup (info);
	RTFRouteToken (info);	/* feed "}" back to router */
}


static void ReadObjGroup(RTF_Info *info)
{
	RTFSkipGroup (info);
	RTFRouteToken (info);	/* feed "}" back to router */
}


/* ---------------------------------------------------------------------- */

/*
 * Routines to return pieces of stylesheet, or font or color tables.
 * References to style 0 are mapped onto the Normal style.
 */

RTFFont *RTFGetFont(const RTF_Info *info, int num)
{
	RTFFont	*f;

	if (num == -1)
		return (info->fontList);
	for (f = info->fontList; f != NULL; f = f->rtfNextFont)
	{
		if (f->rtfFNum == num)
			break;
	}
	return (f);		/* NULL if not found */
}


RTFColor *RTFGetColor(const RTF_Info *info, int num)
{
	RTFColor	*c;

	if (num == -1)
		return (info->colorList);
	for (c = info->colorList; c != NULL; c = c->rtfNextColor)
	{
		if (c->rtfCNum == num)
			break;
	}
	return (c);		/* NULL if not found */
}


/* ---------------------------------------------------------------------- */

/*
 * Control symbol lookup routines
 */


typedef struct RTFKey	RTFKey;

struct RTFKey
{
	int        rtfKMajor;	/* major number */
	int        rtfKMinor;	/* minor number */
	const char *rtfKStr;	/* symbol name */
	int        rtfKHash;	/* symbol name hash value */
};

/*
 * A minor number of -1 means the token has no minor number
 * (all valid minor numbers are >= 0).
 */

static RTFKey	rtfKey[] =
{
	/*
	 * Special characters
	 */

	{ rtfSpecialChar,	rtfIIntVersion,		"vern",		0 },
	{ rtfSpecialChar,	rtfICreateTime,		"creatim",	0 },
	{ rtfSpecialChar,	rtfIRevisionTime,	"revtim",	0 },
	{ rtfSpecialChar,	rtfIPrintTime,		"printim",	0 },
	{ rtfSpecialChar,	rtfIBackupTime,		"buptim",	0 },
	{ rtfSpecialChar,	rtfIEditTime,		"edmins",	0 },
	{ rtfSpecialChar,	rtfIYear,		"yr",		0 },
	{ rtfSpecialChar,	rtfIMonth,		"mo",		0 },
	{ rtfSpecialChar,	rtfIDay,		"dy",		0 },
	{ rtfSpecialChar,	rtfIHour,		"hr",		0 },
	{ rtfSpecialChar,	rtfIMinute,		"min",		0 },
	{ rtfSpecialChar,	rtfISecond,		"sec",		0 },
	{ rtfSpecialChar,	rtfINPages,		"nofpages",	0 },
	{ rtfSpecialChar,	rtfINWords,		"nofwords",	0 },
	{ rtfSpecialChar,	rtfINChars,		"nofchars",	0 },
	{ rtfSpecialChar,	rtfIIntID,		"id",		0 },

	{ rtfSpecialChar,	rtfCurHeadDate,		"chdate",	0 },
	{ rtfSpecialChar,	rtfCurHeadDateLong,	"chdpl",	0 },
	{ rtfSpecialChar,	rtfCurHeadDateAbbrev,	"chdpa",	0 },
	{ rtfSpecialChar,	rtfCurHeadTime,		"chtime",	0 },
	{ rtfSpecialChar,	rtfCurHeadPage,		"chpgn",	0 },
	{ rtfSpecialChar,	rtfSectNum,		"sectnum",	0 },
	{ rtfSpecialChar,	rtfCurFNote,		"chftn",	0 },
	{ rtfSpecialChar,	rtfCurAnnotRef,		"chatn",	0 },
	{ rtfSpecialChar,	rtfFNoteSep,		"chftnsep",	0 },
	{ rtfSpecialChar,	rtfFNoteCont,		"chftnsepc",	0 },
	{ rtfSpecialChar,	rtfCell,		"cell",		0 },
	{ rtfSpecialChar,	rtfRow,			"row",		0 },
	{ rtfSpecialChar,	rtfPar,			"par",		0 },
	/* newline and carriage return are synonyms for */
	/* \par when they are preceded by a \ character */
	{ rtfSpecialChar,	rtfPar,			"\n",		0 },
	{ rtfSpecialChar,	rtfPar,			"\r",		0 },
	{ rtfSpecialChar,	rtfSect,		"sect",		0 },
	{ rtfSpecialChar,	rtfPage,		"page",		0 },
	{ rtfSpecialChar,	rtfColumn,		"column",	0 },
	{ rtfSpecialChar,	rtfLine,		"line",		0 },
	{ rtfSpecialChar,	rtfSoftPage,		"softpage",	0 },
	{ rtfSpecialChar,	rtfSoftColumn,		"softcol",	0 },
	{ rtfSpecialChar,	rtfSoftLine,		"softline",	0 },
	{ rtfSpecialChar,	rtfSoftLineHt,		"softlheight",	0 },
	{ rtfSpecialChar,	rtfTab,			"tab",		0 },
	{ rtfSpecialChar,	rtfEmDash,		"emdash",	0 },
	{ rtfSpecialChar,	rtfEnDash,		"endash",	0 },
	{ rtfSpecialChar,	rtfEmSpace,		"emspace",	0 },
	{ rtfSpecialChar,	rtfEnSpace,		"enspace",	0 },
	{ rtfSpecialChar,	rtfBullet,		"bullet",	0 },
	{ rtfSpecialChar,	rtfLQuote,		"lquote",	0 },
	{ rtfSpecialChar,	rtfRQuote,		"rquote",	0 },
	{ rtfSpecialChar,	rtfLDblQuote,		"ldblquote",	0 },
	{ rtfSpecialChar,	rtfRDblQuote,		"rdblquote",	0 },
	{ rtfSpecialChar,	rtfFormula,		"|",		0 },
	{ rtfSpecialChar,	rtfNoBrkSpace,		"~",		0 },
	{ rtfSpecialChar,	rtfNoReqHyphen,		"-",		0 },
	{ rtfSpecialChar,	rtfNoBrkHyphen,		"_",		0 },
	{ rtfSpecialChar,	rtfOptDest,		"*",		0 },
	{ rtfSpecialChar,	rtfLTRMark,		"ltrmark",	0 },
	{ rtfSpecialChar,	rtfRTLMark,		"rtlmark",	0 },
	{ rtfSpecialChar,	rtfNoWidthJoiner,	"zwj",		0 },
	{ rtfSpecialChar,	rtfNoWidthNonJoiner,	"zwnj",		0 },
	/* is this valid? */
	{ rtfSpecialChar,	rtfCurHeadPict,		"chpict",	0 },
	{ rtfSpecialChar,	rtfUnicode,		"u",		0 },
	{ rtfSpecialChar,	rtfNestCell,		"nestcell",	0 },
	{ rtfSpecialChar,	rtfNestRow,		"nestrow",	0 },

	/*
	 * Character formatting attributes
	 */

	{ rtfCharAttr,	rtfPlain,		"plain",	0 },
	{ rtfCharAttr,	rtfBold,		"b",		0 },
	{ rtfCharAttr,	rtfAllCaps,		"caps",		0 },
	{ rtfCharAttr,	rtfDeleted,		"deleted",	0 },
	{ rtfCharAttr,	rtfSubScript,		"dn",		0 },
	{ rtfCharAttr,	rtfSubScrShrink,	"sub",		0 },
	{ rtfCharAttr,	rtfNoSuperSub,		"nosupersub",	0 },
	{ rtfCharAttr,	rtfExpand,		"expnd",	0 },
	{ rtfCharAttr,	rtfExpandTwips,		"expndtw",	0 },
	{ rtfCharAttr,	rtfKerning,		"kerning",	0 },
	{ rtfCharAttr,	rtfFontNum,		"f",		0 },
	{ rtfCharAttr,	rtfFontSize,		"fs",		0 },
	{ rtfCharAttr,	rtfItalic,		"i",		0 },
	{ rtfCharAttr,	rtfOutline,		"outl",		0 },
	{ rtfCharAttr,	rtfRevised,		"revised",	0 },
	{ rtfCharAttr,	rtfRevAuthor,		"revauth",	0 },
	{ rtfCharAttr,	rtfRevDTTM,		"revdttm",	0 },
	{ rtfCharAttr,	rtfSmallCaps,		"scaps",	0 },
	{ rtfCharAttr,	rtfShadow,		"shad",		0 },
	{ rtfCharAttr,	rtfStrikeThru,		"strike",	0 },
	{ rtfCharAttr,	rtfUnderline,		"ul",		0 },
	{ rtfCharAttr,	rtfDotUnderline,	"uld",		0 },
	{ rtfCharAttr,	rtfDbUnderline,		"uldb",		0 },
	{ rtfCharAttr,	rtfNoUnderline,		"ulnone",	0 },
	{ rtfCharAttr,	rtfWordUnderline,	"ulw",		0 },
	{ rtfCharAttr,	rtfSuperScript,		"up",		0 },
	{ rtfCharAttr,	rtfSuperScrShrink,	"super",	0 },
	{ rtfCharAttr,	rtfInvisible,		"v",		0 },
	{ rtfCharAttr,	rtfForeColor,		"cf",		0 },
	{ rtfCharAttr,	rtfBackColor,		"highlight",	0 },
	{ rtfCharAttr,	rtfRTLChar,		"rtlch",	0 },
	{ rtfCharAttr,	rtfLTRChar,		"ltrch",	0 },
	{ rtfCharAttr,	rtfCharStyleNum,	"cs",		0 },
	{ rtfCharAttr,	rtfCharCharSet,		"cchs",		0 },
	{ rtfCharAttr,	rtfLanguage,		"lang",		0 },
	/* this has disappeared from spec 1.2 */
	{ rtfCharAttr,	rtfGray,		"gray",		0 },
        { rtfCharAttr,	rtfUnicodeLength,	"uc",		0 },

	/*
	 * Paragraph formatting attributes
	 */

	{ rtfParAttr,	rtfParDef,		"pard",		0 },
	{ rtfParAttr,	rtfStyleNum,		"s",		0 },
	{ rtfParAttr,	rtfHyphenate,		"hyphpar",	0 },
	{ rtfParAttr,	rtfInTable,		"intbl",	0 },
	{ rtfParAttr,	rtfKeep,		"keep",		0 },
	{ rtfParAttr,	rtfNoWidowControl,	"nowidctlpar",	0 },
	{ rtfParAttr,	rtfKeepNext,		"keepn",	0 },
	{ rtfParAttr,	rtfOutlineLevel,	"level",	0 },
	{ rtfParAttr,	rtfNoLineNum,		"noline",	0 },
	{ rtfParAttr,	rtfPBBefore,		"pagebb",	0 },
	{ rtfParAttr,	rtfSideBySide,		"sbys",		0 },
	{ rtfParAttr,	rtfQuadLeft,		"ql",		0 },
	{ rtfParAttr,	rtfQuadRight,		"qr",		0 },
	{ rtfParAttr,	rtfQuadJust,		"qj",		0 },
	{ rtfParAttr,	rtfQuadCenter,		"qc",		0 },
	{ rtfParAttr,	rtfFirstIndent,		"fi",		0 },
	{ rtfParAttr,	rtfLeftIndent,		"li",		0 },
	{ rtfParAttr,	rtfRightIndent,		"ri",		0 },
	{ rtfParAttr,	rtfSpaceBefore,		"sb",		0 },
	{ rtfParAttr,	rtfSpaceAfter,		"sa",		0 },
	{ rtfParAttr,	rtfSpaceBetween,	"sl",		0 },
	{ rtfParAttr,	rtfSpaceMultiply,	"slmult",	0 },

	{ rtfParAttr,	rtfSubDocument,		"subdocument",	0 },

	{ rtfParAttr,	rtfRTLPar,		"rtlpar",	0 },
	{ rtfParAttr,	rtfLTRPar,		"ltrpar",	0 },

	{ rtfParAttr,	rtfTabPos,		"tx",		0 },
	/*
	 * FrameMaker writes \tql (to mean left-justified tab, apparently)
	 * although it's not in the spec.  It's also redundant, since lj
	 * tabs are the default.
	 */
	{ rtfParAttr,	rtfTabLeft,		"tql",		0 },
	{ rtfParAttr,	rtfTabRight,		"tqr",		0 },
	{ rtfParAttr,	rtfTabCenter,		"tqc",		0 },
	{ rtfParAttr,	rtfTabDecimal,		"tqdec",	0 },
	{ rtfParAttr,	rtfTabBar,		"tb",		0 },
	{ rtfParAttr,	rtfLeaderDot,		"tldot",	0 },
	{ rtfParAttr,	rtfLeaderHyphen,	"tlhyph",	0 },
	{ rtfParAttr,	rtfLeaderUnder,		"tlul",		0 },
	{ rtfParAttr,	rtfLeaderThick,		"tlth",		0 },
	{ rtfParAttr,	rtfLeaderEqual,		"tleq",		0 },

	{ rtfParAttr,	rtfParLevel,		"pnlvl",	0 },
	{ rtfParAttr,	rtfParBullet,		"pnlvlblt",	0 },
	{ rtfParAttr,	rtfParSimple,		"pnlvlbody",	0 },
	{ rtfParAttr,	rtfParNumCont,		"pnlvlcont",	0 },
	{ rtfParAttr,	rtfParNumOnce,		"pnnumonce",	0 },
	{ rtfParAttr,	rtfParNumAcross,	"pnacross",	0 },
	{ rtfParAttr,	rtfParHangIndent,	"pnhang",	0 },
	{ rtfParAttr,	rtfParNumRestart,	"pnrestart",	0 },
	{ rtfParAttr,	rtfParNumCardinal,	"pncard",	0 },
	{ rtfParAttr,	rtfParNumDecimal,	"pndec",	0 },
	{ rtfParAttr,	rtfParNumULetter,	"pnucltr",	0 },
	{ rtfParAttr,	rtfParNumURoman,	"pnucrm",	0 },
	{ rtfParAttr,	rtfParNumLLetter,	"pnlcltr",	0 },
	{ rtfParAttr,	rtfParNumLRoman,	"pnlcrm",	0 },
	{ rtfParAttr,	rtfParNumOrdinal,	"pnord",	0 },
	{ rtfParAttr,	rtfParNumOrdinalText,	"pnordt",	0 },
	{ rtfParAttr,	rtfParNumBold,		"pnb",		0 },
	{ rtfParAttr,	rtfParNumItalic,	"pni",		0 },
	{ rtfParAttr,	rtfParNumAllCaps,	"pncaps",	0 },
	{ rtfParAttr,	rtfParNumSmallCaps,	"pnscaps",	0 },
	{ rtfParAttr,	rtfParNumUnder,		"pnul",		0 },
	{ rtfParAttr,	rtfParNumDotUnder,	"pnuld",	0 },
	{ rtfParAttr,	rtfParNumDbUnder,	"pnuldb",	0 },
	{ rtfParAttr,	rtfParNumNoUnder,	"pnulnone",	0 },
	{ rtfParAttr,	rtfParNumWordUnder,	"pnulw",	0 },
	{ rtfParAttr,	rtfParNumStrikethru,	"pnstrike",	0 },
	{ rtfParAttr,	rtfParNumForeColor,	"pncf",		0 },
	{ rtfParAttr,	rtfParNumFont,		"pnf",		0 },
	{ rtfParAttr,	rtfParNumFontSize,	"pnfs",		0 },
	{ rtfParAttr,	rtfParNumIndent,	"pnindent",	0 },
	{ rtfParAttr,	rtfParNumSpacing,	"pnsp",		0 },
	{ rtfParAttr,	rtfParNumInclPrev,	"pnprev",	0 },
	{ rtfParAttr,	rtfParNumCenter,	"pnqc",		0 },
	{ rtfParAttr,	rtfParNumLeft,		"pnql",		0 },
	{ rtfParAttr,	rtfParNumRight,		"pnqr",		0 },
	{ rtfParAttr,	rtfParNumStartAt,	"pnstart",	0 },

	{ rtfParAttr,	rtfBorderTop,		"brdrt",	0 },
	{ rtfParAttr,	rtfBorderBottom,	"brdrb",	0 },
	{ rtfParAttr,	rtfBorderLeft,		"brdrl",	0 },
	{ rtfParAttr,	rtfBorderRight,		"brdrr",	0 },
	{ rtfParAttr,	rtfBorderBetween,	"brdrbtw",	0 },
	{ rtfParAttr,	rtfBorderBar,		"brdrbar",	0 },
	{ rtfParAttr,	rtfBorderBox,		"box",		0 },
	{ rtfParAttr,	rtfBorderSingle,	"brdrs",	0 },
	{ rtfParAttr,	rtfBorderThick,		"brdrth",	0 },
	{ rtfParAttr,	rtfBorderShadow,	"brdrsh",	0 },
	{ rtfParAttr,	rtfBorderDouble,	"brdrdb",	0 },
	{ rtfParAttr,	rtfBorderDot,		"brdrdot",	0 },
	{ rtfParAttr,	rtfBorderDot,		"brdrdash",	0 },
	{ rtfParAttr,	rtfBorderHair,		"brdrhair",	0 },
	{ rtfParAttr,	rtfBorderWidth,		"brdrw",	0 },
	{ rtfParAttr,	rtfBorderColor,		"brdrcf",	0 },
	{ rtfParAttr,	rtfBorderSpace,		"brsp",		0 },

	{ rtfParAttr,	rtfShading,		"shading",	0 },
	{ rtfParAttr,	rtfBgPatH,		"bghoriz",	0 },
	{ rtfParAttr,	rtfBgPatV,		"bgvert",	0 },
	{ rtfParAttr,	rtfFwdDiagBgPat,	"bgfdiag",	0 },
	{ rtfParAttr,	rtfBwdDiagBgPat,	"bgbdiag",	0 },
	{ rtfParAttr,	rtfHatchBgPat,		"bgcross",	0 },
	{ rtfParAttr,	rtfDiagHatchBgPat,	"bgdcross",	0 },
	{ rtfParAttr,	rtfDarkBgPatH,		"bgdkhoriz",	0 },
	{ rtfParAttr,	rtfDarkBgPatV,		"bgdkvert",	0 },
	{ rtfParAttr,	rtfFwdDarkBgPat,	"bgdkfdiag",	0 },
	{ rtfParAttr,	rtfBwdDarkBgPat,	"bgdkbdiag",	0 },
	{ rtfParAttr,	rtfDarkHatchBgPat,	"bgdkcross",	0 },
	{ rtfParAttr,	rtfDarkDiagHatchBgPat,	"bgdkdcross",	0 },
	{ rtfParAttr,	rtfBgPatLineColor,	"cfpat",	0 },
	{ rtfParAttr,	rtfBgPatColor,		"cbpat",	0 },
	{ rtfParAttr,	rtfNestLevel,		"itap",		0 },

	/*
	 * Section formatting attributes
	 */

	{ rtfSectAttr,	rtfSectDef,		"sectd",	0 },
	{ rtfSectAttr,	rtfENoteHere,		"endnhere",	0 },
	{ rtfSectAttr,	rtfPrtBinFirst,		"binfsxn",	0 },
	{ rtfSectAttr,	rtfPrtBin,		"binsxn",	0 },
	{ rtfSectAttr,	rtfSectStyleNum,	"ds",		0 },

	{ rtfSectAttr,	rtfNoBreak,		"sbknone",	0 },
	{ rtfSectAttr,	rtfColBreak,		"sbkcol",	0 },
	{ rtfSectAttr,	rtfPageBreak,		"sbkpage",	0 },
	{ rtfSectAttr,	rtfEvenBreak,		"sbkeven",	0 },
	{ rtfSectAttr,	rtfOddBreak,		"sbkodd",	0 },

	{ rtfSectAttr,	rtfColumns,		"cols",		0 },
	{ rtfSectAttr,	rtfColumnSpace,		"colsx",	0 },
	{ rtfSectAttr,	rtfColumnNumber,	"colno",	0 },
	{ rtfSectAttr,	rtfColumnSpRight,	"colsr",	0 },
	{ rtfSectAttr,	rtfColumnWidth,		"colw",		0 },
	{ rtfSectAttr,	rtfColumnLine,		"linebetcol",	0 },

	{ rtfSectAttr,	rtfLineModulus,		"linemod",	0 },
	{ rtfSectAttr,	rtfLineDist,		"linex",	0 },
	{ rtfSectAttr,	rtfLineStarts,		"linestarts",	0 },
	{ rtfSectAttr,	rtfLineRestart,		"linerestart",	0 },
	{ rtfSectAttr,	rtfLineRestartPg,	"lineppage",	0 },
	{ rtfSectAttr,	rtfLineCont,		"linecont",	0 },

	{ rtfSectAttr,	rtfSectPageWid,		"pgwsxn",	0 },
	{ rtfSectAttr,	rtfSectPageHt,		"pghsxn",	0 },
	{ rtfSectAttr,	rtfSectMarginLeft,	"marglsxn",	0 },
	{ rtfSectAttr,	rtfSectMarginRight,	"margrsxn",	0 },
	{ rtfSectAttr,	rtfSectMarginTop,	"margtsxn",	0 },
	{ rtfSectAttr,	rtfSectMarginBottom,	"margbsxn",	0 },
	{ rtfSectAttr,	rtfSectMarginGutter,	"guttersxn",	0 },
	{ rtfSectAttr,	rtfSectLandscape,	"lndscpsxn",	0 },
	{ rtfSectAttr,	rtfTitleSpecial,	"titlepg",	0 },
	{ rtfSectAttr,	rtfHeaderY,		"headery",	0 },
	{ rtfSectAttr,	rtfFooterY,		"footery",	0 },

	{ rtfSectAttr,	rtfPageStarts,		"pgnstarts",	0 },
	{ rtfSectAttr,	rtfPageCont,		"pgncont",	0 },
	{ rtfSectAttr,	rtfPageRestart,		"pgnrestart",	0 },
	{ rtfSectAttr,	rtfPageNumRight,	"pgnx",		0 },
	{ rtfSectAttr,	rtfPageNumTop,		"pgny",		0 },
	{ rtfSectAttr,	rtfPageDecimal,		"pgndec",	0 },
	{ rtfSectAttr,	rtfPageURoman,		"pgnucrm",	0 },
	{ rtfSectAttr,	rtfPageLRoman,		"pgnlcrm",	0 },
	{ rtfSectAttr,	rtfPageULetter,		"pgnucltr",	0 },
	{ rtfSectAttr,	rtfPageLLetter,		"pgnlcltr",	0 },
	{ rtfSectAttr,	rtfPageNumHyphSep,	"pgnhnsh",	0 },
	{ rtfSectAttr,	rtfPageNumSpaceSep,	"pgnhnsp",	0 },
	{ rtfSectAttr,	rtfPageNumColonSep,	"pgnhnsc",	0 },
	{ rtfSectAttr,	rtfPageNumEmdashSep,	"pgnhnsm",	0 },
	{ rtfSectAttr,	rtfPageNumEndashSep,	"pgnhnsn",	0 },

	{ rtfSectAttr,	rtfTopVAlign,		"vertalt",	0 },
	/* misspelled as "vertal" in specification 1.0 */
	{ rtfSectAttr,	rtfBottomVAlign,	"vertalb",	0 },
	{ rtfSectAttr,	rtfCenterVAlign,	"vertalc",	0 },
	{ rtfSectAttr,	rtfJustVAlign,		"vertalj",	0 },

	{ rtfSectAttr,	rtfRTLSect,		"rtlsect",	0 },
	{ rtfSectAttr,	rtfLTRSect,		"ltrsect",	0 },

	/* I've seen these in an old spec, but not in real files... */
	/*rtfSectAttr,	rtfNoBreak,		"nobreak",	0,*/
	/*rtfSectAttr,	rtfColBreak,		"colbreak",	0,*/
	/*rtfSectAttr,	rtfPageBreak,		"pagebreak",	0,*/
	/*rtfSectAttr,	rtfEvenBreak,		"evenbreak",	0,*/
	/*rtfSectAttr,	rtfOddBreak,		"oddbreak",	0,*/

	/*
	 * Document formatting attributes
	 */

	{ rtfDocAttr,	rtfDefTab,		"deftab",	0 },
	{ rtfDocAttr,	rtfHyphHotZone,		"hyphhotz",	0 },
	{ rtfDocAttr,	rtfHyphConsecLines,	"hyphconsec",	0 },
	{ rtfDocAttr,	rtfHyphCaps,		"hyphcaps",	0 },
	{ rtfDocAttr,	rtfHyphAuto,		"hyphauto",	0 },
	{ rtfDocAttr,	rtfLineStart,		"linestart",	0 },
	{ rtfDocAttr,	rtfFracWidth,		"fracwidth",	0 },
	/* \makeback was given in old version of spec, it's now */
	/* listed as \makebackup */
	{ rtfDocAttr,	rtfMakeBackup,		"makeback",	0 },
	{ rtfDocAttr,	rtfMakeBackup,		"makebackup",	0 },
	{ rtfDocAttr,	rtfRTFDefault,		"defformat",	0 },
	{ rtfDocAttr,	rtfPSOverlay,		"psover",	0 },
	{ rtfDocAttr,	rtfDocTemplate,		"doctemp",	0 },
	{ rtfDocAttr,	rtfDefLanguage,		"deflang",	0 },

	{ rtfDocAttr,	rtfFENoteType,		"fet",		0 },
	{ rtfDocAttr,	rtfFNoteEndSect,	"endnotes",	0 },
	{ rtfDocAttr,	rtfFNoteEndDoc,		"enddoc",	0 },
	{ rtfDocAttr,	rtfFNoteText,		"ftntj",	0 },
	{ rtfDocAttr,	rtfFNoteBottom,		"ftnbj",	0 },
	{ rtfDocAttr,	rtfENoteEndSect,	"aendnotes",	0 },
	{ rtfDocAttr,	rtfENoteEndDoc,		"aenddoc",	0 },
	{ rtfDocAttr,	rtfENoteText,		"aftntj",	0 },
	{ rtfDocAttr,	rtfENoteBottom,		"aftnbj",	0 },
	{ rtfDocAttr,	rtfFNoteStart,		"ftnstart",	0 },
	{ rtfDocAttr,	rtfENoteStart,		"aftnstart",	0 },
	{ rtfDocAttr,	rtfFNoteRestartPage,	"ftnrstpg",	0 },
	{ rtfDocAttr,	rtfFNoteRestart,	"ftnrestart",	0 },
	{ rtfDocAttr,	rtfFNoteRestartCont,	"ftnrstcont",	0 },
	{ rtfDocAttr,	rtfENoteRestart,	"aftnrestart",	0 },
	{ rtfDocAttr,	rtfENoteRestartCont,	"aftnrstcont",	0 },
	{ rtfDocAttr,	rtfFNoteNumArabic,	"ftnnar",	0 },
	{ rtfDocAttr,	rtfFNoteNumLLetter,	"ftnnalc",	0 },
	{ rtfDocAttr,	rtfFNoteNumULetter,	"ftnnauc",	0 },
	{ rtfDocAttr,	rtfFNoteNumLRoman,	"ftnnrlc",	0 },
	{ rtfDocAttr,	rtfFNoteNumURoman,	"ftnnruc",	0 },
	{ rtfDocAttr,	rtfFNoteNumChicago,	"ftnnchi",	0 },
	{ rtfDocAttr,	rtfENoteNumArabic,	"aftnnar",	0 },
	{ rtfDocAttr,	rtfENoteNumLLetter,	"aftnnalc",	0 },
	{ rtfDocAttr,	rtfENoteNumULetter,	"aftnnauc",	0 },
	{ rtfDocAttr,	rtfENoteNumLRoman,	"aftnnrlc",	0 },
	{ rtfDocAttr,	rtfENoteNumURoman,	"aftnnruc",	0 },
	{ rtfDocAttr,	rtfENoteNumChicago,	"aftnnchi",	0 },

	{ rtfDocAttr,	rtfPaperWidth,		"paperw",	0 },
	{ rtfDocAttr,	rtfPaperHeight,		"paperh",	0 },
	{ rtfDocAttr,	rtfPaperSize,		"psz",		0 },
	{ rtfDocAttr,	rtfLeftMargin,		"margl",	0 },
	{ rtfDocAttr,	rtfRightMargin,		"margr",	0 },
	{ rtfDocAttr,	rtfTopMargin,		"margt",	0 },
	{ rtfDocAttr,	rtfBottomMargin,	"margb",	0 },
	{ rtfDocAttr,	rtfFacingPage,		"facingp",	0 },
	{ rtfDocAttr,	rtfGutterWid,		"gutter",	0 },
	{ rtfDocAttr,	rtfMirrorMargin,	"margmirror",	0 },
	{ rtfDocAttr,	rtfLandscape,		"landscape",	0 },
	{ rtfDocAttr,	rtfPageStart,		"pgnstart",	0 },
	{ rtfDocAttr,	rtfWidowCtrl,		"widowctrl",	0 },

	{ rtfDocAttr,	rtfLinkStyles,		"linkstyles",	0 },

	{ rtfDocAttr,	rtfNoAutoTabIndent,	"notabind",	0 },
	{ rtfDocAttr,	rtfWrapSpaces,		"wraptrsp",	0 },
	{ rtfDocAttr,	rtfPrintColorsBlack,	"prcolbl",	0 },
	{ rtfDocAttr,	rtfNoExtraSpaceRL,	"noextrasprl",	0 },
	{ rtfDocAttr,	rtfNoColumnBalance,	"nocolbal",	0 },
	{ rtfDocAttr,	rtfCvtMailMergeQuote,	"cvmme",	0 },
	{ rtfDocAttr,	rtfSuppressTopSpace,	"sprstsp",	0 },
	{ rtfDocAttr,	rtfSuppressPreParSpace,	"sprsspbf",	0 },
	{ rtfDocAttr,	rtfCombineTblBorders,	"otblrul",	0 },
	{ rtfDocAttr,	rtfTranspMetafiles,	"transmf",	0 },
	{ rtfDocAttr,	rtfSwapBorders,		"swpbdr",	0 },
	{ rtfDocAttr,	rtfShowHardBreaks,	"brkfrm",	0 },

	{ rtfDocAttr,	rtfFormProtected,	"formprot",	0 },
	{ rtfDocAttr,	rtfAllProtected,	"allprot",	0 },
	{ rtfDocAttr,	rtfFormShading,		"formshade",	0 },
	{ rtfDocAttr,	rtfFormDisplay,		"formdisp",	0 },
	{ rtfDocAttr,	rtfPrintData,		"printdata",	0 },

	{ rtfDocAttr,	rtfRevProtected,	"revprot",	0 },
	{ rtfDocAttr,	rtfRevisions,		"revisions",	0 },
	{ rtfDocAttr,	rtfRevDisplay,		"revprop",	0 },
	{ rtfDocAttr,	rtfRevBar,		"revbar",	0 },

	{ rtfDocAttr,	rtfAnnotProtected,	"annotprot",	0 },

	{ rtfDocAttr,	rtfRTLDoc,		"rtldoc",	0 },
	{ rtfDocAttr,	rtfLTRDoc,		"ltrdoc",	0 },

        { rtfDocAttr,	rtfAnsiCodePage,	"ansicpg",	0 },
        { rtfDocAttr,	rtfUTF8RTF,		"urtf",		0 },

	/*
	 * Style attributes
	 */

	{ rtfStyleAttr,	rtfAdditive,		"additive",	0 },
	{ rtfStyleAttr,	rtfBasedOn,		"sbasedon",	0 },
	{ rtfStyleAttr,	rtfNext,		"snext",	0 },

	/*
	 * Picture attributes
	 */

	{ rtfPictAttr,	rtfMacQD,		"macpict",	0 },
	{ rtfPictAttr,	rtfPMMetafile,		"pmmetafile",	0 },
	{ rtfPictAttr,	rtfWinMetafile,		"wmetafile",	0 },
	{ rtfPictAttr,	rtfDevIndBitmap,	"dibitmap",	0 },
	{ rtfPictAttr,	rtfWinBitmap,		"wbitmap",	0 },
	{ rtfPictAttr,	rtfEmfBlip,		"emfblip",	0 },
	{ rtfPictAttr,	rtfPixelBits,		"wbmbitspixel",	0 },
	{ rtfPictAttr,	rtfBitmapPlanes,	"wbmplanes",	0 },
	{ rtfPictAttr,	rtfBitmapWid,		"wbmwidthbytes", 0 },

	{ rtfPictAttr,	rtfPicWid,		"picw",		0 },
	{ rtfPictAttr,	rtfPicHt,		"pich",		0 },
	{ rtfPictAttr,	rtfPicGoalWid,		"picwgoal",	0 },
	{ rtfPictAttr,	rtfPicGoalHt,		"pichgoal",	0 },
	/* these two aren't in the spec, but some writers emit them */
	{ rtfPictAttr,	rtfPicGoalWid,		"picwGoal",	0 },
	{ rtfPictAttr,	rtfPicGoalHt,		"pichGoal",	0 },
	{ rtfPictAttr,	rtfPicScaleX,		"picscalex",	0 },
	{ rtfPictAttr,	rtfPicScaleY,		"picscaley",	0 },
	{ rtfPictAttr,	rtfPicScaled,		"picscaled",	0 },
	{ rtfPictAttr,	rtfPicCropTop,		"piccropt",	0 },
	{ rtfPictAttr,	rtfPicCropBottom,	"piccropb",	0 },
	{ rtfPictAttr,	rtfPicCropLeft,		"piccropl",	0 },
	{ rtfPictAttr,	rtfPicCropRight,	"piccropr",	0 },

	{ rtfPictAttr,	rtfPicMFHasBitmap,	"picbmp",	0 },
	{ rtfPictAttr,	rtfPicMFBitsPerPixel,	"picbpp",	0 },

	{ rtfPictAttr,	rtfPicBinary,		"bin",		0 },

	/*
	 * NeXT graphic attributes
	 */

	{ rtfNeXTGrAttr,	rtfNeXTGWidth,		"width",	0 },
	{ rtfNeXTGrAttr,	rtfNeXTGHeight,		"height",	0 },

	/*
	 * Destinations
	 */

	{ rtfDestination,	rtfFontTbl,		"fonttbl",	0 },
	{ rtfDestination,	rtfFontAltName,		"falt",		0 },
	{ rtfDestination,	rtfEmbeddedFont,	"fonteb",	0 },
	{ rtfDestination,	rtfFontFile,		"fontfile",	0 },
	{ rtfDestination,	rtfFileTbl,		"filetbl",	0 },
	{ rtfDestination,	rtfFileInfo,		"file",		0 },
	{ rtfDestination,	rtfColorTbl,		"colortbl",	0 },
	{ rtfDestination,	rtfStyleSheet,		"stylesheet",	0 },
	{ rtfDestination,	rtfKeyCode,		"keycode",	0 },
	{ rtfDestination,	rtfRevisionTbl,		"revtbl",	0 },
	{ rtfDestination,	rtfGenerator,		"generator",	0 },
	{ rtfDestination,	rtfInfo,		"info",		0 },
	{ rtfDestination,	rtfITitle,		"title",	0 },
	{ rtfDestination,	rtfISubject,		"subject",	0 },
	{ rtfDestination,	rtfIAuthor,		"author",	0 },
	{ rtfDestination,	rtfIOperator,		"operator",	0 },
	{ rtfDestination,	rtfIKeywords,		"keywords",	0 },
	{ rtfDestination,	rtfIComment,		"comment",	0 },
	{ rtfDestination,	rtfIVersion,		"version",	0 },
	{ rtfDestination,	rtfIDoccomm,		"doccomm",	0 },
	/* \verscomm may not exist -- was seen in earlier spec version */
	{ rtfDestination,	rtfIVerscomm,		"verscomm",	0 },
	{ rtfDestination,	rtfNextFile,		"nextfile",	0 },
	{ rtfDestination,	rtfTemplate,		"template",	0 },
	{ rtfDestination,	rtfFNSep,		"ftnsep",	0 },
	{ rtfDestination,	rtfFNContSep,		"ftnsepc",	0 },
	{ rtfDestination,	rtfFNContNotice,	"ftncn",	0 },
	{ rtfDestination,	rtfENSep,		"aftnsep",	0 },
	{ rtfDestination,	rtfENContSep,		"aftnsepc",	0 },
	{ rtfDestination,	rtfENContNotice,	"aftncn",	0 },
	{ rtfDestination,	rtfPageNumLevel,	"pgnhn",	0 },
	{ rtfDestination,	rtfParNumLevelStyle,	"pnseclvl",	0 },
	{ rtfDestination,	rtfHeader,		"header",	0 },
	{ rtfDestination,	rtfFooter,		"footer",	0 },
	{ rtfDestination,	rtfHeaderLeft,		"headerl",	0 },
	{ rtfDestination,	rtfHeaderRight,		"headerr",	0 },
	{ rtfDestination,	rtfHeaderFirst,		"headerf",	0 },
	{ rtfDestination,	rtfFooterLeft,		"footerl",	0 },
	{ rtfDestination,	rtfFooterRight,		"footerr",	0 },
	{ rtfDestination,	rtfFooterFirst,		"footerf",	0 },
	{ rtfDestination,	rtfParNumText,		"pntext",	0 },
	{ rtfDestination,	rtfParNumbering,	"pn",		0 },
	{ rtfDestination,	rtfParNumTextAfter,	"pntxta",	0 },
	{ rtfDestination,	rtfParNumTextBefore,	"pntxtb",	0 },
	{ rtfDestination,	rtfBookmarkStart,	"bkmkstart",	0 },
	{ rtfDestination,	rtfBookmarkEnd,		"bkmkend",	0 },
	{ rtfDestination,	rtfPict,		"pict",		0 },
	{ rtfDestination,	rtfObject,		"object",	0 },
	{ rtfDestination,	rtfObjClass,		"objclass",	0 },
	{ rtfDestination,	rtfObjName,		"objname",	0 },
	{ rtfObjAttr,	rtfObjTime,		"objtime",	0 },
	{ rtfDestination,	rtfObjData,		"objdata",	0 },
	{ rtfDestination,	rtfObjAlias,		"objalias",	0 },
	{ rtfDestination,	rtfObjSection,		"objsect",	0 },
	/* objitem and objtopic aren't documented in the spec! */
	{ rtfDestination,	rtfObjItem,		"objitem",	0 },
	{ rtfDestination,	rtfObjTopic,		"objtopic",	0 },
	{ rtfDestination,	rtfObjResult,		"result",	0 },
	{ rtfDestination,	rtfDrawObject,		"do",		0 },
	{ rtfDestination,	rtfFootnote,		"footnote",	0 },
	{ rtfDestination,	rtfAnnotRefStart,	"atrfstart",	0 },
	{ rtfDestination,	rtfAnnotRefEnd,		"atrfend",	0 },
	{ rtfDestination,	rtfAnnotID,		"atnid",	0 },
	{ rtfDestination,	rtfAnnotAuthor,		"atnauthor",	0 },
	{ rtfDestination,	rtfAnnotation,		"annotation",	0 },
	{ rtfDestination,	rtfAnnotRef,		"atnref",	0 },
	{ rtfDestination,	rtfAnnotTime,		"atntime",	0 },
	{ rtfDestination,	rtfAnnotIcon,		"atnicn",	0 },
	{ rtfDestination,	rtfField,		"field",	0 },
	{ rtfDestination,	rtfFieldInst,		"fldinst",	0 },
	{ rtfDestination,	rtfFieldResult,		"fldrslt",	0 },
	{ rtfDestination,	rtfDataField,		"datafield",	0 },
	{ rtfDestination,	rtfIndex,		"xe",		0 },
	{ rtfDestination,	rtfIndexText,		"txe",		0 },
	{ rtfDestination,	rtfIndexRange,		"rxe",		0 },
	{ rtfDestination,	rtfTOC,			"tc",		0 },
	{ rtfDestination,	rtfNeXTGraphic,		"NeXTGraphic",	0 },
	{ rtfDestination,	rtfNestTableProps,	"nesttableprops", 0 },
	{ rtfDestination,	rtfNoNestTables,	"nonesttables",	0 },
        { rtfDestination,       rtfShpPict,             "shppict",      0 },
        { rtfDestination,       rtfNonShpPict,          "nonshppict",   0 },

	/*
	 * Font families
	 */

	{ rtfFontFamily,	rtfFFNil,		"fnil",		0 },
	{ rtfFontFamily,	rtfFFRoman,		"froman",	0 },
	{ rtfFontFamily,	rtfFFSwiss,		"fswiss",	0 },
	{ rtfFontFamily,	rtfFFModern,		"fmodern",	0 },
	{ rtfFontFamily,	rtfFFScript,		"fscript",	0 },
	{ rtfFontFamily,	rtfFFDecor,		"fdecor",	0 },
	{ rtfFontFamily,	rtfFFTech,		"ftech",	0 },
	{ rtfFontFamily,	rtfFFBidirectional,	"fbidi",	0 },

	/*
	 * Font attributes
	 */

	{ rtfFontAttr,	rtfFontCharSet,		"fcharset",	0 },
	{ rtfFontAttr,	rtfFontPitch,		"fprq",		0 },
	{ rtfFontAttr,	rtfFontCodePage,	"cpg",		0 },
	{ rtfFontAttr,	rtfFTypeNil,		"ftnil",	0 },
	{ rtfFontAttr,	rtfFTypeTrueType,	"fttruetype",	0 },

	/*
	 * File table attributes
	 */

	{ rtfFileAttr,	rtfFileNum,		"fid",		0 },
	{ rtfFileAttr,	rtfFileRelPath,		"frelative",	0 },
	{ rtfFileAttr,	rtfFileOSNum,		"fosnum",	0 },

	/*
	 * File sources
	 */

	{ rtfFileSource,	rtfSrcMacintosh,	"fvalidmac",	0 },
	{ rtfFileSource,	rtfSrcDOS,		"fvaliddos",	0 },
	{ rtfFileSource,	rtfSrcNTFS,		"fvalidntfs",	0 },
	{ rtfFileSource,	rtfSrcHPFS,		"fvalidhpfs",	0 },
	{ rtfFileSource,	rtfSrcNetwork,		"fnetwork",	0 },

	/*
	 * Color names
	 */

	{ rtfColorName,	rtfRed,			"red",		0 },
	{ rtfColorName,	rtfGreen,		"green",	0 },
	{ rtfColorName,	rtfBlue,		"blue",		0 },

	/*
	 * Charset names
	 */

	{ rtfCharSet,	rtfMacCharSet,		"mac",		0 },
	{ rtfCharSet,	rtfAnsiCharSet,		"ansi",		0 },
	{ rtfCharSet,	rtfPcCharSet,		"pc",		0 },
	{ rtfCharSet,	rtfPcaCharSet,		"pca",		0 },

	/*
	 * Table attributes
	 */

	{ rtfTblAttr,	rtfRowDef,		"trowd",	0 },
	{ rtfTblAttr,	rtfRowGapH,		"trgaph",	0 },
	{ rtfTblAttr,	rtfCellPos,		"cellx",	0 },
	{ rtfTblAttr,	rtfMergeRngFirst,	"clmgf",	0 },
	{ rtfTblAttr,	rtfMergePrevious,	"clmrg",	0 },

	{ rtfTblAttr,	rtfRowLeft,		"trql",		0 },
	{ rtfTblAttr,	rtfRowRight,		"trqr",		0 },
	{ rtfTblAttr,	rtfRowCenter,		"trqc",		0 },
	{ rtfTblAttr,	rtfRowLeftEdge,		"trleft",	0 },
	{ rtfTblAttr,	rtfRowHt,		"trrh",		0 },
	{ rtfTblAttr,	rtfRowHeader,		"trhdr",	0 },
	{ rtfTblAttr,	rtfRowKeep,		"trkeep",	0 },

	{ rtfTblAttr,	rtfRTLRow,		"rtlrow",	0 },
	{ rtfTblAttr,	rtfLTRRow,		"ltrrow",	0 },

	{ rtfTblAttr,	rtfRowBordTop,		"trbrdrt",	0 },
	{ rtfTblAttr,	rtfRowBordLeft,		"trbrdrl",	0 },
	{ rtfTblAttr,	rtfRowBordBottom,	"trbrdrb",	0 },
	{ rtfTblAttr,	rtfRowBordRight,	"trbrdrr",	0 },
	{ rtfTblAttr,	rtfRowBordHoriz,	"trbrdrh",	0 },
	{ rtfTblAttr,	rtfRowBordVert,		"trbrdrv",	0 },

	{ rtfTblAttr,	rtfCellBordBottom,	"clbrdrb",	0 },
	{ rtfTblAttr,	rtfCellBordTop,		"clbrdrt",	0 },
	{ rtfTblAttr,	rtfCellBordLeft,	"clbrdrl",	0 },
	{ rtfTblAttr,	rtfCellBordRight,	"clbrdrr",	0 },

	{ rtfTblAttr,	rtfCellShading,		"clshdng",	0 },
	{ rtfTblAttr,	rtfCellBgPatH,		"clbghoriz",	0 },
	{ rtfTblAttr,	rtfCellBgPatV,		"clbgvert",	0 },
	{ rtfTblAttr,	rtfCellFwdDiagBgPat,	"clbgfdiag",	0 },
	{ rtfTblAttr,	rtfCellBwdDiagBgPat,	"clbgbdiag",	0 },
	{ rtfTblAttr,	rtfCellHatchBgPat,	"clbgcross",	0 },
	{ rtfTblAttr,	rtfCellDiagHatchBgPat,	"clbgdcross",	0 },
	/*
	 * The spec lists "clbgdkhor", but the corresponding non-cell
	 * control is "bgdkhoriz".  At any rate Macintosh Word seems
	 * to accept both "clbgdkhor" and "clbgdkhoriz".
	 */
	{ rtfTblAttr,	rtfCellDarkBgPatH,	"clbgdkhoriz",	0 },
	{ rtfTblAttr,	rtfCellDarkBgPatH,	"clbgdkhor",	0 },
	{ rtfTblAttr,	rtfCellDarkBgPatV,	"clbgdkvert",	0 },
	{ rtfTblAttr,	rtfCellFwdDarkBgPat,	"clbgdkfdiag",	0 },
	{ rtfTblAttr,	rtfCellBwdDarkBgPat,	"clbgdkbdiag",	0 },
	{ rtfTblAttr,	rtfCellDarkHatchBgPat,	"clbgdkcross",	0 },
	{ rtfTblAttr,	rtfCellDarkDiagHatchBgPat, "clbgdkdcross",	0 },
	{ rtfTblAttr,	rtfCellBgPatLineColor, "clcfpat",	0 },
	{ rtfTblAttr,	rtfCellBgPatColor,	"clcbpat",	0 },

	/*
	 * Field attributes
	 */

	{ rtfFieldAttr,	rtfFieldDirty,		"flddirty",	0 },
	{ rtfFieldAttr,	rtfFieldEdited,		"fldedit",	0 },
	{ rtfFieldAttr,	rtfFieldLocked,		"fldlock",	0 },
	{ rtfFieldAttr,	rtfFieldPrivate,	"fldpriv",	0 },
	{ rtfFieldAttr,	rtfFieldAlt,		"fldalt",	0 },

	/*
	 * Positioning attributes
	 */

	{ rtfPosAttr,	rtfAbsWid,		"absw",		0 },
	{ rtfPosAttr,	rtfAbsHt,		"absh",		0 },

	{ rtfPosAttr,	rtfRPosMargH,		"phmrg",	0 },
	{ rtfPosAttr,	rtfRPosPageH,		"phpg",		0 },
	{ rtfPosAttr,	rtfRPosColH,		"phcol",	0 },
	{ rtfPosAttr,	rtfPosX,		"posx",		0 },
	{ rtfPosAttr,	rtfPosNegX,		"posnegx",	0 },
	{ rtfPosAttr,	rtfPosXCenter,		"posxc",	0 },
	{ rtfPosAttr,	rtfPosXInside,		"posxi",	0 },
	{ rtfPosAttr,	rtfPosXOutSide,		"posxo",	0 },
	{ rtfPosAttr,	rtfPosXRight,		"posxr",	0 },
	{ rtfPosAttr,	rtfPosXLeft,		"posxl",	0 },

	{ rtfPosAttr,	rtfRPosMargV,		"pvmrg",	0 },
	{ rtfPosAttr,	rtfRPosPageV,		"pvpg",		0 },
	{ rtfPosAttr,	rtfRPosParaV,		"pvpara",	0 },
	{ rtfPosAttr,	rtfPosY,		"posy",		0 },
	{ rtfPosAttr,	rtfPosNegY,		"posnegy",	0 },
	{ rtfPosAttr,	rtfPosYInline,		"posyil",	0 },
	{ rtfPosAttr,	rtfPosYTop,		"posyt",	0 },
	{ rtfPosAttr,	rtfPosYCenter,		"posyc",	0 },
	{ rtfPosAttr,	rtfPosYBottom,		"posyb",	0 },

	{ rtfPosAttr,	rtfNoWrap,		"nowrap",	0 },
	{ rtfPosAttr,	rtfDistFromTextAll,	"dxfrtext",	0 },
	{ rtfPosAttr,	rtfDistFromTextX,	"dfrmtxtx",	0 },
	{ rtfPosAttr,	rtfDistFromTextY,	"dfrmtxty",	0 },
	/* \dyfrtext no longer exists in spec 1.2, apparently */
	/* replaced by \dfrmtextx and \dfrmtexty. */
	{ rtfPosAttr,	rtfTextDistY,		"dyfrtext",	0 },

	{ rtfPosAttr,	rtfDropCapLines,	"dropcapli",	0 },
	{ rtfPosAttr,	rtfDropCapType,		"dropcapt",	0 },

	/*
	 * Object controls
	 */

	{ rtfObjAttr,	rtfObjEmb,		"objemb",	0 },
	{ rtfObjAttr,	rtfObjLink,		"objlink",	0 },
	{ rtfObjAttr,	rtfObjAutoLink,		"objautlink",	0 },
	{ rtfObjAttr,	rtfObjSubscriber,	"objsub",	0 },
	{ rtfObjAttr,	rtfObjPublisher,	"objpub",	0 },
	{ rtfObjAttr,	rtfObjICEmb,		"objicemb",	0 },

	{ rtfObjAttr,	rtfObjLinkSelf,		"linkself",	0 },
	{ rtfObjAttr,	rtfObjLock,		"objupdate",	0 },
	{ rtfObjAttr,	rtfObjUpdate,		"objlock",	0 },

	{ rtfObjAttr,	rtfObjHt,		"objh",		0 },
	{ rtfObjAttr,	rtfObjWid,		"objw",		0 },
	{ rtfObjAttr,	rtfObjSetSize,		"objsetsize",	0 },
	{ rtfObjAttr,	rtfObjAlign,		"objalign",	0 },
	{ rtfObjAttr,	rtfObjTransposeY,	"objtransy",	0 },
	{ rtfObjAttr,	rtfObjCropTop,		"objcropt",	0 },
	{ rtfObjAttr,	rtfObjCropBottom,	"objcropb",	0 },
	{ rtfObjAttr,	rtfObjCropLeft,		"objcropl",	0 },
	{ rtfObjAttr,	rtfObjCropRight,	"objcropr",	0 },
	{ rtfObjAttr,	rtfObjScaleX,		"objscalex",	0 },
	{ rtfObjAttr,	rtfObjScaleY,		"objscaley",	0 },

	{ rtfObjAttr,	rtfObjResRTF,		"rsltrtf",	0 },
	{ rtfObjAttr,	rtfObjResPict,		"rsltpict",	0 },
	{ rtfObjAttr,	rtfObjResBitmap,	"rsltbmp",	0 },
	{ rtfObjAttr,	rtfObjResText,		"rslttxt",	0 },
	{ rtfObjAttr,	rtfObjResMerge,		"rsltmerge",	0 },

	{ rtfObjAttr,	rtfObjBookmarkPubObj,	"bkmkpub",	0 },
	{ rtfObjAttr,	rtfObjPubAutoUpdate,	"pubauto",	0 },

	/*
	 * Associated character formatting attributes
	 */

	{ rtfACharAttr,	rtfACBold,		"ab",		0 },
	{ rtfACharAttr,	rtfACAllCaps,		"caps",		0 },
	{ rtfACharAttr,	rtfACForeColor,		"acf",		0 },
	{ rtfACharAttr,	rtfACSubScript,		"adn",		0 },
	{ rtfACharAttr,	rtfACExpand,		"aexpnd",	0 },
	{ rtfACharAttr,	rtfACFontNum,		"af",		0 },
	{ rtfACharAttr,	rtfACFontSize,		"afs",		0 },
	{ rtfACharAttr,	rtfACItalic,		"ai",		0 },
	{ rtfACharAttr,	rtfACLanguage,		"alang",	0 },
	{ rtfACharAttr,	rtfACOutline,		"aoutl",	0 },
	{ rtfACharAttr,	rtfACSmallCaps,		"ascaps",	0 },
	{ rtfACharAttr,	rtfACShadow,		"ashad",	0 },
	{ rtfACharAttr,	rtfACStrikeThru,	"astrike",	0 },
	{ rtfACharAttr,	rtfACUnderline,		"aul",		0 },
	{ rtfACharAttr,	rtfACDotUnderline,	"auld",		0 },
	{ rtfACharAttr,	rtfACDbUnderline,	"auldb",	0 },
	{ rtfACharAttr,	rtfACNoUnderline,	"aulnone",	0 },
	{ rtfACharAttr,	rtfACWordUnderline,	"aulw",		0 },
	{ rtfACharAttr,	rtfACSuperScript,	"aup",		0 },

	/*
	 * Footnote attributes
	 */

	{ rtfFNoteAttr,	rtfFNAlt,		"ftnalt",	0 },

	/*
	 * Key code attributes
	 */

	{ rtfKeyCodeAttr,	rtfAltKey,		"alt",		0 },
	{ rtfKeyCodeAttr,	rtfShiftKey,		"shift",	0 },
	{ rtfKeyCodeAttr,	rtfControlKey,		"ctrl",		0 },
	{ rtfKeyCodeAttr,	rtfFunctionKey,		"fn",		0 },

	/*
	 * Bookmark attributes
	 */

	{ rtfBookmarkAttr, rtfBookmarkFirstCol,	"bkmkcolf",	0 },
	{ rtfBookmarkAttr, rtfBookmarkLastCol,	"bkmkcoll",	0 },

	/*
	 * Index entry attributes
	 */

	{ rtfIndexAttr,	rtfIndexNumber,		"xef",		0 },
	{ rtfIndexAttr,	rtfIndexBold,		"bxe",		0 },
	{ rtfIndexAttr,	rtfIndexItalic,		"ixe",		0 },

	/*
	 * Table of contents attributes
	 */

	{ rtfTOCAttr,	rtfTOCType,		"tcf",		0 },
	{ rtfTOCAttr,	rtfTOCLevel,		"tcl",		0 },

	/*
	 * Drawing object attributes
	 */

	{ rtfDrawAttr,	rtfDrawLock,		"dolock",	0 },
	{ rtfDrawAttr,	rtfDrawPageRelX,	"doxpage",	0 },
	{ rtfDrawAttr,	rtfDrawColumnRelX,	"dobxcolumn",	0 },
	{ rtfDrawAttr,	rtfDrawMarginRelX,	"dobxmargin",	0 },
	{ rtfDrawAttr,	rtfDrawPageRelY,	"dobypage",	0 },
	{ rtfDrawAttr,	rtfDrawColumnRelY,	"dobycolumn",	0 },
	{ rtfDrawAttr,	rtfDrawMarginRelY,	"dobymargin",	0 },
	{ rtfDrawAttr,	rtfDrawHeight,		"dobhgt",	0 },

	{ rtfDrawAttr,	rtfDrawBeginGroup,	"dpgroup",	0 },
	{ rtfDrawAttr,	rtfDrawGroupCount,	"dpcount",	0 },
	{ rtfDrawAttr,	rtfDrawEndGroup,	"dpendgroup",	0 },
	{ rtfDrawAttr,	rtfDrawArc,		"dparc",	0 },
	{ rtfDrawAttr,	rtfDrawCallout,		"dpcallout",	0 },
	{ rtfDrawAttr,	rtfDrawEllipse,		"dpellipse",	0 },
	{ rtfDrawAttr,	rtfDrawLine,		"dpline",	0 },
	{ rtfDrawAttr,	rtfDrawPolygon,		"dppolygon",	0 },
	{ rtfDrawAttr,	rtfDrawPolyLine,	"dppolyline",	0 },
	{ rtfDrawAttr,	rtfDrawRect,		"dprect",	0 },
	{ rtfDrawAttr,	rtfDrawTextBox,		"dptxbx",	0 },

	{ rtfDrawAttr,	rtfDrawOffsetX,		"dpx",		0 },
	{ rtfDrawAttr,	rtfDrawSizeX,		"dpxsize",	0 },
	{ rtfDrawAttr,	rtfDrawOffsetY,		"dpy",		0 },
	{ rtfDrawAttr,	rtfDrawSizeY,		"dpysize",	0 },

	{ rtfDrawAttr,	rtfCOAngle,		"dpcoa",	0 },
	{ rtfDrawAttr,	rtfCOAccentBar,		"dpcoaccent",	0 },
	{ rtfDrawAttr,	rtfCOBestFit,		"dpcobestfit",	0 },
	{ rtfDrawAttr,	rtfCOBorder,		"dpcoborder",	0 },
	{ rtfDrawAttr,	rtfCOAttachAbsDist,	"dpcodabs",	0 },
	{ rtfDrawAttr,	rtfCOAttachBottom,	"dpcodbottom",	0 },
	{ rtfDrawAttr,	rtfCOAttachCenter,	"dpcodcenter",	0 },
	{ rtfDrawAttr,	rtfCOAttachTop,		"dpcodtop",	0 },
	{ rtfDrawAttr,	rtfCOLength,		"dpcolength",	0 },
	{ rtfDrawAttr,	rtfCONegXQuadrant,	"dpcominusx",	0 },
	{ rtfDrawAttr,	rtfCONegYQuadrant,	"dpcominusy",	0 },
	{ rtfDrawAttr,	rtfCOOffset,		"dpcooffset",	0 },
	{ rtfDrawAttr,	rtfCOAttachSmart,	"dpcosmarta",	0 },
	{ rtfDrawAttr,	rtfCODoubleLine,	"dpcotdouble",	0 },
	{ rtfDrawAttr,	rtfCORightAngle,	"dpcotright",	0 },
	{ rtfDrawAttr,	rtfCOSingleLine,	"dpcotsingle",	0 },
	{ rtfDrawAttr,	rtfCOTripleLine,	"dpcottriple",	0 },

	{ rtfDrawAttr,	rtfDrawTextBoxMargin,	"dptxbxmar",	0 },
	{ rtfDrawAttr,	rtfDrawTextBoxText,	"dptxbxtext",	0 },
	{ rtfDrawAttr,	rtfDrawRoundRect,	"dproundr",	0 },

	{ rtfDrawAttr,	rtfDrawPointX,		"dpptx",	0 },
	{ rtfDrawAttr,	rtfDrawPointY,		"dppty",	0 },
	{ rtfDrawAttr,	rtfDrawPolyCount,	"dppolycount",	0 },

	{ rtfDrawAttr,	rtfDrawArcFlipX,	"dparcflipx",	0 },
	{ rtfDrawAttr,	rtfDrawArcFlipY,	"dparcflipy",	0 },

	{ rtfDrawAttr,	rtfDrawLineBlue,	"dplinecob",	0 },
	{ rtfDrawAttr,	rtfDrawLineGreen,	"dplinecog",	0 },
	{ rtfDrawAttr,	rtfDrawLineRed,		"dplinecor",	0 },
	{ rtfDrawAttr,	rtfDrawLinePalette,	"dplinepal",	0 },
	{ rtfDrawAttr,	rtfDrawLineDashDot,	"dplinedado",	0 },
	{ rtfDrawAttr,	rtfDrawLineDashDotDot,	"dplinedadodo",	0 },
	{ rtfDrawAttr,	rtfDrawLineDash,	"dplinedash",	0 },
	{ rtfDrawAttr,	rtfDrawLineDot,		"dplinedot",	0 },
	{ rtfDrawAttr,	rtfDrawLineGray,	"dplinegray",	0 },
	{ rtfDrawAttr,	rtfDrawLineHollow,	"dplinehollow",	0 },
	{ rtfDrawAttr,	rtfDrawLineSolid,	"dplinesolid",	0 },
	{ rtfDrawAttr,	rtfDrawLineWidth,	"dplinew",	0 },

	{ rtfDrawAttr,	rtfDrawHollowEndArrow,	"dpaendhol",	0 },
	{ rtfDrawAttr,	rtfDrawEndArrowLength,	"dpaendl",	0 },
	{ rtfDrawAttr,	rtfDrawSolidEndArrow,	"dpaendsol",	0 },
	{ rtfDrawAttr,	rtfDrawEndArrowWidth,	"dpaendw",	0 },
	{ rtfDrawAttr,	rtfDrawHollowStartArrow,"dpastarthol",	0 },
	{ rtfDrawAttr,	rtfDrawStartArrowLength,"dpastartl",	0 },
	{ rtfDrawAttr,	rtfDrawSolidStartArrow,	"dpastartsol",	0 },
	{ rtfDrawAttr,	rtfDrawStartArrowWidth,	"dpastartw",	0 },

	{ rtfDrawAttr,	rtfDrawBgFillBlue,	"dpfillbgcb",	0 },
	{ rtfDrawAttr,	rtfDrawBgFillGreen,	"dpfillbgcg",	0 },
	{ rtfDrawAttr,	rtfDrawBgFillRed,	"dpfillbgcr",	0 },
	{ rtfDrawAttr,	rtfDrawBgFillPalette,	"dpfillbgpal",	0 },
	{ rtfDrawAttr,	rtfDrawBgFillGray,	"dpfillbggray",	0 },
	{ rtfDrawAttr,	rtfDrawFgFillBlue,	"dpfillfgcb",	0 },
	{ rtfDrawAttr,	rtfDrawFgFillGreen,	"dpfillfgcg",	0 },
	{ rtfDrawAttr,	rtfDrawFgFillRed,	"dpfillfgcr",	0 },
	{ rtfDrawAttr,	rtfDrawFgFillPalette,	"dpfillfgpal",	0 },
	{ rtfDrawAttr,	rtfDrawFgFillGray,	"dpfillfggray",	0 },
	{ rtfDrawAttr,	rtfDrawFillPatIndex,	"dpfillpat",	0 },

	{ rtfDrawAttr,	rtfDrawShadow,		"dpshadow",	0 },
	{ rtfDrawAttr,	rtfDrawShadowXOffset,	"dpshadx",	0 },
	{ rtfDrawAttr,	rtfDrawShadowYOffset,	"dpshady",	0 },

	{ rtfVersion,	-1,			"rtf",		0 },
	{ rtfDefFont,	-1,			"deff",		0 },

	{ 0,		-1,			NULL,		0 }
};

typedef struct tagRTFHashTableEntry {
	int count;
	RTFKey **value;
} RTFHashTableEntry;

static RTFHashTableEntry rtfHashTable[ARRAY_SIZE(rtfKey) * 2];


/*
 * Initialize lookup table hash values.  Only need to do this once.
 */

void LookupInit(void)
{
	RTFKey	*rp;

	memset(rtfHashTable, 0, sizeof rtfHashTable);
	for (rp = rtfKey; rp->rtfKStr != NULL; rp++)
	{
		int index;

		rp->rtfKHash = Hash (rp->rtfKStr);
		index = rp->rtfKHash % (ARRAY_SIZE(rtfKey) * 2);
		rtfHashTable[index].value = realloc(rtfHashTable[index].value, sizeof(RTFKey *) * (rtfHashTable[index].count + 1));
		rtfHashTable[index].value[rtfHashTable[index].count++] = rp;
	}
}

void LookupCleanup(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(rtfKey) * 2; i++)
	{
		free(rtfHashTable[i].value);
		rtfHashTable[i].value = NULL;
		rtfHashTable[i].count = 0;
	}
}


/*
 * Determine major and minor number of control token.  If it's
 * not found, the class turns into rtfUnknown.
 */

static void Lookup(RTF_Info *info, char *s)
{
	RTFKey	*rp;
	int	hash;
        RTFHashTableEntry *entry;
        int i;

	++s;			/* skip over the leading \ character */
	hash = Hash (s);
        entry = &rtfHashTable[hash % (ARRAY_SIZE(rtfKey) * 2)];
	for (i = 0; i < entry->count; i++)
	{
                rp = entry->value[i];
		if (hash == rp->rtfKHash && strcmp (s, rp->rtfKStr) == 0)
		{
			info->rtfClass = rtfControl;
			info->rtfMajor = rp->rtfKMajor;
			info->rtfMinor = rp->rtfKMinor;
			return;
		}
	}
	info->rtfClass = rtfUnknown;
}


/*
 * Compute hash value of symbol
 */

static int Hash(const char *s)
{
	char	c;
	int	val = 0;

	while ((c = *s++) != '\0')
		val += c;
	return (val);
}



/* ---------------------------------------------------------------------- */


/*
 * Token comparison routines
 */

int RTFCheckCM(const RTF_Info *info, int class, int major)
{
	return (info->rtfClass == class && info->rtfMajor == major);
}


int RTFCheckCMM(const RTF_Info *info, int class, int major, int minor)
{
	return (info->rtfClass == class && info->rtfMajor == major && info->rtfMinor == minor);
}


int RTFCheckMM(const RTF_Info *info, int major, int minor)
{
	return (info->rtfMajor == major && info->rtfMinor == minor);
}


/* ---------------------------------------------------------------------- */


int RTFCharToHex(char c)
{
	if (isupper (c))
		c = tolower (c);
	if (isdigit (c))
		return (c - '0');	/* '0'..'9' */
	return (c - 'a' + 10);		/* 'a'..'f' */
}


/* ---------------------------------------------------------------------- */

/*
 * originally from RTF tools' text-writer.c
 *
 * text-writer -- RTF-to-text translation writer code.
 *
 * Read RTF input, write text of document (text extraction).
 */

static void	TextClass (RTF_Info *info);
static void	ControlClass (RTF_Info *info);
static void     DefFont(RTF_Info *info);
static void	Destination (RTF_Info *info);
static void	SpecialChar (RTF_Info *info);
static void	RTFPutUnicodeChar (RTF_Info *info, int c);

/*
 * Initialize the writer.
 */

void
WriterInit (RTF_Info *info )
{
}


int
BeginFile (RTF_Info *info )
{
	/* install class callbacks */

	RTFSetClassCallback (info, rtfText, TextClass);
	RTFSetClassCallback (info, rtfControl, ControlClass);

	return (1);
}

/*
 * Write out a character.
 */

static void
TextClass (RTF_Info *info)
{
	RTFPutCodePageChar(info, info->rtfMajor);
}


static void
ControlClass (RTF_Info *info)
{
	switch (info->rtfMajor)
	{
        case rtfCharAttr:
                CharAttr(info);
                ME_RTFCharAttrHook(info);
                break;
        case rtfParAttr:
                ME_RTFParAttrHook(info);
                break;
        case rtfTblAttr:
                ME_RTFTblAttrHook(info);
                break;
        case rtfCharSet:
                CharSet(info);
                break;
        case rtfDefFont:
                DefFont(info);
                break;
	case rtfDestination:
		Destination (info);
		break;
        case rtfDocAttr:
                DocAttr(info);
                break;
	case rtfSpecialChar:
                SpecialChar (info);
                ME_RTFSpecialCharHook(info);
		break;
	}
}


static void
CharAttr(RTF_Info *info)
{
        RTFFont *font;

        switch (info->rtfMinor)
        {
        case rtfFontNum:
                font = RTFGetFont(info, info->rtfParam);
                if (font)
                {
                        if (info->ansiCodePage != CP_UTF8 && info->codePage != font->rtfFCodePage)
                        {
                                RTFFlushOutputBuffer(info);
                                info->codePage = font->rtfFCodePage;
                        }
                        TRACE("font %d codepage %d\n", info->rtfParam, info->codePage);
                }
                else
                        ERR( "unknown font %d\n", info->rtfParam);
                break;
        case rtfUnicodeLength:
                info->unicodeLength = info->rtfParam;
                break;
        }
}


static void
CharSet(RTF_Info *info)
{
	if (info->ansiCodePage == CP_UTF8)
		return;
 
        switch (info->rtfMinor)
        {
        case rtfAnsiCharSet:
                info->ansiCodePage = 1252; /* Latin-1 */
                break;
        case rtfMacCharSet:
                info->ansiCodePage = 10000; /* MacRoman */
                break;
        case rtfPcCharSet:
                info->ansiCodePage = 437;
                break;
        case rtfPcaCharSet:
                info->ansiCodePage = 850;
                break;
        }
}

/*
 * This function notices destinations that aren't explicitly handled
 * and skips to their ends.  This keeps, for instance, picture
 * data from being considered as plain text.
 */

static void
Destination (RTF_Info *info)
{
	if (!RTFGetDestinationCallback(info, info->rtfMinor))
		RTFSkipGroup (info);    
}


static void
DefFont(RTF_Info *info)
{
        TRACE("%d\n", info->rtfParam);
        info->defFont = info->rtfParam;
}


static void
DocAttr(RTF_Info *info)
{
        TRACE("minor %d, param %d\n", info->rtfMinor, info->rtfParam);

        switch (info->rtfMinor)
        {
        case rtfAnsiCodePage:
                info->codePage = info->ansiCodePage = info->rtfParam;
                break;
        case rtfUTF8RTF:
                info->codePage = info->ansiCodePage = CP_UTF8;
                break;
        }
}


static void SpecialChar (RTF_Info *info)
{
	switch (info->rtfMinor)
	{
	case rtfOptDest:
		/* the next token determines destination, if it's unknown, skip the group */
		/* this way we filter out the garbage coming from unknown destinations */ 
		RTFGetToken(info); 
		if (info->rtfClass != rtfDestination)
			RTFSkipGroup(info);
		else
			RTFRouteToken(info); /* "\*" is ignored with known destinations */
		break;
	case rtfUnicode:
	{
                int i;

                RTFPutUnicodeChar(info, info->rtfParam);

                /* After \u we must skip number of character tokens set by \ucN */
                for (i = 0; i < info->unicodeLength; i++)
                {
			RTFGetToken(info);
                        if (info->rtfClass != rtfText)
		        {
                                ERR("The token behind \\u is not text, but (%d,%d,%d)\n",
				info->rtfClass, info->rtfMajor, info->rtfMinor);
                                RTFUngetToken(info);
                                break;
                        }
		}
		break;
	}
	case rtfLine:
            RTFFlushOutputBuffer(info);
            ME_InsertEndRowFromCursor(info->editor, 0);
            break;
	case rtfPage:
	case rtfSect:
	case rtfPar:
                RTFFlushOutputBuffer(info);
                editor_set_selection_para_fmt( info->editor, &info->fmt );
                memset(&info->fmt, 0, sizeof(info->fmt));
                info->fmt.cbSize = sizeof(info->fmt);
		RTFPutUnicodeChar (info, '\r');
		if (info->editor->bEmulateVersion10) RTFPutUnicodeChar (info, '\n');
		break;
	case rtfNoBrkSpace:
		RTFPutUnicodeChar (info, 0x00A0);
		break;
	case rtfTab:
		RTFPutUnicodeChar (info, '\t');
		break;
	case rtfNoBrkHyphen:
		RTFPutUnicodeChar (info, 0x2011);
		break;
	case rtfBullet:
		RTFPutUnicodeChar (info, 0x2022);
		break;
	case rtfEmDash:
		RTFPutUnicodeChar (info, 0x2014);
		break;
	case rtfEnDash:
		RTFPutUnicodeChar (info, 0x2013);
		break;
        case rtfEmSpace:
                RTFPutUnicodeChar (info, ' ');
                break;
        case rtfEnSpace:
                RTFPutUnicodeChar (info, ' ');
                break;
	case rtfLQuote:
		RTFPutUnicodeChar (info, 0x2018);
		break;
	case rtfRQuote:
		RTFPutUnicodeChar (info, 0x2019);
		break;
	case rtfLDblQuote:
		RTFPutUnicodeChar (info, 0x201C);
		break;
	case rtfRDblQuote:
		RTFPutUnicodeChar (info, 0x201D);
		break;
        case rtfLTRMark:
                RTFPutUnicodeChar (info, 0x200E);
                break;
        case rtfRTLMark:
                RTFPutUnicodeChar (info, 0x200F);
                break;
        case rtfNoWidthJoiner:
                RTFPutUnicodeChar (info, 0x200D);
                break;
        case rtfNoWidthNonJoiner:
                RTFPutUnicodeChar (info, 0x200C);
                break;
	}
}


static void
RTFFlushUnicodeOutputBuffer(RTF_Info *info)
{
        if (info->dwOutputCount)
        {
                ME_InsertTextFromCursor(info->editor, 0, info->OutputBuffer,
                                        info->dwOutputCount, info->style);
                info->dwOutputCount = 0;
        }
}


static void
RTFPutUnicodeString(RTF_Info *info, const WCHAR *string, int length)
{
        if (info->dwCPOutputCount)
                RTFFlushCPOutputBuffer(info);
        while (length)
        {
                int fit = min(length, ARRAY_SIZE(info->OutputBuffer) - info->dwOutputCount);

                memmove(info->OutputBuffer + info->dwOutputCount, string, fit * sizeof(WCHAR));
                info->dwOutputCount += fit;
                length -= fit;
                string += fit;
                if (ARRAY_SIZE(info->OutputBuffer) == info->dwOutputCount)
                        RTFFlushUnicodeOutputBuffer(info);
        }
}

static void
RTFFlushCPOutputBuffer(RTF_Info *info)
{
        int bufferMax = info->dwCPOutputCount * 2 * sizeof(WCHAR);
        WCHAR *buffer = malloc(bufferMax);
        int length;

        length = MultiByteToWideChar(info->codePage, 0, info->cpOutputBuffer,
                                     info->dwCPOutputCount, buffer, bufferMax/sizeof(WCHAR));
        info->dwCPOutputCount = 0;

        RTFPutUnicodeString(info, buffer, length);
        free(buffer);
}

void
RTFFlushOutputBuffer(RTF_Info *info)
{
        if (info->dwCPOutputCount)
                RTFFlushCPOutputBuffer(info);
        RTFFlushUnicodeOutputBuffer(info);
}

static void
RTFPutUnicodeChar(RTF_Info *info, int c)
{
	if (info->dwCPOutputCount)
                RTFFlushCPOutputBuffer(info);
        if (info->dwOutputCount * sizeof(WCHAR) >= ( sizeof info->OutputBuffer - 1 ) )
		RTFFlushUnicodeOutputBuffer( info );
	info->OutputBuffer[info->dwOutputCount++] = c;
}

static void
RTFPutCodePageChar(RTF_Info *info, int c)
{
        /* Use dynamic buffer here because it's the best way to handle
         * MBCS codepages without having to worry about partial chars */
        if (info->dwCPOutputCount >= info->dwMaxCPOutputCount)
        {
                info->dwMaxCPOutputCount *= 2;
                info->cpOutputBuffer = realloc(info->cpOutputBuffer, info->dwMaxCPOutputCount);
        }
        info->cpOutputBuffer[info->dwCPOutputCount++] = c;
}
