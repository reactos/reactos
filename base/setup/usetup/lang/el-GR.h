// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY elGRSetupInitPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
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

static MUI_ENTRY elGRLanguagePageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\204\247\240\242\246\232\343 \232\242\351\251\251\230\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        7,
        10,
        "\x07 \217\230\250\230\241\230\242\351 \234\247\240\242\342\245\253\234 \253\236 \232\242\351\251\251\230 \247\246\254 \237\230 \256\250\236\251\240\243\246\247\246\240\236\237\234\345 \241\230\253\341 \253\236\244 \234\232\241\230\253\341\251\253\230\251\236.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        7,
        11,
        "  \213\234\253\341 \247\230\253\343\251\253\234 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        7,
        13,
        "\x07  \200\254\253\343 \236 \232\242\351\251\251\230 \237\230 \234\345\244\230\240 \236 \247\250\246\234\247\240\242\234\232\243\342\244\236 \232\240\230 \253\246 \253\234\242\240\241\346 \251\347\251\253\236\243\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230  F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRWelcomePageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        5,
        8,
        "\211\230\242\351\252 \216\250\345\251\230\253\234 \251\253\236\244 \234\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        5,
        11,
        "\200\254\253\346 \253\246 \243\342\250\246\252 \253\236\252 \234\232\241\230\253\341\251\253\230\251\236\252 \230\244\253\240\232\250\341\255\234\240 \253\246 \242\234\240\253\246\254\250\232\240\241\346 \251\347\251\253\236\243\230 ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        5,
        12,
        "\251\253\246\244 \254\247\246\242\246\232\240\251\253\343 \251\230\252 \241\230\240 \247\250\246\234\253\246\240\243\341\235\234\240 \253\246 \233\234\347\253\234\250\246 \243\342\250\246\252 \253\236\252 \234\232\241\230\253\341\251\253\230\251\236\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        7,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        7,
        17,
        "\x07  \217\230\253\343\251\253\234 R \232\240\230 \244\230 \234\247\240\233\240\246\250\237\351\251\234\253\234 \253\246 ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        7,
        19,
        "\x07  \217\230\253\343\251\253\234 L \232\240\230 \244\230 \233\234\345\253\234 \253\246\254\252 \346\250\246\254\252 \230\233\234\240\246\233\346\253\236\251\236\252 \253\246\254 ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        7,
        21,
        "\x07  \217\230\253\343\251\253\234 F3 \232\240\230 \244\230 \230\247\246\256\340\250\343\251\234\253\234 \256\340\250\345\252 \244\230 \234\232\241\230\253\230\251\253\343\251\234\253\234 \253\246 ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        5,
        23,
        "\202\240\230 \247\234\250\240\251\251\346\253\234\250\234\252 \247\242\236\250\246\255\246\250\345\234\252 \232\240\230 \253\246 ReactOS, \247\230\250\230\241\230\242\246\347\243\234 \234\247\240\251\241\234\255\237\234\345\253\234 \253\246:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        5,
        24,
        "https://reactos.org/",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230  R = \204\247\240\233\240\346\250\237\340\251\236 F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRIntroPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
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

static MUI_ENTRY elGRLicensePageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "\200\233\234\240\246\233\346\253\236\251\236:",
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
        "Warranty:",
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
        "   ENTER = \204\247\240\251\253\250\246\255\343",
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

