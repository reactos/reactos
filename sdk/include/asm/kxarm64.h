
#define ENABLE_FRAME_POINTER 1

#undef TRUE
//#define TRUE 1
#undef FALSE
//#define FALSE 0

//#include "kxarmunw.h"

#ifdef _MSC_VER

    /* Globals */
    GBLS __FuncStartLabel
    GBLS __FuncEpilog1StartLabel
    GBLS __FuncEpilog2StartLabel
    GBLS __FuncEpilog3StartLabel
    GBLS __FuncEpilog4StartLabel
    GBLS __FuncXDataLabel
    GBLS __FuncXDataPrologLabel
    GBLS __FuncXDataEpilog1Label
    GBLS __FuncXDataEpilog2Label
    GBLS __FuncXDataEpilog3Label
    GBLS __FuncXDataEpilog4Label
    GBLS __FuncXDataEndLabel
    GBLS __FuncEndLabel
    GBLS __FuncArea
    GBLS __FuncExceptionHandler

    MACRO
    __DeriveFunctionLabels $FuncName
__FuncStartLabel        SETS "|$FuncName|"
__FuncEndLabel          SETS "|$FuncName._end|"
__FuncEpilog1StartLabel SETS "|$FuncName._epilog1_start|"
__FuncEpilog2StartLabel SETS "|$FuncName._epilog2_start|"
__FuncEpilog3StartLabel SETS "|$FuncName._epilog3_start|"
__FuncEpilog4StartLabel SETS "|$FuncName._epilog4_start|"
__FuncXDataLabel        SETS "|$FuncName._xdata|"
__FuncXDataPrologLabel  SETS "|$FuncName._xdata_prolog|"
__FuncXDataEpilog1Label SETS "|$FuncName._xdata_epilog1|"
__FuncXDataEpilog2Label SETS "|$FuncName._xdata_epilog2|"
__FuncXDataEpilog3Label SETS "|$FuncName._xdata_epilog3|"
__FuncXDataEpilog4Label SETS "|$FuncName._xdata_epilog4|"
__FuncXDataEndLabel     SETS "|$FuncName._xdata_end|"
    MEND

    MACRO
    __ExportName $FuncName
        LCLS Name
Name    SETS "|$FuncName|"
        ALIGN 4
        EXPORT $Name
$Name
    MEND

    MACRO
    __ExportProc $FuncName
        LCLS Name
Name    SETS "|$FuncName|"
        ALIGN 4
        EXPORT $Name
$Name   PROC
    MEND

    MACRO
    TEXTAREA
#if defined(_CONTROL_FLOW_GUARD)
        AREA |.text|,ALIGN=4,CODE,READONLY
#else
        AREA |.text|,ALIGN=2,CODE,READONLY
#endif
    MEND

    MACRO
    DATAAREA
        AREA |.data|,DATA
    MEND

    MACRO
    RODATAAREA
        AREA |.rdata|,DATA,READONLY
    MEND

    MACRO
    NESTED_ENTRY $FuncName, $AreaName, $ExceptHandler
        __DeriveFunctionLabels $FuncName
__FuncArea SETS "|.text|"
        IF "$AreaName" != ""
__FuncArea SETS "$AreaName"
        ENDIF
__FuncExceptionHandler SETS ""
        IF "$ExceptHandler" != ""
__FuncExceptionHandler SETS "|$ExceptHandler|"
        ENDIF
        AREA $__FuncArea,CODE,READONLY
        __ExportProc $FuncName
        ROUT
        // __ResetUnwindState
    MEND

    // FIXME: this does not exist in native
    MACRO
    PROLOG_END
        /* Ignore for now */
    MEND

    MACRO
    NESTED_END $FuncName
$__FuncEndLabel
        LTORG
        ENDP
        //AREA |.pdata|,ALIGN=2,READONLY
        //DCD $__FuncStartLabel
        //RELOC 2
        //DCD $__FuncXDataLabel
        //RELOC 2
        //__EmitUnwindXData
        //AREA $__FuncArea,CODE,READONLY
__FuncStartLabel SETS ""
__FuncEndLabel SETS ""
    MEND

    MACRO
    LEAF_ENTRY $FuncName, $AreaName
        NESTED_ENTRY $FuncName, $AreaName
    MEND

    MACRO
    LEAF_END $FuncName
        NESTED_END $FuncName
    MEND

    MACRO
    LEAF_ENTRY_NO_PDATA $FuncName, $AreaName
        __DeriveFunctionLabels $FuncName
__FuncArea SETS "|.text|"
        IF "$AreaName" != ""
__FuncArea SETS "$AreaName"
        ENDIF
        AREA $__FuncArea,CODE,READONLY
        __ExportProc $FuncName
        ROUT
    MEND

    MACRO
    LEAF_END_NO_PDATA $FuncName
$__FuncEndLabel
        LTORG
        ENDP
__FuncStartLabel SETS ""
__FuncEndLabel SETS ""
    MEND

    MACRO
    ALTERNATE_ENTRY $FuncName
        __ExportName $FuncName
        ROUT
    MEND


    #define CR 13
    #define LF 10
    #define NUL 0

    #define ASCII dcb

    MACRO
    UNIMPLEMENTED $Name
    MEND

#else

/* Compatibility define */
#define EQU .equ

.macro IMPORT Name
    /* Ignore */
.endm

.macro EXPORT Name
    .global &Name
.endm

.macro TEXTAREA
    .section .text, "rx"
#if defined(_CONTROL_FLOW_GUARD)
    .align 4
#else
    .align 2
#endif
.endm

.macro DATAAREA
    .section .data, "rw"
.endm

.macro RODATAAREA
    .section .rdata, "rw"
.endm

.macro NESTED_ENTRY Name
FuncName    .equ    &Name
PrologName  .equ    &Name&_Prolog
FuncEndName .equ    &Name&_end
    .global &FuncName
    .align 2
    .func &FuncName
    &FuncName:
.endm

// FIXME: should go to kxarmunw.h
.macro PROLOG_END
    \PrologName:
.endm

.macro NESTED_END Name
    &FuncEndName:
   .endfunc
.endm

.macro LEAF_ENTRY $FuncName, $AreaName
    NESTED_ENTRY $FuncName, $AreaName
.endm

.macro LEAF_END $FuncName
    NESTED_END $FuncName
.endm


/* Some "intrinsics", see http://codemachine.com/article_armasm.html */

.macro __debugbreak
    DCD 0xDEFE
.endm

.macro __assertfail
    DCD 0xDEFC
.endm

.macro __fastfail
    DCD 0xDEFB
.endm

.macro __rdpmccntr64
    DCD 0xDEFA
.endm

.macro __debugservice
    DCD 0xDEFD
.endm

.macro __brkdiv0
    DCD 0xDEF9
.endm


#endif


