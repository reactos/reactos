/* FILE:        base/setup/usetup/lang/cs-CZ.rc
 * TRANSLATOR:  Radek Liska aka Black_Fox (radekliska at gmail dot com)
 * THANKS TO:   preston for bugfix advice at line 848
 * UPDATED:     2015-04-12
 */

#pragma once

static MUI_ENTRY csCZLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "VìbØr jazyka",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Pros¡m zvolte jazyk, kterì bude bØhem instalace pou§it.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Pot‚ stisknØte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tento jazyk bude vìchoz¡m jazykem v nainstalovan‚m syst‚mu.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat  F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "V¡tejte v instalaci ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Tato Ÿ st instalace nakop¡ruje operaŸn¡ syst‚m ReactOS do vaçeho",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "poŸ¡taŸe a pýiprav¡ druhou Ÿ st instalace.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Stisknut¡m R zah j¡te opravu ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Stiskut¡m L zobraz¡te LicenŸn¡ podm¡nky ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Stisknut¡m F3 ukonŸ¡te instalaci ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "V¡ce informac¡ o ReactOS naleznete na adrese:",
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
        "ENTER = PokraŸovat  R = Opravit  L = Licence  F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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

static MUI_ENTRY csCZLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licence:",
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
        "Z ruka:",
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
        "ENTER = ZpØt",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "N sleduj¡c¡ seznam zobrazuje souŸasn‚ nastaven¡ zaý¡zen¡.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "PoŸ¡taŸ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Obrazovka:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Kl vesnice:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Rozlo§en¡ kl ves:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Pýijmout:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Pýijmout toto nastaven¡ zaý¡zen¡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Nastaven¡ hardwaru lze zmØnit stiskem kl vesy ENTER na ý dku",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "zvolen‚m çipkami nahoru a dol…. Pot‚ lze zvolit jin‚ nastaven¡.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        " ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Jakmile budou vçechna nastaven¡ v poý dku, oznaŸte \"Pýijmout toto",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "nastaven¡ zaý¡zen¡\" a stisknØte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalace ReactOS je v ran‚ vìvojov‚ f zi. Zat¡m nejsou",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "podporov ny vçechny funkce plnØ pou§iteln‚ instalaŸn¡ aplikace.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Opravn‚ funkce zat¡m nejsou implementov ny.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Stisknut¡m U zah j¡te Update syst‚mu.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Stisknut¡m R spust¡te Konzoli obnoven¡.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Stisknut¡m ESC se vr t¡te na hlavn¡ str nku.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Stisknut¡m kl vesy ENTER restartujete poŸ¡taŸ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = Hlavn¡ str nka  U = Aktualizovat  R = Z chrana  ENTER = Restartovat",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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

static MUI_ENTRY csCZComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmØnit typ poŸ¡taŸe, kterì bude nainstalov n.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Po§adovanì typ poŸ¡taŸe zvolte pomoc¡ çipek nahoru a dol….",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Pot‚ stisknØte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Stisknut¡m ESC se vr t¡te na pýedchoz¡ str nku bez zmØny typu",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   poŸ¡taŸe.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   ESC = Zruçit   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Syst‚m se nyn¡ ujiçœuje, §e vçechna data budou ulo§ena na disk.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Toto m…§e trvat nØkolik minut.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Po dokonŸen¡ bude poŸ¡taŸ automaticky zrestartov n.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Uvolåuji cache",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS nen¡ kompletnØ nainstalov n",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "VyjmØte disketu z jednotky A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "a vçechny CD-ROM z CD mechanik.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Stisknut¡m kl vesy ENTER restartujete poŸ¡taŸ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ¬ekejte, pros¡m...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmØnit typ obrazovky, kter  bude nainstalov na.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Po§adovanì typ obrazovky zvolte pomoc¡ çipek nahoru a dol….",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Pot‚ stisknØte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Stisknut¡m ESC se vr t¡te na pýedchoz¡ str nku bez zmØny typu",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   obrazovky.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   ESC = Zruçit   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Z kladn¡ souŸ sti ReactOS byly £spØçnØ nainstalov ny.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "VyjmØte disketu z jednotky A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "a vçechny CD-ROM z CD mechanik.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Stisknut¡m kl vesy ENTER restartujete poŸ¡taŸ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Restartovat poŸ¡taŸ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY csCZBootPageEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "InstalaŸn¡ aplikace nedok §e nainstalovat zavadØŸ na tento",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "disk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Vlo§te naform tovanou disketu do jednotky A:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "a stisknØte ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Na n sleduj¡c¡m seznamu jsou existuj¡c¡ odd¡ly a nevyu§it‚",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "m¡sto pro nov‚ odd¡ly.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Vyberte polo§ku v seznamu pomoc¡ çipek nahoru a dol….",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Stisknut¡m kl vesy ENTER nainstalujete ReactOS na zvolenì odd¡l.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Stistknut¡m P vytvoý¡te prim rn¡ odd¡l.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Stistknut¡m E vytvoý¡te rozç¡ýenì odd¡l.",
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
        "\x07  Stisknut¡m D umo§n¡te smaz n¡ existuj¡c¡ho odd¡lu.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Cekejte, prosim...", //MUSI ZUSTAT BEZ DIAKRITIKY!
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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

