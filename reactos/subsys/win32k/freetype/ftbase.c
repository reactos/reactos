#define FT_FLAT_COMPILE

/***************************************************************************/
/*                                                                         */
/*  ftbase.c                                                               */
/*                                                                         */
/*    Single object library component (body only).                         */
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


#ifdef FT_FLAT_COMPILE

#include "ftcalc.c"
#include "ftobjs.c"
#include "ftstream.c"
#include "ftlist.c"
#include "ftoutln.c"
#include "ftextend.c"
#include "ftnames.c"

#else /* FT_FLAT_COMPILE */

#include <base/ftcalc.c>
#include <base/ftobjs.c>
#include <base/ftstream.c>
#include <base/ftlist.c>
#include <base/ftoutln.c>
#include <base/ftextend.c>
#include <base/ftnames.c>

#endif /* FT_FLAT_COMPILE */


/* END */
