/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            fillmemory_ulong_asm.S
 * PURPOSE:         Memory functions
 * PROGRAMMERS:     Patrick Baggett (baggett.patrick@gmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 *                  Magnus Olsen (magnusolsen@greatlord.com)
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl  _RtlFillMemoryUlong@12          // (no bug) (max optimze code)

/* FUNCTIONS ***************************************************************/

_RtlFillMemoryUlong@12:
    mov ecx, dword [esp + 8 ]    // Length         
    shr ecx,2// Length = Length / sizeof(ULONG) 
	jz 1f    // if (Length==0) goto .zero
         
    push edi
    mov edi, dword [esp + (4 + 4)]         // Destination
    mov eax, dword [esp + (12 + 4)]        // Fill       
    cld 	
    rep stosd// while (Length>0) {Destination[Length-1]=Fill// Length = Length - 1}
    pop edi
1:
    ret 12