static MUI_ENTRY elGRDevicePageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \247\230\250\230\241\341\253\340 \242\345\251\253\230 \233\234\345\256\244\234\240 \253\240\252 \250\254\237\243\345\251\234\240\252 \251\254\251\241\234\254\351\244.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    \223\247\246\242\246\232\240\251\253\343\252:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "       \204\243\255\341\244\240\251\236:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "   \217\242\236\241\253\250\246\242\346\232\240\246:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        2,
        14,
        "\203\240\341\253\230\245\236 \247\242\236\241\253\250\246\242\246\232\345\246\254:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "        \200\247\246\233\246\256\343:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "\200\247\246\233\246\256\343 \230\254\253\351\244 \253\340\244 \250\254\237\243\345\251\234\340\244",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "\213\247\246\250\234\345\253\234 \244\230 \230\242\242\341\245\234\253\234 \253\240\252 \250\254\237\243\345\251\234\240\252 \254\242\240\241\246\347 \247\230\253\351\244\253\230\252 \253\230 \247\242\343\241\253\250\230 \217\200\214\227",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "\343 \211\200\222\227 \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \243\240\230 \250\347\237\243\240\251\236.  ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "\213\234\253\341 \247\230\253\343\251\253\234 \253\246 \247\242\343\241\253\250\246 ENTER \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \341\242\242\234\252 \250\254\237\243\345\251\234\240\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "\356\253\230\244 \346\242\234\252 \246\240 \250\254\237\243\345\251\234\240\252 \234\345\244\230\240 \251\340\251\253\342\252, \234\247\240\242\342\245\253\234 ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\"\200\247\246\233\246\256\343 \230\254\253\351\244 \253\340\244 \250\254\237\243\345\251\234\340\244 \251\254\251\241\234\254\351\244\" \241\230\240 \247\230\253\343\251\253\234 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRRepairPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS \231\250\345\251\241\234\253\230\240 \251\234 \247\250\351\240\243\246 \251\253\341\233\240\246 \230\244\341\247\253\254\245\236\252 \241\230\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\233\234\244 \254\247\246\251\253\236\250\345\235\234\240 \230\241\346\243\230 \346\242\234\252 \253\240\252 \233\254\244\230\253\346\253\236\253\234\252 \243\240\230\252 \247\242\343\250\246\254\252 \234\232\241\230\253\341\251\253\230\251\236\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "\216\240 \242\234\240\253\246\254\250\232\345\234\252 \234\247\240\233\240\346\250\237\340\251\236\252 \233\234\244 \342\256\246\254\244 \254\242\246\247\246\240\236\237\234\345 \230\241\346\243\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  \217\230\253\343\251\253\234 U \232\240\230 \230\244\230\244\342\340\251\236 \253\246\254 \242\234\240\253\250\246\254\250\232\240\241\246\347.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  \217\230\253\343\251\253\234 R \232\240\230 \244\230 \234\241\253\234\242\342\251\234\253\234 \253\236\244 \241\246\244\251\346\242\230 \234\247\240\233\240\346\250\237\340\251\236\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  \217\230\253\343\251\253\234 ESC \232\240\230 \244\230 \234\247\240\251\253\250\342\257\234\253\234 \251\253\236\244 \241\347\250\240\230 \251\234\242\345\233\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  \217\230\253\343\251\253\234 ENTER \232\240\230 \244\230 \234\247\230\244\234\241\241\240\244\343\251\234\253\234 \253\246\244 \254\247\246\242\246\232\240\251\253\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ESC = \211\347\250\240\230 \251\234\242\345\233\230  ENTER = \204\247\230\244\234\241\241\345\244\236\251\236",
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

static MUI_ENTRY elGRUpgradePageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
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

static MUI_ENTRY elGRComputerPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\207\342\242\234\253\234 \244\230 \230\242\242\341\245\234\253\234 \253\246\244 \253\347\247\246 \253\246\254 \254\247\246\242\246\232\240\251\253\343 \247\246\254 \237\230 \234\232\241\230\253\230\251\253\230\237\234\345.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \217\230\253\343\251\253\234 \253\230 \247\242\343\241\253\250\230 \217\200\214\227 \343 \211\200\222\227 \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \253\246\244 \234\247\240\237\254\243\236\253\346",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   \253\347\247\246 \254\247\246\242\246\232\240\251\253\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "   \213\234\253\341 \247\230\253\343\251\253\234 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\x07  \217\230\253\343\251\253\234 \253\246 \247\242\343\241\253\250\246 ESC \232\240\230 \244\230 \234\247\240\251\253\250\342\257\234\253\234 \251\253\236\244 \247\250\246\236\232\246\347\243\234\244\236",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "   \251\234\242\345\233\230 \256\340\250\345\252 \244\230 \230\242\242\341\245\234\253\234 \253\246\244 \253\347\247\246 \254\247\246\242\246\232\240\251\253\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   ESC = \200\241\347\250\340\251\236   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRFlushPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\222\246 \251\347\251\253\236\243\230 \234\247\240\231\234\231\230\240\351\244\234\240 \253\351\250\230 \346\253\240 \346\242\230 \253\230 \233\234\233\246\243\342\244\230 \342\256\246\254\244 \230\247\246\237\236\241\234\254\253\234\345",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "\200\254\253\346 \345\251\340\252 \247\341\250\234\240 \242\345\232\236 \351\250\230",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\356\253\230\244 \246\242\246\241\242\236\250\340\237\234\345, \246 \254\247\246\242\246\232\240\251\253\343\252 \251\230\252 \237\230 \234\247\230\244\234\241\241\240\244\236\237\234\345 \230\254\253\346\243\230\253\230",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \204\241\241\230\237\341\250\240\251\236 \247\250\246\251\340\250\240\244\351\244 \230\250\256\234\345\340\244...",
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

static MUI_ENTRY elGRQuitPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\222\246 ReactOS \233\234\244 \234\232\241\230\253\230\251\253\341\237\236\241\234 \247\242\343\250\340\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "\200\255\230\240\250\342\251\253\234 \253\236 \233\240\251\241\342\253\230 \230\247\346 \253\246 A: \241\230\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\346\242\230 \253\230 CD-ROMs  \230\247\346 \253\230 CD-Drives.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "\217\230\253\343\251\253\234 ENTER \232\240\230 \244\230 \234\247\230\244\234\241\241\240\244\343\251\234\253\234 \253\246\244 \254\247\246\242\246\232\240\251\253\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \217\230\250\230\241\230\242\351 \247\234\250\240\243\342\244\234\253\234...",
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

