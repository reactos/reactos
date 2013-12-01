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
        "Dil Seáimi",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Kurulum sçrasçnda kullançlacak dili seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ardçndan giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Bu dil, kurulacak dizgenin întançmlç dili olacaktçr.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçkçü",
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
        "ReactOS Kur'a hoügeldiniz.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Kurulumun bu aüamasç, ReactOS kÅtÅklerini bilgisayara áoaßltçr",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "ve kurulumun ikinci aüamasçnç ançklar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ReactOS'u kurmak iáin giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ReactOS'u onarmak veyÉ yÅkseltmek iáin R dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ReactOS Ruhsat Koüullarç'nç gîrÅntÅlemek iáin L dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ReactOS'u kurmadan áçkmak iáin F3 dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Daha áok bilgi iáin buraya gidiniz:",
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
        "Giriü = SÅrdÅr  R = Onar veyÉ YÅkselt  L = Ruhsat Koüullarç F3 = Äçkçü",
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
        "ReactOS Kur, în geliütirme evresinde oldußundan daha",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "her tÅrlÅ iülevi desteklemez.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Desteklenmeyen iülevler üunlardçr:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Kur, bir diskte birden fazla birincil bîlÅmÅ yînetemez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Kur, bir diskte, geniületilmiü bir bîlÅm oldußu sÅrece",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  o diskteki birincil bîlÅmÅ silemez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Kur, bir diskte, dißer geniületilmiü bîlÅmlerin oldußu sÅrece",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  o diskteki ilk geniületilmiü bîlÅmÅ silemez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Kur, yalnçzca FAT kÅtÅk dizgelerini destekler.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- KÅtÅk dizgesi denetimi daha bitirilmemiütir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  ReactOS'u kurmak iáin giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  ReactOS'u kurmadan áçkmak iáin F3 dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçkçü",
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
        "Ruhsat Koüullarç:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS, GNU GPL'nin yançsçra X11, BSD ve",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU LPGL gibi dißer uygun ruhsatlardan alçnan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "dÅzgÅleri iáeren bîlÅmlerin koüullarçyla ruhsatlanmçütçr.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Bu yÅzden ReactOS'un tÅm paráalarç, GNU GPL ile birlikte",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "dißer îzgÅn ruhsatlarla yayçnlançr.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Bu yazçlçm, mahallå ve uluslararasç yasalarçn uygunlußu aáçsçndan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "hiábir gÅvenceyle ve hiábir kçsçtlamayla gelmez.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "ReactOS'un ruhsatlanmasç, yalnçzca ÅáÅncÅ kiüilere daßçtmayç kapsar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "Eßer birtakçm nedenlerle ReactOS ile birlikte GNU Umñmå",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Kamu Ruhsatç'nç elde edememiüseniz buraya gidiniz:",
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
        "GÅvence:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        23,
        "Bu bir Åcretsiz yazçlçmdçr, áoßaltma koüullarç iáin kaynaßa bakçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "Burada hiábir gÅvence YOKTUR, hele hele SATILABòLòRLòK veyÉ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "BELòRLò BòR AMACA UYGUNLUK aáçsçndan.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = Geri Dîn",
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
        "Aüaßçdaki dizelge, üimdiki aygçt ayarlarçnç gîsterir.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Bilgisayar TÅrÅ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "GîrÅntÅ Ayarlarç:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "DÅßme Takçmç TÅrÅ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "DÅßme Takçmç DÅzeni:",
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
		"Bu aygçt ayarlarçnç onayla.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Aygçt ayarlarçnç, bir seáeneßi seámek iáin yukarç veyÉ aüaßç dÅßmelerine",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "basarak deßiütirebilirsiniz. Seátikten sonra giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        22,
        "Deßiütirme iüleminden sonra \"Bu aygçt ayarlarçnç onayla.\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "seáeneßini seáiniz, ardçndan giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçkçü",
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
        "ReactOS Kur, în geliütirme evresinde oldußundan daha",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "her tÅrlÅ iülevi desteklemez.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Onarma iülevi daha bitirilmemiütir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  òületim dizgesini yÅkseltmek iáin U dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Kurtarma Konsolu iáin R dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Bir înceki sayfaya geri dînmek iáin áçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Bilgisayarç yeniden baülatmak iáin giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Äçkçü = Geri Dîn  U = YÅkselt  R = Kurtar  Giriü = Bilgisayarç Yeniden Baülat",
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
        "Kurulum yapçlacak bilgisayarçn tÅrÅnÅ seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Seámek istedißiniz bilgisyar tÅrÅnÅ yukarç ve aüaßç dÅßmeleriyle",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   seáiniz, ardçndan giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Bilgisayar tÅrÅnde hiábir deßiüiklik yapçlmadan bir înceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   geri dînmek iáin áçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçkçü",
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
        "Dizge, üimdi diskteki tÅm verileri onaylçyor.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Bu iülem bir dakåka sÅrebilir.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "?òülem bittißinde bilgisayar yeniden baülayacaktçr.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ônbellek temizleniyor...",
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
        "ReactOS, bÅtÅnÅyle kurulmadç.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "A: sÅrÅcÅsÅndeki disketi ve",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "tÅm CD sÅrÅcÅlerindeki CD-ROM'larç áçkartçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Bilgisayarç yeniden baülatmak iáin giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "LÅtfen bekleyiniz...",
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
        "Kurulum yapçlacak bilgisayarçn gîrÅntÅ ayarlarçnç seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  Seámek istedißiniz gîrÅntÅ ayarlarçnç yukarç ve aüaßç dÅßmeleriyle",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   seáiniz, ardçndan giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  GîrÅntÅ ayarlarçnda hiábir deßiüiklik yapçlmadan bir înceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   geri dînmek iáin áçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçkçü",
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
        "ReactOS'un ana bileüenleri baüarçlç bir üekilde kuruldu.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "A: sÅrÅcÅsÅndeki disketi ve",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "tÅm CD sÅrÅcÅlerindeki CD-ROM'larç áçkartçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Bilgisayarç yeniden baülatmak iáin giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = Bilgisayarç Yeniden Baülat",
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
        "Kur, diske înyÅkleyiciyi kuramadç.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "A: sÅrÅcÅsÅne biáimlendirilmiü bir disk takçnçz, ardçndan",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçkçü",
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
        "Aüaßçdaki dizelge, var olan bîlÅmlerle yeni bîlÅmler oluüturmak iáin",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "kullançlmayan boülußu gîsterir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Bir îßeyi seámek iáin yukarç veyÉ aüaßç dÅßmelerine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ReactOS'u seáili bîlÅme yÅklemek iáin giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Seáili boüluktan yeni bir bîlÅm oluüturmak iáin C dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Seáili bîlÅmÅ silmek iáin D dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "LÅtfen bekleyiniz...",
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
        "Kur, üimdi seátißiniz bîlÅmÅ biáimlendirecek.",
        TEXT_STYLE_NORMAL
    },
	{
        6,
        10,
        "SÅrdÅrmek iáin giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçkçü",
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
        "Kur, seáili bîlÅme ReactOS kÅtÅklerini yÅkleyecek. ReactOS'un yÅkleneceßi",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "dizini seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "ônerilen dizini deßiütirmek iáin geri dÅßmesi ile damgalarç siliniz,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "ardçndan ReactOS'un kurulacaßç yeni dizini yazçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçkçü",
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
        "Kur, ReactOS'un kurulacaßç dizine kÅtÅkleri áoßaltçrken lÅtfen bekleyiniz.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Bu iülem birkaá dakåka sÅrebilir.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 LÅtfen bekleyiniz...",
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
        "Kur, üimdi înyÅkleyiciyi kuracak.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "ônyÅkleyiciyi diskin MBR'sine ve VBR'sine kur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "ônyÅkleyiciyi diskin yalnçzca VBR'sine kur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "ônyÅkleyiciyi bir diskete kur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "ônyÅkleyiciyi kurma.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçkçü",
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
        "Kurulum yapçlacak bilgisayarçn dÅßme takçmç tÅrÅnÅ seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Seámek istedißiniz dÅßme takçmç tÅrÅnÅ yukarç ve aüaßç dÅßmeleriyle",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   seáiniz, ardçndan giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  DÅßme takçmç tÅrÅnde hiábir deßiüiklik yapçlmadan bir înceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   geri dînmek iáin áçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçkçü",
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
        "Kurulum yapçlacak bilgisayarçn dÅßme takçmç dÅzenini seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Seámek istedißiniz dÅßme takçmç dÅzenini yukarç ve aüaßç",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   dÅßmeleriyle seáiniz, ardçndan giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  DÅßme takçmç dÅzeninde hiábir deßiüiklik yapçlmadan bir înceki",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   sayfaya geri dînmek iáin áçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçkçü",
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
        "Kur, bilgisayarç ReactOS kÅtÅklerinin áoßaltçlmasçna ançklçyor.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Äoßaltma dizelgesi oluüturuluyor...",
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
        "Aüaßçdaki kÅtÅk dizgelerinden birini seáiniz.",
        0
    },
    {
        8,
        19,
        "\x07  Bir îßeyi seámek iáin yukarç veyÉ aüaßç dÅßmelerine basçnçz.",
        0
    },
    {
        8,
        21,
        "\x07  BîlÅmÅ biáimlendirmek iáin giriü dÅßmesine basçnçz.",
        0
    },
    {
        8,
        23,
        "\x07  Baüka bir bîlÅm seámek iáin áçkçü dÅßmesine basçnçz.",
        0
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçkçü",
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
        "BîlÅmÅ silmeye karar verdiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  BîlÅmÅ silmek iáin D dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "UYARI: Bu bîlÅmdeki tÅm bilgiler silinecektir!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Silme iüleminden vazgeámek iáin áçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = BîlÅmÅ Sil   Äçkçü = òptal   F3 = Äçkçü",
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
        "Kur, dizge yapçlandçrmasçnç üimdikileütiriyor.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Deßer yçßçnlarç oluüturuluyor...",
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
        "ReactOS, bilgisayara bÅtÅnÅyle kurulmadç.\n"
        "Eßer Kur'dan áçkarsançz ReactOS'u kurmak\n"
        "iáin Kur'u yeniden áalçütçrmalçsçnçz.\n"
        "\n"
        "  \x07  Kurulumu sÅrdÅrmek iáin giriü dÅßmesine basçnçz.\n"
        "  \x07  Kur'dan áçkmak iáin F3 dÅßmesine basçnçz.",
        "F3 = Äçkçü  Giriü = SÅrdÅr"
    },
    {
        //ERROR_NO_HDD
        "Kur, bir disk algçlayamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Kur, kaynak sÅrÅcÅyÅ algçlayamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Kur, TXTSETUP.SIF kÅtÅßÅnÅ yÅkleyemedi.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Kur, bozuk bir TXTSETUP.SIF kÅtÅßÅ buldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Kur, TXTSETUP.SIF kÅtÅßÅnde geáersiz bir imzÉ buldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Kur, dizge sÅrÅcÅsÅ bilgisini yÅkleyemedi.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_WRITE_BOOT,
        "Kur, dizge sÅrÅcÅsÅne FAT înyÅkleme dÅzgÅsÅnÅ kuramadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Kur, bilgisayar tÅrÅ dizelgesini yÅkleyemedi.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Kur, gîrÅntÅ ayarlarç dizelgesini yÅkleyemedi.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Kur, dÅßme takçmç tÅrÅ dizelgesini yÅkleyemedi.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Kur, dÅßme takçmç dÅzeni dizelgesini yÅkleyemedi.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_WARN_PARTITION,
        "Kur, en az bir diskte, uyumsuz bir bîlÅm buldu.\n"
        "\n"         
        "Bir bîlÅm silmek veyÉ bir bîlÅm oluüturmak bîyle bir bîlÅmÅ yok edebilir.\n"
        "\n"
        "  \x07  Kur'dan áçkmak iáin F3 dÅßmesine basçnçz.\n"
        "  \x07  SÅrdÅrmek iáin giriü dÅßmesine basçnçz.",
        "F3 = Äçkçü   Giriü = SÅrdÅr"
    },
    {
        //ERROR_NEW_PARTITION,
        "Var olan bir bîlÅmÅn iáine yeni\n"
        "bir bîlÅm oluüturulamaz!\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Kullançlmayan disk boülußu silinemez!\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Kur, dizge sÅrÅcÅsÅne FAT înyÅkleme dÅzgÅsÅnÅ kuramadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_NO_FLOPPY,
        "A: sÅrÅcÅsÅnde disk yok.",
        "Giriü = SÅrdÅr"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Kur, dÅßme takçmç dÅzeni ayarlarçnç üimdikileütiremedi.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Kur, gîrÅntÅ ayarlarçnç üimdikileütiremedi.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Kur, bir yçßçn dosyasçndan bir üey alamadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_FIND_REGISTRY
        "Kur, deßer kÅtÅklerini bulamadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CREATE_HIVE,
        "Kur, deßer yçßçnlarçnç oluüturamadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Kur, Deßer Defteri'ni baülatamadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Dolabçn geáerli bir yapçlandçrma kÅtÅßÅ yok.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CABINET_MISSING,
        "Dolap bulunamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Dolabçn kurulum betißi yok.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_COPY_QUEUE,
        "Kur, kÅtÅk áoßaltma sçrasçnç aáamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CREATE_DIR,
        "Kur, kurulum dizinlerini oluüturamadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Kur, TXTSETUP.SIF kÅtÅßÅnde \"Directories\"\n"
        "bîlÅmÅnÅ bulamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CABINET_SECTION,
        "Kur, dolapta \"Directories\"\n"
        "bîlÅmÅnÅ bulamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Kur, kurulum dizinini oluüturamadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Kur, TXTSETUP.SIF kÅtÅßÅnde \"SetupData\"\n"
        "bîlÅmÅnÅ bulamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Kur, bîlÅm bilgilerini yazamadç.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Kur, Deßer Defteri'ne dÅzgÅ sayfasç bilgisini ekleyemedi.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Kur, dizge mahallå ayÉrçnç yapamadç.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Kur, Deßer Defteri'ne dÅßme takçmç dÅzenlerini ekleyemedi.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Kur, coßrÉfå konumu ayarlayamadç.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_INSUFFICIENT_DISKSPACE,
        "Seáili bîlÅmde yeteri kadar boü alan yok.\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz.",
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
    "   LÅtfen bekleyiniz..."},
    {STRING_INSTALLCREATEPARTITION,
    "   Giriü = Kur   C = BîlÅm Oluütur   F3 = Äçkçü"},
    {STRING_INSTALLDELETEPARTITION,
    "   Giriü = Kur   D = BîlÅmÅ Sil   F3 = Äçkçü"},
    {STRING_PARTITIONSIZE,
    "BÅyÅklÅßÅ giriniz:"},
    {STRING_CHOOSENEWPARTITION,
    "Yeni bir bîlÅm oluüturmayç seátiniz."},
    {STRING_HDDSIZE,
    "Oluüturulacak bîlÅmÅn bÅyÅklÅßÅnÅ megaáoklu olarak giriniz."},
    {STRING_CREATEPARTITION,
    "   Giriü = BîlÅm Oluütur   Äçkçü = òptal   F3 = Äçkçü"},
    {STRING_PARTFORMAT,
    "Bu bîlÅm ileride biáimlendirilecektir."},
    {STRING_NONFORMATTEDPART,
    "ReactOS'u yeni ve biáimlendirilmemiü bir bîlÅme kurmayç seátiniz."},
    {STRING_INSTALLONPART,
    "Kur, ReactOS'u bir bîlÅme kurar."},
    {STRING_CHECKINGPART,
    "Kur, üimdi seáili bîlÅmÅ gîzden geáiriyor."},
    {STRING_QUITCONTINUE,
    "F3 = Äçkçü   Giriü = SÅrdÅr"},
    {STRING_REBOOTCOMPUTER,
    "Giriü = Bilgisayarç Yeniden Baülat"},
    {STRING_TXTSETUPFAILED,
    "Kur, TXTSETUP.SIF kÅtÅßÅndeki \"%S\" bîlÅmÅnÅ\nbulamadç.\n"},
    {STRING_COPYING,
    "   KÅtÅk áoßaltçlçyor: %S..."},
    {STRING_SETUPCOPYINGFILES,
    "Kur, kÅtÅkleri áoßaltçyor..."},
    {STRING_REGHIVEUPDATE,
    "   Deßer yçßçnlarç üimdikileütiriliyor..."},
    {STRING_IMPORTFILE,
    "   KÅtÅkten alçnçyor: %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   GîrÅntÅ ayarlarçnçn deßerleri üimdikileütiriliyor..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Mahallå ayarlar üimdikileütiriliyor..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   DÅßme takçmç dÅzeni ayarlarçnçn deßerleri üimdikileütiriliyor..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Deßer Defteri'ne dÅzgÅ sayfasç bilgisi ekleniyor..."},
    {STRING_DONE,
    "   Bitti!"},
    {STRING_REBOOTCOMPUTER2,
    "   Giriü = Bilgisayarç Yeniden Baülat"},
    {STRING_CONSOLEFAIL1,
    "Konsol aáçlamadç.\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Bunun en bilinen nedeni, bir USB dÅßme takçmç kullançlmasçdçr.\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB dÅßme takçmç, daha tÅmÅyle desteklenmemektedir.\r\n"},
    {STRING_FORMATTINGDISK,
    "Kur, diski biáimlendiriyor."},
    {STRING_CHECKINGDISK,
    "Kur, diski gîzden geáiriyor."},
    {STRING_FORMATDISK1,
    " BîlÅmÅ %S kÅtÅk dizgesiyle hçzlç biáimlendir. "},
    {STRING_FORMATDISK2,
    " BîlÅmÅ %S kÅtÅk dizgesiyle biáimlendir. "},
    {STRING_KEEPFORMAT,
    " BîlÅmÅ biáimlendirme. Hiábir deßiüiklik olmayacak. "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Disk %lu  (Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu), %wZ Åzerinde."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Disk %lu  (Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  TÅr  %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "özerinde: %I64u %s  Disk %lu  (Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu), %wZ Åzerinde."},
    {STRING_HDDINFOUNK3,
    "özerinde: %I64u %s  Disk %lu  (Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Disk %lu (%I64u %s), Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu, %wZ Åzerinde."},
    {STRING_HDDINFOUNK4,
    "%c%c  TÅr  %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "özerinde: Disk %lu (%I64u %s), Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu, %wZ Åzerinde."},
    {STRING_HDDINFOUNK5,
    "%c%c  TÅr  %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Disk %lu  (Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu), %S Åzerinde"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Disk %lu  (Giriü=%hu, Veri Yolu=%hu, Kimlik=%hu)"},
    {STRING_NEWPARTITION,
    "Kur, üu bîlÅmÅ oluüturdu:"},
    {STRING_UNPSPACE,
    "    Kullançlmayan Boüluk             %6lu %s"},
    {STRING_MAXSIZE,
    "MB (En áok %lu MB)"},
    {STRING_UNFORMATTED,
    "Yeni (Biáimlendirilmemiü)"},
    {STRING_FORMATUNUSED,
    "Kullançlmayan"},
    {STRING_FORMATUNKNOWN,
    "Bilinmeyen"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "DÅßme takçmç dÅzenleri ekleniyor..."},
    {0, 0}
};
