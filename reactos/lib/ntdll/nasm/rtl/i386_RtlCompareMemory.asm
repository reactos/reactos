; * base on ntdll/rtl/mem.c v 1.13 2003/07/11 13:50:23 
; * 
; * COPYRIGHT:       See COPYING in the top level directory
; * PROJECT:         ReactOS kernel
; * FILE:            i386_RtlCompareMemory.asm
; * PURPOSE:         Memory functions
; * PROGRAMMER:      Magnus Olsen (magnusolsen@greatlord.com)
; * UPDATE HISTORY:
; *                  Created 20/07-2003
; * 
                        
  


	  BITS 32
        GLOBAL _RtlCompareMemory@12      ; [4]  (no bug)  
        
        SECTION .text

; *
; * [4] ULONG STDCALL RtlCompareMemory(PVOID Source1, PVOID Source2, ULONG Length)
; *

_RtlCompareMemory@12:
      xor eax,eax                    ; count = 0  
      mov ecx, dword [esp + 12 ]     ; ecx = Length                                       
	cmp ecx,0                      ; if (Length==0) goto .zero
	je  .zero
      
      push edi                       ; register that does not to be save eax,ecx,edx to 
      push ebx                       ; the stack for protetion
                            
      mov edi, dword [esp + (4 + 8)] ; edi = Destination                       
      mov edx, dword [esp + (8 + 8)] ; edx = Source	                     

.loop_1byte:
      mov bl,byte [edi + eax ]      ;  if (src[count]!=des[count]) goto .pop_zero           
      cmp byte [edx + eax ],bl      
      jne .pop_zero                 

      inc eax                       ;  count = count + 1
      dec ecx                       ;  Length = Length - 1
      jnz .loop_1byte               ;  if (Length!=0) goto .loop_1byte

.pop_zero:      
      pop ebx                       ; restore regiester 
      pop edi                       
.zero:	  
      ret 12                        ; return count
