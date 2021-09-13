// uSetup in Basque-Euskara Translated by Julen Urizar Compains (04-26-2020)
#pragma once

static MUI_ENTRY euESSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Itxaron mesedez ReactOS Instalazioa hasten ari dela",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "eta aurkitzen dute zure hardware tresnak...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Itxaron mesedez...",
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

static MUI_ENTRY euESLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Hizkuntz aukeraketa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Hautatu instalazio hizkuntza, mesedez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Gero sakatu SARTU.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Hizkuntz hau izango da sistemaren amaierako hizkuntza.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu  F3 = Irten",
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

static MUI_ENTRY euESWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Ongi etorri ReactOS Insalazioa",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Parte honetan Instalazioak kapiatu du ReactOS Sistema Eragilea zure",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "ordenagailuan eta instalazioen bigarren parte prestatzen du.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Sakatu SARTU ReactOS instalatzeko edo hobetzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
     // "\x07  Sakatu R ReactOS instalazioa kompontzeko berreskuragailu erabiltzen.",
        "\x07  Sakatu R ReactOS instalazioa kompontzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Sakatu L ReactOS Lizentziako terminoak eta baldintzak irakurtzea.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Sakatu F3 irteteko ReactOS-rik instalatu gabe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "ReactOS agirbide gehiago nahi baduzun, webgune hau ikusi:",
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
        "SARTU = Jarraitu  R = Kompondu  L = Lizentzia  F3 = Irten",
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

static MUI_ENTRY euESIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS bertsio egoera",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS Alpha mailan dago, honek esan nahi du ReactOS ez da",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "bukatuta eta dago garapen handi aurrean. Komeni da erabilzea bakarrik",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "ebaluazioa edo proba egin helburua, ez egunero Sistema Eragilea.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Ekuskarri fisikon instalatu baino lehen Segurtasun-kopia egin zure datuak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "gordetzeko edo proba egin beste ordenagailu sekundarioa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Sakatu SARTU ReactOS instalazio jarraitzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Sakatu F3 irteteko ReactOS-rik instalatu gabe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   F3 = Irten",
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

static MUI_ENTRY euESLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Lizentzia:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS Sistema lizentziatuta dago GNU GLP-aren lizentzia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL X11 edo BSD eta GNU LGPL lizentziak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "bateragarriak diren kodeekin osatutako zatiak.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Software denak dagoen ReactOS sisteman partez atera dira",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "GNU GPL lizenziaren barruan eta denek bere lizenzia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "mantendu ditu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Software hau ez du BERMERIK edo erabilpen murrizketarik",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "lokala edo internazional legeak izan ezik. ReactOS lizentzia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "bakarrik hirugarrenei banaketa estaltzen die.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Ez badaukazu GNU Lizentzia Publiko Orokorra-ren kopia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "zure ReactOS-ekin mesedez bisitatu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "https://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "Bermeak:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Hau software librea da; irakurri kodea kopiaren baldintzak.arte",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "EZ dago BERMERIK; ezetz MERCANTARITZA edo",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "PARTIKULAR NAHI BETETZE",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Atzera",
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

static MUI_ENTRY euESDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Ondoren zerrenda erakutsi dizkizu zure gailuen/dispositiboen konfigurazioa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Ekipo (PUZ):",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Monitorea:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Teklatua:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Teklatu distribuzioa:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Onartu:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Onartu gailuen konfigurazioa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Aldatu dispositiboak sakatu GORA edo BEHERA teklak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "zerbait seinalatzea. Gero sakatu SARTU tekla hautabide",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "konfigurazio aukeratzea.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Hoberen konfigurazio hartuko duzuenean, aukeratu ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\"Onartu gailuen konfigurazioa\" eta SARTU sakatu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   F3 = Irten",
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

