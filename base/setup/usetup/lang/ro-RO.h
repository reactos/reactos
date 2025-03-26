// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/*
 * PROJECT:     ReactOS Setup
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Romanian resource file
 * TRANSLATORS: Copyright 2011-2019 Ștefan Fulea <stefan.fulea@mail.com>
 *              Copyright 2018 George Bișoc <george.bisoc@reactos.org>
 *              Copyright 2022-2024 Andrei Miloiu <miloiuandrei@gmail.com>
 */

#pragma once

static MUI_ENTRY roROSetupInitPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "A\272tepta\376i ini\376ializarea programului de instalare \272i",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "identificarea dispozitivelor din calculator...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A\272tepta\376i...",
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

static MUI_ENTRY roROLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Selec\376ie limb\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Selecta\376i limba pentru procesul de instalare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Apoi ap\343sa\376i ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Aceasta va fi \356n final limba implicit\343 pentru tot sistemul.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare  F3 = Ie\272ire",
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

static MUI_ENTRY roROWelcomePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Bun venit la instalarea ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Aceast\343 prim\343 etap\343 din instalarea ReactOS va copia fi\272ierele",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "necesare \356n calculatorul dumneavoastr\343 \272i-l va preg\343ti pentru",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "cea de-a doua etap\343 a instal\343rii.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "\x07  Tasta\376i ENTER pentru a instala sau actualiza ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Tasta\376i R pentru a reface un sistem deteriorat.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tasta\376i L pentru Termenii \272i Condi\376iile de Licen\376iere.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        23,
        "\x07  Tasta\376i F3 pentru a ie\272i f\343r\343 a instala ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        26,
        "Pentru mai multe informa\376ii despre ReactOS, vizita\376i:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        27,
        "https://reactos.org/",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare  R = Refacere  L = Licen\376\343  F3 = Ie\272ire",
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

static MUI_ENTRY roROIntroPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Starea versiunii curente a ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS este \356n stadiu alfa de dezvoltare, adic\343 nu are prezint\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "func\376ionalit\343\376i complete \272i \356nc\343 nu este recomandat\343 utilizarea sa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "ca sistem de operare de zi cu zi. Asigura\376i-v\343 copii ale datelor",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "\356n cazul \356n care \356ncerca\376i ReactOS \356n mod neemulat.",
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
        8,
        19,
        "\x07  Tasta\376i ENTER pentru a instala ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tasta\376i F3 pentru a ie\272i f\343r\343 a instala ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie\272ire",
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

static MUI_ENTRY roROLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licen\376iere:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "Sistemul de operare ReactOS este oferit \356n termenii Licen\376ei",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "Publice Generale GNU, referit\343 \356n continuare ca GPL, cu p\343r\376i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "de cod din alte licen\376e compatibile (ca X11, BSD, \272i LGPL).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Toate componentele care fac parte din sistemul ReactOS sunt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "prin urmare oferite sub licen\376a GPL, men\376in\342ndu-\272i astfel \272i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "licen\376ierea original\343 \356n acela\272i timp.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "Acest sistem vine f\343r\343 vreo restric\376ie de utilizare, aceasta",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "fiind o condi\376ie legislativ\343 aplicabil\343 at\342t la nivel local",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "c\342t \272i interna\376ional. Licen\376ierea se refer\343 doar la distri-",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "buirea sistemului ReactOS c\343tre p\343r\376i ter\376e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "Dac\343 din vreun careva motiv nu de\376ine\376i o copie a licen\376ei",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "GPL \356mpreun\343 cu ReactOS, o pute\376i consulta (\356n englez\343)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        23,
        "acces\342nd pagina:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        27,
        "Garan\376ie:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "Acest sistem de operare este distribuit doar \356n speran\376a c\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        30,
        "va fi util, neav\342nd \356ns\343 ata\272at\343 NICI O GARAN\336IE; nici m\343car",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        31,
        "garan\376ia implicit\343 a VANDABILIT\303\336II sau a UTILIT\303\336II \316NTR-UN",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        32,
        "SCOP ANUME.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Revenire",
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

