/*
 * PROJECT:     ReactOS dwrite DLL
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Glue functions between FreeType
 * COPYRIGHT:   Copyright 2026 Mikhail Tyukin <mishakeys20@gmail.com>
 */

#include <dwrite_private.h>

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

/* print a message */
void
FT_Message(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    WARN((PCHAR)format, va);
    va_end(va);
}

/* print a message and exit */
void
FT_Panic(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    ERR((PCHAR)format, va);
    va_end(va);
}
