/*
 * LZ Decompression functions
 *
 * Copyright 1996 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *
 * The LZ (Lempel Ziv) decompression was used in win16 installation programs.
 * It is a simple tabledriven decompression engine, the algorithm is not
 * documented as far as I know. WINE does not contain a compressor for
 * this format.
 *
 * The implementation is complete and there have been no reports of failures
 * for some time.
 *
 * TODO:
 *
 *   o Check whether the return values are correct
 *
 */

#ifdef __REACTOS__

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

#define HFILE_ERROR ((HFILE)-1)

#include "lzexpand.h"

#define _lwrite(a, b, c) (long)(_hwrite(a, b, (long)c))

#else /* __REACTOS__ */

#include "config.h"

#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "lzexpand.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#endif /* __REACTOS__ */

/* The readahead length of the decompressor. Reading single bytes
 * using _lread() would be SLOW.
 */
#define	GETLEN	2048

#define LZ_MAGIC_LEN    8
#define LZ_HEADER_LEN   14

/* Format of first 14 byte of LZ compressed file */
struct lzfileheader {
	BYTE	magic[LZ_MAGIC_LEN];
	BYTE	compressiontype;
	CHAR	lastchar;
	DWORD	reallength;
};
static const BYTE LZMagic[LZ_MAGIC_LEN]={'S','Z','D','D',0x88,0xf0,0x27,0x33};

#define LZ_TABLE_SIZE    0x1000

struct lzstate {
	HFILE	realfd;		/* the real filedescriptor */
	CHAR	lastchar;	/* the last char of the filename */

	DWORD	reallength;	/* the decompressed length of the file */
	DWORD	realcurrent;	/* the position the decompressor currently is */
	DWORD	realwanted;	/* the position the user wants to read from */

	BYTE	table[LZ_TABLE_SIZE];	/* the rotating LZ table */
	UINT	curtabent;	/* CURrent TABle ENTry */

	BYTE	stringlen;	/* length and position of current string */
	DWORD	stringpos;	/* from stringtable */


	WORD	bytetype;	/* bitmask within blocks */

	BYTE	*get;		/* GETLEN bytes */
	DWORD	getcur;		/* current read */
	DWORD	getlen;		/* length last got */
};

#define MAX_LZSTATES 16
static struct lzstate *lzstates[MAX_LZSTATES];

#define LZ_MIN_HANDLE  0x400
#define IS_LZ_HANDLE(h) (((h) >= LZ_MIN_HANDLE) && ((h) < LZ_MIN_HANDLE+MAX_LZSTATES))
#define GET_LZ_STATE(h) (IS_LZ_HANDLE(h) ? lzstates[(h)-LZ_MIN_HANDLE] : NULL)

/* reads one compressed byte, including buffering */
#define GET(lzs,b)	_lzget(lzs,&b)
#define GET_FLUSH(lzs)	lzs->getcur=lzs->getlen;

static int
_lzget(struct lzstate *lzs,BYTE *b) {
	if (lzs->getcur<lzs->getlen) {
		*b		= lzs->get[lzs->getcur++];
		return		1;
	} else {
		int ret = _lread(lzs->realfd,lzs->get,GETLEN);
		if (ret==HFILE_ERROR)
			return HFILE_ERROR;
		if (ret==0)
			return 0;
		lzs->getlen	= ret;
		lzs->getcur	= 1;
		*b		= *(lzs->get);
		return 1;
	}
}
/* internal function, reads lzheader
 * returns BADINHANDLE for non filedescriptors
 * return 0 for file not compressed using LZ
 * return UNKNOWNALG for unknown algorithm
 * returns lzfileheader in *head
 */
