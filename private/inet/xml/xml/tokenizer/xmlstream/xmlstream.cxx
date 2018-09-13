/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include <shlwapip.h>   // IsCharSpace
#ifdef UNIX
// Not needed under UNIX
#else
#ifndef _WIN64
#include <w95wraps.h>
#endif // _WIN64
#endif /* UNIX */


#ifdef _DEBUG
#include "msxmldbg.h"
DeclareTag(tagTokenizer, "XMLStream", "XML Tokenizer");
#endif

#include "xmlstream.hxx"
#include "bufferedstream.hxx"
//#include "htmlent.hxx"
#include "chartype.hxx"
#include "chartype.cxx"
#include "datatype.hxx"
#include "htmlent.hxx"
#include "xmlparser.hxx"

const long BLOCK_SIZE = 512;
const long STACK_INCREMENT = 10;

#define INTERNALERROR return XML_E_INTERNALERROR;

// a is NULL terminated, and b is not.
bool StringEquals(const WCHAR* a, const WCHAR* b, long size, bool caseInsensitive)
{
    if (! caseInsensitive)
    {
        return (::StrCmpNW(a, b, size) == 0) && ((ULONG)size == ::StrLen(a));
    }
    else
    {
        // BUGBUG -- is this smart about real unicode case insensitivity
        // for all locales ??
        return (::StrCmpNIW(a, b, size) == 0) && ((ULONG)size == ::StrLen(a));
    }    
    return false;
}

#define checkeof(a,b) if (_fEOF) return b;

#define ADVANCE hr = (!_fDTD) ? _pInput->nextChar(&_chLookahead, &_fEOF) : DTDAdvance(); if (hr != S_OK) return hr;

#define ADVANCETO(a) hr = AdvanceTo(a);  if (hr != S_OK) return hr;

// isWhiteSpace is now implemented in the BufferedStream, since we have to
// count new lines anyway - it is more efficient to do it there.
#define ISWHITESPACE(ch) _pInput->isWhiteSpace(ch) 

#define STATE(state) { _sSubState = state; return S_OK; }
#define GOTOSTART(state) { _sSubState = state; goto Start; }


// The tokenizer has special handling for the following attribute types.
// These values are derived from the XML_AT_XXXX types provided in SetType
// and are also calculated during parsing of an ATTLIST for parsing of
// default values.
typedef enum 
{
    XMLTYPE_CDATA,       // the default.
    XMLTYPE_NAME,
    XMLTYPE_NAMES,
    XMLTYPE_NMTOKEN,
    XMLTYPE_NMTOKENS,
} XML_ATTRIBUTE_TYPE;

//==============================================================================
// The DTD stuff uses parsing tables to avoid code bloat.
// Eventually we could even compress these tables and store them in the
// resource section.  There's also plenty of opportunity to make the tables
// smaller by making the opcodes smarter.

// Parse an entity declaration.
const StateEntry g_EntityDeclTable[] =
{
// 0    '<!ENTITY'^ S ('%' S)? Name S ...
    { OP_PETEST, NULL, 1, 0,  },

// 1    '<!ENTITY' S Name ^ S ...
    { OP_WS, NULL, 2, 0,   },
// 2    '<!ENTITY' ... ^ (ExternalID | EntityValue ) ...
    { OP_PEEK, L"\"", 4, 3, 0  },
// 3
    { OP_PEEK, L"'", 4, 9, 0  },
// 4    '<!ENTITY' ... "^EntityValue" ) ...
    { OP_STRING, NULL, 5, 0,   },
// 5   '<!ENTITY' ... "EntityValue" ^ S? '>'
    { OP_TOKEN, NULL, 6, XML_STRING, -1  },
// 6   '<!ENTITY' ... "EntityValue" ^ S? '>'
    { OP_OWS, NULL, 7, 0, 0  },
// 7   '<!ENTITY' ... "EntityValue" S? ^ '>'    
    { OP_CHAR, L">", 8, XML_E_EXPECTINGTAGEND,  0 },
// 8
    { OP_POP, NULL, 0, XMLStream::XML_ENDDECL, 0  },
// 9   '<!ENTITY' ... ^ ExternalID ...
    { OP_EXTID, NULL, 10, 0,   },
// 10   '<!ENTITY' ... ExternalID ^ (S 'NDATA' Name)? '>'
    { OP_NWS, NULL, 7, 11, },
// 11
    { OP_OWS, NULL, 12, 0,   },
// 12   '<!ENTITY' ... ExternalID S ^ ('NDATA' Name | '>')
    { OP_CHAR, L">", 8, 13,  0 },
// 13   '<!ENTITY' ... ExternalID S ^ 'NDATA' Name '>'
    { OP_NAME,  NULL, 14, 0,   },
// 14   '<!ENTITY' ... ExternalID S  'NDATA' ^ Name '>'
    { OP_STRCMP, L"NDATA", 15, XML_E_EXPECTING_NDATA, XML_NDATA },
// 15   '<!ENTITY' ... ExternalID S  'NDATA' ^ S Name '>'
    { OP_WS, NULL, 16, 0,   },
// 16   '<!ENTITY' ... ExternalID S  'NDATA' S ^ Name '>'
    { OP_NAME, NULL, 17, 0, },
// 17   '<!ENTITY' ... ExternalID S  'NDATA' S Name ^ '>'
    { OP_TOKEN, NULL, 6, XML_NAME, 0 },
};


//==============================================================================
// Parse an NOTATION declaration.
const StateEntry g_NotationDeclTable[] =
{
// 0    '<!NOTATION' ^ S Name S (ExternalID | PublicID) S? >
    { OP_WS, NULL, 1, 0,  },                    
// 1    '<!NOTATION' S ^ Name S (ExternalID | PublicID) S? >
    { OP_NAME,  NULL, 2,   },                 
// 2    '<!NOTATION' S Name ^ S ...
    { OP_TOKEN,  NULL, 3, XML_NOTATION, 0  },

// 3    '<!NOTATION' S Name ^ S (ExternalID | PublicID) S? >
    { OP_WS, NULL, 4, 0,   },
// 4    '<!NOTATION' S Name S ^ (ExternalID | PublicID) S? >
    { OP_EXTID, NULL, 5, 1,   }, // _fShortPubIdOption=true
// 5    '<!NOTATION' S Name S (ExternalID | PublicID) ^ S? >
    { OP_OWS, NULL, 6, 0, },
// 6    '<!NOTATION' S Name S (ExternalID | PublicID) S? ^ >
    { OP_CHAR, L">", 7, XML_E_EXPECTINGTAGEND, 0  },
// 7    '<!NOTATION' S Name S (ExternalID | PublicID) S? > ^
    { OP_POP, NULL, 0, XMLStream::XML_ENDDECL, 0  },
};

//==============================================================================
// Parse an the DOCTYPE declaration.
const StateEntry g_DocTypeTable[] =
{
// 0    '<!DOCTYPE^ S Name (S ExternalID)? S? ...
    { OP_WS, NULL, 1, 0  },
// 1    '<!DOCTYPE S ^ Name (S ExternalID)? S? ...
    { OP_NAME, NULL, 2, 0  }, 
// 2    '<!DOCTYPE S Name ^ (S ExternalID)? S? ...
    { OP_TOKEN, NULL, 3, XML_DOCTYPE, 0 }, 
// 3    '<!DOCTYPE S Name ^ (S ExternalID)? S? ('['...']' S?)? '>'
    { OP_NWS, NULL, 9, 4 },
// 4    '<!DOCTYPE S Name ^ S
    { OP_OWS, NULL, 5, 0 },
// 5    '<!DOCTYPE S Name S ^ (ExternalID|'['|'>')
    { OP_CHAR, L"[", 10, 6, 0 },
// 6    '<!DOCTYPE S Name S ^ (ExternalID|'>')
    { OP_CHAR, L">", 13, 7, 0 },
// 7    '<!DOCTYPE S Name S ^ ExternalID
    { OP_EXTID, NULL, 8, 0 },
// 8    '<!DOCTYPE S Name S ExternalID ^ S?
    { OP_OWS, NULL, 9, 0 },
// 9    '<!DOCTYPE ... ^ ('[' | '>') 
    { OP_CHAR, L"[", 10, 11, 0 },
// 10    '<!DOCTYPE ...  '[' ^ 
    { OP_SUBSET, NULL, 11, 0 }, // scan the internal subset
// 11   <!DOCTYPE ...  [...] ^ '>'
    { OP_OWS, NULL, 12, 0 },
// 12
    { OP_CHAR, L">", 13, XML_E_EXPECTINGTAGEND, 0 },
// 13
    { OP_POP, NULL, 0, XMLStream::XML_ENDDECL, 0  },
};


//==============================================================================
// Parse an an external id.
const StateEntry g_ExternalIDTable[] =
{
// 0    ^ ( 'PUBLIC' publid [syslit] | 'SYSTEM' syslit)
    { OP_NAME, NULL, 1, 0,  },                    
// 1    ( 'PUBLIC' | 'SYSTEM' ) ^ ...
    { OP_STRCMP, L"PUBLIC", 3, 2, XML_PUBLIC },
// 2    ( 'PUBLIC' | 'SYSTEM' ) ^ ...
    { OP_STRCMP, L"SYSTEM", 13, XML_E_BADEXTERNALID, XML_SYSTEM },
// 3   'PUBLIC' ^ S publid S [syslit]
    { OP_WS, NULL, 4, 0,   },
// 4   'PUBLIC' S ^ publid S [syslit]
    { OP_ATTRVAL, NULL, 5, 0, },
// 5   'PUBLIC' S publid ^ S [syslit]
    { OP_TOKEN, NULL, 6, XML_PCDATA, -1 },
// 6   'PUBLIC' publid ^ [syslit]       (conditional upon _fShortPubIdOption)
    { OP_PUBIDOPTION, NULL, 7, 11, 0 },
// 7   then system literal is optional
    { OP_NWS, NULL, 16, 8 }, // if no whitespace then we're done !
// 8   soak up the white space.
    { OP_WS, NULL, 9,  },
// 9  if we have a quote character, we must have a system literal
    { OP_PEEK, L"\"", 12, 10, 0  },
// 10
    { OP_PEEK, L"'", 12, 16, 0  },  // otherwise we're done.
// 11   'SYSTEM' ^ S syslit
    { OP_WS, NULL, 12, 0,   },
// 12
    { OP_FAKESYSTEM, NULL, 14 },    // and fake a SYSTEM attribute
// 13   'SYSTEM' ^ S syslit
    { OP_WS, NULL, 14, 0,   },
// 14   'SYSTEM' S ^ syslit
    { OP_ATTRVAL, NULL, 15, 0,   },
// 15   'SYSTEM' S syslit ^ 
    { OP_TOKEN, NULL, 16, XML_PCDATA, -1 },
// 16   must be time to return then.
    { OP_POP, NULL, 0, 0, 1 },
};


//==============================================================================
// --- This is brain dead because the OP_CODES are not a real programming
// language.  This would be a lot simpler if I had a STORE and RETRIEVE opcode
// so that I could remember which content model (sequence or choice) I was
// handling.  For now it is just a cut&paste solution for each model type.

const StateEntry g_ContentModelTable[] =
{
// 0    '(' ^ S? Name | choice | seq ')' ('?'|'*'|'+')?
    { OP_OWS, NULL, 1, 0, },
// 1    '(' S? ^ Name | choice | seq ')' ('?'|'*'|'+')?
    { OP_CHAR, L"(", 2, 3, 0 },
// 2    nested model - have to push states.
    { OP_TABLE, (const WCHAR*)g_ContentModelTable, 5, XML_GROUP },
// 3    '(' S? ^ Name 
    { OP_NAME, NULL, 4, },
// 4    '(' S? Name ^
    { OP_TOKEN, NULL, 5, XML_NAME, },
// 5    '(' S? Name ^ ('?'|'*'|'+')? S? ')'
    { OP_CHAR, L"?", 8, 6, XML_QUESTIONMARK },
//  6
    { OP_CHAR, L"*", 8, 7, XML_STAR },
//  7
    { OP_CHAR, L"+", 8, 8, XML_PLUS },
//  8
    { OP_OWS, NULL, 9, 0, },
//  9
    { OP_CHAR, L"|", 13, 10, XML_CHOICE }, // Ah ha, it's a CHOICE model
//  10
    { OP_CHAR, L",", 24, 11, XML_SEQUENCE }, // Ah ha, it's a SEQUENCE model 
//  11
    { OP_CHAR, L")", 35, XML_E_BADCHARINMODEL, XMLStream::XML_CLOSEPAREN }, 
//  12
    { OP_POP, NULL, 0, 0  },     // CAN REMOVE THIS STATE.

//  13  ----- Handle a CHOICE model
    { OP_OWS, NULL, 14, 0, },
//  14    '(' S? ^ Name | choice | seq ')' ('?'|'*'|'+')?
    { OP_CHAR, L"(", 15, 16, 0 },
//  15    nested model - have to push states.
    { OP_TABLE, (const WCHAR*)g_ContentModelTable, 18, XML_GROUP  },
//  16    '(' Name S? '|' S? ^ Name (S? '|' S? Name)* ')'
    { OP_NAME, NULL, 17, },
//  17    '(' Name S? '|' S? Name ^ (S? '|' S? Name)* ')'
    { OP_TOKEN, NULL, 18, XML_NAME, 0},
//  18    '(' S? Name ^ ('?'|'*'|'+')? S? ')'
    { OP_CHAR, L"?", 21, 19, XML_QUESTIONMARK },
//  19
    { OP_CHAR, L"*", 21, 20, XML_STAR },
//  20
    { OP_CHAR, L"+", 21, 21, XML_PLUS },
//  21
    { OP_OWS, NULL, 22, 0, },
//  22
    { OP_CHAR, L"|", 13, 23, XML_CHOICE }, // We know this is a CHOICE model
//  23
    { OP_CHAR, L")", 35, XML_E_BADCHARINMODEL, XMLStream::XML_CLOSEPAREN }, 

//  24  ----- Handle a SEQUENCE model
    { OP_OWS, NULL, 25, 0, },
//  25    '(' S? ^ Name | choice | seq ')' ('?'|'*'|'+')?
    { OP_CHAR, L"(", 26, 27, 0 },
//  26    nested model - have to push states.
    { OP_TABLE, (const WCHAR*)g_ContentModelTable, 29, XML_GROUP  },
//  27    '(' Name S? '|' S? ^ Name (S? '|' S? Name)* ')'
    { OP_NAME, NULL, 28, },
//  28    '(' Name S? '|' S? Name ^ (S? '|' S? Name)* ')'
    { OP_TOKEN, NULL, 29, XML_NAME, 0},
//  29    '(' S? Name ^ ('?'|'*'|'+')? S? ')'
    { OP_CHAR, L"?", 32, 30, XML_QUESTIONMARK },
//  30
    { OP_CHAR, L"*", 32, 31, XML_STAR },
//  31
    { OP_CHAR, L"+", 32, 32, XML_PLUS },
//  32
    { OP_OWS, NULL, 33, 0, },
//  33
    { OP_CHAR, L",", 24, 34, XML_SEQUENCE }, // We know this is a SEQUENCE model
//  34
    { OP_CHAR, L")", 35, XML_E_BADCHARINMODEL, XMLStream::XML_CLOSEPAREN }, 

//  --- trailing '?' | '+' | '*' characters after closing ')'
//  35    '(' model ')' ^ ('?'|'*'|'+')? 
    { OP_CHAR, L"?", 38, 36, XML_QUESTIONMARK },
//  36
    { OP_CHAR, L"*", 38, 37, XML_STAR },
//  37
    { OP_CHAR, L"+", 38, 38, XML_PLUS },
//  38
    { OP_POP, NULL, 0, 0, 0  },
};

