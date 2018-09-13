        page    ,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Fri Aug 23 13:58:54 1996

;Command Line: ..\..\..\dev\tools\binr\thunk.exe -NC _TEXT -o Shl3216 ..\Shl3216.thk 

        TITLE   $Shl3216.asm

        .386
        OPTION READONLY
        OPTION OLDSTRUCTS

IFNDEF IS_16
IFNDEF IS_32
%out command line error: specify one of -DIS_16, -DIS_32
.err
ENDIF  ;IS_32
ENDIF  ;IS_16


IFDEF IS_32
IFDEF IS_16
%out command line error: you can't specify both -DIS_16 and -DIS_32
.err
ENDIF ;IS_16
;************************* START OF 32-BIT CODE *************************


        .model FLAT,STDCALL


;-- Import common flat thunk routines (in k32)

externDef AllocMappedBuffer     :near32
externDef FreeMappedBuffer              :near32
externDef MapHInstLS    :near32
externDef MapHInstLS_PN :near32
externDef MapHInstSL    :near32
externDef MapHInstSL_PN :near32
externDef FT_Prolog     :near32
externDef FT_Thunk      :near32
externDef QT_Thunk      :near32
externDef FT_Exit0      :near32
externDef FT_Exit4      :near32
externDef FT_Exit8      :near32
externDef FT_Exit12     :near32
externDef FT_Exit16     :near32
externDef FT_Exit20     :near32
externDef FT_Exit24     :near32
externDef FT_Exit28     :near32
externDef FT_Exit32     :near32
externDef FT_Exit36     :near32
externDef FT_Exit40     :near32
externDef FT_Exit44     :near32
externDef FT_Exit48     :near32
externDef FT_Exit52     :near32
externDef FT_Exit56     :near32
externDef SMapLS        :near32
externDef SUnMapLS      :near32
externDef SMapLS_IP_EBP_8       :near32
externDef SUnMapLS_IP_EBP_8     :near32
externDef SMapLS_IP_EBP_12      :near32
externDef SUnMapLS_IP_EBP_12    :near32
externDef SMapLS_IP_EBP_16      :near32
externDef SUnMapLS_IP_EBP_16    :near32
externDef SMapLS_IP_EBP_20      :near32
externDef SUnMapLS_IP_EBP_20    :near32
externDef SMapLS_IP_EBP_24      :near32
externDef SUnMapLS_IP_EBP_24    :near32
externDef SMapLS_IP_EBP_28      :near32
externDef SUnMapLS_IP_EBP_28    :near32
externDef SMapLS_IP_EBP_32      :near32
externDef SUnMapLS_IP_EBP_32    :near32
externDef SMapLS_IP_EBP_36      :near32
externDef SUnMapLS_IP_EBP_36    :near32
externDef SMapLS_IP_EBP_40      :near32
externDef SUnMapLS_IP_EBP_40    :near32

MapLS   PROTO NEAR STDCALL :DWORD
UnMapLS PROTO NEAR STDCALL :DWORD
MapSL   PROTO NEAR STDCALL p32:DWORD

;***************** START OF KERNEL32-ONLY SECTION ******************
; Hacks for kernel32 initialization.

IFDEF FT_DEFINEFTCOMMONROUTINES

        .fardata
        db "Smag"               ; junk to give a non-zero address 
public FT_Shl3216TargetTable    ;Flat address of target table in 16-bit module.

public FT_Shl3216Checksum32
FT_Shl3216Checksum32    dd      02e50fh


ENDIF ;FT_DEFINEFTCOMMONROUTINES
;***************** END OF KERNEL32-ONLY SECTION ******************



        .code 

;************************* COMMON PER-MODULE ROUTINES *************************

        .fardata
        db "Smag"               ; junk to give a non-zero address 

