.code32

.section .vmlinuz_start, "ax"

.org 0x00
	
.org 0x1f1
	.byte 0x0A // *((BYTE *)0x901f1))+1)*512; .. Loader Takes us to 0x1600

.org 0x1600
       
	xor	%eax, %eax
	xor	%edx, %edx          
	cld
	
	mov 	$0x03A00000, %edi    	// Destiation
	mov 	$0x00100100, %esi	// Source
	mov 	$0x200000, %ecx		// 2MB
	rep movsb
	wbinvd
	cld	
	ljmp 	$0x10, $0x03A00000                                            

.org 0x1700	// Here, we append the thing we want to load

