
#pragma once

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRTRESTRICT
#define _CRTRESTRICT
#endif

#ifndef DEFINED_localeinfo_struct
typedef struct localeinfo_struct
{
    pthreadlocinfo locinfo;
    pthreadmbcinfo mbcinfo;
} _locale_tstruct, *_locale_t;
#define DEFINED_localeinfo_struct 1
#endif

#ifdef __cplusplus
} // extern "C"
#endif
