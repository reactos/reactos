        page    ,132
        .listall

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Fri Aug 23 13:58:55 1996

;Command Line: ..\..\..\dev\tools\binr\thunk.exe -NC _TEXT -o Shl1632 ..\Shl1632.thk 

        TITLE   $Shl1632.asm

        .386
        OPTION READONLY


IFNDEF IS_16
IFNDEF IS_32
%out command line error: specify one of -DIS_16 -DIS_32
.err
ENDIF
ENDIF
IFDEF IS_16
IFDEF IS_32
%out command line error: you can't specify both -DIS_16 and -DIS_32
.err
ENDIF

        OPTION SEGMENT:USE16
        .model LARGE,PASCAL

f32ptr  typedef ptr far32

externDef SH16To32Int2526:far16
externDef SH16To32DriveIOCTL:far16
externDef SHGetFileInfoA:far16
externDef DriveType:far16
externDef FindExecutable:far16
externDef ShellAbout:far16
externDef ShellExecute:far16
externDef ExtractAssociatedIcon:far16
externDef ExtractIconExA:far16
externDef ExtractIcon:far16
externDef RestartDialog:far16
externDef PickIconDlg:far16

externDef C16ThkSL01:far16
externDef __FLATCS:ABS
externDef __FLATDS:ABS


        .data

public Shl1632_ThunkData16      ;This symbol must be exported.
Shl1632_ThunkData16     dd      31304c53h       ;Protocol 'SL01'
        dd      013065h ;Checksum
        dd      0               ;Flags.
        dd      0               ;RESERVED. MUST BE ZERO.
        dd      0               ;RESERVED. MUST BE ZERO.
        dd      0               ;RESERVED. MUST BE ZERO.
        dd      0               ;RESERVED. MUST BE ZERO.
        dd      3130424ch       ;Late-binding signature 'LB01'
        dd      040000000h              ;More flags.
        dd      0               ;RESERVED. MUST BE ZERO.
        dw      offset Shl1632_ThunkData16ApiDatabase
        dw         seg Shl1632_ThunkData16ApiDatabase


;; Api database. Each entry == 8 bytes:
;;  byte  0:     # of argument bytes.
;;  byte  1,2,3: Reserved: Must initialize to 0.
;;  dword 4:     error return value.
public Shl1632_ThunkData16ApiDatabase
Shl1632_ThunkData16ApiDatabase  label   dword
        db      14
        db      0,0,0
        dd      0
        db      8
        db      0,0,0
        dd      0
        db      16
        db      0,0,0
        dd      0
        db      2
        db      0,0,0
        dd      0
        db      12
        db      0,0,0
        dd      0
        db      12
        db      0,0,0
        dd      0
        db      20
        db      0,0,0
        dd      0
        db      10
        db      0,0,0
        dd      0
        db      16
        db      0,0,0
        dd      0
        db      8
        db      0,0,0
        dd      0
        db      10
        db      0,0,0
        dd      0
        db      12
        db      0,0,0
        dd      0




        .code _TEXT


externDef ThunkConnect16:far16

public Shl1632_ThunkConnect16
Shl1632_ThunkConnect16:
        pop     ax
        pop     dx
        push    seg    Shl1632_ThunkData16
        push    offset Shl1632_ThunkData16
        push    seg    Shl1632_TD32Label
        push    offset Shl1632_TD32Label
        push    cs
        push    dx
        push    ax
        jmp     ThunkConnect16
Shl1632_TD32Label label byte
        db      "Shl1632_ThunkData32",0


SH16To32Int2526 label far16
        mov     cx,0                    ; offset in jump table
        jmp     Shl1632EntryCommon

SH16To32DriveIOCTL label far16
        mov     cx,4                    ; offset in jump table
        jmp     Shl1632EntryCommon

SHGetFileInfoA label far16
        mov     cx,8                    ; offset in jump table
        jmp     Shl1632EntryCommon

DriveType label far16
        mov     cx,12                   ; offset in jump table
        jmp     Shl1632EntryCommon

FindExecutable label far16
        mov     cx,16                   ; offset in jump table
        jmp     Shl1632EntryCommon

ShellAbout label far16
        mov     cx,20                   ; offset in jump table
        jmp     Shl1632EntryCommon

ShellExecute label far16
        mov     cx,24                   ; offset in jump table
        jmp     Shl1632EntryCommon

