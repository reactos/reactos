/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/usercontext.c
 * PURPOSE:     manage user mode contexts (create, destroy, reference)
 * COPYRIGHT:   Copyright 2019 Andreas Maier <staubim@quantentunnel.de>
 */
#ifndef _USERCONTEXT_H_
#define _USERCONTEXT_H_

#include "precomp.h"

// user table
NTSTATUS NTAPI
UsrSpInitUserModeContext(
    LSA_SEC_HANDLE ContextHandle,
    PSecBuffer PackedContext);

// -------------------------------------

// we need a map to have a link between
// user mode context and lsa mode handle
typedef struct _NTLMSSP_CONTEXT_USR
{
    LIST_ENTRY Entry;
    // context handle is from lsa mode
    LSA_SEC_HANDLE ContextHandle;
    // can be a server oder client context
    PNTLMSSP_CONTEXT_HDR Hdr;
} NTLMSSP_CONTEXT_USR, *PNTLMSSP_CONTEXT_USR;

VOID
NtlmUsrContextInitialize(VOID);

PNTLMSSP_CONTEXT_USR
NtlmUsrReferenceContext(
    IN LSA_SEC_HANDLE ContextHandle);

VOID
NtlmUsrDereferenceContext(
    IN PNTLMSSP_CONTEXT_USR Context);

#endif
