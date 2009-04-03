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
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "lzexpand.h"

#include "wine/winbase16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define MAX_LZSTATES 16

#define IS_LZ_HANDLE(h) (((h) >= 0x400) && ((h) < 0x400+MAX_LZSTATES))


/***********************************************************************
 *           LZStart   (LZEXPAND.7)
 */
INT16 WINAPI LZStart16(void)
{
    TRACE("(void)\n");
    return 1;
}


/***********************************************************************
 *           LZInit   (LZEXPAND.3)
 */
HFILE16 WINAPI LZInit16( HFILE16 hfSrc )
{
    HFILE ret = LZInit( (HFILE)DosFileHandleToWin32Handle(hfSrc) );
    if (IS_LZ_HANDLE(ret)) return ret;
    if ((INT)ret <= 0) return ret;
    return hfSrc;
}


/***********************************************************************
 *           GetExpandedName   (LZEXPAND.10)
 */
INT16 WINAPI GetExpandedName16( LPSTR in, LPSTR out )
{
    return (INT16)GetExpandedNameA( in, out );
}


/***********************************************************************
 *           LZRead   (LZEXPAND.5)
 */
INT16 WINAPI LZRead16( HFILE16 fd, LPVOID buf, UINT16 toread )
{
    if (IS_LZ_HANDLE(fd)) return LZRead( fd, buf, toread );
    return _lread( (HFILE)DosFileHandleToWin32Handle(fd), buf, toread );
}


/***********************************************************************
 *           LZSeek   (LZEXPAND.4)
 */
LONG WINAPI LZSeek16( HFILE16 fd, LONG off, INT16 type )
{
    if (IS_LZ_HANDLE(fd)) return LZSeek( fd, off, type );
    return _llseek( (HFILE)DosFileHandleToWin32Handle(fd), off, type );
}


/***********************************************************************
 *           LZCopy   (LZEXPAND.1)
 *
 */
LONG WINAPI LZCopy16( HFILE16 src, HFILE16 dest )
{
    /* already a LZ handle? */
    if (IS_LZ_HANDLE(src)) return LZCopy( src, (HFILE)DosFileHandleToWin32Handle(dest) );

    /* no, try to open one */
    src = LZInit16(src);
    if ((INT16)src <= 0) return 0;
    if (IS_LZ_HANDLE(src))
    {
        LONG ret = LZCopy( src, (HFILE)DosFileHandleToWin32Handle(dest) );
        LZClose( src );
        return ret;
    }
    /* it was not a compressed file */
    return LZCopy( (HFILE)DosFileHandleToWin32Handle(src), (HFILE)DosFileHandleToWin32Handle(dest) );
}


/***********************************************************************
 *           LZOpenFile   (LZEXPAND.2)
 */
HFILE16 WINAPI LZOpenFile16( LPSTR fn, LPOFSTRUCT ofs, UINT16 mode )
{
    HFILE hfret = LZOpenFileA( fn, ofs, mode );
    /* return errors and LZ handles unmodified */
    if ((INT)hfret <= 0) return hfret;
    if (IS_LZ_HANDLE(hfret)) return hfret;
    /* but allocate a dos handle for 'normal' files */
    return Win32HandleToDosFileHandle((HANDLE)hfret);
}


/***********************************************************************
 *           LZClose   (LZEXPAND.6)
 */
void WINAPI LZClose16( HFILE16 fd )
{
    if (IS_LZ_HANDLE(fd)) LZClose( fd );
    else DisposeLZ32Handle( DosFileHandleToWin32Handle((HFILE)fd) );
}


/***********************************************************************
 *           CopyLZFile   (LZEXPAND.8)
 */
LONG WINAPI CopyLZFile16( HFILE16 src, HFILE16 dest )
{
    TRACE("(%d,%d)\n",src,dest);
    return LZCopy16(src,dest);
}


/***********************************************************************
 *           LZDone   (LZEXPAND.9)
 */
void WINAPI LZDone16(void)
{
    TRACE("(void)\n");
}
