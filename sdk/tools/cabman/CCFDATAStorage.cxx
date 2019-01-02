/*
 * PROJECT:     ReactOS cabinet manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CCFDATAStorage class implementation for Linux/Unix
 * COPYRIGHT:   Copyright 2017 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017 Colin Finck <mail@colinfinck.de>
 *              Copyright 2018 Dmitry Bagdanov <dimbo_job@mail.ru>   
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(_WIN32)
#include <dirent.h>
#endif

#include "cabinet.h"
#include "raw.h"
#include "mszip.h"

#if !defined(CAB_READ_ONLY)

 /**
 * @name CCFDATAStorage class
 * @implemented
 *
 * Default constructor
 */
CCFDATAStorage::CCFDATAStorage()
{
    FileHandle = NULL;
}

/**
* @name CCFDATAStorage class
* @implemented
*
* Default destructor
*/
CCFDATAStorage::~CCFDATAStorage()
{
    ASSERT(FileHandle == NULL);
}

/**
* @name CCFDATAStorage class
* @implemented
*
* Creates the file
*
* @return
* Status of operation
*/
ULONG CCFDATAStorage::Create()
{
    if ((FileHandle = tmpfile()) == NULL)
    {
        /* Workaround for breakage on some Windows system */
        FileHandle = fopen(tmpnam(NULL) + 1, "wb");
        if (FileHandle == NULL)
            return CAB_STATUS_CANNOT_CREATE;
    }

    return CAB_STATUS_SUCCESS;
}

/**
* @name CCFDATAStorage class
* @implemented
*
* Destroys the file
*
* @return
* Status of operation
*/
ULONG CCFDATAStorage::Destroy()
{
    ASSERT(FileHandle != NULL);

    fclose(FileHandle);

    FileHandle = NULL;

    return CAB_STATUS_SUCCESS;
}

/**
* @name CCFDATAStorage class
* @implemented
*
* Truncate the scratch file to zero bytes
*
* @return
* Status of operation
*/
ULONG CCFDATAStorage::Truncate()
{
    fclose(FileHandle);
    FileHandle = tmpfile();
    if (FileHandle == NULL)
    {
        DPRINT(MID_TRACE, ("ERROR '%i'.\n", errno));
        return CAB_STATUS_FAILURE;
    }

    return CAB_STATUS_SUCCESS;
}

/**
* @name CCFDATAStorage class
* @implemented
*
* Returns current position in file
*
* @return
* Current position
*/
ULONG CCFDATAStorage::Position()
{
    return (ULONG)ftell(FileHandle);
}


/**
* @name CCFDATAStorage class
* @implemented
*
* Seeks to an absolute position
*
* @param Position
* Absolute position to seek to
*
* @return
* Status of operation
*/
ULONG CCFDATAStorage::Seek(LONG Position)
{
    if (fseek(FileHandle, (off_t)Position, SEEK_SET) != 0)
        return CAB_STATUS_FAILURE;
    else
        return CAB_STATUS_SUCCESS;
}


/**
* @name CCFDATAStorage class
* @implemented
*
* Reads a CFDATA block from the file
*
* @param Data
* Pointer to CFDATA block for the buffer
*
* @param Buffer
* Pointer to buffer to store data read
*
* @param BytesRead
* Pointer to buffer to write number of bytes read
*
* @return
* Status of operation
*/
ULONG CCFDATAStorage::ReadBlock(PCFDATA Data, void* Buffer, PULONG BytesRead)
{
    *BytesRead = fread(Buffer, 1, Data->CompSize, FileHandle);
    if (*BytesRead != Data->CompSize)
        return CAB_STATUS_CANNOT_READ;

    return CAB_STATUS_SUCCESS;
}


/**
* @name CCFDATAStorage class
* @implemented
*
* Writes a CFDATA block to the file
*
* @param Data
* Pointer to CFDATA block for the buffer
*
* @param Buffer
* Pointer to buffer with data to write
*
* @param BytesWritten
* Pointer to buffer to write number of bytes written
*
* @return
* Status of operation
*/
ULONG CCFDATAStorage::WriteBlock(PCFDATA Data, void* Buffer, PULONG BytesWritten)
{
    *BytesWritten = fwrite(Buffer, 1, Data->CompSize, FileHandle);
    if (*BytesWritten != Data->CompSize)
        return CAB_STATUS_CANNOT_WRITE;

    return CAB_STATUS_SUCCESS;
}


#endif /* CAB_READ_ONLY */
