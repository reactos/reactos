#define FT_FLAT_COMPILE

/***************************************************************************/
/*                                                                         */
/*  autohint.c                                                             */
/*                                                                         */
/*    Automatic Hinting wrapper (body only).                               */
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


#define FT_MAKE_OPTION_SINGLE_OBJECT

#ifdef FT_FLAT_COMPILE

#include "ahangles.c"
#include "ahglyph.c"
#include "ahglobal.c"
#include "ahhint.c"
#include "ahmodule.c"

#else

#include <autohint/ahangles.c>
#include <autohint/ahglyph.c>
#include <autohint/ahglobal.c>
#include <autohint/ahhint.c>
#include <autohint/ahmodule.c>

#endif


/* END */
