#ifndef __LIBSKY_H
#define __LIBSKY_H

#ifdef DEBUG
# ifdef NDEBUG
#  define DBG(...)
# else
#  define DBG DbgPrint
# endif
# define DBG1 DbgPrint
#else
# define DBG(...)
# define DBG1(...)
#endif
#define STUB DbgPrint("Stub in %s:%i: ", __FILE__, __LINE__); DbgPrint

#endif /* __LIBSKY_H */

/* EOF */