//==============================================================================
// Parse an a mixed content model (#PCDATA|A|B) or switch to full content model
// if no #PCDATA

const StateEntry g_MixedModelTable[] =
{
// 0   '<!ELEMENT' S Name '(' ^ S? ( '#MIXED' | '(' | Name )
    { OP_OWS, NULL, 1 },
// 1   '<!ELEMENT' S Name '(' S? ^ ( '#MIXED' | '(' | Name )
    { OP_CHAR, L"#", 2, 16, 0 }, 

// ---Ok, it's a mixed model
// 2   '<!ELEMENT' S Name '(' S? '#^PCDATA' 
    { OP_NMTOKEN, NULL, 3, 0, 0 }, 
// 3   '<!ELEMENT' S Name '(' S? '#PCDATA^' 
    { OP_STRCMP, L"#PCDATA", 4, XML_E_INVALID_MODEL, XML_MIXED},
// 4   '(' S? '#MIXED' ^ (( S? '|' S? Name)* S? ')*') | (S? ')')
    { OP_OWS, NULL, 5 },
// 5   '(' S? '#MIXED' (( S? ^ '|' S? Name)* S? ')*') | (S? ^ ')')
    { OP_CHAR, L")", 17, 6, XMLStream::XML_CLOSEPAREN },
// 6   '(' S? '#MIXED' (( S? ^ '|' S? Name)* S? ')*') | (S? ^ ')')
    { OP_CHAR, L"|", 7, XML_E_BADCHARINMIXEDMODEL, XML_CHOICE },

// ---Ok, it has some names too.    (#PCDATA|a|b|c...)
// 7   '(' S? '#MIXED' '|' ^ S? Name
    { OP_OWS, NULL, 8, 0, },
// 8   '(' S? '#MIXED' '|'  S? ^ Name
    { OP_NAME, NULL, 9, 0, },
// 9   '(' S? '#MIXED' '|'  S?  Name ^ 
    { OP_TOKEN, NULL, 10, XML_NAME, 0}, 
// ---(cannot use state 14 because of different terminator ')*'.
// 10   '(' S? '#MIXED' ^ (( S? '|' S? Name)* S? ')*') | (S? ')*')
    { OP_OWS, NULL, 11},
// 11   '(' S? '#MIXED' (( S? ^ '|' S? Name)* S? ')*') | (S? ^ ')*')
    { OP_CHAR, L")", 13, 12, XMLStream::XML_CLOSEPAREN },
// 12   '(' S? '#MIXED' (( S? ^ '|' S? Name)* S? ')*') | (S? ^ ')*')
    { OP_CHAR, L"|", 7, XML_E_BADCHARINMIXEDMODEL, XML_CHOICE },
// 13   '(' S? '#MIXED' ')' ^ '*'
    { OP_CHAR, L"*", 14, XML_E_MISSING_STAR, 0},  
// 14   '(' S? '#MIXED' ')*' ^
    { OP_TOKEN, NULL, 15, XML_STAR, 0}, 
// 15   Ok, we're done !
    { OP_POP, NULL, 0, 0 },

// 16 ---  switch to full blown content model (not #PCDATA)
    { OP_STABLE, (const WCHAR*)g_ContentModelTable, 0 },

// 17   '(' S? '#MIXED' ')' ^ '*'
    { OP_CHAR, L"*", 14, 15, 0},  
};

//==============================================================================
// Parse an <!ELEMENT declaration.
const StateEntry g_ElementDeclTable[] =
{
// 0    '<!ELEMENT' ^ S Name 
    { OP_WS, NULL, 1, 0,  },                    
// 1    '<!ELEMENT' S ^ Name 
    { OP_NAME,  NULL, 2,   },                 
// 2    '<!ELEMENT' S Name ^ 
    { OP_TOKEN,  NULL, 3, XML_ELEMENTDECL, 0  },

// 3    '<!ELEMENT' S Name ^ S contentSpec
    { OP_WS, NULL, 4, 0,   },
// 4    ^ 'EMPTY' | 'ANY' | '(' ... ')'
    { OP_CHAR, L"(", 8, 5, 0 },
// 5    ^ 'EMPTY' | 'ANY'
    { OP_NAME,  NULL, 6,   },                 
// 6    ^ 'EMPTY' | 'ANY'
    { OP_STRCMP,  L"EMPTY", 9, 7, XML_EMPTY },
// 7    ^ 'EMPTY' | 'ANY'
    { OP_STRCMP,  L"ANY", 9, XML_E_INVALID_MODEL, XML_ANY },                 

// 8 return XML_GROUP token and find out if it is a mixed content model or a regular full blown model.
    { OP_TABLE, (const WCHAR*)g_MixedModelTable, 9, XML_GROUP },

// 9   '<!ELEMENT' S Name contentSpec ^ S? >
    { OP_OWS, NULL, 10, 0, },
// 10   '<!ELEMENT' S Name contentSpec  S? ^ >
    { OP_CHAR, L">", 11, XML_E_EXPECTINGTAGEND, 0},
// 11   '<!ELEMENT' S Name contentSpec  S? > ^
    { OP_POP, NULL, 0, XMLStream::XML_ENDDECL, 0  },
  
};

//==============================================================================
// Parse an <!ATTLIST declaration.
const StateEntry g_AttListTable[] =
{
// 0    '<!ATTLIST' ^ S Name 
    { OP_WS, NULL, 1, 0,  },                    
// 1    '<!ATTLIST' S ^ Name 
    { OP_NAME,  NULL, 2,   },                 
// 2    '<!ATTLIST' S Name ^ AttDef* '>'
    { OP_TOKEN,  NULL, 3, XML_ATTLISTDECL, 0  },

// 3    ^ (S Name S AttType S Default) | S? '>'
    { OP_OWS, NULL, 4 },
// 4    ^ (S Name S AttType S Default) | '>'
    { OP_CHAR,  L">", 5, 7, 0  },
// 5    '>' ^
    { OP_POP, NULL, 0, XMLStream::XML_ENDDECL, 0  },

// 6    ^ S Name S AttType S Default
    { OP_WS, NULL, 7, },            // <------- can delete this state...
// 7    S ^ Name S AttType S Default
    { OP_NAME, NULL, 8, },
// 8    S Name ^ S AttType S Default
    { OP_TOKEN,  NULL, 9, XML_ATTDEF, 0  },
// 9    S Name ^ S AttType S Default
    { OP_WS, NULL, 10, },
// 10    S Name S ^ 'CDATA' | 'ID' | ... | '(' enumeration
    { OP_CHAR, L"(", 32, 11, XML_GROUP }, 
// 11   must be a type name.
    { OP_NAME, NULL, 12, },
// 12   must be a type name.
    { OP_STRCMP, L"CDATA", 23, 13, XML_AT_CDATA},
// 13
    { OP_STRCMP, L"ID", 23, 14, XML_AT_ID},
// 14
    { OP_STRCMP, L"IDREF", 23, 15, XML_AT_IDREF},
// 15
    { OP_STRCMP, L"IDREFS", 23, 16, XML_AT_IDREFS},
// 16
    { OP_STRCMP, L"ENTITY", 23, 17, XML_AT_ENTITY},
// 17
    { OP_STRCMP, L"ENTITIES", 23, 18, XML_AT_ENTITIES},
// 18
    { OP_STRCMP, L"NMTOKEN", 23, 19, XML_AT_NMTOKEN},
// 19
    { OP_STRCMP, L"NMTOKENS", 23, 20, XML_AT_NMTOKENS},
// 20
    { OP_STRCMP, L"NOTATION", 21, XML_E_INVALID_TYPE, XML_AT_NOTATION},
// 21    NOTATION ^ S '('
    { OP_WS, NULL, 22, },
// 22    NOTATION S ^ '('
    { OP_CHAR, L"(", 38, XML_E_MISSING_PAREN, XML_GROUP }, 

// 23    AttType ^ S Default
    { OP_WS, NULL, 24, },
// 24    AttType S ^ Default
    { OP_CHAR, L"#", 25, 30, 0 },
// 25    AttType S ^ Default
    { OP_NMTOKEN, NULL, 26, 0, 0},  
// 26
    { OP_STRCMP, L"#REQUIRED", 3, 27, XML_AT_REQUIRED},
// 27
    { OP_STRCMP, L"#IMPLIED", 3, 28, XML_AT_IMPLIED},
// 28
    { OP_STRCMP, L"#FIXED", 29, XML_E_INVALID_PRESENCE, XML_AT_FIXED},
// 29    AttType S ^ S Default
    { OP_WS, NULL, 30, },
// 30    AttType S ^ Default
    { OP_ATTRVAL, NULL, 3, 1, 0},
// 31    ZOMBIE STATE -- CAN BE REMOVED.
    { OP_OWS, NULL, 0, 0, 0},

// 32 -- enumerated type. '(' ^ S? NmToken S '|' S NmToken ... ')'
    { OP_OWS, NULL, 33 },
// 33
    { OP_NMTOKEN, NULL, 34, 1, 0 }, // 1 means do input->Mark
// 34   '(' S? NmToken ^ S '|' S NmToken ... ')'
    { OP_TOKEN, NULL, 35, XML_NMTOKEN },
// 35   '(' S? NmToken ^ ( S '|' S NmToken)? S? ')'
    { OP_OWS, NULL, 36 },
// 36
    { OP_CHAR, L"|", 32, 37, 0 },
// 37
    { OP_CHAR, L")", 23, XML_E_BADCHARINENUMERATION, XMLStream::XML_CLOSEPAREN },

// 38 -- notation type. '(' ^ S? name S '|' S name ... ')'
    { OP_OWS, NULL, 39 },
// 39
    { OP_NAME, NULL, 40,  }, 
// 40   '(' S? name ^ S '|' S name ... ')'
    { OP_TOKEN, NULL, 41, XML_NAME },
// 41   '(' S? name ^ ( S '|' S name)? S? ')'
    { OP_OWS, NULL, 42 },
// 42
    { OP_CHAR, L"|", 38, 43, 0 },
// 43
    { OP_CHAR, L")", 23, XML_E_BADCHARINENUMERATION, XMLStream::XML_CLOSEPAREN },

};

//==============================================================================
// Parse an <!^xxxxxxxx Declaration.
const StateEntry g_DeclarationTable[] =
{
// 0    '<' ^ '!' 
    { OP_CHAR, L"!", 1, XML_E_INTERNALERROR,  },                    
// 1    '<!' ^ '-'
    { OP_PEEK, L"-", 2, 4, 0 },                    
// 2    '<!-'
    { OP_COMMENT,  NULL, 3,   },                 
// 3    done !!
    { OP_POP,  NULL, 0, 0 },

// 4    '<!' ^ '['
    { OP_PEEK, L"[", 5, 6, 0 },                    
// 5    '<![...'
    { OP_CONDSECT,  NULL, 3,   },                 

// 6    '<!' ^ Name
    { OP_NAME, NULL, 7 },

// 7    '<!' Name ^ S
    { OP_STRCMP,  L"DOCTYPE", 8, 9, 0 },
// 8    '<!' DOCTYPE ^ S
    { OP_STABLE, (const WCHAR*)g_DocTypeTable, 0 },

// 9    '<!' Name ^ S
    { OP_STRCMP,  L"ELEMENT", 10, 11, 0 },
// 10    '<!' ELEMENT ^ S
    { OP_STABLE, (const WCHAR*)g_ElementDeclTable, 0 },

// 11    '<!' Name ^ S
    { OP_STRCMP,  L"ENTITY", 12, 13, 0 },
// 12    '<!' ELEMENT ^ S
    { OP_STABLE, (const WCHAR*)g_EntityDeclTable, 0 },

// 13    '<!' Name ^ S
    { OP_STRCMP,  L"ATTLIST", 14, 15, 0 },
// 14    '<!' ELEMENT ^ S
    { OP_STABLE, (const WCHAR*)g_AttListTable, 0 },

// 15    '<!' Name ^ S
    { OP_STRCMP,  L"NOTATION", 16, XML_E_BADDECLNAME, 0 },
// 16    '<!' ELEMENT ^ S
    { OP_STABLE, (const WCHAR*)g_NotationDeclTable, 0 },

};


