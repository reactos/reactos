//-----------------------------------------------------------------------------
//  deblexr.c - replacement (portable) for deblexer.asm
//
//  Copyright (C) 1993, Microsoft Corporation
//
//  Revision History:
//
//  []      27-Apr-1993 Dans    Created
//
//-----------------------------------------------------------------------------

#if 0
; This module implements a very basic transition diagram lexer for
; use in the QC debugging expression evaluator.  It is flexible enough
; to facilitate future expansion to include more operators.
;
; The state tables are fairly simple to operate.  Consider, for example,
; the '>' symbol in C.  This can be followed by '>', '=' or something
; else.  If it is followed by '>', it can thereafter be followed by
; '=' or something else.  In all, we have four possibilities:
;
; >, >=, >>, >>=
;
; The transition diagram would be something like:
;
;        '>'          '>'          '='
; start ----- state1 ----- state2 ----- token('>>=')
;                |            |
;                |            |other
;                |            +----- token('>>')
;                | '='
;                +----- token('>=')
;                |
;                |other
;                +----- token('>')
;
; Each entry in LexTable is a single character (thus, a transition to
; another state based on "char is digit 0..9" CANNOT be handled by this
; code -- that's why it's simple) followed by either the identifier
; INTERMEDIATE or ENDSTATE, indicating whether following that edge leads you
; to a new state or to an actual value (token).  If it is followed by
; INTERMEDIATE, the next word must contain the offset of the new state
; table.  If followed by ENDSTATE, the next word contains the token value.
;
; Thus, the above example would look like this (using the macro defined
; below):
;
; LexTable label byte
;
;   LexEntry  '>',       INTERMEDIATE, <dataOFFSET LTstate1>
;   ...
;   (other entries)
;   ...
;   LexEntry  TABLE_END, 0, 0
;
; LTstate1 label byte
;
;   LexEntry  '>',       INTERMEDIATE, <dataOFFSET LTstate2>
;   LexEntry  '=',       ENDSTATE,     TOK_GTEQ
;   LexEntry  OTHER,     ENDSTATE,     TOK_GT
;
; LTstate2 label byte
;
;   LexEntry  '=',       ENDSTATE,     TOK_GTGTEQ
;   LexEntry  OTHER,     ENDSTATE,     TOK_GTGT
;
; Note that for the intermediate state tables, a TABLE_END entry is
; unnecessary since the OTHER route is automatically taken.
;
; These routines do NOT handle identifiers or constants; only those
; symbol strings explicitly defined in the state tables will be
; recognized (i.e., only operators).
;------------------------------------------------------------
;
;------------------------------------------------------------
; Macro for clean lexer tables
;------------------------------------------------------------

LexEntry    macro   Character, StateType, NextTableOrTok

    db  Character, StateType
ifdef HOST32
    dd  NextTableOrTok
else
    dw  NextTableOrTok
endif

        endm

;------------------------------------------------------------
; Identifiers used for tables
;------------------------------------------------------------

INTERMEDIATE    equ 1
ENDSTATE        equ 2


#endif

#include <stddef.h>
#include "debexpr.h"

typedef struct LEXENT * PLEXENT;

typedef struct LEXENT {
    unsigned char   ch;
    unsigned char   state;
    PLEXENT         plexentNext;
} LEXENT;

/*
**
** Identifiers used for tables
**
*/

#define INTERMEDIATE    1
#define ENDSTATE        2

/*
** The use of the following constants assumes that the character string
** being lexed contains only ASCII values 00h <= val <= 7Fh.
*/

#define OTHER       ((unsigned char) 0xFE)
#define TABLE_END   ((unsigned char) 0xFF)

/*
**; Second state intermediate state tables
*/

LEXENT    LTltlt[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_shleq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_shl
    };

LEXENT    LTgtgt[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_shreq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_shr
    };

LEXENT    LTdashgt[] = {
    '*',    ENDSTATE,   (PLEXENT) OP_pmember,
    OTHER,  ENDSTATE,   (PLEXENT) OP_pointsto
    };

/*
** First state intermediate state tables
*/

LEXENT    LTdash[] = {
    '>',    INTERMEDIATE,   (PLEXENT) LTdashgt,
    '=',    ENDSTATE,   (PLEXENT) OP_minuseq,
    '-',    ENDSTATE,   (PLEXENT) OP_decr,
    OTHER,  ENDSTATE,   (PLEXENT) OP_negate
    };

LEXENT    LTbang[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_bangeq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_bang
    };

LEXENT    LTstar[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_multeq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_fetch
    };

LEXENT    LTampersand[] = {
    '&',    ENDSTATE,   (PLEXENT) OP_andand,
    '=',    ENDSTATE,   (PLEXENT) OP_andeq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_addrof
    };

LEXENT    LTslash[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_diveq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_div
    };

LEXENT    LTpct[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_modeq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_mod
    };

LEXENT    LTplus[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_pluseq,
    '+',    ENDSTATE,   (PLEXENT) OP_incr,
    OTHER,  ENDSTATE,   (PLEXENT) OP_uplus
    };

LEXENT    LTlessthan[] = {
    '<',    INTERMEDIATE,   (PLEXENT) LTltlt,
    '=',    ENDSTATE,   (PLEXENT) OP_lteq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_lt
    };

