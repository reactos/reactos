#ifndef __MSGINA_H
#define __MSGINA_H

#define GINA_VERSION (WLX_VERSION_1_0)

typedef struct {
  HANDLE hWlx;
  LPWSTR station;
  PWLX_DISPATCH_VERSION_1_3 pWlxFuncs;
  HANDLE hDllInstance;
  HANDLE UserToken;
} GINA_CONTEXT, *PGINA_CONTEXT;

HINSTANCE hDllInstance;

#endif /* __MSGINA_H */

/* EOF */
