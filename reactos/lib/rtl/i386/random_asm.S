/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:  ranom.asm
 * PURPOSE:         Random number generator functions
 * PROGRAMMER:      Magnus Olsen (magnusolsen@greatlord.com)
 * UPDATE HISTORY:
 *        Created 27/07/2005
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl _RtlRandom@4	    // [1] (no bug) (max optimze code)
.globl _RtlUniform@4	// [2] (no bug) (max optimze code)
.globl _SavedValue      // [3] (no bug) (max optimze code)

/* FUNCTIONS ***************************************************************/

_RtlRandom@4:   
    // load pointer    
    mov ecx,[esp+4]    
    mov eax,[ecx]

    // Result = *Seed * 0xffffffed + 0x7fffffc3 // take now 3 cycles  
    lea	edx,[eax + eax * 8]

    lea eax,[eax + edx * 2 + 2147483709 ]  //  + 2147483709   
    neg	eax

    cmp	eax,-1
 	je	.RtlRandom_Rand1
	cmp	eax, 2147483646
	je	.RtlRandom_Rand1

	cmp	eax, 2147483647
	je	.RtlRandom_Rand2
    
    test eax,eax
    jns .RtlRandom_Rand3

    // else {    
	mov	edx,eax
	and	edx,1
	add	eax,edx
    and	eax,2147483647     
    mov [ecx],eax	       
    jmp .RtlRandom_Seed      
    
.RtlRandom_Rand1:
    // if (Result == 0xffffffff || Result == 0x7ffffffe)
    add eax, 2
    and eax, 2147483647    
    mov [ecx],eax	       
    jmp .RtlRandom_Seed      

.RtlRandom_Rand2:
    // else if (Result == 0x7fffffff)
    xor eax,eax    
    mov [ecx],eax	       
    jmp .RtlRandom_Seed

.RtlRandom_Rand3:    
    // else if ((Result & 0x80000000) == 0)
    mov  edx,eax
    xor edx,-1  // not edx  lock all clock until it finsish, but xor does not do that
    and edx,1
    add eax,edx
    mov [ecx],eax	       
        
.RtlRandom_Seed:   
    mov eax,[ecx]    

    // Result = *Seed * 0xffffffed + 0x7fffffc3 // take now 3 cycles  
    lea	edx,[eax + eax * 8]

    lea eax,[eax + edx * 2 + 2147483709 ]  //  + 2147483709   09-18
    neg	eax

    cmp	eax,-1
 	je	.RtlRandom_Seed1
	cmp	eax, 2147483646
	je	.RtlRandom_Seed1

	cmp	eax, 2147483647
	je	.RtlRandom_Seed2
    
    test eax,eax
    jns .RtlRandom_Seed3

    // else {    
    mov	edx,eax
	and	edx,1
	add	eax,edx
    and	eax,2147483647     

    // end 
    mov edx,[ecx]
    mov [ecx],eax 
    
    mov ecx,eax // pos
    and ecx, 0x7f // pos = seed & 0x7f    
    mov eax,ecx//       
    mov eax, [_SavedValue + ecx*4]
    mov [_SavedValue + ecx*4], edx       
    ret 4
         
.RtlRandom_Seed1:
    // if (Result == 0xffffffff || Result == 0x7ffffffe)
    add eax, 2
    and eax, 2147483647    

    // end 
    mov edx,[ecx]
    mov [ecx],eax 
    
    mov ecx,eax // pos
    and ecx, 0x7f // pos = seed & 0x7f    
    mov eax,ecx//       
    mov eax, [_SavedValue + ecx*4]    
    mov [_SavedValue + ecx*4], edx       
    ret 4

.RtlRandom_Seed2:
    // else if (Result == 0x7fffffff)
    xor eax,eax    

    // end 
    mov edx,[ecx]
    mov [ecx],eax 
    
    mov ecx,eax // pos
    and ecx, 0x7f // pos = seed & 0x7f    
    mov eax,ecx//       
    mov eax, [_SavedValue + ecx*4]    
    mov [_SavedValue + ecx*4], edx       
    ret 4
      
.RtlRandom_Seed3:    
    // else if ((Result & 0x80000000) == 0)
    mov  edx,eax
    xor edx,-1  // not edx  lock all clock until it finsish, but xor does not do that
    and edx,1
    add eax,edx	       
    
    // end 
    mov edx,[ecx]
    mov [ecx],eax 
    
    mov ecx,eax // pos
    and ecx, 0x7f // pos = seed & 0x7f    
    mov eax,ecx//       
    mov eax, [_SavedValue + ecx*4]    
    mov [_SavedValue + ecx*4], edx       
    ret 4

