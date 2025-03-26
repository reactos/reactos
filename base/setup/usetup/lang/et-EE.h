// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY etEESetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
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

static MUI_ENTRY etEELanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Keele valik",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Vali paigaldamisel kasutatav keel.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Seej\204rel vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Seda keelt kasutatakse hiljem s\201steemi keelena.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka  F3 = V\204lju",
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

static MUI_ENTRY etEEWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Tere tulemast ReactOS'i paigaldusse",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Selles paigaldamise etapis kopeeritakse ReactOSi failid arvutisse ja",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "valmistatakse ette paigaldamise teine j\204rk.",
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
        "\x07  Vajuta R, et ReactOS'i parandada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Vajuta L, et n\204ha ReactOS'i litsentsi ja kasutamise tingimusi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Vajuta F3, et v\204ljuda ReactOS'i paigaldamata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "ReactOS'i kohta saab rohkem infot:",
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
        "ENTER = J\204tka  R = Paranda  L = Litsents  F3 = V\204lju",
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

static MUI_ENTRY etEEIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS'i versiooni seisund",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS on alfa etapis, mis t\204hendab et see ei ole veel funktsionaalselt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "t\204iuslik ja on t\344sises arenguses. Seda on soovitatud ainult kasutada",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "tutvumise ja proovimise eesm\204rkidel, aga mitte igap\204evalise os\201steemina.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Varundage enda andmed v\344i proovi teisej\204rgulisel arvutil kui proovite",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "ReactOS'i p\204ris riistvara peal'.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Vajuta ENTER, et siseneda ReactOS'i paigaldusse.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Vajuta F3, et lahkuda ReactOS'i paigaldamata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   F3 = V\204lju",
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

static MUI_ENTRY etEELicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Litsents:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS kasutab GNU \201ldist avalikku litsentsi(GPL),",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "m\344ned komponendid kasutavad muid \201hilduvaid litsentse,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "nagu n\204iteks X11, BSD ja GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Kogu ReactOS'i s\201steem on seega kaitstud GPL litsentsiga",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "ning samas kehtivad ka algsed litsentsid.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "K\204esoleva tarkvaraga ei anta kaasa garantiid ega m\204\204rata kasutamise",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "piiranguid kehtiva seadusega s\204testatud piirides. ReactOSi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "litsents m\204\204rab ainult levitamise kolmandatele osapooltele.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Kui mingil p\344hjusel ei olnud tarkvaraga kaasas GNU GPL",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\201ldist avalikku litsentsi, siis saab seda vaadata lehel",
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
        "Garantii:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Tegemist on vaba tarkvaraga; kopeerimise tingimused on kirjas algkoodis.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "Garantii puudub; pole isegi turustamiseks v\344i mingil",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "kindlal eesm\204rgil kasutamiseks sobivuse garantiid",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Tagasi",
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

static MUI_ENTRY etEEDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "J\204rgnev nimekiri n\204itab riistvara seadeid.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Arvuti:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Ekraan:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Klaviatuur:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Klaviatuuri asetus:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Rakenda:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Rakenda need s\204tted",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Riistvara s\204tteid saab muuta \201les ja alla liikudes.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "Seadistuse muutmiseks vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Kui seadistus on paigas, vali \"Rakenda need s\204tted\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "ja vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   F3 = V\204lju",
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

static MUI_ENTRY etEERepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS'i paigaldusprogramm on varajases arendusfaasis. Praegu ei ole",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "veel k\344ik korraliku paigaldusprogrammi funktsioonid toetatud.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Parandamine ei ole veel toetatud.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Vajuta U, et s\201steemi uuendada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Vajuta R, et kasutada taastuskonsooli.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Vajuta ESC, et minna tagasi pealehele.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Vajuta ENTER, et arvuti taask\204ivitada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Pealeht  U = Uuenda  R = Taastamine  ENTER = Taask\204ivitus",
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

static MUI_ENTRY etEEUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
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

