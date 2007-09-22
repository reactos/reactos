#ifndef POWERCFG_H
#define POWERCFG_H

#include "powrprof.h"

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;
extern GLOBAL_POWER_POLICY gGPP;
extern POWER_POLICY gPP[];
extern UINT guiIndex;

#define MAX_POWER_PAGES 32

#endif /* __CPL_SAMPLE_H */

/* EOF */
