/*****************************************************************************
 *  FullFAT - High Performance, Thread-Safe Embedded FAT File-System         *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.       *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/fullfat for more information.                      *
 *  Or  http://fullfat.googlecode.com/ for latest releases and the wiki.     *
 *****************************************************************************/
#ifndef _FF_CONFIG_H_
#define _FF_CONFIG_H_
/*
	Here you can change the configuration of FullFAT as appropriate to your
	platform.
*/

//---------- ENDIANESS
#define FF_LITTLE_ENDIAN		// Choosing the Byte-order of your system is important.	
//#define FF_BIG_ENDIAN			// You may be able to provide better Byte-order swapping routines to FullFAT.
								// See ff_memory.c for more information.

//---------- LFN (Long File-name) SUPPORT
#define FF_LFN_SUPPORT			// Comment this out if you don't want to worry about Patent Issues.
								// FullFAT works great with LFNs and without. You choose, its your project!

//---------- LEGAL LFNS
//#define FF_LEGAL_LFNS			// Enabling this define causes FullFAT to not infringe on any of Microsoft's patents when making LFN names.
								// To do this, it will only create LFNs and no shortnames. Microsofts patents are only relevent when mapping
								// a shortname to a long name. This is the same way that Linux gets around the FAT legal issues:
								// see http://lkml.org/lkml/2009/6/26/314
								//
								// Enabling this may break compatibility with devices that cannot read LFN filenames.
								// Enabling this option causes no compatibility issues when reading any media.

//---------- TIME SUPPORT
#define FF_TIME_SUPPORT			// Should FullFAT use time stamping. Only if you have provided the relevant time drivers in ff_time.c
								// Note, by default ff_time.c is set-up for the Windows Demonstration. Please see ff_time.c to disable.

//---------- FILE SPACE ALLOCATION PERFORMANCE
								// Uncomment the prefered method. (Can only choose a single method).
#define FF_ALLOC_DEFAULT		// Only allocate as much as is needed. (Provides good performance, without wasting space).
//#define FF_ALLOC_DOUBLE		// Doubles the size of a file each time allocation is required. (When high-performance writing is required).

//---------- Use Native STDIO.h
//#define FF_USE_NATIVE_STDIO	// Makes FullFAT conform to values provided by your native STDIO.h file.

//---------- FREE SPACE CALCULATION
//#define FF_MOUNT_FIND_FREE	// Uncomment this option to check for Freespace on a volume mount. (Performance Penalty while mounting).
								// If not done in the mount, it will be done on the first call to FF_GetFreeSize() function.

//---------- PATH CACHE
#define FF_PATH_CACHE			// Enables a simply Path Caching mechanism that increases performance of repeated operations
								// within the same path. E.g. a copy \dir1\*.* \dir2\*.* command.
								// This command requires FF_MAX_PATH number of bytes of memory. (Defined below, default 2600).

//---------- BLKDEV USES SEMAPHORE
#define FF_BLKDEV_USES_SEM		// When defined, each call to fnReadBlocks and fnWriteBlocks will be done while semaphore is locked
								// See also ff_safety.c
								// (HT addition)


#define FF_PATH_CACHE_DEPTH	2	// The Number of PATH's to Cache.

//---------- DON'T USE MALLOC
//#define FF_NO_MALLOC

#define	FF_MALLOC(aSize)		malloc(aSize)
#define	FF_FREE(apPtr)			free(apPtr)

//#define FF_INLINE_MEMORY_ACCESS

//#define FF_INLINE static __forceinline	// Keywords to inline functions (Windows)
#define FF_INLINE static inline				// Standard for GCC


//---------- Hash Table Support
//#define FF_HASH_TABLE_SUPPORT	// Enable HASH to speed up file creation.
#ifdef	FF_HASH_TABLE_SUPPORT
#define FF_HASH_FUNCTION	CRC16
//#define FF_HASH_FUNCTION	CRC8
#endif



//---------- FAT12 SUPPORT
#define FF_FAT12_SUPPORT		// Enable FAT12 Suppport. You can reduce the code-size by commenting this out.
								// If you don't need FAT12 support, why have it. FAT12 is more complex to process,
								// therefore savings can be made by not having it.

//---------- 64-Bit Number Support
#define FF_64_NUM_SUPPORT		// This helps to give information about the FreeSpace and VolumeSize of a partition or volume.
								// If you cannot support 64-bit integers, then FullFAT still works, its just that the functions:
								// FF_GetFreeSize() and FF_GetVolumeSize() don't make sense when reporting sizes > 4GB.

//---------- Driver Sleep Time	// How long FullFAT should sleep the thread for in ms, if FF_ERR_DRIVER_BUSY is recieved.
#define FF_DRIVER_BUSY_SLEEP	20	

//---------- Debugging Features
#define FF_DEBUG				// Enable the Error Code string functions. const FF_T_INT8 *FF_GetErrMessage( FF_T_SINT32 iErrorCode);
								// Uncommenting this just stops FullFAT error strings being compiled.

//---------- Actively Determine if partition is FAT
#define FF_FAT_CHECK			// This is experimental, so if FullFAT won't mount your volume, comment this out
								// Also report the problem to james@worm.me.uk


//---------- AUTOMATIC SETTINGS DO NOT EDIT -- These configure your options from above, and check sanity!

#ifdef FF_LFN_SUPPORT
#define FF_MAX_FILENAME		(129)
#else
#define	FF_MAX_FILENAME		13
#endif

#ifdef FF_USE_NATIVE_STDIO
#ifdef	MAX_PATH
#define FF_MAX_PATH MAX_PATH
#else
#define FF_MAX_PATH	2600
#endif
#else
#define FF_MAX_PATH	2600
#endif

#ifndef FF_ALLOC_DOUBLE
#ifndef FF_ALLOC_DEFAULT
#error	FullFAT Invalid ff_config.h file: A file allocation method must be specified. See ff_config.h file.
#endif
#endif

#ifdef FF_ALLOC_DOUBLE
#ifdef FF_ALLOC_DEFAULT
#error FullFAT Invalid ff_config.h file: Must choose a single option for File Allocation Method. DOUBLE or DEFAULT. See ff_config.h file.
#endif
#endif

#ifndef FF_LITTLE_ENDIAN
#ifndef FF_BIG_ENDIAN
#error	FullFAT Invalid ff_config.h file: An ENDIANESS must be defined for your platform. See ff_config.h file.
#endif
#endif

#ifdef FF_LITTLE_ENDIAN
#ifdef FF_BIG_ENDIAN
#error FullFAT Invalid ff_config.h file: Cannot be BIG and LITTLE ENDIAN, choose either or BIG or LITTLE. See ff_config.h file.
#endif
#endif

#ifdef FF_HASH_TABLE_SUPPORT

#if FF_HASH_FUNCTION == CRC16
#define FF_HASH_TABLE_SIZE 8192
#elif FF_HASH_FUNCTION == CRC8
#define FF_HASH_TABLE_SIZE 32
#else
#error Invalid Hashing function selected. CRC16 or CRC8! 
#endif

#endif

#endif
