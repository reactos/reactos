
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/config.h"
#include "wine/exception.h"

#include <debug.h>

void __wine_spec_unimplemented_stub(const char *module, const char *function )
{
    ULONG_PTR args[2];

    args[0] = (ULONG_PTR)module;
    args[1] = (ULONG_PTR)function;
    RaiseException( EXCEPTION_WINE_STUB, EH_NONCONTINUABLE, 2, args );
}
