OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);
MEMORY
{
	ram (rwx) : ORIGIN = 0, LENGTH = 64M
	rom (rx)  : ORIGIN = 0xFFFC0000, LENGTH = 256K
}


RAM_CODE = 0x00100000; 

/* this should be changed depending the actual size of the rom */
ROM_SIZE = 256K;
LOW_ROM = 0xfffc0000; 


TOP_ROM = ( LOW_ROM + ROM_SIZE - 512 );
TOP_ROM_LOAD = ( ROM_SIZE - 512 );

SECTIONS {

	/* ROM Part of the programm */

	.low_rom LOW_ROM : AT ( 0x0 ){
		*(.low_rom);
		_end_rom = . ;
	}

	/* RAM Part of the programm */

	.text (RAM_CODE) : AT( SIZEOF(.low_rom) ) {
		_ram_location = .;
		_start_ramcopy = _end_rom;
		*(.text);
		_start_checksum = _start_ramcopy - LOW_ROM;
	}

	.data (RAM_CODE + SIZEOF(.text)) : AT( SIZEOF(.low_rom) + SIZEOF(.text)) {
		*(.data);
		*(.sdata);
		*(.sdata2);
		*(.got);
	}
       	
       	.rodata ( RAM_CODE + SIZEOF(.text) + SIZEOF(.data) ) : AT ( SIZEOF(.low_rom) + SIZEOF(.text) + SIZEOF(.data)) {
		*(.rodata);
		*(.rodata.str1.1);
		*(.rodata.str1.32);
		*(.rodata.cst4);
		*(.rodata.cst8);
	}
        
	.bss ( RAM_CODE + SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata) ) : AT ( SIZEOF(.low_rom) + SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata)) {
		_bss = .;
		*(.bss)
		*(.sbss)
		*(COMMON)
		_ebss = . ;
		_heap = . ;
	}

        /* We need to copy the .data section to to upper memory */

        _size_ramcopy = SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata) + SIZEOF(.bss);
	_size_sha1hash = SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata);                
                
/*
	.high_rom TOP_ROM : AT (TOP_ROM_LOAD) {
		_start_top_rom = . ;
		. = . + ( 512 - sizeof_top );
		*(.high_rom);
		_end_top_rom = . ;
	} = 0x90909090      
*/

}
