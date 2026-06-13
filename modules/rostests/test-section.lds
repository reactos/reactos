/* Make sure the .test section is kept and sorted */
SECTIONS
{
  .test BLOCK(__section_alignment__) :
  {
    ___test_start__ = . ;
    KEEP (*(SORT(.test$*)))
    ___test_end__ = . ;
  }
}
INSERT BEFORE .reloc; /* .reloc is always at the end */
