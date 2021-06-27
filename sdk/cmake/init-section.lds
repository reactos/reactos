/* Make sure the INIT & .rsrc sections are at the end of the module so we can reclaim the space */
SECTIONS
{
  INIT BLOCK(__section_alignment__) : ALIGN(__file_alignment__)
  {
    __init_start__ = . ;
    *(INIT)
    __init_end__ = . ;
  }
  .rsrc BLOCK(__section_alignment__) : ALIGN(__file_alignment__)
  {
    __rsrc_start__ = . ;
    *(.rsrc)
    __rsrc_end__ = . ;
  }
}
INSERT BEFORE .reloc; /* .reloc is always at the end */
