#ifndef __INCLUDE_CSR_PROTOCOL_H
#define __INCLUDE_CSR_PROTOCOL_H

typedef ULONG CSR_API_NUMBER;

#define MAKE_CSR_OPCODE(s,m) ((s) << 16) + (m)

//
// Shared CSR Message Header for all CSR Servers
// Gary Nebbett - Alex Ionescu
//
typedef struct _CSR_PORT_MESSAGE_HEADER
{
    //
    // LPC Header
    //
    PORT_MESSAGE PortHeader;

    //
    // Buffer allocated with CsrAllocateCaptureBuffer.
    // Sent as 2nd parameter to CsrClientCallServer.
    //
    PVOID CsrCaptureData;

    //
    // CSR API Message ID and Return Value
    //
    CSR_API_NUMBER Opcode;
    ULONG Status; 
    ULONG Reserved;

    //
    // Server-defined union of supported structures
    // 
} CSR_PORT_MESSAGE_HEADER, * PCSR_PORT_MESSAGE_HEADER;

#endif /* ndef __INCLUDE_CSR_LPCPROTO_H */
