int
_kbhit(void)
{
  INPUT_RECORD InputRecord;
  DWORD NumberRead;
  if (char_avail)
    	return(1);
  else {
	PeekConsoleInput(stdin->file,&InputRecord,1,&NumberRead);
	return NumberRead;
  }
  return 0;
}
