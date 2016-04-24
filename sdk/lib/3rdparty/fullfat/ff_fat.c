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
 *	@file		ff_fat.c
 *	@author		James Walmsley
 *	@ingroup	FAT
 *
 *	@defgroup	FAT Fat File-System
 *	@brief		Handles FAT access and traversal.
 *
 *	Provides file-system interfaces for the FAT file-system.
 **/

#include "ff_fat.h"
#include "ff_config.h"
#include <string.h>

void FF_lockFAT(FF_IOMAN *pIoman) {
	FF_PendSemaphore(pIoman->pSemaphore);	// Use Semaphore to protect FAT modifications.
	{
		while((pIoman->Locks & FF_FAT_LOCK)) {
			FF_ReleaseSemaphore(pIoman->pSemaphore);
			FF_Yield();						// Keep Releasing and Yielding until we have the Fat protector.
			FF_PendSemaphore(pIoman->pSemaphore);
		}
		pIoman->Locks |= FF_FAT_LOCK;
	}
	FF_ReleaseSemaphore(pIoman->pSemaphore);
}

void FF_unlockFAT(FF_IOMAN *pIoman) {
	FF_PendSemaphore(pIoman->pSemaphore);
	{
		pIoman->Locks &= ~FF_FAT_LOCK;
	}
	FF_ReleaseSemaphore(pIoman->pSemaphore);
}

/**
 *	@private
 **/
FF_T_UINT32 FF_getRealLBA(FF_IOMAN *pIoman, FF_T_UINT32 LBA) {
	return LBA * pIoman->pPartition->BlkFactor;
}

/**
 *	@private
 **/
FF_T_UINT32 FF_Cluster2LBA(FF_IOMAN *pIoman, FF_T_UINT32 Cluster) {
	FF_T_UINT32 lba = 0;
	FF_PARTITION *pPart;
	if(pIoman) {
		pPart = pIoman->pPartition;

		if(Cluster > 1) {
			lba = ((Cluster - 2) * pPart->SectorsPerCluster) + pPart->FirstDataSector;
		} else {
			lba = pPart->ClusterBeginLBA;
		}
	}
	return lba;
}

/**
 *	@private
 **/
FF_T_UINT32 FF_LBA2Cluster(FF_IOMAN *pIoman, FF_T_UINT32 Address) {
	FF_T_UINT32 cluster = 0;
	FF_PARTITION *pPart;
	if(pIoman) {
		pPart = pIoman->pPartition;
		if(pPart->Type == FF_T_FAT32) {
			cluster = ((Address - pPart->ClusterBeginLBA) / pPart->SectorsPerCluster) + 2;
		} else {
			cluster = ((Address - pPart->ClusterBeginLBA) / pPart->SectorsPerCluster);
		}
	}
	return cluster;
}

/**
 *	@private
 **/