ExtractAssociatedIcon label far16
        mov     cx,28                   ; offset in jump table
        jmp     Shl1632EntryCommon

ExtractIconExA label far16
        mov     cx,32                   ; offset in jump table
        jmp     Shl1632EntryCommon

ExtractIcon label far16
        mov     cx,36                   ; offset in jump table
        jmp     Shl1632EntryCommon

RestartDialog label far16
        mov     cx,40                   ; offset in jump table
        jmp     Shl1632EntryCommon

PickIconDlg label far16
        mov     cx,44                   ; offset in jump table
        jmp     Shl1632EntryCommon

;===========================================================================
; This is the common setup code for 16=>32 thunks.
;
; Entry:  cx  = offset in flat jump table
;
; Don't optimize this code: C16ThkSL01 overwrites it
; after each discard.

align
Shl1632EntryCommon:
        db      0ebh, 030       ;Jump short forward 30 bytes.
;;; Leave at least 30 bytes for C16ThkSL01's code patching.
        db      30 dup(0cch)    ;Patch space.
        push    seg    Shl1632_ThunkData16
        push    offset Shl1632_ThunkData16
        pop     edx
        push    cs
        push    offset Shl1632EntryCommon
        pop     eax
        jmp     C16ThkSL01

ELSE    ; IS_32
        .model FLAT,STDCALL

include thk.inc
include Shl1632.inc

externDef STDCALL K32Thk1632Prolog@0:near32
externDef STDCALL K32Thk1632Epilog@0:near32
externDef STDCALL SH16To32Int2526@20:near32
externDef STDCALL SH16To32DriveIOCTL@12:near32
externDef STDCALL SHGetFileInfo@20:near32
externDef STDCALL DriveType@4:near32
externDef STDCALL FindExecutableA@12:near32
externDef STDCALL ShellAboutA@16:near32
externDef STDCALL ShellExecuteA@24:near32
externDef STDCALL ExtractAssociatedIconA@12:near32
externDef STDCALL ExtractIconEx@20:near32
externDef STDCALL ExtractIconA@12:near32
externDef STDCALL RestartDialog@12:near32
externDef STDCALL PickIconDlg@16:near32

externDef C DebugPrintf:near32

MapSLFix                proto   STDCALL  :DWORD
MapSL           proto   STDCALL  :DWORD
UnMapSLFixArray         proto   STDCALL  :DWORD, :DWORD
LocalAlloc      proto   STDCALL  :DWORD, :DWORD
LocalFree       proto   STDCALL  :DWORD

externDef       MapHInstSL:near32
externDef       MapHInstSL_PN:near32
externDef       MapHInstLS:near32
externDef       MapHInstLS_PN:near32
externDef T_SH16TO32INT2526:near32
externDef T_SH16TO32DRIVEIOCTL:near32
externDef T_SHGETFILEINFOA:near32
externDef T_DRIVETYPE:near32
externDef T_FINDEXECUTABLE:near32
externDef T_SHELLABOUT:near32
externDef T_SHELLEXECUTE:near32
externDef T_EXTRACTASSOCIATEDICON:near32
externDef T_EXTRACTICONEXA:near32
externDef T_EXTRACTICON:near32
externDef T_RESTARTDIALOG:near32
externDef T_PICKICONDLG:near32

;===========================================================================
        .code 


; This is a jump table to API-specific flat thunk code.

align
public Shl1632_JumpTable       ; so lego can see it
Shl1632_JumpTable label dword
        dd      offset FLAT:T_SH16TO32INT2526
        dd      offset FLAT:T_SH16TO32DRIVEIOCTL
        dd      offset FLAT:T_SHGETFILEINFOA
        dd      offset FLAT:T_DRIVETYPE
        dd      offset FLAT:T_FINDEXECUTABLE
        dd      offset FLAT:T_SHELLABOUT
        dd      offset FLAT:T_SHELLEXECUTE
        dd      offset FLAT:T_EXTRACTASSOCIATEDICON
        dd      offset FLAT:T_EXTRACTICONEXA
        dd      offset FLAT:T_EXTRACTICON
        dd      offset FLAT:T_RESTARTDIALOG
        dd      offset FLAT:T_PICKICONDLG

public Shl1632_ThunkDataName    ; so lego can see it
Shl1632_ThunkDataName label byte
        db      "Shl1632_ThunkData16",0

        .fardata
        db "Smag"               ; junk to give a non-zero address 

