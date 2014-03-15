/***************************************************************************/
/*                                                                         */
/*  afscript.h                                                             */
/*                                                                         */
/*    Auto-fitter scripts (specification only).                            */
/*                                                                         */
/*  Copyright 2013 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /* The following part can be included multiple times. */
  /* Define `SCRIPT' as needed.                         */


  /* Add new scripts here. */

  SCRIPT( cyrl, CYRL, "Cyrillic" )
  SCRIPT( deva, DEVA, "Indic scripts" )
  SCRIPT( dflt, DFLT, "no script" )
  SCRIPT( grek, GREK, "Greek" )
  SCRIPT( hani, HANI, "CJKV ideographs" )
  SCRIPT( hebr, HEBR, "Hebrew" )
  SCRIPT( latn, LATN, "Latin" )
#ifdef FT_OPTION_AUTOFIT2
  SCRIPT( ltn2, LTN2, "Latin 2" )
#endif


/* END */
