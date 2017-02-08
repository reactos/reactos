#define NDEBUG

#include "w32csr.h"
#include <debug.h>

CSR_API(CsrCreateDesktop)
{
    return STATUS_SUCCESS;
}

CSR_API(CsrShowDesktop)
{
    return STATUS_SUCCESS;
}

CSR_API(CsrHideDesktop)
{
    return STATUS_SUCCESS;
}

BOOL
FASTCALL DtbgIsDesktopVisible(VOID)
{
    return !((BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_ISCONSOLEMODE));
}

/* EOF */