// prototype: ULONG STDCALL RtlUniform (PULONG Seed)
_RtlUniform@4:
    // load pointer
    mov ecx,[esp+4]
    mov eax,[ecx]
    
    // Result = *Seed * 0xffffffed + 0x7fffffc3 // take now 3 cycles  
    lea	edx,[eax + eax * 8]

    lea eax,[eax + edx * 2 + 2147483709 ]  //  + 2147483709   09-18
    neg	eax

    cmp	eax,-1
 	je	.RtlUniform_jump1
	cmp	eax, 2147483646
	je	.RtlUniform_jump1

	cmp	eax, 2147483647
	je	.RtlUniform_jump2
    
    test eax,eax
    jns .RtlUniform_jump3

    // else {    
	mov	edx,eax
	and	edx,1
	add	eax,edx
    and	eax,2147483647     
    mov [ecx],eax	       
    ret 4      
   
.RtlUniform_jump1:
    // if (Result == 0xffffffff || Result == 0x7ffffffe)
    add eax, 2
    and eax, 2147483647    
    mov [ecx],eax	       
    ret 4      

.RtlUniform_jump2:
    // else if (Result == 0x7fffffff)
    xor eax,eax    
    mov [ecx],eax	       
    ret 4

.RtlUniform_jump3:    
    // else if ((Result & 0x80000000) == 0)
    mov  edx,eax
    xor edx,-1  // not edx  lock all clock until it finsish, but xor does not do that
    and edx,1
    add eax,edx
    mov [ecx],eax	       
    ret 4

.data
// SavedValue[128] 
_SavedValue:	
    .long 0x4c8bc0aa, 0x4c022957, 0x2232827a, 0x2f1e7626
  	.long 0x7f8bdafb, 0x5c37d02a, 0x0ab48f72, 0x2f0c4ffa
  	.long 0x290e1954, 0x6b635f23, 0x5d3885c0, 0x74b49ff8
  	.long 0x5155fa54, 0x6214ad3f, 0x111e9c29, 0x242a3a09
  	.long 0x75932ae1, 0x40ac432e, 0x54f7ba7a, 0x585ccbd5
  	.long 0x6df5c727, 0x0374dad1, 0x7112b3f1, 0x735fc311
  	.long 0x404331a9, 0x74d97781, 0x64495118, 0x323e04be
  	.long 0x5974b425, 0x4862e393, 0x62389c1d, 0x28a68b82
  	.long 0x0f95da37, 0x7a50bbc6, 0x09b0091c, 0x22cdb7b4
  	.long 0x4faaed26, 0x66417ccd, 0x189e4bfa, 0x1ce4e8dd
 	.long 0x5274c742, 0x3bdcf4dc, 0x2d94e907, 0x32eac016
  	.long 0x26d33ca3, 0x60415a8a, 0x31f57880, 0x68c8aa52
  	.long 0x23eb16da, 0x6204f4a1, 0x373927c1, 0x0d24eb7c
  	.long 0x06dd7379, 0x2b3be507, 0x0f9c55b1, 0x2c7925eb
  	.long 0x36d67c9a, 0x42f831d9, 0x5e3961cb, 0x65d637a8
  	.long 0x24bb3820, 0x4d08e33d, 0x2188754f, 0x147e409e
  	.long 0x6a9620a0, 0x62e26657, 0x7bd8ce81, 0x11da0abb
  	.long 0x5f9e7b50, 0x23e444b6, 0x25920c78, 0x5fc894f0
  	.long 0x5e338cbb, 0x404237fd, 0x1d60f80f, 0x320a1743
  	.long 0x76013d2b, 0x070294ee, 0x695e243b, 0x56b177fd
  	.long 0x752492e1, 0x6decd52f, 0x125f5219, 0x139d2e78
  	.long 0x1898d11e, 0x2f7ee785, 0x4db405d8, 0x1a028a35
  	.long 0x63f6f323, 0x1f6d0078, 0x307cfd67, 0x3f32a78a
  	.long 0x6980796c, 0x462b3d83, 0x34b639f2, 0x53fce379
  	.long 0x74ba50f4, 0x1abc2c4b, 0x5eeaeb8d, 0x335a7a0d
  	.long 0x3973dd20, 0x0462d66b, 0x159813ff, 0x1e4643fd
  	.long 0x06bc5c62, 0x3115e3fc, 0x09101613, 0x47af2515
  	.long 0x4f11ec54, 0x78b99911, 0x3db8dd44, 0x1ec10b9b
  	.long 0x5b5506ca, 0x773ce092, 0x567be81a, 0x5475b975
  	.long 0x7a2cde1a, 0x494536f5, 0x34737bb4, 0x76d9750b
  	.long 0x2a1f6232, 0x2e49644d, 0x7dddcbe7, 0x500cebdb
  	.long 0x619dab9e, 0x48c626fe, 0x1cda3193, 0x52dabe9d
