#ifndef _SNDREC32_PCH_
#define _SNDREC32_PCH_

//#include "targetver.h"

#ifdef _UNICODE
#define _sntprintf_s _snwprintf_s
#else
#define _sntprintf_s _snprintf_s
#endif

#include <stdlib.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>

#endif /* _SNDREC32_PCH_ */