FF_T_UINT32 FF_getFatEntry(FF_IOMAN *pIoman, FF_T_UINT32 nCluster, FF_ERROR *pError) {

	FF_BUFFER 	*pBuffer;
	FF_T_UINT32 FatOffset;
	FF_T_UINT32 FatSector;
	FF_T_UINT32 FatSectorEntry;
	FF_T_UINT32 FatEntry;
	FF_T_UINT8	LBAadjust;
	FF_T_UINT16 relClusterEntry;

#ifdef FF_FAT12_SUPPORT
	FF_T_UINT8	F12short[2];		// For FAT12 FAT Table Across sector boundary traversal.
#endif
	
	if(pIoman->pPartition->Type == FF_T_FAT32) {
		FatOffset = nCluster * 4;
	} else if(pIoman->pPartition->Type == FF_T_FAT16) {
		FatOffset = nCluster * 2;
	}else {
		FatOffset = nCluster + (nCluster / 2);
	}
	
	FatSector		= pIoman->pPartition->FatBeginLBA + (FatOffset / pIoman->pPartition->BlkSize);
	FatSectorEntry	= FatOffset % pIoman->pPartition->BlkSize;
	
	LBAadjust		= (FF_T_UINT8)	(FatSectorEntry / pIoman->BlkSize);
	relClusterEntry = (FF_T_UINT32) (FatSectorEntry % pIoman->BlkSize);
	
	FatSector = FF_getRealLBA(pIoman, FatSector);
	
#ifdef FF_FAT12_SUPPORT
	if(pIoman->pPartition->Type == FF_T_FAT12) {
		if(relClusterEntry == (FF_T_UINT32)(pIoman->BlkSize - 1)) {
			// Fat Entry SPANS a Sector!
			// First Buffer get the last Byte in buffer (first byte of our address)!
			pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust, FF_MODE_READ);
			{
				if(!pBuffer) {
					*pError = FF_ERR_DEVICE_DRIVER_FAILED;
					return 0;
				}
				F12short[0] = FF_getChar(pBuffer->pBuffer, (FF_T_UINT16)(pIoman->BlkSize - 1));				
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			// Second Buffer get the first Byte in buffer (second byte of out address)!
			pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust + 1, FF_MODE_READ);
			{
				if(!pBuffer) {
					*pError = FF_ERR_DEVICE_DRIVER_FAILED;
					return 0;
				}
				F12short[1] = FF_getChar(pBuffer->pBuffer, 0);				
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			
			FatEntry = (FF_T_UINT32) FF_getShort((FF_T_UINT8*)&F12short, 0);	// Guarantee correct Endianess!

			if(nCluster & 0x0001) {
				FatEntry = FatEntry >> 4;
			} 
			FatEntry &= 0x0FFF;
			return (FF_T_SINT32) FatEntry;
		}
	}
#endif
	pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust, FF_MODE_READ);
	{
		if(!pBuffer) {
			*pError = FF_ERR_DEVICE_DRIVER_FAILED;
			return 0;
		}
		
		switch(pIoman->pPartition->Type) {
			case FF_T_FAT32:
				FatEntry = FF_getLong(pBuffer->pBuffer, relClusterEntry);
				FatEntry &= 0x0fffffff;	// Clear the top 4 bits.
				break;

			case FF_T_FAT16:
				FatEntry = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, relClusterEntry);
				break;

#ifdef FF_FAT12_SUPPORT
			case FF_T_FAT12:
				FatEntry = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, relClusterEntry);
				if(nCluster & 0x0001) {
					FatEntry = FatEntry >> 4;
				} 
				FatEntry &= 0x0FFF;
				break;
#endif
			default:
				FatEntry = 0;
				break;
		}
	}
	FF_ReleaseBuffer(pIoman, pBuffer);

	return (FF_T_SINT32) FatEntry;
}

FF_ERROR FF_ClearCluster(FF_IOMAN *pIoman, FF_T_UINT32 nCluster) {
	FF_BUFFER *pBuffer;
	FF_T_UINT16 i;
	FF_T_UINT32	BaseLBA;

	BaseLBA = FF_Cluster2LBA(pIoman, nCluster);
	BaseLBA = FF_getRealLBA(pIoman, BaseLBA);

	for(i = 0; i < pIoman->pPartition->SectorsPerCluster; i++) {
		pBuffer = FF_GetBuffer(pIoman, BaseLBA++, FF_MODE_WRITE);
		{
			if(!pBuffer) {
				return FF_ERR_DEVICE_DRIVER_FAILED;
			}
			memset(pBuffer->pBuffer, 0x00, 512);
		}
		FF_ReleaseBuffer(pIoman, pBuffer);
	}

	return FF_ERR_NONE;
}

/**
 *	@private
 *	@brief	Returns the Cluster address of the Cluster number from the beginning of a chain.
 *	
 *	@param	pIoman		FF_IOMAN Object
 *	@param	Start		Cluster address of the first cluster in the chain.
 *	@param	Count		Number of Cluster in the chain, 
 *
 *	
 *
 **/
FF_T_UINT32 FF_TraverseFAT(FF_IOMAN *pIoman, FF_T_UINT32 Start, FF_T_UINT32 Count, FF_ERROR *pError) {
	
	FF_T_UINT32 i;
	FF_T_UINT32 fatEntry = Start, currentCluster = Start;

	*pError = FF_ERR_NONE;

	for(i = 0; i < Count; i++) {
		fatEntry = FF_getFatEntry(pIoman, currentCluster, pError);
		if(*pError) {
			return 0;
		}

		if(FF_isEndOfChain(pIoman, fatEntry)) {
			return currentCluster;
		} else {
			currentCluster = fatEntry;
		}	
	}
	
	return fatEntry;
}

