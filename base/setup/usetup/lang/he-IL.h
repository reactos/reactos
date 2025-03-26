// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once
/* Hebrew text is in visual order */

static MUI_ENTRY heILSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
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

static MUI_ENTRY heILLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\204\224\231 \232\230\211\207\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  .\204\220\227\232\204\204 \212\211\214\204\232 \230\205\201\222 \204\224\231\204 \232\200 \230\207\201 \200\220\200",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   .ENTER \231\227\204 \206\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  .\232\211\224\205\221\204 \232\213\230\222\216\201 \214\203\207\216\204 \232\230\211\230\201 \232\224\231 \204\211\204\232 \232\200\206 \204\224\231",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS \232\220\227\232\204\214 \215\211\200\201\204 \215\211\213\205\230\201",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS \204\214\222\224\204\204 \232\213\230\222\216 \232\200 \227\211\232\222\211 \204\220\227\232\204\204 \214\231 \204\206\204 \201\214\231\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        ".\204\220\227\232\204\204 \214\231 \211\220\231\204 \201\214\231\204 \232\200 \217\211\213\232\205 \212\201\231\207\216\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  .ReactOS \232\200 \202\230\203\231\214 \205\200 \217\211\227\232\204\214 \211\203\213 ENTER \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
     // "\x07  Press R to repair a ReactOS installation using the Recovery Console.",
        "\x07  .ReactOS \232\220\227\232\204 \217\227\232\214 \211\203\213 R \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  .ReactOS \214\231 \211\205\231\211\230\204 \211\200\220\232 \232\205\200\230\214 \211\203\213 L \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  .ReactOS \232\200 \217\211\227\232\204\214 \211\214\201\216 \232\200\226\214 \211\203\213 F3 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        ":\230\232\200\201 \230\227\201\214 \217\232\211\220 ,ReactOS \211\201\202\214 \222\203\211\216 \230\232\205\211\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "https://reactos.org/",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \217\205\211\231\230 = L  \204\220\227\232\204 \217\205\227\211\232 = R  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS \232\221\230\202 \201\226\216",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "\232\205\207\213\205\220 \232\205\220\205\213\232\204 \214\213 \200\214\231 \232\205\222\216\231\216 ,\200\224\214\200 \201\214\231\201 \232\200\226\216\220 ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "\230\205\201\222 \227\230 \232\213\230\222\216\201 \231\216\232\231\204\214 \225\214\216\205\216 .\207\205\232\211\224 \232\207\232\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        ".\215\205\211 \215\205\211 \231\205\216\211\231 \230\205\201\222 \232\213\230\222\216 \212\205\232\201 \200\214\205 \232\205\221\220\232\204\205 \204\227\211\203\201 \232\205\230\210\216\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "\211\220\231\216 \201\231\207\216\201 \231\216\232\231\204\214 \205\200 \212\214\231 \222\203\211\216\204 \232\200 \232\205\201\202\214 \225\214\216\205\216",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        ".\232\211\206\211\224 \204\230\216\205\207 \214\222 ReactOS \232\200 \225\211\230\204\214 \232\220\216 \214\222",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  .ReactOS \232\220\227\232\204 \232\200 \212\211\231\216\204\214 \211\203\213 ENTER \231\227\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  .ReactOS \232\200 \217\211\227\232\204\214 \211\214\201\216 \232\200\226\214 \211\203\213 F3 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        ":\211\205\231\211\230",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "the original license.",
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
        "If for some reason you did not receive a copy of the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
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
        ":\232\205\211\230\207\200",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
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
        "\215\203\205\227 \223\203 = ENTER",
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

static MUI_ENTRY heILDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\215\211\211\207\213\205\220\204 \215\211\220\227\232\204\204 \232\205\230\203\202\204 \232\200 \204\200\230\216 \204\210\216\214 \204\216\211\231\230\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        ":\201\231\207\216",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        ":\204\202\205\226\232",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        ":\232\203\214\227\216",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        ":\232\203\214\227\216 \232\230\205\226\232",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        ":\230\205\231\211\200",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "\215\211\220\227\232\204\204 \232\205\230\203\202\204 \230\205\231\211\200",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "\220\211\232\217 \214\231\220\205\232 \200\232 \204\202\203\230\205\232 \204\204\232\227\220\211\215 \201\214\207\211\226\232 \216\227\231\211 \204\214\216\222\214\204 \205\214\216\210\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        ".\204\220\205\231 \204\230\203\202\204 \230\205\207\201\214 \211\203\213 ENTER \225\207\214 \206\200 .\217\227\232\204 \230\205\207\201\214 \211\203\213",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "\"\215\211\220\227\232\204\204 \232\205\230\203\202\204 \230\205\231\211\200\"\201 \230\207\201 ,\232\205\220\205\213\220 \232\205\230\203\202\204\204 \214\213\231 \211\232\216",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        ".ENTER \225\207\214\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\215\203\227\205\216 \207\205\232\211\224 \201\226\216\201 \232\200\226\216\220 ReactOS \232\220\227\232\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        ".\232\211\220\227\232 \204\220\227\232\204 \232\211\220\213\232 \214\231 \232\205\220\205\213\232\204 \214\213 \232\200 \232\230\231\224\200\216 \204\220\211\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        ".\232\205\216\231\205\211\216 \217\211\211\203\222 \215\220\211\200 \217\205\227\211\232\204 \232\205\220\205\213\232",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  .\232\213\230\222\216\204 \232\200 \217\213\203\222\214 \211\203\213 U \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  .\230\205\206\207\231\204 \223\205\221\216 \232\200 \207\205\232\224\214 \211\203\213 R \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  .\211\231\200\230\204 \203\205\216\222\214 \230\205\206\207\214 \211\203\213 ESC \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  .\201\231\207\216\204 \232\200 \231\203\207\216 \214\211\222\224\204\214 \211\203\213 ENTER \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\231\203\207\216 \204\214\222\224\204 = ENTER  \230\205\206\207\231 = R  \217\205\213\203\222 = U  \211\231\200\230 \203\205\216\222 = ESC",
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