static INT read_header(HFILE fd,struct lzfileheader *head)
{
	BYTE	buf[LZ_HEADER_LEN];

	if (_llseek(fd,0,SEEK_SET)==-1)
		return LZERROR_BADINHANDLE;

	/* We can't directly read the lzfileheader struct due to
	 * structure element alignment
	 */
	if (_lread(fd,buf,LZ_HEADER_LEN)<LZ_HEADER_LEN)
		return 0;
	memcpy(head->magic,buf,LZ_MAGIC_LEN);
	memcpy(&(head->compressiontype),buf+LZ_MAGIC_LEN,1);
	memcpy(&(head->lastchar),buf+LZ_MAGIC_LEN+1,1);

	/* FIXME: consider endianness on non-intel architectures */
	memcpy(&(head->reallength),buf+LZ_MAGIC_LEN+2,4);

	if (memcmp(head->magic,LZMagic,LZ_MAGIC_LEN))
		return 0;
	if (head->compressiontype!='A')
		return LZERROR_UNKNOWNALG;
	return 1;
}


/***********************************************************************
 *           LZStart   (KERNEL32.@)
 */
INT WINAPI LZStart(void)
{
    TRACE("(void)\n");
    return 1;
}


/***********************************************************************
 *           LZInit   (KERNEL32.@)
 *
 * initializes internal decompression buffers, returns lzfiledescriptor.
 * (return value the same as hfSrc, if hfSrc is not compressed)
 * on failure, returns error code <0
 * lzfiledescriptors range from 0x400 to 0x410 (only 16 open files per process)
 *
 * since _llseek uses the same types as libc.lseek, we just use the macros of
 *  libc
 */
HFILE WINAPI LZInit( HFILE hfSrc )
{

	struct	lzfileheader	head;
	struct	lzstate		*lzs;
	int	i, ret;

	TRACE("(%d)\n",hfSrc);
	ret=read_header(hfSrc,&head);
	if (ret<=0) {
		_llseek(hfSrc,0,SEEK_SET);
		return ret?ret:hfSrc;
	}
        for (i = 0; i < MAX_LZSTATES; i++) if (!lzstates[i]) break;
        if (i == MAX_LZSTATES) return LZERROR_GLOBALLOC;
	lzstates[i] = lzs = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*lzs) );
	if(lzs == NULL) return LZERROR_GLOBALLOC;

	lzs->realfd	= hfSrc;
	lzs->lastchar	= head.lastchar;
	lzs->reallength = head.reallength;

	lzs->get	= HeapAlloc( GetProcessHeap(), 0, GETLEN );
	lzs->getlen	= 0;
	lzs->getcur	= 0;

	if(lzs->get == NULL) {
		HeapFree(GetProcessHeap(), 0, lzs);
		lzstates[i] = NULL;
		return LZERROR_GLOBALLOC;
	}

	/* Yes, preinitialize with spaces */
	memset(lzs->table,' ',LZ_TABLE_SIZE);
	/* Yes, start 16 byte from the END of the table */
	lzs->curtabent	= 0xff0;
	return LZ_MIN_HANDLE + i;
}


/***********************************************************************
 *           LZDone   (KERNEL32.@)
 */
void WINAPI LZDone(void)
{
    TRACE("(void)\n");
}


/***********************************************************************
 *           GetExpandedNameA   (KERNEL32.@)
 *
 * gets the full filename of the compressed file 'in' by opening it
 * and reading the header
 *
 * "file." is being translated to "file"
 * "file.bl_" (with lastchar 'a') is being translated to "file.bla"
 * "FILE.BL_" (with lastchar 'a') is being translated to "FILE.BLA"
 */

