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
