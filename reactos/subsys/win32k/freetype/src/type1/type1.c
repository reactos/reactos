/***************************************************************************/
/*                                                                         */
/*  type1.c                                                                */
/*                                                                         */
/*    FreeType Type 1 driver component (body only).                        */
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

#include "t1driver.c"
#include "t1objs.c"
#include "t1load.c"
#include "t1gload.c"
#include "t1tokens.c"
#include "t1parse.c"

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
#include "t1hinter.c"
#endif

#ifndef T1_CONFIG_OPTION_NO_AFM
#include "t1afm.c"
#endif

#else /* FT_FLAT_COMPILE */

#include <freetype/src/type1/t1driver.c>
#include <freetype/src/type1/t1objs.c>
#include <freetype/src/type1/t1load.c>
#include <freetype/src/type1/t1gload.c>
#include <freetype/src/type1/t1tokens.c>
#include <freetype/src/type1/t1parse.c>

#ifndef T1_CONFIG_OPTION_DISABLE_HINTER
#include <freetype/src/type1/t1hinter.c>
#endif

#ifndef T1_CONFIG_OPTION_NO_AFM
#include <freetype/src/type1/t1afm.c>
#endif

#endif /* FT_FLAT_COMPILE */


/* END */
