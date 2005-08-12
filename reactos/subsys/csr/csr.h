#if !defined(_INCLUDE_CSR_H)
#define _INCLUDE_CSR_H

/* PSDK/NDK Headers */
#include <stdio.h>
#include <windows.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <csr/server.h>


#define CSRSS_ARGUMENT_SIZE 16

/* args.c */
#define CSRP_MAX_ARGUMENT_COUNT 512

typedef struct _COMMAND_LINE_ARGUMENT
{
	ULONG		Count;
	UNICODE_STRING	Buffer;
	PWSTR		* Vector;

} COMMAND_LINE_ARGUMENT, *PCOMMAND_LINE_ARGUMENT;

NTSTATUS FASTCALL CsrParseCommandLine (PPEB,PCOMMAND_LINE_ARGUMENT);
VOID FASTCALL CsrFreeCommandLine (PPEB,PCOMMAND_LINE_ARGUMENT);

/* csrsrv.dll  */
NTSTATUS STDCALL CsrServerInitialization (ULONG,LPWSTR*);

#endif /* !def _INCLUDE_CSR_H */
	
