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
 *	@file		ff_dir.c
 *	@author		James Walmsley
 *	@ingroup	DIR
 *
 *	@defgroup	DIR Handles Directory Traversal
 *	@brief		Handles DIR access and traversal.
 *
 *	Provides FindFirst() and FindNext() Interfaces
 **/

#include "ff_dir.h"
#include "ff_string.h"
#include <stdio.h>

static void FF_ProcessShortName(FF_T_INT8 *name);

void FF_lockDIR(FF_IOMAN *pIoman) {
	FF_PendSemaphore(pIoman->pSemaphore);	// Use Semaphore to protect FAT modifications.
	{
		while((pIoman->Locks & FF_DIR_LOCK)) {
			FF_ReleaseSemaphore(pIoman->pSemaphore);
			FF_Yield();						// Keep Releasing and Yielding until we have the Fat protector.
			FF_PendSemaphore(pIoman->pSemaphore);
		}
		pIoman->Locks |= FF_DIR_LOCK;
	}
	FF_ReleaseSemaphore(pIoman->pSemaphore);
}

void FF_unlockDIR(FF_IOMAN *pIoman) {
	FF_PendSemaphore(pIoman->pSemaphore);
	{
		pIoman->Locks &= ~FF_DIR_LOCK;
	}
	FF_ReleaseSemaphore(pIoman->pSemaphore);
}

static FF_T_UINT8 FF_CreateChkSum(const FF_T_UINT8 *pa_pShortName) {
	FF_T_UINT8	cNameLen;
	FF_T_UINT8	ChkSum = 0;

	for(cNameLen = 11; cNameLen != 0; cNameLen--) {
		ChkSum = ((ChkSum & 1) ? 0x80 : 0) + (ChkSum >> 1) + *pa_pShortName++;
	}
	return ChkSum;
}

FF_T_SINT8 FF_FindNextInDir(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent) {
	
	FF_T_UINT8	numLFNs;
	FF_T_UINT8	EntryBuffer[32];

	if(!pIoman) {
		return FF_ERR_NULL_POINTER;
	}

	//pDirent->NumLFNs = 0;
	
	for(; pDirent->CurrentItem < 0xFFFF; pDirent->CurrentItem += 1) {
		if(FF_FetchEntry(pIoman, DirCluster, pDirent->CurrentItem, EntryBuffer)) {
			return -2;
		}
		if(EntryBuffer[0] != 0xE5) {
			if(FF_isEndOfDir(EntryBuffer)){
				return -2;
			}
			pDirent->Attrib = FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
			if((pDirent->Attrib & FF_FAT_ATTR_LFN) == FF_FAT_ATTR_LFN) {
				// LFN Processing
				numLFNs = (FF_T_UINT8)(EntryBuffer[0] & ~0x40);
				//pDirent->NumLFNs = numLFNs;
#ifdef FF_LFN_SUPPORT
				FF_PopulateLongDirent(pIoman, pDirent, DirCluster, pDirent->CurrentItem);
				return 0;
#else 
				pDirent->CurrentItem += (numLFNs - 1);
#endif
			} else if((pDirent->Attrib & FF_FAT_ATTR_VOLID) == FF_FAT_ATTR_VOLID) {
				// Do Nothing
			
			} else {
				FF_PopulateShortDirent(pIoman, pDirent, EntryBuffer);
				pDirent->CurrentItem += 1;
				return 0;
			}
		}
	}
	
	return -1;
}
/*
FF_T_BOOL FF_ShortNameExists(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *name) {

	FF_DIRENT MyDir;

	if(FF_FindEntry(pIoman, DirCluster, name, &MyDir, FF_FALSE)) {
		return FF_FALSE;
	}
	
	return FF_TRUE;
}*/