public Shl3216_ThunkData32      ;This symbol must be exported.
Shl3216_ThunkData32 label dword
        dd      3130534ch       ;Protocol 'LS01'
        dd      02e50fh ;Checksum
        dd      0       ;Jump table address.
        dd      3130424ch       ;'LB01'
        dd      0       ;Flags
        dd      0       ;Reserved (MUST BE 0)
        dd      0       ;Reserved (MUST BE 0)
        dd      offset QT_Thunk_Shl3216 - offset Shl3216_ThunkData32
        dd      offset FT_Prolog_Shl3216 - offset Shl3216_ThunkData32



        .code 


externDef ThunkConnect32@24:near32

public Shl3216_ThunkConnect32@16
Shl3216_ThunkConnect32@16:
        pop     edx
        push    offset Shl3216_ThkData16
        push    offset Shl3216_ThunkData32
        push    edx
        jmp     ThunkConnect32@24
Shl3216_ThkData16 label byte
        db      "Shl3216_ThunkData16",0


                


;;; THE FOLLOWING ROUTINES ARE HERE TO SUPPORT OLD-STYLE
;;; BODYQT macros.


QT_Call16_Raw:
        mov     edx,Shl3216_ThunkData32+8
        pop     dword ptr [ebp-64]
        movzx   ecx, byte ptr [ebp-4]
        mov     edx, [edx+ecx*4]
        call    QT_Thunk
        push    ebx
        mov     eax,ebx
        pop     ebx
        jmp     dword ptr [ebp-64]





QT_Call16_ShortToLong:
        mov     edx,Shl3216_ThunkData32+8
        pop     dword ptr [ebp-64]
        movzx   ecx, byte ptr [ebp-4]
        mov     edx, [edx+ecx*4]
        call    QT_Thunk
        push    ebx
        movsx   ebx,ax
        mov     eax,ebx
        pop     ebx
        jmp     dword ptr [ebp-64]





QT_Call16_UShortToULong:
        mov     edx,Shl3216_ThunkData32+8
        pop     dword ptr [ebp-64]
        movzx   ecx, byte ptr [ebp-4]
        mov     edx, [edx+ecx*4]
        call    QT_Thunk
        push    ebx
        movzx   ebx,ax
        mov     eax,ebx
        pop     ebx
        jmp     dword ptr [ebp-64]





QT_Call16_DWordToDWord:
        mov     edx,Shl3216_ThunkData32+8
        pop     dword ptr [ebp-64]
        movzx   ecx, byte ptr [ebp-4]
        mov     edx, [edx+ecx*4]
        call    QT_Thunk
        push    ebx
        mov     ebx,edx
        shl     ebx,16
        mov     bx,ax
        mov     eax,ebx
        pop     ebx
        jmp     dword ptr [ebp-64]





QT_Call16_Far16ToNear32:
        mov     edx,Shl3216_ThunkData32+8
        pop     dword ptr [ebp-64]
        movzx   ecx, byte ptr [ebp-4]
        mov     edx, [edx+ecx*4]
        call    QT_Thunk
        push    ebx
        push    dx
        push    ax
        call    MapSL
        mov     ebx,eax
        mov     eax,ebx
        pop     ebx
        jmp     dword ptr [ebp-64]





QT_Call16_VoidToTrue:
        mov     edx,Shl3216_ThunkData32+8
        pop     dword ptr [ebp-64]
        movzx   ecx, byte ptr [ebp-4]
        mov     edx, [edx+ecx*4]
        call    QT_Thunk
        push    ebx
        xor     ebx,ebx
        inc     ebx
        mov     eax,ebx
        pop     ebx
        jmp     dword ptr [ebp-64]





QT_Call16_VoidToFalse:
        mov     edx,Shl3216_ThunkData32+8
        pop     dword ptr [ebp-64]
        movzx   ecx, byte ptr [ebp-4]
        mov     edx, [edx+ecx*4]
        call    QT_Thunk
        push    ebx
        xor     ebx,ebx
        mov     eax,ebx
        pop     ebx
        jmp     dword ptr [ebp-64]






pfnQT_Thunk_Shl3216     dd offset QT_Thunk_Shl3216
pfnFT_Prolog_Shl3216    dd offset FT_Prolog_Shl3216
        .fardata
        db "Smag"               ; junk to give a non-zero address 

