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
 *	@file		ff_file.h
 *	@author		James Walmsley
 *	@ingroup	FILEIO
 **/
#ifndef _FF_FILE_H_
#define _FF_FILE_H_

#include "ff_config.h"
#include "ff_types.h"
#include "ff_ioman.h"
#include "ff_dir.h"

#ifdef FF_USE_NATIVE_STDIO
#include <stdio.h>
#define FF_SEEK_SET	SEEK_SET
#define FF_SEEK_CUR	SEEK_CUR
#define FF_SEEK_END	SEEK_END
#else
#define FF_SEEK_SET	1
#define FF_SEEK_CUR	2
#define FF_SEEK_END	3
#endif

typedef struct _FF_FILE {
	FF_IOMAN		*pIoman;			///< Ioman Pointer!
	FF_T_UINT32		 Filesize;			///< File's Size.
	FF_T_UINT32		 ObjectCluster;		///< File's Start Cluster.
	FF_T_UINT32		 iChainLength;		///< Total Length of the File's cluster chain.
	FF_T_UINT32		 CurrentCluster;	///< Prevents FAT Thrashing.
	FF_T_UINT32		 AddrCurrentCluster;///< Address of the current cluster.
	FF_T_UINT32		 iEndOfChain;		///< Address of the last cluster in the chain.
	FF_T_UINT32		 FilePointer;		///< Current Position Pointer.
	//FF_T_UINT32	 AppendPointer;		///< Points to the Append from position. (The original filesize at open).
	FF_T_UINT8		 Mode;				///< Mode that File Was opened in.
	FF_T_UINT32		 DirCluster;		///< Cluster Number that the Dirent is in.
	FF_T_UINT16		 DirEntry;			///< Dirent Entry Number describing this file.
	//FF_T_UINT8		 NumLFNs;			///< Number of LFNs associated with this file.
	FF_T_BOOL		 FileDeleted;
	struct _FF_FILE *Next;				///< Pointer to the next file object in the linked list.
} FF_FILE,
*PFF_FILE;

//---------- PROTOTYPES
// PUBLIC (Interfaces):

#ifdef FF_UNICODE_SUPPORT
FF_FILE *FF_Open(FF_IOMAN *pIoman, const FF_T_WCHAR *path, FF_T_UINT8 Mode, FF_ERROR *pError);
FF_T_BOOL	 FF_isDirEmpty	(FF_IOMAN *pIoman, const FF_T_WCHAR *Path);
FF_ERROR	 FF_RmFile		(FF_IOMAN *pIoman, const FF_T_WCHAR *path);
FF_ERROR	 FF_RmDir		(FF_IOMAN *pIoman, const FF_T_WCHAR *path);
FF_ERROR	 FF_Move		(FF_IOMAN *pIoman, const FF_T_WCHAR *szSourceFile, const FF_T_WCHAR *szDestinationFile);
#else
FF_FILE *FF_Open(FF_IOMAN *pIoman, const FF_T_INT8 *path, FF_T_UINT8 Mode, FF_ERROR *pError);
FF_T_BOOL	 FF_isDirEmpty	(FF_IOMAN *pIoman, const FF_T_INT8 *Path);
FF_ERROR	 FF_RmFile		(FF_IOMAN *pIoman, const FF_T_INT8 *path);
FF_ERROR	 FF_RmDir		(FF_IOMAN *pIoman, const FF_T_INT8 *path);
FF_ERROR	 FF_Move		(FF_IOMAN *pIoman, const FF_T_INT8 *szSourceFile, const FF_T_INT8 *szDestinationFile);
#endif
FF_ERROR	 FF_Close		(FF_FILE *pFile);
FF_T_SINT32	 FF_GetC		(FF_FILE *pFile);
FF_T_SINT32  FF_GetLine		(FF_FILE *pFile, FF_T_INT8 *szLine, FF_T_UINT32 ulLimit);
FF_T_SINT32	 FF_Read		(FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8 *buffer);
FF_T_SINT32	 FF_Write		(FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8 *buffer);
FF_T_BOOL	 FF_isEOF		(FF_FILE *pFile);
FF_ERROR	 FF_Seek		(FF_FILE *pFile, FF_T_SINT32 Offset, FF_T_INT8 Origin);
FF_T_SINT32	 FF_PutC		(FF_FILE *pFile, FF_T_UINT8 Value);
FF_T_UINT32	 FF_Tell		(FF_FILE *pFile);

FF_T_UINT8	 FF_GetModeBits	(FF_T_INT8 *Mode);

// Private :

#endif
