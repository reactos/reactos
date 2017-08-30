#pragma once

static MUI_ENTRY daDKLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Valg af sprog",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  V‘lg det sprog du ›nsker der skal bruges under installationen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Og tryk derefter p† ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Dette vil ogs† blive standardsproget p† det endelige system.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t  F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Velkommen til ReactOS installationen",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Denne del af installationen vil kopiere ReactOS opreativsystemet",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "til din computer og klarg›re den til anden del af installationsprocessen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryk p† ENTER for at installere ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press R to repair a ReactOS installation using the Recovery Console.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryk p† L for at f† vist ReactOS licensbetingelser og vilk†r.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryk p† F3 for at afslutte uden at installere ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "For at f† mere infomation om ReactOS, g† ind p†:",
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
        "ENTER = Forts‘t  R = Reparer  L = Licens  F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS installationen er i en tilelig udviklingsfase. Derfor",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "underst›tter den ikke alle funtionerne i et fult brugbart",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "installationsprogram.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Der er f›lgende begr‘ndsninger:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Installationen underst›tter kun FAT-filsystmet.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Tjek af filsystem er endnu ikke implementeret.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Tryk p† ENTER for at installere ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Tryk p† F3 for at afslutte uden at installere ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   F3 = Afslut",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licensbetingelser:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS systemet er licenseret under betingelserne beskrevet i",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL. Noget kode er fra andre kompatible licenser",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "s† som X11 eller BSD og GNU GPL licenserne.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Alt software som er en del af ReactOS systemet er",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "derfor udgivet under GNU GPL licensen og den oprindelige",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "licens er ligeledes vedligeholdt.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Denne software bliver IKKE leveret med garanti eller begr‘ndsninger af",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "brugen heraf. Dog kun i det udstr‘k international og lokal lov tillader.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS d‘kker kun distribution til tredjeparter",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Hvis du af en eller anden grund ikke har modtaget en kopi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "af GNU General Public Licensen sammen med ReactOS, bes›g siden",
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
        "Garanti:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Dette er gratis software; se kildekoen for betingelser for kopiering.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "Der er absolut IGNEN geranti; ikke engang for SALGSBARHEDEN eller",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "om ReactOS KAN BRUGES TIL NOGET BESTEMT.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Tilbage",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Listen her under viser de nuv‘rende enhedsindstillinger.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Computer:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Sk‘rm:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Tastatur:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Tastaturlayout:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Accepter:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Accepter disse enhedsindstillinger",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Du kan ‘ndre i hardwareindstillingerne ved at trykke p† OP",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "eller NED for at v‘lge et element. Derefer tryk p† ENTER",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "for at v‘lge andre indstillinger.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "N†r alle indstillingerne er korrekte, v‘lg",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "\"Accepter disse enhedsindstillinger\" og tryk derefter p† ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        "\x07  Press ESC to continue a new installation without upgrading.",
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

