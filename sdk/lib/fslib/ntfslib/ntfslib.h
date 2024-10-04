/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/ntfslib.h
 * PURPOSE:     NTFS Lib
 * PROGRAMMERS: Daniel Victor
 */

#pragma once

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <fmifs/fmifs.h>

#define NDEBUG
#include <debug.h>

typedef unsigned __int64 QWORD;

// Structure of the NTFS Boot Sector
typedef struct NTFS_BootSector {
    BYTE jump[3]; 
    BYTE oem_id[8]; 
    WORD bytes_per_sector; 
    BYTE sectors_per_cluster; 
    WORD reserved_sectors; 
    BYTE media_descriptor; 
    WORD sectors_per_track; 
    WORD number_of_heads; 
    DWORD hidden_sectors; 
    QWORD total_sectors; 
    QWORD mft_cluster; 
    QWORD mft_mirror_cluster; 
    CHAR clusters_per_file_record_segment; 
    CHAR clusters_per_index_block; 
    QWORD volume_serial_number; 
    BYTE code[426]; 
    WORD signature; 
} NTFS_BootSector;

// Structure of the Master File Table (MFT)
typedef struct NTFS_MFTEntry {
    DWORD type; 
    WORD usa_offset; 
    WORD usa_count; 
    QWORD lsn; 
    WORD sequence_number; 
    WORD link_count; 
    WORD attrs_offset; 
    WORD flags; 
    DWORD used_size; 
    DWORD allocated_size; 
    QWORD base_record; 
    WORD next_attr_id; 
} NTFS_MFTEntry;

// Structure of the Bitmap
typedef struct NTFS_Bitmap {
    QWORD length; 
    BYTE *bitmap;
} NTFS_Bitmap;

