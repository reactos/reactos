.386p

;   forceres.asm - generate publics to force resolution of symbols we
;   don't want to deal with yet.
;


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        public __fltused

foo     proc    near        ; make the assembler shut up

__fltused:
            int     3
            ret

foo endp


_TEXT   ends
        end
