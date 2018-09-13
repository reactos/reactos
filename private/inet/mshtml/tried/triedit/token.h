/* Copied from ..\htmed\itoken.h and changed name from ITokenizer to ITokGen 
and itoken.h to token.h just to avoid name confusion in future. */

/*

    File: Token.h

    Copyright (c) 1997-1999 Microsoft Corporation.  All Rights Reserved.

    Abstract:
		ITokenizer interface and its types

*/
#if !defined __INC_TOKEN_H__
#define __INC_TOKEN_H__

typedef int TOKEN;
// A text token block indicates the token, and its starting and ending
// indexes in the line of source just lexed.
// Note that for any N > 0, rgtxtb[N].ibTokMin >= rgtxtb[N-1].ibTokMac.
// if it is such that rgtxtb[N].ibTokMin > rgtxtb[N-1].ibTokMac, then
// the intervening unclassified characters are treated as white space tokens.

typedef struct _tagTXTB { // Text token class block
	TOKEN   tok;
	TOKEN   tokClass;
	UINT    ibTokMin;               // token length given by ibTokMac - ibTokMin
	UINT    ibTokMac;               // given in bytes
} TXTB;

// ********* defines specific to TRIEDIT begin here


enum ObjectTypes
    {	
    OT_NONE	= 0,
	OT_ACTIVEXCONTROL	= 0x1,
	OT_DESIGNTIMECONTROL	= 0x2,
	OT_INPUT	= 0x4,
	OT_TABLE	= 0x8,
	OT_APPLET	= 0x10,
	OT_INVISIBLE	= 0x8000,
	OT_VISIBLEITEMS	= 0x4fff,
	OT_ALLITEMS	= 0xffff
    };

enum ParserStates
{
	PS_HTML,
	PS_SIMPLETAG,		// simple tag that cannot have </ nested within it
	PS_OBJECT,
	PS_METADATASTART,
	PS_METADATABODY,
	PS_METADATAEND

};

enum ObjectParserStates
{
	OPS_NONE,
	OPS_CLASSID
};

enum TagStates
{
	TS_NONE,	  	// in tag, looking for the end tag
	TS_FOUNDTAGCLOSE, // found > closing the start element
	TS_FOUNDTAGEND	// found </ next token should be end tag token
};

enum DesignControlStates
{
	DTCS_NONE, 		// just found METADATA
	DTCS_TYPE,		// found TYPE attribute
	DTCS_DTCTYPE,	// found DesignerControl attribute value
	DTCS_ENDSPAN 	// found endspan
};
enum FilterMode
{ modeNone, modeInput, modeOutput };

#define cbHeader 0x8000	// initial buffer size to save all contents before/after <BODY> tag
#define MAX_BLOCKS 20 // max # of blocks that hold the replaced data 
#define MAX_TOKIDLEN 32 // max length of a token identifier


struct TOKSTRUCT // holds elements of token array generated  from the document
{
	TXTB token;
	BOOL fStart;
	UINT ichStart;
	UINT iNextprev;
	UINT iNextPrevAlternate;
	INT tagID;
};

struct TOKSTACK
{
	UINT iMatch;
	INT tagID;
	UINT ichStart; // start char position of this token in the input buffer
	TXTB token; // used in special cases (<%, %>) where tagID is -1
};


// ********* defines specific to TRIEDIT end here
interface ITokenGen : public IUnknown
{
public:
	// Gets then next token given the text
	//	pszText:	stream of text - UNICODE only!!!!
	//	cbText:		count of bytes in pszText
	//	pcbCur:		current byte position in pszText.
	//				set to 0 for start of pszText
	//	pLXS:		should be 0, on initial call
	//	pToken:		TXTB structure which contains the token info
	//	returns:
	//			NOERROR if found the next token
	//			S_FALSE if no more tokens
	//			error if there was an error
	//
	virtual HRESULT STDMETHODCALLTYPE NextToken(
		/*	[in]	  */ LPCWSTR pszText,
		/*	[in]	  */ UINT	 cbText,
		/*	[in, out] */ UINT	*pcbCur,
		/*	[in, out] */ DWORD *pLXS,
		/*	[out]	  */ TXTB	*pToken) = 0;


	// Tokenizes & Parses the input buffer
	//	hOld:	source buffer
	//	phNew or pStmNew: filtered buffer (based on dwFlagsUsePstmNew)
	//	pStmNew
	//	dwFlags:filter flags
	//	mode:	input/output mode.
	//	cbSizeIn: size of input buffer in bytes (if -1, assume NULL terminated buffer)
	//	*pcbSizeOut:size of output buffer in bytes
	//	pUnkTrident:trident's IUnknown
	//	phgTokArray:Token Array (freed by the caller)
	//	pcMaxToken:number of tokens in phgTokArray
	//	phgDocRestore:stores document contents which are used for filtering
	//	bstrBaseURL:used to convert URLs on a page into Relative URL (can be NULL)
	//	dwReserved - must be 0 (added for future use)
	//	returns:
	//		S_OK if no errors
	//		E_OUTOFMEMORY if any allocation failed
	//		E_FILTERFRAMESET or E_FILTERSERVERSCRIPT for html files that cannot be filtered

	virtual HRESULT STDMETHODCALLTYPE hrTokenizeAndParse(
		/*	[in]	*/ HGLOBAL hOld,
		/*	[out]	*/ HGLOBAL *phNew,
		/*	[in/out]*/ IStream *pStmNew,
		/*	[in]	*/ DWORD dwFlags,
		/*	[in]	*/ FilterMode mode,
		/*	[in]	*/ int cbSizeIn,
		/*	[out]	*/ UINT* pcbSizeOut,
		/*	[in]	*/ IUnknown *pUnkTrident,
		/*	[out]	*/ HGLOBAL *phgTokArray,
		/*	[out]	*/ UINT *pcMaxToken,
		/*	[in/out]*/ HGLOBAL *phgDocRestore,
		/*  [in]    */ BSTR bstrBaseURL,
		/*	[in]	*/ DWORD dwReserved) = 0;

};

