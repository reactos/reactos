#include "mktyplib.h"

// LEXER for MKTYPLIB

#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#ifndef WIN32
#include  <ole2.h>		// required for dispatch.h
#include  "dispatch.h"
#endif //WIN32

#include "errors.h"
#include "tokens.h"

// external data
extern TOKEN tok;
extern SYSKIND SysKind;


// stuff for error reporting
extern CHAR szFileCur[];	// file name of current token
extern DWORD lnCur;		// line # of current token
extern WORD colCur;		// column # of current token
extern DWORD lnLast;		// line # of last token
extern WORD colLast;		// column # of last token
extern CHAR * szExpected;	// item for "expected: <item>"
extern FILE *hFileInput;
#if FV_CPP
extern BOOL fCPP;		// true if we pre-processed input file
#endif //FV_CPP

// private types
typedef struct {
    TOKID	id;
    CHAR *	sz;
} RWTABLE;


#define	CB_LINEBUF		1024
#define	CB_MAX_ID		255
#define	CB_MAX_STRING		255

// private data
CHAR * pLineBuf;		// line buffer
CHAR * pLine;			// pointer to current char in line
BOOL fDirectiveOK;


// prototypes
VOID FAR ParseInit (CHAR * szFile);
VOID FAR ConsumeTok (TOKID id, WORD fAccept);
VOID FAR ScanTok ( WORD	fAccept);
LPVOID FAR ParseMalloc ( WORD cbAlloc);

VOID NEAR FillLine(VOID);
VOID NEAR HandleString (VOID);
VOID NEAR HandleId (CHAR ch, WORD fAccept);
CHAR NEAR HandleEscape(VOID);
TOKID NEAR RwLookup (CHAR *sz, RWTABLE *prgIds);
VOID NEAR HandleNumericLit (CHAR ch);
DWORD NEAR HandleHexNum (WORD cDigitsMax);
VOID NEAR HandleDirective (VOID);
BOOL NEAR FValidIdCh (CHAR ch, BOOL fFirst);

// reserved word tables
RWTABLE rgRws[] = {
	RW_LIBRARY,		"library",
	RW_TYPEDEF,		"typedef",
	RW_ENUM,		"enum",
	RW_STRUCT,		"struct",
	RW_MODULE,		"module",
	RW_INTERFACE,		"interface",
	RW_DISPINTERFACE,	"dispinterface",
	RW_COCLASS,		"coclass",
	RW_PROPERTIES,		"properties",
	RW_METHODS,		"methods",
	RW_IMPORTLIB,		"importlib",
	RW_PASCAL,		"pascal",
	RW_PASCAL,		"_pascal",
	RW_PASCAL,		"__pascal",
	RW_CDECL,		"cdecl",
	RW_CDECL,		"_cdecl",
	RW_CDECL,		"__cdecl",
	RW_STDCALL,		"stdcall",
	RW_STDCALL,		"_stdcall",
	RW_STDCALL,		"__stdcall",
	RW_UNSIGNED,		"unsigned",
	RW_UNION,		"union",
	RW_EXTERN,		"extern",
	RW_FAR,			"far",
	RW_FAR,			"_far",
	RW_SAFEARRAY,		"SAFEARRAY",
	RW_SAFEARRAY,		"safearray",
	RW_CONST,		"const",
#if	FV_PROPSET
	RW_PROPERTY_SET,	"property_set",
#endif	//FV_PROPSET
	// the folllowing are only present for error reporting purposes
	RW_LBRACKET,		"[",
	RW_RBRACKET,		"]",
	RW_LCURLY,		"{",
	RW_RCURLY,		"}",
	RW_LPAREN,		"(",
	RW_RPAREN,		")",
	RW_SEMI,		";",
	RW_COLON,		":",
	RW_PERIOD,		".",
	RW_COMMA,		",",
	RW_ASSIGN,		"=",
	RW_POINTER,		"*",
#if 0	// CONSIDER: (V2, EXPR) activate if these operators are activated
	OP_MINUS,		"-",
	OP_PLUS,		"+",
	OP_MUL,			"*",
	OP_DIV,			"/",
	OP_MOD,			"%",
	OP_EXP,			"^",
	OP_LOG_AND,		"&&",
	OP_LOG_OR,		"||",
	OP_LOG_NOT,		"!",
	OP_BIT_AND,		"&",
	OP_BIT_OR,		"|",
	OP_BIT_NOT,		"~",
	OP_LSHIFT,		"<<",
	OP_RSHIFT,		">>",
	OP_EQ,			"==",
	OP_LE,			"<=",
	OP_LT,			"<",
	OP_GE,			">=",
	OP_GT,			">",
#endif	//0
	RW_NOTFOUND,		NULL
	};


