/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            comparememory_asm.S
 * PURPOSE:         Memory functions
 * PROGRAMMERS:     Patrick Baggett (baggett.patrick@gmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 *                  Magnus Olsen (magnusolsen@greatlord.com)
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl  _RtlCompareMemory@12          // [4]  (no bug)

/* FUNCTIONS ***************************************************************/

_RtlCompareMemory@12:
     xor eax,eax       // count = 0  
     mov ecx, dword [esp + 12 ]  // ecx = Length         
	 cmp ecx,0         // if (Length==0) goto .zero
	 je 3f
      
     push edi// register that does not to be save eax,ecx,edx to 
     push ebx// the stack for protetion
        
     mov edi, dword [esp + (4 + 8)]        // edi = Destination   
     mov edx, dword [esp + (8 + 8)]        // edx = Source	 

1:
     mov bl,byte [edi + eax ]    //  if (src[count]!=des[count]) goto .pop_zero 
     cmp byte [edx + eax ],bl      
     jne 2f       

     inc eax //  count = count + 1
     dec ecx //  Length = Length - 1
     jnz 1b  //  if (Length!=0) goto .loop_1byte

2:      
     pop ebx // restore regiester 
     pop edi   
3:	  
     ret 12  // return count
