//***   libx.cpp -- 'source library' inclusions
// DESCRIPTION
//  there are some things that we share in source (rather than .obj or .dll)
// form.  this file builds them in the current directory.

#include "priv.h"

#include "../lib/uassist.cpp"       // 'safe' thunks and cache

// We must include dka explicitly instead of using the one in the lib
// because the lib one did not #include <w95wraps.h>.

#include "../lib/dka.cpp"
#include "../lib/dbgmem.cpp"

#ifdef DEBUG
#define TF_QISTUB   TF_SHDLIFE
#include "../lib/qistub.cpp"
#include "../lib/dbutil.cpp"
#endif