FF_T_UINT32 FF_FindEndOfChain(FF_IOMAN *pIoman, FF_T_UINT32 Start, FF_ERROR *pError) {

	FF_T_UINT32 fatEntry = Start, currentCluster = Start;
	*pError = FF_ERR_NONE;

	while(!FF_isEndOfChain(pIoman, fatEntry)) {
		fatEntry = FF_getFatEntry(pIoman, currentCluster, pError);
		if(*pError) {
			return 0;
		}

		if(FF_isEndOfChain(pIoman, fatEntry)) {
			return currentCluster;
		} else {
			currentCluster = fatEntry;
		}	
	}
	
	return fatEntry;
}


/**
 *	@private
 *	@brief	Tests if the fatEntry is an End of Chain Marker.
 *	
 *	@param	pIoman		FF_IOMAN Object
 *	@param	fatEntry	The fat entry from the FAT table to be checked.
 *
 *	@return	FF_TRUE if it is an end of chain, otherwise FF_FALSE.
 *
 **/
FF_T_BOOL FF_isEndOfChain(FF_IOMAN *pIoman, FF_T_UINT32 fatEntry) {
	FF_T_BOOL result = FF_FALSE;
	if(pIoman->pPartition->Type == FF_T_FAT32) {
		if((fatEntry & 0x0fffffff) >= 0x0ffffff8) {
			result = FF_TRUE;
		}
	} else if(pIoman->pPartition->Type == FF_T_FAT16) {
		if(fatEntry >= 0x0000fff8) {
			result = FF_TRUE;
		}
	} else {
		if(fatEntry >= 0x00000ff8) {
			result = FF_TRUE;
		}	
	}
	if(fatEntry == 0x00000000) {
		result = FF_TRUE;	//Perhaps trying to read a deleted file!
	}
	return result;
}


/**
 *	@private
 *	@brief	Writes a new Entry to the FAT Tables.
 *	
 *	@param	pIoman		IOMAN object.
 *	@param	nCluster	Cluster Number to be modified.
 *	@param	Value		The Value to store.
 **/
FF_ERROR FF_putFatEntry(FF_IOMAN *pIoman, FF_T_UINT32 nCluster, FF_T_UINT32 Value) {

	FF_BUFFER 	*pBuffer;
	FF_T_UINT32 FatOffset;
	FF_T_UINT32 FatSector;
	FF_T_UINT32 FatSectorEntry;
	FF_T_UINT32 FatEntry;
	FF_T_UINT8	LBAadjust;
	FF_T_UINT32 relClusterEntry;
#ifdef FF_FAT12_SUPPORT	
	FF_T_UINT8	F12short[2];		// For FAT12 FAT Table Across sector boundary traversal.
#endif
	
	if(pIoman->pPartition->Type == FF_T_FAT32) {
		FatOffset = nCluster * 4;
	} else if(pIoman->pPartition->Type == FF_T_FAT16) {
		FatOffset = nCluster * 2;
	}else {
		FatOffset = nCluster + (nCluster / 2);
	}
	
	FatSector = pIoman->pPartition->FatBeginLBA + (FatOffset / pIoman->pPartition->BlkSize);
	FatSectorEntry = FatOffset % pIoman->pPartition->BlkSize;
	
	LBAadjust = (FF_T_UINT8) (FatSectorEntry / pIoman->BlkSize);
	relClusterEntry = (FF_T_UINT32)(FatSectorEntry % pIoman->BlkSize);
	
	FatSector = FF_getRealLBA(pIoman, FatSector);

#ifdef FF_FAT12_SUPPORT	
	if(pIoman->pPartition->Type == FF_T_FAT12) {
		if(relClusterEntry == (FF_T_UINT32)(pIoman->BlkSize - 1)) {
			// Fat Entry SPANS a Sector!
			// First Buffer get the last Byte in buffer (first byte of our address)!
			pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust, FF_MODE_READ);
			{
				if(!pBuffer) {
					return FF_ERR_DEVICE_DRIVER_FAILED;
				}
				F12short[0] = FF_getChar(pBuffer->pBuffer, (FF_T_UINT16)(pIoman->BlkSize - 1));				
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			// Second Buffer get the first Byte in buffer (second byte of out address)!
			pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust + 1, FF_MODE_READ);
			{
				if(!pBuffer) {
					return FF_ERR_DEVICE_DRIVER_FAILED;
				}
				F12short[1] = FF_getChar(pBuffer->pBuffer, (FF_T_UINT16) 0x0000);				
			}
			FF_ReleaseBuffer(pIoman, pBuffer);

					
			FatEntry = FF_getShort((FF_T_UINT8*)&F12short, (FF_T_UINT16) 0x0000);	// Guarantee correct Endianess!
			if(nCluster & 0x0001) {
				FatEntry   &= 0x000F;
				Value		= (Value << 4);
				Value	   &= 0xFFF0;
			}  else {
				FatEntry	&= 0xF000;
				Value		&= 0x0FFF;
			}

			FF_putShort((FF_T_UINT8 *)F12short, 0x0000, (FF_T_UINT16) (FatEntry | Value));

			pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust, FF_MODE_WRITE);
			{
				if(!pBuffer) {
					return FF_ERR_DEVICE_DRIVER_FAILED;
				}
				FF_putChar(pBuffer->pBuffer, (FF_T_UINT16)(pIoman->BlkSize - 1), F12short[0]);				
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			// Second Buffer get the first Byte in buffer (second byte of out address)!
			pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust + 1, FF_MODE_READ);
			{
				if(!pBuffer) {
					return FF_ERR_DEVICE_DRIVER_FAILED;
				}
				FF_putChar(pBuffer->pBuffer, 0x0000, F12short[1]);				
			}
			FF_ReleaseBuffer(pIoman, pBuffer);

			return FF_ERR_NONE;
		}
	}
