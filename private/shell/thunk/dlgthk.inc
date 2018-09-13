
CodeData equ <THUNK16BCodeData>

include     thkframe.inc
include     thkrp.inc
include     cbcheck.inc
include     public.inc

include     cbcid.inc
include     dlgcbid.inc

externDef FreeCallback:far16
externDef AllocCallback:far16
externDef ConvertDevModeHandleSL:far16
externDef ConvertDevNamesHandleSL:far16
externDef ConvertDevModeHandleLS:far16
externDef ConvertDevNamesHandleLS:far16


fLocalFlag      equ     fDlgLog16

; note: following size should be the same as in thunkcom\pack.h

OPENFILENAME16_SIZE         EQU  72
OPENFILENAME32_SIZE         EQU  76     ;4Ch
CHOOSECOLOR16_SIZE          EQU  32
CHOOSECOLOR32_SIZE          EQU  36     ;24h
CHOOSEFONT16_SIZE           EQU  46
CHOOSEFONT32_SIZE           EQU  60     ;3Ch
PRINTDLG16_SIZE             EQU  52
PRINTDLG32_SIZE             EQU  66     ;42h

LOGFONT16_SIZE              EQU  50

OFN16_Flags_OFFSET          EQU  48     ;30h 
CC16_rgbResult_OFFSET       EQU   8     ;08h
CF16_iPointSize_OFFSET      EQU  12     ;0Ch
CF16_Flags_OFFSET           EQU  14     ;0Dh
CF16_rgbColors_OFFSET       EQU  18     ;12h
CF16_nFontType_OFFSET       EQU  40     ;28h
PD16_hDevMode_OFFSET        EQU   6     ;06h
PD16_hDevNames_OFFSET       EQU   8     ;08h

OFN32_Flags_OFFSET          EQU  52     ;34h
OFN32_lpfnHook_OFFSET       EQU  68     ;44h
CC32_rgbResult_OFFSET       EQU  12     ;0Ch
CC32_Flags_OFFSET           EQU  20     ;14h
CC32_lpfnHook_OFFSET        EQU  28     ;1Ch
FR32_Flags_OFFSET           EQU  12     ;0CH
FR32_lpfnHook_OFFSET        EQU  32     ;20h
CF32_lpLogFont_OFFSET       EQU  12     ;0Ch
CF32_iPointSize_OFFSET      EQU  16     ;10h
CF32_Flags_OFFSET           EQU  20     ;14h
CF32_rgbColors_OFFSET       EQU  24     ;18h
CF32_lpfnHook_OFFSET        EQU  32     ;20h
CF32_nFontType_OFFSET       EQU  48     ;30h
PD32_Flags_OFFSET           EQU  20     ;1Ch
PD32_hDevMode_OFFSET        EQU   8     ;08h
PD32_hDevNames_OFFSET       EQU  12     ;0Ch
PD32_lpfnPrintHook_OFFSET   EQU  42     ;2Ah
PD32_lpfnSetupHook_OFFSET   EQU  46     ;2Eh


;---------------------------------------------------------------------------
; This macro does the work for GetOpenFilename() and GetSaveFilename().
; The argument "Flags" is a dword ptr expression denoting a thunk-compiler
; temporary that holds the Flags field of the OPENFILENAME struct.
;
; Assumption: If the hInst field is a global handle, the Win32 wrapper
; guaratees it's a (zero-extended) Win16 handle.
;---------------------------------------------------------------------------
RAWPACK__THKGETOPENORSAVEFILENAME_lpOfn    macro    iOffset,iTempOffset,Flags
    local   skip_it
    local    nohook,hook_done
    local    notemplate,template_done

    sub     sp,OPENFILENAME16_SIZE        ; alloc space for MapLS sels
    sub     sp,OPENFILENAME16_SIZE          ; alloc space on stack

    mov     eax,[bp+iOffset]             ; base address
    or      eax,eax                         ; check structure pointer
    jz      skip_it                         ; skip if null

    mov     [bp-iTempOffset],sp          ; save offset to buffer
    mov     [bp-iTempOffset+2],ss        ; save selector to buffer

    mov     di,ss
    mov     es,di
    movzx   edi,sp                          ; es:edi == destination address

    mov     esi,eax
    mov     ds,cs:CodeData                  ; need access to FlatData
    mov     ds,FlatData                     ; ds:esi == source address

    mov    eax,ds:[esi + OFN32_Flags_OFFSET]
    mov    Flags,eax

    cld


    lodsd   ds:[esi]              ;BUGBUG lStructSize should be checked here
    mov     eax,OPENFILENAME16_SIZE         ;sizeof(OPENFILENAME16)
    stosd   es:[edi]              ;lStructSize same size