static MUI_ENTRY elGRDisplayPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\207\342\242\234\253\234 \244\230 \230\242\242\341\245\234\253\234 \253\246\244 \253\347\247\246 \253\236\252 \234\243\255\341\244\240\251\236\252 \247\246\254 \237\230 \234\232\241\230\253\230\251\253\230\237\234\345.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  \217\230\253\343\251\253\234 \253\246 \247\242\343\241\253\250\246 \217\200\214\227 \343 \211\200\222\227 \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \253\246\244 \234\247\240\237\254\243\236\253\346.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
         "  \253\347\247\246 \234\243\255\341\244\240\251\236\252.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        12,
        "   \213\234\253\341 \247\230\253\343\251\253\234 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\x07  \217\230\253\343\251\253\234 \253\246 \247\242\343\241\253\250\246 ESC \232\240\230 \244\230 \234\247\240\251\253\250\342\257\234\253\234 \251\253\236\244 \247\250\246\236\232\246\347\243\234\244\236 ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "   \251\234\242\345\233\230 \256\340\250\345\252 \244\230 \230\242\242\341\245\234\253\234 \253\246\244 \253\347\247\246 \234\243\255\341\244\240\251\236\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   ESC = \200\241\347\250\340\251\236   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRSuccessPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "\222\230 \231\230\251\240\241\341 \251\253\246\240\256\234\345\230 \253\246\254 ReactOS \234\232\241\230\253\230\251\253\341\237\236\241\230\244 \234\247\240\253\254\256\351\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "\200\255\230\240\250\342\251\253\234 \253\236 \233\240\251\241\342\253\230 \230\247\346 \253\246 A: \241\230\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\346\242\230 \253\230 CD-ROMs \230\247\346 \253\246 CD-Drive.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "\217\230\253\343\251\253\234 ENTER \232\240\230 \244\230 \234\247\230\244\234\241\241\240\244\343\251\234\253\234 \253\246\244 \254\247\246\242\246\232\240\251\253\343 \251\230\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343",
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

static MUI_ENTRY elGRSelectPartitionEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \247\230\250\230\241\230\253\340 \242\345\251\253\230 \234\243\255\230\244\345\235\234\240 \253\230 \254\247\341\250\256\246\244\253\230 \233\240\230\243\234\250\345\251\243\230\253\230 \241\230\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\241\230\240 \253\246\244 \234\242\342\254\237\234\250\246 \256\351\250\246 \232\240\230 \244\342\230 \233\240\230\243\234\250\345\251\243\230\253\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  \217\230\253\343\251\253\234 \217\200\214\227 \343 \211\200\222\227 \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \342\244\230 \251\253\246\240\256\234\345\246 \253\236\252 \242\345\251\253\230\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \217\230\253\343\251\253\234 ENTER \232\240\230 \244\230 \234\232\241\230\253\230\251\253\343\251\234\253\234 \253\246 ReactOS \251\253\246 \234\247\240\242\234\232\243\342\244\246 \233\240\230\243\342\250\240\251\243\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press C to create a primary/logical partition.",
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
        "\x07  \217\230\253\343\251\253\234 D \232\240\230 \244\230 \233\240\230\232\250\341\257\234\253\234 \342\244\230 \254\247\341\250\256\246\244 \233\240\230\243\342\250\240\251\243\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \217\230\250\230\241\230\242\351 \247\234\250\240\243\342\244\234\253\234...",
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

static MUI_ENTRY elGRChangeSystemPartition[] =
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

static MUI_ENTRY elGRConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
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

static MUI_ENTRY elGRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\203\240\230\243\346\250\255\340\251\236 \233\240\230\243\234\250\345\251\243\230\253\246\252",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \253\351\250\230 \237\230 \233\240\230\243\246\250\255\351\251\234\240 \253\246 \233\240\230\243\342\250\240\251\243\230 \217\230\253\343\251\253\234 ENTER \232\240\230 \244\230 \251\254\244\234\256\345\251\234\253\234.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRCheckFSEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \234\242\342\232\256\234\240 \253\351\250\230 \253\246 \234\247\240\242\234\232\243\342\244\246 partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\217\230\250\230\241\230\242\351 \247\234\250\240\243\342\244\234\253\234...",
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