public Shl1632_ThunkData32      ;This symbol must be exported.
Shl1632_ThunkData32     dd      31304c53h       ;Protocol 'SL01'
        dd      013065h ;Checksum
        dd      0       ;Reserved (MUST BE 0)
        dd      0       ;Flat address of ThunkData16
        dd      3130424ch       ;'LB01'
        dd      0       ;Flags
        dd      0       ;Reserved (MUST BE 0)
        dd      0       ;Reserved (MUST BE 0)
        dd      offset Shl1632_JumpTable - offset Shl1632_ThunkDataName



        .code 


externDef ThunkConnect32@24:near32

public Shl1632_ThunkConnect32@16
Shl1632_ThunkConnect32@16:
        pop     edx
        push    offset Shl1632_ThunkDataName
        push    offset Shl1632_ThunkData32
        push    edx
        jmp     ThunkConnect32@24


;===========================================================================
; Common routines to restore the stack and registers
; and return to 16-bit code.  There is one for each
; size of 16-bit parameter list in this script.

align
ExitFlat_2:
        mov     cl,2            ; parameter byte count
        mov     esp,ebp         ; point to return address
        retn                    ; return to dispatcher

align
ExitFlat_8:
        mov     cl,8            ; parameter byte count
        mov     esp,ebp         ; point to return address
        retn                    ; return to dispatcher

align
ExitFlat_10:
        mov     cl,10           ; parameter byte count
        mov     esp,ebp         ; point to return address
        retn                    ; return to dispatcher

align
ExitFlat_12:
        mov     cl,12           ; parameter byte count
        mov     esp,ebp         ; point to return address
        retn                    ; return to dispatcher

align
ExitFlat_14:
        mov     cl,14           ; parameter byte count
        mov     esp,ebp         ; point to return address
        retn                    ; return to dispatcher

align
ExitFlat_16:
        mov     cl,16           ; parameter byte count
        mov     esp,ebp         ; point to return address
        retn                    ; return to dispatcher

align
ExitFlat_20:
        mov     cl,20           ; parameter byte count
        mov     esp,ebp         ; point to return address
        retn                    ; return to dispatcher

;===========================================================================
T_SH16TO32INT2526 label near32

; ebx+34   iDrive
; ebx+32   iInt
; ebx+28   lpbuf
; ebx+26   count
; ebx+22   ssector
        APILOGSL        SH16To32Int2526

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
; lpbuf inline mapping
;-------------------------------------
; *** BEGIN parameter packing

; lpbuf
; pointer void --> void
; inline mapping

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; ssector  from: unsigned long
        push    dword ptr [ebx+22]      ; to unsigned long

; count  from: unsigned short
        mov     ax, word ptr [ebx+26]   ; to unsigned short
        push    eax
; lpbuf  from: void
        mov     eax, [ebx+28]           ; base address
        mov     [esp + 8],eax
        push    eax
        call    MapSLFix        
        push    eax

; iInt  from: short
        movsx   eax,word ptr [ebx+32]
        push    eax                     ; to: long

; iDrive  from: short
        movsx   eax,word ptr [ebx+34]
        push    eax                     ; to: long

        call    K32Thk1632Prolog@0
        call    SH16To32Int2526@20              ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code long --> short
; no conversion needed

        lea     ecx, [esp+0]
        push    ecx
        push    dword ptr 1
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_14

;===========================================================================
T_SH16TO32DRIVEIOCTL label near32

; ebx+28   iDrive
; ebx+26   cmd
; ebx+22   pv
        APILOGSL        SH16To32DriveIOCTL

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
; pv inline mapping
;-------------------------------------
; *** BEGIN parameter packing

; pv
; pointer void --> void
; inline mapping

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; pv  from: void
        mov     eax, [ebx+22]           ; base address
        mov     [esp + 0],eax
        push    eax
        call    MapSLFix        
        push    eax

; cmd  from: short
        movsx   eax,word ptr [ebx+26]
        push    eax                     ; to: long

; iDrive  from: short
        movsx   eax,word ptr [ebx+28]
        push    eax                     ; to: long

        call    K32Thk1632Prolog@0
        call    SH16To32DriveIOCTL@12           ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code long --> short
; no conversion needed

        lea     ecx, [esp+0]
        push    ecx
        push    dword ptr 1
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_8

