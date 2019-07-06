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