static MUI_ENTRY elGRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\244\253\240\232\250\341\255\234\240 \253\230 \230\250\256\234\345\230 \253\246\254 ReactOS \251\253\246 \234\247\240\242\234\232\243\342\244\246 \233\240\230\243\342\250\240\251\243\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "\204\247\240\242\342\245\253\234 \253\246\244 \251\253\246\244 \255\341\241\234\242\246 \247\246\254 \237\342\242\234\253\234 \244\230 \234\232\241\230\253\230\251\253\230\237\234\345 \253\246 ReactOS:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\202\240\230 \244\230 \230\242\242\341\245\234\253\234 \253\246\244 \247\250\246\253\234\240\244\346\243\234\244\246 \255\341\241\234\242\246 \247\230\253\343\251\253\234 BACKSPACE \232\240\230 \244\230",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "\233\240\230\232\250\341\257\234\253\234 \256\230\250\230\241\253\343\250\234\252 \241\230\240 \243\234\253\341 \247\242\236\241\253\250\246\242\246\232\234\345\251\253\234 \253\246\244 \255\341\241\234\242\246 \251\253\246\244 \246\247\246\345\246",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "\237\342\242\234\253\234 \244\230 \234\232\241\230\251\253\230\237\234\345 \253\246 ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRFileCopyEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "\217\230\250\230\241\230\242\351 \247\234\250\240\243\342\244\234\253\234 \346\251\246 \236 \234\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS \230\244\253\240\232\250\341\255\234\240",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "\253\230 \230\250\256\234\345\230 \251\253\246 \255\341\241\234\242\246 \234\232\241\230\253\341\251\253\230\251\236\252",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "\200\254\253\343 \236 \233\240\230\233\240\241\230\251\345\230 \243\247\246\250\234\345 \244\230 \241\250\230\253\343\251\234\240 \230\250\241\234\253\341 \242\234\247\253\341.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "                                                           \xB3 \217\230\250\230\241\230\242\351 \247\234\250\240\243\342\244\234\253\234...    ",
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

static MUI_ENTRY elGRBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \232\250\341\255\234\240 \253\246\244 bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "\204\232\241\230\253\341\251\253\230\251\236 \253\246\254 bootloader \251\253\246 \251\241\242\236\250\346 \233\345\251\241\246 (MBR and VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\204\232\241\230\253\341\251\253\230\251\236 \253\246\254 bootloader \251\253\246 \251\241\242\236\250\346 \233\345\251\241\246 (VBR only).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\204\232\241\230\253\341\251\253\230\251\236 \253\246\254 bootloader \251\234 \243\240\230 \233\240\251\241\342\253\230.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\214\230 \243\236 \232\345\244\234\240 \234\232\241\230\253\341\251\253\230\251\236 \253\246\254 bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRBootLoaderInstallPageEntries[] =
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
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \232\250\341\255\234\240 \253\246\244 bootloader.",
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

static MUI_ENTRY elGRBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \232\250\341\255\234\240 \253\246\244 bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "\217\230\250\230\241\230\242\351 \234\240\251\341\232\234\253\234 \243\240\230 \233\240\230\243\246\250\255\340\243\342\244\236 \233\240\251\241\342\253\230 \251\253\246 A: \241\230\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\247\230\253\343\251\253\234 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\207\342\242\234\253\234 \244\230 \230\242\242\341\245\234\253\234 \253\246\244 \253\347\247\246 \253\246\254 \247\242\236\241\253\250\246\242\246\232\345\246\254 \247\246\254 \237\230 \234\232\241\230\253\230\251\253\230\237\234\345.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \217\230\253\343\251\253\234 \253\230 \247\242\343\241\253\250\230 \217\200\214\227 \343 \211\200\222\227 \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \253\246\244 \234\247\240\237\254\243\236\253\346",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   \253\347\247\246 \247\242\236\241\253\250\246\242\246\232\345\246\254.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "   \213\234\253\341 \247\230\253\343\251\253\234 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\x07  \217\230\253\343\251\253\234 \253\246 \247\242\343\241\253\250\246 ESC \232\240\230 \244\230 \234\247\240\251\253\250\342\257\234\253\234 \251\253\236\244 \247\250\246\236\232\246\347\243\234\244\236",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "   \251\234\242\345\233\230 \256\340\250\345\252 \244\230 \230\242\242\341\245\234\253\234 \253\246\244 \253\347\247\246 \253\246\254 \247\242\236\241\253\250\246\242\246\232\345\246\254.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   ESC = \200\241\347\250\340\251\236   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\217\230\250\230\241\230\242\351 \234\247\240\242\342\245\253\234 \243\240\230 \233\240\341\253\230\245\236 \232\240\230 \244\230 \234\232\241\230\253\230\251\253\230\237\234\345 \340\252 \247\250\246\234\247\240\242\234\232\243\342\244\236.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \217\230\253\343\251\253\234 \253\230 \247\242\343\241\253\250\230 \217\200\214\227 \343 \211\200\222\227 \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \253\236\244 \234\247\240\237\254\236\243\236\253\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   \233\240\341\253\230\245\236 \247\242\236\241\253\250\246\242\246\232\345\246\254. \213\234\253\341 \247\230\253\343\251\253\234 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  \217\230\253\343\251\253\234 \253\246 \247\242\343\241\253\250\246 ESC \232\240\230 \244\230 \234\247\240\251\253\250\342\257\234\253\234 \251\253\236\244 \247\250\246\236\232\246\347\243\234\244\236",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   \251\234\242\345\233\230 \256\340\250\345\252 \244\230 \230\242\242\341\245\234\253\234 \253\236\244 \233\240\341\253\230\245\236 \253\246\254 \247\242\236\241\253\250\246\242\246\232\345\246\254.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   ESC = \200\241\347\250\340\251\236   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRPrepareCopyEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \247\250\246\234\253\246\240\243\341\235\234\240 \253\246\244 \254\247\246\242\246\232\240\251\253\343 \251\230\252 \232\240\230 \253\236\244 \230\244\253\240\232\250\230\255\343 \253\340\244 \230\250\256\234\345\340\244 \253\246\254 ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \225\253\345\235\234\253\230\240 \236 \242\345\251\253\230 \253\340\244 \230\250\256\234\345\340\244 \247\250\246\252 \230\244\253\240\232\250\230\255\343...",
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

