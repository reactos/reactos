#pragma once
/* Hebrew text is in visual order */

static MUI_ENTRY heILLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "„”™ š˜‰‡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  .„—š„„ Š‰Œ„š ˜…’ „”™„ š€ ˜‡ €€",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   .ENTER ™—„ †€…",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  .š‰”…‘„ š‹˜’Ž Œƒ‡Ž„ š˜‰˜ š”™ „‰„š š€† „”™",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS š—š„Œ ‰€„ ‰‹…˜",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ReactOS „Œ’”„„ š‹˜’Ž š€ —‰š’‰ „—š„„ Œ™ „†„ Œ™„",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        ".„—š„„ Œ™ ‰™„ Œ™„ š€ ‰‹š… Š™‡ŽŒ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  .ReactOS š€ ‚˜ƒ™Œ …€ ‰—š„Œ ‰ƒ‹ ENTER •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
     // "\x07  Press R to repair a ReactOS installation using the Recovery Console.",
        "\x07  .ReactOS š—š„ —šŒ ‰ƒ‹ R •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  .ReactOS Œ™ ‰…™‰˜„ ‰€š š…€˜Œ ‰ƒ‹ L •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  .ReactOS š€ ‰—š„Œ ‰ŒŽ š€–Œ ‰ƒ‹ F3 •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        ":˜š€ ˜—Œ š‰ ,ReactOS ‰‚Œ ’ƒ‰Ž ˜š…‰Œ",
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
        "„—š„ Œ…ˆ‰ = F3  …‰™˜ = L  „—š„ …—‰š = R  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS š‘˜‚ –Ž",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "š…‡‹… š……‹š„ Œ‹ €Œ™ š…’Ž™Ž ,€”Œ€ Œ™ š€–Ž ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "˜…’ —˜ š‹˜’Ž ™Žš™„Œ •ŒŽ…Ž .‡…š‰” š‡š…",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        ".…‰ …‰ ™…Ž‰™ ˜…’ š‹˜’Ž Š…š €Œ… š…‘š„… „—‰ƒ š…˜ˆŽŒ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "‰™Ž ™‡Ž ™Žš™„Œ …€ ŠŒ™ ’ƒ‰Ž„ š€ š…‚Œ •ŒŽ…Ž",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        ".š‰†‰” „˜Ž…‡ Œ’ ReactOS š€ •‰˜„Œ šŽ Œ’",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  .ReactOS š—š„ š€ Š‰™Ž„Œ ‰ƒ‹ ENTER ™—„",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  .ReactOS š€ ‰—š„Œ ‰ŒŽ š€–Œ ‰ƒ‹ F3 •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        ":‰…™‰˜",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "the original license.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "If for some reason you did not receive a copy of the",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
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
        ":š…‰˜‡€",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ƒ…— “ƒ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".‰‰‡‹…„ ‰—š„„ š…˜ƒ‚„ š€ „€˜Ž „ˆŽŒ „Ž‰™˜„",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        ":™‡Ž",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        ":„‚…–š",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        ":šƒŒ—Ž",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        ":šƒŒ—Ž š˜…–š",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        ":˜…™‰€",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "‰—š„„ š…˜ƒ‚„ ˜…™‰€",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        """„ˆŽŒ""… ""„Œ’ŽŒ""„ ‰™—Ž š–‰‡Œ ‰—š„„ š…˜ƒ‚„ š€ š…™Œ š‰",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        ".„…™ „˜ƒ‚„ ˜…‡Œ ‰ƒ‹ ENTER •‡Œ †€ .—š„ ˜…‡Œ ‰ƒ‹",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        """‰—š„„ š…˜ƒ‚„ ˜…™‰€"" ˜‡ ,š……‹ š…˜ƒ‚„„ Œ‹™ ‰šŽ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        ".ENTER •‡Œ…",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".ƒ—…Ž ‡…š‰” –Ž š€–Ž ReactOS š—š„",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        ".š‰—š „—š„ š‰‹š Œ™ š……‹š„ Œ‹ š€ š˜™”€Ž „‰€…",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        ".š…Ž™…‰Ž ‰‰ƒ’ ‰€ …—‰š„ š……‹š",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  .š‹˜’Ž„ š€ ‹ƒ’Œ ‰ƒ‹ U •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  .˜…†‡™„ “…‘Ž š€ ‡…š”Œ ‰ƒ‹ R •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  .‰™€˜„ ƒ…Ž’Œ ˜…†‡Œ ‰ƒ‹ ESC •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  .™‡Ž„ š€ ™ƒ‡Ž Œ‰’”„Œ ‰ƒ‹ ENTER •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "™ƒ‡Ž „Œ’”„ = ENTER  ˜…†‡™ = R  …‹ƒ’ = U  ‰™€˜ ƒ…Ž’ = ESC",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS š…—š„Ž š‡€ š€ ‚˜ƒ™Œ „Œ…‹‰ ReactOS š—š„",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "„—š„„ š‰‹š ,„Ž…‚” š…—š„„Ž š‡€ € …€ ,„ˆŽŒ š…Ž…™˜„",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        ".„—šŒ š…‘Œ „Œ…‹‰",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        ".„€…ŒŽ šŽ™…‰Ž „‰€ …—‰š„ š‹˜’Ž",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        15,
        "\x07  .„—š„ ˜…‡Œ ‰ƒ‹ ""„ˆŽŒ"" …€ ""„Œ’ŽŒ"" Œ’ •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  .š˜‡„ š‹˜’Ž„ š—š„ š€ ‚˜ƒ™Œ ‰ƒ‹ U •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  .„™ƒ‡ š‹˜’Ž š—š„ ’ Š‰™Ž„Œ ‰ƒ‹ ESC •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  .ReactOS š€ ‰—š„Œ ‰ŒŽ š€–Œ ‰ƒ‹ F3 •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  ‚˜ƒ™Œ €Œ = ESC  ‚…˜ƒ™ = ESC",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".—š…Ž„ ™‡Ž„ ‚…‘ š€ š…™Œ „–…˜ „š€",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  .‰…–˜„ ™‡Ž„ ‚…‘ š€ ˜…‡Œ ‰ƒ‹ ""„ˆŽŒ"" …€ ""„Œ’ŽŒ"" Œ’ •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   .ENTER •‡Œ †€…",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  .™‡Ž„ ‚…‘ š€ š…™Œ ‰ŒŽ ƒ…—„ ƒ…Ž’Œ ˜…†‡Œ ‰ƒ‹ ESC Œ’ •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Œ…ˆ‰ = ESC  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ŠŒ™ …‹„ Œ’ ˜…Ž™ ’ƒ‰Ž„ Œ‹™ €ƒ……š …‰™‹’ š‹˜’Ž„",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "š…—ƒ „Ž‹ š‡—Œ Œ…‹‰ „†",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "‰ˆŽ…ˆ…€ ”…€ ™ƒ‡Ž Œ’”…‰ ™‡Ž„ ,…‰‘",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "…ŽˆŽ„ š€ “ˆ…™",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "€ŒŽ ”…€ š—š…Ž €Œ ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        " :A …‹Ž …ˆ‰Œ—š„ š€ €‰–…„Œ €",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        ".‰…‹„Ž ‰˜…ˆ‰Œ—š„ Œ‹ š€…",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        ".™‡Ž„ š€ ™ƒ‡Ž Œ‰’”„Œ ‰ƒ‹ ENTER •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "...‰šŽ„Œ €",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".—š…Ž„ „‚…–š„ ‚…‘ š€ š…™Œ Š…–˜",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  .‰…–˜„ „‚…–š„ ‚…‘ š€ ˜…‡Œ ‰ƒ‹ ""„ˆŽŒ""Œ ""„Œ’ŽŒ"" Œ’ •‡Œ",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   .ENTER •‡Œ †€…",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  .„‚…–š„ ‚…‘ š€ š…™Œ ‰ŒŽ ˜…†‡Œ ‰ƒ‹ ESC •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Œ…ˆ‰ = ESC  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        ".„‡Œ–„ …—š…„ ReactOS Œ™ ‰‰‘‰‘„ ‰‰‹˜„",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        " :A …‹Ž …ˆ‰Œ—š„ š€ €‰–…„Œ €",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        ".‰…‹„Ž ‰˜…ˆ‰Œ—š„ Œ‹ š€…",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        ".™‡Ž„ š€ ™ƒ‡Ž Œ‰’”„Œ ‰ƒ‹ ENTER •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "™ƒ‡Ž „Œ’”„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "‡‰™—„ …‹„ Œ’ Œ…‡š€„ Œ„Ž š€ ‰—š„Œ „Œ…‹‰ „‰€ „—š„„",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Š™‡Ž",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        ".ENTER •‡Œ… :A …‹Œ Œ‡š…€Ž …ˆ‰Œ—š ‘‹„ „™—",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Œ–…Ž €Œ —‘‰ƒ ‡ˆ™… š…–‰‡Ž ‰‚–…Ž „ˆŽŒ „Ž‰™˜",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        ".š…™ƒ‡ š…–‰‡Ž ˜…’",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  .„Ž…™˜ ˜…‡Œ ‰ƒ‹ ""„ˆŽŒ"" …€ ""„Œ’ŽŒ"" Œ’ •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  .š˜‡„ „–‰‡Ž„ Œ’ ReactOS š€ ‰—š„Œ ‰ƒ‹ ENTER •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  .š‰™€˜ „–‰‡Ž ˜…–‰Œ ‰ƒ‹ P •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  .š‡˜…Ž „–‰‡Ž ˜…–‰Œ ‰ƒ‹ E •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  .š‰‚…Œ „–‰‡Ž ˜…–‰Œ ‰ƒ‹ L •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  .šŽ‰‰— „–‰‡Ž —…‡ŽŒ ‰ƒ‹ D •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "...‰šŽ„Œ €",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".š‹˜’Ž š–‰‡Ž —…‡ŽŒ š˜‡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        ",„˜Ž…‡ š˜ƒ‚„ š…‰‹…š ,…‡€ š…‰‹…š Œ‰‹„Œ š…Œ…‹‰ š‹˜’Ž š…–‰‡Ž",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        ")ReactOS …Ž‹( „Œ’”„ š…‹˜’Ž šŒ’”„Œ š…‰‹š",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        ".„˜Ž…‡„ ˜–‰ ‰ƒ‰ Œ’ š…—”…‘Ž™ š…˜‡€ š…‰‹š …€",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "…Œ€‹ š…‰‹š ‰€™ ‡…ˆ „š€ € —˜ š‹˜’Ž š–‰‡Ž —‡Ž",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        ".š…€ —…‡ŽŒ ‡…ˆ „š€™ …€ ,„–‰‡Ž„ Œ’",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "‡‰™—„ …‹„Ž š…Œ’Œ €Œ Œ…Œ’ ™‡Ž„ ,„–‰‡Ž„ —‡Žš™ ‰šŽ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        ".ReactOS š—š„ …‰‘Œ ƒ’",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  „—‰‡ŽŒ ‰…ƒ‰… ™—š‰ .š‹˜’Ž„ š–‰‡Ž š€ —…‡ŽŒ ‰ƒ‹ ENTER •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   .„—š„ ˜š…‰ ˜‡…€Ž ƒ’…Ž",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  .—‡Ž‰š €Œ „–‰‡Ž„ ,ƒ…—„ ƒ…Ž’Œ ˜…†‡Œ ‰ƒ‹ ESC •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Œ…ˆ‰ = ESC  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "„–‰‡Ž Œ…‡š€",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        ".Š‰™Ž„Œ ‰ƒ‹ ENTER •‡Œ .„–‰‡Ž„ š€ Œ‡š€š …‰™‹’ „—š„„",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY heILInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".š˜‡„ „–‰‡Ž„ Œ’ ReactOS š€ ‰—šš „—š„„",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        ":ReactOS š€ ‰—š„Œ Š…–˜ „ „‰‰—‰š ˜‡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "—…‡ŽŒ ‰ƒ‹ BACKSPACE •‡Œ ,š’–…Ž„ „‰‰—‰š„ š€ š…™Œ ‰ƒ‹",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        ".ReactOS š€ ‰—š„Œ Š…–˜ „ „‰‰—‰š„ š€ …™˜š †€… ‰……š",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "‰–— „—‰š’Ž ReactOS š—š„™ Ž† ‰šŽ„Œ €",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        ".š˜‡™ „—š„„ š‰‰—‰šŒ",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        ".š…—ƒ ˜”‘Ž Š™Ž„Œ „Œ…‹‰ …† „Œ…’”",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 ...‰šŽ„Œ €    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY heILBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Œ…‡š€„ Œ„Ž š€ š’‹ „‰—šŽ „—š„„ š‰‹š",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        ".(VBR-… MBR) ‡‰™—„ —‘‰ƒ Œ’ Œ…‡š€ Œ„Ž —š„",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        ".(ƒŒ VBR) ‡‰™—„ —‘‰ƒ Œ’ Œ…‡š€ Œ„Ž —š„",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        ".…ˆ‰Œ—š Œ’ Œ…‡š€ Œ„Ž —š„",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        ".Œ…‡š€„ Œ„Ž š—š„ Œ’ ‚…Œ‰ƒ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".š—š…Ž„ šƒŒ—Ž„ ‚…‘ š…™Œ Š…–˜",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  .‰…–˜„ šƒŒ—Ž„ ‚…‘ š€ ˜…‡Œ ‰ƒ‹ ""„ˆŽŒ"" …€ ""„Œ’ŽŒ"" Œ’ •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   .ENTER •‡Œ †€…",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  .šƒŒ—Ž„ ‚…‘ š€ š…™Œ ‰ŒŽ ƒ…—„ ƒ…Ž’Œ ˜…†‡Œ ‰ƒ‹ ESC Œ’ •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Œ…ˆ‰ = ESC  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".Œƒ‡Ž š˜‰˜ š—š…Ž„ šƒŒ—Ž„ š˜…–š š€ ˜…‡Œ €",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  .„‰…–˜„ „˜…–š„ š€ ˜…‡Œ ‰ƒ‹ ""„ˆŽŒ"" …€ ""„Œ’ŽŒ"" Œ’ •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    .ENTER •‡Œ †€…",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  .šƒŒ—Ž„ š˜…–š š€ š…™Œ ‰ŒŽ ƒ…—„ ƒ…Ž’Œ ˜…†‡Œ ‰ƒ‹ ESC •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Œ…ˆ‰ = ESC  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        ".ReactOS ‰–— š—š’„Œ Š™‡Ž š€ „‰‹Ž „—š„„ š‰‹š ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "...‰–— š—š’„ šŽ‰™˜ „…",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        ".„ˆŽŒ „Ž‰™˜„Ž ‰–— š‹˜’Ž ˜‡",
        0
    },
    {
        8,
        19,
        "\x07  .‰–— š‹˜’Ž ˜…‡Œ ‰ƒ‹ ""„ˆŽŒ"" …€ ""„Œ’ŽŒ"" Œ’ •‡Œ",
        0
    },
    {
        8,
        21,
        "\x07  .„–‰‡Ž„ š€ Œ‡š€Œ ‰ƒ‹ ENTER •‡Œ",
        0
    },
    {
        8,
        23,
        "\x07  .š˜‡€ „–‰‡Ž ˜…‡Œ ‰ƒ‹ ESC •‡Œ",
        0
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Œ…ˆ‰ = ESC  Š™Ž„ = ENTER",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "„–‰‡Ž„ š€ —…‡ŽŒ š˜‡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  .„–‰‡Ž„ š€ —…‡ŽŒ ‰ƒ‹ D •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "!ƒ€‰ „–‰‡Ž ’ƒ‰Ž„ Œ‹ :„˜„†€",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  .ŒˆŒ ‰ƒ‹ ESC •‡Œ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "„—š„ Œ…ˆ‰ = F3  Œ…ˆ‰ = ESC  „–‰‡Ž —‡Ž = D",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ReactOS " KERNEL_VERSION_STR " š—š„ ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        " .š‹˜’Ž„ š˜…–š š€ š‹ƒ’Ž „—š„„ š‰‹š",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "...šŽ…™‰˜ š…˜……‹ ˜–…‰",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        "„‡Œ–„\n"
    },
    {
        // ERROR_NOT_INSTALLED
        ".Š™‡Ž Œ’ €ŒŽ ”…€ š—š…Ž €Œ ReactOS\n"
        "…™ „—š„„ š€ Œ‰’”„Œ Š˜ˆ–š ,š’‹ „—š„„Ž €–š €\n"
        ".ReactOS š€ ‰—š„Œ ‰ƒ‹\n"
        "\n"
        "  \x07  .„—š„„ Š‰™Ž„Œ ‰ƒ‹ ENTER •‡Œ\n"
        "  \x07  .„—š„„Ž š€–Œ ‰ƒ‹ F3 •‡Œ",
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER"
    },
    {
        // ERROR_NO_HDD
        ".‡‰™— …‹ „€–Ž €Œ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        ".„Œ™ ˜…—Ž„ …‹ š€ „€–Ž €Œ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        ".TXTSETUP.SIF •…— š€ „€–Ž €Œ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        ".…‚” TXTSETUP.SIF •…—„ š€ „€–Ž „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        ".TXTSETUP.SIF- „‰…‚™ šŽš…‡ „€–Ž „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_DRIVE_INFORMATION
        ".š‹˜’Ž„ Œ™ ‰…‹„ ’ƒ‰Ž š€ ‚‰™„Œ „Œ‹‰ €Œ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_WRITE_BOOT,
        ".š‹˜’Ž„ š–‰‡Ž Œ’ %S Œ™ Œ…‡š€„ ƒ…— š€ ‰—š„Œ „Œ™‹ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_LOAD_COMPUTER,
        ".‰™‡Ž„ ‰‚…‘ šŽ‰™˜ š‰’ˆ „Œ™‹ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_LOAD_DISPLAY,
        ".„‚…–š„ š…˜ƒ‚„ ‰‚…‘ šŽ‰™˜ š‰’ˆ „Œ™‹ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        ".š…ƒŒ—Ž„ ‰‚…‘ šŽ‰™˜ š‰’ˆ „Œ™‹ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        ".šƒŒ—Ž„ š…˜…–š šŽ‰™˜ š‰’ˆ „Œ™‹ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_WARN_PARTITION,
        "ƒ‡€ ‡‰™— —‘‰ƒ š…‡”Œ „€–Ž „—š„„ š‰‹š\n"
        "!… š…–‰‡Ž„ šŒˆ Š‰˜–™ …Ž‹ ™Žš™„Œ š‰ €Œ™\n"
        "\n"
        ".š…–‰‡Ž„ šŒˆ š€ ƒ‰Ž™„Œ „Œ…Œ’ „–‰‡Ž š—‰‡Ž …€ š˜‰–‰\n"
        "\n"
        "  \x07  .„—š„„ Œ…ˆ‰Œ F3 •‡Œ\n"
        "  \x07  .Š‰™Ž„Œ ‰ƒ‹ ENTER •‡Œ",
        "„—š„ Œ…ˆ‰ = F3  Š™Ž„ = ENTER"
    },
    {
        // ERROR_NEW_PARTITION,
        "!šŽ‰‰— „–‰‡Ž Š…š „–‰‡Ž ˜…–‰Œ š‰ €Œ\n"
        "\n"
        ".Š‰™Ž„Œ ‰ƒ‹ ™—Ž Œ’ •‡Œ *  ",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "!„–—…Ž €Œ™ —‘‰ƒ ‡ˆ™ —…‡ŽŒ š‰ €Œ\n"
        "\n"
        ".Š‰™Ž„Œ ‰ƒ‹ ™—Ž Œ’ •‡Œ *  ",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        ".š‹˜’Ž„ š–‰‡Ž Œ’ %S ˜…’ Œ…‡š€ ƒ…— š—š„ „™Œ‹ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_NO_FLOPPY,
        ".:A …‹ …ˆ‰Œ—š ‰€",
        "Š™Ž„ = ENTER"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        ".šƒŒ—Ž„ š˜…–š š…˜ƒ‚„ š€ ‹ƒ’Œ „Œ™‹ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        ".„‚…–š„ …™˜ š…˜ƒ‚„ š€ ‹ƒ’Œ „Œ™‹ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_IMPORT_HIVE,
        ".…™‰˜ š˜……‹ •…— €…‰‰ „Œ™‹ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_FIND_REGISTRY
        "š‹‰š „„š—„ ‹™Œ„ ŒŽ–…€ €š —–‰ Ž‰ƒ’ „˜‰™….",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_CREATE_HIVE,
        ".…™‰˜„ š…˜……‹ š˜‰–‰ „Œ™‹ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        ".…™‰˜„ Œ…‡š€ „Œ™‹ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        ".inf •…— ‰€ Cabinet •…—Œ\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_CABINET_MISSING,
        ".€–Ž €Œ Cabinet •…—\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Œ—…• Cabinet €‰ š‘˜‰ˆ „š—„.\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_COPY_QUEUE,
        ".‰–— š—š’„ ˜…š š‡‰š” „Œ™‹ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_CREATE_DIR,
        ".„—š„„ š…‰—‰š š€ ˜…–‰Œ „Œ‹‰ €Œ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        ".TXTSETUP.SIF '%S' —Œ‡ š€ €…–ŽŒ „Œ™‹ „—š„„ š‰‹š\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_CABINET_SECTION,
        "'%S' —Œ‡ š€ €…–ŽŒ „Œ™‹ „—š„„ š‰‹š\n"
        ".Cabinet •…—\n",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        ".„—š„„ „‰‰—‰š š€ š€ ˜…–‰Œ „Œ‹‰ €Œ „—š„„ š‰‹š",
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_WRITE_PTABLE,
        ".š…–‰‡Ž„ šŒˆ š‰š‹ „Œ™‹ „—š„„ š‰‹š\n"
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        ".…™‰˜Œ codepage š”‘…„ „Œ™‹ „—š„„ š‰‹š\n"
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        ".š‹˜’Ž„ ˜…†€ š€ ’…—Œ „Œ‹‰ €Œ „—š„„ š‰‹šn"
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        ".…™‰˜Œ šƒŒ—Ž š…˜…–š š”‘…„ „Œ™‹ „—š„„ š‰‹š\n"
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_UPDATE_GEOID,
        ".geo id „ š€ ’…—Œ „Œ‹‰ €Œ „—š„„ š‰‹š\n"
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        // ERROR_DIRECTORY_NAME,
        ".‰—…‡ €Œ „‰‰—‰š ™\n"
        "\n"
        ".Š‰™Ž„Œ ‰ƒ‹ ™—Ž Œ’ •‡Œ *  "
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        ".ReactOS š€ ‰—š„Œ ‰ƒ‹ —‰”‘Ž „Œ…ƒ‚ €Œ „˜‡™ „–‰‡Ž„\n"
        ".""Ž %lu Œ™ Œƒ…‚ š…‰„Œ š…‡”Œ š‰‰‡ „—š„„ •–‰‡Ž\n"
        "\n"
        ".Š‰™Ž„Œ ‰ƒ‹ ™—Ž Œ’ •‡Œ *  ",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Œ€ ‰š Œ‰–…˜ Ž‡‰–„ ‡ƒ™„ €… Ž‡‰–„ Ž…˜‡š ‡ƒ™„\n"
        ".š…”‘… š…–‰‡ŽŒ …—Ž €ŒŒ „‰„ „Œˆ„ ‰‹ š…–‰‡Ž„ šŒˆ Œ’\n"
        "\n"
        ".Š‰™Ž„Œ ‰ƒ‹ ™—Ž Œ’ •‡Œ *  "
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        ".‡‰™— …‹ Œ‹Œ š‡€ š‡˜…Ž „–‰‡ŽŽ ˜š…‰ ˜…–‰Œ š‰ €Œ\n"
        "\n"
        ".Š‰™Ž„Œ ‰ƒ‹ ™—Ž Œ’ •‡Œ *  "
    },
    {
        // ERROR_FORMATTING_PARTITION,
        ":„–‰‡Ž„ š€ Œ‡š€Œ „Œ‹‰ €Œ „—š„„ š‰‹š\n"
        " %S\n"
        "\n"
        "™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE heILPages[] =
{
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
        BOOT_LOADER_PAGE,
        heILBootLoaderEntries
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
        BOOT_LOADER_FLOPPY_PAGE,
        heILBootPageEntries
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
     "   ...‰šŽ„Œ €"},
    {STRING_INSTALLCREATEPARTITION,
     "   „—š„ Œ…ˆ‰ = F3  š‡˜…Ž ˜…– = E  š‰™€˜ ˜…– = P  —š„ = ENTER"},
    {STRING_INSTALLCREATELOGICAL,
     "   „—š„ Œ…ˆ‰ = F3  š‰‚…Œ „–‰‡Ž ˜…– = L  —š„ = ENTER"},
    {STRING_INSTALLDELETEPARTITION,
     "   „—š„ Œ…ˆ‰ = F3  „–‰‡Ž —‡Ž = D  —š„ = ENTER"},
    {STRING_DELETEPARTITION,
     "   „—š„ Œ…ˆ‰ = F3  „–‰‡Ž —‡Ž = D"},
    {STRING_PARTITIONSIZE,
     ":„™ƒ‡ „–‰‡Ž Œ™ Œƒ…‚"},
    {STRING_CHOOSENEWPARTITION,
     "Œ’ š‰˜—‰’ „–‰‡Ž ˜…–‰Œ š˜‡"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Œ’ š‡˜…Ž „–‰‡Ž ˜…–‰Œ š˜‡"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Œ’ š‰‚…Œ „–‰‡Ž ˜…–‰Œ š˜‡"},
    {STRING_HDDSIZE,
    ".‰š-„‚Ž „™ƒ‡„ „–‰‡Ž„ Œ™ Œƒ…‚„ š€ ƒŒ—„ „™—"},
    {STRING_CREATEPARTITION,
     "   „—š„ Œ…ˆ‰ = F3  Œ…ˆ‰ = ESC  „–‰‡Ž ˜…– = ENTER"},
    {STRING_PARTFORMAT,
    ".Š™Ž„ Œ‡š…€š š€†„ „–‰‡Ž„"},
    {STRING_NONFORMATTEDPART,
    ".šŒ‡š…€Ž €Œ/„™ƒ‡ „–‰‡Ž Œ’ ReactOS š€ ‰—š„Œ š˜‡"},
    {STRING_NONFORMATTEDSYSTEMPART,
    ".šŒ‡š…€Ž €Œ ‰‰ƒ’ š‹˜’Ž„ š–‰‡Ž"},
    {STRING_NONFORMATTEDOTHERPART,
    ".šŒ‡š…€Ž €Œ ‰‰ƒ’ „™ƒ‡„ „–‰‡Ž„"},
    {STRING_INSTALLONPART,
    "„–‰‡Ž Œ’ ReactOS š€ „‰—šŽ „—š„„ š‰‹š"},
    {STRING_CHECKINGPART,
    ".„˜‡™ „–‰‡Ž„ š€ š—ƒ… …‰™‹’ „—š„„ š‰‹š"},
    {STRING_CONTINUE,
    "Š™Ž„ = ENTER"},
    {STRING_QUITCONTINUE,
    "Š™Ž„ = ENTER  „—š„ Œ…ˆ‰ = F3"},
    {STRING_REBOOTCOMPUTER,
    "™ƒ‡Ž „Œ’”„ = ENTER"},
    {STRING_DELETING,
     "   •…— —‡…Ž: %S"},
    {STRING_MOVING,
     "   •…— ˜‰’Ž: %S Œ: %S"},
    {STRING_RENAMING,
     "   •…— ™ „™Ž: %S Œ: %S"},
    {STRING_COPYING,
     "   •…— —‰š’Ž: %S"},
    {STRING_SETUPCOPYINGFILES,
     "...‰–— „—‰š’Ž „—š„„ š‰‹š"},
    {STRING_REGHIVEUPDATE,
    "   ...…™‰˜„ š…˜……‹ š€ ‹ƒ’Ž"},
    {STRING_IMPORTFILE,
    "   €‰‰Ž %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   ...„‚…–š„ Œ™ …™‰˜ š…˜ƒ‚„ ‹ƒ’Ž"},
    {STRING_LOCALESETTINGSUPDATE,
    "   ...˜…†€ š…˜ƒ‚„ ‹ƒ’Ž"},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   ...šƒŒ—Ž„ š˜…–š š…˜ƒ‚„ š€ ‹ƒ’Ž"},
    {STRING_CODEPAGEINFOUPDATE,
    "   ...…™‰˜Œ codepage ’ƒ‰Ž “‰‘…Ž"},
    {STRING_DONE,
    "   ...‰‰‘Ž"},
    {STRING_REBOOTCOMPUTER2,
    "   ™‡Ž„ š€ ™ƒ‡Ž Œ’”„ = ENTER"},
    {STRING_REBOOTPROGRESSBAR,
    " ...š…‰™ %li ƒ…’ ™ƒ‡Ž Œ’”…‰ Š™‡Ž "},
    {STRING_CONSOLEFAIL1,
    "“…‘Ž„ š€ ‡…š”Œ š‰ €Œ\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "USB šƒŒ—Ž ™…Ž‰™ €…„ „† Œ™ „–…” ‰‹„ „‰‘„\r\n"},
    {STRING_CONSOLEFAIL3,
    "€ŒŽ ”…€ š…‹Žš €Œ ‰‰ƒ’ USB š…ƒŒ—Ž\r\n"},
    {STRING_FORMATTINGDISK,
    "…‹„ š€ šŒ‡š€Ž „—š„„ š‰‹š"},
    {STRING_CHECKINGDISK,
    "…‹„ š€ š—ƒ… „—š„„ š‰‹š"},
    {STRING_FORMATDISK1,
    " (˜‰„Ž Œ…‡š€) %S ‰–— š‹˜’Ž ’ „–‰‡Ž Œ…‡š€ "},
    {STRING_FORMATDISK2,
    " %S ‰–— š‹˜’Ž ’ „–‰‡Ž Œ…‡š€ "},
    {STRING_KEEPFORMAT,
    " (‰…‰™ €ŒŒ( š‰‡‹… ‰–— š‹˜’Ž š˜€™„ "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    // {STRING_HDINFOPARTZEROED_2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    // {STRING_HDINFOPARTEXISTS_2,
    // "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sType %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Œ’ „™ƒ‡ „–‰‡Ž „˜–‰ „—š„„ š‰‹š"},
    {STRING_UNPSPACE,
    "    %sUnpartitioned space%s            %6lu %s"},
    {STRING_MAXSIZE,
    ")""Ž %ul .‘—Ž( ""Ž"},
    {STRING_EXTENDED_PARTITION,
    "š‡˜…Ž „–‰‡Ž"},
    {STRING_UNFORMATTED,
    ")Œ‡š…€Ž €Œ( ™ƒ‡"},
    {STRING_FORMATUNUSED,
    "™…Ž‰™ €Œ"},
    {STRING_FORMATUNKNOWN,
    "’…ƒ‰ €Œ"},
    {STRING_KB,
    """—"},
    {STRING_MB,
    """Ž"},
    {STRING_GB,
    """‚"},
    {STRING_ADDKBLAYOUTS,
    "...šƒŒ—Ž š…˜…–š “‰‘…Ž"},
    {0, 0}
};
