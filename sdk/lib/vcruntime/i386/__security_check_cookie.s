//
// __security_check_cookie.asm
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __security_check_cookie for x86.
//
// SPDX-License-Identifier: MIT
//

#include <asm.inc>

EXTERN ___security_cookie:QWORD
EXTERN ___report_gsfailure:PROC

.code

// This function must not clobber any registers!
PUBLIC ___security_check_cookie
___security_check_cookie:
    cmp ecx, dword ptr [___security_cookie]
    jne ___security_check_cookie_fail
    ret
___security_check_cookie_fail:
    jmp ___report_gsfailure

END
