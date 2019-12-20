/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Header file for ramdisk support.
 * COPYRIGHT:   Copyright 2008 ReactOS Portable Systems Group
 *              Copyright 2009 Hervé Poussineau
 *              Copyright 2019 Hermes Belusca-Maito
 */

#pragma once

ARC_STATUS
RamDiskInitialize(
    IN BOOLEAN InitRamDisk,
    IN PCSTR LoadOptions OPTIONAL,
    IN PCSTR DefaultPath OPTIONAL);

extern PVOID gInitRamDiskBase;
extern ULONG gInitRamDiskSize;