FF_T_BOOL FF_ShortNameExists(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *name) {

    FF_T_UINT16 i;
    FF_T_UINT8      EntryBuffer[32];
    FF_T_UINT8      Attrib;

#ifdef FF_HASH_TABLE_SUPPORT

		if(!FF_DirHashed(pIoman, DirCluster)) {
			for(i = 0; i < 0xFFFF; i++) {
				if(FF_FetchEntry(pIoman, DirCluster, i, EntryBuffer)) {
					FF_SetDirHashed(pIoman, DirCluster);
					break;
				}
				Attrib = FF_getChar(EntryBuffer, FF_FAT_DIRENT_ATTRIB);
				if(FF_getChar(EntryBuffer, 0x00) != 0xE5) {
					if(Attrib != FF_FAT_ATTR_LFN) {
						if(FF_isEndOfDir(EntryBuffer)) {
							FF_SetDirHashed(pIoman, DirCluster);
							break;
						}
						FF_ProcessShortName((FF_T_INT8 *)EntryBuffer);
						FF_AddDirentHash(pIoman, DirCluster, FF_GetCRC16((FF_T_UINT8 *) EntryBuffer, strlen(EntryBuffer)));
					}
				}
			}
		}

#if FF_HASH_FUNCTION == CRC16
		if(FF_CheckDirentHash(pIoman, DirCluster, (FF_T_UINT32)FF_GetCRC16((FF_T_UINT8 *) name, strlen(name)))) {
#elif FF_HASH_FUNCTION == CRC8
		if(FF_CheckDirentHash(pIoman, DirCluster, (FF_T_UINT32)FF_GetCRC8((FF_T_UINT8 *) name, strlen(name)))) {
#else
		{
#endif
#endif

			for(i = 0; i < 0xFFFF; i++) {
				FF_FetchEntry(pIoman, DirCluster, i, EntryBuffer);
				Attrib = FF_getChar(EntryBuffer, FF_FAT_DIRENT_ATTRIB);
				if(FF_getChar(EntryBuffer, 0x00) != 0xE5) {
					if(Attrib != FF_FAT_ATTR_LFN) {
						FF_ProcessShortName((FF_T_INT8 *)EntryBuffer);
						if(FF_isEndOfDir(EntryBuffer)) {
								return FF_FALSE;
						}
						if(strcmp(name, (FF_T_INT8 *)EntryBuffer) == 0) {
								return FF_TRUE;
						}
					}
				}
			}
#ifdef FF_HASH_TABLE_SUPPORT
		}
#endif
        
        return FF_FALSE;
}

FF_T_UINT32 FF_FindEntryInDir(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *name, FF_T_UINT8 pa_Attrib, FF_DIRENT *pDirent) {

	FF_T_UINT16		fnameLen;
	FF_T_UINT16		compareLength;
	FF_T_UINT16		nameLen;
	FF_T_INT8		Filename[FF_MAX_FILENAME];
	FF_T_INT8		MyFname[FF_MAX_FILENAME];
	FF_T_BOOL		bBreak = FF_FALSE;
	
	pDirent->CurrentItem = 0;
	nameLen = (FF_T_UINT16) strlen(name);

	while(!bBreak) {	
		if(FF_FindNextInDir(pIoman, DirCluster, pDirent)) {
			break;	// end of dir, file not found!
		}

		if((pDirent->Attrib & pa_Attrib) == pa_Attrib){
			strcpy(Filename, pDirent->FileName);
			fnameLen = (FF_T_UINT16) strlen(Filename);
			FF_tolower(Filename, (FF_T_UINT32) fnameLen);
			if(nameLen < FF_MAX_FILENAME) {
				memcpy(MyFname, name, nameLen + 1);
			} else {
				memcpy(MyFname, name, FF_MAX_FILENAME);
				MyFname[FF_MAX_FILENAME - 1] = '\0';
			}
			FF_tolower(MyFname, (FF_T_UINT32) nameLen);
			if(nameLen > fnameLen) {
				compareLength = nameLen;
			} else {
				compareLength = fnameLen;
			}
			if(strncmp(MyFname, Filename, (FF_T_UINT32) compareLength) == 0) {
				// Object found!
				return pDirent->ObjectCluster;	// Return the cluster number
			}
		}
	}
	
	return 0;
}

/*
#define FF_DIR_LFN_TRAVERSED	0x01
#define FF_DIR_LFN_DELETED		0x02

FF_T_SINT8 FF_PopulateLongBufEntry(FF_IOMAN *pIoman, FF_BUFFER **ppBuffer, FF_DIRENT *pDirent) {
	// Relative positions!
	FF_T_UINT32	RelBlockNum;
	FF_T_UINT32 RelBlockPos = FF_getMinorBlockEntry(pIoman, pDirent->CurrentItem, 32);
	FF_T_UINT32	iItemLBA;
	FF_T_INT8	*DirBuffer	= ((*ppBuffer)->pBuffer + (RelBlockPos * 32));
	FF_T_UINT8	numLFNs		= (FF_getChar(DirBuffer, (FF_T_UINT16) 0) & ~0x40);
	FF_T_UINT16	x,i,y,myShort, lenlfn = 0;
	FF_T_UINT32	CurrentCluster;
	FF_T_SINT8	RetVal	= FF_ERR_NONE;
	FF_T_INT8	ShortName[13];
	FF_T_UINT8	CheckSum = FF_getChar(DirBuffer, FF_FAT_LFN_CHECKSUM);

	while(numLFNs > 0) {
		
		for(i = 0, y = 0; i < 5; i++, y += 2) {
			pDirent->FileName[i + ((numLFNs - 1) * 13)] = DirBuffer[FF_FAT_LFN_NAME_1 + y];
			lenlfn++;
		}

		for(i = 0, y = 0; i < 6; i++, y += 2) {
			pDirent->FileName[i + ((numLFNs - 1) * 13) + 5] = DirBuffer[FF_FAT_LFN_NAME_2 + y];
			lenlfn++;
		}

		for(i = 0, y = 0; i < 2; i++, y += 2) {
			pDirent->FileName[i + ((numLFNs - 1) * 13) + 11] = DirBuffer[FF_FAT_LFN_NAME_3 + y];
			lenlfn++;
		}
		numLFNs--;
		pDirent->CurrentItem += 1;
		
		CurrentCluster	= FF_getClusterChainNumber(pIoman, pDirent->CurrentItem, 32);
		RelBlockNum		= FF_getMajorBlockNumber(pIoman, pDirent->CurrentItem, 32);
		RelBlockPos		= FF_getMinorBlockEntry(pIoman, pDirent->CurrentItem, 32);
		iItemLBA		= FF_Cluster2LBA(pIoman, pDirent->AddrCurrentCluster) + RelBlockNum;
		
		if(CurrentCluster > pDirent->CurrentCluster) {
			pDirent->AddrCurrentCluster = FF_TraverseFAT(pIoman, pDirent->AddrCurrentCluster, 1);
			pDirent->CurrentCluster += 1;
			iItemLBA = FF_Cluster2LBA(pIoman, pDirent->AddrCurrentCluster) + RelBlockNum;
			FF_ReleaseBuffer(pIoman, *ppBuffer);
			*ppBuffer = FF_GetBuffer(pIoman, iItemLBA, FF_MODE_READ);
			RetVal |= FF_DIR_LFN_TRAVERSED;
		} else if(iItemLBA > ((*ppBuffer)->Sector)) {
			FF_ReleaseBuffer(pIoman, *ppBuffer);
			*ppBuffer = FF_GetBuffer(pIoman, iItemLBA, FF_MODE_READ);
			RetVal |= FF_DIR_LFN_TRAVERSED;
		}

		DirBuffer = ((*ppBuffer)->pBuffer + (RelBlockPos * 32));
	}

	if(FF_getChar(DirBuffer, (FF_T_UINT16) 0) == FF_FAT_DELETED) {
		RetVal |= FF_DIR_LFN_DELETED;
		return RetVal;
	}

	pDirent->FileName[lenlfn] = '\0';
	
	// Process the ShortName Entry
	memcpy(ShortName, DirBuffer, 11);
	if(CheckSum != FF_CreateChkSum(ShortName)) {
		FF_ProcessShortName(ShortName);
		strcpy(pDirent->FileName, ShortName);
	} else {
		FF_ProcessShortName(ShortName);
	}
	
	myShort = FF_getShort(DirBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_HIGH));
	pDirent->ObjectCluster = (FF_T_UINT32) (myShort << 16);
	myShort = FF_getShort(DirBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_LOW));
	pDirent->ObjectCluster |= myShort;

	// Get the filesize.
	pDirent->Filesize = FF_getLong(DirBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_FILESIZE));
	// Get the attribute.
	pDirent->Attrib = FF_getChar(DirBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));

	return RetVal;
}*/

/*
FF_T_SINT8 FF_FindEntry(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *Name, FF_DIRENT *pDirent, FF_T_BOOL LFNs) {
	
	FF_T_UINT32		iItemLBA;
	FF_BUFFER		*pBuffer;
	FF_T_INT8		*DirBuffer;
	FF_T_UINT32		fatEntry = DirCluster;
	FF_T_UINT32		i,x;
	FF_T_UINT32		numLFNs;
	FF_T_UINT16		NameLen, DirentNameLen;
	FF_T_BOOL		Compare = FF_FALSE;
	FF_T_SINT8		RetVal = 0;
	FF_T_UINT16		RelEntry;

	pDirent->CurrentItem = 0;
	pDirent->AddrCurrentCluster = DirCluster;
	pDirent->CurrentCluster = 0;
	pDirent->DirCluster = DirCluster;

	do {
		
		pDirent->AddrCurrentCluster = fatEntry;
		iItemLBA = FF_Cluster2LBA(pIoman, pDirent->AddrCurrentCluster);

		for(i = 0; i < pIoman->pPartition->SectorsPerCluster; i++) {
			
			if(FF_getClusterChainNumber(pIoman, pDirent->CurrentItem, 32) > pDirent->CurrentCluster) {
				break;
			}

			pBuffer = FF_GetBuffer(pIoman, iItemLBA + i, FF_MODE_READ);
			{
				if(!pBuffer) {
					return FF_ERR_DEVICE_DRIVER_FAILED;
				}
				pBuffer->Persistance = 1;
				RelEntry = FF_getMinorBlockEntry(pIoman, pDirent->CurrentItem, 32);
				for(x = RelEntry; x < (pIoman->BlkSize / 32); x++) {
					if(FF_getMajorBlockNumber(pIoman, pDirent->CurrentItem, 32) > i) {
						break;
					}
					if(FF_getClusterChainNumber(pIoman, pDirent->CurrentItem, 32) > pDirent->CurrentCluster) {
						break;
					}
					RelEntry = FF_getMinorBlockEntry(pIoman, pDirent->CurrentItem, 32);
					x = RelEntry;
					if(x >= (pIoman->BlkSize / 32)) {
						break;
					}
					DirBuffer = (pBuffer->pBuffer + (32 * x));
					// Process each entry and Compare to Name!
					if(FF_getChar(DirBuffer, (FF_T_UINT16) 0) != FF_FAT_DELETED) {
						if(DirBuffer[0] == 0x00) {
							FF_ReleaseBuffer(pIoman, pBuffer);
							return FF_ERR_DIR_END_OF_DIR;
						}
						pDirent->Attrib = FF_getChar(DirBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
						if((pDirent->Attrib & FF_FAT_ATTR_LFN) == FF_FAT_ATTR_LFN) {
							numLFNs = (FF_T_UINT8)(DirBuffer[0] & ~0x40);
							if(LFNs) {
								RetVal = FF_PopulateLongBufEntry(pIoman, &pBuffer, pDirent);
								if((RetVal & FF_DIR_LFN_DELETED)) {
									Compare = FF_FALSE;
									RetVal &= ~FF_DIR_LFN_DELETED;
								} else {
									Compare = FF_TRUE;
								}

								pDirent->CurrentItem += 1;
							} else {
								pDirent->CurrentItem += numLFNs;
							}
							
						} else {
							FF_PopulateShortDirent(pDirent, DirBuffer);
							Compare = FF_TRUE;
							pDirent->CurrentItem += 1;
						}

						if(Compare) {
							// Compare the Items
							NameLen = strlen(Name);
							DirentNameLen = strlen(pDirent->FileName);

							if(NameLen == DirentNameLen) {	// Names are same length, possible match.
								if(FF_StrMatch(Name, pDirent->FileName, NameLen)) {
									FF_ReleaseBuffer(pIoman, pBuffer);
									return FF_ERR_NONE;	// Success Item found!
								}
							}
							Compare = FF_FALSE;
						}

						if((RetVal & FF_DIR_LFN_TRAVERSED)) {
							break;		
						}

					} else {
						pDirent->CurrentItem += 1;
					}
				}
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			if((RetVal & FF_DIR_LFN_TRAVERSED)) {
				break;		
			}
		}

		// Traverse!
		
		if((RetVal & FF_DIR_LFN_TRAVERSED)) {
			RetVal = FF_ERR_NONE;
			fatEntry = pDirent->AddrCurrentCluster;
		} else {
			if(FF_getClusterChainNumber(pIoman, pDirent->CurrentItem, 32) > pDirent->CurrentCluster) {
				fatEntry = FF_getFatEntry(pIoman, pDirent->AddrCurrentCluster);
				pDirent->AddrCurrentCluster = FF_TraverseFAT(pIoman, pDirent->AddrCurrentCluster, 1);
				pDirent->CurrentCluster += 1;
			}
		}
	} while(!FF_isEndOfChain(pIoman, fatEntry));

	return FF_ERR_DIR_END_OF_DIR;
}*/


/**
 *	@private
 **/
/*FF_T_UINT32 FF_FindEntry(FF_IOMAN *pIoman, FF_T_SINT8 *path, FF_T_UINT8 pa_Attrib, FF_DIRENT *pDirent) {
	
	FF_T_INT32		retVal;
	FF_T_INT8		name[FF_MAX_FILENAME];
	FF_T_INT8		Filename[FF_MAX_FILENAME];
	FF_T_UINT16		fnameLen;
	FF_T_UINT16		compareLength;
	FF_T_UINT16		nameLen;
	FF_T_UINT16		i = strlen(path);


	while(i != 0) {
		if(path[i] == '\\' || path[i] == '/') {
			break;
		}
		i--;
	}
	
	if(i == 0) {
		i = 1;
	}

	nameLen = strlen((path + i));
	strncpy(name, (path + i), nameLen);
	name[nameLen] = '\0';	
	

	if(FF_FindFirst(pIoman, pDirent, path)) {
		return 0; // file not found.
	}

	if((pDirent->Attrib & pa_Attrib) == pa_Attrib){
		strcpy(Filename, pDirent->FileName);
		fnameLen = (FF_T_UINT16) strlen(Filename);
		FF_tolower(Filename, (FF_T_UINT32) fnameLen);
		FF_tolower(name, (FF_T_UINT32) nameLen);
		if(nameLen > fnameLen) {
			compareLength = nameLen;
		} else {
			compareLength = fnameLen;
		}
		if(strncmp(name, Filename, (FF_T_UINT32) compareLength) == 0) {
			// Object found!!
			return pDirent->ObjectCluster;	// Return the cluster number
		}
	}

	while(1) {	
		if(FF_FindNext(pIoman, pDirent)) {
			return 0;	// end of dir, file not found!
		}

		if((pDirent->Attrib & pa_Attrib) == pa_Attrib){
			strcpy(Filename, pDirent->FileName);
			fnameLen = (FF_T_UINT16) strlen(Filename);
			FF_tolower(Filename, (FF_T_UINT32) fnameLen);
			FF_tolower(name, (FF_T_UINT32) nameLen);
			if(nameLen > fnameLen) {
				compareLength = nameLen;
			} else {
				compareLength = fnameLen;
			}
			if(strncmp(name, Filename, (FF_T_UINT32) compareLength) == 0) {
				// Object found!
				return pDirent->ObjectCluster;	// Return the cluster number
			}
		}
	}
	return 0;
}*/

/**
 *	@private
 **/

FF_T_UINT32 FF_FindDir(FF_IOMAN *pIoman, const FF_T_INT8 *path, FF_T_UINT16 pathLen) {
    FF_T_UINT32     dirCluster = pIoman->pPartition->RootDirCluster;
    FF_T_INT8       mytoken[FF_MAX_FILENAME];
    FF_T_INT8       *token;
    FF_T_UINT16     it = 0;         // Re-entrancy Variables for FF_strtok()
    FF_T_BOOL       last = FF_FALSE;
    FF_DIRENT       MyDir;
#ifdef FF_PATH_CACHE
	FF_T_UINT32		i;
#endif

    if(pathLen == 1) {      // Must be the root dir! (/ or \)
		return pIoman->pPartition->RootDirCluster;
    }
    
    if(path[pathLen-1] == '\\' || path[pathLen-1] == '/') {
		pathLen--;      
    }
	
#ifdef FF_PATH_CACHE	// Is the requested path in the PATH CACHE?
	FF_PendSemaphore(pIoman->pSemaphore);	// Thread safety on shared object!
	{
		for(i = 0; i < FF_PATH_CACHE_DEPTH; i++) {
			if(strlen(pIoman->pPartition->PathCache[i].Path) == pathLen) {
				if(FF_strmatch(pIoman->pPartition->PathCache[i].Path, path, pathLen)) {
					FF_ReleaseSemaphore(pIoman->pSemaphore);
					return pIoman->pPartition->PathCache[i].DirCluster;
				}
			}
		}
	}
	FF_ReleaseSemaphore(pIoman->pSemaphore);
#endif

    token = FF_strtok(path, mytoken, &it, &last, pathLen);

     do{
            //lastDirCluster = dirCluster;
            MyDir.CurrentItem = 0;
            dirCluster = FF_FindEntryInDir(pIoman, dirCluster, token, FF_FAT_ATTR_DIR, &MyDir);
			/*if(dirCluster == 0 && MyDir.CurrentItem == 2 && MyDir.FileName[0] == '.') { // .. Dir Entry pointing to root dir.
				dirCluster = pIoman->pPartition->RootDirCluster;
            }*/
            token = FF_strtok(path, mytoken, &it, &last, pathLen);
    }while(token != NULL);

#ifdef FF_PATH_CACHE	// Update the PATH CACHE with a new PATH
	 if(dirCluster) {	// Only cache if the dir was actually found!
		FF_PendSemaphore(pIoman->pSemaphore);
		{
			if(pathLen < FF_MAX_PATH) {	// Ensure the PATH won't cause a buffer overrun.
				memcpy(pIoman->pPartition->PathCache[pIoman->pPartition->PCIndex].Path, path, pathLen);
				pIoman->pPartition->PathCache[pIoman->pPartition->PCIndex].Path[pathLen] = '\0';
				pIoman->pPartition->PathCache[pIoman->pPartition->PCIndex].DirCluster = dirCluster;
#ifdef FF_HASH_TABLE_SUPPORT				
				FF_ClearHashTable(pIoman->pPartition->PathCache[pIoman->pPartition->PCIndex].pHashTable);
#endif			
				pIoman->pPartition->PCIndex += 1;
				if(pIoman->pPartition->PCIndex >= FF_PATH_CACHE_DEPTH) {
					pIoman->pPartition->PCIndex = 0;
				}
			}
		}
		FF_ReleaseSemaphore(pIoman->pSemaphore);
	 }
#endif

    return dirCluster;
}

/*
FF_T_UINT32 FF_FindDir(FF_IOMAN *pIoman, FF_T_INT8 *path, FF_T_UINT16 pathLen) {
    FF_T_UINT32		dirCluster = pIoman->pPartition->RootDirCluster;
    FF_T_INT8       mytoken[FF_MAX_FILENAME];
    FF_T_INT8       *token;
    FF_T_UINT16     it = 0;         // Re-entrancy Variables for FF_strtok()
    FF_T_BOOL       last = FF_FALSE;
    FF_DIRENT       MyDir;

    if(pathLen == 1) {      // Must be the root dir! (/ or \)
            return pIoman->pPartition->RootDirCluster;
    }
    
    if(path[pathLen-1] == '\\' || path[pathLen-1] == '/') {
            pathLen--;      
    }

    token = FF_strtok(path, mytoken, &it, &last, pathLen);

     do{
            //lastDirCluster = dirCluster;
            MyDir.CurrentItem = 0;
			if(FF_FindEntry(pIoman, dirCluster, token, &MyDir, FF_TRUE)) {
				return 0;
			} else {
				dirCluster = MyDir.ObjectCluster;
			}
			if(MyDir.Attrib != FF_FAT_ATTR_DIR) {
				return 0;
			}
			if(dirCluster == 0 && MyDir.CurrentItem == 2 && MyDir.FileName[0] == '.') { // .. Dir Entry pointing to root dir.
				dirCluster = pIoman->pPartition->RootDirCluster;
            }
            token = FF_strtok(path, mytoken, &it, &last, pathLen);
    }while(token != NULL);

    return dirCluster;
}
*/

#ifdef FF_LFN_SUPPORT
/**
 *	@private
 **//*
FF_T_SINT8 FF_getLFN(FF_IOMAN *pIoman, FF_BUFFER *pBuffer, FF_DIRENT *pDirent, FF_T_INT8 *filename) {

	FF_T_UINT8	 	numLFNs;
	FF_T_UINT16		lenlfn = 0;
	FF_T_UINT8		tester;
	FF_T_UINT16		i,y;
	FF_T_UINT32		CurrentCluster;
	FF_T_UINT32		fatEntry;
	FF_T_UINT8		*buffer		= pBuffer->pBuffer;
	FF_T_UINT32		Sector		= pBuffer->Sector;
	FF_T_UINT32		Entry		= FF_getMinorBlockEntry(pIoman, pDirent->CurrentItem, 32);

	tester = FF_getChar(pBuffer->pBuffer, (FF_T_UINT16)(Entry * 32));
	numLFNs = (FF_T_UINT8) (tester & ~0x40);

	while(numLFNs > 0) {
		if(FF_getClusterChainNumber(pIoman, pDirent->CurrentItem, 32) > pDirent->CurrentCluster) {
			FF_ReleaseBuffer(pIoman, pBuffer);
			fatEntry = FF_getFatEntry(pIoman, pDirent->DirCluster);
			if(fatEntry == (FF_T_UINT32) FF_ERR_DEVICE_DRIVER_FAILED) {
				return FF_ERR_DEVICE_DRIVER_FAILED;
			}

			if(FF_isEndOfChain(pIoman, fatEntry)) {
				CurrentCluster = pDirent->DirCluster;
				// ERROR THIS SHOULD NOT OCCUR!
			} else {
				CurrentCluster = fatEntry;
			}

			pBuffer = FF_GetBuffer(pIoman, FF_getRealLBA(pIoman, FF_Cluster2LBA(pIoman, CurrentCluster)), FF_MODE_READ);
			if(!pBuffer) {
				return FF_ERR_DEVICE_DRIVER_FAILED;
			}
			Entry = 0;	
		}

		if(Entry > 15) {
			FF_ReleaseBuffer(pIoman, pBuffer);
			Sector += 1;
			pBuffer = FF_GetBuffer(pIoman, Sector, FF_MODE_READ);
			if(!pBuffer) {
				return FF_ERR_DEVICE_DRIVER_FAILED;
			}
			buffer = pBuffer->pBuffer;
			Entry = 0;
		}

		for(i = 0, y = 0; i < 5; i++, y += 2) {
			filename[i + ((numLFNs - 1) * 13)] = buffer[(Entry * 32) + FF_FAT_LFN_NAME_1 + y];
			lenlfn++;
		}

		for(i = 0, y = 0; i < 6; i++, y += 2) {
			filename[i + ((numLFNs - 1) * 13) + 5] = buffer[(Entry * 32) + FF_FAT_LFN_NAME_2 + y];
			lenlfn++;
		}

		for(i = 0, y = 0; i < 2; i++, y += 2) {
			filename[i + ((numLFNs - 1) * 13) + 11] = buffer[(Entry * 32) + FF_FAT_LFN_NAME_3 + y];
			lenlfn++;
		}

		numLFNs--;

		Entry++;
		pDirent->CurrentItem += 1;
	}

	filename[lenlfn] = '\0';

	return 0;
}*/
#endif

/**
 *	@private
 **/
static void FF_ProcessShortName(FF_T_INT8 *name) {
	FF_T_INT8	shortName[13];
	FF_T_UINT8	i;
	memcpy(shortName, name, 11);
	
	for(i = 0; i < 8; i++) {
		if(shortName[i] == 0x20) {
			name[i] = '\0';
			break;
		}
		name[i] = shortName[i];
	}

	if(shortName[8] != 0x20){
		name[i] = '.';
		name[i+1] = shortName[8];
		name[i+2] = shortName[9];
		name[i+3] = shortName[10];
		name[i+4] = '\0';
		for(i = 0; i < 11; i++) {
			if(name[i] == 0x20) {
				name[i] = '\0';
				break;
			}
		}
	} else {
		name[i] = '\0';
	}

}

#ifdef FF_TIME_SUPPORT
static void FF_PlaceTime(FF_T_UINT8 *EntryBuffer, FF_T_UINT32 Offset) {
	FF_T_UINT16		myShort;
	FF_SYSTEMTIME	str_t;

	FF_GetSystemTime(&str_t);
				
	myShort = 0;
	myShort |= ((str_t.Hour    << 11) & 0xF800);
	myShort |= ((str_t.Minute  <<  5) & 0x07E0);
	myShort |= ((str_t.Second   /  2) & 0x001F);
	FF_putShort(EntryBuffer, (FF_T_UINT16) Offset, myShort);
}

static void FF_PlaceDate(FF_T_UINT8 *EntryBuffer, FF_T_UINT32 Offset) {
	FF_T_UINT16		myShort;
	FF_SYSTEMTIME	str_t;

	FF_GetSystemTime(&str_t);
	
	myShort = 0;
	myShort |= (((str_t.Year- 1980)  <<  9) & 0xFE00) ;
	myShort |= ((str_t.Month <<  5) & 0x01E0);
	myShort |= (str_t.Day & 0x001F);
	FF_putShort(EntryBuffer, (FF_T_UINT16) Offset, myShort);
}


static void FF_GetTime(FF_SYSTEMTIME *pTime, FF_T_UINT8 *EntryBuffer, FF_T_UINT32 Offset) {
	FF_T_UINT16 myShort;
	myShort = FF_getShort(EntryBuffer, (FF_T_UINT16) Offset);
	pTime->Hour		= (((myShort & 0xF800) >> 11) & 0x001F);
	pTime->Minute	= (((myShort & 0x07E0) >>  5) & 0x003F);
	pTime->Second	= 2 * (myShort & 0x01F);
}

static void FF_GetDate(FF_SYSTEMTIME *pTime, FF_T_UINT8 *EntryBuffer, FF_T_UINT32 Offset) {
	FF_T_UINT16 myShort;
	myShort = FF_getShort(EntryBuffer, (FF_T_UINT16) Offset);
	pTime->Year		= 1980 + (((myShort & 0xFE00) >> 9) & 0x07F);
	pTime->Month	= (((myShort & 0x01E0) >> 5) & 0x000F);
	pTime->Day		= myShort & 0x01F;
}



#endif

void FF_PopulateShortDirent(FF_IOMAN *pIoman, FF_DIRENT *pDirent, FF_T_UINT8 *EntryBuffer) {
	FF_T_UINT16 myShort;
	
	memcpy(pDirent->FileName, EntryBuffer, 11);	// Copy the filename into the Dirent object.
	FF_ProcessShortName(pDirent->FileName);		// Format the shortname, for pleasant viewing.

#ifdef FF_HASH_TABLE_SUPPORT
#if FF_HASH_FUNCTION == CRC16
	FF_AddDirentHash(pIoman, pDirent->DirCluster, (FF_T_UINT32)FF_GetCRC16((FF_T_UINT8 *) pDirent->FileName, strlen(pDirent->FileName)));
#elif FF_HASH_FUNCTION == CRC8
	FF_AddDirentHash(pIoman, pDirent->DirCluster, (FF_T_UINT32)FF_GetCRC8((FF_T_UINT8 *) pDirent->FileName, strlen(pDirent->FileName)));
#endif
#else
	pIoman = NULL;
#endif

	FF_tolower(pDirent->FileName, (FF_T_UINT32)strlen(pDirent->FileName));

	// Get the item's Cluster address.
	myShort = FF_getShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_HIGH));
	pDirent->ObjectCluster = (FF_T_UINT32) (myShort << 16);
	myShort = FF_getShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_LOW));
	pDirent->ObjectCluster |= myShort;
