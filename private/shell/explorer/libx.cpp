//***   libx.cpp -- 'source library' inclusions
// DESCRIPTION
//  there are some things that we share in source (rather than .obj or .dll)
// form.  this file builds them in the current directory.

#include "cabinet.h"
#include <shguidp.h>

#include "../lib/uassist.cpp"       // 'safe' thunks and cache
