//***   libx.cpp -- 'source library' inclusions
// DESCRIPTION
//  there are some things that we share in source (rather than .obj or .dll)
// form.  this file builds them in the current directory.

#include "stdafx.h"
#pragma hdrstop

#ifdef DEBUG
#include "../lib/qistub.cpp"
#include "../lib/dbutil.cpp"
#endif
