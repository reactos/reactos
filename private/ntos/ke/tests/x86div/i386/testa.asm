;       Static Name Aliases
;
        TITLE   testa.asm
        NAME    testa

        .386p
        include callconv.inc

            EXTRNP      _DivMarker,0

_DATA   SEGMENT DWORD USE32 PUBLIC 'DATA'

        public  _DivOperand, _DivRegPointer, _DivRegScaler, _ExceptEip, _ExceptEsp
_DivOperand         dd      ?
_DivRegPointer      dd      ?
_DivRegScaler       dd      ?
_ExceptEip          dd      ?
_ExceptEsp          dd      ?

        public  _TestTable, _TestTableCenter
_TestTable          dd      64 dup (?)
_TestTableCenter    dd      64 dup (?)
_DATA   ENDS

DivTest   macro  div,reg,type,labelmod
    ;   public  &div&_&type&_&reg&labelmod      - too many labels
&div&_&type&_&reg&labelmod:
endm

endtest   macro
        call    Marker
        dd      0CCCCCCCCh              ; marker for expcetion
                                        ; handler to find next test
endm

REGDiv  macro  type, reglist
    irp reg,<reglist>
    DivTest div, reg, type, _reg
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        mov     reg, type ptr _DivOperand
        div     reg
        endtest

    DivTest idiv, reg, type, _reg
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        mov     reg, type ptr _DivOperand
        idiv    reg
        endtest
    endm
endm

PTRDiv  macro  typelist
    irp type,<typelist>

    DivTest div, reg, type, _ptr
        mov     edx, _DivOperand
ifidni <type>,<byte>
        mov     byte ptr _TestTableCenter, dl
else
ifidni <type>,<word>
        mov     word ptr _TestTableCenter, dx
else
        mov     dword ptr _TestTableCenter, edx
endif
endif
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        div     type ptr _TestTableCenter
        endtest

    DivTest idiv, reg, type, _ptr
        mov     edx, _DivOperand
ifidni <type>,<byte>
        mov     byte ptr _TestTableCenter, dl
else
ifidni <type>,<word>
        mov     word ptr _TestTableCenter, dx
else
        mov     dword ptr _TestTableCenter, edx
endif
endif
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        idiv    type ptr _TestTableCenter
        endtest
    endm
endm

REGDivP1  macro  type, offset, divpointer, labelmod, reglist
    irp reg,<reglist>
    DivTest div, reg, type, labelmod
        mov     eax, divpointer
        mov     edx, _DivOperand
ifidni <type>,<byte>
        mov     byte ptr [eax + offset], dl
else
ifidni <type>,<word>
        mov     word ptr [eax + offset], dx
else
        mov     dword ptr [eax + offset], edx
endif
endif
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        mov     reg, divpointer
        div     type ptr [reg + offset]
        endtest

    DivTest idiv, reg, type, labelmod
        mov     eax, divpointer
        mov     edx, _DivOperand
ifidni <type>,<byte>
        mov     byte ptr [eax + offset], dl
else
ifidni <type>,<word>
        mov     word ptr [eax + offset], dx
else
        mov     dword ptr [eax + offset], edx
endif
endif
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        mov     reg, divpointer
        idiv    type ptr [reg + offset]
        endtest
    endm
endm

REGDivSIB1  macro  type, offset, divpointer, labelmod, toscale, scaler, reglist
    irp reg,<reglist>
    DivTest div, reg, type, labelmod
        push    ebx
        mov     eax, divpointer
        mov     edx, _DivOperand
        mov     ebx, _DivRegScaler
ifidni <type>,<byte>
        mov     byte ptr [ebx * toscale + eax + offset], dl
else
ifidni <type>,<word>
        mov     word ptr [ebx * toscale + eax + offset], dx
else
        mov     dword ptr [ebx * toscale + eax + offset], edx
endif
endif
        pop     ebx
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        mov     reg, divpointer
        mov     scaler, _DivRegScaler
        div     type ptr [scaler * toscale + reg + offset]
        endtest

    DivTest idiv, reg, type, labelmod
        push    ebx
        mov     eax, divpointer
        mov     edx, _DivOperand
        mov     ebx, _DivRegScaler
ifidni <type>,<byte>
        mov     byte ptr [ebx * toscale + eax + offset], dl
else
ifidni <type>,<word>
        mov     word ptr [ebx * toscale + eax + offset], dx
else
        mov     dword ptr [ebx * toscale + eax + offset], edx
endif
endif
        pop     ebx
        mov     eax, 7f7f7f7fh
        mov     edx, eax
        mov     reg, divpointer
        mov     scaler, _DivRegScaler
        idiv    type ptr [scaler * toscale + reg + offset]
        endtest
    endm
endm


