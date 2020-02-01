#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include "precomp.h"

SECURITY_STATUS SEC_ENTRY
NtlmEncryptMessage(
    IN PNTLMSSP_CONTEXT_HDR Context,
    IN ULONG fQOP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    IN BOOL SignOnly);

#endif
