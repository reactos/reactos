SECTIONS
{
  .data __image_base__ + __section_alignment__ :
  {
    *(.data)
    *(SORT(.data*))
    *(.rdata)
    *(SORT(.rdata*))
    *(.text)
    *(SORT(.text*))
    *(.bss)
    *(COMMON)
  }

  .idata BLOCK(__section_alignment__) :
  {
    SORT(*)(.idata$2)
    SORT(*)(.idata$3)
    /* terminator of the import descriptor list */
    LONG (0); LONG (0); LONG (0); LONG (0); LONG (0);
    SORT(*)(.idata$4)
    SORT(*)(.idata$5)
    SORT(*)(.idata$6)
    SORT(*)(.idata$7)
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

  .edata BLOCK(__section_alignment__) :
  {
    *(.edata)
  }

  /DISCARD/ :
  {
    *(*)
  }
}