static MUI_ENTRY csCZFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Form tov n¡ odd¡lu",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Tento odd¡l bude nyn¡ zform tov n. Stisknut¡m kl vesy ENTER zaŸnete.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY csCZInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalace nyn¡ na zvolenì odd¡l nakop¡ruje soubory ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Zvolte adres ý, kam bude ReactOS nainstalov n:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Chcete-li zmØnit navrhnutì adres ý, stisknut¡m kl vesy BACKSPACE",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "vyma§te text cesty a pot‚ zapiçte cestu, do kter‚ chcete ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "nainstalovat.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "¬ekejte, pros¡m, instalace nyn¡ kop¡ruje soubory do zvolen‚ho adres ýe.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        " ",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Toto m…§e trvat nØkolik minut.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        49,
        0,
        "\xB3 ¬ekejte, pros¡m...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY csCZBootLoaderEntries[] =
{
    {
        4,
        3,
        " Instalace ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalace nyn¡ nainstaluje zavadØŸ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Nainstalovat zavadØŸ na pevnì disk (MBR a VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Nainstalovat zavadØŸ na pevnì disk (pouze VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Nainstalovat zavadØŸ na disketu.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "PýeskoŸit instalaci zavadØŸe.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmØnit typ kl vesnice, kter  bude nainstalov na.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Po§adovanì typ kl vesnice zvolte pomoc¡ çipek nahoru a dol….",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Pot‚ stisknØte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Stisknut¡m ESC se vr t¡te na pýedchoz¡ str nku bez zmØny typu",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   kl vesnice.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   ESC = Zruçit   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Pros¡m zvolte rozlo§en¡, kter‚ bude implicitnØ nainstalov no.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Po§adovan‚ rozlo§en¡ kl ves zvolte pomoc¡ çipek nahoru a dol….",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    Pot‚ stisknØte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Stisknut¡m ESC se vr t¡te na pýedchoz¡ str nku bez zmØny rozlo§en¡",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   kl ves.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   ESC = Zruçit   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalace pýiprav¡ poŸ¡taŸ na kop¡rov n¡ soubor… ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Sestavuji seznam soubor… ke zkop¡rov n¡...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Zvolte souborovì syst‚m z n sleduj¡c¡ho seznamu.",
        0
    },
    {
        8,
        19,
        "\x07  Souborovì syst‚m zvol¡te çipkami nahoru a dol….",
        0
    },
    {
        8,
        21,
        "\x07  Stisknut¡m kl vesy ENTER zform tujete odd¡l.",
        0
    },
    {
        8,
        23,
        "\x07  Stisknut¡m ESC se vr t¡te na vìbØr odd¡lu.",
        0
    },
    {
        0,
        0,
        "ENTER = PokraŸovat   ESC = Zruçit   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Zvolili jste odstranØn¡ odd¡lu",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Stisknut¡m D odstran¡te odd¡l.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "POZOR: Vçechna data na tomto odd¡lu budou ztracena!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Stisknut¡m ESC zruç¡te akci.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Odstranit odd¡l   ESC = Zruçit   F3 = UkonŸit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalace aktualizuje nastaven¡ syst‚mu.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vytv ý¡m registry...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        "ReactOS nen¡ ve vaçem poŸ¡taŸi kompletnØ nainstalov n.\n"
        "Pokud nyn¡ instalaci ukonŸ¡te, budete ji muset pro\n"
        "nainstalov n¡ ReactOS spustit znovu.\n"
        "\n"
        "  \x07  Stisknut¡m kl vesy ENTER budete pokraŸovat v instalaci.\n"
        "  \x07  Stisknut¡m F3 ukonŸ¡te instalaci.",
        "F3 = UkonŸit  ENTER = PokraŸovat"
    },
    {
        // ERROR_NO_HDD
        "Instalace nedok zala naj¡t harddisk.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Instalace nedok zala naj¡t svou zdrojovou mechaniku.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Nepodaýilo se naŸ¡st soubor TXTSETUP.SIF.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Soubor TXTSETUP.SIF je poçkozen.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Soubor TXTSETUP.SIF je neplatnØ podepsanì.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Instalace nedok zala z¡skat informace o syst‚movìch disc¡ch.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_WRITE_BOOT,
        "Nepodaýilo se nainstalovat %S zavadØŸ na syst‚movì odd¡l.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Nepodaýilo se naŸ¡st seznam typ… poŸ¡taŸe.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Nepodaýilo se naŸ¡st seznam nastaven¡ obrazovek.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Nepodaýilo se naŸ¡st seznam typ… kl vesnic.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Nepodaýilo se naŸ¡st seznam rozlo§en¡ kl ves.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_WARN_PARTITION,
          "Instalace zjistila, §e alespoå jeden pevnì disk obsahuje\n"
          "nekompatibiln¡ tabulku odd¡l…, kter  nem…§e bìt spr vnØ zpracov na!\n"
          "\n"
          "Vytv ýen¡ nebo odstraåov n¡ odd¡l… m…§e tuto tabulku odd¡l… zniŸit.\n"
          "\n"
          "  \x07  Stisknut¡m F3 ukonŸ¡te instalaci.\n"
          "  \x07  Stisknut¡m ENTER budete pokraŸovat v instalaci.",
          "F3 = UkonŸit  ENTER = PokraŸovat"
    },
    {
        // ERROR_NEW_PARTITION,
        "Nelze vytvoýit novì odd¡l uvnitý ji§\n"
        "existuj¡c¡ho odd¡lu!\n"
        "\n"
        "  * PokraŸujte stisknut¡m libovoln‚ kl vesy.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "Nelze vymazat m¡sto na disku, kter‚ nepatý¡ § dn‚mu odd¡lu!\n"
        "\n"
        "  * PokraŸujte stisknut¡m libovoln‚ kl vesy.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Nepodaýilo se nainstalovat %S zavadØŸ na syst‚movì odd¡l.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_NO_FLOPPY,
        "V jednotce A: nen¡ disketa.",
        "ENTER = PokraŸovat"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Nepodaýilo se aktualizovat nastaven¡ rozlo§en¡ kl ves.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Nepodaýilo se aktualizovat nastaven¡ zobrazen¡ registru.", //display registry settings
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Nepodaýilo se naimportovat soubor registru.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_FIND_REGISTRY
        "Nepodaýilo se nal‚zt datov‚ soubory registru.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_CREATE_HIVE,
        "Nepodaýilo se zalo§it registr.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Nepodaýilo se inicializovat registry.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "V archivu nen¡ platnì soubor inf.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_CABINET_MISSING,
        "Archiv nebyl nalezen.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Archiv neobsahuje instalaŸn¡ skript.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_COPY_QUEUE,
        "Nepodaýilo se otevý¡t frontu kop¡rov n¡.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_CREATE_DIR,
        "Nepodaýilo se vytvoýit instalaŸn¡ adres ýe.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Nepodaýilo se nal‚zt sekci '%S' v souboru\n"
        "TXTSETUP.SIF.\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_CABINET_SECTION,
        "Nepodaýilo se nal‚zt sekci '%S' v archivu.\n"
        "\n",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Nepodaýilo se vytvoýit instalaŸn¡ adres ý.",
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Nepodaýilo se zapsat tabulky odd¡l….\n"
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Nepodaýilo se pýidat k¢dovou str nku do registru.\n"
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Nepodaýilo se nastavit m¡stn¡ nastaven¡.\n"
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Nepodaýilo se pýidat rozlo§en¡ kl vesnice do registru.\n"
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Nepodaýilo se nastavit geo id.\n"
        "ENTER = Restartovat poŸ¡taŸ"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Neplatnì n zev adres ýe.\n"
        "\n"
        "  * PokraŸujte stisknut¡m libovoln‚ kl vesy."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Zvolenì odd¡l nen¡ pro instalaci ReactOS dostateŸnØ velkì.\n"
        "InstalaŸn¡ odd¡l mus¡ m¡t velikost alespoå %lu MB.\n"
        "\n"
        "  * PokraŸujte stisknut¡m libovoln‚ kl vesy.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Nepodaýilo se vytvoýit novì prim rn¡ nebo rozç¡ýenì odd¡l\n"
        "v tabulce odd¡l… na zvolen‚m disku, proto§e tabulka odd¡l… je pln .\n"
        "\n"
        "  * PokraŸujte stisknut¡m libovoln‚ kl vesy."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Nen¡ mo§n‚ vytvoýit v¡ce ne§ jeden rozç¡ýenì odd¡l na disk.\n"
        "\n"
        "  * PokraŸujte stisknut¡m libovoln‚ kl vesy."
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
        BOOT_LOADER_PAGE,
        csCZBootLoaderEntries
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
        BOOT_LOADER_FLOPPY_PAGE,
        csCZBootPageEntries
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
     "   ¬ekejte, pros¡m..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalovat   P = Novì prim rn¡   E = Novì rozç¡ýenì   F3 = UkonŸit"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalovat   L = Vytvoýit logickì odd¡l   F3 = UkonŸit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalovat   D = Odstranit odd¡l   F3 = UkonŸit"},
    {STRING_DELETEPARTITION,
     "   D = Odstranit odd¡l   F3 = UkonŸit"},
    {STRING_PARTITIONSIZE,
     "Velikost nov‚ho odd¡lu:"},
    {STRING_CHOOSENEWPARTITION,
     "Zvolili jste vytvoýen¡ nov‚ho prim rn¡ho odd¡lu na"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Zvolili jste vytvoýen¡ nov‚ho rozç¡ýen‚ho odd¡lu na"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Zvolili jste vytvoýen¡ nov‚ho logick‚ho odd¡lu na"},
    {STRING_HDDSIZE,
    "Zadejte velikost nov‚ho odd¡lu v megabajtech."},
    {STRING_CREATEPARTITION,
     "   ENTER = Vytvoýit odd¡l   ESC = Zruçit   F3 = UkonŸit"},
    {STRING_PARTFORMAT,
    "Tento odd¡l bude zform tov n."},
    {STRING_NONFORMATTEDPART,
    "Zvolili jste instalaci ReactOS na novì nebo nezform tovanì odd¡l."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Instalace nakop¡ruje ReactOS na odd¡l"},
    {STRING_CHECKINGPART,
    "Instalace nyn¡ kontroluje zvolenì odd¡l."},
    {STRING_CONTINUE,
    "ENTER = PokraŸovat"},
    {STRING_QUITCONTINUE,
    "F3 = UkonŸit  ENTER = PokraŸovat"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Restartovat poŸ¡taŸ"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kop¡ruji soubor: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Instalace kop¡ruje soubory..."},
    {STRING_REGHIVEUPDATE,
    "   Aktualizuji registr..."},
    {STRING_IMPORTFILE,
    "   Importuji %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Aktualizuji nastaven¡ zobrazen¡ registru..."}, //display registry settings
    {STRING_LOCALESETTINGSUPDATE,
    "   Aktualizuji m¡stn¡ nastaven¡..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Aktualizuji nastaven¡ rozlo§en¡ kl ves..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Pýid v m do registru informaci o znakov‚ str nce..."},
    {STRING_DONE,
    "   Hotovo..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Restartovat poŸ¡taŸ"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Nelze otevý¡t konzoli\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "NejbØ§nØjç¡ pý¡Ÿinou je pou§¡v n¡ USB kl vesnice\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB kl vesnice zat¡m nejsou plnØ podporov ny\r\n"},
    {STRING_FORMATTINGDISK,
    "Instalace form tuje disk"},
    {STRING_CHECKINGDISK,
    "Instalace kontroluje disk"},
    {STRING_FORMATDISK1,
    " Zform tovat odd¡l na souborovì syst‚m %S (rychle) "},
    {STRING_FORMATDISK2,
    " Zform tovat odd¡l na souborovì syst‚m %S "},
    {STRING_KEEPFORMAT,
    " Ponechat souŸasnì souborovì syst‚m (bez zmØny) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "na %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "na %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "na harddisku %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTyp %-3u%s                     %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Instalace vytvoýila novì odd¡l na"},
    {STRING_UNPSPACE,
    "    %sM¡sto bez odd¡l…%s               %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Rozç¡ýenì odd¡l"},
    {STRING_UNFORMATTED,
    "Novì (Nenaform tovanì)"},
    {STRING_FORMATUNUSED,
    "Nepou§itì"},
    {STRING_FORMATUNKNOWN,
    "Nezn mì"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Pýid v m rozlo§en¡ kl ves"},
    {0, 0}
};