INT WINAPI GetExpandedNameA( LPSTR in, LPSTR out )
{
	struct lzfileheader	head;
	HFILE		fd;
	OFSTRUCT	ofs;
	INT		fnislowercased,ret,len;
	LPSTR		s,t;

	TRACE("(%s)\n",in);
	fd=OpenFile(in,&ofs,OF_READ);
	if (fd==HFILE_ERROR)
		return (INT)(INT16)LZERROR_BADINHANDLE;
	strcpy(out,in);
	ret=read_header(fd,&head);
	if (ret<=0) {
		/* not a LZ compressed file, so the expanded name is the same
		 * as the input name */
		_lclose(fd);
		return 1;
	}


	/* look for directory prefix and skip it. */
	s=out;
	while (NULL!=(t=strpbrk(s,"/\\:")))
		s=t+1;

	/* now mangle the basename */
	if (!*s) {
		/* FIXME: hmm. shouldn't happen? */
		WARN("Specified a directory or what? (%s)\n",in);
		_lclose(fd);
		return 1;
	}
	/* see if we should use lowercase or uppercase on the last char */
	fnislowercased=1;
	t=s+strlen(s)-1;
	while (t>=out) {
		if (!isalpha(*t)) {
			t--;
			continue;
		}
		fnislowercased=islower(*t);
		break;
	}
	if (isalpha(head.lastchar)) {
		if (fnislowercased)
			head.lastchar=tolower(head.lastchar);
		else
			head.lastchar=toupper(head.lastchar);
	}

	/* now look where to replace the last character */
	if (NULL!=(t=strchr(s,'.'))) {
		if (t[1]=='\0') {
			t[0]='\0';
		} else {
			len=strlen(t)-1;
			if (t[len]=='_')
				t[len]=head.lastchar;
		}
	} /* else no modification necessary */
	_lclose(fd);
	return 1;
}


/***********************************************************************
 *           GetExpandedNameW   (KERNEL32.@)
 */
INT WINAPI GetExpandedNameW( LPWSTR in, LPWSTR out )
{
    INT ret;
    DWORD len = WideCharToMultiByte( CP_ACP, 0, in, -1, NULL, 0, NULL, NULL );
    char *xin = HeapAlloc( GetProcessHeap(), 0, len );
    char *xout = HeapAlloc( GetProcessHeap(), 0, len+3 );
    WideCharToMultiByte( CP_ACP, 0, in, -1, xin, len, NULL, NULL );
    if ((ret = GetExpandedNameA( xin, xout )) > 0)
        MultiByteToWideChar( CP_ACP, 0, xout, -1, out, strlenW(in)+4 );
    HeapFree( GetProcessHeap(), 0, xin );
    HeapFree( GetProcessHeap(), 0, xout );
    return ret;
}


/***********************************************************************
 *           LZRead   (KERNEL32.@)
 */
INT WINAPI LZRead( HFILE fd, LPSTR vbuf, INT toread )
{
	int	howmuch;
	BYTE	b,*buf;
	struct	lzstate	*lzs;

	buf=(LPBYTE)vbuf;
	TRACE("(%d,%p,%d)\n",fd,buf,toread);
	howmuch=toread;
	if (!(lzs = GET_LZ_STATE(fd))) return _lread(fd,buf,toread);

/* The decompressor itself is in a define, cause we need it twice
 * in this function. (the decompressed byte will be in b)
 */
#define DECOMPRESS_ONE_BYTE 						\
		if (lzs->stringlen) {					\
			b		= lzs->table[lzs->stringpos];	\
			lzs->stringpos	= (lzs->stringpos+1)&0xFFF;	\
			lzs->stringlen--;				\
		} else {						\
			if (!(lzs->bytetype&0x100)) {			\
				if (1!=GET(lzs,b)) 			\
					return toread-howmuch;		\
				lzs->bytetype = b|0xFF00;		\
			}						\
			if (lzs->bytetype & 1) {			\
				if (1!=GET(lzs,b))			\
					return toread-howmuch;		\
			} else {					\
				BYTE	b1,b2;				\
									\
				if (1!=GET(lzs,b1))			\
					return toread-howmuch;		\
				if (1!=GET(lzs,b2))			\
					return toread-howmuch;		\
				/* Format:				\
				 * b1 b2				\
				 * AB CD 				\
				 * where CAB is the stringoffset in the table\
				 * and D+3 is the len of the string	\
				 */					\
				lzs->stringpos	= b1|((b2&0xf0)<<4);	\
				lzs->stringlen	= (b2&0xf)+2; 		\
				/* 3, but we use a  byte already below ... */\
				b		= lzs->table[lzs->stringpos];\
				lzs->stringpos	= (lzs->stringpos+1)&0xFFF;\
			}						\
			lzs->bytetype>>=1;				\
		}							\
		/* store b in table */					\
		lzs->table[lzs->curtabent++]= b;			\
		lzs->curtabent	&= 0xFFF;				\
		lzs->realcurrent++;

	/* if someone has seeked, we have to bring the decompressor
	 * to that position
	 */
	if (lzs->realcurrent!=lzs->realwanted) {
		/* if the wanted position is before the current position
		 * I see no easy way to unroll ... We have to restart at
		 * the beginning. *sigh*
		 */
		if (lzs->realcurrent>lzs->realwanted) {
			/* flush decompressor state */
			_llseek(lzs->realfd,LZ_HEADER_LEN,SEEK_SET);
			GET_FLUSH(lzs);
			lzs->realcurrent= 0;
			lzs->bytetype	= 0;
			lzs->stringlen	= 0;
			memset(lzs->table,' ',LZ_TABLE_SIZE);
			lzs->curtabent	= 0xFF0;
		}
		while (lzs->realcurrent<lzs->realwanted) {
			DECOMPRESS_ONE_BYTE;
		}
	}

	while (howmuch) {
		DECOMPRESS_ONE_BYTE;
		lzs->realwanted++;
		*buf++		= b;
		howmuch--;
	}
	return 	toread;
#undef DECOMPRESS_ONE_BYTE
}