//==============================================================================
// Parse an <?xml or <?xml:namespace declaration.
const StateEntry g_XMLDeclarationTable[] =
{
// 0    must be xml declaration - and not xml namespace declaration        
    { OP_TOKEN, NULL, 1, XML_XMLDECL, 0 },
// 1    '<?xml' ^ S version="1.0" ...
    { OP_OWS, NULL, 2 },
// 2    '<?xml' S ^ version="1.0" ...
    { OP_SNCHAR, NULL, 3, XML_E_XMLDECLSYNTAX },
// 3    '<?xml' S ^ version="1.0" ...
    { OP_NAME, NULL, 4, },
// 4    '<?xml' S version^="1.0" ...
    { OP_STRCMP, L"version", 5, 12, XML_VERSION },
// 5
    { OP_EQUALS, NULL, 6 },
// 6    '<?xml' S version = ^ "1.0" ...
    { OP_ATTRVAL, NULL, 32, 0},
// 7    '<?xml' S version '=' value ^ 
    { OP_TOKEN, NULL, 8, XML_PCDATA, -1 },
// 8    ^ are we done ?
    { OP_CHARWS, L"?", 28, 9 },    // must be '?' or whitespace.
// 9    ^ S? [encoding|standalone] '?>'
    { OP_OWS, NULL, 10 },
// 10
    { OP_CHAR, L"?", 28, 33 },    // may have '?' after skipping whitespace.
// 11    ^ [encoding|standalone] '?>'
    { OP_NAME, NULL, 12, },
// 12
    { OP_STRCMP, L"standalone", 23, 13, XML_STANDALONE },
// 13
    { OP_STRCMP, L"encoding", 14, XML_E_UNEXPECTED_ATTRIBUTE, XML_ENCODING },
// 14
    { OP_EQUALS, NULL, 15 },
// 15   
    { OP_ATTRVAL, NULL, 16, 0 },
// 16
    { OP_ENCODING, NULL, 17, 0, -1 },
// 17
    { OP_TOKEN, NULL, 18, XML_PCDATA, -1 },

// 18    ^ are we done ?
    { OP_CHARWS, L"?", 28, 19 },    // must be '?' or whitespace.
// 19    ^ S? standalone '?>'
    { OP_OWS, NULL, 20 },
// 20
    { OP_CHAR, L"?", 28, 34 },    // may have '?' after skipping whitespace.
// 21    ^ standalone '?>'
    { OP_NAME, NULL, 22, },
// 22 
    { OP_STRCMP, L"standalone", 23, XML_E_UNEXPECTED_ATTRIBUTE, XML_STANDALONE },
// 23
    { OP_EQUALS, NULL, 24 },
// 24
    { OP_ATTRVAL, NULL, 25, 0 },
// 25   
    { OP_STRCMP, L"yes", 31, 30, -1  },

// 26    <?xml ....... ^ '?>'   -- now expecting just the closing '?>' chars
    { OP_OWS, NULL, 27 },
// 27    
    { OP_CHAR, L"?", 28, XML_E_XMLDECLSYNTAX, 0 },
// 28   
    { OP_CHAR, L">", 29, XML_E_XMLDECLSYNTAX, 0 },
// 29    done !!
    { OP_POP,  NULL, 0, XMLStream::XML_ENDXMLDECL },

//----------------------- check standalone values  "yes" or "no"
// 30
    { OP_STRCMP, L"no", 31, XML_E_INVALID_STANDALONE, -1  },
// 31
    { OP_TOKEN, NULL, 26, XML_PCDATA, -1 },
    
//----------------------- check version = "1.0"
// 32
    { OP_STRCMP, L"1.0", 7, XML_E_INVALID_VERSION, -1 },
// 33 
    { OP_SNCHAR, NULL, 11, XML_E_XMLDECLSYNTAX },   
// 34 
    { OP_SNCHAR, NULL, 21, XML_E_XMLDECLSYNTAX },  
};

#define DELAYMARK(hr) (hr == S_OK || (hr >= XML_E_TOKEN_ERROR && hr < XML_E_LASTERROR))

#define XML_E_FOUNDPEREF 0x8000e5ff

//==============================================================================
static const WCHAR* g_pstrCDATA = L"CDATA";

XMLStream::XMLStream(XMLParser * pXMLParser)
:   _pStack(1), _pStreams(1)
{   
    // precondition: 'func' is never NULL
    _fnState = &XMLStream::init;
    _pInput = NULL;
    _pchBuffer = NULL;
    _fDTD = false;
    _fInternalSubset = false;
    _cStreamDepth = 0;
    _pXMLParser = pXMLParser;

    _init();
    SetFlags(0);
}

HRESULT 
XMLStream::init()
{
    HRESULT hr = S_OK;

    if (_pInput == NULL) 
    {
        // haven't called put_stream yet.
        return XML_E_ENDOFINPUT;
    }

    _init();

    if (_fDTD)
    {
        _fnState = &XMLStream::parseDTDContent;
    }
    else
    {
        _fnState =  &XMLStream::parseContent;
    }
    checkhr2(push(&XMLStream::firstAdvance,0));

    return hr;
}

void
XMLStream::_init()
{
    _fEOF = false;
    _fEOPE = false;
    _chLookahead = 0;
    _nToken = XML_PENDING;
    _chTerminator = 0;
    _lLengthDelta = 0;
    _lNslen = _lNssep = 0;
    _sSubState = 0;
    _lMarkDelta = 0;
    _nAttrType = XMLTYPE_CDATA;
    _fUsingBuffer = false;
    _lBufLen = 0;
    delete[] _pchBuffer;
    _pchBuffer = NULL;
    _lBufSize = 0;
    _fDelayMark = false;
    _fFoundWhitespace = false;
    _fFoundNonWhitespace = false;
    _fFoundPEREf = false;
    _fWasUsingBuffer = false;
    _chNextLookahead = 0;
    _lParseStringLevel = 0;
    _cConditionalSection = 0;
    _cIgnoreSectLevel = 0;
    _fWasDTD = false;
    _fParsingAttDef = false;
    _fFoundFirstElement = false;
    _fReturnAttributeValue = true;
    _fHandlePE = true;

    _pTable = NULL;
    _lEOFError = 0;
}

XMLStream::~XMLStream()
{
    delete _pInput;
    delete[] _pchBuffer;

    InputInfo* pi = _pStreams.peek();
    while (pi != NULL)
    {
        // Previous stream is finished also, so
        // pop it and continue on.
        delete pi->_pInput;
        pi = _pStreams.pop();
    }
}

HRESULT  
XMLStream::AppendData( 
    /* [in] */ const BYTE  *buffer,
    /* [in] */ long  length,
    /* [in] */ BOOL  last)
{
    if (_pInput == NULL)
    {
        _pInput = new_ne BufferedStream(this);
        if (_pInput == NULL)
            return E_OUTOFMEMORY;
        init();
    }

    HRESULT hr = _pInput->AppendData(buffer, length, last);

    return hr;
}

HRESULT  
XMLStream::Reset( void)
{
    init();
    delete _pInput;
    _pInput = NULL;
    return S_OK;
}

HRESULT  
XMLStream::PushStream( 
        /* [unique][in] */ EncodingStream  *p,
        /* [in] */ bool fExternalPE)
{
    HRESULT hr;
    if (_pStreams.used() == 0 && _pInput == NULL)
        init();

    _cStreamDepth++;

    _fEOPE = false;

    if (_fDelayMark && _pInput != NULL)
    {
        mark(_lMarkDelta);
        _lMarkDelta = 0;
        _fDelayMark = false;
    }

    // Save current input stream.
    if (_pInput != NULL)
    {
        InputInfo* pi = _pStreams.push();
        if (pi == NULL)
            return E_OUTOFMEMORY;
 
        pi->_pInput = _pInput;
        pi->_chLookahead = _chLookahead;
        pi->_fPE = true; // assume this is a parameter entity.
        pi->_fExternalPE = fExternalPE;
        pi->_fInternalSubset = _fInternalSubset;
        if (&XMLStream::skipWhiteSpace == _fnState  && _pStack.used() > 0)
        {
            StateInfo* pSI = _pStack.peek();
            pi->_fnState = pSI->_fnState;
        }
        else
        {
            pi->_fnState = _fnState;
        }

        if (fExternalPE)
        {
            // now we are not in the internal subset any more because we're
            // processing an external external Parameter Entity.
            _fInternalSubset = false; 
        }
        // and prepend pe text with space as per xml spec.
        _chLookahead = L' ';
        _chNextLookahead = _chLookahead;
        _pInput = NULL;
    }

    _pInput = new_ne BufferedStream(this);
    if (_pInput == NULL)
        return E_OUTOFMEMORY;

    if (p != NULL)
    {
        _pInput->Load(p);
    }
    if (_chLookahead == L' ')
        _pInput->setWhiteSpace(); // _pInput didn't see this space char.
    return S_OK;
}

HRESULT
XMLStream::InsertData(
    /* [in] */ const WCHAR *buffer,
    /* [in] */ long length,
    /* [in] */ bool pedata)
{
    HRESULT hr = S_OK;
    checkhr2(PushStream(NULL, false));
    InputInfo* pi = _pStreams.peek();
    pi->_fPE = pedata; // record whether this is a parameter entity or not !

    checkhr2(_pInput->AppendData((const BYTE*)s_ByteOrderMark, sizeof(s_ByteOrderMark), FALSE));
    checkhr2(_pInput->AppendData((const BYTE*)buffer, length*sizeof(WCHAR), TRUE));

    if (! pedata && _fDTD)
    {
        // soak up initial whitespace that PushStream put there - 
        // since this is not a parameter entity.
        hr = DTDAdvance();
    }
    return hr;
}

HRESULT 
XMLStream::PopStream()
{
    // This method has to pop all streams until it finds a stream that
    // can deliver the next _chLookahead character.

    HRESULT hr = S_OK;

    InputInfo* pi = NULL;

    pi = _pStreams.peek();
    if (pi == NULL) return S_FALSE;

    if (pi->_fPE)
    {
        // Check the proper Declaration/PE Nesting constraint
        // See XML spec section 2.8, [29]
        StateInfo* pSI = _pStack.peek();
        if ((pi->_fnState == &XMLStream::parseDTDContent || 
             pSI->_fnState == &XMLStream::parseDTDContent)
            && (pSI->_fnState != pi->_fnState))
        {
            hr = XML_E_PE_NESTING;
        }

        // get next char from previous stream.
        _chNextLookahead = pi->_chLookahead;
    
        _chLookahead = L' '; // append parameter entity text with space as per XML spec.
    }
    else
    {
        _chLookahead = pi->_chLookahead;
    }

    // Found previous stream, so we can continue.
    _fEOF = false;

    // Ok, so we actually got the next character, so
    // we can now safely throw away the previous 
    // lookahead character and return the next
    // non-whitespace character from the previous stream.
    delete _pInput;

    _pInput = pi->_pInput;
    if (_chLookahead == L' ')
        _pInput->setWhiteSpace();

    // BUGBUG: we need to clear this so that the parser does not
    // try and pop a download in the internalPE case (when handling XML_E_ENDOFINPUT in run())
    // but this means that internal PEs never get XMLNF_ENDENTITY notifications generated.
    // The DTDNodeFactory requires this behaviour currently (incorrectly)
    _fEOPE = pi->_fExternalPE;
    if (pi->_fExternalPE)
    {
        // Restore _fInternalSubset to the saved value since we've finished
        // with the external parameter entity now.
        _fInternalSubset = pi->_fInternalSubset;
    }

    _pStreams.pop();

    _cStreamDepth--;

    return hr;
}

HRESULT
XMLStream::ContinueDTDAdvance()
{
    HRESULT hr = S_OK;
    checkhr2(pop());
    return DTDAdvance();
}

HRESULT 
XMLStream::DTDAdvance()
{
    // The DTD Advance is a little tricker because we have to take
    // parameter entities into account.
    HRESULT hr = S_OK;

    if (_fEOPE)
    {
        hr = PopStream();
        if (FAILED(hr))
            return hr;

        if (_fEOPE)
        {
            _fEOPE = false;
            // Must return end of input to tell the XMLParser to pop the
            // download, and then pick up in the current state and continue on.
            checkhr2(push(&XMLStream::ContinueDTDAdvance, _sSubState));
            return XML_E_ENDOFINPUT;
        }
    }

    if (_chNextLookahead != 0)
    {
        _chLookahead = _chNextLookahead;
        _pInput->setWhiteSpace(::isWhiteSpace(_chLookahead) != 0); // tell it we are on a whitespace.
        _chNextLookahead = 0;
    }
    else
    {
        hr = _pInput->nextChar(&_chLookahead, &_fEOF);
        if (_fEOF && _pStreams.used() > 0) 
        {
            InputInfo* pi = _pStreams.peek();
            if (pi->_fPE)
            {
                // So we reached the end of a parameter entity, but instead of popping
                // the stream immediately, we return the trailing space that we're 
                // required to return, and then the next advance will pop the stream.
                _chLookahead = L' ';
                _pInput->setWhiteSpace(); // tell it we are on a whitespace.                _fEOPE = true;
                _fEOPE = true;
            }
            else
            {
                // Pop it immediately then, since this is not a parameter entity,
                // and PopStream will set the _chLookahead character accordingly.
                PopStream();
            }
            _fEOF = false;
        }
    }

    if (_chLookahead == L'%' && _fHandlePE && _fnState != &XMLStream::parsePERef)
    {
        if (_fInternalSubset)
        {
            // Parameter entities are not allowed INSIDE a 
            // declaration when we are parsing the internal subset.
            if (_fnState != &XMLStream::parseDTDContent)
            {
                if (_fnState == &XMLStream::skipWhiteSpace && _pStack.used() > 0)
                {
                    StateInfo* pSI = _pStack.peek();
                    if (pSI->_fnState != &XMLStream::parseDTDContent)
                    {
                        hr = XML_E_BADPEREFINSUBSET;
                    }
                }
                else
                {
                    hr = XML_E_BADPEREFINSUBSET;
                }
            }
        }
        if (S_OK == hr)
        {
            _fFoundPEREf = true;
            _chLookahead = L' '; // return space to end previous token.
            _pInput->setWhiteSpace(); // tell it we are on a whitespace.                
            if (_fUsingBuffer)
            {    
                return XML_E_FOUNDPEREF; // must break out of while loops !!
            }
        }
    }

    return hr;
}


HRESULT  
XMLStream::GetNextToken( 
        /* [out] */ DWORD  *t,
        /* [out] */ const WCHAR  **text,
        /* [out] */ long  *length,
        /* [out] */ long  *nslen)
{
    HRESULT hr;

    if (_fDTD)
        return GetNextTokenInDTD(t,text,length,nslen);

    if (_fDelayMark)
    {
        mark(_lMarkDelta);
        _lMarkDelta = 0;
        _fDelayMark = false;
    }

    hr = (this->*_fnState)();
    while (hr == S_OK && _nToken == XML_PENDING)
    {
        hr = (this->*_fnState)();
    }

    if (hr == S_OK)
    {
        *t = _nToken;
    }
    else if (hr == E_PENDING)
    {
        *t = XML_PENDING;
        *length = *nslen = 0;
        *text = NULL;
        goto CleanUp;
    }
    else
    {
        *t = XML_PENDING;
    }

    // At this point hr == S_OK or it is some error.  So we
    // want to return the text of the current token, since this
    // is useful in both cases.

    if (! _fUsingBuffer)
    {
        getToken(text,length);
        if (_lLengthDelta != 0)
        {
            *length += _lLengthDelta;
            _lLengthDelta = 0;
        }
// This can only happen in the context of a DTD.
//        if (_fWasUsingBuffer)
//        {
//            _fUsingBuffer = _fWasUsingBuffer;
//            _fWasUsingBuffer = false;
//        }
    }
    else
    {
        *text = _pchBuffer;
        *length = _lBufLen;
        _fUsingBuffer = false;
        _fFoundWhitespace = false;
        _lBufLen = 0;
        _lLengthDelta = 0;
    }
    
    if (DELAYMARK(hr))
    {
        // Mark next time around so that error information points to the
        // beginning of this token.
        _fDelayMark = true;
    }
    else 
    {
        // otherwise mark this spot right away so we point to the exact
        // source of the error.
        mark(_lMarkDelta);
        _lMarkDelta = 0;
    }

    _nToken = XML_PENDING;
    *nslen = _lNslen;
    _lNslen = _lNssep = 0;

CleanUp:
    return hr;
}

