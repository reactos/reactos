/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>

FILE _iob[5] =
{
	// stdin
{
 NULL, 0, NULL,
  _IOREAD | _IO_LBF ,
  0, 0,0, NULL
},
	// stdout
{
 NULL, 0, NULL,
   _IOWRT | _IO_LBF |_IOSTRG,
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
   _IOREAD | _IOWRT | _IONBF,
  3,0,0, NULL
},
	// stdprn
{
 NULL, 0, NULL,
  _IOWRT | _IONBF,
  4, 0,0,NULL
}
};

/*
 * @implemented
 */
FILE *__p__iob(void)
{
  return &_iob[0];
}
