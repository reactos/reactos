/*
    De gebruikte codepagina is 850.
    stip    = \x07
    e-aigu  = \x82
    e-trema = \x89
    i-trema = \x8B
*/

#pragma once

static MUI_ENTRY nlNLSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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

static MUI_ENTRY nlNLLanguagePageEntries[] =
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
        "Taalkeuze",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Kies de taal die u voor de installatie wilt gebruiken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Druk daarna op ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Deze taal wordt later als standaard taal door het systeem gebruikt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   F3 = Afsluiten",
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

static MUI_ENTRY nlNLWelcomePageEntries[] =
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
        "Welkom bij ReactOS Setup",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Dit deel van de installatie kopieert ReactOS naar uw computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "en bereidt het tweede deel voor.",
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
        "\x07  Druk op R om ReactOS te repareren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Druk op L om de ReactOS licentieovereenkomst te bekijken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Druk op F3 als u Setup wilt afsluiten zonder ReactOS te",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "   installeren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "Voor meer informatie over ReactOS bezoekt u:",
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
        "ENTER = Doorgaan   R = Herstellen   L = Licentie   F3 = Afsluiten",
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

static MUI_ENTRY nlNLIntroPageEntries[] =
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
        "ENTER = Doorgaan   F3 = Afsluiten",
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

static MUI_ENTRY nlNLLicensePageEntries[] =
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
        6,
        "Licentie:",
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
        "ENTER = Ga terug",
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

static MUI_ENTRY nlNLDevicePageEntries[] =
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
        "Onderstaande lijst bevat de huidige apparaatinstellingen.",
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
        "Beeldscherm:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Toetsenbord:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Toetsenbord indeling:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Accepteren:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Accepteren van apparaatinstellingen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Selecteer een apparaatinstelling door middel van de pijltjestoetsen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "Druk daarna op de ENTER toets om de instelling aan te passen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Als alle instellingen juist zijn, selecteer ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\"Accepteren van apparaatinstellingen\" en druk op ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   F3 = Afsluiten",
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

static MUI_ENTRY nlNLRepairPageEntries[] =
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
        "ReactOS Setup is nog in een vroege ontwikkelingsfase",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Het ondersteunt nog niet alle functies van een volledig",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "setup programma.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "De herstelfuncties zijn nog niet ge\x8Bmplementeerd.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Druk op U om ReactOS bij te werken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Druk op R voor de Herstelconsole.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Druk op ESC om terug naar het hoofdscherm te gaan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Druk op ENTER om de computer opnieuw op te starten.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Hoofdscherm   U = Bijwerken   R = Herstelconsole   ENTER = Reboot",
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

static MUI_ENTRY nlNLUpgradePageEntries[] =
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

static MUI_ENTRY nlNLComputerPageEntries[] =
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
        "U wilt het te installeren computertype instellen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Selecteer het gewenste computertype met behulp van de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   pijltjestoetsen en druk daarna op ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Druk op de ESC toets om naar het vorige scherm te gaan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   zonder het computertype te wijzigen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   ESC = Annuleren   F3 = Afsluiten",
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

static MUI_ENTRY nlNLFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Het systeem is alle gegegevens naar de schijf aan het wegschrijven.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Dit kan enige minuten duren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Wanneer gereed, zal uw computer automatisch opnieuw opstarten.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Cache wordt geleegd.",
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

static MUI_ENTRY nlNLQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS is niet volledig ge\x8Bnstalleerd.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Verwijder de diskette uit station A: en",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "alle cd-rom's uit de cd-stations.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Druk op ENTER om uw computer opnieuw op te starten.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Een ogenblik geduld...",
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

static MUI_ENTRY nlNLDisplayPageEntries[] =
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
        "U wilt het te installeren beeldschermtype instellen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Selecteer het gewenste beeldschermtype met behulp van de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   pijltjestoetsen en druk daarna op ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Druk op de ESC toets om naar het vorige scherm te gaan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   zonder het beeldschermtype te wijzigen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   ESC = Annuleren   F3 = Afsluiten",
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

static MUI_ENTRY nlNLSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "De basisonderdelen van ReactOS zijn succesvol ge\x8Bnstalleerd.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Verwijder de diskette uit station A: en",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "alle cd-rom's uit de cd-stations.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Druk op ENTER om uw computer opnieuw op te starten.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Opnieuw opstarten",
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

