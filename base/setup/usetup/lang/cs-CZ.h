// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/* FILE:        base/setup/usetup/lang/cs-CZ.rc
 * TRANSLATOR:  Radek Liska aka Black_Fox (radekliska at gmail dot com)
 * THANKS TO:   preston for bugfix advice at line 848
 * UPDATED:     2015-04-12
 */

#pragma once

static MUI_ENTRY csCZSetupInitPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY csCZLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "V\354b\330r jazyka",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Pros\241m zvolte jazyk, kter\354 bude b\330hem instalace pou\247it.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Pot\202 stiskn\330te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tento jazyk bude v\354choz\241m jazykem v nainstalovan\202m syst\202mu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat  F3 = Ukon\237it",
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

static MUI_ENTRY csCZWelcomePageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "V\241tejte v instalaci ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Tato \237\240st instalace nakop\241ruje opera\237n\241 syst\202m ReactOS do va\347eho",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "po\237\241ta\237e a p\375iprav\241 druhou \237\240st instalace.",
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
        "\x07  Stisknut\241m R zah\240j\241te opravu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Stiskut\241m L zobraz\241te Licen\237n\241 podm\241nky ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Stisknut\241m F3 ukon\237\241te instalaci ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "V\241ce informac\241 o ReactOS naleznete na adrese:",
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
        "ENTER = Pokra\237ovat  R = Opravit  L = Licence  F3 = Ukon\237it",
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

static MUI_ENTRY csCZIntroPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY csCZLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licence:",
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
        "Z\240ruka:",
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
        "ENTER = Zp\330t",
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

static MUI_ENTRY csCZDevicePageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "N\240sleduj\241c\241 seznam zobrazuje sou\237asn\202 nastaven\241 za\375\241zen\241.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Po\237\241ta\237:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Obrazovka:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Kl\240vesnice:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Rozlo\247en\241 kl\240ves:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "P\375ijmout:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "P\375ijmout toto nastaven\241 za\375\241zen\241",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Nastaven\241 hardwaru lze zm\330nit stiskem kl\240vesy ENTER na \375\240dku",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "zvolen\202m \347ipkami nahoru a dol\205. Pot\202 lze zvolit jin\202 nastaven\241.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        " ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Jakmile budou v\347echna nastaven\241 v po\375\240dku, ozna\237te \"P\375ijmout toto",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "nastaven\241 za\375\241zen\241\" a stiskn\330te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   F3 = Ukon\237it",
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

static MUI_ENTRY csCZRepairPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalace ReactOS je v ran\202 v\354vojov\202 f\240zi. Zat\241m nejsou",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "podporov\240ny v\347echny funkce pln\330 pou\247iteln\202 instala\237n\241 aplikace.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Opravn\202 funkce zat\241m nejsou implementov\240ny.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Stisknut\241m U zah\240j\241te Update syst\202mu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Stisknut\241m R spust\241te Konzoli obnoven\241.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Stisknut\241m ESC se vr\240t\241te na hlavn\241 str\240nku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Stisknut\241m kl\240vesy ENTER restartujete po\237\241ta\237.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Hlavn\241 str\240nka  U = Aktualizovat  R = Z\240chrana  ENTER = Restartovat",
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

static MUI_ENTRY csCZUpgradePageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY csCZComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcete zm\330nit typ po\237\241ta\237e, kter\354 bude nainstalov\240n.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Po\247adovan\354 typ po\237\241ta\237e zvolte pomoc\241 \347ipek nahoru a dol\205.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Pot\202 stiskn\330te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stisknut\241m ESC se vr\240t\241te na p\375edchoz\241 str\240nku bez zm\330ny typu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   po\237\241ta\237e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   ESC = Zru\347it   F3 = Ukon\237it",
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

static MUI_ENTRY csCZFlushPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Syst\202m se nyn\241 uji\347\234uje, \247e v\347echna data budou ulo\247ena na disk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Toto m\205\247e trvat n\330kolik minut.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Po dokon\237en\241 bude po\237\241ta\237 automaticky zrestartov\240n.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Uvol\345uji cache",
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

