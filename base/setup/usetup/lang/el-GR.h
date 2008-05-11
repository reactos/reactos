#ifndef LANG_EL_GR_H__
#define LANG_EL_GR_H__

static MUI_ENTRY elGRLanguagePageEntries[] =
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
        "Language Selection.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  This Language will be the default language for the final system.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue  F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ЙШвщк ОихйШлЬ йлЮд ЬЪбШлсйлШйЮ лжм ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Амлц лж гтижк лЮк ЬЪбШлсйлШйЮк ШдлаЪиснЬа лж вЬалжмиЪабц йчйлЮгШ ReactOS йлжд",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "мзжвжЪайлу йШк бШа зижЬлжагсЭЬа лж ЫЬчлЬиж гтижк лЮк ЬЪбШлсйлШйЮк.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ПШлуйлЬ ENTER ЪаШ дШ ЬЪбШлШйлуйЬлЬ лж ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ПШлуйлЬ R ЪаШ дШ ЬзаЫажиЯщйЬлЬ лж ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ПШлуйлЬ L ЪаШ дШ ЫЬхлЬ лжмк цижмк ШЫЬажЫцлЮйЮк бШа лак зижшзжЯтйЬак ЪаШ лж ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ПШлуйлЬ F3 ЪаШ дШ ШзжориуйЬлЬ орихк дШ ЬЪбШлШйлуйЬлЬ лж ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "ВаШ зЬиаййцлЬиЬк звЮижнжихЬк ЪаШ лж ReactOS, зШиШбШвжчгЬ ЬзайбЬнЯЬхлЬ лж:",
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
        "Ж ЬЪбШлсйлШйЮ лжм ReactOS ЬхдШа йЬ Шиоабу нсйЮ зижЪиШггШлайгжч. ГЬд мзжйлЮихЭЬа ШбцгШ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "цвЬк лак вЬалжмиЪабцлЮлШк гаШк звуижмк ЬЪбШлсйлШйЮк вЬалжмиЪабжч.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Оа ЬзцгЬджа зЬиажиайгжх айочжмд:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Ж ЬЪбШлсйлШйЮ ЫЬ гзжиЬх дШ ШдлШзЬетвЯЬа гЬ зсдр Шзц тдШ primary partition Шдс Ыхйбж.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Ж ЬЪбШлсйлШйЮ ЫЬ гзжиЬх дШ ЫаШЪиспЬа тдШ primary partition Шзц тдШ Ыхйбж",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  Ьнцйжд мзсиожмд extended partitions йлж Ыхйбж Шмлц.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Ж ЬЪбШлсйШйЮ ЫЬ гзжиЬх дШ ЫаШЪиспЬа лж зищлж extended partition Ьдцк Ыхйбжм",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  Ьнцйжд мзсиожмд ба сввШ extended partitions йлж Ыхйбж Шмлц.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Ж ЬЪбШлсйлШйЮ мзжйлЮихЭЬа гцдж FAT ймйлугШлШ ШиоЬхрд.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- File system checks are not implemented yet.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  ПШлуйлЬ ENTER ЪаШ дШ ЬЪбШлШйлуйЬлЬ лж ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  ПШлуйлЬ F3 ЪаШ дШ ШзжориуйЬлЬ орихк дШ ЬЪбШлШйлуйЬлЬ лж ReactOS.",
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

