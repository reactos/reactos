/*
 * PROJECT:     ReactOS apisets
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Interface for resolving the apisets
 * COPYRIGHT:   Copyright 2024 Mark Jansen <mark.jansen@reactos.org>
 */
#ifndef APISETS_H
#define APISETS_H

#ifdef __cplusplus
extern "C"
{
#endif


#define APISET_WIN7  (1 << 0)
#define APISET_WIN8  (1 << 1)
#define APISET_WIN81 (1 << 2)
#define APISET_WIN10 (1 << 3)
//#define APISET_WIN11 (1 << 4)


NTSTATUS
ApiSetResolveToHost(_In_ DWORD ApisetVersion, _In_ PCUNICODE_STRING ApiToResolve, _Out_ PBOOLEAN Resolved, _Out_ PUNICODE_STRING Output);

#ifdef __cplusplus
}
#endif


#endif // APISETS_H