#endif
	
	pBuffer = FF_GetBuffer(pIoman, FatSector + LBAadjust, FF_MODE_WRITE);
	{
		if(!pBuffer) {
			return FF_ERR_DEVICE_DRIVER_FAILED;
		}
		if(pIoman->pPartition->Type == FF_T_FAT32) {
			Value &= 0x0fffffff;	// Clear the top 4 bits.
			FF_putLong(pBuffer->pBuffer, relClusterEntry, Value);
		} else if(pIoman->pPartition->Type == FF_T_FAT16) {
			FF_putShort(pBuffer->pBuffer, relClusterEntry, (FF_T_UINT16) Value);
		} else {
			FatEntry	= (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, relClusterEntry);
			if(nCluster & 0x0001) {
				FatEntry   &= 0x000F;
				Value		= (Value << 4);
				Value	   &= 0xFFF0;
			}  else {
				FatEntry	&= 0xF000;
				Value		&= 0x0FFF;
			}
			
			FF_putShort(pBuffer->pBuffer, relClusterEntry, (FF_T_UINT16) (FatEntry | Value));
		}
	}
	FF_ReleaseBuffer(pIoman, pBuffer);

	return FF_ERR_NONE;
}



/**
 *	@private
 *	@brief	Finds a Free Cluster and returns its number.
 *
 *	@param	pIoman	IOMAN Object.
 *
 *	@return	The number of the cluster found to be free.
 *	@return 0 on error.
 **/
#ifdef FF_FAT12_SUPPORT
static FF_T_UINT32 FF_FindFreeClusterOLD(FF_IOMAN *pIoman, FF_ERROR *pError) {
	FF_T_UINT32 nCluster;
	FF_T_UINT32 fatEntry;

	*pError = FF_ERR_NONE;

	for(nCluster = pIoman->pPartition->LastFreeCluster; nCluster < pIoman->pPartition->NumClusters; nCluster++) {
		fatEntry = FF_getFatEntry(pIoman, nCluster, pError);
		if(*pError) {
			return 0;
		}
		if(fatEntry == 0x00000000) {
			pIoman->pPartition->LastFreeCluster = nCluster;
			return nCluster;
		}
	}
	return 0;
}
#endif