static MUI_ENTRY csCZQuitPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS nen\241 kompletn\330 nainstalov\240n.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Vyjm\330te disketu z jednotky A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "a v\347echny CD-ROM z CD mechanik.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Stisknut\241m kl\240vesy ENTER restartujete po\237\241ta\237.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   \254ekejte, pros\241m...",
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

static MUI_ENTRY csCZDisplayPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcete zm\330nit typ obrazovky, kter\240 bude nainstalov\240na.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Po\247adovan\354 typ obrazovky zvolte pomoc\241 \347ipek nahoru a dol\205.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Pot\202 stiskn\330te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stisknut\241m ESC se vr\240t\241te na p\375edchoz\241 str\240nku bez zm\330ny typu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   obrazovky.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   ESC = Zru\347it   F3 = Ukon\237it",
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

static MUI_ENTRY csCZSuccessPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Z\240kladn\241 sou\237\240sti ReactOS byly \243sp\330\347n\330 nainstalov\240ny.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Vyjm\330te disketu z jednotky A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "a v\347echny CD-ROM z CD mechanik.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Stisknut\241m kl\240vesy ENTER restartujete po\237\241ta\237.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Restartovat po\237\241ta\237",
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

static MUI_ENTRY csCZSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Na n\240sleduj\241c\241m seznamu jsou existuj\241c\241 odd\241ly a nevyu\247it\202",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "m\241sto pro nov\202 odd\241ly.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Vyberte polo\247ku v seznamu pomoc\241 \347ipek nahoru a dol\205.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stisknut\241m kl\240vesy ENTER nainstalujete ReactOS na zvolen\354 odd\241l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Stistknut\241m C vytvo\375\241te prim\240rn\241/logick\354 odd\241l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Stistknut\241m E vytvo\375\241te roz\347\241\375en\354 odd\241l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Stisknut\241m D umo\247n\241te smaz\240n\241 existuj\241c\241ho odd\241lu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Cekejte, prosim...", //MUSI ZUSTAT BEZ DIAKRITIKY!
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

static MUI_ENTRY csCZChangeSystemPartition[] =
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

static MUI_ENTRY csCZConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY csCZFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Form\240tov\240n\241 odd\241lu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Tento odd\241l bude nyn\241 zform\240tov\240n. Stisknut\241m kl\240vesy ENTER za\237nete.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   F3 = Ukon\237it",
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

static MUI_ENTRY csCZCheckFSEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalace nyn\241 kontroluje zvolen\354 odd\241l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\254ekejte, pros\241m...",
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

static MUI_ENTRY csCZInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalace nyn\241 na zvolen\354 odd\241l nakop\241ruje soubory ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Zvolte adres\240\375, kam bude ReactOS nainstalov\240n:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Chcete-li zm\330nit navrhnut\354 adres\240\375, stisknut\241m kl\240vesy BACKSPACE",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "vyma\247te text cesty a pot\202 zapi\347te cestu, do kter\202 chcete ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "nainstalovat.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   F3 = Ukon\237it",
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

static MUI_ENTRY csCZFileCopyEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "\254ekejte, pros\241m, instalace nyn\241 kop\241ruje soubory do zvolen\202ho adres\240\375e.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        " ",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Toto m\205\247e trvat n\330kolik minut.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        49,
        0,
        "\xB3 \254ekejte, pros\241m...    ",
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

