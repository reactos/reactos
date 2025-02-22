// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY daDKSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
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

static MUI_ENTRY daDKLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Valg af sprog",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  V\221lg det sprog du \233nsker der skal bruges under installationen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Og tryk derefter p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Dette vil ogs\206 blive standardsproget p\206 det endelige system.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t  F3 = Afslut",
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

static MUI_ENTRY daDKWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Velkommen til ReactOS installationen",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Denne del af installationen vil kopiere ReactOS opreativsystemet",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "til din computer og klarg\233re den til anden del af installationsprocessen.",
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
        "\x07  Tryk p\206 R reparere ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tryk p\206 L for at f\206 vist ReactOS licensbetingelser og vilk\206r.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tryk p\206 F3 for at afslutte uden at installere ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "For at f\206 mere infomation om ReactOS, g\206 ind p\206:",
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
        "ENTER = Forts\221t  R = Reparer  L = Licens  F3 = Afslut",
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

static MUI_ENTRY daDKIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
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

static MUI_ENTRY daDKLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licensbetingelser:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS systemet er licenseret under betingelserne beskrevet i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL. Noget kode er fra andre kompatible licenser",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "s\206 som X11 eller BSD og GNU GPL licenserne.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Alt software som er en del af ReactOS systemet er",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "derfor udgivet under GNU GPL licensen og den oprindelige",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "licens er ligeledes vedligeholdt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Denne software bliver IKKE leveret med garanti eller begr\221ndsninger af",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "brugen heraf. Dog kun i det udstr\221k international og lokal lov tillader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "ReactOS d\221kker kun distribution til tredjeparter",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Hvis du af en eller anden grund ikke har modtaget en kopi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "af GNU General Public Licensen sammen med ReactOS, bes\233g siden",
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
        "Garanti:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Dette er gratis software; se kildekoen for betingelser for kopiering.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "Der er absolut IGNEN geranti; ikke engang for SALGSBARHEDEN eller",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "om ReactOS KAN BRUGES TIL NOGET BESTEMT.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Tilbage",
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

static MUI_ENTRY daDKDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Listen her under viser de nuv\221rende enhedsindstillinger.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Computer:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Sk\221rm:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Tastatur:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Tastaturlayout:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Accepter:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Accepter disse enhedsindstillinger",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Du kan \221ndre i hardwareindstillingerne ved at trykke p\206 OP",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "eller NED for at v\221lge et element. Derefer tryk p\206 ENTER",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "for at v\221lge andre indstillinger.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "N\206r alle indstillingerne er korrekte, v\221lg",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\"Accepter disse enhedsindstillinger\" og tryk derefter p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   F3 = Afslut",
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

static MUI_ENTRY daDKRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS installationen er i en tilelig udviklingsfase. Derfor",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "underst\233tter den ikke alle funtionerne i et fult brugbart",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "installationsprogram.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Funktionerne til reperation er endnu ikke blevet implementeret.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tryk p\206 U for opdatere OS'et.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tryk p\206 R for at starte gendannelskonsollen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tryk p\206 ESC for at vende tilbage til hovedsk\221rmen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tryk p\206 ENTER for at genstart din computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Hovedsk\221rm  U = Opdater  R = Gendan  ENTER = Genstart",
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

static MUI_ENTRY daDKUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
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

static MUI_ENTRY daDKComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Du vil \221ndre hvilken type computer der skal installeres.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tryk p\206 OP eller NED for at v\221lge den \233nskede type computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Tryk derefter p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryk p\206 ESC for at vende tilbage til det forrige sk\221rmbillede",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   uden at \221ndre computertypen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   ESC = Annuller   F3 = Afslut",
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

static MUI_ENTRY daDKFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Systemet tjekker i \233jeblikket om alt er blevet kopieret til din disk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Dette tager et \233jeblik.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "N\206r det er udf\233rt genstarter din computer automatisk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Fjerner midlertidige filer",
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

static MUI_ENTRY daDKQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS er ikke blevet helt installeret.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Fjern eventuelle disketter fra drev A: og",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "alle CD-ROMmer fra CD-Drev.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tryk p\206 ENTER for at genstarte din computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vent...",
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

static MUI_ENTRY daDKDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Du vil \221ndre hvilken sk\221rm der skal installeres.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tryk p\206 OP eller NED for at v\221lge den \233nskede sk\221rm.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Tryk derefter p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryk p\206 ESC for at vende tilbage til det forrige sk\221rmbillede",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   uden at \221ndre sk\221rmen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   ESC = Annuller   F3 = Afslut",
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

