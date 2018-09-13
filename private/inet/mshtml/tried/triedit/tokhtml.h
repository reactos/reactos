// tokhtml.h - Tokens and lex state for HTML
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//
// Include lex.h before including this file.
//

#ifndef __TOKHTML_H__
#define __TOKHTML_H__

#if 0
// Moved to the IDL
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
// 	inText       HTML text content -- process markup
//	inPLAINTEXT  after a <PLAINTEXT> tag - remainder of file is not HTML
//	inCOMMENT    COMMENT content -- suppress all markup but </COMMENT>
//               color text as comment
//	inXMP        XMP content -- suppress all markup but </XMP>
//	inLISTING    LISTING content -- suppress all markup but </LISTING>
//	inSCRIPT	 SCRIPT content -- color using script engine.
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
// inString	   inside a " string, terminated by "
// inStringA   inside a ' (Alternate) string, terminated by '
//
enum HtmlLexState
{
	// tag types
	inTag        = 0x00000001, // <  ... >
	inBangTag    = 0x00000002, // <! ... >
	inPITag      = 0x00000004, // <? ... >
	inHTXTag     = 0x00000008, // <% ... %>
	inEndTag	 = 0x00000010, // </ ... >

	// tag scanning states
	inAttribute  = 0x00000020,
	inValue      = 0x00000040,

	inComment    = 0x00000080,
	inString     = 0x00000100,
	inStringA    = 0x00000200,

	// text content model states
	inPLAINTEXT  = 0x00001000,
	inCOMMENT    = 0x00002000,
	inXMP        = 0x00004000,
	inLISTING    = 0x00008000,
	inSCRIPT     = 0x00010000,

	// sublanguages
	inVariant    = 0x00F00000, // mask for sublang index
	inHTML2      = 0x00000000,
	inIExplore2  = 0x00100000,
	inIExplore3  = 0x00200000,

	//  script languages
	inJavaScript = 0x01000000,
	inVBScript   = 0x02000000,

};

// masks for subsets of the state
#define INTAG (inTag|inBangTag|inPITag|inHTXTag|inEndTag)
#define INSTRING (inString|inStringA)
#define TAGMASK (INTAG|inAttribute|inValue|inComment|INSTRING)
#define TEXTMASK (inPLAINTEXT|inCOMMENT|inXMP|inLISTING|inSCRIPT)
#define STATEMASK (TAGMASK|TEXTMASK)

#endif


// convert state <-> sublang index
inline DWORD SubLangIndexFromLxs(DWORD lxs) { return (lxs & inVariant) >> 20UL; }
inline DWORD LxsFromSubLangIndex(DWORD isl) { return (isl << 20UL) & inVariant; }

#endif // __TOKHTML_H__

