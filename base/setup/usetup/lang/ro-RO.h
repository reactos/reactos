/* Ştefan Fulea (stefan dot fulea at mail dot md) */
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
        "Selecţie limbă",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Selectaţi limba pentru procesul de instalare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Apoi apăsaţi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Aceasta va fi în final limba implicită pentru tot sistemul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare  F3 = Ieşire",
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
        "Această primă etapă din instalarea ReactOS va copia fişierele",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "necesare în calculatorul dumneavoastră şi-l va pregăti pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "cea de-a doua etapă a instalării.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "\x07  Apăsaţi ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Tastaţi R pentru a reface un sistem deteriorat sau pentru",
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
        "\x07  Tastaţi L pentru Termenii şi Condiţiile de Licenţiere.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Apăsaţi F3 pentru a ieşi fără a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        26,
        "Pentru mai multe informaţii despre ReactOS, vizitaţi:",
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
        "ENTER = Continuare  R = Refacere  L = Licenţă  F3 = Ieşire",
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
        "Starea versiunii ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ReactOS este în stadiul Alfa, însemnând că nu sunt implementate toate",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "caracteristicile și este în continuă dezvoltare. Este recomandat doar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "pentru scopuri de evaluare și testare și nu pentru activități",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "de zi cu zi.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "Copiați datele sau testați pe un computer secundar dacă încercați",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "să rulați ReactOS pe un hardware.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Apăsaţi ENTER pentru a continua instalarea ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Apăsaţi F3 pentru a ieşi fără a instala ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieşire",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        "Licenţiere:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "Sistemul de operare ReactOS este oferit în termenii Licenţei",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "Publice Generale GNU, referită în continuare ca GPL, cu părţi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "de cod din alte licenţe compatibile (ca X11, BSD, şi LGPL).",
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
        "prin urmare oferite sub licenţa GPL, menţinându-şi astfel şi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "licenţierea originală în acelaşi timp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "Acest sistem vine fără vreo restricţie de utilizare, aceasta",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "fiind o condiţie legislativă aplicabilă atât la nivel local",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "cât şi internaţional. Licenţierea se referă doar la distri-",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "buirea sistemului ReactOS către părţi terţe.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "Dacă din vreun careva motiv nu deţineţi o copie a licenţei",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "GPL împreună cu ReactOS, o puteţi consulta (în engleză)",
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
        "Garanţie:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        29,
        "Acest sistem de operare este distribuit doar în speranţa că",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        30,
        "va fi util, neavând însă ataşată NICI O GARANŢIE; nici măcar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        31,
        "garanţia implicită a VANDABILITĂŢII sau a UTILITĂŢII ÎNTR-UN",
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
        "Configurare dispozitive de bază",
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
        "Model tastatură:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Aranj. tastatură:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Acceptă:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16,
        "Accept configuraţia dispozitivelor",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Puteţi modifica starea curentă. Utilizaţi tastele SUS/JOS pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "alegerea unui dispozitiv, apoi apăsaţi ENTER pentru a-i modifica",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "configuraţia ataşată.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Când configuraţia dispozitivele enumerate este cea corectă,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "selectaţi \"Accept configuraţia dispozitivelor\", apoi confirmaţi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        25,
        "apăsând ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieşire",
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
        "Programul de instalare ReactOS este încă într-o fază incipientă de",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "dezvoltare şi nu posedă o funcţionalitate completă.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Funcţionalitatea de refacere încă nu este implementată.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tastaţi U pentru actualizarea sistemului.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tastaţi R pentru consola de Recuperare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Apăsaţi ESC pentru a reveni la pagina principală.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Apăsaţi ENTER pentru a reporni calculatorul.",
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
        "Doriţi specificarea unei alte arhitecturi de calcul?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaţi tastele SUS/JOS pentru a selecta o",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   arhitectură de calcul, apoi apăsaţi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apăsaţi ESC pentru a reveni la pagina precedentă",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   fără a specifica o altă arhitectură de calcul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ieşire",
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
        "Sistemul verifică integritatea datelor scrise pe disc.",
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
        "Scoateţi discul flexibil din unitatea A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "şi toate mediile CD-ROM din unităţile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Apăsaţi ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Aşteptaţi...",
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
        "Doriţi modificarea parametrilor grafici de afişare?",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  Utilizaţi tastele SUS/JOS pentru a selecta",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   un grup de parametri, apoi apăsaţi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apăsaţi ESC pentru a reveni la pagina precedentă",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ieşire",
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
        "Componentele de bază ale ReactOS au fost instalate cu succes.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Scoateţi discul flexibil din unitatea A: şi toate mediile",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "CD-ROM din unităţile CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Apăsaţi ENTER pentru a reporni calculatorul.",
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
        "iniţializare a calculatorului pe discul local.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Introduceţi un disc flexibil formatat în",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "unitatea A: apoi să apăsaţi ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieşire",
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
        "Următoarea listă cuprinde partiţiile existente, precum şi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "spaţiul liber disponibil pentru crearea de noi partiţii.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Utilizaţi tastele SUS/JOS pentru a selecta o opţiune.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apăsaţi ENTER pentru a instala pe partiţia selectată.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tastaţi P pentru a crea o partiţie primară.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tastaţi E pentru a crea o partiţie extinsă.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tastaţi L pentru a crea o partiţie logică.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tastaţi D pentru a şterge o partiţie existentă.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Aşteptaţi...",
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
        "Aţi solicitat ştergerea partiţiei de sistem.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Partiţiile de sistem pot conţine programe de diagnoză, programe de con-",
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
        "(ca ReactOS) şi alte programe furnizate de producătorii calculorului.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Ştergeţi o partiţie de sistem doar când sunteţi siguri că nu există aşa",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "programe pe partiţie, sau când sunteţi siguri că doriţi să le ştergeţi.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "Ştergând partiţia apare riscul de a nu mai putea porni calculatorul de",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "pe discul local decât după finalizarea instalării ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Apăsaţi ENTER pentru a şterge partiţia de sistem. Vi se va",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   cere să confirmaţi din nou această ştergere a partiţiei.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Apăsaţi ESC pentru a reveni la pagina precedentă. Partiţia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   nu va fi ştearsă.",
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
        "Urmează formatarea partiţiei.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Apăsaţi ENTER pentru a continua.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieşire",
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
        "Alegeţi un director de instalare pe partiţia aleasă.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Aici vor fi amplasate fişierele sistemului ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Puteţi indica un alt director, apăsând BACKSPACE pentru",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "a şterge caractere, apoi scriind calea directorului unde",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "doriţi să instalaţi ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieşire",
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
        "Are loc copierea de fişiere în directorul ReactOS specificat.",
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
        "\xB3 Aşteptaţi...",
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
        "Instalare modul de iniţializare al calculatorului",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Instalează iniţializatorul pe discul intern (MBR şi VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Instalează iniţializatorul pe discul intern (doar VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Instalează iniţializatorul pe un disc flexibil.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Omite instalarea modulului de iniţializare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ieşire",
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
        "Doriţi specificarea modelului tastaturii instalate?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaţi tastele SUS/JOS pentru a selecta un model",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   de tastatură, apoi apăsaţi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apăsaţi ESC pentru a reveni la pagina precedentă fără",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ieşire",
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
        "Doriţi specificarea unui aranjament implicit de tastatură?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Utilizaţi tastele SUS/JOS pentru a selecta un aranjament",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    dorit de tastatură, apoi apăsaţi ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Apăsaţi ESC pentru a reveni la pagina precedentă fără",
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
        "ENTER = Continuare   ESC = Anulare   F3 = Ieşire",
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
        "Au loc pregătirile necesare pentru copierea de fişiere.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Listă de fişiere în curs de creare...",
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
        "Alegeţi un sistem de fişiere din lista de mai jos.",
        0
    },
    {
        8,
        16,
        "\x07  Utilizaţi tastele SUS/JOS pentru a selecta",
        0
    },
    {
        8,
        17,
        "   un sistem de fişiere.",
        0
    },
    {
        8,
        19,
        "\x07  Apăsaţi ENTER pentru a formata partiţia.",
        0
    },
    {
        8,
        21,
        "\x07  Apăsaţi ESC pentru a alege o altă partiţie.",
        0
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ieşire",
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
        "Aţi ales să ştergeţi partiţia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Tastaţi D pentru a şterge partiţia.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ATENŢIE: Toate datele de pe această partiţie vor fi pierdute!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Apăsaţi ESC pentru a anula.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Ştergere partiţie   ESC = Anulare   F3 = Ieşire",
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
        "Are loc actualizarea configuraţiei sistemului.",
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
        "Dacă abandonaţi instalarea acum, altă dată, pentru a\n"
        "instala ReactOS, va fi nevoie să repetaţi toţi paşii.\n"
        "\n"
        "  \x07  Apăsaţi ENTER pentru a continua instalarea.\n"
        "  \x07  Apăsaţi F3 pentru a abandona instalarea.",
        "F3 = Ieşire  ENTER = Continuare"
    },
    {
        //ERROR_NO_HDD
        "Eşec la identificarea unităţilor interne de stocare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Eşec la accesarea unitatăţii de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Eşec la încărcarea fişierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Fişieul TXTSETUP.SIF a fos găsit deteriorat.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Fişierul TXTSETUP.SIF conţine o semnătură nevalidă.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Eşec la obţinerea de informaţii despre\n"
        "dispozitivele din calculator.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_BOOT,
        "Eşec la instalarea codului FAT de iniţializare\n"
        "pe partiţia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Eşec la încărcarea listei cu arhitecturi de\n"
        "calcul disponibile.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Eşec la încărcarea listei cu parametri de\n"
        "afişare pentru ecran.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Eşec la încărcarea listei cu tipuri\n"
        "disponibile de tastatură.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Eşec la încărcarea listei de configuraţii\n"
        "ale tastaturii.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WARN_PARTITION,
        "A fost găsită cel puţin un disc cu tabelă nerecunoscută\n"
        "de partiţii, care nu poate fi gestionată corespunzător!\n"
        "\n"
        "Crearea sau ştergerea de partiţii poate astfel cauza\n"
        "distrugerea tabelei de partiţii."
        "\n"
        "  \x07  Apăsaţi F3 pentru a abandona instalarea.\n"
        "  \x07  Apăsaţi ENTER pentru a continua.",
        "F3 = Ieşire  ENTER = Continuare"
    },
    {
        //ERROR_NEW_PARTITION,
        "O partiţie nouă nu poate fi creată în interiorul\n"
        "unei partiţii existente!\n"
        "\n"
        "  * Tastaţi pentru a continua.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Aţi încercat ştergerea de spaţiu nepartiţionat,\n"
        "însă doar spaţiul partiţionat poate fi şters!\n"
        "\n"
        "  * Tastaţi pentru a continua.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Eşec la instalarea codului FAT de iniţializare\n"
        "pe partiţia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_NO_FLOPPY,
        "Nu există discuri flexibile în unitatea A:",
        "ENTER = Continuare"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Eşec la actualizarea configuraţiei de tastatură.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Eşec la actualizarea registrului cu\n"
        "parametrii grafici ai ecranului!",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Eşec la importarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_REGISTRY
        "Eşec la localizarea fişierelor\n"
        "cu datele registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_HIVE,
        "Eşec la crearea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Eşec la iniţializarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Fişierul cabinet nu conţine nici\n"
        "un fişier valid de tip inf.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_MISSING,
        "Eşec la localizarea fişierului cabinet.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Fişierul cabinet nu conţine nici\n"
        "un script de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_COPY_QUEUE,
        "Eşec la accesarea listei cu\n"
        "fişiere pentru copiere.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_DIR,
        "Eşec la crearea directoarelor de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Eşec la identificarea secţiunii de\n"
        "directoare în fişierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CABINET_SECTION,
        "Eşec la identificarea secţiunii de\n"
        "directoare în fişierul cabinet.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Eşec la crearea directorului de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Eşec la localizarea secţiunii pentru date\n"
        "de instalare din fişierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Eşec la scrierea tabelelor de partiţii.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Eşec la includerea paginării în registru.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Eşec la instituirea sistemului de localizare.\n",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Eşec la includerea în registru a configuraţiei\n"
        "de tastatură.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Eşec la instituirea de geo id.",
        "ENTER = Repornire calculator"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Numele de director este nevalid.\n"
        "\n"
        "  * Tastaţi pentru a continua."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Partiţia selectată este prea mică pentru a instala ReactOS.\n"
        "Partiţia de instalare trebuie să aibă cel puţin %lu Mocteţi.\n"
        "\n"
        "  * Tastaţi pentru a continua.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Nu poate fi creată o nouă partiţie primară sau extinsă în tabela\n"
        "de partiţii a acestui disc deoarece tabela de partiţii e plină.\n"
        "\n"
        "  * Tastaţi pentru a continua."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Nu poate fi creată mai mult de o partiţie extinsă pe un disc.\n"
        "\n"
        "  * Tastaţi pentru a continua."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Eşec la formatarea partiţiei:\n"
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
     "   Aşteptaţi..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalare   P/E = Creare partiţie Primară/Extinsă   F3 = Ieşire"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalare   L = Creare partiţie Logică   F3 = Ieşire"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalare   D = Ştergere partiţie   F3 = Ieşire"},
    {STRING_DELETEPARTITION,
     "   D = Ştergere partiţie   F3 = Ieşire"},
    {STRING_PARTITIONSIZE,
     "Mărimea noii partiţii:"},
    {STRING_CHOOSENEWPARTITION,
     "Aţi ales crearea unei partiţii primare pe"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Aţi ales crearea unei partiţii extinse pe"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Aţi ales crearea unei partiţii logice pe"},
    {STRING_HDDSIZE,
    "Introduceţi mărimea noii partiţii în megaocteţi."},
    {STRING_CREATEPARTITION,
     "   ENTER = Creare partiţie   ESC = Anulare   F3 = Ieşire"},
    {STRING_PARTFORMAT,
    "Această partiţie urmează să fie formatată."},
    {STRING_NONFORMATTEDPART,
    "Alegeţi să instalaţi ReactOS pe partiţie nouă sau neformatată."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Partiţia de sistem încă nu a fost formatată."},
    {STRING_NONFORMATTEDOTHERPART,
    "Noua partiţie încă nu a fost formatată."},
    {STRING_INSTALLONPART,
    "ReactOS va fi instalat pe partiţia"},
    {STRING_CHECKINGPART,
    "Programul de instalare verifică acum partiţia aleasă."},
    {STRING_CONTINUE,
    "ENTER = Continuare"},
    {STRING_QUITCONTINUE,
    "F3 = Ieşire  ENTER = Continuare"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Repornire calculator"},
    {STRING_TXTSETUPFAILED,
    "Nu s-a reuşit găsirea sesiunii\n'%S' în TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Fişierul curent: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Fişiere în curs de copiere..."},
    {STRING_REGHIVEUPDATE,
    "   Registru în curs de actualizare..."},
    {STRING_IMPORTFILE,
    "   În curs de importare din %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Registru de configuraţie grafică în actualizare..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Particularităţi locale în actualizare..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Configuraţie de tastatură în actualizare..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Date de paginare în curs de adaugare în registru..."},
    {STRING_DONE,
    "   Terminat!"},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Repornire calculator"},
    {STRING_CONSOLEFAIL1,
    "Eşec la deschiderea consolei\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Cea mai frecventă cauză pentru asta este utilizarea unei tastaturi USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Tastaturile USB nu sunt încă toate acceptate\r\n"},
    {STRING_FORMATTINGDISK,
    "Disc în curs de formatare..."},
    {STRING_CHECKINGDISK,
    "Disc în curs de verificare..."},
    {STRING_FORMATDISK1,
    " Formatează partiţia ca sistem de fişiere %S (formatare rapidă) "},
    {STRING_FORMATDISK2,
    " Formatează partiţia ca sistem de fişiere %S "},
    {STRING_KEEPFORMAT,
    " Păstrează sistemul de fişiere actual (fără schimbări) "},
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
    "O nouă partiţie a fost creată în"},
    {STRING_UNPSPACE,
    "    %sSpaţiu nepartiţionat%s           %6lu %s"},
    {STRING_MAXSIZE,
    "Mo (max. %lu Mo)"},
    {STRING_EXTENDED_PARTITION,
    "Partiţie extinsă"},
    {STRING_UNFORMATTED,
    "Part. nouă (neformatată)"},
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
    "Adăugare configuraţii de tastatură"},
    {0, 0}
};
