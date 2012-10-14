OUTPUT_FORMAT(pei-x86-64)
ENTRY(_mainCRTStartup)
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
     ___CTOR_LIST__ = .; __CTOR_LIST__ = . ; 
			LONG (-1); *(.ctors); *(.ctor); LONG (0); 
     ___DTOR_LIST__ = .; __DTOR_LIST__ = . ; 
			LONG (-1); *(.dtors); *(.dtor);  LONG (0); 
     *(.fini)
    /* ??? Why is .gcc_exc here?  */
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
  }
  .rdata BLOCK(__section_alignment__) :
  {
    *(.rdata)
    *(SORT(.rdata$*))
    *(.eh_frame)
  }
  .stab BLOCK(__section_alignment__) (NOLOAD) :
  {
    [ .stab ]
  }
  .stabstr BLOCK(__section_alignment__) (NOLOAD) :
  {
    [ .stabstr ]
  }

}