#ifdef FF_TIME_SUPPORT
	// Get the creation Time & Date
	FF_GetTime(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_CREATE_TIME);
	FF_GetDate(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_CREATE_DATE);
	// Get the modified Time & Date
	FF_GetTime(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_LASTMOD_TIME);
	FF_GetDate(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_LASTMOD_DATE);
	// Get the last accessed Date.
	FF_GetDate(&pDirent->AccessedTime, EntryBuffer, FF_FAT_DIRENT_LASTACC_DATE);
	pDirent->AccessedTime.Hour		= 0;
	pDirent->AccessedTime.Minute	= 0;
	pDirent->AccessedTime.Second	= 0;
#endif
	// Get the filesize.
	pDirent->Filesize = FF_getLong(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_FILESIZE));
	// Get the attribute.
	pDirent->Attrib = FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
}

FF_T_SINT8 FF_FetchEntry(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 nEntry, FF_T_UINT8 *buffer) {
	FF_BUFFER *pBuffer;
	FF_T_UINT32 itemLBA;
	FF_T_UINT32 chainLength		= FF_GetChainLength(pIoman, DirCluster, NULL);	// BottleNeck
	FF_T_UINT32 clusterNum		= FF_getClusterChainNumber	(pIoman, nEntry, (FF_T_UINT16)32);
	FF_T_UINT32 relItem			= FF_getMinorBlockEntry		(pIoman, nEntry, (FF_T_UINT16)32);
	FF_T_UINT32 clusterAddress	= FF_TraverseFAT(pIoman, DirCluster, clusterNum);	// BottleNeck

	if(pIoman->pPartition->Type != FF_T_FAT32) {
		if(DirCluster == pIoman->pPartition->RootDirCluster) {
			chainLength = pIoman->pPartition->RootDirSectors / pIoman->pPartition->SectorsPerCluster;
			if(!chainLength) {		// Some media has RootDirSectors < SectorsPerCluster. This is wrong, as it should be atleast 1 cluster!
				chainLength = 1;
			}
			clusterAddress = DirCluster;
			clusterNum = 0;
			if(nEntry > ((pIoman->pPartition->RootDirSectors * pIoman->pPartition->BlkSize) / 32)) {
				return FF_ERR_DIR_END_OF_DIR;
			}
		}
	}
		
	if((clusterNum + 1) > chainLength) {
		return FF_ERR_DIR_END_OF_DIR;	// End of Dir was reached!
	}

	itemLBA = FF_Cluster2LBA(pIoman, clusterAddress)	+ FF_getMajorBlockNumber(pIoman, nEntry, (FF_T_UINT16)32);
	itemLBA = FF_getRealLBA	(pIoman, itemLBA)			+ FF_getMinorBlockNumber(pIoman, relItem, (FF_T_UINT16)32);
	
	pBuffer = FF_GetBuffer(pIoman, itemLBA, FF_MODE_READ);
	{
		memcpy(buffer, (pBuffer->pBuffer + (relItem*32)), 32);
		pBuffer->Persistance = 1;
	}
	FF_ReleaseBuffer(pIoman, pBuffer);
 
    return FF_ERR_NONE;
}


