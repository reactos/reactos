#ifndef __SHARED_INCLUDED__
#define __SHARED_INCLUDED__

#ifndef precondition
#define precondition(x) assert(x)
#define postcondition(x) assert(x)
#endif

#ifndef self
#define self (*this)
#endif

#ifndef traceOnly
#define traceOnly(x)
#endif

#ifndef debug
# if defined(NDEBUG)
#define debug(x)
# else
#define debug(x) x
# endif
#endif

typedef unsigned short  HASH;
typedef unsigned long   LHASH;

#endif