static MUI_ENTRY etEEComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Arvuti t\201\201bi muutmine.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Liigu \201les-alla, et valida sobiv arvuti t\201\201p.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Seej\204rel vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Vajuta ESC, et minna tagasi eelmisele lehele",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   ilma arvuti t\201\201pi muutmata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   ESC = Katkesta   F3 = V\204lju",
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

static MUI_ENTRY etEEFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "S\201steem kirjutab n\201\201d andmed kettale.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "V\344ib kuluda veidi aega.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "L\344petamisel taask\204ivitub arvuti automaatselt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vahem\204lu t\201hjendamine",
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

static MUI_ENTRY etEEQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS ei ole t\204ielikult paigaldatud.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Eemalda flopiketas ja CD-ROMid draividest.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Vajuta ENTER, et arvuti taask\204ivitada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Palun oota...",
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

static MUI_ENTRY etEEDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Ekraani t\201\201bi muutmine.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Liigu \201les-alla, et ekraani t\201\201pi muuta.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Seej\204rel vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Vajuta ESC, et minna tagasi eelmisele lehele",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   ilma ekraani t\201\201pi muutmata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   ESC = Katkesta   F3 = V\204lju",
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

static MUI_ENTRY etEESuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS'i p\344hilised komponendid on edukalt paigaldatud.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Eemalda flopiketas ja CD-ROMid draividest.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Vajuta ENTER, et arvuti taask\204ivitada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Taask\204ivita arvuti",
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

static MUI_ENTRY etEESelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "See nimekiri n\204itab partitsioone ja vaba ruumi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "uute partitsioonide jaoks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Liigu \201les-alla, et valida kirje.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Vajuta ENTER, et paigaldada ReactOS valitud partitsioonile.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Vajuta C uue primaarse/loogilise partitsiooni loomiseks.",
//        "\x07  Vajuta C, et teha uus partitsioon.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Vajuta E uue laiendatud partitsiooni loomiseks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Vajuta D olemasoleva partitsiooni kustutamiseks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Palun oota...",
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

static MUI_ENTRY etEEChangeSystemPartition[] =
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

static MUI_ENTRY etEEConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Olete valinud s\201steemi partitsiooni kustutamise.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "S\201steemi partitsioon v\344ib sisaldada diagnostika programme, riistvara konfiguratsiooni",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programme, programme millega alustada ops\201steeme (nagu ReactOS) v\344i muid",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "programme riistvara tootja poolt esitatud.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Kustuta s\201steemi partitsioon ainult siis kui olete kindel, et seal ei ole \201htegi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "sellist programmi partitsioonil v\344i kui olete kindel, et tahate neid kustutada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Kui kustutad partitsiooni, v\344ib juhtuda, et Te ei saa k\204ivitada",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "arvutit k\344vakettalt kuni l\344petate ReactOS'i paigalduse.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Vajuta ENTER s\201steemi partitsiooni kustutamiseks. Teilt k\201sitakse",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   hiljem uuesti kinnitust partitsiooni kustutamiseks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Vajuta ESC eelmise lehele tagasip\224\224rdumiseks. Partitsiooni",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   ei kustutata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=J\204tka  ESC=Loobu",
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

static MUI_ENTRY etEEFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Partitsiooni vormindamine",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "N\201\201d vormindatakse partitsioon. Vajuta ENTER j\204tkamiseks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = J\204tka   F3 = V\204lju",
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

static MUI_ENTRY etEECheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Valitud partitsiooni kontrollitakse.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Palun oota...",
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

static MUI_ENTRY etEEInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS paigaldatakse valitud partitsioonile.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Vali kaust, kuhu ReactOS paigaldada:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Kausta muutmiseks kustuta kirje BACKSPACE klahviga ja",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "tr\201ki asemele kaust, kuhu ReactOS installeerida.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   F3 = V\204lju",
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