;===========================================================================
T_SHGETFILEINFOA label near32

; ebx+34   pszPath
; ebx+30   dwFileAttributes
; ebx+26   psfi
; ebx+24   cbFileInfo
; ebx+22   uFlags
        APILOGSL        SHGetFileInfoA

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
; pszPath inline mapping
        push    eax                     ; ptr param #2   psfi
;-------------------------------------
; *** BEGIN parameter packing

; pszPath
; pointer char --> char
; inline mapping

; psfi
; pointer struct --> struct
        cld                             ; esi, edi will increment

        sub     esp,352                 ; psfi alloc space on stack

        mov     eax,[ebx+26]            ; base address
        or      eax,eax
        jz      L0                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [esp+352],esp           ; save pointer to buffer

        mov     [esp + 356],eax
        push    eax
        call    MapSLFix        
        mov     esi,eax                 ; source flat address
        mov     edi,esp                 ; destination flat address

        xor     eax,eax
        lodsw
        stosd
        lodsw
        cwde
        stosd
        mov     ecx,86
        rep     movsd
L0:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; uFlags  from: unsigned short
        movzx   eax,word ptr [ebx+22]
        push    eax                     ; to: unsigned long

; cbFileInfo  from: unsigned short
        movzx   eax,word ptr [ebx+24]
        push    eax                     ; to: unsigned long

; psfi  from: struct
        push    dword ptr [esp+360]     ; to: struct

; dwFileAttributes  from: unsigned long
        push    dword ptr [ebx+30]      ; to unsigned long

; pszPath  from: char
        mov     eax, [ebx+34]           ; base address
        mov     [esp + 376],eax
        push    eax
        call    MapSLFix        
        push    eax

        call    K32Thk1632Prolog@0
        call    SHGetFileInfo@20                ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code unsigned long --> unsigned long
        mov     edx,eax
        rol     edx,16

;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code
        push    edx                     ; save return code

        cld                             ; esi, edi will increment

; psfi
; pointer struct --> struct
        mov     eax,[ebx+26]            ; base address
        or      eax,eax
        jz      L1                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [esp + 372],eax
        push    eax
        call    MapSLFix        
        mov     edi,eax                 ; destination flat address
        mov     esi,[esp+360]           ; source flat address

        lodsd
        stosw
        lodsd
        stosw
        mov     ecx,86
        rep     movsd
L1:
        pop     edx                     ; restore return code
        pop     eax                     ; restore return code

; *** END   parameter packing
        lea     ecx, [esp+356]
        push    ecx
        push    dword ptr 3
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_16

;===========================================================================
T_DRIVETYPE label near32

; ebx+22   iDrive
        APILOGSL        DriveType

;-------------------------------------
; create new call frame and make the call

; iDrive  from: short
        movsx   eax,word ptr [ebx+22]
        push    eax                     ; to: long

        call    K32Thk1632Prolog@0
        call    DriveType@4             ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code long --> short
; no conversion needed

;-------------------------------------
        jmp     ExitFlat_2

;===========================================================================
T_FINDEXECUTABLE label near32

; ebx+30   lpFile
; ebx+26   lpDirectory
; ebx+22   lpResult
        APILOGSL        FindExecutable

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
; lpFile inline mapping
; lpDirectory inline mapping
; lpResult inline mapping
;-------------------------------------
; *** BEGIN parameter packing

; lpFile
; pointer char --> char
; inline mapping

; lpDirectory
; pointer char --> char
; inline mapping

; lpResult
; pointer char --> char
; inline mapping

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpResult  from: char
        mov     eax, [ebx+22]           ; base address
        mov     [esp + 0],eax
        push    eax
        call    MapSLFix        
        push    eax

; lpDirectory  from: char
        mov     eax, [ebx+26]           ; base address
        mov     [esp + 8],eax
        push    eax
        call    MapSLFix        
        push    eax

; lpFile  from: char
        mov     eax, [ebx+30]           ; base address
        mov     [esp + 16],eax
        push    eax
        call    MapSLFix        
        push    eax

        call    K32Thk1632Prolog@0
        call    FindExecutableA@12              ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code unsigned long --> unsigned short
; no conversion needed

;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; lpResult
; pointer char --> char
        mov     eax,[ebx+22]            ; base address
        or      eax,eax
        jz      L2                      ; skip if null

