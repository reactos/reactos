
#ifndef __WINE_DPLAYX_PRECOMP_H
#define __WINE_DPLAYX_PRECOMP_H

#include <wine/config.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS
#include "dplay_global.h"
#include "dplayx_global.h"
#include "dplayx_messages.h"
#include "name_server.h"

#include <winreg.h>
#include <wine/debug.h>
#include <wine/unicode.h>

#ifdef __REACTOS__
#include <strsafe.h>
/* wcsnlen is an NT6+ function. To cover all cases, use a private implementation */
static inline size_t hacked_wcsnlen(const wchar_t* str, size_t size)
{
    StringCchLengthW(str, size, &size);
    return size;
}
#define wcsnlen hacked_wcsnlen
#endif

#endif /* !__WINE_DPLAYX_PRECOMP_H */
