/***
**
**   Module: Trans
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      contains functions that will convert T1 specific data into
**      corresponding TT data, such as hints and font metrics.
**
**   Author: Michael Jansson
**
**   Created: 5/28/93
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


struct TTMetrics;
struct T1Metrics;
struct TTHandle;
struct T1Glyph;
struct TTGlyph;
struct TTComposite;
struct Composite;


/***
** Function: ConvertComposite
**
** Description:
**   This function convertes the data associated to a T1 font seac glyph
**   into the corresponding data used in a TT font composite glyph.
**
***/
errcode FASTCALL  ConvertComposite  _ARGS((INOUT   struct T1Metrics *,
                                           IN      struct Composite *,
                                           OUT     struct TTComposite *));
/***
** Function: ConvertGlyph
**
** Description:
**   This function convertes the data associated to a T1 font glyph
**   into the corresponding data used in a TT font glyph.
***/
errcode FASTCALL  ConvertGlyph      _ARGS((INOUT   struct T1Metrics *,
                                           IN      struct T1Glyph *,
                                           OUT     struct TTGlyph **,
                                           IN      int));
/***
** Function: ConvertMetrics
**
** Description:
**
***/
errcode FASTCALL  ConvertMetrics    _ARGS((IN      struct TTHandle *,
                                           INOUT   struct T1Metrics *,
                                           OUT     struct TTMetrics *,
                                           IN      char *tag));

/***
** Function: TransX
**
** Description:
**   Translate a horizontal coordinate according to a transformation matrix.
***/
funit FASTCALL    TransX            _ARGS((IN      struct T1Metrics *t1,
                                           IN      funit x));

/***
** Function: TransY
**
** Description:
**   Translate a vertical coordinate according to a transformation matrix.
***/
funit FASTCALL    TransY            _ARGS((IN      struct T1Metrics *t1,
                                           IN      funit y));

/***
** Function: TransAllPoints
**
** Description:
**   Translate a coordinate according to a transformation matrix.
***/
void  FASTCALL    TransAllPoints    _ARGS((IN      struct T1Metrics *t1,
                                           INOUT   Point *pts,
                                           IN      USHORT cnt,
                                           IN      f16d16 *fmatrix));
