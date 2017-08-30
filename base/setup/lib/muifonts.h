#pragma once

MUI_SUBFONT LatinFonts[] =
{
    /*Font                       Substitute  */
    { L"Arial",                  L"Liberation Sans" },
    { L"Courier",                L"FreeMono" },
    { L"Courier New",            L"FreeMono" },
    { L"Fixedsys",               L"Fixedsys Excelsior 3.01-L2" },
    { L"Franklin Gothic Medium", L"Libre Franklin Bold" },
    { L"Helv",                   L"Tahoma" },
    { L"Helvetica",              L"Liberation Sans" },
    { L"Lucida Console",         L"DejaVu Sans Mono" },
    { L"MS Sans Serif",          L"Tahoma" },
    { L"MS Shell Dlg",           L"Tahoma" },
    { L"MS Shell Dlg 2",         L"Tahoma" },
    { L"Tahoma",                 L"Tahoma" },
    { L"Terminal",               L"DejaVu Sans Mono" },
    { L"Times New Roman",        L"Liberation Serif" },
    { L"Trebuchet MS",           L"Open Sans" },
    { L"System",                 L"FreeSans" },
    { NULL, NULL }
};

MUI_SUBFONT CyrillicFonts[] =
{
    { L"Arial",                  L"Liberation Sans" },
    { L"Courier",                L"FreeMono" },
    { L"Courier New",            L"FreeMono" },
    { L"Fixedsys",               L"Fixedsys Excelsior 3.01-L2" },
    { L"Franklin Gothic Medium", L"Libre Franklin" },
    { L"Helv",                   L"Tahoma" },
    { L"Helvetica",              L"Liberation Sans" },
    { L"Lucida Console",         L"DejaVu Sans Mono" },
    { L"MS Sans Serif",          L"Tahoma" },
    { L"MS Shell Dlg",           L"Tahoma" },
    { L"MS Shell Dlg 2",         L"Tahoma" },
    { L"Tahoma",                 L"Tahoma" },
    { L"Terminal",               L"DejaVu Sans Mono" },
    { L"Times New Roman",        L"Liberation Serif" },
    { L"Trebuchet MS",           L"Open Sans" },
    { L"System",                 L"FreeSans" },
    { NULL, NULL }
};

MUI_SUBFONT GreekFonts[] =
{
    { L"Arial",                  L"Liberation Sans" },
    { L"Courier",                L"FreeMono" },
    { L"Courier New",            L"FreeMono" },
    { L"Fixedsys",               L"Fixedsys Excelsior 3.01-L2" },
    { L"Franklin Gothic Medium", L"Libre Franklin" },
    { L"Helv",                   L"DejaVu Sans" },
    { L"Helvetica",              L"Liberation Sans" },
    { L"Lucida Console",         L"DejaVu Sans Mono" },
    { L"MS Sans Serif",          L"DejaVu Sans" },
    { L"MS Shell Dlg",           L"DejaVu Sans" },
    { L"MS Shell Dlg 2",         L"DejaVu Sans" },
    { L"Tahoma",                 L"DejaVu Sans" },
    { L"Terminal",               L"DejaVu Sans Mono" },
    { L"Times New Roman",        L"Liberation Serif" },
    { L"Trebuchet MS",           L"Open Sans" },
    { L"System",                 L"FreeSans" },
    { NULL, NULL }
};

MUI_SUBFONT HebrewFonts[] =
{
    { L"Arial",                  L"DejaVu Sans" },
    { L"Courier",                L"FreeMono" },
    { L"Courier New",            L"FreeMono" },
    { L"Fixedsys",               L"Fixedsys Excelsior 3.01-L2" },
    { L"Franklin Gothic Medium", L"Libre Franklin" },
    { L"Helv",                   L"DejaVu Sans" },
    { L"Helvetica",              L"DejaVu Sans" },
    { L"Lucida Console",         L"DejaVu Sans Mono" },
    { L"MS Sans Serif",          L"DejaVu Sans" },
    { L"MS Shell Dlg",           L"DejaVu Sans" },
    { L"MS Shell Dlg 2",         L"DejaVu Sans" },
    { L"Tahoma",                 L"DejaVu Sans" },
    { L"Terminal",               L"DejaVu Sans Mono" },
    { L"Times New Roman",        L"DejaVu Serif" },
    { L"Trebuchet MS",           L"Open Sans" },
    { L"System",                 L"FreeSans" },
    { NULL, NULL }
};

