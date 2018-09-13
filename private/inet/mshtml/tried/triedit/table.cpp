// table.cpp
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
// HTML keyword tables
// If you modify the element, attribute, or entity tables, then you MUST 
// update Token.h.


#include "stdafx.h"

#include "resource.h"
#include "guids.h"
#include "table.h"

#undef ASSERT
#define ASSERT(b) _ASSERTE(b)

// qsort/bsearch helper
int CmpFunc(const void *a, const void *b);

static const TCHAR szFileSig[] = _T("@HLX@");
static const TCHAR szElTag[]   = _T("[Elements]");
static const TCHAR szAttTag[]  = _T("[Attributes]");
static const TCHAR szEntTag[]  = _T("[Entities]");

////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
//
// table verification routines
//
int CheckWordTable(ReservedWord *arw, int cel, LPCTSTR szName /*= NULL*/)
{
	int cerr = 0;
	int cch;
	for (int i = 0; i < cel; i++)
	{
		// table must be sorted in ascending alpha order
		//
		if (i > 1)
		{
			if (!(_tcscmp(arw[i-1].psz, arw[i].psz) < 0))
			{
				ATLTRACE(_T("lexer:entries in %s out of order at %d: %s - %s\n"),
					szName?szName:_T("?"), i-1, arw[i-1].psz, arw[i].psz);
				cerr++;
			}
		}

		// length must match
		//
		cch = _tcslen(arw[i].psz);
		if (cch != arw[i].cb)
		{
			ATLTRACE(_T("lexer:Incorrect entry in %s: %s,%d should be %d\n"),
				szName?szName:_T("?"), arw[i].psz, arw[i].cb, cch);
			cerr++;
		}
	}
	return cerr;
}

