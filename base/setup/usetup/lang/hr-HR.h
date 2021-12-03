// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY hrHRSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Pri\304\215ekajte da se instalacija u\304\215ita",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "i otkrije va\305\241e ure\304\221aje",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Molimo pri\304\215ekajte",
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

static MUI_ENTRY hrHRLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Odabir jezika",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Molimo odaberite jezik koji \304\207e se koristiti za vrijeme instalacije",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   I pritisnite ENTER",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Ovaj jezik \304\207e biti zadan za cijeli sustav",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi  F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Dobrodo\305\241li u ReactOS instalaciju",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Ovaj dio instalacije kopira ReactOS na va\305\241e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "ra\304\215unalo i priprema se za drugi dio instalacije.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Pritisnite ENTER za instalaciju ili nadogradnju ReactOS-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
     // "\x07  Pritisnite R za popravak ReactOS instalacije koriste\304\207i Recovery Console.",
        "\x07  Pritisnite R za popravak ReactOS instalacije.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Pritisnite L da biste vidjeli ReactOS licencu (Licensing Terms and Conditions).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pritisnite F3 da biste iza\305\241li bez instalacije ReactOS-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Za vi\305\241e informacija o ReactOS-a, posjetite:",
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
        "ENTER = Nastavi  R = Popravi  L = Licenca  F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Status Verzije",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS je u Alphi, \305\241to zna\304\215i da nije potpun svih funkcija",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "i nalazi se u intezivnom razvoju. Preporu\304\215uje se da se koristi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "za testiranje te se ne preporu\304\215uje da se koristi kao dnevni operativan sustav.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Sigurnosno kopirajte svoje datoteke ili testirajte na pomo\304\207nom ra\304\215unalu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "da biste pokrenuli ReactOS na fizi\304\215kom ra\304\215unalu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Pritisnite ENTER da biste nastavili ReactOS instalaciju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pritisnite F3 da biste iza\305\241li bez instalacije ReactOS-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licenca:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS je pod licencom",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL s dijelovima koji sadr\305\276i kod iz drugih kompatibilnih",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licenci kao X11 or BSD i GNU LGPL licence.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Sav software je dio ReactOS sustava",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "te je objavljen pod GNU GPL licencom kao i odr\305\276avanje",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "originalne licence.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Ovaj software dolazi BEZ GARANCIJE ili restrikcija kori\305\241tenja",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "osim primjenjivog lokalnog i me\304\221unarodnog prava. Licenciranje",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "ReactOS-a pokriva distribuciju samo tre\304\207im stranama.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Ako za neki slu\304\215aj niste primili kopiju",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "GNU General Public licence s ReactOS molimo posjetite",
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
        "Garancija:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Ovo je besplatan software; pogledajte izvor za uvjete kopiranja.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "Ovdje NEMA garancije; \304\215ak ni za TRGOVINU ili za",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "PRIKLADNOST ZA ODRE\304\220ENU NAMJENU",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Povratak",
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

static MUI_ENTRY hrHRDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Lista ispod prikazuje trenutne postavke ure\304\221aja.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Ra\304\215unalo:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Zaslon:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Tipkovnica:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Izgled tipkovnice:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Prihvati:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Prihvati ove postavke ure\304\221aja",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Mo\305\276ete promijeniti postavki hardware-a koriste\304\207i UP ili DOWN tipke",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "da biste ozna\304\215ili odrednicu. I pritisnite ENTER tipku da biste ozna\304\215ili altrenativu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "postavki.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Kada su sve postavke to\304\215ne, odaberite \"Prihva\304\207am ove postavke ure\304\221aja\"", //maybe a problem
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "i pritisnite ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS instalacija je u ranoj fazi razvijanja. Jo\305\241 nema",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "podr\305\241ku svih funkcija potupne aplikacije za instalaciju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Funkcije popravka nisu jo\305\241 napravljene.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Pritisnite U za a\305\276uriranje operativnog sustava.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Pritisnite R za Recovery Console.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Pritisnite ESC da biste se vartili na glavnu stranicu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pritisnite ENTER za ponovno pokretanje ra\304\215unala.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = glavna stranica  U = A\305\276uriranje  R = Oporavak  ENTER = Ponovno pokretanje",
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

