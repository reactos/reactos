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
  0, 0,0, NULL, 0
},
	// stdout
{
 NULL, 0, NULL,
   _IOWRT | _IOLBF |_IOSTRG,
  1,0,0, NULL, 0
},
	// stderr
{
 NULL, 0, NULL,
  _IOWRT | _IONBF,
  2,0,0, NULL, 0
},
	// stdaux
{
 NULL, 0, NULL,
  _IORW | _IONBF,
  3,0,0, NULL, 0
},
	// stdprn
{
 NULL, 0, NULL,
  _IOWRT | _IONBF,
  4, 0,0,NULL, 0
}
};

FILE (*_iob)[] = &_crtdll_iob;

FILE (*__imp__iob)[] = &_crtdll_iob;
