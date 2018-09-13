/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include <port1632.h>
#include <dcrc.h>
#include <dynacomm.h>


#if OLDCODE

WORD lread(handle, buffer, count)            /* mbbx 2.01.13 ... */
INT      handle;
LPSTR    buffer;
WORD     count;
{
   return(_LREAD(handle, buffer, count));
}


WORD lwrite(handle, buffer, count)           /* mbbx 2.01.13 ... */
INT      handle;
LPSTR    buffer;
WORD     count;
{
   return(_LWRITE(handle, buffer, count));
}

#endif