WCHAR CSF_LocalName0[] = {0x5B8B, 0x4F53, 0};
WCHAR CSF_LocalName1[] = {0x65B0, 0x5B8B, 0x4F53, 0};
WCHAR CSF_LocalName2[] = {0x4E2D, 0x6613, 0x5B8B, 0x4F53, 0};
WCHAR CSF_LocalName3[] = {'M', 'S', 0x5B8B, 0x4F53, 0};
MUI_SUBFONT ChineseSimplifiedFonts[] =
{
    { L"Arial",                  L"Liberation Sans" },
    { L"Courier",                L"FreeMono" },
    { L"Courier New",            L"FreeMono" },
    { L"Fixedsys",               L"Fixedsys Excelsior 3.01-L2" },
    { L"Franklin Gothic Medium", L"Libre Franklin" },
    { L"Helv",                   L"Droid Sans Fallback" },
    { L"Helvetica",              L"Liberation Sans" },
    { L"Lucida Console",         L"DejaVu Sans Mono" },
    { L"MS Sans Serif",          L"Droid Sans Fallback" },
    { L"MS Shell Dlg",           L"Droid Sans Fallback" },
    { L"MS Shell Dlg 2",         L"Droid Sans Fallback" },
    { L"MS UI Gothic",           L"Droid Sans Fallback" },
    { L"MS UI Gothic 2",         L"Droid Sans Fallback" },
    { L"Tahoma",                 L"Droid Sans Fallback" },
    { L"Terminal",               L"DejaVu Sans Mono" },
    { L"Times New Roman",        L"Liberation Serif" },
    { L"Trebuchet MS",           L"Open Sans" },
    { L"SimSun",                 L"Droid Sans Fallback" },
    { L"NSimSun",                L"Droid Sans Fallback" },
    { L"MS Song",                L"Droid Sans Fallback" },
    { L"System",                 L"Droid Sans Fallback" },
    /* localized names */
    { CSF_LocalName0,            L"Droid Sans Fallback" },
    { CSF_LocalName1,            L"Droid Sans Fallback" },
    { CSF_LocalName2,            L"Droid Sans Fallback" },
    { CSF_LocalName3,            L"Droid Sans Fallback" },
    { NULL, NULL }
};

WCHAR CTF_LocalName0[] = {0x7D30, 0x660E, 0x9AD4, 0};
WCHAR CTF_LocalName1[] = {0x65B0, 0x7D30, 0x660E, 0x9AD4, 0};
WCHAR CTF_LocalName2[] = {0x83EF, 0x5EB7, 0x4E2D, 0x660E, 0x9AD4, 0};
WCHAR CTF_LocalName3[] = {0x83EF, 0x5EB7, 0x7C97, 0x660E, 0x9AD4, 0};
MUI_SUBFONT ChineseTraditionalFonts[] =
{
    { L"Arial",           L"Liberation Sans" },
    { L"Courier",         L"FreeMono" },
    { L"Courier New",     L"FreeMono" },
    { L"Fixedsys",        L"Fixedsys Excelsior 3.01-L2" },
    { L"Helv",            L"Droid Sans Fallback" },
    { L"Helvetica",       L"Liberation Sans" },
    { L"Lucida Console",  L"DejaVu Sans Mono" },
    { L"MS Sans Serif",   L"Droid Sans Fallback" },
    { L"MS Shell Dlg",    L"Droid Sans Fallback" },
    { L"MS Shell Dlg 2",  L"Droid Sans Fallback" },
    { L"MS UI Gothic",    L"Droid Sans Fallback" },
    { L"MS UI Gothic 2",  L"Droid Sans Fallback" },
    { L"Tahoma",          L"Droid Sans Fallback" },
    { L"Terminal",        L"DejaVu Sans Mono" },
    { L"Times New Roman", L"Liberation Serif" },
    { L"Ming Light",      L"Droid Sans Fallback" },
    { L"MingLiU",         L"Droid Sans Fallback" },
    { L"PMingLiU",        L"Droid Sans Fallback" },
    { L"DLCMingMedium",   L"Droid Sans Fallback" },
    { L"DLCMingBold",     L"Droid Sans Fallback" },
    { L"System",          L"Droid Sans Fallback" },
    /* localized names */
    { CTF_LocalName0,     L"Droid Sans Fallback" },
    { CTF_LocalName1,     L"Droid Sans Fallback" },
    { CTF_LocalName2,     L"Droid Sans Fallback" },
    { CTF_LocalName3,     L"Droid Sans Fallback" },
    { NULL, NULL }
};