L2:
        pop     eax                     ; restore return code

; *** END   parameter packing
        lea     ecx, [esp+0]
        push    ecx
        push    dword ptr 3
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_12

;===========================================================================
T_SHELLABOUT label near32

; ebx+32   hWnd
; ebx+28   szApp
; ebx+24   szOtherStuff
; ebx+22   hIcon
        APILOGSL        ShellAbout

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
; szApp inline mapping
; szOtherStuff inline mapping
;-------------------------------------
; *** BEGIN parameter packing

; szApp
; pointer char --> char
; inline mapping

; szOtherStuff
; pointer char --> char
; inline mapping

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; hIcon  from: unsigned short
        movzx   eax,word ptr [ebx+22]
        push    eax                     ; to: unsigned long

; szOtherStuff  from: char
        mov     eax, [ebx+24]           ; base address
        mov     [esp + 4],eax
        push    eax
        call    MapSLFix        
        push    eax

; szApp  from: char
        mov     eax, [ebx+28]           ; base address
        mov     [esp + 12],eax
        push    eax
        call    MapSLFix        
        push    eax

; hWnd  from: unsigned short
        movzx   eax,word ptr [ebx+32]
        push    eax                     ; to: unsigned long

        call    K32Thk1632Prolog@0
        call    ShellAboutA@16          ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code long --> short
; no conversion needed

        lea     ecx, [esp+0]
        push    ecx
        push    dword ptr 2
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_12

;===========================================================================
T_SHELLEXECUTE label near32

; ebx+40   hwnd
; ebx+36   lpszOp
; ebx+32   lpszFile
; ebx+28   lpszParams
; ebx+24   lpszDir
; ebx+22   wShowCmd
        APILOGSL        ShellExecute

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
; lpszOp inline mapping
; lpszFile inline mapping
; lpszParams inline mapping
; lpszDir inline mapping
;-------------------------------------
; *** BEGIN parameter packing

; lpszOp
; pointer char --> char
; inline mapping

; lpszFile
; pointer char --> char
; inline mapping

; lpszParams
; pointer char --> char
; inline mapping

; lpszDir
; pointer char --> char
; inline mapping

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; wShowCmd  from: short
        movsx   eax,word ptr [ebx+22]
        push    eax                     ; to: long

; lpszDir  from: char
        mov     eax, [ebx+24]           ; base address
        mov     [esp + 4],eax
        push    eax
        call    MapSLFix        
        push    eax

; lpszParams  from: char
        mov     eax, [ebx+28]           ; base address
        mov     [esp + 12],eax
        push    eax
        call    MapSLFix        
        push    eax

; lpszFile  from: char
        mov     eax, [ebx+32]           ; base address
        mov     [esp + 20],eax
        push    eax
        call    MapSLFix        
        push    eax

; lpszOp  from: char
        mov     eax, [ebx+36]           ; base address
        mov     [esp + 28],eax
        push    eax
        call    MapSLFix        
        push    eax

; hwnd  from: unsigned short
        movzx   eax,word ptr [ebx+40]
        push    eax                     ; to: unsigned long

        call    K32Thk1632Prolog@0
        call    ShellExecuteA@24                ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code unsigned long --> unsigned short
; no conversion needed

        lea     ecx, [esp+0]
        push    ecx
        push    dword ptr 4
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_20

;===========================================================================
T_EXTRACTASSOCIATEDICON label near32

; ebx+30   hInst
; ebx+26   lpIconPath
; ebx+22   lpiIcon
        APILOGSL        ExtractAssociatedIcon

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
; lpIconPath inline mapping
        push    eax                     ; ptr param #2   lpiIcon
;-------------------------------------
; *** BEGIN parameter packing

; lpIconPath
; pointer char --> char
; inline mapping

; lpiIcon
; pointer short --> long
        sub     esp,4                   ; lpiIcon alloc space on stack

        mov     eax,[ebx+22]            ; base address
        or      eax,eax
        jz      L3                      ; skip if null

        mov     [esp + 8],eax
        push    eax
        call    MapSLFix        
        movsx   eax,word ptr [eax]
        mov     [esp],eax
        mov     [esp+4],esp             ; save pointer to flat storage
L3:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpiIcon  from: short
        push    dword ptr [esp+4]       ; to: long

