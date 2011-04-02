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
 *	@file		ff_dir.h
 *	@author		James Walmsley
 *	@ingroup	DIR
 **/
#ifndef _FF_DIR_H_
#define _FF_DIR_H_

#include "ff_types.h"
#include "ff_config.h"
#include "ff_error.h"
#include "ff_ioman.h"
#include "ff_blk.h"
#include "ff_fat.h"
#include "ff_fatdef.h"
#include "ff_memory.h"
#include "ff_time.h"
#include "ff_hash.h"
#include "ff_crc.h"
#include "ff_file.h"
#include <string.h>

typedef struct {
	FF_T_INT8	FileName[FF_MAX_FILENAME];
	FF_T_UINT8	Attrib;
	FF_T_UINT32 Filesize;
	FF_T_UINT32	ObjectCluster;

#ifdef FF_TIME_SUPPORT	
	FF_SYSTEMTIME	CreateTime;		///< Date and Time Created.
	FF_SYSTEMTIME	ModifiedTime;	///< Date and Time Modified.
	FF_SYSTEMTIME	AccessedTime;	///< Date of Last Access.
#endif

	//---- Book Keeping for FF_Find Functions
	FF_T_UINT16	CurrentItem;	
	FF_T_UINT32	DirCluster;
	FF_T_UINT32	CurrentCluster;
	FF_T_UINT32 AddrCurrentCluster;
	//FF_T_UINT8	NumLFNs;
} FF_DIRENT;

		FF_ERROR	FF_GetEntry		(FF_IOMAN *pIoman, FF_T_UINT16 nEntry, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent);
		FF_T_SINT8  FF_PutEntry		(FF_IOMAN *pIoman, FF_T_UINT16 Entry, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent);
		FF_T_SINT8	FF_FindEntry	(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *Name, FF_DIRENT *pDirent, FF_T_BOOL LFNs);
		FF_ERROR	FF_FindFirst	(FF_IOMAN *pIoman, FF_DIRENT *pDirent, const FF_T_INT8 *path);
		FF_ERROR	FF_FindNext		(FF_IOMAN *pIoman, FF_DIRENT *pDirent);
		void FF_PopulateShortDirent(FF_IOMAN *pIoman, FF_DIRENT *pDirent, FF_T_UINT8 *EntryBuffer);
		FF_T_SINT8	FF_PopulateLongDirent(FF_IOMAN *pIoman, FF_DIRENT *pDirent, FF_T_UINT32 DirCluster, FF_T_UINT16 nEntry);
		FF_T_SINT8	FF_FetchEntry	(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 nEntry, FF_T_UINT8 *buffer);
		FF_T_SINT8	FF_PushEntry	(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 nEntry, FF_T_UINT8 *buffer);
		FF_T_BOOL	FF_isEndOfDir	(FF_T_UINT8 *EntryBuffer);
		FF_T_SINT8	FF_FindNextInDir(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent);
		FF_T_UINT32 FF_FindEntryInDir(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *name, FF_T_UINT8 pa_Attrib, FF_DIRENT *pDirent);
		FF_T_SINT8	FF_CreateShortName(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *ShortName, FF_T_INT8 *LongName);

void		FF_lockDIR		(FF_IOMAN *pIoman);
void		FF_unlockDIR	(FF_IOMAN *pIoman);
FF_T_UINT32			FF_CreateFile(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *FileName, FF_DIRENT *pDirent);
FF_ERROR FF_MkDir(FF_IOMAN *pIoman, const FF_T_INT8 *Path);
FF_T_SINT8 FF_CreateDirent(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent);
FF_T_SINT8 FF_ExtendDirectory(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster);
FF_T_UINT32 FF_FindDir(FF_IOMAN *pIoman, const FF_T_INT8 *path, FF_T_UINT16 pathLen);


FF_T_BOOL FF_CheckDirentHash(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT32 nHash);
FF_T_BOOL FF_DirHashed(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster);
FF_ERROR FF_AddDirentHash(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT32 nHash);
void FF_SetDirHashed(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster);

void FF_RmLFNs(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 DirEntry);

#endif