static MUI_ENTRY hrHRUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS instalacija mo\305\276e a\305\276urirati jednu postoje\304\207u ReactOS instalaciju",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "napisanu ispod, ili, ako je ReactOS instalacija je o\305\241te\304\207ena, instalacijski program",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "mo\305\276e poku\305\241ati popraviti je.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Funkcije popravka nisu jo\305\241 napravljene.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Pritisnite UP ili DOWN tipke da biste odabrali instalaciju operativnog sustava",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07 Pritisnite U za nadogradnju odabrane instalacije operativnog sustava.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Pritisnite ESC da biste nastavili s novom instalacijom.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pritisnite F3 da biste iza\305\241li bez instalacije ReactOS-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = A\305\276uriraj   ESC = nemoj a\305\276urirati   F3 = Quit",
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

static MUI_ENTRY hrHRComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\305\275elite promijeniti vrstu ra\304\215unala da bi se moglo instalirati.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Pritisnite tipku UP ili DOWN za odabir \305\276eljene vrste ra\304\215unala.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   I pritisnite ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Pritisnite ESC tipku da biste se vartili na prija\305\241nju stranicu bez promjena",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   vrste ra\304\215unala.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   ESC = Odustani   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Sustav se brine da sve datoteke skaldi\305\241tu na va\305\241em disku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Ovo \304\207e potrajati jednu minutu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Kada zavr\305\241i, va\305\241e \304\207e se ra\304\215unalo automatski ponovo pokrenuti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\304\214i\305\241\304\207enje predmemorije",
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

static MUI_ENTRY hrHRQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS nije potpuno instaliran.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Uklonite disketu iz Diska A: i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "sve CD-ROM-ove iz CD utora.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Pritisnite ENTER da biste ponovo pokrenuli ra\304\215unalo.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Molimo pri\304\215ekajte...",
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

static MUI_ENTRY hrHRDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\305\275elite promijeniti vrstu zaslona da bi se moglo instalirati.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Pritisnite UP ili DOWN tipke za odabir \305\276eljene vrste prikaza.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   I pritisnite ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Pritisnite ESC tipku da biste se vartili na prija\305\241nju stranicu bez promjene",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   vrste prikaza.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   ESC = Odustani   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Osnovni dijelovi ReactOS-a su se uspje\305\241no instalirali.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Uklonite disketu iz Diska A: i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "sve CD-ROM-ove iz CD utora.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Pritisnite ENTER da biste ponovno pokrenuli ra\304\215unalo.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Ponovno pokreni ra\304\215unalo",
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

static MUI_ENTRY hrHRBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalacijski program ne mo\305\276e instalirati bootloader (pokreta\304\215 operativnog sustava) na va\305\241",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "tvrdi disk u ra\304\215unalu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Ubacite formatiranu disketu u Disk A: i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "pritisnite ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Lista ispod prikazuje postoje\304\207e particije i ne kori\305\241teni diskovni",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "prostor za nove particije.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Pritisnite UP ili DOWN tipke da biste odabrali unos u popis.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Pritisnite ENTER da biste instalirali ReactOS na odabranu particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Pritisnite P da biste stvorili primarnu particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Pritisnite P da biste stvorili pro\305\241irenu particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Pritisnite P da biste stvorili logi\304\215ku particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pritisnite D da biste izbrisali postoje\304\207u particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Molimo pri\304\215ekajte...",
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

