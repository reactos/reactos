/*----------------------------------------------------------------------------
	%%File: jislex.c
	%%Unit: fechmap
	%%Contact: jpick

	Simple converter for decoding a subset of possible ISO-2022-7 encoded
	files (ISO-2022).  Data is translated to and from Unicode.  Converter
	operates according to user options.
	
	Module currently handles ISO-2022-JP (and JIS) and ISO-2022-KR.  
	
	Converter is set up to handle ISO-2022-TW and ISO-2022-CN, but there
	are as yet no conversion tables for these.
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stddef.h>

#include "private.h"
#include "fechmap_.h"
#include "lexint_.h"


// State table for reading ISO-2022-7 encoded text
//
// Lexer recognizes the following designator sequences, used 
// to select a one or two byte character set:
//
//    <esc> $ @				-- JIS C 6626-1978	(synonym of <esc> $ ( @)
//    <esc> $ A				-- GB 2312-80		(synonym of <esc> $ ( A)
//    <esc> $ B				-- JIS X 0208-1983	(synonym of <esc> $ ( B)
//
//    <esc> $ ( @			-- JIS C 6626-1978
//    <esc> $ ( A			-- GB 2312-80
//    <esc> $ ( B			-- JIS X 0208-1983
//    <esc> $ ( C			-- KS C 5601-1992
//    <esc> $ ( D			-- JIS X 0212-1990
//    <esc> $ ( E			-- ??? (ISO-IR-165:1992) ???
//    <esc> $ ( G			-- CNS 11643-1992 Plane 1
//    <esc> $ ( H			-- CNS 11643-1992 Plane 2
//    <esc> $ ( I			-- CNS 11643-1992 Plane 3
//    <esc> $ ( J			-- CNS 11643-1992 Plane 4
//    <esc> $ ( K			-- CNS 11643-1992 Plane 5
//    <esc> $ ( L			-- CNS 11643-1992 Plane 6
//    <esc> $ ( M			-- CNS 11643-1992 Plane 7
//
//    <esc> $ ) C			-- KSC 5601-1987 (Implies ISO-2022-KR ??)
//
//    <esc> & @ <esc> $ B	-- JIS X 0208-1990
//
//    <esc> ( B 			-- Ascii
//    <esc> ( H 			-- Deprecated variant of JIS-Roman
//    <esc> ( I 			-- Half-Width Katakana
//    <esc> ( J 			-- JIS-Roman
//    <esc> ( T 			-- GB 1988-89 Roman
//
// Lexer recognizes the following shift sequences, used to allow
// interpretation of a given byte or bytes:
//
//    <si>					-- locking shift, interpret bytes as G0
//    <so>					-- locking shift, interpret bytes as G1
//    <esc> n				-- locking shift, interpret bytes as G2
//    <esc> o				-- locking shift, interpret bytes as G3
//    <esc> N				-- single shift, interpret bytes as G2
//    <esc> O				-- single shift, interpret bytes as G3
//
// REVIEW (jpick): don't currently need the final four shift
//   sequences.  If we support ISO-2022-CN, we'll need to use
//   G2 and G3 and potentially, then, the last four shifts.
//

/*----------------------------------------------------------------------------
	Character Classification Table
----------------------------------------------------------------------------*/

// Tokens
//
#define	txt			(JTK) 0
#define	ext			(JTK) 1		// extended characters that are legal under certain circumstances
#define	esc			(JTK) 2
#define	si			(JTK) 3
#define	so			(JTK) 4
#define	dlr			(JTK) 5
#define	at			(JTK) 6
#define	amp			(JTK) 7
#define	opr			(JTK) 8
#define	cpr			(JTK) 9
#define	tkA			(JTK) 10
#define	tkB			(JTK) 11
#define	tkC			(JTK) 12
#define	tkD			(JTK) 13
#define	tkE			(JTK) 14
#define	tkG			(JTK) 15
#define	tkH			(JTK) 16
#define	tkI			(JTK) 17
#define	tkJ			(JTK) 18
#define	tkK			(JTK) 19
#define	tkL			(JTK) 20
#define	tkM			(JTK) 21
#define	tkT			(JTK) 22
#define	unk			(JTK) 23	// Unexpected character
#define	eof			(JTK) 24	// end-of-file
#define	err			(JTK) 25	// read error

#define nTokens		26

