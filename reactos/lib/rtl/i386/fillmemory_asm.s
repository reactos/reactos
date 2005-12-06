/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            fillmemory_asm.S
 * PURPOSE:         Memory functions
 * PROGRAMMERS:     Patrick Baggett (baggett.patrick@gmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 *                  Magnus Olsen (magnusolsen@greatlord.com)
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl  _RtlFillMemory@12          //[4]  (no bug)

/* FUNCTIONS ***************************************************************/

_RtlFillMemory@12:
    mov ecx,dword [esp + 8 ]     // ecx = Length         
	cmp ecx,0// if (Length==0) goto .zero 
	je  2f	        

    mov edx, dword [esp + 4]     // edx = Destination 
    mov eax, dword [esp + 12]    // eax = fill         	        
1:     
    mov	byte [edx + ecx -1],al   // src[Length - 1] = fill
    dec ecx  // Length = Length - 1
    jnz 1b   // if (Length!=0) goto .loop
2:	 	
    ret 12   // return
