#ifndef _WIN32K_CLIPBOARD_H
#define _WIN32K_CLIPBOARD_H

#include <windows.h>

UINT FASTCALL
IntEnumClipboardFormats(UINT format);

#endif /* _WIN32K_CLIPBOARD_H */