public QT_Thunk_Shl3216         ; this is so lego can see it
QT_Thunk_Shl3216 label byte
        db      32 dup(0cch)    ;Patch space.

public FT_Prolog_Shl3216        ; this is so lego can see it
FT_Prolog_Shl3216 label byte
        db      32 dup(0cch)    ;Patch space.


        .code 




ebp_top         equ     <[ebp + 8]>     ;First api parameter
ebp_retval      equ     <[ebp + -64]>   ;Api return value
FT_ESPFIXUP     macro   dwSpOffset
        or      dword ptr [ebp + -20], 1 SHL ((dwSpOffset) SHR 1)
endm


ebp_qttop       equ     <[ebp + 8]>


include fltthk.inc      ;Support definitions
include Shl3216.inc



;************************ START OF THUNK BODIES************************




;
public CallCPLEntry16@24
CallCPLEntry16@24:
        FAPILOG16       362
        mov     cl,15
; CallCPLEntry16(16) = CallCPLEntry16(32) {}
;
; dword ptr [ebp+8]:  hinst
; dword ptr [ebp+12]:  lpfnEntry
; dword ptr [ebp+16]:  hwndCPL
; dword ptr [ebp+20]:  msg
; dword ptr [ebp+24]:  lParam1
; dword ptr [ebp+28]:  lParam2
;
public IICallCPLEntry16@24
IICallCPLEntry16@24:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        BODYQT_CALLCPLENTRY16   15
        leave
        retn    24





;
public GetModuleHandle16@4
GetModuleHandle16@4:
        FAPILOG16       340
        mov     cl,14
; GetModuleHandle16(16) = GetModuleHandle16(32) {}
;
; dword ptr [ebp+8]:  szName
;
public IIGetModuleHandle16@4
IIGetModuleHandle16@4:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        call    SMapLS_IP_EBP_8
        push    eax
        call    dword ptr [pfnQT_Thunk_Shl3216]
        movzx   eax,ax
        call    SUnMapLS_IP_EBP_8
        leave
        retn    4





;
public GetModuleFileName16@12
GetModuleFileName16@12:
        FAPILOG16       316
        mov     cl,13
; GetModuleFileName16(16) = GetModuleFileName16(32) {}
;
; dword ptr [ebp+8]:  hinst
; dword ptr [ebp+12]:  szFileName
; dword ptr [ebp+16]:  cbMax
;
public IIGetModuleFileName16@12
IIGetModuleFileName16@12:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    word ptr [ebp+8]        ;hinst: dword->word
        call    SMapLS_IP_EBP_12
        push    eax
        push    word ptr [ebp+16]       ;cbMax: dword->word
        call    dword ptr [pfnQT_Thunk_Shl3216]
        cwde
        call    SUnMapLS_IP_EBP_12
        leave
        retn    12





;
public CheckResourcesBeforeExec@0
CheckResourcesBeforeExec@0:
        FAPILOG16       265
        mov     cl,11
; CheckResourcesBeforeExec(16) = CheckResourcesBeforeExec(32) {}
;
;
public IICheckResourcesBeforeExec@0
IICheckResourcesBeforeExec@0:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        call    dword ptr [pfnQT_Thunk_Shl3216]
        cwde
        leave
        retn





;
public CallAddPropSheetPages16@12
CallAddPropSheetPages16@12:
        FAPILOG16       237
        mov     cl,10
; CallAddPropSheetPages16(16) = CallAddPropSheetPages16(32) {}
;
; dword ptr [ebp+8]:  lpfn16
; dword ptr [ebp+12]:  hdrop
; dword ptr [ebp+16]:  papg
;
public IICallAddPropSheetPages16@12
IICallAddPropSheetPages16@12:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    dword ptr [ebp+8]       ;lpfn16: dword->dword
        call    SMapLS_IP_EBP_12
        push    eax
        call    SMapLS_IP_EBP_16
        push    eax
        call    dword ptr [pfnQT_Thunk_Shl3216]
        call    SUnMapLS_IP_EBP_12
        call    SUnMapLS_IP_EBP_16
        leave
        retn    12





