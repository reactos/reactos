//***   libx.cpp -- 'source library' inclusions
// DESCRIPTION
//  there are some things that we share in source (rather than .obj or .dll)
// form.  this file builds them in the current directory.

#include "priv.h"

#include "../lib/uassist.cpp"       // 'safe' thunks and cache
#include "../lib/dbgmem.cpp"

#ifdef DEBUG
#define TF_QISTUB   TF_SHDLIFE

// BugBug: qistub.cpp uses kernel string functions and unsafe buffer functions.

#undef wsprintf
#undef lstrcpyn

#undef  wsprintfW
#define wsprintf    wsprintfW
#undef  lstrcpynW
#define lstrcpyn    lstrcpynW

#include "../lib/qistub.cpp"

#undef  wsprintf
#define wsprintf    Do_not_use_wsprintf_use_wnsprintf
#undef  lstrcpyn
#define lstrcpyn    Do_not_use_lstrcpyn_use_StrCpyN

#define lstrcpynW   Do_not_use_lstrcpynW_use_StrCpyNW
#define wsprintfW   Do_not_use_wsprintfW_use_wnsprintfW


#include "../lib/dbutil.cpp"
#endif
