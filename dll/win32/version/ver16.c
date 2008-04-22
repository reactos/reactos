/*
 * Implementation of VER.DLL
 *
 * Copyright 1999 Ulrich Weigand
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
#include "wine/winbase16.h"
#include "winver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);


/*************************************************************************
 * GetFileVersionInfoSize                  [VER.6]
 */
DWORD WINAPI GetFileVersionInfoSize16( LPCSTR lpszFileName, LPDWORD lpdwHandle )
{
    TRACE("(%s, %p)\n", debugstr_a(lpszFileName), lpdwHandle );
    return GetFileVersionInfoSizeA( lpszFileName, lpdwHandle );
}

/*************************************************************************
 * GetFileVersionInfo                      [VER.7]
 */
DWORD WINAPI GetFileVersionInfo16( LPCSTR lpszFileName, DWORD handle,
                                   DWORD cbBuf, LPVOID lpvData )
{
    TRACE("(%s, %08x, %d, %p)\n",
                debugstr_a(lpszFileName), handle, cbBuf, lpvData );

    return GetFileVersionInfoA( lpszFileName, handle, cbBuf, lpvData );
}

/*************************************************************************
 * VerFindFile                             [VER.8]
 */
DWORD WINAPI VerFindFile16( UINT16 flags, LPSTR lpszFilename,
                            LPSTR lpszWinDir, LPSTR lpszAppDir,
                            LPSTR lpszCurDir, UINT16 *lpuCurDirLen,
                            LPSTR lpszDestDir, UINT16 *lpuDestDirLen )
{
    UINT curDirLen, destDirLen;
    DWORD retv = VerFindFileA( flags, lpszFilename, lpszWinDir, lpszAppDir,
                                 lpszCurDir, &curDirLen, lpszDestDir, &destDirLen );

    *lpuCurDirLen = (UINT16)curDirLen;
    *lpuDestDirLen = (UINT16)destDirLen;
    return retv;
}

/*************************************************************************
 * VerInstallFile                          [VER.9]
 */
DWORD WINAPI VerInstallFile16( UINT16 flags,
                               LPSTR lpszSrcFilename, LPSTR lpszDestFilename,
                               LPSTR lpszSrcDir, LPSTR lpszDestDir, LPSTR lpszCurDir,
                               LPSTR lpszTmpFile, UINT16 *lpwTmpFileLen )
{
    UINT filelen;
    DWORD retv = VerInstallFileA( flags, lpszSrcFilename, lpszDestFilename,
                                    lpszSrcDir, lpszDestDir, lpszCurDir,
                                    lpszTmpFile, &filelen);

    *lpwTmpFileLen = (UINT16)filelen;
    return retv;
}

/*************************************************************************
 * VerLanguageName                        [VER.10]
 */
DWORD WINAPI VerLanguageName16( UINT16 uLang, LPSTR lpszLang, UINT16 cbLang )
{
    return VerLanguageNameA( uLang, lpszLang, cbLang );
}

/*************************************************************************
 * VerQueryValue                          [VER.11]
 */
DWORD WINAPI VerQueryValue16( SEGPTR spvBlock, LPSTR lpszSubBlock,
                              SEGPTR *lpspBuffer, UINT16 *lpcb )
{
    LPVOID lpvBlock = MapSL( spvBlock );
    LPVOID buffer = lpvBlock;
    UINT buflen;
    DWORD retv;

    TRACE("(%p, %s, %p, %p)\n",
                lpvBlock, debugstr_a(lpszSubBlock), lpspBuffer, lpcb );

    retv = VerQueryValueA( lpvBlock, lpszSubBlock, &buffer, &buflen );
    if ( !retv ) return FALSE;

    if ( OFFSETOF( spvBlock ) + ((char *) buffer - (char *) lpvBlock) >= 0x10000 )
    {
        FIXME("offset %08X too large relative to %04X:%04X\n",
               (char *) buffer - (char *) lpvBlock, SELECTOROF( spvBlock ), OFFSETOF( spvBlock ) );
        return FALSE;
    }

    if (lpcb) *lpcb = buflen;
    *lpspBuffer = (SEGPTR) ((char *) spvBlock + ((char *) buffer - (char *) lpvBlock));

    return retv;
}