/***********************************************************************
 *           LZSeek   (KERNEL32.@)
 */
LONG WINAPI LZSeek( HFILE fd, LONG off, INT type )
{
	struct	lzstate	*lzs;
	LONG	newwanted;

	TRACE("(%d,%d,%d)\n",fd,off,type);
	/* not compressed? just use normal _llseek() */
        if (!(lzs = GET_LZ_STATE(fd))) return _llseek(fd,off,type);
	newwanted = lzs->realwanted;
	switch (type) {
	case 1:	/* SEEK_CUR */
		newwanted      += off;
		break;
	case 2:	/* SEEK_END */
		newwanted	= lzs->reallength-off;
		break;
	default:/* SEEK_SET */
		newwanted	= off;
		break;
	}
	if (newwanted>lzs->reallength)
		return LZERROR_BADVALUE;
	if (newwanted<0)
		return LZERROR_BADVALUE;
	lzs->realwanted	= newwanted;
	return newwanted;
}


/***********************************************************************
 *           LZCopy   (KERNEL32.@)
 *
 * Copies everything from src to dest
 * if src is a LZ compressed file, it will be uncompressed.
 * will return the number of bytes written to dest or errors.
 */
LONG WINAPI LZCopy( HFILE src, HFILE dest )
{
	int	usedlzinit = 0, ret, wret;
	LONG	len;
	HFILE	oldsrc = src, srcfd;
	FILETIME filetime;
	struct	lzstate	*lzs;
#define BUFLEN	1000
	CHAR	buf[BUFLEN];
	/* we need that weird typedef, for i can't seem to get function pointer
	 * casts right. (Or they probably just do not like WINAPI in general)
	 */
	typedef	UINT	(WINAPI *_readfun)(HFILE,LPVOID,UINT);

	_readfun	xread;

	TRACE("(%d,%d)\n",src,dest);
	if (!IS_LZ_HANDLE(src)) {
		src = LZInit(src);
                if ((INT)src <= 0) return 0;
		if (src != oldsrc) usedlzinit=1;
	}

	/* not compressed? just copy */
        if (!IS_LZ_HANDLE(src))
#ifdef __REACTOS__
		xread=(_readfun)_hread; // ROSHACK
#else
		xread=_lread;
#endif
	else
		xread=(_readfun)LZRead;
	len=0;
	while (1) {
		ret=xread(src,buf,BUFLEN);
		if (ret<=0) {
			if (ret==0)
				break;
			if (ret==-1)
				return LZERROR_READ;
			return ret;
		}
		len    += ret;
		wret	= _lwrite(dest,buf,ret);
		if (wret!=ret)
			return LZERROR_WRITE;
	}

	/* Maintain the timestamp of source file to destination file */
	srcfd = (!(lzs = GET_LZ_STATE(src))) ? src : lzs->realfd;
	GetFileTime( LongToHandle(srcfd), NULL, NULL, &filetime );
	SetFileTime( LongToHandle(dest), NULL, NULL, &filetime );

	/* close handle */
	if (usedlzinit)
		LZClose(src);
	return len;
#undef BUFLEN
}