FF_T_UINT32 FF_FindFreeCluster(FF_IOMAN *pIoman, FF_ERROR *pError) {
	FF_BUFFER	*pBuffer;
	FF_T_UINT32	i, x, nCluster = pIoman->pPartition->LastFreeCluster;
	FF_T_UINT32	FatOffset;
	FF_T_UINT32	FatSector;
	FF_T_UINT32	FatSectorEntry;
	FF_T_UINT32	EntriesPerSector;
	FF_T_UINT32 FatEntry = 1;

	*pError = FF_ERR_NONE;

#ifdef FF_FAT12_SUPPORT
	if(pIoman->pPartition->Type == FF_T_FAT12) {	// FAT12 tables are too small to optimise, and would make it very complicated!
		return FF_FindFreeClusterOLD(pIoman, pError);
	}
#endif

	if(pIoman->pPartition->Type == FF_T_FAT32) {
		EntriesPerSector = pIoman->BlkSize / 4;
		FatOffset = nCluster * 4;
	} else {
		EntriesPerSector = pIoman->BlkSize / 2;
		FatOffset = nCluster * 2;
	}
	
	// HT addition: don't use non-existing clusters
	if (nCluster >= pIoman->pPartition->NumClusters) {
		*pError = FF_ERR_FAT_NO_FREE_CLUSTERS;
		return 0;
	}

	FatSector = (FatOffset / pIoman->pPartition->BlkSize);
	
	for(i = FatSector; i < pIoman->pPartition->SectorsPerFAT; i++) {
		pBuffer = FF_GetBuffer(pIoman, pIoman->pPartition->FatBeginLBA + i, FF_MODE_READ);
		{
			if(!pBuffer) {
				*pError = FF_ERR_DEVICE_DRIVER_FAILED;
				return 0;
			}
			for(x = nCluster % EntriesPerSector; x < EntriesPerSector; x++) {
				if(pIoman->pPartition->Type == FF_T_FAT32) {
					FatOffset = x * 4;
					FatSectorEntry	= FatOffset % pIoman->pPartition->BlkSize;
					FatEntry = FF_getLong(pBuffer->pBuffer, (FF_T_UINT16)FatSectorEntry);
					FatEntry &= 0x0fffffff;	// Clear the top 4 bits.
				} else {
					FatOffset = x * 2;
					FatSectorEntry	= FatOffset % pIoman->pPartition->BlkSize;
					FatEntry = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, (FF_T_UINT16)FatSectorEntry);
				}
				if(FatEntry == 0x00000000) {
					FF_ReleaseBuffer(pIoman, pBuffer);
					pIoman->pPartition->LastFreeCluster = nCluster;
					
					return nCluster;
				}
				
				nCluster++;
			}	
		}
		FF_ReleaseBuffer(pIoman, pBuffer);
	}

	return 0;
}

/**
 * @private
 * @brief	Create's a Cluster Chain
 **/
FF_T_UINT32 FF_CreateClusterChain(FF_IOMAN *pIoman, FF_ERROR *pError) {
	FF_T_UINT32	iStartCluster;
	FF_ERROR	Error;
	*pError = FF_ERR_NONE;

	FF_lockFAT(pIoman);
	{
		iStartCluster = FF_FindFreeCluster(pIoman, &Error);
		if(Error) {
			*pError = Error;
			FF_unlockFAT(pIoman);
			return 0;
		}

		if(iStartCluster) {
			Error = FF_putFatEntry(pIoman, iStartCluster, 0xFFFFFFFF); // Mark the cluster as End-Of-Chain
			if(Error) {
				*pError = Error;
				FF_unlockFAT(pIoman);
				return 0;
			}
		}
	}
	FF_unlockFAT(pIoman);

	if(iStartCluster) {
		Error = FF_DecreaseFreeClusters(pIoman, 1);
		if(Error) {
			*pError = Error;
			return 0;
		}
	}

	return iStartCluster;
}

FF_T_UINT32 FF_GetChainLength(FF_IOMAN *pIoman, FF_T_UINT32 pa_nStartCluster, FF_T_UINT32 *piEndOfChain, FF_ERROR *pError) {
	FF_T_UINT32 iLength = 0;

	*pError = FF_ERR_NONE;
	
	FF_lockFAT(pIoman);
	{
		while(!FF_isEndOfChain(pIoman, pa_nStartCluster)) {
			pa_nStartCluster = FF_getFatEntry(pIoman, pa_nStartCluster, pError);
			if(*pError) {
				return 0;
			}
			iLength++;
		}
		if(piEndOfChain) {
			*piEndOfChain = pa_nStartCluster;
		}
	}
	FF_unlockFAT(pIoman);

	return iLength;
}

/**
 *	@private
 *	@brief Extend a Cluster chain by Count number of Clusters
 *	
 *	@param	pIoman			IOMAN object.
 *	@param	StartCluster	Cluster Number that starts the chain.
 *	@param	Count			Number of clusters to extend the chain with.
 *
 **/
