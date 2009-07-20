#ifndef __WIN32K_CSR_H
#define __WIN32K_CSR_H

/* Notifies CSR about a new desktop */
NTSTATUS NTAPI
CsrNotifyCreateDesktop(HDESK Desktop);

/* Notifies CSR about a show desktop event */
NTSTATUS NTAPI
CsrNotifyShowDesktop(HWND DesktopWindow, ULONG Width, ULONG Height);

#endif
