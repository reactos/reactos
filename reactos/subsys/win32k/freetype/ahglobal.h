/***************************************************************************/
/*                                                                         */
/*  ahglobal.h                                                             */
/*                                                                         */
/*    Routines used to compute global metrics automatically                */
/*    (specification).                                                     */
/*                                                                         */
/*  Copyright 2000 Catharon Productions Inc.                               */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


#ifndef AHGLOBAL_H
#define AHGLOBAL_H

#ifdef FT_FLAT_COMPILE

#include "ahtypes.h"

#else

#include <autohint/ahtypes.h>

#endif


#include <freetype/internal/ftobjs.h>  /* for LOCAL_DEF/LOCAL_FUNC */


#define AH_IS_TOP_BLUE( b )  ( (b) == ah_blue_capital_top || \
                               (b) == ah_blue_small_top   )


  /* compute global metrics automatically */
  LOCAL_DEF
  FT_Error  ah_hinter_compute_globals( AH_Hinter*  hinter );


#endif /* AHGLOBAL_H */


/* END */