static MUI_ENTRY etEEFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Palun oota, kuni ReactOS paigaldatakse sihtkausta.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "V\344ib kuluda mitu minutit.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Palun oota...    ",
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

static MUI_ENTRY etEEBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
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
        "Paigalda alglaadur k\344vakettale (MBR ja VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Paigalda alglaadur k\344vakettale (ainult VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Paigalda alglaadur flopikettale.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\216ra paigalda alglaadurit.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   F3 = V\204lju",
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

static MUI_ENTRY etEEBootLoaderInstallPageEntries[] =
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
        "Alglaaduri paigaldamine.",
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

static MUI_ENTRY etEEBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Alglaaduri paigaldamine.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Sisesta vormindatud flopiketas draivi A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "ja vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   F3 = V\204lju",
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

static MUI_ENTRY etEEKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Klaviatuuri t\201\201bi muutmine.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Vajuta \201les-alla, et valida klaviatuuri t\201\201p.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Seej\204rel vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Vajuta ESC eelmisele lehele tagasi p\224\224rdumiseks",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   klaviatuuri t\201\201pi muutmata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   ESC = Katkesta   F3 = V\204lju",
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

static MUI_ENTRY etEELayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Vali vaikimisi klaviatuuriasetus.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Liigu \201les-alla, et valida klaviatuuriasetus.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    Seej\204rel vajuta ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Vajuta ESC eelmisele lehele tagasi p\224\224rdumiseks",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   klaviatuuriasetust muutmata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = J\204tka   ESC = Katkesta   F3 = V\204lju",
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

static MUI_ENTRY etEEPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Arvutit valmistatakse ette ReactOSi failide kopeerimiseks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Failide nimekirja loomine...",
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

