#define FT_FLAT_COMPILE

/***************************************************************************/
/*                                                                         */
/*  cff.c                                                                  */
/*                                                                         */
/*    FreeType OpenType driver component (body only).                      */
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

#include "t2driver.c"    /* driver interface     */
#include "t2parse.c"     /* token parser         */
#include "t2load.c"      /* tables loader        */
#include "t2objs.c"      /* object management    */
#include "t2gload.c"     /* glyph loader         */

#else

#include <cff/t2driver.c>    /* driver interface     */
#include <cff/t2parse.c>     /* token parser         */
#include <cff/t2load.c>      /* tables loader        */
#include <cff/t2objs.c>      /* object management    */
#include <cff/t2gload.c>     /* glyph loader         */

#endif


/* END */
