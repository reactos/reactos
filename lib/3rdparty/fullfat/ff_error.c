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

/**
 *	@file		ff_error.c
 *	@author		James Walmsley
 *	@ingroup	ERROR
 *
 *	@defgroup	ERR Error Message
 *	@brief		Used to return pretty strings for FullFAT error codes.
 *
 **/
#include "ff_config.h"
#include "ff_types.h"
#include "ff_error.h"

#ifdef FF_DEBUG
const struct _FFERRTAB
{
    const FF_T_INT8 * const strErrorString;
    const FF_T_SINT32 iErrorCode;

} gcpFullFATErrorTable[] =
{
	{"Unknown or Generic Error! - Please contact FullFAT DEV - james@worm.me.uk",	-1000},
	{"No Error.",																	FF_ERR_NONE},
	{"Null Pointer provided, (probably for IOMAN).",								FF_ERR_NULL_POINTER},
    {"Not enough memory (malloc() returned NULL).",									FF_ERR_NOT_ENOUGH_MEMORY},
    {"Device Driver returned a FATAL error!.",										FF_ERR_DEVICE_DRIVER_FAILED},
    {"The blocksize is not 512 multiple.",											FF_ERR_IOMAN_BAD_BLKSIZE},
    {"The memory size, is not a multiple of the blocksize. (Atleast 2 Blocks).",	FF_ERR_IOMAN_BAD_MEMSIZE},
    {"Device is already registered, use FF_UnregisterBlkDevice() first.",			FF_ERR_IOMAN_DEV_ALREADY_REGD},
    {"No mountable partition was found on the specified device.",					FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION},
    {"The format of the MBR was unrecognised.",										FF_ERR_IOMAN_INVALID_FORMAT},
    {"The provided partition number is out-of-range (0 - 3).",						FF_ERR_IOMAN_INVALID_PARTITION_NUM},
    {"The selected partition / volume doesn't appear to be FAT formatted.",			FF_ERR_IOMAN_NOT_FAT_FORMATTED},
    {"Cannot register device. (BlkSize not a multiple of 512).",					FF_ERR_IOMAN_DEV_INVALID_BLKSIZE},
    {"Cannot unregister device, a partition is still mounted.",						FF_ERR_IOMAN_PARTITION_MOUNTED},
    {"Cannot unmount the partition while there are active FILE handles.",			FF_ERR_IOMAN_ACTIVE_HANDLES},
	{"The GPT partition header appears to be corrupt, refusing to mount.",			FF_ERR_IOMAN_GPT_HEADER_CORRUPT},
    {"Cannot open the file, file already in use.",									FF_ERR_FILE_ALREADY_OPEN},
    {"The specified file could not be found.",										FF_ERR_FILE_NOT_FOUND},
    {"Cannot open a Directory.",													FF_ERR_FILE_OBJECT_IS_A_DIR},
	{"Cannot open for writing: File is marked as Read-Only.",						FF_ERR_FILE_IS_READ_ONLY},
    {"Path not found.",																FF_ERR_FILE_INVALID_PATH},
    {"A file or folder of the same name already exists.",							FF_ERR_DIR_OBJECT_EXISTS},
    {"FF_ERR_DIR_DIRECTORY_FULL",													FF_ERR_DIR_DIRECTORY_FULL},
    {"FF_ERR_DIR_END_OF_DIR",														FF_ERR_DIR_END_OF_DIR},
    {"The directory is not empty.",													FF_ERR_DIR_NOT_EMPTY},
	{"Could not extend File or Folder - No Free Space!",							FF_ERR_FAT_NO_FREE_CLUSTERS},
	{"Could not find the directory specified by the path.",							FF_ERR_DIR_INVALID_PATH},
	{"The Root Dir is full, and cannot be extended on Fat12 or 16 volumes.",		FF_ERR_DIR_CANT_EXTEND_ROOT_DIR},
	{"File operation failed - the file was not opened for writing.",				FF_ERR_FILE_NOT_OPENED_IN_WRITE_MODE},
	{"File operation failed - the file was not opened for reading.",				FF_ERR_FILE_NOT_OPENED_IN_READ_MODE},
	{"File operation failed - could not extend file.",								FF_ERR_FILE_EXTEND_FAILED},
	{"Destination file already exists.",											FF_ERR_FILE_DESTINATION_EXISTS},
	{"Source file was not found.",													FF_ERR_FILE_SOURCE_NOT_FOUND},
	{"Destination path (dir) was not found.",										FF_ERR_FILE_DIR_NOT_FOUND},
	{"Failed to create the directory Entry.",										FF_ERR_FILE_COULD_NOT_CREATE_DIRENT},
	{"Not enough free disk space to complete the disk transaction.",				FF_ERR_IOMAN_NOT_ENOUGH_FREE_SPACE},
	{"Attempted to Read a sector out of bounds.",									FF_ERR_IOMAN_OUT_OF_BOUNDS_READ},
	{"Attempted to Write a sector out of bounds.",									FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE},
};

/**
 *	@public
 *	@brief	Returns a pointer to a string relating to a FullFAT error code.
 *	
 *	@param	iErrorCode	The error code.
 *
 *	@return	Pointer to a string describing the error.
 *
 **/
const FF_T_INT8 *FF_GetErrMessage(FF_ERROR iErrorCode) {
    FF_T_UINT32 stCount = sizeof (gcpFullFATErrorTable) / sizeof ( struct _FFERRTAB);
    while (stCount--){
        if (gcpFullFATErrorTable[stCount].iErrorCode == iErrorCode) {
            return gcpFullFATErrorTable[stCount].strErrorString;
        }
    }
	return gcpFullFATErrorTable[0].strErrorString;
}
#endif
