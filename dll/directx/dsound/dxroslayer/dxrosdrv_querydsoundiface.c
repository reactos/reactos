/*
 * reactos emulation layer betwin wine and windows api for directx 
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
 * TODO:
 *      write hardware support for windows nt 4.0 and higher
 *      put it in own library that call dxroslayer.a
 *
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
#include "dsconf.h"
#include "windows.h"
#include "dxros_layer.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);


DWORD dxrosdrv_drv_querydsoundiface(HWAVEIN wDevID, PIDSDRIVER* drv)
{  
	// no hardware support for dsound NT 4.0 does not support it
	// but win 2000/xp drv does support hardware support of direct sound 
	drv = NULL; 
	// drv should be fild with hardware pointers see PIDSDRIVER struct
	return MMSYSERR_NOERROR;
}
