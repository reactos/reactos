/***
**
**   Module: T1Parser
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      will extract information from a T1 font file, by parsing
**      the data/commands found in PFB, PFM and AFM files.
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



struct T1Arg;
struct T1Info;
struct T1Glyph;
struct T1Handle;
struct T1Metrics;
struct Composite;
struct GlyphFilter;



/***
** Function: InitT1Input
**
** Description:
**   Allocate and initiate a handle for a T1 font file, including
**   extracting data from the font prolog that is needed to
**   read the glyphs, such as /FontMatrix, /Subrs and /lenIV.
***/
errcode           InitT1Input       _ARGS((IN      struct T1Arg *,
                                           OUT     struct T1Handle **,
                                           OUT     struct T1Metrics **,
                                           IN      short (*cb)(IN char *,
                                                               IN char *,
                                                               IN char *)));
/***
** Function: CleanUpT1
**
** Description:
**   Free the resources allocated for the T1 handle.
***/
errcode           CleanUpT1         _ARGS((INOUT   struct T1Handle *));


/***
** Function: ReadOtherMetrics
**
** Description:
**   Return font level information about the T1 font (mostly
**   metrics).
***/
errcode           ReadOtherMetrics  _ARGS((INOUT   struct T1Metrics *,
                                           IN      char *metrics));

/***
** Function: GetT1Glyph
**
** Description:
**   The current file position of the T1 font file must be
**   at the begining of an entry in the /CharStrings dictionary.
**   The function will decode the font commands, parse them, and
**   finally build a representation of the glyph.
***/
errcode           GetT1Glyph        _ARGS((INOUT   struct T1Handle *,
                                           OUT     struct T1Glyph *,
                                           IN      struct GlyphFilter *));
/***
** Function: FreeT1Glyph
**
** Description:
**   This function frees the memory used to represent
**   a glyph that has been translated.
***/
void              FreeT1Glyph       _ARGS((INOUT   struct T1Glyph *));


/***
** Function: GetT1Composite
**
** Description:
**   This function unlinks the first composite glyph
**   from the list of recorded composite glyphs, which
**   is returned to the caller.
***/
struct Composite  *GetT1Composite   _ARGS((INOUT   struct T1Handle *));


/***
** Function: GetT1AccentGlyph
**
** Description:
**   This function parses the charstring code associated to the
**   accent character of a composite character, if that glyph
**   is not already converted.
***/
errcode           GetT1AccentGlyph  _ARGS((INOUT   struct T1Handle *,
                                           IN      struct Composite *,
                                           OUT     struct T1Glyph *));
/***
** Function: GetT1BaseGlyph
**
** Description:
**   This function parses the charstring code associated to the
**   base character of a composite character, if that glyph
**   is not already converted.
***/
errcode           GetT1BaseGlyph    _ARGS((INOUT   struct T1Handle *,
                                           IN      struct Composite *,
                                           OUT     struct T1Glyph *));
/***
** Function: FlushWorkspace
**
** Description:
**   Free the resources allocated for the T1 handle.
***/
void              FlushWorkspace    _ARGS((INOUT   struct T1Handle *t1));
