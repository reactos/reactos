/***************************************************************************/
/*                                                                         */
/*  aflatin2.h                                                             */
/*                                                                         */
/*    Auto-fitter hinting routines for latin script (specification).       */
/*                                                                         */
/*  Copyright 2003-2007, 2012, 2013 by                                     */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __AFLATIN2_H__
#define __AFLATIN2_H__

#include "afhints.h"


FT_BEGIN_HEADER


  /* the `latin' writing system */

  AF_DECLARE_WRITING_SYSTEM_CLASS( af_latin2_writing_system_class )


  /* the latin-specific script classes */

  AF_DECLARE_SCRIPT_CLASS( af_ltn2_script_class )  /* XXX */
#if 0
  AF_DECLARE_SCRIPT_CLASS( af_arm2_script_class )
  AF_DECLARE_SCRIPT_CLASS( af_cyr2_script_class )
  AF_DECLARE_SCRIPT_CLASS( af_grk2_script_class )
  AF_DECLARE_SCRIPT_CLASS( af_hbr2_script_class )
#endif


/* */

FT_END_HEADER

#endif /* __AFLATIN_H__ */


/* END */
