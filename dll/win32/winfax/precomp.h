#include <windows.h>
#include <winfax.h>

/* INTERNAL *******************************************************************/

ULONG DbgPrint(PCH Format,...);
#define UNIMPLEMENTED \
  DbgPrint("%s:%i: %s() UNIMPLEMENTED!\n", __FILE__, __LINE__, __FUNCTION__); \
  SetLastError(	ERROR_CALL_NOT_IMPLEMENTED)

/* EOF */
