/*	File: D:\WACKER\tdll\file_io.c (Created: 26-Jan-1994)
 *
 *	Copyright 1994,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */
#include <windows.h>
#pragma hdrstop

// #define	DEBUGSTR	1
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>

#include "file_io.h"
#include "file_msc.h"

/*
 * This stuff is a replacement for some sort of buffered file I/O.
 *
 * It is directly modeled after (read lifted from) the "stdio.h" stuff.
 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	_fio_fill_buf
 *
 * DESCRIPTION:
 *	This is an "internal" function called by the "fio_getc" macro.  It is a
 *	replacement for the "_filbuf" function in "stdio".
 *
 * PARAMETERS:
 *	pF -- a pointer to a file structure.
 *
 * RETURNS:
 *	The next character available or an EOF.
 */
int _fio_fill_buf(ST_IOBUF *pF)
	{
	DWORD dwSize;

	assert(pF);
	assert(pF->_fio_magic == _FIO_MAGIC);

	if (pF->_fio_flag != 0)
		return EOF;

	if (pF->_fio_base == NULL)
		{
		pF->_fio_base = malloc(pF->_fio_bufsiz);
		if (pF->_fio_base == NULL)
			{
			pF->_fio_flag |= _FIO_IOERR;
			return EOF;
			}
		}

	pF->_fio_ptr = pF->_fio_base;

	dwSize = 0;
	DbgOutStr("fio_fill_buf reads %d bytes", pF->_fio_bufsiz, 0,0,0,0);
	ReadFile(pF->_fio_handle,
			pF->_fio_ptr,
			pF->_fio_bufsiz,
			&dwSize,
			NULL);
	DbgOutStr("...done\r\n", 0,0,0,0,0);
	if (dwSize == 0)
		{
		pF->_fio_flag |= _FIO_IOEOF;
		return EOF;
		}

	pF->_fio_cnt = dwSize;

	return (fio_getc(pF));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	_fio_flush_buf
 *
 * DESCRIPTION:
 *	This is an "internal" function called by the "fio_putc" macro.  It is a
 *	replacement for the "_flsbuf" function in "stdio".
 *
 * PARAMETERS:
 *	c   -- the next character to be written out
 *	pF  -- a pointer to a file structure
 *
 * RETURNS:
 *	The character buffered of an EOF.
 */
int _fio_flush_buf(int c, ST_IOBUF *pF)
	{
	int size;
	DWORD dwFoo;

	assert(pF);
	assert(pF->_fio_magic == _FIO_MAGIC);

	if (pF->_fio_flag != 0)
		return EOF;

	if (pF->_fio_base == NULL)
		{
		pF->_fio_base = malloc(pF->_fio_bufsiz);
		if (pF->_fio_base == NULL)
			{
			pF->_fio_flag |= _FIO_IOERR;
			return EOF;
			}
		}
	else
		{
		/* We have been here before, dump the buffer */
		size = (int)(pF->_fio_ptr - pF->_fio_base);
		if (size > 0)
			{
			DbgOutStr("fio_putc writes %d bytes", size, 0,0,0,0);
			WriteFile(pF->_fio_handle,
					pF->_fio_base,
					size,
					&dwFoo,
					NULL);
			DbgOutStr("...done\r\n", 0,0,0,0,0);
			}
		}
	pF->_fio_ptr = pF->_fio_base;
	pF->_fio_cnt = pF->_fio_bufsiz;

	return fio_putc(c, pF);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fio_open
 *
 * DESCRIPTION:
 *	This function creates a file structure handle and initializes the handle.
 *
 * PARAMETERS:
 *	fname -- a pointer to the file name
 *	mode  -- flags, see file_io.h
 *
 * RETURNS:
 *	A pointer to an initialize structure or a NULL.
 */
ST_IOBUF *fio_open(char *fname, int mode)
	{
	int nFileExists;
	DWORD dwMode;
	DWORD dwShare;
	DWORD dwCreate;
	ST_IOBUF *pF;

	nFileExists = GetFileSizeFromName(fname, NULL);

	dwMode = 0;
	if ((mode & FIO_READ) != 0)
		dwMode |= GENERIC_READ;
	if ((mode & FIO_WRITE) != 0)
		dwMode |= GENERIC_WRITE;
	if (dwMode == 0)
		return NULL;

	dwShare = FILE_SHARE_READ;
	if ((mode & FIO_WRITE) == 0)
		dwShare |= FILE_SHARE_WRITE;

	dwCreate = 0;
	if ((mode & FIO_CREATE) == 0)
		{
		/* Don't wack the file here */
		if (nFileExists)
			{
			dwCreate = OPEN_EXISTING;
			}
		else
			{
			dwCreate = CREATE_NEW;
			}
		}
	else
		{
		/* FIO_CREATE means always wack the file */
		if (nFileExists)
			{
			if ((mode & FIO_WRITE) == 0)
				{
				dwCreate = OPEN_EXISTING;
				}
			else
				{
				dwCreate = TRUNCATE_EXISTING;
				}
			}
		else
			{
			dwCreate = CREATE_NEW;
			}
		}

	pF = (ST_IOBUF *)malloc(sizeof(ST_IOBUF));
	if (pF != (ST_IOBUF *)0)
		{
		pF->_fio_magic = 0;
		pF->_fio_ptr = NULL;
		pF->_fio_cnt = 0;
		pF->_fio_base = NULL;
		pF->_fio_flag = 0;
		pF->_file = 0;
		pF->_fio_handle = 0;
		pF->_fio_mode = mode;
		pF->_fio_charbuf = 0;
		pF->_fio_bufsiz = _FIO_BSIZE;
		pF->_fio_tmpfname = NULL;

		/*
		 * Try and open the file
		 */
		pF->_fio_handle = CreateFile(fname,
									dwMode,
									dwShare,
									NULL,
									dwCreate,
									0,
									NULL);
		if (pF->_fio_handle == INVALID_HANDLE_VALUE)
			{
			free(pF);
			pF = (ST_IOBUF *)0;
			}
		}
	if (pF)
		{
		if ((mode & FIO_APPEND) != 0)
			{
			SetFilePointer(pF->_fio_handle,
							0,
							NULL,
							FILE_END);
			}
		}
	if (pF)
		{
		/* Mark as a valid structure */
		pF->_fio_magic = _FIO_MAGIC;
		}
	return pF;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fio_close
 *
 * DESCRIPTION:
 *	This function flushes whatever data needs to be flushed and closes stuff
 *	up.
 *
 * PARAMETERS:
 *	pF  -- a pointer to a file structure
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an EOF.
 */
int fio_close(ST_IOBUF *pF)
	{
	int size;
	DWORD dwFoo;

	assert(pF);
	assert(pF->_fio_magic == _FIO_MAGIC);

	if (pF)
		{
		/*
		 * Make sure any data is written out
		 */
		if ((pF->_fio_mode & FIO_WRITE) != 0)
			{
			if (pF->_fio_ptr != NULL)
				{
				size = (int)(pF->_fio_ptr - pF->_fio_base);
				if (size > 0)
					{
					DbgOutStr("fio_close writes %d bytes", size, 0,0,0,0);
					WriteFile(pF->_fio_handle,
							pF->_fio_base,
							size,
							&dwFoo,
							NULL);
					DbgOutStr("...done\r\n", 0,0,0,0,0);
					}
				}
			}

		CloseHandle(pF->_fio_handle);
		pF->_fio_handle = INVALID_HANDLE_VALUE;
		free(pF);
		pF = NULL;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fio_seek
 *
 * DESCRIPTION:
 *	This function is a replacement for the fseek function.
 *
 * PARAMETERS:
 *	pF       -- a pointer to a file structure
 *	position -- where to move the file pointer to
 *	mode     -- starting address of the move
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an EOF.
 */
int fio_seek(ST_IOBUF *pF, size_t position, int mode)
	{
	DWORD dwMethod;
	int size;

	assert(pF);
	assert(pF->_fio_magic == _FIO_MAGIC);

	switch (mode)
		{
		default:
			return EOF;
		case FIO_SEEK_CUR:
			dwMethod = FILE_CURRENT;
			break;
		case FIO_SEEK_END:
			dwMethod = FILE_END;
			break;
		case FIO_SEEK_SET:
			dwMethod = FILE_BEGIN;
			break;
		}

	if (pF)
		{
		/*
		 * Make sure any data is written out
		 */
		if ((pF->_fio_mode & FIO_WRITE) != 0)
			{
			if (pF->_fio_ptr != NULL)
				{
				size = (int)(pF->_fio_ptr - pF->_fio_base);
				if (size > 0)
					{
					DbgOutStr("fio_seek writes %d bytes", size, 0,0,0,0);
					WriteFile(pF->_fio_handle,
							pF->_fio_base,
							size,
							NULL,
							NULL);
					DbgOutStr("...done\r\n", 0,0,0,0,0);
					}
				}
			}
		pF->_fio_cnt = 0;

		SetFilePointer(pF->_fio_handle,
						position,
						NULL,
						dwMethod);
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fio_read
 *
 * DESCRIPTION:
 *	This function is a replacement for the "fread" function in "stdio".
 *
 * PARAMETERS:
 *	buffer -- address of the data to read
 *	size   -- the size of each item (object?) to read
 *	count  -- the number of items to read
 *	pF     -- a pointer to a file structure
 *
 * RETURNS:
 *	The number of items read from the file, ZERO indicating EOF.
 */
int fio_read(void *buffer, size_t size, size_t count, ST_IOBUF *pF)
	{
	DWORD dwSize;
	DWORD dwGot;

	assert(pF);
	assert(pF->_fio_magic == _FIO_MAGIC);

	/* For now, don't allow intermix of buffered and non_buffered */
	assert(pF->_fio_base == NULL);

	if (pF)
		{
		dwSize = (DWORD)(size * count);
		dwGot = 0;
		DbgOutStr("fio_read reads %d bytes", dwSize, 0,0,0,0);
		ReadFile(pF->_fio_handle,
				buffer,
				dwSize,
				&dwGot,
				NULL);
		DbgOutStr("...done\r\n", 0,0,0,0,0);
		if (dwGot == 0)
			{
			pF->_fio_flag |= _FIO_IOEOF;
			}
		return dwGot / size;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fio_write
 *
 * DESCRIPTION:
 *	This function is a replacement for the "fwrite" function in "stdio".
 *
 * PARAMETERS:
 *	buffer -- address of the data to write
 *	size   -- the size of each item (object?) to write
 *	count  -- the number of items to write
 *	pF     -- a pointer to a file structure
 *
 * RETURNS:
 *	The number of items written to the file.
 */
int fio_write(void *buffer, size_t size, size_t count, ST_IOBUF *pF)
	{
	DWORD dwSize;
	DWORD dwPut;

	assert(pF);
	assert(pF->_fio_magic == _FIO_MAGIC);

	/* For now, don't allow intermix of buffered and non_buffered */
	assert(pF->_fio_base == NULL);

	if (pF)
		{
		dwSize = (DWORD)(size * count);
		dwPut = 0;
		DbgOutStr("fio_write writes %d bytes", dwSize, 0,0,0,0);
		WriteFile(pF->_fio_handle,
				buffer,
				dwSize,
				&dwPut,
				NULL);
		DbgOutStr("...done\r\n", 0,0,0,0,0);
		if (dwPut == 0)
			{
			pF->_fio_flag |= _FIO_IOEOF;
			}
		return dwPut / size;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
