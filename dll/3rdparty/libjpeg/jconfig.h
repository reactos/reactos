#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR

#define HAVE_STDDEF_H
#define HAVE_STDLIB_H

#ifndef __RPCNDR_H__		/* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		/* prevent jmorecfg.h from redefining it */

#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS	/* we presume a 32-bit flat memory model */
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

typedef long INT32;
