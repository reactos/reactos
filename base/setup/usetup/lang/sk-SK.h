// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/* TRANSLATOR:  Mario Kacmar /Mario Kacmar/ aka Kario (kario@szm.sk)
 * DATE OF TR:  22-01-2008
 * Encoding  :  Latin II (852)
 * LastChange:  22-05-2011
 */

#pragma once

static MUI_ENTRY skSKSetupInitPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY skSKLanguagePageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "V\354ber jazyka.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Zvo\226te si jazyk, ktor\354 sa pou\247ije po\237as in\347tal\240cie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Potom stla\237te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tento jazyk bude predvolen\354m jazykom nain\347talovan\202ho syst\202mu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKWelcomePageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "V\241ta V\240s In\347tal\240tor syst\202mu ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Tento stupe\345 In\347tal\240tora skop\241ruje opera\237n\354 syst\202m ReactOS na V\240\347",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "po\237\241ta\237 a priprav\241 druh\354 stupe\345 In\347tal\240tora.",
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
        "\x07  Stla\237te R pre opravu ciu syst\202mu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Stla\237te L, ak chcete zobrazi\234 licen\237n\202 podmienky syst\202mu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Stla\237te F3 pre skon\237enie in\347tal\240cie, syst\202m ReactOS sa nenain\347taluje.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Pre viac inform\240ci\241 o syst\202me ReactOS, nav\347t\241vte pros\241m:",
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
        "ENTER = Pokra\237ova\234  R = Opravi\234  L = Licencia  F3 = Skon\237i\234",
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

static MUI_ENTRY skSKIntroPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY skSKLicensePageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licencia:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "Syst\202m ReactOS je vydan\354 za podmienok licencie",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL s \237as\234ami obsahuj\243cimi k\242d z in\354ch kompatibiln\354ch",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licenci\241 ako s\243 X11 alebo BSD a licencie GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Preto v\347etok softv\202r, ktor\354 je s\243\237as\234ou syst\202mu ReactOS,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "je vydan\354 pod licenciou GNU GPL, a rovnako s\243 zachovan\202",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "aj p\223vodn\202 licencie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Tento softv\202r prich\240dza BEZ Z\265RUKY alebo obmedzen\241 pou\247\241vania",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "s v\354nimkou platn\202ho miestneho a medzin\240rodn\202ho pr\240va. Licencia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "syst\202mu ReactOS pokr\354va iba distrib\243ciu k tren\241m stran\240m.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Ak z nejak\202ho d\223vodu neobdr\247\241te k\242piu licencie GNU GPL",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "spolu so syst\202mom ReactOS, nav\347t\241vte, pros\241m, str\240nku:",
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
        "Toto je slobodn\354 softv\202r; see the source for copying conditions.",
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
        "ENTER = N\240vrat",
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

static MUI_ENTRY skSKDevicePageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zoznam ni\247\347ie, zobrazuje s\243\237asn\202 nastavenia zariaden\241.",
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
        "Monitor:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Kl\240vesnica:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Rozlo\247enie kl.:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Akceptova\234:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Akceptova\234 tieto nastavenia zariaden\241",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "M\223\247ete zmeni\234 hardv\202rov\202 nastavenia stla\237en\241m kl\240vesov HORE alebo DOLE",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "pre v\354ber polo\247ky. Potom stla\237te kl\240ves ENTER pre v\354ber alternat\241vnych",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "nastaven\241.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Ak s\243 v\347etky nastavenia spr\240vne, vyberte polo\247ku",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\"Akceptova\234 tieto nastavenia zariaden\241\" a stla\237te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKRepairPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "In\347tal\240tor syst\202mu ReactOS je v za\237iato\237nom \347t\240diu v\354voja. Zatia\226",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "nepodporuje v\347etky funkcie plne vyu\247\241vaj\243ce program In\347tal\240tor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Funkcie na opravu syst\202mu zatia\226 nie s\243 implementovan\202.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Stla\237te U pre aktualiz\240ciu OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Stla\237te R pre z\240chrann\243 konzolu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Stla\237te ESC pre n\240vrat na hlavn\243 str\240nku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Stla\237te ENTER pre re\347tart po\237\241ta\237a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Hlavn\240 str\240nka  U = Aktualizova\234  R = Z\240chrana  ENTER = Re\347tart",
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

