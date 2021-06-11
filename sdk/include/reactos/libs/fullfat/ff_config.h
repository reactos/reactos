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
#define FF_LITTLE_ENDIAN				// Choosing the Byte-order of your system is important.
//#define FF_BIG_ENDIAN					// You may be able to provide better Byte-order swapping routines to FullFAT.
										// See ff_memory.c for more information.


//---------- LFN (Long File-name) SUPPORT
#define FF_LFN_SUPPORT					// Comment this out if you don't want to worry about Patent Issues.
										// FullFAT works great with LFNs and without. You choose, its your project!

//#define FF_INCLUDE_SHORT_NAME			// HT addition, in 'FF_DIRENT', beside FileName, ShortName will be filled as well
                                     	// Useful for debugging, but also some situations its useful to know both.
//---------- SHORTNAMES CAN USE THE CASE BITS
#define FF_SHORTNAME_CASE				// Works for XP+ e.g. short.TXT or SHORT.txt.


//---------- UNICODE SUPPORT
#define FF_UNICODE_SUPPORT			// If this is defined, then all of FullFAT's API's will expect to receive UTF-16 formatted strings.
										// FF_FindFirst() and FF_FindNext() will also return Filenames in UTF-16 format.
										// NOTE: This option may cause FullFAT to not "Clean-compile" when using GCC. This is because
										// pedantically GCC refuses to accept C99 library functions, unless the -std=c99 flag is used.
										// To use UNICODE (UTF-16, or UTF-32 depending on the size of wchar_t) you must have a C99 compliant
										// compiler and library.

//#define FF_UNICODE_UTF8_SUPPORT		// If this is defined, then all of FullFAT's API's will expect to receive UTF-8 formatted strings.
										// FF_FindFirst() and FF_FindNext() will also return Filenames in UTF-8 format.

										// Note the 2 UNICODE options are mutually exclusive. Only one can be enabled.

										// Ensure that dirents are big enough to hold the maximum UTF-8 sequence.


//---------- FAT12 SUPPORT
#define FF_FAT12_SUPPORT				// Enable FAT12 Suppport. You can reduce the code-size by commenting this out.
										// If you don't need FAT12 support, why have it. FAT12 is more complex to process,
										// therefore savings can be made by not having it.


//---------- TIME SUPPORT
#define FF_TIME_SUPPORT					// Should FullFAT use time stamping. Only if you have provided the relevant time drivers in ff_time.c
										// Note, by default ff_time.c is set-up for the Windows Demonstration. Please see ff_time.c to disable.


//---------- FILE SPACE ALLOCATION PERFORMANCE
										// Uncomment the prefered method. (Can only choose a single method).
#define FF_ALLOC_DEFAULT				// Only allocate as much as is needed. (Provides good performance, without wasting space).
//#define FF_ALLOC_DOUBLE				// Doubles the size of a file each time allocation is required. (When high-performance writing is required).


//---------- Use Native STDIO.h
//#define FF_USE_NATIVE_STDIO			// Makes FullFAT conform to values provided by your native STDIO.h file.


//---------- FREE SPACE CALCULATION
//#define FF_MOUNT_FIND_FREE			// Uncomment this option to check for Freespace on a volume mount. (Performance Penalty while mounting).
										// If not done in the mount, it will be done on the first call to FF_GetFreeSize() function.


//---------- FIND API WILD-CARD SUPPORT
#define FF_FINDAPI_ALLOW_WILDCARDS		// Defined to enable Wild-cards in the API. Disabling this, makes the API consistent with 1.0.x series.

#define FF_WILDCARD_CASE_INSENSITIVE	// Alter the case insensitivity of the Wild-card checking behaviour.


//---------- PATH CACHE ----------
#define FF_PATH_CACHE					// Enables a simply Path Caching mechanism that increases performance of repeated operations
										// within the same path. E.g. a copy \dir1\*.* \dir2\*.* command.
										// This command requires FF_MAX_PATH number of bytes of memory. (Defined below, default 2600).

#define FF_PATH_CACHE_DEPTH		5		// The Number of PATH's to Cache. (Memory Requirement ~= FF_PATH_CACHE_DEPTH * FF_MAX_PATH).


