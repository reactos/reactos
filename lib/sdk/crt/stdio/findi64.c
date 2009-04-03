#include <precomp.h>
#include <tchar.h>
#include <io.h>

// Generate _findfirsti64 and _findnexti64
#undef _findfirst
#define _findfirst _findfirsti64
#undef _findnext
#define _findnext _findnexti64
#undef _finddata_t
#define _finddata_t _finddatai64_t

#include "findgen.c"
