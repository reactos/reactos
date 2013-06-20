/* src/config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if inline functions a la GCC are available.  */
#undef HAVE_INLINE

/* Define if function attributes a la GCC 2.5 and higher are available.  */
#undef HAVE_GNUC25_ATTRIB

/* Define if constant functions a la GCC 2.5 and higher are available.  */
#undef HAVE_GNUC25_CONST

/* Define if nonreturning functions a la GCC 2.5 and higher are available.  */
#undef HAVE_GNUC25_NORETURN

/* Define if printf-format argument lists a la GCC are available.  */
#undef HAVE_GNUC25_PRINTFFORMAT

/* Define if we want to include rpc/types.h.  Crap BSDs put INADDR_LOOPBACK there. */
#undef HAVEUSE_RPCTYPES_H

/* Define if you have the poll function.  */
#undef HAVE_POLL

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the nsl library (-lnsl).  */
#undef HAVE_LIBNSL

/* Define if you have the socket library (-lsocket).  */
#undef HAVE_LIBSOCKET

/* Use the definitions: */

#ifndef HAVE_INLINE
#define inline
#endif

#ifdef HAVE_POLL
#include <sys/poll.h>
#else
/* kludge it up */
struct pollfd { int fd; short events; short revents; };
#define POLLIN  1
#define POLLPRI 2
#define POLLOUT 4
#endif

/* GNU C attributes. */
#ifndef FUNCATTR
#ifdef HAVE_GNUC25_ATTRIB
#define FUNCATTR(x) __attribute__(x)
#else
#define FUNCATTR(x)
#endif
#endif

/* GNU C printf formats, or null. */
#ifndef ATTRPRINTF
#ifdef HAVE_GNUC25_PRINTFFORMAT
#define ATTRPRINTF(si,tc) format(printf,si,tc)
#else
#define ATTRPRINTF(si,tc)
#endif
#endif
#ifndef PRINTFFORMAT
#define PRINTFFORMAT(si,tc) FUNCATTR((ATTRPRINTF(si,tc)))
#endif

/* GNU C nonreturning functions, or null. */
#ifndef ATTRNORETURN
#ifdef HAVE_GNUC25_NORETURN
#define ATTRNORETURN noreturn
#else
#define ATTRNORETURN
#endif
#endif
#ifndef NONRETURNING
#define NONRETURNING FUNCATTR((ATTRNORETURN))
#endif

/* Combination of both the above. */
#ifndef NONRETURNPRINTFFORMAT
#define NONRETURNPRINTFFORMAT(si,tc) FUNCATTR((ATTRPRINTF(si,tc),ATTRNORETURN))
#endif

/* GNU C constant functions, or null. */
#ifndef ATTRCONST
#ifdef HAVE_GNUC25_CONST
#define ATTRCONST const
#else
#define ATTRCONST
#endif
#endif
#ifndef CONSTANT
#define CONSTANT FUNCATTR((ATTRCONST))
#endif

#ifdef HAVEUSE_RPCTYPES_H
#include <rpc/types.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
