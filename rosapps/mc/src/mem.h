#ifndef __MEM_H
#define __MEM_H

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#   include <string.h>
    /* An ANSI string.h and pre-ANSI memory.h might conflict */
#   if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#       include <memory.h>
#   endif /* not STDC_HEADERS and HAVE_MEMORY_H */

#   ifndef index
#define index strchr
#   endif

#   ifndef rindex 
#       define rindex strrchr
#   endif

#   define bcopy(s,d,n) memcpy ((d), (s), (n))
#   define bcmp(s1,s2,n) memcmp ((s1), (s2), (n))
#   define bzero(s,n) memset ((s), 0, (n))

#else /* not STDC_HEADERS and not HAVE_STRING_H */
#   include <strings.h>
    /* memory and strings.h conflict on other systems */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */
#endif 	/* __MEM_H */

