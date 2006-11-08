/*
 * reactos emulation layer betwin wine and windows api for directx 
 * get hardware dec
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
 *      soucre clean
 *      need to rewrite almost everthing so it get all info from the hardware instead
 *      see todo.rtf 
 *
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
#include "dxros_layer.h"
#include "dsconf.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

DWORD dxrosdrv_drv_querydsounddescss(int type, HWAVEOUT hwo_out,HWAVEIN  hwo_in, PDSDRIVERDESC pDESC)
{  
  WAVEOUTCAPSA ac_play;
  WAVEINCAPSA  ac_rec;
  DWORD msg;
  
  

  // type 0 = out
  // clear data
  memset(pDESC,0,sizeof(DSDRIVERDESC));
  memset((char *)pDESC->szDrvname,0,255);  	
  if (type==0) memset(&ac_play,0,sizeof(WAVEOUTCAPSA));
  else memset(&ac_rec,0,sizeof(WAVEINCAPSA));

  // get some data
 if (type==0) {
	            msg = waveOutGetDevCapsA((UINT)hwo_out,&ac_play,sizeof(WAVEOUTCAPSA));
                if  (ac_play.szPname==NULL) return MMSYSERR_NOTSUPPORTED;	
                }

  else {
	     msg = waveInGetDevCapsA((UINT)hwo_in,&ac_rec,sizeof(WAVEINCAPSA));
		 if  (ac_rec.szPname==NULL) return MMSYSERR_NOTSUPPORTED;	
         }

  if (msg!=MMSYSERR_NOERROR) return msg;



  // setting up value 
  //pDESC->wReserved = NULL; 
  

  if (type==0) {
	            pDESC->ulDeviceNum = (ULONG)hwo_out;
	            memcpy((char *)pDESC->szDesc,ac_play.szPname,strlen(ac_play.szPname));
              }
  else {
	    pDESC->ulDeviceNum = (ULONG)hwo_in;
	    memcpy((char *)pDESC->szDesc,ac_rec.szPname,strlen(ac_rec.szPname));
        }

  // FIXME
  /* how to fill these
       pDESC->dwFlags |= DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT |
                      DSDDESC_USESYSTEMMEMORY | DSDDESC_DONTNEEDPRIMARYLOCK |
                      DSDDESC_DONTNEEDSECONDARYLOCK;
        //pDesc->dnDevNode		= WOutDev[This->wDevID].waveDesc.dnDevNode;
       pDESC->wVxdId		= 0;
       pDESC->wReserved		= 0;    
       pDESC->dwHeapType		= DSDHEAP_NOHEAP;
       pDESC->pvDirectDrawHeap	= NULL;
       pDESC->dwMemStartAddress	= 0;
       pDESC->dwMemEndAddress	= 0;
       pDESC->dwMemAllocExtra	= 0;
       pDESC->pvReserved1		= NULL;
       pDESC->pvReserved2		= NULL;

*/

  pDESC->pvReserved1 = NULL;
  pDESC->pvReserved2 = NULL;
  
  //  we need to fill it right so we do not need ddraw.dll
  pDESC->pvDirectDrawHeap = NULL; // wine dsound does not use ddraw.dll

     	 
  // need to write dective for it
  pDESC->dwHeapType = DSDHEAP_NOHEAP;

  // have take the value from wine audio drv  
  pDESC->dwFlags = DSDDESC_DOMMSYSTEMOPEN | 
	                                    DSDDESC_DOMMSYSTEMSETFORMAT |
                                        DSDDESC_USESYSTEMMEMORY | 
										DSDDESC_DONTNEEDPRIMARYLOCK |
                                        DSDDESC_DONTNEEDSECONDARYLOCK;


  //WAVEOPENDESC->DevNode need to fig. how to get it from mmdrv
  pDESC->dnDevNode = 0;  // wine dsound are using this value

   // need to fill the rest also     

   // must contain the audio drv name
   // but how to get it ?
   //memcpy((char *)pDESC->szDrvname,(char *)&"kx.sys",6);	 	 

   
    
   pDESC->dwMemStartAddress = 0;
   pDESC->dwMemAllocExtra = 0;	 
   pDESC->wVxdId = 0;
     

  return MMSYSERR_NOERROR;
}