REGDivP  macro  typelist, reglist
    irp type, <typelist>
        REGDivP1 type,  0, _DivRegPointer, _d, <reglist>
        REGDivP1 type,  1, _DivRegPointer, _p, <reglist>
        REGDivP1 type, -1, _DivRegPointer, _m, <reglist>
        REGDivP1 type, _TestTableCenter,  0, _rd, <reglist>
        REGDivP1 type, _TestTableCenter,  1, _rp, <reglist>
        REGDivP1 type, _TestTableCenter, -1, _rm, <reglist>
    endm
endm

REGDivSIB  macro  typelist, scaler, reglist
    irp type, <typelist>
        REGDivSIB1 type,  0, _DivRegPointer, _&scaler&_d,  1, scaler, <reglist>
        REGDivSIB1 type,  1, _DivRegPointer, _&scaler&_p,  1, scaler, <reglist>
        REGDivSIB1 type, -1, _DivRegPointer, _&scaler&_m,  1, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  0, _&scaler&_rd, 1, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  1, _&scaler&_rp, 1, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter, -1, _&scaler&_rm, 1, scaler, <reglist>
    ;---
        REGDivSIB1 type,  0, _DivRegPointer, _&scaler&_d2,  2, scaler, <reglist>
        REGDivSIB1 type,  1, _DivRegPointer, _&scaler&_p2,  2, scaler, <reglist>
        REGDivSIB1 type, -1, _DivRegPointer, _&scaler&_m2,  2, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  0, _&scaler&_r2d, 2, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  1, _&scaler&_r2p, 2, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter, -1, _&scaler&_r2m, 2, scaler, <reglist>
    ;---
        REGDivSIB1 type,  0, _DivRegPointer, _&scaler&_d4,  4, scaler, <reglist>
        REGDivSIB1 type,  1, _DivRegPointer, _&scaler&_p4,  4, scaler, <reglist>
        REGDivSIB1 type, -1, _DivRegPointer, _&scaler&_m4,  4, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  0, _&scaler&_r4d, 4, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  1, _&scaler&_r4p, 4, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter, -1, _&scaler&_r4m, 4, scaler, <reglist>
    ;---
        REGDivSIB1 type,  0, _DivRegPointer, _&scaler&_d8,  8, scaler, <reglist>
        REGDivSIB1 type,  1, _DivRegPointer, _&scaler&_p8,  8, scaler, <reglist>
        REGDivSIB1 type, -1, _DivRegPointer, _&scaler&_m8,  8, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  0, _&scaler&_r8d, 8, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter,  1, _&scaler&_r8p, 8, scaler, <reglist>
        REGDivSIB1 type, _TestTableCenter, -1, _&scaler&_r8m, 8, scaler, <reglist>
    endm
endm


_TEXT   SEGMENT DWORD PUBLIC USE32 'CODE'       ; Start 32 bit code
        ASSUME  CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


cPublicProc     _TestDiv

    ; save c runtime registers

            push    ebp
            push    ebx
            push    esi
            push    edi

    ; prime outer loop with initial exception
            endtest

    ; start of div test

             PTRDiv      <byte, word, dword>

             REGDiv      <byte>,  <bl,bh,cl,ch,dl,dh>
             REGDiv      <word>,  <ax,bx,cx,si,di,bp>
             REGDiv      <dword>, <ebx,ecx,edx,esi,edi,ebp>

             REGDivP     <byte>, <ebx,ecx,edx,esi,edi,ebp>
             REGDivP     <word, dword>, <eax,ebx,ecx,esi,edi,ebp>

             REGDivSIB   <byte>, <ebx>, <ecx,edx,esi,edi,ebp>
             REGDivSIB   <byte>, <ecx>, <ebx,edx,esi,edi,ebp>
             REGDivSIB   <byte>, <edx>, <ebx,ecx,esi,edi,ebp>
             REGDivSIB   <byte>, <esi>, <ebx,ecx,edx,edi,ebp>
             REGDivSIB   <byte>, <edi>, <ebx,ecx,edx,esi,ebp>
             REGDivSIB   <byte>, <ebp>, <ebx,ecx,edx,edi,esi>

             REGDivSIB   <word, dword>, <eax>, <ebx,ecx,esi,edi,ebp>
             REGDivSIB   <word, dword>, <ebx>, <eax,ecx,esi,edi,ebp>
             REGDivSIB   <word, dword>, <ecx>, <eax,ebx,esi,edi,ebp>
             REGDivSIB   <word, dword>, <esi>, <eax,ebx,ecx,edi,ebp>
             REGDivSIB   <word, dword>, <edi>, <eax,ebx,ecx,esi,ebp>
             REGDivSIB   <word, dword>, <ebp>, <eax,ebx,ecx,edi,esi>

    ; end of test

            pop     edi
            pop     esi
            pop     ebx
            pop     ebp

            stdRET  _TestDiv
stdENDP _TestDiv

cPublicProc     Marker
        pop     eax
        mov     _ExceptEip, eax
        mov     _ExceptEsp, esp
        stdCall _DivMarker
    int 3
stdENDP Marker

_TEXT   ENDS
END
