#pragma once

MUI_LAYOUTS ruRULayouts[] =
{
    { L"0419", L"00000419" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY ruRULanguagePageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‚ë¡®à ï§ëª ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ®¦ «ã©áâ , ¢ë¡¥à¨â¥ ï§ëª, ¨á¯®«ì§ã¥¬ë© ¯à¨ ãáâ ­®¢ª¥.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ‡ â¥¬ ­ ¦¬¨â¥ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  â®â ï§ëª ¡ã¤¥â ¨á¯®«ì§®¢ âìáï ¯® ã¬®«ç ­¨î ¢ ãáâ ­®¢«¥­­®© á¨áâ¥¬¥.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì  F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUWelcomePageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "„®¡à® ¯®¦ «®¢ âì ¢ ¯à®£à ¬¬ã ãáâ ­®¢ª¨ ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "  íâ®© áâ ¤¨¨ ¡ã¤ãâ áª®¯¨à®¢ ­ë ä ©«ë ®¯¥à æ¨®­­®© á¨áâ¥¬ë ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "­  ‚ è ª®¬¯ìîâ¥à ¨ ¯®¤£®â®¢«¥­  ¢â®à ï áâ ¤¨ï ãáâ ­®¢ª¨.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07   ¦¬¨â¥ ENTER ¤«ï ãáâ ­®¢ª¨ ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07   ¦¬¨â¥ R ¤«ï ¢®ááâ ­®¢«¥­¨ï ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07   ¦¬¨â¥ L ¤«ï ¯à®á¬®âà  «¨æ¥­§¨®­­®£® á®£« è¥­¨ï ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07   ¦¬¨â¥ F3 ¤«ï ¢ëå®¤  ¨§ ¯à®£à ¬¬ë ãáâ ­®¢ª¨ ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "„«ï ¤®¯®«­¨â¥«ì­®© ¨­ä®à¬ æ¨¨ ® ReactOS ¯®á¥â¨â¥:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.ru",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì  R = ‚®ááâ ­®¢«¥­¨¥  L = ‹¨æ. á®£« è¥­¨¥  F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUIntroPageEntries[] =
{
    {
        4,
        3,
        " Установка " KERNEL_VERSION_STR " ReactOS ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Состояние версии ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ReactOS находится в Альфа стадии,некоторая функциональность может отсутствовать",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "и в активной разработке. Рекомендуется использовать только для",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "оценики и тестирования,а не как ОС для ежедневного использования.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "Сохраните Ваши данные или протестируете на другом компьютере,если Вы пытаетесь",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "запустить ReactOS на реальном устройстве.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Нажмите ENTER чтобы продолжить утсановку ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Нажмите F3 чтобы выйти без установки ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Продолжить   F3 = Выход",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRULicensePageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "‹¨æ¥­§¨ï:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS «¨æ¥­§¨à®¢ ­  ¢ á®®â¢¥âáâ¢¨¨ á Žâªàëâë¬ «¨æ¥­§¨®­­ë¬",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "á®£« è¥­¨¥¬ GNU GPL ¨ á®¤¥à¦¨â ª®¬¯®­¥­âë, à á¯à®áâà ­ï¥¬ë¥",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "á á®¢¬¥áâ¨¬ë¬¨ «¨æ¥­§¨ï¬¨: X11, BSD ¨ GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "‚á¥ ¯à®£à ¬¬­®¥ ®¡¥á¯¥ç¥­¨¥ ¢å®¤ïé¥¥ ¢ á¨áâ¥¬ã ReactOS ¢ë¯ãé¥­®",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "¯®¤ Žâªàëâë¬ «¨æ¥­§¨®­­ë¬ á®£« è¥­¨¥¬ GNU GPL á á®åà ­¥­¨¥¬",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "¯¥à¢®­ ç «ì­®© «¨æ¥­§¨¨.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "„ ­­®¥ ¯à®£à ¬¬­®¥ ®¡¥á¯¥ç¥­¨¥ ¯®áâ ¢«ï¥âáï …‡ ƒ€€’ˆˆ ¨ ¡¥§",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "®£à ­¨ç¥­¨© ¢ ¨á¯®«ì§®¢ ­¨¨, ª ª ¢ ¬¥áâ­®¬, â ª ¨ ¬¥¦¤ã­ à®¤­®¬ ¯à ¢¥.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "‹¨æ¥­§¨ï ReactOS à §à¥è ¥â ¯¥à¥¤ çã ¯à®¤ãªâ  âà¥âì¨¬ «¨æ ¬.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "…á«¨ ¯® ª ª®¬-«¨¡® ¯à¨ç¨­ ¬ ¢ë ­¥ ¯®«ãç¨«¨ ª®¯¨î Žâªàëâ®£®",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "«¨æ¥­§¨®­­®£® á®£« è¥­¨ï GNU ¢¬¥áâ¥ á ReactOS, ¯®á¥â¨â¥",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        22,
        "ƒ à ­â¨¨:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "â® á¢®¡®¤­®¥ ¯à®£à ¬¬­®¥ ®¡¥á¯¥ç¥­¨¥; á¬. ¨áâ®ç­¨ª ¤«ï ¯à®á¬®âà  ¯à ¢.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "…’ ˆŠ€Šˆ• ƒ€€’ˆ‰; ­¥â £ à ­â¨¨ ’Ž‚€ŽƒŽ ‘Ž‘’ŽŸˆŸ ¨«¨",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "ˆƒŽ„Ž‘’ˆ „‹Ÿ ŠŽŠ…’›• –…‹…‰",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ‚®§¢à â",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUDevicePageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‚ á¯¨áª¥ ­¨¦¥ ¯à¨¢¥¤¥­ë ãáâà®©áâ¢  ¨ ¨å ¯ à ¬¥âàë.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Š®¬¯ìîâ¥à:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "ªà ­:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Š« ¢¨ âãà :",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        " áª« ¤ª :",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "à¨¬¥­¨âì:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "à¨¬¥­¨âì ¤ ­­ë¥ ¯ à ¬¥âàë ãáâà®©áâ¢",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "‚ë ¬®¦¥â¥ ¨§¬¥­¨âì ¯ à ¬¥âàë ãáâà®©áâ¢, ­ ¦¨¬ ï ª« ¢¨è¨ ‚‚…• ¨ ‚ˆ‡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "¤«ï ¢ë¤¥«¥­¨ï í«¥¬¥­â , ¨ ª« ¢¨èã ENTER ¤«ï ¢ë¡®à  ¤àã£¨å ¢ à¨ ­â®¢",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "¯ à ¬¥âà®¢.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Š®£¤  ¢á¥ ¯ à ¬¥âàë ®¯à¥¤¥«¥­ë, ¢ë¡¥à¨â¥ \"à¨¬¥­¨âì ¤ ­­ë¥ ¯ à ¬¥âàë",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "ãáâà®©áâ¢\" ¨ ­ ¦¬¨â¥ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRURepairPageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS ­ å®¤¨âáï ¢ à ­­¥© áâ ¤¨¨ à §à ¡®âª¨ ¨ ­¥ ¯®¤¤¥à¦¨¢ ¥â ¢á¥",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "äã­ªæ¨¨ ¤«ï ¯®«­®© á®¢¬¥áâ¨¬®áâ¨ á ãáâ ­ ¢«¨¢ ¥¬ë¬¨ ¯à¨«®¦¥­¨ï¬¨.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "”ã­ªæ¨ï ¢®ááâ ­®¢«¥­¨ï ¢ ¤ ­­ë¬ ¬®¬¥­â ®âáãâáâ¢ã¥â.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07   ¦¬¨â¥ U ¤«ï ®¡­®¢«¥­¨ï Ž‘.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07   ¦¬¨â¥ R ¤«ï § ¯ãáª  ª®­á®«¨ ¢®ááâ ­®¢«¥­¨ï.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07   ¦¬¨â¥ ESC ¤«ï ¢®§¢à â  ­  £« ¢­ãî áâà ­¨æã.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07   ¦¬¨â¥ ENTER ¤«ï ¯¥à¥§ £àã§ª¨ ª®¬¯ìîâ¥à .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC =   £« ¢­ãî  U = Ž¡­®¢«¥­¨¥  R = ‚®ááâ ­®¢«¥­¨¥  ENTER = ¥à¥§ £àã§ª ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUComputerPageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‚ë å®â¨â¥ ¨§¬¥­¨âì ãáâ ­ ¢«¨¢ ¥¬ë© â¨¯ ª®¬¯ìîâ¥à .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07   ¦¬¨â¥ ª« ¢¨èã ‚‚…• ¨«¨ ‚ˆ‡ ¤«ï ¢ë¡®à  ¯à¥¤¯®çâ¨â¥«ì­®£® â¨¯ ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ª®¬¯ìîâ¥à . ‡ â¥¬ ­ ¦¬¨â¥ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07   ¦¬¨â¥ ª« ¢¨èã ESC ¤«ï ¢®§¢à â  ª ¯à¥¤ë¤ãé¥© áâà ­¨æ¥ ¡¥§ ¨§¬¥­¥­¨ï",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   â¨¯  ª®¬¯ìîâ¥à .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   ESC = Žâ¬¥­    F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUFlushPageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "‘¨áâ¥¬  ¯à®¢¥àï¥â, ¢á¥ «¨ ¤ ­­ë¥ § ¯¨á ­ë ­  ¤¨áª",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "â® ¬®¦¥â § ­ïâì ­¥ª®â®à®¥ ¢à¥¬ï.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "®á«¥ § ¢¥àè¥­¨ï ª®¬¯ìîâ¥à ¡ã¤¥â  ¢â®¬ â¨ç¥áª¨ ¯¥à¥§ £àã¦¥­.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Žç¨áâª  ª¥è ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUQuitPageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS ãáâ ­®¢«¥­ ­¥ ¯®«­®áâìî",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ˆ§¢«¥ª¨â¥ £¨¡ª¨© ¤¨áª ¨§ ¤¨áª®¢®¤  A: ¨",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "¢á¥ CD-ROM ¨§ CD-¤¨áª®¢®¤®¢.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        " ¦¬¨â¥ ENTER ¤«ï ¯¥à¥§ £àã§ª¨ ª®¬¯ìîâ¥à .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "®¦ «ã©áâ , ¯®¤®¦¤¨â¥ ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUDisplayPageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‚ë å®â¨â¥ ¨§¬¥­¨âì ãáâ ­ ¢«¨¢ ¥¬ë© â¨¯ íªà ­ .",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07   ¦¬¨â¥ ª« ¢¨è¨ ‚‚…• ¨«¨ ‚ˆ‡ ¤«ï ¢ë¡®à  â¨¯  íªà ­ .",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ‡ â¥¬ ­ ¦¬¨â¥ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07   ¦¬¨â¥ ª« ¢¨èã ESC ¤«ï ¢®§¢à â  ª ¯à¥¤ë¤ãé¥© áâà ­¨æ¥ ¡¥§ ¨§¬¥­¥­¨ï",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   â¨¯  íªà ­ .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   ESC = Žâ¬¥­    F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUSuccessPageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Žá­®¢­ë¥ ª®¬¯®­¥­âë ReactOS ¡ë«¨ ãá¯¥è­® ãáâ ­®¢«¥­ë.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ˆ§¢«¥ª¨â¥ £¨¡ª¨© ¤¨áª ¨§ ¤¨áª®¢®¤  A: ¨",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "¢á¥ CD-ROM ¨§ CD-¤¨áª®¢®¤®¢.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        " ¦¬¨â¥ ENTER ¤«ï ¯¥à¥§ £àã§ª¨ ª®¬¯ìîâ¥à .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¥à¥§ £àã§ª ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUBootPageEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "à®£à ¬¬  ãáâ ­®¢ª¨ ­¥ á¬®£«  ãáâ ­®¢¨âì § £àã§ç¨ª ­ ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "¦¥áâª¨© ¤¨áª ¢ è¥£® ª®¬¯ìîâ¥à .",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "®¦ «ã©áâ  ¢áâ ¢ìâ¥ ®âä®à¬ â¨à®¢ ­­ë© £¨¡ª¨© ¤¨áª ¢ ¤¨áª®¢®¤ A: ¨",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "­ ¦¬¨â¥ ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY ruRUSelectPartitionEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‚ á¯¨áª¥ ­¨¦¥ ¯®ª § ­ë áãé¥áâ¢ãîé¨¥ à §¤¥«ë ¨ ­¥¨á¯®«ì§ã¥¬®¥",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "¯à®áâà ­áâ¢® ¤«ï ­®¢®£® à §¤¥« .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07   ¦¬¨â¥ ‚‚…• ¨«¨ ‚ˆ‡ ¤«ï ¢ë¡®à  í«¥¬¥­â .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07   ¦¬¨â¥ ENTER ¤«ï ãáâ ­®¢ª¨ ReactOS ­  ¢ë¤¥«¥­­ë© à §¤¥«.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07   ¦¬¨â¥ P ¤«ï á®§¤ ­¨ï ¯¥à¢¨ç­®£® à §¤¥« .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07   ¦¬¨â¥ E ¤«ï á®§¤ ­¨ï à áè¨à¥­­®£® à §¤¥« .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07   ¦¬¨â¥ L ¤«ï á®§¤ ­¨ï «®£¨ç¥áª®£® à §¤¥« .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07   ¦¬¨â¥ D ¤«ï ã¤ «¥­¨ï áãé¥áâ¢ãîé¥£® à §¤¥« .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "®¦ «ã©áâ , ¯®¤®¦¤¨â¥...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‚ë å®â¨â¥ ã¤ «¨âì á¨áâ¥¬­ë© à §¤¥«.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "‘¨áâ¥¬­ë¥ à §¤¥«ë ¬®£ãâ á®¤¥à¦ âì ¤¨ £­®áâ¨ç¥áª¨¥ ¯à®£à ¬¬ë, ¯à®£à ¬¬ë",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "­ áâà®©ª¨  ¯¯ à â­ëå áà¥¤áâ¢, ¯à®£à ¬¬ë § ¯ãáª  Ž‘ (¯®¤®¡­ë¥ ReactOS)",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "¨«¨ ¤àã£®¥ Ž, ¯à¥¤®áâ ¢«¥­­®¥ ¨§£®â®¢¨â¥«¥¬ ®¡®àã¤®¢ ­¨ï.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "“¤ «ï©â¥ á¨áâ¥¬­ë© à §¤¥«, ª®£¤  ã¢¥à¥­ë, çâ® ­  ­¥¬ ­¥â ¢ ¦­ëå ¯à®£à ¬¬",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "¨«¨ ª®£¤  ¢ë ã¢¥à¥­ë, çâ® ®­¨ ­¥ ­ã¦­ë.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "Š®£¤  ¢ë ã¤ «¨â¥ á¨áâ¥¬­ë© à §¤¥«, ¢ë ­¥ á¬®¦¥â¥ § £àã§¨âì",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "ª®¬¯ìîâ¥à á ¦¥áâª®£® ¤¨áª , ¯®ª  ­¥ § ª®­ç¨â¥ ãáâ ­®¢ªã ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07   ¦¬¨â¥ ENTER çâ®¡ë ã¤ «¨âì á¨áâ¥¬­ë© à §¤¥«. ‚ë ¤®«¦­ë ¡ã¤¥â¥",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   ¯®¤â¢¥à¤¨âì ã¤ «¥­¨¥ ¯®§¦¥ á­®¢ .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07   ¦¬¨â¥ ESC çâ®¡ë ¢¥à­ãâìáï ª ¯à¥¤ë¤ãé¥© áâà ­¨æ¥.  §¤¥« ­¥",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   ¡ã¤¥â ã¤ «¥­.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=à®¤®«¦¨âì  ESC=Žâ¬¥­ ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUFormatPartitionEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "”®à¬ â¨à®¢ ­¨¥ à §¤¥« ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "„«ï ãáâ ­®¢ª¨ à §¤¥« ¡ã¤¥â ®âä®à¬ â¨à®¢ ­.  ¦¬¨â¥ ENTER ¤«ï ¯à®¤®«¦¥­¨ï.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY ruRUInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "“áâ ­®¢ª  ä ©«®¢ ReactOS ­  ¢ë¡à ­­ë© à §¤¥«. ‚ë¡¥à¨â¥ ¤¨à¥ªâ®à¨î,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "¢ ª®â®àãî ¡ã¤¥â ãáâ ­®¢«¥­  á¨áâ¥¬ :",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "—â®¡ë ¨§¬¥­¨âì ¢ë¡à ­­ãî ¤¨à¥ªâ®à¨î, ­ ¦¬¨â¥ BACKSPACE ¤«ï ã¤ «¥­¨ï",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "á¨¬¢®«®¢,   §  â¥¬ ­ ¡¥à¨â¥ ­®¢®¥ ¨¬ï ¤¨à¥ªâ®à¨¨ ¤«ï ãáâ ­®¢ª¨ ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        " ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUFileCopyEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "®¦ «ã©áâ , ¯®¤®¦¤¨â¥, ¯®ª  ¯à®£à ¬¬  ãáâ ­®¢ª¨ áª®¯¨àã¥â ä ©«ë",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "ReactOS ¢ ãáâ ­®¢®ç­ãî ¤¨à¥ªâ®à¨î.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "â® ¬®¦¥â § ­ïâì ­¥áª®«ìª® ¬¨­ãâ.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 ®¦ «ã©áâ , ¯®¤®¦¤¨â¥...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUBootLoaderEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "“áâ ­®¢ª  § £àã§ç¨ª  ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "“áâ ­®¢¨âì § £àã§ç¨ª ­  ¦¥áâª¨© ¤¨áª (MBR ¨ VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "“áâ ­®¢¨âì § £àã§ç¨ª ­  ¦¥áâª¨© ¤¨áª (â®«ìª® VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "“áâ ­®¢¨âì § £àã§ç¨ª ­  £¨¡ª¨© ¤¨áª.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "¥ ãáâ ­ ¢«¨¢ âì § £àã§ç¨ª.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ˆ§¬¥­¥­¨¥ â¨¯  ª« ¢¨ âãàë.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07   ¦¬¨â¥ ‚‚…• ¨«¨ ‚ˆ‡ ¤«ï ¢ë¡®à  ­ã¦­®£® â¨¯  ª« ¢¨ âãàë.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ‡ â¥¬ ­ ¦¬¨â¥ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07   ¦¬¨â¥ ª« ¢¨èã ESC ¤«ï ¢®§¢à â  ª ¯à¥¤ë¤ãé¥© áâà ­¨æ¥ ¡¥§ ¨§¬¥­¥­¨ï",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   â¨¯  ª« ¢¨ âãàë.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   ESC = Žâ¬¥­    F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRULayoutSettingsEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "®¦ «ã©áâ  ¢ë¡¥à¨â¥ à áª« ¤ªã, ª®â®à ï ¡ã¤¥â ãáâ ­®¢«¥­  ¯® ã¬®«ç ­¨î.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07   ¦¬¨â¥ ‚‚…• ¨«¨ ‚ˆ‡ ¤«ï ¢ë¡®à  ­ã¦­®© à áª« ¤ª¨ ª« ¢¨ âãàë.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ‡ â¥¬ ­ ¦¬¨â¥ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07   ¦¬¨â¥ ª« ¢¨èã ESC ¤«ï ¢®§¢à â  ª ¯à¥¤ë¤ãé¥© áâà ­¨æ¥ ¡¥§",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ¨§¬¥­¥­¨ï à áª« ¤ª¨ ª« ¢¨ âãàë.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   ESC = Žâ¬¥­    F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ruRUPrepareCopyEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "®¤£®â®¢ª  ¢ è¥£® ª®¬¯ìîâ¥à  ª ª®¯¨à®¢ ­¨î ä ©«®¢ ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "®¤£®â®¢ª  á¯¨áª  ª®¯¨àã¥¬ëå ä ©«®¢...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ruRUSelectFSEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "‚ë¡¥à¨â¥ ä ©«®¢ãî á¨áâ¥¬ã ¨§ á¯¨áª  ­¨¦¥.",
        0
    },
    {
        8,
        19,
        "\x07   ¦¬¨â¥ ‚‚…• ¨«¨ ‚ˆ‡ ¤«ï ¢ë¡®à  ä ©«®¢®© á¨áâ¥¬ë.",
        0
    },
    {
        8,
        21,
        "\x07   ¦¬¨â¥ ENTER ¤«ï ä®à¬ â¨à®¢ ­¨ï à §¤¥« .",
        0
    },
    {
        8,
        23,
        "\x07   ¦¬¨â¥ ESC ¤«ï ¢ë¡®à  ¤àã£®£® à §¤¥« .",
        0
    },
    {
        0,
        0,
        "ENTER = à®¤®«¦¨âì   ESC = Žâ¬¥­    F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRUDeletePartitionEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‚ë ¢ë¡à «¨ ã¤ «¥­¨¥ à §¤¥« .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07   ¦¬¨â¥ D ¤«ï ã¤ «¥­¨ï à §¤¥« .",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "‚ˆŒ€ˆ…: ‚á¥ ¤ ­­ë¥ á íâ®£® à §¤¥«  ¡ã¤ãâ ¯®â¥àï­ë!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07   ¦¬¨â¥ ESC ¤«ï ®â¬¥­ë.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = “¤ «¨âì à §¤¥«   ESC = Žâ¬¥­    F3 = ‚ëå®¤",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ruRURegistryEntries[] =
{
    {
        4,
        3,
        " “áâ ­®¢ª  ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "à®£à ¬¬  ãáâ ­®¢ª¨ ®¡­®¢«ï¥â ª®­ä¨£ãà æ¨î á¨áâ¥¬ë. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "‘®§¤ ­¨¥ ªãáâ®¢ á¨áâ¥¬­®£® à¥¥áâà ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR ruRUErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "“á¯¥è­®\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS ­¥ ¡ë« ¯®«­®áâìî ãáâ ­®¢«¥­ ­  ¢ è\n"
        "ª®¬¯ìîâ¥à. …á«¨ ¢ë ¢ë©¤¨â¥ ¨§ ãáâ ­®¢ª¨ á¥©ç á,\n"
        "â® ¢ ¬ ­ã¦­® § ¯ãáâ¨âì ¯à®£à ¬¬ã ãáâ ­®¢ª¨ á­®¢ ,\n"
        "¥á«¨ ¢ë å®â¨â¥ ãáâ ­®¢¨âì ReactOS\n"
        "  \x07   ¦¬¨â¥ ENTER ¤«ï ¯à®¤®«¦¥­¨ï ãáâ ­®¢ª¨.\n"
        "  \x07   ¦¬¨â¥ F3 ¢ëå®¤  ¨§ ãáâ ­®¢ª¨.",
        "F3 = ‚ëå®¤  ENTER = à®¤®«¦¨âì"
    },
    {
        //ERROR_NO_HDD
        "¥ ã¤ «®áì ­ ©â¨ ¦¥áâª¨© ¤¨áª.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "¥ ã¤ «®áì ­ ©â¨ ãáâ ­®¢®ç­ë© ¤¨áª.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "¥ ã¤ «®áì § £àã§¨âì ä ©« TXTSETUP.SIF.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "” ©« TXTSETUP.SIF ¯®¢à¥¦¤¥­.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Ž¡­ àã¦¥­  ­¥ª®àà¥ªâ­ ï ¯®¤¯¨áì ¢ TXTSETUP.SIF.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_DRIVE_INFORMATION
        "¥ ã¤ «®áì ¯®«ãç¨âì ¨­ä®à¬ æ¨î ® á¨áâ¥¬­®¬ ¤¨áª¥.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_WRITE_BOOT,
        "¥ ã¤ «®áì ãáâ ­®¢¨âì § £àã§ç¨ª FAT ­  á¨áâ¥¬­ë© à §¤¥«.",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_LOAD_COMPUTER,
        "¥ ã¤ «®áì § £àã§¨âì á¯¨á®ª â¨¯®¢ ª®¬¯ìîâ¥à .\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_LOAD_DISPLAY,
        "¥ ã¤ «®áì § £àã§¨âì á¯¨á®ª à¥¦¨¬®¢ íªà ­ .\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "¥ ã¤ «®áì § £àã§¨âì á¯¨á®ª â¨¯®¢ ª« ¢¨ âãàë.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "¥ ã¤ «®áì § £àã§¨âì á¯¨á®ª à áª« ¤®ª ª« ¢¨ âãàë.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_WARN_PARTITION,
        " ©¤¥­ ¯® ªà ©­¥© ¬¥à¥ ®¤¨­ ¦¥áâª¨© ¤¨áª, ª®â®àë© á®¤¥à¦¨â à §¤¥«\n"
        "­¥¯®¤¤¥à¦¨¢ ¥¬ë© ReactOS!\n"
        "\n"
        "‘®§¤ ­¨¥ ¨«¨ ã¤ «¥­¨¥ à §¤¥«®¢ ¬®¦¥â ­ àãè¨âì â ¡«¨æã à §¤¥«®¢.\n"
        "\n"
        "  \x07   ¦¬¨â¥ F3 ¤«ï ¢ëå®¤  ¨§ ãáâ ­®¢ª¨.\n"
        "  \x07   ¦¬¨â¥ ENTER ¤«ï ¯à®¤®«¦¥­¨ï.",
        "F3 = ‚ëå®¤  ENTER = à®¤®«¦¨âì"
    },
    {
        //ERROR_NEW_PARTITION,
        "‚ë ­¥ ¬®¦¥â¥ á®§¤ âì ­®¢ë© à §¤¥« ¤¨áª  ¢\n"
        "ã¦¥ áãé¥áâ¢ãîé¥¬ à §¤¥«¥!\n"
        "\n"
        "  *  ¦¬¨â¥ «î¡ãî ª« ¢¨èã ¤«ï ¯à®¤®«¦¥­¨ï.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "‚ë ­¥ ¬®¦¥â¥ ã¤ «¨âì ­¥à §¤¥«¥­­®¥ ¤¨áª®¢®¥ ¯à®áâà ­áâ¢®!\n"
        "\n"
        "  *  ¦¬¨â¥ «î¡ãî ª« ¢¨èã ¤«ï ¯à®¤®«¦¥­¨ï.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "¥ ã¤ «®áì ãáâ ­®¢¨âì § £àã§ç¨ª FAT ­  á¨áâ¥¬­ë© à §¤¥«.",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_NO_FLOPPY,
        "¥â ¤¨áª  ¢ ¤¨áª®¢®¤¥ A:.",
        "ENTER = à®¤®«¦¨âì"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "¥ ã¤ «®áì ®¡­®¢¨âì ¯ à ¬¥âàë à áª« ¤ª¨ ª« ¢¨ âãàë.",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "¥ ã¤ «®áì ®¡­®¢¨âì ¯ à ¬¥âàë íªà ­  ¢ à¥¥áâà¥.",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_IMPORT_HIVE,
        "¥ ã¤ «®áì ¨¬¯®àâ¨à®¢ âì ä ©«ë ªãáâ®¢ à¥¥áâà .",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_FIND_REGISTRY
        "¥ ã¤ «®áì ­ ©â¨ ä ©«ë á¨áâ¥¬­®£® à¥¥áâà .",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_CREATE_HIVE,
        "¥ ã¤ «®áì á®§¤ âì ªãáâë á¨áâ¥¬­®£® à¥¥áâà .",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "¥ ã¤ «®áì ¨­¨æ¨ «¨§¨à®¢ âì á¨áâ¥¬­ë© à¥¥áâà.",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet ­¥ ¯®«ãç¨« ª®àà¥ªâ­ë© inf-ä ©«.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet ­¥ ­ ©¤¥­.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet ­¥ á¬®£ ­ ©â¨ ãáâ ­®¢®ç­ë© áªà¨¯â.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_COPY_QUEUE,
        "¥ ã¤ «®áì ®âªàëâì ®ç¥à¥¤ì ª®¯¨à®¢ ­¨ï ä ©«®¢.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_CREATE_DIR,
        "¥ ã¤ «®áì á®§¤ âì ãáâ ­®¢®ç­ë¥ ¤¨à¥ªâ®à¨¨.",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "¥ ã¤ «®áì ­ ©â¨ á¥ªæ¨î 'Directories'\n"
        "¢ ä ©«¥ TXTSETUP.SIF.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_CABINET_SECTION,
        "¥ ã¤ «®áì ­ ©â¨ á¥ªæ¨î 'Directories'\n"
        "¢ cabinet.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "¥ ã¤ «®áì á®§¤ âì ¤¨à¥ªâ®à¨î ¤«ï ãáâ ­®¢ª¨.",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_FIND_SETUPDATA,
        "¥ ã¤ «®áì ­ ©â¨ á¥ªæ¨î 'SetupData'\n"
        "¢ ä ©«¥ TXTSETUP.SIF.\n",
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_WRITE_PTABLE,
        "¥ ã¤ «®áì § ¯¨á âì â ¡«¨æã à §¤¥«®¢.\n"
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "¥ ã¤ «®áì ¤®¡ ¢¨âì ¯ à ¬¥âàë ª®¤¨à®¢ª¨ ¢ à¥¥áâà.\n"
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "¥ ã¤ «®áì ãáâ ­®¢¨âì ï§ëª á¨áâ¥¬ë.\n"
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "¥ ã¤ «®áì ¤®¡ ¢¨âì à áª« ¤ªã ª« ¢¨ âãàë ¢ à¥¥áâà.\n"
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_UPDATE_GEOID,
        "¥ ã¤ «®áì ãáâ ­®¢¨âì geo id.\n"
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        //ERROR_DIRECTORY_NAME,
        "¥¢¥à­®¥ ­ §¢ ­¨¥ ¤¨à¥ªâ®à¨¨.\n"
        "\n"
        "  *  ¦¬¨â¥ «î¡ãî ª« ¢¨èã ¤«ï ¯à®¤®«¦¥­¨ï."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "‚ë¡à ­­ë© à §¤¥« á«¨èª®¬ ¬ « ¤«ï ãáâ ­®¢ª¨ ReactOS.\n"
        "“áâ ­®¢®ç­ë© à §¤¥« ¤®«¦¥­ ¨¬¥âì ¯® ªà ©­¥© ¬¥à¥ %lu MB ¯à®áâà ­áâ¢ .\n"
        "\n"
        "  *  ¦¬¨â¥ «î¡ãî ª« ¢¨èã ¤«ï ¯à®¤®«¦¥­¨ï.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "‚ë ­¥ ¬®¦¥â¥ á®§¤ âì ¯¥à¢¨ç­ë© ¨«¨ à áè¨à¥­­ë© à §¤¥« ¢ â ¡«¨æ¥\n"
        "à §¤¥«®¢ ¤¨áª , ¯®â®¬ã çâ® ®­  § ¯®«­¥­ .\n"
        "\n"
        "  *  ¦¬¨â¥ «î¡ãî ª« ¢¨èã ¤«ï ¯à®¤®«¦¥­¨ï."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "‚ë ­¥ ¬®¦¥â¥ á®§¤ âì ¡®«ìè¥ ®¤­®£® à áè¨à¥­­®£® à §¤¥«  ­  ¤¨áª.\n"
        "\n"
        "  *  ¦¬¨â¥ «î¡ãî ª« ¢¨èã ¤«ï ¯à®¤®«¦¥­¨ï."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "¥ ã¤ «®áì ä®à¬ â¨à®¢ âì à §¤¥«:\n"
        " %S\n"
        "\n"
        "ENTER = ¥à¥§ £àã§ª "
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE ruRUPages[] =
{
    {
        LANGUAGE_PAGE,
        ruRULanguagePageEntries
    },
    {
        START_PAGE,
        ruRUWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        ruRUIntroPageEntries
    },
    {
        LICENSE_PAGE,
        ruRULicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        ruRUDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        ruRURepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        ruRUComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        ruRUDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        ruRUFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        ruRUSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        ruRUConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        ruRUSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        ruRUFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        ruRUDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        ruRUInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        ruRUPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        ruRUFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        ruRUKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        ruRUBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        ruRULayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        ruRUQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        ruRUSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        ruRUBootPageEntries
    },
    {
        REGISTRY_PAGE,
        ruRURegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING ruRUStrings[] =
{
    {STRING_PLEASEWAIT,
     "   ®¦ «ã©áâ , ¯®¤®¦¤¨â¥..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = “áâ ­®¢¨âì   P = ¥à¢¨ç­ë© à §¤¥«   E =  áè¨à¥­­ë©   F3 = ‚ëå®¤"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = “áâ ­®¢¨âì   L = ‘®§¤ âì «®£¨ç¥áª¨© à §¤¥«   F3 = ‚ëå®¤"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = “áâ ­®¢¨âì   D = “¤ «¨âì à §¤¥«   F3 = ‚ëå®¤"},
    {STRING_DELETEPARTITION,
     "   D = “¤ «¨âì à §¤¥«   F3 = ‚ëå®¤"},
    {STRING_PARTITIONSIZE,
     " §¬¥à ­®¢®£® à §¤¥« :"},
    {STRING_CHOOSENEWPARTITION,
     "‚ë å®â¨â¥ á®§¤ âì ¯¥à¢¨ç­ë© à §¤¥« ­ "},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "‚ë å®â¨â¥ á®§¤ âì à áè¨à¥­­ë© à §¤¥« ­ "},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "‚ë å®â¨â¥ á®§¤ âì «®£¨ç¥áª¨© à §¤¥« ­ "},
    {STRING_HDDSIZE,
    "®¦ «ã©áâ , ¢¢¥¤¨â¥ à §¬¥à ­®¢®£® à §¤¥«  ¢ ¬¥£ ¡ ©â å."},
    {STRING_CREATEPARTITION,
     "   ENTER = ‘®§¤ âì à §¤¥«   ESC = Žâ¬¥­    F3 = ‚ëå®¤"},
    {STRING_PARTFORMAT,
    "â®â à §¤¥« ¡ã¤¥â ®âä®à¬ â¨à®¢ ­ ¤ «¥¥."},
    {STRING_NONFORMATTEDPART,
    "‚ë ¢ë¡à «¨ ãáâ ­®¢ªã ReactOS ­  ­®¢ë© ­¥®âä®à¬ â¨à®¢ ­­ë© à §¤¥«."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "‘¨áâ¥¬­ë© à §¤¥« ­¥ ®âä®à¬ â¨à®¢ ­."},
    {STRING_NONFORMATTEDOTHERPART,
    "®¢ë© à §¤¥« ­¥ ®âä®à¬ â¨à®¢ ­."},
    {STRING_INSTALLONPART,
    "ReactOS ãáâ ­ ¢«¨¢ ¥âáï ­  à §¤¥«:"},
    {STRING_CHECKINGPART,
    "à®£à ¬¬  ãáâ ­®¢ª¨ ¯à®¢¥àï¥â ¢ë¡à ­­ë© à §¤¥«."},
    {STRING_CONTINUE,
    "ENTER = à®¤®«¦¨âì"},
    {STRING_QUITCONTINUE,
    "F3 = ‚ëå®¤  ENTER = à®¤®«¦¨âì"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = ¥à¥§ £àã§ª "},
    {STRING_TXTSETUPFAILED,
    "à®£à ¬¬  ãáâ ­®¢ª¨ ­¥ á¬®£«  ­ ©â¨ á¥ªæ¨î '%S'\n¢ ä ©«¥ TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Š®¯¨à®¢ ­¨¥ ä ©« : %S"},
    {STRING_SETUPCOPYINGFILES,
     "à®£à ¬¬  ãáâ ­®¢ª¨ ª®¯¨àã¥â ä ©«ë..."},
    {STRING_REGHIVEUPDATE,
    "   Ž¡­®¢«¥­¨¥ ªãáâ®¢ à¥¥áâà ..."},
    {STRING_IMPORTFILE,
    "   ˆ¬¯®àâ¨à®¢ ­¨¥ %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Ž¡­®¢«¥­¨¥ ¯ à ¬¥âà®¢ íªà ­  ¢ à¥¥áâà¥..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Ž¡­®¢«¥­¨¥ ¯ à ¬¥âà®¢ ï§ëª ..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Ž¡­®¢«¥­¨¥ ¯ à ¬¥âà®¢ à áª« ¤ª¨ ª« ¢¨ âãàë..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   „®¡ ¢«¥­¨¥ ¨­ä®à¬ æ¨¨ ® ª®¤®¢®© áâà ­¨æ¥ ¢ à¥¥áâà..."},
    {STRING_DONE,
    "   ‡ ¢¥àè¥­®..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = ¥à¥§ £àã§ª "},
    {STRING_CONSOLEFAIL1,
    "¥ ã¤ «®áì ®âªàëâì ª®­á®«ì\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    " ¨¡®«¥¥ ¢¥à®ïâ­ ï ¯à¨ç¨­  íâ®£® - ¨á¯®«ì§®¢ ­¨¥ USB-ª« ¢¨ âãàë\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB ª« ¢¨ âãàë á¥©ç á ¯®¤¤¥à¦¨¢ îâáï ­¥ ¯®«­®áâìî\r\n"},
    {STRING_FORMATTINGDISK,
    "à®£à ¬¬  ãáâ ­®¢ª¨ ä®à¬ â¨àã¥â ¢ è ¤¨áª"},
    {STRING_CHECKINGDISK,
    "à®£à ¬¬  ãáâ ­®¢ª¨ ¯à®¢¥àï¥â ¢ è ¤¨áª"},
    {STRING_FORMATDISK1,
    " ”®à¬ â¨à®¢ ­¨¥ à §¤¥«  ¢ ä ©«®¢®© á¨áâ¥¬¥ %S (¡ëáâà®¥) "},
    {STRING_FORMATDISK2,
    " ”®à¬ â¨à®¢ ­¨¥ à §¤¥«  ¢ ä ©«®¢®© á¨áâ¥¬¥ %S "},
    {STRING_KEEPFORMAT,
    " Žáâ ¢¨âì áãé¥áâ¢ãîéãî ä ©«®¢ãî á¨áâ¥¬ã (¡¥§ ¨§¬¥­¥­¨©) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  †¥áâª¨© ¤¨áª %lu  (®àâ=%hu, ˜¨­ =%hu, Id=%hu) ­  %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  †¥áâª¨© ¤¨áª %lu  (®àâ=%hu, ˜¨­ =%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  ‡ ¯¨áì 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "­  %I64u %s  †¥áâª¨© ¤¨áª %lu  (®àâ=%hu, ˜¨­ =%hu, Id=%hu) ­  %wZ."},
    {STRING_HDDINFOUNK3,
    "­  %I64u %s  †¥áâª¨© ¤¨áª %lu  (®àâ=%hu, ˜¨­ =%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "†¥áâª¨© ¤¨áª %lu (%I64u %s), ®àâ=%hu, ˜¨­ =%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  ‡ ¯¨áì 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "­  ¦¥áâª®¬ ¤¨áª¥ %lu (%I64u %s), ®àâ=%hu, ˜¨­ =%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %s‡ ¯¨áì %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  †¥áâª¨© ¤¨áª %lu  (®àâ=%hu, ˜¨­ =%hu, Id=%hu) ­  %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  †¥áâª¨© ¤¨áª %lu  (®àâ=%hu, ˜¨­ =%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "à®£à ¬¬  ãáâ ­®¢ª¨ á®§¤ «  ­®¢ë© à §¤¥« ­ :"},
    {STRING_UNPSPACE,
    "    %s¥à §¬¥ç¥­­®¥ ¯à®áâà ­áâ¢®%s            %6lu %s"},
    {STRING_MAXSIZE,
    "Œ (¬ ªá. %lu Œ)"},
    {STRING_EXTENDED_PARTITION,
    " áè¨à¥­­ë© à §¤¥«"},
    {STRING_UNFORMATTED,
    "®¢ë© (­¥®âä®à¬ â¨à®¢ ­­ë©)"},
    {STRING_FORMATUNUSED,
    "¥ ¨á¯®«ì§®¢ ­®"},
    {STRING_FORMATUNKNOWN,
    "¥¨§¢¥áâ­ë©"},
    {STRING_KB,
    "Š"},
    {STRING_MB,
    "Œ"},
    {STRING_GB,
    "ƒ"},
    {STRING_ADDKBLAYOUTS,
    "„®¡ ¢«¥­¨¥ à áª« ¤ª¨ ª« ¢¨ âãàë"},
    {0, 0}
};
