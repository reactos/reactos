/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:  mem.asm
 * PURPOSE:         Memory functions
 * PROGRAMMER:      Magnus Olsen (magnusolsen@greatlord.com)
 * UPDATE HISTORY:
 *        Created 27/07/2005
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl  _RtlZeroMemory@8          // (no bug) (max optimze code)
.globl  _RtlFillMemoryUlong@12    // (no bug) (max optimze code)
.globl  _RtlFillMemory@12         // [4]  (no bug)
.globl  _RtlCompareMemoryUlong@12 // [5]  (no bug)  
.globl  _RtlCompareMemory@12      // [4]  (no bug)

/* FUNCTIONS ***************************************************************/

_RtlZeroMemory@8:     
    mov ecx,dword [esp + 8 ]     // Length     
    cmp ecx,0// if (Length==0) goto .zero  
	je 3f	        

    pushad   // Save all register on the stack      
    mov edi, dword [esp + (4 + 32)]        // Destination 
    xor eax,eax        // ZeroFillByte = 0

// code for take four byte each time it loop
    mov ebx,ecx        // temp_Length = Length    
    shr ecx,2// Length = Length / sizeof(ULONG)      
    jz 1f    // if (Length==0) goto .1byte
       
    shl ecx,2// Length = Length * sizeof(ULONG)  
    sub ebx,ecx        // temp_Length = temp_Length - Length// 
    jz 2f    // if (temp_Length==0) goto .4byte

// move 4byte and 1byte
    shr ecx,2// Length = Length / sizeof(ULONG)      
    cld      // clear d flag 
    rep stosd// while (Length!=0) { (ULONG *) Destination[Length-1]=ZeroFillByte// Legnth = Legnth - 1 }  
    mov ecx,ebx        // Length = temp_Length 
    rep stosb// while (Length!=0) { (UCHAR *) Destination[Length-1]=ZeroFillByte// Legnth = Legnth - 1 }  
    popad    // restore register 
    ret 8    // return  
     
// move 1byte  
1: 
    mov ecx,dword [esp + (12 +32) ]        // Length       
    cld      // clear d flag 
    rep stosb// while (Length!=0) { (UCHAR *) Destination[Length-1]=ZeroFillByte// Legnth = Legnth - 1 }
    popad    // restore register
    ret 8    // return

// move 4bytes     
2:
    shr ecx,2// Length = Length / sizeof(ULONG)      
    cld      // clear d flag 
    rep stosd// while (Length!=0) { (ULONG *) Destination[Length-1]=ZeroFillByte// Legnth = Legnth - 1 } 
    popad    // restore register 
3:
    ret 8    // return

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
