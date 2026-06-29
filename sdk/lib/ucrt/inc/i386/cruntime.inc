;***
;cruntime.inc - multi-model assembly macros for interfacing to HLLs
;
;       Copyright (c) Microsoft Corporation.  All rights reserved.
;
;Purpose:
;       This file defines the current memory model being used.
;
;*******************************************************************************

;==============================================================================
;
;Use the following defines to control processor/segment model
;
;   default is -DI86 -Dmem_S
;
;==============================================================================
;
;The following variables are defined by this file:
;   cpu                         86, 286, or 386
;   mmodel                      english name of the memory model, i.e. "Medium"
;   ISIZE, LSIZE, NSIZE         size of ints, longs, shorts
;   FLTSIZE, DBLSIZE, LDBLSIZE  size of float, double, long double
;
;The following macros allow easy writing of combined 16/32 bit code:
;
; 16/32 bit registers:
;   rax, rbx, rcx, rdx,         expand to native registers (rax = eax or ax)
;   rsi, rdi, rsp, rbp
;   CBI                         convert byte to int (al to rax)
; Numeric type instructions:
;   IWORD, LWORD, SWORD         data type of int, long, short
;   DFLOAT, DDOUBLE, DLDOUBLE   define float, double, long double
;
;The following utility macros are provided:
;   codeseg                     define/declare code segment
;   error <msg>                 stop assembly with message
;   display <msg>               display a message, unless QUIET defined
;   _if cond <instruction>      assemble instruction only if cond is TRUE
;   _ife cond <instruction>     assemble instruction only if cond is FALSE
;   _ifd symbol <instruction>   assemble instruction only if symbol defined
;   _ifnd symbol <instruction>  assemble instruction only if symbol not defined
;
;   lab  LabelName              assembles to "LabelName:" If DEBUG is defined
;                               LabelName is made public
;
;   JS* (ex. JSE,JSZ,JSB ...)   assemble to "je short","jz short","jb short"
;
;   Cmacro look alikes
;   static* Name, InitialValue, Repeat   defines a static variable of type *
;   global* Name, InitialValue, Repeat   defines a global variable of type *
;   label*  Name, {PUBLIC,PASCAL,C}      defines a label of type *
;
;==============================================================================

; error <msg>   -    Output message and generate error

error   MACRO   msg
if2                     ;; only on pass 2 can we generate errors
        %out    **********************************************************
        %out    *** E r r o r  --  msg
        %out    **********************************************************
        .err
endif
        ENDM

; display msg   -    Output message unless QUIET defined

display MACRO   msg
ifndef QUIET            ;; only when quiet flag not set
if1                     ;; and on pass 1, then display message
        %out msg
endif
endif
        ENDM

; One line conditionals:
;   here we create the capability of writing code lines like
;
; _if sizeD   <push ds>   as opposed to    if sizeD
;                                              push  ds
;                                          endif

_if     MACRO   cond,text
    if  cond
        text
    endif
        ENDM

_ife    MACRO   cond,text
    ife cond
        text
    endif
        ENDM

_ifd    MACRO   cond,text
    ifdef   cond
        text
    endif
        ENDM

_ifnd   MACRO   cond,text
    ifndef  cond
        text
    endif
        ENDM

; Process processor arguments

        .686

;  Set memory model

        .model  flat, C

; Define registers:
; Instead of using the "word" registers directly, we will use a set of
; text equates.  This will allow you to use the native word size instead of
; hard coded to 16 bit words.  We also have some instruction equates for
; instruction with the register type hard coded in.

    rax equ <eax>
    rbx equ <ebx>
    rcx equ <ecx>
    rdx equ <edx>
    rdi equ <edi>
    rsi equ <esi>
    rbp equ <ebp>
    rsp equ <esp>

    CBI   equ <movsx eax, al>    ; convert byte to int (al to rax)

; The next set of equates deals with the size of SHORTS, INTS, LONGS, and
; pointers.

    ; parameters and locals
    IWORD   equ <dword>

    ; sizes for fixing SP, stepping through tables, etc.
    ISIZE   equ 4

; Float/double definitions
; (currently the same for 16- and 32-bit segments)

