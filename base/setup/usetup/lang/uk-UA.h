/*
 *      translated by Artem Reznikov, Igor Paliychuk, 2010
 *      http://www.reactos.org/uk/
 */ 

#ifndef LANG_UK_UA_H__
#define LANG_UK_UA_H__

MUI_LAYOUTS ukUALayouts[] =
{
    { L"0422", L"00000422" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY ukUALanguagePageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ì·¢Šá ÒÖë·",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  £ç¦í-Ð ãÆ , ë·¢¨áŠåí ÒÖëç, ÞÆ  ¢ç¦¨ ë·ÆÖá·ãå Ô  ØŠ¦ û ã ëãå ÔÖëÐ¨ÔÔÞ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Š Ô å·ãÔŠåí ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ¥Þ ÒÖë  ¢ç¦¨ ë·ÆÖá·ãåÖëçë å·ãí ØÖ ó ÒÖëûçë ÔÔœ ç ëãå ÔÖëÐ¨ÔŠ½ ã·ãå¨ÒŠ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·  F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAWelcomePageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ñ ãÆ ëÖ ØáÖã·ÒÖ ¦Ö ØáÖ¬á Ò· ëãå ÔÖëÐ¨ÔÔÞ ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Õ  ¤íÖÒç ¨å ØŠ ëãå ÔÖëÐ¨ÔÔÞ ëŠ¦¢ç¦¨åíãÞ ÆÖØŠœë ÔÔÞ ReactOS Ô  ì õ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "ÆÖÒØ'œå¨á Š ØŠ¦¬ÖåÖëÆ  ¦Ö ¦áç¬Ö¬Ö ¨å Øç ëãå ÔÖëÐ¨ÔÔÞ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Õ å·ãÔŠåí ENTER ùÖ¢ ëãå ÔÖë·å· ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Õ å·ãÔŠåí R ¦ÐÞ ÖÔÖëÐ¨ÔÔÞ ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Ô å·ãÔŠåí L ¦ÐÞ Ø¨á¨¬ÐÞ¦ç ÐŠ¤¨ÔóŠ½Ô·µ çÒÖë ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Õ å·ãÔŠåí F3 ùÖ¢ ë·½å·. Ô¨ ëãå ÔÖëÐœœû· ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "§ÐÞ Öåá·Ò ÔÔÞ ¦¨å ÐíÔŠõÖŒ ŠÔªÖáÒ ¤ŠŒ ØáÖ ReactOS, ¢ç¦í-Ð ãÆ  ëŠ¦ëŠ¦ ½å¨:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org/uk/",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·  R = ìŠ¦ÔÖë·å·  L = ÑŠ¤¨ÔóŠÞ  F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAIntroPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ìãå ÔÖëÐœë û ReactOS óÔ µÖ¦·åíãÞ ë á ÔÔŠ½ ãå ¦ŠŒ áÖóáÖ¢Æ· Š ù¨ Ô¨",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ØŠ¦åá·Òç† ëãŠ ªçÔÆ¤ŠŒ ØÖëÔÖ¤ŠÔÔÖŒ ØáÖ¬á Ò· ëãå ÔÖëÐ¨ÔÔÞ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Ýá·ãçåÔŠ Ô ãåçØÔŠ Ö¢Ò¨é¨ÔÔÞ:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- ìãå ÔÖëÐœë û Ô¨ ØŠ¦åá·Òç† ¢ŠÐíõ¨ ÔŠé Ö¦·Ô Ø¨áë·ÔÔ·½ áÖó¦ŠÐ Ô  ¦·ãÆ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- ìãå ÔÖëÐœë û Ô¨ ÒÖé¨ ë·¦ Ð·å· Ø¨áë·ÔÔ·½ áÖó¦ŠÐ ó ¦·ãÆç",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  ØÖÆ· Ô  ¦·ãÆç Ô ÞëÔ·½ áÖóõ·á¨Ô·½ áÖó¦ŠÐ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- ìãå ÔÖëÐœë û Ô¨ ÒÖé¨ ë·¦ Ð·å· Ø¨áõ·½ áÖóõ·á¨Ô·½ áÖó¦ŠÐ ó ¦·ãÆç",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  ØÖÆ· Ô  ¦·ãÆç ŠãÔçœåí ŠÔõŠ áÖóõ·á¨ÔŠ áÖó¦ŠÐ·.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- ìãå ÔÖëÐœë û ØŠ¦åá·Òç† Ð·õ¨ ª ½ÐÖëç ã·ãå¨Òç FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- Ý¨á¨ëŠáÆ  ª ½ÐÖëÖŒ ã·ãå¨Ò· ù¨ Ô¨ ëØáÖë ¦é¨Ô .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Õ å·ãÔŠåí ENTER ùÖ¢ ëãå ÔÖë·å· ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Õ å·ãÔŠåí F3 ùÖ¢ ë·½å·, Ô¨ ëãå ÔÖëÐœœû· ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUALicensePageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "ÑŠ¤¨ÔóŠÞ:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS ÐŠ¤¨ÔóÖë ÔÖ ëŠ¦ØÖëŠ¦ÔÖ ¦Ö çÒÖë",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL. æ ÆÖé ReactOS ÒŠãå·åí ÆÖÒØÖÔ¨Ôå·, ÞÆŠ ÐŠ¤¨ÔóÖë ÔÖ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "ó  ãçÒŠãÔ·Ò· ÐŠ¤¨ÔóŠÞÒ·: X11, BSD, GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "ìã¨ ØáÖ¬á ÒÔ¨ ó ¢¨óØ¨û¨ÔÔÞ, ÞÆ¨ ëµÖ¦·åí ë ã·ãå¨Òç ReactOS, ë·Øçù¨ÔÖ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Øi¦ ìi¦Æá·åÖœ Ði¤¨Ôói½ÔÖœ ç¬Ö¦Öœ GNU GPL ió ó¢¨á¨é¨ÔÔÞÒ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Ø¨áë·ÔÔ·µ Ði¤¨ÔóiŒ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "§ Ô¨ ØáÖ¬á ÒÔ¨ ó ¢¨óØ¨û¨ÔÔÞ ØÖãå ëÐÞ†åíãÞ £©ô ­¡â¡Õæi i ¢¨ó Ö¢Ò¨é¨Ôí",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "ç ë·ÆÖá·ãå ÔÔi, ÞÆ ó  Òiã¤¨ë·Ò, å Æ i ÒiéÔ áÖ¦Ô·Ò Øá ëÖÒ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "Ñi¤¨ÔóiÞ ReactOS ¦ÖóëÖÐÞ† Ø¨á¨¦ ûç ØáÖ¦çÆåç åá¨åiÒ ÖãÖ¢ Ò.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "àÆùÖ û¨á¨ó ¢ç¦í-ÞÆi Øá·û·Ô· ì· Ô¨ Öåá·Ò Ð· ÆÖØiœ ìi¦Æá·åÖŒ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "Ði¤¨Ôói½ÔÖŒ ç¬Ö¦· GNU á óÖÒ ó ReactOS, ëi¦ëi¦ †å¨",
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
        "­ á ÔåŠŒ:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "¥¨ † ëiÐíÔ¨ ØáÖ¬á ÒÔ¨ ó ¢¨óØ¨û¨ÔÔÞ; ¦·ë. ¦é¨á¨ÐÖ ¦ÐÞ Ø¨á¨¬ÐÞ¦ç Øá ë.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "Õ¨ ¦ œåíãÞ Õ‹àÇ‹ ¬ á ÔåŠŒ; ÔŠ ¬ á ÔåiŒ æ×ì¡âÕ×­× äæ¡Õè, ÔŠ ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "Ýâ¸§¡æÕ×äæi §Ñà Ç×ÕÇâ©æÕ¸¶ ¥iÑ©¾",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝÖë¨áÔçå·ãí",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADevicePageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "è ãØ·ãÆç Ô·éû¨ Øá·ë¨¦¨Ôi ØÖåÖûÔŠ Ø á Ò¨åá· Øá·ãåáÖŒë.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "ÇÖÒØ'œå¨á:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "©Æá Ô:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "ÇÐ ëi åçá :",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "ÇÐ ë. áÖóÆÐ ¦Æ :",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Ýá·½ÔÞå·:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "ô ãåÖãçë å· ¦ Ôi Ø á Ò¨åá· Øá·ãåáÖŒë",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "ì· ÒÖé¨å¨ óÒiÔ·å· Ø á Ò¨åá· Øá·ãåáÖŒë Ô å·ãÆ œû· ÆÐ ëiõi ì­×âè i ìÕ¸ô",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "¦ÐÞ ë·¦iÐ¨ÔÔÞ ¨Ð¨Ò¨Ôåç i ÆÐ ëiõç ENTER ¦ÐÞ ë·¢Öáç iÔõ·µ ë ái Ôåië",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "Ø á Ò¨åáië.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "ÇÖÐ· ëãi Ø á Ò¨åá· ¢ç¦çåí ë·óÔ û¨Ôi, ë·¢¨áiåí \"ô ãåÖãçë å· ¦ Ôi Ø á Ò¨åá· Øá·ãåáÖŒë\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "i Ô å·ãÔiåí ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUARepairPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ìãå ÔÖëÐœë û ReactOS óÔ µÖ¦·åíãÞ ë á ÔÔŠ½ ãå ¦ŠŒ áÖóáÖ¢Æ· Š ù¨ Ô¨",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ØŠ¦åá·Òç† ëãŠ ªçÔÆ¤ŠŒ ØÖëÔÖ¤ŠÔÔÖŒ ØáÖ¬á Ò· ëãå ÔÖëÐ¨ÔÔÞ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "«çÔÆ¤ŠŒ ëŠ¦ÔÖëÐ¨ÔÔÞ ù¨ Ô¨ ëØáÖë ¦é¨ÔŠ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Õ å·ãÔŠåí U ùÖ¢ ÖÔÖë·å· OS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Õ å·ãÔŠåí R ¦ÐÞ ó ØçãÆç ÇÖÔãÖÐŠ ìŠ¦ÔÖëÐ¨ÔÔÞ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Õ å·ãÔŠåí ESC ¦ÐÞ ØÖë¨áÔ¨ÔÔÞ ¦Ö ¬ÖÐÖëÔÖŒ ãåÖáŠÔÆ·.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Õ å·ãÔŠåí ENTER ùÖ¢ Ø¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = ­ÖÐÖëÔ  äåÖáŠÔÆ   U = ×ÔÖë·å·  R = ìŠ¦ÔÖë·å·  ENTER = Ý¨á¨ó ë Ôå é·å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY ukUAComputerPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "æçå ì· ÒÖé¨å¨ óÒŠÔ·å· å·Ø ì õÖ¬Ö ÆÖÒØ'œå¨á .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Õ å·ãÆ ½å¨ ÆÐ ëŠõŠ ìì©â¶ å  ìÕ¸ô ¦ÐÞ ë·¢Öáç å·Øç ì õÖ¬Ö ÆÖÒØ'œå¨á .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Š Ô å·ãÔŠåí ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Õ å·ãÔŠåí ESC ¦ÐÞ ØÖë¨áÔ¨ÔÔÞ ¦Ö ØÖØ¨á¨¦ÔíÖŒ ãåÖáŠÔÆ· ¢¨ó óÒŠÔ·",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   å·Øç ÆÖÒØ'œå¨á .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ØáÖ¦Öëé·å·   ESC = ìŠ¦ÒŠÔ·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFlushPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ä·ãå¨Ò  Ø¨á¨ëŠáÞ† û· ëãŠ ¦ ÔŠ ó¢¨á¨é¨ÔÖ Ô  ¦·ãÆ",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "¥¨ ÒÖé¨ ó ½ÔÞå· ¦¨ÆŠÐíÆ  µë·Ð·Ô",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "ÝŠãÐÞ ó ë¨áõ¨ÔÔÞ ÆÖÒØ'œå¨á ¢ç¦¨  ëåÖÒ å·ûÔÖ Ø¨á¨ó ë Ôå é¨ÔÖ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "×û·ùçœ Æ¨õ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAQuitPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS Ô¨ ëãå ÔÖëÐ¨ÔÖ ØÖëÔŠãåœ",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ì·åÞ¬ÔŠåí ¦·ãÆçåç ó ¦·ãÆÖëÖ¦ç A: å ",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "ëãŠ CD-ROM ó CD-Øá·ëÖ¦Šë.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Õ å·ãÔŠåí ENTER ùÖ¢ Ø¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "£ç¦í-Ð ãÆ  ó û¨Æ ½å¨ ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADisplayPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "æçå ë· ÒÖé¨å¨ óÒŠÔ·å· å·Ø ¨Æá Ôç.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Õ å·ãÆ ½å¨ ÆÐ ëŠõŠ ìì©â¶ å  ìÕ¸ô ¦ÐÞ ë·¢Öáç ØÖåáŠ¢ÔÖ¬Ö å·Øç ÒÖÔŠåÖáç.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Š Ô å·ãÔŠåí ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Õ å·ãÔŠåí ESC ¦ÐÞ ØÖë¨áÔ¨ÔÔÞ ¦Ö ØÖØ¨á¨¦ÔíÖŒ ãåÖáŠÔÆ· ¢¨ó óÒŠÔ·",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   å·Øç ÒÖÔŠåÖá .",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   ESC = ìŠ¦ÒŠÔ·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUASuccessPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "×ãÔÖëÔŠ ÆÖÒØÖÔ¨Ôå· ReactOS ¢çÐ· çãØŠõÔÖ ëãå ÔÖëÐ¨ÔŠ.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ì·åÞ¬ÔŠåí ¦·ãÆ¨åç ó ¦·ãÆÖëÖ¦ç A: å ",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "ëãŠµ CD-ROM ó CD-Øá·ëÖ¦Šë.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Õ å·ãÔŠåí ENTER ùÖ¢ Ø¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUABootPageEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ìãå ÔÖëÐœë û Ô¨ ÒÖé¨ ëãå ÔÖë·å· bootloader Ô  éÖáãåÆ·½ ¦·ãÆ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ì õÖ¬Ö ÆÖÒØ'œå¨á ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "£ç¦í-Ð ãÆ  ëãå ëå¨ ëŠ¦ªÖáÒ åÖë Ôç ¦·ãÆ¨åç ë ¦·ëÆÖëÖ¦ A: å ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Ô å·ãÔŠåí ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY ukUASelectPartitionEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Õ·éû¨ Øá·ë¨¦¨Ô·½ ãØ·ãÖÆ ŠãÔçœû·µ áÖó¦ŠÐŠë å  Ô¨ó ½ÔÞåÖ¬Ö ÒŠã¤Þ, ¦¨ ÒÖéÔ ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ãåëÖá·å· ÔÖëŠ áÖó¦ŠÐ·.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Õ å·ãÆ ½å¨ ÆÐ ëŠõŠ ìì©â¶ å  ìÕ¸ô ¦ÐÞ ë·¢Öáç ØçÔÆåç.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Õ å·ãÔŠåí ENTER ùÖ¢ ëãå ÔÖë·å· ReactOS Ô  ë·¢á Ô·½ áÖó¦ŠÐ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Õ å·ãÔŠåí C ùÖ¢ ãåëÖá·å· ÔÖë·½ áÖó¦ŠÐ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Õ å·ãÔŠåí D ùÖ¢ ë·¦ Ð·å· ŠãÔçœû·½ áÖó¦ŠÐ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "£ç¦í-Ð ãÆ  ó û¨Æ ½å¨...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "«ÖáÒ åçë ÔÔÞ áÖó¦ŠÐç",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "ô á ó ëãå ÔÖëÐœë û ëŠ¦ªÖáÒ åç† áÖó¦ŠÐ. Õ å·ãÔŠåí ENTER ¦ÐÞ ØáÖ¦Öëé¨ÔÔÞ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY ukUAInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ìãå ÔÖëÐœë û ëãå ÔÖë·åí ª ½Ð· ReactOS Ô  ë·¢á Ô·½ áÖó¦ŠÐ. ì·¢¨áŠåí",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "¦·á¨ÆåÖáŠœ, ë ÞÆç ì· µÖû¨å¨ ëãå ÔÖë·å· ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "úÖ¢ óÒŠÔ·å· ¦·á¨ÆåÖáŠœ Ô å·ãÔŠåí BACKSPACE ¦ÐÞ ë·¦ Ð¨ÔÔÞ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "ã·ÒëÖÐŠë ØŠãÐÞ ûÖ¬Ö ëë¨¦Šåí Ô óëç ¦·á¨ÆåÖáŠŒ ¦ÐÞ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "ëãå ÔÖëÐ¨ÔÔÞ ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFileCopyEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "£ç¦í-Ð ãÆ , ó û¨Æ ½å¨ ØÖÆ· ëãå ÔÖëÐœë û ReactOS ÆÖØŠœ† ª ½Ð· ¦Ö",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "Ø ØÆ· Øá·óÔ û¨ÔÔÞ.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "¥¨ ÒÖé¨ ó ½ÔÞå· ¦¨ÆŠÐíÆ  µë·Ð·Ô.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 £ç¦í-Ð ãÆ  ó û¨Æ ½å¨...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUABootLoaderEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ìãå ÔÖëÐœë û ëãå ÔÖëÐœ† boot loader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "ìãå ÔÖë·å· bootloader Ô  éÖáãåÆ·½ ¦·ãÆ (bootsector).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "ìãå ÔÖë·å· bootloader Ô  ¦·ãÆ¨åç.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Õ¨ ëãå ÔÖëÐœë å· bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "æçå ì· ÒÖé¨å¨ óÒŠÔ·å· å·Ø ÆÐ ëŠ åçá·.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Õ å·ãÆ ½å¨ ÆÐ ëŠõŠ ìì©â¶ å  ìÕ¸ô ¦ÐÞ ë·¢Öáç ØÖåáŠ¢ÔÖ¬Ö å·Øç ÆÐ ëŠ åçá·.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Š Ô å·ãÔŠåí ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Õ å·ãÔŠåí ESC ¦ÐÞ ØÖë¨áÔ¨ÔÔÞ Ô  ØÖØ¨á¨¦Ôœ ãåÖáŠÔÆç ¢¨ó óÒŠÔ·",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   å·Øç ÆÐ ëŠ åçá·.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   ESC = ìŠ¦ÒŠÔ·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUALayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ì·¢¨áŠåí áÖóÆÐ ¦Æç, ÞÆ  ¢ç¦¨ ëãå ÔÖëÐ¨Ô  ÞÆ  ãå Ô¦ áåÔ .",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Õ å·ãÆ ½å¨ ÆÐ ëŠõŠ ìì©â¶ å  ìÕ¸ô ¦ÐÞ ë·¢Öáç ØÖåáŠ¢ÔÖŒ áÖóÆÐ ¦Æ·",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    ÆÐ ëŠ åçá· Š Ô å·ãÔŠåí ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Õ å·ãÔŠåí ESC ¦ÐÞ ØÖë¨áÔ¨ÔÔÞ Ô  ØÖØ¨á¨¦Ôœ ãåÖáŠÔÆç ¢¨ó óÒŠÔ·",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   áÖóÆÐ ¦Æ· ÆÐ ëŠ åçá·.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   ESC = ìŠ¦ÒŠÔ·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ukUAPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ìãå ÔÖëÐœë û ¬Öåç† ì õ ÆÖÒØ'œå¨á ¦ÐÞ ÆÖØŠœë ÔÔÞ ª ½ÐŠë ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "­¨Ô¨áçœ ãØ·ãÖÆ ª ½ÐŠë...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ukUASelectFSEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "ì·¢¨áŠåí ª ½ÐÖëç ã·ãå¨Òç óŠ ãØ·ãÆç Ô·éû¨.",
        0
    },
    {
        8,
        19,
        "\x07  Õ å·ãÆ ½å¨ ÆÐ ëŠõŠ ìì©â¶ å  ìÕ¸ô ¦ÐÞ ë·¢Öáç ª ½ÐÖëÖŒ ã·ãå¨Ò·.",
        0
    },
    {
        8,
        21,
        "\x07  Õ å·ãÔŠåí ENTER ùÖ¢ ëŠ¦ªÖáÒ åçë å· áÖó¦ŠÐ.",
        0
    },
    {
        8,
        23,
        "\x07  Õ å·ãÔŠåí ESC ¦ÐÞ ë·¢Öáç ŠÔõÖ¬Ö áÖó¦ŠÐç.",
        0
    },
    {
        0,
        0,
        "ENTER = ÝáÖ¦Öëé·å·   ESC = ìŠ¦ÒŠÔ·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADeletePartitionEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ì· ë·¢á Ð· ë·¦ Ð¨ÔÔÞ áÖó¦ŠÐç",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Õ å·ãÔŠåí D ¦ÐÞ ë·¦ Ð¨ÔÔÞ áÖó¦ŠÐç.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "èì¡­¡: ìãŠ ¦ ÔŠ Ô  ¤íÖÒç áÖó¦ŠÐŠ ¢ç¦çåí ëåá û¨ÔŠ!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Õ å·ãÔŠåí ESC ¦ÐÞ ëŠ¦ŠÒÔ·.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = ì·¦ Ð·å· âÖó¦ŠÐ   ESC = ìŠ¦ÒŠÔ·å·   F3 = ì·½å·",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUARegistryEntries[] =
{
    {
        4,
        3,
        " ìãå ÔÖëÐ¨ÔÔÞ ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ìãå ÔÖëÐœë û ÖÔÖëÐœ† ÆÖÔªŠ¬çá ¤Šœ ã·ãå¨Ò·. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "äåëÖáœœ ãåáçÆåçáç á¨†ãåáç...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR ukUAErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS Ô¨ ¢çë ØÖëÔŠãåœ ëãå ÔÖëÐ¨Ô·½ Ô  ì õ\n"
        "ÆÖÒØ'œå¨á. àÆùÖ ë· ë·½¦¨å¨ ó ëãå ÔÖëÐœë û  ó á ó,\n"
        "åÖ ì Ò ¢ç¦¨ Ô¨Ö¢µŠ¦ÔÖ ó Øçãå·å· ØáÖ¬á Òç ëãå ÔÖëÐ¨ÔÔÞ\n"
        "óÔÖëç, ÞÆùÖ ì· µÖû¨å¨ ëãå ÔÖë·å· ReactOS,\n"
        "\n"
        "  \x07  Õ å·ãÔŠåí ENTER ùÖ¢ ØáÖ¦Öëé·å· ëãå ÔÖëÐ¨ÔÔÞ.\n"
        "  \x07  Õ å·ãÔŠåí F3 ¦ÐÞ ë·µÖ¦ç ó ëãå ÔÖëÐœë û .",
        "F3 = ì·½å·  ENTER = ÝáÖ¦Öëé·å·"
    },
    {
        //ERROR_NO_HDD
        "Õ¨ ë¦ ÐÖãí óÔ ½å· éÖáãåÆ·½ ¦·ãÆ.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Õ¨ ë¦ ÐÖãí óÔ ½å· çãå ÔÖëÖûÔ·½ ¦·ãÆ.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Õ¨ ë¦ ÐÖãí ó ë Ôå é·å· ª ½Ð TXTSETUP.SIF.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "« ½Ð TXTSETUP.SIF ØÖõÆÖ¦é¨Ô·½.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "ì·ÞëÐ¨ÔÖ Ô¨ÆÖá¨ÆåÔ·½ ØŠ¦Ø·ã ë TXTSETUP.SIF.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Õ¨ ë¦ ÐÖãí Öåá·Ò å· ¦ ÔŠ ØáÖ ã·ãå¨ÒÔ·½ ¦·ãÆ.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_WRITE_BOOT,
        "Õ¨ ë¦ ÐÖãí ëãå ÔÖë·å· ó ë Ôå éçë ÐíÔ·½ ÆÖ¦ FAT Ô  ã·å¨ÒÔ·½ áÖó¦ŠÐ.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Õ¨ ë¦ ÐÖãí ó ë Ôå é·å· ãØ·ãÖÆ å·ØŠë ÆÖÒØ'œå¨á .\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Õ¨ ë¦ ÐÖãí ó ë Ôå é·å· ãØ·ãÖÆ á¨é·ÒŠë ¨Æá Ôç.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Õ¨ ë¦ ÐÖãí ó ë Ôå é·å· ãØ·ãÖÆ å·ØŠë ÆÐ ëŠ åçá·.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Õ¨ ë¦ ÐÖãí ó ë Ôå é·å· ãØ·ãÖÆ áÖóÆÐ ¦ÖÆ ÆÐ ëŠ åçá·.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_WARN_PARTITION,
          "ôÔ ½¦¨ÔÖ ÞÆ ÒŠÔŠÒçÒ Ö¦·Ô éÖáãåÆ·½ ¦·ãÆ, ùÖ ÒŠãå·åí áÖó¦ŠÐ,\n"
          "ÞÆ·½ Ô¨ ØŠ¦åá·Òç†åíãÞ ReactOS!\n"
          "\n"
          "äåëÖá¨ÔÔÞ û· ë·¦ Ð¨ÔÔÞ áÖó¦ŠÐŠë ÒÖé¨ óáç½Ôçë å· å ¢Ð·¤œ áÖó¦ŠÐŠë.\n"
          "\n"
          "  \x07  Õ å·ãÔŠåí F3 ¦ÐÞ ë·µÖ¦ç ó ëãå ÔÖëÐœë û .\n"
          "  \x07  Õ å·ãÔŠåí ENTER ùÖ¢ ØáÖ¦Öëé·å·.",
          "F3= ì·½å·  ENTER = ÝáÖ¦Öëé·å·"
    },
    {
        //ERROR_NEW_PARTITION,
        "ì· Ô¨ ÒÖé¨å¨ ãåëÖá·å· ÔÖë·½ áÖó¦ŠÐ Ô \n"
        "ëé¨ ŠãÔçœûÖÒç áÖó¦ŠÐŠ!\n"
        "\n"
        "  * Õ å·ãÔŠåí ¢ç¦í-ÞÆç ÆÐ ëŠõç ùÖ¢ ØáÖ¦Öëé·å·.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Õ¨ ÒÖéÔ  ë·¦ Ð·å· Ô¨áÖóÒŠû¨Ôç Ö¢Ð ãåí Ô  ¦·ãÆç!\n"
        "\n"
        "  * Õ å·ãÔŠåí ¢ç¦í-ÞÆç ÆÐ ëŠõç ùÖ¢ ØáÖ¦Öëé·å·.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Õ¨ ë¦ ÐÖãí ëãå ÔÖë·å· ó ë Ôå éçë ÐíÔ·½ ÆÖ¦ FAT Ô  ã·å¨ÒÔ·½ áÖó¦ŠÐ.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_NO_FLOPPY,
        "ìŠ¦ãçåÔÞ ¦·ãÆ¨å  ë ¦·ãÆÖëÖ¦Š A:.",
        "ENTER = ÝáÖ¦Öëé·å·"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Õ¨ ë¦ ÐÖãí ÖÔÖë·å· Ø á Ò¨åá· áÖóÆÐ ¦Æ· ÆÐ ëŠ åçá·.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Õ¨ ë¦ ÐÖãí ÖÔÖë·å· Ø á Ò¨åá· ¨Æá Ôç ë á¨†ãåáŠ.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Õ¨ ë¦ ÐÖãí ŠÒØÖáåçë å· ª ½Ð ÆçùŠë á¨†ãåáç.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_FIND_REGISTRY
        "Õ¨ ë¦ ÐÖãí óÔ ½å· ª ½Ð· ¦ Ô·µ á¨†ãåáç.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_CREATE_HIVE,
        "Õ¨ ë¦ ÐÖãí ãåëÖá·å· ÆçùŠ á¨†ãåáç.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Õ¨ ë¦ ÐÖãí ŠÔŠ¤Š ÐŠóçë å· á¨†ãåá.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet Ò † Ô¨ÆÖá¨ÆåÔ·½ inf-ª ½Ð.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet Ô¨ óÔ ½¦¨ÔÖ.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet Ô¨ Ò † çãå ÔÖëÖûÔÖ¬Ö ã¤¨Ô áŠœ.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_COPY_QUEUE,
        "Õ¨ ë¦ ÐÖãí ëŠ¦Æá·å· û¨á¬ç ÆÖØŠœë ÔÔÞ ª ½ÐŠë.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_CREATE_DIR,
        "Õ¨ ë¦ ÐÖãí ãåëÖá·å· ¦·á¨ÆåÖáŠŒ ¦ÐÞ ëãå ÔÖëÐ¨ÔÔÞ.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Õ¨ ë¦ ÐÖãí óÔ ½å· ã¨Æ¤Šœ 'Directories'\n"
        "ë ª ½ÐŠ TXTSETUP.SIF.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_CABINET_SECTION,
        "Õ¨ ë¦ ÐÖãí óÔ ½å· ã¨Æ¤Šœ 'Directories'\n"
        "ë cabinet.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Õ¨ ë¦ ÐÖãí ãåëÖá·å· ¦·á¨ÆåÖáŠœ ¦ÐÞ ëãå ÔÖëÐÔÔÞ.",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Õ¨ ë¦ ÐÖãí óÔ ½å· ã¨Æ¤Šœ 'SetupData'\n"
        "ë ª ½ÐŠ TXTSETUP.SIF.\n",
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Õ¨ ë¦ ÐÖãí ó Ø·ã å· å ¢Ð·¤Š áÖó¦ŠÐŠë.\n"
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Õ¨ ë¦ ÐÖãí ¦Ö¦ å· Ø á Ò¨åá· ÆÖ¦çë ÔÔÞ ë á¨†ãåá.\n"
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Õ¨ ë¦ ÐÖãí ëãå ÔÖë·å· ÐÖÆ Ðí ã·ãå¨Ò·.\n"
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Õ¨ ë¦ ÐÖãí ¦Ö¦ å· áÖóÆÐ ¦Æ· ÆÐ ëŠ åçá· ¦Ö á¨†ãåáç.\n"
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Õ¨ ë¦ ÐÖãí ëãå ÔÖë·å· geo id.\n"
        "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE ukUAPages[] =
{
    {
        LANGUAGE_PAGE,
        ukUALanguagePageEntries
    },
    {
        START_PAGE,
        ukUAWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        ukUAIntroPageEntries
    },
    {
        LICENSE_PAGE,
        ukUALicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        ukUADevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        ukUARepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        ukUAComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        ukUADisplayPageEntries
    },
    {
        FLUSH_PAGE,
        ukUAFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        ukUASelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        ukUASelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        ukUAFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        ukUADeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        ukUAInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        ukUAPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        ukUAFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        ukUAKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        ukUABootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        ukUALayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        ukUAQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        ukUASuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        ukUABootPageEntries
    },
    {
        REGISTRY_PAGE,
        ukUARegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING ukUAStrings[] =
{
    {STRING_PLEASEWAIT,
     "   £ç¦í-Ð ãÆ , ó û¨Æ ½å¨..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = ìãå ÔÖë·å·   C = äåëÖá·å· âÖó¦ŠÐ   F3 = ì·½å·"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = ìãå ÔÖë·å·   D = ì·¦ Ð·å· âÖó¦ŠÐ   F3 = ì·½å·"},
    {STRING_PARTITIONSIZE,
     "âÖóÒŠá ÔÖëÖ¬Ö áÖó¦ŠÐç:"},
    {STRING_CHOOSENEWPARTITION,
     "ì· µÖû¨å¨ ãåëÖá·å· ÔÖë·½ áÖó¦ŠÐ Ô "},
    {STRING_HDDSIZE,
    "£ç¦í-Ð ãÆ , ëë¨¦Šåí áÖóÒŠá ÔÖëÖ¬Ö áÖó¦ŠÐç ë Ò¨¬ ¢ ½å µ."},
    {STRING_CREATEPARTITION,
     "   ENTER = äåëÖá·å· âÖó¦ŠÐ   ESC = ìŠ¦ÒŠÔ·å·   F3 = ì·½å·"},
    {STRING_PARTFORMAT,
    "¥¨½ áÖó¦ŠÐ ¢ç¦¨ ëŠ¦ªÖáÒ åÖë ÔÖ."},
    {STRING_NONFORMATTEDPART,
    "ì· ë·¢á Ð· ëãå ÔÖëÐ¨ÔÔÞ ReactOS Ô  ÔÖë·½  ¢Ö Ô¨ªÖáÒ åÖë Ô·½ áÖó¦ŠÐ."},
    {STRING_INSTALLONPART,
    "ReactOS ëãå ÔÖëÐœ†åíãÞ Ô  áÖó¦ŠÐ"},
    {STRING_CHECKINGPART,
    "ìãå ÔÖëÐœë û Ø¨á¨ëŠáÞ† ë·¢á Ô·½ áÖó¦ŠÐ."},
    {STRING_QUITCONTINUE,
    "F3= ì·½å·  ENTER = ÝáÖ¦Öëé·å·"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"},
    {STRING_TXTSETUPFAILED,
    "ìãå ÔÖëÐœë û Ô¨ óÒŠ¬ óÔ ½å· ã¨Æ¤Šœ '%S' \në ª ½ÐŠ TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "\xB3 ÇÖØŠœë ÔÔÞ: %S"},
    {STRING_SETUPCOPYINGFILES,
     "ìãå ÔÖëÐœë û ÆÖØŠœ† ª ½Ð·..."},
    {STRING_REGHIVEUPDATE,
    "   ×ÔÖëÐ¨ÔÔÞ ÆçùŠë á¨†ãåáç..."},
    {STRING_IMPORTFILE,
    "   ‹ÒØÖáåçë ÔÔÞ %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   ×ÔÖëÐ¨ÔÔÞ Ø á Ò¨åáŠë ¨Æá Ôç ë á¨†ãåáŠ..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   ×ÔÖëÐ¨ÔÔÞ Ø á Ò¨åáŠë ÐÖÆ ÐŠ..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   ×ÔÖëÐ¨ÔÔÞ Ø á Ò¨åáŠë áÖóÆÐ ¦Æ· ÆÐ ëŠ åçá·..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   §Ö¦ ë ÔÔÞ ¦ Ô·µ ØáÖ ÆÖ¦Öëç ãåÖáŠÔÆç ë á¨†ãåá..."},
    {STRING_DONE,
    "   ­ÖåÖëÖ..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Ý¨á¨ó ë Ôå é·å· ÆÖÒØ'œå¨á"},
    {STRING_CONSOLEFAIL1,
    "Õ¨ ë¦ ÐÖãí ëŠ¦Æá·å· ÆÖÔãÖÐí\n\n"},
    {STRING_CONSOLEFAIL2,
    "Õ ½¢ŠÐíõ ½ÒÖëŠáÔ  Øá·û·Ô  ¤íÖ¬Ö -  ë·ÆÖá·ãå ÔÔÞ USB ÆÐ ëŠ åçá·\n"},
    {STRING_CONSOLEFAIL3,
    "USB ÆÐ ëŠ åçá· ù¨ Ô¨ ØŠ¦åá·ÒçœåíãÞ ØÖëÔŠãåœ\n"},
    {STRING_FORMATTINGDISK,
    "ìãå ÔÖëÐœë û ªÖáÒ åç† ë õ ¦·ãÆ"},
    {STRING_CHECKINGDISK,
    "ìãå ÔÖëÐœë û Ø¨á¨ëŠáÞ† ë õ ¦·ãÆ"},
    {STRING_FORMATDISK1,
    " «ÖáÒ åçë å· áÖó¦ŠÐ ë ª ½ÐÖëŠ½ ã·ãå¨ÒŠ %S (õë·¦Æ¨ ªÖáÒ åçë ÔÔÞ) "},
    {STRING_FORMATDISK2,
    " «ÖáÒ åçë å· áÖó¦ŠÐ ë ª ½ÐÖëŠ½ ã·ãå¨ÒŠ %S  "},
    {STRING_KEEPFORMAT,
    " ô Ð·õ·å· ŠãÔçœûç ª ½ÐÖëç ã·ãå¨Òç (¢¨ó óÒŠÔ) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  êÖáãåÆ·½ ¦·ãÆ %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  êÖáãåÆ·½ ¦·ãÆ %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "Ô  %I64u %s  êÖáãåÆ·½ ¦·ãÆ %lu  (ÝÖáå=%hu, ö·Ô =%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "Ô  %I64u %s  êÖáãåÆ·½ ¦·ãÆ %lu  (ÝÖáå=%hu, ö·Ô =%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "êÖáãåÆ·½ ¦·ãÆ %lu (%I64u %s), ÝÖáå=%hu, ö·Ô =%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "Ô  êÖáãåÆÖÒç ¦·ãÆç %lu (%I64u %s), ÝÖáå=%hu, ö·Ô =%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Type %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  êÖáãåÆ·½ ¦·ãÆ %lu  (Port=%hu, Bus=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  êÖáãåÆ·½ ¦·ãÆ %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "ìãå ÔÖëÐœë û ãåëÖá·ë ÔÖë·½ áÖó¦ŠÐ Ô "},
    {STRING_UNPSPACE,
    "    Õ¨áÖóÒŠû¨Ô  Ö¢Ð ãåí              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (Ò Æã. %lu MB)"},
    {STRING_UNFORMATTED,
    "ÕÖë·½ (Õ¨ªÖáÒ åÖë Ô·½)"},
    {STRING_FORMATUNUSED,
    "Õ¨ ë·ÆÖá·ãå ÔÖ"},
    {STRING_FORMATUNKNOWN,
    "Õ¨ëŠ¦ÖÒÖ"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "§Ö¦ ë ÔÔÞ áÖóÆÐ ¦ÖÆ ÆÐ ëŠ åçá·"},
    {0, 0}
};

#endif
