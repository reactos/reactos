/*
 * text-writer -- RTF-to-text translation writer code.
 *
 * Read RTF input, write text of document (text extraction).
 *
 * Wrapper must call WriterInit() once before processing any files,
 * then set up input and call BeginFile() for each input file.
 *
 * This installs callbacks for the text and control token classes.
 * The control class is necessary so that special characters such as
 * \par, \tab, \sect, etc.  can be converted.
 *
 * It's problematic what to do with text in headers and footers, and
 * what to do about tables.
 *
 * This really is quite a stupid program, for instance, it could keep
 * track of the current leader character and dump that out when a tab
 * is encountered.
 *
 * 04 Feb 91	Paul DuBois	dubois@primate.wisc.edu
 *
 * This software may be redistributed without restriction and used for
 * any purpose whatsoever.
 *
 * 04 Feb 91
 * -Created.
 * 27 Feb 91
 * - Updated for distribution 1.05.
 * 13 Jul 93
 * - Updated to compile under THINK C 6.0.
 * 31 Aug 93
 * - Added Mike Sendall's entries for Macintosh char map.
 * 07 Sep 93
 * - Uses charset map and output sequence map for character translation.
 * 11 Mar 94
 * - Updated for 1.10 distribution.
 */

#include <stdio.h>

#include "rtf.h"
#include "rtf2text.h"
#include "charlist.h"
#include "debug.h"

#if 0
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);
#else
  #define TRACE DPRINT
  #define WARN DPRINT
  #define FIXME DPRINT
#endif

static void	TextClass ();
static void	ControlClass ();
static void	Destination ();
static void	SpecialChar ();
static void	PutStdChar ();
static void	PutLitChar ();
static void	PutLitStr ();

static char	*outMap[rtfSC_MaxChar];

static CHARLIST charlist = {0, NULL, NULL};

int RTFToBuffer(char* pBuffer, int nBufferSize);
int RTFToBuffer(char* pBuffer, int nBufferSize)
{

   /* check if the buffer is big enough to hold all characters  */
   /* we require one more for the '\0'                          */

   TRACE("\n");

   if(nBufferSize < charlist.nCount + 1) {
        return charlist.nCount + CHARLIST_CountChar(&charlist, '\n') + 1;
   }

   while(charlist.nCount)
   {
       *pBuffer = CHARLIST_Dequeue(&charlist);
       if(*pBuffer=='\n')
       {
         *pBuffer = '\r';
         pBuffer++;
         *pBuffer = '\n';
       }
       pBuffer++;
   }
   *pBuffer = '\0';

   return 0;
}


/*
 * Initialize the writer.
 */

void
WriterInit ()
{
	RTFReadOutputMap (outMap,1);
}


int
BeginFile ()
{
	/* install class callbacks */

	RTFSetClassCallback (rtfText, TextClass);
	RTFSetClassCallback (rtfControl, ControlClass);

	return (1);
}


/*
 * Write out a character.  rtfMajor contains the input character, rtfMinor
 * contains the corresponding standard character code.
 *
 * If the input character isn't in the charset map, try to print some
 * representation of it.
 */

static void
TextClass ()
{
char	buf[rtfBufSiz];

	TRACE("\n");

	if (rtfFormat == SF_TEXT)
	        PutLitChar (rtfMajor);
	else if (rtfMinor != rtfSC_nothing)
		PutStdChar (rtfMinor);
	else
	{
		if (rtfMajor < 128)	/* in ASCII range */
			sprintf (buf, "[[%c]]", rtfMajor);
		else
			sprintf (buf, "[[\\'%02x]]", rtfMajor);
		PutLitStr (buf);
	}
}


static void
ControlClass ()
{
	TRACE("\n");
	switch (rtfMajor)
	{
	case rtfDestination:
		Destination ();
		break;
	case rtfSpecialChar:
		SpecialChar ();
		break;
	}
}


/*
 * This function notices destinations that should be ignored
 * and skips to their ends.  This keeps, for instance, picture
 * data from being considered as plain text.
 */

static void
Destination ()
{

	TRACE("\n");

	switch (rtfMinor)
	{
	case rtfPict:
	case rtfFNContSep:
	case rtfFNContNotice:
	case rtfInfo:
	case rtfIndexRange:
	case rtfITitle:
	case rtfISubject:
	case rtfIAuthor:
	case rtfIOperator:
	case rtfIKeywords:
	case rtfIComment:
	case rtfIVersion:
	case rtfIDoccomm:
		RTFSkipGroup ();
		break;
	}
}


/*
 * The reason these use the rtfSC_xxx thingies instead of just writing
 * out ' ', '-', '"', etc., is so that the mapping for these characters
 * can be controlled by the text-map file.
 */

void SpecialChar ()
{

	TRACE("\n");

	switch (rtfMinor)
	{
	case rtfPage:
	case rtfSect:
	case rtfRow:
	case rtfLine:
	case rtfPar:
		PutLitChar ('\n');
		break;
	case rtfCell:
		PutStdChar (rtfSC_space);	/* make sure cells are separated */
		break;
	case rtfNoBrkSpace:
		PutStdChar (rtfSC_nobrkspace);
		break;
	case rtfTab:
		PutLitChar ('\t');
		break;
	case rtfNoBrkHyphen:
		PutStdChar (rtfSC_nobrkhyphen);
		break;
	case rtfBullet:
		PutStdChar (rtfSC_bullet);
		break;
	case rtfEmDash:
		PutStdChar (rtfSC_emdash);
		break;
	case rtfEnDash:
		PutStdChar (rtfSC_endash);
		break;
	case rtfLQuote:
		PutStdChar (rtfSC_quoteleft);
		break;
	case rtfRQuote:
		PutStdChar (rtfSC_quoteright);
		break;
	case rtfLDblQuote:
		PutStdChar (rtfSC_quotedblleft);
		break;
	case rtfRDblQuote:
		PutStdChar (rtfSC_quotedblright);
		break;
	}
}


/*
 * Eventually this should keep track of the destination of the
 * current state and only write text when in the initial state.
 *
 * If the output sequence is unspecified in the output map, write
 * the character's standard name instead.  This makes map deficiencies
 * obvious and provides incentive to fix it. :-)
 */

void PutStdChar (int stdCode)
{

  char	*oStr = (char *) NULL;
  char	buf[rtfBufSiz];

/*	if (stdCode == rtfSC_nothing)
		RTFPanic ("Unknown character code, logic error\n");
*/
	TRACE("\n");

	oStr = outMap[stdCode];
	if (oStr == (char *) NULL)	/* no output sequence in map */
	{
		sprintf (buf, "[[%s]]", RTFStdCharName (stdCode));
		oStr = buf;
	}
	PutLitStr (oStr);
}


void PutLitChar (int c)
{
	CHARLIST_Enqueue(&charlist, (char) c);
        /* fputc (c, ostream); */
}


static void PutLitStr (char	*s)
{
	for(;*s;s++)
	{
	  CHARLIST_Enqueue(&charlist, *s);
	}
	/* fputs (s, ostream); */
}
