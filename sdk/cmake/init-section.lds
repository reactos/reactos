/* Make sure the INIT section is at the end of the module so we can reclaim the space */
SECTIONS
{
  INIT BLOCK(__section_alignment__) :
  {
    __init_start__ = . ;
    *(INIT)
    __init_end__ = . ;
  }
}
INSERT AFTER .reloc;
