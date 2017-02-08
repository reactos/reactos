/*
 * Copyright 1997 Victor Schneider
 * Copyright 2002 Alexandre Julliard
 * Copyright 2007 Hans Leidekker
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

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <stdio.h>
#include <lzexpand.h>
#include <setupapi.h>

static int myprintf(const char* format, ...)
{
    va_list     va;
    char        tmp[8192];
    DWORD       w = 0;
    int         len;

    va_start(va, format);
    len = vsnprintf(tmp, sizeof(tmp), format, va);
    if (len > 0)
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), tmp, len, &w, NULL);
    va_end(va);
    return w;
}

static UINT CALLBACK set_outfile( PVOID context, UINT notification, UINT_PTR param1, UINT_PTR param2 )
{
    FILE_IN_CABINET_INFO_A *info = (FILE_IN_CABINET_INFO_A *)param1;
    char buffer[MAX_PATH];
    char* basename;

    switch (notification)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        LPSTR outfile = context;
        if (outfile[0] != 0)
        {
            SetLastError( ERROR_NOT_SUPPORTED );
            return FILEOP_ABORT;
        }
        GetFullPathNameA( info->NameInCabinet, sizeof(buffer), buffer, &basename );
        strcpy( outfile, basename );
        return FILEOP_SKIP;
    }
    default: return NO_ERROR;
    }
}

static UINT CALLBACK extract_callback( PVOID context, UINT notification, UINT_PTR param1, UINT_PTR param2 )
{
    FILE_IN_CABINET_INFO_A *info = (FILE_IN_CABINET_INFO_A *)param1;

    switch (notification)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        LPCSTR targetname = context;

        strcpy( info->FullTargetName, targetname );
        return FILEOP_DOIT;
    }
    default: return NO_ERROR;
    }
}

static BOOL option_equal(LPCSTR str1, LPCSTR str2)
{
    if (str1[0] != '/' && str1[0] != '-')
        return FALSE;
    return !lstrcmpA( str1 + 1, str2 );
}

int main(int argc, char *argv[])
{
    int ret = 0;
    char infile[MAX_PATH], outfile[MAX_PATH], actual_name[MAX_PATH];
    char outfile_basename[MAX_PATH], *basename_index;
    UINT comp;

    if (argc < 3)
    {
        myprintf( "Usage:\n" );
        myprintf( "\t%s infile outfile\n", argv[0] );
        myprintf( "\t%s /r infile\n", argv[0] );
        return 1;
    }

    if (argc == 3 && (option_equal(argv[1], "R") || option_equal(argv[1], "r")))
        GetFullPathNameA( argv[2], sizeof(infile), infile, NULL );
    else
        GetFullPathNameA( argv[1], sizeof(infile), infile, NULL );

    if (!SetupGetFileCompressionInfoExA( infile, actual_name, sizeof(actual_name), NULL, NULL, NULL, &comp ))
    {
        myprintf( "%s: can't open input file %s\n", argv[0], infile );
        return 1;
    }

    if (argc == 3 && (option_equal(argv[1], "R") || option_equal(argv[1], "r")))
    {
        switch (comp)
        {
        case FILE_COMPRESSION_MSZIP:
            outfile_basename[0] = 0;
            if (!SetupIterateCabinetA( infile, 0, set_outfile, outfile_basename ))
            {
                myprintf( "%s: can't determine original name\n", argv[0] );
                return 1;
            }
            GetFullPathNameA( infile, sizeof(outfile), outfile, &basename_index );
            *basename_index = 0;
            strcat( outfile, outfile_basename );
            break;
        case FILE_COMPRESSION_WINLZA:
            GetExpandedNameA( infile, outfile_basename );
            break;
        default:
            myprintf( "%s: can't determine original\n", argv[0] );
            return 1;
        }
    }
    else
        GetFullPathNameA( argv[2], sizeof(outfile), outfile, NULL );

    if (!lstrcmpiA( infile, outfile ))
    {
        myprintf( "%s: can't expand file to itself\n", argv[0] );
        return 1;
    }

    switch (comp)
    {
    case FILE_COMPRESSION_MSZIP:
        if (!SetupIterateCabinetA( infile, 0, extract_callback, outfile ))
        {
            myprintf( "%s: cabinet extraction failed\n", argv[0] );
            return 1;
        }
        break;
    case FILE_COMPRESSION_WINLZA:
    {
        INT hin, hout;
        OFSTRUCT ofin, ofout;
        LONG error;

        if ((hin = LZOpenFileA( infile, &ofin, OF_READ )) < 0)
        {
            myprintf( "%s: can't open input file %s\n", argv[0], infile );
            return 1;
        }
        if ((hout = LZOpenFileA( outfile, &ofout, OF_CREATE | OF_WRITE )) < 0)
        {
            LZClose( hin );
            myprintf( "%s: can't open output file %s\n", argv[0], outfile );
            return 1;
        }
        error = LZCopy( hin, hout );

        LZClose( hin );
        LZClose( hout );

        if (error < 0)
        {
            myprintf( "%s: LZCopy failed, error is %d\n", argv[0], error );
            return 1;
        }
        break;
    }
    default:
        if (!CopyFileA( infile, outfile, FALSE ))
        {
            myprintf( "%s: CopyFileA failed\n", argv[0] );
            return 1;
        }
        break;
    }
    return ret;
}
