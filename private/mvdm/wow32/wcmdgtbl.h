/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1993, Microsoft Corporation
 *
 *  WCMDGTBL.h
 *  WOW32 16-bit Commdlg tables
 *
 *  History:
 *      John Vert (jvert) 31-Dec-1992 - created
 *
 *  This file is included into the master thunk table.
 *
--*/

    {W32FUN(UNIMPLEMENTEDAPI,              "DUMMYENTRY",           MOD_COMMDLG,    0)},
    {W32FUN(WCD32GetOpenFileName,          "GETOPENFILENAME",      MOD_COMMDLG,    sizeof(GETOPENFILENAME16))},
    {W32FUN(WCD32GetSaveFileName,          "GETSAVEFILENAME",      MOD_COMMDLG,    sizeof(GETSAVEFILENAME16))},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(WCD32ChooseColor,              "CHOOSECOLOR",          MOD_COMMDLG,    sizeof(CHOOSECOLOR16))},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},

  /*** 0010 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(WCD32FindText,                 "FINDTEXT",             MOD_COMMDLG,    sizeof(FINDREPLACE16))},
    {W32FUN(WCD32ReplaceText,              "REPLACETEXT",          MOD_COMMDLG,    sizeof(FINDREPLACE16))},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(WCD32ChooseFont,               "CHOOSEFONT",           MOD_COMMDLG,    sizeof(CHOOSEFONT16))},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},

  /*** 0020 ***/
    {W32FUN(WCD32PrintDlg,                 "PRINTDLG",             MOD_COMMDLG,    sizeof(PRINTDLG16))},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_COMMDLG,    0)},
    {W32FUN(WCD32ExtendedError,            "COMMDLGEXTENDEDERROR", MOD_COMMDLG,    0)},