static MUI_ENTRY heILUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS \232\205\220\227\232\204\216 \232\207\200 \232\200 \202\230\203\231\214 \204\214\205\213\211 ReactOS \232\220\227\232\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\204\220\227\232\204\204 \232\211\220\213\232 ,\204\216\205\202\224 \232\205\220\227\232\204\204\216 \232\207\200 \215\200 \205\200 ,\204\210\216\214 \232\205\216\205\231\230\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        ".\204\220\227\232\214 \232\205\221\220\214 \204\214\205\213\211",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        ".\204\200\205\214\216\201 \232\216\231\205\211\216 \204\220\211\200 \217\205\227\211\232\204 \232\213\230\222\216",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  .\204\220\227\232\204 \230\205\207\201\214 \211\203\213 \204\210\216\214 \205\200 \204\214\222\216\214 \214\222 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  .\232\230\207\201\220\204 \232\213\230\222\216\204 \232\220\227\232\204 \232\200 \202\230\203\231\214 \211\203\213 U \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  .\204\231\203\207 \232\213\230\222\216 \232\220\227\232\204 \215\222 \212\211\231\216\204\214 \211\203\213 ESC \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  .ReactOS \232\200 \217\211\227\232\204\214 \211\214\201\216 \232\200\226\214 \211\203\213 F3 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \202\230\203\231\214 \200\214 = ESC  \202\205\230\203\231 = ESC",
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

static MUI_ENTRY heILComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\217\227\232\205\216\204 \201\231\207\216\204 \202\205\221 \232\200 \232\205\220\231\214 \204\226\205\230 \204\232\200",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  .\211\205\226\230\204 \201\231\207\216\204 \202\205\221 \232\200 \230\205\207\201\214 \211\203\213 \204\210\216\214 \205\200 \204\214\222\216\214 \214\222 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   .ENTER \225\207\214 \206\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  .\201\231\207\216\204 \202\205\221 \232\200 \232\205\220\231\214 \211\214\201\216 \215\203\205\227\204 \203\205\216\222\214 \230\205\206\207\214 \211\203\213 ESC \214\222 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \214\205\210\211\201 = ESC  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\212\214\231 \217\220\205\213\204 \214\222 \230\205\216\231 \222\203\211\216\204 \214\213\231 \200\203\205\205\232 \205\211\231\213\222 \232\213\230\222\216\204.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "\232\205\227\203 \204\216\213 \232\207\227\214 \214\205\213\211 \204\206.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\211\210\216\205\210\205\200 \217\224\205\200\201 \231\203\207\216 \214\222\224\205\211 \201\231\207\216\204 ,\215\205\211\221\201.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\217\205\216\210\216\204 \232\200 \223\210\205\231",
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

static MUI_ENTRY heILQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\200\214\216 \217\224\205\200\201 \232\220\227\232\205\216 \200\214 ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        " :A \217\220\205\213\216 \217\205\210\211\214\227\232\204 \232\200 \200\211\226\205\204\214 \200\220",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        ".\215\211\220\220\205\213\204\216 \215\211\230\205\210\211\214\227\232\204 \214\213 \232\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        ".\201\231\207\216\204 \232\200 \231\203\207\216 \214\211\222\224\204\214 \211\203\213 ENTER \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "...\217\211\232\216\204\214 \200\220",
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

