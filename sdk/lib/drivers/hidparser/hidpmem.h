/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser km/um memory functions wrapper
 * COPYRIGHT:   Copyright  Michael Martin <michael.martin@reactos.org>
 *              Copyright  Johannes Anderwald <johannes.anderwald@reactos.org>
 */

PVOID NTAPI AllocFunction(ULONG Size);
VOID NTAPI FreeFunction(PVOID Item);
VOID NTAPI ZeroFunction(PVOID Item, ULONG Size);
VOID NTAPI CopyFunction(PVOID Target, PVOID Source, ULONG Size);
