/*	File: D:\WACKER\tdll\file_io.h (Created: 26-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:40p $
 */

/*
 * This stuff is a replacement for some sort of buffered file I/O.
 *
 * It is directly modeled after (read lifted from) the "stdio.h" stuff.
 */

#if !defined(EOF)
#define	EOF	(-1)
#endif

#define	_FIO_IOEOF		0x0001
#define	_FIO_IOERR		0x0002

#define	_FIO_BSIZE		512

#define	_FIO_MAGIC		0x1234A587

struct _fileio_buf {
	long      _fio_magic;
	char     *_fio_ptr;
	int       _fio_cnt;
	char     *_fio_base;
	int       _fio_flag;
	int       _file;			/* Not used, replaced with the following */
	HANDLE    _fio_handle;
	int       _fio_mode;
	int       _fio_charbuf;
	int       _fio_bufsiz;
	char     *_fio_tmpfname;
	};

typedef struct _fileio_buf ST_IOBUF;

/* Macro definitions */

#define fio_feof(_stream)	  ((_stream)->_fio_flag & _FIO_IOEOF)

#define fio_ferror(_stream)   ((_stream)->_fio_flag & _FIO_IOERR)

#define	fio_errclr(_stream)	((_stream)->_fio_flag = 0)

#define _fileno(_stream)  ((_stream)->_file)

#define	fio_gethandle(_stream)	((_stream)->_fio_handle)

#define fio_getc(_s)	  (--(_s)->_fio_cnt >= 0 \
		? 0xff & *(_s)->_fio_ptr++ : _fio_fill_buf(_s))

/* TODO: make this work better */
#define	fio_ungetc(_c,_s)	(*(--(_s)->_fio_ptr) = (char)_c);((_s)->_fio_cnt++)

#define fio_putc(_c,_s)  (--(_s)->_fio_cnt >= 0 \
		? 0xff & (*(_s)->_fio_ptr++ = (char)(_c)) : _fio_flush_buf((_c),(_s)))

int _fio_fill_buf(ST_IOBUF *);

int _fio_flush_buf(int, ST_IOBUF *);

/* mode flags for fio_open, may be or'd together */

#define	FIO_CREATE	0x0001
#define	FIO_READ	0x0002
#define	FIO_WRITE	0x0004
/* append means just reposition at the end of the file after open */
#define	FIO_APPEND	0x0008

ST_IOBUF *fio_open(char *, int);

int fio_close(ST_IOBUF *);

#define	FIO_SEEK_CUR	0x0001
#define	FIO_SEEK_END	0x0002
#define	FIO_SEEK_SET	0x0003

int fio_seek(ST_IOBUF *, size_t, int);

int fio_read(void *buffer, size_t size, size_t count, ST_IOBUF *pF);

int fio_write(void *buffer, size_t size, size_t count, ST_IOBUF *pF);

