; * base on ntdll/rtl/mem.c v 1.13 2003/07/11 13:50:23 
; * 
; * COPYRIGHT:       See COPYING in the top level directory
; * PROJECT:         ReactOS kernel
; * FILE:            i386_RtlCompareMemoryUlong.asm
; * PURPOSE:         Memory functions
; * PROGRAMMER:      Magnus Olsen (magnusolsen@greatlord.com)
; * UPDATE HISTORY:
; *                  Created 20/07-2003
; * 

  


	  BITS 32
        GLOBAL _RtlCompareMemoryUlong@12 ; [5]  (no bug)  
               
	  SECTION .text
;*
;* [5] ULONG STDCALL RtlCompareMemoryUlong (PVOID Source, ULONG	Length, ULONG Value)
;*

_RtlCompareMemoryUlong@12:        
        xor eax,eax
        mov ecx, dword [esp + 8 ]  ; ecx = Length                                              
        shr ecx,2                  ; Length / sizeof(ULONG) 
	  jz .zero                   ; if (Length==0) goto .zero 
        
        push edi                   ; register that does not to be save eax,ecx,edx to 
        push ebx                   ; the stack for protetion
         
        mov edi, dword [esp + (4 + 8)]   ; edx = Destination
        mov	eax, dword [esp + (12 + 8)]  ; ebx = value                 
        mov ebx,ecx
        cld
        repe scasd

        inc ecx
        mov eax,ebx
  
        sub eax,ecx
        shl eax,2
       
        pop ebx
        pop edi

.zero
        ret 12

        