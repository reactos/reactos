        title  "ioaccess"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ioaccess.asm
;
; Abstract:
;
;    Procedures to correctly touch I/O registers.
;
; Author:
;
;    Bryan Willman (bryanwi) 16 May 1990
;
; Environment:
;
;    User or Kernel, although privledge (IOPL) may be required.
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
;++
;
; I/O memory space read and write functions.
;
;  These have to be actual functions on the 386, because we need
;  to use assembler, but cannot return a value if we inline it.
;
;  This set of functions manipulates I/O registers in MEMORY space.
;  (Uses x86 mov instructions)
;
;--



;++
;
;   UCHAR
;   READ_REGISTER_UCHAR(
;       PUCHAR  Register
;       )
;
;   Memory space references will use lock prefix to force real access,
;   flush through posted write buffers, and so forth.
;
;   Arguments:
;       (esp+4) = Register
;
;   Returns:
;       Value in register.
;
;--
cPublicProc _READ_REGISTER_UCHAR ,1
cPublicFpo 1,0

        mov     edx,[esp+4]             ; (edx) = Register
        mov     al,[edx]                ; (al) = byte, lock forces real access
        stdRET    _READ_REGISTER_UCHAR

stdENDP _READ_REGISTER_UCHAR



;++
;
;   USHORT
;   READ_REGISTER_USHORT(
;       PUSHORT Register
;       )
;
;   Memory space references will use lock prefix to force real access,
;   flush through posted write buffers, and so forth.
;
;   Arguments:
;       (esp+4) = Register
;
;   Returns:
;       Value in register.
;
;--
cPublicProc _READ_REGISTER_USHORT ,1
cPublicFpo 1,0

        mov     edx,[esp+4]             ; (edx) = Register
        mov     ax,[edx]                ; (ax) = word, lock forces real access
        stdRET    _READ_REGISTER_USHORT

stdENDP _READ_REGISTER_USHORT



;++
;
;   ULONG
;   READ_REGISTER_ULONG(
;       PULONG  Register
;       )
;
;   Memory space references will use lock prefix to force real access,
;   flush through posted write buffers, and so forth.
;
;   Arguments:
;       (esp+4) = Register
;
;   Returns:
;       Value in register.
;
;--
cPublicProc _READ_REGISTER_ULONG ,1
cPublicFpo 1,0

        mov     edx,[esp+4]             ; (edx) = Register
        mov     eax,[edx]               ; (eax) = dword, lock forces real access
        stdRET    _READ_REGISTER_ULONG

stdENDP _READ_REGISTER_ULONG


