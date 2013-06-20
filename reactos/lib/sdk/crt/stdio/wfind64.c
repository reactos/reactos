#define UNICODE
#define _UNICODE

#include <precomp.h>
#include <tchar.h>
#include <io.h>

// Generate _findfirst64 and _findnext64
#undef _wfindfirst
#define _wfindfirst _wfindfirst64
#undef _wfindnext
#define _wfindnext _wfindnext64
#undef _wfinddata_t
#define _wfinddata_t _wfinddata64_t

#include "findgen.c"
