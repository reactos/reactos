#ifndef LANG_JA_JP_H__
#define LANG_JA_JP_H__

MUI_LAYOUTS jaJPLayouts[] =
{
//    { L"0411", L"e0010411" },
    { L"0411", L"00000411" },
    { NULL, NULL }
};

static MUI_ENTRY jaJPLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "¹ÞÝºÞÉ ¾ÝÀ¸",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ²Ý½Ä°Ù¼ÞÆ ¼Ö³½Ù ¹ÞÝºÞ¦ ¾ÝÀ¸ ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Â·ÞÆ¤ ENTER ·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ººÃÞ ¾ÝÀ¸ ¼À ¹ÞÝºÞÊ »²¼­³Ã·Æ ¼½ÃÑÉ ·Ã²É ¹ÞÝºÞÆ ¾¯Ã²»ÚÏ½¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³  F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS ¾¯Ä±¯ÌßÍ Ö³º¿",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ºÉ ¾¯Ä±¯ÌßÉ ÀÞÝ¶²ÃÞÊ ReactOS µÍßÚ°Ã¨Ý¸Þ¼½ÃÑ¦",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "ºÝËß­°ÀÆ ºËß°¼¤ ¾¯Ä±¯ÌßÉ Â·ÞÉ ÀÞÝ¶²ÍÉ ¼Þ­ÝËÞ¦ ¼Ï½¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ReactOS¦ ²Ý½Ä°Ù ½ÙÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ReactOS¦ ¼­³Ì¸ Ó¼¸Ê º³¼Ý ½ÙÆÊ R·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ReactOSÉ ×²¾Ý½¼Þ®³¹Ý ¦Ë®³¼Þ ½ÙÆÊ L·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ReactOS¦ ²Ý½Ä°Ù¾½ÞÆ Á­³¼½Ù ÊÞ±²Ê F3·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "ReactOSÉ ¼®³»²Å ¼Þ®³Î³Æ ¶Ý¼ÃÊ ¶·¦ ºÞ×Ý¸ÀÞ»²:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³  R = ¼­³Ì¸  L = ×²¾Ý½  F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS ¾¯Ä±¯ÌßÊ ¼®· ¶²ÊÂ ÀÞÝ¶²Æ ±ØÏ½¡ ¿ÉÀÒ¤ ÏÀÞ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "¼Þ­³ÌÞÝÆ ØÖ³ÃÞ·Ù ¾¯Ä±¯Ìß±ÌßØ¹°¼®Ý É ½ÍÞÃÉ·É³Ê »Îß°Ä »ÚÏ¾Ý¡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Â·ÞÉ ¾²Ô¸¶Þ Ã·Ö³ »ÚÏ½:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- ¾¯Ä±¯ÌßÊ 1ÂÉ ÃÞ¨½¸Æ Â· 1Â²¼Þ®³É Ìß×²ÏØ Êß°Ã¨¼®Ý¦ ±Â¶³ºÄÊ ÃÞ·Ï¾Ý¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- ¶¸Á®³ Êß°Ã¨¼®Ý ¶Þ ºÉÃÞ¨½¸¼Þ®³Æ ¿Ý»Þ²½Ù ÊÞ±²¤ ¾¯Ä±¯ÌßÊ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  Ìß×²ÏØ Êß°Ã¨¼®Ý¦ ÃÞ¨½¸¶× »¸¼Þ® ÃÞ·Ï¾Ý¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- ÀÉ ¶¸Á®³ Ø®³²·¶Þ ºÉ ÃÞ¨½¸¼Þ®³Æ ¿Ý»Þ² ¼Ã²Ù ÊÞ±²¤ ¾¯Ä±¯ÌßÊ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  »²¼®É ¶¸Á®³ Ø®³²·¦ ÃÞ¨½¸¶× »¸¼Þ® ÃÞ·Ï¾Ý¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- ¾¯Ä±¯ÌßÊ FAT Ì§²Ù¼½ÃÑ ÉÐ »Îß°Ä ¼Ï½¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- Ì§²Ù¼½ÃÑÉ Áª¯¸·É³Ê ÏÀÞ ¼Þ¯¿³ »ÚÃ ²Ï¾Ý¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  ReactOS¦ ²Ý½Ä°Ù ½ÙÆÊ ENTER ·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  ReactOS¦ ²Ý½Ä°Ù¾½ÞÆ Á­³¼ ½Ù ÊÞ±²Ê F3·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   F3 = Á­³¼",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "¼Ö³ ·®ÀÞ¸:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ºÉ ReactOS ¼½ÃÑÊ ¸Ð±Ü¾ ¶É³Å ×²¾Ý½(X11Ô¤BSD µÖËÞ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU LGPL×²¾Ý½ ÅÄÞ) É º°ÄÞ¦ Ì¸Ñ Êß°ÂÄ ÄÓÆ GNU GPLÉ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "¼Þ®³¹Ý É ÓÄÆ ×²¾Ý½ »ÚÃ²Ï½¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "ReactOS ¼½ÃÑÉ ²ÁÌÞÉ ½ÍÞÃÉ ¿ÌÄ³ª±Ê ReactOS ¼½ÃÑÉ ²ÁÌÞÃÞ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "±ÙÕ´Æ GNU GPLÀÞ¹ÃÞÅ¸¤ ¿É ¿ÌÄ³ª±É µØ¼ÞÅÙ×²¾Ý½É ÓÄÆÓ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "ØØ°½ »ÚÃ ²Ï½¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "ºÉ ¿ÌÄ³ª±Ê 'ÑÎ¼®³' ÃÞ Ã³·®³ »Ú¤ Á²·Î³Ô º¸»²Î³¶Þ Ã·µ³»ÚÙ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "ÊÞ±²¦ É¿Þ²Ã¤ ØÖ³ ¾²¹ÞÝ¦ ³¹Ï½¡ ReactOSÉ ×²¾Ý½ ·®³ÖÊ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ÀÞ²»Ý¼¬ÍÉ ÊÝÊÞ²¦ Ì¾¸Þ ÀÞ¹ ÃÞ½¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "GNU General Public License¦ ReactOSÄ ÄÓÆ ³¹Ä×Å¶¯À",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "ÊÞ±²Ê¤ Â·Þ¦ ºÞ»Ý¼®³ ¸ÀÞ»²",
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
        "ËÝ¼Â Î¼®³:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "ºÚÊ ÌØ°¿ÌÄ³ª± ÃÞ½¡ ¼®³»²Ê ¿°½É ºËß°¼Þ®³¹Ý¦ ºÞ×Ý ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "Î¼®³Ê Ï¯À¸ '±ØÏ¾Ý'¡ '¼¼Þ®³¾²' Ô 'Ã·ºÞ³¾²'",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "Æµ²ÃÓ ÄÞ³Ö³ ÃÞ½¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ÓÄÞÙ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "²¶É Ø½ÄÊ ¹ÞÝ»Þ²É ÃÞÊÞ²½ ¾¯Ã²ÃÞ½¡",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "ºÝËß­°À:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "ÃÞ¨½ÌßÚ²:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "·°ÎÞ°ÄÞ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "·°ÎÞ°ÄÞ Ú²±³Ä:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "¼Þ­ÀÞ¸:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "ºÚ×É ÃÞÊÞ²½ ¾¯Ã²¦ ¼Þ­ÀÞ¸ ½Ù",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "UP Ó¼¸Ê DOWN·°¦ µ¼Ã ´ÝÄØ°¦ ¾ÝÀ¸ ½Ù ºÄÆÖØ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "Ê°ÄÞ³ª±É ¾¯Ã²¦ ÍÝº³ ÃÞ·Ï½¡ ¾ÝÀ¸ ¼À×¤ ENTER·°¦ µ¼Ã Ã·¾ÂÅ ¾¯Ã²¦",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "¾ÝÀ¸ ¼Ï½¡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "ºÚ×É ¾¯Ã²¶Þ ½ÍÞÃ ÀÀÞ¼² ÊÞ±²Ê¤ \"ºÚ×É ÃÞÊÞ²½ ¾¯Ã²¦ ¼Þ­ÀÞ¸ ½Ù\" ¦ ¾ÝÀ¸ ¼Ã¤",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS ¾¯Ä±¯ÌßÊ ¼®·¶²ÊÂ ÀÞÝ¶²Æ ±ØÏ½¡ ¿ÉÀÒ¤ ÏÀÞ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "¼Þ­³ÌÞÝÆ ØÖ³ ÃÞ·Ù ¾¯Ä±¯Ìß ±ÌßØ¹°¼®ÝÉ ½ÍÞÃÉ ·É³Ê »Îß°Ä »ÚÏ¾Ý¡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "¼­³Ì¸·É³Ê ÏÀÞ ¼Þ¯¿³ »ÚÃ ²Ï¾Ý¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  OS¦ º³¼Ý ½Ù ÆÊ U·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ¶²Ì¸ ºÝ¿°Ù¦ Ë×¸ÆÊ R·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Ò²ÝÍß°¼ÞÆ ÓÄÞÙ ÆÊ ESC·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ºÝËß­°À¦ »²·ÄÞ³ ½ÙÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = Ò²ÝÍß°¼Þ  U = º³¼Ý  R = ¶²Ì¸  ENTER = »²·ÄÞ³",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY jaJPComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "²Ý½Ä°Ù »ÚÙ ºÝËß­°ÀÉ ¼­Ù²¦ ÍÝº³ ½Ù ºÄ¶Þ ¾ÝÀ¸ »ÚÏ¼À¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  UP Ó¼¸Ê DOWN·°¦ µ¼Ã Ã·½Ù ºÝËß­°ÀÉ ¼­Ù²¦ ¾ÝÀ¸ ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Â·ÞÆ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ºÝËß­°ÀÉ ¼­Ù²¦ ÍÝº³¾½ÞÆ Ï´É Íß°¼ÞÆ ÓÄÞÙ ÊÞ±²Ê ESC·°¦",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   ESC = ·¬Ý¾Ù   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "¼½ÃÑÊ ¶¸¼ÞÂÆ ½ÍÞÃÉ ½ÍÞÃÉ ÃÞ°À¶Þ ÃÞ¨½¸¼Þ®³Æ Î¿ÞÝ »ÚÙ Ö³Æ ¼Ã²Ï½",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ºÚÆÊ ¼®³¼®³ ¼Þ¶Ý¶Þ ¶¶Ù ÊÞ±²¶Þ ±ØÏ½",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "¶ÝØ®³ºÞ¤ ºÝËß­°ÀÊ ¼ÞÄÞ³ÃÆ »²·ÄÞ³ »ÚÏ½",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "·¬¯¼­¦ ¼®³·® ¼Ã ²Ï½",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOSÊ Ï¯À¸ ²Ý½Ä°Ù »ÚÏ¾Ý",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ÄÞ×²ÌÞ A: ¶× ÌÛ¯Ëß° ÃÞ¨½¸¤ CD ÄÞ×²ÌÞ ¶×",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "½ÍÞÃÉ CD-ROM¦ ÄØÀÞ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ºÝËß­°À¦ »²·ÄÞ³ ½ÙÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "µÏÁ¸ÀÞ»² ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "²Ý½Ä°Ù »ÚÙ ÃÞ¨½ÌßÚ²É ¼­Ù²¦ ÍÝº³ ½Ù ºÄ¶Þ ¾ÝÀ¸ »ÚÏ¼À¡",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  UP Ó¼¸Ê DOWN·°¦ µ¼Ã Ã·½Ù ÃÞ¨½ÌßÚ²É ¼­Ù²¦ ¾ÝÀ¸ ¼Ã ¸ÀÞ»²¡",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Â·ÞÆ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ÃÞ¨½ÌßÚ²É ¼­Ù²¦ ÍÝº³¾½ÞÆ Ï´É Íß°¼ÞÆ ÓÄÞÙÆÊ ESC·°¦",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   ESC = ·¬Ý¾Ù   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOSÉ ·ÎÝÌÞÌÞÝÉ ²Ý½Ä°ÙÊ ¾²º³ ¼Ï¼À¡",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ÄÞ×²ÌÞ A: ¶× ÌÛ¯Ëß° ÃÞ¨½¸¤ CD ÄÞ×²ÌÞ ¶×",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "½ÍÞÃÉ CD-ROM¦ ÄØÀÞ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ºÝËß­°À¦ »²·ÄÞ³ ½Ù ÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "¾¯Ä±¯ÌßÊ ÌÞ°ÄÛ°ÀÞ¦ ºÝËß­°ÀÉ Ê°ÄÞÃÞ¨½¸ ¼Þ®³ Æ ²Ý½Ä°Ù",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ÃÞ·Ï¾ÝÃÞ¼À",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "ÄÞ×²ÌÞ A: Æ Ì«°Ï¯Ä»ÚÀ ÌÛ¯Ëß° ÃÞ¨½¸¦ ²ÚÃ¤",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY jaJPSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "²¶É Ø½ÄÊ ¿Ý»Þ²½Ù Êß°Ã¨¼®ÝÄ ¼Ý· Êß°Ã¨¼®ÝÆ Ã·¼À Ð¼Ö³É",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ÃÞ²½¸ ½Íß°½É ²Á×Ý ÃÞ½¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  UP Ó¼¸Ê DOWN·°¦ µ¼Ã Ø½Ä´ÝÄØ°¦ ¾ÝÀ¸ ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ¾ÝÀ¸»ÚÀ Êß°Ã¨¼®ÝÆ ReactOS¦ ²Ý½Ä°Ù½Ù ÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ±À×¼² Êß°Ã¨¼®Ý ¦ »¸¾²½Ù ÆÊ C·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ·¿ÝÉ Êß°Ã¨¼®Ý¦ »¸¼Þ®½Ù ÆÊ D·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "µÏÁ¸ÀÞ»²...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Êß°Ã¨¼®ÝÉ Ì«°Ï¯Ä",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "¾¯Ä±¯ÌßÊ Êß°Ã¨¼®Ý¦ Ì«°Ï¯Ä ¼Ï½¡ ¿Þ¯º³ ½ÙÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY jaJPInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "¾¯Ä±¯ÌßÊ ReactOSÉ Ì§²Ù¦ ¾ÝÀ¸ »ÚÀ Êß°Ã¨¼®Ý ¼Þ®³Æ ²Ý½Ä°Ù¼Ï½¡ ReactOS¦",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "²Ý½Ä°Ù½Ù ÃÞ¨Ú¸ÄØ¦ ¾¯Ã² ¼Ã ¸ÀÞ»²:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "½²¼®³ ÃÞ¨Ú¸ÄØ¦ ÍÝº³½ÙÆÊABACKSPACE·°ÃÞ »¸¼Þ®¼À ±ÄÆ¤",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "ReactOS¦ ²Ý½Ä°Ù ½Ù ÃÞ¨Ú¸ÄØ¦ Æ­³Ø®¸",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "ReactOS ¾¯Ä±¯Ìß¶Þ ReactOS ²Ý½Ä°Ù Ì«ÙÀÞÆ Ì§²Ù¦",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "ºËß°½Ù ±²ÀÞ ¼ÊÞ×¸ µÏÁ ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "¶ÝØ®³ÏÃÞÆ ½³ÌÝ ¶¶Ù ÊÞ±²¶Þ ±ØÏ½¡",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 µÏÁ ¸ÀÞ»²...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "¾¯Ä±¯ÌßÊ ÌÞ°ÄÛ°ÀÞ¦ ²Ý½Ä°Ù ¼Ï½",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "ÌÞ°ÄÛ°ÀÞ¦ Ê°ÄÞÃÞ¨½¸ (MBR) Æ ²Ý½Ä°Ù ½Ù¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "ÌÞ°ÄÛ°ÀÞ¦ ÌÛ¯Ëß° ÃÞ¨½¸Æ ²Ý½Ä°Ù ½Ù¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "ÌÞ°ÄÛ°ÀÞÉ ²Ý½Ä°Ù¦ ½·¯Ìß ½Ù¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "²Ý½Ä°Ù »ÚÙ ·°ÎÞ°ÄÞÉ ¼­Ù²¦ ÍÝº³½Ù ºÄ¶Þ ¾ÝÀ¸ »ÚÏ¼À¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  UP Ó¼¸Ê DOWN·°¦ µ¼Ã Ã·½Ù ·°ÎÞ°ÄÞÉ ¼­Ù²¦ ¾ÝÀ¸¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Â·ÞÆ ENTER·°¦ µ¼Ã¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ·°ÎÞ°ÄÞÉ ¼­Ù²¦ ÍÝº³ ¾½ÞÆ Ï´É Íß°¼ÞÆ ÓÄÞÙÆÊ ESC·°¦",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   ESC = ·¬Ý¾Ù   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "²Ý½Ä°Ù»ÚÙ ·ÄÃ²É Ú²±³Ä¦ ¾ÝÀ¸¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  UP Ó¼¸Ê DOWN·°¦ µ¼Ã Ã·½Ù ·°ÎÞ°ÄÞ Ú²±³Ä¦ ¾ÝÀ¸¼Ã¤",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    Â·ÞÆ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ·°ÎÞ°ÄÞ Ú²±³Ä¦ ÍÝº³ ¾½ÞÆ Ï´É Íß°¼ÞÆ ÓÄÞÙÆÊ ESC·°¦",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   ESC = ·¬Ý¾Ù   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY jaJPPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "¾¯Ä±¯ÌßÊ ReactOSÉÌ§²Ù¦ ºÝËß­°ÀÆ ºËß°½Ù ¼Þ­ÝËÞ¦ ¼Ã ²Ï½¡ ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ºËß°½Ù Ì§²Ù Ø½Ä¦ »¸¾² Á­³...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY jaJPSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "¼ÀÉ Ø½Ä¶× Ì§²Ù ¼½ÃÑ¦ ¾ÝÀ¸ ¼Ã ¸ÀÞ»²¡",
        0
    },
    {
        8,
        19,
        "\x07  UP Ó¼¸Ê DOWN·°¦ µ¼Ã Ì§²Ù ¼½ÃÑ¦ ¾ÝÀ¸¼Ã ¸ÀÞ»²¡",
        0
    },
    {
        8,
        21,
        "\x07  Êß°Ã¨¼®Ý¦ Ì«°Ï¯Ä½ÙÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
        0
    },
    {
        8,
        23,
        "\x07  ÍÞÂÉ Êß°Ã¨¼®Ý¦ ¾ÝÀ¸½Ù ÊÞ±²¤ ESC·°¦ µ¼Ã ¸ÀÞ»²¡",
        0
    },
    {
        0,
        0,
        "ENTER = ¿Þ¯º³   ESC = ·¬Ý¾Ù   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ºÉ Êß°Ã¨¼®Ý¦ »¸¼Þ®½Ù ºÄ¶Þ ¾ÝÀ¸ »ÚÏ¼À",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  ºÉ Êß°Ã¨¼®Ý¦ »¸¼Þ® ½ÙÆÊ D·°¦ µ¼Ã ¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "¹²º¸(WARNING): ºÉ Êß°Ã¨¼®Ý ¼Þ®³É ½ÍÞÃÉ ÃÞ°ÀÊ ³¼ÅÜÚ Ï½!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ·¬Ý¾Ù½Ù ÆÊ ESC·° ¦ µ¼Ã¸ÀÞ»²¡",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Êß°Ã¼®Ý »¸¼Þ®   ESC = ·¬Ý¾Ù   F3 = Á­³¼",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY jaJPRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " ¾¯Ä±¯Ìß ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "¾¯Ä±¯ÌßÊ ¼½ÃÑÉ º³¾²¦ º³¼Ý ¼Ã²Ï½¡ ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Ú¼Þ½ÄØ Ê²ÌÞ¦ »¸¾² Á­³...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR jaJPErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOSÊ ºÝËß­°ÀÆ Ï¯À¸ ²Ý½Ä°Ù\n"
        "»ÚÏ¾Ý¡ ¾¯Ä±¯Ìß¦ Á­³¼ ½Ù ÊÞ±²¤ ReactOS¦ ²Ý½Ä°Ù½Ù ÆÊ ¾¯Ä±¯Ìß¦\n"
        "Ó³²ÁÄÞ ¼Þ¯º³ ½Ù ËÂÖ³¶Þ ±ØÏ½¡\n"
        "\n"
        "  \x07  ¾¯Ä±¯Ìß¦ ¿Þ¯º³½ÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡\n"
        "  \x07  ¾¯Ä±¯Ìß¦ Á­³¼½ÙÆÊ F3·°¦ µ¼Ã ¸ÀÞ»²¡",
        "F3 = Á­³¼  ENTER = ¿Þ¯º³"
    },
    {
        //ERROR_NO_HDD
        "¾¯Ä±¯ÌßÊ Ê°ÄÞÃÞ¨½¸¦ ¹Ý¼­Â ÃÞ·Ï¾Ý ÃÞ¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "¾¯Ä±¯ÌßÊ ¿°½ ÄÞ×²ÌÞ ¦ ¹Ý¼­Â ÃÞ·Ï¾Ý ÃÞ¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "¾¯Ä±¯ÌßÊ Ì§²Ù TXTSETUP.SIF É ÖÐºÐÆ ¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "¾¯Ä±¯ÌßÊ  TXTSETUP.SIF ¶Þ Ê¿Ý ¼Ã²Ù ºÄ¦ ¹Ý¼­Â ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "¾¯Ä±¯ÌßÊ TXTSETUP.SIF É Ñº³Å ¼®Ò²¦ ¹Ý¼­Â ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "¾¯Ä±¯ÌßÊ ¼½ÃÑ ÄÞ×²ÌÞÉ ¼Þ®³Î³¦ ¼ÐÄØ ÃÞ·Ï¾Ý ÃÞ¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_WRITE_BOOT,
        "¾¯Ä±¯ÌßÊ ¼½ÃÑ Êß°Ã¨¼®Ý ¼Þ®³ÍÉ FAT ÌÞ°Äº°ÄÞÉ ²Ý½Ä°ÙÆ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "¾¯Ä±¯ÌßÊ ºÝËß­°ÀÉ ¼­Ù² Ø½Ä É ÖÐºÐÆ ¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "¾¯Ä±¯ÌßÊ ÃÞ¨½ÌßÚ²É ¾¯Ã² Ø½Ä É ÖÐºÐÆ ¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "¾¯Ä±¯ÌßÊ ·°ÎÞ°ÄÞÉ ¼­Ù² Ø½Ä É ÖÐºÐÆ ¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "¾¯Ä±¯ÌßÊ ·°ÎÞ°ÄÞ Ú²±³Ä Ø½Ä É ÖÐºÐÆ ¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_WARN_PARTITION,
          "¾¯Ä±¯ÌßÊ ½¸Å¸ÄÓ 1ÂÉ Ê°ÄÞÃÞ¨½¸¶Þ Ã·¾ÂÆ ±Â¶´Å² ºÞ¶Ý¾²É Å²\n"
          "Êß°Ã¨¼®Ý Ã°ÌÞÙ¦ Ì¸Ñ ºÄ¦ Ê¯¹Ý ¼Ï¼À!\n"
          "\n"
          "Êß°Ã¨¼®Ý¦ »¸¾² ÏÀÊ »¸¼Þ®¼Ã¤ ºÉ Êß°Ã¨¼®Ý Ã°ÌÞÙ¦ Ê¶² ÃÞ·Ï½\n"
          "\n"
          "  \x07  ¾¯Ä±¯Ìß¦ Á­³¼ ½ÙÆÊ F3·°¦ µ¼Ã ¸ÀÞ»²¡"
          "  \x07  ¿Þ¯º³ ½ÙÆÊ ENTER·°¦ µ¼Ã ¸ÀÞ»²¡",
          "F3= Á­³¼  ENTER = ¿Þ¯º³"
    },
    {
        //ERROR_NEW_PARTITION,
        "±À×¼² Êß°Ã¨¼®Ý ¦ ½ÃÞÆ ¿Ý»Þ²½Ù\n"
        "Êß°Ã¨¼®ÝÉ Å²ÌÞÆ »¸¾² ½ÙºÄÊ ÃÞ·Ï¾Ý!\n"
        "\n"
        "  * ¿Þ¯º³ ½ÙÆÊ ÅÆ¶ ·°¦ µ¼Ã ¸ÀÞ»²¡",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "ÐÌÞÝ¶Â É ÃÞ¨½¸ ½Íß°½ ¦ »¸¼Þ® ½ÙºÄÊ ÃÞ·Ï¾Ý!\n"
        "\n"
        "  * ¿Þ¯º³ ½ÙÆÊ ÅÆ¶ ·°¦ µ¼Ã ¸ÀÞ»²¡",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "¾¯Ä±¯ÌßÊ ¼½ÃÑ Êß°Ã¨¼®Ý ¼Þ®³ÍÉ FAT ÌÞ°Ä Úº°ÄÞ É ²Ý½Ä°ÙÆ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_NO_FLOPPY,
        "ÄÞ×²ÌÞ A: Æ ÃÞ¨½¸¶Þ ±ØÏ¾Ý¡",
        "ENTER = ¿Þ¯º³"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "¾¯Ä±¯ÌßÊ ·°ÎÞ°ÄÞ Ú²±³Ä É ¾¯Ã²É º³ÆÝÆ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "¾¯Ä±¯ÌßÊ ÃÞ¨½ÌßÚ²É Ú¼Þ½ÄØ ¾¯Ã² É º³¼ÝÆ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_IMPORT_HIVE,
        "¾¯Ä±¯ÌßÊ Ê²ÌÞ Ì§²Ù É ²ÝÎß°ÄÆ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_FIND_REGISTRY
        "¾¯Ä±¯ÌßÊ Ú¼Þ½ÄØ ÃÞ°À Ì§²Ù É ¹Ý¼­ÂÆ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_CREATE_HIVE,
        "¾¯Ä±¯ÌßÊ Ú¼Þ½ÄØ Ê²ÌÞ É »¸¾²Æ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "¾¯Ä±¯ÌßÊ Ú¼Þ½ÄØ É ¼®·¶Æ ¼¯Êß² ¼Ï¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "·¬ËÞÈ¯ÄÆ Ñº³Å inf Ì§²Ù¶Þ Ì¸ÏÚÃ ²Ï½¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_CABINET_MISSING,
        "·¬ËÞÈ¯Ä ¶Þ ÐÂ¶ØÏ¾Ý¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "·¬ËÞÈ¯ÄÅ²Æ ¾¯Ä±¯Ìß ½¸ØÌßÄ ¶Þ ÐÂ¶ØÏ¾Ý¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_COPY_QUEUE,
        "¾¯Ä±¯ÌßÊ ºËß° Ì§²Ù ·­° É µ°ÌßÝÆ ¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_CREATE_DIR,
        "¾¯Ä±¯ÌßÊ ²Ý½Ä°Ù ÃÞ¨Ú¸ÄØ ¦ »¸¾² ÃÞ·Ï¾Ý ÃÞ¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "¾¯Ä±¯ÌßÊ TXTSETUP.SIF Å²É 'Directories' ¾¸¼®Ý É¹Ý»¸Æ\n"
        "¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_CABINET_SECTION,
        "¾¯Ä±¯ÌßÊ ·¬ËÞÈ¯Ä Å²É 'Directories' ¾¸¼®Ý É¹Ý»¸Æ\n"
        "¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "¾¯Ä±¯ÌßÊ ²Ý½Ä°Ù ÃÞ¨Ú¸ÄØ ¦ »¸¾² ÃÞ·Ï¾Ý ÃÞ¼À¡",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "¾¯Ä±¯ÌßÊ TXTSETUP.SIF Å²É 'SetupData' ¾¸¼®Ý É¹Ý»¸Æ\n"
        "¼¯Êß² ¼Ï¼À¡\n",
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_WRITE_PTABLE,
        "¾¯Ä±¯ÌßÊ Êß°Ã¨¼®Ý Ã°ÌÞÙ É ¶·ºÐ Æ ¼¯Êß² ¼Ï¼À¡\n"
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "¾¯Ä±¯ÌßÊ Ú¼Þ½ÄØÍÉ º°ÄÞÍß°¼ÞÉ Â²¶Æ ¼¯Êß² ¼Ï¼À¡\n"
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "¾¯Ä±¯ÌßÊ ¼½ÃÑ Û¹°Ù ¦ ¾¯Ã² ÃÞ·Ï¾Ý ÃÞ¼À¡\n"
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "¾¯Ä±¯ÌßÊ Ú¼Þ½ÄØÍÉ ·°ÎÞ°ÄÞ Ú²±³ÄÉ Â²¶Æ ¼¯Êß² ¼Ï¼À¡\n"
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        //ERROR_UPDATE_GEOID,
        "¾¯Ä±¯ÌßÊ geo id ¦ ¾¯Ã² ÃÞ·Ï¾Ý ÃÞ¼À¡\n"
        "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE jaJPPages[] =
{
    {
        LANGUAGE_PAGE,
        jaJPLanguagePageEntries
    },
    {
        START_PAGE,
        jaJPWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        jaJPIntroPageEntries
    },
    {
        LICENSE_PAGE,
        jaJPLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        jaJPDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        jaJPRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        jaJPComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        jaJPDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        jaJPFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        jaJPSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        jaJPSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        jaJPFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        jaJPDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        jaJPInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        jaJPPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        jaJPFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        jaJPKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        jaJPBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        jaJPLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        jaJPQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        jaJPSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        jaJPBootPageEntries
    },
    {
        REGISTRY_PAGE,
        jaJPRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING jaJPStrings[] =
{
    {STRING_PLEASEWAIT,
     "   µÏÁ ¸ÀÞ»²..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = ²Ý½Ä°Ù   C = Êß°Ã¨¼®Ý »¸¾²   F3 = Á­³¼"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = ²Ý½Ä°Ù   D = Êß°Ã¨¼®Ý »¸¼Þ®   F3 = Á­³¼"},
    {STRING_PARTITIONSIZE,
     "±À×¼² Êß°Ã¨¼®ÝÉ »²½Þ:"},
    {STRING_CHOOSENEWPARTITION,
     "±À×¼² Êß°Ã¨¼®Ý ¦ Â·ÞÆ »¸¾²½Ù ºÄ¶Þ ¾ÝÀ¸ »ÚÏ¼À:"},
    {STRING_HDDSIZE,
    "±À×¼² Êß°Ã¨¼®ÝÉ »²½Þ¦ Ò¶ÞÊÞ²Ä ÀÝ²ÃÞ Æ­³Ø®¸ ¼Ã¸ÀÞ»²¡"},
    {STRING_CREATEPARTITION,
     "   ENTER = Êß°Ã¨¼®Ý »¸¾²   ESC = ·¬Ý¾Ù   F3 = Á­³¼"},
    {STRING_PARTFORMAT,
    "ºÉ Êß°Ã¨¼®ÝÊ Â·ÞÆ Ì«°Ï¯Ä »ÚÏ½¡"},
    {STRING_NONFORMATTEDPART,
    "ReactOS¦ ¼Ý· ÏÀÊ ÐÌ«°Ï¯ÄÉ Êß°Ã¨¼®ÝÆ ²Ý½Ä°Ù½Ù ºÄ¶Þ ¾ÝÀ¸ »ÚÏ¼À¡"},
    {STRING_INSTALLONPART,
    "¾¯Ä±¯ÌßÊ ReactOS¦ Êß°Ã¨¼®Ý ¼Þ®³Æ ²Ý½Ä°Ù¼Ï½¡"},
    {STRING_CHECKINGPART,
    "¾¯Ä±¯ÌßÊ ¾ÝÀ¸ »ÚÀ Êß°Ã¨¼®Ý¦ ¹Ý» ¼Ã²Ï½¡"},
    {STRING_QUITCONTINUE,
    "F3= Á­³¼  ENTER = ¿Þ¯º³"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"},
    {STRING_TXTSETUPFAILED,
    "¾¯Ä±¯ÌßÊ TXTSETUP.SIF É '%S' ¾¸¼®ÝÉ ¹Ý¼­ÂÆ\n¼¯Êß² ¼Ï¼À¡\n"},
    {STRING_COPYING,
     "\xB3 ºËß° Á­³É Ì§²Ù: %S"},
    {STRING_SETUPCOPYINGFILES,
     "¾¯Ä±¯ÌßÊ Ì§²Ù¦ ºËß° ¼Ã ²Ï½..."},
    {STRING_REGHIVEUPDATE,
    "   Ú¼Þ½ÄØ Ê²ÌÞ É º³¼Ý Á­³..."},
    {STRING_IMPORTFILE,
    "   %S ¦ ²ÝÎß°Ä Á­³..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   ÃÞ¨½ÌßÚ² Ú¼Þ½ÄØ ¾¯Ã²¦ º³¼Ý Á­³..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Á²· ¾¯Ã²É º³¼Ý Á­³..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   ·°ÎÞ°ÄÞ Ú²±³ÄÉ ¾¯Ã² º³¼Ý Á­³..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   º°ÄÞ Íß°¼Þ É ¼Þ®³Î³¦ Ú¼Þ½ÄØÆ Â²¶ Á­³..."},
    {STRING_DONE,
    "   ¶ÝØ®³..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = ºÝËß­°ÀÉ »²·ÄÞ³"},
    {STRING_CONSOLEFAIL1,
    "ºÝ¿°Ù¦ µ°ÌßÝ ÃÞ·Ï¾Ý\n\n"},
    {STRING_CONSOLEFAIL2,
    "²¯ÊßÝÃ·Å ¹ÞÝ²Ý Ä¼Ã USB ·°ÎÞ°ÄÞ ¦ Â¶¯Ã ²ÙºÄ¶Þ ¶Ý¶Þ´×Ú Ï½\n"},
    {STRING_CONSOLEFAIL3,
    "USB ·°ÎÞ°ÄÞ Ê ÏÀÞ ¶Ý¾ÞÝÆ »Îß°Ä »ÚÃ ²Ï¾Ý\n"},
    {STRING_FORMATTINGDISK,
    "¾¯Ä±¯ÌßÊ ÃÞ¨½¸¦ Ì«°Ï¯Ä ¼Ã²Ï½"},
    {STRING_CHECKINGDISK,
    "¾¯Ä±¯ÌßÊ ÃÞ¨½¸¦ ¹Ý» ¼Ã²Ï½¡"},
    {STRING_FORMATDISK1,
    " Êß°Ã¨¼®Ý¦ %S Ì§²Ù ¼½ÃÑ ÃÞÌ«°Ï¯Ä (¸²¯¸ Ì«°Ï¯Ä) "},
    {STRING_FORMATDISK2,
    " Êß°Ã¨¼®Ý¦ %S Ì§²Ù ¼½ÃÑ ÃÞÌ«°Ï¯Ä "},
    {STRING_KEEPFORMAT,
    " ¹ÞÝ»Þ²É Ì§²Ù ¼½ÃÑÉ ÏÏ (ÍÝº³ ¼Å²) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Ê°ÄÞÃÞ¨½¸ %lu  (Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Ê°ÄÞÃÞ¨½¸ %lu  (Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  ¼­Ù² %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  Ê°ÄÞÃÞ¨½¸ %lu  (Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  Ê°ÄÞÃÞ¨½¸ %lu  (Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Ê°ÄÞÃÞ¨½¸ %lu (%I64u %s), Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  ¼­Ù² %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "on Ê°ÄÞÃÞ¨½¸ %lu (%I64u %s), Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  ¼­Ù² %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Ê°ÄÞÃÞ¨½¸ %lu  (Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Ê°ÄÞÃÞ¨½¸ %lu  (Îß°Ä=%hu, ÊÞ½=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "¾¯Ä±¯ÌßÊ ±À×¼² Êß°Ã¨¼®Ý¦ Â·ÞÆ »¸¾²¼Ï¼À:"},
    {STRING_UNPSPACE,
    "    ÐÌÞÝ¶ÂÉ ½Íß°½              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (»²ÀÞ². %lu MB)"},
    {STRING_UNFORMATTED,
    "¼Ý· (Ð Ì«°Ï¯Ä)"},
    {STRING_FORMATUNUSED,
    "Ð ¼Ö³"},
    {STRING_FORMATUNKNOWN,
    "ÌÒ²"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "·°ÎÞ°ÄÞ Ú²±³Ä É Â²¶ Á­³"},
    {0, 0}
};

#endif
