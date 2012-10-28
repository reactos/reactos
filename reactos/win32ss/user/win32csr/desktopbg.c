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
    HWND VisibleDesktopWindow = GetDesktopWindow(); // DESKTOPWNDPROC

    if (VisibleDesktopWindow != NULL &&
            !IsWindowVisible(VisibleDesktopWindow))
    {
        VisibleDesktopWindow = NULL;
    }

    return VisibleDesktopWindow != NULL;
}

/* EOF */
