/*
 * PROJECT:         MSVC runtime check support library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Provides support functions for MSVC runtime checks
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>
.code

EXTERN __RTC_Failure:PROC

/*

    This function is invoked like this:

        mov esi, esp
        // Do the actual function call
        cmp esp, esi
        call __RTC_CheckEsp

    http://stackoverflow.com/questions/3914750/hows-rtc-checkesp-implemented
*/
PUBLIC __RTC_CheckEsp
__RTC_CheckEsp:

    /* We check if the zero flag is set, and if it is, everything is fine
       and we return to the caller */
    je __RTC_CheckEsp_return

    push ebp
    mov ebp, esp
    pusha

    // void _RTC_Failure(void* retaddr, int errnum);
    push 0 // errnum
    push dword ptr [esp + 4] // retaddr
    call __RTC_Failure
    add esp, 8

    popa
    pop ebp

__RTC_CheckEsp_return:
    ret

END


