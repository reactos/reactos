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
 *	@file		ff_format.c
 *	@author		James Walmsley
 *	@ingroup	FORMAT
 *
 *	@defgroup	FORMAT Independent FAT Formatter
 *	@brief		Provides an interface to format a partition with FAT.
 *
 *	
 *	
 **/


#include "ff_format.h"
#include "ff_types.h"
#include "ff_ioman.h"
#include "ff_fatdef.h"

static FF_T_SINT8 FF_PartitionCount (FF_T_UINT8 *pBuffer)
{
	FF_T_SINT8 count = 0;
	FF_T_SINT8 part;
	// Check PBR or MBR signature
	if (FF_getChar(pBuffer, FF_FAT_MBR_SIGNATURE) != 0x55 &&
		FF_getChar(pBuffer, FF_FAT_MBR_SIGNATURE) != 0xAA ) {
		// No MBR, but is it a PBR ?
		if (FF_getChar(pBuffer, 0) == 0xEB &&          // PBR Byte 0
		    FF_getChar(pBuffer, 2) == 0x90 &&          // PBR Byte 2
		    (FF_getChar(pBuffer, 21) & 0xF0) == 0xF0) {// PBR Byte 21 : Media byte
			return 1;	// No MBR but PBR exist then only one partition
		}
		return 0;   // No MBR and no PBR then no partition found
	}
	for (part = 0; part < 4; part++)  {
		FF_T_UINT8 active = FF_getChar(pBuffer, FF_FAT_PTBL + FF_FAT_PTBL_ACTIVE + (16 * part));
		FF_T_UINT8 part_id = FF_getChar(pBuffer, FF_FAT_PTBL + FF_FAT_PTBL_ID + (16 * part));
		// The first sector must be a MBR, then check the partition entry in the MBR
		if (active != 0x80 && (active != 0 || part_id == 0)) {
			break;
		}
		count++;
	}
	return count;
}

FF_ERROR FF_FormatPartition(FF_IOMAN *pIoman, FF_T_UINT32 ulPartitionNumber, FF_T_UINT32 ulClusterSize) {
	
	FF_BUFFER *pBuffer;
	FF_T_UINT8	ucPartitionType;
	FF_T_SINT8	scPartitionCount;

	FF_T_UINT32 /*ulPartitionBeginLBA, ulPartitionLength,*/ ulPnum;

	FF_ERROR	Error = FF_ERR_NONE;

	ulClusterSize = 0;

	// Get Partition Metrics, and pass on to FF_Format() function

	pBuffer = FF_GetBuffer(pIoman, 0, FF_MODE_READ);
	{
		if(!pBuffer) {
			return FF_ERR_DEVICE_DRIVER_FAILED;
		}

		scPartitionCount = FF_PartitionCount(pBuffer->pBuffer);

		ucPartitionType = FF_getChar(pBuffer->pBuffer, FF_FAT_PTBL + FF_FAT_PTBL_ID);

		if(ucPartitionType == 0xEE) {
			// Handle Extended Partitions
			ulPnum = 0;			
		} else {
			if(ulPartitionNumber > (FF_T_UINT32) scPartitionCount) {
				FF_ReleaseBuffer(pIoman, pBuffer);
				return FF_ERR_IOMAN_INVALID_PARTITION_NUM;
			}
			ulPnum = ulPartitionNumber;
		}

	}
	FF_ReleaseBuffer(pIoman, pBuffer);



	return Error;
	
}

FF_ERROR FF_Format(FF_IOMAN *pIoman, FF_T_UINT32 ulStartLBA, FF_T_UINT32 ulEndLBA, FF_T_UINT32 ulClusterSize) {
	FF_T_UINT32 ulTotalSectors;
	FF_T_UINT32 ulTotalClusters;

	ulTotalSectors	= ulEndLBA - ulStartLBA;
	ulTotalClusters = ulTotalSectors / (ulClusterSize / pIoman->BlkSize);


	return -1;


}