; comments refer to action going 16->32

    ;hwndOwner
    copyt

    mov     ecx,Flags
    and     ecx,OFN_ENABLETEMPLATE
    lodsd   ds:[esi]        ;hInst
    MAP_CD_NULL_HINST    eax,ecx
    stosw   es:[edi]

    MAP_POINTER es:[di+OPENFILENAME16_SIZE] ;PLUGGED lpstrFilter
    MAP_POINTER es:[di+OPENFILENAME16_SIZE] ;PLUGGED lpstrCustomFilter
    ncopyd  2                               ;nMaxCustFilter,nFilterIndex
    MAP_POINTER es:[di+OPENFILENAME16_SIZE] ;PLUGGED lpstrFile
    copyd                                   ;nMaxFile
    MAP_POINTER es:[di+OPENFILENAME16_SIZE] ;PLUGGED lpstrFileTitle
    copyd                                   ;nMaxFileTitle
    MAP_POINTER es:[di+OPENFILENAME16_SIZE] ;PLUGGED lpstrInitialDir
    MAP_POINTER es:[di+OPENFILENAME16_SIZE] ;PLUGGED lpstrTitle
    copyd                                   ;Flags
    ncopyw  2                               ;nFileOffset,nFileExtension
    MAP_POINTER  es:[di+OPENFILENAME16_SIZE];PLUGGED lpstrDefExt
    copyd                                   ;lCustData

    test    Flags,OFN_ENABLEHOOK
    jz      nohook
    MAP_CALLBACK CBID_OPENFILENAME           ;PLUGGED lpfnHook
    jmp     short hook_done
nohook:
    copyd
hook_done:

    ;lpTemplate must be mapped only if OFN_ENABLETEMPLATE is set
    xor     eax,eax
    mov     es:[di+OPENFILENAME16_SIZE],eax  ;in case we skip mapping
    test    Flags, OFN_ENABLETEMPLATE         ; if OFN_ENABLETEMPLATE is set
    jz      notemplate
    MAP_POINTER  es:[di+OPENFILENAME16_SIZE] ;PLUGGED lpTemplateName
    jmp     short template_done
notemplate:
    copyd                       ;lpTemplate
template_done:
skip_it:
endm

RAWUNPACK__THKGETOPENORSAVEFILENAME_lpOfn macro    iOffset,iTempOffset,Flags
    local   skip_it
    local    nohook,hook_done
    local    notemplate,template_done

    mov     eax,[bp+iOffset]              ; base address
    or      eax,eax                          ; check structure pointer
    jz      skip_it                          ; skip if null

    mov     es,cs:CodeData
    mov     es,es:FlatData
    mov     edi,eax                          ; source in es:edi

    push    ds
    lds     si,[bp-iTempOffset]
    movzx   esi,si            ;in case code uses esi by accident

    cld

    lods    dword ptr ds:[si]               ;BUGBUG lStructSize should be checked here
    mov     eax,OPENFILENAME32_SIZE         ;sizeof(OPENFILENAME32)
    stos    dword ptr es:[edi]              ;lStructSize same size

