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
	FF_T_UINT32	ulChainLength;
	FF_T_UINT32	ulDirCluster;
	FF_T_UINT32	ulCurrentClusterLCN;
	FF_T_UINT32	ulCurrentClusterNum;
	FF_T_UINT32	ulCurrentEntry;
	FF_BUFFER	*pBuffer;
} FF_FETCH_CONTEXT;

typedef struct {
	FF_T_UINT32 Filesize;
	FF_T_UINT32	ObjectCluster;

	// Book Keeping
	FF_T_UINT32	CurrentCluster;
	FF_T_UINT32 AddrCurrentCluster;
	FF_T_UINT32	DirCluster;
	FF_T_UINT16	CurrentItem;
	// End Book Keeping

#ifdef FF_TIME_SUPPORT
	FF_SYSTEMTIME	CreateTime;		///< Date and Time Created.
	FF_SYSTEMTIME	ModifiedTime;	///< Date and Time Modified.
	FF_SYSTEMTIME	AccessedTime;	///< Date of Last Access.
#endif

#ifdef FF_FINDAPI_ALLOW_WILDCARDS
#ifdef FF_UNICODE_SUPPORT
	FF_T_WCHAR	szWildCard[FF_MAX_FILENAME];
#else
	FF_T_INT8	szWildCard[FF_MAX_FILENAME];
#endif
#endif

#ifdef FF_UNICODE_SUPPORT
	FF_T_WCHAR	FileName[FF_MAX_FILENAME];
#else
	FF_T_INT8	FileName[FF_MAX_FILENAME];
#endif

#if defined(FF_LFN_SUPPORT) && defined(FF_INCLUDE_SHORT_NAME)
	FF_T_INT8	ShortName[13];
#endif
	FF_T_UINT8	Attrib;
	FF_FETCH_CONTEXT FetchContext;
} FF_DIRENT;



// PUBLIC API
#ifdef FF_UNICODE_SUPPORT
FF_ERROR	FF_FindFirst		(FF_IOMAN *pIoman, FF_DIRENT *pDirent, const FF_T_WCHAR *path);
FF_ERROR	FF_MkDir			(FF_IOMAN *pIoman, const FF_T_WCHAR *Path);
#else
FF_ERROR	FF_FindFirst		(FF_IOMAN *pIoman, FF_DIRENT *pDirent, const FF_T_INT8 *path);
FF_ERROR	FF_MkDir			(FF_IOMAN *pIoman, const FF_T_INT8 *Path);
#endif

FF_ERROR	FF_FindNext			(FF_IOMAN *pIoman, FF_DIRENT *pDirent);


// INTERNAL API
FF_ERROR	FF_GetEntry			(FF_IOMAN *pIoman, FF_T_UINT16 nEntry, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent);
FF_ERROR	FF_PutEntry			(FF_IOMAN *pIoman, FF_T_UINT16 Entry, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent);
FF_T_SINT8	FF_FindEntry		(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *Name, FF_DIRENT *pDirent, FF_T_BOOL LFNs);

void		FF_PopulateShortDirent		(FF_IOMAN *pIoman, FF_DIRENT *pDirent, FF_T_UINT8 *EntryBuffer);
FF_ERROR	FF_PopulateLongDirent		(FF_IOMAN *pIoman, FF_DIRENT *pDirent, FF_T_UINT16 nEntry, FF_FETCH_CONTEXT *pFetchContext);

FF_ERROR	FF_InitEntryFetch			(FF_IOMAN *pIoman, FF_T_UINT32 ulDirCluster, FF_FETCH_CONTEXT *pContext);
FF_ERROR	FF_FetchEntryWithContext	(FF_IOMAN *pIoman, FF_T_UINT32 ulEntry, FF_FETCH_CONTEXT *pContext, FF_T_UINT8 *pEntryBuffer);
FF_ERROR	FF_PushEntryWithContext		(FF_IOMAN *pIoman, FF_T_UINT32 ulEntry, FF_FETCH_CONTEXT *pContext, FF_T_UINT8 *pEntryBuffer);
void		FF_CleanupEntryFetch		(FF_IOMAN *pIoman, FF_FETCH_CONTEXT *pContext);

FF_T_SINT8	FF_PushEntry				(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT16 nEntry, FF_T_UINT8 *buffer, void *pParam);
FF_T_BOOL	FF_isEndOfDir				(FF_T_UINT8 *EntryBuffer);
FF_ERROR	FF_FindNextInDir			(FF_IOMAN *pIoman, FF_DIRENT *pDirent, FF_FETCH_CONTEXT *pFetchContext);

#ifdef FF_UNICODE_SUPPORT
FF_T_UINT32 FF_FindEntryInDir			(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, const FF_T_WCHAR *name, FF_T_UINT8 pa_Attrib, FF_DIRENT *pDirent, FF_ERROR *pError);
FF_T_SINT8	FF_CreateShortName			(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_WCHAR *ShortName, FF_T_WCHAR *LongName);
#else
FF_T_UINT32 FF_FindEntryInDir			(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, const FF_T_INT8 *name, FF_T_UINT8 pa_Attrib, FF_DIRENT *pDirent, FF_ERROR *pError);
FF_T_SINT8	FF_CreateShortName			(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *ShortName, FF_T_INT8 *LongName);
#endif


void		FF_lockDIR			(FF_IOMAN *pIoman);
void		FF_unlockDIR		(FF_IOMAN *pIoman);

#ifdef FF_UNICODE_SUPPORT
FF_T_UINT32 FF_CreateFile(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_WCHAR *FileName, FF_DIRENT *pDirent, FF_ERROR *pError);
#else
FF_T_UINT32 FF_CreateFile(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_INT8 *FileName, FF_DIRENT *pDirent, FF_ERROR *pError);
#endif

FF_ERROR		FF_CreateDirent		(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_DIRENT *pDirent);
FF_ERROR		FF_ExtendDirectory	(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster);

#ifdef FF_UNICODE_SUPPORT
FF_T_UINT32		FF_FindDir			(FF_IOMAN *pIoman, const FF_T_WCHAR *path, FF_T_UINT16 pathLen, FF_ERROR *pError);
#else
FF_T_UINT32		FF_FindDir			(FF_IOMAN *pIoman, const FF_T_INT8 *path, FF_T_UINT16 pathLen, FF_ERROR *pError);
#endif

#ifdef FF_HASH_CACHE
FF_T_BOOL FF_CheckDirentHash		(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT32 nHash);
FF_T_BOOL FF_DirHashed				(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster);
FF_ERROR FF_AddDirentHash			(FF_IOMAN *pIoman, FF_T_UINT32 DirCluster, FF_T_UINT32 nHash);
FF_ERROR FF_HashDir					(FF_IOMAN *pIoman, FF_T_UINT32 ulDirCluster);
#endif

FF_ERROR FF_RmLFNs(FF_IOMAN *pIoman, FF_T_UINT16 usDirEntry, FF_FETCH_CONTEXT *pContext);

#endif

