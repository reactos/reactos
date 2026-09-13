//
// __security_check_cookie.asm
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __security_check_cookie for x64.
//
// SPDX-License-Identifier: MIT
//

#include <asm.inc>

EXTERN __security_cookie:QWORD
EXTERN __report_gsfailure:PROC

.code64

// This function must not clobber any registers!
PUBLIC __security_check_cookie
__security_check_cookie:
    cmp rcx, qword ptr __security_cookie[rip]
    jne __security_check_cookie_fail
    ret
__security_check_cookie_fail:
    jmp __report_gsfailure

END