/*
FF_T_UINT32 FF_ExtendClusterChain(FF_IOMAN *pIoman, FF_T_UINT32 StartCluster, FF_T_UINT32 Count) {
	
	FF_T_UINT32 currentCluster = StartCluster, nextCluster;
	FF_T_UINT32 clusEndOfChain;
	FF_T_UINT32 i;
	
	clusEndOfChain = FF_FindEndOfChain(pIoman, StartCluster);

	nextCluster = FF_FindFreeCluster(pIoman);	// Find Free clusters!

	FF_putFatEntry(pIoman, clusEndOfChain, nextCluster);

	for(i = 0; i <= Count; i++) {
		currentCluster = nextCluster;
		if(i == Count) {
			FF_putFatEntry(pIoman, currentCluster, 0xFFFFFFFF);
			break;
		}

		nextCluster = FF_FindFreeCluster(pIoman);
		FF_putFatEntry(pIoman, currentCluster, ++nextCluster);
	}
	FF_FlushCache(pIoman);
	return currentCluster;
}*/


/**
 *	@private
 *	@brief Free's Disk space by freeing unused links on Cluster Chains
 *
 *	@param	pIoman,			IOMAN object.
 *	@param	StartCluster	Cluster Number that starts the chain.
 *	@param	Count			Number of Clusters from the end of the chain to unlink.
 *	@param	Count			0 Means Free the entire chain (delete file).
 *
 *	@return 0 On Success.
 *	@return	-1 If the device driver failed to provide access.
 *
 **/
FF_ERROR FF_UnlinkClusterChain(FF_IOMAN *pIoman, FF_T_UINT32 StartCluster, FF_T_UINT16 Count) {
	
	FF_T_UINT32 fatEntry;
	FF_T_UINT32 currentCluster, chainLength = 0;
	FF_T_UINT32	iLen = 0;
	FF_T_UINT32 lastFree = StartCluster;	/* HT addition : reset LastFreeCluster */
	FF_ERROR	Error;

	fatEntry = StartCluster;

	if(Count == 0) {
		// Free all clusters in the chain!
		currentCluster = StartCluster;
		fatEntry = currentCluster;
        do {
			fatEntry = FF_getFatEntry(pIoman, fatEntry, &Error);
			if(Error) {
				return Error;
			}
			Error = FF_putFatEntry(pIoman, currentCluster, 0x00000000);
			if(Error) {
				return Error;
			}

			if (lastFree > currentCluster) {
				lastFree = currentCluster;
			}
			currentCluster = fatEntry;
			iLen ++;
		}while(!FF_isEndOfChain(pIoman, fatEntry));
		if (pIoman->pPartition->LastFreeCluster > lastFree) {
			pIoman->pPartition->LastFreeCluster = lastFree;
		}
		Error = FF_IncreaseFreeClusters(pIoman, iLen);
		if(Error) {
			return Error;
		}
	} else {
		// Truncation - This is quite hard, because we can only do it backwards.
		do {
			fatEntry = FF_getFatEntry(pIoman, fatEntry, &Error);
			if(Error) {
				return Error;
			}
			chainLength++;
		}while(!FF_isEndOfChain(pIoman, fatEntry));
	}

	return FF_ERR_NONE;
}

#ifdef FF_FAT12_SUPPORT
FF_T_UINT32 FF_CountFreeClustersOLD(FF_IOMAN *pIoman, FF_ERROR *pError) {
	FF_T_UINT32 i;
	FF_T_UINT32 TotalClusters = pIoman->pPartition->DataSectors / pIoman->pPartition->SectorsPerCluster;
	FF_T_UINT32 FatEntry;
	FF_T_UINT32 FreeClusters = 0;

	*pError = FF_ERR_NONE;

	for(i = 0; i < TotalClusters; i++) {
		FatEntry = FF_getFatEntry(pIoman, i, pError);
		if(*pError) {
			return 0;
		}
		if(!FatEntry) {
			FreeClusters++;
		}
	}

	return FreeClusters;
}
#endif


