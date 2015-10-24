/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _splitpath
 * PROGRAMMER:      Timo Kreuzer
 */

#include <apitest.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdarg.h>

START_TEST(splitpath)
{
    char drive[5];
    char dir[64];
    char fname[32];
    char ext[10];
    DWORD Major;

    Major = (DWORD)(LOBYTE(LOWORD(GetVersion())));

    drive[2] = 0xFF;
    _splitpath("c:\\dir1\\dir2\\file.ext", drive, dir, fname, ext);
    ok_str(drive, "c:");
    ok_str(dir, "\\dir1\\dir2\\");
    ok_str(fname, "file");
    ok_str(ext, ".ext");
    ok_int(drive[2], 0);

    *_errno() = 0;
    _splitpath("c:\\dir1\\dir2\\file.ext", 0, 0, 0, 0);
    ok_int(*_errno(), 0);

    if (Major >= 6)
    {
        *_errno() = 0;
        _splitpath(0, drive, dir, fname, ext);
        ok_int(*_errno(), EINVAL);
        ok_str(drive, "");
        ok_str(dir, "");
        ok_str(fname, "");
        ok_str(ext, "");
    }
    else
    {
        win_skip("This test only succeed on NT6+\n");
    }

    _splitpath("\\\\?\\c:\\dir1\\dir2\\file.ext", drive, dir, fname, ext);
    if (Major >= 6)
    {
        ok_str(drive, "c:");
        ok_str(dir, "\\dir1\\dir2\\");
    }
    else
    {
        ok_str(drive, "");
        ok_str(dir, "\\\\?\\c:\\dir1\\dir2\\");
    }
    ok_str(fname, "file");
    ok_str(ext, ".ext");

    _splitpath("ab:\\dir1\\..\\file", drive, dir, fname, ext);
    ok_str(drive, "");
    ok_str(dir, "ab:\\dir1\\..\\");
    ok_str(fname, "file");
    ok_str(ext, "");

    _splitpath("//?/c:/dir1/dir2/file.ext", drive, dir, fname, ext);
    ok_str(drive, "");
    ok_str(dir, "//?/c:/dir1/dir2/");
    ok_str(fname, "file");
    ok_str(ext, ".ext");

    _splitpath("\\\\?\\0:/dir1\\dir2/file.", drive, dir, fname, ext);
    if (Major >= 6)
    {
        ok_str(drive, "0:");
        ok_str(dir, "/dir1\\dir2/");
    }
    else
    {
        ok_str(drive, "");
        ok_str(dir, "\\\\?\\0:/dir1\\dir2/");
    }
    ok_str(fname, "file");
    ok_str(ext, ".");

    _splitpath("\\\\.\\c:\\dir1\\dir2\\.ext.ext2", drive, dir, fname, ext);
    ok_str(drive, "");
    ok_str(dir, "\\\\.\\c:\\dir1\\dir2\\");
    ok_str(fname, ".ext");
    ok_str(ext, ".ext2");

    _splitpath("\\??\\c:\\dir1\\dir2\\file. ~ ", drive, dir, fname, ext);
    ok_str(drive, "");
    ok_str(dir, "\\??\\c:\\dir1\\dir2\\");
    ok_str(fname, "file");
    ok_str(ext, ". ~ ");

    _splitpath("x: dir1\\/dir2 \\.blub", drive, dir, fname, ext);
    ok_str(drive, "x:");
    ok_str(dir, " dir1\\/dir2 \\");
    ok_str(fname, "");
    ok_str(ext, ".blub");

    _splitpath("/:\\dir1\\dir2\\file.ext", drive, dir, fname, ext);
    ok_str(drive, "/:");
    ok_str(dir, "\\dir1\\dir2\\");
    ok_str(fname, "file");
    ok_str(ext, ".ext");

}