; lpIconPath  from: char
        mov     eax, [ebx+26]           ; base address
        mov     [esp + 16],eax
        push    eax
        call    MapSLFix        
        push    eax

; hInst  from: unsigned short
        movzx   eax,word ptr [ebx+30]
        push    eax                     ; to: unsigned long

        call    K32Thk1632Prolog@0
        call    ExtractAssociatedIconA@12               ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code unsigned long --> unsigned short
; no conversion needed

;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; lpIconPath
; pointer char --> char
        mov     eax,[ebx+26]            ; base address
        or      eax,eax
        jz      L4                      ; skip if null

L4:
; lpiIcon
; pointer long --> short
        mov     eax,[ebx+22]            ; base address
        or      eax,eax
        jz      L5                      ; skip if null

        mov     eax,[esp+8]             ; get the flat address
        mov     edi,[eax]               ; get the flat data
        mov     eax,dword ptr [ebx+22]
        mov     [esp + 20],eax
        push    eax
        call    MapSLFix        
        mov     word ptr [eax],di       ; copy out
L5:
        pop     eax                     ; restore return code

; *** END   parameter packing
        lea     ecx, [esp+8]
        push    ecx
        push    dword ptr 3
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_10

;===========================================================================
T_EXTRACTICONEXA label near32

; ebx+34   lpszExeFileName
; ebx+32   nIconIndex
; ebx+28   phiconLarge
; ebx+24   phiconSmall
; ebx+22   nIcons
        APILOGSL        ExtractIconExA

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
; lpszExeFileName inline mapping
        push    eax                     ; ptr param #2   phiconLarge
        push    eax                     ; ptr param #3   phiconSmall
;-------------------------------------
; *** BEGIN parameter packing

; lpszExeFileName
; pointer char --> char
; inline mapping

; phiconLarge
; pointer unsigned short --> unsigned long
        sub     esp,4                   ; phiconLarge alloc space on stack

        mov     eax,[ebx+28]            ; base address
        or      eax,eax
        jz      L6                      ; skip if null

        mov     [esp+8],esp             ; save pointer to flat storage
L6:

; phiconSmall
; pointer unsigned short --> unsigned long
        sub     esp,4                   ; phiconSmall alloc space on stack

        mov     eax,[ebx+24]            ; base address
        or      eax,eax
        jz      L7                      ; skip if null

        mov     [esp+8],esp             ; save pointer to flat storage
L7:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; nIcons  from: short
        movsx   eax,word ptr [ebx+22]
        push    eax                     ; to: long

; phiconSmall  from: unsigned short
        push    dword ptr [esp+12]      ; to: unsigned long

; phiconLarge  from: unsigned short
        push    dword ptr [esp+20]      ; to: unsigned long

; nIconIndex  from: short
        movsx   eax,word ptr [ebx+32]
        push    eax                     ; to: long

; lpszExeFileName  from: char
        mov     eax, [ebx+34]           ; base address
        mov     [esp + 32],eax
        push    eax
        call    MapSLFix        
        push    eax

        call    K32Thk1632Prolog@0
        call    ExtractIconEx@20                ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code unsigned long --> unsigned short
; no conversion needed

;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; phiconLarge
; pointer unsigned long --> unsigned short
        mov     eax,[ebx+28]            ; base address
        or      eax,eax
        jz      L8                      ; skip if null

        mov     eax,[esp+16]            ; get the flat address
        mov     edi,[eax]               ; get the flat data
        mov     eax,dword ptr [ebx+28]
        mov     [esp + 24],eax
        push    eax
        call    MapSLFix        
        mov     word ptr [eax],di       ; copy out
L8:
; phiconSmall
; pointer unsigned long --> unsigned short
        mov     eax,[ebx+24]            ; base address
        or      eax,eax
        jz      L9                      ; skip if null

        mov     eax,[esp+12]            ; get the flat address
        mov     edi,[eax]               ; get the flat data
        mov     eax,dword ptr [ebx+24]
        mov     [esp + 28],eax
        push    eax
        call    MapSLFix        
        mov     word ptr [eax],di       ; copy out
L9:
        pop     eax                     ; restore return code

; *** END   parameter packing
        lea     ecx, [esp+16]
        push    ecx
        push    dword ptr 3
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_16

;===========================================================================
T_EXTRACTICON label near32

