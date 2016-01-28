
#include <asm.inc>

.code
.align 4

MACRO(DEFINE_THISCALL_WRAPPER, cxxname, stdcallname)
EXTERN &stdcallname:PROC
PUBLIC &cxxname
&cxxname:
    pop eax
    push ecx
    push eax
    jmp &stdcallname
ENDM

DEFINE_THISCALL_WRAPPER ??0CVssWriter@@QAE@XZ, _VSSAPI_CVssWriter_default_ctor@4
DEFINE_THISCALL_WRAPPER ??1CVssWriter@@UAE@XZ, _VSSAPI_CVssWriter_dtor@4
DEFINE_THISCALL_WRAPPER ?Initialize@CVssWriter@@QAGJU_GUID@@PBGW4VSS_USAGE_TYPE@@W4VSS_SOURCE_TYPE@@W4_VSS_APPLICATION_LEVEL@@KW4VSS_ALTERNATE_WRITER_STATE@@_N@Z, _VSSAPI_CVssWriter_Initialize@52
DEFINE_THISCALL_WRAPPER ?Subscribe@CVssWriter@@QAGJK@Z, _VSSAPI_CVssWriter_Subscribe@8
DEFINE_THISCALL_WRAPPER ?Unsubscribe@CVssWriter@@QAGJXZ, _VSSAPI_CVssWriter_Unsubscribe@4

END
