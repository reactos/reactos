// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/* TRANSLATORS: 2013-2015 Erdem Ersoy (eersoy93) (erdemersoy [at] live [dot] com), 2018 Ercan Ersoy (ercanersoy) (ercanersoy [at] ercanersoy [dot] net) */

#pragma once

static MUI_ENTRY trTRSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "ReactOS Kurulumu kendini ba\237lat\215rken ve ayg\215tlar\215n\215z\215",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "alg\215larken l\201tfen bekleyin...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L\201tfen bekleyin...",
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

static MUI_ENTRY trTRLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Dil Se\207imi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  L\201tfen kurulum i\237lemi i\207in kullan\215lacak dili se\207iniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Ard\215ndan ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Bu dil, kurulacak sistemin varsay\215lan dili olacakt\215r.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et  F3 = \200\215k",
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

static MUI_ENTRY trTRWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Kurulum Sihirbaz\215na ho\237 geldiniz.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Kurulumun bu b\224l\201m\201, ReactOS \230\237letim Sistemi'ni bilgisayar\215n\215za",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "kopyalar ve kurulumun ikinci b\224l\201m\201n\201 haz\215rlar.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  ReactOS'u y\201klemek ya da y\201kseltmek i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  ReactOS'u onarmak veya y\201kseltmek i\207in R'ye bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  ReactOS Lisans Terimleri'ni ve \236artlar\215'n\215 g\224rmek i\207in L'ye bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS'u kurmadan \207\215kmak i\207in F3'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Daha \207ok bilgi i\207in l\201tfen u\247ray\215n\215z:",
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
        "ENTER = Devam Et  R = Onar veya Y\201kselt  L = Lisans  F3 = \200\215k",
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

static MUI_ENTRY trTRIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS S\201r\201m Durumu",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS alfa a\237amas\215ndad\215r, \224zellikleri tamamlanmam\215\237 anlam\215na gelmektedir",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "ve a\247\215r geli\237tirme alt\215ndad\215r. Yaln\215zca de\247erlendirme ve deneme amac\215yla",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "kullan\215m\215 \224nerilir ve g\201nl\201k kullan\215m i\237letim sisteminiz de\247ildir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "E\247er, ReactOS'u ger\207ek donan\215m \201zerinde \207al\215\237t\215rmay\215 deneyecekseniz",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "verilerinizi yedekleyiniz veya ikinci bir bilgisayar\215n\215zda deneyiniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  ReactOS Kur'a devam etmek i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS'u kurmadan \207\215kmak i\207in F3'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   F3 = \200\215k",
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

static MUI_ENTRY trTRLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Lisanslama:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS Sistemi, GNU GPL'yle X11, BSD ve GNU LGPL",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "lisanslar\215 gibi ba\237ka uygun lisanslardan kod i\207eren",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "b\224l\201mlerin \237artlar\215 alt\215nda lisanslanm\215\237t\215r.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Bu y\201zden ReactOS sisteminin b\224l\201m\201 olan t\201m yaz\215l\215mlar, korunan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "\224zg\201n lisanslar\215yla birlikte GNU GPL alt\215nda yay\215mlan\215r.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Bu yaz\215l\215m, yerel ve uluslararas\215 yasa uygulanabilir kullan\215m",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\201zerine hi\207bir g\201vence ve k\215s\215tlamayla gelmez. ReactOS'un",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "lisanslanmas\215 yaln\215zca \201\207\201nc\201 yanlara da\247\215tmay\215 kapsar.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "E\247er baz\215 nedenlerden dolay\215 ReactOS ile GNU Genel",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Kamu Lisans\215'n\215n bir kopyas\215n\215 almad\215ysan\215z l\201tfen u\247ray\215n\215z:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "G\201vence:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        23,
        "Bu \224zg\201r yaz\215l\215md\215r, kopyalama \237artlar\215 i\207in kayna\247a bak\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Burada hi\207bir g\201vence YOKTUR, SATILAB\230L\230RL\230K veya",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "BEL\230RL\230 B\230R AMACA UYGUNLUK i\207in bile.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Geri D\224n",
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