static MUI_ENTRY euESRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Instalazioa hasierako garapen zatian dago. Oraindik ez du",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "eusgarri funtzio osoan instalatzio apliketako  of a fully usable setup application.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Zuzentzaile funtzioak ez daude artean ezarrita.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Sakatu U SO-a eguneratuko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Sakatu R Errekuperazio Konsola zabaltzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Sakatu ESC orri printzipala itzultzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Sakatu SARTU zure ordenagailua berrabiarazi.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Orri printzipala  U = Eguneratu  R = Errekuperazio  SARTU = Berrabiarazi",
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

static MUI_ENTRY euESUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Instalazioak hurrengo zerrendaren ReactOS sistema bakoitz",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "hobetu ahal du edo, ReactOS sistema apurtuta badago, Instalazioak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "saiatuko badu zuzendu egin.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Oraindik zuzendu funtzioak ez daude ezarrita.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Sakatu GORA edo BEHERA SO instalazio bat aukeratzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Sakatu U SO instalazio aukeratua hobetzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Sakatu ESC instalazio berria jarraitzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Sakatu F3 irteteko ReactOS-rik instalatu gabe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Hobetu   ESC = Ez hobetu   F3 = Irten",
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

static MUI_ENTRY euESComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zu ordenagailu-mota aldatu nahi duzu instalatzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Sakatu GORA edo BEHERA tekla ordenagailu-mota bat aukeratzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Gero sakatu SARTU.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Sakatu ESC tekla aurreko orria itzuliko ordenagailu-mota",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   aldaketarik gabe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   ESC = Utzi   F3 = Irten",
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

static MUI_ENTRY euESFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Orain sistema ziurtatzen ari da data guztiak diskon bilduta daude.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Minutu bat iraun dezake.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Bukatutakoan, zure ordenagailua berrabiatu da automatikoki.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Katxea garbitzen ari da",
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

static MUI_ENTRY euESQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS ez dago zeharo instalatuta.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Kendu diskete A: unitatetik eta",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "CD-ROM denak CD-unitate dauden.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Sakatu SARTU berrabiatuko zure ordenagailua.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Itxaron mesedez...",
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

static MUI_ENTRY euESDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zu pantaila-mota aldatu nahi duzu instalatzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Sakatu GORA edo BEHERA tekla pantaila-mota bat aukeratzeko.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Gero sakatu SARTU.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Sakatu ESC tekla aurreko orria itzuliko pantaila-mota",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   aldaketarik gabe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   ESC = Utzi   F3 = Irten",
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

static MUI_ENTRY euESSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS oinarrizko zatiak arrakaztaz instalatuta da.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Kendu diskete A: unitatetik eta",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "CD-ROM denak CD-unitate dauden.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Sakatu SARTU berrabiatuko zure ordenagailua.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = berrabiatu ordenagailua",
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

static MUI_ENTRY euESBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalazio ez du ahalmenik bootloader-a instalatu zure ordenagailuaren",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "disko gogorra",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Mesedez sartu diskete bat formateatuta A: unitaten barnean",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "eta sakatu SARTU.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   F3 = Irten",
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

static MUI_ENTRY euESSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Hurrengo zerrenda erakutsiko dizkizu oraingo partizioak eta ez higatu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "partizio berriak diskoen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Sakatu GORA edo BEHERA zerrendaren hauta aukeratzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Sakatu SARTU ReactOS instalatzeko partizio aukeratuta barruan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Sakatu P partizio nagusi bat sortzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Sakatu E partizio luzatuta bat sortzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Sakatu L partizio logika bat sortzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Sakatu D dagoen partizio kentzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Itxaron mesedez...",
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

