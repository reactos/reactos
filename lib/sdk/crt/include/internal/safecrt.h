
#include <errno.h>

extern void * __pInvalidArgHandler;

void _invalid_parameter(
   const wchar_t * expression,
   const wchar_t * function, 
   const wchar_t * file, 
   unsigned int line,
   uintptr_t pReserved);

#ifndef _LIBCNT_
#define MSVCRT_INVALID_PMT(x) _invalid_parameter(NULL, NULL, NULL, 0, 0)
#define MSVCRT_CHECK_PMT(x)   ((x) || (MSVCRT_INVALID_PMT(0),0))
#else
/* disable secure crt parameter checks */
#define MSVCRT_CHECK_PMT
#define MSVCRT_INVALID_PMT
#endif
