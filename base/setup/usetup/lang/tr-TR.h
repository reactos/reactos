/* TRANSLATOR: 2013-2015 Erdem Ersoy (eersoy93) (erdemersoy@live.com) */

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
        "\x07  LÅtfen kurulum sÅreci iáin kullançlacak dili seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ardçndan Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Bu dil, kurulacak dizgenin în tançmlç dili olacaktçr.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçk",
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
        "ReactOS Kur'a hoü geldiniz.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Kurulumun bu bîlÅmÅ, ReactOS òületim Dizgesi'ni bilgisayarçnçza",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "áoßaltçr ve kurulumun ikinci bîlÅmÅnÅ ançklar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ReactOS'u kurmak iáin Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press R to repair a ReactOS installation using the Recovery Console.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ReactOS Ruhsatlama Istçlahlarç ve ûartlarç'nç gîrÅntÅlemek iáin L'ye basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ReactOS'u kurmadan áçkmak iáin F3'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Daha áok bilgi iáin lÅtfen ußrayçnçz:",
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
        "Giriü = SÅrdÅr  R = Onar veyÉ YÅkselt  L = Ruhsat F3 = Äçk",
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
        "ReactOS Kur, bir în geliüme evresindedir. Daha tÅmÅyle kullançülç",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "bir kurulum uygulamasçnçn tÅm iülevlerini desteklemez.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Aüaßçdaki kçsçtlamalar uygulançr:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Kur, yalnçzca FAT kÅtÅk dizgelerini destekler.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- KÅtÅk dizgesi denetimi daha bitirilmemiütir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  ReactOS'u kurmak iáin Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  ReactOS'u kurmadan áçkmak iáin F3'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçk",
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
        "Ruhsatlama:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS Dizgesi, GNU GPL'yle X11, BSD ve GNU LPGL",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "ruhsatlarç gibi baüka uygun ruhsatlardan kod iáeren",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "kçsçmlarçn üartlarç altçnda ruhsatlanmçütçr.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Bu yÅzden ReactOS dizgesinin kçsmç olan tÅm yazçlçmlar, korunan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "îzgÅn ruhsatçyla birlikte GNU GPL altçnda yayçmlançr.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Bu yazçlçm, yerli ve uluslararasç yasa uygulanabilir kullançm",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Åzerine hiábir gÅvence ve kçsçtlamayla gelmez. ReactOS'un",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        " ruhsatlanmasç yalnçzca ÅáÅncÅ yanlara daßçtmayç kapsar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "Eßer birtakçm nedenlerden dolayç ReactOS ile GNU Umñmå",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Kamu Ruhsatç'nçn bir kopyasçnç almadçysançz lÅtfen ußrayçnçz:",
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
        "Bu îzgÅr yazçlçmdçr, áoßaltma üartlarç iáin kaynaßa bakçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "Burada hiábir gÅvence YOKTUR, SATILABòLòRLòK veyÉ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "BELòRLò BòR AMACA UYGUNLUK iáin bile.",
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
        "Aüaßçdaki dizelge üimdiki aygçt ayarlarçnç gîsterir.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Bilgisayar:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "GîrÅntÅ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "DÅßme Takçmç:",
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
        "Doßrula:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16,
        "Bu aygçt ayarlarçnç doßrula.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Aygçt ayarlarçnç, bir seáenek seámek iáin Yukarç veyÉ Aüaßç dÅßmelerine",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "basarak deßiütirebilirsiniz. Sonra baüka ayarlar seámek iáin Giriü",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "TÅm ayarlar uygun oldußunda ""Bu aygçt ayarlarçnç doßrula.""yç",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "seáiniz ve Giriü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçk",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        "Kurulum yapçlacak bilgisayarçn tÅrÅnÅ seámek isteyebilirsiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  òstenen bilgisyar tÅrÅnÅ seámek iáin Yukarç'ya veyÉ Aüaßç'ya basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ardçndan Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Bilgisayar tÅrÅnÅ deßiütirmeden bir înceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   dînmek iáin Äçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçk",
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
        "Dizge, üimdi diskinize saklanmçü tÅm veriyi doßruluyor.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Bu bir dakåka sÅrebilir.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Bittißinde bilgisayarçnçz kendilißinden yeniden baülayacaktçr.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ôn bellek arçnçyor...",
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
        "ReactOS, tÅmÅyle kurulmadç.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "A: sÅrÅcÅsÅnden disketi ve tÅm CD sÅrÅcÅlerinden",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "CD-ROM'larç áçkartçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Bilgisayarçnçzç yeniden baülatmak iáin Giriü'e basçnçz.",
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
        "Kurulum yapçlacak gîrÅntÅnÅn tÅrÅnÅ seámek isteyebilirsiniz.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  òstenen gîrÅntÅ tÅrÅnÅ seámek iáin Yukarç'ya veyÉ Aüaßç'ya basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ardçndan Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  GîrÅntÅ tÅrÅnÅ deßiütirmeden bir înceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   dînmek iáin Äçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçk",
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
        "ReactOS'un ana bileüenleri baüarçlç olarak kuruldu.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "A: sÅrÅcÅsÅnden disketi ve tÅm CD sÅrÅcÅlerinden",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "CD-ROM'larç áçkartçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Bilgisayarçnçzç yeniden baülatmak iáin Giriü'e basçnçz.",
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
        "Kur, bilgisayarçnçzçn sÉbit diskine în yÅkleyiciyi kuramadç.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "LÅtfen A: sÅrÅcÅsÅne biáimlendirilmiü bir disket takçnçz",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "ve Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçk",
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
        "Aüaßçdaki dizelge, var olan bîlÅmleri ve yeni bîlÅmler iáin",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "kullançlmayan disk boülußunu gîsterir.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Bir dizelge girdisini seámek iáin Yukarç'ya veyÉ Aüaßç'ya basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Seáili bîlÅme ReactOS'u yÅklemek iáin Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Bir ana bîlÅm oluüturmak iáin P'ye basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Bir geniületilmiü bîlÅm oluüturmak iáin E'ye basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Mantçklçk bir bîlÅm oluüturmak iáin L'ye basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Var olan bir bîlÅm silmek iáin D'ye basçnçz.",
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