HRESULT  
XMLStream::GetNextTokenInDTD( 
        /* [out] */ DWORD  *t,
        /* [out] */ const WCHAR  **text,
        /* [out] */ long  *length,
        /* [out] */ long  *nslen)
{
    HRESULT hr;

prepare:

    if (_fDelayMark)
    {
        mark(_lMarkDelta);
        _lMarkDelta = 0;
        _fDelayMark = false;
    }
    if (_fFoundPEREf)
    {
        goto peref;
    }

start:

    hr = (this->*_fnState)();
    while (hr == S_OK && _nToken == XML_PENDING && ! _fFoundPEREf)
    {
        hr = (this->*_fnState)();
    }

    if (hr == S_OK)
    {
        if (_nToken == XML_PENDING)
        {
            if (_fFoundPEREf)
            {
peref:
                // This was just a trick to pop us back up to this level and
                // break out of parseString or parseComment loops.  Now
                // we can continue on and parse the peref.
                ADVANCE; // past the '%' character.
                checkhr2(push(&XMLStream::parsePERef,_sSubState));
                // being careful not to mess up the current buffer if one
                // is being used.
                _fWasUsingBuffer = _fUsingBuffer;
                _fUsingBuffer = false;
                _fFoundPEREf = false;
                goto start;
            }
        }
        *t = _nToken;
    }
    else if (hr == E_PENDING || 
            hr == XML_E_ENDOFINPUT)  // for parameter entities, etc.
    {
        *t = XML_PENDING;
        *length = *nslen = 0;
        *text = NULL;
        goto CleanUp;
    }
    else if (hr == XML_E_FOUNDPEREF)
    {
        goto peref;
    }
    else
    {
        *t = XML_PENDING;
    }

    if (! _fUsingBuffer)
    {
        getToken(text,length);
        *length += _lLengthDelta;
        if (_fWasUsingBuffer)
        {
            _fUsingBuffer = _fWasUsingBuffer;
            _fWasUsingBuffer = false;
        }
        _lLengthDelta = 0;
    }
    else
    {
        *text = _pchBuffer;
        *length = _lBufLen;
        _fUsingBuffer = false;
        _fFoundWhitespace = false;
        _lBufLen = 0;
        _lLengthDelta = 0;
    }

    if (hr != 0 && hr != E_PENDING && _fInternalSubset && _chLookahead == L'%')
    {
        hr = XML_E_BADPEREFINSUBSET;
    }

    if (DELAYMARK(hr))
    {
        // Mark next time around so that error information points to the
        // beginning of this token.
        _fDelayMark = true;
    }
    else 
    {
        // otherwise mark this spot right away so we point to the exact
        // source of the error.
        mark(_lMarkDelta);
        _lMarkDelta = 0;
    }
    _nToken = XML_PENDING;
    *nslen = _lNslen;
    _lNslen = _lNssep = 0;

    // In IE4 mode we do not return DTD tokens at all except for the
    // one big XML_DTDSUBSET token for the entire subset.
    if (_fNoDTDNodes && _fDTD && hr == S_OK)
    {
        goto prepare;
    }

CleanUp:
    return hr;
}

ULONG  
XMLStream::GetLine()    
{
    BufferedStream* input = getCurrentStream();
    if (input != NULL)
        return input->getLine();
    return 0;
}

ULONG  
XMLStream::GetLinePosition( )
{
    BufferedStream* input = getCurrentStream();
    if (input != NULL)
        return input->getLinePos();
    return 0;
}

ULONG  
XMLStream::GetInputPosition( )
{
    BufferedStream* input = getCurrentStream();
    if (input != NULL)
        return input->getInputPos();
    return 0;
}

HRESULT  
XMLStream::GetLineBuffer( 
    /* [out] */ const WCHAR  * *buf, ULONG* len, ULONG* startpos)
{
    if (buf == NULL || len == NULL)
        return E_INVALIDARG;

    *buf = NULL;
    BufferedStream* input = getCurrentStream();
    if (input)
        *buf = input->getLineBuf(len, startpos);
    return S_OK;
}

BufferedStream* 
XMLStream::getCurrentStream()
{
    // Return the most recent stream that
    // actually has somthing to return.
    BufferedStream* input = _pInput;
    if (!_pInput)
    {
        return NULL;
    }
    int i = _pStreams.used()-1;    
    do 
    {
        ULONG len = 0, pos = 0;
        const WCHAR* buf = input->getLineBuf(&len, &pos);
        if (len > 0)
            return input;

        if (i >= 0)
            input = _pStreams[i--]->_pInput;
        else
            break;
    }
    while (input != NULL);
    return NULL;
}

void 
XMLStream::SetFlags( unsigned short usFlags)
{
    _usFlags = usFlags;
    // And break out the flags for performance reasons.
    _fFloatingAmp = (usFlags & XMLFLAG_FLOATINGAMP) != 0;
    _fShortEndTags = (usFlags & XMLFLAG_SHORTENDTAGS) != 0;
    _fCaseInsensitive = (usFlags & XMLFLAG_CASEINSENSITIVE) != 0;
    _fNoNamespaces = (usFlags & XMLFLAG_NONAMESPACES) != 0;
    _fNoWhitespaceNodes = false; // this is now bogus.  (usFlags & XMLFLAG_NOWHITESPACE) != 0;
    _fIE4Quirks = (_usFlags & XMLFLAG_IE4QUIRKS) != 0;
    _fNoDTDNodes = (_usFlags & XMLFLAG_NODTDNODES) != 0;
}

unsigned short 
XMLStream::GetFlags()
{
    return _usFlags;
}

void XMLStream::SetType(DWORD type)
{
    static short s_TypeMap[11] = { 
            XMLTYPE_CDATA,      // XML_AT_CDATA
            XMLTYPE_NAME,       // XML_AT_ID
            XMLTYPE_NAME,       // XML_AT_IDREF
            XMLTYPE_NAMES,      // XML_AT_IDREFS
            XMLTYPE_NAME,       // XML_AT_ENTITY
            XMLTYPE_NAMES,      // XML_AT_ENTITIES
            XMLTYPE_NMTOKEN,    // XML_AT_NMTOKEN
            XMLTYPE_NMTOKENS,   // XML_AT_NMTOKENS
            XMLTYPE_NMTOKEN,    // XML_AT_NOTATION
    };
    _nAttrType = s_TypeMap[type - XML_AT_CDATA];
}

WCHAR*  
XMLStream::GetEncoding()
{
    if (_pInput == NULL)
        return NULL;
    else
        return _pInput->getEncoding();
}


//======================================================================
// Real Implementation
HRESULT 
XMLStream::firstAdvance()
{
    HRESULT hr;
    ADVANCE;
    checkhr2(pop(false));
    return S_OK;
}


HRESULT 
XMLStream::parseContent()
{
    HRESULT hr = S_OK;

    if (_fEOF)
        return XML_E_ENDOFINPUT;

    switch (_chLookahead)
    {
    case L'<':
        ADVANCE;
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        switch (_chLookahead)
        {
        case L'!':
            checkhr2(_pInput->Freeze()); // stop shifting data until '>'
            return pushTable( 0, g_DeclarationTable, XML_E_UNCLOSEDDECL);
        case L'?':
            checkhr2(push( &XMLStream::parsePI ));
            return parsePI();
        case L'/':
            checkhr2(push(&XMLStream::parseEndTag));
            return parseEndTag();
        default:
            checkhr2(push( &XMLStream::parseElement ));
            if (_fFoundFirstElement)
            {
                return parseElement();
            }
            else
            {
                // Return special end prolog token and then continue with 
                // with parseElement.
                _fFoundFirstElement = true;
                _nToken = XML_ENDPROLOG;
            }
        }
        break;

    default:
        checkhr2(push(&XMLStream::parsePCData));
        return parsePCData();
        break;
    }
    return S_OK;
}

HRESULT 
XMLStream::skipWhiteSpace()
{
    HRESULT hr = S_OK;

    while (ISWHITESPACE(_chLookahead) && ! _fEOF)
    {
        ADVANCE;
        if (_fFoundPEREf) return S_OK;
    }
    checkhr2(pop(false));
    return hr;
}

HRESULT 
XMLStream::parseElement()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        checkhr2(_pInput->Freeze()); // stop shifting data until '>'
        checkhr2(push( &XMLStream::parseName, 1));
        checkhr2(parseName());
        _sSubState = 1;
        // fall through
    case 1:
        checkeof(_chLookahead, XML_E_UNCLOSEDSTARTTAG);
        _nToken = XML_ELEMENT;
        // and then try and parse the attributes, and return
        // to state 2 to finish up.  With an optimization
        // for the case where there are no attributes.
        if (_chLookahead == L'/' || _chLookahead == L'>')
        {
            _sSubState = 2;
        }
        else if (_fIE4Quirks && _chLookahead == L'=')
        {
            _sSubState = 4; // weird compatibility case.
        }
        else 
        {
            if (!ISWHITESPACE(_chLookahead))
            {
                return XML_E_BADNAMECHAR;
            }
            _chEndChar = L'/'; // for empty tags.
            checkhr2(push(&XMLStream::parseAttributes,2));
        }
        return S_OK;
        break;

    case 2: // finish up with start tag.
        mark(); // only return '>' or '/>' in _nToken text
        if (_chLookahead == L'/')
        {
            // must be empty tag sequence '/>'.
            ADVANCE;
            _nToken = XML_EMPTYTAGEND;
        } 
        else if (_chLookahead == L'>')
        {
            _nToken = XML_TAGEND;
        }
        else if (ISWHITESPACE(_chLookahead))
        {
            return XML_E_UNEXPECTED_WHITESPACE;
        }
        else
            return XML_E_EXPECTINGTAGEND;

        _sSubState = 3;
        // fall through
    case 3:
        checkeof(_chLookahead, XML_E_UNCLOSEDSTARTTAG);
        if (_chLookahead != L'>')
        {
            if (ISWHITESPACE(_chLookahead))
                return XML_E_UNEXPECTED_WHITESPACE;
            else 
                return XML_E_EXPECTINGTAGEND;
        }
        ADVANCE; 
        mark();
        checkhr2(pop());// return to parseContent.
        return _pInput->UnFreeze(); 
        break;

    case 4: // swollow up bad tag
        // Allow the weird CDF madness <PRECACHE="YES"/>
        // For total compatibility we fake out the parser by returning
        // XML_EMPTYTAGEND, this way the rest of the tag becomes PCDATA.
        // YUK -- but it works.
        _nToken = XML_EMPTYTAGEND;
        mark();
        checkhr2(pop());// return to parseContent.
        return _pInput->UnFreeze(); 
        break;

    default:
        INTERNALERROR;
    }
    return S_OK;
}

HRESULT 
XMLStream::parseEndTag()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        ADVANCE; // soak up the '/'
        mark(); 
        // SHORT END TAG SUPPORT, IE4 Compatibility Mode only.
        if (! _fShortEndTags || _chLookahead != L'>') 
        {
            checkhr2(push( &XMLStream::parseName, 1));
            checkhr2(parseName());
        }
        _sSubState = 1;
        // fall through
        
    case 1: // finish parsing end tag
        checkeof(_chLookahead, XML_E_UNCLOSEDENDTAG);
        _nToken = XML_ENDTAG;
        checkhr2(push(&XMLStream::skipWhiteSpace, 2));
        return S_OK;

    case 2:
        checkeof(_chLookahead, XML_E_UNCLOSEDENDTAG);
        if (_chLookahead != L'>')
        {
            return XML_E_BADNAMECHAR;
        }
        ADVANCE;
        mark();
        checkhr2(pop());// return to parseContent.
        break;

    default:
        INTERNALERROR;
    }
    return S_OK;
}

HRESULT 
XMLStream::parsePI()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        _fWasDTD = _fDTD; // as far as Advance is concerned, the contents
        _fHandlePE = false;    // of a PI are not special.
        ADVANCE;
        checkhr2(_pInput->Freeze()); // stop shifting data until '?>'
        mark(); // don't include '?' in tag name.
        if (_chLookahead == L'x' || _chLookahead == L'X')
        {
            // perhaps this is the magic <?xml version="1.0"?> declaration.
            STATE(7);  // jump to state 7.
        }
        // fall through
        _sSubState = 1;
    case 1:
        checkhr2(push( &XMLStream::parseName, 2));
        checkhr2(parseName()); 
        _sSubState = 2;
        // fall through
    case 2:
        checkeof(_chLookahead, XML_E_UNCLOSEDPI);
        if (_chLookahead != L'?' && ! ISWHITESPACE(_chLookahead))
        { 
            return XML_E_BADNAMECHAR;
        }
        _nToken = XML_PI;
        STATE(3);   // found startpi _nToken and return to _sSubState 3
        break;

    case 3: // finish with rest of PI
        if (_chLookahead == L'?')
        {
            ADVANCE;
            if (_chLookahead == L'>')
            {
                STATE(6);
            }
            else
            {
                return XML_E_EXPECTINGTAGEND;
            }
        }

        checkhr2(push(&XMLStream::skipWhiteSpace, 4));
        checkhr2( skipWhiteSpace() );
        _sSubState = 4;
        // fall through

    case 4: // support for normalized whitespace
        mark(); // strip whitespace from beginning of PI data, since this is
                // just the separator between the PI target name and the PI data.
        _sSubState = 5;
        // fallthrough

    case 5:
        while (! _fEOF )
        {
            if (_chLookahead == L'?')
            {
                ADVANCE;
                break;
            }
            if (! isCharData(_chLookahead))
                return XML_E_PIDECLSYNTAX;
            ADVANCE;
        }
        _sSubState = 6; // go to next state
        // fall through.
    case 6:
        checkeof(_chLookahead, XML_E_UNCLOSEDPI);
        if (_chLookahead == L'>')
        {
            ADVANCE;
            _lLengthDelta = -2; // don't include '?>' in PI CDATA.
        }
        else
        {
            // Hmmm.  Must be  a lone '?' so go back to state 5.
            STATE(5);
        }
        _nToken = XML_ENDPI;
        _fHandlePE = true;
        checkhr2(pop());
        return _pInput->UnFreeze();
        break;      

    case 7: // recognize 'm' in '<?xml' declaration
        ADVANCE;
        if (_chLookahead != L'm' && _chLookahead != L'M')
        {
            STATE(11); // not 'xml' so jump to state 11 to parse name
        }
        _sSubState = 8;
        // fall through                

    case 8: // recognize L'l' in '<?xml' declaration
        ADVANCE;
        if (_chLookahead != L'l' && _chLookahead != L'L')
        {
            STATE(11); // not 'xml' so jump to state 11 to parse name
        }
        _sSubState = 9;
        // fall through                

    case 9: // now need whitespace or ':' or '?' to terminate name.
        ADVANCE;
        if (ISWHITESPACE(_chLookahead))
        {
            if (! _fCaseInsensitive)
            {
                const WCHAR* t;
                long len;
                getToken(&t,&len);
                if (! StringEquals(L"xml",t,3,false)) // case sensitive
                    return XML_E_BADXMLCASE;
            }
            return pushTable(10, g_XMLDeclarationTable, XML_E_UNCLOSEDPI);
        }
        if (isNameChar(_chLookahead) || _chLookahead == ':')  
        {
            STATE(11); // Hmmm.  Must be something else then so continue parsing name
        }
        else
        {
            return XML_E_XMLDECLSYNTAX;
        }
        break;

    case 10:
        _fHandlePE = true;
        checkhr2(pop());
        return _pInput->UnFreeze();
        break;

    case 11:
        if (_chLookahead == ':')
            ADVANCE;
        _sSubState = 12;
        // fall through
    case 12:
        if (isNameChar(_chLookahead))
        {
            checkhr2(push( &XMLStream::parseName, 2));
            _sSubState = 1; // but skip IsStartNameChar test
            checkhr2(parseName());
            return S_OK;
        } 
        else
        {
            STATE(2);
        }
        break;

    default:
        INTERNALERROR;
    }

    return S_OK;
}

