#include "internal.h"

int __lconv_init (void);

int mingw_initcharmax = 0;

int _charmax = 255;

_CRTALLOC(".CRT$XIC") _PIFV __mingw_pinit = __lconv_init;