static MUI_ENTRY skSKUpgradePageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY skSKComputerPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcete zmeni\234 typ po\237\241ta\237a, ktor\354 m\240 by\234 nain\347talovan\354.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Stla\237te kl\240ves HORE alebo DOLE pre v\354ber po\247adovan\202ho typu po\237\241ta\237a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Potom stla\237te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stla\237te kl\240ves ESC pre n\240vrat na predch\240dzaj\243cu str\240nku bez zmeny",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   typu po\237\241ta\237a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   ESC = Zru\347i\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKFlushPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Syst\202m pr\240ve overuje v\347etky ulo\247en\202 \243daje na Va\347om disku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "To m\223\247e trva\234 nieko\226ko min\243t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Ke\324 skon\237\241, po\237\241ta\237 sa automaticky re\347tartuje.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vypr\240zd\345ujem cache", //Flushing cache (zapisuje sa na disk obsah cache, doslovne "Preplachovanie cashe")
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

static MUI_ENTRY skSKQuitPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Syst\202m ReactOS nie je nain\347talovan\354 kompletne.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "v\347etky m\202di\240 CD-ROM z CD mechan\241k.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Stla\237te ENTER pre re\347tart po\237\241ta\237a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Po\237kajte, pros\241m...",
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

static MUI_ENTRY skSKDisplayPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcete zmeni\234 typ monitora, ktor\354 m\240 by\234 nain\347talovan\354.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Stla\237te kl\240ves HORE alebo DOLE pre v\354ber po\247adovan\202ho typu monitora.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Potom stla\237te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stla\237te kl\240ves ESC pre n\240vrat na predch\240dzaj\243cu str\240nku bez zmeny",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   typu monitora.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   ESC = Zru\347i\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKSuccessPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Z\240kladn\202 s\243\237ast\241 syst\202mu ReactOS boli \243spe\347ne nain\347talovan\202.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "v\347etky m\202di\240 CD-ROM z CD mechan\241k.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Stla\237te ENTER pre re\347tart po\237\241ta\237a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Re\347tart po\237\241ta\237a",
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

static MUI_ENTRY skSKSelectPartitionEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zoznam ni\247\347ie, zobrazuje existuj\243ce oblasti a nevyu\247it\202 miesto",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "na disku vhodn\202 pre nov\202 oblasti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Stla\237te HORE alebo DOLE pre v\354ber zo zoznamu polo\247iek.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stla\237te ENTER pre in\347tal\240ciu syst\202mu ReactOS na vybran\243 oblas\234.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press C to create a primary/logical partition.",
//        "\x07  Stla\237te C pre vytvorenie novej oblasti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Press E to create an extended partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Stla\237te D pre vymazanie existuj\243cej oblasti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Po\237kajte, pros\241m...",
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

static MUI_ENTRY skSKChangeSystemPartition[] =
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

static MUI_ENTRY skSKConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY skSKFormatPartitionEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Form\240tovanie oblasti",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "In\347tal\240tor teraz naform\240tuje oblas\234. Stla\237te ENTER pre pokra\237ovanie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKCheckFSEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "In\347tal\240tor teraz skontroluje vybran\243 oblas\234.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Po\237kajte, pros\241m ...",
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

static MUI_ENTRY skSKInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "In\347tal\240tor nain\347taluje s\243bory syst\202mu ReactOS na zvolen\243 oblas\234.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Vyberte adres\240r kam chcete nain\347talova\234 syst\202m ReactOS:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Ak chcete zmeni\234 odpor\243\237an\354 adres\240r, stla\237te BACKSPACE a vyma\247te",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "znaky. Potom nap\241\347te n\240zov adres\240ra, v ktorom chcete aby bol",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "syst\202m ReactOS nain\347talovan\354.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKFileCopyEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        11,
        12,
        "Po\237kajte, pros\241m, k\354m In\347tal\240tor skop\241ruje s\243bory do in\347tala\237n\202ho",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        30,
        13,
        "prie\237inka pre ReactOS.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        20,
        14,
        "Dokon\237enie m\223\247e trva\234 nieko\226ko min\243t.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Po\237kajte, pros\241m ...    ",
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

