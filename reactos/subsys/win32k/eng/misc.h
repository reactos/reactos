#ifndef __ENG_MISC_H
#define __ENG_MISC_H

#ifndef __ENG_OBJECTS_H
#include "objects.h"
#endif

typedef struct INTENG_ENTER_LEAVE_TAG
  {
  /* Contents is private to EngEnter/EngLeave */
  SURFOBJ *DestObj;
  SURFGDI *DestGDI;
  SURFOBJ *OutputObj;
  HBITMAP OutputBitmap;
  CLIPOBJ *TrivialClipObj;
  RECTL DestRect;
  BOOL ReadOnly;
  } INTENG_ENTER_LEAVE, *PINTENG_ENTER_LEAVE;

extern BOOL STDCALL IntEngEnter(PINTENG_ENTER_LEAVE EnterLeave,
                                SURFOBJ *DestObj,
                                RECTL *DestRect,
                                BOOL ReadOnly,
                                POINTL *Translate,
                                SURFOBJ **OutputObj);
extern BOOL STDCALL IntEngLeave(PINTENG_ENTER_LEAVE EnterLeave);

#endif