LEXENT    LTgreaterthan[] = {
    '>',    INTERMEDIATE,   (PLEXENT) LTgtgt,
    '=',    ENDSTATE,   (PLEXENT) OP_gteq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_gt
    };

LEXENT    LTequals[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_eqeq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_eq,
    };

LEXENT    LTcaret[] = {
    '=',    ENDSTATE,   (PLEXENT) OP_xoreq,

    OTHER,  ENDSTATE,   (PLEXENT) OP_xor
    };

LEXENT    LTpipe[] = {
    '|',    ENDSTATE,   (PLEXENT) OP_oror,
    '=',    ENDSTATE,   (PLEXENT) OP_oreq,
    OTHER,  ENDSTATE,   (PLEXENT) OP_or
    };

LEXENT    LTdot[] = {
    '*',    ENDSTATE,   (PLEXENT) OP_dotmember,
    OTHER,  ENDSTATE,   (PLEXENT) OP_dot
    };

LEXENT    LTcolon[] = {
    ':',    ENDSTATE,   (PLEXENT) OP_uscope,
    '>',    ENDSTATE,   (PLEXENT) OP_baseptr,
    OTHER,  ENDSTATE,   (PLEXENT) OP_segop
    };


/*
** main Lexer table
*/

LEXENT    LexTable [] = {
    '+',    INTERMEDIATE,   (PLEXENT) LTplus,
    '-',    INTERMEDIATE,   (PLEXENT) LTdash,
    '*',    INTERMEDIATE,   (PLEXENT) LTstar,
    '&',    INTERMEDIATE,   (PLEXENT) LTampersand,
    '/',    INTERMEDIATE,   (PLEXENT) LTslash,
    '.',    INTERMEDIATE,   (PLEXENT) LTdot,
    '!',    INTERMEDIATE,   (PLEXENT) LTbang,
    '~',    ENDSTATE,       (PLEXENT) OP_tilde,
    '%',    INTERMEDIATE,   (PLEXENT) LTpct,
    '<',    INTERMEDIATE,   (PLEXENT) LTlessthan,
    '>',    INTERMEDIATE,   (PLEXENT) LTgreaterthan,
    '=',    INTERMEDIATE,   (PLEXENT) LTequals,
    '^',    INTERMEDIATE,   (PLEXENT) LTcaret,
    '|',    INTERMEDIATE,   (PLEXENT) LTpipe,
    ':',    INTERMEDIATE,   (PLEXENT) LTcolon,
    ';',    ENDSTATE,       (PLEXENT) OP_lowprec,
    ',',    ENDSTATE,       (PLEXENT) OP_comma,
    '(',    ENDSTATE,       (PLEXENT) OP_lparen,
    ')',    ENDSTATE,       (PLEXENT) OP_rparen,
    '[',    ENDSTATE,       (PLEXENT) OP_lbrack,
    ']',    ENDSTATE,       (PLEXENT) OP_rbrack,
    '{',    ENDSTATE,       (PLEXENT) OP_lcurly,
    '}',    ENDSTATE,       (PLEXENT) OP_rcurly,

    TABLE_END,  0,      0
};


/*------------------------------------------------------------
; ptoken_t ParseOp (pb, pTok)
; unsigned char *pb;
; token_t *pTok;
;
; Scans the input string (pb) for the next token and returns
; the token type.  Also returns the number of characters in
; the token so that the caller can advance the input stream
; before calling again.  The string need not be NULL-terminated:
; it will only scan as deep as the lexer tables indicate.
;------------------------------------------------------------
*/

EESTATUS
ParseOp (
    unsigned char * pb,
    token_t * lpTok
    )
{
    PLEXENT plexent = &LexTable[0];

    /*
    **  Skip over any leading white space in the string
    **  as this is not part of the next token
    */

    while (*pb == ' ')
        pb++;

    while ( TRUE ) {
        /*
        **  Check for the end of this lexer table.  If we
        **  run off the table then we can not recognized this
        **  token and return an error.
        */

        if (plexent->ch == TABLE_END) {
            lpTok->opTok = OP_badtok;
            return /*EESYNTAX*/ 10;
            }

        /*
        **  Check for the wild card marker.  This means that
        **  we have found a complete token prior to this character.
        **  An example of this is '<a'.
        */

        if (plexent->ch == OTHER) {
//          Assert(plexent->state == ENDSTATE);
            lpTok->pbEnd = (char *) pb;
            lpTok->opTok = (op_t) (INT_PTR) plexent->plexentNext;
            return EENOERROR;
            }

        /*
        **  Check for a match of this character against
        **  the parser table
        */

        if (plexent->ch == *pb) {
            /*
            **  It matches -- see if we have found a complete token
            */

            pb++;
            if (plexent->state == ENDSTATE) {
                lpTok->pbEnd = (char *) pb;
                lpTok->opTok = (op_t) (INT_PTR) plexent->plexentNext;
                return EENOERROR;
                }
            else {
                plexent = plexent->plexentNext;
                }
            }
        else {
            /*
            **  Move to the next entry in the lexer table
            */

            plexent++;
            }
        }
}  /* ParseOp() */