static MUI_ENTRY skSKBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
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
        "Nain\347talova\234 zav\240dza\237 syst\202mu na pevn\354 disk (MBR a VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Nain\347talova\234 zav\240dza\237 syst\202mu na pevn\354 disk (iba VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Nain\347talova\234 zav\240dza\237 syst\202mu na disketu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Presko\237i\234 in\347tal\240ciu zav\240dza\237a syst\202mu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKBootLoaderInstallPageEntries[] =
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
        "In\347tal\240tor je pripraven\354 nain\347talova\234 zav\240dza\237 opera\237n\202ho syst\202mu.",
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

static MUI_ENTRY skSKBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "In\347tal\240tor je pripraven\354 nain\347talova\234 zav\240dza\237 opera\237n\202ho syst\202mu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Vlo\247te pros\241m, naform\240tovan\243 disketu do mechaniky A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "a stla\237te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcete zmeni\234 typ kl\240vesnice, ktor\354 m\240 by\234 nain\347talovan\354.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Stla\237te kl\240ves HORE alebo DOLE a vyberte po\247adovan\354 typ kl\240vesnice.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Potom stla\237te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stla\237te kl\240ves ESC pre n\240vrat na predch\240dzaj\243cu str\240nku bez zmeny",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   typu kl\240vesnice.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   ESC = Zru\347i\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Pros\241m, vyberte rozlo\247enie, ktor\202 sa nain\347taluje ako predvolen\202.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Stla\237te kl\240ves HORE alebo DOLE pre v\354ber po\247adovan\202ho rozlo\247enia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   kl\240vesnice. Potom stla\237te ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Stla\237te kl\240ves ESC pre n\240vrat na predch\240dzaj\243cu str\240nku bez zmeny",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   rozlo\247enia kl\240vesnice.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   ESC = Zru\347i\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKPrepareCopyEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Pripravuje sa kop\241rovanie s\243borov syst\202mu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vytv\240ra sa zoznam potrebn\354ch s\243borov ...",
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