;
public ShellGetNextDriverName@12
ShellGetNextDriverName@12:
        FAPILOG16       210
        mov     cl,9
; ShellGetNextDriverName(16) = ShellGetNextDriverName(32) {}
;
; dword ptr [ebp+8]:  hdrv
; dword ptr [ebp+12]:  pszName
; dword ptr [ebp+16]:  cbName
;
public IIShellGetNextDriverName@12
IIShellGetNextDriverName@12:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    word ptr [ebp+8]        ;hdrv: dword->word
        call    SMapLS_IP_EBP_12
        push    eax
        push    word ptr [ebp+16]       ;cbName: dword->word
        call    dword ptr [pfnQT_Thunk_Shl3216]
        movzx   eax,ax
        call    SUnMapLS_IP_EBP_12
        leave
        retn    12





;
public SHRestartWindows@4
SHRestartWindows@4:
        FAPILOG16       189
        mov     cl,8
; SHRestartWindows(16) = SHRestartWindows(32) {}
;
; dword ptr [ebp+8]:  dwReturn
;
public IISHRestartWindows@4
IISHRestartWindows@4:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    dword ptr [ebp+8]       ;dwReturn: dword->dword
        call    dword ptr [pfnQT_Thunk_Shl3216]
        cwde
        leave
        retn    4





;
public SHGetAboutInformation@8
SHGetAboutInformation@8:
        FAPILOG16       163
        mov     cl,7
; SHGetAboutInformation(16) = SHGetAboutInformation(32) {}
;
; dword ptr [ebp+8]:  puSysResource
; dword ptr [ebp+12]:  plMem
;
public IISHGetAboutInformation@8
IISHGetAboutInformation@8:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        call    SMapLS_IP_EBP_8
        push    eax
        call    SMapLS_IP_EBP_12
        push    eax
        call    dword ptr [pfnQT_Thunk_Shl3216]
        call    SUnMapLS_IP_EBP_8
        call    SUnMapLS_IP_EBP_12
        leave
        retn    8





;
public SHFormatDrive@16
SHFormatDrive@16:
        FAPILOG16       145
        mov     cl,6
; SHFormatDrive(16) = SHFormatDrive(32) {}
;
; dword ptr [ebp+8]:  hwnd
; dword ptr [ebp+12]:  drive
; dword ptr [ebp+16]:  fmtID
; dword ptr [ebp+20]:  options
;
public IISHFormatDrive@16
IISHFormatDrive@16:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    word ptr [ebp+8]        ;hwnd: dword->word
        push    word ptr [ebp+12]       ;drive: dword->word
        push    word ptr [ebp+16]       ;fmtID: dword->word
        push    word ptr [ebp+20]       ;options: dword->word
        call    dword ptr [pfnQT_Thunk_Shl3216]
        shl     eax,16
        shrd    eax,edx,16
        leave
        retn    16





;
public PifMgr_OpenProperties@16
PifMgr_OpenProperties@16:
        FAPILOG16       119
        mov     cl,5
; PifMgr_OpenProperties(16) = PifMgr_OpenProperties(32) {}
;
; dword ptr [ebp+8]:  lpszApp
; dword ptr [ebp+12]:  lpszPIF
; dword ptr [ebp+16]:  hInf
; dword ptr [ebp+20]:  flOpt
;
public IIPifMgr_OpenProperties@16
IIPifMgr_OpenProperties@16:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        call    SMapLS_IP_EBP_8
        push    eax
        call    SMapLS_IP_EBP_12
        push    eax
        push    word ptr [ebp+16]       ;hInf: dword->word
        push    word ptr [ebp+20]       ;flOpt: dword->word
        call    dword ptr [pfnQT_Thunk_Shl3216]
        cwde
        call    SUnMapLS_IP_EBP_8
        call    SUnMapLS_IP_EBP_12
        leave
        retn    16