static MUI_ENTRY daDKSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "De grundl\221ggende komponenter i ReactOS blev installeret med success.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Fjern eventuelle disketter fra drev A: og",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "alle CD-ROMmer fra CD-Drev.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tryk p\206 ENTER for at genstarte din computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Genstart",
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

static MUI_ENTRY daDKSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Listen her under viser eksisterende partitioner og ubrugt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "diskplads til nye partitioner.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Tryk p\206 OP eller NED for at v\221lge et element i listen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryk p\206 ENTER for at installere ReactOS til den valgte patition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tryk p\206 C for at lave en ny prim\221r/logisk partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tryk p\206 E for at lave en ny udviddet partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tryk p\206 D for at slette en eksisterende partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vent...",
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

static MUI_ENTRY daDKChangeSystemPartition[] =
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

static MUI_ENTRY daDKConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Du har bedt installationen om at slette systempartitionen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Systempartitioner kan indeholde programmer til diagnosdisering, hardware,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "oprativsystem (s\206 som ReactOS) eller styreprogrammer til hardware.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Slet kun systempartitioner hvis du er sikker p\206 at der ikke er s\206danne",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "progammer p\206 partitionen, hvis du er sikker p\206 at du vil slette dem.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Hvis du sletter partitionen, kan du m\206ske ikke starte din computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "fra din harddisk f\233r at du har gennemf\233rt installatonen af ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Tryk p\206 ENTER for at slette systempartitionen. Du vil blive bedt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   om at bekr\221fte sletningen af partitionen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Tryk p\206 ESC for at vende tilbage til forrige sk\221rmbillede.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   Partitionen vil ikke blive slettet.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t  ESC = Annuller",
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

static MUI_ENTRY daDKFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Formater partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Installationen vil nu formatere partitionen. Tryk p\206 ENTER for at forts\221tte.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Forts\221t   F3 = Afslut",
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

static MUI_ENTRY daDKCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Installationen tjekker den valgte partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vent...",
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

static MUI_ENTRY daDKInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Installationen vil installere filer p\206 den valgte partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "V\221lg den mappe hvor du \233nsker ReactOS skal installeres:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "For at \221ndre den forsl\206ede mappe, tryk p\206 TILABE for at slette",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "det der st\206r og derefter skriv den mappe du \233nsker",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "ReactOS skal installeres til.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   F3 = Afslut",
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

static MUI_ENTRY daDKFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Vent mens ReactOS installationen kopiere filer til din",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "ReactOS-installationsmappe.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Dette kan tage flerere minutter at udf\233re.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Vent...    ",
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

static MUI_ENTRY daDKBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
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
        "Installer opstartsl\221seren p\206 harddisken (MBR og VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Installer opstarsl\221steren p\206 harddisken (kun VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Installer opstartl\221seren p\206 en diskette.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Spring installation af opstartl\221seren over.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   F3 = Afslut",
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

static MUI_ENTRY daDKBootLoaderInstallPageEntries[] =
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
        "Installatione af opstartsl\221ser.",
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

static MUI_ENTRY daDKBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Installatione af opstartsl\221ser.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "S\221t en formateret diskette i drev A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "og tryk p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   F3 = Afslut",
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

static MUI_ENTRY daDKKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Du vil \221ndre hvilken type tastatur der skal installeres.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tryk p\206 OP eller NED for at v\221lge den \233nskede type tastatur.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Tryk derefter p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryk p\206 ESC for at vende tilbage til det forrige sk\221rmbillede",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   uden at \221ndre tastaturtypen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   ESC = Annuller   F3 = Afslut",
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

static MUI_ENTRY daDKLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Du vil \221ndre hvilket tastaturlayout der skal bruges som standard.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tryk p\206 OP eller NED for at v\221lge det \233nskede tastaturlayout.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Tryk derefter p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryk p\206 ESC for at vende tilbage til det forrige sk\221rmbillede",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   uden at \221ndre tastaturlayoutet.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Forts\221t   ESC = Annuller   F3 = Afslut",
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

static MUI_ENTRY daDKPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Installationen g\233r din computer klar til at kopiere ReactOS filerne.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Opbygger listen med filer der skal kopieres...",
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