enum TOKCLS { // token classes
	tokclsError = 0,

	// all standard language keywords
	tokclsKeyWordMin = 1,

	// for block start/end that are keywords instead of operators...like
	// Pascal or BASIC for instance.
	tokclsKeyWordOpenBlock = 0xfe,
	tokclsKeyWordCloseBlock = 0xff,

	tokclsKeyWordMax = 0x100,

	// all language operators
	tokclsOpMin = 0x100,
	tokclsOpSpecOpenBlock = 0x1fe,
	tokclsOpSpecCloseBlock = 0x1ff,
	tokclsOpMax = 0x200,

	// special, hard coded operators that editor keys off of
	tokclsOpSpecMin = 0x200,
	tokclsOpSpecEOL = 0x200,
	tokclsOpSpecLineCmt = 0x201, // automatic skip to eol on this one
	tokclsOpSpecEOS = 0x202,
	tokclsOpSpecMax = 0x210,

	// all identifiers, give ~500 possibilities
	tokclsIdentMin = 0x210,
	tokclsIdentUser = 0x211,        // special idents (user keywords)
	tokclsIdentMax = 0x400,

	// all constants (numeric and string)
	tokclsConstMin = 0x400,
	tokclsConstInteger = 0x400,
	tokclsConstReal = 0x401,
	tokclsConstString = 0x402,
	tokclsStringPart = 0x402,               // partial string ("....)
	tokclsConstMax = 0x410,

	// comments
	tokclsCommentMin = 0x500,
	tokclsCommentPart = 0x500,      // partial comment (/* ...)
	tokclsCommentMax = 0x510,

	// language dependent token class(es) start at 0x800 to 0xfff
	tokclsUserMin = 0x800,
	tokclsUserLast = 0xfff,
	tokclsUserMax = 0x1000,

	// mask to ignore all the bits in a token[class] that the lexer can use
	// for private status.  they will be masked off and ignored by clients
	// of the lexer.  A good use of this feature is to encode the real token
	// type in the lexer private portion (area is ~tokclsMask) when including
	// meta-token types (such as MFC/Wizard user token types) so that other
	// clients of the lexer can keep that information.
	tokclsUserMask = ~(tokclsUserMin - 1),
	tokclsMask = tokclsUserLast,

};


