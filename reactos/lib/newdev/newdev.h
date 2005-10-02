#include <windows.h>
#include <setupapi.h>

ULONG DbgPrint(PCH Format,...);
#define UNIMPLEMENTED \
  DbgPrint("NEWDEV:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)
#define DPRINT1 DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
