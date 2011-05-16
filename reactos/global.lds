OUTPUT_FORMAT(pei-i386)
SECTIONS
{
  .text  __image_base__ + __section_alignment__  : 
  {
    __text_start__ = .;
    *(.init)
    *(.text)
    *(SORT(.text$*))
    *(.glue_7t)
    *(.glue_7)
    *(.jcr)
     ___CTOR_LIST__ = .; __CTOR_LIST__ = . ; 
			LONG (-1);*(.ctors); *(.ctor); *(SORT(.ctors.*)); LONG (0); 
     ___DTOR_LIST__ = .; __DTOR_LIST__ = . ; 
			LONG (-1);*(.dtors); *(.dtor); *(SORT(.dtors.*)); LONG (0); 
     *(.fini)
     *(.gcc_exc)
     __text_end__ = .;
    *(.gcc_except_table)
  }
  init BLOCK(__section_alignment__) :
  {
    __init_start__ = . ;
    *(init)
    __init_end__ = . ;
  }
  .data BLOCK(__section_alignment__) : 
  {
    __data_start__ = . ;
    *(.data)
    *(.data2)
    *(SORT(.data$*))
    __data_end__ = . ;
    __bss_start__ = . ;
    *(.bss)
    *(COMMON)
    __bss_end__ = . ;
    *(SORT(.CRT$*))
  }
  .rdata BLOCK(__section_alignment__) :
  {
    *(.rdata)
    *(SORT(.rdata$*))
    *(.eh_frame)
  }
  .edata BLOCK(__section_alignment__) :   {     *(.edata)   }
  .idata BLOCK(__section_alignment__) :
  {
    /* This cannot currently be handled with grouped sections.
	See pe.em:sort_sections.  */
    SORT(*)(.idata$2)
    SORT(*)(.idata$3)
    /* These zeroes mark the end of the import list.  */
    LONG (0); LONG (0); LONG (0); LONG (0); LONG (0);
    SORT(*)(.idata$4)	
    SORT(*)(.idata$5)
    SORT(*)(.idata$6)
    SORT(*)(.idata$7)
  }
  .tls BLOCK(__section_alignment__) :
  {
    *(.tls)
    *(.tls$)
    *(SORT(.tls$*))
   }
  /DISCARD/ :
  {
    *(.debug$S)
    *(.debug$T)
    *(.debug$F)
    *(.drectve)
	*(.debug_loc)
	*(.debug_ranges)
	*(.debug_aranges)
	*(.debug_str)
	*(.debug_frame)
	*(.debug_pubnames)
	*(.debug_pubtypes)
	*(.gnu.linkonce.wt.*)
  }
  .rsrc BLOCK(__section_alignment__) :
  { 					
    *(.rsrc)
    *(SORT(.rsrc$*))
  }
  .reloc BLOCK(__section_alignment__) :
  { 					
    *(.reloc)
  }
  .debug_info BLOCK(__section_alignment__) (NOLOAD) :
  {
    [ .debug_info ]
  }
  .debug_abbrev BLOCK(__section_alignment__) (NOLOAD) :
  {
    [ .debug_abbrev ]
  }
  .debug_line BLOCK(__section_alignment__) (NOLOAD) :
  {
    [ .debug_line ]
  }
}