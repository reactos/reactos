//***   libx.cpp -- 'source library' inclusions
// DESCRIPTION
//  there are some things that we share in source (rather than .obj or .dll)
// form.  this file builds them in the current directory.

#include "stdafx.h"
#pragma hdrstop

#include "../lib/uassist.cpp"       // 'safe' thunks and cache

#ifdef DEBUG
//#define TF_QISTUB ... see shellprv.h ...
#include "../lib/qistub.cpp"
#include "../lib/dbutil.cpp"
#endif
