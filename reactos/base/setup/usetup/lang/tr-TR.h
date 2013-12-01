/* TRANSLATOR: 2013 - Erdem Ersoy (eersoy93) (erdemersoy@live.com) */

#pragma once

MUI_LAYOUTS trTRLayouts[] =
{
    { L"041F", L"0000041F" },
    { L"041F", L"0001041f" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY trTRLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Dil Se‡imi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Kurulum s?ras?nda kullan?lacak dili se‡iniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ard?ndan giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Bu dil, kurulacak dizgenin ”ntan?ml? dili olacakt?r.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Kur'a ho?geldiniz.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Kurulumun bu a?amas?, ReactOS k?t?klerini bilgisayara ‡oa§lt?r",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "ve kurulumun ikinci a?amas?n? an?klar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ReactOS'u kurmak i‡in giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ReactOS'u onarmak veyƒ y?kseltmek i‡in R d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ReactOS Ruhsat Ko?ullar?'n? g”r?nt?lemek i‡in L d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ReactOS'u kurmadan ‡?kmak i‡in F3 d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Daha ‡ok bilgi i‡in buraya gidiniz:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "Giri? = S?rd?r  R = Onar veyƒ Y?kselt  L = Ruhsat Ko?ullar? F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Kur, ”n geli?tirme evresinde oldu§undan daha",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "her t?rl? i?levi desteklemez.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Desteklenmeyen i?levler ?unlard?r:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Kur, bir diskte birden fazla birincil b”l?m? y”netemez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Kur, bir diskte, geni?letilmi? bir b”l?m oldu§u s?rece",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  o diskteki birincil b”l?m? silemez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Kur, bir diskte, di§er geni?letilmi? b”l?mlerin oldu§u s?rece",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  o diskteki ilk geni?letilmi? b”l?m? silemez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Kur, yaln?zca FAT k?t?k dizgelerini destekler.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- K?t?k dizgesi denetimi daha bitirilmemi?tir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  ReactOS'u kurmak i‡in giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  ReactOS'u kurmadan ‡?kmak i‡in F3 d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Ruhsat Ko?ullar?:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS, GNU GPL'nin yan?s?ra X11, BSD ve",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU LPGL gibi di§er uygun ruhsatlardan al?nan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "d?zg?leri i‡eren b”l?mlerin ko?ullar?yla ruhsatlanm??t?r.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Bu y?zden ReactOS'un t?m par‡alar?, GNU GPL ile birlikte",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "di§er ”zg?n ruhsatlarla yay?nlan?r.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Bu yaz?l?m, mahallŒ ve uluslararas? yasalar?n uygunlu§u a‡?s?ndan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "hi‡bir g?venceyle ve hi‡bir k?s?tlamayla gelmez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "ReactOS'un ruhsatlanmas?, yaln?zca ?‡?nc? ki?ilere da§?tmay? kapsar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "E§er birtak?m nedenlerle ReactOS ile birlikte GNU Um–mŒ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Kamu Ruhsat?'n? elde edememi?seniz buraya gidiniz:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        21,
        "G?vence:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        23,
        "Bu bir ?cretsiz yaz?l?md?r, ‡o§altma ko?ullar? i‡in kayna§a bak?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "Burada hi‡bir g?vence YOKTUR, hele hele SATILAB?L?RL?K veyƒ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "BEL?RL? B?R AMACA UYGUNLUK a‡?s?ndan.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = Geri D”n",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A?a§?daki dizelge, ?imdiki ayg?t ayarlar?n? g”sterir.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Bilgisayar T?r?:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "G”r?nt? Ayarlar?:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "D?§me Tak?m? T?r?:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "D?§me Tak?m? D?zeni:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Onayla:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16,
		"Bu ayg?t ayarlar?n? onayla.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Ayg?t ayarlar?n?, bir se‡ene§i se‡mek i‡in yukar? veyƒ a?a§? d?§melerine",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "basarak de§i?tirebilirsiniz. Se‡tikten sonra giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        22,
        "De§i?tirme i?leminden sonra \"Bu ayg?t ayarlar?n? onayla.\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "se‡ene§ini se‡iniz, ard?ndan giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Kur, ”n geli?tirme evresinde oldu§undan daha",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "her t?rl? i?levi desteklemez.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Onarma i?levi daha bitirilmemi?tir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ??letim dizgesini y?kseltmek i‡in U d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Kurtarma Konsolu i‡in R d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Bir ”nceki sayfaya geri d”nmek i‡in ‡?k?? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Bilgisayar? yeniden ba?latmak i‡in giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "€?k?? = Geri D”n  U = Y?kselt  R = Kurtar  Giri? = Bilgisayar? Yeniden Ba?lat",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kurulum yap?lacak bilgisayar?n t?r?n? se‡iniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Se‡mek istedi§iniz bilgisyar t?r?n? yukar? ve a?a§? d?§meleriyle",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   se‡iniz, ard?ndan giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Bilgisayar t?r?nde hi‡bir de§i?iklik yap?lmadan bir ”nceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   geri d”nmek i‡in ‡?k?? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   €?k?? = ?ptal   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Dizge, ?imdi diskteki t?m verileri onayl?yor.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Bu i?lem bir dakŒka s?rebilir.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "???lem bitti§inde bilgisayar yeniden ba?layacakt?r.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "™nbellek temizleniyor...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS, b?t?n?yle kurulmad?.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "A: s?r?c?s?ndeki disketi ve",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "t?m CD s?r?c?lerindeki CD-ROM'lar? ‡?kart?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Bilgisayar? yeniden ba?latmak i‡in giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "L?tfen bekleyiniz...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kurulum yap?lacak bilgisayar?n g”r?nt? ayarlar?n? se‡iniz.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  Se‡mek istedi§iniz g”r?nt? ayarlar?n? yukar? ve a?a§? d?§meleriyle",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   se‡iniz, ard?ndan giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  G”r?nt? ayarlar?nda hi‡bir de§i?iklik yap?lmadan bir ”nceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   geri d”nmek i‡in ‡?k?? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   €?k?? = ?ptal   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS'un ana bile?enleri ba?ar?l? bir ?ekilde kuruldu.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "A: s?r?c?s?ndeki disketi ve",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "t?m CD s?r?c?lerindeki CD-ROM'lar? ‡?kart?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Bilgisayar? yeniden ba?latmak i‡in giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = Bilgisayar? Yeniden Ba?lat",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY trTRBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kur, diske ”ny?kleyiciyi kuramad?.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "A: s?r?c?s?ne bi‡imlendirilmi? bir disk tak?n?z, ard?ndan",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "A?a§?daki dizelge, var olan b”l?mlerle yeni b”l?mler olu?turmak i‡in",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "kullan?lmayan bo?lu§u g”sterir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Bir ”§eyi se‡mek i‡in yukar? veyƒ a?a§? d?§melerine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ReactOS'u se‡ili b”l?me y?klemek i‡in giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Se‡ili bo?luktan yeni bir b”l?m olu?turmak i‡in C d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Se‡ili b”l?m? silmek i‡in D d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "L?tfen bekleyiniz...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kur, ?imdi se‡ti§iniz b”l?m? bi‡imlendirecek.",
        TEXT_STYLE_NORMAL
    },
	{
        6,
        10,
        "S?rd?rmek i‡in giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY trTRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kur, se‡ili b”l?me ReactOS k?t?klerini y?kleyecek. ReactOS'un y?klenece§i",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "dizini se‡iniz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "™nerilen dizini de§i?tirmek i‡in geri d?§mesi ile damgalar? siliniz,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "ard?ndan ReactOS'un kurulaca§? yeni dizini yaz?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Kur, ReactOS'un kurulaca§? dizine k?t?kleri ‡o§alt?rken l?tfen bekleyiniz.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Bu i?lem birka‡ dakŒka s?rebilir.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 L?tfen bekleyiniz...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY trTRBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Kur ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kur, ?imdi ”ny?kleyiciyi kuracak.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "™ny?kleyiciyi diskin MBR'sine ve VBR'sine kur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "™ny?kleyiciyi diskin yaln?zca VBR'sine kur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "™ny?kleyiciyi bir diskete kur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "™ny?kleyiciyi kurma.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kurulum yap?lacak bilgisayar?n d?§me tak?m? t?r?n? se‡iniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Se‡mek istedi§iniz d?§me tak?m? t?r?n? yukar? ve a?a§? d?§meleriyle",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   se‡iniz, ard?ndan giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  D?§me tak?m? t?r?nde hi‡bir de§i?iklik yap?lmadan bir ”nceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   geri d”nmek i‡in ‡?k?? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   €?k?? = ?ptal   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kurulum yap?lacak bilgisayar?n d?§me tak?m? d?zenini se‡iniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Se‡mek istedi§iniz d?§me tak?m? d?zenini yukar? ve a?a§?",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   d?§meleriyle se‡iniz, ard?ndan giri? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  D?§me tak?m? d?zeninde hi‡bir de§i?iklik yap?lmadan bir ”nceki",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   sayfaya geri d”nmek i‡in ‡?k?? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giri? = S?rd?r   €?k?? = ?ptal   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kur, bilgisayar? ReactOS k?t?klerinin ‡o§alt?lmas?na an?kl?yor.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "€o§altma dizelgesi olu?turuluyor...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "A?a§?daki k?t?k dizgelerinden birini se‡iniz.",
        0
    },
    {
        8,
        19,
        "\x07  Bir ”§eyi se‡mek i‡in yukar? veyƒ a?a§? d?§melerine bas?n?z.",
        0
    },
    {
        8,
        21,
        "\x07  B”l?m? bi‡imlendirmek i‡in giri? d?§mesine bas?n?z.",
        0
    },
    {
        8,
        23,
        "\x07  Ba?ka bir b”l?m se‡mek i‡in ‡?k?? d?§mesine bas?n?z.",
        0
    },
    {
        0,
        0,
        "Giri? = S?rd?r   €?k?? = ?ptal   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "B”l?m? silmeye karar verdiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  B”l?m? silmek i‡in D d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "UYARI: Bu b”l?mdeki t?m bilgiler silinecektir!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Silme i?leminden vazge‡mek i‡in ‡?k?? d?§mesine bas?n?z.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = B”l?m? Sil   €?k?? = ?ptal   F3 = €?k??",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Kur, dizge yap?land?rmas?n? ?imdikile?tiriyor.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "De§er y?§?nlar? olu?turuluyor...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        //ERROR_NOT_INSTALLED
        "ReactOS, bilgisayara b?t?n?yle kurulmad?.\n"
        "E§er Kur'dan ‡?karsan?z ReactOS'u kurmak\n"
        "i‡in Kur'u yeniden ‡al??t?rmal?s?n?z.\n"
        "\n"
        "  \x07  Kurulumu s?rd?rmek i‡in giri? d?§mesine bas?n?z.\n"
        "  \x07  Kur'dan ‡?kmak i‡in F3 d?§mesine bas?n?z.",
        "F3 = €?k??  Giri? = S?rd?r"
    },
    {
        //ERROR_NO_HDD
        "Kur, bir disk alg?layamad?.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Kur, kaynak s?r?c?y? alg?layamad?.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Kur, TXTSETUP.SIF k?t?§?n? y?kleyemedi.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Kur, bozuk bir TXTSETUP.SIF k?t?§? buldu.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Kur, TXTSETUP.SIF k?t?§?nde ge‡ersiz bir imzƒ buldu.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Kur, dizge s?r?c?s? bilgisini y?kleyemedi.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_WRITE_BOOT,
        "Kur, dizge s?r?c?s?ne FAT ”ny?kleme d?zg?s?n? kuramad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Kur, bilgisayar t?r? dizelgesini y?kleyemedi.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Kur, g”r?nt? ayarlar? dizelgesini y?kleyemedi.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Kur, d?§me tak?m? t?r? dizelgesini y?kleyemedi.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Kur, d?§me tak?m? d?zeni dizelgesini y?kleyemedi.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_WARN_PARTITION,
        "Kur, en az bir diskte, uyumsuz bir b”l?m buldu.\n"
        "\n"         
        "Bir b”l?m silmek veyƒ bir b”l?m olu?turmak b”yle bir b”l?m? yok edebilir.\n"
        "\n"
        "  \x07  Kur'dan ‡?kmak i‡in F3 d?§mesine bas?n?z.\n"
        "  \x07  S?rd?rmek i‡in giri? d?§mesine bas?n?z.",
        "F3 = €?k??   Giri? = S?rd?r"
    },
    {
        //ERROR_NEW_PARTITION,
        "Var olan bir b”l?m?n i‡ine yeni\n"
        "bir b”l?m olu?turulamaz!\n"
        "\n"
        "  * S?rd?rmek i‡in bir d?§meye bas?n?z.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Kullan?lmayan disk bo?lu§u silinemez!\n"
        "\n"
        "  * S?rd?rmek i‡in bir d?§meye bas?n?z.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Kur, dizge s?r?c?s?ne FAT ”ny?kleme d?zg?s?n? kuramad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_NO_FLOPPY,
        "A: s?r?c?s?nde disk yok.",
        "Giri? = S?rd?r"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Kur, d?§me tak?m? d?zeni ayarlar?n? ?imdikile?tiremedi.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Kur, g”r?nt? ayarlar?n? ?imdikile?tiremedi.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Kur, bir y?§?n dosyas?ndan bir ?ey alamad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_FIND_REGISTRY
        "Kur, de§er k?t?klerini bulamad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_CREATE_HIVE,
        "Kur, de§er y?§?nlar?n? olu?turamad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Kur, De§er Defteri'ni ba?latamad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Dolab?n ge‡erli bir yap?land?rma k?t?§? yok.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_CABINET_MISSING,
        "Dolap bulunamad?.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Dolab?n kurulum beti§i yok.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_COPY_QUEUE,
        "Kur, k?t?k ‡o§altma s?ras?n? a‡amad?.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_CREATE_DIR,
        "Kur, kurulum dizinlerini olu?turamad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Kur, TXTSETUP.SIF k?t?§?nde \"Directories\"\n"
        "b”l?m?n? bulamad?.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_CABINET_SECTION,
        "Kur, dolapta \"Directories\"\n"
        "b”l?m?n? bulamad?.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Kur, kurulum dizinini olu?turamad?.",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Kur, TXTSETUP.SIF k?t?§?nde \"SetupData\"\n"
        "b”l?m?n? bulamad?.\n",
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Kur, b”l?m bilgilerini yazamad?.\n"
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Kur, De§er Defteri'ne d?zg? sayfas? bilgisini ekleyemedi.\n"
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Kur, dizge mahallŒ ayƒr?n? yapamad?.\n"
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Kur, De§er Defteri'ne d?§me tak?m? d?zenlerini ekleyemedi.\n"
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Kur, co§rƒfŒ konumu ayarlayamad?.\n"
        "Giri? = Bilgisayar? Yeniden Ba?lat"
    },
    {
        //ERROR_INSUFFICIENT_DISKSPACE,
        "Se‡ili b”l?mde yeteri kadar bo? alan yok.\n"
        "  * S?rd?rmek i‡in bir d?§meye bas?n?z.",
        NULL
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE trTRPages[] =
{
    {
        LANGUAGE_PAGE,
        trTRLanguagePageEntries
    },
    {
        START_PAGE,
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
        SELECT_FILE_SYSTEM_PAGE,
        trTRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        trTRFormatPartitionEntries
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
        BOOT_LOADER_PAGE,
        trTRBootLoaderEntries
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
        BOOT_LOADER_FLOPPY_PAGE,
        trTRBootPageEntries
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
    "   L?tfen bekleyiniz..."},
    {STRING_INSTALLCREATEPARTITION,
    "   Giri? = Kur   C = B”l?m Olu?tur   F3 = €?k??"},
    {STRING_INSTALLDELETEPARTITION,
    "   Giri? = Kur   D = B”l?m? Sil   F3 = €?k??"},
    {STRING_PARTITIONSIZE,
    "B?y?kl?§? giriniz:"},
    {STRING_CHOOSENEWPARTITION,
    "Yeni bir b”l?m olu?turmay? se‡tiniz."},
    {STRING_HDDSIZE,
    "Olu?turulacak b”l?m?n b?y?kl?§?n? mega‡oklu olarak giriniz."},
    {STRING_CREATEPARTITION,
    "   Giri? = B”l?m Olu?tur   €?k?? = ?ptal   F3 = €?k??"},
    {STRING_PARTFORMAT,
    "Bu b”l?m ileride bi‡imlendirilecektir."},
    {STRING_NONFORMATTEDPART,
    "ReactOS'u yeni ve bi‡imlendirilmemi? bir b”l?me kurmay? se‡tiniz."},
    {STRING_INSTALLONPART,
    "Kur, ReactOS'u bir b”l?me kurar."},
    {STRING_CHECKINGPART,
    "Kur, ?imdi se‡ili b”l?m? g”zden ge‡iriyor."},
    {STRING_QUITCONTINUE,
    "F3 = €?k??   Giri? = S?rd?r"},
    {STRING_REBOOTCOMPUTER,
    "Giri? = Bilgisayar? Yeniden Ba?lat"},
    {STRING_TXTSETUPFAILED,
    "Kur, TXTSETUP.SIF k?t?§?ndeki \"%S\" b”l?m?n?\nbulamad?.\n"},
    {STRING_COPYING,
    "   K?t?k ‡o§alt?l?yor: %S..."},
    {STRING_SETUPCOPYINGFILES,
    "Kur, k?t?kleri ‡o§alt?yor..."},
    {STRING_REGHIVEUPDATE,
    "   De§er y?§?nlar? ?imdikile?tiriliyor..."},
    {STRING_IMPORTFILE,
    "   K?t?kten al?n?yor: %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   G”r?nt? ayarlar?n?n de§erleri ?imdikile?tiriliyor..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   MahallŒ ayarlar ?imdikile?tiriliyor..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   D?§me tak?m? d?zeni ayarlar?n?n de§erleri ?imdikile?tiriliyor..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   De§er Defteri'ne d?zg? sayfas? bilgisi ekleniyor..."},
    {STRING_DONE,
    "   Bitti!"},
    {STRING_REBOOTCOMPUTER2,
    "   Giri? = Bilgisayar? Yeniden Ba?lat"},
    {STRING_CONSOLEFAIL1,
    "Konsol a‡?lamad?.\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Bunun en bilinen nedeni, bir USB d?§me tak?m? kullan?lmas?d?r.\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB d?§me tak?m?, daha t?m?yle desteklenmemektedir.\r\n"},
    {STRING_FORMATTINGDISK,
    "Kur, diski bi‡imlendiriyor."},
    {STRING_CHECKINGDISK,
    "Kur, diski g”zden ge‡iriyor."},
    {STRING_FORMATDISK1,
    " B”l?m? %S k?t?k dizgesiyle h?zl? bi‡imlendir. "},
    {STRING_FORMATDISK2,
    " B”l?m? %S k?t?k dizgesiyle bi‡imlendir. "},
    {STRING_KEEPFORMAT,
    " B”l?m? bi‡imlendirme. Hi‡bir de§i?iklik olmayacak. "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Disk %lu  (Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu), %wZ ?zerinde."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Disk %lu  (Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  T?r  %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "?zerinde: %I64u %s  Disk %lu  (Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu), %wZ ?zerinde."},
    {STRING_HDDINFOUNK3,
    "?zerinde: %I64u %s  Disk %lu  (Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Disk %lu (%I64u %s), Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu, %wZ ?zerinde."},
    {STRING_HDDINFOUNK4,
    "%c%c  T?r  %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "?zerinde: Disk %lu (%I64u %s), Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu, %wZ ?zerinde."},
    {STRING_HDDINFOUNK5,
    "%c%c  T?r  %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Disk %lu  (Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu), %S ?zerinde"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Disk %lu  (Giri?=%hu, Veri Yolu=%hu, Kimlik=%hu)"},
    {STRING_NEWPARTITION,
    "Kur, ?u b”l?m? olu?turdu:"},
    {STRING_UNPSPACE,
    "    Kullan?lmayan Bo?luk             %6lu %s"},
    {STRING_MAXSIZE,
    "MB (En ‡ok %lu MB)"},
    {STRING_UNFORMATTED,
    "Yeni (Bi‡imlendirilmemi?)"},
    {STRING_FORMATUNUSED,
    "Kullan?lmayan"},
    {STRING_FORMATUNKNOWN,
    "Bilinmeyen"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "D?§me tak?m? d?zenleri ekleniyor..."},
    {0, 0}
};
