#define FT_FLAT_COMPILE

/***************************************************************************/
/*                                                                         */
/*  truetype.c                                                             */
/*                                                                         */
/*    FreeType TrueType driver component (body only).                      */
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

#include "ttdriver.c"    /* driver interface     */
#include "ttpload.c"     /* tables loader        */
#include "ttgload.c"     /* glyph loader         */
#include "ttobjs.c"      /* object manager       */

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#include "ttinterp.c"    /* bytecode interpreter */
#endif

#else /* FT_FLAT_COMPILE */

#include <truetype/ttdriver.c>    /* driver interface     */
#include <truetype/ttpload.c>     /* tables loader        */
#include <truetype/ttgload.c>     /* glyph loader         */
#include <truetype/ttobjs.c>      /* object manager       */

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#include <truetype/ttinterp.c>    /* bytecode interpreter */
#endif

#endif /* FT_FLAT_COMPILE */


/* END */