RWTABLE rgAttrs[] = {
	ATTR_UUID,		"uuid",
	ATTR_VERSION,		"version",
	ATTR_DLLNAME,		"dllname",
	ATTR_ENTRY,		"entry",
	ATTR_ID,		"id",
	ATTR_HELPSTRING,	"helpstring",
	ATTR_HELPCONTEXT,	"helpcontext",
	ATTR_HELPFILE,		"helpfile",
	ATTR_LCID,		"lcid",
	ATTR_PROPGET,		"propget",
	ATTR_PROPPUT,		"propput",
	ATTR_PROPPUTREF,	"propputref",
	ATTR_OPTIONAL,		"optional",
	ATTR_IN,		"in",
	ATTR_OUT,		"out",
	ATTR_STRING,		"string",
	ATTR_VARARG,		"vararg",
	ATTR_APPOBJECT,		"appobject",
	ATTR_RESTRICTED,	"restricted",
	ATTR_PUBLIC,		"public",
	ATTR_READONLY,		"readonly",
	ATTR_ODL,		"odl",
	ATTR_DEFAULT,		"default",
	ATTR_SOURCE,		"source",
	ATTR_BINDABLE,		"bindable",
	ATTR_REQUESTEDIT,	"requestedit",
	ATTR_DISPLAYBIND,	"displaybind",
	ATTR_DEFAULTBIND,	"defaultbind",
	ATTR_LICENSED,		"licensed",
	ATTR_PREDECLID, 	"predeclid",
	ATTR_HIDDEN,		"hidden",
	ATTR_RETVAL,		"retval",
	ATTR_CONTROL,		"control",
	ATTR_DUAL,		"dual",
	ATTR_NONEXTENSIBLE,	"nonextensible",
	ATTR_OLEAUTOMATION,	"oleautomation",
	RW_NOTFOUND,		NULL
	};

// ************************************************************************
// File init/term/read routines
// ************************************************************************

VOID FAR ParseInit
(
 CHAR * szFile
)
{
    pLineBuf = (CHAR *)malloc(CB_LINEBUF+1); // alloc buffer to read lines into

#ifdef WIN16
    // convert szFile in-place to OEM char set
    AnsiToOem(szFile, szFile);
#endif // WIN16

    hFileInput = fopen(szFile, "rb");	// open input file

#ifdef WIN16
    // convert back to ANSI
    OemToAnsi(szFile, szFile);
#endif // WIN16

    if (hFileInput == NULL)
#if FV_CPP
	ParseError(fCPP ? ERR_CPP : ERR_CANT_OPEN_INPUTFILE);
#else
	ParseError(ERR_CANT_OPEN_INPUTFILE);
#endif

    FillLine();			// get first line

    ScanTok(0);			// get first token
}


// fill up a line buffer
VOID NEAR FillLine()
{
    pLine = pLineBuf;		// reset pointer to BOL

    if (!fgets(pLineBuf, CB_LINEBUF, hFileInput))	// get a line
	{	// eof or error
	   if (feof(hFileInput))
		*pLine = '\0';	 // signal EOL
	   else	// CONSIDER: store error code & report it
	      ParseError(PERR_READ_ERROR);
	}
    lnCur++;			// increment current line

#ifdef DEBUG
#if 0
    if (fDebug)
	{
	   CHAR szDebugOut[1030];
           if (!*pLine)
		DebugOut("<EOF>\n");
           else
           	{
		sprintf(szDebugOut, "%ld: %s", lnCur, pLine);
		DebugOut(szDebugOut);	// dump input to screen
		}
	}
#endif // 0
#endif // DEBUG

    fDirectiveOK = TRUE;		// directives must start line
}

// ************************************************************************
// Token scanning routines
// ************************************************************************

