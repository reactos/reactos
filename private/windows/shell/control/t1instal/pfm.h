/***
**
**   Module: PFM
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      will extract information from a T1 font metrics file, by parsing
**      the data/commands found in a PFM file.
**
**      Please note the all data stored in a PFM file is represented
**      in the little-endian order.
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

struct T1Metrics;


/***
** Function: ReadPFMMetrics
**
** Description:
**   This function parses a Printer Font Metrics
**   (*.pfm) file. 
***/
errcode  ReadPFMMetrics    _ARGS((IN      char *metrics,
                                  OUT     struct T1Metrics *t1m));