HRESULT 
XMLStream::parseComment()
{
    // ok, so '<!-' has been parsed so far
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        _fWasDTD = _fDTD; // as far as the DTD is concerned, the contents
        _fHandlePE = false;    // of a COMMENT are not special.
        ADVANCE; // soak up first '-'
        checkeof(_chLookahead, XML_E_UNCLOSEDCOMMENT);
        if (_chLookahead != L'-')
        {
            return XML_E_COMMENTSYNTAX;
        }
        _sSubState = 1;
        // fall through
    case 1:
        ADVANCE; // soak up second '-'
        mark(); // don't include '<!--' in comment text
        _sSubState = 2;
        // fall through;
    case 2:
        while (! _fEOF)
        {
            if (_chLookahead == L'-')
            {
                ADVANCE; // soak up first closing L'-'                
                break;
            }
            if (! isCharData(_chLookahead))
                return XML_E_BADCHARDATA;
            ADVANCE;
        }
        checkeof(_chLookahead, XML_E_UNCLOSEDCOMMENT);
        _sSubState = 3; // advance to next state        
        // fall through.
    case 3:
        if (_chLookahead != L'-')
        {
            // Hmmm, must have been a floating L'-' so go back to state 2
            STATE(2);
        }
        ADVANCE; // soak up second closing L'-'
        _sSubState = 4; 
        // fall through
    case 4:
        checkeof(_chLookahead, XML_E_UNCLOSEDCOMMENT);
        if (_chLookahead != L'>' && ! _fIE4Quirks)
        {
            // cannot have floating L'--' unless we are in compatibility mode.
            return XML_E_COMMENTSYNTAX;
        }
        ADVANCE; // soak up closing L'>'
        _lLengthDelta = -3; // don't include L'-->' in PI CDATA.
        _nToken = XML_COMMENT;
        checkhr2(pop());
        _fHandlePE = true;
        break;

    default:
        INTERNALERROR;
    }    
    return S_OK;
}


HRESULT 
XMLStream::parseName()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        if (! isStartNameChar(_chLookahead))
        {
            if (ISWHITESPACE(_chLookahead))
                hr = XML_E_UNEXPECTED_WHITESPACE;
            else
                hr = XML_E_BADSTARTNAMECHAR;
            goto CleanUp;
        }
        mark(); 
        _sSubState = 1;
        // fall through

    case 1:
        _lNslen = _lNssep = 0;
        if (_fNoNamespaces)
        {
            goto simple;            
        }
        // When handling namespaces, L':' is not allowed as a start name character
        if (_chLookahead == L':')
        {
            hr = XML_E_BADSTARTNAMECHAR;
            goto CleanUp;
        }
        ADVANCE;
        _sSubState = 2;
        // fall through

    case 2:
loop:
        while (_chLookahead != L':' && isNameChar(_chLookahead) && !_fEOF)
        {
            ADVANCE;
        }
        if (_chLookahead == L':')
        {
            if (_lNssep == 0)
            {
                _lNslen = _pInput->getTokenLength();
                _lNssep++;
            }
            else
            {
                hr = XML_E_MULTIPLE_COLONS;
                goto CleanUp;
            }
            // Must not re-enter this state since _lNssep has already been incremented.
            ADVANCETO(2);
            goto loop;
        }
        else
        {
            hr = pop(false);
            break;
        }

    case 3:  // this is the case when we are not supporting namespaces.
simple:
        while (isNameChar(_chLookahead) && !_fEOF)
        {
            ADVANCE;
        }
        hr = pop(false);
        break;

    default:
        INTERNALERROR;
    }

CleanUp:
    return hr;
}

HRESULT 
XMLStream::parseNmToken()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        if (! isNameChar(_chLookahead))
        {
            if (ISWHITESPACE(_chLookahead))
                return XML_E_UNEXPECTED_WHITESPACE;

            return XML_E_BADNAMECHAR;
        }
        _sSubState = 1;
        // fall through
    case 1:
        while (_chLookahead != L'>' && isNameChar(_chLookahead) && ! _fEOF)
        {
            ADVANCE;
        }
        checkhr2(pop(false));
        break;

    default:
        INTERNALERROR;
    }
    return hr;
}

HRESULT 
XMLStream::parseAttributes()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        _nAttrType = XMLTYPE_CDATA;
        _fCheckAttribute = false;
        checkhr2(push(&XMLStream::skipWhiteSpace, 1));
        checkhr2( skipWhiteSpace() );
        _sSubState = 1;
        // fall through
    case 1:
        if (_chLookahead == _chEndChar || _chLookahead == L'>' )
        {
            checkhr2(pop()); // no attributes.
            return S_OK;
        }
        if (_chLookahead == L'x')
        {
            _fCheckAttribute = true;
        }
        checkhr2( push( &XMLStream::parseName, 2 ) );
        checkhr2( parseName() );

        if (!ISWHITESPACE(_chLookahead) && _chLookahead != L'=')
        {
            return XML_E_BADNAMECHAR;
        }
        _sSubState = 2;
        // fall through
    case 2:
        if (ISWHITESPACE(_chLookahead))
        {
            // Eq ::= S? '=' S?
            STATE(7);
        }

        checkeof(_chLookahead, XML_E_UNCLOSEDSTARTTAG);
        if (_fCheckAttribute)
        {
            const WCHAR* t; long len;
            getToken(&t, &len);
            if (StringEquals(L"xml:space",t,len+_lLengthDelta,_fCaseInsensitive))
            {
                _nToken = XML_XMLSPACE;
            }
            else if (StringEquals(L"xml:lang",t,len+_lLengthDelta,_fCaseInsensitive))
            {
                _nToken = XML_XMLLANG;
            }
            else if ((_lNslen == 5 && 
                StringEquals(L"xmlns",t,_lNslen,_fCaseInsensitive)) ||
                StringEquals(L"xmlns",t,len+_lLengthDelta,_fCaseInsensitive))
            {
                if (len+_lLengthDelta >= 9 &&
                    StringEquals(L"xml", &t[6], 3, TRUE))
                {
                    return XML_E_RESERVEDNAMESPACE;
                }
                _nToken = XML_NS;
            }
            else if (_fIE4Quirks && 
                StringEquals(L"xml-space",t,len+_lLengthDelta,_fCaseInsensitive))
            {
                _nToken = XML_XMLSPACE;
            }
            else
            {
                _nToken = XML_ATTRIBUTE;
            }
        }
        else
        {
            _nToken = XML_ATTRIBUTE;
        }
        _sSubState = 3;
        return S_OK;
        break;

    case 3:
        if (ISWHITESPACE(_chLookahead))
            return XML_E_UNEXPECTED_WHITESPACE;
        _fWhitespace = false;
        _sSubState = 4;
        // fall through

    case 4:
        if (_chLookahead != L'=')
        {
            return XML_E_MISSINGEQUALS;
        }
        ADVANCE;
        if (ISWHITESPACE(_chLookahead))
        {
            // allow whitespace between '=' and attribute value.
            checkhr2(push(&XMLStream::skipWhiteSpace, 5));
            checkhr2( skipWhiteSpace() );            
        }
        _sSubState = 5;
        // fall through

    case 5:
        if (ISWHITESPACE(_chLookahead))
            return XML_E_UNEXPECTED_WHITESPACE;
        if (_chLookahead != L'"' && _chLookahead != L'\'')
        {
            return XML_E_MISSINGQUOTE;
        }
        _chTerminator = _chLookahead;
        ADVANCE;
        mark(); 
        return push(&XMLStream::parseAttrValue, 6);
        _sSubState = 6;
    // fall through;

    case 6:
        checkeof(_chLookahead, XML_E_UNCLOSEDSTARTTAG);
        if (_chLookahead == _chEndChar || _chLookahead == L'>')
        {
            checkhr2(pop());
            return S_OK;
        }
        if (! ISWHITESPACE(_chLookahead) && !_fIE4Quirks)
        {
            return XML_E_MISSINGWHITESPACE;
        }
        STATE(0); // go back to state 0
        break;

    case 7:
        // allow whitespace between attribute and '='
        _lLengthDelta = _pInput->getTokenLength();
        checkhr2(push(&XMLStream::skipWhiteSpace, 8));
        checkhr2( skipWhiteSpace() );       
        _sSubState = 8;
        // fall through

    case 8:
        checkeof(_chLookahead, XML_E_UNCLOSEDSTARTTAG);
        _lLengthDelta -= _pInput->getTokenLength();
        STATE(2);
        break;

    default:
        INTERNALERROR;
    }
    return hr;
}

HRESULT XMLStream::parseAttrValue()
{
    HRESULT hr = S_OK;

    switch (_sSubState)
    {
    case 0: 
        _fParsingAttDef = true;        
        // mark beginning of attribute data           
        _sSubState =  2;
        // fall through;

    case 2:
        while ( _chLookahead != _chTerminator && 
                _chLookahead != L'<' &&
                ! _fEOF  ) 
        {
            if (_chLookahead == L'&')
            {
                // then parse entity ref and then return
                // to state 2 to continue with PCDATA.
                return push(&XMLStream::parseEntityRef,2);
            }
            hr = _pInput->scanPCData(&_chLookahead, &_fWhitespace);
            if (FAILED(hr))
            {
                if (hr == E_PENDING)
                {
                    hr = S_OK;
                    ADVANCE;
                }
                return hr;
            }
        }
        _sSubState = 3;
        // fall through
    case 3:
        checkeof(_chLookahead, XML_E_UNCLOSEDSTRING);
        if (_chLookahead == _chTerminator)
        {
            ADVANCE;
            if (_fReturnAttributeValue)
            {
                // return what we have so far - if anything.
                if ((_fUsingBuffer && _lBufLen > 0) ||
                    _pInput->getTokenLength() > 1)
                {
                    _lLengthDelta = -1; // don't include string _chTerminator.
                    _nToken = XML_PCDATA;
                }
            }
            else
            {
                _fReturnAttributeValue = true; // reset to default value.
            }
            _fParsingAttDef = false;
            checkhr2(pop());
            return S_OK;
        } 
        else if (_chLookahead == L'<' && _fIE4Quirks)
        {
            // This was allowed in IE4
            ADVANCE;
            STATE(2);
        }
        else
        {
            return XML_E_BADCHARINSTRING;
        }        
        break;

    default:
        INTERNALERROR;
    }
    return hr;
}

// Also, in order to support validation of entity references in this situation, we also
// expand entities.  In order to do this we have to use the local buffer for accumulating
// the names in case they cross entity ref boundaries.  For example: value="A&b;C" will
// expand to a single name "ABC" if the entitiy is defined to be <!ENTITY b "B">.

HRESULT 
XMLStream::expandEntity()
{
    HRESULT hr = S_OK;

    switch (_sSubState)
    {
    case 0:
        // Switch out of DTD mode and buffer mode temporarily.
        _fDTD = _fWasDTD; // but first reset the _fDTD flag.
        _fWasUsingBuffer = _fUsingBuffer;
        _fUsingBuffer = false;
        _fResolved = false;
        _cStreams = _pStreams.used(); // save current # of streams.
        // (NodeFactory will call XMLParser::ExpandEntity).
        return push(&XMLStream::parseEntityRef,1);

    case 1:
        if (! _fResolved)
        {
            if (_cStreams != _pStreams.used()) // was pushstream called ?
            {
                // If so then suck up the space character that pushStream inserts
                // because in this case we don't want inserted whitespace.
                _fDTD = true;
                ADVANCE; 
                _fDTD = _fWasDTD;
            }
        }
        else
        {
            _fUsingBuffer = _fWasUsingBuffer;
            _fWasUsingBuffer = false;
        }
        pop();
        _fWasDTD = _fDTD;
        _fDTD = true;
        break;

    }
    return S_OK;
}

HRESULT 
XMLStream::parseNMString()
{
    HRESULT hr = S_OK;

    switch (_sSubState)
    {
    case 0:
        if (_chLookahead != L'"' && _chLookahead != L'\'')
        {
            hr = XML_E_MISSINGQUOTE;
            goto CleanUp;
        }
        _chTerminator = _chLookahead;
        _sSubState = 1;
        // fall through

    case 1:
        ADVANCE; // skip the string _chTerminator.
        if (! isStartNameChar(_chLookahead) || _chLookahead == L':')
        {
            goto Error;
        }
        mark(); 
        _sSubState = 2;

    case 2:
        while (_chLookahead != L':' && isNameChar(_chLookahead) && !_fEOF)
        {
            ADVANCE;
        }
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead != _chTerminator)
        {
            goto Error;
        }
        _sSubState = 3;
        // fall through

    case 3:
        ADVANCE;
        _lLengthDelta = -1; // don't include string _chTerminator.
        hr = pop();
        break;

    default:
        INTERNALERROR;
    }

CleanUp:
    return hr;

Error:
    if (ISWHITESPACE(_chLookahead))
        hr = XML_E_UNEXPECTED_WHITESPACE;
    else
        hr = XML_E_BADSTARTNAMECHAR;
    goto CleanUp;
}

