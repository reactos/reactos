#ifndef __ACLUI_INTERNAL_H
#define __ACLUI_INTERNAL_H

ULONG DbgPrint(PCH Format,...);
#define DPRINT DbgPrint

extern HINSTANCE hDllInstance;

#endif /* __ACLUI_INTERNAL_H */

/* EOF */
