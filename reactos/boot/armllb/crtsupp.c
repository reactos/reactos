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
    /* Write to the serial port */
//    LlbSerialPutChar(c);
    
    /* Write to the screen too */
    LlbVideoPutChar(c);
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

    va_start (args, fmt);

    /* For this to work, printbuffer must be larger than
     * anything we ever want to print.
     */
    i = vsprintf (printbuffer, fmt, args);
    va_end (args);

    /* Print the string */
    return puts(printbuffer);
}

/* EOF */