HRESULT 
XMLStream::ScanHexDigits()
{
    HRESULT hr = S_OK;
    while (! _fEOF && _chLookahead != L';')
    {
        if (! isHexDigit(_chLookahead))
        {
            return ISWHITESPACE(_chLookahead) ? XML_E_UNEXPECTED_WHITESPACE : XML_E_BADCHARINENTREF;
        }
        ADVANCE;
    }
    checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
    return hr;
}

HRESULT 
XMLStream::ScanDecimalDigits()
{
    HRESULT hr = S_OK;
    while (! _fEOF && _chLookahead != L';')
    {
        if (! isDigit(_chLookahead))
        {
            return ISWHITESPACE(_chLookahead) ? XML_E_UNEXPECTED_WHITESPACE : XML_E_BADCHARINENTREF;
        }
        ADVANCE;
    }
    checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
    return hr;
}

HRESULT 
XMLStream::parseString()
{
    // This method is used to parse the entity declaration value where we resolve numeric entities
    // but NOT named entities.
    HRESULT hr = S_OK;

    switch (_sSubState)
    {
    case 0:
        if (_chLookahead != L'"' && _chLookahead != L'\'')
        {
            return XML_E_MISSINGQUOTE;
        }
        _lParseStringLevel = _pStreams.used();
        _fUsingBuffer = true;
        _chTerminator = _chLookahead;
        _sSubState = 1;
        // fall through
    case 1:
        ADVANCE;
        mark(); // don't include ' or " in string.
        _sSubState = 2; // and don't do this again.
        // fall through;
    case 2:
        while ( ((_chLookahead != _chTerminator && 
                _chLookahead != _chBreakChar) ||  _lParseStringLevel < _pStreams.used()) &&
                ! _fEOF ) 
        {
            if (_chLookahead == L'&')
            {
                // then parse entity ref resolving numeric entities and 
                // leaving named entities alone.
                ADVANCE; // soak up the ampersand.
                STATE(3);
            }
            PushChar(_chLookahead);
            // 6/24/98 - we must use ADVANCETO because PushChar has
            // already saved the _chLookahead character, and so we 
            // don't want to re-enter this state without the next char.
            ADVANCETO(2); 
        }
        checkeof(_chLookahead, XML_E_UNCLOSEDSTRING);
        if (_chLookahead == _chTerminator)
        {
            ADVANCE;
            _lLengthDelta = -1; // don't include string _chTerminator.
            checkhr2(pop()); 
        }
        else
        {
            return XML_E_BADCHARINSTRING;
        }
        break;

    case 3:
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead == L'#')
        {
            ADVANCE;
        }
        else if (! isStartNameChar(_chLookahead))
        {
            hr = XML_E_BADCHARINENTREF;
            break;
        }
        else
        {
            // we are parsing an entity so don't resolve named entities yet.
            PushChar(L'&');
            STATE(7);
        }
        _sSubState = 4;
        // fall through

        // ------------- Numeric entity references --------------------
    case 4:
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead == L'x')
        {
            // hex character reference.
            ADVANCE;
            mark();
            STATE(6); // go to state 6
        }
        _sSubState = 5;
        mark();
        // fall through

    case 5: // '&#' ^ [0-9]+ ';'
        checkhr2(ScanDecimalDigits());
        if (_chLookahead != L';')
        {
            return XML_E_MISSINGSEMICOLON;
        }
        else
        {
            // Just resolve the numeric reference into the buffer.
            const WCHAR* t; long len; WCHAR ch;
            getToken(&t, &len);
            checkhr2(DecimalToUnicode(t,len,ch));
            PushChar(ch);
        }
        // 6/24/98 - we must use ADVANCETO because PushChar has
        // already saved the _chLookahead character, and so we 
        // don't want to re-enter this state without the next char.
        ADVANCETO(2); // soak up the L';'
        STATE(2);
        break;

    case 6: // '&#X' ^ [0-9a-fA-F]+
        checkhr2(ScanHexDigits());
        if (_chLookahead != L';')
        {
            return XML_E_MISSINGSEMICOLON;
        }
        else
        {
            // Just resolve the numeric reference into the buffer.
            const WCHAR* t; long len; WCHAR ch;
            getToken(&t, &len);
            checkhr2(HexToUnicode(t,len,ch));
            PushChar(ch);
        }
        // 6/24/98 - we must use ADVANCETO because PushChar has
        // already saved the _chLookahead character, and so we 
        // don't want to re-enter this state without the next char.
        ADVANCETO(2); // soak up the L';'
        STATE(2);
        break;

    case 7:
        // ------------- Named Entity References --------------------
        // Just make sure it is valid - we don't actually return it.
        while (isNameChar(_chLookahead) && !_fEOF)
        {
            PushChar(_chLookahead);
            // 6/24/98 - we must use ADVANCETO because PushChar has
            // already saved the _chLookahead character, and so we 
            // don't want to re-enter this state without the next char.
            ADVANCETO(7); 
        }
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead != L';')
        {
            return XML_E_BADCHARINENTREF;
        }
        STATE(2);
        break;

    default:
        INTERNALERROR;
    }
    return hr;
}

HRESULT 
XMLStream::parsePCData()
{
    HRESULT hr = S_OK;

Start:
    switch (_sSubState)
    {
    case 0:
        _fWhitespace = true;
        _sSubState = 1;
        // fall through;

    case 1:
        // This state is used when we are not normalizing white space.  This
        // is a separate state for performance reasons.  
        // Normalizing whitespace is about 11% slower.
        while (_chLookahead != L'<' && ! _fEOF )
        {
            if (_chLookahead == L'&')
            {
                // then parse entity ref and then return
                // to state 1 to continue with PCDATA.
                return push(&XMLStream::parseEntityRef,1);
            }
 
            if (_chLookahead == L'>' && ! _fIE4Quirks)
            {
                WCHAR* pText;
                long len;
                _pInput->getToken((const WCHAR**)&pText, &len);
                if (len >= 2 && StrCmpN(L"]]", pText + len - 2, 2) == 0)
                     return XML_E_INVALID_CDATACLOSINGTAG;               
            }
// This slows us down too much.
//            else if (! isCharData(_chLookahead))
//            {
//                return XML_E_BADCHARDATA;
//            }

            hr = _pInput->scanPCData(&_chLookahead, &_fWhitespace);
            if (FAILED(hr))
            {
                if (hr == E_PENDING)
                {
                    hr = S_OK;
                    ADVANCE;
                }
                return hr;
            }
            checkhr(hr);
        }
        _sSubState = 2;
        // fall through

    case 2:
        if (_pInput->getTokenLength() > 0 || _fUsingBuffer)
        {
            _nToken = _fWhitespace ? XML_WHITESPACE : XML_PCDATA;
        }
        checkhr2(pop());
        break;

    default:
        INTERNALERROR;
    }   
    return S_OK;
}

HRESULT 
XMLStream::parseEntityRef()
{
    HRESULT hr = S_OK;
    long entityLen = 0, i, lLen = 1;
    const WCHAR* t; 
    long len;

Start:
    switch (_sSubState)
    {
    case 0: // ^ ( '&#' [0-9]+ ) | ('&#X' [0-9a-fA-F]+) | ('&' Name) ';'
        _nPreToken = XML_PENDING;
        _lEntityPos = _pInput->getTokenLength(); // record entity position.
        _fPCDataPending = (_lEntityPos > 0);

        if (PreEntityText())
        {
            // remember the pending text before parsing the entity.
            _nPreToken = _nToken;
            _nToken = XML_PENDING;
        }
        _sSubState = 1;
        // fall through
    case 1:
        ADVANCE; // soak up the '&'
        _sSubState = 2;
        // fall through
    case 2:
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead == L'#')
        {
            ADVANCE;
            _sSubState = 3;
            // fall through
        }
        else
        {
            // Loose entity parsing allows "...&6..."
            if (! isStartNameChar(_chLookahead))
            {
                if (_fFloatingAmp)
                {
                    // then it isn't an entity reference, so go back to PCDATA
                    if (_fUsingBuffer)
                    {
                        // this in case we are normalizing white space.
                        PushChar(L'&');
                    }
                    _fWhitespace = false;
                    checkhr2(pop());
                    return S_OK;
                }
                else if (ISWHITESPACE(_chLookahead))
                    return XML_E_UNEXPECTED_WHITESPACE;
                else
                    return XML_E_BADSTARTNAMECHAR;
            }
            checkhr2(push(&XMLStream::parseName, 6));
            _sSubState = 1; // avoid doing a mark() so we can return PCDATA if necessary.
            return parseName();
        }
        break;

        // ------------- Numeric entity references --------------------
    case 3:
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead == L'x')
        {
            // hex character reference.
            ADVANCE;
            STATE(5); // go to state 5
        }
        _sSubState = 4;
        // fall through

    case 4: // '&#' ^ [0-9]+ ';'
        checkhr2(ScanDecimalDigits());
        if (_chLookahead != L';')
        {
            STATE(9);
        }

        entityLen = _pInput->getTokenLength() - _lEntityPos;
        getToken(&t, &len);
        checkhr2(DecimalToUnicode(t + _lEntityPos + 2, entityLen - 2, _wcEntityValue));
        lLen = 2;
        _nToken = XML_NUMENTITYREF;
        GOTOSTART(10); // have to use GOTOSTART() because we want to use the values of t and len
        break;

    case 5: // '&#X' ^ [0-9a-fA-F]+
        checkhr2(ScanHexDigits());
        if (_chLookahead != L';')
        {
            STATE(9);
        }

        entityLen = _pInput->getTokenLength() - _lEntityPos;
        getToken(&t, &len);
        checkhr2(HexToUnicode(t + _lEntityPos + 3, entityLen - 3, _wcEntityValue));
        lLen = 3;
        _nToken = XML_HEXENTITYREF;
        GOTOSTART(10);  // have to use GOTOSTART() because we want to use the values of t and len
        break;
        
        // ------------- Named Entity References --------------------
    case 6: // '&' Name ^ ';'
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead != L';')
        {
            STATE(9);
        }

        // If parseName found a namespace then we need to calculate the
        // real nslen taking the pending PC data and '&' into account
        // and remember this in case we have to return the PCDATA.
        _nEntityNSLen = (_lNslen > 0) ? _lNslen - _lEntityPos - 1 : 0;
        _fUsingBuffer = false;

        entityLen = _pInput->getTokenLength() - _lEntityPos;
        getToken(&t, &len);

        if (0 != (_wcEntityValue = BuiltinEntity(t + _lEntityPos + 1, entityLen - 1)) ||
            (_fIE4Quirks && 0xFFFF != (_wcEntityValue = LookupBuiltinEntity(t + _lEntityPos + 1, entityLen - 1))))
        {
            lLen = 1;
            _nToken = XML_BUILTINENTITYREF;
            GOTOSTART(10);  // have to use GOTOSTART() because we want to use the values of t and len
        }
        else if (_nPreToken != XML_PENDING)
        {
            // Return previous token (XML_PCDATA or XML_WHITESPACE)
            _lLengthDelta = -entityLen;
            _lMarkDelta = entityLen - 1; // don't include '&' in _nToken.
            _nToken = _nPreToken;
            STATE(7);
        }

        mark(entityLen-1); // don't include '&' in _nToken.
        _sSubState = 7;
        // fall through

    case 7:
        ADVANCE; // soak up the ';'
        _nToken = XML_ENTITYREF;
        _lNslen = _nEntityNSLen;
        _lLengthDelta = -1; // don't include the ';'
        STATE(8); // return token and resume in state 8.
        break;

    case 8:
        mark();
        checkhr2(pop());
        return S_OK;

    case 9:
        // Soft entity handling - we just continue with PCDATA in 
        // this case.
        if (_fFloatingAmp)
        {
            if (_fUsingBuffer)
            {
                // this in case we are normalizing white space.  In this case
                // we have to copy what we have so far to the normalized buffer.
                long endpos = _pInput->getTokenLength();
                const WCHAR* t; long len;
                getToken(&t, &len);
                for (long i = _lEntityPos; i < endpos; i++)
                    PushChar(t[i]);
            }
            _fWhitespace = false;
            checkhr2(pop());
            return S_OK;
        }
        else
            return XML_E_MISSINGSEMICOLON;
        break;

    case 10:
        // Return the text before builtin or char entityref as XML_PCDATA
        if (_nPreToken)
        {
            _nPreToken = _nToken;
            _nToken = XML_PCDATA;
            _lLengthDelta = -entityLen;
            _lMarkDelta = entityLen - lLen; // don't include '&' in _nToken.
            STATE(11);  // return token and resume in state 12.
        }
        else
        {
            _nPreToken = _nToken;
            mark(entityLen - lLen);
            GOTOSTART(11);
        }
        break;

    case 11:
        // push the builtin entity
        _fUsingBuffer = true;
        PushChar(_wcEntityValue);
        _nToken = _nPreToken;
        STATE(12); // return token and resume in state 12.
        break;

    case 12:
        ADVANCE; // soak up the ';'
        STATE(8); // resume in state 8.
        break;

    default:
        INTERNALERROR;
    }   
    return S_OK;      
}

HRESULT 
XMLStream::parsePEDecl()
{
    // This method is used to distinguish between the following cases:
    //  <!ENTITY ^ S % name ... > <!-- a parameter entity declaration -->
    //  <!ENTITY ^ S %foo; ... >  <!-- a parameter entity reference -->

    HRESULT hr = S_OK;
    long entityLen = 0;

    switch (_sSubState)
    {
    case 0: // ^ S 
        if (! ISWHITESPACE(_chLookahead))
            return XML_E_MISSINGWHITESPACE;
        _sSubState = 1;
        // fall through
    case 1:
        _fHandlePE = false; // so we don't try and parse peref from inside skipWhiteSpace.
        checkhr2(push(&XMLStream::skipWhiteSpace, 2));
        checkhr2(skipWhiteSpace())
        _sSubState = 2;
        // fall through
    case 2:
        _fHandlePE = true;
        checkeof(_chLookahead, _lEOFError);
        if (_chLookahead == L'%')
        {
            ADVANCE;    // soak up the '%'
            STATE(4);
        }
        checkhr2(push(&XMLStream::parseName, 3));
        checkhr2( parseName() );
        _sSubState = 3;
        // fall through
    case 3:
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        _nToken = XML_ENTITYDECL;
        checkhr2(pop(false));
        return S_OK;

    case 4:
        if (ISWHITESPACE(_chLookahead))
        {
            checkhr2(push(&XMLStream::skipWhiteSpace, 5));
            return skipWhiteSpace();
        }
        // parsePEREf, then try again in state 1.
        return push(&XMLStream::parsePERef, 1); 

    case 5:
        checkhr2(push(&XMLStream::parseName, 6));
        checkhr2(parseName());
        _sSubState = 6;
        // fall through.

    case 6:
        _nToken = XML_PENTITYDECL;
        checkhr2(pop(false));
        return S_OK;

    default:
        INTERNALERROR;
    }   
    return S_OK;      

}