static MUI_ENTRY daDKSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "V\221lg et filsystem fra listen her under:",
        0
    },
    {
        8,
        19,
        "\x07  Tryk p\206 OP eller NED for at v\221lge et filsystem.",
        0
    },
    {
        8,
        21,
        "\x07  Tryk p\206 ENTER for at formatere partitionen som valgte filsystem.",
        0
    },
    {
        8,
        23,
        "\x07  Tryk p\206 ESC for at v\221lge en anden partition.",
        0
    },
    {
        0,
        0,
        "ENTER = Forts\221t   ESC = Annuller   F3 = Afslut",
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

static MUI_ENTRY daDKDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Du har valgt at slette partitionen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Tryk p\206 L for at slette partitionen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "ADVARSELL Alle data p\206 partitionen vil g\206 tabt!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tryk p\206 ESC for at annulere.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Slet partition   ESC = Annuller   F3 = Afslut",
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

static MUI_ENTRY daDKRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Installationen opdatere systemkonfigurationen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Laver registreringsdatabasen...",
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

MUI_ERROR daDKErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS er endnu ikke f\221rdiginstalleret p\206\n"
        "din computer. Hvis du afslutter installationen nu skal\n"
        "du k\233rer installationen igen for at installere ReactOS.\n"
        "\n"
        "  \x07  Tryk p\206 ENTER for at forts\221tte installationen.\n"
        "  \x07  Tryk p\206 F3 afslutte installationen.",
        "F3 = Afslut  ENTER = Forts\221t"
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
        "Installationen kunne ikke finde en harddisk.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Installationen kunne ikke finde dets kildedrev.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Installationen kunne ikke indl\221st TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Installationen fasdt en \233delagt TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Installationen fandt en ubrugbar signatur i TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Installationen kunne ikke hente information fra registreringsdatabasen.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_WRITE_BOOT,
        "Installationen kunne ikke installere %S-startkode p\206 systempartitionen.",
        "ENTER = Genstart"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Installationen kunne ikke indl\221se listen over computertyper.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Installationen kunne ikke indl\221ste listen over sk\221rmindstillinger.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Installationen kunne ikke indl\221se listen over tastaturtyper.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Installationen kunne ikke indl\221se listen over tastaturlayouts.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_WARN_PARTITION,
        "Installationen fandt ud af at midst en harddisk indeholder en\n"
        "indkompatibel partitionstabel og kan ikke h\206ndteeres korrekt!\n"
        "\n"
        "Oprettelse eller sletning af partitionen kan \233del\221gge partiotnstabellen.\n"
        "\n"
        "  \x07  Tryk p\206 F3 for at afslutte installationen.\n"
        "  \x07  Tryk p\206 ENTER for at forts\221tte.",
        "F3 = Afslut  ENTER = Forts\221t"
    },
    {
        // ERROR_NEW_PARTITION,
        "Du kan ikke lave en partitionstabel\n"
        "inde i en allerede eksisterende partition!\n"
        "\n"
        "  * Tryk p\206 en vilk\206rligtast for at forts\221tte.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Installationen kunne ikke installere %S-startkode p\206 systempartitionen.",
        "ENTER = Genstart"
    },
    {
        // ERROR_NO_FLOPPY,
        "Der er ingen diskette i drev A:.",
        "ENTER = Forts\221t"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Installationen kunne ikke opdatere indstillingene for tastaturlayout.",
        "ENTER = Genstart"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Installationen kunne ikke opdatere indstillingerne for sk\221rmregistrereing",
        "ENTER = Genstart"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Installationen kunne ikke importere registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        // ERROR_FIND_REGISTRY
        "Installationen kunne ikke finde filerne til registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        // ERROR_CREATE_HIVE,
        "Installationen kunne ikke lave registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Installationen kunne ikke indl\221e registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Cabinet-filen indeholder ingen brugbar inf-fil.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cabinet-filen blev ikke fundet.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Cabinet-filen har ingen installationsscript.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_COPY_QUEUE,
        "Installationen kunne ikke \206bne filkopieringsk\233en.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_CREATE_DIR,
        "Installationen kunne ikke lave installationsmapperne.",
        "ENTER = Genstart"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Installationen kunne ikke finde '%S'-sektionen\n"
        "i TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_CABINET_SECTION,
        "Installationen kunne ikke finde '%S'-sektionen\n"
        "i cabinet-filen.\n",
        "ENTER = Genstart"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Installationen kunne ikke lave installationsmappen.",
        "ENTER = Genstart"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Installationen kunne ikke skrive partitionstabllerne.\n"
        "ENTER = Genstart"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Installationen kunne ikke tilf\233je en tegntabel til registreringsdatabasen.\n"
        "ENTER = Genstart"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Installationen kunne ikke s\221tte systemsproget.\n"
        "ENTER = Genstart"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Installationen kunne ikke tilf\233je tastaturlayouts til registrereingsdatabasen.\n"
        "ENTER = Genstart"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Installationen kunne ikke s\221tte geo id'et.\n"
        "ENTER = Genstart"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Ubrugligt mappenavn.\n"
        "\n"
        "  * Tryk p\206 en vilk\206rligtast for at forts\221tte."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Den valgte partition er ikke stor nok til at installere ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "installationspartitionen skal mindst have %lu MB ledig.\n"
        "\n"
        "  * Tryk p\206 en vilk\206rligtast for at forts\221tte.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Du kan ikke lave en ny prim\221r eller udviddet partition i\n"
        "partitionstabellen p\206 denne disk da den er fuld..\n"
        "\n"
        "  * Tryk p\206 en vilk\206rligtast for at forts\221tte."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Du kan ikke lave mere end en udviddet partition per disk.\n"
        "\n"
        "  * Tryk p\206 en vilk\206rligtast for at forts\221tte."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Installationen kan ikke formatere partitionen:\n"
        " %S\n"
        "\n"
        "ENTER = Genstart"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE daDKPages[] =
{
    {
        SETUP_INIT_PAGE,
        daDKSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        daDKLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        daDKWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        daDKIntroPageEntries
    },
    {
        LICENSE_PAGE,
        daDKLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        daDKDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        daDKRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        daDKUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        daDKComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        daDKDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        daDKFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        daDKSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        daDKChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        daDKConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        daDKSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        daDKFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        daDKCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        daDKDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        daDKInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        daDKPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        daDKFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        daDKKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        daDKBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        daDKLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        daDKQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        daDKSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        daDKBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        daDKBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        daDKRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING daDKStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Vent..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = installer   C = Lav prim\221r   E = Lav udviddet   F3 = Afslut"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = installer   C = Lav logisk partition   F3 = Afslut"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = installer   D = Slet partition   F3 = Afslut"},
    {STRING_DELETEPARTITION,
     "   D = Slet partition   F3 = Afslut"},
    {STRING_PARTITIONSIZE,
     "St\233rrelse p\206 den nye partition:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Du har valge at lave en ny prim\221r partition p\206"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Du har valgt at lave en ny udviddet partition p\206"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Du har valge at lave en ny logisk partition p\206"},
    {STRING_HDPARTSIZE,
    "Indtast st\233rrelsen p\206 den nye partition i megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Lav partition   ESC = Annuller   F3 = Afslut"},
    {STRING_NEWPARTITION,
    "Installationen har lavet en ny partition p\206"},
    {STRING_PARTFORMAT,
    "Denne partition vil blive formateret som det n\221ste."},
    {STRING_NONFORMATTEDPART,
    "Du har valgt at installere ReactOS til en ny eller uformateret partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Systempartitionen er endnu ikke blevet formateret."},
    {STRING_NONFORMATTEDOTHERPART,
    "Den nye partition er endnu ikke blevet formateret."},
    {STRING_INSTALLONPART,
    "Installationen installere ReactOS p\206 partitionen"},
    {STRING_CONTINUE,
    "ENTER = Forts\221t"},
    {STRING_QUITCONTINUE,
    "F3 = Afslut  ENTER = Forts\221t"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Genstart"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kopiere filen: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Installationen kopiere filer..."},
    {STRING_REGHIVEUPDATE,
    "   Opdatere registreringsdatabasen..."},
    {STRING_IMPORTFILE,
    "   Importere %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Opdatere indstillinger for registrering af sk\221rm..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Opdatere indstillinger for sprog..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Opdatere indstillinger for tastaturlauout..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Tilf\233jrer tegntabelsinfomation..."},
    {STRING_DONE,
    "   Udf\233rt..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Genstart"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Kunne ikke \206bne konsollen\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Dette skykdes ofte at du bruger et USB-tastatur\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB-tastatuere er endnu ikke fuldt underst\233ttet\r\n"},
    {STRING_FORMATTINGPART,
    "Installationen formaterer partitionen..."},
    {STRING_CHECKINGDISK,
    "Installationen tjekker disken..."},
    {STRING_FORMATDISK1,
    " Formater partitionen som %S-filesystemet (hurtigformatering) "},
    {STRING_FORMATDISK2,
    " Formater partitionen som %S-filesystemet "},
    {STRING_KEEPFORMAT,
    " Behold nuv\221rende filsystem (ingen \221ndringer) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "p\206 %s."},
    {STRING_PARTTYPE,
    "Type 0x%02x"},
    {STRING_HDDINFO1,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) p\206 %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Upartitioneret plads"},
    {STRING_MAXSIZE,
    "MB (maks %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Udviddet partition"},
    {STRING_UNFORMATTED,
    "Ny (uformateret)"},
    {STRING_FORMATUNUSED,
    "Ubrugt"},
    {STRING_FORMATUNKNOWN,
    "Ukendt"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Tilf\233jer tastaturlayouts"},
    {0, 0}
};
