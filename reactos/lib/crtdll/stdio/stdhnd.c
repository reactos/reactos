/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>
//#include <crtdll/stdiohk.h>



FILE _crtdll_iob[5] =
{
	// stdin
{
 NULL, 0, NULL,
  _IOREAD | _IOLBF ,
  0, 0,0, NULL
},
	// stdout
{
 NULL, 0, NULL,
   _IOWRT | _IOLBF |_IOSTRG,
  1,0,0, NULL
},
	// stderr
{
 NULL, 0, NULL,
  _IOWRT | _IONBF,
  2,0,0, NULL
},
	// stdaux
{
 NULL, 0, NULL,
  _IORW | _IONBF,
  3,0,0, NULL
},
	// stdprn
{
 NULL, 0, NULL,
  _IOWRT | _IONBF,
  4, 0,0,NULL
}
};

FILE (*_iob)[] = &_crtdll_iob;