// NOTE:
// ========================================================================
// PLEASE LET sridharc KNOW IF YOU PLAN TO ADD/REMOVE A TOKEN FROM
// THE BELOW ENUM, SINCE HE IS DEPENDING UPON THIS ORDER FOR HIS TAG PROPERTY
// EDITING.  THANKS.
// ========================================================================
typedef enum tagElementTokens
{
	TokElem_Empty            = 0  	,  /* Empty    		*/
	TokElem_A				 = 1	,  /* A				*/
	TokElem_ADDRESS			 = 2	,  /* ADDRESS		*/
	TokElem_APPLET			 = 3	,  /* APPLET		*/
	TokElem_AREA			 = 4	,  /* AREA			*/
	TokElem_B				 = 5	,  /* B				*/
	TokElem_BASE			 = 6	,  /* BASE			*/
	TokElem_BASEFONT		 = 7	,  /* BASEFONT		*/
	TokElem_BGSOUND			 = 8	,  /* BGSOUND		*/
	TokElem_BIG				 = 9	,  /* BIG			*/
	TokElem_BLINK			 = 10	,  /* BLINK			*/
	TokElem_BLOCKQUOTE		 = 11	,  /* BLOCKQUOTE	*/
	TokElem_BODY			 = 12	,  /* BODY			*/
	TokElem_BR				 = 13	,  /* BR			*/
	TokElem_BUTTON			 = 14	,  /* BUTTON		*/
	TokElem_CAPTION			 = 15	,  /* CAPTION		*/
	TokElem_CENTER			 = 16	,  /* CENTER		*/
	TokElem_CITE			 = 17	,  /* CITE			*/
	TokElem_CODE			 = 18	,  /* CODE			*/
	TokElem_COL				 = 19	,  /* COL			*/
	TokElem_COLGROUP		 = 20	,  /* COLGROUP		*/
	TokElem_COMMENT			 = 21	,  /* COMMENT		*/
	TokElem_DD				 = 22	,  /* DD			*/
	TokElem_DFN				 = 23	,  /* DFN			*/
	TokElem_DIR				 = 24	,  /* DIR			*/
	TokElem_DIV				 = 25	,  /* DIV			*/
	TokElem_DL				 = 26	,  /* DL			*/
	TokElem_DT				 = 27	,  /* DT			*/
	TokElem_EM				 = 28	,  /* EM			*/
	TokElem_EMBED			 = 29	,  /* EMBED			*/
	TokElem_FIELDSET		 = 30	,  /* FIELDSET		*/
	TokElem_FONT			 = 31	,  /* FONT			*/
	TokElem_FORM			 = 32	,  /* FORM			*/
	TokElem_FRAME			 = 33	,  /* FRAME			*/
	TokElem_FRAMESET		 = 34	,  /* FRAMESET		*/
	TokElem_H1				 = 35	,  /* H1			*/
	TokElem_H2				 = 36	,  /* H2			*/
	TokElem_H3				 = 37	,  /* H3			*/
	TokElem_H4				 = 38	,  /* H4			*/
	TokElem_H5				 = 39	,  /* H5			*/
	TokElem_H6				 = 40	,  /* H6			*/
	TokElem_HEAD			 = 41	,  /* HEAD			*/
	TokElem_HR				 = 42	,  /* HR			*/
	TokElem_HTML			 = 43	,  /* HTML			*/
	TokElem_I				 = 44	,  /* I				*/
	TokElem_IFRAME			 = 45	,  /* IFRAME		*/
	TokElem_IMG				 = 46	,  /* IMG			*/
	TokElem_INPUT			 = 47	,  /* INPUT			*/
	TokElem_ISINDEX			 = 48	,  /* ISINDEX		*/
	TokElem_KBD				 = 49	,  /* KBD			*/
	TokElem_LABEL			 = 50	,  /* LABEL			*/
	TokElem_LEGEND			 = 51	,  /* LEGEND		*/
	TokElem_LI				 = 52	,  /* LI			*/
	TokElem_LINK			 = 53	,  /* LINK			*/
	TokElem_LISTING			 = 54	,  /* LISTING		*/
	TokElem_MAP				 = 55	,  /* MAP			*/
	TokElem_MARQUEE			 = 56	,  /* MARQUEE		*/
	TokElem_MENU			 = 57	,  /* MENU			*/
	TokElem_META			 = 58	,  /* META			*/
	TokElem_METADATA		 = 59	,  /* METADATA		*/
	TokElem_NOBR			 = 60	,  /* NOBR			*/
	TokElem_NOFRAMES		 = 61	,  /* NOFRAMES		*/
	TokElem_NOSCRIPT		 = 62	,  /* NOSCRIPT		*/
	TokElem_OBJECT			 = 63	,  /* OBJECT		*/
	TokElem_OL				 = 64	,  /* OL			*/
	TokElem_OPTION			 = 65	,  /* OPTION		*/
	TokElem_P				 = 66	,  /* P				*/
	TokElem_PARAM			 = 67	,  /* PARAM			*/
	TokElem_PLAINTEXT		 = 68	,  /* PLAINTEXT		*/
	TokElem_PRE				 = 69	,  /* PRE			*/
	TokElem_S				 = 70	,  /* S				*/
	TokElem_SAMP			 = 71	,  /* SAMP			*/
	TokElem_SCRIPT			 = 72	,  /* SCRIPT		*/
	TokElem_SELECT			 = 73	,  /* SELECT		*/
	TokElem_SMALL			 = 74	,  /* SMALL			*/
	TokElem_SPAN			 = 75	,  /* SPAN			*/
	TokElem_STRIKE			 = 76	,  /* STRIKE		*/
	TokElem_STRONG			 = 77	,  /* STRONG		*/
	TokElem_STYLE			 = 78	,  /* STYLE			*/
	TokElem_SUB				 = 79	,  /* SUB			*/
	TokElem_SUP				 = 80	,  /* SUP			*/
	TokElem_TABLE			 = 81	,  /* TABLE			*/
	TokElem_TBODY			 = 82	,  /* TBODY			*/
	TokElem_TD				 = 83	,  /* TD			*/
	TokElem_TEXTAREA		 = 84	,  /* TEXTAREA		*/
	TokElem_TFOOT			 = 85	,  /* TFOOT			*/
	TokElem_TH				 = 86	,  /* TH			*/
	TokElem_THEAD			 = 87	,  /* THEAD			*/
	TokElem_TITLE			 = 88	,  /* TITLE			*/
	TokElem_TR				 = 89	,  /* TR			*/
	TokElem_TT				 = 90	,  /* TT			*/
	TokElem_U				 = 91	,  /* U				*/
	TokElem_UL				 = 92	,  /* UL			*/
	TokElem_VAR				 = 93	,  /* VAR			*/
	TokElem_WBR				 = 94	,  /* WBR			*/
	TokElem_XMP				 = 95	  /* XMP			*/
} ElementTokens;
// NOTE:
// ========================================================================
// PLEASE LET sridharc KNOW IF YOU PLAN TO ADD/REMOVE A TOKEN FROM
// THE ABOVE ENUM, SINCE HE IS DEPENDING UPON THIS ORDER FOR HIS TAG PROPERTY
// ========================================================================