static MUI_ENTRY hrHRChangeSystemPartition[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Trenutna sistemska particija na va\305\241em ra\304\215unalu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "na sistemskom disku",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "koristi format koji nije podr\305\276an od strane ReactOS-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "Da bi se provela uspje\305\241na instalacija ReactOS-a, instalacijski program mora promijeniti",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "trenutnu sistemsku particiju u novi sustav particija.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Opcije novog sustava particija:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Da biste prihavtili ovaj odabir, pritisnite ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Da biste ru\304\215no promijenili sistemsku particiju, pritisnite ESC tipku da biste se vratili natrag",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   na listu odabiranje particija, te odaberite ili stvorite novi sustav",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   particija na sistemskom disku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "U slu\304\215aju da ima i drugih operativnih sustava koji se oslanjaju na original",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "sistemske particije, mo\305\276da trebate njih rekonfigurirati ih za novu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "sistemsku particiju, ili mo\305\276da trebate vratiti sustav partcija na",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "original nakon \305\241to se instalacija ReactOS-a zavr\305\241i.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   ESC = Odustani",
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

static MUI_ENTRY hrHRConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Odabarli ste izbrisati sistemsku particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Sistemska particija mo\305\276e sadr\305\276avati dijagnosti\304\215ke programe, konfiguraciju hardware-a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programe, programi koji pokre\304\207u operativa sustav (npr. ReactOS) ili drugi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "programi koje pru\305\276a proizvo\304\221a\304\215 hardware-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Mo\305\276ete izbrisati sistemsku particiju samo ako ste sigurni da nema",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "programa na particji, ili ako ste sigurni u brisanje.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Kada izbri\305\241ete particju, mo\305\276da ne\304\207ete mo\304\207i pokrenuti operativni sustav",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        " s trvdog diska u ra\304\215unalu until dok ne zavr\305\241ite instalaciju ReactOS-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Pritisnite ENTER da biste izbrisali sistemsku particju. Biti \304\207ete pitani",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   ponovno da potvrdite brisanje particije kasnije.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Pritisnite ESC tipku da se vartite na prija\305\241nju stranicu. Particija",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   se ne\304\207e izbrisati.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Nastavi  ESC=Odustani",
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

static MUI_ENTRY hrHRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Formatiraj particiju",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Instalacijski program \304\207e sada formatirati particiju. Pritisnite ENTER da bsite nastavili.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Nastavi   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalacijski program provjerava odabranu particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Molimo pri\304\215ekajte...",
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

static MUI_ENTRY hrHRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalacijski program kopira datoteke ReactOS-a na odabranu particju. Odaberite",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "direktorij gdje \305\276elite da se ReactOS instalira:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Da biste promijenili preporu\304\215en dikterorij instalacije, pritisnite BACKSPACE da biste izbrisali",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "preporu\304\215en direktorij i unesite direktorij u kojem \305\276elite da se ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "instalira.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Pri\304\215ekajte dok ReactOS instalacijski program kopira datoteke u Va\305\241 ReactOS",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "instalacijski direktorij.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Ovo mo\305\276da potraje nekoliko minuta dok se proces ne zavr\305\241i.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Molimo pri\304\215ekajte...    ",
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

static MUI_ENTRY hrHRBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalacijski program instalirava bootloader (pokteta\304\215a operativnog sustava)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Instaliraj bootloader na tvrdi disk (MBR i VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Instaliraj bootloader na tvrdi disk (VBR samo).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Instaliraj bootloader na disketu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Presko\304\215i instalaciju bootloader-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRBootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Instalacija bootloader-a, molimo pri\304\215elajte...",
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

static MUI_ENTRY hrHRKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Ako \305\276elite promijeniti vrtu tipkovnice koji \304\207e se instalirati",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  pritisnite UP ili DOWN tipke za odabir \305\276eljene vrste tipkovnice.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   I pritisnite ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Pritisnite ESC tipku da biste se vratili na prija\305\241nju stranicu bez promjena",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   vrste tipkovnice.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   ESC = Odustani   F3 = iza\304\221i",
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

static MUI_ENTRY hrHRLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Please select a layout to be installed by default.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Pritisnite UP ili DOWN tipke da biste odabrali \305\276eljeni raspored",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    tipkovnice. Tada pritisnite ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Pritisnite ESC tipku da biste se vratili na prija\305\241nju stranicu bez promjena",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   rasporeda tipkovnice.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Nastavi   ESC = Odustani   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalacijski sustav priprema ra\304\215unalo na kopiranje datoteke ReactOS-a.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Izrada popisa kopija datoteka...",
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

static MUI_ENTRY hrHRSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Odaberite sustav datoteka iz donje liste.",
        0
    },
    {
        8,
        19,
        "\x07  Pritisnite UP ili DOWN da biste odabrali \305\276eljeni sustav datoteka.",
        0
    },
    {
        8,
        21,
        "\x07  Pritisnite ENTER da biste formatirali particiju.",
        0
    },
    {
        8,
        23,
        "\x07  Pritisnite ESC da biste odabrali drugu particju.",
        0
    },
    {
        0,
        0,
        "ENTER = Nastavi   ESC = Odustani   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Odabrali ste izbrisati particiju",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Pritisnite L da biste izbrisali particiju.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "UPOZORENJE: Svi podaci na particiji \304\207e biti izgubljeni",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Pritisnite ESC da biste odustali.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Izbri\305\241i particiju   ESC = Odustani   F3 = Iza\304\221i",
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

static MUI_ENTRY hrHRRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " instalacija ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalacijski program a\305\276urira konfiguraciju sustava.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Stvaranje registry klju\304\215eva",
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

MUI_ERROR hrHRErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Uspje\305\241na instalacija ReactOS-a\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS nije u potpunosti instaliran na Va\305\241em\n"
        "ra\304\215unalu. Ako sada iza\304\221ete iz Instalacijskog programa, trebati \304\207ete ponovno\n"
        "pokrenuti Instalacijski program da biste instalirali ReactOS.\n"
        "\n"
        "  \x07  Pritisnite ENTER da nasatvite.\n"
        "  \x07  Pritisnite F3 da iza\304\221ete iz Instalacijskog programa.",
        "F3 = Quit  ENTER = Continue"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Neuspje\305\241nja gradnja instalacijskih putanja za ReactOS instalacijski direktorij!\n"
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_SOURCE_PATH
        "Ne mo\305\276ete izbrisati particiju koja sadr\305\276i instalaciju!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_DIR
        "You cannot install ReactOS within the installation source directory!\n"
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_NO_HDD
        "Instalacijski program ne mo\305\276e prona\304\207i tvrdi disk.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Instalacijski program nije mogap prona\304\207i izvorni disk.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Instalacijski program ne mo\305\276e prona\304\207i datoteku TXTSETUP.SIF.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Instalacijski program je prona\305\241ao korumpiranu datoteku TXTSETUP.SIF.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Instalacijski program je na\305\241ao neva\305\276e\304\207i potpis (signaturu) u datoteci TXTSETUP.SIF.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Instalacijski program nije mogao dohvatiti informacije o pogonu sustava.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_WRITE_BOOT,
        "Instalacijski program je neuspje\305\241no instalirao %S bootcode na sistemsku particiju.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Instalacijski program je neuspje\305\241no u\304\215itato popis vrste ra\304\215unala.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Instalacijski program je neuspje\305\241no u\304\215itato popis postavki zaslona.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Instalacijski program je neuspje\305\241no u\304\215itato popis vrste tipkovnice.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Instalacijski program je neuspje\305\241no u\304\215itato popis rasporeda tipkovnice.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_WARN_PARTITION,
        "Instalacijski program je otkrio da barem jedan tvrdi disk sadr\305\276i nekompatibilanu\n"
        "particijsku tablicu s kojim se ne mo\305\276e pravilno postupati!\n"
        "\n"
        "Stvaranje or ili brisanje particija mo\305\276e uni\305\241titi particijsku tablicu.\n"
        "\n"
        "  \x07  Pritisnite F3 da iza\304\221ete iz Instalacijskog programa.\n"
        "  \x07  Pritisnite ENTER da nastavite.",
        "F3 = Iza\304\221i  ENTER = Nastavi"
    },
    {
        // ERROR_NEW_PARTITION,
        "Ne mo\305\276ete stvoriti novu particiju unutar\n"
        "postoje\304\207e particije!\n"
        "\n"
        "  * Pritisnite bilo koju tipku za nastavak.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "Ne mo\305\276ete obriati neparticionirani diskovni prostor!\n"
        "\n"
        "  * Press any key to Pritisnite bilo koju tipku za nastavak.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Instalacijski program je neuspje\305\241no instalirao %S bootcode na sistemsku particiju.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_NO_FLOPPY,
        "Nema diska (diskete) u Disku A:",
        "ENTER = Nastavi"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Instalacijski program je neuspje\305\241no a\305\276urirao popis rasporeda tipkovnice.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Instalacijski program je neuspje\305\241no a\305\276urirao popis postavki zaslona registry-a.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Instalacijski program je neuspje\305\241no uvezao hive datoteku (hive file).",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Setup failed to initialize the registry.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Cabinet nema valjane inf datoteke.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cabinet nije prona\304\221en.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Kabinet (Cabinet) nema setup skriptu.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_COPY_QUEUE,
        "Instalacijski program je neuspje\305\241no otvorio kopiju datoteke queue.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_CREATE_DIR,
        "Instalacijski program ne mo\305\276e stvoriti instalacijske direktorije.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Instalacijski program ne mo\305\276e na\304\207i '%S' sekciju\n"
        "u TXTSETUP.SIF.\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_CABINET_SECTION,
        "Instalacijski program ne mo\305\276e na\304\207i '%S' sekciju\n"
        "u kabinetu (cabinet).\n",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Instalacijski program ne mo\305\276e stvoriti instalacijski direktorij.",
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Instalacijski program je neuspje\305\241no upsiao u particijsku tablicu.\n"
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Instalacijski program nije uspjelo dodati kodnu stranicu u registry.\n"
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Instalacijski program nije mogao postaviti lokalizaciju sustava.\n"
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Instalacijski program nije mogao postaviti raspored tipkovnice u registry.\n"
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Instalacijski program nije mogao postaviti GEO ID (geografski polo\305\276aj).\n"
        "ENTER = Ponovno pokreni ra\304\215unalo"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Neva\305\276e\304\207e ime direktorija.\n"
        "\n"
        "  * Pritisnite bilo koju tipku da biste nastavili."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Odabrana particija nije dovoljno velika za instalaciju ReactOS-a.\n"
        "Instalacijska partcija mora biti velika minimalno %lu MB.\n"
        "\n"
        "  * Pritisnite bilo koju tipku da biste nastavite.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Ne mo\305\276ete stvoriti primarnu ili pro\305\241irenu particiju u\n"
        "tablici particija diska jer je tablica particija puna.\n"
        "\n"
        "  * Pritisnite bilo koju tipku da biste nastavite."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Ne mo\305\276te stvoriti vi\305\241e od jedne pro\305\241irene particije po disku.\n"
        "\n"
        "  * Pritisnite bilo koju tipku da biste nastavite."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Instalacijski program ne mo\305\276e foramtirati:\n"
        " %S\n"
        "\n"
        "ENTER = Ponovno pokrenite ra\304\215unalo"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE hrHRPages[] =
{
    {
        SETUP_INIT_PAGE,
        hrHRSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        hrHRLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        hrHRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        hrHRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        hrHRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        hrHRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        hrHRRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        hrHRUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        hrHRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        hrHRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        hrHRFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        hrHRSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        hrHRChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        hrHRConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        hrHRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        hrHRFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        hrHRCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        hrHRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        hrHRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        hrHRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        hrHRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        hrHRKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        hrHRBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        hrHRLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        hrHRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        hrHRSuccessPageEntries
    },
    {
        BOOT_LOADER_INSTALLATION_PAGE,
        hrHRBootLoaderInstallPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        hrHRBootPageEntries
    },
    {
        REGISTRY_PAGE,
        hrHRRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING hrHRStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Molimo pri\304\215ekajte..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instaliraj   P = Stvori Primarnu   E = Stvori Pro\305\241irenu   F3 = Iza\304\221i"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instaliraj   L = Stvori Logi\304\215ku particiju  F3 = Iza\304\221i"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instaliraj   D = Izbri\305\241i particiju   F3 = Iza\304\221i"},
    {STRING_DELETEPARTITION,
     "   D = Izbri\305\241i particiju   F3 = Iza\304\221i"},
    {STRING_PARTITIONSIZE,
     "Veli\304\215ina nove particije:"},
    {STRING_CHOOSENEWPARTITION,
     "Odabrali ste stvoriti primarnu particiju na"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Odabrali ste stvoriti pro\305\241irenu particiju na"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Odabrali ste stvoriti logi\304\215ku particiju na"},
    {STRING_HDDSIZE,
    "Molimo da unesite veli\304\215inu nove particije u MB."},
    {STRING_CREATEPARTITION,
     "   ENTER = Stvori particiju   ESC = Odustani   F3 = Iza\304\221i"},
    {STRING_PARTFORMAT,
    "Ova \304\207e particija biti sljede\304\207a formatirana."},
    {STRING_NONFORMATTEDPART,
    "Odabrali ste instalirati ReactOS na novu ili neformatiranu particiju."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Sistemska particija nije jo\305\241 formatirana."},
    {STRING_NONFORMATTEDOTHERPART,
    "Nova partcija nije jo\305\241 formatirana."},
    {STRING_INSTALLONPART,
    "Instalacijski program instalirava ReactOS na particiju"},
    {STRING_CONTINUE,
    "ENTER = Nastavi"},
    {STRING_QUITCONTINUE,
    "F3 = Iza\304\221i  ENTER = Nastavi"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Ponovno pokreni ra\304\215unalo"},
    {STRING_DELETING,
     "   Brisanje datoteka: %S"},
    {STRING_MOVING,
     "   Premje\305\241tanje datoteka: %S u: %S"},
    {STRING_RENAMING,
     "   Preimenovanje datoteka: %S u: %S"},
    {STRING_COPYING,
     "   Kopiranje datoteka: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Instalacijski program kopira datoteke..."},
    {STRING_REGHIVEUPDATE,
    "   A\305\276uriranje registry hives..."},
    {STRING_IMPORTFILE,
    "   Uvoz %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   A\305\276uriranje registry postavke zaslona..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   A\305\276uriranje postavki lokalizacije..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   A\305\276uriranje postavki rasporeda tipkovnice..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Dodavanje codepage inforamcija u registry..."},
    {STRING_DONE,
    "   Gotovo..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Ponvono pokreni ra\304\215uanlo"},
    {STRING_REBOOTPROGRESSBAR,
    " Va\305\241e \304\207e se ra\304\215unalo automatski ponovno pokrenuti u %li sekundi/e/u... "},
    {STRING_CONSOLEFAIL1,
    "Neuspje\305\241no otvaranje konzole (console)\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Naj\304\215e\305\241\304\207i uzrok tome je kori\305\241tenje USB tipkovnice\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB tipkovnice nisu jo\305\241 potupuno podr\305\276ane\r\n"},
    {STRING_FORMATTINGDISK,
    "Instalacijski program formatira va\305\241 disk"},
    {STRING_CHECKINGDISK,
    "Instalacijski sustav provjerava va\305\241 disk"},
    {STRING_FORMATDISK1,
    " Formatiraj particiju kao %S sustav datoteka (brzo formatiranje) "},
    {STRING_FORMATDISK2,
    " Formatiraj particiju kao %S sustav datoteka "},
    {STRING_KEEPFORMAT,
    " \304\214uvaj trenutni sustav datoteka (bez promjene) "},
    {STRING_HDINFOPARTCREATE_1,
    "%s."},
    {STRING_HDINFOPARTDELETE_1,
    "na %s."},
    {STRING_PARTTYPE,
    "Vrsta 0x%02x"},
    {STRING_HDDINFO_1,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDDINFO_2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Instalacijski program je stvorio novu particiju na"},
    {STRING_UNPSPACE,
    "neparticiranom prostoru"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Pro\305\241irena Particija"},
    {STRING_UNFORMATTED,
    "Nova (neformatirana)"},
    {STRING_FORMATUNUSED,
    "Nekori\305\241tena"},
    {STRING_FORMATUNKNOWN,
    "Nepozanto"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Dodavanje rasporeda tipkovnice"},
    {0, 0}
};
