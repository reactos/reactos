/* ªtefan Fulea (stefan dot fulea at mail dot md) */
#pragma once

MUI_LAYOUTS roROLayouts[] =
{
    { L"0418", L"00000418" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY roROLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Selecþie limbã",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Selectaþi limba pentru procesul de instalare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Apoi apãsaþi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Aceasta va fi în final limba implicitã pentru tot sistemul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare  F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROWelcomePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Bun venit la instalarea ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Aceastã primã etapã din instalarea ReactOS va copia fiºierele",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "necesare în calculatorul dumneavoastrã ºi-l va pregãti pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "cea de-a doua etapã a instalãrii.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "\x07  Apãsaþi ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Press R to repair a ReactOS installation using the Recovery Console.",
        TEXT_STYLE_NORMAL
    },
    // {
        // 8,
        // 19,
        // "   a actualiza ReactOS.",
        // TEXT_STYLE_NORMAL
    // },
    {
        8,
        21,
        "\x07  Tastaþi L pentru Termenii ºi Condiþiile de Licenþiere.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Apãsaþi F3 pentru a ieºi fãrã a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        26,
        "Pentru mai multe informaþii despre ReactOS, vizitaþi:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        27,
        "http://www.reactos.org",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "ENTER = Continuare  R = Refacere  L = Licenþã  F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROIntroPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Programul curent de instalare este încã într-un stadiu primar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "de dezvoltare ºi nu conþine toate funcþionalitãþile unui",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "program de instalare complet.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Sunt aplicabile urmãtoarele limitãri:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Programul curent de instalare poate opera doar cu sisteme",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  de fiºiere FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Verificãrile de integritate pentru fiºiere nu sunt",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  încã implementate.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "\x07  Apãsaþi ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        28,
        "\x07  Apãsaþi F3 pentru a ieºi fãrã a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieºire",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licenþiere:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "Sistemul de operare ReactOS este oferit în termenii Licenþei",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "Publice Generale GNU, referitã în continuare ca GPL, cu pãrþi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "de cod din alte licenþe compatibile (ca X11, BSD, ºi LGPL).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Toate componentele care fac parte din sistemul ReactOS sunt",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "prin urmare oferite sub licenþa GPL, menþinându-ºi astfel ºi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "licenþierea originalã în acelaºi timp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "Acest sistem vine fãrã vreo restricþie de utilizare, aceasta",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "fiind o condiþie legislativã aplicabilã atât la nivel local",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "cât ºi internaþional. Licenþierea se referã doar la distri-",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "buirea sistemului ReactOS cãtre pãrþi terþe.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "Dacã din vreun careva motiv nu deþineþi o copie a licenþei",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "GPL împreunã cu ReactOS, o puteþi consulta (în englezã)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "accesând pagina:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        27,
        "Garanþie:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        29,
        "Acest sistem de operare este distribuit doar în speranþa cã",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        30,
        "va fi util, neavând însã ataºatã NICI O GARANÞIE; nici mãcar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        31,
        "garanþia implicitã a VANDABILITÃÞII sau a UTILITÃÞII ÎNTR-UN",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        32,
        "SCOP ANUME.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Revenire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roRODevicePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Configurare dispozitive de bazã",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Arh. de calcul:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Parametri grafici:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Model tastaturã:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Aranj. tastaturã:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Acceptã:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16,
        "Accept configuraþia dispozitivelor",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Puteþi modifica starea curentã. Utilizaþi tastele SUS/JOS pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "alegerea unui dispozitiv, apoi apãsaþi ENTER pentru a-i modifica",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "configuraþia ataºatã.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Când configuraþia dispozitivele enumerate este cea corectã,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "selectaþi \"Accept configuraþia dispozitivelor\", apoi confirmaþi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        25,
        "apãsând ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROUpgradePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY roROComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Doriþi specificarea unei alte arhitecturi de calcul?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaþi tastele SUS/JOS pentru a selecta o",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   arhitecturã de calcul, apoi apãsaþi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apãsaþi ESC pentru a reveni la pagina precedentã",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   fãrã a specifica o altã arhitecturã de calcul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROFlushPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Sistemul verificã integritatea datelor scrise pe disc.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Aceasta poate dura câteva momente.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "La final, calculatorul va fi repornit automat.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Eliberare memorie...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROQuitPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS nu a fost instalat în întregime.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Scoateþi discul flexibil din unitatea A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "ºi toate mediile CD-ROM din unitãþile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Apãsaþi ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Aºteptaþi...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roRODisplayPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Doriþi modificarea parametrilor grafici de afiºare?",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  Utilizaþi tastele SUS/JOS pentru a selecta",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   un grup de parametri, apoi apãsaþi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apãsaþi ESC pentru a reveni la pagina precedentã",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   fara a modifica parametrii grafici actuali.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROSuccessPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Componentele de bazã ale ReactOS au fost instalate cu succes.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Scoateþi discul flexibil din unitatea A: ºi toate mediile",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "CD-ROM din unitãþile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Apãsaþi ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Repornire calculator",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROBootPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Programul de instalare nu poate instala modulul de",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "iniþializare a calculatorului pe discul local.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Introduceþi un disc flexibil formatat în",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "unitatea A: apoi sã apãsaþi ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY roROSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Urmãtoarea listã cuprinde partiþiile existente, precum ºi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "spaþiul liber disponibil pentru crearea de noi partiþii.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Utilizaþi tastele SUS/JOS pentru a selecta o opþiune.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apãsaþi ENTER pentru a instala pe partiþia selectatã.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tastaþi P pentru a crea o partiþie primarã.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tastaþi E pentru a crea o partiþie extinsã.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tastaþi L pentru a crea o partiþie logicã.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tastaþi D pentru a ºterge o partiþie existentã.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Aºteptaþi...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Aþi solicitat ºtergerea partiþiei de sistem.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Partiþiile de sistem pot conþine programe de diagnozã, programe de con-",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "figurare a echipamentelor, programe de lansare a unui sistem de operare",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "(ca ReactOS) ºi alte programe furnizate de producãtorii calculorului.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "ªtergeþi o partiþie de sistem doar când sunteþi siguri cã nu existã aºa",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "programe pe partiþie, sau când sunteþi siguri cã doriþi sã le ºtergeþi.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "ªtergând partiþia apare riscul de a nu mai putea porni calculatorul de",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "pe discul local decât dupã finalizarea instalãrii ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Apãsaþi ENTER pentru a ºterge partiþia de sistem. Vi se va",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   cere sã confirmaþi din nou aceastã ºtergere a partiþiei.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Apãsaþi ESC pentru a reveni la pagina precedentã. Partiþia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   nu va fi ºtearsã.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Urmeazã formatarea partiþiei.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Apãsaþi ENTER pentru a continua.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY roROInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Alegeþi un director de instalare pe partiþia aleasã.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Aici vor fi amplasate fiºierele sistemului ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Puteþi indica un alt director, apãsând BACKSPACE pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "a ºterge caractere, apoi scriind calea directorului unde",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "doriþi sã instalaþi ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROFileCopyEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Are loc copierea de fiºiere în directorul ReactOS specificat.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "(aceasta poate dura câteva momente)",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Aºteptaþi...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROBootLoaderEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalare modul de iniþializare al calculatorului",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Instaleazã iniþializatorul pe discul intern (MBR ºi VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Instaleazã iniþializatorul pe discul intern (doar VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Instaleazã iniþializatorul pe un disc flexibil.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Omite instalarea modulului de iniþializare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Doriþi specificarea modelului tastaturii instalate?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaþi tastele SUS/JOS pentru a selecta un model",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   de tastaturã, apoi apãsaþi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apãsaþi ESC pentru a reveni la pagina precedentã fãrã",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   a schimba modelul tastaturii curente.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roROLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Doriþi specificarea unui aranjament implicit de tastaturã?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaþi tastele SUS/JOS pentru a selecta un aranjament",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    dorit de tastaturã, apoi apãsaþi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apãsaþi ESC pentru a reveni la pagina precedentã fãrã",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   a schimba aranjamentul curent al tastaturii.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY roROPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Au loc pregãtirile necesare pentru copierea de fiºiere.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Listã de fiºiere în curs de creare...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY roROSelectFSEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        14,
        "Alegeþi un sistem de fiºiere din lista de mai jos.",
        0
    },
    {
        8,
        16,
        "\x07  Utilizaþi tastele SUS/JOS pentru a selecta",
        0
    },
    {
        8,
        17,
        "   un sistem de fiºiere.",
        0
    },
    {
        8,
        19,
        "\x07  Apãsaþi ENTER pentru a formata partiþia.",
        0
    },
    {
        8,
        21,
        "\x07  Apãsaþi ESC pentru a alege o altã partiþie.",
        0
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roRODeletePartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Aþi ales sã ºtergeþi partiþia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Tastaþi D pentru a ºterge partiþia.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ATENÞIE: Toate datele de pe aceastã partiþie vor fi pierdute!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Apãsaþi ESC pentru a anula.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = ªtergere partiþie   ESC = Anulare   F3 = Ieºire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roRORegistryEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Are loc actualizarea configuraþiei sistemului.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Registru în curs de creare...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR roROErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Succes\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS nu a fost instalat în totalitate în calculator.\n"
        "Dacã abandonaþi instalarea acum, altã datã, pentru a\n"
        "instala ReactOS, va fi nevoie sã repetaþi toþi paºii.\n"
        "\n"
        "  \x07  Apãsaþi ENTER pentru a continua instalarea.\n"
        "  \x07  Apãsaþi F3 pentru a abandona instalarea.",
        "F3 = Ieºire  ENTER = Continuare"
    },
    {
        //ERROR_NO_HDD
        "Eºec la identificarea unitãþilor interne de stocare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Eºec la accesarea unitatãþii de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Eºec la încãrcarea fiºierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Fiºieul TXTSETUP.SIF a fos gãsit deteriorat.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Fiºierul TXTSETUP.SIF conþine o semnãturã nevalidã.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Eºec la obþinerea de informaþii despre\n"
        "dispozitivele din calculator.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_BOOT,
        "Eºec la instalarea codului FAT de iniþializare\n"
        "pe partiþia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Eºec la încãrcarea listei cu arhitecturi de\n"
        "calcul disponibile.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Eºec la încãrcarea listei cu parametri de\n"
        "afiºare pentru ecran.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Eºec la încãrcarea listei cu tipuri\n"
        "disponibile de tastaturã.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Eºec la încãrcarea listei de configuraþii\n"
        "ale tastaturii.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WARN_PARTITION,
        "A fost gãsitã cel puþin un disc cu tabelã nerecunoscutã\n"
        "de partiþii, care nu poate fi gestionatã corespunzãtor!\n"
        "\n"
        "Crearea sau ºtergerea de partiþii poate astfel cauza\n"
        "distrugerea tabelei de partiþii."
        "\n"
        "  \x07  Apãsaþi F3 pentru a abandona instalarea.\n"
        "  \x07  Apãsaþi ENTER pentru a continua.",
        "F3 = Ieºire  ENTER = Continuare"
    },
    {
        //ERROR_NEW_PARTITION,
        "O partiþie nouã nu poate fi creatã în interiorul\n"
        "unei partiþii existente!\n"
        "\n"
        "  * Tastaþi pentru a continua.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Aþi încercat ºtergerea de spaþiu nepartiþionat,\n"
        "însã doar spaþiul partiþionat poate fi ºters!\n"
        "\n"
        "  * Tastaþi pentru a continua.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Eºec la instalarea codului FAT de iniþializare\n"
        "pe partiþia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_FLOPPY,
        "Nu existã discuri flexibile în unitatea A:",
        "ENTER = Continuare"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Eºec la actualizarea configuraþiei de tastaturã.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Eºec la actualizarea registrului cu\n"
        "parametrii grafici ai ecranului!",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Eºec la importarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_REGISTRY
        "Eºec la localizarea fiºierelor\n"
        "cu datele registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_HIVE,
        "Eºec la crearea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Eºec la iniþializarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Fiºierul cabinet nu conþine nici\n"
        "un fiºier valid de tip inf.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_MISSING,
        "Eºec la localizarea fiºierului cabinet.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Fiºierul cabinet nu conþine nici\n"
        "un script de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_COPY_QUEUE,
        "Eºec la accesarea listei cu\n"
        "fiºiere pentru copiere.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_DIR,
        "Eºec la crearea directoarelor de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Eºec la identificarea secþiunii de\n"
        "directoare în fiºierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SECTION,
        "Eºec la identificarea secþiunii de\n"
        "directoare în fiºierul cabinet.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Eºec la crearea directorului de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Eºec la localizarea secþiunii pentru date\n"
        "de instalare din fiºierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Eºec la scrierea tabelelor de partiþii.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Eºec la includerea paginãrii în registru.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Eºec la instituirea sistemului de localizare.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Eºec la includerea în registru a configuraþiei\n"
        "de tastaturã.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Eºec la instituirea de geo id.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Numele de director este nevalid.\n"
        "\n"
        "  * Tastaþi pentru a continua."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Partiþia selectatã este prea micã pentru a instala ReactOS.\n"
        "Partiþia de instalare trebuie sã aibã cel puþin %lu Mocteþi.\n"
        "\n"
        "  * Tastaþi pentru a continua.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Nu poate fi creatã o nouã partiþie primarã sau extinsã în tabela\n"
        "de partiþii a acestui disc deoarece tabela de partiþii e plinã.\n"
        "\n"
        "  * Tastaþi pentru a continua."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Nu poate fi creatã mai mult de o partiþie extinsã pe un disc.\n"
        "\n"
        "  * Tastaþi pentru a continua."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Eºec la formatarea partiþiei:\n"
        " %S\n"
        "\n"
        "ENTER = Repornire calculator"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE roROPages[] =
{
    {
        LANGUAGE_PAGE,
        roROLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        roROWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        roROIntroPageEntries
    },
    {
        LICENSE_PAGE,
        roROLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        roRODevicePageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        roROUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        roROComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        roRODisplayPageEntries
    },
    {
        FLUSH_PAGE,
        roROFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        roROSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        roROConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        roROSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        roROFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        roRODeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        roROInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        roROPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        roROFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        roROKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        roROBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        roROLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        roROQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        roROSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        roROBootPageEntries
    },
    {
        REGISTRY_PAGE,
        roRORegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING roROStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Aºteptaþi..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalare   P/E = Creare partiþie Primarã/Extinsã   F3 = Ieºire"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalare   L = Creare partiþie Logicã   F3 = Ieºire"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalare   D = ªtergere partiþie   F3 = Ieºire"},
    {STRING_DELETEPARTITION,
     "   D = ªtergere partiþie   F3 = Ieºire"},
    {STRING_PARTITIONSIZE,
     "Mãrimea noii partiþii:"},
    {STRING_CHOOSENEWPARTITION,
     "Aþi ales crearea unei partiþii primare pe"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Aþi ales crearea unei partiþii extinse pe"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Aþi ales crearea unei partiþii logice pe"},
    {STRING_HDDSIZE,
    "Introduceþi mãrimea noii partiþii în megaocteþi."},
    {STRING_CREATEPARTITION,
     "   ENTER = Creare partiþie   ESC = Anulare   F3 = Ieºire"},
    {STRING_PARTFORMAT,
    "Aceastã partiþie urmeazã sã fie formatatã."},
    {STRING_NONFORMATTEDPART,
    "Alegeþi sã instalaþi ReactOS pe partiþie nouã sau neformatatã."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Partiþia de sistem încã nu a fost formatatã."},
    {STRING_NONFORMATTEDOTHERPART,
    "Noua partiþie încã nu a fost formatatã."},
    {STRING_INSTALLONPART,
    "ReactOS va fi instalat pe partiþia"},
    {STRING_CHECKINGPART,
    "Programul de instalare verificã acum partiþia aleasã."},
    {STRING_CONTINUE,
    "ENTER = Continuare"},
    {STRING_QUITCONTINUE,
    "F3 = Ieºire  ENTER = Continuare"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Repornire calculator"},
    {STRING_TXTSETUPFAILED,
    "Nu s-a reuºit gãsirea sesiunii\n'%S' în TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Fiºierul curent: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Fiºiere în curs de copiere..."},
    {STRING_REGHIVEUPDATE,
    "   Registru în curs de actualizare..."},
    {STRING_IMPORTFILE,
    "   În curs de importare din %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Registru de configuraþie graficã în actualizare..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Particularitãþi locale în actualizare..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Configuraþie de tastaturã în actualizare..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Date de paginare în curs de adaugare în registru..."},
    {STRING_DONE,
    "   Terminat!"},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Repornire calculator"},
    {STRING_CONSOLEFAIL1,
    "Eºec la deschiderea consolei\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Cea mai frecventã cauzã pentru asta este utilizarea unei tastaturi USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Tastaturile USB nu sunt încã toate acceptate\r\n"},
    {STRING_FORMATTINGDISK,
    "Disc în curs de formatare..."},
    {STRING_CHECKINGDISK,
    "Disc în curs de verificare..."},
    {STRING_FORMATDISK1,
    " Formateazã partiþia ca sistem de fiºiere %S (formatare rapidã) "},
    {STRING_FORMATDISK2,
    " Formateazã partiþia ca sistem de fiºiere %S "},
    {STRING_KEEPFORMAT,
    " Pãstreazã sistemul de fiºiere actual (fãrã schimbãri) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu) de tip %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tip 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "de pe %I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu) de tip %wZ."},
    {STRING_HDDINFOUNK3,
    "de pe %I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Discul %lu (%I64u %s), Port=%hu, Magistrala=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tip 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "de pe Discul %lu (%I64u %s), Port=%hu, Magistrala=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTip %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu) de tip %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "O nouã partiþie a fost creatã în"},
    {STRING_UNPSPACE,
    "    %sSpaþiu nepartiþionat%s           %6lu %s"},
    {STRING_MAXSIZE,
    "Mo (max. %lu Mo)"},
    {STRING_EXTENDED_PARTITION,
    "Partiþie extinsã"},
    {STRING_UNFORMATTED,
    "Part. nouã (neformatatã)"},
    {STRING_FORMATUNUSED,
    "Nefolosit"},
    {STRING_FORMATUNKNOWN,
    "Necunoscut"},
    {STRING_KB,
    "ko"},
    {STRING_MB,
    "Mo"},
    {STRING_GB,
    "Go"},
    {STRING_ADDKBLAYOUTS,
    "Adãugare configuraþii de tastaturã"},
    {0, 0}
};
