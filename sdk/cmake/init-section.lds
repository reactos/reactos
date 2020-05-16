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
/*INSERT AFTER .reloc;*//*This is what we actually want to do, but do not dare due to LD bug ROSBE-154*/
INSERT AFTER .rsrc; /*This is what we do instead to at least fix CORE-14683*/