static MUI_ENTRY heILDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\217\227\232\205\216\204 \204\202\205\226\232\204 \202\205\221 \232\200 \232\205\220\231\214 \212\220\205\226\230\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  .\211\205\226\230\204 \204\202\205\226\232\204 \202\205\221 \232\200 \230\205\207\201\214 \211\203\213 \204\210\216\214 \205\200 \204\214\222\216\214 \214\222 \225\207\214",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   .ENTER \225\207\214 \206\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  .\204\202\205\226\232\204 \202\205\221 \232\200 \232\205\220\231\214 \211\214\201\216 \230\205\206\207\214 \211\203\213 ESC \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \214\205\210\211\201 = ESC  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        ".\204\207\214\226\204\201 \205\220\227\232\205\204 ReactOS \214\231 \215\211\211\221\211\221\201\204 \215\211\201\211\213\230\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        " :A \217\220\205\213\216 \217\205\210\211\214\227\232\204 \232\200 \200\211\226\205\204\214 \200\220",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        ".\215\211\220\220\205\213\204\216 \215\211\230\205\210\211\214\227\232\204 \214\213 \232\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        ".\201\231\207\216\204 \232\200 \231\203\207\216 \214\211\222\224\204\214 \211\203\213 ENTER \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\231\203\207\216 \204\214\222\224\204 = ENTER",
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

static MUI_ENTRY heILSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\214\226\205\220\216 \200\214 \227\221\211\203 \207\210\231\205 \232\205\226\211\207\216 \215\211\202\226\205\216 \204\210\216\214 \204\216\211\231\230\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        ".\232\205\231\203\207 \232\205\226\211\207\216 \230\205\201\222",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  .\204\216\205\231\230 \230\205\207\201\214 \211\203\213 \204\210\216\214 \205\200 \204\214\222\216\214 \214\222 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  .\232\230\207\201\220\204 \204\226\211\207\216\204 \214\222 ReactOS \232\200 \217\211\227\232\204\214 \211\203\213 ENTER \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  .\232\211\202\205\214/\232\211\231\200\230 \204\226\211\207\216 \230\205\226\211\214 \211\203\213 C \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  .\232\201\207\230\205\216 \204\226\211\207\216 \230\205\226\211\214 \211\203\213 E \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  .\232\216\211\211\227 \204\226\211\207\216 \227\205\207\216\214 \211\203\213 D \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "...\217\211\232\216\204\214 \200\220",
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

static MUI_ENTRY heILChangeSystemPartition[] =
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

static MUI_ENTRY heILConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\232\213\230\222\216 \232\226\211\207\216 \227\205\207\216\214 \232\230\207\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        ",\204\230\216\205\207 \232\230\203\202\204 \232\205\211\220\213\205\232 ,\217\205\207\201\200 \232\205\211\220\213\205\232 \214\211\213\204\214 \232\205\214\205\213\211 \232\213\230\222\216 \232\205\226\211\207\216",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        ")ReactOS \205\216\213( \204\214\222\224\204 \232\205\213\230\222\216 \232\214\222\224\204\214 \232\205\211\220\213\232",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        ".\204\230\216\205\207\204 \217\230\226\211 \211\203\211 \214\222 \232\205\227\224\205\221\216\231 \232\205\230\207\200 \232\205\211\220\213\232 \205\200",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\205\214\200\213 \232\205\211\220\213\232 \217\211\200\231 \207\205\210\201 \204\232\200 \215\200 \227\230 \232\213\230\222\216 \232\226\211\207\216 \227\207\216",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        ".\217\232\205\200 \227\205\207\216\214 \207\205\210\201 \204\232\200\231 \205\200 ,\204\226\211\207\216\204 \214\222",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "\207\211\231\227\204 \217\220\205\213\204\216 \232\205\214\222\214 \200\214 \214\205\214\222 \201\231\207\216\204 ,\204\226\211\207\216\204 \227\207\216\232\231 \211\232\216",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        ".ReactOS \232\220\227\232\204 \215\205\211\221\214 \203\222",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  \204\227\211\207\216\214 \211\205\203\211\205 \231\227\201\232\211 .\232\213\230\222\216\204 \232\226\211\207\216 \232\200 \227\205\207\216\214 \211\203\213 ENTER \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   .\204\220\227\232\204\201 \230\232\205\211 \230\207\205\200\216 \203\222\205\216\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  .\227\207\216\211\232 \200\214 \204\226\211\207\216\204 ,\215\203\205\227\204 \203\205\216\222\214 \230\205\206\207\214 \211\203\213 ESC \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\214\205\210\211\201 = ESC  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\204\226\211\207\216 \214\205\207\232\200",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        ".\212\211\231\216\204\214 \211\203\213 ENTER \225\207\214 .\204\226\211\207\216\204 \232\200 \214\207\232\200\232 \205\211\231\213\222 \204\220\227\232\204\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\204\230\207\201\220\231 \204\226\211\207\216\204 \232\200 \232\227\203\205\201 \205\211\231\213\222 \204\220\227\232\204\204 \232\211\220\213\232",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "...\217\211\232\216\204\214 \200\220",
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

