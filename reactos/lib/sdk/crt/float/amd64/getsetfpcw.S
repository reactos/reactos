
#include <asm.inc>

.code64

PUBLIC __getfpcw87
FUNC __getfpcw87
    sub rsp, 8
    .ENDPROLOG
    stmxcsr [rsp]
    mov rax, [rsp]
    add rsp, 8
    ret
ENDFUNC __getfpcw87

PUBLIC __setfpcw87
FUNC __setfpcw87
    mov qword ptr [rsp + 8], rcx
    .ENDPROLOG
    ldmxcsr [rsp + 8]
    ret
ENDFUNC __setfpcw87

END