// Lookup table for ISO-2022-7 encoded files
//
static JTK _rgjtkCharClass[256] =
//  0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
    {
//  nul  soh  stx  etx  eot  enq  ack  bel  bs   tab  lf   vt   np   cr   so   si		0
    txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, so,  si, 

//  dle  dc1  dc2  dc3  dc4  nak  syn  etb  can  em   eof  esc  fs   gs   rs   us		1
    txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, esc, txt, txt, txt, txt, 

//  sp   !    "    #    $    %    &    '    (    )    *    +    ,    -    .    /		2
    txt, txt, txt, txt, dlr, txt, amp, txt, opr, cpr, txt, txt, txt, txt, txt, txt, 

//  0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?		3
    txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, 

//  @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O		4
    at,  tkA, tkB, tkC, tkD, tkE, txt, tkG, tkH, tkI, tkJ, tkK, tkL, tkM, txt, txt, 

//  P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _		5
    txt, txt, txt, txt, tkT, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, 

//  `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o		6
    txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, 

//  p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~    del		7
    txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, txt, 

//																		                8
    unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, 

//																		                9
    unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, 

//																		                a
    unk, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, 

//																		                b
    ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, 

//																		                c
    ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, 

//																		                d
    ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, ext, 

//																		                e
    unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, 

//																		                f
    unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, unk, 

//  0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
};


/*----------------------------------------------------------------------------
	State Table
----------------------------------------------------------------------------*/

// Final states have the high-bit set.  States that represent the reading
// of a valid character escape sequence also encode the character set
// "name" (moniker??) -- the state with the high bit masked off.
//
// Table State
//
typedef unsigned char TST;

// Final State Mask, Related
//
#define grfFinal 							(TST) 0x80
#define _NEscTypeFromState(nState)			(int) ((nState) & 0x7f)

// ASCII Escape Sequence (Final State)
#define ASC		(TST) (grfFinal | 0x00)		// Ascii

// Japanese Escape Sequences (Final States)
#define JS0		(TST) (grfFinal | 0x01)		// JIS-Roman
#define JS1		(TST) (grfFinal | 0x02)		// Half-Width Katakana
#define JS2 	(TST) (grfFinal | 0x03)		// JIS C 6226-1978
#define JS3		(TST) (grfFinal | 0x04)		// JIS X 0208-1983
#define JS4		(TST) (grfFinal | 0x05)		// JIS X 0208-1990
#define JS5		(TST) (grfFinal | 0x06)		// JIS X 0212-1990

// Chinese (PRC) Escape Sequences (Final States)
#define CS0		(TST) (grfFinal | 0x07)		// GB 1988-89 Roman
#define CS1		(TST) (grfFinal | 0x08)		// GB 2312-80

// Chinese (Taiwan) Escape Sequences (Final States)
#define TS0		(TST) (grfFinal | 0x09)		// CNS 11643-1992 Plane 1
#define TS1		(TST) (grfFinal | 0x0a)		// CNS 11643-1992 Plane 2
#define TS2		(TST) (grfFinal | 0x0b)		// CNS 11643-1992 Plane 3
#define TS3		(TST) (grfFinal | 0x0c)		// CNS 11643-1992 Plane 4
#define TS4		(TST) (grfFinal | 0x0d)		// CNS 11643-1992 Plane 5
#define TS5		(TST) (grfFinal | 0x0e)		// CNS 11643-1992 Plane 6
#define TS6		(TST) (grfFinal | 0x0f)		// CNS 11643-1992 Plane 7

// Korean Escape Sequences (Final State)
#define KS0		(TST) (grfFinal | 0x10)		// KS C 5601-1992

// Document "Signal" for ISO-2022-KR (Doc needs special processing)
#define KSD		(TST) (grfFinal | 0x11)		// ISO-2022-KR Document Signal

// Number of unique *character set* escape sequences
//
#define cCsEsc	18

// Special States (not escape sequence) (Final States)
//
#define TXT		(TST) (grfFinal | (cCsEsc + 1))		// Process Text
#define EXT		(TST) (grfFinal | (cCsEsc + 2))		// Process (Possibly Illegal) Extended Chars
#define FIN		(TST) (grfFinal | (cCsEsc + 3))		// Finish
#define EOI		(TST) (grfFinal | (cCsEsc + 4))		// Unexpected End-Of-Input
#define UNK		(TST) (grfFinal | (cCsEsc + 5))		// Unknown State (Unexpected Character)
#define ERR		(TST) (grfFinal | (cCsEsc + 6))		// Read Error

// Shift Sequences (do not specify character set) (Final States)
//
#define LSO		(TST) (grfFinal | (cCsEsc + 7))		// Locking shift out (g1 into GL)
#define LSI		(TST) (grfFinal | (cCsEsc + 8))		// Locking shift in (g0 into GL)