; comments refer to action going 16->32

    copyzx
    mov     ecx,Flags
    and     ecx,OFN_ENABLETEMPLATE
    lodsw   ds:[si]         ;hInst
    UMAP_CD_NULL_HINST    ax,ecx
    stosd   es:[edi]
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpstrFilter must be mapped
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpstrCustomFilter must be mapped
    ncopyd  2
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpstrFile must be mapped
    copyd
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpstrFileTitle must be mapped
    copyd
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpstrInitialDir must be mapped
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpstrTitle must be mapped
    copyd
    ncopyw  2
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpstrDefExt must be mapped
    copyd

    test    Flags,OFN_ENABLEHOOK
    jz      nohook
    lodsd   ds:[si]
    FREE_CALLBACK  CBID_OPENFILENAME
    stos    dword ptr es:[edi]
    jmp     short hook_done
nohook:
    copyd
hook_done:

    test    Flags,OFN_ENABLETEMPLATE
    jz      notemplate
    UMAP_POINTER ds:[si+OPENFILENAME16_SIZE] ;lpTemplateName must be mapped
    jmp     short template_done
notemplate:
    copyd
template_done:

    pop     ds
skip_it:

endm



;-----------------------------------------------------------------------------
; Pack/unpack OPENFILENAME struct for GetOpenFilename().
;-----------------------------------------------------------------------------
RAWPACK__THKGETOPENFILENAME_lpOfn    macro    iOffset,iTempOffset
    RAWPACK__THKGETOPENORSAVEFILENAME_lpOfn    iOffset,iTempOffset,dword ptr LOCAL__THKGETOPENFILENAME_Flags
endm

RAWUNPACK__THKGETOPENFILENAME_lpOfn    macro    iOffset,iTempOffset
    RAWUNPACK__THKGETOPENORSAVEFILENAME_lpOfn iOffset,iTempOffset,dword ptr LOCAL__THKGETOPENFILENAME_Flags
endm



;-----------------------------------------------------------------------------
; Pack/unpack OPENFILENAME struct for GetSaveFilename().
;-----------------------------------------------------------------------------
RAWPACK__THKGETSAVEFILENAME_lpOfn    macro    iOffset,iTempOffset
    RAWPACK__THKGETOPENORSAVEFILENAME_lpOfn    iOffset,iTempOffset,dword ptr LOCAL__THKGETSAVEFILENAME_Flags
endm

RAWUNPACK__THKGETSAVEFILENAME_lpOfn    macro    iOffset,iTempOffset
    RAWUNPACK__THKGETOPENORSAVEFILENAME_lpOfn iOffset,iTempOffset,dword ptr LOCAL__THKGETSAVEFILENAME_Flags
endm



;-----------------------------------------------------------------------------
; Pack CHOOSECOLOR struct for ChooseColor().
;
; Assumption: If the hInst field is a global handle, the Win32 wrapper
; guaratees it's a (zero-extended) Win16 handle.
;-----------------------------------------------------------------------------
RAWPACK__THKCHOOSECOLOR_lpcc        macro    iOffset,iTempOffset
    local        nohook,hook_done
    local        notemplate,template_done
    local        skip_it
    
    sub     sp,CHOOSECOLOR16_SIZE    ;Alloc space on stack

    mov     eax,[bp+iOffset]
    or      eax,eax
    jz      skip_it
    
    mov     [bp-iTempOffset],sp    ;Save offset to buffer
    mov     [bp-iTempOffset+2],ss    ;Save selector to buffer

    mov     di,ss
    mov     es,di
    movzx   edi,sp

    mov     esi,eax
    mov     ds,cs:CodeData
    mov     ds,FlatData

    mov     eax,ds:[esi + CC32_Flags_OFFSET]
    mov     LOCAL__THKCHOOSECOLOR_Flags,eax

    lodsd   ds:[esi]
    mov     eax,CHOOSECOLOR16_SIZE
    stosd   es:[di]    ;lStructSize

    ;hwndOwner
    copyt

    ;hInstance
    mov     ecx,LOCAL__THKCHOOSECOLOR_Flags
    and     ecx,CC_ENABLETEMPLATE
    lodsd   ds:[esi]    ;hInst
    MAP_CD_NULL_HINST    eax,ecx
    stosw   es:[di]

    ;rgbResult
    copyd

    MAP_POINTER    LOCAL__THKCHOOSECOLOR_lpCustColorsSeg  ;PLUGGED lpCustColors

    ;Flags, lCustData
    ncopyd  2

