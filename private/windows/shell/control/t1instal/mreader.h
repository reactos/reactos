/***
**
**   Module: MReader
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      will extract information from a T1 font metrics file, by parsing
**      the data/commands found in PFM and AFM files.
**
**   Author: Michael Jansson
**
**   Created: 5/26/93
**
***/


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif



/***
** Function: ReadFontMetrics
**
** Description:
**  Read a font metrics file that associated to a type 1 font.
***/
errcode   ReadFontMetrics   _ARGS((IN   char *metrics,
                                   OUT  struct T1Metrics *t1m));