static MUI_ENTRY elGRLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licensing:",
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
        "   ENTER = Return",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "       УзжвжЪайлук:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "        ДгнсдайЮ:",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "       ПвЮближвцЪаж:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "ГаслШеЮ звЮближвжЪхжм:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "         АзжЫжоу:",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "АзжЫжоу Шмлщд лрд имЯгхйЬрд ймйбЬмщд",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "ЛзжиЬхлЬ дШ ШввсеЬлЬ лак имЯгхйЬак мвабжч зШлщдлШк лШ звублиШ UP у DOWN",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "ЪаШ дШ ЬзавтеЬлЬ гаШ ичЯгайЮ. ЛЬлс зШлуйлЬ лж звуближ ENTER ЪаШ дШ ЬзавтеЬлЬ сввЬк",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "имЯгхйЬак.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "юлШд цвЬк жа имЯгхйЬак ЬхдШа йрйлтк, ЬзавтелЬ \"АзжЫжоу Шмлщд лрд имЯгхйЬрд ймйбЬмщд\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "бШа зШлуйлЬ ENTER.",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ лжм ReactOS ЩихйбЬлШа йЬ зищагж йлсЫаж ШдсзлмеЮк. ГЬд мзжйлЮихЭЬа ШбцгШ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "цвЬк лак вЬалжмиЪабцлЮлЬк гаШк звуижмк ЬЪбШлсйлШйЮк вЬалжмиЪабжч.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Ж вЬалжмиЪхЬк ЬзаЫациЯрйЮк ЫЬд тожмд мвжзжаЮЯЬх ШбцгШ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ПШлуйлЬ U ЪаШ ШдШдтрйЮ лжм OS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ПШлуйлЬ R ЪаШ лЮ Recovery Console.",
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
static MUI_ENTRY elGRComputerPageEntries[] =
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
        "ЗтвЬлЬ дШ ШввсеЬлЬ лжд лчзж лжм мзжвжЪайлу зжм ЯШ ЬЪбШлШйлШЯЬх.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ПШлуйлЬ лШ звублиШ UP у DOWN ЪаШ дШ ЬзавтеЬлЬ лжд ЬзаЯмгЮлц лчзж мзжвжЪайлу.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ЛЬлс зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ПШлуйлЬ лж звуближ ESC ЪаШ дШ ЬзайлитпЬлЬ йлЮд зжиЮЪжчгЬдЮ йЬвхЫШ орихк дШ ШввсеЬлЬ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   лжд лчзж мзжвжЪайлу.",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Тж йчйлЮгШ ЬзаЩЬЩШащдЬа лщиШ цла цвШ лШ ЫЬЫжгтдШ ШзжЯЮбЬчлЮбШд йлж Ыхйбж",
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
        "   Flushing cache",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
         "\x07  ПШлуйлЬ лж звуближ UP у DOWN ЪаШ дШ ЬзавтеЬлЬ лжд ЬзаЯмгЮлц лчзж ЬгнсдайЮк.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   ЛЬлс зШлуйлЬ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ПШлуйлЬ лж звуближ ESC ЪаШ дШ ЬзайлитпЬлЬ йлЮд зижЮЪжчгЬдЮ йЬвхЫШ орихк дШ ШввсеЬлЬ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   лжд лчзж ЬгнсдайЮк.",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ж ЬЪбШлсйлШйЮ ЫЬ гзжиЬх дШ ЬЪбШлШйлуйЬа лжд bootloader йлж йбвЮиц Ыхйбж",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "лжм мзжвжЪайлу йШк",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "The list below shows existing partitions and unused disk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "space for new partitions.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Press UP or DOWN to select a list entry.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press ENTER to install ReactOS onto the selected partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press C to create a new partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press D to delete an existing partition.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Please wait...",
        TEXT_TYPE_STATUS
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Format partition",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Setup will now format the partition. Press ENTER to continue.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup installs ReactOS files onto the selected partition. Choose a",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "directory where you want ReactOS to be installed:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "To change the suggested directory, press BACKSPACE to delete",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "characters and then type the directory where you want ReactOS to",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        30,
        13,
        "installation folder.",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "This may take several minutes to complete.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Please wait...    ",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup is installing the boot loader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Install bootloader on the harddisk (MBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on a floppy disk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Skip install bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the type of keyboard to be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard type.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the keyboard type.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the keyboard layout to be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    layout. Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the keyboard layout.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup prepares your computer for copying the ReactOS files. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Building the file copy list...",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Select a file system from the list below.",
        0
    },
    {
        8,
        19,
        "\x07  Press UP or DOWN to select a file system.",
        0
    },
    {
        8,
        21,
        "\x07  Press ENTER to format the partition.",
        0
    },
    {
        8,
        23,
        "\x07  Press ESC to select another partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the partition",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Press D to delete the partition.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "WARNING: All data on this partition will be lost!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ESC to cancel.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Delete Partition   ESC = Cancel   F3 = Quit",
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
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup is updating the system configuration. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Creating registry hives...",
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
        //ERROR_NOT_INSTALLED
        "Тж ReactOS ЫЬд ЬЪбШлШйлсЯЮбЬ звуирк йлжд\n"
        "мзжвжЪайлу йШк. Ад ШзжориуйЬлЬ Шзц лЮд ДЪбШлсйлШйЮ лщиШ, ЯШ зитзЬа дШ\n"
        "еШдШлитеЬлЬ лЮд ДЪбШлсйлШйЮ ЪаШ дШ ЬЪбШлШйлуйЬл лж ReactOS.\n"
        "\n"
        "  \x07  ПШлуйлЬ ENTER ЪаШ Шд ймдЬохйЬлЬ лЮд ДЪбШлсйлШйЮ.\n"
        "  \x07  ПШлуйлЬ F3 ЪаШ дШ ШзжориуйЬлЬ Шзц лЮд ДЪбШлсйлШйЮ.",
        "F3= АзжощиЮйЮ  ENTER = СмдтоЬаШ"
    },
    {
        //ERROR_NO_HDD
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ ЩиЬа бсзжажд йбвЮиц Ыхйбж.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лж ШиоЬхж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Ж ДЪбШлсйлйЮ ЩиубЬ тдШ бШлЬйлиШгтдж ШиоЬхж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Ж ДЪбШлсйлШйЮ ЩиубЬ гаШ гЮ тЪбмиЮ мзжЪиШну йлж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лак звЮижнжихЬк лжм Ыхйбжм ймйлугШлжк.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ лчзрд мзжвжЪайлу.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ лчзрд ЬгнсдайЮк.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ лчзрд звЮближвжЪхжм.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ нжилщйЬа лЮ вхйлШ ЫаШлсеЬрд звЮближвжЪхжм.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_WARN_PARTITION,
          "Ж ДЪбШлсйлШйЮ ЩиубЬ цла лжмвсоайлжд тдШк йбвЮицк Ыхйбжк зЬиатоЬа тдШ гЮ ймгЩШлц\n"
          "partition table зжм ЫЬ гзжиЬх дШ ЬвЬЪоЯЬх йрйлс!\n"
          "\n"
          "Ж ЫЮгажмиЪхШ у ЫаШЪиШну partitions гзжиЬх дШ бШлШйлитпЬа лж partiton table.\n"
          "\n"
          "  \x07  ПШлуйлЬ F3 ЪаШ дШ ШзжориуйЬлЬ Шзц лЮд ДЪбШлсйлШйЮ."
          "  \x07  ПШлуйлЬ ENTER ЪаШ дШ ймдЬохйЬлЬ.",
          "F3= АзжощиЮйЮ  ENTER = СмдтоЬаШ"
    },
    {
        //ERROR_NEW_PARTITION,
        "ГЬ гзжиЬхлЬ дШ ЫЮгажмиЪуйЬлЬ тдШ Partition гтйШ йЬ\n"
        "тдШ сввж мзсиожд Partition!\n"
        "\n"
        "  * ПШлуйлЬ жзжажЫузжлЬ звуближ ЪаШ дШ ймдЬохйЬлЬ.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "ГЬ гзжиЬхлЬ дШ ЫаШЪиспЬлЬ тдШд гЮ ЫаШгжинргтдж ощиж Ыхйбжм!\n"
        "\n"
        "  * ПШлуйлЬ жзжажЫузжлЬ звуближ ЪаШ дШ ймдЬохйЬлЬ.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_NO_FLOPPY,
        "ГЬд мзсиоЬа ЫайбтлШ йлж A:.",
        "ENTER = СмдтоЬаШ"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Ж ДЪбШлсйШйЮ ШзтлмоЬ дШ ШдШдЬщйЬа лак имЯгхйЬак ЪаШ лЮ ЫаслШеЮ звЮближвжЪхжм.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ ШдШдЬщйЬа лак имЯгхйЬак гЮлищжм ЪаШ лЮд ЬгнсдайЮ.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ нжилщйЬа тдШ hive ШиоЬхж.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_FIND_REGISTRY
        "Ж ДЪбШлсйШйЮ ШзтлмоШ дШ ЩиЬа лШ ШиоЬхШ ЫЬЫжгтдрд лжм гЮлищжм.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_CREATE_HIVE,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ ЫЮгажмиЪуйЬа лШ registry hives.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Тж cabinet ЫЬд тоЬа тЪбмиж ШиоЬхж inf.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_CABINET_MISSING,
        "Тж cabinet ЫЬ ЩитЯЮбЬ.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Тж cabinet ЫЬд тоЬа бШдтдШ йбиазл ЬЪбШлсйлШйЮк.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_COPY_QUEUE,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ ШджхеЬа лЮд жмис ШиоЬхрд зижк ШдлаЪиШну.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_CREATE_DIR,
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ ЫЮгажмиЪуйЬа лжмк бШлШвцЪжмк ЬЪбШлсйлШйЮк.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ ЩиЬа лжд лжгтШ 'Directories'\n"
        "йлж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_CABINET_SECTION,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ ЩиЬа лжд лжгтШ 'Directories'\n"
        "йлж cabinet.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Ж ДЪбШлсйлШйЮ ЫЬ гзциЬйЬ дШ ЫЮгажмиЪуйЬа лжд бШлсвжЪж ЬЪбШлсйлШйЮк.",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ ЩиЬа лжд лжгтШ 'SetupData'\n"
        "йлж TXTSETUP.SIF.\n",
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Ж ДЪбШлсйлШйЮ ШзтлмоЬ дШ ЪиспЬа лШ partition tables.\n"
        "ENTER = ДзШдЬббхдЮйЮ мзжвжЪайлу"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Setup failed to add keyboard layouts to registry.\n"
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
       START_PAGE,
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
     "   Please wait..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   C = Create Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Install   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Size of new partition:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a new partition on"},
    {STRING_HDDSIZE,
    "Please enter the size of the new partition in megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Create Partition   ESC = Cancel   F3 = Quit"},
    {STRING_PARTFORMAT,
    "This Partition will be formatted next."},
    {STRING_NONFORMATTEDPART,
    "You chose to install ReactOS on a new or unformatted Partition."},
    {STRING_INSTALLONPART,
    "Setup install ReactOS onto Partition"},
    {STRING_CHECKINGPART,
    "Setup is now checking the selected partition."},
    {STRING_QUITCONTINUE,
    "F3= Quit  ENTER = Continue"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Reboot computer"},
    {STRING_TXTSETUPFAILED,
    "Setup failed to find the '%S' section\nin TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "\xB3 Copying file: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup is copying files..."},
    {STRING_PAGEDMEM,
     "Paged Memory"},
    {STRING_NONPAGEDMEM,
     "Nonpaged Memory"},
    {STRING_FREEMEM,
     "Free Memory"},
    {STRING_REGHIVEUPDATE,
    "   Updating registry hives..."},
    {STRING_IMPORTFILE,
    "   Importing %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Updating display registry settings..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Updating locale settings..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Updating keyboard layout settings..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adding codepage information to registry..."},
    {STRING_DONE,
    "   Done..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Reboot computer"},
    {STRING_CONSOLEFAIL1,
    "Unable to open the console\n\n"},
    {STRING_CONSOLEFAIL2,
    "The most common cause of this is using an USB keyboard\n"},
    {STRING_CONSOLEFAIL3,
    "USB keyboards are not fully supported yet\n"},
    {STRING_FORMATTINGDISK,
    "Setup is formatting your disk"},
    {STRING_CHECKINGDISK,
    "Setup is checking your disk"},
    {STRING_FORMATDISK1,
    " Format partition as %S file system (quick format) "},
    {STRING_FORMATDISK2,
    " Format partition as %S file system "},
    {STRING_KEEPFORMAT,
    " Keep current file system (no changes) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Type %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup created a new partition on"},
    {STRING_UNPSPACE,
    "    Unpartitioned space              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_UNFORMATTED,
    "New (Unformatted)"},
    {STRING_FORMATUNUSED,
    "Unused"},
    {STRING_FORMATUNKNOWN,
    "Unknown"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Adding keyboard layouts"},
    {0, 0}
};

#endif
