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
        GLOBAL _RtlFillMemory@12      ; [4]  (no bug)  
        
        SECTION .text

; *
; * [6] VOID STDCALL RtlFillMemory (PVOID Destination, ULONG Length, UCHAR Fill)
; * 

_RtlFillMemory@12:
      mov ecx,dword [esp + 8 ]      ; ecx = Length                                       
	cmp ecx,0                     ; if (Length==0) goto .zero 
	je  .zero	        

      mov edx, dword [esp + 4]      ; edx = Destination           
      mov eax, dword [esp + 12]     ; eax = fill         	        
.loop:     
      mov	byte [edx + ecx -1],al  ; src[Length - 1] = fill
      dec ecx                       ; Length = Length - 1
      jnz .loop                     ; if (Length!=0) goto .loop
.zero:	           	
      ret 12                        ; return