/* reverses GetExpandedPathname */
static LPSTR LZEXPAND_MangleName( LPCSTR fn )
{
    char *p;
    char *mfn = HeapAlloc( GetProcessHeap(), 0, strlen(fn) + 3 ); /* "._" and \0 */
    if(mfn == NULL) return NULL;
    strcpy( mfn, fn );
    if (!(p = strrchr( mfn, '\\' ))) p = mfn;
    if ((p = strchr( p, '.' )))
    {
        p++;
        if (strlen(p) < 3) strcat( p, "_" );  /* append '_' */
        else p[strlen(p)-1] = '_';  /* replace last character */
    }
    else strcat( mfn, "._" );	/* append "._" */
    return mfn;
}


/***********************************************************************
 *           LZOpenFileA   (KERNEL32.@)
 *
 * Opens a file. If not compressed, open it as a normal file.
 */
HFILE WINAPI LZOpenFileA( LPSTR fn, LPOFSTRUCT ofs, WORD mode )
{
	HFILE	fd,cfd;
	BYTE    ofs_cBytes = ofs->cBytes;

	TRACE("(%s,%p,%d)\n",fn,ofs,mode);
	/* 0x70 represents all OF_SHARE_* flags, ignore them for the check */
	fd=OpenFile(fn,ofs,mode);
	if (fd==HFILE_ERROR)
        {
            LPSTR mfn = LZEXPAND_MangleName(fn);
            fd = OpenFile(mfn,ofs,mode);
            HeapFree( GetProcessHeap(), 0, mfn );
	}
	if (fd==HFILE_ERROR)
		ofs->cBytes = ofs_cBytes;
	if ((mode&~0x70)!=OF_READ)
		return fd;
	if (fd==HFILE_ERROR)
		return HFILE_ERROR;
	cfd=LZInit(fd);
	if ((INT)cfd <= 0) return fd;
	return cfd;
}


/***********************************************************************
 *           LZOpenFileW   (KERNEL32.@)
 */
HFILE WINAPI LZOpenFileW( LPWSTR fn, LPOFSTRUCT ofs, WORD mode )
{
    HFILE ret;
    DWORD len = WideCharToMultiByte( CP_ACP, 0, fn, -1, NULL, 0, NULL, NULL );
    LPSTR xfn = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, fn, -1, xfn, len, NULL, NULL );
    ret = LZOpenFileA(xfn,ofs,mode);
    HeapFree( GetProcessHeap(), 0, xfn );
    return ret;
}


/***********************************************************************
 *           LZClose   (KERNEL32.@)
 */
void WINAPI LZClose( HFILE fd )
{
	struct lzstate *lzs;

	TRACE("(%d)\n",fd);
        if (!(lzs = GET_LZ_STATE(fd))) _lclose(fd);
        else
        {
            HeapFree( GetProcessHeap(), 0, lzs->get );
            CloseHandle( LongToHandle(lzs->realfd) );
            lzstates[fd - LZ_MIN_HANDLE] = NULL;
            HeapFree( GetProcessHeap(), 0, lzs );
        }
}

#ifdef __REACTOS__

/*
 * @implemented
 */
VOID
WINAPI
LZCloseFile(IN HFILE FileHandle)
{
    /* One function uses _lclose, the other CloseHandle -- same thing */
    LZClose(FileHandle);
}

/*
 * @unimplemented
 */
ULONG
WINAPI
LZCreateFileW(IN LPCWSTR FileName,
              IN DWORD dwDesiredAccess,
              IN DWORD dwShareMode,
              IN DWORD dwCreationDisposition,
              IN LPWSTR lpString1)
{
    WARN(" LZCreateFileW Not implemented!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#endif /* __REACTOS__ */
