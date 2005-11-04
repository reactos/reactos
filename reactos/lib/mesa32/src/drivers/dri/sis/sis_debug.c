/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_debug.c,v 1.5 2000/09/26 15:56:48 tsi Exp $ */

/*
 * Authors:
 *    Sung-Ching Lin <sclin@sis.com.tw>
 *
 */

/* 
 * dump HW states, set environment variable SIS_DEBUG
 * to enable these functions 
 */

#include <fcntl.h>
#include <assert.h>

#include "sis_context.h"

/* for SiS 300/630/540 */
#define MMIOLength (0x8FFF-0x8800+1)
#define MMIO3DOffset (0x8800)
#define FILE_NAME "300.dump"

char *IOBase4Debug = 0;

char *prevLockFile = NULL;
int prevLockLine = 0;

GLint _empty[0x10000];

void
dump_agp (void *addr, int dword_count)
{
  if (!getenv ("SIS_DEBUG"))
    return;

  {
    int i;
    FILE *file = fopen ("300agp.dump", "w");

    if (file)
      {
	for (i = 0; i < dword_count; i++)
	  {
	    fprintf (file, "%f\n", *(float *) addr);
	    ((unsigned char *) addr) += 4;
	  }
	fclose (file);
      }
  }
}

void
d2f_once (GLcontext * ctx)
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  sisContextPtr smesa = SIS_CONTEXT(ctx);

  static int serialNumber = -1;

  if (serialNumber == smesa->serialNumber)
    return;
  else
    serialNumber = smesa->serialNumber;

  d2f();
}

void
d2f (void)
{
  if (!getenv ("SIS_DEBUG"))
    return;

  /* dump 0x8800 - 0x8AFF */
  {
    int fh;
    int rval;
    void *addr = IOBase4Debug + MMIO3DOffset;

    assert (IOBase4Debug);

    if ((fh = open (FILE_NAME, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE)) != -1)
      {
	rval = write (fh, addr, MMIOLength);
	assert (rval != -1);
	close (fh);
      }
  }
}

/* dump to HW */
void
d2h (char *file_name)
{
  int fh;
  int rval;
  void *addr[MMIOLength];

  if (!getenv ("SIS_DEBUG"))
    return;

  if ((fh = open (file_name, O_CREAT, S_IREAD | S_IWRITE)) != -1)
    {
      rval = read (fh, addr, MMIOLength);
      assert (rval != -1);
      close (fh);
    }
  memcpy (IOBase4Debug + MMIO3DOffset, addr, MMIOLength);

}

/* dump video memory to file  */
void
dvidmem (unsigned char *addr, int size)
{
  int fh;
  int rval;
  static char *file_name = "vidmem.dump";

  if (!getenv ("SIS_DEBUG"))
    return;

  if ((fh = open (file_name, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE)) != -1)
    {
      rval = write (fh, addr, size);
      assert (rval != -1);
      close (fh);
    }
}