HRESULT
XMLStream::parsePERef()
{
    HRESULT hr;
    switch (_sSubState)
    {
    case 0:
        _sSubState = 1;
        // fall through

    case 1:
        checkhr2(push(&XMLStream::parseName, 2));
        checkhr2( parseName() );
        _sSubState = 2;
        // fall through

    case 2:
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        if (_chLookahead != L';') return XML_E_MISSINGSEMICOLON;
        ADVANCE;    // soak up the ';'
        _sSubState = 3;
        // fall through

    case 3:
        checkeof(_chLookahead, XML_E_UNEXPECTEDEOF);
        _nToken = XML_PEREF;
        _lLengthDelta = -1; // don't include the ';'
        checkhr2(pop());
        break;

    default:
        INTERNALERROR;
    }   
    return S_OK;      

}

HRESULT 
XMLStream::pushTable(short substate, const StateEntry* table, DWORD le)
{
    HRESULT hr = S_OK;

    checkhr2(push(&XMLStream::parseTable, substate));
    _pTable = table;
    _lEOFError = le;
    return hr;
}


HRESULT 
XMLStream::push(StateFunc f, short s)
{
    StateInfo* pSI = _pStack.push();
    if (pSI == NULL)
        return E_OUTOFMEMORY;
    pSI->_sSubState = s;
    pSI->_fnState = _fnState;
    pSI->_pTable = _pTable;
    pSI->_lEOFError = _lEOFError;
    pSI->_cStreamDepth = _cStreamDepth;

    _sSubState = 0;
    _fnState = f;

    return S_OK;
}

HRESULT
XMLStream::pop(bool boundary)
{
    StateInfo* pSI = _pStack.peek();

    if (_fDTD && 
        ! (_fParsingAttDef) && boundary && _cStreamDepth != pSI->_cStreamDepth) // _fParsingNames || 
    {
        // If we are in a PE and we are popping out to a state that is NOT in a PE
        // and this is a pop where we need to check this condition, then return an error.
        // For example, the following is not well formed because the parameter entity
        // pops us out of the ContentModel state in which the PE was found:
        // <!DOCTYPE foo [
        //      <!ENTITY % foo "a)">
        //      <!ELEMENT bar ( %foo; >
        //  ]>...
        return XML_E_PE_NESTING;
    }
    _fnState = pSI->_fnState;
    _sSubState = pSI->_sSubState;
    _pTable = pSI->_pTable;
    _lEOFError = pSI->_lEOFError;
    _pStack.pop();
    return S_OK;
}

HRESULT 
XMLStream::switchTo(StateFunc f)
{
    HRESULT hr;

    // Make sure we keep the old stream depth.
    StateInfo* pSI = _pStack.peek();
    int currentDepth = _cStreamDepth;
    _cStreamDepth = pSI->_cStreamDepth;

    checkhr2(pop(false));
    checkhr2(push(f,_sSubState)); // keep return to _sSubState the same

    _cStreamDepth = currentDepth;

    return (this->*f)();
}

HRESULT 
XMLStream::switchToTable(const StateEntry* table, DWORD le)
{
    HRESULT hr;

    // Make sure we keep the old stream depth.
    StateInfo* pSI = _pStack.peek();
    int currentDepth = _cStreamDepth;
    _cStreamDepth = pSI->_cStreamDepth;

    checkhr2(pop(false));
    checkhr2(pushTable(_sSubState,table,le)); // keep return to _sSubState the same

    _cStreamDepth = currentDepth;

    return parseTable();
}

//========================================================================
HRESULT 
XMLStream::parseCondSect()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        ADVANCE; // soak up the '[' character
        if (_fFoundPEREf) return S_OK;
        _sSubState = 1;
        // fall through
    case 1: // now match magic '[CDATA[' sequence.     
        checkeof(_chLookahead, XML_E_UNCLOSEDMARKUPDECL);
        if (_chLookahead == L'C')
        {
            _pchCDataState = g_pstrCDATA;
            STATE(5); // goto state 5
        }
        _sSubState = 2;   // must be IGNORE, INCLUDE or %pe;
        // fall through

    case 2: // must be DTD markup declaration
        // '<![' ^ S? ('INCLUDE' | 'IGNORE' | %pe;) S? [...]]> or 
        // skip optional whitespace
        if (_fInternalSubset)
            return XML_E_CONDSECTINSUBSET;
        checkeof(_chLookahead, XML_E_EXPECTINGOPENBRACKET);
        checkhr2(push(&XMLStream::skipWhiteSpace, 3));
        return skipWhiteSpace(); // must return because of %pe;

    case 3:
        checkeof(_chLookahead, XML_E_UNCLOSEDMARKUPDECL);
        checkhr2(push(&XMLStream::parseName,4));
        return parseName();

    case 4: // scanned 'INCLUDE' or 'IGNORE'
        {
            const WCHAR* t;
            long len;
            getToken(&t,&len);
            if (StringEquals(L"IGNORE",t,len,false))
            {
                return switchTo(&XMLStream::parseIgnoreSect);
            }
            else if (StringEquals(L"INCLUDE",t,len,false))
            {
                return switchTo(&XMLStream::parseIncludeSect);
            }
            else
                return XML_E_BADENDCONDSECT;
        }
        break;

    case 5: // parse CDATA name
        while (*_pchCDataState != 0 && _chLookahead == *_pchCDataState && ! _fEOF)
        {
            ADVANCE;            // advance first, before incrementing _pchCDataState
            _pchCDataState++;   // so that this state is re-entrant in the E_PENDING case.
            checkeof(_chLookahead, XML_E_UNCLOSEDMARKUPDECL);
        }
        if (*_pchCDataState != 0)
        {
            // must be INCLUDE or IGNORE section so go to state 2.
            _sSubState = 2;
        } 
        else if (_chLookahead != L'[')
        {
            return XML_E_EXPECTINGOPENBRACKET;
        }
        else if (_fDTD)
            return XML_E_CDATAINVALID;
        else
            return switchTo(&XMLStream::parseCData);

        return S_OK;
        break;        

    default:
        INTERNALERROR;
    }
    return S_OK;
}


HRESULT 
XMLStream::parseCData()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0:
        ADVANCE; // soak up the '[' character.
        mark(); // don't include 'CDATA[' in CDATA text
        _sSubState = 1;
        // fall through
    case 1:
        while (_chLookahead != L']' && ! _fEOF)
        {
            // scanPCData will stop when it sees a ']' character.
            hr = _pInput->scanPCData(&_chLookahead, &_fWhitespace);
            if (FAILED(hr))
            {
                if (hr == E_PENDING)
                {
                    hr = S_OK;
                    ADVANCE;
                }
                return hr;
            }
        }
        checkeof(_chLookahead, XML_E_UNCLOSEDCDATA);
        _sSubState = 2;
        // fall through
    case 2:
        ADVANCE; // soak up first L']' character.
        checkeof(_chLookahead, XML_E_UNCLOSEDCDATA);
        if (_chLookahead != L']')
        {
            // must have been floating ']' character, so
            // return to state 1.
            STATE(1); 
        }
        _sSubState = 3;
        // fall through
    case 3:
        ADVANCE; // soak up second ']' character.
        checkeof(_chLookahead, XML_E_UNCLOSEDCDATA);
        if (_chLookahead == L']')
        {
            // Ah, an extra ']' character, tricky !!  
            // In this case we stay in state 3 until we find a non ']' character
            // so you can terminate a CDATA section with ']]]]]]]]]]]]]]]]>'
            // and everying except the final ']]>' is treated as CDATA.
            STATE(3);
        }
        else if (_chLookahead != L'>')
        {
            // must have been floating "]]" pair, so
            // return to state 1.
            STATE(1);
        }
        _sSubState = 4;
        // fall through
    case 4:
        ADVANCE; // soak up the '>'
        _nToken = XML_CDATA;
        _lLengthDelta = -3; // don't include terminating ']]>' in text.
        checkhr2(pop()); // return to parseContent.
        return S_OK;
        break;

    default:
        INTERNALERROR;
    }
    return S_OK;
}

HRESULT 
XMLStream::parseIncludeSect()
{
    // now parse conditional section contents.
    // which is like parseDTDContent except it
    // terminates with ']]>' instead of ']' and it
    // does not allow another '<!['.
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0: // '<![' INCLUDE ^ S? '[' (decl | PI | comment | pe | S)* ']]>'
        _nToken = XML_INCLUDESECT;
        checkhr2(push(&XMLStream::skipWhiteSpace, 1));
        return skipWhiteSpace();

    case 1:
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (_chLookahead != L'[')
        {
            return XML_E_EXPECTINGOPENBRACKET;
        }
        ADVANCE;
        mark();
        if (_fFoundPEREf) return S_OK;
        _sSubState = 2;
        // fall through

    case 2: // '<![' name '[' ^ (decl | PI | comment | pe | S)* ']]>'
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        _cConditionalSection++;
        checkhr2(push(&XMLStream::parseDTDContent,3));
        return parseDTDContent();

    case 3: // ']' ^ ']>'
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (ISWHITESPACE(_chLookahead))
            return XML_E_UNEXPECTED_WHITESPACE;
        else if (_chLookahead != L']')
        {
            return XML_E_BADENDCONDSECT;
        }
        ADVANCE;
        _sSubState = 4;
        // fall through
    case 4: // ']]' ^ '>'
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (ISWHITESPACE(_chLookahead))
            return XML_E_UNEXPECTED_WHITESPACE;
        else if (_chLookahead != L'>')
        {
            return XML_E_BADENDCONDSECT;
        }
        ADVANCE;
        _nToken = XML_ENDCONDSECT;
        checkhr2(pop());
        return S_OK;
        
    default:
        INTERNALERROR;

    }
    return S_OK;
}

HRESULT 
XMLStream::parseIgnoreSect()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0: // <![IGNORE ^ S? '[' ...
        _cIgnoreSectLevel++; // allow nested ignore sections.
        checkhr2(push(&XMLStream::skipWhiteSpace, 1));
        return skipWhiteSpace();

    case 1: // <![IGNORE S? ^ '[' ...
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (_chLookahead != L'[')
        {
            return XML_E_EXPECTINGOPENBRACKET;
        }
        ADVANCE;
        if (_cIgnoreSectLevel == 1) mark();
        checkhr2(push(&XMLStream::skipWhiteSpace, 2));
        return skipWhiteSpace();

    case 2:
        _fHandlePE = false; // turn of parameter entity handling.
        while (_chLookahead != L']' && _chLookahead != L'<' && ! _fEOF)
        {
            ADVANCE;
        }
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (_chLookahead == L'<')    // watch out for nested conditional sections.
            STATE(6);
        _sSubState = 3;
        // fall through
    case 3:
        ADVANCE; // soak up first ']' character.
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (_chLookahead != L']')
        {
            // must have been floating ']' character, so\
            // return to state 2.
            STATE(2); 
        }
        _sSubState = 4;
        // fall through
    case 4:
        ADVANCE; // soak up second ']' character.
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (_chLookahead == L']')
        {
            // Ah, an extra ']' character, tricky !!  
            // In this case we stay in state 3 until we find a non ']' character
            // so you can terminate a CDATA section with ']]]]]]]]]]]]]]]]>'
            // and everying except the final ']]>' is treated as CDATA.
            STATE(4);
        }
        else if (_chLookahead != L'>')
        {
            // must have been floating "]]" pair, so
            // return to state 2.
            STATE(2);
        }
        _sSubState = 5;
        // fall through
    case 5:
        ADVANCE; // soak up the L'>'
        _cIgnoreSectLevel--;
        if (_cIgnoreSectLevel > 0)
        {
            // continue on in state 2.
            STATE(2);
        }
        else
        {
            _nToken = XML_IGNORESECT;   // return contents as one big string !
            _lLengthDelta = -3; // don't include terminating ']]>' in text.
            checkhr2(pop()); // return to parseContent.
            _fHandlePE = true; // turn parameter entities back on.
            return S_OK;
        }
        break;

    case 6: // <![IGNORE[ .... ^ '<'
        ADVANCE;    // soak up the '<' character
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (_chLookahead != L'!')
        {
            // must be something other than '<!'
            STATE(2); 
        }
        _sSubState = 7;
        // fall through

    case 7: // <![IGNORE[ .... '<' ^ '!'
        ADVANCE;    // soak up the '<' character
        checkeof(_chLookahead, XML_E_UNCLOSEDDECL);
        if (_chLookahead != L'[')
        {
            // must be something other than '<!['
            STATE(2); 
        }
        // Ok, this is the start of a nested conditional section, 
        // so start again in state 0 which will increment the nesting count.
        STATE(0);

    default:
        INTERNALERROR;
    }
    return S_OK;
}

HRESULT 
XMLStream::skipInternalSubset()
{
    HRESULT hr = S_OK;

    switch (_sSubState)
    {
    case 0:
        // Ok, now we skip to the end of the internal subset
        // and return it as one big string.  The client can then
        // create a new parser, put it in DTD mode and 
        // parse the DTD stuff.
        _nToken = XML_STARTDTDSUBSET;
        mark();
        _pInput->Lock();
        _fInternalSubset = true;
        _fDTD = true;
        checkhr2(push(&XMLStream::parseDTDContent, 1));
        return S_OK;
        break;

    case 1:
        _lLengthDelta = -1; // don't return closing ']' in token.
        _pInput->UnLock();
        _nToken = XML_DTDSUBSET;
        checkhr2(pop());
        break;
    }
    return hr;
}

