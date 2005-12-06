/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            comparememory_ulong_asm.S
 * PURPOSE:         Memory functions
 * PROGRAMMERS:     Patrick Baggett (baggett.patrick@gmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 *                  Magnus Olsen (magnusolsen@greatlord.com)
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl  _RtlCompareMemoryUlong@12          // [5]  (no bug)

/* FUNCTIONS ***************************************************************/

_RtlCompareMemoryUlong@12:
     xor eax,eax
     mov ecx, dword [esp + 8 ]   // ecx = Length      
     shr ecx,2         // Length / sizeof(ULONG) 
	 jz 1f   // if (Length==0) goto .zero 
        
     push edi// register that does not to be save eax,ecx,edx to 
     push ebx// the stack for protetion
         
     mov edi, dword [esp + (4 + 8)]        // edx = Destination
     mov eax, dword [esp + (12 + 8)]       // ebx = value       
     mov ebx,ecx
     cld
     repe scasd

     inc ecx
     mov eax,ebx
  
     sub eax,ecx
     shl eax,2
       
     pop ebx
     pop edi

1:
     ret 12