static MUI_ENTRY etEESelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Vali nimekirjast failis\201steem.",
        0
    },
    {
        8,
        19,
        "\x07  Liigu \201les-alla, et valida failis\201steem.",
        0
    },
    {
        8,
        21,
        "\x07  Vajuta ENTER partitsiooni vormindamiseks.",
        0
    },
    {
        8,
        23,
        "\x07  Vajuta ESC muu partitsiooni valimiseks.",
        0
    },
    {
        0,
        0,
        "ENTER = J\204tka   ESC = Katkesta   F3 = V\204lju",
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

static MUI_ENTRY etEEDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Oled valinud partitsiooni kustutamise",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Vajuta L partitsiooni kustutamiseks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "HOIATUS: K\344ik andmed partitsioonil kustutatakse!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Vajuta ESC katkestamiseks.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Kustuta partitsioon   ESC = Katkesta   F3 = V\204lju",
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

static MUI_ENTRY etEERegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " paigaldus ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Uuendatakse s\201steemi seadistust.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Registri v\344tmete loomine...",
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

MUI_ERROR etEEErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "\345nnestus\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS ei ole t\204ielikult paigaldatud.\n"
        "Kui paigaldamine praegu katkestada, siis tuleb\n"
        "ReactOS'i paigaldamiseks paigaldusprogramm uuesti k\204ivitada.\n"
        "\n"
        "  \x07  Vajuta ENTER paigalduse j\204tmaiseks.\n"
        "  \x07  Vajuta F3 paigalduse seiskamiseks.",
        "F3 = V\204lju  ENTER = J\204tka"
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
        "K\344vaketast ei leitud.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Paigaldusprogramm ei leidnud ketast, millelt see k\204ivitati.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "TXTSETUP.SIF faili ei \344nnestunud laadida.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "TXTSETUP.SIF on vigane.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "TXTSETUP.SIF faili signatuur on vigane.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "S\201steemiketta parameetreid ei \344nnestunud lugeda.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_WRITE_BOOT,
        "S\201steemikettale ei \344nnestunud kirjutada %S alglaadimiskoodi.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Arvutit\201\201pide nimekirja ei \344nnestunud laadida.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Ekraanide nimekirja ei \344nnestunud laadida.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Klaviatuuri t\201\201pide nimekirja ei \344nnestunud laadida.\n",
         "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Klaviatuuriasetuste nimekirja ei \344nnestunud laadida.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_WARN_PARTITION,
          "Leiti v\204hemalt \201ks k\344vaketas, millel on \201hildamatu partitsioonitabel,\n"
          "millega ei saanud korralikult \201mber k\204ia!\n"
          "\n"
          "Partitsioonide loomine v\344i kustutamine v\344ib vigastada partitsioonitabelit.\n"
          "\n"
          "  \x07  Vajuta F3 paigaldusest v\204ljumiseks..\n"
          "  \x07  Vajuta ENTER j\204tkamiseks.",
          "F3 = V\204lju  ENTER = J\204tka"
    },
    {
        // ERROR_NEW_PARTITION,
        "Uut partitsioonitabelit ei saa juba olemasoleva\n"
        "partitsiooni sisse tekitada!\n"
        "\n"
        "  * Vajuta mis tahes klahvi, et j\204tkata.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "S\201steemikettale ei \344nnestunud paigaldada %S alglaadimiskoodi.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_NO_FLOPPY,
        "Draivis A: ei ole flopiketast.",
        "ENTER = J\204tka"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Klaviatuuriasetuse seadistust ei \344nnestunud uuendada.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Ekraani seadistust registris ei \344nnestunud uuendada.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Tarufaili ei \344nnestunud importida.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_FIND_REGISTRY
        "Registri andmete faile ei leitud.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_CREATE_HIVE,
        "Registri tarusid ei \344nnestunud luua.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Registrit ei \344nnestunud luua.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Kapifailis ei olnud p\204devaid inf faile.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_CABINET_MISSING,
        "Kapifaili ei leitud.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Kapifailis puudub paigaldusskript.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_COPY_QUEUE,
        "Kopeeritavate failide nimekirja ei \344nnestunud avada.\n",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_CREATE_DIR,
        "Paigalduskaustu ei \344nnestunud luua.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "TXTSETUP.SIF failist ei leitud '%S' sektsiooni.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_CABINET_SECTION,
        "Kapifailist ei leitud '%S' sektsiooni.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Paigalduskausta ei \344nnestunud luua.",
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Partitsioonitabeleid ei \344nnestunud kirjutada.\n"
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Kooditabelit ei \344nnestunud registrisse lisada.\n"
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "S\201steemilokaati ei \344nnestunud sedistada.\n"
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Klaviatuuriasetusi ei \344nnestunud registrisse lisada.\n"
        "ENTER = Taask\204ivita arvuti"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Geograafilist asukohta ei \344nnestunud seadistada.\n"
        "ENTER = Taask\204ivita arvuti"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Sobimatu kausta nimi.\n"
        "\n"
        "  * Vajuta mis tahes klahvi, et j\204tkata."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Valitud partitsioon ei ole piisavalt suur ReactOS'i paigaldamiseks.\n"
        "Paigalduse partitsioon peab v\204heamlt %lu MB suur olema.\n"
        "\n"
        "  * Vajuta mis tahes klahvi, et j\204tkata.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "You cannot create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Vajuta mis tahes klahvi, et j\204tkata."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Te ei saa luua rohkem kui \201he laiendatud partitsiooni ketta kohta.\n"
        "\n"
        "  * Vajuta mis tahes klahvi, et j\204tkata."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Viisard ei saanud vormindada partitsiooni:\n"
        " %S\n"
        "\n"
        "ENTER = Taask\204ivita arvuti"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE etEEPages[] =
{
    {
        SETUP_INIT_PAGE,
        etEESetupInitPageEntries
    },
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
        REPAIR_INTRO_PAGE,
        etEERepairPageEntries
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
        CHANGE_SYSTEM_PARTITION,
        etEEChangeSystemPartition
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
        CHECK_FILE_SYSTEM_PAGE,
        etEECheckFSEntries
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
        BOOTLOADER_SELECT_PAGE,
        etEEBootLoaderSelectPageEntries
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
        BOOTLOADER_INSTALL_PAGE,
        etEEBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        etEEBootLoaderRemovableDiskPageEntries
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
     "   ENTER = Paigalda   C = Loo primaarne   E = Loo laiendatud   F3 = V\204lju"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Paigalda   C = Loo loogiline partitsioon   F3 = V\204lju"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Paigalda  D = Kustuta partitsioon  F3 = V\204lju"},
    {STRING_DELETEPARTITION,
     "   D = Kustuta partitsioon   F3 = V\204lju"},
    {STRING_PARTITIONSIZE,
     "Uue partitsiooni suurus:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "You have chosen to create a primary partition on"},
//     "Oled valinud kettale uue partitsiooni loomise"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDPARTSIZE,
    "Sisesta uue partitsiooni suurus megabaitides."},
    {STRING_CREATEPARTITION,
     "   ENTER = Loo partitsioon   ESC = Katkesta   F3 = V\204lju"},
    {STRING_NEWPARTITION,
    "Loodi uus partitsioon"},
    {STRING_PARTFORMAT,
    "J\204rgmisena vormindatakse seda partitsiooni."},
    {STRING_NONFORMATTEDPART,
    "Oled valinud ReactOS'i paigaldamise uuele v\344i vormindamata partitsioonile."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "S\201steemipartitsioon on veel vormindamata."},
    {STRING_NONFORMATTEDOTHERPART,
    "Uus partitsioon on veel vormindamata."},
    {STRING_INSTALLONPART,
    "ReactOS paigaldatakse partitsioonile"},
    {STRING_CONTINUE,
    "ENTER = J\204tka"},
    {STRING_QUITCONTINUE,
    "F3 = V\204lju  ENTER = J\204tka"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Taask\204ivita arvuti"},
    {STRING_DELETING,
     "   Faili kustutamine: %S"},
    {STRING_MOVING,
     "   Faili liigutamine: %S asukohta %S"},
    {STRING_RENAMING,
     "   Faili \201mbernimetamine: %S %S-ks"},
    {STRING_COPYING,
     "   Faili kopeerimine: %S"},
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
    "   Kooditabeli info lisamine..."},
    {STRING_DONE,
    "   Valmis..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Taask\204ivita arvuti"},
    {STRING_REBOOTPROGRESSBAR,
    " Teie arvuti taask\204ivitub %li sekundi p\204rast... "},
    {STRING_CONSOLEFAIL1,
    "Konsooli ei \344nnestunud avada\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "T\344en\204oliselt on probleem USB klaviatuuri kasutamises\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB klaviatuurid ei ole veel toetatud\r\n"},
    {STRING_FORMATTINGPART,
    "Partitsioon vormindamine..."},
    {STRING_CHECKINGDISK,
    "K\344vaketta kontrollimine..."},
    {STRING_FORMATDISK1,
    " Vorminda partitsioon %S failis\201steemiga (kiire vormindus) "},
    {STRING_FORMATDISK2,
    " Vorminda partitsioon %S failis\201steemiga "},
    {STRING_KEEPFORMAT,
    " \216ra muuda praegust failis\201steemi "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "%s."},
    {STRING_PARTTYPE,
    "Type 0x%02x"},
    {STRING_HDDINFO1,
    // "K\344vaketas %lu (%I64u %s), Port=%hu, Siin=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s K\344vaketas %lu (Port=%hu, Siin=%hu, Id=%hu) - %wZ [%s]"},
    {STRING_HDDINFO2,
    // "K\344vaketas %lu (%I64u %s), Port=%hu, Siin=%hu, Id=%hu [%s]"
    "%I64u %s K\344vaketas %lu (Port=%hu, Siin=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Kasutamata kettaruum"},
    {STRING_MAXSIZE,
    "MB (maks. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Laiendatud partitsioon"},
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