static MUI_ENTRY roRODevicePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Configurare dispozitive de baz\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Arhitectur\343 de calcul:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Parametri grafici:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Model tastatur\343:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Aranj. tastatur\343:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Accept\343:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16,
        "Accept configura\376ia dispozitivelor",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Pute\376i modifica starea curent\343. Utiliza\376i tastele SUS/JOS pentru",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "alegerea unui dispozitiv, apoi ap\343sa\376i ENTER pentru a-i modifica",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "configura\376ia ata\272at\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "C\342nd configura\376ia dispozitivele enumerate este cea corect\343,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "selecta\376i \"Accept configura\376ia dispozitivelor\", apoi confirma\376i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        25,
        "ap\343s\342nd ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie\272ire",
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

static MUI_ENTRY roRORepairPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Programul de instalare ReactOS este \356nc\343 \356ntr-o faz\343 incipient\343 de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "dezvoltare \272i nu posed\343 o func\376ionalitate complet\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Func\376ionalitatea de refacere \356nc\343 nu este implementat\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tasta\376i U pentru actualizarea sistemului.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tasta\376i R pentru consola de Recuperare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tasta\376i ESC pentru a reveni la pagina principal\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tasta\376i ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Revenire  U = Actualizare  R = Recuperare  ENTER = Repornire",
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

static MUI_ENTRY roROUpgradePageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Programul de Instalare ReactOS ofer\343 actualizarea urm\343toarelor instal\343ri",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "existente, sau, dac\343 o instalare este deteriorat\343, programul de instalare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "poate \356ncerca s\343 o repare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Func\376ionalitatea de reparare \356nc\343 nu este complet implementat\343.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tasta\376i SUS sau JOS pentru a selecta o instalare existent\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tasta\376i U pentru a actualiza instalarea selectat\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tasta\376i ESC pentru a continua cu o nou\343 instalare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tasta\376i F3 pentru a ie\272i f\343r\343 a instala ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Actualizare   ESC = Evitare actualizare   F3 = Ie\272ire",
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

static MUI_ENTRY roROComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Dori\376i specificarea unei alte arhitecturi de calcul?",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Utiliza\376i tastele SUS/JOS pentru a selecta o",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   arhitectur\343 de calcul, apoi ap\343sa\376i ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tasta\376i ESC pentru a reveni la pagina precedent\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   f\343r\343 a specifica o alt\343 arhitectur\343 de calcul.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie\272ire",
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

static MUI_ENTRY roROFlushPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Sistemul verific\343 integritatea datelor scrise pe disc.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Aceasta poate dura c\342teva momente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "La final, calculatorul va fi repornit automat.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Eliberare memorie...",
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

static MUI_ENTRY roROQuitPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS nu a fost instalat \356n \356ntregime.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Scoate\376i discul flexibil din unitatea A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "\272i toate mediile CD-ROM din unit\343\376ile CD.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tasta\376i ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A\272tepta\376i...",
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

static MUI_ENTRY roRODisplayPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Dori\376i modificarea parametrilor grafici de afi\272are?",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Utiliza\376i tastele SUS/JOS pentru a selecta",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   un grup de parametri, apoi ap\343sa\376i ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tasta\376i ESC pentru a reveni la pagina precedent\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   fara a modifica parametrii grafici actuali.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie\272ire",
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

static MUI_ENTRY roROSuccessPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Componentele de baz\343 ale ReactOS au fost instalate cu succes.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Scoate\376i discul flexibil din unitatea A: \272i toate mediile",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "CD-ROM din unit\343\376ile CD.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tasta\376i ENTER pentru a reporni calculatorul.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Repornire calculator",
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

static MUI_ENTRY roROSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Urm\343toarea list\343 cuprinde parti\376iile existente, precum \272i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "spa\376iul liber disponibil pentru crearea de noi parti\376ii.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Utiliza\376i tastele SUS/JOS pentru a selecta o op\376iune.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tasta\376i ENTER pentru a instala pe parti\376ia selectat\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tasta\376i C pentru a crea o parti\376ie primar\343/logic\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tasta\376i E pentru a crea o parti\376ie extins\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tasta\376i D pentru a \272terge o parti\376ie existent\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A\272tepta\376i...",
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

