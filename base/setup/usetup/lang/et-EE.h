#pragma once

static MUI_ENTRY etEELanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Keele valik",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Vali keel, mida paigaldamisel kasutada.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ja vajuta ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Seda keelt kasutatakse hiljem sÅsteemi keelena.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka  F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Tere tulemast ReactOSi paigaldama",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Selles paigaldamise osas kopeeritakse ReactOSi failid arvutisse ja",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "valmistatakse ette paigaldamise teine jÑrk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Vajuta ENTER, et ReactOS paigaldada.",
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
        "\x07  Vajuta L, et nÑha ReactOSi litsentsi ja kasutamise tingimusi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Vajuta F3, et vÑljuda ReactOSi paigaldamata.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "ReactOSi kohta saab rohkem infot:",
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
        "ENTER = JÑtka  R = Paranda  L = Litsents  F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOSi paigaldusprogramm on varajases arendusfaasis. Praegu ei ole",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "veel k‰ik korraliku paigaldusprogrammi funktsioonid toetatud.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Kehtivad jÑrgmised piirangud:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Toetatud on ainult FAT failisÅsteem.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- FailisÅsteemi kontrollimist veel ei tehta.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Vajuta ENTER, et ReactOS paigaldada.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Vajuta F3, et vÑljuda ReactOSi paigaldamata.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   F3 = VÑlju",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEELicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Litsents:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS kasutab GNU Åldist avalikku litsentsi(GPL),",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "m‰ned komponendid kasutavad muid Åhilduvaid litsentse,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "nagu nÑiteks X11, BSD ja GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Kogu ReactOSi sÅsteem on seega kaitstud GPL litsentsiga",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "ning samas kehtivad ka algsed litsentsid.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "KÑesoleva tarkvaraga ei anta kaasa garantiid ega mÑÑrata kasutamise",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "piiranguid kehtiva seadusega sÑtestatud piirides. ReactOSi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "litsents mÑÑrab ainult levitamise kolmandatele osapooltele.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Kui mingil p‰hjusel ei olnud tarkvaraga kaasas GNU GPL",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "Åldist avalikku litsentsi, siis saab seda vaadata lehel",
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
        "Garantii:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Tegemist on vaba tarkvaraga; kopeerimise tingimused on kirjas algkoodis.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "Garantii puudub; pole isegi turustamiseks v‰i mingil",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "kindlal eesmÑrgil kasutamiseks sobivuse garantiid",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Tagasi",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "JÑrgnev nimekiri nÑitab riistvara seadeid.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Arvuti:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Monitor:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Klaviatuur:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Klaviatuuri asetus:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Rakenda:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Rakenda need seaded",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Riistvara seadeid saab muuta Åles ja alla liikudes.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "Seadistuse muutmiseks vajuta ENTER.",
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
        "Kui seadistus on paigas, vali \"Rakenda need seaded\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "ja vajuta ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
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

static MUI_ENTRY etEEComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Arvuti tÅÅbi muutmine.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Liigu Åles-alla, et valida sobiv arvuti tÅÅp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   SeejÑrel vajuta ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Vajuta ESC, et minna tagasi eelmisele lehele",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ilma arvuti tÅÅpi muutmata.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   ESC = Katkesta   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "SÅsteem kirjutab nÅÅd andmed kettale",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "V‰ib kuluda veidi aega",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "L‰petamisel taaskÑivitub arvuti automaatselt",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "VahemÑlu tÅhjendamine",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS ei ole tÑielikult paigaldatud",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Eemalda flopiketas ja CD-ROMid draividest.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Vajuta ENTER, et arvuti taaskÑivitada.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Palun oota...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Monitori tÅÅbi muutmine.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Liigu Åles-alla, et monitori tÅÅpi muuta.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   SeejÑrel vajuta ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Vajuta ESC, et minna tagasi eelmisele lehele",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ilma monitori tÅÅpi muutmata.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   ESC = Katkesta   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEESuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOSi p‰hilised komponendid on edukalt paigaldatud.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Eemalda flopiketas ja CD-ROMid draividest.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Vajuta ENTER, et arvuti taaskÑivitada.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = TaaskÑivita arvuti",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Alglaadurit ei saanud kettale kirjutada.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Sisesta vormindatud flopiketas draivi A:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "ja vajuta ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = JÑtka   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY etEESelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "See nimekiri nÑitab partitsioone ja vaba ruumi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "uute partitsioonide jaoks.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Liigu Åles-alla, et valida kirje.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Vajuta ENTER, et paigaldada ReactOS valitud partitsioonile.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  Vajuta C, et teha uus partitsioon.",
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
        "\x07  Vajuta D, et kustutada olemasolev partitsioon.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Palun oota...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
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

