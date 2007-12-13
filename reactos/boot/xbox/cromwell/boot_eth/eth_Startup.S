.code32

.section .vmlinuz_start, "ax"

.org 0x00
	
	xor	%eax, %eax
	xor	%edx, %edx          
	cld
	
	mov 	$0x03A00000, %edi    	// Destiation
	mov 	$0x00100100, %esi	// Source
	mov 	$0x200000, %ecx		// 2MB
	rep movsb
	wbinvd
/*	
	mov $0xc004, %dx 
	mov $0x20, %al 
	out %al, %dx 
	mov $0xc008, %dx 
	mov $0x8, %al 
	out %al, %dx 
	mov $0xc006, %dx 
	mov $0xa6, %al 
	out %al, %dx 
	mov $0xc006, %dx 
	in %dx,%al 
	mov $0xc002, %dx 
	mov $0x1a, %al 
	out %al, %dx 
	mov $0xc000, %dx 

	ledspin: in %dx, %al 
	cmp $0x10, %al 
	jnz ledspin 
	mov $0xc004, %dx 
	mov $0x20, %al 
	out %al, %dx 
	mov $0xc008, %dx 
	mov $0x7, %al 
	out %al, %dx 
	mov $0xc006, %dx 
	mov $0x1, %al 
	out %al, %dx 
	mov $0xc006, %dx 
	in %dx,%al 
	mov $0xc002, %dx 
	mov $0x1a, %al 
	out %al, %dx 
	mov $0xc000, %dx 

	ledspin1: in %dx, %al
	cmp $0x10, %al
	jnz ledspin1 

	jmp ledspin1 
*/
    
	cld	
	ljmp 	$0x10, $0x03A00000                                            

.org 0x0100	// append crom image here
