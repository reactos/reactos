#ifndef _VMSSS_H_INCLUDED_
#define _VMSSS_H_INCLUDED_

#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/helper.h>

/* init.c */
extern HANDLE VmsApiPort;
NTSTATUS VmsInitializeServer(VOID);

/* server.c */
NTSTATUS VmsRunServer(VOID);

#endif /* ndef _VMSSS_H_INCLUDED_ */