FLTSIZE  equ    4       ; float
DBLSIZE  equ    8       ; double
LDBLSIZE equ    10      ; long double

DFLOAT   equ    <dd>
DDOUBLE  equ    <dq>
DLDOUBLE equ    <dt>

; codeseg - Define/declare the standard code segment. Maps to the proper
; form of the .code directive.
;
; Input:
;
; Output:
;       .code _TEXT     ; for large code models
;       .code           ; for small code models
;       assume  cs:FLAT ; for 386
;       assume  ds:FLAT ; for 386
;       assume  es:FLAT ; for 386
;       assume  ss:FLAT ; for 386
;

codeseg MACRO

        .code

        assume  ds:FLAT
        assume  es:FLAT
        assume  ss:FLAT

        ENDM

; Define named constants for ISA levels.

__ISA_AVAILABLE_X86     equ 0
__ISA_AVAILABLE_SSE2    equ 1
__ISA_AVAILABLE_SSE42   equ 2
__ISA_AVAILABLE_AVX     equ 3

; Define named constants for favor

__FAVOR_ATOM           equ 0
__FAVOR_ENFSTRG        equ 1

;***************************************************************
;*
;*  Debug lab macro
;*
;***************************************************************

lab     macro name
ifdef   DEBUG
    public  pascal name     ;; define label public for Symdeb
endif
name:
        endm


;***************************************************************
;*
;*  Conditional jump short macros
;*
;***************************************************************


        irp     x,<Z,NZ,E,NE,P,PE,AE,BE,G>
JS&x    equ   <j&x short>
        endm


;***************************************************************
;*
;*  Global data definition macros
;*
;*  Usage:
;*      globalI   Name, InitialValue, Repeat
;*
;***************************************************************


MakeGlobal  macro   suffix, DataType        ;; makes all of the global* macros

global&suffix  macro   name, data, rep
public  name
ifb     <rep>
    _repeat = 1
else
    _repeat = (rep)
endif

name    &DataType  _repeat dup( data )
        endm

        endm


    MakeGlobal  T, dt                   ; globalT
    MakeGlobal  Q, dq                   ; globalQ
    MakeGlobal  D, dd                   ; globalD
    MakeGlobal  W, dw                   ; globalW
    MakeGlobal  B, db                   ; globalB

;***************************************************************
;*
;*  Static data definition macros
;*
;*  Usage:
;*      staticI   Name, InitialValue, Repeat
;*
;***************************************************************


MakeStatic  macro   suffix, DataType        ;; makes all of the static* macros

static&suffix  macro   name, data, rep

ifdef  DEBUG
    public  pascal name                     ;; make statics public if DEBUG
endif

ifb     <rep>
    _repeat = 1
else
    _repeat = (rep)
endif

name    &DataType  _repeat dup( data )
        endm

        endm


    MakeStatic  T, dt                   ; staticT
    MakeStatic  Q, dq                   ; staticQ
    MakeStatic  D, dd                   ; staticD
    MakeStatic  W, dw                   ; staticW
    MakeStatic  B, db                   ; staticB

;***************************************************************
;*
;*  Label definition macros
;*
;*  Usage:
;*      labelI   Name, {PUBLIC, PASCAL, C}
;*
;***************************************************************

__MakePublic    macro   name, option    ;; decides if a label should be
ifidni  <option>, <PUBLIC>              ;; made public
    public  name
elseifidni  <option>, <PASCAL>
    public  pascal name
elseifidni  <option>, <C>
    public  C name
elseifb  <option>
    ifdef  DEBUG
        public  pascal name     ;; make public if DEBUG
    endif
endif
                endm


MakeLabel   macro suffix, LabelType     ;; makes all of the label* macros

%@CatStr(<label>,<suffix>)      macro   name, option
        __MakePublic    <name>,<option>
name    label   &LabelType
        endm

        endm


        MakeLabel   T, tbyte    ; make labelT
        MakeLabel   Q, qword    ; make labelQ
        MakeLabel   D, dword    ; make labelD
        MakeLabel   W, word     ; make labelW
        MakeLabel   B, byte     ; make labelB

        MakeLabel   NP, near    ; make labelNP
