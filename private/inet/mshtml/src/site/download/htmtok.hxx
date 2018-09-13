//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmtok.hxx
//
//  Contents:   enums shared by htmpre.cxx and htmtok.cxx
//
//-------------------------------------------------------------------------

#ifndef I_HTMTOK_HXX_
#define I_HTMTOK_HXX_
#pragma INCMSG("--- Beg 'htmtok.hxx'")


enum {
    TS_TEXT        = 0,
    TS_PLAINTEXT   = 1,
    TS_TAGOPEN     = 2,
    TS_TAGSCAN     = 3,
    TS_TAGSCANQ    = 4,
    TS_TAGASP      = 5,
    TS_TAGBANG     = 6,
    TS_TAGCOMMENT  = 7,
    TS_TAGCOMDASH  = 8,
    TS_ENTOPEN     = 9,
    TS_ENTMATCH    = 10,
    TS_ENTCLOSE    = 11,
    TS_ENTNUMBER   = 12,
    TS_SUSPEND     = 13,
    TS_NEWCODEPAGE = 14,
    TS_ENTHEX      = 16,
    TS_CONDSCAN    = 17,
    TS_CONDITIONAL = 18,
    TS_ENDCOND     = 19,
    TS_ENDCONDBANG = 20,
    TS_ENDCONDSCAN = 21,
    TS_Last_Enum
};

// OutputTag codes
enum {OT_NORMAL = 0, OT_REJECT };


#pragma INCMSG("--- End 'htmtok.hxx'")
#else
#pragma INCMSG("*** Dup 'htmtok.hxx'")
#endif