static MUI_ENTRY euESChangeSystemPartition[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zure oraingo ordenagailuaren sistemaren partizioa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "sistemaren diskon",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "formatok ReactOS-k ez da euskarri erabiltzen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "ReactOS arrakastaz instalatzeko, Instalazio programa aldatu behar du",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "oraingo partizio sistema berrien bat.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Sistema partizioa hautagai berria izango da:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Aukeraketa onartzeko sakatu SARTU.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Eskuz aldatzeko sistemaren partizioa, sakatu ESC partizio aukeraketa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "    zerrenda aurreko orria itzuliko, gero beste sistema aukeratu edo partizio",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   berria sortu sistemaren diskon.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "Beste sistema operatibo-ek badaude eta partizioren sistema originalen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "beharra badu, berrekonfiguratu behar bazara berri partizioren sistemarantz",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "edo zuk aldatu behar baduzu lehenaren partiziotik jatorrizkotara",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "ReactOS instalazioa bukatuz gero.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   ESC = Utzi",
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

static MUI_ENTRY euESConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zuk sistema partizioa ezabatuko aukera hautatu duzu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Sistema partizioak diagnostiko programak, hardware konfig., programak,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programak sistema operatibo hasten direna (ReactOS bezala) edo beste",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "hornitzailearen hardware programak eman direna eduki dezakete.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Bakarrik ezabatu sistema partizioa ziur zaudenean ez dauden programarik",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "importantea partizionen, edo noizbait ziur zaude ezabatu nahi dituzula.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Partizioa ezabatu duzuenean, zu ez ahal baduzu hasiera ordenagailua",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "diskogogorretik ReactOS Instalazioa amaitu arte.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Sakatu SARTU sistema partizioa ezabatuko. Gero berriro",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   galdetuko dizu partizio ezabatzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Sakatu ESC atzeko orrialdera itzultzeko. Partizioa ez",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   izango da ezabatuko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU=Jarraitu  ESC=Utzi",
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

static MUI_ENTRY euESFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Partizio formatua",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Orain instalazioa formatuko du partizioa. Sakatu SARTU jarraitzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "SARTU = Jarraitu   F3 = Irten",
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

static MUI_ENTRY euESCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalazioa ikusi du partizio aukertatutan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Itxaron mesedez...",
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

static MUI_ENTRY euESInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalazioa partizioari fitxategiak sartu dizkio. Hautatu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "direktorioa zu nahi duzun instalatzeko:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Proposatutako direktorioa aldatzeko, sakatu ATZERESABATU hitzi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "ezabatzeko eta idatzi direktorioa zu ReactOS nahi duzun",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "instalatuta.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   F3 = Irten",
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

static MUI_ENTRY euESFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Itxaron mesedez ReactOS Instalazioa fitxategiak kopiatuen bitartean ReactOS-n",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "karpeta.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Minutu batzuk itxaron behar duzu osatuko.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Itxaron mesedez...    ",
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

static MUI_ENTRY euESBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalazioak bootloader-a sartzen ari du",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Instalatu bootloader disko gogor barruan (MBR eta VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Instalatu bootloader disko gogor barruan (bakarrik VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Instalatu bootloader diskete barruan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Jautzi bootloader instalazioa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   F3 = Irten",
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

static MUI_ENTRY euESBootLoaderInstallPageEntries[] =
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

static MUI_ENTRY euESKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Zu teklatu-mota aldatu nahi duzu instalatzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Sakatu the GORA edo BEHERA tekla teklatu-mota bat aukeratzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Gero sakatu SARTU.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Sakatu ESC tekla aurreko orria itzuliko teklatu-mota",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   aldaketarik gabe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   ESC = Utzi   F3 = Irten",
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

static MUI_ENTRY euESLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Aukeratu teklatu lehentsi banaketa instalatzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Sakatu the GORA edo BEHERA tekla teklatu-banaketa bat ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    aukeratzeko. Gero sakatu SARTU.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Sakatu ESC tekla aurreko orria itzuliko teklatu-banaketa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   aldaketarik gabe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "SARTU = Jarraitu   ESC = Utzi   F3 = Irten",
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

static MUI_ENTRY euESPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalazioak zure ordenagailua antolatu du ReactOS fitxategiak kopiatzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Fitxategi-kopia serrendan eratzen ari da...",
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

static MUI_ENTRY euESSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Aukeratu hurrengo serrendaren sistema fitxategi bat.",
        0
    },
    {
        8,
        19,
        "\x07  Sakatu GORA edo BEHERA sistema fitxategi aukeratzeko.",
        0
    },
    {
        8,
        21,
        "\x07  Sakatu SARTU partizioa formateatzeko.",
        0
    },
    {
        8,
        23,
        "\x07  Sakatu ESC beste partizio aukeratzeko.",
        0
    },
    {
        0,
        0,
        "SARTU = Jarraitu   ESC = Utzi   F3 = Irten",
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

static MUI_ENTRY euESDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Haukeratu duzu partizioa formateatzeko",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Sakatu L partizioa ezabatzeko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "KONTUZ: Edozein data partizio honetan dagoen galduko dira!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Sakatu ESC utziko.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Delete Partition   ESC = Utzi   F3 = Irten",
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

static MUI_ENTRY euESRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalazioa ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalazioa sistemaren konfigurazioa eguneratzen ari du.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Erregistro herlauntzak sortzen...",
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

MUI_ERROR euESErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Arrakasta\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS ez dago instalatuta guztiz zure\n"
        "ordenagailuan. Orain instalazioa utzi baduzu, zuk nahiago\n"
        "duzu Instalazioa berriro hasi ReactOS instalatzeko.\n"
        "\n"
        "  \x07  Sakatu SARTU Instalazioa jarraitzeko.\n"
        "  \x07  Sakatu F3 Instalazioa utzi.",
        "F3 = Irten  SARTU = Jarraitu"
    },
    {
        // ERROR_NO_BUILD_PATH
        "ReactOS instalazio direktorio bide eraikitzea huts egin da!\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_SOURCE_PATH
        "Ez ahal duzu partizioa ezabatzeko instalazio baliabidea badauka!\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_SOURCE_DIR
        "Ez ahal duzu ReactOS instalatzeko instalazio direktorion barruan!\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_NO_HDD
        "Instalazioa disko gogorra aurkitu ez ahal du.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Instalazioa ez aurkitu ahal du unitate baliabidea.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Instalazioa huts egin du TXTSETUP.SIF fitxategia kargatu.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Instalazioa aurkitu du TXTSETUP.SIF fitxategia usteltuta.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Instalazioa aurkitu du oker sinadura TXTSETUP.SIF barruan.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Instalazioa ez ahal du berreskuratu sistemaren unitate informazioa.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_WRITE_BOOT,
        "Instalazioa huts egin du bootcode instalatzeko %S sistemaren partizioan.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Instalazioa huts egin du ordenagailu-motaren zerrenda kargatzen.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Instalazioa huts egin du pantaila-motaren zerrenda kargatzen.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Instalazioa huts egin du teklatu-motaren zerrenda kargatzen.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Instalazioa huts egin du teklatu-distribuzioa zerrenda kargatzen.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_WARN_PARTITION,
        "Instalazioak aurkitu du disko gogorran partizio taula ez da bateragarria\n"
        "gutxienez. Ez ahal du zuzen erabili!\n"
        "\n"
        "Partizioak sortu edo ezabatu partizioren taula suntsitu ahal du.\n"
        "\n"
        "  \x07  Sakatu F3 Instalazio irten.\n"
        "  \x07  Sakatu SARTU jarraitzeko.",
        "F3 = Irten  SARTU = Jarraitu"
    },
    {
        // ERROR_NEW_PARTITION,
        "Ez ahal duzu partizioa sortzeko beste\n"
        "orain partizioaren barruan izan den!\n"
        "\n"
        "  * Sakatu tekla bat jarraitzeko.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "Ez ahal duzu diskoaren ez-partiziotatu ezpazioa ezabatzeko!\n"
        "\n"
        "  * Sakatu tekla bat jarraitzeko.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Instalazioa huts egin du bootcode instalatzeko %S sistemaren partizioan.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_NO_FLOPPY,
        "Ez dago disko A: unitatean.",
        "SARTU = Jarraitu"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Instalazioa huts egin du teklatuaren distribuzio konfigurazioak.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Instalazioa huts egin du pantaila registru konfigurazioak eguneratuko.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Instalazioa huts egin du erlauntz fitxatekia inportatu.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_FIND_REGISTRY
        "Instalazioa huts egin du registro datak fitxategi aurkitzea.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_CREATE_HIVE,
        "Instalazioa huts egin du registro erlauntzak sortzea.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Instalazioa huts egin du registro hasierako.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_INVALID_Kabinete_INF,
        "Kabinete ez du baliozko inf fitxategia.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_Kabinete_MISSING,
        "Kabinete ez du aurkitzen.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_Kabinete_SCRIPT,
        "Kabinete ez du instalazio script-a.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_COPY_QUEUE,
        "Instalazioa huts egin du fitxategi kopia lerroa irekitzea.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_CREATE_DIR,
        "Instalazioa ez ahal du instalazio direktorioak sortzen.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Instalazioa huts egin du '%S' sekzioa aurkitzen\n"
        "TXTSETUP.SIF barruan.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_Kabinete_SECTION,
        "Instalazioa huts egin du '%S' sekzioa aurkitzen\n"
        "Kabinete barruan.\n",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Instalazioa ez ahal du instalazio direktorioa sortzeko.",
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Instalazioa huts egin du partizio taulak idazten.\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Instalazioa huts egin du orri-kodea registrora gehitu.\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Instalazioa ez ahal du sistemaren hitzkuntza jarri.\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Instalazioa huts egin du teklatu-distribuzioa registrora gehitu.\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Instalazioa ez ahal du globala ID jartzen.\n"
        "SARTU = Berrabiarazi"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Direktorio izena baliogabe.\n"
        "\n"
        "  * Sakatu tekla batzuk jarraitzeko."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Partizio aukeratuta ez da nahiko haundia ReactOS instalatzeko.\n"
        "Instalatzeko partizioa %lu MB handi gutzienez behar dauka.\n"
        "\n"
        "  * Sakatu tekla batzuk jarraitzeko.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Zu ez ahal du nagusi edo luzatu partizio berria sotzen\n"
        "partizioren taula disko honetan partizio taula beteta dago-eta.\n"
        "\n"
        "  * Sakatu tekla batzuk jarraitzeko."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Zu ez ahal duzu luzatu partizioa bat baino gehiago sotzen disko batzuetan.\n"
        "\n"
        "  * Sakatu tekla batzuk jarraitzeko."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Instalazioa ez ahal du partizioa formateatzea:\n"
        " %S\n"
        "\n"
        "SARTU = Berrabiarazi"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE euESPages[] =
{
    {
        SETUP_INIT_PAGE,
        euESSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        euESLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        euESWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        euESIntroPageEntries
    },
    {
        LICENSE_PAGE,
        euESLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        euESDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        euESRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        euESUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        euESComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        euESDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        euESFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        euESSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        euESChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        euESConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        euESSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        euESFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        euESCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        euESDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        euESInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        euESPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        euESFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        euESKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        euESBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        euESLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        euESQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        euESSuccessPageEntries
    },
    {
        BOOT_LOADER_INSTALLATION_PAGE,
        euESBootLoaderInstallPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        euESBootPageEntries
    },
    {
        REGISTRY_PAGE,
        euESRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING euESStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Itxaron mesedez..."},
    {STRING_INSTALLCREATEPARTITION,
     "   SARTU = Instalatu   P = Sortu Nagusia   E = Sortu Luzatuta   F3 = Irten"},
    {STRING_INSTALLCREATELOGICAL,
     "   SARTU = Instalatu   L = Sortu Partizio Logika   F3 = Irten"},
    {STRING_INSTALLDELETEPARTITION,
     "   SARTU = Instalatu   D = Ezabatu Partitioa   F3 = Irten"},
    {STRING_DELETEPARTITION,
     "   D = Ezabatu Partitioa   F3 = Irten"},
    {STRING_PARTITIONSIZE,
     "Partizio berriaren tamainu:"},
    {STRING_CHOOSENEWPARTITION,
     "Zu aukeratu duzu partizio nagusia sortu barruan"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Zu aukeratu duzu partizio luzatuta sortu barruan"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Zu aukeratu duzu partizio logika sortu barruan"},
    {STRING_HDDSIZE,
    "Mezedez osatu tamainua megabytes-en."},
    {STRING_CREATEPARTITION,
     "   SARTU = Sortu Partizioa   ESC = Utzi   F3 = Irten"},
    {STRING_PARTFORMAT,
    "Partizio hau izango da formatuta gero."},
    {STRING_NONFORMATTEDPART,
    "Zu aukeratu duzu ReactOS instalatzeko partizio berri edo garbi barruan."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Oraindik sistema partizioa ez dago formatuta."},
    {STRING_NONFORMATTEDOTHERPART,
    "Oraindik partizio berria ez dago formatuta."},
    {STRING_INSTALLONPART,
    "Instalazioa ReactOS instalatu du partizio barruan"},
    {STRING_CONTINUE,
    "SARTU = Jarraitu"},
    {STRING_QUITCONTINUE,
    "F3 = Irten  SARTU = Jarraitu"},
    {STRING_REBOOTCOMPUTER,
    "SARTU = Berrabiatu ordenagailua"},
    {STRING_DELETING,
     "   Fixtategia ezabatzen: %S"},
    {STRING_MOVING,
     "   Fixtategia mugitzen: %Sdik %Sra"},
    {STRING_RENAMING,
     "   fitxategia ber-izenatu: %Sdik %Sra"},
    {STRING_COPYING,
     "   Fitxategiak kopiatzen: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Instalazioa fitxategiak kopiatzen ari da..."},
    {STRING_REGHIVEUPDATE,
    "   Registro erlauntzak eguneratzen..."},
    {STRING_IMPORTFILE,
    "   Inportatzen %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Pantaila registro konfigurazioa eguneratzen..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Hizkuntza konfigurazioa eguneratzen..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Teklatu konfigurazioa eguneratzen..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Kodeorri informazioa gehitu da registron..."},
    {STRING_DONE,
    "   Amaitu da..."},
    {STRING_REBOOTCOMPUTER2,
    "   SARTU = Berrabiatu ordenagailua"},
    {STRING_REBOOTPROGRESSBAR,
    " Zure ordenagailua berrabiatuko da %li segundu eta gero... "},
    {STRING_CONSOLEFAIL1,
    "Ez da posible konsola ireki\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Kausa arruntak da USB teklatua erabili\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB teklatua ez dago oraindik osoan euskarri\r\n"},
    {STRING_FORMATTINGDISK,
    "Instalazioa zure disko formateatzen ari da"},
    {STRING_CHECKINGDISK,
    "Instalazioa zure disko ikusten ari da"},
    {STRING_FORMATDISK1,
    " Formatu partizioa %S sistemaren fitxategia (format azkar) "},
    {STRING_FORMATDISK2,
    " Formatu partizioa %S sistemaren fitxategia "},
    {STRING_KEEPFORMAT,
    " Mantendu sistemaren fitxategia (aldaketarik ez) "},
    {STRING_HDINFOPARTCREATE_1,
    "%s."},
    {STRING_HDINFOPARTDELETE_1,
    "%s."},
    {STRING_PARTTYPE,
    "Mota 0x%02x"},
    {STRING_HDDINFO_1,
    // "Disko gogor %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Disko gogor %lu (Port=%hu, Bus=%hu, Id=%hu) %wZn [%s]"},
    {STRING_HDDINFO_2,
    // "Disko gogor %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Disko gogor %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Instalazioa sortu du partizio berria barruan"},
    {STRING_UNPSPACE,
    "Ezpartizionatu espazio"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partizio"},
    {STRING_UNFORMATTED,
    "Berri (Formateorik ez)"},
    {STRING_FORMATUNUSED,
    "Ezerabili"},
    {STRING_FORMATUNKNOWN,
    "Ezdakit"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Teklatu Distribuzioa Gehitu"},
    {0, 0}
};