static MUI_ENTRY skSKSelectFSEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Vyberte syst\202m s\243borov zo zoznamu uveden\202ho ni\247\347ie.",
        0
    },
    {
        8,
        19,
        "\x07  Stla\237te HORE alebo DOLE pre v\354ber syst\202mu s\243borov.",
        0
    },
    {
        8,
        21,
        "\x07  Stla\237te ENTER pre form\240tovanie oblasti.",
        0
    },
    {
        8,
        23,
        "\x07  Stla\237te ESC pre v\354ber inej oblasti.",
        0
    },
    {
        0,
        0,
        "ENTER = Pokra\237ova\234   ESC = Zru\347i\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKDeletePartitionEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Vybrali ste si odstr\240nenie oblasti",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Stla\237te L pre odstr\240nenie oblasti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "UPOZORNENIE: V\347etky \243daje na tejto oblasti sa nen\240vratne stratia!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Stla\237te ESC pre zru\347enie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Odstr\240ni\234 oblas\234   ESC = Zru\347i\234   F3 = Skon\237i\234",
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

static MUI_ENTRY skSKRegistryEntries[] =
{
    {
        4,
        3,
        " In\347tal\240tor syst\202mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Aktualizuj\243 sa syst\202mov\202 nastavenia.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vytv\240raj\243 sa polo\247ky registrov ...", //registry hives
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

MUI_ERROR skSKErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "Syst\202m ReactOS nie je kompletne nain\347talovan\354 na Va\347om\n"
        "po\237\241ta\237i. Ak teraz preru\347\241te in\347tal\240ciu, budete musie\234\n"
        "spusti\234 In\347tal\240tor znova, aby sa syst\202m ReactOS nain\347taloval.\n"
        "\n"
        "  \x07  Stla\237te ENTER pre pokra\237ovanie v in\347tal\240cii.\n"
        "  \x07  Stla\237te F3 pre skon\237enie in\347tal\240cie.",
        "F3 = Skon\237i\234  ENTER = Pokra\237ova\234"
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
        "In\347tal\240toru sa nepodarilo n\240js\234 pevn\354 disk.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "In\347tal\240toru sa nepodarilo n\240js\234 jej zdrojov\243 mechaniku.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "In\347tal\240tor zlyhal pri nahr\240van\241 s\243boru TXTSETUP.SIF.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "In\347tal\240tor na\347iel po\347koden\354 s\243bor TXTSETUP.SIF.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n", //chybnì (neplatnì) podpis (znak, zna\237ka, \347ifra)
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "In\347tal\240tor nemohol z\241ska\234 inform\240cie o syst\202mov\354ch diskoch.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_WRITE_BOOT,
        "In\347tal\240toru sa nepodarilo nain\347talova\234 %S zav\240dzac\241 k\242d s\243borov\202ho\n"
        "syst\202mu FAT na syst\202mov\243 part\241ciu.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "In\347tal\240tor zlyhal pri nahr\240van\241 zoznamu typov po\237\241ta\237ov.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "In\347tal\240tor zlyhal pri nahr\240van\241 zoznamu nastaven\241 monitora.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "In\347tal\240tor zlyhal pri nahr\240van\241 zoznamu typov kl\240vesn\241c.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "In\347tal\240tor zlyhal pri nahr\240van\241 zoznamu rozlo\247enia kl\240vesn\241c.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_WARN_PARTITION,
//          "In\347tal tor zistil, §e najmenej jeden pevnì disk obsahuje nekompatibiln£\n"
          "In\347tal\240tor na\347iel najmenej na jednom pevnom disku nekompatibiln\243\n"
          "tabu\226ku oblast\241, s ktorou sa ned\240 spr\240vne zaobch\240dza\234!\n"
          "\n"
          "Vytvorenie alebo odstr\240nenie oblast\241 m\223\247e zni\237i\234 tabu\226ku oblast\241.\n"
          "\n"
          "  \x07  Stla\237te F3 pre skon\237enie in\347tal\240cie.\n"
          "  \x07  Stla\237te ENTER pre pokra\237ovanie.",
          "F3 = Skon\237i\234  ENTER = Pokra\237ova\234"
    },
    {
        // ERROR_NEW_PARTITION,
        "Nem\223\247ete vytvori\234 nov\243 oblas\234\n"
        "vo vn\243tri u\247 existuj\243cej oblasti!\n"
        "\n"
        "  * Pokra\237ujte stla\237en\241m \226ubovo\226n\202ho kl\240vesu.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "In\347tal\240toru sa nepodarilo nain\347talova\234 %S zav\240dzac\241 k\242d s\243borov\202ho\n"
        "syst\202mu FAT na syst\202mov\243 part\241ciu.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_NO_FLOPPY,
        "V mechanike A: nie je disketa.",
        "ENTER = Pokra\237ova\234"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "In\347tal\240tor zlyhal pri aktualiz\240cii nastaven\241 rozlo\247enia kl\240vesnice.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_CREATE_DIR,
        "In\347tal\240tor nemohol vytvori\234 in\347tala\237n\202 adres\240re.",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "In\347tal\240tor zlyhal pri h\226adan\241 sekcie '%S'\n"
        "v s\243bore TXTSETUP.SIF.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_CABINET_SECTION,
        "In\347tal\240tor zlyhal pri h\226adan\241 sekcie '%S'\n"
        "v s\243bore cabinet.\n",
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "In\347tal\240tor nemohol vytvori\234 in\347tala\237n\354 adres\240r.", //could not = nemohol
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_WRITE_PTABLE,
        "In\347tal\240tor zlyhal pri z\240pise do tabuliek oblast\241.\n"
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "In\347tal\240tor zlyhal pri prid\240van\241 k\242dovej str\240nky do registrov.\n"
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "In\347tal\240tor zlyhal pri prid\240van\241 rozlo\247en\241 kl\240vesnice do registrov.\n"
        "ENTER = Re\347tart po\237\241ta\237a"
    },
    {
        // ERROR_UPDATE_GEOID,
        "In\347tal\240tor nemohol nastavi\234 geo id.\n"
        "ENTER = Re\347tart po\237\241ta\237a"
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
        "  * Pokra\237ujte stla\237en\241m \226ubovo\226n\202ho kl\240vesu.",
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

MUI_PAGE skSKPages[] =
{
    {
        SETUP_INIT_PAGE,
        skSKSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        skSKLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        skSKWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        skSKIntroPageEntries
    },
    {
        LICENSE_PAGE,
        skSKLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        skSKDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        skSKRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        skSKUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        skSKComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        skSKDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        skSKFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        skSKSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        skSKChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        skSKConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        skSKSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        skSKFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        skSKCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        skSKDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        skSKInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        skSKPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        skSKFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        skSKKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        skSKBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        skSKLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        skSKQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        skSKSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        skSKBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        skSKBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        skSKRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING skSKStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Po\237kajte, pros\241m ..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = In\347talova\234   C = Create Primary   E = Create Extended   F3 = Skon\237i\234"},
//     "   ENTER = In\347talova\234   C = Vytvori\234 oblas\234   F3 = Skon\237i\234"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = In\347talova\234   C = Create Logical Partition   F3 = Skon\237i\234"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = In\347talova\234   D = Odstr\240ni\234 oblas\234   F3 = Skon\237i\234"},
    {STRING_DELETEPARTITION,
     "   D = Odstr\240ni\234 oblas\234   F3 = Skon\237i\234"},
    {STRING_PARTITIONSIZE,
     "Ve\226kos\234 novej oblasti:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "You have chosen to create a primary partition on"},
//     "Zvolili ste vytvorenie novej oblasti na"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDPARTSIZE,
    "Zadajte, pros\241m, ve\226kos\234 novej oblasti v megabajtoch."},
    {STRING_CREATEPARTITION,
     "   ENTER = Vytvori\234 oblas\234   ESC = Zru\347i\234   F3 = Skon\237i\234"},
    {STRING_NEWPARTITION,
    "In\347tal\240tor vytvoril nov\243 oblas\234 na"},
    {STRING_PARTFORMAT,
    "T\240to oblas\234 sa bude form\240tova\234 ako \324al\347ia."},
    {STRING_NONFORMATTEDPART,
    "Zvolili ste in\347tal\240ciu syst\202mu ReactOS na nov\243 alebo nenaform\240tovan\243 oblas\234."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "In\347tal\240tor nain\347taluje syst\202m ReactOS na oblas\234"},
    {STRING_CONTINUE,
    "ENTER = Pokra\237ova\234"},
    {STRING_QUITCONTINUE,
    "F3 = Skon\237i\234  ENTER = Pokra\237ova\234"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Re\347tart po\237\241ta\237a"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kop\241ruje sa s\243bor: %S"},
    {STRING_SETUPCOPYINGFILES,
     "In\347tal\240tor kop\241ruje s\243bory..."},
    {STRING_REGHIVEUPDATE,
    "   Aktualizujem polo\247ky registrov..."},
    {STRING_IMPORTFILE,
    "   Importujem %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Aktualizujem nastavenia obrazovky v registrov..."}, //display registry settings
    {STRING_LOCALESETTINGSUPDATE,
    "   Aktualizujem miestne nastavenia..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Aktualizujem nastavenia rozlo\247enia kl\240vesnice..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Prid\240inform\240cie o k\242dovej str\240nke..."},
    {STRING_DONE,
    "   Hotovo..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Re\347tart po\237\241ta\237a"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Nemo\247no otvori\234 konzolu\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Najbe\247nej\347ou pr\241\237inou tohto je pou\247itie USB kl\240vesnice\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB kl\240vesnica e\347te nie je plne podporovan\240\r\n"},
    {STRING_FORMATTINGPART,
    "In\347tal\240tor form\240tuje oblas\234..."},
    {STRING_CHECKINGDISK,
    "In\347tal\240tor kontroluje disk..."},
    {STRING_FORMATDISK1,
    " Naform\240tova\234 oblas\234 ako syst\202m s\243borov %S (r\354chly form\240t) "},
    {STRING_FORMATDISK2,
    " Naform\240tova\234 oblas\234 ako syst\202m s\243borov %S "},
    {STRING_KEEPFORMAT,
    " Ponecha\234 s\243\237asn\354 syst\202m s\243borov (bez zmeny) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "na: %s."},
    {STRING_PARTTYPE,
    "Typ 0x%02x"},
    {STRING_HDDINFO1,
    // "pevn\354 disk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s pevn\354 disk %lu (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]"},
    {STRING_HDDINFO2,
    // "pevn\354 disk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s pevn\354 disk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Miesto bez oblast\241"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Nov\240 (Nenaform\240tovan\240)"},
    {STRING_FORMATUNUSED,
    "Nepou\247it\202"},
    {STRING_FORMATUNKNOWN,
    "Nezn\240me"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Prid\240vam rozlo\247enia kl\240vesnice"},
    {0, 0}
};