HRESULT 
XMLStream::parseDTDContent()
{
    // This is similar to parseContent except it doesn't allow
    // PCData and normal XML elements.
    HRESULT hr = S_OK;
    if (_fEOF) 
    {
        if (_pStack.used() == 0)
            return XML_E_ENDOFINPUT;
        else if (_cConditionalSection)
            return XML_E_UNCLOSEDDECL;
        else
            return XML_E_UNEXPECTEDEOF;
    }

    switch (_sSubState)
    {
    case 0: // '[' ^ (decl | PI | comment | pe | S)* ']'
        switch (_chLookahead)
        {
        case L'<':
            ADVANCE;
            if (_chLookahead == L'!')
            {            
                checkhr2(_pInput->Freeze()); // stop shifting data until '>'
                return pushTable(  0, g_DeclarationTable, XML_E_UNCLOSEDDECL);
            }
            else if (_chLookahead == L'?')
            {
                checkhr2(push( &XMLStream::parsePI, 0));
                return parsePI();
            }
            else if (ISWHITESPACE(_chLookahead))
                return XML_E_UNEXPECTED_WHITESPACE;
            else 
                return XML_E_BADELEMENTINDTD;
            break;
        case L'%':
            ADVANCE;
            checkhr2(push(&XMLStream::parsePERef,0));
            return parsePERef();
        case L']':
            if (_fInternalSubset || _cConditionalSection)
            {
                // Internal Subset or conditional section is finished
                mark();
                ADVANCE; // soak up L']'
                if (_fInternalSubset)
                {
                    _fDTD = false;
                    _fInternalSubset = false;
                }
                _cConditionalSection--;
                checkhr2(pop()); // we're done, and now we should have '>'
                return S_OK;
            }
            else
            {
                return XML_E_BADCHARINDTD;
            }
            break;
        default:
            break;
        }
        if (! ISWHITESPACE(_chLookahead))
        {
            return XML_E_BADCHARINDTD;
        }
        // then soak up the whitespace - but return because
        // we may find parameter entity reference.
        checkhr2(push(XMLStream::skipWhiteSpace,1));
        return skipWhiteSpace();

    case 1:
        // return whitespace _nToken and continue in state 1.
        _sSubState = 0;
        if (_pInput->getTokenLength() > 0) // && ! _fNoWhitespaceNodes)
        {
            _nToken = XML_WHITESPACE;
        }
        return S_OK;

    default:
        INTERNALERROR;

    }
    return S_OK;
}

HRESULT
XMLStream::parseEquals()
{
    HRESULT hr = S_OK;
    switch (_sSubState)
    {
    case 0: // Eq ::= S? '=' S? 
        if (ISWHITESPACE(_chLookahead))
        {
            // allow whitespace between attribute and '='
            checkhr2(push(&XMLStream::skipWhiteSpace, 1));
            checkhr2( skipWhiteSpace() );            
        }
        _sSubState = 1;
        // fall through

    case 1:
        if (_chLookahead != L'=')
        {
            return XML_E_MISSINGEQUALS;
        }
        ADVANCE;
        if (ISWHITESPACE(_chLookahead))
        {
            // allow whitespace between '=' and attribute value.
            checkhr2(push(&XMLStream::skipWhiteSpace, 2));
            checkhr2( skipWhiteSpace() );            
        }
        _sSubState = 2;
        // fall through

    case 2:
        checkhr2(pop(false));
        break;

    default:
        INTERNALERROR;

    }
    return S_OK;
}

//===============================================================================
// Parse from a StateEntry parse table...

HRESULT 
XMLStream::parseTable()
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_pTable == g_EntityDeclTable)
        TaggedTrace(tagTokenizer,"g_EntityDeclTable");
    else if (_pTable == g_NotationDeclTable)
        TaggedTrace(tagTokenizer,"g_NotationDeclTable");
    else if (_pTable == g_DocTypeTable)
        TaggedTrace(tagTokenizer,"g_DocTypeTable");
    else if (_pTable == g_ExternalIDTable)
        TaggedTrace(tagTokenizer,"g_ExternalIDTable");
    else if (_pTable == g_ContentModelTable)
        TaggedTrace(tagTokenizer,"g_ContentModelTable");
    else if (_pTable == g_ElementDeclTable)
        TaggedTrace(tagTokenizer,"g_ElementDeclTable");
    else if (_pTable == g_AttListTable)
        TaggedTrace(tagTokenizer,"g_AttListTable");
    else if (_pTable == g_DeclarationTable)
        TaggedTrace(tagTokenizer,"g_DeclarationTable");
    else if (_pTable == g_XMLDeclarationTable)
        TaggedTrace(tagTokenizer,"g_XMLDeclarationTable");
#endif

    while (hr == S_OK && _nToken == XML_PENDING)
    {
        const StateEntry* pSE = &_pTable[_sSubState];

        DWORD newState = pSE->_sGoto;

#ifdef _DEBUG
        TaggedTrace(tagTokenizer,"state=%d", _sSubState);
#endif
        switch (pSE->_sOp)
        {
        case OP_WS:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            if (! ISWHITESPACE(_chLookahead))
                return XML_E_MISSINGWHITESPACE;
            // fall through
        case OP_OWS:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            checkhr2(push(&XMLStream::skipWhiteSpace, (short)newState));
            checkhr2(skipWhiteSpace());
            if (_fFoundPEREf) return XML_E_FOUNDPEREF;
            break;
        case OP_NWS:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            if (! ISWHITESPACE(_chLookahead))
                newState = pSE->_sGoto;
            else
                newState = pSE->_sArg1;
            break;
        case OP_CHARWS:
            if (_fFoundPEREf) return S_OK;
            mark();
            checkeof(_chLookahead, _lEOFError);
            if (_chLookahead == pSE->_pch[0])
            {
                ADVANCE;
                newState = pSE->_sGoto;
                _nToken = pSE->_lDelta;
            }
            else if (! ISWHITESPACE(_chLookahead))
            {
                return XML_E_WHITESPACEORQUESTIONMARK;
            }
            else
                newState = pSE->_sArg1;
            break;
        case OP_CHAR:
            if (_fFoundPEREf) return S_OK;
            mark();
        case OP_CHAR2:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            if (_chLookahead == pSE->_pch[0])
            {
                ADVANCE;
                newState = pSE->_sGoto;
                _nToken = pSE->_lDelta;
                if (_nToken == XML_GROUP)
                    _nAttrType = XMLTYPE_NMTOKEN;
            }
            else
            {
                newState = pSE->_sArg1;
                if (newState >= XML_E_PARSEERRORBASE &&
                    ISWHITESPACE(_chLookahead))
                    return XML_E_UNEXPECTED_WHITESPACE;
            }
            break;
        case OP_PEEK:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            if (_chLookahead == pSE->_pch[0])
            {
                newState = pSE->_sGoto;
            }
            else
                newState = pSE->_sArg1;
            break;
        case OP_NAME:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            checkhr2(push(&XMLStream::parseName, (short)newState));
            checkhr2(parseName());
            break;
        case OP_NMTOKEN:
            if (_fFoundPEREf) return S_OK;
            if (pSE->_sArg1 != 0)
                mark();

            checkeof(_chLookahead, _lEOFError);
            checkhr2(push(&XMLStream::parseNmToken, (short)newState));
            checkhr2(parseNmToken());
            break;
        case OP_TOKEN:
            _nToken = pSE->_sArg1;
            _lLengthDelta = pSE->_lDelta;  
            break;
        case OP_STRING:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            _chBreakChar = 0;
            checkhr2(push(&XMLStream::parseString, (short)newState));
            checkhr2(parseString());
            break;
        case OP_EXTID:
            if (_fFoundPEREf) return S_OK;
            // must be ExternalID (section 4.2.2)
            checkeof(_chLookahead, _lEOFError);
            _fShortPubIdOption = (pSE->_sArg1 != 0);
            // push the g_ExternalIDTable parse table !!
            return pushTable((short)newState, g_ExternalIDTable, XML_E_UNCLOSEDDECL);
            break;
        case OP_POP:
            _lLengthDelta = pSE->_lDelta;
            if (_lLengthDelta == 0) mark();
            // The _lDelta field contains a boolean flag to tell us whether this
            // pop needs to check for parameter entity boundary or not.
            checkhr2(pop(pSE->_lDelta == 0)); // we're done !
            _nToken = pSE->_sArg1;
            _nAttrType = XMLTYPE_CDATA;
            return S_OK;
        case OP_STRCMP:
            {
                const WCHAR* t;
                long len;
                getToken(&t,&len);
                long delta = (pSE->_lDelta < 0) ? pSE->_lDelta : 0;
                if (StringEquals(pSE->_pch,t,len+delta,_fCaseInsensitive))
                {
                    if (pSE->_lDelta > 0) 
                    {
                        _nToken = pSE->_lDelta;
                        _lLengthDelta = 0;
                    }


                    if (_pTable == g_AttListTable)
                    {
                        // special case for <!ATTLIST parsing...
                        switch (_nToken)
                        {
                        case XML_AT_CDATA:
                            _nAttrType = XMLTYPE_CDATA;
                            break;
                        case XML_AT_ID:
                        case XML_AT_IDREF:
                        case XML_AT_ENTITY:
                            _nAttrType = XMLTYPE_NAME;
                            break;
                        case XML_AT_IDREFS:
                        case XML_AT_ENTITIES:
                            _nAttrType = XMLTYPE_NAMES;
                            break;
                        case XML_AT_NMTOKEN:
                        case XML_AT_NOTATION:
                            _nAttrType = XMLTYPE_NMTOKEN;
                            break;
                        case XML_AT_NMTOKENS:
                            _nAttrType = XMLTYPE_NMTOKENS;
                            break;
                        }
                    }

                    newState = pSE->_sGoto;
                }
                else
                    newState = pSE->_sArg1;
             }
             break;
        case OP_SUBSET:
            if (_fFoundPEREf) return S_OK;
            checkhr2(push(&XMLStream::skipInternalSubset, (short)newState));
            return skipInternalSubset();

        case OP_PUBIDOPTION:
            if (_fShortPubIdOption)
                newState = pSE->_sGoto;
            else
                newState = pSE->_sArg1;
            break;

        case OP_FAKESYSTEM:
            // Now we have to also fake a "SYSTEM" token.
            _nToken = XML_SYSTEM;
            _fUsingBuffer = true;
            PushChar(L'S');
            PushChar(L'Y');
            PushChar(L'S');
            PushChar(L'T');
            PushChar(L'E');
            PushChar(L'M');
            break;

        case OP_TABLE:
            checkeof(_chLookahead, _lEOFError);
            _nToken = pSE->_sArg1;
            return pushTable((short)newState, 
                (const StateEntry*)(pSE->_pch), XML_E_UNCLOSEDDECL);

        case OP_STABLE:
            checkeof(_chLookahead, _lEOFError);
            return switchToTable((const StateEntry*)(pSE->_pch), XML_E_UNCLOSEDDECL);

        case OP_COMMENT:
            return push(&XMLStream::parseComment, (short)newState);
            break;

        case OP_CONDSECT:
            if (_fFoundPEREf) return S_OK;
            // parse <![CDATA[...]]> or <![IGNORE[...]]>
            return push(&XMLStream::parseCondSect, (short)newState);

        case OP_SNCHAR:
            checkeof(_chLookahead, _lEOFError);
            if (isStartNameChar(_chLookahead))
            {
                newState = pSE->_sGoto;
            }
            else
                newState = pSE->_sArg1;
            break;
        case OP_EQUALS:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            checkhr2(push(&XMLStream::parseEquals, (short)newState));
            checkhr2(parseEquals());
            break;
        case OP_ENCODING:
            {
                const WCHAR* t;
                long len;
                _pInput->getToken(&t,&len);
                hr =  _pInput->switchEncoding(t, len+pSE->_lDelta);
            }
            break;

        case OP_ATTRVAL:
            if (_fFoundPEREf) return S_OK;
            if (_chLookahead != L'"' && _chLookahead != L'\'')
            {
                return XML_E_MISSINGQUOTE;
            }
            _chTerminator = _chLookahead;
            ADVANCE; 
            mark();
            _fReturnAttributeValue = (pSE->_sArg1 == 1);
            checkeof(_chLookahead, _lEOFError);
            return push(&XMLStream::parseAttrValue, (short)newState);
            break;

        case OP_PETEST:
            if (_fFoundPEREf) return S_OK;
            checkeof(_chLookahead, _lEOFError);
            checkhr2(push(&XMLStream::parsePEDecl, (short)newState));
            return parsePEDecl();
            break;

        case OP_NMSTRING:
            checkhr2(push(&XMLStream::parseNMString, (short)newState));
            checkhr2(parseNMString());
            break;
        }
        if (_fnState != &XMLStream::parseTable)
            return S_OK;

        if (newState >= XML_E_PARSEERRORBASE)
            return (HRESULT)newState;
        else
            _sSubState = (short)newState;
    }

    if (_nToken == XMLStream::XML_ENDDECL)
    {
        return _pInput->UnFreeze();
    }
    return S_OK;
}


HRESULT    
XMLStream::_PushChar(WCHAR ch) 
{
    // buffer needs to grow.
    long newsize =  (_lBufSize+512)*2 ;
    WCHAR* newbuf = new_ne WCHAR[newsize];
    if (newbuf == NULL)
        return E_OUTOFMEMORY;
    if (_pchBuffer != NULL)
    {
        ::memcpy(newbuf, _pchBuffer, sizeof(WCHAR)*_lBufLen);
        delete[] _pchBuffer;
    }
    _lBufSize = newsize;
    _pchBuffer = newbuf;   
    _pchBuffer[_lBufLen++] = ch;
    return S_OK;
}

HRESULT 
XMLStream::AdvanceTo(short substate)
{
    // This method combines and advance with a state switch in one
    // atomic operation that handles the E_PENDING case properly.

    _sSubState = substate;

    HRESULT hr = (!_fDTD) ? _pInput->nextChar(&_chLookahead, &_fEOF) : DTDAdvance(); 
    if (hr != S_OK && (hr == E_PENDING || hr == E_DATA_AVAILABLE || hr == E_DATA_REALLOCATE || hr == XML_E_FOUNDPEREF))
    {
        // Then we must do an advance next time around before continuing
        // with previous state.  Push will save the _sSubState and return
        // to it.
        push(&XMLStream::firstAdvance,substate);
    }    
    return hr;
}

bool
XMLStream::PreEntityText()
{
    // This is a helper function that calculates whether or not to
    // return some PCDATA or WHITEPACE before an entity reference.
    if (_fPCDataPending)
    {
        // return what we have so far.
        if (_fWhitespace && ! _fIE4Quirks) // in IE4 mode we do not have WHITESPACE nodes
                                           // and entities are always resolved, so return
                                           // the leading whitespace as PCDATA.
            _nToken = XML_WHITESPACE;                                
        else                               
            _nToken = XML_PCDATA;

        long entityLen = _pInput->getTokenLength() - _lEntityPos;
        _lLengthDelta = -entityLen;
        _lMarkDelta = entityLen;
        _fPCDataPending = false;
        _fWhitespace = true;
        return true;
    }

    return false;
}

HRESULT 
XMLStream::ErrorCallback(HRESULT hr)
{
    if (hr == E_DATA_AVAILABLE)
        hr = XML_DATAAVAILABLE;
    else if (hr == E_DATA_REALLOCATE)
        hr = XML_DATAREALLOCATE;
    return _pXMLParser->ErrorCallback(hr);
}
