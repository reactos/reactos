/*
 * PROJECT:     ReactOS Spooler Router API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for AlignRpcPtr/UndoAlignRpcPtr
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <spoolss.h>

START_TEST(AlignRpcPtr)
{
    char* pMemory;
    char* pInputBuffer;
    char* pOutputBuffer;
    DWORD cbBuffer;
    PDWORD pcbBuffer;

    // Allocate memory with GlobalAlloc. It is guaranteed to be aligned to a 8-byte boundary.
    pMemory = (char*)GlobalAlloc(GMEM_FIXED, 16);

    // First try AlignRpcPtr with already aligned memory and buffer size. It should leave everything unchanged.
    pInputBuffer = pMemory;
    cbBuffer = 8;
    pOutputBuffer = (char*)AlignRpcPtr(pInputBuffer, &cbBuffer);
    ok(pOutputBuffer == pInputBuffer, "pOutputBuffer != pInputBuffer\n");
    ok(cbBuffer == 8, "cbBuffer is %lu\n", cbBuffer);

    // Now try it with unaligned buffer size. The size should be aligned down while the buffer stays the same.
    pInputBuffer = pMemory;
    cbBuffer = 7;
    pOutputBuffer = (char*)AlignRpcPtr(pInputBuffer, &cbBuffer);
    ok(pOutputBuffer == pInputBuffer, "pOutputBuffer != pInputBuffer\n");
    ok(cbBuffer == 4, "cbBuffer is %lu\n", cbBuffer);

    // Now try with unaligned memory, but aligned buffer size. A new buffer is allocated while the size stays the same.
    // The allocated buffer is then freed with UndoAlignRpcPtr. It is important to specify 0 as the size here, otherwise
    // the NULL pointer for pDestinationBuffer is accessed.
    pInputBuffer = pMemory + 1;
    cbBuffer = 8;
    pOutputBuffer = (char*)AlignRpcPtr(pInputBuffer, &cbBuffer);
    ok(pOutputBuffer != pInputBuffer, "pOutputBuffer == pInputBuffer\n");
    ok(cbBuffer == 8, "cbBuffer is %lu\n", cbBuffer);
    ok(!UndoAlignRpcPtr(NULL, pOutputBuffer, 0, NULL), "UndoAlignRpcPtr returns something\n");

    // Now try with memory and buffer size unaligned. A new buffer of the aligned down size is allocated.
    pInputBuffer = pMemory + 1;
    cbBuffer = 7;
    pOutputBuffer = (char*)AlignRpcPtr(pInputBuffer, &cbBuffer);
    ok(pOutputBuffer != pInputBuffer, "pOutputBuffer == pInputBuffer\n");
    ok(cbBuffer == 4, "cbBuffer is %lu\n", cbBuffer);

    // Prove that AlignRpcPtr also works with a NULL buffer. The size should be aligned down.
    cbBuffer = 6;
    ok(!AlignRpcPtr(NULL, &cbBuffer), "AlignRpcPtr returns something\n");
    ok(cbBuffer == 4, "cbBuffer is %lu\n", cbBuffer);

    // We can also test all parameters of UndoAlignRpcPtr here.
    // Because pOutputBuffer != pInputBuffer, it copies the given 4 bytes from (aligned) pOutputBuffer to (unaligned) pInputBuffer
    // while aligning up the given 7 bytes in our passed &cbBuffer.
    // &cbBuffer is also returned.
    strcpy(pOutputBuffer, "abc");
    strcpy(pInputBuffer, "XXXXXXXXX");
    cbBuffer = 5;
    pcbBuffer = UndoAlignRpcPtr(pInputBuffer, pOutputBuffer, 4, &cbBuffer);
    ok(strcmp(pInputBuffer, "abc") == 0, "pInputBuffer is %s\n", pInputBuffer);
    ok(pcbBuffer == &cbBuffer, "pcbBuffer != &cbBuffer\n");
    ok(cbBuffer == 8, "cbBuffer is %lu\n", cbBuffer);

    // Prove that UndoAlignRpcPtr works without any parameters and doesn't try to copy data from NULL pointers.
    ok(!UndoAlignRpcPtr(NULL, NULL, 0, NULL), "UndoAlignRpcPtr returns something\n");
    ok(!UndoAlignRpcPtr(NULL, NULL, 6, NULL), "UndoAlignRpcPtr returns something\n");

    // Prove that UndoAlignRpcPtr doesn't access source and destination memory at all when they are equal.
    // If it did, it should crash here, because I'm giving invalid memory addresses.
    ok(!UndoAlignRpcPtr((PVOID)1, (PVOID)1, 4, NULL), "UndoAlignRpcPtr returns something\n");

    // Prove that the pcbNeeded parameter of UndoAlignRpcPtr works independently and aligns up to a DWORD.
    cbBuffer = 0xFFFFFFFD;
    pcbBuffer = UndoAlignRpcPtr(NULL, NULL, 0, &cbBuffer);
    ok(pcbBuffer == &cbBuffer, "pcbBuffer != &cbBuffer\n");
    ok(cbBuffer == 0, "cbBuffer is %lu\n", cbBuffer);

    GlobalFree(pMemory);
}