; Map hook function (only do the mapping if CC_ENABLEHOOK flag set).
    mov     eax,LOCAL__THKCHOOSECOLOR_Flags
    test    eax,CC_ENABLEHOOK
    jz      nohook
    MAP_CALLBACK    CBID_CHOOSECOLOR  ;PLUGGED lpfnHook
    jmp     short hook_done
nohook:
    copyd
hook_done:

; Map template name (only if CC_ENABLETEMPLATE flag set).
    mov     eax,LOCAL__THKCHOOSECOLOR_Flags
    test    eax,CC_ENABLETEMPLATE
    jz      notemplate
    MAP_POINTER    LOCAL__THKCHOOSECOLOR_lpTemplateNameSeg ;PLUGGED
    jmp     short template_done    ;lpTemplate
notemplate:
    copyd
template_done:
;  Finished

skip_it:
endm

;-----------------------------------------------------------------------------
; Unpack CHOOSECOLOR struct for ChooseColor().
;-----------------------------------------------------------------------------
RAWUNPACK__THKCHOOSECOLOR_lpcc    macro    iOffset,iTempOffset
    local   skip_it
    local    nohook

    mov     eax,[bp+iOffset]              ; base address
    or      eax,eax                          ; check structure pointer
    jz      skip_it                          ; skip if null

    mov     es,cs:CodeData
    mov     es,es:FlatData
    mov     edi,eax                          ; source in es:edi

; Free callback (if CC_ENABLEHOOK flag set).
    mov    eax,LOCAL__THKCHOOSECOLOR_Flags
    test    eax,CC_ENABLEHOOK
    jz    nohook
    mov     eax,es:[edi+CC32_lpfnHook_OFFSET]
    FREE_CALLBACK   CBID_CHOOSECOLOR
nohook:

; Propagate copy-outs field back to caller's struct.
    mov     si, [bp-iTempOffset]
    mov     eax,ss:[si+CC16_rgbResult_OFFSET]
    mov     es:[edi+CC32_rgbResult_OFFSET], eax

; Free up selectors (if no mapping happened, UnmapLS safely ignores
; NULL segments).
    pushd    LOCAL__THKCHOOSECOLOR_lpCustColorsSeg
    call    UnmapLS
    pushd    LOCAL__THKCHOOSECOLOR_lpTemplateNameSeg
    call    UnmapLS
skip_it:

endm




;-----------------------------------------------------------------------------
; Pack/unpack CHOOSEFONT struct for ChooseFont().
;
; Assumption: If the hInst field is a global handle, the Win32 wrapper
; guaratees it's a (zero-extended) Win16 handle.
;-----------------------------------------------------------------------------
RAWPACK__THKCHOOSEFONT_lpcf        macro    iOffset,iTempOffset
    local   skip_it
    local    nohook,hook_done
    local    notemplate,template_done
    local   no_log_font

    sub     sp,CHOOSEFONT16_SIZE+LOGFONT16_SIZE  ; alloc space on stack

    mov     eax,[bp+iOffset]             ; base address
    or      eax,eax                         ; check structure pointer
    jz      skip_it                         ; skip if null

    mov     [bp-iTempOffset],sp          ; save offset to buffer
    mov     [bp-iTempOffset+2],ss        ; save selector to buffer

    mov     di,ss
    mov     es,di
    movzx   edi,sp                          ; es:edi == destination address

    mov     esi,eax
    mov     ds,cs:CodeData                  ; need access to FlatData
    mov     ds,FlatData                     ; ds:esi == source address

    mov     eax,ds:[esi + CF32_Flags_OFFSET]
    mov     LOCAL__THKCHOOSEFONT_Flags,eax


    cld

; comments refer to action going 16->32

    lodsd   ds:[esi]              ;BUGBUG lStructSize should be checked here
    mov     eax,CHOOSEFONT16_SIZE           ;sizeof(CHOOSEFONT16)
    stosd   es:[di]                         ;lStructSize same size

    ;hwndOwner, hDC
    ncopyt  2

    lodsd   ds:[esi]
    or      eax,eax
    jz      no_log_font

    mov     ax,ss
    ror     eax,16
    mov     ax,di
    add     ax,CHOOSEFONT16_SIZE-8          ;already moved DI by 8

