#define UNICODE
#define _UNICODE

#include <precomp.h>
#include <tchar.h>
#include <io.h>

// Generate _findfirsti64 and _findnexti64
#undef _wfindfirst
#define _wfindfirst _wfindfirsti64
#undef _wfindnext
#define _wfindnext _wfindnexti64
#undef _wfinddata_t
#define _wfinddata_t _wfinddatai64_t

#include "findgen.c"