static MUI_ENTRY trTRConfirmDeleteSystemPartitionEntries[] =
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
        "Kur'a dizge bîlÅmÅnÅ silmeyi sordunuz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Dizge bîlÅmleri; tançlama izlenceleri, donançm yapçlandçrma",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "izlenceleri, ReactOS gibi bir iületim dizgesini baülatmak iáin izlenceler",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "veyÉ donançm Åreticisi eliyle saßlanan baüka izlenceler iáerebilir.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Yalnçzca bîlÅmde bîyle izlencelerin olmadçßçndan emin oldußunuzda ya da",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "onlarç silmek istedißinizden emin oldußunuzda bir dizge bîlÅmÅnÅ siliniz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "BîlÅmÅ sildißinizde ReactOS Kur'u bitirene dek bilgisayarç sÉbit diskten",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "în yÅkleyemeyebilirsiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Dizge bîlÅmÅnÅ silmek iáin Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "Sonra bîlÅmÅ silmeyi onaylamak iáin yeniden sorulacaksçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Bir înceki sayfaya dînmek iáin Äçkçü'a basçnçz. BîlÅm silinmeyecek.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr  Äçkçü = òptal",
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
        "BîlÅm Biáimlendirme",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Kur, üimdi bîlÅmÅ biáimlendirecek. SÅrdÅrmek iáin Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçk",
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
        "Kur, seáili bîlÅme ReactOS kÅtÅklerini yÅkler. ReactOS'un",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "yÅklenmesini istedißiniz bir dizin seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "ônerilen dizini deßiütirmek iáin, damgalarç silmek iáin Silme'ye basçnçz",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "ve ardçndan ReactOS'un yÅklenmesini istedißiniz dizini yazçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçk",
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
        "ReactOS Kur, ReactOS kurulum dizininize kÅtÅkleri áoßaltçrken",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        " lÅtfen bekleyiniz.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Bu, bitirmek iáin birkaá dakåka sÅrebilir.",
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
        "Kur, în yÅkleyiciyi kuruyor.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "ôn yÅkleyiciyi sÉbit diskin Åzerine kur. (MBR ve VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "ôn yÅkleyiciyi sÉbit diskin Åzerine kur. (Yalnçzca VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "ôn yÅkleyiciyi bir diskete kur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "ôn yÅkleyici kurulumunu geá.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   F3 = Äçk",
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
        "Kurulum yapçlacak dÅßme takçmçnçn tÅrÅnÅ seámek isteyebilirsiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  òstenen dÅßme takçmç tÅrÅnÅ seámek iáin Yukarç'ya veyÉ Aüaßç'ya basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ardçndan Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  DÅßme takçmç tÅrÅnÅ deßiütirmeden bir înceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   dînmek iáin Äçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçk",
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
        "LÅtfen în tançmlç olarak kurulacak bir dÅzen seáiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  òstenen dÅßme takçmç dÅzenini seámek iáin Yukarç'ya veyÉ Aüaßç'ya basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Ardçndan Giriü'e basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  GîrÅntÅ tÅrÅnÅ deßiütirmeden bir înceki sayfaya",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   dînmek iáin Äçkçü dÅßmesine basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçk",
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
        "Kur, ReactOS kÅtÅklerini áoßaltmak iáin bilgisayarçnçzç ançklçyor.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "KÅtÅk áoßaltma dizelgesi oluüturuluyor...",
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
        "Aüaßçdaki dizelgeden bir kÅtÅk dizgesi seáiniz.",
        0
    },
    {
        8,
        19,
        "\x07  Bir kÅtÅk dizgesi seámek iáin Yukarç'ya veyÉ Aüaßç'ya basçnçz.",
        0
    },
    {
        8,
        21,
        "\x07  BîlÅmÅ biáimlendirmek iáin Giriü'e basçnçz.",
        0
    },
    {
        8,
        23,
        "\x07  Baüka bir bîlÅm seámek iáin Äçkçü'a basçnçz.",
        0
    },
    {
        0,
        0,
        "Giriü = SÅrdÅr   Äçkçü = òptal   F3 = Äçk",
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
        "BîlÅmÅ silmeyi seátiniz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  BîlÅmÅ silmek iáin D'ye basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "UYARI: Bu bîlÅmdeki tÅm veriler yitirilecektir!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  òptal etmek iáin Äçkçü'a basçnçz.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = BîlÅm Sil   Äçkçü = òptal   F3 = Äçk",
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
        // NOT_AN_ERROR
        "Baüarçlç\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS, bilgisayara tÅmÅyle kurulmadç. Eßer üimdi\n"
        "Kur'dan áçkarsançz ReactOS'u kurmak iáin Kur'u\n"
        "yeniden áalçütçrmaya gereksinim duyacaksçnçz.\n"
        "\n"
        "  \x07  Kur'u sÅrdÅrmek iáin Giriü'e basçnçz.\n"
        "  \x07  Kur'dan áçkmak iáin F3'e basçnçz.",
        "F3 = Äçk  Giriü = SÅrdÅr"
    },
    {
        //ERROR_NO_HDD
        "Kur, bir sÉbit disk bulamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Kur, kaynak sÅrÅcÅyÅ bulamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Kur, TXTSETUP.SIF kÅtÅßÅnÅ yÅklemede baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Kur, bozuk bir TXTSETUP.SIF buldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Kur, TXTSETUP.SIF'ta geáersiz bir im buldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Kur, dizge sÅrÅcÅ bilgisini alamadç.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_WRITE_BOOT,
        "Kur, dizge bîlÅmÅne FAT în yÅkleme kodunu kuramadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Kur, bilgisayar tÅrÅ dizelgesini yÅklemede baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Kur, gîrÅntÅ ayarlarç dizelgesini yÅklemede baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Kur, dÅßme takçmç tÅrÅ dizelgesini yÅklemede baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Kur, dÅßme takçmç dÅzeni dizelgesini yÅklemede baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_WARN_PARTITION,
        "Kur, dÅzgÅn yînetilemeyen bir uyumsuz bîlÅm tablosu iáeren en az\n"
        "bir sÉbit disk buldu!\n"
        "\n"         
        "BîlÅmleri oluüturmak veyÉ silmek bîlÅm tablosunu yok edebilir.\n"
        "\n"
        "  \x07  Kur'dan áçkmak iáin F3'e basçnçz.\n"
        "  \x07  SÅrdÅrmek iáin Giriü'e basçnçz.",
        "F3 = Äçk   Giriü = SÅrdÅr"
    },
    {
        //ERROR_NEW_PARTITION,
        "ôneden var olan bir bîlÅmÅn iáine yeni\n"
        "bir bîlÅm oluüturamazsçnçz!\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "BîlÅmlenmemiü disk boülußunu silemezsiniz!\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Kur, dizge bîlÅmÅ Åzerinde FAT în yÅkleme kodunu kurmada baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_NO_FLOPPY,
        "A: sÅrÅcÅsÅnde disk yok.",
        "Giriü = SÅrdÅr"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Kur, dÅßme takçmç dÅzeni ayarlarçnç üimdikileütirmede baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Kur, gîrÅntÅ deßer ayarlarçnç üimdikileütirmede baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Kur, bir yçßçn kÅtÅßÅ almada baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_FIND_REGISTRY
        "Kur, deßer veri kÅtÅklerini bulmada baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CREATE_HIVE,
        "Kur, deßer yçßçnlarçnç oluüturmada baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Kur, Deßer Defteri'ni baülatmada baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Dolabçn geáerli yapçlandçrma kÅtÅßÅ yok.\n",
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
        "Kur, kÅtÅk áoßaltma kuyrußunu aámada baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CREATE_DIR,
        "Kur, kurulum dizinlerini oluüturmada baüarçsçz oldu.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Kur, TXTSETUP.SIF'de ""Directories"" bîlÅmÅnÅ\n"
        "bulmada baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CABINET_SECTION,
        "Kur, dolapta ""Directories"" bîlÅmÅnÅ\n"
        "bulmada baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Kur, kurulum dizinini oluüturamadç.",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Kur, TXTSETUP.SIF'de ""SetupData"" bîlÅmÅnÅ\n"
        "bulmada baüarçsçz oldu.\n",
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Kur, bîlÅm tablolarç yazmada baüarçsçz oldu.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Kur, Deßer Defteri'ne kod sayfasç eklemede baüarçsçz oldu.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Kur, dizge yerli ayÉrçnç yapamadç.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Kur, Deßer Defteri'ne dÅßme takçmç dÅzenleri eklemede baüarçsçz oldu.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Kur, coßrÉfå kimlißi ayarlayamadç.\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Geáersiz dizin adç.\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Seáili bîlÅm ReactOS'u kurmak iáin yetecek îláÅde bÅyÅk deßil.\n"
        "Kurulum bîlÅmÅ en az %lu MB bÅyÅklÅßÅnde olmalç.\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "BîlÅm tablosu dolu oldußundan dolayç bu diskin bîlÅm tablosunda yeni bir ana bîlÅm\n"
        "ya da yeni bir geniületilmiü bîlÅm oluüturamazsçnçz.\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Bir diskte birden áok geniületilmiü bîlÅm oluüturamazsçnçz.\n"
        "\n"
        "  * SÅrdÅrmek iáin bir dÅßmeye basçnçz."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Kur, bîlÅmÅ biáimlendiremez::\n"
        " %S\n"
        "\n"
        "Giriü = Bilgisayarç Yeniden Baülat"
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
    "   Giriü = Kur  P = Ana BîlÅm Oluütur  E = Geniületilmiü BîlÅm Oluütur  F3 = Äçk"},
    {STRING_INSTALLCREATELOGICAL,
    "   ENTER = Kur   L = Mantçklçk BîlÅm Oluütur   F3 = Äçk"},
    {STRING_INSTALLDELETEPARTITION,
    "   Giriü = Kur   D = BîlÅmÅ Sil   F3 = Äçk"},
    {STRING_DELETEPARTITION,
    "   D = BîlÅmÅ Sil   F3 = Äçk"},
    {STRING_PARTITIONSIZE,
    "Yeni bîlÅmÅn bÅyÅklÅßÅnÅ giriniz:"},
    {STRING_CHOOSENEWPARTITION,
    "özerinde bir ana bîlÅm oluüturmayç seátiniz:"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
    "özerinde bir geniületilmiü bîlÅm oluüturmayç seátiniz:"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
    "özerinde bir mantçklçk bîlÅm oluüturmayç seátiniz:"},
    {STRING_HDDSIZE,
    "LÅtfen yeni bîlÅmÅn bÅyÅklÅßÅnÅ megaáoklu olarak giriniz."},
    {STRING_CREATEPARTITION,
    "   Giriü = BîlÅm Oluütur   Äçkçü = òptal   F3 = Äçk"},
    {STRING_PARTFORMAT,
    "Bu bîlÅm ileride biáimlendirilecektir."},
    {STRING_NONFORMATTEDPART,
    "ReactOS'u yeni ya da biáimlendirilmemiü bir bîlÅme kurmayç seátiniz."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Dizge bîlÅmÅ daha biáimlendirilmemiü."},
    {STRING_NONFORMATTEDOTHERPART,
    "Yeni bîlÅm daha biáimlendirilmemiü."},
    {STRING_INSTALLONPART,
    "Kur, ReactOS'u bîlÅm Åzerine kurar."},
    {STRING_CHECKINGPART,
    "Kur, üimdi seáili bîlÅmÅ gîzden geáiriyor."},
    {STRING_CONTINUE,
    "Giriü = SÅrdÅr"},
    {STRING_QUITCONTINUE,
    "F3 = Äçk   Giriü = SÅrdÅr"},
    {STRING_REBOOTCOMPUTER,
    "Giriü = Bilgisayarç Yeniden Baülat"},
    {STRING_TXTSETUPFAILED,
    "Kur, TXTSETUP.SIF'de ""%S"" bîlÅmÅnÅ\nbulmada baüarçsçz oldu.\n"},
    {STRING_COPYING,
    "   KÅtÅk áoßaltçlçyor: %S..."},
    {STRING_SETUPCOPYINGFILES,
    "Kur, kÅtÅkleri áoßaltçyor..."},
    {STRING_REGHIVEUPDATE,
    "   Deßer yçßçnlarç üimdikileütiriliyor..."},
    {STRING_IMPORTFILE,
    "   Alçnçyor: %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   GîrÅntÅ ayarlarç deßerleri üimdikileütiriliyor..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Yerli ayarlar üimdikileütiriliyor..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   DÅßme takçmç dÅzeni ayarlarç üimdikileütiriliyor..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Deßer Defteri'ne kod sayfasç bilgisi ekleniyor..."},
    {STRING_DONE,
    "   Bitti..."},
    {STRING_REBOOTCOMPUTER2,
    "   Giriü = Bilgisayarç Yeniden Baülat"},
    {STRING_CONSOLEFAIL1,
    "Konsol aáçlamçyor.\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Bunun en bilinen nedeni, bir USB dÅßme takçmç kullançlmasçdçr.\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB dÅßme takçmlarç daha tÅmÅyle desteklenmemektedir.\r\n"},
    {STRING_FORMATTINGDISK,
    "Kur, diskinizi biáimlendiriyor."},
    {STRING_CHECKINGDISK,
    "Kur, diskinizi gîzden geáiriyor."},
    {STRING_FORMATDISK1,
    " BîlÅmÅ %S kÅtÅk dizgesiyle hçzlç biáimlendir. "},
    {STRING_FORMATDISK2,
    " BîlÅmÅ %S kÅtÅk dizgesiyle biáimlendir. "},
    {STRING_KEEPFORMAT,
    " ûimdiki kÅtÅk dizgesini koru. (Deßiüiklik yok.) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  SÉbit Disk %lu  (Giriü=%hu, Veriyolu=%hu, Kimlik=%hu), %wZ Åzerinde [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  SÉbit Disk %lu  (Giriü=%hu, Veriyolu=%hu, Kimlik=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  TÅr  0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "özerinde: %I64u %s  SÉbit Disk %lu  (Giriü=%hu, Veriyolu=%hu, Kimlik=%hu), %wZ Åzerinde [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "özerinde: %I64u %s  SÉbit Disk %lu  (Giriü=%hu, Veriyolu=%hu, Kimlik=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "SÉbit Disk %lu (%I64u %s), Giriü=%hu, Veriyolu=%hu, Kimlik=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  TÅr  0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "özerinde: SÉbit Disk %lu (%I64u %s), Giriü=%hu, Veriyolu=%hu, Kimlik=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTÅr  %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  SÉbit Disk %lu  (Giriü=%hu, Veriyolu=%hu, Kimlik=%hu), %wZ Åzerinde [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  SÉbit Disk %lu  (Giriü=%hu, Veriyolu=%hu, Kimlik=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Kur, Åzerinde bir yeni bîlÅm oluüturdu:"},
    {STRING_UNPSPACE,
    "    %sKullançlmayan Boüluk%s           %6lu %s"},
    {STRING_MAXSIZE,
    "MB (En áok %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Geniületilmiü BîlÅm"},
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
