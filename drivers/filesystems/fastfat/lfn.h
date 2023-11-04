/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Lfn.h

Abstract:

    This module defines the on-disk structure of long file names on FAT.


--*/

#ifndef _LFN_
#define _LFN_

//
//  This strucure defines the on disk format on long file name dirents.
//

typedef struct _PACKED_LFN_DIRENT {
    UCHAR     Ordinal;    //  offset =  0
    UCHAR     Name1[10];  //  offset =  1 (Really 5 chars, but not WCHAR aligned)
    UCHAR     Attributes; //  offset = 11
    UCHAR     Type;       //  offset = 12
    UCHAR     Checksum;   //  offset = 13
    WCHAR     Name2[6];   //  offset = 14
    USHORT    MustBeZero; //  offset = 26
    WCHAR     Name3[2];   //  offset = 28
} PACKED_LFN_DIRENT;      //  sizeof = 32
typedef PACKED_LFN_DIRENT *PPACKED_LFN_DIRENT;

#define FAT_LAST_LONG_ENTRY             0x40 // Ordinal field
#define FAT_LONG_NAME_COMP              0x0  // Type field

//
//  A packed lfn dirent is already quadword aligned so simply declare a
//  lfn dirent as a packed lfn dirent.
//

typedef PACKED_LFN_DIRENT LFN_DIRENT;
typedef LFN_DIRENT *PLFN_DIRENT;

//
//  This is the largest size buffer we would ever need to read an Lfn
//

#define MAX_LFN_CHARACTERS              260
#define MAX_LFN_DIRENTS                 20

#define FAT_LFN_DIRENTS_NEEDED(NAME) (((NAME)->Length/sizeof(WCHAR) + 12)/13)

#endif // _LFN_

