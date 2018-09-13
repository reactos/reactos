/***
**
**   Module: Hints
**
**   Description:
**      This is a module of the T1 to TT font converter. This is a
**      sub-module of the T1 to TT data translator module. It deals
**      with hints. Any part pf the T1 font that gets translated into
** TrueType instructions is done within this module.
**
**   Author: Michael Jansson
**
**   Created: 8/24/93
**
***/


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif



/***
** Function: ConvertHints
**
** Description:
**   This functions converts hstem, hstem3, vstem, vstem3 and flex
**   hints, as well as doing diagonal control.
***/
errcode        ConvertHints      _ARGS((INOUT   struct T1Metrics *t1m,
                                        IN      Hints *hints,
                                        IN      Outline *orgpaths,
                                        IN      Outline *paths,
                                        IN      short *sideboard,
                                        OUT     UBYTE **gpgm,
                                        OUT     USHORT *num,
                                        OUT     USHORT *stack,
                                        OUT     USHORT *twilight));

/***
** Function: BuildPreProgram
**
** Description:
**   This function builds the pre-program that will compute
**   the CVT and storage entries for the TT stem hint
**   instructions to work. 
***/
USHORT         BuildPreProgram   _ARGS((IN      struct T1Metrics *t1m,
                                        IN      WeightControl *weight,
                                        INOUT   Blues *blues,
                                        INOUT   AlignmentControl *align,
                                        INOUT   UBYTE **prep,
                                        IN      int prepsize,
                                        OUT     USHORT *maxprepstack));
/***
** Function: MatchingFamily
**
** Description:
**   Locate the family alignment zone that is closest to
**   a given alignment zone.
***/
short          MatchingFamily    _ARGS((IN      funit pos,
                                        IN      funit *family,
                                        IN      USHORT fcnt));
/***
** Function: GetRomanHints
**
** Description:
***/
const UBYTE    *GetRomanHints    _ARGS((OUT     int *size));


/***
** Function: GetSwissHints
**
** Description:
***/
const UBYTE    *GetSwissHints    _ARGS((OUT     int *size));


/***
** Function: GetFontProg
**
** Description:
**   Return the font program.
***/
const UBYTE    *GetFontProg      _ARGS((void));


/***
** Function: GetFontProgSize
**
** Description:
**   Return the size of the font program.
***/
const USHORT   GetFontProgSize   _ARGS((void));


/***
** Function: GetNumFuns
**
** Description:
**   Return the number of functions defined in
**   the font program.
***/
const USHORT   GetNumFuns        _ARGS((void));