typedef enum tagAttributeTokens
{
	TokAttrib_Empty                         =0  ,   /*                      */
	TokAttrib_ACCESSKEY						=1    	,// ACCESSKEY
	TokAttrib_ACTION						=2    	,// ACTION
	TokAttrib_ALIGN							=3    	,// ALIGN
	TokAttrib_ALINK							=4    	,// ALINK
	TokAttrib_ALT							=5    	,// ALT
	TokAttrib_APPNAME						=6    	,// APPNAME
	TokAttrib_APPVERSION					=7    	,// APPVERSION
	TokAttrib_BACKGROUND					=8    	,// BACKGROUND
	TokAttrib_BACKGROUNDATTACHMENT			=9    	,// BACKGROUNDATTACHMENT
	TokAttrib_BACKGROUNDCOLOR				=10   	,// BACKGROUNDCOLOR
	TokAttrib_BACKGROUNDIMAGE				=11   	,// BACKGROUNDIMAGE
	TokAttrib_BACKGROUNDPOSITION			=12   	,// BACKGROUNDPOSITION
	TokAttrib_BACKGROUNDPOSITIONX			=13   	,// BACKGROUNDPOSITIONX
	TokAttrib_BACKGROUNDPOSITIONY			=14   	,// BACKGROUNDPOSITIONY
	TokAttrib_BACKGROUNDREPEAT				=15   	,// BACKGROUNDREPEAT
	TokAttrib_BALANCE						=16   	,// BALANCE
	TokAttrib_BEHAVIOR						=17   	,// BEHAVIOR
	TokAttrib_BGCOLOR						=18   	,// BGCOLOR
	TokAttrib_BGPROPERTIES					=19   	,// BGPROPERTIES
	TokAttrib_BORDER						=20   	,// BORDER
	TokAttrib_BORDERBOTTOM					=21   	,// BORDERBOTTOM
	TokAttrib_BORDERBOTTOMCOLOR				=22   	,// BORDERBOTTOMCOLOR
	TokAttrib_BORDERBOTTOMSTYLE				=23   	,// BORDERBOTTOMSTYLE
	TokAttrib_BORDERBOTTOMWIDTH				=24   	,// BORDERBOTTOMWIDTH
	TokAttrib_BORDERCOLOR					=25   	,// BORDERCOLOR
	TokAttrib_BORDERCOLORDARK				=26   	,// BORDERCOLORDARK
	TokAttrib_BORDERCOLORLIGHT				=27   	,// BORDERCOLORLIGHT
	TokAttrib_BORDERLEFT					=28   	,// BORDERLEFT
	TokAttrib_BORDERLEFTCOLOR				=29   	,// BORDERLEFTCOLOR
	TokAttrib_BORDERLEFTSTYLE				=30   	,// BORDERLEFTSTYLE
	TokAttrib_BORDERLEFTWIDTH				=31   	,// BORDERLEFTWIDTH
	TokAttrib_BORDERRIGHT					=32   	,// BORDERRIGHT
	TokAttrib_BORDERRIGHTCOLOR				=33   	,// BORDERRIGHTCOLOR
	TokAttrib_BORDERRIGHTSTYLE				=34   	,// BORDERRIGHTSTYLE
	TokAttrib_BORDERRIGHTWIDTH				=35   	,// BORDERRIGHTWIDTH
	TokAttrib_BORDERSTYLE					=36   	,// BORDERSTYLE
	TokAttrib_BORDERTOP						=37   	,// BORDERTOP
	TokAttrib_BORDERTOPCOLOR				=38   	,// BORDERTOPCOLOR
	TokAttrib_BORDERTOPSTYLE				=39   	,// BORDERTOPSTYLE
	TokAttrib_BORDERTOPWIDTH				=40   	,// BORDERTOPWIDTH
	TokAttrib_BORDERWIDTH					=41   	,// BORDERWIDTH
	TokAttrib_BOTTOMMARGIN					=42   	,// BOTTOMMARGIN
	TokAttrib_BREAKPOINT					=43   	,// BREAKPOINT
	TokAttrib_BUFFERDEPTH					=44   	,// BUFFERDEPTH
	TokAttrib_BUTTON						=45   	,// BUTTON
	TokAttrib_CANCELBUBBLE					=46   	,// CANCELBUBBLE
	TokAttrib_CELLPADDING					=47   	,// CELLPADDING
	TokAttrib_CELLSPACING					=48   	,// CELLSPACING
	TokAttrib_CENTER						=49   	,// CENTER
	TokAttrib_CHARSET						=50   	,// CHARSET
	TokAttrib_CHECKED						=51   	,// CHECKED
	TokAttrib_CLASS							=52   	,// CLASS
	TokAttrib_CLASSID						=53   	,// CLASSID
	TokAttrib_CLASSNAME						=54   	,// CLASSNAME
	TokAttrib_CLEAR							=55   	,// CLEAR
	TokAttrib_CLIP							=56   	,// CLIP
	TokAttrib_CODE							=57   	,// CODE
	TokAttrib_CODEBASE						=58   	,// CODEBASE
	TokAttrib_CODETYPE						=59   	,// CODETYPE
	TokAttrib_COLOR							=60   	,// COLOR
	TokAttrib_COLORDEPTH					=61   	,// COLORDEPTH
	TokAttrib_COLS							=62   	,// COLS
	TokAttrib_COLSPAN						=63   	,// COLSPAN
	TokAttrib_COMPACT						=64   	,// COMPACT
	TokAttrib_COMPLETE						=65   	,// COMPLETE
	TokAttrib_CONTENT						=66   	,// CONTENT
	TokAttrib_CONTROLS						=67   	,// CONTROLS
	TokAttrib_COOKIE						=68   	,// COOKIE
	TokAttrib_COOKIEENABLED					=69   	,// COOKIEENABLED
	TokAttrib_COORDS						=70   	,// COORDS
	TokAttrib_CSSTEXT						=71   	,// CSSTEXT
	TokAttrib_CTRLKEY						=72   	,// CTRLKEY
	TokAttrib_CURSOR						=73   	,// CURSOR
	TokAttrib_DATA							=74   	,// DATA
	TokAttrib_DECLARE						=75   	,// DECLARE
	TokAttrib_DATAFLD						=76   	,// DATAFLD
	TokAttrib_DATAFORMATAS					=77   	,// DATAFORMATAS
	TokAttrib_DATAPAGESIZE					=78   	,// DATAPAGESIZE
	TokAttrib_DATASRC						=79   	,// DATASRC
	TokAttrib_DEFAULTCHECKED				=80   	,// DEFAULTCHECKED
	TokAttrib_DEFAULTSELECTED				=81   	,// DEFAULTSELECTED
	TokAttrib_DEFAULTSTATUS					=82   	,// DEFAULTSTATUS
	TokAttrib_DEFAULTVALUE					=83   	,// DEFAULTVALUE
	TokAttrib_DIALOGARGUMENTS				=84   	,// DIALOGARGUMENTS
	TokAttrib_DIALOGHEIGHT					=85   	,// DIALOGHEIGHT
	TokAttrib_DIALOGLEFT					=86   	,// DIALOGLEFT
	TokAttrib_DIALOGTOP						=87   	,// DIALOGTOP
	TokAttrib_DIALOGWIDTH					=88   	,// DIALOGWIDTH
	TokAttrib_DIR							=89   	,// DIR
	TokAttrib_DIRECTION						=90   	,// DIRECTION
	TokAttrib_DISABLED						=91   	,// DISABLED
	TokAttrib_DISPLAY						=92   	,// DISPLAY
	TokAttrib_DOMAIN						=93   	,// DOMAIN
	TokAttrib_DYNSRC						=94   	,// DYNSRC
	TokAttrib_ENCODING						=95   	,// ENCODING
	TokAttrib_ENCTYPE						=96   	,// ENCTYPE
	TokAttrib_ENDSPAN						=97   	,// ENDSPAN
	TokAttrib_ENDSPAN__						=98   	,// ENDSPAN--
	TokAttrib_EVENT							=99   	,// EVENT
	TokAttrib_FACE							=100  	,// FACE
	TokAttrib_FGCOLOR						=101  	,// FGCOLOR
	TokAttrib_FILTER						=102  	,// FILTER
	TokAttrib_FONT							=103  	,// FONT
	TokAttrib_FONTFAMILY					=104  	,// FONTFAMILY
	TokAttrib_FONTSIZE						=105  	,// FONTSIZE
	TokAttrib_FONTSTYLE						=106  	,// FONTSTYLE
	TokAttrib_FONTVARIANT					=107  	,// FONTVARIANT
	TokAttrib_FONTWEIGHT					=108  	,// FONTWEIGHT
	TokAttrib_FOR							=109  	,// FOR
	TokAttrib_FORM							=110  	,// FORM
	TokAttrib_FRAME							=111  	,// FRAME
	TokAttrib_FRAMEBORDER					=112  	,// FRAMEBORDER
	TokAttrib_FRAMESPACING					=113  	,// FRAMESPACING
	TokAttrib_FROMELEMENT					=114  	,// FROMELEMENT
	TokAttrib_HASH							=115  	,// HASH
	TokAttrib_HEIGHT						=116  	,// HEIGHT
	TokAttrib_HIDDEN						=117  	,// HIDDEN
	TokAttrib_HOST							=118  	,// HOST
	TokAttrib_HOSTNAME						=119  	,// HOSTNAME
	TokAttrib_HREF							=120  	,// HREF
	TokAttrib_HSPACE						=121  	,// HSPACE
	TokAttrib_HTMLFOR						=122  	,// HTMLFOR
	TokAttrib_HTMLTEXT						=123  	,// HTMLTEXT
	TokAttrib_HTTPEQUIV						=124  	,// HTTPEQUIV
	TokAttrib_HTTP_EQUIV					=125  	,// HTTP-EQUIV
	TokAttrib_ID							=126  	,// ID
	TokAttrib_IN							=127  	,// IN
	TokAttrib_INDETERMINATE					=128  	,// INDETERMINATE
	TokAttrib_INDEX							=129  	,// INDEX
	TokAttrib_ISMAP							=130  	,// ISMAP
	TokAttrib_LANG							=131  	,// LANG
	TokAttrib_LANGUAGE						=132  	,// LANGUAGE
	TokAttrib_LEFTMARGIN					=133  	,// LEFTMARGIN
	TokAttrib_LENGTH						=134  	,// LENGTH
	TokAttrib_LETTERSPACING					=135  	,// LETTERSPACING
	TokAttrib_LINEHEIGHT					=136  	,// LINEHEIGHT
	TokAttrib_LINK							=137  	,// LINK
	TokAttrib_LINKCOLOR						=138  	,// LINKCOLOR
	TokAttrib_LISTSTYLE						=139  	,// LISTSTYLE
	TokAttrib_LISTSTYLEIMAGE				=140  	,// LISTSTYLEIMAGE
	TokAttrib_LISTSTYLEPOSITION				=141  	,// LISTSTYLEPOSITION
	TokAttrib_LISTSTYLETYPE					=142  	,// LISTSTYLETYPE
	TokAttrib_LOCATION						=143  	,// LOCATION
	TokAttrib_LOOP							=144  	,// LOOP
	TokAttrib_LOWSRC						=145  	,// LOWSRC
	TokAttrib_MAP							=146  	,// MAP
	TokAttrib_MARGIN						=147  	,// MARGIN
	TokAttrib_MARGINBOTTOM					=148  	,// MARGINBOTTOM
	TokAttrib_MARGINHEIGHT					=149  	,// MARGINHEIGHT
	TokAttrib_MARGINLEFT					=150  	,// MARGINLEFT
	TokAttrib_MARGINRIGHT					=151  	,// MARGINRIGHT
	TokAttrib_MARGINTOP						=152  	,// MARGINTOP
	TokAttrib_MARGINWIDTH					=153  	,// MARGINWIDTH
	TokAttrib_MAXLENGTH						=154  	,// MAXLENGTH
	TokAttrib_METHOD						=155  	,// METHOD
	TokAttrib_METHODS						=156  	,// METHODS
	TokAttrib_MIMETYPES						=157  	,// MIMETYPES
	TokAttrib_MULTIPLE						=158  	,// MULTIPLE
	TokAttrib_NAME							=159  	,// NAME
	TokAttrib_NOHREF						=160  	,// NOHREF
	TokAttrib_NORESIZE						=161  	,// NORESIZE
	TokAttrib_NOSHADE						=162  	,// NOSHADE
	TokAttrib_NOWRAP						=163  	,// NOWRAP
	TokAttrib_OBJECT						=164  	,// OBJECT
	TokAttrib_OFFSCREENBUFFERING			=165  	,// OFFSCREENBUFFERING
	TokAttrib_OFFSETHEIGHT					=166  	,// OFFSETHEIGHT
	TokAttrib_OFFSETLEFT					=167  	,// OFFSETLEFT
	TokAttrib_OFFSETPARENT					=168  	,// OFFSETPARENT
	TokAttrib_OFFSETTOP						=169  	,// OFFSETTOP
	TokAttrib_OFFSETWIDTH					=170  	,// OFFSETWIDTH
	TokAttrib_OFFSETX						=171  	,// OFFSETX
	TokAttrib_OFFSETY						=172  	,// OFFSETY
	TokAttrib_ONABORT						=173  	,// ONABORT
	TokAttrib_ONAFTERUPDATE					=174  	,// ONAFTERUPDATE
	TokAttrib_ONBEFOREUNLOAD				=175  	,// ONBEFOREUNLOAD
	TokAttrib_ONBEFOREUPDATE				=176  	,// ONBEFOREUPDATE
	TokAttrib_ONBLUR						=177  	,// ONBLUR
	TokAttrib_ONBOUNCE						=178  	,// ONBOUNCE
	TokAttrib_ONCHANGE						=179  	,// ONCHANGE
	TokAttrib_ONCLICK						=180  	,// ONCLICK
	TokAttrib_ONDATAAVAILABLE				=181  	,// ONDATAAVAILABLE
	TokAttrib_ONDATASETCHANGED				=182  	,// ONDATASETCHANGED
	TokAttrib_ONDATASETCOMPLETE				=183  	,// ONDATASETCOMPLETE
	TokAttrib_ONDBLCLICK					=184  	,// ONDBLCLICK
	TokAttrib_ONDRAGSTART					=185  	,// ONDRAGSTART
	TokAttrib_ONERROR						=186  	,// ONERROR
	TokAttrib_ONERRORUPDATE					=187  	,// ONERRORUPDATE
	TokAttrib_ONFILTERCHANGE				=188  	,// ONFILTERCHANGE
	TokAttrib_ONFINISH						=189  	,// ONFINISH
	TokAttrib_ONFOCUS						=190  	,// ONFOCUS
	TokAttrib_ONHELP						=191  	,// ONHELP
	TokAttrib_ONKEYDOWN						=192  	,// ONKEYDOWN
	TokAttrib_ONKEYPRESS					=193  	,// ONKEYPRESS
	TokAttrib_ONKEYUP						=194  	,// ONKEYUP
	TokAttrib_ONLOAD						=195  	,// ONLOAD
	TokAttrib_ONMOUSEOUT					=196  	,// ONMOUSEOUT
	TokAttrib_ONMOUSEOVER					=197  	,// ONMOUSEOVER
	TokAttrib_ONMOUSEUP						=198  	,// ONMOUSEUP
	TokAttrib_ONREADYSTATECHANGE			=199  	,// ONREADYSTATECHANGE
	TokAttrib_ONRESET						=200  	,// ONRESET
	TokAttrib_ONRESIZE						=201  	,// ONRESIZE
	TokAttrib_ONROWENTER					=202  	,// ONROWENTER
	TokAttrib_ONROWEXIT						=203  	,// ONROWEXIT
	TokAttrib_ONSCROLL						=204  	,// ONSCROLL
	TokAttrib_ONSELECT						=205  	,// ONSELECT
	TokAttrib_ONSELECTSTART					=206  	,// ONSELECTSTART
	TokAttrib_ONSUBMIT						=207  	,// ONSUBMIT
	TokAttrib_ONUNLOAD						=208  	,// ONUNLOAD
	TokAttrib_OPENER						=209  	,// OPENER
	TokAttrib_OUTERHTML						=210  	,// OUTERHTML
	TokAttrib_OUTERTEXT						=211  	,// OUTERTEXT
	TokAttrib_OUTLINE						=212  	,// OUTLINE
	TokAttrib_OVERFLOW						=213  	,// OVERFLOW
	TokAttrib_OWNINGELEMENT					=214  	,// OWNINGELEMENT
	TokAttrib_PADDING						=215  	,// PADDING
	TokAttrib_PADDINGBOTTOM					=216  	,// PADDINGBOTTOM
	TokAttrib_PADDINGLEFT					=217  	,// PADDINGLEFT
	TokAttrib_PADDINGRIGHT					=218  	,// PADDINGRIGHT
	TokAttrib_PADDINGTOP					=219  	,// PADDINGTOP
	TokAttrib_PAGEBREAKAFTER				=220  	,// PAGEBREAKAFTER
	TokAttrib_PAGEBREAKBEFORE				=221  	,// PAGEBREAKBEFORE
	TokAttrib_PALETTE						=222  	,// PALETTE
	TokAttrib_PARENT						=223  	,// PARENT
	TokAttrib_PARENTELEMENT					=224  	,// PARENTELEMENT
	TokAttrib_PARENTSTYLESHEET				=225  	,// PARENTSTYLESHEET
	TokAttrib_PARENTTEXTEDIT				=226  	,// PARENTTEXTEDIT
	TokAttrib_PARENTWINDOW					=227  	,// PARENTWINDOW
	TokAttrib_PATHNAME						=228  	,// PATHNAME
	TokAttrib_PIXELHEIGHT					=229  	,// PIXELHEIGHT
	TokAttrib_PIXELLEFT						=230  	,// PIXELLEFT
	TokAttrib_PIXELTOP						=231  	,// PIXELTOP
	TokAttrib_PIXELWIDTH					=232  	,// PIXELWIDTH
	TokAttrib_PLUGINS						=233  	,// PLUGINS
	TokAttrib_PLUGINSPAGE					=234  	,// PLUGINSPAGE
	TokAttrib_PORT							=235  	,// PORT
	TokAttrib_POSHEIGHT						=236  	,// POSHEIGHT
	TokAttrib_POSITION						=237  	,// POSITION
	TokAttrib_POSLEFT						=238  	,// POSLEFT
	TokAttrib_POSTOP						=239  	,// POSTOP
	TokAttrib_POSWIDTH						=240  	,// POSWIDTH
	TokAttrib_PROMPT						=241  	,// PROMPT
	TokAttrib_PROTOCOL						=242  	,// PROTOCOL
	TokAttrib_READONLY						=243  	,// READONLY
	TokAttrib_READYSTATE					=244  	,// READYSTATE
	TokAttrib_REASON						=245  	,// REASON
	TokAttrib_RECORDNUMBER					=246  	,// RECORDNUMBER
	TokAttrib_RECORDSET						=247  	,// RECORDSET
	TokAttrib_REF							=248  	,// REF
	TokAttrib_REFERRER						=249  	,// REFERRER
	TokAttrib_REL							=250  	,// REL
	TokAttrib_RETURNVALUE					=251  	,// RETURNVALUE
	TokAttrib_REV							=252  	,// REV
	TokAttrib_RIGHTMARGIN					=253  	,// RIGHTMARGIN
	TokAttrib_ROWS							=254  	,// ROWS
	TokAttrib_ROWSPAN						=255  	,// ROWSPAN
	TokAttrib_RULES							=256  	,// RULES
	TokAttrib_RUNAT							=257  	,// RUNAT
	TokAttrib_SCREENX						=258  	,// SCREENX
	TokAttrib_SCREENY						=259  	,// SCREENY
	TokAttrib_SCRIPTENGINE					=260  	,// SCRIPTENGINE
	TokAttrib_SCROLL						=261  	,// SCROLL
	TokAttrib_SCROLLAMOUNT					=262  	,// SCROLLAMOUNT
	TokAttrib_SCROLLDELAY					=263  	,// SCROLLDELAY
	TokAttrib_SCROLLHEIGHT					=264  	,// SCROLLHEIGHT
	TokAttrib_SCROLLING						=265  	,// SCROLLING
	TokAttrib_SCROLLLEFT					=266  	,// SCROLLLEFT
	TokAttrib_SCROLLTOP						=267  	,// SCROLLTOP
	TokAttrib_SCROLLWIDTH					=268  	,// SCROLLWIDTH
	TokAttrib_SEARCH						=269  	,// SEARCH
	TokAttrib_SELECTED						=270  	,// SELECTED
	TokAttrib_SELECTEDINDEX					=271  	,// SELECTEDINDEX
	TokAttrib_SELF							=272  	,// SELF
	TokAttrib_SHAPE							=273  	,// SHAPE
	TokAttrib_SHAPES						=274  	,// SHAPES
	TokAttrib_SHIFTKEY						=275  	,// SHIFTKEY
	TokAttrib_SIZE							=276  	,// SIZE
	TokAttrib_SPAN							=277  	,// SPAN
	TokAttrib_SOURCEINDEX					=278  	,// SOURCEINDEX
	TokAttrib_SRC							=279  	,// SRC
	TokAttrib_SRCELEMENT					=280  	,// SRCELEMENT
	TokAttrib_SRCFILTER						=281  	,// SRCFILTER
	TokAttrib_STANDBY						=282  	,// STANDBY
	TokAttrib_START							=283  	,// START
	TokAttrib_STARTSPAN						=284  	,// STARTSPAN
	TokAttrib_STATUS						=285  	,// STATUS
	TokAttrib_STYLE							=286  	,// STYLE
	TokAttrib_STYLEFLOAT					=287  	,// STYLEFLOAT
	TokAttrib_TABINDEX						=288  	,// TABINDEX
	TokAttrib_TAGNAME						=289  	,// TAGNAME
	TokAttrib_TARGET						=290  	,// TARGET
	TokAttrib_TEXT							=291  	,// TEXT
	TokAttrib_TEXTALIGN						=292  	,// TEXTALIGN
	TokAttrib_TEXTDECORATION				=293  	,// TEXTDECORATION
	TokAttrib_TEXTDECORATIONBLINK			=294  	,// TEXTDECORATIONBLINK
	TokAttrib_TEXTDECORATIONLINETHROUGH		=295  	,// TEXTDECORATIONLINETHROUGH
	TokAttrib_TEXTDECORATIONNONE			=296  	,// TEXTDECORATIONNONE
	TokAttrib_TEXTDECORATIONOVERLINE		=297  	,// TEXTDECORATIONOVERLINE
	TokAttrib_TEXTDECORATIONUNDERLINE		=298  	,// TEXTDECORATIONUNDERLINE
	TokAttrib_TEXTINDENT					=299  	,// TEXTINDENT
	TokAttrib_TEXTTRANSFORM					=300  	,// TEXTTRANSFORM
	TokAttrib_TITLE							=301  	,// TITLE
	TokAttrib_TOELEMENT						=302  	,// TOELEMENT
	TokAttrib_TOP							=303  	,// TOP
	TokAttrib_TOPMARGIN						=304  	,// TOPMARGIN
	TokAttrib_TRUESPEED						=305  	,// TRUESPEED
	TokAttrib_TYPE							=306  	,// TYPE
	TokAttrib_UPDATEINTERVAL				=307  	,// UPDATEINTERVAL
	TokAttrib_URL							=308  	,// URL
	TokAttrib_URN							=309  	,// URN
	TokAttrib_USEMAP						=310  	,// USEMAP
	TokAttrib_USERAGENT						=311  	,// USERAGENT
	TokAttrib_VALIGN						=312  	,// VALIGN
	TokAttrib_VALUE							=313  	,// VALUE
	TokAttrib_VERSION						=314  	,// VERSION
	TokAttrib_VERTICALALIGN					=315  	,// VERTICALALIGN
	TokAttrib_VIEWASTEXT					=316  	,// VIEWASTEXT
	TokAttrib_VISIBILITY					=317  	,// VISIBILITY
	TokAttrib_VLINK							=318  	,// VLINK
	TokAttrib_VLINKCOLOR					=319  	,// VLINKCOLOR
	TokAttrib_VOLUME						=320  	,// VOLUME
	TokAttrib_VRML							=321  	,// VRML
	TokAttrib_VSPACE						=322  	,// VSPACE
	TokAttrib_WIDTH							=323  	,// WIDTH
	TokAttrib_WRAP							=324  	,// WRAP
	TokAttrib_X								=325  	,// X
	TokAttrib_Y								=326  	,// Y
	TokAttrib_ZINDEX						=327  	// ZINDEX

} AttributeTokens;

