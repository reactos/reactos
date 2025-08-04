.code

PUBLIC get_fpu_cw_raw
get_fpu_cw_raw PROC
    fnstcw word ptr [rcx]
    stmxcsr dword ptr [rdx]
    ret
get_fpu_cw_raw ENDP

END
