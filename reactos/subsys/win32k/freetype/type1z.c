#define FT_FLAT_COMPILE

/***************************************************************************/
/*                                                                         */
/*  type1z.c                                                               */
/*                                                                         */
/*    FreeType experimental Type 1 driver component (body only).           */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#define FT_MAKE_OPTION_SINGLE_OBJECT


#ifdef FT_FLAT_COMPILE

#include "z1parse.c"
#include "z1load.c"
#include "z1objs.c"
#include "z1driver.c"
#include "z1gload.c"

#ifndef Z1_CONFIG_OPTION_NO_AFM
#include "z1afm.c"
#endif

#else /* FT_FLAT_COMPILE */

#include <type1z/z1parse.c>
#include <type1z/z1load.c>
#include <type1z/z1objs.c>
#include <type1z/z1driver.c>
#include <type1z/z1gload.c>

#ifndef Z1_CONFIG_OPTION_NO_AFM
#include <type1z/z1afm.c>
#endif

#endif /* FT_FLAT_COMPILE */


/* END */