static MUI_ENTRY elGRSelectFSEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "\204\247\240\242\342\245\253\234 \342\244\230 \251\347\251\253\236\243\230 \230\250\256\234\345\340\244 \230\247\346 \253\236\244 \247\230\250\230\241\341\253\340 \242\345\251\253\230.",
        0
    },
    {
        8,
        19,
        "\x07  \217\230\253\343\251\253\234 \253\230 \247\242\343\241\253\250\230 \217\200\214\227 \343 \211\200\222\227 \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \253\246 \251\347\251\253\236\243\230 \230\250\256\234\345\340\244.",
        0
    },
    {
        8,
        21,
        "\x07  \217\230\253\343\251\253\234 ENTER \232\240\230 \244\230 \233\240\230\243\246\250\255\351\251\234\253\234 \253\246 parition.",
        0
    },
    {
        8,
        23,
        "\x07  \217\230\253\343\251\253\234 ESC \232\240\230 \244\230 \234\247\240\242\342\245\234\253\234 \341\242\242\246 partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = \221\254\244\342\256\234\240\230   ESC = \200\241\347\250\340\251\236   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRDeletePartitionEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\204\247\240\242\342\245\230\253\234 \244\230 \233\240\230\232\250\341\257\234\253\234 \230\254\253\346 \253\246 partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  \217\230\253\343\251\253\234 L \232\240\230 \244\230 \233\240\230\232\250\341\257\234\253\234 \253\246 partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "\217\220\216\204\210\203\216\217\216\210\206\221\206: \356\242\230 \253\230 \233\234\233\246\243\342\244\230 \251\234 \230\254\253\346 \253\246 partition \237\230 \256\230\237\246\347\244!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  \217\230\253\343\251\253\234 ESC \232\240\230 \230\241\347\250\340\251\236.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   L = \203\240\230\232\250\230\255\343 Partition   ESC = \200\241\347\250\340\251\236   F3 = \200\247\246\256\351\250\236\251\236",
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

static MUI_ENTRY elGRRegistryEntries[] =
{
    {
        4,
        3,
        " \204\232\241\230\253\341\251\253\230\251\236 \253\246\254 ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\244\230\244\234\351\244\234\240 \253\236 \233\246\243\343 \253\246\254 \251\254\251\253\343\243\230\253\246\252.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \203\236\243\240\246\254\250\232\246\347\244\253\230\240 \253\230 registry hives...",
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

MUI_ERROR elGRErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "\222\246 ReactOS \233\234\244 \234\232\241\230\253\230\251\253\341\237\236\241\234 \247\242\343\250\340\252 \251\253\246\244\n"
        "\254\247\246\242\246\232\240\251\253\343 \251\230\252. \200\244 \230\247\246\256\340\250\343\251\234\253\234 \230\247\346 \253\236\244 \204\232\241\230\253\341\251\253\230\251\236 \253\351\250\230, \237\230 \247\250\342\247\234\240 \244\230\n"
        "\245\230\244\230\253\250\342\245\234\253\234 \253\236\244 \204\232\241\230\253\341\251\253\230\251\236 \232\240\230 \244\230 \234\232\241\230\253\230\251\253\343\251\234\253 \253\246 ReactOS.\n"
        "\n"
        "  \x07  \217\230\253\343\251\253\234 ENTER \232\240\230 \230\244 \251\254\244\234\256\345\251\234\253\234 \253\236\244 \204\232\241\230\253\341\251\253\230\251\236.\n"
        "  \x07  \217\230\253\343\251\253\234 F3 \232\240\230 \244\230 \230\247\246\256\340\250\343\251\234\253\234 \230\247\346 \253\236\244 \204\232\241\230\253\341\251\253\230\251\236.",
        "F3 = \200\247\246\256\351\250\236\251\236  ENTER = \221\254\244\342\256\234\240\230"
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
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \231\250\234\240 \241\341\247\246\240\246\244 \251\241\242\236\250\346 \233\345\251\241\246.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \255\246\250\253\351\251\234\240 \253\246 \230\250\256\234\345\246 TXTSETUP.SIF.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "\206 \234\232\241\230\253\341\251\253\251\236 \231\250\343\241\234 \342\244\230 \241\230\253\234\251\253\250\230\243\342\244\246 \230\250\256\234\345\246 TXTSETUP.SIF.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "\206 \204\232\241\230\253\341\251\253\230\251\236 \231\250\343\241\234 \243\240\230 \243\236 \342\232\241\254\250\236 \254\247\246\232\250\230\255\343 \251\253\246 TXTSETUP.SIF.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \255\246\250\253\351\251\234\240 \253\240\252 \247\242\236\250\246\255\246\250\345\234\252 \253\246\254 \233\345\251\241\246\254 \251\254\251\253\343\243\230\253\246\252.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_WRITE_BOOT,
        "Setup failed to install %S bootcode on the system partition.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \255\246\250\253\351\251\234\240 \253\236 \242\345\251\253\230 \253\347\247\340\244 \254\247\246\242\246\232\240\251\253\343.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \255\246\250\253\351\251\234\240 \253\236 \242\345\251\253\230 \253\347\247\340\244 \234\243\255\341\244\240\251\236\252.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \255\246\250\253\351\251\234\240 \253\236 \242\345\251\253\230 \253\347\247\340\244 \247\242\236\241\253\250\246\242\246\232\345\246\254.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \255\246\250\253\351\251\234\240 \253\236 \242\345\251\253\230 \233\240\230\253\341\245\234\340\244 \247\242\236\241\253\250\246\242\246\232\345\246\254.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_WARN_PARTITION,
          "\206 \234\232\241\230\253\341\251\253\230\251\236 \231\250\343\241\234 \346\253\240 \253\246\254\242\341\256\240\251\253\246\244 \342\244\230\252 \251\241\242\236\250\346\252 \233\345\251\241\246\252 \247\234\250\240\342\256\234\240 \342\244\230 \243\236 \251\254\243\231\230\253\346\n"
          "partition table \247\246\254 \233\234 \243\247\246\250\234\345 \244\230 \234\242\234\232\256\237\234\345 \251\340\251\253\341!\n"
          "\n"
          "\206 \233\236\243\240\246\254\250\232\345\230 \343 \233\240\230\232\250\230\255\343 partitions \243\247\246\250\234\345 \244\230 \241\230\253\230\251\253\250\342\257\234\240 \253\246 partition table.\n"
          "\n"
          "  \x07  \217\230\253\343\251\253\234 F3 \232\240\230 \244\230 \230\247\246\256\340\250\343\251\234\253\234 \230\247\346 \253\236\244 \204\232\241\230\253\341\251\253\230\251\236.\n"
          "  \x07  \217\230\253\343\251\253\234 ENTER \232\240\230 \244\230 \251\254\244\234\256\345\251\234\253\234.",
          "F3 = \200\247\246\256\351\250\236\251\236  ENTER = \221\254\244\342\256\234\240\230"
    },
    {
        // ERROR_NEW_PARTITION,
        "\203\234 \243\247\246\250\234\345\253\234 \244\230 \233\236\243\240\246\254\250\232\343\251\234\253\234 \342\244\230 Partition \243\342\251\230 \251\234\n"
        "\342\244\230 \341\242\242\246 \254\247\341\250\256\246\244 Partition!\n"
        "\n"
        "  * \217\230\253\343\251\253\234 \246\247\246\240\246\233\343\247\246\253\234 \247\242\343\241\253\250\246 \232\240\230 \244\230 \251\254\244\234\256\345\251\234\253\234.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the %S bootcode on the system partition.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_NO_FLOPPY,
        "\203\234\244 \254\247\341\250\256\234\240 \233\240\251\241\342\253\230 \251\253\246 A:.",
        "ENTER = \221\254\244\342\256\234\240\230"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "\206 \204\232\241\230\253\341\251\230\251\236 \230\247\342\253\254\256\234 \244\230 \230\244\230\244\234\351\251\234\240 \253\240\252 \250\254\237\243\345\251\234\240\252 \232\240\230 \253\236 \233\240\341\253\230\245\236 \247\242\236\241\253\250\246\242\246\232\345\246\254.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \230\244\230\244\234\351\251\234\240 \253\240\252 \250\254\237\243\345\251\234\240\252 \243\236\253\250\351\246\254 \232\240\230 \253\236\244 \234\243\255\341\244\240\251\236.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_IMPORT_HIVE,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \255\246\250\253\351\251\234\240 \342\244\230 hive \230\250\256\234\345\246.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_FIND_REGISTRY
        "\206 \234\232\241\230\253\341\251\230\251\236 \230\247\342\253\254\256\234 \244\230 \231\250\234\240 \253\230 \230\250\256\234\345\230 \233\234\233\246\243\342\244\340\244 \253\246\254 \243\236\253\250\351\246\254.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_CREATE_HIVE,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \233\236\243\240\246\254\250\232\343\251\234\240 \253\230 registry hives.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \230\250\256\240\241\246\247\246\240\343\251\234\240 \253\246 \243\236\253\250\351\246.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "\222\246 cabinet \233\234\244 \342\256\234\240 \342\232\241\254\250\246 \230\250\256\234\345\246 inf.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_CABINET_MISSING,
        "\222\246 cabinet \233\234 \231\250\342\237\236\241\234.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "\222\246 cabinet \233\234\244 \342\256\234\240 \241\230\244\342\244\230 \251\241\250\240\247\253 \234\232\241\230\253\341\251\253\230\251\236\252.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_COPY_QUEUE,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \230\244\246\345\245\234\240 \253\236\244 \246\254\250\341 \230\250\256\234\345\340\244 \247\250\246\252 \230\244\253\240\232\250\230\255\343.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_CREATE_DIR,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \233\236\243\240\246\254\250\232\343\251\234\240 \253\246\254\252 \241\230\253\230\242\346\232\246\254\252 \234\232\241\230\253\341\251\253\230\251\236\252.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \231\250\234\240 \253\246\244 \253\246\243\342\230 '%S'\n"
        "\251\253\246 TXTSETUP.SIF.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_CABINET_SECTION,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \231\250\234\240 \253\246\244 \253\246\243\342\230 '%S'\n"
        "\251\253\246 cabinet.\n",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\234 \243\247\346\250\234\251\234 \244\230 \233\236\243\240\246\254\250\232\343\251\234\240 \253\246\244 \241\230\253\341\242\246\232\246 \234\232\241\230\253\341\251\253\230\251\236\252.",
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_WRITE_PTABLE,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \232\250\341\257\234\240 \253\230 partition tables.\n"
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to the registry.\n"
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\247\342\253\254\256\234 \244\230 \247\250\246\251\237\342\251\234\240 \253\240\252 \233\240\230\253\341\245\234\240\252 \247\242\236\241\253\250\246\242\246\232\345\340\244 \251\253\246 \243\236\253\250\351\246.\n"
        "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Setup could not set the geo id.\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
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

MUI_PAGE elGRPages[] =
{
    {
        SETUP_INIT_PAGE,
        elGRSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        elGRLanguagePageEntries
    },
    {
       WELCOME_PAGE,
       elGRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        elGRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        elGRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        elGRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        elGRRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        elGRUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        elGRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        elGRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        elGRFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        elGRSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        elGRChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        elGRConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        elGRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        elGRFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        elGRCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        elGRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        elGRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        elGRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        elGRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        elGRKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        elGRBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        elGRLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        elGRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        elGRSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        elGRBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        elGRBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        elGRRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING elGRStrings[] =
{
    {STRING_PLEASEWAIT,
     "   \217\230\250\230\241\230\242\351 \247\234\250\240\243\342\244\234\253\234..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   C = Create Primary   E = Create Extended   F3 = Quit"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   C = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = \204\232\241\230\253\341\251\253\230\251\236   D = \203\240\230\232\250\230\255\343 Partition   F3 = \200\247\246\256\351\250\236\251\236"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "\213\342\232\234\237\246\252 \253\246\254 \244\342\246\254 partition:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "You have chosen to create a primary partition on"},
//     "„§ ¢â¥˜«œ ¤˜ ›ž£ ¦¬¨šã©œ«œ â¤˜ ¤â¦ partition on"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDPARTSIZE,
    "\217\230\250\230\241\230\242\351 \233\351\251\253\234 \253\246 \243\342\232\234\237\246\252 \253\246\254 partition \251\234 megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = \203\236\243\240\246\254\250\232\345\230 Partition   ESC = \200\241\347\250\340\251\236   F3 = \200\247\246\256\351\250\236\251\236"},
    {STRING_NEWPARTITION,
    "\206 \234\232\241\230\253\341\251\253\230\251\236 \233\236\243\240\246\347\250\232\236\251\234 \342\244\230 \244\342\246 partition \251\253\246"},
    {STRING_PARTFORMAT,
    "\200\254\253\346 \253\246 Partition \237\230 \233\240\230\243\246\250\255\340\237\234\345 \243\234\253\341."},
    {STRING_NONFORMATTEDPART,
    "\204\247\240\242\342\245\230\253\234 \244\230 \234\232\241\230\253\230\251\253\343\251\234\253\234 \253\246 ReactOS \251\234 \342\244\230 \244\342\246 \343 \243\236 \233\240\230\243\246\250\255\340\243\342\244\246 Partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Setup install ReactOS onto Partition"},
    {STRING_CONTINUE,
    "ENTER = \221\254\244\342\256\234\240\230"},
    {STRING_QUITCONTINUE,
    "F3 = \200\247\246\256\351\250\236\251\236  ENTER = \221\254\244\342\256\234\240\230"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = \204\247\230\244\234\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   \200\244\253\240\232\250\341\255\234\253\230\240 \253\246 \230\250\256\234\345\246: %S"},
    {STRING_SETUPCOPYINGFILES,
     "\206 \234\232\241\230\253\341\251\253\230\251\236 \230\244\253\240\232\250\341\255\234\240 \230\250\256\234\345\230..."},
    {STRING_REGHIVEUPDATE,
    "   \202\345\244\234\253\230\240 \230\244\230\244\342\340\251\236 \253\340\244 registry hives..."},
    {STRING_IMPORTFILE,
    "   \202\345\244\234\253\230\240 \234\240\251\230\232\340\232\343 \253\246\254 %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   \202\345\244\234\253\230\240 \230\244\230\244\342\340\251\236 \253\340\244 \250\254\237\243\345\251\234\340\244 \234\243\255\341\244\240\251\236\252 \253\246\254 \243\236\253\250\351\246\254..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   \202\345\244\234\253\230\240 \230\244\230\244\342\340\251\236 \253\340\244 \250\254\237\243\345\251\234\340\244 \232\242\351\251\251\230\252..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   \202\345\244\234\253\230\240 \230\244\230\244\342\340\251\236 \253\340\244 \250\254\237\243\345\251\234\340\244 \233\240\341\253\230\245\236\252 \247\242\236\241\253\250\246\242\246\232\345\246\254..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adding codepage information..."},
    {STRING_DONE,
    "   \216\242\246\241\242\236\250\351\237\236\241\234..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = \204\247\234\244\240\241\241\345\244\236\251\236 \254\247\246\242\246\232\240\251\253\343"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "\200\233\347\244\230\253\246 \244\230 \230\244\246\240\256\253\234\345 \236 \241\246\244\251\346\242\230\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "The most common cause of this is using an USB keyboard\r\n"},
    {STRING_CONSOLEFAIL3,
    "\222\230 USB \247\242\236\241\253\250\246\242\346\232\240\230 \233\234\244 \234\345\244\230\240 \247\242\343\250\340\252 \254\247\246\251\253\236\250\240\235\346\243\234\244\230 \230\241\346\243\230\r\n"},
    {STRING_FORMATTINGPART,
    "Setup is formatting the partition..."},
    {STRING_CHECKINGDISK,
    "Setup is checking the disk..."},
    {STRING_FORMATDISK1,
    " \203\240\230\243\346\250\255\340\251\236 \253\246\254 partition \340\252 %S \251\347\251\253\236\243\230 \230\250\256\234\345\340\244 (\232\250\343\232\246\250\236 \233\240\230\243\346\250\255\340\251\236) "},
    {STRING_FORMATDISK2,
    " \203\240\230\243\346\250\255\340\251\236 \253\246\254 partition \340\252 %S \251\347\251\253\236\243\230 \230\250\256\234\345\340\244 "},
    {STRING_KEEPFORMAT,
    " \214\230 \247\230\250\230\243\234\345\244\234\240 \253\246 \251\347\251\253\236\243\230 \230\250\256\234\345\340\244 \340\252 \342\256\234\240 (\241\230\243\345\230 \230\242\242\230\232\343) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "\251\253\246: %s."},
    {STRING_PARTTYPE,
    "Type 0x%02x"},
    {STRING_HDDINFO1,
    // "\221\241\242\236\250\346\252 \233\345\251\241\246\252 %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s \221\241\242\236\250\346\252 \233\345\251\241\246\252 %lu (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDDINFO2,
    // "\221\241\242\236\250\346\252 \233\345\251\241\246\252 %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s \221\241\242\236\250\346\252 \233\345\251\241\246\252 %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Unpartitioned space"},
    {STRING_MAXSIZE,
    "MB (\243\234\232. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "\214\342\246 (\213\236 \233\240\230\243\246\250\255\340\243\342\244\246)"},
    {STRING_FORMATUNUSED,
    "\200\256\250\236\251\240\243\246\247\246\345\236\253\246"},
    {STRING_FORMATUNKNOWN,
    "\352\232\244\340\251\253\246"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "\202\345\244\234\253\230\240 \247\250\246\251\237\343\241\236 \253\340\244 \233\240\230\253\341\245\234\340\244 \247\242\236\241\253\250\246\242\246\232\345\246\254"},
    {0, 0}
};