WCHAR JF_LocalName0[] = {0xFF2D, 0xFF33, ' ', 0x660E, 0x671D, 0};
WCHAR JF_LocalName1[] = {0xFF2D, 0xFF33, ' ', 0xFF30, 0x660E, 0x671D, 0};
WCHAR JF_LocalName2[] = {0xFF2D, 0xFF33, ' ', 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0};
WCHAR JF_LocalName3[] = {0xFF2D, 0xFF33, ' ', 0xFF30, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0};
MUI_SUBFONT JapaneseFonts[] =
{
    { L"Arial",           L"Liberation Sans" },
    { L"Courier",         L"FreeMono" },
    { L"Courier New",     L"FreeMono" },
    { L"Fixedsys",        L"Fixedsys Excelsior 3.01-L2" },
    { L"Helv",            L"Droid Sans Fallback" },
    { L"Helvetica",       L"Liberation Sans" },
    { L"Lucida Console",  L"DejaVu Sans Mono" },
    { L"MS Sans Serif",   L"Droid Sans Fallback" },
    { L"MS Shell Dlg",    L"Droid Sans Fallback" },
    { L"MS Shell Dlg 2",  L"Droid Sans Fallback" },
    { L"MS UI Gothic",    L"Droid Sans Fallback" },
    { L"MS UI Gothic 2",  L"Droid Sans Fallback" },
    { L"Tahoma",          L"Droid Sans Fallback" },
    { L"Terminal",        L"DejaVu Sans Mono" },
    { L"Times New Roman", L"Liberation Serif" },
    { L"MS Mincho",       L"Droid Sans Fallback" },
    { L"MS PMincho",      L"Droid Sans Fallback" },
    { L"MS Gothic",       L"Droid Sans Fallback" },
    { L"MS PGothic",      L"Droid Sans Fallback" },
    { L"System",          L"Droid Sans Fallback" },
    /* localized names */
    { JF_LocalName0,      L"Droid Sans Fallback" },
    { JF_LocalName1,      L"Droid Sans Fallback" },
    { JF_LocalName2,      L"Droid Sans Fallback" },
    { JF_LocalName3,      L"Droid Sans Fallback" },
    { NULL, NULL }
};

WCHAR KF_LocalName0[] = {0xBC14, 0xD0D5, 0};
WCHAR KF_LocalName1[] = {0xBC14, 0xD0D5, 0xCCB4, 0};
WCHAR KF_LocalName2[] = {0xAD81, 0xC11C, 0};
WCHAR KF_LocalName3[] = {0xAD81, 0xC11C, 0xCCB4, 0};
WCHAR KF_LocalName4[] = {0xAD74, 0xB9BC, 0};
WCHAR KF_LocalName5[] = {0xAD74, 0xB9BC, 0xCCB4, 0};
MUI_SUBFONT KoreanFonts[] =
{
    { L"Arial",           L"Liberation Sans" },
    { L"Courier",         L"FreeMono" },
    { L"Courier New",     L"FreeMono" },
    { L"Fixedsys",        L"Fixedsys Excelsior 3.01-L2" },
    { L"Helv",            L"Droid Sans Fallback" },
    { L"Helvetica",       L"Liberation Sans" },
    { L"Lucida Console",  L"DejaVu Sans Mono" },
    { L"MS Sans Serif",   L"Droid Sans Fallback" },
    { L"MS Shell Dlg",    L"Droid Sans Fallback" },
    { L"MS Shell Dlg 2",  L"Droid Sans Fallback" },
    { L"MS UI Gothic",    L"Droid Sans Fallback" },
    { L"MS UI Gothic 2",  L"Droid Sans Fallback" },
    { L"Tahoma",          L"Droid Sans Fallback" },
    { L"Terminal",        L"DejaVu Sans Mono" },
    { L"Times New Roman", L"Liberation Serif" },
    { L"Batang",          L"Droid Sans Fallback" },
    { L"BatangChe",       L"Droid Sans Fallback" },
    { L"Gungsuh",         L"Droid Sans Fallback" },
    { L"GungsuhChe",      L"Droid Sans Fallback" },
    { L"Gulim",           L"Droid Sans Fallback" },
    { L"GulimChe",        L"Droid Sans Fallback" },
    { L"System",          L"Droid Sans Fallback" },
    /* localized names */
    { KF_LocalName0,      L"Droid Sans Fallback" },
    { KF_LocalName1,      L"Droid Sans Fallback" },
    { KF_LocalName2,      L"Droid Sans Fallback" },
    { KF_LocalName3,      L"Droid Sans Fallback" },
    { KF_LocalName4,      L"Droid Sans Fallback" },
    { KF_LocalName5,      L"Droid Sans Fallback" },
    { NULL, NULL }
};

MUI_SUBFONT UnicodeFonts[] =
{
    { L"Arial",                  L"DejaVu Sans" },
    { L"Courier",                L"DejaVu Sans Mono" },
    { L"Courier New",            L"DejaVu Sans Mono" },
    { L"Fixedsys",               L"Fixedsys Excelsior 3.01-L2" },
    { L"Franklin Gothic Medium", L"Libre Franklin Bold" },
    { L"Helv",                   L"DejaVu Sans" },
    { L"Helvetica",              L"DejaVu Sans" },
    { L"Lucida Console",         L"DejaVu Sans Mono" },
    { L"MS Sans Serif",          L"DejaVu Sans" },
    { L"MS Shell Dlg",           L"DejaVu Sans" },
    { L"MS Shell Dlg 2",         L"DejaVu Sans" },
    { L"Tahoma",                 L"DejaVu Sans" },
    { L"Terminal",               L"DejaVu Sans Mono" },
    { L"Times New Roman",        L"DejaVu Serif" },
    { L"Trebuchet MS",           L"Open Sans" },
    { L"System",                 L"FreeSans" },
    { NULL, NULL }
};
