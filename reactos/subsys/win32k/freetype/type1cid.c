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

#include "cidparse.c"
#include "cidload.c"
#include "cidobjs.c"
#include "cidriver.c"
#include "cidgload.c"

#else

#include <cid/cidparse.c>
#include <cid/cidload.c>
#include <cid/cidobjs.c>
#include <cid/cidriver.c>
#include <cid/cidgload.c>

#endif


/* END */