VOID FAR ConsumeTok
(
TOKID	id,
WORD	fAccept
)
{
    RWTABLE * prgIds;

    if (tok.id != id)
	{
	    // syntax error -- lookup expected constant/RW in rgRws, for better
	    // error reporting.

	    prgIds = rgRws;	// just look in RW table
	    // CONSIDER: Do I need to look in rgAttrs too -- I don't think anybody is
	    // CONSIDER: looking for a specific attribute right now.
  	    while (prgIds->id != id)
  	        {
  	            Assert(prgIds->id != RW_NOTFOUND);   // had better be in the RW table
		    prgIds++;
		}
	    szExpected = prgIds->sz;
	    Assert(szExpected);		// should have found this name
	    ParseError(PERR_EXPECTED);
	}
    ScanTok(fAccept);
}


// fills token structure with next token, subject to context
VOID FAR ScanTok
(
WORD	fAccept
)
{
    CHAR ch;

    lnLast = lnCur;		// save info about previous token
    colLast = colCur;
    // CONSIDER: add a szFileLast (might be slow, however);

top:
    ch = *pLine++;		// ch = next char in line
    colCur = (WORD)(pLine-pLineBuf); // save 1-based column # of current token
				// for error reporting

    switch (ch)
    {

    case '\0':			// EOF -- quit
        if (!(fAccept & fACCEPT_EOF))
	    ParseError(PERR_UNEXP_EOF);
	tok.id = RW_EOF;
	break;

    case '#':		// probably a #line n "filename"
	// if not first non-blank char on line, then give error
	if (!fDirectiveOK)
	    goto default_char;
        HandleDirective();
        // fall into code below, to get next line & continue

    case '\n':			// EOL -- advance to next line, if any
	FillLine();		// get next line
    	goto top;		// and continue
	break;

    case '[':
	tok.id = RW_LBRACKET;
	break;

    case ']':
	tok.id = RW_RBRACKET;
	break;

    case '{':
	tok.id = RW_LCURLY;
	break;

    case '}':
	tok.id = RW_RCURLY;
	break;

    case '(':
	tok.id = RW_LPAREN;
	break;

    case ')':
	tok.id = RW_RPAREN;
	break;

    case ';':
	tok.id = RW_SEMI;
	break;

    case ':':
	tok.id = RW_COLON;
	break;

    case '.':
	tok.id = RW_PERIOD;
	break;

    case ',':
	tok.id = RW_COMMA;
	break;

    case '=':
	tok.id = RW_ASSIGN;
	//CONSIDER: (V2, EXPR) handle "==" (OP_EQ) when fAccept & fACCEPT_OPERATOR
	break;

    case '*':
	tok.id = RW_POINTER;
	break;

    case '-':	// subtract operator or a hyphen (perhaps a unary minus)
	tok.id = (WORD)((fAccept & fACCEPT_OPERATOR) ? OP_SUB : RW_HYPHEN);
	break;

    case '"':			// start of string literal
        if (!(fAccept & fACCEPT_STRING)) // if unexpected string, give
	    goto default_char;		// proper error (expected number/id)
	HandleString();
	break;

    case '/':			// probably a start of a comment
	if (*pLine == '/')	// if "//"
	    {
		FillLine();	// get next line
		goto top;	// and continue
		break;
	    }
	else if (*pLine == '*')	// if "/*"
	    {
		pLine++;		// skip "*"
		while (*pLine)		// while not EOF
		    {

			pLine = XStrChr(pLine, '*');

			if (pLine == NULL)	// if no "*" found on this line
			    {
				FillLine();	// get next line
			    }
			else
			    if (*(++pLine) == '/')	// if found "*/"
				{
				    pLine++;	// skip the "/"
				    goto top;	// and continue
				}
		    }
		ParseError(PERR_UNTERMINATED_COMMENT);
	    }

	// otherwise, fall into default char processing


    default:
	if ((BYTE)ch <= ' ') // anything less than a space, just treat as white space
	    goto top;

default_char:
	if (fAccept & fACCEPT_UUID)
	    {		// if only looking for a UUID
		// parse 16-byte UUID constant
		CHAR * start;	// starting position
		#define CCH_SZGUID0  39  // chars in ascii guid (including NULL)
		CHAR buffer[CCH_SZGUID0];

		tok.lpUuid = (GUID FAR *)ParseMalloc(sizeof(GUID));

                start = --pLine;

		// use our number parsers to help validate the UUID
		HandleHexNum(8);
		if (*pLine++ != '-')
		    ParseError(PERR_INV_UUID);
		HandleHexNum(4);
		if (*pLine++ != '-')
		    ParseError(PERR_INV_UUID);
		HandleHexNum(4);
		if (*pLine++ != '-')
		    ParseError(PERR_INV_UUID);
		HandleHexNum(4);
		if (*pLine++ != '-')
		    ParseError(PERR_INV_UUID);
		HandleHexNum(8);
		HandleHexNum(4);
		if (pLine - start != (32+4))	// 32 digits + 4 hyphens
		    ParseError(PERR_INV_UUID);

		// get uuid in a string of the form:
		// {numbnumb-numb-numb-numb-numbnumbnumb} so OLE can use it.

		#define chClsPrefix '{'
		#define cbClsPrefix 1
		#define chClsSuffix '}'

		*buffer = chClsPrefix;
		_fmemcpy(buffer+cbClsPrefix, start, 32+4); 
		*(buffer+cbClsPrefix+32+4) = chClsSuffix;
		*(buffer+cbClsPrefix+32+5) = '\0';

		// have OLE translate this into it's special UUID format
#if FV_UNICODE_OLE
		{
		OLECHAR bufferW[CCH_SZGUID0];
	        SideAssert (MultiByteToWideChar(CP_ACP,
						MB_PRECOMPOSED,
						buffer,
						-1,
						bufferW,
						CCH_SZGUID0) != 0);
		SideAssert (!FAILED(CLSIDFromString(bufferW, (LPCLSID)tok.lpUuid)));
		}
#else  //FV_UNICODE_OLE
		SideAssert (!FAILED(CLSIDFromString(buffer, (LPCLSID)tok.lpUuid)));
#endif  //FV_UNICODE_OLE

		tok.id = LIT_UUID;
	        break;
	    }

	if (fAccept & fACCEPT_OPERATOR)
	    {	// if looking for an operator
		switch (ch)
		   {
		   case '+':
			tok.id = OP_ADD;
			break;

		   case '-':
			tok.id = OP_SUB;
			break;

		   case '*':
			tok.id = OP_MUL;
			break;

		   case '/':
			tok.id = OP_DIV;
			break;

		   case '%':
			tok.id = OP_MOD;
			break;

		   case '^':
			tok.id = OP_EXP;
			break;

		   case '!':
			tok.id = OP_LOG_NOT;
			break;

		   case '&':
			tok.id = OP_BIT_AND;
			//CONSIDER: (V2, EXPR) handle "&&" (OP_LOG_AND)
			break;

		   case '|':
			tok.id = OP_BIT_OR;
			//CONSIDER: (V2, EXPR) handle "||" (OP_LOG_OR)
			break;

		   case '~':
			tok.id = OP_BIT_NOT;
			break;

		   case '<':
			tok.id = OP_LT;
			//CONSIDER: (V2, EXPR) handle "<=" (OP_LE)
			//CONSIDER: (V2, EXPR) handle "<<" (OP_LSHIFT)
			break;

		   case '>':
			tok.id = OP_GT;
			//CONSIDER: (V2, EXPR) handle ">=" (OP_GE)
			//CONSIDER: (V2, EXPR) handle ">>" (OP_RSHIFT)
			break;

		   default:
			// valid chars to end expressions (such as a
			// right paren) should have been accepted above.
			ParseError(PERR_EXP_OPERATOR);
		   }

		break;	// got an operator -- exit
	    }

	if (fAccept & fACCEPT_NUMBER)
	    {	// if looking for a numeric value
		HandleNumericLit(ch);
		break;
	    }

	if (fAccept & fACCEPT_STRING)	 // error if looking for a string
	    ParseError(PERR_EXP_STRING); // and it wasn't found above

	HandleId(ch, fAccept);		// ID token of some sort
	break;				// (could be RW, ATTR, id, or type)

    }

    fDirectiveOK = FALSE;		// pre-processor directives
					// not allowed again until next line
}



