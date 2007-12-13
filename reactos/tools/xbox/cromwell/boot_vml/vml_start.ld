/*
 * loader script
 */

OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);
MEMORY
{
	ram (rwx) : ORIGIN = 0, LENGTH = 64M
	rom (rx)  : ORIGIN = 0x03A00000, LENGTH = 2M
}


LOW_ROM = 0x00100000; 

SECTIONS {

		/* ROM allocations */

	.vmlinuz_start LOW_ROM : AT ( 0 ){
		
		*(.vmlinuz_start);
		
	}

	.rodata (LOW_ROM + SIZEOF(.vmlinuz_start)) : AT (SIZEOF(.vmlinuz_start)) {
		*(.rodata);
		*(.rodata.str1.1);
		*(.rodata.str1.32);
		*(.rodata.cst4);
		*(.rodata.cst8);

	}
        
	/* ram allocations */        
	
	.data (LOW_ROM + SIZEOF(.vmlinuz_start) + SIZEOF(.rodata)) : AT( SIZEOF(.vmlinuz_start) + SIZEOF(.rodata) ) {
		_start_data = .;
		*(.data);
		*(.sdata);
		*(.sdata2);
		*(.got);
		_end_data = .;
	}

	/* the data (initialized globals) is moved to ram by the startup code */


	.bss ( ADDR(.data) + SIZEOF(.data) ) : {
		_bss = .;
		*(.bss)
		*(.sbss)
		*(COMMON)
		_ebss = . ;
		_heap = . ;
	}

}
