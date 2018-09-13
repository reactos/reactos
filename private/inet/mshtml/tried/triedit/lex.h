// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __LEX__
#define __LEX__

#include <tchar.h>

#include "fmtinfo.h"
#include "token.h"

#define P_IN(x)     const x &
#define P_OUT(x)    x &
#define P_IO(x)     x &
#define PURE        = 0

// Lex state, kept at the beginning of every line (lxsBOL) from
// previous line's state at its end (lxsEOL). Must fit all bits
// necessary to restart lexing on a line by line basis.

typedef DWORD   LXS;
typedef LXS *   PLXS;

// Lexer and language Metrics
const unsigned ctchUserTokenPhrase = 100;
struct USERTOKENS
{
    INT         token;      // preassigned in the user range
    TCHAR       szToken[ctchUserTokenPhrase+1]; // token class name exposed to user
    COLORREF    RGBText;
    COLORREF    RGBBackground;
    AUTO_COLOR  autocolorFore;
    AUTO_COLOR  autocolorBack;
};
typedef USERTOKENS *        PUSERTOKENS;
typedef const USERTOKENS *  PCUSERTOKENS;


// Alternate way of looking at a token, editor will only look at tokUser.
// Other clients of the lexer (like the parser or the EE) may want to look
// at the actual token in tokAct.  If any of tokAct is set, then it is expected
// that the actual token is different than the meta token it passed back.
// The status bits are only used by the lexer for whatever it wants.

union TOK_ALT  {
    TOKEN   tok;
    struct {
        unsigned        tokUser : 12;
        unsigned        tokUserStatus : 4;
        unsigned        tokAct : 12;
        unsigned        tokActStatus : 4;
    };
};


// A SUBLANG structure was originally used for identifying different
// dialects of the same language (like fortran fixed and fortran free)
// that use the same lexer, can be treated as two languages in the editor,
// and share all the same color/font info in the format dialog.
//
// We've extended it to be a general descriptor for a type of text file.
//
struct SUBLANG
{
    LPCTSTR szSubLang;
    LXS     lxsInitial;
    UINT    nIdTemplate; // Icon and MFC doc template string resource id
    CLSID   clsidTemplate;
};
typedef SUBLANG * PSUBLANG;
typedef const SUBLANG * PCSUBLANG;

#define MAX_LANGNAMELEN (50)

#endif