VOID NEAR HandleString ()
{
    CHAR   szBuffer[CB_MAX_STRING+1];
    WORD   cbsz;
    CHAR * psz;
    CHAR   ch;

    psz = szBuffer;	// where to copy string
    cbsz = 0;		// no chars copied so far
    ch = *pLine++;		// ch = next char in line
    while (ch != '\"')
	{	// while not end of string
	    // error if premature EOL\EOF or buffer overflow
	    if (ch == '\n' || ch == '\0' || ++cbsz >= sizeof(szBuffer))
		ParseError(PERR_INV_STRING);
	    if (ch == '\\')		// handle escape sequences
		ch = HandleEscape();
	    *psz++ = ch;
	    //if ch is first byte of a DBCS char, then must copy another char
	    if (IsLeadByte(ch))
		{
		    if (++cbsz >= sizeof(szBuffer))	// 1 more byte in string
			ParseError(PERR_INV_STRING);
		    *psz++ = *pLine++;	// copy 2nd byte
		}
	    ch = *pLine++;		// ch = next char in line
	}
    *psz = '\0';		// null-terminate the data
    tok.cbsz = cbsz;		// save char count (not including null term)
    cbsz++;
    tok.lpsz = (LPSTR)ParseMalloc(cbsz);
    _fmemcpy(tok.lpsz, szBuffer, cbsz);
    tok.id = LIT_STRING;
}