static MUI_ENTRY trTRDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A\237a\247\215daki liste \237imdiki ayg\215t ayarlar\215n\215 g\224sterir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Bilgisayar:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "G\224r\201nt\201:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Klavye:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Klavye D\201zeni:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Onayla:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16,
        "Bu ayg\215t ayarlar\215n\215 onayla.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Ayg\215t ayarlar\215n\215, bir se\207enek se\207mek i\207in YUKARI veya A\236A\246I",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "tu\237lar\215na basarak de\247i\237tirebilirsiniz. Sonra ba\237ka ayarlar se\207mek i\207in",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "ENTER tu\237una bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "T\201m ayarlar uygun oldu\247unda \"Bu ayg\215t ayarlar\215n\215 onayla\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "se\207ene\247ini se\207iniz ve ENTER tu\237una bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   F3 = \200\215k",
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

static MUI_ENTRY trTRRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Kur, bir \224n geli\237me evresindedir. Daha t\201m\201yle kullan\215\237l\215",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "bir kurulum uygulamas\215n\215n t\201m i\237levlerini desteklemez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Onarma i\237levleri daha bitirilmemi\237tir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  \230\237letim sistemini y\201kseltmek i\207in U'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Kurtarma Konsolu i\207in R'ye bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Ana sayfaya geri d\224nmek i\207in ESC'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Bilgisayar\215n\215z\215 yeniden ba\237latmak i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Ana Sayfa  U = Y\201kselt  R = Kurtarma  ENTER = Yeniden Ba\237lat",
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

static MUI_ENTRY trTRUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS Kur, a\237a\247\215da listelenen bir mevcut ReactOS kurulumunu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "y\201kseltebilir ya da ReactOS kurulumu zarar g\224rm\201\237se Kur, onu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "onarmay\215 deneyebilir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Onar\215m i\237levleri hen\201z tamamlanmam\215\237t\215r.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Bir i\237letim sistemi se\207mek i\207in YUKARI'ya ya da A\236A\246I'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Se\207ilen i\237letim sistemi kurulumunu y\201kseltmek i\207in U'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Yeni bir kuruluma devam etmek i\207in ESC'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS kurmadan \207\215kmak i\207in F3'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Y\201kselt   ESC = Y\201kseltme   F3 = \200\215k\215\237",
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

static MUI_ENTRY trTRComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kurulum yap\215lacak bilgisayar\215n t\201r\201n\201 se\207mek isteyebilirsiniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \230stenen bilgisayar t\201r\201n\201 se\207mek i\207in YUKARI'ya veya A\236A\246I'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Ard\215ndan ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Bilgisayar t\201r\201n\201 de\247i\237tirmeden bir \224nceki sayfaya",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   d\224nmek i\207in ESC tu\237una bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   ESC = \230ptal   F3 = \200\215k",
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

static MUI_ENTRY trTRFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Sistem, \237imdi diskinize kaydedilmi\237 t\201m veriyi do\247ruluyor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Bu bir dakika s\201rebilir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Bitti\247inde bilgisayar\215n\215z otomatik olarak yeniden ba\237layacakt\215r.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\231n bellek temizleniyor...",
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

static MUI_ENTRY trTRQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS, t\201m\201yle kurulmad\215.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "A: s\201r\201c\201s\201nden disketi ve t\201m CD s\201r\201c\201lerinden",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "CD-ROM'lar\215 \207\215kart\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Bilgisayar\215n\215z\215 yeniden ba\237latmak i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L\201tfen bekleyiniz...",
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

static MUI_ENTRY trTRDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kurulum yap\215lacak g\224r\201nt\201n\201n t\201r\201n\201 se\207mek isteyebilirsiniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \230stenen g\224r\201nt\201 t\201r\201n\201 se\207mek i\207in YUKARI'ya veya A\236A\246I'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Ard\215ndan ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  G\224r\201nt\201 t\201r\201n\201 de\247i\237tirmeden bir \224nceki sayfaya",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   d\224nmek i\207in ESC tu\237una bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   ESC = \230ptal   F3 = \200\215k",
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

static MUI_ENTRY trTRSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS'un ana bile\237enleri ba\237ar\215yla kuruldu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "A: s\201r\201c\201s\201nden disketi ve t\201m CD s\201r\201c\201lerinden",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "CD-ROM'lar\215 \207\215kart\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Bilgisayar\215n\215z\215 yeniden ba\237latmak i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat",
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