static MUI_ENTRY roROChangeSystemPartition[] =
{
    {
        4,
        3,
        " Instalare " KERNEL_VERSION_STR " ReactOS ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Parti\376ia curent\343 a calculatorului dumneavoastr\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "pe un disc de sistem",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "folose\272te un format ce nu este suportat de ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "Pentru a instala cu succes ReactOS, programul de instalare trebuie s\343 schimbe",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "parti\376ia curent\343 a sistemului cu una nou\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Noua parti\376ie de sistem nominalizat\343 este:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Pentru a accepta aceast\343 alegere, ap\343sa\376i ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Pentru a schimba manual parti\376ia sistemului, ap\343sa\376i ESC pentru a merge \356napoi la",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   parti\376ia din lista de selec\376ie, apoi selecta\376i sau crea\376i o nou\343 parti\376ie",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   de sistem pe discul de sistem.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "\316n cazul \356n care mai sunt \272i alte sisteme de operare care depind de parti\376ia original\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "a sistemului, ve\376i fi nevoi ca fie s\343 le reconfigura\376i pentru noua",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "parti\376ie a sistemului, fie ve\376i fi nevoit s\343 schimba\376i parti\376ia sistemului \356napoi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "la cea original\343, dup\343 terminarea instal\343rii ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Ie\272ire",
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

static MUI_ENTRY roROConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR,
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A\376i solicitat \272tergerea parti\376iei de sistem.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Parti\376iile de sistem pot con\376ine programe de diagnoz\343, programe de con-",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "figurare a echipamentelor, programe de lansare a unui sistem de operare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "(ca ReactOS) \272i alte programe furnizate de produc\343torii calculorului.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\252terge\376i o parti\376ie de sistem doar c\342nd fie ave\376i siguran\376a c\343 nu ve\376i",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "\272terge accidental a\272a programe pe parti\376ie, fie dori\376i s\343 le \272terge\376i.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "\252terg\342nd parti\376ia apare riscul de a nu mai putea porni calculatorul de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "pe discul local dec\342t dup\343 finalizarea instal\343rii ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Tasta\376i ENTER pentru a \272terge parti\376ia de sistem. Vi se va",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   cere s\343 confirma\376i din nou aceast\343 \272tergere a parti\376iei.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Tasta\376i ESC pentru a reveni la pagina precedent\343. Parti\376ia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   nu va fi \272tears\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare",
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

static MUI_ENTRY roROFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Urmeaz\343 formatarea parti\376iei.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Urmeaz\343 formatarea parti\376iei. Tasta\376i ENTER pentru a continua.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie\272ire",
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

static MUI_ENTRY roROCheckFSEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Programul de instalare verific\343 acum parti\376ia aleas\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "A\272tepta\376i...",
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

static MUI_ENTRY roROInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Alege\376i un director de instalare pe parti\376ia aleas\343.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Alege\376i un director \356n care va fi instalat ReactOS:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Pute\376i indica un alt director, ap\343s\342nd BACKSPACE pentru",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "a \272terge caractere, apoi scriind calea directorului unde",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "dori\376i s\343 instala\376i ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie\272ire",
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

static MUI_ENTRY roROFileCopyEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Are loc copierea de fi\272iere \356n directorul ReactOS specificat.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "(aceasta poate dura c\342teva momente)",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 A\272tepta\376i...",
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

static MUI_ENTRY roROBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
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
        "Instaleaz\343 ini\376ializatorul pe discul intern (MBR \272i VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Instaleaz\343 ini\376ializatorul pe discul intern (doar VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Instaleaz\343 ini\376ializatorul pe un disc flexibil.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Omite instalarea modulului de ini\376ializare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie\272ire",
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

static MUI_ENTRY roROBootLoaderInstallPageEntries[] =
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
        "Instalare modul de ini\376ializare al calculatorului.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Instalarea programului de pornire pe suport, v\343 rug\343m s\343 a\272tepta\376i...",
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

static MUI_ENTRY roROBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalare modul de ini\376ializare al calculatorului.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Introduce\376i un disc flexibil formatat \356n unitatea A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "apoi s\343 ap\343sa\376i ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   F3 = Ie\272ire",
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

static MUI_ENTRY roROKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC,
    },
    {
        6,
        8,
        "Dori\376i specificarea modelului tastaturii instalate?",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Utiliza\376i tastele SUS/JOS pentru a selecta un model",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   de tastatur\343, apoi ap\343sa\376i ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tasta\376i ESC pentru a reveni la pagina precedent\343 f\343r\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   a schimba modelul tastaturii curente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie\272ire",
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

static MUI_ENTRY roROLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Dori\376i specificarea unui aranjament implicit de tastatur\343?",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Utiliza\376i tastele SUS/JOS pentru a selecta un aranjament",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    dorit de tastatur\343, apoi ap\343sa\376i ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tasta\376i ESC pentru a reveni la pagina precedent\343 f\343r\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   a schimba aranjamentul curent al tastaturii.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie\272ire",
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

static MUI_ENTRY roROPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Au loc preg\343tirile necesare pentru copierea de fi\272iere.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "List\343 de fi\272iere \356n curs de creare...",
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

static MUI_ENTRY roROSelectFSEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Alege\376i un sistem de fi\272iere din lista de mai jos.",
        0
    },
    {
        8,
        16,
        "\x07  Utiliza\376i tastele SUS/JOS pentru a selecta",
        0
    },
    {
        8,
        17,
        "   un sistem de fi\272iere.",
        0
    },
    {
        8,
        19,
        "\x07  Tasta\376i ENTER pentru a formata parti\376ia.",
        0
    },
    {
        8,
        21,
        "\x07  Tasta\376i ESC pentru a alege o alt\343 parti\376ie.",
        0
    },
    {
        0,
        0,
        "ENTER = Continuare   ESC = Anulare   F3 = Ie\272ire",
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

static MUI_ENTRY roRODeletePartitionEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A\376i ales s\343 \272terge\376i parti\376ia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Tasta\376i L pentru a \272terge parti\376ia.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "ATEN\336IE: Toate datele de pe aceast\343 parti\376ie vor fi pierdute!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tasta\376i ESC pentru a anula.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = \252tergere parti\376ie   ESC = Anulare   F3 = Ie\272ire",
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

static MUI_ENTRY roRORegistryEntries[] =
{
    {
        4,
        3,
        " Instalare ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Are loc actualizarea configura\376iei sistemului.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Registru \356n curs de creare...",
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

MUI_ERROR roROErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Succes\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS nu a fost instalat \356n totalitate \356n calculator.\n"
        "Dac\343 abandona\376i instalarea acum, alt\343 dat\343, pentru a\n"
        "instala ReactOS, va fi nevoie s\343 repeta\376i to\376i pa\272ii.\n"
        "\n"
        "  \x07  Tasta\376i ENTER pentru a continua instalarea.\n"
        "  \x07  Tasta\376i F3 pentru a abandona instalarea.",
        "F3 = Ie\272ire  ENTER = Continuare"
    },
    {
        // ERROR_NO_BUILD_PATH
        "E\272ec \356n construirea c\343ilor de instalare pentru directorul de instalare ReactOS!\n"
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_SOURCE_PATH
        "Nu pute\376i \272terge parti\376ia surs\343 de instalare!\n"
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_SOURCE_DIR
        "Nu pute\376i instala ReactOS \356n directorul surs\343 de instalare!\n"
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_NO_HDD
        "E\272ec la identificarea unit\343\376ilor interne de stocare.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "E\272ec la accesarea unitat\343\376ii de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "E\272ec la \356nc\343rcarea fi\272ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Fi\272ieul TXTSETUP.SIF a fos g\343sit deteriorat.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Fi\272ierul TXTSETUP.SIF con\376ine o semn\343tur\343 nevalid\343.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "E\272ec la ob\376inerea de informa\376ii despre\n"
        "dispozitivele din calculator.\n",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_WRITE_BOOT,
        "E\272ec la instalarea codului %S de ini\376ializare\n"
        "pe parti\376ia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "E\272ec la \356nc\343rcarea listei cu arhitecturi de\n"
        "calcul disponibile.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "E\272ec la \356nc\343rcarea listei cu parametri de\n"
        "afi\272are pentru ecran.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "E\272ec la \356nc\343rcarea listei cu tipuri\n"
        "disponibile de tastatur\343.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "E\272ec la \356nc\343rcarea listei de configura\376ii\n"
        "ale tastaturii.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_WARN_PARTITION,
        "A fost g\343sit\343 cel pu\376in un disc cu tabel\343 nerecunoscut\343\n"
        "de parti\376ii, care nu poate fi gestionat\343 corespunz\343tor!\n"
        "\n"
        "Crearea sau \272tergerea de parti\376ii poate astfel cauza\n"
        "distrugerea tabelei de parti\376ii."
        "\n"
        "  \x07  Tasta\376i F3 pentru a abandona instalarea.\n"
        "  \x07  Tasta\376i ENTER pentru a continua.",
        "F3 = Ie\272ire  ENTER = Continuare"
    },
    {
        // ERROR_NEW_PARTITION,
        "O parti\376ie nou\343 nu poate fi creat\343 \356n interiorul\n"
        "unei parti\376ii existente!\n"
        "\n"
        "  * Tasta\376i pentru a continua.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "E\272ec la instalarea codului %S de ini\376ializare\n"
        "pe parti\376ia de sistem.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_NO_FLOPPY,
        "Nu exist\343 discuri flexibile \356n unitatea A:",
        "ENTER = Continuare"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "E\272ec la actualizarea configura\376iei de tastatur\343.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "E\272ec la actualizarea registrului cu\n"
        "parametrii grafici ai ecranului!",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_IMPORT_HIVE,
        "E\272ec la importarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_FIND_REGISTRY
        "E\272ec la localizarea fi\272ierelor\n"
        "cu datele registrului.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_CREATE_HIVE,
        "E\272ec la crearea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "E\272ec la ini\376ializarea registrului.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Fi\272ierul cabinet nu con\376ine nici\n"
        "un fi\272ier valid de tip inf.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_CABINET_MISSING,
        "E\272ec la localizarea fi\272ierului cabinet.\n",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Fi\272ierul cabinet nu con\376ine nici\n"
        "un script de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_COPY_QUEUE,
        "E\272ec la accesarea listei cu\n"
        "fi\272iere pentru copiere.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_CREATE_DIR,
        "E\272ec la crearea directoarelor de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "E\272ec la identificarea sec\376iunii\n"
        "'%S' \356n fi\272ierul TXTSETUP.SIF.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_CABINET_SECTION,
        "E\272ec la identificarea sec\376iunii\n"
        "'%S' \356n fi\272ierul cabinet.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "E\272ec la crearea directorului de instalare.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_WRITE_PTABLE,
        "E\272ec la scrierea tabelelor de parti\376ii.\n",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "E\272ec la includerea pagin\343rii \356n registru.\n",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Instalatoprul nu a putut seta loca\376ia sistemului.\n",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "E\272ec la includerea \356n registru a configura\376iei\n"
        "de tastatur\343.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Instalatorul nu a putut seta geo id.",
        "ENTER = Repornire calculator"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Numele de director este nevalid.\n"
        "\n"
        "  * Tasta\376i pentru a continua."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Parti\376ia selectat\343 este prea mic\343 pentru a instala ReactOS.\n"
        "Parti\376ia de instalare trebuie s\343 aib\343 cel pu\376in %lu Mocte\376i.\n"
        "\n"
        "  * Tasta\376i pentru a continua.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Nu poate fi creat\343 o nou\343 parti\376ie primar\343 sau extins\343 \356n tabela\n"
        "de parti\376ii a acestui disc deoarece tabela de parti\376ii e plin\343.\n"
        "\n"
        "  * Tasta\376i pentru a continua."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Nu poate fi creat\343 mai mult de o parti\376ie extins\343 pe un disc.\n"
        "\n"
        "  * Tasta\376i pentru a continua."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "E\272ec la formatarea parti\376iei:\n"
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
        SETUP_INIT_PAGE,
        roROSetupInitPageEntries
    },
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
        REPAIR_INTRO_PAGE,
        roRORepairPageEntries
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
        CHANGE_SYSTEM_PARTITION,
        roROChangeSystemPartition
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
        CHECK_FILE_SYSTEM_PAGE,
        roROCheckFSEntries
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
        BOOTLOADER_SELECT_PAGE,
        roROBootLoaderSelectPageEntries
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
        BOOTLOADER_INSTALL_PAGE,
        roROBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        roROBootLoaderRemovableDiskPageEntries
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
     "   A\272tepta\376i..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Instalare   C/E = Creare parti\376ie Primar\343/Extins\343   F3 = Ie\272ire"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalare   C = Creare parti\376ie Logic\343   F3 = Ie\272ire"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalare   D = \252tergere parti\376ie   F3 = Ie\272ire"},
    {STRING_DELETEPARTITION,
     "   D = \252tergere parti\376ie   F3 = Ie\272ire"},
    {STRING_PARTITIONSIZE,
     "M\343rimea noii parti\376ii:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "A\376i ales crearea unei parti\376ii primare pe"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "A\376i ales crearea unei parti\376ii extinse pe"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "A\376i ales crearea unei parti\376ii logice pe"},
    {STRING_HDPARTSIZE,
    "Introduce\376i m\343rimea noii parti\376ii \356n megaocte\376i."},
    {STRING_CREATEPARTITION,
     "   ENTER = Creare parti\376ie   ESC = Anulare   F3 = Ie\272ire"},
    {STRING_NEWPARTITION,
    "O nou\343 parti\376ie a fost creat\343 \356n"},
    {STRING_PARTFORMAT,
    "Aceast\343 parti\376ie urmeaz\343 s\343 fie formatat\343."},
    {STRING_NONFORMATTEDPART,
    "Alege\376i s\343 instala\376i ReactOS pe parti\376ie nou\343 sau neformatat\343."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Parti\376ia de sistem \356nc\343 nu a fost formatat\343."},
    {STRING_NONFORMATTEDOTHERPART,
    "Noua parti\376ie \356nc\343 nu a fost formatat\343."},
    {STRING_INSTALLONPART,
    "ReactOS va fi instalat pe parti\376ia"},
    {STRING_CONTINUE,
    "ENTER = Continuare"},
    {STRING_QUITCONTINUE,
    "F3 = Ie\272ire  ENTER = Continuare"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Repornire calculator"},
    {STRING_DELETING,
     "   Copiere fi\272ier: %S"},
    {STRING_MOVING,
     "   Mutare fi\272ier: %S to: %S"},
    {STRING_RENAMING,
     "   Redenumire fi\272ier: %S to: %S"},
    {STRING_COPYING,
     "   Fi\272ierul curent: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Fi\272iere \356n curs de copiere..."},
    {STRING_REGHIVEUPDATE,
    "   Registru \356n curs de actualizare..."},
    {STRING_IMPORTFILE,
    "   \316n curs de importare din %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Registru de configura\376ie grafic\343 \356n actualizare..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Particularit\343\376i locale \356n actualizare..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Configura\376ie de tastatur\343 \356n actualizare..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Date de paginare \356n curs de adaugare..."},
    {STRING_DONE,
    "   Terminat!"},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Repornire calculator"},
    {STRING_REBOOTPROGRESSBAR,
    " Calculatorul va reporni \356n %li secunde... "},
    {STRING_CONSOLEFAIL1,
    "E\272ec la deschiderea consolei\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Cea mai frecvent\343 cauz\343 pentru asta este utilizarea unei tastaturi USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Tastaturile USB nu sunt \356nc\343 toate acceptate\r\n"},
    {STRING_FORMATTINGPART,
    "Parti\376ia \356n curs de formatare..."},
    {STRING_CHECKINGDISK,
    "Disc \356n curs de verificare..."},
    {STRING_FORMATDISK1,
    " Formateaz\343 parti\376ia ca sistem de fi\272iere %S (formatare rapid\343) "},
    {STRING_FORMATDISK2,
    " Formateaz\343 parti\376ia ca sistem de fi\272iere %S "},
    {STRING_KEEPFORMAT,
    " P\343streaz\343 sistemul de fi\272iere actual (f\343r\343 schimb\343ri) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "de pe %s."},
    {STRING_PARTTYPE,
    "Tip 0x%02x"},
    {STRING_HDDINFO1,
    // "Discul %lu (%I64u %s), Port=%hu, Magistrala=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Discul %lu (Port=%hu, Magistrala=%hu, Id=%hu) de tip %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Discul %lu (%I64u %s), Port=%hu, Magistrala=%hu, Id=%hu [%s]"
    "%I64u %s Discul %lu (Port=%hu, Magistrala=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Spa\376iu neparti\376ionat"},
    {STRING_MAXSIZE,
    "Mo (max. %lu Mo)"},
    {STRING_EXTENDED_PARTITION,
    "Parti\376ie extins\343"},
    {STRING_UNFORMATTED,
    "Part. nou\343 (neformatat\343)"},
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
    "Ad\343ugare configura\376ii de tastatur\343"},
    {0, 0}
};
