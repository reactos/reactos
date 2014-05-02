/* ∏tefan Fulea (stefan dot fulea at mail dot md) */
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
        "SelecÓie limb«",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  SelectaÓi limba pentru procesul de instalare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Apoi ap«saÓi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Aceasta va fi ån final limba implicit« pentru tot sistemul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare  F3 = Ie≠ire",
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
        "Aceast« prim« etap« din instalarea ReactOS va copia fi≠ierele",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "necesare ån calculatorul dumneavoastr« ≠i-l va preg«ti pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "cea de-a doua etap« a instal«rii.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "\x07  Ap«saÓi ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  TastaÓi R pentru a reface un sistem deteriorat sau pentru",
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
        "\x07  TastaÓi L pentru Termenii ≠i CondiÓiile de LicenÓiere",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Ap«saÓi F3 pentru a ie≠i f«r« a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        26,
        "Pentru mai multe informaÓii despre ReactOS, vizitaÓi:",
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
        "ENTER = Continuare  R = Refacere  L = LicenÓ«  F3 = Ie≠ire",
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
        "Programul curent de instalare este ånc« åntr-un stadiu primar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "de dezvoltare ≠i nu conÓine toate funcÓionalit«Óile unei",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "aplicaÓii de instalare complete.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Sunt aplicabile urm«toarele limit«ri:",
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
        "  de o partiÓie per disc.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Programul curent de instalare nu poate ≠terge o partiÓie",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  primar« cÉt timp exist« partiÓii extinse.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Programul curent de instalare nu poate ≠terge prima partiÓie",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "  extins«, cÉt timp exist« alte partiÓii extinse.",
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
        "  de fi≠iere FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "- Verific«rile de integritate pentru fi≠iere nu sunt",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "  ånc« implementate.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "\x07  Ap«saÓi ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        28,
        "\x07  Ap«saÓi F3 pentru a ie≠i f«r« a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie≠ire",
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
        "LicenÓiere:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "Sistemul de operare ReactOS este oferit ån termenii GNU GPL,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "cu p«rÓi de cod din alte licenÓe compatibile, cum ar fi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenÓele X11, BSD ≠i GNU LGPL.",
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
        "prin urmare oferite sub licenÓ« GNU GPL, menÓinÉndu-≠i ån",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "acela≠i timp ≠i licenÓierea original«.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "Acest sistem vine f«r« vreo restricÓie de utilizare, aceasta",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "fiind o condiÓie legislativ« aplicabil« atÉt la nivel local",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "cÉt ≠i internaÓional. LicenÓierea se refer« doar la",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "distribuirea sistemului ReactOS c«tre p«rÓi terÓe.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "Dac« din vreun motiv careva nu aÓi primit o copie a LicenÓei",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "Publice Generale GNU åmpreun« cu ReactOS, sunteÓi rugaÓi s«",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "vizitaÓi:",
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
        "GaranÓie:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        28,
        "Acest sistem de operare este distribuit doar ån speranÓa c«",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        29,
        "va fi util, neavÉnd åns« ata≠at« NICI O GARANıIE; nici m«car",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        30,
        "garanÓia implicit« a VANDABILIT∆ıII sau a UTILIT∆ıII ◊NTR-UN",
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
        "Configurarea dispozitivelor de baz«.",
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
        "Model tastatur«:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        " Aranj. tastatur«:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Accept«:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Accept configuraÓia dispozitivelor",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "PuteÓi modifica starea curent«. UtilizaÓi tastele SUS/JOS pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "alegerea unui dispozitiv, apoi ap«saÓi ENTER pentru a-i modifica",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "configuraÓia ata≠at«.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "CÉnd configuraÓia dispozitivele enumerate este cea corect«,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "selectaÓi \"Accept configuraÓia dispozitivelor\", apoi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        25,
        "confirmaÓi-o ap«sÉnd ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie≠ire",
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
        "Programul de instalare ReactOS este ånc« åntr-o faz« incipient« de",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "dezvoltare ≠i nu posed« o funcÓionalitate complet«.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "FuncÓionalitatea de refacere ånc« nu este implementat«.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  TastaÓi U pentru actualizarea SO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  TastaÓi R pentru consola de Recuperare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Ap«saÓi ESC pentru a reveni la pagina principal«.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Ap«saÓi ENTER pentru a reporni calculatorul.",
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
        "DoriÓi specificarea arhitecturii de calcul?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  UtilizaÓi tastele SUS/JOS pentru a selecta o",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   arhitectur« de calcul, apoi ap«saÓi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Ap«saÓi ESC pentru a reveni la pagina precedent«",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   f«r« a specifica o alt« arhitectur« de calcul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie≠ire",
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
        "Verificarea stoc«rii datelor necesare.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "La final, calculatorul va fi repornit automat.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "A≠teptaÓi...",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Se elibereaz« memoria...",
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
        "ScoateÓi discul flexibil din unitatea A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "≠i toate CD-ROM din unit«Óile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Ap«saÓi ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "A≠teptaÓi...",
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
        "DoriÓi modificarea parametrilor grafici de afi≠are?",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  UtilizaÓi tastele SUS/JOS pentru a selecta",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   un grup de parametri, apoi ap«saÓi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Ap«saÓi ESC pentru a reveni la pagina precedent«",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ie≠ire",
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
        "Componentele de baz« ale ReactOS au fost instalate cu succes.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "ScoateÓi discul flexibil din unitatea A: ≠i toate CD-ROM",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "din unit«Óile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Ap«saÓi ENTER pentru a reporni calculatorul.",
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
        "Programul de instalare nu poate instala aplicaÓia de iniÓializare",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "a calculatorului pe discul calculatorului dumneavoastr«.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "IntroduceÓi un disc flexibil formatat ån",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "unitatea A: apoi s« ap«saÓi ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie≠ire",
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
        "Urm«toarea list« cuprinde partiÓiile existente precum",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "≠i spaÓiul nefolosit pentru crearea de noi partiÓii.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  UtilizaÓi tastele SUS/JOS pentru a selecta o opÓiune.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Ap«saÓi ENTER pentru a instala pe partiÓia selectat«.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  TastaÓi C pentru a crea o nou« partiÓie.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  TastaÓi D pentru a ≠terge o partiÓie existent«.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "A≠teptaÓi...",
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
        "Urmeaz« formatarea partiÓiei.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Ap«saÓi ENTER pentru a continua.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie≠ire",
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
        "AlegeÓi un director de instalare pe partiÓia aleas«.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Aici vor fi amplasate fi≠ierele sistemului ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "PuteÓi indica un alt director, ap«sÉnd BACKSPACE pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "a ≠terge caractere, apoi scriind calea directorului unde",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "doriÓi instalarea ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie≠ire",
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
        " A≠teptaÓi copierea de fi≠iere ån directorul ReactOS specificat.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "(aceasta poate dura cÉteva minute)",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 A≠teptaÓi...    ",
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
        "Instalarea aplicaÓiei de iniÓializare a calculatorului",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Instaleaz« iniÓializatorul pe discul intern (MBR ≠i VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Instaleaz« iniÓializatorul pe discul intern (doar VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Instaleaz« iniÓializatorul pe un disc flexibil.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Omite instalarea aplicaÓiei de iniÓializare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie≠ire",
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
        "DoriÓi specificarea modelului tastaturii instalate?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  UtilizaÓi tastele SUS/JOS pentru a selecta un model",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   de tastatur«, apoi ap«saÓi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Ap«saÓi ESC pentru a reveni la pagina precedent« f«r«",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ie≠ire",
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
        "DoriÓi specificarea unui aranjament implicit de tastatur«?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  UtilizaÓi tastele SUS/JOS pentru a selecta un aranjament",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    dorit de tastatur«, apoi ap«saÓi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Ap«saÓi ESC pentru a reveni la pagina precedent« f«r«",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ie≠ire",
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
        "Se fac preg«tirile necesare pentru copierea de fi≠iere.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Se creaz« lista de fi≠iere...",
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
        "AlegeÓi un sistem de fi≠iere din lista de mai jos.",
        0
    },
    {
        8,
        16,
        "\x07  UtilizaÓi tastele SUS/JOS pentru a selecta",
        0
    },
    {
        8,
        17,
        "   un sistem de fi≠iere.",
        0
    },
    {
        8,
        19,
        "\x07  Ap«saÓi ENTER pentru a formata partiÓia.",
        0
    },
    {
        8,
        21,
        "\x07  Ap«saÓi ESC pentru a alege o alt« partiÓie.",
        0
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie≠ire",
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
        "AÓi ales s« ≠tergeÓi partiÓia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  TastaÓi D pentru a ≠terge partiÓia.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ATENıIE: Toate datele de pe aceast« partiÓie vor fi pierdute!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Ap«saÓi ESC pentru a anula.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = ∏tergere partiÓie   ESC = Anulare   F3 = Ie≠ire",
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
        "Se actualizeaz« configuraÓia sistemului...",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Se creaz« registrul...",
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
        //ERROR_NOT_INSTALLED
        "ReactOS nu a fost instalat complet ån calculatorul\n"
        "dumneavoastr«. Dac« abandonaÓi instalarea ReactOS\n"
		"acum, va fi nevoie s« o reluaÓi din nou alt« dat«.\n"
        "\n"
        "  \x07  Ap«saÓi ENTER pentru a continua instalarea.\n"
        "  \x07  Ap«saÓi F3 pentru a abandona instalarea.",
        "F3 = Ie≠ire  ENTER = Continuare"
    },
    {
        //ERROR_NO_HDD
        "Nu se pot g«si discuri interne.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Nu se poate g«si unitatea de citire.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Nu poate ånc«rca fi≠ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Fi≠ieul TXTSETUP.SIF a fos g«sit deteriorat.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Fi≠ierul TXTSETUP.SIF conÓine o semn«tur« invalid«.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Nu se pot obÓine informaÓii despre dispozitiv(ele)\n"
		"din calculator.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_BOOT,
        "Nu s-a reu≠it instalarea codului FAT de iniÓializare\n"
		"pe partiÓia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "A e≠uat ånc«rcarea listei cu arhitecturi de\n"
		"calcul disponibile.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "A e≠uat ånc«rcarea listei cu parametri de\n"
		"afi≠are pentru ecran.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "A e≠uat ånc«rcarea listei cu tipuri\n"
		"disponibile de tastatur«.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "A e≠uat ånc«rcarea listei de configuraÓii\n"
		"ale tastaturii.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WARN_PARTITION,
        "A fost g«sit cel puÓin un disc cu tabel« de partiÓii\n"
        "nerecunoscut«, ce nu pot fi gestionat« corespunz«tor!\n"
        "\n"
        "Crearea sau ≠tergerea de partiÓii poate astfel cauza\n"
		"distrugerea tabelei de partiÓii."
        "\n"
        "  \x07  Ap«saÓi F3 pentru a abandona instalarea.\n"
        "  \x07  Ap«saÓi ENTER pentru a continua.",
        "F3= Ie≠ire  ENTER = Continuare"
    },
    {
        //ERROR_NEW_PARTITION,
        "O partiÓie nou« nu se creaz« ån interiorul\n"
        "unei partiÓii existente!\n"
        "\n"
        "  * TastaÓi pentru a continua.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "AÓi åncercat ≠tergerea de spaÓiu nepartiÓionat,\n"
        "åns« doar spaÓiul partiÓionat poate fi ≠ters!\n"
        "\n"
        "  * TastaÓi pentru a continua.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Nu s-a reu≠it instalarea codului FAT de iniÓializare\n"
		"pe partiÓia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_FLOPPY,
        "Nu exist« discuri flexibile ån unitatea A:",
        "ENTER = Continuare"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "A e≠uat actualizarea configuraÓiei de tastatur«.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Nu s-a reu≠it actualizarea registrului cu\n"
		"parametrii grafici ai ecranului!",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_IMPORT_HIVE,
		"Nu s-a reu≠it importarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_REGISTRY
        "Fi≠ierele cu datele registrului\n"
		"nu au putut fi localizate.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_HIVE,
        "Crearea registrului local a e≠uat!",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "IniÓializarea registrului a e≠uat.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Fi≠ierul cabinet nu conÓine nici un fi≠ier\n"
		"valid de tip inf.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_MISSING,
        "Fi≠ierul cabinet nu e g«sit.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Fi≠ierul cabinet nu conÓine nici un script\n"
		"de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_COPY_QUEUE,
        "Nu se poate deschide lista de fi≠iere\n"
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
        "Nu se poate g«si secÓiunea de directoare\n"
		"ån fi≠ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SECTION,
        "Nu se poate g«si secÓiunea de directoare\n"
		"ån fi≠ierul cabinet.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Nu se poate crea directorul de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Nu se poate g«si secÓiunea pentru date de\n"
		"instalare din fi≠ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_PTABLE,
        "A e≠uat scrierea tabelelor de partiÓii.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "A e≠uat includerea pagin«rii ån registre.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Nu se poate seta sistemul de localizare.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "A e≠uat includerea ån registre a configuraÓiei\n"
		"de tastatur«.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Nu s-a reu≠it setarea geo id.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INSUFFICIENT_DISKSPACE,
        "Pe partiÓia selectat« nu exist« suficient\n"
		"spaÓiu liber."
        "  * TastaÓi pentru a continua.",
        NULL
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
     "   A≠teptaÓi..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalare   C = Creare partiÓie   F3 = Ie≠ire"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalare   D = ∏tergere partiÓie   F3 = Ie≠ire"},
    {STRING_PARTITIONSIZE,
     "M«rimea noii partiÓii:"},
    {STRING_CHOOSENEWPARTITION,
     "AÓi ales crearea unei noi partiÓii pe"},
    {STRING_HDDSIZE,
    "IntroduceÓi m«rimea noii partiÓii ån megaocteÓi."},
    {STRING_CREATEPARTITION,
     "   ENTER = Creare partiÓie   ESC = Anulare   F3 = Ie≠ire"},
    {STRING_PARTFORMAT,
    "Aceast« PartiÓie urmeaz« s« fie formatat«."},
    {STRING_NONFORMATTEDPART,
    "AlegeÓi s« instalaÓi ReactOS pe partiÓie nou« sau neformatat«."},
    {STRING_INSTALLONPART,
    "ReactOS va fi instalat pe partiÓia"},
    {STRING_CHECKINGPART,
    "Programul de instalare verific« acum partiÓia aleas«."},
    {STRING_QUITCONTINUE,
    "F3= Ie≠ire  ENTER = Continuare"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Repornire calculator"},
    {STRING_TXTSETUPFAILED,
    "Nu s-a reu≠it g«sirea sesiunii\n'%S' ån TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Fi≠ierul curent: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Se copie fi≠ierele..."},
    {STRING_REGHIVEUPDATE,
    "   Se actualizeaz« registrul..."},
    {STRING_IMPORTFILE,
    "   Se import« %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Se actualizeaz« registrul configuraÓiei grafice..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Se actualizeaz« particularit«Óile locale..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Se actualizeaz« configuraÓia tastaturii..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Se adaug« datele de paginare ån registru..."},
    {STRING_DONE,
    "   Terminat!"},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Repornire calculator"},
    {STRING_CONSOLEFAIL1,
    "Nu se poate deschide consola\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Cea mai frecvent« cauz« pentru asta este utilizarea unei tastaturi USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Tastaturile USB ånc« nu sunt complet suportate\r\n"},
    {STRING_FORMATTINGDISK,
    "Se formateaz« discul..."},
    {STRING_CHECKINGDISK,
    "Se verific« discul..."},
    {STRING_FORMATDISK1,
    " Formateaz« partiÓia ca sistem de fi≠iere %S (formatare rapid«) "},
    {STRING_FORMATDISK2,
    " Formateaz« partiÓia ca sistem de fi≠iere %S "},
    {STRING_KEEPFORMAT,
    " P«streaz« sistemul de fi≠iere actual (f«r« schimb«ri) "},
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
    "%c%c  Tip %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu) de tip %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Discul %lu  (Port=%hu, Magistrala=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "O nou« partiÓie a fost creat« ån"},
    {STRING_UNPSPACE,
    "    SpaÓiu nepartiÓionat             %6lu %s"},
    {STRING_MAXSIZE,
    "Mo (max. %lu Mo)"},
    {STRING_UNFORMATTED,
    "Part. nou« (neformatat«)"},
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
    "Ad«ugare configuraÓii de tastatur«"},
    {0, 0}
};