FF_T_SINT8 FF_PushEntry(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 nEntry, FF_T_UINT8 *buffer) {
	FF_BUFFER *pBuffer;
	FF_T_UINT32 itemLBA;
	FF_T_UINT32 chainLength		= FF_GetChainLength(pIoman, DirCluster, NULL);	// BottleNeck
	FF_T_UINT32 clusterNum		= FF_getClusterChainNumber	(pIoman, nEntry, (FF_T_UINT16)32);
	FF_T_UINT32 relItem			= FF_getMinorBlockEntry		(pIoman, nEntry, (FF_T_UINT16)32);
	FF_T_UINT32 clusterAddress	= FF_TraverseFAT(pIoman, DirCluster, clusterNum);	// BottleNeck
	
	if((clusterNum + 1) > chainLength) {
		return FF_ERR_DIR_END_OF_DIR;	// End of Dir was reached!
	}

	itemLBA = FF_Cluster2LBA(pIoman, clusterAddress)	+ FF_getMajorBlockNumber(pIoman, nEntry, (FF_T_UINT16)32);
	itemLBA = FF_getRealLBA	(pIoman, itemLBA)			+ FF_getMinorBlockNumber(pIoman, relItem, (FF_T_UINT16)32);
	
	pBuffer = FF_GetBuffer(pIoman, itemLBA, FF_MODE_WRITE);
	{
		memcpy((pBuffer->pBuffer + (relItem*32)), buffer, 32);
	}
	FF_ReleaseBuffer(pIoman, pBuffer);
 
    return 0;
}


