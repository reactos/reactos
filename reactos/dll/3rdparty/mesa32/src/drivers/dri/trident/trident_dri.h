#ifndef _TRIDENT_DRI_
#define _TRIDENT_DRI_

#include "xf86drm.h"

typedef struct {
  drm_handle_t regs;
  drmSize regsSize;
  drmAddress regsMap;
  int deviceID;
  int width;
  int height;
  int mem;
  int frontOffset;
  int frontPitch;
  int backOffset;
  int backPitch;
  int depthOffset;
  int depthPitch;
  int cpp;
#if 0
  int textureOffset;
  int textureSize;
#endif
  unsigned int sarea_priv_offset;
} TRIDENTDRIRec, *TRIDENTDRIPtr;

#endif
