// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY bgBGSetupInitPageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Please wait while the ReactOS Setup initializes itself",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "and discovers your devices...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Please wait...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGLanguagePageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\210\247\241\256\340 \255\240 \245\247\250\252",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \210\247\241\245\340\245\342\245 \245\247\250\252, \252\256\251\342\256 \244\240 \250\247\257\256\253\242\240\342\245 \257\340\250 \341\253\240\243\240\255\245\342\256.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   \255\240\342\250\341\255\245\342\245 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \205\247\250\252\352\342 \351\245 \241\352\244\245 \257\256\244\340\240\247\241\250\340\240\255\250\357\342 \247\240 \252\340\240\251\255\240\342\240 \343\340\245\244\241\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245  F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGWelcomePageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\220\245\240\252\342\216\221 \242\250 \257\340\250\242\245\342\341\342\242\240!",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "\222\240\247\250 \347\240\341\342 \256\342 \255\240\341\342\340\256\251\252\240\342\240 \247\240\257\250\341\242\240 \340\240\241\256\342\255\240\342\240 \343\340\245\244\241\240 \220\245\240\252\342\216\221",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "\255\240 \252\256\254\257\356\342\352\340\240 \242\250 \250 \257\256\244\243\256\342\242\357 \242\342\256\340\240\342\240 \347\240\341\342 \255\240 \255\240\341\342\340\256\251\252\240\342\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  \215\240\342\250\341\255\245\342\245 R \247\240 \257\256\257\340\240\242\252\240 \255\240 \220\245\240\252\342\216\221.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  \215\240\342\250\341\255\245\342\245 L, \247\240 \244\240 \242\250\244\250\342\245 \340\240\247\340\245\350\250\342\245\253\255\250\342\245 (\253\250\346\245\255\247\255\250\342\245)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "   \250\247\250\341\252\242\240\255\250\357 \250 \343\341\253\256\242\250\357 \255\240 \220\245\240\252\342\216\221",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "\x07  \215\240\342\250\341\255\245\342\245 F3 \247\240 \250\247\345\256\244 \241\245\247 \341\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\207\240 \257\256\242\245\347\245 \341\242\245\244\245\255\250\357 \247\240 \220\245\240\252\342\216\221, \257\256\341\245\342\245\342\245:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        25,
        "https://reactos.org/",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   R = \217\256\257\340\240\242\252\240   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGIntroPageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGLicensePageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "\213\250\346\245\255\247\250\340\240\255\245:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "\223\340\245\244\241\240\342\240 \220\245\240\252\342\216\221 \245 \253\250\346\245\255\247\250\340\240\255\240 \257\340\250 \343\341\253\256\242\250\357\342\240 \255\240 GNU GPL",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "\341 \347\240\341\342\250, \341\352\244\352\340\246\240\351\250 \252\256\244 \256\342 \244\340\343\243\250 \341\352\242\254\245\341\342\250\254\250 \253\250\346\245\255\247\250 \252\240\342\256",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "X11, BSD \250\253\250 GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\221\253\245\244\256\242\240\342\245\253\255\256 \242\341\357\252\256 \256\341\250\243\343\340\357\242\240\255\245, \252\256\245\342\256 \245 \347\240\341\342 \256\342",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "\220\245\240\252\342\216\221, \341\245 \256\241\255\240\340\256\244\242\240 \257\256\244 GNU GPL, \341 \257\256\244\244\352\340\246\240\255\245\342\256 \255\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\255\240 \256\340\250\243\250\255\240\253\255\250\357 \253\250\346\245\255\247.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\200\252\256 \257\256\340\240\244\250 \255\357\252\240\252\242\240 \257\340\250\347\250\255\240, \247\240\245\244\255\256 \341 \220\245\240\252\342\216\221 \255\245 \341\342\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\257\256\253\343\347\250\253\250 \252\256\257\250\245 \255\240 GNU General Public License, \257\256\341\245\342\245\342\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "\203\240\340\240\255\346\250\357:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\222\256\242\240 \245 \341\242\256\241\256\244\245\255 \341\256\344\342\343\245\340; \242\250\246\342\245 \250\247\345\256\244\255\250\357 \252\256\244 \247\240 \343\341\253\256\242\250\357\342\240 \255\240 \242\352\247\257\340\256\250\247\242\245\246\244\240\255\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \202\340\352\351\240\255\245",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGDevicePageEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\221\257\250\341\352\252\352\342 \257\256- \244\256\253\343 \257\256\252\240\247\242\240 \342\245\252\343\351\250\342\245 \255\240\341\342\340\256\251\252\250 \255\240 \343\341\342\340\256\251\341\342\242\240\342\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "       \212\256\254\257\356\342\352\340:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "          \205\252\340\240\255:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        3,
        13,
        "          \212\253\240\242\250\240\342\343\340\240:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        3,
        14,
        "\212\253\240\242\250\240\342\343\340\255\240 \257\256\244\340\245\244\241\240:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        3,
        16,
        "            \217\340\250\245\254\240\255\245:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "\217\340\250\245\254\240\255\245 \255\240 \255\240\341\342\340\256\251\252\250\342\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "\207\240 \244\240 \257\340\256\254\245\255\250\342\245 \255\240\341\342\340\256\251\252\250\342\245 \255\240 \256\241\256\340\343\244\242\240\255\245\342\256, \250\247\257\256\253\247\242\240\251\342\245 \341\342\340\245\253\252\250\342\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "\215\200\203\216\220\205 \250 \215\200\204\216\213\223. \221\253\245\244 \342\256\242\240 \255\240\342\250\341\255\245\342\245 ENTER, \247\240 \244\240 \250\247\241\245\340\245\342\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "\247\240\254\245\341\342\242\240\351\250 \255\240\341\342\340\256\251\252\250.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "\212\256\243\240\342\256 \255\240\257\340\240\242\250\342\245 \242\341\250\347\252\250 \255\240\341\342\340\256\251\252\250, \250\247\241\245\340\245\342\245 '\217\340\250\245\254\240\255\245 \255\240 \255\240\341\342\340\256\251\252\250\342\245'",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\250 \255\240\342\250\341\255\245\342\245 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGRepairPageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\240 \220\245\240\252\342\216\221 \245 \242 \340\240\255\255\240 \341\342\245\257\245\255 \255\240 \340\240\247\340\240\241\256\342\252\240. \202\341\245 \256\351\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\255\357\254\240 \242\341\250\347\252\250 \242\352\247\254\256\246\255\256\341\342\250 \255\240 \255\240\257\352\253\255\256 \250\247\257\256\253\247\242\240\245\254\256 \255\240\341\342\340\256\251\242\240\351\256 \257\340\250\253\256\246\245\255\250\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "\202\352\247\254\256\246\255\256\341\342\342\240 \247\240 \257\256\257\340\240\242\252\240 \256\351\245 \255\245 \245 \243\256\342\256\242\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  \255\240\342\250\341\255\245\342\245 U \247\240 \256\241\255\256\242\357\242\240\255\245 \255\240 \256\257\245\340\240\346\250\256\255\255\240\342\240 \341\250\341\342\245\254\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  \215\240\342\250\341\255\245\342\245 R \247\240 \242\352\247\341\342\240\255\256\242\357\242\240\351\240 \341\340\245\244\240 (\252\256\255\247\256\253\240).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  \255\240\342\341\250\255\245\342\245 ESC \247\240 \242\340\352\351\240\255\245 \252\352\254 \243\253\240\242\255\240\342\240 \341\342\340\240\255\250\346\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  \215\240\342\250\341\255\245\342\245 ENTER \247\240 \257\340\245\247\240\257\343\341\252 \255\240 \252\256\254\257\356\342\352\340\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ESC = \203\253\240\242\255\240 \341\342\340\240\255\250\346\240  ENTER = \217\340\245\247\240\257\343\341\252",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGUpgradePageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "The ReactOS Setup can upgrade one of the available ReactOS installations",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "listed below, or, if a ReactOS installation is damaged, the Setup program",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "can attempt to repair it.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "The repair functions are not all implemented yet.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press UP or DOWN to select an OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Press U for upgrading the selected OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ESC to continue with a new installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Upgrade   ESC = Do not upgrade   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGComputerPageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\220\245\350\250\253\250 \341\342\245 \244\240 \341\254\245\255\250\342\245 \242\250\244\240 \255\240 \252\256\254\257\356\342\352\340\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \210\247\241\245\340\245\342\245 \242\250\244\240 \255\240 \245\252\340\240\255\240 \341\352\341 \341\342\340\245\253\252\250\342\245 \255\240\243\256\340\245 \250 \255\240\244\256\253\343 \250 ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   \255\240\342\250\341\255\245\342\245 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \215\240\342\250\341\255\245\342\245 ESC, \247\240 \244\240 \341\245 \242\352\340\255\245\342\245 \252\352\254 \257\340\245\244\345\256\244\255\240\342\240 \341\342\340\240\255\250\346\240, \241\245\247 \244\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   \341\254\245\255\357\342\245 \242\250\244\240 \255\240 \252\256\254\257\356\342\352\340\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   ESC = \216\342\252\240\247   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGFlushPageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\223\340\245\244\241\240\342\240 \257\340\256\242\245\340\357\242\240, \244\240\253\250 \242\341\250\347\252\250 \244\240\255\255\250 \341\240 \341\352\345\340\240\255\245\255\250 \255\240 \244\250\341\252\240 \242\250.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "\222\256\242\240 \351\245 \256\342\255\245\254\245 \254\250\255\343\342\252\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\212\256\254\257\356\342\352\340\352\342 \242\250 \351\245 \341\245 \257\340\245\247\240\257\343\341\255\245 \341\240\254, \252\256\243\240\342\256 \257\340\250\252\253\356\347\250.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \210\247\347\250\341\342\242\240\255\245 \255\240 \341\252\253\240\244\240",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGQuitPageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\221\253\240\243\240\255\245\342\256 \255\240 \220\245\240\252\342\216\221 \255\245 \245 \247\240\242\352\340\350\250\253\256.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "\210\247\242\240\244\245\342\245 \244\250\341\252\245\342\240\342\240 \256\342 \343\341\342\340\256\251\341\342\242\256 \200: \250",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\242\341\250\347\252\250 \255\256\341\250\342\245\253\250 \256\342 \212\204 \250 DVD \343\341\342\340\256\251\341\342\242\240\342\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "\215\240\342\250\341\255\245\342\245 ENTER, \247\240 \244\240 \257\340\245\247\240\257\343\341\255\245\342\245 \252\256\254\257\356\342\352\340\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \210\247\347\240\252\240\251\342\245...",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGDisplayPageEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\220\245\350\250\253\250 \341\342\245 \244\240 \341\254\245\255\250\342\245 \242\250\244\240 \255\240 \245\252\340\240\255\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  \210\247\241\245\340\245\342\245 \242\250\244\240 \255\240 \245\252\340\240\255\240 \341\352\341 \341\342\340\245\253\252\250\342\245 \255\240\243\256\340\245 \250 \255\240\244\256\253\343 \250 ",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   \255\240\342\250\341\255\245\342\245 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \215\240\342\250\341\255\245\342\245 ESC, \247\240 \244\240 \341\245 \242\352\340\255\245\342\245 \252\352\254 \257\340\245\244\345\256\244\255\240\342\240 \341\342\340\240\255\250\346\240, \241\245\247 \244\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   \341\254\245\255\357\342\245 \242\250\244\240 \255\240 \245\252\340\240\255\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   ESC = \216\342\252\240\247   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGSuccessPageEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\216\341\255\256\242\255\250\342\245 \341\352\341\342\240\242\252\250 \255\240 \220\245\240\252\342\216\221 \341\240 \341\253\256\246\245\255\250 \343\341\257\245\350\255\256.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "\210\247\242\240\244\245\342\245 \244\250\341\252\245\342\240\342\240 \256\342 \343\341\342\340\256\251\341\342\242\256 \200: \250",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\242\341\250\347\252\250 \255\256\341\250\342\245\253\250 \256\342 \256\257\342\250\347\255\250\342\245 \343\341\342\340\256\251\341\342\242\240 (\212\204/DVD)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "\215\240\342\250\341\255\245\342\245 ENTER, \247\240 \244\240 \257\340\245\247\240\257\343\341\255\245\342\245 \252\256\254\257\356\342\352\340\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGBootPageEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\221\253\240\243\240\255\245\342\256 \255\240 \247\240\340\245\246\244\240\347\240 (bootloader) \255\240 \244\250\341\252\240 \255\240 \252\256\254\257\356\342\352\340\240 \242\250",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\241\245 \255\245\343\341\257\245\350\255\256.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "\221\253\256\246\245\342\245 \344\256\340\254\240\342\250\340\240\255\240 \244\250\341\252\245\342\240 \242 \343\341\342\340\256\251\341\342\242\256 A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\250 \255\240\342\250\341\255\245\342\245 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY bgBGSelectPartitionEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\221\257\250\341\352\252\352\342 \257\256- \244\256\253\343 \341\352\244\352\340\246\240 \341\352\351\245\341\342\242\343\242\240\351\250\342\245 \244\357\253\256\242\245 \250 \257\340\240\247\255\256\342\256",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\254\357\341\342\256 \247\240 \255\256\242\250 \244\357\253\256\242\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  \210\247\257\256\253\247\242\240\251\342\245 \341\342\340\245\253\252\250\342\245 \247\240 \250\247\241\256\340 \256\342 \341\257\250\341\352\252\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \215\240\342\250\341\255\245\342\245 ENTER \247\240 \341\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 \255\240 \250\247\241\340\240\255\250\357 \244\357\253.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  Натиснете C за създаване на нов дял.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Press E to create an extended partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press L to create a logical partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  \215\240\342\250\341\255\245\342\245 D \247\240 \250\247\342\340\250\242\240\255\245 \255\240 \341\352\351\245\341\342\242\343\242\240\351 \244\357\253.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \217\256\347\240\252\240\251\342\245...",  /* Редът да не се превежда, защото списъкът с дяловете ще се размести */
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGChangeSystemPartition[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "The current system partition of your computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "on the system disk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "uses a format not supported by ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "In order to successfully install ReactOS, the Setup program must change",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "the current system partition to a new one.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "The new candidate system partition is:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  To accept this choice, press ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  To manually change the system partition, press ESC to go back to",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   the partition selection list, then select or create a new system",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   partition on the system disk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "In case there are other operating systems that depend on the original",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "system partition, you may need to either reconfigure them for the new",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "system partition, or you may need to change the system partition back",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "to the original one after finishing the installation of ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   ESC = Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   not be deleted.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGFormatPartitionEntries[] =
{
    {
        4,
        3,
        " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\224\256\340\254\240\342\250\340\240\255\245 \255\240 \244\357\253",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "\204\357\253\352\342 \351\245 \241\352\244\245 \344\256\340\254\240\342\250\340\240\255. \215\240\342\250\341\255\245\342\245 ENTER \247\240 \257\340\256\244\352\253\246\240\242\240\255\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGCheckFSEntries[] =
{
    {
        4,
        3, " \221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\222\245\347\245 \257\340\256\242\245\340\252\240 \255\240 \250\247\241\340\240\255\250\357 \244\357\253.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\217\256\347\240\252\240\251\342\245...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\224\240\251\253\256\242\245\342\245 \255\240 \220\245\240\252\342\216\221 \351\245 \241\352\244\240\342 \341\253\256\246\245\255\250 \255\240 \250\247\241\340\240\255\250\357 \244\357\253. \210\247\241\245\340\245\342\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\257\240\257\252\240, \242 \252\256\357\342\256 \244\240 \241\352\244\245 \341\253\256\246\245\255 \220\245\240\252\342\216\221:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\207\240 \341\254\357\255\240 \255\240 \257\340\245\244\253\256\246\245\255\240\342\240 \257\240\257\252\240 \255\240\342\250\341\255\245\342\245 BACKSPACE, \247\240 \244\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "\250\247\342\340\250\245\342\245 \247\255\240\346\250\342\245 \250 \342\256\243\240\242\240 \255\240\257\250\350\245\342\245 \257\240\257\252\240\342\240, \242 \252\256\357\342\256 \244\240 \241\352\244\245",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "\341\253\256\246\245\255 \220\245\240\252\342\216\221.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGFileCopyEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        11,
        12,
        "\210\247\347\240\252\240\251\342\245 \244\240 \257\340\250\252\253\356\347\250 \247\240\257\250\341\352\342 \255\240 \344\240\251\253\256\242\245\342\245 \242 \250\247\241\340\240\255\240\342\240 \257\240\257\252\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        30,
        13,
       // "избраната папка.",
       "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        20,
        14,
        "\222\256\242\240 \254\256\246\245 \244\240 \256\342\255\245\254\245 \255\357\252\256\253\252\256 \254\250\255\343\342\250.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "                                                           \xB3 \217\256\347\240\252\240\251\342\245...      ",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGBootLoaderEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\217\340\256\342\250\347\240 \341\253\240\243\240\255\245\342\256 \255\240 \247\240\340\245\246\244\240\347\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "\221\253\240\243\240\255\245 \255\240 \247\240\340\245\246\244\240\347 \255\240 \342\242\352\340\244\250\357 \244\250\341\252 (MBR \250 VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\221\253\240\243\240\255\245 \255\240 \247\240\340\245\246\244\240\347 \255\240 \342\242\352\340\244\250\357 \244\250\341\252 (\341\240\254\256 VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\221\253\240\243\240\255\245 \255\240 \247\240\340\245\246\244\240\347 \255\240 \244\250\341\252\245\342\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\204\240 \255\245 \341\245 \341\253\240\243\240 \247\240\340\245\246\244\240\347.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\210\341\252\240\342\245 \244\240 \341\254\245\255\250\342\245 \242\250\244\240 \255\240 \252\253\240\242\250\240\342\343\340\240\342\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \210\247\257\256\253\247\242\240\251\342\245 \341\342\340\245\253\252\250\342\245, \247\240 \244\240 \250\247\241\245\340\245\342\245 \242\250\244\240 \255\240 \252\253\240\242\250\240\342\343\340\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   \255\240\342\250\341\255\245\342\245 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \215\240\342\250\341\255\245\342\245 ESC, \247\240 \244\240 \341\245 \242\352\340\255\245\342\245 \252\352\254 \257\340\245\244\345\256\244\255\240\342\240 \341\342\340\240\255\250\346\240, \241\245\247 \244\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   \341\254\245\255\357\342\245 \242\250\244\240 \255\240 \252\253\240\242\250\240\342\343\340\240\342\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   ESC = \216\342\252\240\247   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\210\247\241\245\340\245\342\245 \257\256\244\340\240\247\241\250\340\240\255\240 \252\253\240\242\250\240\342\343\340\255\240 \257\256\244\340\245\244\241\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \210\247\257\256\253\247\242\240\251\342\245 \341\342\340\245\253\252\250\342\245, \247\240 \244\240 \250\247\241\245\340\245\342\245 \246\245\253\240\255\240\342\240 \252\253\240\242\250\240\342\343\340\255\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    \257\256\244\340\245\244\241\240 \250 \257\256\341\253\245 \255\240\342\250\341\255\245\342\245 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \215\240\342\250\341\255\245\342\245 ESC, \247\240 \244\240 \341\245 \242\352\340\255\245\342\245 \252\352\254 \257\340\245\244\345\256\244\255\240\342\240 \341\342\340\240\255\250\346\240, \241\245\247 \244\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   \341\254\245\255\357\342\245 \252\253\240\242\250\240\342\343\340\255\240\342\240 \257\256\244\340\245\244\241\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   ESC = \216\342\252\240\247   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY bgBGPrepareCopyEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\212\256\254\257\356\342\352\340\352\342 \341\245 \257\256\244\243\256\342\242\357 \247\240 \247\240\257\250\341 \255\240 \344\240\251\253\256\242\245\342\245 \255\240 \220\245\240\252\342\216\221.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \221\352\341\342\240\242\357\255\245 \255\240 \341\257\250\341\352\252\240 \256\342 \344\240\251\253\256\242\245 \247\240 \247\240\257\250\341...",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY bgBGSelectFSEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "\207\240 \244\240 \250\247\241\245\340\245\342\245 \344\240\251\253\256\242\240 \341\250\341\342\245\254\240 \256\342 \244\256\253\255\250\357 \341\257\250\341\352\252:",
        0
    },
    {
        8,
        19,
        "\x07  \210\247\241\245\340\245\342\245 \344\240\251\253\256\242\240 \341\250\341\342\245\254\240 \341\352\341 \341\342\340\245\253\252\250\342\245.",
        0
    },
    {
        8,
        21,
        "\x07  \215\240\342\250\341\255\245\342\245 ENTER, \247\240 \244\240 \344\256\340\254\240\342\250\340\240\342\245 \244\357\253\240.",
        0
    },
    {
        8,
        23,
        "\x07  \215\240\342\250\341\255\245\342\245 ESC, \247\240 \244\240 \250\247\241\245\340\245\342\245 \244\340\343\243 \244\357\253.",
        0
    },
    {
        0,
        0,
        "   ENTER = \217\340\256\244\352\253\246\240\242\240\255\245   ESC = \216\342\252\240\247   F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGDeletePartitionEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\210\247\241\340\240\253\250 \341\342\245 \244\240 \250\247\342\340\250\245\342\245 \244\357\253",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  \215\240\342\250\341\255\245\342\245 L, \247\240 \244\240 \250\247\342\340\250\245\342\245 \244\357\253\240.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "\202\215\210\214\200\215\210\205: \202\341\250\347\252\250 \244\240\255\255\250 \255\240 \342\256\247\250 \244\357\253 \351\245 \241\352\244\240\342 \343\255\250\351\256\246\245\255\250!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  \215\240\342\250\341\255\245\342\245 ESC, \247\240 \244\240 \341\245 \256\342\252\240\246\245\342\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   L = \210\247\342\340\250\242\240\255\245 \255\240 \244\357\253\240, ESC = \216\342\252\240\247    F3 = \210\247\345\256\244",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY bgBGRegistryEntries[] =
{
    {
        4,
        3,
        " \215\240\341\342\340\256\251\252\240 \255\240 \220\245\240\252\342\216\221 " KERNEL_VERSION_STR " .",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\217\340\256\342\250\347\240 \256\241\255\256\242\357\242\240\255\245 \255\240 \341\250\341\342\245\254\255\250\342\245 \255\240\341\342\340\256\251\252\250.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \221\352\247\244\240\242\240\255\245 \255\240 \340\245\243\250\341\342\352\340\255\250\342\245 \340\256\245\242\245...",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR bgBGErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "\220\245\240\252\342\216\221 \255\245 \245 \255\240\257\352\253\255\256 \341\253\256\246\245\255 \255\240 \252\256\254\257\356\342\352\340\240\n"
        "\242\250. \200\252\256 \341\245\243\240 \250\247\253\245\247\245\342\245 \256\342 \341\253\240\243\240\255\245\342\245, \351\245 \342\340\357\241\242\240\n"
        "\244\240 \257\343\341\255\245\342\245 \255\240\341\342\340\256\251\252\240\342\240 \256\342\255\256\242\256, \247\240 \244\240 \250\255\341\342\240\253\250\340\240\342\245 \220\245\240\252\342\216\221.\n"
        "\n"
        "  \x07  \207\240 \244\240 \257\340\256\244\352\253\246\250 \341\253\240\243\240\255\245\342\256, \255\240\342\250\341\255\245\342\245 ENTER.\n"
        "  \x07  \207\240 \250\247\345\256\244 \255\240\342\250\341\255\245\342\245 F3.",
        "F3 = \210\247\345\256\244 ENTER = \217\340\256\244\352\253\246\240\242\240\255\245"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Failed to build the installation paths for the ReactOS installation directory!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_PATH
        "You cannot delete the partition containing the installation source!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_DIR
        "You cannot install ReactOS within the installation source directory!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_NO_HDD
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \255\240\254\245\340\250 \342\242\352\340\244 \244\250\341\252.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \255\240\254\245\340\250 \250\247\345\256\244\255\256\342\256 \341\250 \343\341\342\340\256\251\341\342\242\256.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \343\341\257\357 \244\240 \255\240\254\245\340\250 \344\240\251\253 TXTSETUP.SIF.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\240\254\245\340\250 \257\256\242\340\245\244\245\255 \344\240\251\253 TXTSETUP.SIF.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\240\254\245\340\250 \255\245\340\245\244\245\255 \257\256\244\257\250\341 \242 TXTSETUP.SIF.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \343\341\257\357 \244\240 \256\342\252\340\250\245 \341\242\245\244\245\255\250\357\342\240 \255\240 \341\250\341\342\245\254\255\256\342\256 \242\250 \343\341\342\340\256\251\341\342\242\256.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_WRITE_BOOT,
        "\215\245\343\341\257\245\350\255\256 \341\253\240\243\240\255\245 \255\240 \256\247\255\240\347\240\242\240\350 \247\240\257\250\341 (bootcode) \247\240 %S \242 \341\250\341\342\245\254\255\250\357 \244\357\253.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \343\341\257\357 \244\240 \247\240\340\245\244\250 \341\257\250\341\352\252\240 \341 \242\250\244\256\242\245\342\245 \252\256\254\257\356\342\340\250.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \343\341\257\357 \244\240 \247\240\340\245\244\250 \341\257\250\341\352\252\240 \341 \255\240\342\340\256\251\252\250 \247\240 \254\256\255\250\342\256\340\250.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \343\341\257\357 \244\240 \247\240\340\245\244\250 \341\257\250\341\352\252\240 \341 \242\250\244\256\242\245\342\245 \252\253\240\242\250\240\342\343\340\250.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \343\341\257\357 \244\240 \247\240\340\245\244\250 \341\257\250\341\352\252\240 \341 \252\253\240\242\250\240\342\343\340\255\250\342\245 \257\256\244\340\245\244\241\250.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_WARN_PARTITION,
          "\215\240\341\342\340\256\251\242\240\347\352\342 \343\341\342\240\255\256\242\250, \347\245 \257\256\255\245 \245\244\250\255 \342\242\352\340\244 \244\250\341\252 \341\352\244\352\340\246\240 \255\245\341\352\242\254\245\341\342\250\254\240\n"
          "\244\357\253\256\242\240 \342\240\241\253\250\346\240, \341 \252\256\357\342\256 \255\245 \254\256\246\245 \244\240 \341\245 \340\240\241\256\342\250 \257\340\240\242\250\253\255\256!\n"
          "\n"
          "\221\352\247\244\240\242\240\255\245\342\256 \250\253\250 \250\247\342\340\250\242\240\255\245\342\256 \255\240 \244\357\253\256\242\245 \254\256\246\245 \244\240 \343\255\250\351\256\246\250 \244\357\253\256\242\240\342\240 \342\240\241\253\250\346\240.\n"
          "\n"
          "  \x07  \207\240 \250\247\345\256\244 \255\240\342\250\341\255\245\342\245 F3.\n"
          "  \x07  \215\240\342\250\341\255\245\342\245 ENTER \247\240 \257\340\256\244\352\253\246\240\242\240\255\245.",
          "F3 = \210\247\345\256\244 ENTER = \217\340\256\244\352\253\246\240\242\240\255\245"
    },
    {
        // ERROR_NEW_PARTITION,
        "\215\245 \254\256\246\245\342\245 \244\240 \341\352\247\244\240\244\245\342\245 \255\256\242 \244\357\253 \242 \244\357\253,\n"
        "\252\256\251\342\256 \242\245\347\245 \341\352\351\245\341\342\242\343\242\240!\n"
        "\n"
        "  * \215\240\342\250\341\255\245\342\245 \252\253\240\242\250\350, \247\240 \244\240 \257\340\256\244\352\253\246\250\342\245.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "\215\245 \254\256\246\245\342\245 \247\240 \250\247\342\340\250\245\342\245 \255\245\340\240\247\257\340\245\244\245\253\245\255\256\342\256 \244\250\341\252\256\242\256 \254\357\341\342\256!\n"
        "\n"
        "  * \215\240\342\250\341\255\245\342\245 \252\253\240\242\250\350, \247\240 \244\240 \257\340\256\244\352\253\246\250\342\245.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        //"Setup failed to install the FAT bootcode on the system partition.",
        "\215\245\343\341\257\245\350\255\256 \341\253\240\243\240\255\245 \255\240 \256\241\343\242\240\351\250\357 \252\256\244 \247\240 %S \255\240 \341\250\341\342\245\254\255\250\357 \244\357\253.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_NO_FLOPPY,
        "\202 \343\341\342\340\256\251\341\342\242\256 A: \255\357\254\240 \255\256\341\250\342\245\253.",
        "ENTER = \217\340\256\244\352\253\246\240\242\240\255\245"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "\215\245\343\341\257\245\350\255\256 \256\241\255\256\242\357\242\240\255\245 \255\240 \255\240\341\342\340\256\251\252\250\342\245 \255\240 \252\253\240\242\250\240\342\343\340\255\240\342\240 \257\256\244\340\245\244\241\240.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "\215\245\343\341\257\245\350\255\256 \256\241\255\256\242\357\242\240\255\245 \255\240 \340\245\243\250\341\342\352\340\255\250 \255\240\341\342\340\256\251\252\250 \247\240 \254\256\255\250\342\256\340\240.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_IMPORT_HIVE,
        "\215\245\343\341\257\245\350\255\256 \242\255\240\341\357\255\245 \255\240 \340\256\245\242\250\357 \344\240\251\253.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_FIND_REGISTRY
        "\215\245 \241\357\345\240 \256\342\252\340\250\342\250 \344\240\251\253\256\242\245\342\245 \341 \340\245\243\250\341\342\352\340\255\250 \244\240\255\255\250.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_CREATE_HIVE,
        "\215\240\341\342\340\256\251\242\240\347\352\342 \255\245 \343\341\257\357 \244\240 \341\352\247\244\240\244\245 \340\245\243\250\341\342\352\340\255\250\342\245 \340\256\245\242\245.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        //There is something wrong with this line.
        "\215\245\343\341\257\245\350\255\256 \247\240\244\240\242\240\255\245 \255\240 \255\240\347\240\253\255\250 \341\342\256\251\255\256\341\342\250 \255\240 \340\245\243\250\341\342\352\340\240.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Cab \344\240\251\253\352\342 \255\357\254\240 \257\340\240\242\250\253\245\255 inf \344\240\251\253.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cab \344\240\251\253\352\342 \255\245 \245 \256\342\252\340\250\342.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Cab \344\240\251\253\352\342 \255\357\254\240 \255\240\341\342\340\256\245\347\255\256 \257\250\341\240\255\250\245.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_COPY_QUEUE,
        "\215\245\343\341\257\245\350\255\256 \256\342\242\240\340\357\255\245 \255\240 \256\257\240\350\252\240\342\240 \256\342 \344\240\251\253\256\242\245 \247\240 \247\240\257\250\341.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_CREATE_DIR,
        "\215\245\343\341\257\245\350\255\256 \341\352\247\244\240\242\240\255\245 \255\240 \257\240\257\252\250\342\245 \247\240 \341\253\240\243\240\255\245.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "\220\240\247\244\245\253\352\342 '%S' \255\245 \241\245 \256\342\252\340\250\342\n"
        "\242 TXTSETUP.SIF.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_CABINET_SECTION,
        "\220\240\247\244\245\253\352\342 '%S' \255\245 \241\245 \256\342\252\340\250\342\n"
        "\242 cab \344\240\251\253\240.\n",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "\215\245\343\341\257\245\350\255\256 \341\352\247\244\240\242\240\255\245 \255\240 \257\240\257\252\240\342\240 \247\240 \341\253\240\243\240\255\245.",
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_WRITE_PTABLE,
        "\215\245\343\341\257\245\350\255\256 \247\240\257\250\341\242\240\255\245 \255\240 \244\357\253\256\242\250\342\245 \342\240\241\253\250\346\250.\n"
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "\215\245\343\341\257\245\350\255\256 \244\256\241\240\242\357\255\245 \255\240 \247\255\240\252\256\242\250\357 \255\240\241\256\340 \242 \340\245\243\250\341\342\352\340\240.\n"
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "\215\245\343\341\257\245\350\255\256 \343\341\342\240\255\256\242\357\242\240\255\245 \255\240 \254\245\341\342\255\250\342\245 \255\240\341\342\340\256\251\252\250.\n"
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "\215\245\343\341\257\245\350\255\256 \244\256\241\240\242\357\255\245 \255\240 \252\253\240\242\250\240\342\343\340\255\250\342\245 \257\256\244\340\245\244\241\250 \242 \340\245\243\250\341\342\352\340\240.\n"
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_UPDATE_GEOID,
        "\215\240\341\342\340\256\251\252\240\342\240 \255\245 \254\256\246\240 \244\240 \343\341\342\240\255\256\242\250 \256\247\255\240\347\250\342\245\253\357 \255\240 \243\245\256\243\340\240\344\341\252\256\342\256 \257\256\253\256\246\245\255\250\245.\n"
        "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * \215\240\342\250\341\255\245\342\245 \252\253\240\242\250\350, \247\240 \244\240 \257\340\256\244\352\253\246\250\342\245.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "You cannot create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "You cannot create more than one extended partition per disk.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Setup is unable to format the partition:\n"
        " %S\n"
        "\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE bgBGPages[] =
{
    {
        SETUP_INIT_PAGE,
        bgBGSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        bgBGLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        bgBGWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        bgBGIntroPageEntries
    },
    {
        LICENSE_PAGE,
        bgBGLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        bgBGDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        bgBGRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        bgBGUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        bgBGComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        bgBGDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        bgBGFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        bgBGSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        bgBGChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        bgBGConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        bgBGSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        bgBGFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        bgBGCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        bgBGDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        bgBGInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        bgBGPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        bgBGFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        bgBGKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        bgBGBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        bgBGLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        bgBGQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        bgBGSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        bgBGBootPageEntries
    },
    {
        REGISTRY_PAGE,
        bgBGRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING bgBGStrings[] =
{
    {STRING_PLEASEWAIT,
     "   \217\256\347\240\252\240\251\342\245..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Слагане   C = Създаване на дял   F3 = Изход"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = \221\253\240\243\240\255\245   D = \210\247\342\340\250\242\240\255\245 \255\240 \244\357\253   F3 = \210\247\345\256\244"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "\220\240\247\254\245\340 \255\240 \255\256\242\250\357 \244\357\253:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//     "Избрали сте да създадете нов дял на"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "\202\352\242\245\244\245\342\245 \340\240\247\254\245\340\240 \255\240 \255\256\242\250\357 \244\357\253 (\242 \254\245\243\240\241\240\251\342\250)."},
    {STRING_CREATEPARTITION,
     "   ENTER = \221\352\247\244\240\242\240\255\245 \255\240 \244\357\253   ESC = \216\342\252\240\247   F3 = \210\247\345\256\244"},
    {STRING_PARTFORMAT,
    "\217\340\245\244\341\342\256\250 \344\256\340\254\240\342\250\340\240\255\245 \255\240 \244\357\253\240."},
    {STRING_NONFORMATTEDPART,
    "\210\247\241\340\240\253\250 \341\342\245 \244\240 \341\253\256\246\250\342\245 \220\245\240\252\342\216\221 \255\240 \255\256\242 \250\253\250 \255\245\340\240\247\257\340\245\244\245\253\245\255 \244\357\253."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "\221\253\240\243\240\255\245 \255\240 \220\245\240\252\342\216\221 \242\352\340\345\343 \244\357\253"},
    {STRING_CONTINUE,
    "ENTER = \217\340\256\244\352\253\246\240\242\240\255\245"},
    {STRING_QUITCONTINUE,
    "F3 = \210\247\345\256\244  ENTER = \217\340\256\244\352\253\246\240\242\240\255\245"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   \207\240\257\250\341 \255\240 \344\240\251\253: %S"},
    {STRING_SETUPCOPYINGFILES,
     "\224\240\251\253\256\242\245\342\245 \341\245 \247\240\257\250\341\242\240\342..."},
    {STRING_REGHIVEUPDATE,
    "   \216\341\352\242\340\245\254\245\255\357\242\240\255\245 \255\240 \340\245\243\250\341\342\352\340\255\250\342\245 \340\256\245\242\245..."},
    {STRING_IMPORTFILE,
    "   \202\255\240\341\357\255\245 \255\240 %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   \216\341\352\242\340\245\254\245\255\357\242\240\255\245 \340\245\243\250\341\342\340\256\242\250\342\245 \255\240\341\342\340\256\251\252\250 \255\240 \245\252\340\240\255\240..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   \216\341\352\242\340\245\254\245\255\357\242\240\255\245 \255\240 \254\245\341\342\255\250\342\245 \255\240\341\342\340\256\251\252\250..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   \216\341\352\242\340\245\254\245\255\357\242\240\255\245 \255\240\341\342\340\256\251\252\250\342\245 \255\240 \252\253\240\242\250\240\342\343\340\255\250\342\245 \257\256\244\340\245\244\241\250..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   \204\256\241\240\242\357\255\245 \242 \340\245\243\250\341\342\352\340\240 \255\240 \341\242\245\244\245\255\250\357 \247\240 \247\255\240\252\256\242\250\357 \255\240\241\256\340..."},
    {STRING_DONE,
    "   \203\256\342\256\242\256..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = \217\340\245\247\240\257\343\341\252\240\255\245 \255\240 \252\256\254\257\356\342\352\340\240"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "\216\342\242\240\340\357\255\245\342\256 \255\240 \252\256\255\247\256\253\240\342\240 \245 \255\245\242\352\247\254\256\246\255\256\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "\222\256\242\240 \341\245 \341\253\343\347\242\240 \255\240\251- \347\245\341\342\256 \257\340\250 \343\257\256\342\340\245\241\240 \255\240 USB \252\253\240\242\250\240\342\343\340\240\r\n"},
    {STRING_CONSOLEFAIL3,
    "\217\256\244\244\340\352\246\252\240\342\240 \255\240 USB \245 \242\341\245 \256\351\245 \255\245\257\352\253\255\240\r\n"},
    {STRING_FORMATTINGDISK,
    "\222\245\347\245 \344\256\340\254\240\342\250\340\240\255\245 \255\240 \244\250\341\252\240"},
    {STRING_CHECKINGDISK,
    "\222\245\347\245 \257\340\256\242\245\340\252\240 \255\240 \244\250\341\252\240"},
    {STRING_FORMATDISK1,
    " \224\256\340\254\240\342\250\340\240\255\245 \255\240 \244\357\253\240 \252\240\342\256 %S \344\240\251\253\256\242\240 \343\340\245\244\241\240 (\241\352\340\247\256 \344\256\340\254\240\342\250\340\240\255\245) "},
    {STRING_FORMATDISK2,
    " \224\256\340\254\240\342\250\340\240\255\245 \255\240 \244\357\253\240 \252\240\342\256 %S \344\240\251\253\256\242\240 \343\340\245\244\241\240 "},
    {STRING_KEEPFORMAT,
    " \207\240\257\240\247\242\240\255\245 \255\240 \344\240\251\253\256\242\240\342\240 \343\340\245\244\241\240 (\241\245\247 \257\340\256\254\245\255\250) "},
    {STRING_HDINFOPARTCREATE_1,
    "%s."},
    {STRING_HDINFOPARTDELETE_1,
    "\255\240: %s."},
    {STRING_PARTTYPE,
    "\242\250\244 0x%02x"},
    {STRING_HDDINFO_1,
    // "\342\242\352\340\244 \244\250\341\252 %lu (%I64u %s), \210\247\242\256\244=%hu, \230\250\255\240=%hu, \216\223=%hu (%wZ) [%s]"
    "%I64u %s \342\242\352\340\244 \244\250\341\252 %lu (\210\247\242\256\244=%hu, \230\250\255\240=%hu, \216\223=%hu) \255\240 %wZ [%s]"},
    {STRING_HDDINFO_2,
    // "\342\242\352\340\244 \244\250\341\252 %lu (%I64u %s), \210\247\242\256\244=%hu, \230\250\255\240=%hu, \216\223=%hu [%s]"
    "%I64u %s \342\242\352\340\244 \244\250\341\252 %lu (\210\247\242\256\244=%hu, \230\250\255\240=%hu, \216\223=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "\201\245 \341\352\247\244\240\244\245\255 \255\256\242 \244\357\253 \255\240"},
    {STRING_UNPSPACE,
    "\215\245\340\240\247\257\340\245\244\245\253\245\255\256 \254\357\341\342\256"},
    {STRING_MAXSIZE,
    "\214\201 (\244\256 %lu \214\201)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "\215\256\242 (\215\245\344\256\340\254\240\342\250\340\240\255)"},
    {STRING_FORMATUNUSED,
    "\215\245\250\247\257\256\253\247\242\240\255"},
    {STRING_FORMATUNKNOWN,
    "\215\245\250\247\242\245\341\342\245\255"},
    {STRING_KB,
    "\212\201"},
    {STRING_MB,
    "\214\201"},
    {STRING_GB,
    "\203\201"},
    {STRING_ADDKBLAYOUTS,
    "\204\256\241\240\242\357\255\245 \255\240 \252\253\240\242\250\240\342\343\340\255\250 \257\256\244\340\245\244\241\250"},
    {0, 0}
};
