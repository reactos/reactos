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

BOOLEAN IniFileInitialize(VOID)
{
    FILEINFORMATION FileInformation;
    ULONG FileId; // File handle for freeldr.ini
    PCHAR FreeLoaderIniFileData;
    ULONG FreeLoaderIniFileSize, Count;
    ARC_STATUS Status;
    BOOLEAN Success;

    TRACE("IniFileInitialize()\n");

    /* Try to open freeldr.ini */
    Status = FsOpenFile("freeldr.ini", FrLdrBootPath, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        ERR("Error while opening freeldr.ini, Status: %d\n", Status);

        /* Try to open boot.ini */
        Status = FsOpenFile("boot.ini", FrLdrBootPath, OpenReadOnly, &FileId);
        if (Status != ESUCCESS)
        {
            ERR("Error while opening boot.ini, Status: %d\n", Status);
            UiMessageBoxCritical("Error opening freeldr.ini/boot.ini or file not found.\nYou need to re-install FreeLoader.");
            return FALSE;
        }
    }

    /* Get the file size */
    Status = ArcGetFileInformation(FileId, &FileInformation);
    if (Status != ESUCCESS || FileInformation.EndingAddress.HighPart != 0)
    {
        UiMessageBoxCritical("Error while getting informations about freeldr.ini.\nYou need to re-install FreeLoader.");
        ArcClose(FileId);
        return FALSE;
    }
    FreeLoaderIniFileSize = FileInformation.EndingAddress.LowPart;

    /* Allocate memory to cache the whole freeldr.ini */
    FreeLoaderIniFileData = FrLdrTempAlloc(FreeLoaderIniFileSize, TAG_INI_FILE);
    if (!FreeLoaderIniFileData)
    {
        UiMessageBoxCritical("Out of memory while loading freeldr.ini.");
        ArcClose(FileId);
        return FALSE;
    }

    /* Load freeldr.ini from the disk */
    Status = ArcRead(FileId, FreeLoaderIniFileData, FreeLoaderIniFileSize, &Count);
    if (Status != ESUCCESS || Count != FreeLoaderIniFileSize)
    {
        ERR("Error while reading freeldr.ini, Status: %d\n", Status);
        UiMessageBoxCritical("Error while reading freeldr.ini.");
        ArcClose(FileId);
        FrLdrTempFree(FreeLoaderIniFileData, TAG_INI_FILE);
        return FALSE;
    }

    /* Parse the .ini file data */
    Success = IniParseFile(FreeLoaderIniFileData, FreeLoaderIniFileSize);

    /* Do some cleanup, and return */
    ArcClose(FileId);
    FrLdrTempFree(FreeLoaderIniFileData, TAG_INI_FILE);

    return Success;
}