// For convenience, also define constants for the sets
// that the states represent.
//
#define csNIL		(-1)							// Invalid Designator
#define csASC		(_NEscTypeFromState(ASC))		// Ascii
#define csJS0		(_NEscTypeFromState(JS0))		// JIS-Roman
#define csJS1		(_NEscTypeFromState(JS1))		// Half-Width Katakana
#define csJS2		(_NEscTypeFromState(JS2))		// JIS C 6226-1978
#define csJS3		(_NEscTypeFromState(JS3))		// JIS X 0208-1983
#define csJS4		(_NEscTypeFromState(JS4))		// JIS X 0208-1990
#define csJS5		(_NEscTypeFromState(JS5))		// JIS X 0212-1990
#define csCS0		(_NEscTypeFromState(CS0))		// GB 1988-89 Roman
#define csCS1		(_NEscTypeFromState(CS1))		// GB 2312-80
#define csTS0		(_NEscTypeFromState(TS0))		// CNS 11643-1992 Plane 1
#define csTS1		(_NEscTypeFromState(TS1))		// CNS 11643-1992 Plane 2
#define csTS2		(_NEscTypeFromState(TS2))		// CNS 11643-1992 Plane 3
#define csTS3		(_NEscTypeFromState(TS3))		// CNS 11643-1992 Plane 4
#define csTS4		(_NEscTypeFromState(TS4))		// CNS 11643-1992 Plane 5
#define csTS5		(_NEscTypeFromState(TS5))		// CNS 11643-1992 Plane 6
#define csTS6		(_NEscTypeFromState(TS6))		// CNS 11643-1992 Plane 7
#define csKS0		(_NEscTypeFromState(KS0))		// KS C 5601-1992 (into G0)
#define csKSD		(_NEscTypeFromState(KSD))		// KS C 5601-1992 (into G1)

// Table States (Intermediate States)
#define ST0		(TST)  0
#define ST1		(TST)  1
#define ST2		(TST)  2
#define ST3		(TST)  3
#define ST4		(TST)  4
#define ST5		(TST)  5
#define ST6		(TST)  6
#define ST7		(TST)  7
#define ST8		(TST)  8
#define ST9		(TST)  9

// Number of "real" (table) states
//
#define nStates		10

#define	IsFinal(state)	((state) & grfFinal)


// State	Have Seen				Looking For
// ----------------------------------------------------------
// ST0		-- Start State --		<ESC> Text
// ST1		<ESC>					$ & (
// ST2		<ESC> $					( ) @ A B   (**)
// ST3		<ESC> $ (				@ A B C D E G H I J K L M
// ST4		<ESC> $ )				C
// ST5		<ESC> &					@
// ST6		<ESC> & @				<ESC>
// ST7		<ESC> & @ <ESC>			$
// ST8		<ESC> & @ <ESC> $		B
// ST9		<ESC> (					B H I J T
//
// (**)  "<ESC> $ ID" is a synonym of "<ESC> $ ( ID" for ID=(@, A, B)
//
// Because of the large number of tokens, this table is
// inverted (tokens x states).
//
static signed char _rgchNextState[nTokens][nStates] =
{
//
//           S     S     S     S     S     S     S     S     S     S 
//           T     T     T     T     T     T     T     T     T     T  
//           0     1     2     3     4     5     6     7     8     9   
//--------------------------------------------------------------------
//
/* txt */  TXT,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* ext */  EXT,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* esc */  ST1,  UNK,  UNK,  UNK,  UNK,  UNK,  ST7,  UNK,  UNK,  UNK,
/* si  */  LSI,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* so  */  LSO,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* $   */  TXT,  ST2,  UNK,  UNK,  UNK,  UNK,  UNK,  ST8,  UNK,  UNK,
/* @   */  TXT,  UNK,  JS2,  JS2,  UNK,  ST6,  UNK,  UNK,  UNK,  UNK,
/* &   */  TXT,  ST5,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* (   */  TXT,  ST9,  ST3,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* )   */  TXT,  UNK,  ST4,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* A   */  TXT,  UNK,  CS1,  CS1,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* B   */  TXT,  UNK,  JS3,  JS3,  UNK,  UNK,  UNK,  UNK,  JS4,  ASC,
/* C   */  TXT,  UNK,  UNK,  KS0,  KSD,  UNK,  UNK,  UNK,  UNK,  UNK,
/* D   */  TXT,  UNK,  UNK,  JS5,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* E   */  TXT,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* G   */  TXT,  UNK,  UNK,  TS0,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* H   */  TXT,  UNK,  UNK,  TS1,  UNK,  UNK,  UNK,  UNK,  UNK,  JS0,
/* I   */  TXT,  UNK,  UNK,  TS2,  UNK,  UNK,  UNK,  UNK,  UNK,  JS1,
/* J   */  TXT,  UNK,  UNK,  TS3,  UNK,  UNK,  UNK,  UNK,  UNK,  JS0,
/* K   */  TXT,  UNK,  UNK,  TS4,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* L   */  TXT,  UNK,  UNK,  TS5,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* M   */  TXT,  UNK,  UNK,  TS6,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* T   */  TXT,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  CS0,
/* unk */  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,
/* eof */  FIN,  EOI,  EOI,  EOI,  EOI,  EOI,  EOI,  EOI,  EOI,  EOI,
/* err */  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,
};


