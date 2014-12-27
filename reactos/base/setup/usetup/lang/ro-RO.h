/* ¸tefan Fulea (stefan dot fulea at mail dot md) */
#pragma once

MUI_LAYOUTS roROLayouts[] =
{
    { L"0409", L"00000409" },
    { L"0418", L"00000418" },
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
        "Selecîie limbÇ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Selectaîi limba pentru procesul de instalare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Apoi apÇsaîi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Aceasta va fi Œn final limba implicitÇ pentru tot sistemul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare  F3 = Ie­ire",
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
        "AceastÇ primÇ etapÇ din instalarea ReactOS va copia fi­ierele",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "necesare Œn calculatorul dumneavoastrÇ ­i-l va pregÇti pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "cea de-a doua etapÇ a instalÇrii.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "\x07  ApÇsaîi ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Tastaîi R pentru a reface un sistem deteriorat sau pentru",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "   a actualiza ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tastaîi L pentru Termenii ­i Condiîiile de Licenîiere.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  ApÇsaîi F3 pentru a ie­i fÇrÇ a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        26,
        "Pentru mai multe informaîii despre ReactOS, vizitaîi:",
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
        "ENTER = Continuare  R = Refacere  L = LicenîÇ  F3 = Ie­ire",
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
        "Programul curent de instalare este ŒncÇ Œntr-un stadiu primar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "de dezvoltare ­i nu conîine toate funcîionalitÇîile unei",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "aplicaîii de instalare complete.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Sunt aplicabile urmÇtoarele limitÇri:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Programul curent de instalare nu e capabil de mai mult",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  de o partiîie per disc.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Programul curent de instalare nu poate ­terge o partiîie",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  primarÇ cƒt timp existÇ partiîii extinse.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Programul curent de instalare nu poate ­terge prima partiîie",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "  extinsÇ, cƒt timp existÇ alte partiîii extinse.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "- Programul curent de instalare poate opera doar cu sisteme",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "  de fi­iere FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "- VerificÇrile de integritate pentru fi­iere nu sunt",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "  ŒncÇ implementate.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "\x07  ApÇsaîi ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        28,
        "\x07  ApÇsaîi F3 pentru a ie­i fÇrÇ a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie­ire",
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
        "Licenîiere:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "Sistemul de operare ReactOS este oferit Œn termenii GNU GPL,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "cu pÇrîi de cod din alte licenîe compatibile, cum ar fi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenîele X11, BSD ­i GNU LGPL.",
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
        "prin urmare oferite sub licenîÇ GNU GPL, menîinƒndu-­i Œn",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "acela­i timp ­i licenîierea originalÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "Acest sistem vine fÇrÇ vreo restricîie de utilizare, aceasta",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "fiind o condiîie legislativÇ aplicabilÇ atƒt la nivel local",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "cƒt ­i internaîional. Licenîierea se referÇ doar la distri-",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "buirea sistemului ReactOS cÇtre pÇrîi terîe.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "DacÇ din vreun motiv careva nu aîi primit o copie a Licenîei",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "Publice Generale GNU ŒmpreunÇ cu ReactOS, sunteîi rugaîi sÇ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "vizitaîi:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        26,
        "Garanîie:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        28,
        "Acest sistem de operare este distribuit doar Œn speranîa cÇ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        29,
        "va fi util, neavƒnd ŒnsÇ ata­atÇ NICI O GARANõIE; nici mÇcar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        30,
        "garanîia implicitÇ a VANDABILITÆõII sau a UTILITÆõII ×NTR-UN",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        31,
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
        "Configurare dispozitive de bazÇ",
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
        "Model tastaturÇ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        " Aranj. tastaturÇ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "AcceptÇ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Accept configuraîia dispozitivelor",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Puteîi modifica starea curentÇ. Utilizaîi tastele SUS/JOS pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "alegerea unui dispozitiv, apoi apÇsaîi ENTER pentru a-i modifica",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "configuraîia ata­atÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Cƒnd configuraîia dispozitivele enumerate este cea corectÇ,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "selectaîi \"Accept configuraîia dispozitivelor\", apoi confirmaîi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        25,
        "apÇsƒnd ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie­ire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY roRORepairPageEntries[] =
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
        "Programul de instalare ReactOS este ŒncÇ Œntr-o fazÇ incipientÇ de",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "dezvoltare ­i nu posedÇ o funcîionalitate completÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Funcîionalitatea de refacere ŒncÇ nu este implementatÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tastaîi U pentru actualizarea SO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tastaîi R pentru consola de Recuperare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ApÇsaîi ESC pentru a reveni la pagina principalÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ApÇsaîi ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = Revenire  U = Actualizare  R = Recuperare  ENTER = Repornire",
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
        "Doriîi specificarea unei alte arhitecturi de calcul?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaîi tastele SUS/JOS pentru a selecta o",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   arhitecturÇ de calcul, apoi apÇsaîi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ApÇsaîi ESC pentru a reveni la pagina precedentÇ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   fÇrÇ a specifica o altÇ arhitecturÇ de calcul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie­ire",
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
        "Se verificÇ stocarea datelor necesare.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Aceasta poate dura cƒteva momente.",
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
        "Se elibereazÇ memoria...",
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
        "ReactOS nu a fost instalat complet.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Scoateîi discul flexibil din unitatea A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "­i toate mediile CD-ROM din unitÇîile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ApÇsaîi ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "A­teptaîi...",
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
        "Doriîi modificarea parametrilor grafici de afi­are?",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  Utilizaîi tastele SUS/JOS pentru a selecta",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   un grup de parametri, apoi apÇsaîi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ApÇsaîi ESC pentru a reveni la pagina precedentÇ",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ie­ire",
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
        "Componentele de bazÇ ale ReactOS au fost instalate cu succes.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Scoateîi discul flexibil din unitatea A: ­i toate mediile",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "CD-ROM din unitÇîile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ApÇsaîi ENTER pentru a reporni calculatorul.",
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
        "Programul de instalare nu poate instala aplicaîia de iniîializare",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "a calculatorului pe discul calculatorului dumneavoastrÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Introduceîi un disc flexibil formatat Œn",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "unitatea A: apoi sÇ apÇsaîi ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie­ire",
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
        "UrmÇtoarea listÇ cuprinde partiîiile existente, precum ­i",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "spaîiul liber disponibil pentru crearea de noi partiîii.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Utilizaîi tastele SUS/JOS pentru a selecta o opîiune.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ApÇsaîi ENTER pentru a instala pe partiîia selectatÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tastaîi P pentru a crea o partiîie primarÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tastaîi E pentru a crea o partiîie extinsÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tastaîi D pentru a ­terge o partiîie existentÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "A­teptaîi...",
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
        "UrmeazÇ formatarea partiîiei.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "ApÇsaîi ENTER pentru a continua.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie­ire",
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
        "Alegeîi un director de instalare pe partiîia aleasÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Aici vor fi amplasate fi­ierele sistemului ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Puteîi indica un alt director, apÇsƒnd BACKSPACE pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "a ­terge caractere, apoi scriind calea directorului unde",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "doriîi instalarea ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie­ire",
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
        " A­teptaîi copierea de fi­iere Œn directorul ReactOS specificat.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "(aceasta poate dura cƒteva minute)",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 A­teptaîi...    ",
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
        "Instalare aplicaîie de iniîializare a calculatorului",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "InstaleazÇ iniîializatorul pe discul intern (MBR ­i VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "InstaleazÇ iniîializatorul pe discul intern (doar VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "InstaleazÇ iniîializatorul pe un disc flexibil.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Omite instalarea aplicaîiei de iniîializare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie­ire",
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
        "Doriîi specificarea modelului tastaturii instalate?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaîi tastele SUS/JOS pentru a selecta un model",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   de tastaturÇ, apoi apÇsaîi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ApÇsaîi ESC pentru a reveni la pagina precedentÇ fÇrÇ",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ie­ire",
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
        "Doriîi specificarea unui aranjament implicit de tastaturÇ?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaîi tastele SUS/JOS pentru a selecta un aranjament",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    dorit de tastaturÇ, apoi apÇsaîi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ApÇsaîi ESC pentru a reveni la pagina precedentÇ fÇrÇ",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ie­ire",
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
        "Se fac pregÇtirile necesare pentru copierea de fi­iere...",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Se creazÇ lista de fi­iere...",
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
        "Alegeîi un sistem de fi­iere din lista de mai jos.",
        0
    },
    {
        8,
        16,
        "\x07  Utilizaîi tastele SUS/JOS pentru a selecta",
        0
    },
    {
        8,
        17,
        "   un sistem de fi­iere.",
        0
    },
    {
        8,
        19,
        "\x07  ApÇsaîi ENTER pentru a formata partiîia.",
        0
    },
    {
        8,
        21,
        "\x07  ApÇsaîi ESC pentru a alege o altÇ partiîie.",
        0
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie­ire",
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
        "Aîi ales sÇ ­tergeîi partiîia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Tastaîi D pentru a ­terge partiîia.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ATENõIE: Toate datele de pe aceastÇ partiîie vor fi pierdute!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ApÇsaîi ESC pentru a anula.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = ¸tergere partiîie   ESC = Anulare   F3 = Ie­ire",
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
        "Se actualizeazÇ configuraîia sistemului...",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Se creazÇ registrul...",
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
        "ReactOS nu a fost instalat complet Œn calculatorul\n"
        "dumneavoastrÇ. DacÇ abandonaîi instalarea ReactOS\n"
		"acum, va fi nevoie sÇ o reluaîi din nou altÇ datÇ.\n"
        "\n"
        "  \x07  ApÇsaîi ENTER pentru a continua instalarea.\n"
        "  \x07  ApÇsaîi F3 pentru a abandona instalarea.",
        "F3 = Ie­ire  ENTER = Continuare"
    },
    {
        //ERROR_NO_HDD
        "Nu se pot gÇsi discuri interne.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Nu se poate gÇsi unitatea de citire.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Nu poate ŒncÇrca fi­ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Fi­ieul TXTSETUP.SIF a fos gÇsit deteriorat.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Fi­ierul TXTSETUP.SIF conîine o semnÇturÇ invalidÇ.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Nu se pot obîine informaîii despre dispozitiv(ele)\n"
		"din calculator.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_BOOT,
        "Nu s-a reu­it instalarea codului FAT de iniîializare\n"
		"pe partiîia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "A e­uat ŒncÇrcarea listei cu arhitecturi de\n"
		"calcul disponibile.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "A e­uat ŒncÇrcarea listei cu parametri de\n"
		"afi­are pentru ecran.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "A e­uat ŒncÇrcarea listei cu tipuri\n"
		"disponibile de tastaturÇ.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "A e­uat ŒncÇrcarea listei de configuraîii\n"
		"ale tastaturii.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WARN_PARTITION,
        "A fost gÇsit cel puîin un disc cu tabelÇ de partiîii\n"
        "nerecunoscutÇ, ce nu pot fi gestionatÇ corespunzÇtor!\n"
        "\n"
        "Crearea sau ­tergerea de partiîii poate astfel cauza\n"
		"distrugerea tabelei de partiîii."
        "\n"
        "  \x07  ApÇsaîi F3 pentru a abandona instalarea.\n"
        "  \x07  ApÇsaîi ENTER pentru a continua.",
        "F3= Ie­ire  ENTER = Continuare"
    },
    {
        //ERROR_NEW_PARTITION,
        "O partiîie nouÇ nu se creazÇ Œn interiorul\n"
        "unei partiîii existente!\n"
        "\n"
        "  * Tastaîi pentru a continua.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Aîi Œncercat ­tergerea de spaîiu nepartiîionat,\n"
        "ŒnsÇ doar spaîiul partiîionat poate fi ­ters!\n"
        "\n"
        "  * Tastaîi pentru a continua.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Nu s-a reu­it instalarea codului FAT de iniîializare\n"
		"pe partiîia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_FLOPPY,
        "Nu existÇ discuri flexibile Œn unitatea A:",
        "ENTER = Continuare"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "A e­uat actualizarea configuraîiei de tastaturÇ.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Nu s-a reu­it actualizarea registrului cu\n"
		"parametrii grafici ai ecranului!",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_IMPORT_HIVE,
		"Nu s-a reu­it importarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_REGISTRY
        "Fi­ierele cu datele registrului\n"
		"nu au putut fi localizate.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_HIVE,
        "Crearea registrului local a e­uat!",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Iniîializarea registrului a e­uat.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Fi­ierul cabinet nu conîine nici un fi­ier\n"
		"valid de tip inf.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_MISSING,
        "Fi­ierul cabinet nu e gÇsit.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Fi­ierul cabinet nu conîine nici un script\n"
		"de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_COPY_QUEUE,
        "Nu se poate deschide lista de fi­iere\n"
		"pentru copiere.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_DIR,
        "Nu se pot crea directoarele de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Nu se poate gÇsi secîiunea de directoare\n"
		"Œn fi­ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SECTION,
        "Nu se poate gÇsi secîiunea de directoare\n"
        "Œn fi­ierul cabinet.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Nu se poate crea directorul de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Nu se poate gÇsi secîiunea pentru date de\n"
        "instalare din fi­ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_PTABLE,
        "A e­uat scrierea tabelelor de partiîii.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "A e­uat includerea paginÇrii Œn registre.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Nu se poate seta sistemul de localizare.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "A e­uat includerea Œn registre a configuraîiei\n"
        "de tastaturÇ.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Nu s-a reu­it setarea geo id.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Tastaîi pentru a continua.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Nu se poate crea o nouÇ partiîie primarÇ sau extinsÇ Œn tabela\n"
        "de partiîii a acestui disc deoarece tabela de partiîii e plinÇ.\n"
        "\n"
        "  * Tastaîi pentru a continua."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Nu se poate crea mai mult de o partiîie extinsÇ pe un disc.\n"
        "\n"
        "  * Tastaîi pentru a continua."
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
        START_PAGE,
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
        REPAIR_INTRO_PAGE,
        roRORepairPageEntries
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
     "   A­teptaîi..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalare   P/E = Creare partiîie PrimarÇ/ExtinsÇ   F3 = Ie­ire"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalare   L = Creare partiîie LogicÇ   F3 = Ie­ire"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalare   D = ¸tergere partiîie   F3 = Ie­ire"},
    {STRING_DELETEPARTITION,
     "   D = ¸tergere partiîie   F3 = Ie­ire"},
    {STRING_PARTITIONSIZE,
     "MÇrimea noii partiîii:"},
    {STRING_CHOOSENEWPARTITION,
     "Aîi ales crearea unei partiîii primare pe"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Aîi ales crearea unei partiîii extinse pe"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Aîi ales crearea unei partiîii logice pe"},
    {STRING_HDDSIZE,
    "Introduceîi mÇrimea noii partiîii Œn megaocteîi."},
    {STRING_CREATEPARTITION,
     "   ENTER = Creare partiîie   ESC = Anulare   F3 = Ie­ire"},
    {STRING_PARTFORMAT,
    "AceastÇ partiîie urmeazÇ sÇ fie formatatÇ."},
    {STRING_NONFORMATTEDPART,
    "Alegeîi sÇ instalaîi ReactOS pe partiîie nouÇ sau neformatatÇ."},
    {STRING_INSTALLONPART,
    "ReactOS va fi instalat pe partiîia"},
    {STRING_CHECKINGPART,
    "Programul de instalare verificÇ acum partiîia aleasÇ."},
    {STRING_QUITCONTINUE,
    "F3= Ie­ire  ENTER = Continuare"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Repornire calculator"},
    {STRING_TXTSETUPFAILED,
    "Nu s-a reu­it gÇsirea sesiunii\n'%S' Œn TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Fi­ierul curent: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Se copie fi­ierele..."},
    {STRING_REGHIVEUPDATE,
    "   Se actualizeazÇ registrul..."},
    {STRING_IMPORTFILE,
    "   Se importÇ %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Se actualizeazÇ registrul configuraîiei grafice..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Se actualizeazÇ particularitÇîile locale..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Se actualizeazÇ configuraîia tastaturii..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Se adaugÇ datele de paginare Œn registru..."},
    {STRING_DONE,
    "   Terminat!"},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Repornire calculator"},
    {STRING_CONSOLEFAIL1,
    "Nu se poate deschide consola\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Cea mai frecventÇ cauzÇ pentru asta este utilizarea unei tastaturi USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Tastaturile USB ŒncÇ nu sunt complet suportate\r\n"},
    {STRING_FORMATTINGDISK,
    "Se formateazÇ discul..."},
    {STRING_CHECKINGDISK,
    "Se verificÇ discul..."},
    {STRING_FORMATDISK1,
    " FormateazÇ partiîia ca sistem de fi­iere %S (formatare rapidÇ) "},
    {STRING_FORMATDISK2,
    " FormateazÇ partiîia ca sistem de fi­iere %S "},
    {STRING_KEEPFORMAT,
    " PÇstreazÇ sistemul de fi­iere actual (fÇrÇ schimbÇri) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu) de tip %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tip %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "de pe %I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu) de tip %wZ."},
    {STRING_HDDINFOUNK3,
    "de pe %I64u %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Discul %lu (%I64u %s), Port=%hu, Magistrala=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tip %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "de pe Discul %lu (%I64u %s), Port=%hu, Magistrala=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTip %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu) de tip %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "O nouÇ partiîie a fost creatÇ Œn"},
    {STRING_UNPSPACE,
    "    %sSpaîiu nepartiîionat%s           %6lu %s"},
    {STRING_MAXSIZE,
    "Mo (max. %lu Mo)"},
    {STRING_EXTENDED_PARTITION,
    "Partiîie extinsÇ"},
    {STRING_UNFORMATTED,
    "Part. nouÇ (neformatatÇ)"},
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
    "AdÇugare configuraîii de tastaturÇ"},
    {0, 0}
};