static MUI_ENTRY csCZBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Please select where Setup should install the bootloader:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Nainstalovat zavad\330\237 na pevn\354 disk (MBR a VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Nainstalovat zavad\330\237 na pevn\354 disk (pouze VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Nainstalovat zavad\330\237 na disketu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "P\375esko\237it instalaci zavad\330\237e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   F3 = Ukon\237it",
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

static MUI_ENTRY csCZBootLoaderInstallPageEntries[] =
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
        "Instalace nyn\241 nainstaluje zavad\330\237.",
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

static MUI_ENTRY csCZBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalace nyn\241 nainstaluje zavad\330\237.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Vlo\247te naform\240tovanou disketu do jednotky A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "a stiskn\330te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   F3 = Ukon\237it",
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

static MUI_ENTRY csCZKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcete zm\330nit typ kl\240vesnice, kter\240 bude nainstalov\240na.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Po\247adovan\354 typ kl\240vesnice zvolte pomoc\241 \347ipek nahoru a dol\205.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Pot\202 stiskn\330te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stisknut\241m ESC se vr\240t\241te na p\375edchoz\241 str\240nku bez zm\330ny typu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   kl\240vesnice.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   ESC = Zru\347it   F3 = Ukon\237it",
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

static MUI_ENTRY csCZLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Pros\241m zvolte rozlo\247en\241, kter\202 bude implicitn\330 nainstalov\240no.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Po\247adovan\202 rozlo\247en\241 kl\240ves zvolte pomoc\241 \347ipek nahoru a dol\205.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    Pot\202 stiskn\330te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stisknut\241m ESC se vr\240t\241te na p\375edchoz\241 str\240nku bez zm\330ny rozlo\247en\241",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   kl\240ves.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   ESC = Zru\347it   F3 = Ukon\237it",
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

static MUI_ENTRY csCZPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalace p\375iprav\241 po\237\241ta\237 na kop\241rov\240n\241 soubor\205 ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Sestavuji seznam soubor\205 ke zkop\241rov\240n\241...",
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

static MUI_ENTRY csCZSelectFSEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Zvolte souborov\354 syst\202m z n\240sleduj\241c\241ho seznamu.",
        0
    },
    {
        8,
        19,
        "\x07  Souborov\354 syst\202m zvol\241te \347ipkami nahoru a dol\205.",
        0
    },
    {
        8,
        21,
        "\x07  Stisknut\241m kl\240vesy ENTER zform\240tujete odd\241l.",
        0
    },
    {
        8,
        23,
        "\x07  Stisknut\241m ESC se vr\240t\241te na v\354b\330r odd\241lu.",
        0
    },
    {
        0,
        0,
        "ENTER = Pokra\237ovat   ESC = Zru\347it   F3 = Ukon\237it",
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

static MUI_ENTRY csCZDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zvolili jste odstran\330n\241 odd\241lu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Stisknut\241m L odstran\241te odd\241l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "POZOR: V\347echna data na tomto odd\241lu budou ztracena!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Stisknut\241m ESC zru\347\241te akci.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Odstranit odd\241l   ESC = Zru\347it   F3 = Ukon\237it",
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

static MUI_ENTRY csCZRegistryEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalace aktualizuje nastaven\241 syst\202mu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vytv\240\375\241m registry...",
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

MUI_ERROR csCZErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS nen\241 ve va\347em po\237\241ta\237i kompletn\330 nainstalov\240n.\n"
        "Pokud nyn\241 instalaci ukon\237\241te, budete ji muset pro\n"
        "nainstalov\240n\241 ReactOS spustit znovu.\n"
        "\n"
        "  \x07  Stisknut\241m kl\240vesy ENTER budete pokra\237ovat v instalaci.\n"
        "  \x07  Stisknut\241m F3 ukon\237\241te instalaci.",
        "F3 = Ukon\237it  ENTER = Pokra\237ovat"
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
        "Instalace nedok\240zala naj\241t harddisk.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Instalace nedok\240zala naj\241t svou zdrojovou mechaniku.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Nepoda\375ilo se na\237\241st soubor TXTSETUP.SIF.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Soubor TXTSETUP.SIF je po\347kozen.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Soubor TXTSETUP.SIF je neplatn\330 podepsan\354.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Instalace nedok\240zala z\241skat informace o syst\202mov\354ch disc\241ch.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_WRITE_BOOT,
        "Nepoda\375ilo se nainstalovat %S zavad\330\237 na syst\202mov\354 odd\241l.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Nepoda\375ilo se na\237\241st seznam typ\205 po\237\241ta\237e.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Nepoda\375ilo se na\237\241st seznam nastaven\241 obrazovek.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Nepoda\375ilo se na\237\241st seznam typ\205 kl\240vesnic.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Nepoda\375ilo se na\237\241st seznam rozlo\247en\241 kl\240ves.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_WARN_PARTITION,
          "Instalace zjistila, \247e alespo\345 jeden pevn\354 disk obsahuje\n"
          "nekompatibiln\241 tabulku odd\241l\205, kter\240 nem\205\247e b\354t spr\240vn\330 zpracov\240na!\n"
          "\n"
          "Vytv\240\375en\241 nebo odstra\345ov\240n\241 odd\241l\205 m\205\247e tuto tabulku odd\241l\205 zni\237it.\n"
          "\n"
          "  \x07  Stisknut\241m F3 ukon\237\241te instalaci.\n"
          "  \x07  Stisknut\241m ENTER budete pokra\237ovat v instalaci.",
          "F3 = Ukon\237it  ENTER = Pokra\237ovat"
    },
    {
        // ERROR_NEW_PARTITION,
        "Nelze vytvo\375it nov\354 odd\241l uvnit\375 ji\247\n"
        "existuj\241c\241ho odd\241lu!\n"
        "\n"
        "  * Pokra\237ujte stisknut\241m libovoln\202 kl\240vesy.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Nepoda\375ilo se nainstalovat %S zavad\330\237 na syst\202mov\354 odd\241l.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_NO_FLOPPY,
        "V jednotce A: nen\241 disketa.",
        "ENTER = Pokra\237ovat"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Nepoda\375ilo se aktualizovat nastaven\241 rozlo\247en\241 kl\240ves.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Nepoda\375ilo se aktualizovat nastaven\241 zobrazen\241 registru.", //display registry settings
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Nepoda\375ilo se naimportovat soubor registru.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_FIND_REGISTRY
        "Nepoda\375ilo se nal\202zt datov\202 soubory registru.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_CREATE_HIVE,
        "Nepoda\375ilo se zalo\247it registr.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Nepoda\375ilo se inicializovat registry.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "V archivu nen\241 platn\354 soubor inf.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_CABINET_MISSING,
        "Archiv nebyl nalezen.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Archiv neobsahuje instala\237n\241 skript.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_COPY_QUEUE,
        "Nepoda\375ilo se otev\375\241t frontu kop\241rov\240n\241.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_CREATE_DIR,
        "Nepoda\375ilo se vytvo\375it instala\237n\241 adres\240\375e.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Nepoda\375ilo se nal\202zt sekci '%S' v souboru\n"
        "TXTSETUP.SIF.\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_CABINET_SECTION,
        "Nepoda\375ilo se nal\202zt sekci '%S' v archivu.\n"
        "\n",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Nepoda\375ilo se vytvo\375it instala\237n\241 adres\240\375.",
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Nepoda\375ilo se zapsat tabulky odd\241l\205.\n"
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Nepoda\375ilo se p\375idat k\242dovou str\240nku do registru.\n"
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Nepoda\375ilo se nastavit m\241stn\241 nastaven\241.\n"
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Nepoda\375ilo se p\375idat rozlo\247en\241 kl\240vesnice do registru.\n"
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Nepoda\375ilo se nastavit geo id.\n"
        "ENTER = Restartovat po\237\241ta\237"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Neplatn\354 n\240zev adres\240\375e.\n"
        "\n"
        "  * Pokra\237ujte stisknut\241m libovoln\202 kl\240vesy."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Zvolen\354 odd\241l nen\241 pro instalaci ReactOS dostate\237n\330 velk\354.\n"
        "Instala\237n\241 odd\241l mus\241 m\241t velikost alespo\345 %lu MB.\n"
        "\n"
        "  * Pokra\237ujte stisknut\241m libovoln\202 kl\240vesy.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Nepoda\375ilo se vytvo\375it nov\354 prim\240rn\241 nebo roz\347\241\375en\354 odd\241l\n"
        "v tabulce odd\241l\205 na zvolen\202m disku, proto\247e tabulka odd\241l\205 je pln\240.\n"
        "\n"
        "  * Pokra\237ujte stisknut\241m libovoln\202 kl\240vesy."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Nen\241 mo\247n\202 vytvo\375it v\241ce ne\247 jeden roz\347\241\375en\354 odd\241l na disk.\n"
        "\n"
        "  * Pokra\237ujte stisknut\241m libovoln\202 kl\240vesy."
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

MUI_PAGE csCZPages[] =
{
    {
        SETUP_INIT_PAGE,
        csCZSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        csCZLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        csCZWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        csCZIntroPageEntries
    },
    {
        LICENSE_PAGE,
        csCZLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        csCZDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        csCZRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        csCZUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        csCZComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        csCZDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        csCZFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        csCZSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        csCZChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        csCZConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        csCZSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        csCZFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        csCZCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        csCZDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        csCZInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        csCZPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        csCZFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        csCZKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        csCZBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        csCZLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        csCZQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        csCZSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        csCZBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        csCZBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        csCZRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING csCZStrings[] =
{
    {STRING_PLEASEWAIT,
     "   \254ekejte, pros\241m..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalovat   C = Nov\354 prim\240rn\241   E = Nov\354 roz\347\241\375en\354   F3 = Ukon\237it"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalovat   C = Vytvo\375it logick\354 odd\241l   F3 = Ukon\237it"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalovat   D = Odstranit odd\241l   F3 = Ukon\237it"},
    {STRING_DELETEPARTITION,
     "   D = Odstranit odd\241l   F3 = Ukon\237it"},
    {STRING_PARTITIONSIZE,
     "Velikost nov\202ho odd\241lu:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Zvolili jste vytvo\375en\241 nov\202ho prim\240rn\241ho odd\241lu na"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Zvolili jste vytvo\375en\241 nov\202ho roz\347\241\375en\202ho odd\241lu na"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Zvolili jste vytvo\375en\241 nov\202ho logick\202ho odd\241lu na"},
    {STRING_HDPARTSIZE,
    "Zadejte velikost nov\202ho odd\241lu v megabajtech."},
    {STRING_CREATEPARTITION,
     "   ENTER = Vytvo\375it odd\241l   ESC = Zru\347it   F3 = Ukon\237it"},
    {STRING_NEWPARTITION,
    "Instalace vytvo\375ila nov\354 odd\241l na"},
    {STRING_PARTFORMAT,
    "Tento odd\241l bude zform\240tov\240n."},
    {STRING_NONFORMATTEDPART,
    "Zvolili jste instalaci ReactOS na nov\354 nebo nezform\240tovan\354 odd\241l."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Instalace nakop\241ruje ReactOS na odd\241l"},
    {STRING_CONTINUE,
    "ENTER = Pokra\237ovat"},
    {STRING_QUITCONTINUE,
    "F3 = Ukon\237it  ENTER = Pokra\237ovat"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Restartovat po\237\241ta\237"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kop\241ruji soubor: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Instalace kop\241ruje soubory..."},
    {STRING_REGHIVEUPDATE,
    "   Aktualizuji registr..."},
    {STRING_IMPORTFILE,
    "   Importuji %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Aktualizuji nastaven\241 zobrazen\241 registru..."}, //display registry settings
    {STRING_LOCALESETTINGSUPDATE,
    "   Aktualizuji m\241stn\241 nastaven\241..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Aktualizuji nastaven\241 rozlo\247en\241 kl\240ves..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   P\375id\240v\240m informaci o znakov\202 str\240nce..."},
    {STRING_DONE,
    "   Hotovo..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Restartovat po\237\241ta\237"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Nelze otev\375\241t konzoli\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Nejb\330\247n\330j\347\241 p\375\241\237inou je pou\247\241v\240n\241 USB kl\240vesnice\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB kl\240vesnice zat\241m nejsou pln\330 podporov\240ny\r\n"},
    {STRING_FORMATTINGPART,
    "Instalace form\240tuje odd\241l..."},
    {STRING_CHECKINGDISK,
    "Instalace kontroluje disk..."},
    {STRING_FORMATDISK1,
    " Zform\240tovat odd\241l na souborov\354 syst\202m %S (rychle) "},
    {STRING_FORMATDISK2,
    " Zform\240tovat odd\241l na souborov\354 syst\202m %S "},
    {STRING_KEEPFORMAT,
    " Ponechat sou\237asn\354 souborov\354 syst\202m (bez zm\330ny) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "na: %s."},
    {STRING_PARTTYPE,
    "Typ 0x%02x"},
    {STRING_HDDINFO1,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "M\241sto bez odd\241l\205"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Roz\347\241\375en\354 odd\241l"},
    {STRING_UNFORMATTED,
    "Nov\354 (Nenaform\240tovan\354)"},
    {STRING_FORMATUNUSED,
    "Nepou\247it\354"},
    {STRING_FORMATUNKNOWN,
    "Nezn\240m\354"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "P\375id\240v\240m rozlo\247en\241 kl\240ves"},
    {0, 0}
};