;
public PifMgr_SetProperties@20
PifMgr_SetProperties@20:
        FAPILOG16       69
        mov     cl,3
        jmp     IIPifMgr_SetProperties@20
public PifMgr_GetProperties@20
PifMgr_GetProperties@20:
        FAPILOG16       94
        mov     cl,4
; PifMgr_SetProperties(16) = PifMgr_SetProperties(32) {}
;
; dword ptr [ebp+8]:  hProps
; dword ptr [ebp+12]:  lpszGroup
; dword ptr [ebp+16]:  lpProps
; dword ptr [ebp+20]:  cbProps
; dword ptr [ebp+24]:  flOpt
;
public IIPifMgr_SetProperties@20
IIPifMgr_SetProperties@20:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    word ptr [ebp+8]        ;hProps: dword->word
        call    SMapLS_IP_EBP_12
        push    eax
        call    SMapLS_IP_EBP_16
        push    eax
        push    word ptr [ebp+20]       ;cbProps: dword->word
        push    word ptr [ebp+24]       ;flOpt: dword->word
        call    dword ptr [pfnQT_Thunk_Shl3216]
        cwde
        call    SUnMapLS_IP_EBP_12
        call    SUnMapLS_IP_EBP_16
        leave
        retn    20





;
public PifMgr_CloseProperties@8
PifMgr_CloseProperties@8:
        FAPILOG16       42
        mov     cl,2
        jmp     IIPifMgr_CloseProperties@8
public RegisterShellHook@8
RegisterShellHook@8:
        FAPILOG16       294
        mov     cl,12
; PifMgr_CloseProperties(16) = PifMgr_CloseProperties(32) {}
;
; dword ptr [ebp+8]:  hProps
; dword ptr [ebp+12]:  flOpt
;
public IIPifMgr_CloseProperties@8
IIPifMgr_CloseProperties@8:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    word ptr [ebp+8]        ;hProps: dword->word
        push    word ptr [ebp+12]       ;flOpt: dword->word
        call    dword ptr [pfnQT_Thunk_Shl3216]
        cwde
        leave
        retn    8





;
public SHGlobalDefect@4
SHGlobalDefect@4:
        FAPILOG16       23
        mov     cl,1
; SHGlobalDefect(16) = SHGlobalDefect(32) {}
;
; dword ptr [ebp+8]:  dwHnd32
;
public IISHGlobalDefect@4
IISHGlobalDefect@4:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    dword ptr [ebp+8]       ;dwHnd32: dword->dword
        call    dword ptr [pfnQT_Thunk_Shl3216]
        leave
        retn    4





;
public RunDll_CallEntry16@20
RunDll_CallEntry16@20:
        FAPILOG16       0
        mov     cl,0
; RunDll_CallEntry16(16) = RunDll_CallEntry16(32) {}
;
; dword ptr [ebp+8]:  pfn
; dword ptr [ebp+12]:  hwndStub
; dword ptr [ebp+16]:  hinst
; dword ptr [ebp+20]:  pszParam
; dword ptr [ebp+24]:  nCmdShow
;
public IIRunDll_CallEntry16@20
IIRunDll_CallEntry16@20:
        push    ebp
        mov     ebp,esp
        push    ecx
        sub     esp,60
        push    dword ptr [ebp+8]       ;pfn: dword->dword
        push    word ptr [ebp+12]       ;hwndStub: dword->word
        mov     eax, dword ptr [ebp+16] ;hinst: hinst32->hinst16
        call    MapHInstLS
        push    ax
        call    SMapLS_IP_EBP_20
        push    eax
        push    word ptr [ebp+24]       ;nCmdShow: dword->word
        call    dword ptr [pfnQT_Thunk_Shl3216]
        call    SUnMapLS_IP_EBP_20
        leave
        retn    20




