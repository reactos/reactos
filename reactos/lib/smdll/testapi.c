/* $Id$ */
#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>

VOID STDCALL SmPrintPortMessage (PSM_PORT_MESSAGE SmMessage)
{
	DbgPrint ("SM_PORT_MESSAGE %08lx:\n", (ULONG) SmMessage);
	DbgPrint ("  Header:\n");
	DbgPrint ("    MessageType = %u\n", SmMessage->Header.MessageType);
	DbgPrint ("    DataSize    = %d\n", SmMessage->Header.DataSize);
	DbgPrint ("    MessageSize = %d\n", SmMessage->Header.MessageSize);
	DbgPrint ("  ApiIndex      = %ld\n", SmMessage->ApiIndex);
	DbgPrint ("  Status        = %08lx\n", SmMessage->Status);
	DbgPrint ("  ExecPgm:\n");
	DbgPrint ("    NameLength  = %ld\n", SmMessage->ExecPgm.NameLength);
	DbgPrint ("    Name        = %ls\n", (LPWSTR) & SmMessage->ExecPgm.Name);
}
/* EOF */