static MUI_ENTRY heILInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\232\230\207\201\220\204 \204\226\211\207\216\204 \214\222 ReactOS \232\200 \217\211\227\232\232 \204\220\227\232\204\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        ":ReactOS \232\200 \217\211\227\232\204\214 \212\220\205\226\230\201 \204\201 \204\211\211\227\211\232 \230\207\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\227\205\207\216\214 \211\203\213 BACKSPACE \225\207\214 ,\232\222\226\205\216\204 \204\211\211\227\211\232\204 \232\200 \232\205\220\231\214 \211\203\213",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        ".ReactOS \232\200 \217\211\227\232\204\214 \212\220\205\226\230\201 \204\201 \204\211\211\227\211\232\204 \232\200 \215\205\231\230\232 \206\200\205 \215\211\205\205\232",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "\215\211\226\201\227 \204\227\211\232\222\216 ReactOS \232\220\227\232\204\231 \217\216\206\201 \217\211\232\216\204\214 \200\220",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        ".\232\230\207\201\231 \204\220\227\232\204\204 \232\211\211\227\211\232\214",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        ".\232\205\227\203 \230\224\221\216 \212\231\216\204\214 \204\214\205\213\211 \205\206 \204\214\205\222\224",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 ...\217\211\232\216\204\214 \200\220    ",
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

static MUI_ENTRY heILBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\214\205\207\232\200\204 \214\204\220\216 \232\200 \232\222\213 \204\220\211\227\232\216 \204\220\227\232\204\204 \232\211\220\213\232.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        ".(VBR-\205 MBR) \207\211\231\227\204 \227\221\211\203 \214\222 \214\205\207\232\200 \214\204\220\216 \217\227\232\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        ".(\203\201\214\201 VBR) \207\211\231\227\204 \227\221\211\203 \214\222 \214\205\207\232\200 \214\204\220\216 \217\227\232\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        ".\217\205\210\211\214\227\232 \214\222 \214\205\207\232\200 \214\204\220\216 \217\227\232\204",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        ".\214\205\207\232\200\204 \214\204\220\216 \232\220\227\232\204 \214\222 \202\205\214\211\203",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILBootLoaderInstallPageEntries[] =
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
        "\214\205\207\232\200\204 \214\204\220\216 \232\200 \232\222\213 \204\220\211\227\232\216 \204\220\227\232\204\204 \232\211\220\213\232.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Installing the bootloader onto the media, please wait...",
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

