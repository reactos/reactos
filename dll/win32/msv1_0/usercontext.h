/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/usercontext.c
 * PURPOSE:     manage user mode contexts (create, destroy, reference)
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */
#ifndef _USERCONTEXT_H_
#define _USERCONTEXT_H_

#include "precomp.h"

NTSTATUS NTAPI
UsrSpInitUserModeContext(
    _In_ LSA_SEC_HANDLE ContextHandle,
    _In_ PSecBuffer PackedContext);

#endif