static MUI_ENTRY etEEFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Partitsiooni vormindamine",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "NÅÅd vormindatakse partitsioon. Vajuta ENTER, et jÑtkata.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY etEEInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS paigaldatakse valitud partitsioonile.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Vali kaust, kuhu ReactOS paigaldada:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Kausta muutmiseks kustuta kirje BACKSPACE klahviga ja",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "trÅki asemele kaust, kuhu ReactOS installeerida.",
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
        "ENTER = JÑtka   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Palun oota, kuni ReactOS paigaldatakse sihtkausta.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "V‰ib kuluda mitu minutit.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Palun oota...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Alglaaduri paigaldamine",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Paigalda alglaadur k‰vakettale (MBR ja VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Paigalda alglaadur k‰vakettale (ainult VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Paigalda alglaadur flopikettale.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "éra paigalda alglaadurit.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Klaviatuuri tÅÅbi muutmine.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Vajuta Åles-alla, et valida klaviatuuri tÅÅp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   SeejÑrel vajuta ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Vajuta ESC, et minna tagasi eelmisele lehele",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   klaviatuuri tÅÅpi muutmata.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   ESC = Katkesta   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEELayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vali vaikimisi klaviatuuriasetus.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Liigu Åles-alla, et valida klaviatuuriasetus.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    SeejÑrel vajuta ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Vajuta ESC, et minna tagasi eelmisele lehele",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   klaviatuuriasetust muutmata.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = JÑtka   ESC = Katkesta   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY etEEPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Arvutit valmistatakse ette ReactOSi failide kopeerimiseks.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Failide nimekirja loomine...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY etEESelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Vali nimekirjast failisÅsteem.",
        0
    },
    {
        8,
        19,
        "\x07  Liigu Åles-alla, et valida failisÅsteem.",
        0
    },
    {
        8,
        21,
        "\x07  Vajuta ENTER, et partitsioon vormindada.",
        0
    },
    {
        8,
        23,
        "\x07  Vajuta ESC, et valida muu partitsioon.",
        0
    },
    {
        0,
        0,
        "ENTER = JÑtka   ESC = Katkesta   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEEDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Oled valinud partitsiooni kustutamise",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Vajuta D, et partitsioon kustutada.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "HOIATUS: K‰ik andmed partitsioonil kustutatakse!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Vajuta ESC, et katkestada.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Kustuta partitsioon   ESC = Katkesta   F3 = VÑlju",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY etEERegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldamine ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Uuendatakse sÅsteemi seadistust.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Registri v‰tmete loomine...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR etEEErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS ei ole tÑielikult paigaldatud.\n"
        "Kui paigaldamine praegu katkestada, siis tuleb\n"
        "ReactOSi paigaldamiseks paigaldusprogramm uuesti kÑivitada.\n"
        "\n"
        "  \x07  Vajuta ENTER, et paigaldamist jÑtkata.\n"
        "  \x07  Vajuta F3, et paigaldamie peatada.",
        "F3 = VÑlju  ENTER = JÑtka"
    },
    {
        //ERROR_NO_HDD
        "K‰vaketast ei leitud.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Paigaldusprogramm ei leidnud ketast, millelt see kÑivitati.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "TXTSETUP.SIF faili ei ‰nnestunud laadida.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "TXTSETUP.SIF on vigane.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "TXTSETUP.SIF faili signatuur on vigane.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "SÅsteemiketta parameetreid ei ‰nnestunud lugeda.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_WRITE_BOOT,
        "SÅsteemikettale ei ‰nnestunud kirjutada FAT alglaadimiskoodi.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "ArvutitÅÅpide nimekirja ei ‰nnestunud laadida.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Monitoride nimekirja ei ‰nnestunud laadida.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Klaviatuuri tÅÅpide nimekirja ei ‰nnestunud laadida.\n",
         "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Klaviatuuriasetuste nimekirja ei ‰nnestunud laadida.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_WARN_PARTITION,
          "Leiti vÑhemalt Åks k‰vaketas, millel on Åhildamatu partitsioonitabel,\n"
          "millega ei saanud korralikult Åmber kÑia!\n"
          "\n"
          "Partitsioonide loomine v‰i kustutamine v‰ib vigastada partitsioonitabelit.\n"
          "\n"
          "  \x07  Vajuta F3, et vÑljuda paigaldusest..\n"
          "  \x07  Vajuta ENTER, et jÑtkata.",
          "F3 = VÑlju  ENTER = JÑtka"
    },
    {
        //ERROR_NEW_PARTITION,
        "Uut partitsioonitabelit ei saa juba olemasoleva\n"
        "partitsiooni sisse tekitada!\n"
        "\n"
        "  * Vajuta suvalist klahvi, et jÑtkata.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Partitsioneerimata kettaruumi ei saa kustutada!\n"
        "\n"
        "  * Vajuta suvalist klahvi, et jÑtkata.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "SÅsteemikettale ei ‰nnestunud paigaldada FAT alglaadimiskoodi.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_NO_FLOPPY,
        "Draivis A: ei ole flopiketast.",
        "ENTER = JÑtka"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Klaviatuuriasetuse seadistust ei ‰nnestunud uuendada.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Monitori seadistust registris ei ‰nnestunud uuendada.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Tarufaili ei ‰nnestunud importida.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_FIND_REGISTRY
        "Registri andmete faile ei leitud.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_CREATE_HIVE,
        "Registri tarusid ei ‰nnestunud luua.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Registrit ei ‰nnestunud luua.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Kapifailis ei olnud pÑdevaid inf faile.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_CABINET_MISSING,
        "Kapifaili ei leitud.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Kapifailis puudub paigaldusskript.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_COPY_QUEUE,
        "Kopeeritavate failide nimekirja ei ‰nnestunud avada.\n",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_CREATE_DIR,
        "Paigalduskaustu ei ‰nnestunud luua.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "TXTSETUP.SIF failist ei leitud 'Directories' sektsiooni.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_CABINET_SECTION,
        "Kapifailist ei leitud 'Directories' sektsiooni.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Paigalduskausta ei ‰nnestunud luua.",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "TXTSETUP.SIF failist ei leitud 'SetupData' sektsiooni",
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Partitsioonitabeleid ei ‰nnestunud kirjutada.\n"
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Kooditabelit ei ‰nnestunud registrisse lisada.\n"
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "SÅsteemilokaati ei ‰nnestunud sedistada.\n"
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Klaviatuuriasetusi ei ‰nnestunud registrisse lisada.\n"
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Geograafilist asukohta ei ‰nnestunud seadistada.\n"
        "ENTER = TaaskÑivita arvuti"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Vajuta suvalist klahvi, et jÑtkata.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "You can not create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "You can not create more than one extended partition per disk.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_FORMATTING_PARTITION,
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

MUI_PAGE etEEPages[] =
{
    {
        LANGUAGE_PAGE,
        etEELanguagePageEntries
    },
    {
        WELCOME_PAGE,
        etEEWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        etEEIntroPageEntries
    },
    {
        LICENSE_PAGE,
        etEELicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        etEEDevicePageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        etEEUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        etEEComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        etEEDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        etEEFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        etEESelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        etEEConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        etEESelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        etEEFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        etEEDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        etEEInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        etEEPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        etEEFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        etEEKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        etEEBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        etEELayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        etEEQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        etEESuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        etEEBootPageEntries
    },
    {
        REGISTRY_PAGE,
        etEERegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING etEEStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Palun oota..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Paigalda  C = Loo partitsioon    F3 = VÑlju"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Paigalda  D = Kustuta partitsioon  F3 = VÑlju"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Uue partitsiooni suurus:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//     "Oled valinud kettale uue partitsiooni loomise"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "Sisesta uue partitsiooni suurus megabaitides."},
    {STRING_CREATEPARTITION,
     "   ENTER = Loo partitsioon   ESC = Katkesta   F3 = VÑlju"},
    {STRING_PARTFORMAT,
    "JÑrgmisena vormindatakse seda partitsiooni."},
    {STRING_NONFORMATTEDPART,
    "Oled valinud ReactOSi paigaldamise uuele v‰i vormindamata partitsioonile."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "ReactOS paigaldatakse partitsioonile"},
    {STRING_CHECKINGPART,
    "Valitud partitsiooni kontrollitakse."},
    {STRING_CONTINUE,
    "ENTER = JÑtka"},
    {STRING_QUITCONTINUE,
    "F3 = VÑlju  ENTER = JÑtka"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = TaaskÑivita arvuti"},
    {STRING_TXTSETUPFAILED,
     "TXTSETUP.SIF failist ei leitud '%S' sektsiooni\n"},
    {STRING_COPYING,
     "   Kopeerimine: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Failide kopeerimine..."},
    {STRING_REGHIVEUPDATE,
    "   Registritarude uuendamine..."},
    {STRING_IMPORTFILE,
    "   %S importimine..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Monitori seadistuse uuendamine registris..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Lokaadi seadistuse uuendamine..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Klaviatuuriasetuse seadistuse uuendamine..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Kooditabeli info lisamine registrisse..."},
    {STRING_DONE,
    "   Valmis..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = TaaskÑivita arvuti"},
    {STRING_CONSOLEFAIL1,
    "Konsooli ei ‰nnestunud avada\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "T‰enÑoliselt on probleem USB klaviatuuri kasutamises\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB klaviatuurid ei ole veel toetatud\r\n"},
    {STRING_FORMATTINGDISK,
    "K‰vaketta vormindamine"},
    {STRING_CHECKINGDISK,
    "K‰vaketta kontrollimine"},
    {STRING_FORMATDISK1,
    " Vorminda partitsioon %S failisÅsteemiga (kiire vormindus) "},
    {STRING_FORMATDISK2,
    " Vorminda partitsioon %S failisÅsteemiga "},
    {STRING_KEEPFORMAT,
    " éra muuda praegust failisÅsteemi "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  K‰vaketas %lu  (Port=%hu, Siin=%hu, Id=%hu) - %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  K‰vaketas %lu  (Port=%hu, Siin=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "%I64u %s  K‰vaketas %lu  (Port=%hu, Siin=%hu, Id=%hu) - %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "%I64u %s  K‰vaketas %lu  (Port=%hu, Siin=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "K‰vaketas %lu (%I64u %s), Port=%hu, Siin=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "K‰vaketas %lu (%I64u %s), Port=%hu, Siin=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTÅÅp %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  K‰vaketas %lu  (Port=%hu, Siin=%hu, Id=%hu) - %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  K‰vaketas %lu  (Port=%hu, Siin=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Loodi uus partitsioon"},
    {STRING_UNPSPACE,
    "    %sKasutamata kettaruum%s              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (maks. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Uus (Vormindamata)"},
    {STRING_FORMATUNUSED,
    "Kasutamata"},
    {STRING_FORMATUNKNOWN,
    "Tundmatu"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Klaviatuuriasetuste lisamine"},
    {0, 0}
};
