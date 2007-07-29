/*
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
 * ReactOS emulation layer betwin wine and windows api for directx 
 * This transform wine specfiy api to native reactos/windows api
 * wine have done some hack to geting dsound working. But does 
 * hack does not work on windows or reactos. It need to warp thuse
 * api hack to true native api. 
 * 
 * This layer have some weekness 
 * it does not support hardware accleration of the sound card.
 * it need complete implant of it here, and we need also wdm 
 * in reactos to complete dsound. for monet it is not posibile
 * to get all value and fill them. eg the soundcard drv name.
 *
 * Wine does get almost everthing from there sound drv, then
 * pass it thurg winmm, then to dsound. but windows drv does
 * not pass this info to the winmm. it send it on wdm instead.
 * 
 * Do rember this dsound is hardcode to software mode only.
 * the program will not notice it. it will think it is hardware.
 * for the flag never report back it is in software mode. 
 *
 * 
 * Copyright 2004 Magnus Olsen
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
#include "dxros_layer.h"
#include "dsconf.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);


DWORD RosWineWaveOutMessage(HWAVEOUT  hwo, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
 DWORD msg;
 switch (uMsg) {
                case DRV_QUERYDSOUNDDESC:
	            msg = dxrosdrv_drv_querydsounddescss(0, (HWAVEOUT)((ULONG)hwo),(HWAVEIN) 0, (PDSDRIVERDESC) dwParam1);	 
	            break;

                case DRV_QUERYDSOUNDIFACE:
                msg = dxrosdrv_drv_querydsoundiface((HWAVEIN)hwo, (PIDSDRIVER*)dwParam1);
				break;
	            				
                default :
                msg = waveOutMessage(hwo, uMsg, dwParam1, dwParam2);
	            break;
                }   	                
return msg;
}

DWORD RosWineWaveInMessage(HWAVEIN  hwo, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
 DWORD msg;
 switch (uMsg) {
                 case DRV_QUERYDSOUNDDESC:
	             msg = dxrosdrv_drv_querydsounddescss(1, (HWAVEOUT)((ULONG)0),(HWAVEIN)((ULONG)hwo), (PDSDRIVERDESC) dwParam1);	 
	             break;

                 case DRV_QUERYDSOUNDIFACE:                 
                 msg = dxrosdrv_drv_querydsoundiface(hwo, (PIDSDRIVER*)dwParam1);                
                 break; 
	             
                 default :
                 msg = waveInMessage(hwo, uMsg, dwParam1, dwParam2);
	             break;
                 }   	                
return msg;
}