typedef enum tagTagTokens
{
	TokTag_START 	=	1,	/* <  	*/
	TokTag_END 		=	2,	/* </ 	*/
	TokTag_CLOSE 	=	3,	/* >  	*/
	TokTag_BANG		=	4,	/* <! 	*/
	TokTag_PI 		=	5,	/* <? 	*/
	TokTag_SSSOPEN	=	6, 	/* <% 	*/
	TokTag_SSSCLOSE	=	7, 	/* %> 	*/
	TokTag_SSSOPEN_TRIEDIT	=	8, 	/* <% 	inside <script block>*/
	TokTag_SSSCLOSE_TRIEDIT	=	9 	/* %> 	inside <script block>*/
} TagTokens;

enum HtmlToken
{
    tokUNKNOWN = tokclsError,
    tokIDENTIFIER = tokclsIdentMin,     // identifier/plain text
    tokNEWLINE = tokclsUserMin,
	//
	// colored HTML items
	//
    tokElem,     // element name
    tokAttr,     // attribute name
	tokValue,    // attribute value
	tokComment,  // comment
    tokEntity,   // entity reference: e.g. "&nbsp;"
	tokTag,      // tag delimiter
	tokString,   // string
	tokSpace,    // whitespace and unrecognized text in a tag
	tokOp,       // operator
	tokSSS,      // server-side script <%...%>
	//
	// parsed HTML and SGML items - tokens folded with items above
	//
	tokName,     // NAMETOKEN
	tokNum,      // NUMTOKEN
	tokParEnt,   // parameter entity: e.g. "%name;"
	tokResName,  // reserved name
	//
	// operators - colors folded with tokOp above
	//
	tokOP_MIN,
	tokOpDash = tokOP_MIN,         // -
	tokOP_SINGLE,
	tokOpQuestion = tokOP_SINGLE,  // ?
	tokOpComma,                    // ,
	tokOpPipe,                     // |
	tokOpPlus,                     // +
	tokOpEqual,                    // =
	tokOpStar,                     // *
	tokOpAmp,                      // &
	tokOpCent,                     // %
	tokOpLP,                       // (
	tokOpRP,                       // )
	tokOpLB,                       // [
	tokOpRB,                       // ]
    tokOP_MAX,                     // token op MAX

