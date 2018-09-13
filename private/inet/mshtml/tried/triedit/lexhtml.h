// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __LEXHTML__
#define __LEXHTML__

#include "lex.h"
#include "tokhtml.h"

#define DS_HTML_IE3         _T("HTML - IE 3.0")
#define DS_HTML_RFC1866     _T("HTML 2.0 (RFC 1866)")


// token hints
#define BASED_HINT  0x9000
#define ERR         BASED_HINT +  0  // error
#define HWS         BASED_HINT +  1  // white space
#define HST         BASED_HINT +  2  // string "..."
#define HSL         BASED_HINT +  3  // string alternate '...'
#define HNU         BASED_HINT +  5  // number
#define HKW         BASED_HINT +  6  // keyword
#define HEN         BASED_HINT +  7  // entity &...;
#define HRN         BASED_HINT +  8  // reserved name #...
#define HEP         BASED_HINT +  9  // parameter entity %...;
#define HTA         BASED_HINT + 10  // tag open <
#define HTE         BASED_HINT + 11  // tag end >
#define HDB         BASED_HINT + 12  // dbcs (> 128).   HTMED CHANGE (walts)
#define HAV         BASED_HINT + 13  // valid attr value start char     HTMED CHANGE (walts)

// strictly single ops
#define ODA tokOpDash
#define OCO tokOpComma
#define OPI tokOpPipe
#define OPL tokOpPlus
#define OEQ tokOpEqual
#define OST tokOpStar
#define OLP tokOpLP
#define ORP tokOpRP
#define OLB tokOpLB
#define ORB tokOpRB
#define OQU tokOpQuestion
#define OLC tokDELI_LCBKT   
#define ORC tokDELI_RCBKT   
#define ONL tokNEWLINE
#define EOS tokEOF

typedef unsigned short HINT;

typedef BYTE RWATT_T;
//
// Reserved Word Attributes - HTML variant
//
enum RWATT
{
    HTML2 = 0x01,  // RFC 1866
//  IEXP2 = 0x02,  // Internet Explorer 2.0
    IEXP3 = 0x04,  // Internet Explorer 3.0
    ALL   = 0xff,  // all browsers
};
#define IEXPn (IEXP3)
#define IE40  (ALL)

typedef struct ReservedWord
{
    TCHAR *     psz;        // pointer to reserved word string 
    BYTE        cb;         // length of reserved word
    RWATT_T     att;        // attributes
} ReservedWord;

#endif // __LEXHTML__

