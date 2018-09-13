//*********************************************************************
//*                  Microsoft Windows                               **
//*         Copyright (c) 1996 - 1999 Microsoft Corporation. All rights reserved.**///*********************************************************************

//;begin_internal
/***********************************************************************************************

  This is a distributed SDK component - do not put any #includes or other directives that rely
  upon files not dropped. If in doubt - build iedev

  If you add comments please include either ;BUGBUG at the beginning of a single line OR
  enclose in a ;begin_internal, ;end_internal block - such as this one!

 ***********************************************************************************************/
//;end_internal

//;begin_internal
#ifndef __MSXQLDID_H__
#define __MSXQLDID_H__
//;end_internal

#define DISPID_XOBJ_MIN                 0x00010000
#define DISPID_XOBJ_MAX                 0x0001FFFF
#define DISPID_XOBJ_BASE                DISPID_XOBJ_MIN


#define  DISPID_XQLPARSER               DISPID_XOBJ_BASE
#define  DISPID_XQLPARSER_QUERY         DISPID_XQLPARSER + 1
#define  DISPID_XQLPARSER_SORTEDQUERY   DISPID_XQLPARSER + 2

#define  DISPID_XQLQUERY                DISPID_XQLPARSER + 100
#define  DISPID_XQLQUERY_DATASRC        DISPID_XQLQUERY +  1
#define  DISPID_XQLQUERY_RESET          DISPID_XQLQUERY +  2
#define  DISPID_XQLQUERY_PEEK           DISPID_XQLQUERY +  3
#define  DISPID_XQLQUERY_NEXT           DISPID_XQLQUERY +  4
#define  DISPID_XQLQUERY_INDEX          DISPID_XQLQUERY +  5
#define  DISPID_XQLQUERY_CONTAINS       DISPID_XQLQUERY +  6

#define  DISPID_XQLQUERY_DATASRC_DOM    DISPID_XQLQUERY +  7
#define  DISPID_XQLQUERY_PEEK_DOM       DISPID_XQLQUERY +  8
#define  DISPID_XQLQUERY_NEXT_DOM       DISPID_XQLQUERY +  9
#define  DISPID_XQLQUERY_CONTAINS_DOM   DISPID_XQLQUERY +  10


//;begin_internal
#endif // __MSXQLDID_H__
//;end_internal