// Also for ISO-2022 out.  Build arrays of possible character
// sets for each type of input character set.  Character sets
// should appear in order of hit probability (e.g., in 2022-Jp
// JS3 is the most common set).  Mark the end of array with -1.
// (Only store these for non-ascii sets).
//
//
// China (icetIso2022Cn)
static int _rgceCn[] = { -1, };

// Japan (icetIso2022Jp)
static int _rgceJp[] = { csJS3, csJS1, csJS5, -1, };

// Korea (icetIso2022Kr)
static int _rgceKr[] = { -1, };

// Taiwan (icetIso2022Tw)
static int _rgceTw[] = { -1, };

static int *_mpicetrgce[icetCount] =
	{
	0,				// icetEucCn
	0,				// icetEucJp
	0,				// icetEucKr
	0,				// icetEucTw
	_rgceCn,		// icetIso2022Cn
	_rgceJp,		// icetIso2022Jp
	_rgceKr,		// icetIso2022Kr
	_rgceTw,		// icetIso2022Tw
	0,				// icetBig5
	0,				// icetGbk
	0,				// icetShiftJis
	0,				// icetWansung
	0,				// icetUtf8
	};

/* _ J T K  G E T  N E X T */
/*----------------------------------------------------------------------------
	%%Function: _JtkGetNext
	%%Contact: jpick

	Get the next character and classify it.  Return the token.
----------------------------------------------------------------------------*/
static JTK __inline _JtkGetNext(IStream *pstmIn, PUCHAR puch)
{
	ULONG rc;
    HRESULT hr;
		  
    hr = pstmIn->Read(puch, 1, &rc);
	
	if (hr != S_OK )
		return err;
	else if (rc == 0)
		return eof;
	else
		return _rgjtkCharClass[*puch];
}

/* C C E  R E A D  E S C  S E Q */
/*----------------------------------------------------------------------------
	%%Function: CceReadEscSeq
	%%Contact: jpick

	Read pointer is positioned at an escape sequence, figure out
	which escape sequence it is.
----------------------------------------------------------------------------*/
CCE CceReadEscSeq(IStream *pstmIn, ICET *lpicet)
{
    UCHAR uch;
	TST tstCurr;
	JTK jtk;
	CCE cceRet;
#ifdef DEBUG
	TST tstPrev;
#endif

	// Sanity checks ...
	//
#ifdef DEBUG
	if (!pstmIn || !lpicet)
		return cceInvalidParameter;
#endif
		
	tstCurr = ST0;

	while (1)
		{
		// Find the next stopping state.
		//
		do
			{
			// Get the next character and clasify it.
			//
			jtk = _JtkGetNext(pstmIn, &uch);
				
#ifdef DEBUG
			// Save the previous state for debugging purposes, only.
			//
			tstPrev = tstCurr;
#endif
			// Transition -- note that order is different than
			// "normal" transition tables.
			//
			tstCurr = _rgchNextState[jtk][tstCurr];
		
			} while (!IsFinal(tstCurr));
		
		switch (tstCurr)
			{
			case JS0:			// JIS-Roman
			case JS1:			// Half-Width Katakana
			case JS2:			// JIS C 6226-1978
			case JS3:			// JIS X 0208-1983
			case JS4:			// JIS X 0208-1990
			case JS5:			// JIS X 0212-1990
				*lpicet = icetIso2022Jp;
				cceRet = cceSuccess;
				goto _LRet;
			case CS0:			// GB 1988-89 Roman
			case CS1:			// GB 2312-80
				*lpicet = icetIso2022Cn;
				cceRet = cceSuccess;
				goto _LRet;
			case TS0:			// CNS 11643-1992 Plane 1
			case TS1:			// CNS 11643-1992 Plane 2
			case TS2:			// CNS 11643-1992 Plane 3
			case TS3:			// CNS 11643-1992 Plane 4
			case TS4:			// CNS 11643-1992 Plane 5
			case TS5:			// CNS 11643-1992 Plane 6
			case TS6:			// CNS 11643-1992 Plane 7
				*lpicet = icetIso2022Tw;
				cceRet = cceSuccess;
				goto _LRet;
			case KS0:			// KS C 5601-1992
			case KSD:			// ISO-2022-KR Document Signal
				*lpicet = icetIso2022Kr;
				cceRet = cceSuccess;
				goto _LRet;
			case ASC:			// Ascii
			case LSO:
			case LSI:
			case TXT:
			case EXT:
			case FIN:
				// Insufficient information to choose a flavor ...
				cceRet = cceMayBeAscii;
				goto _LRet;
			case ERR:
				cceRet = cceRead;
				goto _LRet;
			default:			// UNK, EOI
				cceRet = cceUnknownInput;
				goto _LRet;
			}
		}
		
_LRet:

	return cceRet;
}
