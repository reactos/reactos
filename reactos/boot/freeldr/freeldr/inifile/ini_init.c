/*
 *  FreeLoader
 *  Copyright (C) 2009     Hervé Poussineau  <hpoussin@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <debug.h>
DBG_DEFAULT_CHANNEL(INIFILE);

static LONG IniOpenIniFile(ULONG* FileId)
{
    CHAR FreeldrPath[MAX_PATH];
    LONG ret;

    //
    // Create full freeldr.ini path
    //
    MachDiskGetBootPath(FreeldrPath, sizeof(FreeldrPath));
    strcat(FreeldrPath, "\\freeldr.ini");

    // Try to open freeldr.ini
    ret = ArcOpen(FreeldrPath, OpenReadOnly, FileId);

    return ret;
}

BOOLEAN IniFileInitialize(VOID)
{
    FILEINFORMATION FileInformation;
    ULONG FileId; // File handle for freeldr.ini
    PCHAR FreeLoaderIniFileData;
    ULONG FreeLoaderIniFileSize, Count;
    LONG ret;
    BOOLEAN Success;
    TRACE("IniFileInitialize()\n");

    //
    // Open freeldr.ini
    //
    ret = IniOpenIniFile(&FileId);
    if (ret != ESUCCESS)
    {
        UiMessageBoxCritical("Error opening freeldr.ini or file not found.\nYou need to re-install FreeLoader.");
        return FALSE;
    }

    //
    // Get the file size
    //
    ret = ArcGetFileInformation(FileId, &FileInformation);
    if (ret != ESUCCESS || FileInformation.EndingAddress.HighPart != 0)
    {
        UiMessageBoxCritical("Error while getting informations about freeldr.ini.\nYou need to re-install FreeLoader.");
        return FALSE;
    }
    FreeLoaderIniFileSize = FileInformation.EndingAddress.LowPart;

    //
    // Allocate memory to cache the whole freeldr.ini
    //
    FreeLoaderIniFileData = FrLdrTempAlloc(FreeLoaderIniFileSize, TAG_INI_FILE);
    if (!FreeLoaderIniFileData)
    {
        UiMessageBoxCritical("Out of memory while loading freeldr.ini.");
        ArcClose(FileId);
        return FALSE;
    }

    //
    // Read freeldr.ini off the disk
    //
    ret = ArcRead(FileId, FreeLoaderIniFileData, FreeLoaderIniFileSize, &Count);
    if (ret != ESUCCESS || Count != FreeLoaderIniFileSize)
    {
        UiMessageBoxCritical("Error while reading freeldr.ini.");
        ArcClose(FileId);
        FrLdrTempFree(FreeLoaderIniFileData, TAG_INI_FILE);
        return FALSE;
    }

    //
    // Parse the .ini file data
    //
    Success = IniParseFile(FreeLoaderIniFileData, FreeLoaderIniFileSize);

    //
    // Do some cleanup, and return
    //
    ArcClose(FileId);
    FrLdrTempFree(FreeLoaderIniFileData, TAG_INI_FILE);

    return Success;
}