static MUI_ENTRY daDKComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Du vil ‘ndre hvilken type computer der skal installeres.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryk p† OP eller NED for at v‘lge den ›nskede type computer.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryk derefter p† ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryk p† ESC for at vende tilbage til det forrige sk‘rmbillede",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   uden at ‘ndre computertypen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   ESC = Annuller   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Systemet tjekker i ›jeblikket om alt er blevet kopieret til din disk",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Dette tager et ›jeblik",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "N†r det er udf›rt genstarter din computer automatisk.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Fjerner midlertidige filer",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS er ikke blevet helt installeret",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Fjern eventuelle disketter fra drev A: og",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "alle CD-ROMmer fra CD-Drev.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryk p† ENTER for at genstarte din computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vent...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Du vil ‘ndre hvilken sk‘rm der skal installeres.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryk p† OP eller NED for at v‘lge den ›nskede sk‘rm.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryk derefter p† ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryk p† ESC for at vende tilbage til det forrige sk‘rmbillede",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   uden at ‘ndre sk‘rmen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   ESC = Annuller   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "De grundl‘ggende komponenter i ReactOS blev installeret med success.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Fjern eventuelle disketter fra drev A: og",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "alle CD-ROMmer fra CD-Drev.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryk p† ENTER for at genstarte din computer.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryk p† ENTER for at genstarte din comouter.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Genstart",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY daDKBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Installationen kan ikke installere opstartsl‘seren p†",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "din computers hardisk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "S‘t en formateret diskette i drev A: og",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "tryk p† ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Forts‘t   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Listen her under viser eksisterende partitioner og ubrugt",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "diskplads til nye partitioner.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Tryk p† OP eller NED for at v‘lge et element i listen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryk p† ENTER for at installere ReactOS til den valgte patition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryk p† P for at lave en ny prim‘r partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryk p† E for at lave en ny udviddet partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryk p† L for at lave en ny logisk partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryk p† D for at slette en eksisterende partition.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vent...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Du har bedt installationen om at slette systempartitionen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Systempartitioner kan indeholde programmer til diagnosdisering, hardware,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "oprativsystem (s† som ReactOS) eller styreprogrammer til hardware.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Slet kun systempartitioner hvis du er sikker p† at der ikke er s†danne",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "progammer p† partitionen, hvis du er sikker p† at du vil slette dem.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "Hvis du sletter partitionen, kan du m†ske ikke starte din computer",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "fra din harddisk f›r at du har gennemf›rt installatonen af ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Tryk p† ENTER for at slette systempartitionen. Du vil blive bedt",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   om at bekr‘fte sletningen af partitionen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Tryk p† ESC for at vende tilbage til forrige sk‘rmbillede.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   Partitionen vil ikke blive slettet.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t  ESC = Annuller",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formater partition",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Installationen vil nu formatere partitionen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "Tryk p† ENTER for at forts‘tte.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY daDKInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Installationen vil installere filer p† den valgte partition.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "V‘lg den mappe hvor du ›nsker ReactOS skal installeres:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "For at ‘ndre den forsl†ede mappe, tryk p† TILABE for at slette",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "det der st†r og derefter skriv den mappe du ›nsker",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "ReactOS skal installeres til.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Vent mens ReactOS installationen kopiere filer til din",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "ReactOS-installationsmappe.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Dette kan tage flerere minutter at udf›re.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Vent...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY daDKBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " installationen ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Installatione af opstartsl‘ser",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Installer opstartsl‘seren p† harddisken (MBR og VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Installer opstarsl‘steren p† harddisken (kun VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Installer opstartl‘seren p† en diskette.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Spring installation af opstartl‘seren over.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Du vil ‘ndre hvilken type tastatur der skal installeres.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryk p† OP eller NED for at v‘lge den ›nskede type tastatur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryk derefter p† ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryk p† ESC for at vende tilbage til det forrige sk‘rmbillede",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   uden at ‘ndre tastaturtypen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   ESC = Annuller   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Du vil ‘ndre hvilket tastaturlayout der skal bruges som standard.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryk p† OP eller NED for at v‘lge det ›nskede tastaturlayout.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryk derefter p† ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryk p† ESC for at vende tilbage til det forrige sk‘rmbillede",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   uden at ‘ndre tastaturlayoutet.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Forts‘t   ESC = Annuller   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Installationen g›r din computer klar til at kopiere ReactOS filerne. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Opbygger listen med filer der skal kopieres...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "V‘lg et filsystem fra listen her under:",
        0
    },
    {
        8,
        19,
        "\x07  Tryk p† OP eller NED for at v‘lge et filsystem.",
        0
    },
    {
        8,
        21,
        "\x07  Tryk p† ENTER for at formatere partitionen som valgte filsystem.",
        0
    },
    {
        8,
        23,
        "\x07  Tryk p† ESC for at v‘lge en anden partition.",
        0
    },
    {
        0,
        0,
        "ENTER = Forts‘t   ESC = Annuller   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Du har valgt at slette partitionen",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Tryk p† D for at slette partitionen.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ADVARSELL Alle data p† partitionen vil g† tabt!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryk p† ESC for at annulere.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Slet partition   ESC = Annuller   F3 = Afslut",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Installationen opdatere systemkonfigurationen. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Laver registreringsdatabasen...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        //ERROR_NOT_INSTALLED
        "ReactOS er endnu ikke f‘rdiginstalleret p†\n"
        "din computer. Hvis du afslutter installationen nu skal\n"
        "du k›rer installationen igen for at installere ReactOS.\n"
        "\n"
        "  \x07  Tryk p† ENTER for at forts‘tte installationen.\n"
        "  \x07  Tryk p† F3 afslutte installationen.",
        "F3 = Afslut  ENTER = Forts‘t"
    },
    {
        //ERROR_NO_HDD
        "Installationen kunne ikke finde en harddisk.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Installationen kunne ikke finde dets kildedrev.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Installationen kunne ikke indl‘st TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Installationen fasdt en ›delagt TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Installationen fandt en ubrugbar signatur i TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Installationen kunne ikke hente information fra registreringsdatabasen.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_WRITE_BOOT,
        "Installationen kunne ikke installere FAT-startkode p† systempartitionen.",
        "ENTER = Genstart"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Installationen kunne ikke indl‘se listen over computertyper.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Installationen kunne ikke indl‘ste listen over sk‘rmindstillinger.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Installationen kunne ikke indl‘se listen over tastaturtyper.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Installationen kunne ikke indl‘se listen over tastaturlayouts.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_WARN_PARTITION,
        "Installationen fandt ud af at midst en harddisk indeholder en\n"
        "indkompatibel partitionstabel og kan ikke h†ndteeres korrekt!\n"
        "\n"
        "Oprettelse eller sletning af partitionen kan ›del‘gge partiotnstabellen.\n"
        "\n"
        "  \x07  Tryk p† F3 for at afslutte installationen.\n"
        "  \x07  Tryk p† ENTER for at forts‘tte.",
        "F3 = Afslut  ENTER = Forts‘t"
    },
    {
        //ERROR_NEW_PARTITION,
        "Du kan ikke lave en partitionstabel\n"
        "inde i en allerede eksisterende partition!\n"
        "\n"
        "  * Tryk p† en vilk†rligtast for at forts‘tte.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Du kan ikke slette upartitionernet diskplads!\n"
        "\n"
        "  * Tryk p† en vilk†rligtast for at forts‘tte.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Installationen kunne ikke installere FAT-startkode p† systempartitionen.",
        "ENTER = Genstart"
    },
    {
        //ERROR_NO_FLOPPY,
        "Der er ingen diskette i drev A:.",
        "ENTER = Forts‘t"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Installationen kunne ikke opdatere indstillingene for tastaturlayout.",
        "ENTER = Genstart"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Installationen kunne ikke opdatere indstillingerne for sk‘rmregistrereing",
        "ENTER = Genstart"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Installationen kunne ikke importere registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        //ERROR_FIND_REGISTRY
        "Installationen kunne ikke finde filerne til registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        //ERROR_CREATE_HIVE,
        "Installationen kunne ikke lave registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Installationen kunne ikke indl‘e registreringsdatabasen.",
        "ENTER = Genstart"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet-filen indeholder ingen brugbar inf-fil.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet-filen blev ikke fundet.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet-filen har ingen installationsscript.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_COPY_QUEUE,
        "Installationen kunne ikke †bne filkopieringsk›en.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_CREATE_DIR,
        "Installationen kunne ikke lave installationsmapperne.",
        "ENTER = Genstart"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Installationen kunne ikke finde 'Directories'-sektionen\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_CABINET_SECTION,
        "Installationen kunne ikke finde 'Directories'-sektionen\n"
        "i cabinet-filen.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Installationen kunne ikke lave installationsmappen.",
        "ENTER = Genstart"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Installationen kunne ikkde finde 'SetupData'-sektionen\n"
        "i TXTSETUP.SIF.\n",
        "ENTER = Genstart"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Installationen kunne ikke skrive partitionstabllerne.\n"
        "ENTER = Genstart"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Installationen kunne ikke tilf›je en tegntabel til registreringsdatabasen.\n"
        "ENTER = Genstart"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Installationen kunne ikke s‘tte systemsproget.\n"
        "ENTER = Genstart"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Installationen kunne ikke tilf›je tastaturlayouts til registrereingsdatabasen.\n"
        "ENTER = Genstart"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Installationen kunne ikke s‘tte geo id'et.\n"
        "ENTER = Genstart"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Ubrugligt mappenavn.\n"
        "\n"
        "  * Tryk p† en vilk†rligtast for at forts‘tte."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Den valgte partition er ikke stor nok til at installere ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "installationspartitionen skal mindst have %lu MB ledig.\n"
        "\n"
        "  * Tryk p† en vilk†rligtast for at forts‘tte.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Du kan ikke lave en ny prim‘r eller udviddet partition i\n"
        "partitionstabellen p† denne disk da den er fuld..\n"
        "\n"
        "  * Tryk p† en vilk†rligtast for at forts‘tte."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Du kan ikke lave mere end en udviddet partition per disk.\n"
        "\n"
        "  * Tryk p† en vilk†rligtast for at forts‘tte."
    },
    {
        //ERROR_FORMATTING_PARTITION,
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
        BOOT_LOADER_PAGE,
        daDKBootLoaderEntries
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
        BOOT_LOADER_FLOPPY_PAGE,
        daDKBootPageEntries
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
     "   ENTER = installer   P = Lav prim‘r   E = Lav udviddet   F3 = Afslut"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = installer   L = Lav logisk partition   F3 = Afslut"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = installer   D = Slet partition   F3 = Afslut"},
    {STRING_DELETEPARTITION,
     "   D = Slet partition   F3 = Afslut"},
    {STRING_PARTITIONSIZE,
     "St›rrelse p† den nye partition:"},
    {STRING_CHOOSENEWPARTITION,
     "Du har valge at lave en ny prim‘r partition p†"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Du har valgt at lave en ny udviddet partition p†"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Du har valge at lave en ny logisk partition p†"},
    {STRING_HDDSIZE,
    "Indtast st›rrelsen p† den nye partition i megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Lav partition   ESC = Annuller   F3 = Afslut"},
    {STRING_PARTFORMAT,
    "Denne partition vil blive formateret som det n‘ste."},
    {STRING_NONFORMATTEDPART,
    "Du har valgt at installere ReactOS til en ny eller uformateret partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Systempartitionen er endnu ikke blevet formateret."},
    {STRING_NONFORMATTEDOTHERPART,
    "Den nye partition er endnu ikke blevet formateret."},
    {STRING_INSTALLONPART,
    "Installationen installere ReactOS p† partitionen"},
    {STRING_CHECKINGPART,
    "Installationen tjekker den valgte partition."},
    {STRING_CONTINUE,
    "ENTER = Forts‘t"},
    {STRING_QUITCONTINUE,
    "F3 = Afslut  ENTER = Forts‘t"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Genstart"},
    {STRING_TXTSETUPFAILED,
    "Installationen kunne ikke finde '%S'-sektionen\ni TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Kopiere filen: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Installationen kopiere filer..."},
    {STRING_REGHIVEUPDATE,
    "   Opdatere registreringsdatabasen..."},
    {STRING_IMPORTFILE,
    "   Importere %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Opdatere indstillinger for registrering af sk‘rm..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Opdatere indstillinger for sprog..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Opdatere indstillinger for tastaturlauout..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Tilf›jrer tegntabelsinfomation til registrerinsdatabasen..."},
    {STRING_DONE,
    "   Udf›rt..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Genstart"},
    {STRING_CONSOLEFAIL1,
    "Kunne ikke †bne konsollen\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Dette skykdes ofte at du bruger et USB-tastatur\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB-tastatuere er endnu ikke fuldt underst›ttet\r\n"},
    {STRING_FORMATTINGDISK,
    "Installationen formatere din disk"},
    {STRING_CHECKINGDISK,
    "Installationen tjekker din disk"},
    {STRING_FORMATDISK1,
    " Formater partitionen som %S-filesystemet (hurtigformatering) "},
    {STRING_FORMATDISK2,
    " Formater partitionen som %S-filesystemet "},
    {STRING_KEEPFORMAT,
    " Behold nuv‘rende filsystem (ingen ‘ndringer) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) p† %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "p† %I64u %s  harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) p† %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "p† %I64u %s  harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "p† harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %stype %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) p† %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Installationen har lavet en ny partition p†"},
    {STRING_UNPSPACE,
    "    %sUpartitioneret plads%s           %6lu %s"},
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
    "Tilf›jer tastaturlayouts"},
    {0, 0}
};