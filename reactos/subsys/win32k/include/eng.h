#ifndef __WIN32K_ENG_H
#define __WIN32K_ENG_H

BOOL STDCALL  EngIntersectRect (PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);
VOID FASTCALL EngDeleteXlate (XLATEOBJ *XlateObj);

#endif /* __WIN32K_ENG_H */