VOID NEAR HandleId
(
    CHAR ch,
    WORD fAccept
)
{
    CHAR   szBuffer[CB_MAX_ID+1];
    WORD   cbsz;
    CHAR * psz;

    psz = szBuffer;	// where to copy ID
    cbsz = 0;		// no chars copied so far
    if (!FValidIdCh(ch, TRUE))
	{
	    tok.id = RW_NOTFOUND;	// not a valid id -- return
	    return;
	}
    do
	{
	    if (++cbsz >= sizeof(szBuffer))
		ParseError(PERR_INV_IDENTIFIER);
	    *psz++ = ch;		// copy char
	    //if ch is first byte of a DBCS char, then
	    //copy another char and keep looping. 
	    if (IsLeadByte(ch))
		{
		    if (++cbsz >= sizeof(szBuffer))
		        ParseError(PERR_INV_IDENTIFIER);
		    *psz++ = *pLine++;	// then copy 2nd byte

		}

            ch = *pLine++;	// ch = next char in ID
	} while (FValidIdCh(ch, FALSE));	// while more chars in id

    *psz = '\0';		// null-terminate the data
    cbsz++;                     // include NULL in count
    pLine--; 			// back up to char that terminated id

    tok.id = 0;
    if (fAccept & fACCEPT_ATTR)
	{	// see if looking for attribute
		// returns appropriate ATTR_xxx or 0 if not found
	    tok.id = RwLookup(szBuffer, rgAttrs);
	}

    if (tok.id == 0)	// see if normal RW
	tok.id = RwLookup(szBuffer, rgRws);

    if (tok.id == 0)		// if not any RW, must be an ID
	{
	    tok.lpsz = (LPSTR)ParseMalloc(cbsz);
	    _fmemcpy(tok.lpsz, szBuffer, cbsz);
	    tok.id = LIT_ID;
	}
}


// Translate escape char.  The following are supported:
//	\0, \a, \b, \f, \n, \r, \t, \v
//
// (use hard-coded values so using mktyplib on different platforms will
//  produce identical typelibs)
//
CHAR NEAR HandleEscape ()
{
   CHAR ch;

   ch = *pLine++;	// get char after "\"

   switch (ch)
	{
	case '0':
	    return 0x00;
	case 'a':
	    return 0x07;
	case 'b':
	    return 0x08;
	case 'f':
	    return 0x0c;
	case 'n':
            if (SysKind == SYS_MAC) 
                return 0x0d;
            else
                return 0x0a;  
	case 'r':
            if (SysKind == SYS_MAC)
                return 0x0a;
            else
                return 0x0d;
	case 't':
	    return 0x09;
	case 'v':
	    return 0x0b;
	default:
	    return ch;		// otherwise, just treat as char we got
	}
}


// returns TRUE if char is A-Z, a-z, underscore, or >= 128
BOOL NEAR FValidIdCh
(
    CHAR ch,
    BOOL fFirst
)
{
    // Assumes all DBCS lead bytes are > 128.  This seems to be true.

    // NOTE: using ch < 0 instead of ch >= 128, because ch is SIGNED.
    return (isalpha(ch) || ch < 0 || ch == '_' || (!fFirst && isdigit(ch)));
}


