/*
 * PROJECT:     ReactOS File System Recognizer
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CDFS Recognizer
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
 */

// Information from http://wiki.osdev.org/ISO_9660#Volume_Descriptors

// Structures
typedef struct _VD_HEADER
{
    char Type;
    char Identifier[5];
    char Version;
}
VD_HEADER, *PVD_HEADER;

// Constants
#define VD_HEADER_OFFSET        32768       // Offset of the VD Header
#define VD_IDENTIFIER           "CD001"     // Identifier that must be in the Volume Descriptor
#define VD_IDENTIFIER_LENGTH    5           // Character count of VD_IDENTIFIER
#define VD_TYPE_PRIMARY         1           // Type code for Primary Volume Descriptor
#define VD_VERSION              1           // Volume Descriptor Version
