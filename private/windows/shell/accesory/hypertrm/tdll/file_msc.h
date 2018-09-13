/*	File: D:\WACKER\tdll\file_msc.h (Created: 26-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:37p $
 */

/*
 * Error codes
 */
#define	FM_ERR_BASE				0x400
#define	FM_ERR_NO_MEM			FM_ERR_BASE+1
#define	FM_ERR_BAD_HANDLE		FM_ERR_BASE+1

/*
 * Constants
 */
#define	FM_CHUNK_SIZE			64

extern HFILES CreateFilesDirsHdl(const HSESSION hSession);

extern INT InitializeFilesDirsHdl(const HSESSION hSession, HFILES hFile);

extern INT LoadFilesDirsHdl(HFILES hFile);

extern INT DestroyFilesDirsHdl(const HFILES hFile);

extern INT SaveFilesDirsHdl(const HFILES hFile);

extern LPCTSTR filesQuerySendDirectory(HFILES hFile);

extern LPCTSTR filesQueryRecvDirectory(HFILES hFile);

extern VOID filesSetRecvDirectory(HFILES hFile, LPCTSTR pszDir);

extern VOID filesSetSendDirectory(HFILES hFile, LPCTSTR pszDir);

extern HBITMAP fileReadBitmapFromFile(HDC hDC, LPTSTR pszName, int fCmp);

/*
 * The following function returns data in the following format:
 *
 * An array of pointers to strings is alocated. As file names are found, a
 * new string is allocated and the pointer to the string is put in the array.
 * The array is expanded as needed.  Freeing memory is the responsiblity of
 * the caller.
 */

extern int fileBuildFileList(void **pData,
							int *pCnt,
							LPCTSTR pszName,
							int nSubdir,
							LPCTSTR pszDirectory);


extern int fileFinalizeName(LPTSTR pszOldname,
					LPTSTR pszOlddir,
					LPTSTR pszNewname,
					const size_t cb);

extern int fileFinalizeDIR(HSESSION hSession,
							LPTSTR pszOldname,
							LPTSTR pszNewname);

/*
 * The following are generic functions that an operating system SHOULD have.
 */
extern int GetFileSizeFromName(TCHAR *pszName,
							unsigned long * const pulFileSize);
int SetFileSize(const TCHAR *pszName, unsigned long ulFileSize);

extern int IsValidDirectory(LPCTSTR pszName);

extern int GetModuleDirectoryName(HINSTANCE hInst, LPTSTR pszName, int dLen);

extern int ValidateFileName(LPSTR pszName);
