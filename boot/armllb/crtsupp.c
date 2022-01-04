/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/crtsupp.c
 * PURPOSE:         CRT Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

int
putchar(int c)
{
    /* Write to the screen */
    LlbVideoPutChar(c);

    /* For DEBUGGING ONLY */
    LlbSerialPutChar(c);
    return 0;
}

int
puts(const char* string)
{
    while (*string) putchar(*string++);
    return 0;
}

int printf(const char *fmt, ...)
{
    va_list args;
    unsigned int i;
    char printbuffer[1024];

    va_start(args, fmt);
    i = vsprintf(printbuffer, fmt, args);
    va_end(args);

    /* Print the string */
    return puts(printbuffer);
}

ULONG
DbgPrint(const char *fmt, ...)
{
    va_list args;
    unsigned int i, j;
    char Buffer[1024];

    va_start(args, fmt);
    i = vsprintf(Buffer, fmt, args);
    va_end(args);

    for (j = 0; j < i; j++) LlbSerialPutChar(Buffer[j]);
    return 0;
}

/* EOF */
