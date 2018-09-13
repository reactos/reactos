/***
 **
 **   Module: Trig
 **
 **   Description:
 **    This is a module of the T1 to TT font converter. The module
 **    contains a look-up table for computing atan2() faster, and
 **    with less precision than that of the c run-time library.
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
#ifndef FASTCALL
#  ifdef MSDOS
#     define FASTCALL   __fastcall
#  else
#     define FASTCALL
#  endif
#endif

#define PI    1024
#define PI2   512
#define PI4   256



/***
** Function: Atan2
**
** Description:
**   Compute atan2()
***/
int FASTCALL Atan2   _ARGS((IN int dy, IN int dx));
