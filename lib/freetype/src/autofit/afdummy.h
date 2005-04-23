#ifndef __AFDUMMY_H__
#define __AFDUMMY_H__

#include "aftypes.h"

FT_BEGIN_HEADER

 /* a dummy script metrics class used when no hinting should
  * be performed. This is the default for non-latin glyphs !
  */

  FT_LOCAL( const AF_ScriptClassRec )    af_dummy_script_class;

/* */

FT_END_HEADER

#endif /* __AFDUMMY_H__ */
