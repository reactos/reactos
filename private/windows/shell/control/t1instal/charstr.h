/***
 **
 **   Module: CharStr
 **
 **   Description:
 **    This is a module of the T1 to TT font converter. The module
 **    contain one function that interprets the commands in a T1
 **    CharString and builds a representation of the glyph for the
 **    it.
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

struct Subrs {
   USHORT len;
   UBYTE *code;
};


struct T1Glyph;
struct Composite;
struct PSState;

/***
** Function: AllocPSState
**
** Description:
**   This function allocates the workspace
**   used by the t1 parser.
***/
struct PSState *AllocPSState     _ARGS((void));


/***
** Function: InitPS
**
** Description:
**   This function initiate the workspace
**   used by the t1 parser.
***/
void           InitPS            _ARGS((INOUT   struct PSState *ps));


/***
** Function: FreePSState
**
** Description:
**   This function frees the workspace
**   used by the t1 parser.
***/
void           FreePSState       _ARGS((INOUT   struct PSState *ps));


/***
** Function: ParseCharString
**
** Description:
**   This function parses a CharString and builds a
**   of the charstring glyph.
***/
errcode        ParseCharString   _ARGS((INOUT   struct T1Glyph *glyph,
                                        INOUT   struct Composite **comp,
                                        INOUT   struct PSState *ps,
                                        IN      struct Subrs *subrs,
                                        INOUT   UBYTE *code,
                                        INOUT   USHORT len));

