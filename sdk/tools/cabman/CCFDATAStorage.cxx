/*
 * PROJECT:     ReactOS cabinet manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CCFDATAStorage class implementation
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

#include "CCFDATAStorage.h"
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
#if defined(_WIN32)
    char TmpName[PATH_MAX];
    char *pName;
    int length;

    if (tmpnam(TmpName) == NULL)
        return CAB_STATUS_CANNOT_CREATE;

    /* Append 'tmp' if the file name ends with a dot */
    length = strlen(TmpName);
    if (length > 0 && TmpName[length - 1] == '.')
        strcat(TmpName, "tmp");

    /* Skip a leading slash or backslash */
    pName = TmpName;
    if (*pName == '/' || *pName == '\\')
        pName++;

    strcpy(FullName, pName);

    FileHandle = fopen(FullName, "w+b");
    if (FileHandle == NULL)
        return CAB_STATUS_CANNOT_CREATE;
#else
    if ((FileHandle = tmpfile()) == NULL)
        return CAB_STATUS_CANNOT_CREATE;
#endif
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

#if defined(_WIN32)
    remove(FullName);
#endif

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
#if defined(_WIN32)
    FileHandle = fopen(FullName, "w+b");
#else
    FileHandle = tmpfile();
#endif
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
