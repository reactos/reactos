/* TRANSLATOR : Ardit Dani (Ard1t) (ardit.dani@gmail.com) 
 * DATE OF TR:  29-11-2013
*/

#pragma once

MUI_LAYOUTS sqALLayouts[] =
{
    { L"041C", L"0000041C" },
        { NULL, NULL }
};

static MUI_ENTRY sqALLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "P‰rzgjedhja e Gjuh‰s",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Ju lutem p‰rzgjedhni gjuh‰n p‰r p‰rdorim gjat‰ instalimit.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Kliko ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Kjo gjuh‰ do j‰te e parazgjedhur p‰r sistemin final.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo  F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALWelcomePageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Mir‰ se vini n‰ instalimin e ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Kjo pjese e instalimit kopjon Sistemin Opererativ t‰ ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "n‰ kompjuter dhe pergatit pjesen e dyt‰ t‰ instalimit.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Kliko ENTER p‰r instalimin e ReactOS.",
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
        "\x07  Kliko L p‰r t‰ v‰zhguar Termat e Li‰enses dhe kushtet e ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Kliko F3 t‰ dilni pa instaluar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "P‰r m‰ shum‰ informacione mbi ReactOS, ju lutem vizitoni:",
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
        "ENTER = Vazhdo  R = Riparo  L = Li‰ens‰  F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALIntroPageEntries[] =
{
    {
        4,
        3,
        "Instalimi i  ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalimi i ReactOS ‰sht‰ n‰ fazat e para t‰ zhvillimit. Ajo ende nuk i",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "suporton te gjitha funksionet e nj‰ instalimit plot‰sisht t‰ p‰rdorsh‰m.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Kufizimet e meposhtme aplikohen:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Instalimi suporton vet‰m dokumentat FAT t‰ sistemit.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Kontrollet e dokumentave t‰ sistemit nuk jan‰ implementuar ende.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Kliko ENTER p‰r t‰ instaluar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Kliko F3 t‰ dilni pa instaluar ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   F3 = Dil",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Li‰ensa:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "Sistemi ReactOS ‰sht‰ i li‰ensuar nd‰r termat e",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL me pjes‰ q‰ p‰rmbajn‰ kode nga li‰ensa t‰ tjera",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "n‰ p‰rputhje me X11 apo BSD dhe GNU LGPL li‰ens.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "T‰ gjitha programet q‰ jan‰ pjes‰ e sistemit ReactOS jan‰",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "t‰ l‰shuara si dhe t‰ mirembajtura n‰n GNU GPL t‰",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "li‰enses origjinale.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Ky sistem vjen me asnj‰ garanci opo kufizim mbi p‰rdorimin e tij,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "ruani ligjin e aplikueshem vendor dhe nd‰rkombetar. Li‰encimi i",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS vet‰m mbulon shp‰rndarjen e pal‰ve t‰ treta.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "N‰se p‰r ndonj‰ arsye ju nuk keni marr‰ nj‰ kopje t‰",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public Li‰ense me ReactOS ju lutem vizitoni",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        22,
        "Garanci:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Ky sistem ‰sht‰ falas; shih burimet dhe kushtet p‰r kopjim.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "Nuk ka asnj‰ GARNCI; as edhe p‰r TREGTUESHMERINE ose",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "P…RDORIMIT P…R NJ… Q…LLIM T… CAKTUAR",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kthehu",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALDevicePageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Lista e meposhtme tregon parametrat aktuale t‰ pajisjeve.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Komputer:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Ekran:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Tastier‰:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Paraqitja e Tastier‰s:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Prano:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Prano k‰to konfigurime t‰ pajisjeve",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Ju mund t‰ ndryshoni parametrat e pajisjeve me butonat UP ose DOWN",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "p‰r t‰ zgjedhur nj‰ hyrje. Pastaj ENTER t‰ p‰rzgjedhni alternativat",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "konfiguruse.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "N?se cilesimet jan‰ t‰ sakta, zgjidhni\"Prano konfigurimin e pajisjeve\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "dhe klikoni ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALUpgradePageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY sqALComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "D‰shironi t‰ ndryshoni llojin e kompjuterit p‰r t‰ instaluar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Klikoni butonat UP ose DOWN p‰r t‰ p‰rzgjedhur tipin e kompjuterit t‰ deshiruar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Pastaj klikoni ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Klikoni butonin ESC p‰r tu kthyer tek menuja e meparshme pa b‰r‰ ndryshime",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   tipin e kompjuterit.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   ESC = Anulo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALFlushPageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Sistemi ‰sht‰ tani duke u siguruar t‰ gjitha te dh‰nat jan‰ ruajtur n‰ diskun tuaj",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Kjo mund te marr‰ nje minut‰",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Kur te p‰rfundoi, kompjuteri juaj do t‰ riniset automatikisht",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Pastro mbetjet",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALQuitPageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS nuk ‰sht‰ instaluar plot‰sisht",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Hiq floppy disk nga Drive A: dhe",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "t‰ gjith‰ CD-ROMs nga CD-Drives.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Kliko ENTER t‰ rinisni kompjuterin tuaj.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Ju lutem prisni ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALDisplayPageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ju deshironi t‰ ndryshoje llojin e ekranit p‰r t‰ instaluar.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Klikoni butonat UP aso DOWN p‰r t‰ p‰rzgjedhur tipin e ekranin t‰ d‰shiruar.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Pastaj klikoni ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Klikoni butonin ESC  p‰r tu kthyer te menuja e meparshme pa b‰r‰ ndryshime",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   tipin e ekranit.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   ESC = Anulo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALSuccessPageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Komponent‰t themelore t‰ ReactOS jan‰ instaluar me sukses.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Hiqni floppy disk nga Drive A: dhe",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "te gjith CD-ROMs nga CD-Drive.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Klikoni ENTER p‰r t‰ rinisur kompjuterin tuaj.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Rinis kompjuterin",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALBootPageEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalimi nuk mund t‰ instaloj‰ programin e bootloaderit ne kompjuterin tuaj",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "hardisku",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Ju lutem fusni nje floppy disk t‰ formatuar n‰ drive A: dhe",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "klikoni ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Vazhdo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY sqALSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Lista meposht tregon particionet dhe pjes‰n e paperdorur t‰ hard diskut",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "p‰r particione t‰ reja.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Kliko butonin UP ose DOWN p‰r t‰ zgjedhur listen hyr‰se.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Kliko ENTER p‰r t‰ instaluar ReactOS n‰ particionin e p‰rzgjedhur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  Kiko C p‰r t‰ krijuar nj‰ particion t‰ ri.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press E to create an extended partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press L to create a logical partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Kliko D p‰r t‰ fshir‰ nj‰ particion ekzistues.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Ju luttem prisni...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   not be deleted.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formato particionin",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Instalimi tani do t‰ formatoj‰ particionin. Kliko ENTER p‰r t‰ vazhduar.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY sqALInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalimi i ReactOS n‰ particionet e p‰rzgjedhura. Zgjidh nj‰",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "skede ku deshironi t‰ instaloni ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "P‰r t‰ ndryshuar skeden e sygjeruar, klikoni BACKSPACE p‰r t‰ fshir‰",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "karakteret dhe pastaj shkruani skeden q‰ d‰shironi q‰ ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "t‰ instalohet.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALFileCopyEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Ju lutem prisni nderkohe q‰ instaluesi i ReactOS kopjon dokumentat tuaj",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "n‰ skedat p‰rkatese.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Kjo mund t‰ marr‰ disa minuta p‰r t‰ p‰rfunduar.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Ju lutem prisni...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALBootLoaderEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalimi po instalon boot loaderin",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Instalo programin bootloader mbi harddisk (MBR dhe VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Instalo programin bootloader mbi harddisk (vetem VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Instalo bootloaderin ne nje floppy disk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Kalo instalimin e bootloaderit.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ju deshironi t‰ ndryshoni llojin e tasti‰res p‰r t‰ instaluar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Klikoni butonin UP ose DOWN p‰r t‰ p‰rzgjedhur tastieren e deshiruar.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Kliko ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Kliko butonin ESC p‰r tu kthyer tek menuja e m‰parshme pa b‰r‰ ndryshimet e",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   tipit t‰ tastier‰s.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   ESC = Anulo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ju lutem zgjidhni nj‰ p‰rzgjedhje t‰ instalimit.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Klikoni butoin UP ose DOWN p‰r t‰ p‰rzgjedhur paraqitjen e tastier‰s",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    s‰ deshiruar. Pastaj Klikoni ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Klikoni butonin ESC p‰r tu kthyer tek menuja e m‰parshme pa b‰r‰ ndryshimet e",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   perzgjedhura te tastier‰s.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Vazhdo   ESC = Anulo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY sqALPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalimi pergatit kompjuterin tuaj p‰r kopjimin e dokumentave t‰ ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Ndertimi i list‰s s‰ dokumentave p‰r tu kopjuar...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY sqALSelectFSEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Zgjidh nj‰ sistem dokumentesh nga lista e meposhtme.",
        0
    },
    {
        8,
        19,
        "\x07  Klikoni UP ose DOWN p‰r tv p‰rzgjedhur sistemin e dokumentave.",
        0
    },
    {
        8,
        21,
        "\x07  Kliko ENTER p‰r t‰ formatuar particionin.",
        0
    },
    {
        8,
        23,
        "\x07  Kliko ESC p‰r t‰ p‰rzgjedhur nj‰ particion tjet‰r.",
        0
    },
    {
        0,
        0,
        "ENTER = Vazhdo   ESC = Anulo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ju keni zgjedhur p‰r t‰ fshir‰ particionin",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Kliko D p‰r t‰ fshir‰ particionin.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "KUJDES: T‰ gjitha t‰ dh‰nat n‰ k‰t‰ PARTICION do t‰ humbin!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Kliko ESC p‰r ta anuluar.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Fshi Particionin   ESC = Anulo   F3 = Dil",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY sqALRegistryEntries[] =
{
    {
        4,
        3,
        " Instalimi i ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalimi po apdejton sistemin e konfigurimit. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Krijimi i kosheres s‰ rregjistrit...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR sqALErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS nuk ‰shte instaluar plotesisht ne kompjuterin\n"
        "tuaj. N‰se dilni nga instalimi tani, ju do t‰ duhet t‰\n"
        "rifilloni instalimin e ReactOS p‰rs‰ri.\n"
        "\n"
        "  \x07  Kliko ENTER p‰r t‰ vazhduar instalimin.\n"
        "  \x07  Kliko F3 t‰ dal‰sh nga instalimi.",
        "F3 = Dil  ENTER = Vazhdo"
    },
    {
        //ERROR_NO_HDD
        "Instalimi nuk mund t‰ gjej nj‰ harddisk.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Instalimi nuk mund t‰ gjej burimin e t‰ dh‰nave/drive.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Instalimi d‰shtoj p‰r t‰ ngarkuar dokumentin TXTSETUP.SIF.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Instalimi zbuloj nj‰ dokument t‰ korruptuar TXTSETUP.SIF.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Instalimi zbuloj nj‰ firm‰ t‰ pavleshm‰ ne TXTSETUP.SIF.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Instalimi nuk gjeti informacionet n‰ drive'rin e systemit.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_WRITE_BOOT,
        "Instalimi deshtoj n‰ instalimin e FAT bootcode n‰ particionin e sistemit.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Instalimi d‰shtoj n‰ ngarkimin e list‰s s‰ kompjuterit.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Instalimi d‰shtoj n‰ ngarkimin e list‰s s‰ konfigurimit t‰ ekranit.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Instalimi d‰shtoj n‰ ngarkimin e list‰s s‰ tipit t‰ p‰rzgjsdhjes t‰ tastier‰s.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Instalimi d‰shtoj n‰ ngarkimin e list‰s s‰ tipit t‰ p‰rzgjedhjes t‰ tastier‰s.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_WARN_PARTITION,
          "Instalimi zbuloi q‰ t‰ pakten nj‰ harddisk p‰rmban nj‰ mosp‰rputhshmeri\n"
          "n‰ tabel‰n e particionit q‰ nuk mund t‰ trajtohet tamam!\n"
          "\n"
          "Krijimi apo fshirja e particionit mund t‰ shkat‰rroi tabelen e partiocioneve.\n"
          "\n"
          "  \x07  Kliko F3 p‰r daljen nga instalimi.\n"
          "  \x07  Kliko ENTER p‰r t‰ vazhduar.",
          "F3 = Dil  ENTER = Vazhdo"
    },
    {
        //ERROR_NEW_PARTITION,
        "Tani ju mund t‰ krijoni nj‰ particion brenda\n"
        "nj‰ particioni ekzistues!\n"
        "\n"
        "  * Shtypni nj‰ tast ‰far‰do p‰r t‰ vazhduar.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Ju nuk mund t‰ fshini hap‰sir‰ n‰ disk jasht particioneve!\n"
        "\n"
        "  * Shtypni nj‰ tast cfar‰do p‰r t‰ vazhduar.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Instalimi d‰shtoj n‰ instalimin e FAT bootcode n‰ particionin e sistemit.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_NO_FLOPPY,
        "Ska disk n‰ drive A:.",
        "ENTER = Vazhdo"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Instalimi d‰shtoj n‰ ngarkimin e list‰s s‰ tipit tv p‰rzgjsdhjes t‰ tastier‰s.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Instalimi d‰shtoi p‰r t‰ rinovuar konfigurimet e regjistrit.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Instalimi d‰shtoi n‰ importimin e skedes koshere.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_FIND_REGISTRY
        "Instalimi d‰shtoi p‰r t‰ gjetur dokumentat e regjistrit t‰ t‰ dh‰nave.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_CREATE_HIVE,
        "Instalimi d‰shtoi p‰r t krijuar rgjistrin e koshere.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Instalimi d‰shtoi n‰ nisjen e regjistrit.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Kabineti nuk ka t‰ vlefshme dokumentin inf.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_CABINET_MISSING,
        "Kabineti nuk u gj‰nd.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Kabineti ska asnj‰ skript konfigurimi.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_COPY_QUEUE,
        "Installimi d‰shtoi rradh‰n e kopjimit t‰ dokumentave.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_CREATE_DIR,
        "Instalimi nuk mund t‰ krijoj‰ skedat p‰r instalim.",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Instalimi d‰shtoi p‰r t‰ gjetur seksionin e 'sked‰s'\n"
        "ne TXTSETUP.SIF.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_CABINET_SECTION,
        "Instalimi d‰shtoi p‰r t‰ gjetur seksionin e 'sked‰s'\n"
        "ne kabinet.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Instalimi nuk mund t‰ krijoj‰ skedat p‰r instalim.",
        "‰NT‰R = Ristarto kompjuterin"
    },
    {
        //‰RROR_FIND_S‰TUPDATA,
        "Instalimi d‰shtoi p‰r t‰ gjetur seksionin e 'SetupData'\n"
        "ne TXTSETUP.SIF.\n",
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Instalimi d‰shtoi p‰r t‰ shkruar tabelen e particionit.\n"
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Instalimi d‰shtoi p‰r t‰ shtuar codepage n‰ regjister.\n"
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Instalimi nuk mund t‰ v‰ndosi v‰ndnoshjen n‰ sistem.\n"
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Instalimi d‰shtoi p‰r t‰ shtuar zgj‰dhjen e tastier‰s n‰ regjister.\n"
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Instalimi nuk mund t‰ vendosni id geo.\n"
        "ENTER = Ristarto kompjuterin"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Shtypni nj‰ tast cfar‰do p‰r t‰ vazhduar.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "You can not create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "You can not create more than one extended partition per disk.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Setup is unable to format the partition:\n"
        " %S\n"
        "\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE sqALPages[] =
{
    {
        LANGUAGE_PAGE,
        sqALLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        sqALWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        sqALIntroPageEntries
    },
    {
        LICENSE_PAGE,
        sqALLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        sqALDevicePageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        sqALUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        sqALComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        sqALDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        sqALFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        sqALSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        sqALConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        sqALSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        sqALFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        sqALDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        sqALInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        sqALPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        sqALFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        sqALKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        sqALBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        sqALLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        sqALQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        sqALSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        sqALBootPageEntries
    },
    {
        REGISTRY_PAGE,
        sqALRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING sqALStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Ju lutem prisni..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Instalo   C = Krijo Particion   F3 = Dil"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalo   D = Fshi Particion   F3 = Dil"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Madh‰sia e particionit t‰ ri:"},
    {STRING_CHOOSENEWPARTITION,
//     "You have chosen to create a primary partition on"},
     "Ju keni zgjedhur p‰r t‰ krijuar nj‰ ndarje t‰ re n‰"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "Ju lutem, jepini madh‰sin‰ e particionit t‰ ri n‰ megabajt."},
    {STRING_CREATEPARTITION,
     "   ENTER = Krijo Particion   ESC = Anulo   F3 = Dil"},
    {STRING_PARTFORMAT,
    "Ky particion do t‰ formatohet tani."},
    {STRING_NONFORMATTEDPART,
    "Ju zgjodh‰t ReactOS p‰r tu instaluar n‰ nj‰ particion t'ri t‰ paformatuar."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Instalimi i ReactOS ne Particion"},
    {STRING_CHECKINGPART,
    "Instalimi tani ‰sht‰ duke kontrolluar particionin e p‰rzgjedhur."},
    {STRING_CONTINUE,
    "ENTER = Vazhdo"},
    {STRING_QUITCONTINUE,
    "F3 = Dil  ENTER = Vazhdo"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Ristarto kompjuterin"},
    {STRING_TXTSETUPFAILED,
    "Instalimi d‰shtoj p‰r t‰ gjetur '%S' sectorin\nne TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Kopjo dokumentat: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Instalimi po kopjon dokumentat..."},
    {STRING_REGHIVEUPDATE,
    "   Apdejtimi i kosheres s‰ regjistrit..."},
    {STRING_IMPORTFILE,
    "   Importimi %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Apdejtimi i regjistrit p‰r ekranin..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Apdejtimi i konfigurimit vendas..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Apdejtimi i p‰rzgjedhj‰s se konfigurimit t‰ tastier‰s..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Shtimi i informacioneve codepage n‰ regjister..."},
    {STRING_DONE,
    "   Mbaruam..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Ristarto kompjuterin"},
    {STRING_CONSOLEFAIL1,
    "N‰ pamundesi p‰r t‰ hapur konsollin\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Shkaku m‰ i zakonsh‰m i k‰saj ‰sht‰ arsyea e perdorimit t‰ nj‰ tastiere USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Tastierat USB nuk jan‰ t‰ mb‰shtetura ende plot‰sisht\r\n"},
    {STRING_FORMATTINGDISK,
    "Instalimi po formaton diskun tuaj"},
    {STRING_CHECKINGDISK,
    "Instalimi ‰sht‰ duke kontrolluar diskun tuaj"},
    {STRING_FORMATDISK1,
    " Formato particionin si %S dokumentat e sistemit (formatim i shpejt‰) "},
    {STRING_FORMATDISK2,
    " Formato particionin si %S dokumentat e sistemit"},
    {STRING_KEEPFORMAT,
    " Mbaj dokumentat e sistemit siq jan‰ (pa ndryshime) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Harddisku %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Harddisku %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tipi 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  Harddisku %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  Harddisku %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Harddisku %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tipi 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "on Harddisku %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTipi %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Harddisku %lu  (Port=%hu, Bus=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Harddisku %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Instalimi krijoj nj‰ particion t‰ ri n‰"},
    {STRING_UNPSPACE,
    "    %sHap‰sire e papjesesezuar%s            %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "I ri (papjesesezuar)"},
    {STRING_FORMATUNUSED,
    "E paperdorur"},
    {STRING_FORMATUNKNOWN,
    "E paditur"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Shtim e p‰rzgjedhjes s‰ tastier‰s"},
    {0, 0}
};