static MUI_ENTRY nlNLSelectPartitionEntries[] =
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
        "Onderstaande lijst bevat de huidige partities en ongebruikte",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "ruimte voor nieuwe partities.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Selecteer een partitie door middel van de pijltjestoetsen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Druk op ENTER om ReactOS te installeren op de geselecteerde partitie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Druk op C om een primaire/logische partitie aan te maken.",
//        "\x07  Druk op C om een nieuwe partitie aan te maken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Druk op E om een uitgebreide partitie aan te maken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Druk op D om een bestaande partitie te verwijderen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Een ogenblik geduld...",
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

static MUI_ENTRY nlNLChangeSystemPartition[] =
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

static MUI_ENTRY nlNLConfirmDeleteSystemPartitionEntries[] =
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

static MUI_ENTRY nlNLFormatPartitionEntries[] =
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
        "Partitie formatteren",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Setup gaat nu de partitie formatteren. Druk op ENTER om door te gaan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Doorgaan   F3 = Afsluiten",
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

static MUI_ENTRY nlNLCheckFSEntries[] =
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
        "Setup controleert nu de geselecteerde partitie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Een ogenblik geduld...",
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

static MUI_ENTRY nlNLInstallDirectoryEntries[] =
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
        "Setup installeert ReactOS bestanden op de geselecteerde partitie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Geef een pad op waar ReactOS ge\x8Bnstalleerd moet worden:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Om het voorgestelde pad te wijzigen: druk op BACKSPACE om",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "karakters te verwijderen en voer dan in waar u ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "wilt installeren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   F3 = Afsluiten",
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

static MUI_ENTRY nlNLFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Bezig met het kopi\x89ren van bestanden naar uw ReactOS installatiemap.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "Even geduld alstublieft.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Dit kan enkele minuten duren.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Een ogenblik geduld...",
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

static MUI_ENTRY nlNLBootLoaderSelectPageEntries[] =
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
        "Please select where Setup should install the bootloader:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Op vaste schijf installeren (MBR en VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Op vaste schijf installeren (enkel VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Op een diskette installeren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Installeren bootloader overslaan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   F3 = Afsluiten",
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

static MUI_ENTRY nlNLBootLoaderInstallPageEntries[] =
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
        "Setup installeert de bootloader.",
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

static MUI_ENTRY nlNLBootLoaderRemovableDiskPageEntries[] =
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
        "Setup installeert de bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Voer een geformatteerde diskette in station A: in",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "en druk op ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   F3 = Afsluiten",
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

static MUI_ENTRY nlNLkeyboardSettingsEntries[] =
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
        "U wilt het te installeren toetsenbordtype instellen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Selecteer het gewenste toetsenbordtype met behulp van de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   pijltjestoetsen en druk daarna op ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Druk op de ESC toets om naar het vorige scherm te gaan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   zonder het toetsenbordtype te wijzigen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   ESC = Annuleren   F3 = Afsluiten",
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

static MUI_ENTRY nlNLLayoutSettingsEntries[] =
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
        "U wilt de te installeren standaard indeling instellen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Selecteer de gewenste standaard indeling met behulp van de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   pijltjestoetsen en druk daarna op ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Druk op de ESC toets om naar het vorige scherm te gaan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   zonder de standaard indeling te wijzigen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Doorgaan   ESC = Annuleren   F3 = Afsluiten",
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

static MUI_ENTRY nlNLPrepareCopyEntries[] =
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
        "Setup bereidt uw computer voor op het kopi\x89ren van ReactOS bestanden.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Aanmaken van de lijst van te kopi\x89ren bestanden...",
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

