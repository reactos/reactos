/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <windows.h>

#undef putchar
int
puts(const char *s)
{
	/*
  int c;

  while ((c = *s++))
    putchar(c);
  return putchar('\n');
  */
	int r = 0;
	if ( !WriteFile(_get_osfhandle(stdout->_file),s,strlen(s),&r,NULL) ) 
		return -1;

  	return putchar('\n');;
}