/**
 *	@private
 **/
FF_ERROR FF_GetEntry(FF_IOMAN *pIoman, FF_T_UINT16 nEntry, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent) {
	FF_T_UINT8 EntryBuffer[32];
	FF_T_UINT8 numLFNs;
	
	if(FF_FetchEntry(pIoman, DirCluster, nEntry, EntryBuffer)) {
			return FF_ERR_DIR_END_OF_DIR;
	}
	if(EntryBuffer[0] != 0xE5) {
		if(FF_isEndOfDir(EntryBuffer)){
			return FF_ERR_DIR_END_OF_DIR;
		}
		
		pDirent->Attrib = FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
		
		if((pDirent->Attrib & FF_FAT_ATTR_LFN) == FF_FAT_ATTR_LFN) {
			// LFN Processing
			numLFNs = (FF_T_UINT8)(EntryBuffer[0] & ~0x40);
	#ifdef FF_LFN_SUPPORT
			FF_PopulateLongDirent(pIoman, pDirent, DirCluster, nEntry);
			return 0;
	#else 
			pDirent->CurrentItem += (numLFNs - 1);
	#endif
		} else if((pDirent->Attrib & FF_FAT_ATTR_VOLID) == FF_FAT_ATTR_VOLID) {
			// Do Nothing
		
		} else {
			FF_PopulateShortDirent(pIoman, pDirent, EntryBuffer);
			pDirent->CurrentItem += 1;
			return 0;
		}
	}
	return FF_ERR_NONE;
}

FF_T_BOOL FF_isEndOfDir(FF_T_UINT8 *EntryBuffer) {
	if(EntryBuffer[0] == 0x00) {
		return FF_TRUE;
	}
	return FF_FALSE;
}

#ifdef FF_HASH_TABLE_SUPPORT
FF_ERROR FF_AddDirentHash(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT32 nHash) {
	FF_T_UINT32 i;
	FF_HASH_TABLE pHash = NULL;
	for(i = 0; i < FF_PATH_CACHE_DEPTH; i++) {
		if(pIoman->pPartition->PathCache[i].DirCluster == DirCluster) {
			pHash = pIoman->pPartition->PathCache[i].pHashTable;
			break;
		}
	}

	if(pHash) {
		FF_SetHash(pHash, nHash);
	}

	return FF_ERR_NONE;
}

FF_T_BOOL FF_CheckDirentHash(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT32 nHash) {
	FF_T_UINT32 i;
	FF_HASH_TABLE pHash = NULL;
	for(i = 0; i < FF_PATH_CACHE_DEPTH; i++) {
		if(pIoman->pPartition->PathCache[i].DirCluster == DirCluster) {
			pHash = pIoman->pPartition->PathCache[i].pHashTable;
			break;
		}
	}

	if(pHash) {
		return FF_isHashSet(pHash, nHash);
	}

	return FF_FALSE;
}

FF_T_BOOL FF_DirHashed(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster) {
	FF_T_UINT32 i;
	for(i = 0; i < FF_PATH_CACHE_DEPTH; i++) {
		if(pIoman->pPartition->PathCache[i].DirCluster == DirCluster) {
			if(pIoman->pPartition->PathCache[i].bHashed) {
				return FF_TRUE;
			}
		}
	}

	return FF_FALSE;
}

void FF_SetDirHashed(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster) {
	int i;
	for(i = 0; i < FF_PATH_CACHE_DEPTH; i++) {
		if(pIoman->pPartition->PathCache[i].DirCluster == DirCluster) {
			pIoman->pPartition->PathCache[i].bHashed = FF_TRUE;
			return;
		}
	}
}
#endif


FF_T_SINT8 FF_PopulateLongDirent(FF_IOMAN *pIoman, FF_DIRENT *pDirent, FF_T_UINT32 DirCluster, FF_T_UINT16 nEntry) {
	FF_T_UINT8	EntryBuffer[32];
	FF_T_INT8	ShortName[13];
	FF_T_UINT8 numLFNs;
	FF_T_UINT8 x;
	FF_T_UINT8 CheckSum = 0;
	FF_T_UINT16 i,y;
	FF_T_UINT16 lenlfn = 0;
	FF_T_UINT16 myShort;
	
	FF_FetchEntry(pIoman, DirCluster, nEntry++, EntryBuffer);
	numLFNs = (FF_T_UINT8)(EntryBuffer[0] & ~0x40);
	// Handle the name
	CheckSum = FF_getChar(EntryBuffer, FF_FAT_LFN_CHECKSUM);

	x = numLFNs;
	while(numLFNs) {
		for(i = 0, y = 0; i < 5; i++, y += 2) {
			pDirent->FileName[i + ((numLFNs - 1) * 13)] = EntryBuffer[FF_FAT_LFN_NAME_1 + y];
			lenlfn++;
		}

		for(i = 0, y = 0; i < 6; i++, y += 2) {
			pDirent->FileName[i + ((numLFNs - 1) * 13) + 5] = EntryBuffer[FF_FAT_LFN_NAME_2 + y];
			lenlfn++;
		}

		for(i = 0, y = 0; i < 2; i++, y += 2) {
			pDirent->FileName[i + ((numLFNs - 1) * 13) + 11] = EntryBuffer[FF_FAT_LFN_NAME_3 + y];
			lenlfn++;
		}

		FF_FetchEntry(pIoman, DirCluster, nEntry++, EntryBuffer);
		numLFNs--;
	}

	pDirent->FileName[lenlfn] = '\0';
	
	// Process the ShortName Entry
	memcpy(ShortName, EntryBuffer, 11);
	if(CheckSum != FF_CreateChkSum(EntryBuffer)) {
		FF_ProcessShortName(ShortName);
		strcpy(pDirent->FileName, ShortName);
	} else {
		FF_ProcessShortName(ShortName);
	}

#ifdef FF_HASH_TABLE_SUPPORT
#if FF_HASH_FUNCTION == CRC16
	FF_AddDirentHash(pIoman, DirCluster, (FF_T_UINT32)FF_GetCRC16((FF_T_UINT8 *) ShortName, strlen(ShortName)));
#elif FF_HASH_FUNCTION == CRC8
	FF_AddDirentHash(pIoman, DirCluster, (FF_T_UINT32)FF_GetCRC8((FF_T_UINT8 *) ShortName, strlen(ShortName)));
#endif
#endif
	
	myShort = FF_getShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_HIGH));
	pDirent->ObjectCluster = (FF_T_UINT32) (myShort << 16);
	myShort = FF_getShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_LOW));
	pDirent->ObjectCluster |= myShort;

#ifdef FF_TIME_SUPPORT
	// Get the creation Time & Date
	FF_GetTime(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_CREATE_TIME);
	FF_GetDate(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_CREATE_DATE);
	// Get the modified Time & Date
	FF_GetTime(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_LASTMOD_TIME);
	FF_GetDate(&pDirent->CreateTime, EntryBuffer, FF_FAT_DIRENT_LASTMOD_DATE);
	// Get the last accessed Date.
	FF_GetDate(&pDirent->AccessedTime, EntryBuffer, FF_FAT_DIRENT_LASTACC_DATE);
	pDirent->AccessedTime.Hour		= 0;
	pDirent->AccessedTime.Minute	= 0;
	pDirent->AccessedTime.Second	= 0;
#endif

	// Get the filesize.
	pDirent->Filesize = FF_getLong(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_FILESIZE));
	// Get the attribute.
	pDirent->Attrib = FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
	
	pDirent->CurrentItem = nEntry;
	return x;
}

/**
 *	@public
 *	@brief	Find's the first directory entry for the provided path.
 *
 *	All values recorded in pDirent must be preserved to and between calls to
 *	FF_FindNext().
 *
 *	@param	pIoman		FF_IOMAN object that was created by FF_CreateIOMAN().
 *	@param	pDirent		FF_DIRENT object to store the entry information.
 *	@param	path		String to of the path to the Dir being listed.
 *
 *	@return	0 on success
 *	@return	FF_ERR_DEVICE_DRIVER_FAILED if device access failed.
 *	@return -2 if Dir was not found.
 *
 **/
