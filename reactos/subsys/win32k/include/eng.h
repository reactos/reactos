#ifndef _WIN32K_ENG_H
#define _WIN32K_ENG_H

BOOL STDCALL  EngIntersectRect (PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);
VOID FASTCALL EngDeleteXlate (XLATEOBJ *XlateObj);

#endif /* _WIN32K_ENG_H */