static MUI_ENTRY trTRSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A\237a\247\215daki liste, var olan b\224l\201mleri ve yeni b\224l\201mler i\207in",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "kullan\215lmayan disk bo\237lu\247unu g\224sterir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Bir liste girdisini se\207mek i\207in YUKARI'ya ya da A\236A\246I'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Se\207ili b\224l\201me ReactOS'u y\201klemek i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press C to create a primary/logical partition.",
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
        "\x07  Var olan bir b\224l\201m\224 silmek i\207in D'ye bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L\201tfen bekleyiniz...",
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

static MUI_ENTRY trTRChangeSystemPartition[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Bilgisayar\215n\215z\215n sistem diskindeki mevcut sistem",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "b\224l\201m\201, ReactOS taraf\215ndan desteklenmeyen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "bir bi\207im kullan\215yor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "ReactOS'u ba\237ar\215yla kurmak i\207in, Kurulum program\215 mevcut",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "sistem b\224l\201m\201n\201 yenisiyle de\247i\215tirmelidir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Yeni aday sistem b\224l\201m\201:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Bu se\207imi kabul etmek i\207in ENTER'a bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Sistem b\224l\201m\201n\201 elle de\247i\237tirmek i\207in ESC'e basarak geri d\224n\201n\201z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   ard\215ndan sistem diskinde yeni bir sistem b\224l\201m\201 se\207iniz",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   veya olu\237turunuz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "Orijinal sistem b\224l\201m\201ne ba\247l\215 olan ba\237ka i\237letim sistemleri olmas\215",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "durumunda, bunlar\215 yeni sistem b\224l\201m\201 i\207in yeniden yap\215land\215rman\215z",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "gerekebilir veya ReactOS kurulumunu tamamlad\215ktan sonra sistem ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "b\224l\201m\201n\201 orijinal b\224l\201m\201e geri d\224nd\201rmeniz gerekebilir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et  ESC = \230ptal",
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

static MUI_ENTRY trTRConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kur'un sistem b\224l\201m\201n\201 silmesini istediniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Sistem b\224l\201mleri; tan\215lama programlar\215, donan\215m yap\215land\215rma",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programlar\215, ReactOS gibi bir i\237letim sistemini ba\237latmak i\207in programlar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "veya donan\215m \201reticisi taraf\215ndan sa\247lanan ba\237ka programlar i\207erebilir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Yaln\215zca, b\224l\201mde b\224yle programlar\215n olmad\215\247\215ndan emin oldu\247unuzda ya da",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "onlar\215 silmek istedi\247inizden emin oldu\247unuzda bir sistem b\224l\201m\201n\201 siliniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "B\224l\201m\201 sildi\247inizde ReactOS Kur'u bitirene kadar bilgisayar\215 sabit diskten",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "ba\237latamayabilirsiniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Sistem b\224l\201m\201n\201 silmek i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "Sonra b\224l\201m\201 silmeyi yeniden onaylaman\215z istenecek.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Bir \224nceki sayfaya d\224nmek i\207in ESC'e bas\215n\215z. B\224l\201m silinmeyecek.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et  ESC = \230ptal",
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

static MUI_ENTRY trTRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "B\224l\201m Bi\207imlendirme",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Kur, \237imdi b\224l\201m\201 bi\207imlendirecek. Devam etmek i\207in ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Devam Et   F3 = \200\215k",
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

static MUI_ENTRY trTRCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kur, \237imdi se\207ili b\224l\201m\201 g\224zden ge\207iriyor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L\201tfen bekleyiniz...",
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

static MUI_ENTRY trTRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kur, se\207ili b\224l\201me ReactOS dosyalar\215n\215 y\201kler. ReactOS'un",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "y\201klenmesini istedi\247iniz bir dizin se\207iniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "\231nerilen dizini de\247i\237tirmek i\207in, karakterleri silmek i\207in BACKSPACE'e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "bas\215n\215z ve ard\215ndan ReactOS'un y\201klenmesini istedi\247iniz dizini yaz\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   F3 = \200\215k",
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

static MUI_ENTRY trTRFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "ReactOS Kur, ReactOS kurulum dizininize dosyalar\215 kopyalarken",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        " l\201tfen bekleyiniz.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Bu i\237lem birka\207 dakika s\201rebilir.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 L\201tfen bekleyiniz...",
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

static MUI_ENTRY trTRBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
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
        "\231ny\201kleyiciyi sabit diskin \201zerine kur. (MBR ve VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\231ny\201kleyiciyi sabit diskin \201zerine kur. (Yaln\215zca VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "\231ny\201kleyiciyi bir diskete kur.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\231ny\201kleyici kurulumunu ge\207.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   F3 = \200\215k",
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

static MUI_ENTRY trTRBootLoaderInstallPageEntries[] =
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
        "Kur, \224ny\201kleyiciyi kuruyor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\231ny\201kleyici diske kuruluyor, l\201tfen bekleyiniz...",
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

static MUI_ENTRY trTRBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kur, \224ny\201kleyiciyi kuruyor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "L\201tfen A: s\201r\201c\201s\201ne bi\207imlendirilmi\237 bir disket tak\215n\215z",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "ve ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = S\201rd\201r   F3 = \200\215k",
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

static MUI_ENTRY trTRKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kurulum yap\215lacak klavyenin t\201r\201n\201 se\207mek isteyebilirsiniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \230stenen klavye t\201r\201n\201 se\207mek i\207in YUKARI'ya veya A\236A\246I'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Ard\215ndan ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Klavye t\201r\201n\201 de\247i\237tirmeden bir \224nceki sayfaya",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   d\224nmek i\207in ESC tu\237una bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   ESC = \230ptal   F3 = \200\215k",
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

static MUI_ENTRY trTRLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "L\201tfen varsay\215lan olarak kurulacak bir klavye d\201zeni se\207iniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  \230stenen klavye d\201zenini se\207mek i\207in YUKARI'ya veya A\236A\246I'ya bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Ard\215ndan ENTER'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Klavye d\201zenini de\247i\237tirmeden bir \224nceki sayfaya",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   d\224nmek i\207in ESC'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Devam Et   ESC = \230ptal   F3 = \200\215k",
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

static MUI_ENTRY trTRPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kur, ReactOS dosyalar\215n\215 kopyalamak i\207in bilgisayar\215n\215z\215 haz\215rl\215yor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Dosya kopyalama listesi olu\237turuluyor...",
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

static MUI_ENTRY trTRSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "A\237a\247\215daki listeden bir dosya sistemi se\207iniz.",
        0
    },
    {
        8,
        19,
        "\x07  Bir dosya sistemi se\207mek i\207in YUKARI'ya veya A\236A\246I'ya bas\215n\215z.",
        0
    },
    {
        8,
        21,
        "\x07  B\224l\201m\201 bi\207imlendirmek i\207in ENTER'e bas\215n\215z.",
        0
    },
    {
        8,
        23,
        "\x07  Ba\237ka bir b\224l\201m se\207mek i\207in ESC'e bas\215n\215z.",
        0
    },
    {
        0,
        0,
        "ENTER = Devam Et   ESC = \230ptal   F3 = \200\215k",
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

static MUI_ENTRY trTRDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "B\224l\201m\201 silmeyi se\207tiniz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  B\224l\201m\201 silmek i\207in L'ye bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "UYARI: Bu b\224l\201mdeki t\201m veriler kaybedilecektir!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  \230ptal etmek i\207in ESC'e bas\215n\215z.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = B\224l\201m Sil   ESC = \230ptal   F3 = \200\215k",
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

static MUI_ENTRY trTRRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kur, sistem yap\215land\215rmas\215n\215 g\201ncelle\237tiriyor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Kay\215t y\215\247\215nlar\215 olu\237turuluyor...",
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

MUI_ERROR trTRErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Ba\237ar\215l\215\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS, bilgisayara t\201m\201yle kurulmad\215. E\247er \237imdi\n"
        "Kur'dan \207\215karsan\215z ReactOS'u kurmak i\207in Kur'u\n"
        "yeniden \207al\215\237t\215rman\215z gerekecektir.\n"
        "\n"
        "  \x07  Kur'a devam etmek i\207in ENTER'e bas\215n\215z.\n"
        "  \x07  Kur'dan \207\215kmak i\207in F3'e bas\215n\215z.",
        "F3 = \200\215k  ENTER = Devam Et"
    },
    {
        // ERROR_NO_BUILD_PATH
        "ReactOS kurulum dizini i\207in kurulum yollar\215 olu\237turulamad\215!!\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_SOURCE_PATH
        "Kurulum kayna\247\215n\215 i\207eren b\224l\201m\201 silemezsiniz!\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_SOURCE_DIR
        "ReactOS'u kurulum kaynak dizini i\207ine kuramazs\215n\215z!\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_NO_HDD
        "Kur, bir sabit disk bulamad\215.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Kur, kaynak s\201r\201c\201y\201 bulamad\215.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Kur, TXTSETUP.SIF dosyas\215n\215 y\201klemede ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Kur, bozuk bir TXTSETUP.SIF buldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Kur, TXTSETUP.SIF'ta ge\207ersiz bir imza buldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Kur, sistem s\201r\201c\201 bilgisini alamad\215.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_WRITE_BOOT,
        "Kur, sistem b\224l\201m\201ne %S \224ny\201kleme kodunu kuramad\215.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Kur, bilgisayar t\201r\201 listesini y\201klemede ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Kur, g\224r\201nt\201 ayarlar\215 listesini y\201klemede ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Kur, klavye t\201r\201 listesini y\201klemede ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Kur, klavye d\201zeni listesini y\201klemede ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_WARN_PARTITION,
        "Kur, d\201zg\201n y\224netilemeyecek uyumsuz bir b\224l\201m tablosu i\207eren\n"
        "en az bir sabit disk buldu!\n"
        "\n"
        "B\224l\201mleri olu\237turmak veya silmek, b\224l\201m tablosunu yok edebilir.\n"
        "\n"
        "  \x07  Kur'dan \207\215kmak i\207in F3'e bas\215n\215z.\n"
        "  \x07  Devam etmek i\207in ENTER'e bas\215n\215z.",
        "F3 = \200\215k   ENTER = Devam Et"
    },
    {
        // ERROR_NEW_PARTITION,
        "Zaten var olan bir b\224l\201m\201n i\207ine yeni\n"
        "bir b\224l\201m olu\237turamazs\215n\215z!\n"
        "\n"
        "  * Devam etmek i\207in bir tu\237a bas\215n\215z.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Kur, sistem b\224l\201m\201 \201zerinde %S \224ny\201kleme kodunu kurmada ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_NO_FLOPPY,
        "A: s\201r\201c\201s\201nde disk yok.",
        "ENTER = Devam Et"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Kur, klavye d\201zeni ayarlar\215n\215 g\201ncelle\237tirmede ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Kur, g\224r\201nt\201 kay\215t ayarlar\215n\215 g\201ncelle\237tirmede ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Kur, bir y\215\247\215n dosyas\215n\215 almada ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_FIND_REGISTRY
        "Kur, kay\215t veri dosyalar\215n\215 bulmada ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_CREATE_HIVE,
        "Kur, kay\215t y\215\247\215nlar\215n\215 olu\237turmada ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Kur, kay\215t defterini ba\237latmada ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Dolab\215n ge\207erli yap\215land\215rma dosyas\215 yok.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_CABINET_MISSING,
        "Dolap dosyas\215 bulunamad\215.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Dolap dosyas\215 kurulum beti\247i yok.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_COPY_QUEUE,
        "Kur, dosya kopyalama kuyru\247unu a\207mada ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_CREATE_DIR,
        "Kur, kurulum dizinlerini olu\237turmada ba\237ar\215s\215z oldu.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Kur, TXTSETUP.SIF'de '%S' b\224l\201m\201n\201\n"
        "bulmada ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_CABINET_SECTION,
        "Kur, dolapta '%S' b\224l\201m\201n\201\n"
        "bulmada ba\237ar\215s\215z oldu.\n",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Kur, kurulum dizinini olu\237turamad\215.",
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Kur, b\224l\201m tablolar\215 yazmada ba\237ar\215s\215z oldu.\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Kur, kay\215t defterine kod sayfas\215 eklemede ba\237ar\215s\215z oldu.\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Kur, sistem yerel ayar\215n\215 yapamad\215.\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Kur, kay\215t defterine klavye d\201zenleri eklemede ba\237ar\215s\215z oldu.\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Kur, co\247rafi kimli\247i ayarlayamad\215.\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Ge\207ersiz dizin ad\215.\n"
        "\n"
        "  * Devam etmek i\207in bir tu\237a bas\215n\215z."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Se\207ili b\224l\201m ReactOS'u kurmak i\207in yeteri katar b\201y\201k de\247il.\n"
        "Kurulum b\224l\201m\201 en az %lu MB b\201y\201kl\201\247\201nde olmal\215.\n"
        "\n"
        "  * Devam etmek i\207in bir tu\237a bas\215n\215z.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "B\224l\201m tablosu dolu oldu\247undan dolay\215 bu diskin b\224l\201m tablosunda yeni bir ana b\224l\201m\n"
        "ya da yeni bir geni\237letilmi\237 b\224l\201m olu\237turamazs\215n\215z.\n"
        "\n"
        "  * Devam etmek i\207in bir tu\237a bas\215n\215z."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Bir diskte birden \207ok geni\237letilmi\237 b\224l\201m olu\237turamazs\215n\215z.\n"
        "\n"
        "  * Devam etmek i\207in bir tu\237a bas\215n\215z."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Kur, b\224l\201m\201 bi\207imlendiremiyor:\n"
        " %S\n"
        "\n"
        "ENTER = Bilgisayar\215 Yeniden Ba\237lat"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE trTRPages[] =
{
    {
        SETUP_INIT_PAGE,
        trTRSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        trTRLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        trTRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        trTRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        trTRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        trTRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        trTRRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        trTRUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        trTRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        trTRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        trTRFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        trTRSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        trTRChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        trTRConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        trTRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        trTRFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        trTRCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        trTRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        trTRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        trTRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        trTRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        trTRKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        trTRBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        trTRLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        trTRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        trTRSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        trTRBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        trTRBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        trTRRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING trTRStrings[] =
{
    {STRING_PLEASEWAIT,
    "   L\201tfen bekleyiniz..."},
    {STRING_INSTALLCREATEPARTITION,
    "   ENTER = Kur  C = Ana B\224l\201m Olu\237tur  E = Geni\237letilmi\237 B\224l\201m Olu\237tur  F3 = \200\215k"},
    {STRING_INSTALLCREATELOGICAL,
    "   ENTER = Kur   C = Mant\215ksal B\224l\201m Olu\237tur   F3 = \200\215k"},
    {STRING_INSTALLDELETEPARTITION,
    "   ENTER = Kur   D = B\224l\201m\201 Sil   F3 = \200\215k"},
    {STRING_DELETEPARTITION,
    "   D = B\224l\201m\201 Sil   F3 = \200\215k"},
    {STRING_PARTITIONSIZE,
    "Yeni b\224l\201m\201n b\201y\201kl\201\247\201:"},
    {STRING_CHOOSE_NEW_PARTITION,
    "\232zerinde bir ana b\224l\201m olu\237turmay\215 se\207tiniz:"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
    "\232zerinde bir geni\237letilmi\237 b\224l\201m olu\237turmay\215 se\207tiniz:"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
    "\232zerinde bir mant\215ksal b\224l\201m olu\237turmay\215 se\207tiniz:"},
    {STRING_HDPARTSIZE,
    "L\201tfen yeni b\224l\201m\201n b\201y\201kl\201\247\201n\201 megabayt olarak giriniz."},
    {STRING_CREATEPARTITION,
    "   ENTER = B\224l\201m Olu\237tur   ESC = \230ptal   F3 = \200\215k"},
    {STRING_NEWPARTITION,
    "Kur, \201zerinde bir yeni b\224l\201m olu\237turdu:"},
    {STRING_PARTFORMAT,
    "Bu b\224l\201m ileride bi\207imlendirilecektir."},
    {STRING_NONFORMATTEDPART,
    "ReactOS'u yeni ya da bi\207imlendirilmemi\237 bir b\224l\201me kurmay\215 se\207tiniz."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Sistem b\224l\201m\201 hen\201z bi\207imlendirilmemi\237."},
    {STRING_NONFORMATTEDOTHERPART,
    "Yeni b\224l\201m hen\201z bi\207imlendirilmemi\237."},
    {STRING_INSTALLONPART,
    "Kur, ReactOS'u b\224l\201m \201zerine kurar."},
    {STRING_CONTINUE,
    "ENTER = Devam Et"},
    {STRING_QUITCONTINUE,
    "F3 = \200\215k   ENTER = Devam Et"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Bilgisayar\215 Yeniden Ba\237lat"},
    {STRING_DELETING,
     "   Dosya siliniyor: %S"},
    {STRING_MOVING,
     "   Dosya ta\237\215n\215yor: %S \237uraya: %S"},
    {STRING_RENAMING,
     "   Dosya yeniden adland\215r\215l\215yor: %S \237uraya: %S"},
    {STRING_COPYING,
    "   Dosya kopyalan\215yor: %S"},
    {STRING_SETUPCOPYINGFILES,
    "Kur, dosyalar\215 kopyal\215yor..."},
    {STRING_REGHIVEUPDATE,
    "   Kay\215t y\215\247\215nlar\215 g\201ncelle\237tiriliyor..."},
    {STRING_IMPORTFILE,
    "   Al\215n\215yor: %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   G\224r\201nt\201 ayarlar\215 de\247erleri g\201ncelle\237tiriliyor..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Yerel ayarlar g\201ncelle\237tiriliyor..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Klavye d\201zeni ayarlar\215 g\201ncelle\237tiriliyor..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Kod sayfas\215 bilgisi ekleniyor..."},
    {STRING_DONE,
    "   Bitti..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Bilgisayar\215 Yeniden Ba\237lat"},
    {STRING_REBOOTPROGRESSBAR,
    " Bilgisayar\215n\215z %li saniye sonra yeniden ba\237lat\215lacak... "},
    {STRING_CONSOLEFAIL1,
    "Konsol a\207\215lam\215yor.\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Bunun en bilinen nedeni, bir USB klavye kullan\215lmas\215d\215r.\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB klavye daha t\201m\201yle desteklenmemektedir.\r\n"},
    {STRING_FORMATTINGPART,
    "Kur, b\224l\201m\201 bi\207imlendiriyor..."},
    {STRING_CHECKINGDISK,
    "Kur, diski g\224zden ge\207iriyor..."},
    {STRING_FORMATDISK1,
    " B\224l\201m\201 %S dosya sistemiyle h\215zl\215 bi\207imlendir. "},
    {STRING_FORMATDISK2,
    " B\224l\201m\201 %S dosya sistemiyle bi\207imlendir. "},
    {STRING_KEEPFORMAT,
    " \236imdiki dosya sistemini koru. (De\247i\237iklik yok.) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "\232zerinde: %s."},
    {STRING_PARTTYPE,
    "T\201r 0x%02x"},
    {STRING_HDDINFO1,
    // "Sabit Disk %lu (%I64u %s), Giri\237=%hu, Veriyolu=%hu, Kimlik=%hu (%wZ) [%s]"
    "%I64u %s Disk %lu (Giri\237=%hu, VYolu=%hu, Kimlik=%hu), (%wZ) [%s]"},
    {STRING_HDDINFO2,
    // "Sabit Disk %lu (%I64u %s), Giri\237=%hu, Veriyolu=%hu, Kimlik=%hu [%s]"
    "%I64u %s Disk %lu (Giri\237=%hu, VYolu=%hu, Kimlik=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Kullan\215lmayan Alan"},
    {STRING_MAXSIZE,
    "MB (En \207ok %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Geni\237letilmi\237 B\224l\201m"},
    {STRING_UNFORMATTED,
    "Yeni (Bi\207imlendirilmemi\237)"},
    {STRING_FORMATUNUSED,
    "Kullan\215lmayan"},
    {STRING_FORMATUNKNOWN,
    "Bilinmeyen"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Klavye d\201zenleri ekleniyor..."},
    {0, 0}
};
