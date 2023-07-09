#ifndef _DBACKP_H_
#define _DBACKP_H_

#define BP_NEWPAT           0x00000001
#define BP_NEWWALL          0x00000002
#define BP_TILE             0x00000004
#define BP_REINIT           0x00000008
#define BP_EXTERNALWALL     0x00000010
#define BP_STRETCH          0x00000020

#define WM_SETBACKINFO  (WM_USER+1)

BOOL RegisterBackPreviewClass();

#endif