//---------- HASH CACHE					// Speed up File-creation with a HASH table. Provides up to 20x performance boost.
//#define FF_HASH_CACHE					// Enable HASH to speed up file creation.
#define FF_HASH_CACHE_DEPTH		10		// Number of Directories to be Hashed. (For CRC16 memory is 8KB * DEPTH)
#define FF_HASH_FUNCTION		CRC16	// Choose a 16-bit hash.
//#define FF_HASH_FUNCTION		CRC8	// Choose an 8-bit hash.


//---------- BLKDEV USES SEMAPHORE
#define FF_BLKDEV_USES_SEM				// When defined, each call to fnReadBlocks and fnWriteBlocks will be done while semaphore is locked
										// See also ff_safety.c
										// (HT addition) - Thanks to Hein Tibosch


//---------- MALLOC
										// These should map on to platform specific memory allocators.
#define	FF_MALLOC(aSize)				FF_Malloc(aSize)
#define	FF_FREE(apPtr)	 				FF_Free(apPtr)


//---------- IN-LINE FUNCTIONS
//---------- INLINE KeyWord				// Define FF_INLINE as your compiler's inline keyword. This is placed before the type qualifier.
#define FF_INLINE static __forceinline	// Keywords to inline functions (Windows)
//#define FF_INLINE static inline		// Standard for GCC

//---------- Inline Memory Independence Routines for better performance, but bigger codesize.
//#define FF_INLINE_MEMORY_ACCESS
//---------- Inline Block Calculation Routines for slightly better performance in critical sections.
//#define FF_INLINE_BLOCK_CALCULATIONS


//---------- 64-Bit Number Support
#define FF_64_NUM_SUPPORT				// This helps to give information about the FreeSpace and VolumeSize of a partition or volume.
										// If you cannot support 64-bit integers, then FullFAT still works, its just that the functions:
										// FF_GetFreeSize() and FF_GetVolumeSize() don't make sense when reporting sizes > 4GB.


//---------- Driver Sleep Time
#define FF_DRIVER_BUSY_SLEEP	20		// How long FullFAT should sleep the thread for in ms, if FF_ERR_DRIVER_BUSY is recieved.


//---------- DEBUGGING FEATURES (HELPFUL ERROR MESSAGES)
#define FF_DEBUG						// Enable the Error Code string functions. const FF_T_INT8 *FF_GetErrMessage( FF_T_SINT32 iErrorCode);
										// Uncommenting this just stops FullFAT error strings being compiled.
										// Further calls to FF_GetErrMessage() are safe, and simply returns a pointer to a NULL string. ("").
										// This should be disabled to reduce code-size dramatically.


//---------- AUTOMATIC SETTINGS DO NOT EDIT -- These configure your options from above, and check sanity!

#ifdef FF_LFN_SUPPORT
#define FF_MAX_FILENAME		(260)
#else
#define	FF_MAX_FILENAME		(13)
#endif

#ifdef FF_USE_NATIVE_STDIO
#ifdef	MAX_PATH
#define FF_MAX_PATH MAX_PATH
#elif	PATH_MAX
#define	FF_MAX_PATH	PATH_MAX
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

#ifdef FF_UNICODE_SUPPORT
#ifdef FF_UNICODE_UTF8_SUPPORT
#error FullFAT Invalid ff_config.h file: Must choose a single UNICODE support option. FF_UNICODE_SUPPORT for UTF-16, FF_UNICODE_UTF8_SUPPORT for UTF-8.
#endif
#endif

#ifndef FF_FAT_CHECK	// FF_FAT_CHECK is now forced.
#define FF_FAT_CHECK
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

#ifdef FF_HASH_CACHE

#if FF_HASH_FUNCTION == CRC16
#define FF_HASH_TABLE_SIZE 8192
#elif FF_HASH_FUNCTION == CRC8
#define FF_HASH_TABLE_SIZE 32
#else
#error FullFAT Invalid ff_config.h file: Invalid Hashing function selected. CRC16 or CRC8!
#endif

#endif

#endif

//---------- END-OF-CONFIGURATION