FF_T_UINT32 FF_CountFreeClusters(FF_IOMAN *pIoman, FF_ERROR *pError) {
	FF_BUFFER	*pBuffer;
	FF_T_UINT32	i, x, nCluster = 0;
	FF_T_UINT32	FatOffset;
	FF_T_UINT32	FatSector;
	FF_T_UINT32	FatSectorEntry;
	FF_T_UINT32	EntriesPerSector;
	FF_T_UINT32 FatEntry = 1;
	FF_T_UINT32	FreeClusters = 0;

	*pError = FF_ERR_NONE;

#ifdef FF_FAT12_SUPPORT
	if(pIoman->pPartition->Type == FF_T_FAT12) {	// FAT12 tables are too small to optimise, and would make it very complicated!
		FreeClusters = FF_CountFreeClustersOLD(pIoman, pError);
		if(*pError) {
			return 0;
		}
	}
#endif

	if(pIoman->pPartition->Type == FF_T_FAT32) {
		EntriesPerSector = pIoman->BlkSize / 4;
		FatOffset = nCluster * 4;
	} else {
		EntriesPerSector = pIoman->BlkSize / 2;
		FatOffset = nCluster * 2;
	}

	FatSector = (FatOffset / pIoman->pPartition->BlkSize);
	
	for(i = 0; i < pIoman->pPartition->SectorsPerFAT; i++) {
		pBuffer = FF_GetBuffer(pIoman, pIoman->pPartition->FatBeginLBA + i, FF_MODE_READ);
		{
			if(!pBuffer) {
				*pError = FF_ERR_DEVICE_DRIVER_FAILED;
				return 0;
			}
			for(x = nCluster % EntriesPerSector; x < EntriesPerSector; x++) {
				if(pIoman->pPartition->Type == FF_T_FAT32) {
					FatOffset = x * 4;
					FatSectorEntry	= FatOffset % pIoman->pPartition->BlkSize;
					FatEntry = FF_getLong(pBuffer->pBuffer, (FF_T_UINT16)FatSectorEntry);
					FatEntry &= 0x0fffffff;	// Clear the top 4 bits.
				} else {
					FatOffset = x * 2;
					FatSectorEntry	= FatOffset % pIoman->pPartition->BlkSize;
					FatEntry = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, FatSectorEntry);
				}
				if(FatEntry == 0x00000000) {
					FreeClusters += 1;
				}
				
				nCluster++;
			}	
		}
		FF_ReleaseBuffer(pIoman, pBuffer);
	}

	return FreeClusters <= pIoman->pPartition->NumClusters ? FreeClusters : pIoman->pPartition->NumClusters;
}

#ifdef FF_64_NUM_SUPPORT
FF_T_UINT64 FF_GetFreeSize(FF_IOMAN *pIoman, FF_ERROR *pError) {
	FF_T_UINT32 FreeClusters;
	FF_T_UINT64 FreeSize;
	FF_ERROR	Error;
	
	if(pIoman) {
		FF_lockFAT(pIoman);
		{	
			if(!pIoman->pPartition->FreeClusterCount) {
				pIoman->pPartition->FreeClusterCount = FF_CountFreeClusters(pIoman, &Error);
				if(Error) {
					if(pError) {
						*pError = Error;
					}
					FF_unlockFAT(pIoman);
					return 0;
				}
			}
			FreeClusters = pIoman->pPartition->FreeClusterCount;
		}
		FF_unlockFAT(pIoman);
		FreeSize = (FF_T_UINT64) ((FF_T_UINT64)FreeClusters * (FF_T_UINT64)((FF_T_UINT64)pIoman->pPartition->SectorsPerCluster * (FF_T_UINT64)pIoman->pPartition->BlkSize));
		return FreeSize;
	}
	return 0;
}
#else
FF_T_UINT32 FF_GetFreeSize(FF_IOMAN *pIoman) {
	FF_T_UINT32 FreeClusters;
	FF_T_UINT32 FreeSize;
	
	if(pIoman) {
		FF_lockFAT(pIoman);
		{	
			if(!pIoman->pPartition->FreeClusterCount) {
				pIoman->pPartition->FreeClusterCount = FF_CountFreeClusters(pIoman);
			}
			FreeClusters = pIoman->pPartition->FreeClusterCount;
		}
		FF_unlockFAT(pIoman);
		FreeSize = (FF_T_UINT32) ((FF_T_UINT32)FreeClusters * (FF_T_UINT32)((FF_T_UINT32)pIoman->pPartition->SectorsPerCluster * (FF_T_UINT32)pIoman->pPartition->BlkSize));
		return FreeSize;
	}
	return 0;
}
#endif