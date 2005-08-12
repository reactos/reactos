#ifndef __LIBSKY_H
#define __LIBSKY_H

ULONG DbgPrint(PCH Format,...);

#if defined(DBG)
#undef DBG
#endif

#define DBG DPRINT
#define STUB DbgPrint("Stub in %s:%i: ", __FILE__, __LINE__); DbgPrint

#endif /* __LIBSKY_H */

/* EOF */
