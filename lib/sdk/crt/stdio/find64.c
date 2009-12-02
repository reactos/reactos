#include <precomp.h>
#include <tchar.h>
#include <io.h>

// Generate _findfirst64 and _findnext64
#undef _findfirst
#define _findfirst _findfirst64
#undef _findnext
#define _findnext _findnext64
#undef _finddata_t
#define _finddata_t __finddata64_t

#include "findgen.c"
