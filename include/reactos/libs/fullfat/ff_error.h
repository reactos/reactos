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
#ifndef _FF_ERROR_H_
#define _FF_ERROR_H_

/**
 *	@file		ff_error.h
 *	@author		James Walmsley
 *	@ingroup	ERROR
 **/

#include "ff_config.h"
#include "ff_types.h"

/*	FullFAT defines different Error-Code spaces for each module. This ensures
	that all error codes remain unique, and their meaning can be quickly identified.
*/
// Global Error Codes
#define FF_ERR_NONE								  0	///< No Error
#define FF_ERR_NULL_POINTER						 -2	///< pIoman was NULL.
#define FF_ERR_NOT_ENOUGH_MEMORY				 -3	///< malloc() failed! - Could not allocate handle memory.
#define FF_ERR_DEVICE_DRIVER_FAILED				 -4	///< The Block Device driver reported a FATAL error, cannot continue.


// IOMAN Error Codes
#define	FF_ERR_IOMAN_BAD_BLKSIZE				-11	///< The provided blocksize was not a multiple of 512.
#define FF_ERR_IOMAN_BAD_MEMSIZE				-12	///< The memory size was not a multiple of the blocksize.
#define FF_ERR_IOMAN_DEV_ALREADY_REGD			-13 ///< Device was already registered. Use FF_UnRegister() to re-use this IOMAN with another device.
#define FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION		-14	///< A mountable partition could not be found on the device.
#define FF_ERR_IOMAN_INVALID_FORMAT				-15	///< The 
#define FF_ERR_IOMAN_INVALID_PARTITION_NUM		-16	///< The partition number provided was out of range.
#define FF_ERR_IOMAN_NOT_FAT_FORMATTED			-17	///< The partition did not look like a FAT partition.
#define FF_ERR_IOMAN_DEV_INVALID_BLKSIZE		-18 ///< IOMAN object BlkSize is not compatible with the blocksize of this device driver.
#define FF_ERR_IOMAN_PARTITION_MOUNTED			-19	///< Device is in use by an actively mounted partition. Unmount the partition first.
#define FF_ERR_IOMAN_ACTIVE_HANDLES				-20 ///< The partition cannot be unmounted until all active file handles are closed. (There may also be active handles on the cache).


// File Error Codes								-30 +
#define FF_ERR_FILE_ALREADY_OPEN				-30	///< File is in use.
#define FF_ERR_FILE_NOT_FOUND					-31	///< File was not found.
#define FF_ERR_FILE_OBJECT_IS_A_DIR				-32	///< Tried to FF_Open() a Directory.
#define FF_ERR_FILE_IS_READ_ONLY				-33	///< Tried to FF_Open() a file marked read only.
#define FF_ERR_FILE_INVALID_PATH				-34	///< The path of the file was not found.
#define FF_ERR_FILE_NOT_OPENED_IN_WRITE_MODE	-35
#define FF_ERR_FILE_NOT_OPENED_IN_READ_MODE		-36
#define FF_ERR_FILE_EXTEND_FAILED				-37	///< Could not extend the file.
#define FF_ERR_FILE_DESTINATION_EXISTS			-38
#define FF_ERR_FILE_SOURCE_NOT_FOUND			-39
#define FF_ERR_FILE_DIR_NOT_FOUND				-40
#define FF_ERR_FILE_COULD_NOT_CREATE_DIRENT		-41

// Directory Error Codes						-50 +
#define FF_ERR_DIR_OBJECT_EXISTS				-50	///< A file or folder of the same name already exists in the current directory.
#define FF_ERR_DIR_DIRECTORY_FULL				-51	///< No more items could be added to the directory.
#define FF_ERR_DIR_END_OF_DIR					-52	///
#define FF_ERR_DIR_NOT_EMPTY					-53	///< Cannot delete a directory that contains files or folders.
#define FF_ERR_DIR_INVALID_PATH					-54 ///< Could not find the directory specified by the path.
#define FF_ERR_DIR_CANT_EXTEND_ROOT_DIR			-55	///< Can't extend the root dir.

// Fat Error Codes								-70 +
#define FF_ERR_FAT_NO_FREE_CLUSTERS				-70	///< No more free space is available on the disk.

#ifdef FF_DEBUG
const FF_T_INT8 *FF_GetErrMessage(FF_ERROR iErrorCode);
#else
#define FF_GetErrMessage(X) ""					// A special MACRO incase FF_GetErrMessage() isn't gated with FF_DEBUG
#endif											// Function call is safely replaced with a NULL string.

#endif