FF_ERROR FF_FindFirst(FF_IOMAN *pIoman, FF_DIRENT *pDirent, const FF_T_INT8 *path) {
	FF_T_UINT8	numLFNs;
	FF_T_UINT8	EntryBuffer[32];
	FF_T_UINT16	PathLen = (FF_T_UINT16) strlen(path);

	if(!pIoman) {
		return FF_ERR_NULL_POINTER;
	}

	pDirent->DirCluster = FF_FindDir(pIoman, path, PathLen);	// Get the directory cluster, if it exists.

	if(pDirent->DirCluster == 0) {
		return FF_ERR_DIR_INVALID_PATH;
	}
	
	for(pDirent->CurrentItem = 0; pDirent->CurrentItem < 0xFFFF; pDirent->CurrentItem += 1) {
		if(FF_FetchEntry(pIoman, pDirent->DirCluster, pDirent->CurrentItem, EntryBuffer)) {
			return FF_ERR_DIR_END_OF_DIR;
		}
		if(EntryBuffer[0] != FF_FAT_DELETED) {
			if(FF_isEndOfDir(EntryBuffer)){
				return FF_ERR_DIR_END_OF_DIR;
			}
			pDirent->Attrib = FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
			if((pDirent->Attrib & FF_FAT_ATTR_LFN) == FF_FAT_ATTR_LFN) {
				// LFN Processing
				numLFNs = (FF_T_UINT8)(EntryBuffer[0] & ~0x40);
				// Get the shortname and check if it is marked deleted.
#ifdef FF_LFN_SUPPORT
				// Fetch the shortname, and get it's checksum, or for a deleted item with
				// orphaned LFN entries.
				if(FF_FetchEntry(pIoman, pDirent->DirCluster, (pDirent->CurrentItem + numLFNs), EntryBuffer)) {
					return FF_ERR_DIR_END_OF_DIR;
				}
				
				if(EntryBuffer[0] != FF_FAT_DELETED) {
					FF_PopulateLongDirent(pIoman, pDirent, pDirent->DirCluster, pDirent->CurrentItem);
					return FF_ERR_NONE;
				}
#else 
				pDirent->CurrentItem += (numLFNs - 1);
#endif
			} else if((pDirent->Attrib & FF_FAT_ATTR_VOLID) == FF_FAT_ATTR_VOLID) {
				// Do Nothing
			
			} else {
				FF_PopulateShortDirent(pIoman, pDirent, EntryBuffer);
				pDirent->CurrentItem += 1;
				return FF_ERR_NONE;
			}
		}
	}

	return FF_ERR_DIR_END_OF_DIR;
}

/**
 *	@public
 *	@brief	Get's the next Entry based on the data recorded in the FF_DIRENT object.
 *
 *	All values recorded in pDirent must be preserved to and between calls to
 *	FF_FindNext().
 *
 *	@param	pIoman		FF_IOMAN object that was created by FF_CreateIOMAN().
 *	@param	pDirent		FF_DIRENT object to store the entry information.
 *
 *	@return FF_ERR_DEVICE_DRIVER_FAILED is device access failed.
 *
 **/
FF_ERROR FF_FindNext(FF_IOMAN *pIoman, FF_DIRENT *pDirent) {
	
	FF_T_UINT8	numLFNs;
	FF_T_UINT8	EntryBuffer[32];

	if(!pIoman) {
		return FF_ERR_NULL_POINTER;
	}
	
	for(; pDirent->CurrentItem < 0xFFFF; pDirent->CurrentItem += 1) {
		if(FF_FetchEntry(pIoman, pDirent->DirCluster, pDirent->CurrentItem, EntryBuffer)) {
			return FF_ERR_DIR_END_OF_DIR;
		}
		if(EntryBuffer[0] != FF_FAT_DELETED) {
			if(FF_isEndOfDir(EntryBuffer)){
				return FF_ERR_DIR_END_OF_DIR;
			}
			pDirent->Attrib = FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
			if((pDirent->Attrib & FF_FAT_ATTR_LFN) == FF_FAT_ATTR_LFN) {
				// LFN Processing
				numLFNs = (FF_T_UINT8)(EntryBuffer[0] & ~0x40);
				// Get the shortname and check if it is marked deleted.
#ifdef FF_LFN_SUPPORT
				// Fetch the shortname, and get it's checksum, or for a deleted item with
				// orphaned LFN entries.
				if(FF_FetchEntry(pIoman, pDirent->DirCluster, (pDirent->CurrentItem + numLFNs), EntryBuffer)) {
					return FF_ERR_DIR_END_OF_DIR;
				}
				
				if(EntryBuffer[0] != FF_FAT_DELETED) {
					FF_PopulateLongDirent(pIoman, pDirent, pDirent->DirCluster, pDirent->CurrentItem);
					return FF_ERR_NONE;
				}
#else 
				pDirent->CurrentItem += (numLFNs - 1);
#endif
			} else if((pDirent->Attrib & FF_FAT_ATTR_VOLID) == FF_FAT_ATTR_VOLID) {
				// Do Nothing
			
			} else {
				FF_PopulateShortDirent(pIoman, pDirent, EntryBuffer);
				pDirent->CurrentItem += 1;
				return FF_ERR_NONE;
			}
		}
	}
	
	return FF_ERR_DIR_END_OF_DIR;
}


FF_T_SINT8 FF_RewindFind(FF_IOMAN *pIoman, FF_DIRENT *pDirent) {
	if(!pIoman) {
		return FF_ERR_NULL_POINTER;
	}
	pDirent->CurrentItem = 0;
	return 0;
}


FF_T_SINT32 FF_FindFreeDirent(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 Sequential) {

	FF_T_UINT8	EntryBuffer[32];
	FF_T_UINT16	i = 0;
	FF_T_UINT16 nEntry;
	FF_T_SINT8	RetVal;
	FF_T_UINT32	DirLength;
	FF_T_UINT32	iEndOfChain;
	
	for(nEntry = 0; nEntry < 0xFFFF; nEntry++) {
		if(FF_FetchEntry(pIoman, DirCluster, nEntry, EntryBuffer) == FF_ERR_DIR_END_OF_DIR) {
			RetVal = FF_ExtendDirectory(pIoman, DirCluster);
			if(RetVal != FF_ERR_NONE) {
				return RetVal;
			}
			return nEntry;
		}
		if(FF_isEndOfDir(EntryBuffer)) {	// If its the end of the Dir, then FreeDirents from here.
			// Check Dir is long enough!
			DirLength = FF_GetChainLength(pIoman, DirCluster, &iEndOfChain);
			if((nEntry + Sequential) > (FF_T_UINT16)(((pIoman->pPartition->SectorsPerCluster * pIoman->pPartition->BlkSize) * DirLength) / 32)) {
				FF_ExtendDirectory(pIoman, DirCluster);
			}
			return nEntry;
		}
		if(EntryBuffer[0] == 0xE5) {
			i++;
		} else {
			i = 0;
		}

		if(i == Sequential) {
			return (nEntry - (Sequential - 1));// Return the beginning entry in the sequential sequence.
		}
	}
	
	return FF_ERR_DIR_DIRECTORY_FULL;	
}




FF_T_SINT8 FF_PutEntry(FF_IOMAN *pIoman, FF_T_UINT16 Entry, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent) {
	FF_BUFFER *pBuffer;
	FF_T_UINT32 itemLBA;
	FF_T_UINT32 clusterNum		= FF_getClusterChainNumber	(pIoman, Entry, (FF_T_UINT16)32);
	FF_T_UINT32 relItem			= FF_getMinorBlockEntry		(pIoman, Entry, (FF_T_UINT16)32);
	FF_T_UINT32 clusterAddress	= FF_TraverseFAT(pIoman, DirCluster, clusterNum);

	itemLBA = FF_Cluster2LBA(pIoman, clusterAddress)	+ FF_getMajorBlockNumber(pIoman, Entry, (FF_T_UINT16)32);
	itemLBA = FF_getRealLBA	(pIoman, itemLBA)			+ FF_getMinorBlockNumber(pIoman, relItem, (FF_T_UINT16)32);
	
	pBuffer = FF_GetBuffer(pIoman, itemLBA, FF_MODE_WRITE);
	{
		// Modify the Entry!
		//memcpy((pBuffer->pBuffer + (32*relItem)), pDirent->FileName, 11);
		FF_putChar(pBuffer->pBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB + (32 * relItem)), pDirent->Attrib);
		FF_putShort(pBuffer->pBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_HIGH + (32 * relItem)), (FF_T_UINT16)(pDirent->ObjectCluster >> 16));
		FF_putShort(pBuffer->pBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_LOW  + (32 * relItem)), (FF_T_UINT16)(pDirent->ObjectCluster));
		FF_putLong(pBuffer->pBuffer,  (FF_T_UINT16)(FF_FAT_DIRENT_FILESIZE  + (32 * relItem)), pDirent->Filesize);