    tokEOF
};

// the state of lexical analyser
//
// We're generally in one of two states:
// 1.  scanning text
// 2.  scanning tag info
//
// Within these states, the lexer can be in several substates.
//
// Text substates:
//
//      inText       HTML text content -- process markup
//      inPLAINTEXT  after a <PLAINTEXT> tag - remainder of file is not HTML
//      inCOMMENT    COMMENT content -- suppress all markup but </COMMENT>
//               color text as comment
//      inXMP        XMP content -- suppress all markup but </XMP>
//      inLISTING    LISTING content -- suppress all markup but </LISTING>
//		inSCRIPT	 SCRIPT content -- colorize with script engine
//
// Tag substates:
//
// inTag       inside a tag < ... >
// inBangTag   inside an SGML MDO tag <! ... >
// inPITag     inside an SGML Prcessing Instruction tag <? ... >
// inHTXTag    inside an ODBC HTML Extension template tag <% ... %>
// inEndTag    inside an end tag </name>
// inAttribute expecting an attribute
// inValue     expecting an attribute value (right of =)
// inComment   inside a comment
// inString        inside a " string, terminated by "
// inStringA   inside a ' (Alternate) string, terminated by '
//
enum HtmlLexState
{
	// tag types
	inTag        = 0x00000001, // <  ... >
	inBangTag    = 0x00000002, // <! ... >
	inPITag      = 0x00000004, // <? ... >
	inHTXTag     = 0x00000008, // <% ... %>
	inEndTag         = 0x00000010, // </ ... >

