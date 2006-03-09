#ifndef __MSGINA_H
#define __MSGINA_H

#define GINA_VERSION (WLX_VERSION_1_3)

#define PWLX_DISPATCH_VERSION PWLX_DISPATCH_VERSION_1_3

typedef struct {
  HANDLE hWlx;
  LPWSTR station;
  PWLX_DISPATCH_VERSION pWlxFuncs;
  HANDLE hDllInstance;
  HANDLE UserToken;
  HWND hStatusWindow;
  BOOL SignaledStatusWindowCreated;
} GINA_CONTEXT, *PGINA_CONTEXT;

HINSTANCE hDllInstance;

#endif /* __MSGINA_H */

/* EOF */