;-----------------------------------------------------------
ifdef DEBUG
FT_ThunkLogNames label byte
        db      '[F] RunDll_CallEntry16',0
        db      '[F] SHGlobalDefect',0
        db      '[F] PifMgr_CloseProperties',0
        db      '[F] PifMgr_SetProperties',0
        db      '[F] PifMgr_GetProperties',0
        db      '[F] PifMgr_OpenProperties',0
        db      '[F] SHFormatDrive',0
        db      '[F] SHGetAboutInformation',0
        db      '[F] SHRestartWindows',0
        db      '[F] ShellGetNextDriverName',0
        db      '[F] CallAddPropSheetPages16',0
        db      '[F] CheckResourcesBeforeExec',0
        db      '[F] RegisterShellHook',0
        db      '[F] GetModuleFileName16',0
        db      '[F] GetModuleHandle16',0
        db      '[F] CallCPLEntry16',0
endif ;DEBUG
;-----------------------------------------------------------



ELSE
;************************* START OF 16-BIT CODE *************************




        OPTION SEGMENT:USE16
        .model LARGE,PASCAL


        .code   _TEXT



externDef RunDll_CallEntry16:far16
externDef SHGlobalDefect:far16
externDef PifMgr_CloseProperties:far16
externDef PifMgr_SetProperties:far16
externDef PifMgr_GetProperties:far16
externDef PifMgr_OpenProperties:far16
externDef SHFormatDrive:far16
externDef SHGetAboutInformation:far16
externDef SHRestartWindows:far16
externDef ShellGetNextDriverName:far16
externDef CallAddPropSheetPages16:far16
externDef CheckResourcesBeforeExec:far16
externDef RegisterShellHook:far16
externDef GetModuleFileName16:far16
externDef GetModuleHandle16:far16
externDef CallCPLEntry16:far16


FT_Shl3216TargetTable label word
        dw      offset RunDll_CallEntry16
        dw         seg RunDll_CallEntry16
        dw      offset SHGlobalDefect
        dw         seg SHGlobalDefect
        dw      offset PifMgr_CloseProperties
        dw         seg PifMgr_CloseProperties
        dw      offset PifMgr_SetProperties
        dw         seg PifMgr_SetProperties
        dw      offset PifMgr_GetProperties
        dw         seg PifMgr_GetProperties
        dw      offset PifMgr_OpenProperties
        dw         seg PifMgr_OpenProperties
        dw      offset SHFormatDrive
        dw         seg SHFormatDrive
        dw      offset SHGetAboutInformation
        dw         seg SHGetAboutInformation
        dw      offset SHRestartWindows
        dw         seg SHRestartWindows
        dw      offset ShellGetNextDriverName
        dw         seg ShellGetNextDriverName
        dw      offset CallAddPropSheetPages16
        dw         seg CallAddPropSheetPages16
        dw      offset CheckResourcesBeforeExec
        dw         seg CheckResourcesBeforeExec
        dw      offset RegisterShellHook
        dw         seg RegisterShellHook
        dw      offset GetModuleFileName16
        dw         seg GetModuleFileName16
        dw      offset GetModuleHandle16
        dw         seg GetModuleHandle16
        dw      offset CallCPLEntry16
        dw         seg CallCPLEntry16




        .data

public Shl3216_ThunkData16      ;This symbol must be exported.
Shl3216_ThunkData16     dd      3130534ch       ;Protocol 'LS01'
        dd      02e50fh ;Checksum
        dw      offset FT_Shl3216TargetTable
        dw      seg    FT_Shl3216TargetTable
        dd      0       ;First-time flag.



        .code _TEXT


externDef ThunkConnect16:far16

public Shl3216_ThunkConnect16
Shl3216_ThunkConnect16:
        pop     ax
        pop     dx
        push    seg    Shl3216_ThunkData16
        push    offset Shl3216_ThunkData16
        push    seg    Shl3216_ThkData32
        push    offset Shl3216_ThkData32
        push    cs
        push    dx
        push    ax
        jmp     ThunkConnect16
Shl3216_ThkData32 label byte
        db      "Shl3216_ThunkData32",0





ENDIF
END
