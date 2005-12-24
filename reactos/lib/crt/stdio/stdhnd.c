/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

FILE _iob[20] =
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
}
};

/*
 * @implemented
 */
FILE *__p__iob(void)
{
  return &_iob[0];
}
