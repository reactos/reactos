; * base on ntdll/rtl/mem.c v 1.13 2003/07/11 13:50:23 
; * 
; * COPYRIGHT:       See COPYING in the top level directory
; * PROJECT:         ReactOS kernel
; * FILE:            i386_RtlMemoryUlong.asm
; * PURPOSE:         Memory functions
; * PROGRAMMER:      Magnus Olsen (magnusolsen@greatlord.com)
; * UPDATE HISTORY:
; *                  Created 20/07-2003
; * 

  

        BITS 32
        GLOBAL _RtlFillMemoryUlong@12    ; (no bug) (max optimze code)  
        
        SECTION .text




; * 
; * VOID STDCALL RtlFillMemoryUlong (PVOID Destination, ULONG Length, ULONG Fill)
; * 

_RtlFillMemoryUlong@12:
        mov ecx, dword [esp + 8 ]                   ; Length                                       
        shr ecx,2                                   ; Length = Length / sizeof(ULONG) 
	  jz .zero                                    ; if (Length==0) goto .zero
         
        push edi
        mov edi, dword [esp + (4 + 4)]             ; Destination
        mov eax, dword [esp + (12 + 4)]            ; Fill       
        cld                     	          
        rep stosd                                  ; while (Length>0) {Destination[Length-1]=Fill; Length = Length - 1}
        pop edi
.zero:
        ret 12
