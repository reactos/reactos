NPX_CONTEXT_FULL		EQU 00001000Fh	;/ Full context

ContextFlags equ CsContextFlags
ctx_env      equ CsFloatSave
ctx_stack    equ CsFloatSave+FpRegisterArea  ;need to change this puppy
ctx_Cr0NpxState equ CsFloatSave+FpCr0NpxState
ctx_SegGs    equ CsSegGs
ctx_SegFs    equ CsSegFs
ctx_SegEs    equ CsSegEs
ctx_SegDs    equ CsSegDs
ctx_RegEdi   equ CsEdi
ctx_RegEsi   equ CsEsi
ctx_RegEbp   equ CsEbp
ctx_RegEbx   equ CsEbx
ctx_RegEdx   equ CsEdx
ctx_RegEcx   equ CsEcx
ctx_RegEax   equ CsEax
ctx_RegEip   equ CsEip
ctx_SegCs    equ CsSegCs
ctx_EFlags   equ CsEflags
ctx_RegEsp   equ CsEsp
ctx_SegSs    equ CsSegSs

XCPT_FLOAT_INVALID_OPERATION    EQU STATUS_FLOAT_INVALID_OPERATION
XCPT_FLOAT_DENORMAL_OPERAND     EQU STATUS_FLOAT_DENORMAL_OPERAND
XCPT_FLOAT_DIVIDE_BY_ZERO       EQU STATUS_FLOAT_DIVIDE_BY_ZERO
XCPT_FLOAT_OVERFLOW             EQU STATUS_FLOAT_OVERFLOW
XCPT_FLOAT_UNDERFLOW            EQU STATUS_FLOAT_UNDERFLOW
XCPT_FLOAT_INEXACT_RESULT       EQU STATUS_FLOAT_INEXACT_RESULT
XCPT_FLOAT_STACK_CHECK          EQU STATUS_FLOAT_STACK_CHECK

ExceptionNum                equ ErExceptionCode
FHandlerFlags               equ ErExceptionFlags
NestedExceptionReportRecord equ ErExceptionRecord
ExceptionAddress            equ ErExceptionAddress
CParameters                 equ ErNumberParameters


Em87Busy    equ     1
Em87Idle    equ     0
