/*
 * ReactOS emulation layer betwin wine and windows api for directx
 * convort string to GUID
 *
 * Copyright 2004 Magnus Olsen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * TODO:
 *      soucre clean
 *      Rewrite so it use unicode instead for asc or find how windows convert it
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dxros_layer.h"
#include "dsconf.h"
#include "windows.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);



void dxGetGuidFromString( char *in_str, GUID *guid )
{
    unsigned long c=0;
	int i;

	// this string hex converter need to be rewrite or find uhow windows convort a string
	// to GUID

	for (i=1;i<9;i++)
	{
	 if (in_str[i]>='0' && in_str[i]<='9')
	 {
	  c=c * 16 + (in_str[i] - 48);
	  }
	 else if (in_str[i]>='A' && in_str[i]<='F')
	 {
	  c=c * 16 + (in_str[i] - 55);
	  }
	 }
	 guid->Data1 = c;
	 c=0;

	for (i=9;i<14;i++)
	{
	  if (in_str[i]>='0' && in_str[i]<='9')
	  {
	   c=c * 16 + (in_str[i] - 48);
	  }
	  else if (in_str[i]>='A' && in_str[i]<='F')
	  {
	   c=c * 16 + (in_str[i] - 55);
	  }
     }

	 guid->Data2 = (short) c;
	 c=0;

	for (i=14;i<19;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data3 = (short) c;
		 c=0;

	for (i=20;i<22;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[0] = (BYTE) c;
		 c=0;

	for (i=22;i<24;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[1] = (BYTE) c;
		 c=0;


	for (i=25;i<27;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[2] = (BYTE) c;
		 c=0;

	for (i=27;i<29;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[3] = (BYTE) c;
		 c=0;

	for (i=29;i<31;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[4] = (BYTE) c;
		 c=0;

	for (i=31;i<33;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[5] = (BYTE) c;
		 c=0;

	for (i=33;i<35;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[6] = (BYTE) c;
		 c=0;

	for (i=35;i<37;i++)
		 {
		 if (in_str[i]>='0' && in_str[i]<='9') c=c * 16 + (in_str[i] - 48);
		 else if (in_str[i]>='A' && in_str[i]<='F') c=c * 16 + (in_str[i] - 55);
		 }
		 guid->Data4[7] = (BYTE) c;
		 c=0;
     }