;++
;
;   VOID
;   READ_REGISTER_BUFFER_UCHAR(
;       PUCHAR  Register,
;       PUCHAR  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc _READ_REGISTER_BUFFER_UCHAR ,3
cPublicFpo 3,0

        mov     eax, esi
        mov     edx, edi                ; Save esi, edi

        mov     ecx,[esp+12]            ; (ecx) = transfer count
        mov     esi,[esp+4]             ; (edx) = Register
        mov     edi,[esp+8]             ; (edi) = buffer
    rep movsb

        mov     edi, edx
        mov     esi, eax

        stdRET    _READ_REGISTER_BUFFER_UCHAR

stdENDP _READ_REGISTER_BUFFER_UCHAR


;++
;
;   VOID
;   READ_REGISTER_BUFFER_USHORT(
;       PUSHORT Register,
;       PUSHORT Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc _READ_REGISTER_BUFFER_USHORT ,3
cPublicFpo 3,0

        mov     eax, esi
        mov     edx, edi                ; Save esi, edi

        mov     ecx,[esp+12]            ; (ecx) = transfer count
        mov     esi,[esp+4]             ; (edx) = Register
        mov     edi,[esp+8]             ; (edi) = buffer
    rep movsw

        mov     edi, edx
        mov     esi, eax
        stdRET    _READ_REGISTER_BUFFER_USHORT

stdENDP _READ_REGISTER_BUFFER_USHORT


;++
;
;   VOID
;   READ_REGISTER_BUFFER_ULONG(
;       PULONG  Register,
;       PULONG  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc _READ_REGISTER_BUFFER_ULONG ,3
cPublicFpo 3,0

        mov     eax, esi
        mov     edx, edi                ; Save esi, edi

        mov     ecx,[esp+12]            ; (ecx) = transfer count
        mov     esi,[esp+4]             ; (edx) = Register
        mov     edi,[esp+8]             ; (edi) = buffer
    rep movsd

        mov     edi, edx
        mov     esi, eax
        stdRET    _READ_REGISTER_BUFFER_ULONG

stdENDP _READ_REGISTER_BUFFER_ULONG



;++
;
;   VOID
;   WRITE_REGISTER_UCHAR(
;       PUCHAR  Register,
;       UCHAR   Value
;       )
;
;   Memory space references will use lock prefix to force real access,
;   flush through posted write buffers, and so forth.
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Value
;
;--
cPublicProc _WRITE_REGISTER_UCHAR ,2
cPublicFpo 2,0

        mov     edx,[esp+4]             ; (edx) = Register
        mov     al,[esp+8]              ; (al) = Value
        mov     [edx],al                ; do write
   lock or      [esp+4],edx     ; flush processors posted-write buffers
        stdRET    _WRITE_REGISTER_UCHAR

stdENDP _WRITE_REGISTER_UCHAR



;++
;
;   VOID
;   WRITE_REGISTER_USHORT(
;       PUSHORT Register,
;       USHORT  Value
;       )
;
;   Memory space references will use lock prefix to force real access,
;   flush through posted write buffers, and so forth.
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Value
;
;--
cPublicProc _WRITE_REGISTER_USHORT ,2
cPublicFpo 2,0

        mov     edx,[esp+4]             ; (edx) = Register
        mov     eax,[esp+8]             ; (ax) = Value
        mov     [edx],ax                ; do write
   lock or      [esp+4],edx     ; flush processors posted-write buffers
        stdRET    _WRITE_REGISTER_USHORT

stdENDP _WRITE_REGISTER_USHORT



;++
;
;   VOID
;   WRITE_REGISTER_ULONG(
;       PULONG  Register,
;       ULONG   Value
;       )
;
;   Memory space references will use lock prefix to force real access,
;   flush through posted write buffers, and so forth.
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Value
;
;--
cPublicProc _WRITE_REGISTER_ULONG ,2
cPublicFpo 2,0

        mov     edx,[esp+4]             ; (edx) = Register
        mov     eax,[esp+8]             ; (eax) = Value
        mov     [edx],eax               ; do write
   lock or      [esp+4],edx     ; flush processors posted-write buffers
        stdRET    _WRITE_REGISTER_ULONG

stdENDP _WRITE_REGISTER_ULONG


;++
;
;   VOID
;   WRITE_REGISTER_BUFFER_UCHAR(
;       PUCHAR  Register,
;       PUCHAR  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc _WRITE_REGISTER_BUFFER_UCHAR ,3
cPublicFpo 3,0

        mov     eax, esi
        mov     edx, edi                ; Save esi, edi

        mov     ecx,[esp+12]            ; (ecx) = transfer count
        mov     esi,[esp+8]             ; (edi) = buffer
        mov     edi,[esp+4]             ; (edx) = Register
    rep movsb
   lock or      [esp+4],ecx     ; flush processors posted-write buffers

        mov     edi, edx
        mov     esi, eax

        stdRET    _WRITE_REGISTER_BUFFER_UCHAR

stdENDP _WRITE_REGISTER_BUFFER_UCHAR


;++
;
;   VOID
;   WRITE_REGISTER_BUFFER_USHORT(
;       PUSHORT Register,
;       PUSHORT Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc _WRITE_REGISTER_BUFFER_USHORT ,3
cPublicFpo 3,0

        mov     eax, esi
        mov     edx, edi                ; Save esi, edi

        mov     ecx,[esp+12]            ; (ecx) = transfer count
        mov     esi,[esp+8]             ; (edi) = buffer
        mov     edi,[esp+4]             ; (edx) = Register
    rep movsw
   lock or      [esp+4],ecx     ; flush processors posted-write buffers

        mov     edi, edx
        mov     esi, eax
        stdRET    _WRITE_REGISTER_BUFFER_USHORT

stdENDP _WRITE_REGISTER_BUFFER_USHORT


;++
;
;   VOID
;   WRITE_REGISTER_BUFFER_ULONG(
;       PULONG  Register,
;       PULONG  Buffer,
;       ULONG   Count
;       )
;
;   Arguments:
;       (esp+4) = Register
;       (esp+8) = Buffer address
;       (esp+12) = Count
;
;--
cPublicProc _WRITE_REGISTER_BUFFER_ULONG ,3
cPublicFpo  0, 3

;FPO ( 0, 3, 0, 0, 0, 0 )

        mov     eax, esi
        mov     edx, edi                ; Save esi, edi

        mov     ecx,[esp+12]            ; (ecx) = transfer count
        mov     esi,[esp+8]             ; (edi) = buffer
        mov     edi,[esp+4]             ; (edx) = Register
    rep movsd
   lock or      [esp+4],ecx     ; flush processors posted-write buffers

        mov     edi, edx
        mov     esi, eax
        stdRET    _WRITE_REGISTER_BUFFER_ULONG

stdENDP _WRITE_REGISTER_BUFFER_ULONG

_TEXT   ends
        end
