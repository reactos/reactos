#ifndef __INCLUDE_CSR_PROTOCOL_H
#define __INCLUDE_CSR_PROTOCOL_H

typedef ULONG CSR_API_NUMBER;

#define MAKE_CSR_OPCODE(s,m) ((s) << 16) + (m)

// Gary Nebbett
typedef struct _CSR_PORT_MESSAGE_HEADER
{
	DWORD Unused1;
	ULONG Opcode;
	ULONG Status;
	ULONG Unused2;

} CSR_PORT_MESSAGE_HEADER, * PCSR_PORT_MESSAGE_HEADER;

#endif /* ndef __INCLUDE_CSR_LPCPROTO_H */