static MUI_ENTRY heILBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\214\205\207\232\200\204 \214\204\220\216 \232\200 \232\222\213 \204\220\211\227\232\216 \204\220\227\232\204\204 \232\211\220\213\232.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        ".ENTER \225\207\214\205 :A \217\220\205\213\214 \214\207\232\205\200\216 \217\205\210\211\214\227\232 \221\220\213\204 \204\231\227\201\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\232\220\227\232\205\216\204 \232\203\214\227\216\204 \202\205\221 \232\205\220\231\214 \212\220\205\226\230\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  .\211\205\226\230\204 \232\203\214\227\216\204 \202\205\221 \232\200 \230\205\207\201\214 \211\203\213 \204\210\216\214 \205\200 \204\214\222\216\214 \214\222 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   .ENTER \225\207\214 \206\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  .\232\203\214\227\216\204 \202\205\221 \232\200 \232\205\220\231\214 \211\214\201\216 \215\203\205\227\204 \203\205\216\222\214 \230\205\206\207\214 \211\203\213 ESC \214\222 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \214\205\210\211\201 = ESC  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".\214\203\207\216 \232\230\211\230\201 \232\220\227\232\205\216\204 \232\203\214\227\216\204 \232\230\205\226\232 \232\200 \230\205\207\201\214 \200\220",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  .\204\211\205\226\230\204 \204\230\205\226\232\204 \232\200 \230\205\207\201\214 \211\203\213 \204\210\216\214 \205\200 \204\214\222\216\214 \214\222 \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    .ENTER \225\207\214 \206\200\205",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  .\232\203\214\227\216\204 \232\230\205\226\232 \232\200 \232\205\220\231\214 \211\214\201\216 \215\203\205\227\204 \203\205\216\222\214 \230\205\206\207\214 \211\203\213 ESC \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \214\205\210\211\201 = ESC  \212\231\216\204 = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY heILPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        ".ReactOS \211\226\201\227 \232\227\232\222\204\214 \212\201\231\207\216 \232\200 \204\220\211\213\216 \204\220\227\232\204\204 \232\211\220\213\232 ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "...\215\211\226\201\227 \232\227\232\222\204 \232\216\211\231\230 \204\220\205\201",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY heILSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        ".\204\210\216\214 \204\216\211\231\230\204\216 \215\211\226\201\227 \232\213\230\222\216 \230\207\201",
        0
    },
    {
        8,
        19,
        "\x07  .\215\211\226\201\227 \232\213\230\222\216 \230\205\207\201\214 \211\203\213 \204\210\216\214 \205\200 \204\214\222\216\214 \214\222 \225\207\214",
        0
    },
    {
        8,
        21,
        "\x07  .\204\226\211\207\216\204 \232\200 \214\207\232\200\214 \211\203\213 ENTER \225\207\214",
        0
    },
    {
        8,
        23,
        "\x07  .\232\230\207\200 \204\226\211\207\216 \230\205\207\201\214 \211\203\213 ESC \225\207\214",
        0
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \214\205\210\211\201 = ESC  \212\231\216\204 = ENTER",
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

static MUI_ENTRY heILDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\204\226\211\207\216\204 \232\200 \227\205\207\216\214 \232\230\207\201",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  .\204\226\211\207\216\204 \232\200 \227\205\207\216\214 \211\203\213 L \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "!\203\201\200\211 \204\226\211\207\216\201 \222\203\211\216\204 \214\213 :\204\230\204\206\200",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  .\214\210\201\214 \211\203\213 ESC \225\207\214",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \214\205\210\211\201 = ESC  \204\226\211\207\216 \227\207\216 = L",
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

static MUI_ENTRY heILRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " \232\220\227\232\204 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        " .\232\213\230\222\216\204 \232\230\205\226\232 \232\200 \232\213\203\222\216 \204\220\227\232\204\204 \232\211\220\213\232",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "...\232\216\205\231\211\230 \232\205\230\205\205\213 \230\226\205\211",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR heILErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "\204\207\214\226\204\n"
    },
    {
        // ERROR_NOT_INSTALLED
        ".\212\201\231\207\216 \214\222 \200\214\216 \217\224\205\200\201 \232\220\227\232\205\216 \200\214 ReactOS\n"
        "\201\205\231 \204\220\227\232\204\204 \232\200 \214\211\222\224\204\214 \212\230\210\226\232 ,\232\222\213 \204\220\227\232\204\204\216 \200\226\232 \215\200\n"
        ".ReactOS \232\200 \217\211\227\232\204\214 \211\203\213\n"
        "\n"
        "  \x07  .\204\220\227\232\204\204\201 \212\211\231\216\204\214 \211\203\213 ENTER \225\207\214\n"
        "  \x07  .\204\220\227\232\204\204\216 \232\200\226\214 \211\203\213 F3 \225\207\214",
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER"
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
        ".\207\211\231\227 \217\220\205\213 \204\200\226\216 \200\214 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        ".\204\214\231 \230\205\227\216\204 \217\220\205\213 \232\200 \204\200\226\216 \200\214 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        ".TXTSETUP.SIF \225\201\205\227 \232\200 \204\200\226\216 \200\214 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        ".\215\205\202\224 TXTSETUP.SIF \225\201\205\227\204 \232\200 \204\200\226\216 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        ".TXTSETUP.SIF-\201 \204\211\205\202\231 \232\216\232\205\207 \204\200\226\216 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_DRIVE_INFORMATION
        ".\232\213\230\222\216\204 \214\231 \215\211\220\220\205\213\204 \222\203\211\216 \232\200 \202\211\231\204\214 \204\214\213\211 \200\214 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_WRITE_BOOT,
        ".\232\213\230\222\216\204 \232\226\211\207\216 \214\222 %S \214\231 \214\205\207\232\200\204 \203\205\227 \232\200 \217\211\227\232\204\214\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_LOAD_COMPUTER,
        ".\215\211\201\231\207\216\204 \211\202\205\221 \232\216\211\231\230 \232\220\211\222\210\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_LOAD_DISPLAY,
        ".\204\202\205\226\232\204 \232\205\230\203\202\204 \211\202\205\221 \232\216\211\231\230 \232\220\211\222\210\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        ".\232\205\203\214\227\216\204 \211\202\205\221 \232\216\211\231\230 \232\220\211\222\210\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        ".\232\203\214\227\216\204 \232\205\230\205\226\232 \232\216\211\231\230 \232\220\211\222\210\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_WARN_PARTITION,
        "\203\207\200 \207\211\231\227 \227\221\211\203 \232\205\207\224\214 \204\200\226\216 \204\220\227\232\204\204 \232\211\220\213\232\n"
        "!\205\201 \232\205\226\211\207\216\204 \232\214\201\210\201 \212\211\230\226\231 \205\216\213 \231\216\232\231\204\214 \217\232\211\220 \200\214\231\n"
        "\n"
        ".\232\205\226\211\207\216\204 \232\214\201\210 \232\200 \203\211\216\231\204\214 \204\214\205\214\222 \204\226\211\207\216 \232\227\211\207\216 \205\200 \232\230\211\226\211\n"
        "\n"
        "  \x07  .\204\220\227\232\204\204 \214\205\210\211\201\214 F3 \225\207\214\n"
        "  \x07  .\212\211\231\216\204\214 \211\203\213 ENTER \225\207\214",
        "\204\220\227\232\204 \214\205\210\211\201 = F3  \212\231\216\204 = ENTER"
    },
    {
        // ERROR_NEW_PARTITION,
        "!\232\216\211\211\227 \204\226\211\207\216 \212\205\232\201 \204\226\211\207\216 \230\205\226\211\214 \217\232\211\220 \200\214\n"
        "\n"
        ".\212\211\231\216\204\214 \211\203\213 \231\227\216 \214\222 \225\207\214 *  ",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        ".\232\213\230\222\216\204 \232\226\211\207\216 \214\222 %S \230\205\201\222 \214\205\207\232\200 \203\205\227 \232\220\227\232\204\201 \204\231\214\213\220 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_NO_FLOPPY,
        ".:A \217\220\205\213\201 \217\205\210\211\214\227\232 \217\211\200",
        "\212\231\216\204 = ENTER"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        ".\232\203\214\227\216\204 \232\230\205\226\232 \232\205\230\203\202\204 \232\200 \217\213\203\222\214\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        ".\204\202\205\226\232\204 \215\205\231\230 \232\205\230\203\202\204 \232\200 \217\213\203\222\214\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_IMPORT_HIVE,
        ".\215\205\231\211\230 \232\230\205\205\213 \225\201\205\227 \200\205\201\211\211\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_FIND_REGISTRY
        "\232\213\220\211\232 \204\204\232\227\220\204 \220\213\231\214\204 \201\214\216\226\205\200 \200\232 \227\201\226\211 \216\211\203\222 \204\230\211\231\205\215.",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_CREATE_HIVE,
        ".\215\205\231\211\230\204 \232\205\230\205\205\213 \232\230\211\226\211\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        ".\215\205\231\211\230\204 \214\205\207\232\200\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        ".inf \225\201\205\227 \217\211\200 Cabinet \225\201\205\227\214\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_CABINET_MISSING,
        ".\200\226\216\220 \200\214 Cabinet \225\201\205\227\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "\214\227\205\201\225 Cabinet \200\211\217 \232\221\230\211\210 \204\232\227\220\204.\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_COPY_QUEUE,
        ".\215\211\201\226\227 \232\227\232\222\204 \230\205\232 \232\207\211\232\224\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_CREATE_DIR,
        ".\204\220\227\232\204\204 \232\205\211\227\211\232 \232\200 \230\205\226\211\214 \204\214\213\211 \200\214 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        ".TXTSETUP.SIF\201 '%S' \227\214\207 \232\200 \200\205\226\216\214 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_CABINET_SECTION,
        "'%S' \227\214\207 \232\200 \200\205\226\216\214 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n"
        ".Cabinet \225\201\205\227\201\n",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        ".\204\220\227\232\204\204 \204\211\211\227\211\232 \232\200 \232\200 \230\205\226\211\214 \204\214\213\211 \200\214 \204\220\227\232\204\204 \232\211\220\213\232",
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_WRITE_PTABLE,
        ".\232\205\226\211\207\216\204 \232\214\201\210 \232\201\211\232\213\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n"
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        ".\215\205\231\211\230\214 codepage \232\224\221\205\204\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n"
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        ".\232\213\230\222\216\204 \230\205\206\200 \232\200 \222\205\201\227\214 \204\214\213\211 \200\214 \204\220\227\232\204\204 \232\211\220\213\232n"
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        ".\215\205\231\211\230\214 \232\203\214\227\216 \232\205\230\205\226\232 \232\224\221\205\204\201 \204\214\231\213\220 \204\220\227\232\204\204 \232\211\220\213\232\n"
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_UPDATE_GEOID,
        ".geo id \204 \232\200 \222\205\201\227\214 \204\214\213\211 \200\214 \204\220\227\232\204\204 \232\211\220\213\232\n"
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        // ERROR_DIRECTORY_NAME,
        ".\211\227\205\207 \200\214 \204\211\211\227\211\232 \215\231\n"
        "\n"
        ".\212\211\231\216\204\214 \211\203\213 \231\227\216 \214\222 \225\207\214 *  "
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        ".ReactOS \232\200 \217\211\227\232\204\214 \211\203\213 \227\211\224\221\216 \204\214\205\203\202 \200\214 \204\230\207\201\220\231 \204\226\211\207\216\204\n"
        ".\201\"\216 %lu \214\231 \214\203\205\202\201 \232\205\211\204\214 \232\205\207\224\214 \232\201\211\211\207 \204\220\227\232\204\204 \225\226\211\207\216\n"
        "\n"
        ".\212\211\231\216\204\214 \211\203\213 \231\227\216 \214\222 \225\207\214 *  ",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "\214\200 \220\211\232\217 \214\211\226\205\230 \216\207\211\226\204 \207\203\231\204 \200\205 \216\207\211\226\204 \216\205\230\207\201\232 \207\203\231\204\n"
        ".\232\205\224\221\205\220 \232\205\226\211\207\216\214 \215\205\227\216 \200\214\214 \204\220\211\204 \204\214\201\210\204 \211\213 \232\205\226\211\207\216\204 \232\214\201\210 \214\222\n"
        "\n"
        ".\212\211\231\216\204\214 \211\203\213 \231\227\216 \214\222 \225\207\214 *  "
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        ".\207\211\231\227 \217\220\205\213 \214\213\214 \232\207\200 \232\201\207\230\205\216 \204\226\211\207\216\216 \230\232\205\211 \230\205\226\211\214 \217\232\211\220 \200\214\n"
        "\n"
        ".\212\211\231\216\204\214 \211\203\213 \231\227\216 \214\222 \225\207\214 *  "
    },
    {
        // ERROR_FORMATTING_PARTITION,
        ":\204\226\211\207\216\204 \232\200 \214\207\232\200\214 \204\214\213\211 \200\214 \204\220\227\232\204\204 \232\211\220\213\232\n"
        " %S\n"
        "\n"
        "\201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE heILPages[] =
{
    {
        SETUP_INIT_PAGE,
        heILSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        heILLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        heILWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        heILIntroPageEntries
    },
    {
        LICENSE_PAGE,
        heILLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        heILDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        heILRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        heILUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        heILComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        heILDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        heILFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        heILSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        heILChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        heILConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        heILSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        heILFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        heILCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        heILDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        heILInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        heILPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        heILFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        heILKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        heILBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        heILLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        heILQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        heILSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        heILBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        heILBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        heILRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING heILStrings[] =
{
    {STRING_PLEASEWAIT,
     "   ...\217\211\232\216\204\214 \200\220"},
    {STRING_INSTALLCREATEPARTITION,
     "   \204\220\227\232\204 \214\205\210\211\201 = F3  \232\201\207\230\205\216 \230\205\226 = E  \232\211\231\200\230 \230\205\226 = C  \217\227\232\204 = ENTER"},
    {STRING_INSTALLCREATELOGICAL,
     "   \204\220\227\232\204 \214\205\210\211\201 = F3  \232\211\202\205\214 \204\226\211\207\216 \230\205\226 = C  \217\227\232\204 = ENTER"},
    {STRING_INSTALLDELETEPARTITION,
     "   \204\220\227\232\204 \214\205\210\211\201 = F3  \204\226\211\207\216 \227\207\216 = D  \217\227\232\204 = ENTER"},
    {STRING_DELETEPARTITION,
     "   \204\220\227\232\204 \214\205\210\211\201 = F3  \204\226\211\207\216 \227\207\216 = D"},
    {STRING_PARTITIONSIZE,
     ":\204\231\203\207 \204\226\211\207\216 \214\231 \214\203\205\202"},
    {STRING_CHOOSE_NEW_PARTITION,
     "\214\222 \232\211\230\227\211\222 \204\226\211\207\216 \230\205\226\211\214 \232\230\207\201"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "\214\222 \232\201\207\230\205\216 \204\226\211\207\216 \230\205\226\211\214 \232\230\207\201"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "\214\222 \232\211\202\205\214 \204\226\211\207\216 \230\205\226\211\214 \232\230\207\201"},
    {STRING_HDPARTSIZE,
    ".\215\211\232\201-\204\202\216\201 \204\231\203\207\204 \204\226\211\207\216\204 \214\231 \214\203\205\202\204 \232\200 \203\214\227\204 \204\231\227\201\201"},
    {STRING_CREATEPARTITION,
     "   \204\220\227\232\204 \214\205\210\211\201 = F3  \214\205\210\211\201 = ESC  \204\226\211\207\216 \230\205\226 = ENTER"},
    {STRING_NEWPARTITION,
    "\214\222 \204\231\203\207 \204\226\211\207\216 \204\230\226\211 \204\220\227\232\204\204 \232\211\220\213\232"},
    {STRING_PARTFORMAT,
    ".\212\231\216\204\201 \214\207\232\205\200\232 \232\200\206\204 \204\226\211\207\216\204"},
    {STRING_NONFORMATTEDPART,
    ".\232\214\207\232\205\200\216 \200\214/\204\231\203\207 \204\226\211\207\216 \214\222 ReactOS \232\200 \217\211\227\232\204\214 \232\230\207\201"},
    {STRING_NONFORMATTEDSYSTEMPART,
    ".\232\214\207\232\205\200\216 \200\214 \217\211\211\203\222 \232\213\230\222\216\204 \232\226\211\207\216"},
    {STRING_NONFORMATTEDOTHERPART,
    ".\232\214\207\232\205\200\216 \200\214 \217\211\211\203\222 \204\231\203\207\204 \204\226\211\207\216\204"},
    {STRING_INSTALLONPART,
    "\204\226\211\207\216 \214\222 ReactOS \232\200 \204\220\211\227\232\216 \204\220\227\232\204\204 \232\211\220\213\232"},
    {STRING_CONTINUE,
    "\212\231\216\204 = ENTER"},
    {STRING_QUITCONTINUE,
    "\212\231\216\204 = ENTER  \204\220\227\232\204 \214\205\210\211\201 = F3"},
    {STRING_REBOOTCOMPUTER,
    "\231\203\207\216 \204\214\222\224\204 = ENTER"},
    {STRING_DELETING,
     "   \225\201\205\227 \227\207\205\216: %S"},
    {STRING_MOVING,
     "   \225\201\205\227 \230\211\201\222\216: %S \214: %S"},
    {STRING_RENAMING,
     "   \225\201\205\227 \215\231 \204\220\231\216: %S \214: %S"},
    {STRING_COPYING,
     "   \225\201\205\227 \227\211\232\222\216: %S"},
    {STRING_SETUPCOPYINGFILES,
     "...\215\211\226\201\227 \204\227\211\232\222\216 \204\220\227\232\204\204 \232\211\220\213\232"},
    {STRING_REGHIVEUPDATE,
    "   ...\215\205\231\211\230\204 \232\205\230\205\205\213 \232\200 \217\213\203\222\216"},
    {STRING_IMPORTFILE,
    "   \200\201\211\211\216 %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   ...\204\202\205\226\232\204 \214\231 \215\205\231\211\230 \232\205\230\203\202\204 \217\213\203\222\216"},
    {STRING_LOCALESETTINGSUPDATE,
    "   ...\230\205\206\200 \232\205\230\203\202\204 \217\213\203\222\216"},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   ...\232\203\214\227\216\204 \232\230\205\226\232 \232\205\230\203\202\204 \232\200 \217\213\203\222\216"},
    {STRING_CODEPAGEINFOUPDATE,
    "   ...codepage \222\203\211\216 \223\211\221\205\216"},
    {STRING_DONE,
    "   ...\215\211\211\221\216"},
    {STRING_REBOOTCOMPUTER2,
    "   \201\231\207\216\204 \232\200 \231\203\207\216 \214\222\224\204 = ENTER"},
    {STRING_REBOOTPROGRESSBAR,
    " ...\232\205\211\220\231 %li \203\205\222\201 \231\203\207\216 \214\222\224\205\211 \212\201\231\207\216 "},
    {STRING_CONSOLEFAIL1,
    "\223\205\221\216\204 \232\200 \207\205\232\224\214 \217\232\211\220 \200\214\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "USB \232\203\214\227\216\201 \231\205\216\211\231 \200\205\204 \204\206 \214\231 \204\226\205\224\220 \211\213\204 \204\201\211\221\204\r\n"},
    {STRING_CONSOLEFAIL3,
    "\200\214\216 \217\224\205\200\201 \232\205\213\216\232\220 \200\214 \217\211\211\203\222 USB \232\205\203\214\227\216\r\n"},
    {STRING_FORMATTINGPART,
    "Setup is formatting the partition..."},
    {STRING_CHECKINGDISK,
    "Setup is checking the disk..."},
    {STRING_FORMATDISK1,
    " (\230\211\204\216 \214\205\207\232\200) %S \215\211\226\201\227 \232\213\230\222\216 \215\222 \204\226\211\207\216 \214\205\207\232\200 "},
    {STRING_FORMATDISK2,
    " %S \215\211\226\201\227 \232\213\230\222\216 \215\222 \204\226\211\207\216 \214\205\207\232\200 "},
    {STRING_KEEPFORMAT,
    " (\211\205\220\211\231 \200\214\214( \232\211\207\213\205\220 \215\211\226\201\227 \232\213\230\222\216 \232\230\200\231\204 "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "on %s."},
    {STRING_PARTTYPE,
    "Type 0x%02x"},
    {STRING_HDDINFO1,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Unpartitioned space"},
    {STRING_MAXSIZE,
    "\216\"\201 (\216\227\221. ul% \216\"\201)"},
    {STRING_EXTENDED_PARTITION,
    "\232\201\207\230\205\216 \204\226\211\207\216"},
    {STRING_UNFORMATTED,
    ")\214\207\232\205\200\216 \200\214( \231\203\207"},
    {STRING_FORMATUNUSED,
    "\231\205\216\211\231\201 \200\214"},
    {STRING_FORMATUNKNOWN,
    "\222\205\203\211 \200\214"},
    {STRING_KB,
    "\201\"\227"},
    {STRING_MB,
    "\201\"\216"},
    {STRING_GB,
    "\201\"\202"},
    {STRING_ADDKBLAYOUTS,
    "...\232\203\214\227\216 \232\205\230\205\226\232 \223\211\221\205\216"},
    {0, 0}
};