#ifdef FF_TIME_SUPPORT
	FF_PlaceDate((pBuffer->pBuffer + (32 * relItem)), FF_FAT_DIRENT_LASTACC_DATE);	// Last accessed date.
#endif
	}
	FF_ReleaseBuffer(pIoman, pBuffer);
 
    return 0;
}


/*static FF_T_BOOL FF_isShortName(const FF_T_UINT8 *Name, FF_T_UINT16 StrLen) {
	FF_T_UINT16 i;
	for(i = 0; i < StrLen; i++) {
		if(Name[i] == '.') {
			i--;
		}
	}
	if(i < 11) {
		return FF_TRUE;
	}
	return FF_FALSE;
}*/

FF_T_SINT8 FF_CreateShortName(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *ShortName, FF_T_INT8 *LongName) {
	FF_T_UINT16 i,x,y;
	FF_T_INT8	TempName[FF_MAX_FILENAME];
	FF_T_INT8	MyShortName[13];
	FF_T_UINT16 NameLen; 
	FF_T_BOOL	FitsShort = FF_FALSE;
	FF_DIRENT	MyDir;
	//FF_T_SINT8	RetVal = 0;
	FF_T_INT8	NumberBuf[6];
	// Create a Short Name
	strncpy(TempName, LongName, FF_MAX_FILENAME);
	NameLen = (FF_T_UINT16) strlen(TempName);
	FF_toupper(TempName, NameLen);

	// Initialise Shortname

	for(i = 0; i < 11; i++) {
		ShortName[i] = 0x20;
	}

	// Does LongName fit a shortname?

	for(i = 0, x = 0; i < NameLen; i++) {
		if(TempName[i] != '.') {
			x++;
		}
	}

	if(x <= 11) {
		//FitsShort = FF_TRUE;
	}

	// Main part of the name
	for(i = 0, x = 0; i < 8; i++, x++) {
		if(i == 0 && TempName[x] == '.') {
			i--;
		} else {
			if(TempName[x] == '.') {
				break;
			} else if(TempName[x] == ' ') {
				i--;
			} else {
				ShortName[i] = TempName[x];
				if(ShortName[i] == 0x00) {
					ShortName[i] = 0x20;
				}
			}
		}
	}
	
	for(i = NameLen; i > x; i--) {
		if(TempName[i] == '.') {
			break;
		}
	}

	if(TempName[i] == '.') {
		x = i + 1;
		for(i = 0; i < 3; i++) {
			if(x < NameLen) {
				ShortName[8 + i] = TempName[x++];
			}
		}
	}

	// Tail :
	memcpy(MyShortName, ShortName, 11);
	FF_ProcessShortName(MyShortName);
	
	if(!FF_FindEntryInDir(pIoman, DirCluster, MyShortName, 0x00, &MyDir) && FitsShort) {
		return 0;
	} else {
		if(FitsShort) {
			return FF_ERR_DIR_OBJECT_EXISTS;
		}
		for(i = 1; i < 0x0000FFFF; i++) { // Max Number of Entries in a DIR!
			sprintf(NumberBuf, "%d", i);
			NameLen = (FF_T_UINT16) strlen(NumberBuf);
			x = 7 - NameLen;
			ShortName[x++] = '~';
			for(y = 0; y < NameLen; y++) {
				ShortName[x+y] = NumberBuf[y];
			}
			memcpy(MyShortName, ShortName, 11);
			FF_ProcessShortName(MyShortName);
			if(!FF_ShortNameExists(pIoman, DirCluster, MyShortName)) {
				return 0;
			}
		}
		// Add a tail and special number until we're happy :D
	}

	return FF_ERR_DIR_DIRECTORY_FULL;
}
#ifdef FF_LFN_SUPPORT
static FF_T_SINT8 FF_CreateLFNEntry(FF_T_UINT8 *EntryBuffer, FF_T_INT8 *Name, FF_T_UINT8 NameLen, FF_T_UINT8 nLFN, FF_T_UINT8 CheckSum) {
	
	FF_T_UINT8 i, x;
	
	memset(EntryBuffer, 0, 32);

	FF_putChar(EntryBuffer, FF_FAT_LFN_ORD,			(FF_T_UINT8) ((nLFN & ~0x40)));
	FF_putChar(EntryBuffer, FF_FAT_DIRENT_ATTRIB,	(FF_T_UINT8) FF_FAT_ATTR_LFN);
	FF_putChar(EntryBuffer, FF_FAT_LFN_CHECKSUM,	(FF_T_UINT8) CheckSum);

	// Name_1
	for(i = 0, x = 0; i < 5; i++, x += 2) {
		if(i < NameLen) {
			EntryBuffer[FF_FAT_LFN_NAME_1 + x] = Name[i];
		} else if (i == NameLen) {
			EntryBuffer[FF_FAT_LFN_NAME_1 + x] = '\0';
		}else {
			EntryBuffer[FF_FAT_LFN_NAME_1 + x]		= 0xFF;
			EntryBuffer[FF_FAT_LFN_NAME_1 + x + 1]	= 0xFF;
		}
	}

	// Name_2
	for(i = 0, x = 0; i < 6; i++, x += 2) {
		if((i + 5) < NameLen) {
			EntryBuffer[FF_FAT_LFN_NAME_2 + x] = Name[i+5];
		} else if ((i + 5) == NameLen) {
			EntryBuffer[FF_FAT_LFN_NAME_2 + x] = '\0';
		}else {
			EntryBuffer[FF_FAT_LFN_NAME_2 + x]		= 0xFF;
			EntryBuffer[FF_FAT_LFN_NAME_2 + x + 1]	= 0xFF;
		}
	}

	// Name_3
	for(i = 0, x = 0; i < 2; i++, x += 2) {
		if((i + 11) < NameLen) {
			EntryBuffer[FF_FAT_LFN_NAME_3 + x] = Name[i+11];
		} else if ((i + 11) == NameLen) {
			EntryBuffer[FF_FAT_LFN_NAME_3 + x] = '\0';
		}else {
			EntryBuffer[FF_FAT_LFN_NAME_3 + x]		= 0xFF;
			EntryBuffer[FF_FAT_LFN_NAME_3 + x + 1]	= 0xFF;
		}
	}
	
	return FF_ERR_NONE;
}

static FF_T_SINT8 FF_CreateLFNs(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *Name, FF_T_UINT8 CheckSum, FF_T_UINT16 nEntry) {
	FF_T_UINT8	EntryBuffer[32];
	FF_T_UINT16 NameLen = (FF_T_UINT16) strlen(Name);
	FF_T_UINT8	NumLFNs = (FF_T_UINT8)	(NameLen / 13);
	FF_T_UINT8	i;
	FF_T_UINT8	EndPos = (NameLen % 13);

	if(EndPos) {
		NumLFNs ++;
	} else {
		EndPos = 13;
	}

	for(i = NumLFNs; i > 0; i--) {
		if(i == NumLFNs) {
			FF_CreateLFNEntry(EntryBuffer, (Name + (13 * (i - 1))), EndPos, i, CheckSum);
			EntryBuffer[0] |= 0x40;
		} else {
			FF_CreateLFNEntry(EntryBuffer, (Name + (13 * (i - 1))), 13, i, CheckSum);
		}
		FF_PushEntry(pIoman, DirCluster, nEntry + (NumLFNs - i), EntryBuffer);
	}

	return FF_ERR_NONE;
}
#endif

FF_T_SINT8 FF_ExtendDirectory(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster) {
	FF_T_UINT32 CurrentCluster;
	FF_T_UINT32 NextCluster;

	if(pIoman->pPartition->Type != FF_T_FAT32) {
		if(DirCluster == pIoman->pPartition->RootDirCluster) {
			return FF_ERR_DIR_CANT_EXTEND_ROOT_DIR;
		}
	}

	if(!pIoman->pPartition->FreeClusterCount) {
		pIoman->pPartition->FreeClusterCount = FF_CountFreeClusters(pIoman);
		if(pIoman->pPartition->FreeClusterCount == 0) {
			return FF_ERR_FAT_NO_FREE_CLUSTERS;
		}
	}
	
	FF_lockFAT(pIoman);
	{
		CurrentCluster = FF_FindEndOfChain(pIoman, DirCluster);
		NextCluster = FF_FindFreeCluster(pIoman);
		FF_putFatEntry(pIoman, CurrentCluster, NextCluster);
		FF_putFatEntry(pIoman, NextCluster, 0xFFFFFFFF);
	}
	FF_unlockFAT(pIoman);

	FF_ClearCluster(pIoman, NextCluster);
	FF_DecreaseFreeClusters(pIoman, 1);

	return FF_ERR_NONE;
}

static void FF_MakeNameCompliant(FF_T_INT8 *Name) {
	
	if((FF_T_UINT8) Name[0] == 0xE5) {	// Support Japanese KANJI symbol.
		Name[0] = 0x05;
	}
	
	while(*Name) {
		if(*Name < 0x20 || *Name == 0x7F || *Name == 0x22 || *Name == 0x7C) {	// Leave all extended chars as they are.
			*Name = '_';
		}
		if(*Name >= 0x2A && *Name <= 0x2F && *Name != 0x2B && *Name != 0x2E) {
			*Name = '_';
		}
		if(*Name >= 0x3A && *Name <= 0x3F) {
			*Name = '_';
		}
		if(*Name >= 0x5B && *Name <= 0x5C) {
			*Name = '_';
		}
		Name++;
	}
}

