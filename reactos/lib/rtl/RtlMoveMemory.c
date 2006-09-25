#include <rtl.h>

#define NDEBUG
#include <debug.h>

#undef RtlMoveMemory
/*
 * @implemented
 */
VOID
NTAPI
RtlMoveMemory (
   PVOID    Destination,
   CONST VOID  * Source,
   ULONG    Length
)
{
   memmove (
      Destination,
      Source,
      Length
   );
}