VOID NEAR HandleNumericLit
(
    CHAR ch
)
{
    DWORD prevVal;

    tok.number = 0;
    tok.id = LIT_NUMBER;		// assume valid number
    if (ch == '0' && *pLine == 'x')	// parse hex constant
	{
	pLine++;			// advance to first hex digit
	tok.number = HandleHexNum(8);	// input up to a 8-digit hex number
	if (isxdigit(*pLine))		// error if more digits left
	    ParseError(PERR_INV_NUMBER);
	}
    else if (isdigit(ch))		// parse decimal constant
	{
#pragma warning(disable:4127)
	while (TRUE)
#pragma warning(default:4127)
	    {
		prevVal = tok.number;
		if (prevVal > 429496729L)	//error if *10 will overflow
		    ParseError(PERR_NUMBER_OV);
		tok.number *= 10;
		tok.number += (ch - '0');
		if (tok.number < prevVal) // error if overflow
		    ParseError(PERR_NUMBER_OV);
		ch = *pLine;		// get next char
		if (!isdigit(ch))	// if next char not digit, we're done
		    break;
		pLine++;               // consume digit
	    };
	}

    else if (ch == '\'')
	{   // support for numeric literals of the form:  'a' or '\0'.
	    ch = *pLine++;		// ch = char after quote
	    if (ch == '\\')		// handle escape sequences
	        ch = HandleEscape();
	    tok.number = (BYTE)ch;	// don't sign-extend
	    if (*pLine++ != '\'')	// error if no close quote
		{
		    //CONSIDER: if we're to support 4-char literals, then code
		    //CONSIDER: must be added here.  Issues with 4-char literals
		    //CONSIDER: are byte order and whether we need them or not.
	            ParseError(PERR_INV_NUMBER);
		}
	}

    else
	ParseError(PERR_EXP_NUMBER);
}


DWORD NEAR HandleHexNum
(
   WORD cDigitsMax
)
{
    DWORD result = 0;
    CHAR  ch;
    WORD  digit;

    if (!isxdigit(*pLine))		// error if no hex digits
	ParseError(PERR_INV_NUMBER);

    while (isxdigit(ch = *pLine) && cDigitsMax--)
	{
	    pLine++;
	    result = result << 4;

	    digit = ch;
	    if (ch < 'A')
		digit -= '0';
	    else if (ch < 'a')
		digit -= ('A'-10);
	    else
		digit -= ('a'-10);
	    result += digit;
	}

    return result;
}


VOID NEAR HandleDirective()
{
    fDirectiveOK = FALSE;		// don't allow nested directives
    if (memcmp(pLine, "line ", 5) == 0)	// if got #line
	{   // handle #line <line #> "<filename>"
	    // UNDONE: (CPP) Do all pre-processors emit #line directives in
	    // UNDONE: (CPP) the above format?

	    pLine+= 5;			// skip "line "
	    ScanTok(fACCEPT_NUMBER);    // read in the new current line number
	    if (tok.id != LIT_NUMBER)
	        ParseError(PERR_EXP_NUMBER);   // error
	    lnCur = tok.number - 1;	// set new current line # (-1 because
	    				// FillLine will increment it)
	    ScanTok(fACCEPT_STRING);	// get current file name
	    if (tok.id != LIT_STRING)
	        ParseError(PERR_EXP_STRING);   // error
	    _fstrcpy(szFileCur, tok.lpsz);  // save current file name
	    _ffree(tok.lpsz);		// free unused memory
	}
    // Just ignore any other directives
    // CONSIDER: (CPP) Maybe give warning instead of ignoring other directives?
}


// *********************************************************************
// utility routines
// *********************************************************************


// if sz in rgIds, return appropriate constant.
TOKID NEAR RwLookup
(
    CHAR * psz,
    RWTABLE * prgIds
)
{

    TOKID id;

#pragma warning(disable:4127)
    while (TRUE)
#pragma warning(default:4127)
        {
  	    id = prgIds->id;
	    if (prgIds->sz == NULL)
		break;		// quit if end of table
	    if (strcmp(prgIds->sz, psz) == 0)
		break;		// quit if found match
	    prgIds++;
        }

    return id;
}

// cover for fmalloc -- calls ParseError(ERR_OM) if alloc fails
LPVOID FAR ParseMalloc
(
    WORD cbAlloc
)
{
    LPVOID retVal;

    if ((retVal = _fmalloc(cbAlloc)) == NULL)
	ParseError(ERR_OM);

    return retVal;
}


// XStrChr:  perform strchr on SBCS or DBCS string
extern char * FAR XStrChr
(
    char * xsz,
    int ch
)
{
    char * pchFind = NULL;

    while(*xsz != '\0') {
      if (*xsz == ch)
        pchFind = xsz;
      if (IsLeadByte(*xsz)) {
        pchFind = NULL;
        xsz++;
        if (*xsz == '\0')
          break;
      }
      xsz++;
    }
    return pchFind;

}
