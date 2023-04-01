
#include <asm.inc>

.code64

// void __fastfail(unsigned int Code<rcx>);
PUBLIC __fastfail
__fastfail:
    // mov ecx, [rsp + 4]
    int HEX(29)

// void __lgdt(void *Source<rcx>);
PUBLIC __lgdt
__lgdt:
#ifdef _USE_ML
    lgdt fword ptr [rcx]
#else
    lgdt cs:[rcx]
#endif
    ret

// void __ltr(unsigned short Source<rcx>);
PUBLIC __ltr
__ltr:
    ltr cx
    ret

// void _sgdt(void *Destination<rcx>);
PUBLIC __sgdt
__sgdt:
#ifdef _USE_ML
    sgdt fword ptr [rcx]
#else
    sgdt cs:[rcx]
#endif
    ret

END
