#ifndef FAST_ALLOC_H_
#define FAST_ALLOC_H_

/* Written by: Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   License: Public Domain (http://creativecommons.org/licenses/publicdomain/)
   Take all the code you want, I'll just write more.
*/

#ifdef _WIN32
#include <stddef.h> // size_t on windows
#else
#include <sys/types.h> // size_t
#endif

#ifdef __cplusplus
extern "C"
{
#endif

void *  fast_malloc(size_t size);
void    fast_free(void *p);
void *  fast_realloc(void *oldP, size_t size);

#ifdef __cplusplus
}
#endif

#endif
