#pragma once

static MUI_ENTRY elGRLanguagePageEntries[] =
{
    {
        4,
        3,
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ДзавжЪу ЪвщййШк.",
        TEXT_STYLE_NORMAL
    },
    {
        7,
        10,
        "\x07 ПШиШбШвщ ЬзавтелЬ лЮ ЪвщййШ зжм ЯШ оиЮйагжзжаЮЯЬх бШлс лЮд ЬЪбШлсйлШйЮ.",
        TEXT_STYLE_NORMAL
    },
    {
        7,
        11,
        "  ЛЬлс зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        7,
        13,
        "\x07  Амлу Ю ЪвщййШ ЯШ ЬхдШа Ю зижЬзавЬЪгтдЮ ЪаШ лж лЬвабц йчйлЮгШ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ  F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        5,
        8,
        "ЙШвщк ОихйШлЬ йлЮд ЬЪбШлсйлШйЮ лжм ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        5,
        11,
        "Амлц лж гтижк лЮк ЬЪбШлсйлШйЮк ШдлаЪиснЬа лж вЬалжмиЪабц йчйлЮгШ ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        5,
        12,
        "йлжд мзжвжЪайлу йШк бШа зижЬлжагсЭЬа лж ЫЬчлЬиж гтижк лЮк ЬЪбШлсйлШйЮк.",
        TEXT_STYLE_NORMAL
    },
    {
        7,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        7,
        17,
        "\x07  ПШлуйлЬ R ЪаШ дШ ЬзаЫажиЯщйЬлЬ лж ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        7,
        19,
        "\x07  ПШлуйлЬ L ЪаШ дШ ЫЬхлЬ лжмк цижмк ШЫЬажЫцлЮйЮк лжм ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        7,
        21,
        "\x07  ПШлуйлЬ F3 ЪаШ дШ ШзжориуйЬлЬ орихк дШ ЬЪбШлШйлуйЬлЬ лж ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        5,
        23,
        "ВаШ зЬиаййцлЬиЬк звЮижнжихЬк ЪаШ лж ReactOS, зШиШбШвжчгЬ ЬзайбЬнЯЬхлЬ лж:",
        TEXT_STYLE_NORMAL
    },
    {
        5,
        24,
        "http://www.reactos.org",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ  R = ДзаЫациЯрйЮ F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "АЫЬажЫцлЮйЮ:",
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
        "Warranty:",
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
        "   ENTER = Дзайлижну",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж зШиШбслр вхйлШ ЫЬходЬа лак имЯгхйЬак ймйбЬмщд.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    УзжвжЪайлук:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "       ДгнсдайЮ:",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "   ПвЮближвцЪаж:",
        TEXT_STYLE_NORMAL
    },
    {
        2,
        14,
        "ГаслШеЮ звЮближвжЪхжм:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "        АзжЫжоу:",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "АзжЫжоу Шмлщд лрд имЯгхйЬрд",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "ЛзжиЬхлЬ дШ ШввсеЬлЬ лак имЯгхйЬак мвабжч зШлщдлШк лШ звублиШ ПАМЧ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "у ЙАТЧ ЪаШ дШ ЬзавтеЬлЬ гаШ ичЯгайЮ.  ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "ЛЬлс зШлуйлЬ лж звуближ ENTER ЪаШ дШ ЬзавтеЬлЬ сввЬк имЯгхйЬак.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "юлШд цвЬк жа имЯгхйЬак ЬхдШа йрйлтк, ЬзавтелЬ ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "\"АзжЫжоу Шмлщд лрд имЯгхйЬрд ймйбЬмщд\" бШа зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ лжм ReactOS ЩихйбЬлШа йЬ зищагж йлсЫаж ШдсзлмеЮк бШа",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ЫЬд мзжйлЮихЭЬа ШбцгШ цвЬк лак ЫмдШлцлЮлЬк гаШк звуижмк ЬЪбШлсйлШйЮк.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Оа вЬалжмиЪхЬк ЬзаЫациЯрйЮк ЫЬд тожмд мвжзжаЮЯЬх ШбцгШ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ПШлуйлЬ U ЪаШ ШдШдтрйЮ лжм вЬалижмиЪабжч.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ПШлуйлЬ R ЪаШ дШ ЬблЬвтйЬлЬ лЮд бждйцвШ ЬзаЫациЯрйЮк.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ПШлуйлЬ ESC ЪаШ дШ ЬзайлитпЬлЬ йлЮд бчиаШ йЬвхЫШ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ПШлуйлЬ ENTER ЪаШ дШ ЬзШдЬббадуйЬлЬ лжд мзжвжЪайлу.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ESC = ЙчиаШ йЬвхЫШ  ENTER = ДзШдЬббхдЮйЮ",
        TEXT_TYPE_STATUS
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "The ReactOS Setup can upgrade one of the available ReactOS installations",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "listed below, or, if a ReactOS installation is damaged, the Setup program",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "can attempt to repair it.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "The repair functions are not all implemented yet.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        15,
        "\x07  Press UP or DOWN to select an OS installation.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press U for upgrading the selected OS installation.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ESC to continue with a new installation.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "U = Upgrade   ESC = Do not upgrade   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ЗтвЬлЬ дШ ШввсеЬлЬ лжд лчзж лжм мзжвжЪайлу зжм ЯШ ЬЪбШлШйлШЯЬх.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ПШлуйлЬ лШ звублиШ ПАМЧ у ЙАТЧ ЪаШ дШ ЬзавтеЬлЬ лжд ЬзаЯмгЮлц",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   лчзж мзжвжЪайлу.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "   ЛЬлс зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "\x07  ПШлуйлЬ лж звуближ ESC ЪаШ дШ ЬзайлитпЬлЬ йлЮд зижЮЪжчгЬдЮ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "   йЬвхЫШ орихк дШ ШввсеЬлЬ лжд лчзж мзжвжЪайлу.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   ESC = АбчирйЮ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Тж йчйлЮгШ ЬзаЩЬЩШащдЬа лщиШ цла цвШ лШ ЫЬЫжгтдШ тожмд ШзжЯЮбЬмлЬх",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Амлц хйрк зсиЬа вхЪЮ щиШ",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "юлШд жвжбвЮирЯЬх, ж мзжвжЪайлук йШк ЯШ ЬзШдЬббадЮЯЬх ШмлцгШлШ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ДббШЯсиайЮ зижйриадщд ШиоЬхрд...",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Тж ReactOS ЫЬд ЬЪбШлШйлсЯЮбЬ звуирк",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "АнШаитйлЬ лЮ ЫайбтлШ Шзц лж A: бШа",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "цвШ лШ CD-ROMs  Шзц лШ CD-Drives.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ПШлуйлЬ ENTER ЪаШ дШ ЬзШдЬббадуйЬлЬ лжд мзжвжЪайлу.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ПШиШбШвщ зЬиагтдЬлЬ ...",
        TEXT_TYPE_STATUS,
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ЗтвЬлЬ дШ ШввсеЬлЬ лжд лчзж лЮк ЬгнсдайЮк зжм ЯШ ЬЪбШлШйлШЯЬх.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  ПШлуйлЬ лж звуближ ПАМЧ у ЙАТЧ ЪаШ дШ ЬзавтеЬлЬ лжд ЬзаЯмгЮлц.",
         TEXT_STYLE_NORMAL
    },
    {   8,
        11,
         "  лчзж ЬгнсдайЮк.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "   ЛЬлс зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "\x07  ПШлуйлЬ лж звуближ ESC ЪаШ дШ ЬзайлитпЬлЬ йлЮд зижЮЪжчгЬдЮ ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "   йЬвхЫШ орихк дШ ШввсеЬлЬ лжд лчзж ЬгнсдайЮк.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   ESC = АбчирйЮ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ТШ ЩШйабс йлжаоЬхШ лжм ReactOS ЬЪбШлШйлсЯЮбШд Ьзалмощк.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "АнШаитйлЬ лЮ ЫайбтлШ Шзц лж A: бШа",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "цвШ лШ CD-ROMs Шзц лж CD-Drive.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ПШлуйлЬ ENTER ЪаШ дШ ЬзШдЬббадуйЬлЬ лжд мзжвжЪайлу йШк.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRBootPageEntries[] =
{
    {
        4,
        3,
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзжиЬх дШ ЬЪбШлШйлуйЬа лжд bootloader",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "йлж йбвЮиц Ыхйбж лжм мзжвжЪайлу йШк",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "ПШиШбШвщ ЬайсЪЬлЬ гаШ ЫаШгжинргтдЮ ЫайбтлШ йлж A: бШа",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж зШиШбШлр вхйлШ ЬгнШдхЭЬа лШ мзсиождлШ ЫаШгЬихйгШлШ бШа",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "бШа лжд ЬвтмЯЬиж ощиж ЪаШ дтШ ЫаШгЬихйгШлШ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  ПШлуйлЬ ПАМЧ у ЙАТЧ ЪаШ дШ ЬзавтеЬлЬ тдШ йлжаоЬхж лЮк вхйлШк.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ПШлуйлЬ ENTER ЪаШ дШ ЬЪбШлШйлуйЬлЬ лж ReactOS йлж ЬзавЬЪгтдж ЫаШгтиайгШ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  ПШлуйлЬ C ЪаШ дШ ЫЮгажмиЪуйЬлЬ тдШ дтж ЫаШгтиайгШ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press E to create an extended partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press L to create a logical partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ПШлуйлЬ D ЪаШ дШ ЫаШЪиспЬлЬ тдШ мзсиожд ЫаШгтиайгШ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ПШиШбШвщ зЬиагтдЬлЬ...",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   not be deleted.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ГаШгцинрйЮ ЫаШгЬихйгШлжк",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Ж ЬЪбШлсйлШйЮ лщиШ ЯШ ЫаШгжинщйЬа лж ЫаШгтиайгШ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "ПШлуйлЬ ENTER ЪаШ дШ ймдЬохйЬлЬ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY elGRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ ШдлаЪиснЬа лШ ШиоЬхШ лжм ReactOS йлж ЬзавЬЪгтдж ЫаШгтиайгШ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "ДзавтелЬ лжд йлжд нсбЬвж зжм ЯтвЬлЬ дШ ЬЪбШлШйлШЯЬх лж ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "ВаШ дШ ШввсеЬлЬ лжд зижлЬадцгЬдж нсбЬвж зШлуйлЬ BACKSPACE ЪаШ дШ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "ЫаШЪиспЬлЬ оШиШблуиЬк бШа гЬлс звЮближвжЪЬхйлЬ лжд нсбЬвж йлжд жзжхж",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "ЯтвЬлЬ дШ ЬЪбШйлШЯЬх лж ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "ПШиШбШвщ зЬиагтдЬлЬ цйж Ю ЬЪбШлсйлШйЮ лжм ReactOS ШдлаЪиснЬа",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "лШ ШиоЬхШ йлж нсбЬвж ЬЪбШлсйлШйЮк",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Амлу Ю ЫаШЫабШйхШ гзжиЬх дШ биШлуйЬа ШибЬлс вЬзлс.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        0,
        "                                                           \xB3 ПШиШбШвщ зЬиагтдЬлЬ...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRBootLoaderEntries[] =
{
    {
        4,
        3,
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ ЪиснЬа лжд boot loader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "ДЪбШлсйлШйЮ лжм bootloader йлж йбвЮиц Ыхйбж (MBR and VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "ДЪбШлсйлШйЮ лжм bootloader йлж йбвЮиц Ыхйбж (VBR only).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "ДЪбШлсйлШйЮ лжм bootloader йЬ гаШ ЫайбтлШ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "МШ гЮ ЪхдЬа ЬЪбШлсйлШйЮ лжм bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ЗтвЬлЬ дШ ШввсеЬлЬ лжд лчзж лжм звЮближвжЪхжм зжм ЯШ ЬЪбШлШйлШЯЬх.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ПШлуйлЬ лШ звублиШ ПАМЧ у ЙАТЧ ЪаШ дШ ЬзавтеЬлЬ лжд ЬзаЯмгЮлц",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   лчзж звЮближвжЪхжм.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "   ЛЬлс зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "\x07  ПШлуйлЬ лж звуближ ESC ЪаШ дШ ЬзайлитпЬлЬ йлЮд зижЮЪжчгЬдЮ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "   йЬвхЫШ орихк дШ ШввсеЬлЬ лжд лчзж лжм звЮближвжЪхжм.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   ESC = АбчирйЮ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ПШиШбШвщ ЬзавтелЬ гаШ ЫаслШеЮ ЪаШ дШ ЬЪбШлШйлШЯЬх рк зижЬзавЬЪгтдЮ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ПШлуйлЬ лШ звублиШ ПАМЧ у ЙАТЧ ЪаШ дШ ЬзавтеЬлЬ лЮд ЬзаЯмЮгЮлу",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ЫаслШеЮ звЮближвжЪхжм. ЛЬлс зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ПШлуйлЬ лж звуближ ESC ЪаШ дШ ЬзайлитпЬлЬ йлЮд зижЮЪжчгЬдЮ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   йЬвхЫШ орихк дШ ШввсеЬлЬ лЮд ЫаслШеЮ лжм звЮближвжЪхжм.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   ESC = АбчирйЮ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ зижЬлжагсЭЬа лжд мзжвжЪайлу йШк ЪаШ лЮд ШдлаЪиШну лрд ШиоЬхрд лжм ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ХлхЭЬлШа Ю вхйлШ лрд ШиоЬхрд зижк ШдлаЪиШну...",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "ДзавтелЬ тдШ йчйлЮгШ ШиоЬхрд Шзц лЮд зШиШбслр вхйлШ.",
        0
    },
    {
        8,
        19,
        "\x07  ПШлуйлЬ лШ звублиШ ПАМЧ у ЙАТЧ ЪаШ дШ ЬзавтеЬлЬ лж йчйлЮгШ ШиоЬхрд.",
        0
    },
    {
        8,
        21,
        "\x07  ПШлуйлЬ ENTER ЪаШ дШ ЫаШгжинщйЬлЬ лж parition.",
        0
    },
    {
        8,
        23,
        "\x07  ПШлуйлЬ ESC ЪаШ дШ ЬзавтеЬлЬ сввж partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = СмдтоЬаШ   ESC = АбчирйЮ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ДзавтеШлЬ дШ ЫаШЪиспЬлЬ Шмлц лж partition",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  ПШлуйлЬ D ЪаШ дШ ЫаШЪиспЬлЬ лж partition.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ПРОДИГОПОИЖСЖ: ювШ лШ ЫЬЫжгтдШ йЬ Шмлц лж partition ЯШ оШЯжчд!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ПШлуйлЬ ESC ЪаШ ШбчирйЮ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = ГаШЪиШну Partition   ESC = АбчирйЮ   F3 = АзжощиЮйЮ",
        TEXT_TYPE_STATUS
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
        " ДЪбШлсйлШйЮ лжм ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ ШдШдЬщдЬа лЮ Ыжгу лжм ймйлугШлжк. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ГЮгажмиЪжчдлШа лШ registry hives...",
        TEXT_TYPE_STATUS
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
        "Тж ReactOS ЫЬд ЬЪбШлШйлсЯЮбЬ звуирк йлжд\n"
        "мзжвжЪайлу йШк. Ад ШзжориуйЬлЬ Шзц лЮд ДЪбШлсйлШйЮ лщиШ, ЯШ зитзЬа дШ\n"
        "еШдШлитеЬлЬ лЮд ДЪбШлсйлШйЮ ЪаШ дШ ЬЪбШлШйлуйЬл лж ReactOS.\n"
        "\n"
        "  \x07  ПШлуйлЬ ENTER ЪаШ Шд ймдЬохйЬлЬ лЮд ДЪбШлсйлШйЮ.\n"
        "  \x07  ПШлуйлЬ F3 ЪаШ дШ ШзжориуйЬлЬ Шзц лЮд ДЪбШлсйлШйЮ.",
        "F3 = АзжощиЮйЮ  ENTER = СмдтоЬаШ"
    },
    {
        // ERROR_NO_HDD
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ ЩиЬа бсзжажд йбвЮиц Ыхйбж.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лж ШиоЬхж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Ж ЬЪбШлсйлйЮ ЩиубЬ тдШ бШлЬйлиШгтдж ШиоЬхж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Ж ДЪбШлсйлШйЮ ЩиубЬ гаШ гЮ тЪбмиЮ мзжЪиШну йлж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лак звЮижнжихЬк лжм Ыхйбжм ймйлугШлжк.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_WRITE_BOOT,
        "Setup failed to install %S bootcode on the system partition.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ лчзрд мзжвжЪайлу.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ лчзрд ЬгнсдайЮк.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ лчзрд звЮближвжЪхжм.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ ЫаШлсеЬрд звЮближвжЪхжм.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_WARN_PARTITION,
          "Ж ЬЪбШлсйлШйЮ ЩиубЬ цла лжмвсоайлжд тдШк йбвЮицк Ыхйбжк зЬиатоЬа тдШ гЮ ймгЩШлц\n"
          "partition table зжм ЫЬ гзжиЬх дШ ЬвЬЪоЯЬх йрйлс!\n"
          "\n"
          "Ж ЫЮгажмиЪхШ у ЫаШЪиШну partitions гзжиЬх дШ бШлШйлитпЬа лж partition table.\n"
          "\n"
          "  \x07  ПШлуйлЬ F3 ЪаШ дШ ШзжориуйЬлЬ Шзц лЮд ДЪбШлсйлШйЮ.\n"
          "  \x07  ПШлуйлЬ ENTER ЪаШ дШ ймдЬохйЬлЬ.",
          "F3 = АзжощиЮйЮ  ENTER = СмдтоЬаШ"
    },
    {
        // ERROR_NEW_PARTITION,
        "ГЬ гзжиЬхлЬ дШ ЫЮгажмиЪуйЬлЬ тдШ Partition гтйШ йЬ\n"
        "тдШ сввж мзсиожд Partition!\n"
        "\n"
        "  * ПШлуйлЬ жзжажЫузжлЬ звуближ ЪаШ дШ ймдЬохйЬлЬ.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "ГЬ гзжиЬхлЬ дШ ЫаШЪиспЬлЬ тдШд гЮ ЫаШгжинргтдж ощиж Ыхйбжм!\n"
        "\n"
        "  * ПШлуйлЬ жзжажЫузжлЬ звуближ ЪаШ дШ ймдЬохйЬлЬ.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the %S bootcode on the system partition.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_NO_FLOPPY,
        "ГЬд мзсиоЬа ЫайбтлШ йлж A:.",
        "ENTER = СмдтоЬаШ"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Ж ДЪбШлсйШйЮ ШзтлмоЬ дШ ШдШдЬщйЬа лак имЯгхйЬак ЪаШ лЮ ЫаслШеЮ звЮближвжЪхжм.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ ШдШдЬщйЬа лак имЯгхйЬак гЮлищжм ЪаШ лЮд ЬгнсдайЮ.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ нжилщйЬа тдШ hive ШиоЬхж.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_FIND_REGISTRY
        "Ж ЬЪбШлсйШйЮ ШзтлмоЬ дШ ЩиЬа лШ ШиоЬхШ ЫЬЫжгтдрд лжм гЮлищжм.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_CREATE_HIVE,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ ЫЮгажмиЪуйЬа лШ registry hives.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ ШиоабжзжауйЬа лж гЮлищж.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Тж cabinet ЫЬд тоЬа тЪбмиж ШиоЬхж inf.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_CABINET_MISSING,
        "Тж cabinet ЫЬ ЩитЯЮбЬ.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Тж cabinet ЫЬд тоЬа бШдтдШ йбиазл ЬЪбШлсйлШйЮк.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_COPY_QUEUE,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ ШджхеЬа лЮд жмис ШиоЬхрд зижк ШдлаЪиШну.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_CREATE_DIR,
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ ЫЮгажмиЪуйЬа лжмк бШлШвцЪжмк ЬЪбШлсйлШйЮк.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ ЩиЬа лжд лжгтШ '%S'\n"
        "йлж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_CABINET_SECTION,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ ЩиЬа лжд лжгтШ '%S'\n"
        "йлж cabinet.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ ЫЮгажмиЪуйЬа лжд бШлсвжЪж ЬЪбШлсйлШйЮк.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ ЪиспЬа лШ partition tables.\n"
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Ж ЬЪбШлсйлШйЮ ШзтлмоЬ дШ зижйЯтйЬа лак ЫаШлсеЬак звЮближвжЪхрд йлж гЮлищж.\n"
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
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
        "You can not create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "You can not create more than one extended partition per disk.\n"
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
        BOOT_LOADER_PAGE,
        elGRBootLoaderEntries
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
        BOOT_LOADER_FLOPPY_PAGE,
        elGRBootPageEntries
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
     "   ПШиШбШвщ зЬиагтдЬлЬ..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = ДЪбШлсйлШйЮ   C = ГЮгажмиЪхШ Partition   F3 = АзжощиЮйЮ"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = ДЪбШлсйлШйЮ   D = ГаШЪиШну Partition   F3 = АзжощиЮйЮ"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "ЛтЪЬЯжк лжм дтжм partition:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//     "ДзавтеШлЬ дШ ЫЮгажмиЪуйЬлЬ тдШ дтж partition on"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "ПШиШбШвщ ЫщйлЬ лж гтЪЬЯжк лжм partition йЬ megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = ГЮгажмиЪхШ Partition   ESC = АбчирйЮ   F3 = АзжощиЮйЮ"},
    {STRING_PARTFORMAT,
    "Амлц лж Partition ЯШ ЫаШгжинрЯЬх гЬлс."},
    {STRING_NONFORMATTEDPART,
    "ДзавтеШлЬ дШ ЬЪбШлШйлуйЬлЬ лж ReactOS йЬ тдШ дтж у гЮ ЫаШгжинргтдж Partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Setup install ReactOS onto Partition"},
    {STRING_CHECKINGPART,
    "Ж ЬЪбШлсйлШйЮ ЬвтЪоЬа лщиШ лж ЬзавЬЪгтдж partition."},
    {STRING_CONTINUE,
    "ENTER = СмдтоЬаШ"},
    {STRING_QUITCONTINUE,
    "F3 = АзжощиЮйЮ  ENTER = СмдтоЬаШ"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   АдлаЪиснЬлШа лж ШиоЬхж: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Ж ЬЪбШлсйлШйЮ ШдлаЪиснЬа ШиоЬхШ..."},
    {STRING_REGHIVEUPDATE,
    "   ВхдЬлШа ШдШдтрйЮ лрд registry hives..."},
    {STRING_IMPORTFILE,
    "   ВхдЬлШа ЬайШЪрЪу лжм %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   ВхдЬлШа ШдШдтрйЮ лрд имЯгхйЬрд ЬгнсдайЮк лжм гЮлищжм..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   ВхдЬлШа ШдШдтрйЮ лрд имЯгхйЬрд ЪвщййШк..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   ВхдЬлШа ШдШдтрйЮ лрд имЯгхйЬрд ЫаслШеЮк звЮближвжЪхжм..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adding codepage information to registry..."},
    {STRING_DONE,
    "   ОвжбвЮищЯЮбЬ..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = ДзЬдаббхдЮйЮ мзжвжЪайлу"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "АЫчдШлж дШ ШджаолЬх Ю бждйцвШ\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "The most common cause of this is using an USB keyboard\r\n"},
    {STRING_CONSOLEFAIL3,
    "ТШ USB звЮближвцЪаШ ЫЬд ЬхдШа звуирк мзжйлЮиаЭцгЬдШ ШбцгШ\r\n"},
    {STRING_FORMATTINGDISK,
    "Ж ЬЪбШлсйлШйЮ ЫаШгжинщдЬа лж Ыхйбж йШк"},
    {STRING_CHECKINGDISK,
    "Ж ЬЪбШлсйлШйЮ ЬвтЪоЬа лж Ыхйбж йШк"},
    {STRING_FORMATDISK1,
    " ГаШгцинрйЮ лжм partition рк %S йчйлЮгШ ШиоЬхрд (ЪиуЪжиЮ ЫаШгцинрйЮ) "},
    {STRING_FORMATDISK2,
    " ГаШгцинрйЮ лжм partition рк %S йчйлЮгШ ШиоЬхрд "},
    {STRING_KEEPFORMAT,
    " МШ зШиШгЬхдЬа лж йчйлЮгШ ШиоЬхрд рк тоЬа (бШгхШ ШввШЪу) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  СбвЮицк Ыхйбжк %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  СбвЮицк Ыхйбжк %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "СбвЮицк Ыхйбжк %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "йлж йбвЮиц Ыхйбж %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sType %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  СбвЮицк Ыхйбжк %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  СбвЮицк Ыхйбжк %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Ж ЬЪбШлсйлШйЮ ЫЮгажчиЪЮйЬ тдШ дтж partition йлж"},
    {STRING_UNPSPACE,
    "    %sUnpartitioned space%s            %6lu %s"},
    {STRING_MAXSIZE,
    "MB (гЬЪ. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Мтж (ЛЮ ЫаШгжинргтдж)"},
    {STRING_FORMATUNUSED,
    "АоиЮйагжзжхЮлж"},
    {STRING_FORMATUNKNOWN,
    "ъЪдрйлж"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "ВхдЬлШа зижйЯубЮ лрд ЫаШлсеЬрд звЮближвжЪхжм"},
    {0, 0}
};