no_log_font:
    stosd   es:[di]                         ;lpLogFont widened (need to pack it)

    ;iPointSize
    copyt

    ;Flags, rgbColors, lCustData
    ncopyd  3
    
    test    dword ptr LOCAL__THKCHOOSEFONT_Flags,CF_ENABLEHOOK
    jz      nohook
    MAP_CALLBACK CBID_CHOOSEFONT            ;PLUGGED lpfnHook
    jmp     short hook_done
nohook:
    copyd
hook_done:

    test    dword ptr LOCAL__THKCHOOSEFONT_Flags,CF_ENABLETEMPLATE
    jz      notemplate
    MAP_POINTER  LOCAL__THKCHOOSEFONT_lpTemplateNameTemp  ;PLUGGED
    jmp     short template_done
notemplate:
    copyd
template_done:

    mov     ecx,LOCAL__THKCHOOSEFONT_Flags
    and     ecx,CF_ENABLETEMPLATE
    lodsd   ds:[esi]    ;hInst
    MAP_CD_NULL_HINST    eax,ecx
    stosw   es:[di]

    ;lpszStyle must be mapped
    MAP_POINTER LOCAL__THKCHOOSEFONT_lpszStyleTemp  ;PLUGGED

    ;nFontType, nSizeMin, nSizeMax
    ncopyt  3

    mov     esi,[bp+iOffset]             ; base address
    mov     esi,[esi].CF32_lpLogFont_OFFSET ; ds:esi == source address
    or      esi,esi                         ; check structure pointer
    jz      skip_it                         ; skip if null

    cld

; comments refer to action going 16->32

    ;lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight
    ncopyt  5

    ;lfItalic, ..., lfFaceName 
    ncopyd  (40/4)
skip_it:
endm

RAWUNPACK__THKCHOOSEFONT_lpcf        macro    iOffset,iTempOffset
    local   skip_it
    local    nohook

;!!! We're only unpacking the log font structure!  Does the rest of
;    the CHOOSEFONT structure need to be unpacked as well?

    mov     eax,[bp+iOffset]              ; base address
    or      eax,eax                          ; check structure pointer
    jz      skip_it                          ; skip if null

    

    mov     es,cs:CodeData
    mov     es,es:FlatData
    mov     edi,eax                          ; dest in es:edi

    test    dword ptr LOCAL__THKCHOOSEFONT_Flags,CF_ENABLEHOOK
    jz    nohook
    mov     eax,es:[edi+CF32_lpfnHook_OFFSET]
    FREE_CALLBACK   CBID_CHOOSEFONT
nohook:

;; The iPointSize, rgbColors, Flags & nFontType fields need to be copied back
;; to the caller's buffer.

    lds     si,dword ptr [bp-iTempOffset]
    movzx   esi,si

    mov     eax,[si+CF16_rgbColors_OFFSET]	; rgbColors
    mov     es:[edi+CF32_rgbColors_OFFSET],eax

    mov     ax,[si+CF16_iPointSize_OFFSET]	; iPointSize
    movsx   eax,ax
    mov     es:[edi+CF32_iPointSize_OFFSET],eax

    mov     eax,[si+CF16_Flags_OFFSET]		; Flags
    mov     es:[edi+CF32_Flags_OFFSET],eax

    mov     ax,[si+CF16_nFontType_OFFSET]	; nFontType
    movzx   eax,ax
    mov     es:[edi+CF32_nFontType_OFFSET],eax

    mov     edi,es:[edi+CF32_lpLogFont_OFFSET]
    or      edi,edi
    jz      skip_it

    add     si,CHOOSEFONT16_SIZE

; comments refer to action going 16->32

    cld

    ncopysx 5
    ncopyd  (40/4)

skip_it:
    pushd    LOCAL__THKCHOOSEFONT_lpszStyleTemp
    call    UnmapLS
    pushd    LOCAL__THKCHOOSEFONT_lpTemplateNameTemp
    call    UnmapLS

endm