int CheckWordTableIndex(ReservedWord *arw, int cel, int *ai, BOOL bCase /*= FALSE*/, LPCTSTR szName /* = NULL*/)
{
	int cerr = 0;
	int index;
	int max = bCase ? 52 : 26;

	_ASSERTE(NULL != arw);
	_ASSERTE(NULL != ai);

	int aik[] =
	{
		//A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
		  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		//a b c d e f g h i j k l m n o p q r s t u v w x y z
		  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	// Build correct index array on static aik
	cerr += MakeIndexHere( arw, cel, aik, bCase );

	// Compare with declared arrray
	//
	if (0 != memcmp(aik, ai, max))
		cerr++;

	// If errors, dump correct array
	if (cerr)
	{
		ATLTRACE(_T("lexer: Correct index array for %s: \n{\n\t"), szName ? szName : _T("?"));
		for (index = 0; index < max - 1; index++)
		{
			ATLTRACE(_T("%3d,"), aik[index]);
			if (index % 13 == 12)
				ATLTRACE(_T("\n\t"));
		}
		ATLTRACE(_T("%3d\n};\n"), aik[index]);
	}
	return cerr;
}
#endif

int MakeIndexHere(ReservedWord *arw, int cel, int *ab, BOOL bCase /*= FALSE*/, LPCTSTR szName /*= NULL*/)
{
	int cerr = 0;
	int index;

	ASSERT(ab != NULL);
	ASSERT(arw != NULL);

	for (int irw = cel - 1; irw > 0; irw--)
	{
		index = PeekIndex(*arw[irw].psz, bCase);
		if (-1 != index)
			ab[index] = irw;
		else
		{
			ATLTRACE(_T("lexer:error in %s: non-alpha token %s\n"), szName?szName:_T("?"), arw[irw].psz);
			cerr++;
		}
	}
	return cerr;
}

int MakeIndex(ReservedWord *arw, int cel, int **pab, BOOL bCase /*= FALSE*/, LPCTSTR szName /*= NULL*/)
{
	ASSERT(NULL != arw);
	ASSERT(NULL != pab);
	*pab = new int[ bCase ? 52 : 26 ];
	if (NULL == *pab)
		return -1;
	return MakeIndexHere(arw, cel, *pab, bCase, szName);
}

#define RW_Entry(string, attribute) \
	_T( #string ), sizeof( #string ) - 1, attribute

////////////////////////////////////////////////////////////////////////////
// reserved word table
// Two tables:
//   reserved[] = sorted table of reserved words
//   index[initial(token)] = (index of first reserved word with that initial)
// If you modify the element, attribute, or entity tables, then you MUST 
// update Token.h.
//
// NOTE: the "HPn" elements are considered obsolete
//
static ReservedWord _rgElementNames[] =
{// psz             cb      att
	_T(""),	0, 0,
	RW_Entry(A 				,ALL	),
	RW_Entry(ADDRESS 		,ALL	),
	RW_Entry(APPLET 		,IEXPn	), // ???
	RW_Entry(AREA 			,IEXPn	),
	RW_Entry(B 				,ALL	),
	RW_Entry(BASE 			,ALL	),
	RW_Entry(BASEFONT 		,IEXPn	),
	RW_Entry(BGSOUND 		,IEXPn	), // IExplore
  	RW_Entry(BIG 			,IEXP3	),
	RW_Entry(BLINK 			,IEXPn	), // Netscape
	RW_Entry(BLOCKQUOTE 	,ALL	),
	RW_Entry(BODY 			,ALL	),
	RW_Entry(BR 			,ALL	),
	RW_Entry(BUTTON 		,IE40	),
	RW_Entry(CAPTION 		,IEXPn	), // tables
	RW_Entry(CENTER 		,IEXPn	),
	RW_Entry(CITE 			,ALL	),
	RW_Entry(CODE 			,ALL	),
	RW_Entry(COL 			,IEXP3	),
	RW_Entry(COLGROUP 		,IEXP3	), // HTML 3 tables?
	RW_Entry(COMMENT 		,ALL	), // considered obsolete
	RW_Entry(DD 			,ALL	),
	RW_Entry(DFN 			,ALL	), // RFC1866: not in the RFC, but deployed. ital or bold ital
	RW_Entry(DIR 			,ALL	),
	RW_Entry(DIV 			,IEXP3	),     // HTML 3
	RW_Entry(DL 			,ALL	),
	RW_Entry(DT 			,ALL	),
	RW_Entry(EM 			,ALL	),
	RW_Entry(EMBED 			,IEXP3	), // netscape -- IEXP3
	RW_Entry(FIELDSET 		,IE40	),
	RW_Entry(FONT 			,IEXPn	),
	RW_Entry(FORM 			,ALL	),   // forms
	RW_Entry(FRAME 			,IEXP3	), // framesets
	RW_Entry(FRAMESET 		,IEXP3	), // framesets
	RW_Entry(H1 			,ALL	), // heading 1
	RW_Entry(H2 			,ALL	), // heading 2
	RW_Entry(H3 			,ALL	), // heading 3
	RW_Entry(H4 			,ALL	), // heading 4
	RW_Entry(H5 			,ALL	), // heading 5
	RW_Entry(H6 			,ALL	), // heading 6
	RW_Entry(HEAD 			,ALL	), // document head
	RW_Entry(HR 			,ALL	),
	RW_Entry(HTML 			,ALL	),
	RW_Entry(I 				,ALL	),
	RW_Entry(IFRAME 		,IEXP3	),	// inline frames
	RW_Entry(IMG 			,ALL	),
	RW_Entry(INPUT 			,ALL	), // forms
	RW_Entry(ISINDEX 		,ALL	),
	RW_Entry(KBD 			,ALL	),
	RW_Entry(LABEL 			,IE40	),
	RW_Entry(LEGEND 		,IE40	),
	RW_Entry(LI 			,ALL	),
	RW_Entry(LINK 			,ALL	),
	RW_Entry(LISTING 		,ALL	), // RFC 1866: obsolete
	RW_Entry(MAP 			,IEXPn	),
	RW_Entry(MARQUEE 		,IEXPn	), // IExplore
	RW_Entry(MENU 			,ALL	),
	RW_Entry(META 			,ALL	),
	RW_Entry(METADATA 		,ALL	),
	RW_Entry(NOBR 			,IEXPn	),
	RW_Entry(NOFRAMES 		,IEXP3	), // framesets
	RW_Entry(NOSCRIPT 		,IE40	), // IE4 only
	RW_Entry(OBJECT 		,IEXP3	), // ActiveX
	RW_Entry(OL 			,ALL	),
	RW_Entry(OPTION 		,ALL	),   // forms
	RW_Entry(P 				,ALL	),
	RW_Entry(PARAM 			,IEXP3	), // ActiveX
	RW_Entry(PLAINTEXT 		,ALL	),   // RFC 1866: deprecated, noted as obsolete
	RW_Entry(PRE 			,ALL	),
	RW_Entry(S 				,IEXPn	), // (apparently) synonym for strike
	RW_Entry(SAMP 			,ALL	),
	RW_Entry(SCRIPT 		,IEXP3	), // ActiveX
	RW_Entry(SELECT 		,ALL	),
	RW_Entry(SMALL 			,IEXP3	),
	RW_Entry(SPAN 			,IEXP3	), // tables
	RW_Entry(STRIKE 		,IEXPn	), // not in RFC 1866 DTD, but noted as deployed
	RW_Entry(STRONG 		,ALL	),
    RW_Entry(STYLE 			,IEXP3	), // HTML 3 stylesheets
	RW_Entry(SUB 			,IEXP3	), // HTML3 ???
	RW_Entry(SUP 			,IEXP3	), // HTML3 ???
	RW_Entry(TABLE 			,IEXPn	), // tables
	RW_Entry(TBODY 			,IEXP3	), // HTML 3 tables
	RW_Entry(TD 			,IEXPn	), // tables
	RW_Entry(TEXTAREA 		,ALL	),   // forms
	RW_Entry(TFOOT 			,IEXP3	), // HTML 3 tables
	RW_Entry(TH 			,IEXPn	), // tables
	RW_Entry(THEAD 			,IEXP3	), // HTML 3 tables
	RW_Entry(TITLE 			,ALL	),
	RW_Entry(TR 			,IEXPn	), // tables
	RW_Entry(TT 			,ALL	),
	RW_Entry(U 				,ALL	),   // not in RFC 1866 DTD, but noted as deployed
	RW_Entry(UL 			,ALL	),
	RW_Entry(VAR 			,ALL	),
	RW_Entry(WBR 			,IEXPn	),
	RW_Entry(XMP 			,ALL	),   // deprecated by RFC 1866
};

// If you modify the element, attribute, or entity tables, then you MUST 
// update Token.h.
// The following array is a mapping of each letter to a position in the
// table where tokens starting with that letter begin.
//
static int _rgIndexElementNames[] = // [Elements]
{
	/* A */ TokElem_A			,
	/* B */ TokElem_B			,
	/* C */	TokElem_CAPTION		,
	/* D */	TokElem_DD			,
	/* E */	TokElem_EM			,
	/* F */	TokElem_FIELDSET	,
	/* G */	0					,
	/* H */	TokElem_H1			,
	/* I */	TokElem_I			,
	/* J */	0					,
	/* K */	TokElem_KBD			,
	/* L */	TokElem_LABEL		,
	/* M */	TokElem_MAP			,
	/* N */	TokElem_NOBR		,
	/* O */	TokElem_OBJECT		,
	/* P */	TokElem_P			,
	/* Q */	0					,
	/* R */	0					,
	/* S */	TokElem_S			,
	/* T */	TokElem_TABLE		,
	/* U */	TokElem_U			,
	/* V */	TokElem_VAR			,
	/* W */	TokElem_WBR			,
	/* X */	TokElem_XMP			,
	/* Y */	0					,
	/* Z */	0
};

// If you modify the element, attribute, or entity tables, then you MUST 
// update Token.h.

//
// attribute name table
//
static ReservedWord _rgAttributeNames[] =
{// psz                 cb   att
	_T(""), 0, 0,
	RW_Entry(ACCESSKEY					,IEXP3	),
	RW_Entry(ACTION						,ALL	),
	RW_Entry(ALIGN						,ALL	),
	RW_Entry(ALINK						,IEXPn	),
	RW_Entry(ALT						,ALL	),
	RW_Entry(APPNAME					,IE40	),
	RW_Entry(APPVERSION					,IE40	),
	RW_Entry(BACKGROUND					,IEXPn	),
	RW_Entry(BACKGROUNDATTACHMENT		,IE40	),
	RW_Entry(BACKGROUNDCOLOR			,IE40	),
	RW_Entry(BACKGROUNDIMAGE			,IE40	),
	RW_Entry(BACKGROUNDPOSITION			,IE40	),
	RW_Entry(BACKGROUNDPOSITIONX		,IE40	),
	RW_Entry(BACKGROUNDPOSITIONY		,IE40	),
	RW_Entry(BACKGROUNDREPEAT			,IE40	),
	RW_Entry(BALANCE					,IE40	),
	RW_Entry(BEHAVIOR					,IEXPn	), // MARQUEE
	RW_Entry(BGCOLOR					,IEXPn	),
	RW_Entry(BGPROPERTIES				,IEXPn	),
	RW_Entry(BORDER						,IEXPn	),
	RW_Entry(BORDERBOTTOM				,IE40	),
	RW_Entry(BORDERBOTTOMCOLOR			,IE40	),
	RW_Entry(BORDERBOTTOMSTYLE			,IE40	),
	RW_Entry(BORDERBOTTOMWIDTH			,IE40	),
	RW_Entry(BORDERCOLOR				,IEXPn	), // tables
	RW_Entry(BORDERCOLORDARK			,IEXPn	), // tables
	RW_Entry(BORDERCOLORLIGHT			,IEXPn	), // tables
	RW_Entry(BORDERLEFT					,IE40	),
	RW_Entry(BORDERLEFTCOLOR			,IE40	),
	RW_Entry(BORDERLEFTSTYLE			,IE40	),
	RW_Entry(BORDERLEFTWIDTH			,IE40	),
	RW_Entry(BORDERRIGHT				,IE40	),
	RW_Entry(BORDERRIGHTCOLOR			,IE40	),
	RW_Entry(BORDERRIGHTSTYLE			,IE40	),
	RW_Entry(BORDERRIGHTWIDTH			,IE40	),
	RW_Entry(BORDERSTYLE				,IE40	),
	RW_Entry(BORDERTOP					,IE40	),
	RW_Entry(BORDERTOPCOLOR				,IE40	),
	RW_Entry(BORDERTOPSTYLE				,IE40	),
	RW_Entry(BORDERTOPWIDTH				,IE40	),
	RW_Entry(BORDERWIDTH				,IE40	),
	RW_Entry(BOTTOMMARGIN				,IEXPn	),
	RW_Entry(BREAKPOINT					,IEXPn	), // (walts) hidden META tag attribute for brkpt mapping.
	RW_Entry(BUFFERDEPTH				,IE40	),
	RW_Entry(BUTTON						,IE40	),
	RW_Entry(CANCELBUBBLE				,IE40	),
	RW_Entry(CELLPADDING				,IEXPn	), // tables
	RW_Entry(CELLSPACING				,IEXPn	), // tables
	RW_Entry(CENTER						,IEXPn	),
	RW_Entry(CHARSET					,IE40	),
	RW_Entry(CHECKED					,ALL	),
	RW_Entry(CLASS						,IEXPn	),
	RW_Entry(CLASSID					,IEXP3	), //objects
	RW_Entry(CLASSNAME					,IE40	),
	RW_Entry(CLEAR						,IEXP3	),
	RW_Entry(CLIP						,IE40	),
	RW_Entry(CODE						,IEXPn	),
	RW_Entry(CODEBASE					,IEXP3	), //objects
	RW_Entry(CODETYPE					,IE40	),
	RW_Entry(COLOR						,IEXPn	), // font
	RW_Entry(COLORDEPTH					,IE40	),
	RW_Entry(COLS						,ALL	),
	RW_Entry(COLSPAN					,IEXPn	), // tables
	RW_Entry(COMPACT					,ALL	),
	RW_Entry(COMPLETE					,IE40	),
	RW_Entry(CONTENT					,ALL	),
	RW_Entry(CONTROLS					,IEXPn	),
	RW_Entry(COOKIE						,IE40	),
	RW_Entry(COOKIEENABLED				,IE40	),
	RW_Entry(COORDS						,IEXPn	),
	RW_Entry(CSSTEXT					,IE40	),
	RW_Entry(CTRLKEY					,IE40	),
	RW_Entry(CURSOR						,IE40	),
	RW_Entry(DATA						,IEXP3	), //objects
	RW_Entry(DATAFLD					,IE40	),
	RW_Entry(DATAFORMATAS				,IE40	),
	RW_Entry(DATAPAGESIZE				,IE40	),
	RW_Entry(DATASRC					,IE40	),
	RW_Entry(DECLARE					,IEXP3	), //objects
	RW_Entry(DEFAULTCHECKED				,IE40	),
	RW_Entry(DEFAULTSELECTED			,IE40	),
	RW_Entry(DEFAULTSTATUS				,IE40	),
	RW_Entry(DEFAULTVALUE				,IE40	),
	RW_Entry(DIALOGARGUMENTS			,IE40	),
	RW_Entry(DIALOGHEIGHT				,IE40	),
	RW_Entry(DIALOGLEFT					,IE40	),
	RW_Entry(DIALOGTOP					,IE40	),
	RW_Entry(DIALOGWIDTH				,IE40	),
	RW_Entry(DIR						,IEXP3	), // HTML 3 ???
	RW_Entry(DIRECTION					,IEXPn	), // MARQUEE
	RW_Entry(DISABLED					,IE40	),
	RW_Entry(DISPLAY					,IE40	),
	RW_Entry(DOMAIN						,IE40	),
	RW_Entry(DYNSRC						,IEXPn	),
	RW_Entry(ENCODING					,IE40	),
	RW_Entry(ENCTYPE					,ALL	),
	RW_Entry(ENDSPAN					,IE40	),	// Designer control tags
	RW_Entry(ENDSPAN--					,IE40	),	// Designer control tags HACK to handle nonspace
	RW_Entry(EVENT						,IEXP3	), // ActiveX <SCRIPT>
	RW_Entry(FACE						,IEXPn	), // font
	RW_Entry(FGCOLOR					,IE40	),
	RW_Entry(FILTER						,IE40	),
	RW_Entry(FONT						,IE40	),
	RW_Entry(FONTFAMILY					,IE40	),
	RW_Entry(FONTSIZE					,IE40	),
	RW_Entry(FONTSTYLE					,IE40	),
	RW_Entry(FONTVARIANT				,IE40	),
	RW_Entry(FONTWEIGHT					,IE40	),
	RW_Entry(FOR						,IEXP3	), // ActiveX <SCRIPT>
	RW_Entry(FORM						,IE40	),
	RW_Entry(FRAME						,IE40	),
	RW_Entry(FRAMEBORDER				,IEXP3	),
	RW_Entry(FRAMESPACING				,IEXP3	),
	RW_Entry(FROMELEMENT				,IE40	),
	RW_Entry(HASH						,IE40	),
	RW_Entry(HEIGHT						,IEXPn	),
	RW_Entry(HIDDEN						,IE40	),
	RW_Entry(HOST						,IE40	),
	RW_Entry(HOSTNAME					,IE40	),
	RW_Entry(HREF						,ALL	),
	RW_Entry(HSPACE						,IEXPn	),
	RW_Entry(HTMLFOR					,IE40	),
	RW_Entry(HTMLTEXT					,IE40	),
	RW_Entry(HTTP-EQUIV					,ALL	),
	RW_Entry(HTTPEQUIV					,IE40	),
	RW_Entry(ID							,IEXPn	),
	RW_Entry(IN							,IEXP3	), // ActiveX <SCRIPT>
	RW_Entry(INDETERMINATE				,IE40	),
	RW_Entry(INDEX						,IE40	),
	RW_Entry(ISMAP						,ALL	),
	RW_Entry(LANG						,IEXPn	),
	RW_Entry(LANGUAGE					,IEXP3	),
	RW_Entry(LEFTMARGIN					,IEXPn	),
	RW_Entry(LENGTH						,IE40	),
	RW_Entry(LETTERSPACING				,IE40	),
	RW_Entry(LINEHEIGHT					,IE40	),
	RW_Entry(LINK						,IEXPn	),
	RW_Entry(LINKCOLOR					,IE40	),
	RW_Entry(LISTSTYLE					,IE40	),
	RW_Entry(LISTSTYLEIMAGE				,IE40	),
	RW_Entry(LISTSTYLEPOSITION			,IE40	),
	RW_Entry(LISTSTYLETYPE				,IE40	),
	RW_Entry(LOCATION					,IE40	),
	RW_Entry(LOOP						,IEXPn	),
	RW_Entry(LOWSRC						,IE40	),
	RW_Entry(MAP						,IE40	),
	RW_Entry(MARGIN						,IE40	),
	RW_Entry(MARGINBOTTOM				,IE40	),
	RW_Entry(MARGINHEIGHT				,IEXP3	),
	RW_Entry(MARGINLEFT					,IE40	),
	RW_Entry(MARGINRIGHT				,IE40	),
	RW_Entry(MARGINTOP					,IE40	),
	RW_Entry(MARGINWIDTH				,IEXP3	),
	RW_Entry(MAXLENGTH					,ALL	),
	RW_Entry(METHOD						,ALL	),
	RW_Entry(METHODS					,ALL	),
	RW_Entry(MIMETYPES					,IE40	),
	RW_Entry(MULTIPLE					,ALL	),
	RW_Entry(NAME						,ALL	),
	RW_Entry(NOHREF						,IEXPn	),
	RW_Entry(NORESIZE					,IEXP3	),
	RW_Entry(NOSHADE					,IEXP3	), // not implemented by IExplore 2
	RW_Entry(NOWRAP						,IEXPn	),
	RW_Entry(OBJECT						,IEXP3	), // <PARAM>
	RW_Entry(OFFSCREENBUFFERING			,IE40	),
	RW_Entry(OFFSETHEIGHT				,IE40	),
	RW_Entry(OFFSETLEFT					,IE40	),
	RW_Entry(OFFSETPARENT				,IE40	),
	RW_Entry(OFFSETTOP					,IE40	),
	RW_Entry(OFFSETWIDTH				,IE40	),
	RW_Entry(OFFSETX					,IE40	),
	RW_Entry(OFFSETY					,IE40	),
	RW_Entry(ONABORT					,IE40	),
	RW_Entry(ONAFTERUPDATE				,IE40	),
	RW_Entry(ONBEFOREUNLOAD				,IE40	),
	RW_Entry(ONBEFOREUPDATE				,IE40	),
	RW_Entry(ONBLUR						,IEXP3	), // SELECT, INPUT, TEXTAREA
	RW_Entry(ONBOUNCE					,IE40	),
	RW_Entry(ONCHANGE					,IEXP3	), // SELECT, INPUT, TEXTAREA
	RW_Entry(ONCLICK					,IEXP3	), // INPUT, A, <more>
	RW_Entry(ONDATAAVAILABLE			,IE40	),
	RW_Entry(ONDATASETCHANGED			,IE40	),
	RW_Entry(ONDATASETCOMPLETE			,IE40	),
	RW_Entry(ONDBLCLICK					,IE40	),
	RW_Entry(ONDRAGSTART				,IE40	),
	RW_Entry(ONERROR					,IE40	),
	RW_Entry(ONERRORUPDATE				,IE40	),
	RW_Entry(ONFILTERCHANGE				,IE40	),
	RW_Entry(ONFINISH					,IE40	),
	RW_Entry(ONFOCUS					,IEXP3	), // SELECT, INPUT, TEXTAREA
	RW_Entry(ONHELP						,IE40	),
	RW_Entry(ONKEYDOWN					,IE40	),
	RW_Entry(ONKEYPRESS					,IE40	),
	RW_Entry(ONKEYUP					,IE40	),
	RW_Entry(ONLOAD						,IEXP3	), // FRAMESET, BODY
	RW_Entry(ONMOUSEOUT					,IEXP3	), // A, AREA, <more>
	RW_Entry(ONMOUSEOVER				,IEXP3	), // A, AREA, <more>
	RW_Entry(ONMOUSEUP					,IE40	),
	RW_Entry(ONREADYSTATECHANGE			,IE40	),
	RW_Entry(ONRESET					,IE40	),
	RW_Entry(ONRESIZE					,IE40	),
	RW_Entry(ONROWENTER					,IE40	),
	RW_Entry(ONROWEXIT					,IE40	),
	RW_Entry(ONSCROLL					,IE40	),
	RW_Entry(ONSELECT					,IEXP3	), // INPUT, TEXTAREA
	RW_Entry(ONSELECTSTART				,IE40	),
	RW_Entry(ONSUBMIT					,IEXP3	), // FORM
	RW_Entry(ONUNLOAD					,IEXP3	), // FRAMESET, BODY
	RW_Entry(OPENER						,IE40	),
	RW_Entry(OUTERHTML					,IE40	),
	RW_Entry(OUTERTEXT					,IE40	),
	RW_Entry(OUTLINE					,IEXP3	),
	RW_Entry(OVERFLOW					,IE40	),
	RW_Entry(OWNINGELEMENT				,IE40	),
	RW_Entry(PADDING					,IE40	),
	RW_Entry(PADDINGBOTTOM				,IE40	),
	RW_Entry(PADDINGLEFT				,IE40	),
	RW_Entry(PADDINGRIGHT				,IE40	),
	RW_Entry(PADDINGTOP					,IE40	),
	RW_Entry(PAGEBREAKAFTER				,IE40	),
	RW_Entry(PAGEBREAKBEFORE			,IE40	),
	RW_Entry(PALETTE					,IE40	),
	RW_Entry(PARENT						,IE40	),
	RW_Entry(PARENTELEMENT				,IE40	),
	RW_Entry(PARENTSTYLESHEET			,IE40	),
	RW_Entry(PARENTTEXTEDIT				,IE40	),
	RW_Entry(PARENTWINDOW				,IE40	),
	RW_Entry(PATHNAME					,IE40	),
	RW_Entry(PIXELHEIGHT				,IE40	),
	RW_Entry(PIXELLEFT					,IE40	),
	RW_Entry(PIXELTOP					,IE40	),
	RW_Entry(PIXELWIDTH					,IE40	),
	RW_Entry(PLUGINS					,IE40	),
	RW_Entry(PLUGINSPAGE				,IE40	),
	RW_Entry(PORT						,IE40	),
	RW_Entry(POSHEIGHT					,IE40	),
	RW_Entry(POSITION					,IE40	),
	RW_Entry(POSLEFT					,IE40	),
	RW_Entry(POSTOP						,IE40	),
	RW_Entry(POSWIDTH					,IE40	),
	RW_Entry(PROMPT						,IEXPn	),
	RW_Entry(PROTOCOL					,IE40	),
	RW_Entry(READONLY					,IE40	),
	RW_Entry(READYSTATE					,IE40	),
	RW_Entry(REASON						,IE40	),
	RW_Entry(RECORDNUMBER				,IE40	),
	RW_Entry(RECORDSET					,IE40	),
	RW_Entry(REF						,IEXP3	),
	RW_Entry(REFERRER					,IE40	),
	RW_Entry(REL						,ALL	),
	RW_Entry(RETURNVALUE				,IE40	),
	RW_Entry(REV						,ALL	),
	RW_Entry(RIGHTMARGIN				,IEXPn	),
	RW_Entry(ROWS						,ALL	),
	RW_Entry(ROWSPAN					,IEXPn	), // tables
	RW_Entry(RULES						,IEXP3	),
	RW_Entry(RUNAT						,IEXP3	), // SCRIPT
	RW_Entry(SCREENX					,IE40	),
	RW_Entry(SCREENY					,IE40	),
	RW_Entry(SCRIPTENGINE				,IEXP3	),
	RW_Entry(SCROLL						,IE40	),
	RW_Entry(SCROLLAMOUNT				,IEXPn	), // MARQUEE
	RW_Entry(SCROLLDELAY				,IEXPn	), // MARQUEE
	RW_Entry(SCROLLHEIGHT				,IE40	),
	RW_Entry(SCROLLING					,IEXP3	), // frameset
	RW_Entry(SCROLLLEFT					,IE40	),
	RW_Entry(SCROLLTOP					,IE40	),
	RW_Entry(SCROLLWIDTH				,IE40	),
	RW_Entry(SEARCH						,IE40	),
	RW_Entry(SELECTED					,ALL	),
	RW_Entry(SELECTEDINDEX				,IE40	),
	RW_Entry(SELF						,IE40	),
	RW_Entry(SHAPE						,IEXPn	),
	RW_Entry(SHAPES						,IEXP3	), //objects
	RW_Entry(SHIFTKEY					,IE40	),
	RW_Entry(SIZE						,ALL	),
	RW_Entry(SOURCEINDEX				,IE40	),
	RW_Entry(SPAN						,IEXP3	),
	RW_Entry(SRC						,ALL	),
	RW_Entry(SRCELEMENT					,IE40	),
	RW_Entry(SRCFILTER					,IE40	),
	RW_Entry(STANDBY					,IEXP3	), //objects
	RW_Entry(START						,IEXPn	),
	RW_Entry(STARTSPAN					,ALL	),	// Designer control tags
	RW_Entry(STATUS						,IE40	),
	RW_Entry(STYLE						,IEXP3	),
	RW_Entry(STYLEFLOAT					,IE40	),
	RW_Entry(TABINDEX					,IEXP3	),
	RW_Entry(TAGNAME					,IE40	),
	RW_Entry(TARGET						,IEXP3	),
	RW_Entry(TEXT						,IEXPn	),
	RW_Entry(TEXTALIGN					,IE40	),
	RW_Entry(TEXTDECORATION				,IE40	),
	RW_Entry(TEXTDECORATIONBLINK		,IE40	),
	RW_Entry(TEXTDECORATIONLINETHROUGH	,IE40	),
	RW_Entry(TEXTDECORATIONNONE			,IE40	),
	RW_Entry(TEXTDECORATIONOVERLINE		,IE40	),
	RW_Entry(TEXTDECORATIONUNDERLINE	,IE40	),
	RW_Entry(TEXTINDENT					,IE40	),
	RW_Entry(TEXTTRANSFORM				,IE40	),
	RW_Entry(TITLE						,ALL	),
	RW_Entry(TOELEMENT					,IE40	),
	RW_Entry(TOP						,IE40	),
	RW_Entry(TOPMARGIN					,IEXPn	),
	RW_Entry(TRUESPEED					,IE40	),
	RW_Entry(TYPE						,IEXPn	),
	RW_Entry(UPDATEINTERVAL				,IE40	),
	RW_Entry(URL						,IEXP3	),
	RW_Entry(URN						,ALL	),
	RW_Entry(USEMAP						,IEXPn	),
	RW_Entry(USERAGENT					,IE40	),
	RW_Entry(VALIGN						,IEXPn	),
	RW_Entry(VALUE						,ALL	),
	RW_Entry(VERSION					,IEXP3	),	// HTML
	RW_Entry(VERTICALALIGN				,IE40	),
	RW_Entry(VIEWASTEXT					,ALL	),	// ViewAsText for AspView only
	RW_Entry(VISIBILITY					,IE40	),
	RW_Entry(VLINK						,IEXPn	),
	RW_Entry(VLINKCOLOR					,IE40	),
	RW_Entry(VOLUME						,IE40	),
	RW_Entry(VRML						,IEXPn	),
	RW_Entry(VSPACE						,IEXPn	),
	RW_Entry(WIDTH						,ALL	),
	RW_Entry(WRAP						,IEXP3	),
	RW_Entry(X							,IE40	),
	RW_Entry(Y							,IE40	),
	RW_Entry(ZINDEX						,IE40	),
};

// If you modify the element, attribute, or entity tables, then you MUST 
// update Token.h.
static int _rgIndexAttributeNames[] = // [Attributes]
{
	/* A */	TokAttrib_ACCESSKEY			,
	/* B */	TokAttrib_BACKGROUND		,
	/* C */	TokAttrib_CANCELBUBBLE		,
	/* D */	TokAttrib_DATA				,
	/* E */	TokAttrib_ENCODING			,
	/* F */	TokAttrib_FACE				,
	/* G */	0							,
	/* H */	TokAttrib_HASH				,
	/* I */	TokAttrib_ID				,
	/* J */	0							,
	/* K */	0							,
	/* L */	TokAttrib_LANG				,
	/* M */	TokAttrib_MAP				,
	/* N */	TokAttrib_NAME				,
	/* O */	TokAttrib_OBJECT			,
	/* P */	TokAttrib_PADDING			,
	/* Q */	0							,
	/* R */	TokAttrib_READONLY			,
	/* S */	TokAttrib_SCREENX			,
	/* T */	TokAttrib_TABINDEX			,
	/* U */	TokAttrib_UPDATEINTERVAL	,
	/* V */	TokAttrib_VALIGN			,
	/* W */	TokAttrib_WIDTH				,
	/* X */	TokAttrib_X					,
	/* Y */	TokAttrib_Y					,
	/* Z */	TokAttrib_ZINDEX
};

//
// Entities
//
// ALL   - Basic             - RFC 1866, 9.7.1. Numeric and Special Graphic Entity Set
// ALL   - ISO Latin 1       - RFC 1866, 9.7.2. ISO Latin 1 Character Entity Set
// IEXPn - ISO Latin 1 Added - RFC 1866, 14.    Proposed Entities
//
// If you modify the element, attribute, or entity tables, then you MUST 
// update Token.h.
static ReservedWord _rgEntity[] =
{
    _T(""),       0,      0,
    _T("AElig"),  5,      ALL,    // <!ENTITY AElig  CDATA "&#198;") -- capital AE diphthong (ligature) -->
    _T("Aacute"), 6,      ALL,    // <!ENTITY Aacute CDATA "&#193;") -- capital A, acute accent -->
    _T("Acirc"),  5,      ALL,    // <!ENTITY Acirc  CDATA "&#194;") -- capital A, circumflex accent -->
    _T("Agrave"), 6,      ALL,    // <!ENTITY Agrave CDATA "&#192;") -- capital A, grave accent -->
    _T("Aring"),  5,      ALL,    // <!ENTITY Aring  CDATA "&#197;") -- capital A, ring -->
    _T("Atilde"), 6,      ALL,    // <!ENTITY Atilde CDATA "&#195;") -- capital A, tilde -->
    _T("Auml"),   4,      ALL,    // <!ENTITY Auml   CDATA "&#196;") -- capital A, dieresis or umlaut mark -->
    _T("Ccedil"), 6,      ALL,    // <!ENTITY Ccedil CDATA "&#199;") -- capital C, cedilla -->
    _T("ETH"),    3,      ALL,    // <!ENTITY ETH    CDATA "&#208;") -- capital Eth, Icelandic -->
    _T("Eacute"), 6,      ALL,    // <!ENTITY Eacute CDATA "&#201;") -- capital E, acute accent -->
    _T("Ecirc"),  5,      ALL,    // <!ENTITY Ecirc  CDATA "&#202;") -- capital E, circumflex accent -->
    _T("Egrave"), 6,      ALL,    // <!ENTITY Egrave CDATA "&#200;") -- capital E, grave accent -->
    _T("Euml"),   4,      ALL,    // <!ENTITY Euml   CDATA "&#203;") -- capital E, dieresis or umlaut mark -->
    _T("Iacute"), 6,      ALL,    // <!ENTITY Iacute CDATA "&#205;") -- capital I, acute accent -->
    _T("Icirc"),  5,      ALL,    // <!ENTITY Icirc  CDATA "&#206;") -- capital I, circumflex accent -->
    _T("Igrave"), 6,      ALL,    // <!ENTITY Igrave CDATA "&#204;") -- capital I, grave accent -->
    _T("Iuml"),   4,      ALL,    // <!ENTITY Iuml   CDATA "&#207;") -- capital I, dieresis or umlaut mark -->
    _T("Ntilde"), 6,      ALL,    // <!ENTITY Ntilde CDATA "&#209;") -- capital N, tilde -->
    _T("Oacute"), 6,      ALL,    // <!ENTITY Oacute CDATA "&#211;") -- capital O, acute accent -->
    _T("Ocirc"),  5,      ALL,    // <!ENTITY Ocirc  CDATA "&#212;") -- capital O, circumflex accent -->
    _T("Ograve"), 6,      ALL,    // <!ENTITY Ograve CDATA "&#210;") -- capital O, grave accent -->
    _T("Oslash"), 6,      ALL,    // <!ENTITY Oslash CDATA "&#216;") -- capital O, slash -->
    _T("Otilde"), 6,      ALL,    // <!ENTITY Otilde CDATA "&#213;") -- capital O, tilde -->
    _T("Ouml"),   4,      ALL,    // <!ENTITY Ouml   CDATA "&#214;") -- capital O, dieresis or umlaut mark -->
    _T("THORN"),  5,      ALL,    // <!ENTITY THORN  CDATA "&#222;") -- capital THORN, Icelandic -->
    _T("Uacute"), 6,      ALL,    // <!ENTITY Uacute CDATA "&#218;") -- capital U, acute accent -->
    _T("Ucirc"),  5,      ALL,    // <!ENTITY Ucirc  CDATA "&#219;") -- capital U, circumflex accent -->
    _T("Ugrave"), 6,      ALL,    // <!ENTITY Ugrave CDATA "&#217;") -- capital U, grave accent -->
    _T("Uuml"),   4,      ALL,    // <!ENTITY Uuml   CDATA "&#220;") -- capital U, dieresis or umlaut mark -->
    _T("Yacute"), 6,      ALL,    // <!ENTITY Yacute CDATA "&#221;") -- capital Y, acute accent -->
    _T("aacute"), 6,      ALL,    // <!ENTITY aacute CDATA "&#225;") -- small a, acute accent -->
    _T("acirc"),  5,      ALL,    // <!ENTITY acirc  CDATA "&#226;") -- small a, circumflex accent -->
	_T("acute"),  5,      IEXPn,  // <!ENTITY acute  CDATA "&#180;") -- acute accent -->
    _T("aelig"),  5,      ALL,    // <!ENTITY aelig  CDATA "&#230;") -- small ae diphthong (ligature) -->
    _T("agrave"), 6,      ALL,    // <!ENTITY agrave CDATA "&#224;") -- small a, grave accent -->
	_T("amp"),    3,      ALL,
    _T("aring"),  5,      ALL,    // <!ENTITY aring  CDATA "&#229;") -- small a, ring -->
    _T("atilde"), 6,      ALL,    // <!ENTITY atilde CDATA "&#227;") -- small a, tilde -->
    _T("auml"),   4,      ALL,    // <!ENTITY auml   CDATA "&#228;") -- small a, dieresis or umlaut mark -->
	_T("brvbar"), 6,      IEXPn,  // <!ENTITY brvbar CDATA "&#166;") -- broken (vertical) bar -->
    _T("ccedil"), 6,      ALL,    // <!ENTITY ccedil CDATA "&#231;") -- small c, cedilla -->
	_T("cedil"),  5,      IEXPn,  // <!ENTITY cedil  CDATA "&#184;") -- cedilla -->
	_T("cent"),   4,      IEXPn,  // <!ENTITY cent   CDATA "&#162;") -- cent sign -->
	_T("copy"),   4,      IEXPn,  // <!ENTITY copy   CDATA "&#169;") -- copyright sign -->
	_T("curren"), 6,      IEXPn,  // <!ENTITY curren CDATA "&#164;") -- general currency sign -->
	_T("deg"),    3,      IEXPn,  // <!ENTITY deg    CDATA "&#176;") -- degree sign -->
	_T("divide"), 6,      IEXPn,  // <!ENTITY divide CDATA "&#247;") -- divide sign -->
    _T("eacute"), 6,      ALL,    // <!ENTITY eacute CDATA "&#233;") -- small e, acute accent -->
    _T("ecirc"),  5,      ALL,    // <!ENTITY ecirc  CDATA "&#234;") -- small e, circumflex accent -->
    _T("egrave"), 6,      ALL,    // <!ENTITY egrave CDATA "&#232;") -- small e, grave accent -->
    _T("eth"),    3,      ALL,    // <!ENTITY eth    CDATA "&#240;") -- small eth, Icelandic -->
    _T("euml"),   4,      ALL,    // <!ENTITY euml   CDATA "&#235;") -- small e, dieresis or umlaut mark -->
	_T("frac12"), 6,      IEXPn,  // <!ENTITY frac12 CDATA "&#189;") -- fraction one-half -->
	_T("frac14"), 6,      IEXPn,  // <!ENTITY frac14 CDATA "&#188;") -- fraction one-quarter -->
	_T("frac34"), 6,      IEXPn,  // <!ENTITY frac34 CDATA "&#190;") -- fraction three-quarters -->
	_T("gt"),     2,      ALL,
    _T("iacute"), 6,      ALL,    // <!ENTITY iacute CDATA "&#237;") -- small i, acute accent -->
    _T("icirc"),  5,      ALL,    // <!ENTITY icirc  CDATA "&#238;") -- small i, circumflex accent -->
	_T("iexcl"),  5,      IEXPn,  // <!ENTITY iexcl  CDATA "&#161;") -- inverted exclamation mark -->
    _T("igrave"), 6,      ALL,    // <!ENTITY igrave CDATA "&#236;") -- small i, grave accent -->
	_T("iquest"), 6,      IEXPn,  // <!ENTITY iquest CDATA "&#191;") -- inverted question mark -->
    _T("iuml"),   4,      ALL,    // <!ENTITY iuml   CDATA "&#239;") -- small i, dieresis or umlaut mark -->
	_T("laquo"),  5,      IEXPn,  // <!ENTITY laquo  CDATA "&#171;") -- angle quotation mark, left -->
	_T("lt"),     2,      ALL,
	_T("macr"),   4,      IEXPn,  // <!ENTITY macr   CDATA "&#175;") -- macron -->
	_T("micro"),  5,      IEXPn,  // <!ENTITY micro  CDATA "&#181;") -- micro sign -->
	_T("middot"), 6,      IEXPn,  // <!ENTITY middot CDATA "&#183;") -- middle dot -->
	_T("nbsp"),   4,      IEXPn,  // <!ENTITY nbsp   CDATA "&#160;") -- no-break space -->
	_T("not"),    3,      IEXPn,  // <!ENTITY not    CDATA "&#172;") -- not sign -->
    _T("ntilde"), 6,      ALL,    // <!ENTITY ntilde CDATA "&#241;") -- small n, tilde -->
    _T("oacute"), 6,      ALL,    // <!ENTITY oacute CDATA "&#243;") -- small o, acute accent -->
    _T("ocirc"),  5,      ALL,    // <!ENTITY ocirc  CDATA "&#244;") -- small o, circumflex accent -->
    _T("ograve"), 6,      ALL,    // <!ENTITY ograve CDATA "&#242;") -- small o, grave accent -->
	_T("ordf"),   4,      IEXPn,  // <!ENTITY ordf   CDATA "&#170;") -- ordinal indicator, feminine -->
	_T("ordm"),   4,      IEXPn,  // <!ENTITY ordm   CDATA "&#186;") -- ordinal indicator, masculine -->
    _T("oslash"), 6,      ALL,    // <!ENTITY oslash CDATA "&#248;") -- small o, slash -->
    _T("otilde"), 6,      ALL,    // <!ENTITY otilde CDATA "&#245;") -- small o, tilde -->
    _T("ouml"),   4,      ALL,    // <!ENTITY ouml   CDATA "&#246;") -- small o, dieresis or umlaut mark -->
	_T("para"),   4,      IEXPn,  // <!ENTITY para   CDATA "&#182;") -- pilcrow (paragraph sign) -->
	_T("plusmn"), 6,      IEXPn,  // <!ENTITY plusmn CDATA "&#177;") -- plus-or-minus sign -->
	_T("pound"),  5,      IEXPn,  // <!ENTITY pound  CDATA "&#163;") -- pound sterling sign -->
	_T("quot"),   4,      ALL,
	_T("raquo"),  5,      IEXPn,  // <!ENTITY raquo  CDATA "&#187;") -- angle quotation mark, right -->
	_T("reg"),    3,      IEXPn,  // <!ENTITY reg    CDATA "&#174;") -- registered sign -->
	_T("sect"),   4,      IEXPn,  // <!ENTITY sect   CDATA "&#167;") -- section sign -->
	_T("shy"),    3,      IEXPn,  // <!ENTITY shy    CDATA "&#173;") -- soft hyphen -->
	_T("sup1"),   4,      IEXPn,  // <!ENTITY sup1   CDATA "&#185;") -- superscript one -->
	_T("sup2"),   4,      IEXPn,  // <!ENTITY sup2   CDATA "&#178;") -- superscript two -->
	_T("sup3"),   4,      IEXPn,  // <!ENTITY sup3   CDATA "&#179;") -- superscript three -->
    _T("szlig"),  5,      ALL,    // <!ENTITY szlig  CDATA "&#223;") -- small sharp s, German (sz ligature)->
    _T("thorn"),  5,      ALL,    // <!ENTITY thorn  CDATA "&#254;") -- small thorn, Icelandic -->
	_T("times"),  5,      IEXPn,  // <!ENTITY times  CDATA "&#215;") -- multiply sign -->
    _T("uacute"), 6,      ALL,    // <!ENTITY uacute CDATA "&#250;") -- small u, acute accent -->
    _T("ucirc"),  5,      ALL,    // <!ENTITY ucirc  CDATA "&#251;") -- small u, circumflex accent -->
    _T("ugrave"), 6,      ALL,    // <!ENTITY ugrave CDATA "&#249;") -- small u, grave accent -->
	_T("uml"),    3,      IEXPn,  // <!ENTITY uml    CDATA "&#168;") -- umlaut (dieresis) -->
    _T("uuml"),   4,      ALL,    // <!ENTITY uuml   CDATA "&#252;") -- small u, dieresis or umlaut mark -->
    _T("yacute"), 6,      ALL,    // <!ENTITY yacute CDATA "&#253;") -- small y, acute accent -->
	_T("yen"),    3,      IEXPn,  // <!ENTITY yen    CDATA "&#165;") -- yen sign -->
    _T("yuml"),   4,      ALL,    // <!ENTITY yuml   CDATA "&#255;") -- small y, dieresis or umlaut mark -->

};

// If you modify the element, attribute, or entity tables, then you MUST 
// update Token.h.
static int _rgIndexEntity[] =
{
//  A   B   C   D   E   F   G   H   I   J   K   L   M
	1,  0,  8,  0,  9,  0,  0,  0, 14,  0,  0,  0,  0,
//  N   O   P   Q   R   S   T   U   V   W   X   Y   Z
   18, 19,  0,  0,  0,  0, 25, 26,  0,  0,  0, 30,  0,
//  a   b   c   d   e   f   g   h   i   j   k   l   m
   31, 40, 41, 46, 48, 53, 56,  0, 57,  0,  0, 63, 65,
//  n   o   p   q   r   s   t   u   v   w   x   y   z
   68, 71, 79, 82, 83, 85, 91, 93,  0,  0,  0, 98,  0
};

////////////////////////////////////////////////////////////////////////////


//
//
//	int LookupLinearKeyword
//
//	Description:
//		Does the lookup in the given table.
//		Returns index into table if found, NOT_FOUND otw.
//
int LookupLinearKeyword
(
	ReservedWord 	*rwTable,
	int 			cel,
	RWATT_T 		att,
	LPCTSTR 		pchLine,
	int 			cbLen,
	BOOL 			bCase /* = NOCASE */
)
{
	int iTable = 0;
	ASSERT(cel > 0);

	PFNNCMP pfnNCmp = bCase ? (_tcsncmp) : (_tcsnicmp);

	do
	{
		int Cmp;
		if (0 == (Cmp = pfnNCmp(pchLine, rwTable[iTable].psz, cbLen)) &&
			 (cbLen == rwTable[iTable].cb))
			return (0 != (rwTable[iTable].att & att)) ? iTable : NOT_FOUND;
		else if (Cmp < 0)
			return NOT_FOUND;
		else
			iTable++;
	} while (iTable < cel);
	return NOT_FOUND;
}

////////////////////////////////////////////////////////////////////////////
//  LookupIndexedKeyword()
//
int LookupIndexedKeyword
(
	ReservedWord 	*rwTable,
	int 			cel,
	int * 			indexTable,
	RWATT_T 		att,
	LPCTSTR 		pchLine,
	int 			cbLen,
	BOOL 			bCase 	/* = NOCASE */
)
{
	// lookup table:
	int iTable;
	int index = PeekIndex(*pchLine, bCase);
	if (index < 0)
		return NOT_FOUND;
	else
		iTable = indexTable[index];
	if (0 == iTable)
		return NOT_FOUND;
	int iFound = LookupLinearKeyword(&rwTable[iTable], cel - iTable,
		att, pchLine, cbLen, bCase);

	return (iFound == NOT_FOUND) ? NOT_FOUND : iTable + iFound;
}

////////////////////////////////////////////////////////////////////////////
// hinting table - character classification

// HOP
// () ? , | + [] * =
//   in tag, op
//
// HDA
// - op
// -- comment
//
// HEN
// & in text, entity ref
//   in tag, op
//
// HEP
// % in tag, parameter entity ref (%name) or op (%WS)
//
// HRN
// # reserved name
//
// HTA
// <    tag open
// </   tag end
// <!   MDO Markup delimiter open
// <?   processing instruction - what's the syntax for a complete PI tag?

// Hint table:
HINT g_hintTable[128] =
{
//    0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
    EOS, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, HWS, ONL, ERR, ERR, ERR, ERR, ERR,
//   10   11   12   13   14   15   16   17   18   19   1a   1b   1c   1d   1e   1f
    ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR,
//  spc   !    "    #    $    %    &    '    (    )    *    +    ,    -    .    /
    HWS, ERR, HST, HRN, ERR, HEP, HEN, HSL, OLP, ORP, OST, OPL, OCO, ODA, HKW, HAV,
//   0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
    HNU, HNU, HNU, HNU, HNU, HNU, HNU, HNU, HNU, HNU, ERR, ERR, HTA, OEQ, HTE, OQU,
//   @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
    ERR, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW,
//   P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _
    HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, OLB, HAV, ORB, ERR, HAV,
//   `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
    ERR, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW,
//   p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~   DEL
    HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, HKW, ERR, OPI, ERR, ERR, ERR
};

////////////////////////////////////////////////////////////////////////////
// content model
//
// Map between element / lex state
//

// 0-terminated list
// if we get more, consider putting text state in element table
static ELLEX _ElTextStateTable[] =
{
	_T("COMMENT"),   7, inCOMMENT,
	_T("LISTING"),   7, inLISTING,
	_T("PLAINTEXT"), 9, inPLAINTEXT,
	_T("SCRIPT"),	 6, inSCRIPT,
	_T("XMP"),       3, inXMP,
	0, 0, 0
};

DWORD TextStateFromElement(LPCTSTR szEl, int cb)
{
	int cmp;
	for (ELLEX *pel = _ElTextStateTable; pel->sz != 0; pel++)
	{
		if (0 == (cmp = _tcsnicmp(pel->sz, szEl, cb)))
		{
			if (cb == pel->cb)
				return pel->lxs;
		}
		else if (cmp > 0)
			return 0;
	}
	return 0;
}

ELLEX * pellexFromTextState(DWORD state)
{
	DWORD t = (state & TEXTMASK); // only want text state bits
	for (ELLEX *pellex = _ElTextStateTable; pellex->lxs != 0; pellex++)
	{
		if (t == pellex->lxs)
			return pellex;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////
// CStaticTable
CStaticTable::CStaticTable(RWATT_T att,
						   ReservedWord *prgrw,
						   UINT cel,
						   int *prgi /*= NULL*/,
						   BOOL bCase /*= FALSE*/,
						   LPCTSTR szName /*= NULL*/)
: m_att(att), m_prgrw(prgrw), m_cel(cel), m_prgi(prgi), m_bCase(bCase)
{
	ASSERT(NULL != m_prgrw);
	ASSERT(m_cel > 0);
	ASSERT(0 == CheckWordTable(m_prgrw, cel, szName));
	if (NULL == m_prgi)
	{
		MakeIndex(m_prgrw, m_cel, &m_prgi, m_bCase, szName);
	}
	else
		ASSERT(0 == CheckWordTableIndex(m_prgrw, cel, prgi, m_bCase, szName));
}

int CStaticTable::Find(LPCTSTR pch, int cb)
{
	return LookupIndexedKeyword(m_prgrw, m_cel, m_prgi, m_att, pch, cb, m_bCase);
}

////////////////////////////////////////////////////////////////////////////
CStaticTableSet::CStaticTableSet(RWATT_T att, UINT nIdName)
:	m_Elements  ( att, _rgElementNames, CELEM_ARRAY(_rgElementNames),
				 _rgIndexElementNames, NOCASE, szElTag ),
	m_Attributes( att, _rgAttributeNames, CELEM_ARRAY(_rgAttributeNames),
				 _rgIndexAttributeNames, NOCASE, szAttTag ),
	m_Entities  ( att, _rgEntity, CELEM_ARRAY(_rgEntity),
				 _rgIndexEntity, CASE, szEntTag )
{
	::LoadString(	_Module.GetModuleInstance(),
					nIdName,
					m_strName,
					sizeof(m_strName)
					);
}

int CStaticTableSet::FindElement(LPCTSTR pch, int cb)
{
	return m_Elements.Find(pch, cb);
}

int CStaticTableSet::FindAttribute(LPCTSTR pch, int cb)
{
	return m_Attributes.Find(pch, cb);
}

int CStaticTableSet::FindEntity(LPCTSTR pch, int cb)
{
	return m_Entities.Find(pch, cb);
}

CStaticTableSet * g_pTabDefault;
PTABLESET g_pTable = 0;

////////////////////////////////////////////////////////////////////////////
//
// Custom HTML tables
//
/*

@HLX@ "Internet Explorer 3.0"
;Custom HTML tagset file must begin with the "@HLX@"
;signature and the name of the HTML variant in quotes.

[Elements]
; element set

[Attributes]
; attribute set

[Entities]
; entity set

*/

// qsort/bsearch helper
int CmpFunc(const void *a, const void *b)
{
	CLStr *A = (CLStr*)a;
	CLStr *B = (CLStr*)b;
	int r = memcmp(A->m_rgb, B->m_rgb, __min(A->m_cb, B->m_cb));
	return (0 == r) ? (A->m_cb - B->m_cb) : r;
}

