/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_dri.h,v 1.9 2003/08/29 08:50:54 twini Exp $ */

/* modified from tdfx_dri.h */

#ifndef _SIS_DRI_
#define _SIS_DRI_

#include "xf86drm.h"
#include "drm.h"

#define SIS_MAX_DRAWABLES 256
#define SISIOMAPSIZE (64*1024)

typedef struct {
  int CtxOwner;
  int QueueLength;
  unsigned int AGPCmdBufNext;
  unsigned int FrameCount;
#ifdef SIS315DRI
  /* For 315 series */
  unsigned long sharedWPoffset;
#endif
#if 0
  unsigned char *AGPCmdBufBase;
  unsigned long AGPCmdBufAddr;
  unsigned long AGPCmdBufOffset;
  unsigned int  AGPCmdBufSize;
  unsigned long AGPCmdBufNext;
#endif
} SISSAREAPriv, *SISSAREAPrivPtr;

#define AGPVtxBufNext AGPCmdBufNext

#define SIS_FRONT 0
#define SIS_BACK 1
#define SIS_DEPTH 2

typedef struct {
  drm_handle_t handle;
  drmSize size;
} sisRegion, *sisRegionPtr;

typedef struct {
  sisRegion regs, agp;
  int deviceID;
  int width;
  int height;
  int mem;				/* unused in Mesa 3 DRI */
  int bytesPerPixel;
  int priv1;				/* unused in Mesa 3 DRI */
  int priv2;				/* unused in Mesa 3 DRI */
  int fbOffset;				/* unused in Mesa 3 DRI */
  int backOffset;			/* unused in Mesa 3 DRI */
  int depthOffset;			/* unused in Mesa 3 DRI */
  int textureOffset;			/* unused in Mesa 3 DRI */
  int textureSize;			/* unused in Mesa 3 DRI */
  unsigned int AGPCmdBufOffset;
  unsigned int AGPCmdBufSize;
  int irqEnabled;			/* unused in Mesa 3 DRI */
  unsigned int scrnX, scrnY;		/* unused in Mesa 3 DRI */
} SISDRIRec, *SISDRIPtr;

#define AGPVtxBufOffset AGPCmdBufOffset
#define AGPVtxBufSize AGPCmdBufSize

typedef struct {
  /* Nothing here yet */
  int dummy;
} SISConfigPrivRec, *SISConfigPrivPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} SISDRIContextRec, *SISDRIContextPtr;

#ifdef XFree86Server

#include "screenint.h"

Bool SISDRIScreenInit(ScreenPtr pScreen);
void SISDRICloseScreen(ScreenPtr pScreen);
Bool SISDRIFinishScreenInit(ScreenPtr pScreen);

#endif
#endif
