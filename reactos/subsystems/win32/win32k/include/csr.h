#ifndef __WIN32K_CSR_H
#define __WIN32K_CSR_H

/* Notifies CSR about a new desktop */
NTSTATUS NTAPI
CsrNotifyCreateDesktop(HDESK Desktop);

#endif
