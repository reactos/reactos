
#include <errno.h>

extern void * __pInvalidArgHandler;

void _invalid_parameter(
   const wchar_t * expression,
   const wchar_t * function,
   const wchar_t * file,
   unsigned int line,
   uintptr_t pReserved);

#ifndef _LIBCNT_
#define MSVCRT_INVALID_PMT(x,err)   (*_errno() = (err), _invalid_parameter(NULL, NULL, NULL, 0, 0))
#define MSVCRT_CHECK_PMT_ERR(x,err) ((x) || (MSVCRT_INVALID_PMT( 0, (err) ), 0))
#define MSVCRT_CHECK_PMT(x)         MSVCRT_CHECK_PMT_ERR((x), EINVAL)
#else
/* disable secure crt parameter checks */
#define MSVCRT_INVALID_PMT(x,err)
#define MSVCRT_CHECK_PMT_ERR(x,err)
#define MSVCRT_CHECK_PMT(x) (x)
#endif