	// tag scanning states
	inAttribute  = 0x00000020,
	inValue      = 0x00000040,

	inComment    = 0x00000080,
	inString     = 0x00000100,
	inStringA    = 0x00000200,
	inScriptText = 0x00000400,
	inNestedQuoteinSSS= 0x00000800, // e.g. attr="<%if something Response.Write("X")%>"

	// text content model states
	inPLAINTEXT  = 0x00001000,
	inCOMMENT    = 0x00002000,
	inXMP        = 0x00004000,
	inLISTING    = 0x00008000,
	inSCRIPT	 = 0x00010000,

	// sublanguages
	inVariant    = 0x00F00000, // mask for sublang index
	inHTML2      = 0x00000000,
	inIExplore2  = 0x00100000,
	inIExplore3  = 0x00200000,

	//  script languages
	inJavaScript = 0x01000000,
	inVBScript   = 0x02000000,
	inServerASP  = 0x04000000, // in triedit's special script (serverside->clientside conversion)

};

// masks for subsets of the state
// These masks will not show up in the generated file
// Just copy paste these into your file.
#define INTAG (inTag|inBangTag|inPITag|inHTXTag|inEndTag)
#define INSTRING (inString|inStringA)
#define TAGMASK (INTAG|inAttribute|inValue|inComment|INSTRING)
#define TEXTMASK (inPLAINTEXT|inCOMMENT|inXMP|inLISTING)
#define STATEMASK (TAGMASK|TEXTMASK)

#endif __INC_TOKEN_H__