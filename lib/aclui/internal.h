#ifndef __ACLUI_INTERNAL_H
#define __ACLUI_INTERNAL_H

ULONG DbgPrint(PCH Format,...);
#define DPRINT DbgPrint

extern HINSTANCE hDllInstance;

typedef struct _SECURITY_PAGE
{
  HWND hWnd;
  HWND hWndUsrList;
  HIMAGELIST hiUsrs;
  LPSECURITYINFO psi;
} SECURITY_PAGE, *PSECURITY_PAGE;

#endif /* __ACLUI_INTERNAL_H */

/* EOF */