static MUI_ENTRY nlNLSelectFSEntries[] =
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
        17,
        "Selecteer een bestandssysteem uit onderstaande lijst.",
        0
    },
    {
        8,
        19,
        "\x07  Gebruik de pijltjestoetsen om een bestandssysteem te selecteren.",
        0
    },
    {
        8,
        21,
        "\x07  Druk op ENTER om de partitie te formatteren.",
        0
    },
    {
        8,
        23,
        "\x07  Druk op ESC om een andere partitie te selecteren.",
        0
    },
    {
        0,
        0,
        "ENTER = Doorgaan   ESC = Annuleren   F3 = Afsluiten",
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

static MUI_ENTRY nlNLDeletePartitionEntries[] =
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
        "U wilt de partitie verwijderen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Druk op L om de partitie te verwijderen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "WAARSCHUWING: Alle gegevens op deze partitie worden gewist!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Druk op ESC om te annuleren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Verwijder partitie   ESC = Annuleren   F3 = Afsluiten",
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

static MUI_ENTRY nlNLRegistryEntries[] =
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
        "Setup is de systeemconfiguratie aan het bijwerken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        /*
            Opmerking: de bestanden heten in het Engels hives omdat \x82\x82n van de
            NT ontwikkelaars een hekel had aan bijen. Ja echt. Dus in het
            Nederlands zouden ze korven moeten heten, maar ik zie de meerwaarde
            boven het woord bestand eerlijk gezegd niet.
        */
        "Registerbestanden aanmaken...",
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

MUI_ERROR nlNLErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS is niet geheel ge\x8Bnstalleerd op uw\n"
        "computer. Als u Setup nu afsluit moet u\n"
        "Setup opnieuw starten om ReactOS te installeren.\n"
        "\n"
        "  \x07  Druk op ENTER om door te gaan met Setup.\n"
        "  \x07  Druk op F3 om Setup af te sluiten.",
        "F3 = Afsluiten   ENTER = Doorgaan"
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
        "Setup kan geen vaste schijf vinden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Setup kan zijn station met bronbestanden niet vinden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Setup kan het bestand TXTSETUP.SIF niet laden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Setup heeft een ongeldig TXTSETUP.SIF gevonden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup heeft een ongeldige signatuur in TXTSETUP.SIF gevonden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Setup kan de stationsinformatie niet laden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_WRITE_BOOT,
        "Setup kan de %S bootcode op de systeempartitie niet installeren.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Setup kan de computertype-lijst niet laden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Setup kan de beeldscherminstellingen-lijst niet laden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Setup kan de toetsenbordtype-lijst niet laden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Setup kan de toetsenbordindeling-lijst niet laden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_WARN_PARTITION,
          "Setup ontdekte dat ten minste \x82\x82n vaste schijf een niet compatibele\n"
          "partitietabel bevat dat niet goed wordt ondersteund!\n"
          "\n"
          "Aanmaken of verwijderen van partities kan de partitietabel\n"
          "vernietigen.\n"
          "\n"
          "  \x07  Druk op F3 om Setup af te sluiten.\n"
          "  \x07  Druk op ENTER om door te gaan.",
          "F3 = Afsluiten   ENTER = Doorgaan"
    },
    {
        // ERROR_NEW_PARTITION,
        "U kunt geen nieuwe partitie aanmaken binnen\n"
        "een reeds bestaande partitie!\n"
        "\n"
        "  * Druk op een willekeurige toets om door te gaan.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Setup kan de %S bootcode op de systeempartitie niet installeren.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_NO_FLOPPY,
        "Geen diskette in station A:.",
        "ENTER = Doorgaan"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Setup kan de toetsenbordindeling instellingen niet bijwerken.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup kan de beeldscherminstellingen niet bijwerken.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Setup kan een registerbestand niet importeren.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_FIND_REGISTRY
        /*
            This is about the hives, no?
        */
        "Setup kan de registerbestanden niet vinden.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_CREATE_HIVE,
        "Setup kan de registerbestanden niet aanmaken.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Setup kan het register niet initialiseren.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Cabinet bevat geen geldig .inf bestand.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cabinet bestand niet gevonden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Cabinet bestand heeft geen setup script.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_COPY_QUEUE,
        "Setup kan de lijst met te kopi\x89ren bestanden niet vinden.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_CREATE_DIR,
        "Setup kan de installatiemappen niet aanmaken.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Setup kan de '%S' sectie niet vinden\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_CABINET_SECTION,
        "Setup kan de '%S' sectie niet vinden\n"
        "in het cabinet bestand.\n",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Setup kan de installatiemap niet aanmaken.",
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Setup kan de partitietabellen niet schrijven.\n"
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Setup kan de codepagina niet toevoegen aan het register.\n"
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup kan de systeemtaal niet instellen.\n"
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Setup kan de toetsenbordindelingen niet toevoegen aan het register.\n"
        "ENTER = Computer opnieuw opstarten"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Setup kan de geografische positie niet instellen.\n"
        "ENTER = Computer opnieuw opstarten"
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
        "  * Druk op een toets om door te gaan.",
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

MUI_PAGE nlNLPages[] =
{
    {
        SETUP_INIT_PAGE,
        nlNLSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        nlNLLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        nlNLWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        nlNLIntroPageEntries
    },
    {
        LICENSE_PAGE,
        nlNLLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        nlNLDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        nlNLRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        nlNLUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        nlNLComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        nlNLDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        nlNLFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        nlNLSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        nlNLChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        nlNLConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        nlNLSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        nlNLFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        nlNLCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        nlNLDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        nlNLInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        nlNLPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        nlNLFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        nlNLkeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        nlNLBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        nlNLLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        nlNLQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        nlNLSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        nlNLBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        nlNLBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        nlNLRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING nlNLStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Een ogenblik geduld..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Installeren   C = Primaire aanmaken   E = Uitgebreide aanmaken   F3 = Afsluiten"},
//     "   ENTER = Installeren   C = Partitie aanmaken   F3 = Afsluiten"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Installeren   C = Logische Partitie aanmaken   F3 = Afsluiten"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Installeren   D = Partitie verwijderen   F3 = Afsluiten"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Afsluiten"},
    {STRING_PARTITIONSIZE,
     "Grootte nieuwe partitie:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "You have chosen to create a primary partition on"},
//     "U wilt een nieuwe partitie aanmaken op"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDPARTSIZE,
    "Voert u de grootte van de nieuwe partitie in in megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Partitie Aanmaken   ESC = Annuleren   F3 = Afsluiten"},
    {STRING_NEWPARTITION,
    "Setup heeft een nieuwe partitie aangemaakt op"},
    {STRING_PARTFORMAT,
    "Deze partitie zal vervolgens geformatteerd worden."},
    {STRING_NONFORMATTEDPART,
    "U wilt ReactOS installeren op een nieuwe of ongeformatteerde partitie."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Setup installeert ReactOS op Partitie"},
    {STRING_CONTINUE,
    "ENTER = Doorgaan"},
    {STRING_QUITCONTINUE,
    "F3 = Afsluiten   ENTER = Doorgaan"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Computer opnieuw opstarten"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kopi\x89ren bestand: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup is bestand aan het kopi\x89ren..."},
    {STRING_REGHIVEUPDATE,
    "   Bijwerken registerbestanden..."},
    {STRING_IMPORTFILE,
    "   Importeren %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Bijwerken beeldscherminstellingen..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Bijwerken systeemtaalinstellingen..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Bijwerken toetsenbordindeling..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Toevoegen informatie codepagina..."},
    {STRING_DONE,
    "   Voltooid..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Computer opnieuw opstarten"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Kan console niet openen.\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "De meest voorkomende oorzaak is het gebruik van een USB toetsenbord.\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB toetsenborden worden nog niet volledig ondersteund.\r\n"},
    {STRING_FORMATTINGPART,
    "Setup is de partitie aan het formatteren..."},
    {STRING_CHECKINGDISK,
    "Setup is de vaste schijf aan het controleren..."},
    {STRING_FORMATDISK1,
    " Formatteer partitie als %S bestandssysteem (snel) "},
    {STRING_FORMATDISK2,
    " Formatteer partitie als %S bestandssysteem "},
    {STRING_KEEPFORMAT,
    " Behoud huidig bestandssysteem (geen wijzigingen) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "op %s."},
    {STRING_PARTTYPE,
    "Type 0x%02x"},
    {STRING_HDDINFO1,
    // "Schijf %lu (%I64u %s), Poort=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Schijf %lu (Poort=%hu, Bus=%hu, Id=%hu) op %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Schijf %lu (%I64u %s), Poort=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Schijf %lu (Poort=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Niet gepartitioneerde ruimte"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Nieuw (Ongeformatteerd)"},
    {STRING_FORMATUNUSED,
    "Ongebruikt"},
    {STRING_FORMATUNKNOWN,
    "Onbekend"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Toevoegen toetsenbordindelingen"},
    {0, 0}
};