; ebx+28   hInst
; ebx+24   lpszExeFileName
; ebx+22   nIconIndex
        APILOGSL        ExtractIcon

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
; lpszExeFileName inline mapping
;-------------------------------------
; *** BEGIN parameter packing

; lpszExeFileName
; pointer char --> char
; inline mapping

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; nIconIndex  from: short
        movsx   eax,word ptr [ebx+22]
        push    eax                     ; to: long

; lpszExeFileName  from: char
        mov     eax, [ebx+24]           ; base address
        mov     [esp + 4],eax
        push    eax
        call    MapSLFix        
        push    eax

; hInst  from: unsigned short
        movzx   eax,word ptr [ebx+28]
        push    eax                     ; to: unsigned long

        call    K32Thk1632Prolog@0
        call    ExtractIconA@12         ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code unsigned long --> unsigned short
; no conversion needed

        lea     ecx, [esp+0]
        push    ecx
        push    dword ptr 1
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_8

;===========================================================================
T_RESTARTDIALOG label near32

; ebx+30   hwnd
; ebx+26   lpPrompt
; ebx+22   dwReturn
        APILOGSL        RestartDialog

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
; lpPrompt inline mapping
;-------------------------------------
; *** BEGIN parameter packing

; lpPrompt
; pointer char --> char
; inline mapping

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; dwReturn  from: unsigned long
        push    dword ptr [ebx+22]      ; to unsigned long

; lpPrompt  from: char
        mov     eax, [ebx+26]           ; base address
        mov     [esp + 4],eax
        push    eax
        call    MapSLFix        
        push    eax

; hwnd  from: unsigned short
        movzx   eax,word ptr [ebx+30]
        push    eax                     ; to: unsigned long

        call    K32Thk1632Prolog@0
        call    RestartDialog@12                ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code long --> short
; no conversion needed

        lea     ecx, [esp+0]
        push    ecx
        push    dword ptr 1
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_10

;===========================================================================
T_PICKICONDLG label near32

; ebx+32   hwnd
; ebx+28   pszIconPath
; ebx+26   cbIconPath
; ebx+22   piIconIndex
        APILOGSL        PickIconDlg

;-------------------------------------
; temp storage

        xor     eax,eax
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
        push    eax     ;MapLS unfix temp
; pszIconPath inline mapping
        push    eax                     ; ptr param #2   piIconIndex
;-------------------------------------
; *** BEGIN parameter packing

; pszIconPath
; pointer char --> char
; inline mapping

; piIconIndex
; pointer short --> long
        sub     esp,4                   ; piIconIndex alloc space on stack

        mov     eax,[ebx+22]            ; base address
        or      eax,eax
        jz      L10                     ; skip if null

        mov     [esp + 8],eax
        push    eax
        call    MapSLFix        
        movsx   eax,word ptr [eax]
        mov     [esp],eax
        mov     [esp+4],esp             ; save pointer to flat storage
L10:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; piIconIndex  from: short
        push    dword ptr [esp+4]       ; to: long

; cbIconPath  from: unsigned short
        movzx   eax,word ptr [ebx+26]
        push    eax                     ; to: unsigned long

; pszIconPath  from: char
        mov     eax, [ebx+28]           ; base address
        mov     [esp + 20],eax
        push    eax
        call    MapSLFix        
        push    eax

; hwnd  from: unsigned short
        movzx   eax,word ptr [ebx+32]
        push    eax                     ; to: unsigned long

        call    K32Thk1632Prolog@0
        call    PickIconDlg@16          ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code long --> short
; no conversion needed

;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; pszIconPath
; pointer char --> char
        mov     eax,[ebx+28]            ; base address
        or      eax,eax
        jz      L11                     ; skip if null

L11:
; piIconIndex
; pointer long --> short
        mov     eax,[ebx+22]            ; base address
        or      eax,eax
        jz      L12                     ; skip if null

        mov     eax,[esp+8]             ; get the flat address
        mov     edi,[eax]               ; get the flat data
        mov     eax,dword ptr [ebx+22]
        mov     [esp + 20],eax
        push    eax
        call    MapSLFix        
        mov     word ptr [eax],di       ; copy out
L12:
        pop     eax                     ; restore return code

; *** END   parameter packing
        lea     ecx, [esp+8]
        push    ecx
        push    dword ptr 3
        call    UnMapSLFixArray ;! Preserves eax & edx
;-------------------------------------
        jmp     ExitFlat_12

ENDIF
END
