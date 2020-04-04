/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/history.h
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

#pragma once

VOID
HistoryAddEntry(PCONSRV_CONSOLE Console,
                PUNICODE_STRING ExeName,
                PUNICODE_STRING Entry);

BOOL
HistoryFindEntryByPrefix(PCONSRV_CONSOLE Console,
                         PUNICODE_STRING ExeName,
                         PUNICODE_STRING Prefix,
                         PUNICODE_STRING Entry);

VOID
HistoryGetCurrentEntry(PCONSRV_CONSOLE Console,
                       PUNICODE_STRING ExeName,
                       PUNICODE_STRING Entry);

BOOL
HistoryRecallHistory(PCONSRV_CONSOLE Console,
                     PUNICODE_STRING ExeName,
                     INT Offset,
                     PUNICODE_STRING Entry);

VOID
HistoryDeleteCurrentBuffer(PCONSRV_CONSOLE Console,
                           PUNICODE_STRING ExeName);

VOID HistoryDeleteBuffers(PCONSRV_CONSOLE Console);

VOID
HistoryReshapeAllBuffers(
    IN PCONSRV_CONSOLE Console,
    IN ULONG HistoryBufferSize,
    IN ULONG MaxNumberOfHistoryBuffers,
    IN BOOLEAN HistoryNoDup);