FF_T_SINT8 FF_CreateDirent(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent) {
	
	FF_T_UINT8	EntryBuffer[32];
	FF_T_UINT16	NameLen = (FF_T_UINT16) strlen(pDirent->FileName);
	FF_T_UINT8	numLFNs = (FF_T_UINT8) (NameLen / 13);
	FF_T_SINT32	FreeEntry;
	FF_T_SINT8	RetVal = 0;
	FF_T_UINT8	Entries;
#ifdef FF_LFN_SUPPORT
	FF_T_UINT8	CheckSum;
#endif

	FF_MakeNameCompliant(pDirent->FileName);	// Ensure we don't break the Dir tables.
	memset(EntryBuffer, 0, 32);



	if(NameLen % 13) {
		numLFNs ++;
	}

#ifdef FF_LFN_SUPPORT
	// Create and push the LFN's
	Entries = numLFNs + 1;	// Find enough places for the LFNs and the ShortName	
#else
	Entries = 1;
#endif

	// Create the ShortName
	FF_lockDIR(pIoman);
	{
		if((FreeEntry = FF_FindFreeDirent(pIoman, DirCluster, Entries)) >= 0) {
			RetVal = FF_CreateShortName(pIoman, DirCluster, (FF_T_INT8 *) EntryBuffer, pDirent->FileName);
			
			if(!RetVal) {
#ifdef FF_LFN_SUPPORT
				CheckSum = FF_CreateChkSum(EntryBuffer);
				FF_CreateLFNs(pIoman, DirCluster, pDirent->FileName, CheckSum, (FF_T_UINT16) FreeEntry);
#else
				numLFNs = 0;
#endif				
				
#ifdef FF_TIME_SUPPORT
				FF_PlaceTime(EntryBuffer, FF_FAT_DIRENT_CREATE_TIME);
				FF_PlaceDate(EntryBuffer, FF_FAT_DIRENT_CREATE_DATE);
				FF_PlaceTime(EntryBuffer, FF_FAT_DIRENT_LASTMOD_TIME);
				FF_PlaceDate(EntryBuffer, FF_FAT_DIRENT_LASTMOD_DATE);
#endif

				FF_putChar(EntryBuffer,  (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB), pDirent->Attrib);
				FF_putShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_HIGH), (FF_T_UINT16)(pDirent->ObjectCluster >> 16));
				FF_putShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_LOW), (FF_T_UINT16)(pDirent->ObjectCluster));
				FF_putLong(EntryBuffer,  (FF_T_UINT16)(FF_FAT_DIRENT_FILESIZE), pDirent->Filesize);

				FF_PushEntry(pIoman, DirCluster, (FF_T_UINT16) (FreeEntry + numLFNs), EntryBuffer);
			}
		}else {
			RetVal = (FF_T_SINT8) FreeEntry;
		}
	}
	FF_unlockDIR(pIoman);

	if(RetVal) {
		return RetVal;
	}

	if(pDirent) {
		pDirent->CurrentItem = (FF_T_UINT16) (FreeEntry + numLFNs);
	}
	
	return 0;
}

FF_T_UINT32 FF_CreateFile(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *FileName, FF_DIRENT *pDirent) {
	FF_DIRENT MyFile;
	strncpy(MyFile.FileName, FileName, FF_MAX_FILENAME);

	MyFile.Attrib = 0x00;
	MyFile.Filesize = 0;
	MyFile.ObjectCluster = FF_CreateClusterChain(pIoman);
	MyFile.CurrentItem = 0;

	if(FF_CreateDirent(pIoman, DirCluster, &MyFile)) {
		FF_UnlinkClusterChain(pIoman, MyFile.ObjectCluster, 0);
		FF_FlushCache(pIoman);
		return 0;
	}

	FF_FlushCache(pIoman);

	if(pDirent) {
		memcpy(pDirent, &MyFile, sizeof(FF_DIRENT));
	}

	return MyFile.ObjectCluster;
}


/**
 *	@brief Creates a Directory of the specified path.
 *
 *	@param	pIoman	Pointer to the FF_IOMAN object.
 *	@param	Path	Path of the directory to create.
 *
 *	@return	FF_ERR_NULL_POINTER if pIoman was NULL.
 *	@return FF_ERR_DIR_OBJECT_EXISTS if the object specified by path already exists.
 *	@return	FF_ERR_DIR_INVALID_PATH
 *	@return FF_ERR_NONE on success.
 **/
FF_ERROR FF_MkDir(FF_IOMAN *pIoman, const FF_T_INT8 *Path) {
	FF_DIRENT	MyDir;
	FF_T_UINT32 DirCluster;
	FF_T_INT8	DirName[FF_MAX_FILENAME];
	FF_T_UINT8	EntryBuffer[32];
	FF_T_UINT32 DotDotCluster;
	FF_T_UINT16	i;
	FF_T_SINT8	RetVal = 0;

	if(!pIoman) {
		return FF_ERR_NULL_POINTER;
	}

	i = (FF_T_UINT16) strlen(Path);

	while(i != 0) {
		if(Path[i] == '\\' || Path[i] == '/') {
			break;
		}
		i--;
	}

	strncpy(DirName, (Path + i + 1), FF_MAX_FILENAME);

	if(i == 0) {
		i = 1;
	}

	DirCluster = FF_FindDir(pIoman, Path, i);

	if(DirCluster) {

		/*if(!FF_FindEntry(pIoman, DirCluster, DirName, &MyDir, FF_TRUE)) {
			return FF_ERR_DIR_OBJECT_EXISTS;
		}*/
		if(FF_FindEntryInDir(pIoman, DirCluster, DirName, 0x00, &MyDir)) {
			return FF_ERR_DIR_OBJECT_EXISTS;
		}

		strncpy(MyDir.FileName, DirName, FF_MAX_FILENAME);
		MyDir.Filesize		= 0;
		MyDir.Attrib		= FF_FAT_ATTR_DIR;
		MyDir.ObjectCluster	= FF_CreateClusterChain(pIoman);
		FF_ClearCluster(pIoman, MyDir.ObjectCluster);

		RetVal = FF_CreateDirent(pIoman, DirCluster, &MyDir);

		if(RetVal) {
			FF_UnlinkClusterChain(pIoman, MyDir.ObjectCluster, 0);
			FF_FlushCache(pIoman);
			return RetVal;
		}
		
		memset(EntryBuffer, 0, 32);
		EntryBuffer[0] = '.';
		for(i = 1; i < 11; i++) {
			EntryBuffer[i] = 0x20;
		}
		FF_putChar(EntryBuffer,  (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB), FF_FAT_ATTR_DIR);
		FF_putShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_HIGH), (FF_T_UINT16)(MyDir.ObjectCluster >> 16));
		FF_putShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_LOW), (FF_T_UINT16) MyDir.ObjectCluster);
		FF_putLong(EntryBuffer,  (FF_T_UINT16)(FF_FAT_DIRENT_FILESIZE), 0);

		FF_PushEntry(pIoman, MyDir.ObjectCluster, 0, EntryBuffer);

		memset(EntryBuffer, 0, 32);
		EntryBuffer[0] = '.';
		EntryBuffer[1] = '.';
		for(i = 2; i < 11; i++) {
			EntryBuffer[i] = 0x20;
		}
		
		if(DirCluster == pIoman->pPartition->RootDirCluster) {
			DotDotCluster = 0;
		} else {
			DotDotCluster = DirCluster;
		}

		FF_putChar(EntryBuffer,  (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB), FF_FAT_ATTR_DIR);
		FF_putShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_HIGH), (FF_T_UINT16)(DotDotCluster >> 16));
		FF_putShort(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_CLUS_LOW), (FF_T_UINT16) DotDotCluster);
		FF_putLong(EntryBuffer,  (FF_T_UINT16)(FF_FAT_DIRENT_FILESIZE), 0);
		
		FF_PushEntry(pIoman, MyDir.ObjectCluster, 1, EntryBuffer);
		
		FF_FlushCache(pIoman);	// Ensure dir was flushed to the disk!

		return FF_ERR_NONE;
	}
	
	return FF_ERR_DIR_INVALID_PATH;
}



void FF_RmLFNs(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 DirEntry) {

	FF_T_UINT8	EntryBuffer[32];

	DirEntry--;

	do {
		FF_FetchEntry(pIoman, DirCluster, DirEntry, EntryBuffer);
		
		if(FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB)) == FF_FAT_ATTR_LFN) {
			FF_putChar(EntryBuffer, (FF_T_UINT16) 0, (FF_T_UINT8) 0xE5);
			FF_PushEntry(pIoman, DirCluster, DirEntry, EntryBuffer);
		}

		if(DirEntry == 0) {
			break;
		}
		DirEntry--;
	}while(FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB)) == FF_FAT_ATTR_LFN);

}
