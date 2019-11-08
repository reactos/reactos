/*
 * PROJECT:     ReactOS FreeLoader installer for Linux
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume functions header file
 * COPYRIGHT:   Copyright 2001 Brian Palmer (brianp@sginet.com)
 *              Copyright 2019 Arnav Bhatt (arnavbhatt2004@gmail.com)
 */

#pragma once

bool    OpenVolume(char* lpszVolumeName);
void    CloseVolume(void);
bool    ReadVolumeSector(long SectorNumber, void* SectorBuffer);
bool    WriteVolumeSector(long SectorNumber, void* SectorBuffer);
