#include <windows.h>
#include <WinSCard.h>

/* INTERNAL *******************************************************************/

ULONG DbgPrint(PCH Format,...);
#define UNIMPLEMENTED \
  DbgPrint("%s:%i: %s() UNIMPLEMENTED!\n", __FILE__, __LINE__, __FUNCTION__)

/* EOF */
