/* TRANSLATOR:  M rio KaŸm r /Mario Kacmar/ aka Kario (kario@szm.sk)
 * DATE OF TR:  22-01-2008
 * Encoding  :  Latin II (852)
 * LastChange:  22-05-2011
 */

#pragma once

static MUI_ENTRY skSKLanguagePageEntries[] =
{
    {
        4,
        3,
         " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vìber jazyka.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Zvo–te si jazyk, ktorì sa pou§ije poŸas inçtal cie.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tento jazyk bude predvolenìm jazykom nainçtalovan‚ho syst‚mu.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKWelcomePageEntries[] =
{
    {
        4,
        3,
         " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "V¡ta V s Inçtal tor syst‚mu ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Tento stupeå Inçtal tora skop¡ruje operaŸnì syst‚m ReactOS na V ç",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "poŸ¡taŸ a priprav¡ druhì stupeå Inçtal tora.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaŸte R pre opravu ciu syst‚mu ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  StlaŸte L, ak chcete zobraziœ licenŸn‚ podmienky syst‚mu ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaŸte F3 pre skonŸenie inçtal cie, syst‚m ReactOS sa nenainçtaluje.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Pre viac inform ci¡ o syst‚me ReactOS, navçt¡vte pros¡m:",
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
        "ENTER = PokraŸovaœ  R = Opraviœ  L = Licencia  F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
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
        "ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKLicensePageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licencia:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "Syst‚m ReactOS je vydanì za podmienok licencie",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL s Ÿasœami obsahuj£cimi k¢d z inìch kompatibilnìch",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenci¡ ako s£ X11 alebo BSD a licencie GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Preto vçetok softv‚r, ktorì je s£Ÿasœou syst‚mu ReactOS,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "je vydanì pod licenciou GNU GPL, a rovnako s£ zachovan‚",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "aj p“vodn‚ licencie.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Tento softv‚r prich dza BEZ ZµRUKY alebo obmedzen¡ pou§¡vania",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "s vìnimkou platn‚ho miestneho a medzin rodn‚ho pr va. Licencia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "syst‚mu ReactOS pokrìva iba distrib£ciu k tren¡m stran m.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Ak z nejak‚ho d“vodu neobdr§¡te k¢piu licencie GNU GPL",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "spolu so syst‚mom ReactOS, navçt¡vte, pros¡m, str nku:",
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
        "Z ruka:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Toto je slobodnì softv‚r; see the source for copying conditions.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = N vrat",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKDevicePageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Zoznam ni§çie, zobrazuje s£Ÿasn‚ nastavenia zariaden¡.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "PoŸ¡taŸ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Monitor:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Kl vesnica:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Rozlo§enie kl.:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Akceptovaœ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Akceptovaœ tieto nastavenia zariaden¡",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "M“§ete zmeniœ hardv‚rov‚ nastavenia stlaŸen¡m kl vesov HORE alebo DOLE",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "pre vìber polo§ky. Potom stlaŸte kl ves ENTER pre vìber alternat¡vnych",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "nastaven¡.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Ak s£ vçetky nastavenia spr vne, vyberte polo§ku",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "\"Akceptovaœ tieto nastavenia zariaden¡\" a stlaŸte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKRepairPageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor syst‚mu ReactOS je v zaŸiatoŸnom çt diu vìvoja. Zatia–",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "nepodporuje vçetky funkcie plne vyu§¡vaj£ce program Inçtal tor.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Funkcie na opravu syst‚mu zatia– nie s£ implementovan‚.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  StlaŸte U pre aktualiz ciu OS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaŸte R pre z chrann£ konzolu.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  StlaŸte ESC pre n vrat na hlavn£ str nku.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaŸte ENTER pre reçtart poŸ¡taŸa.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = Hlavn  str nka  U = Aktualizovaœ  R = Z chrana  ENTER = Reçtart",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "\x07  Press ESC to continue with a new installation.",
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

static MUI_ENTRY skSKComputerPageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniœ typ poŸ¡taŸa, ktorì m  byœ nainçtalovanì.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaŸte kl ves HORE alebo DOLE pre vìber po§adovan‚ho typu poŸ¡taŸa.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   typu poŸ¡taŸa.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKFlushPageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Syst‚m pr ve overuje vçetky ulo§en‚ £daje na Vaçom disku",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "To m“§e trvaœ nieko–ko min£t",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "KeÔ skonŸ¡, poŸ¡taŸ sa automaticky reçtartuje",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vypr zdåujem cache", //Flushing cache (zapisuje sa na disk obsah cache, doslovne "Preplachovanie cashe")
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKQuitPageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Syst‚m ReactOS nie je nainçtalovanì kompletne",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "vçetky m‚di  CD-ROM z CD mechan¡k.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "StlaŸte ENTER pre reçtart poŸ¡taŸa.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "PoŸkajte, pros¡m ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKDisplayPageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniœ typ monitora, ktorì m  byœ nainçtalovanì.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  StlaŸte kl ves HORE alebo DOLE pre vìber po§adovan‚ho typu monitora.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   typu monitora.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKSuccessPageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Z kladn‚ s£Ÿast¡ syst‚mu ReactOS boli £speçne nainçtalovan‚.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "vçetky m‚di  CD-ROM z CD mechan¡k.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "StlaŸte ENTER pre reçtart poŸ¡taŸa.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Reçtart poŸ¡taŸa",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKBootPageEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor nem“§e nainçtalovaœ zav dzaŸ syst‚mu na pevnì disk V çho", //bootloader = zav dzaŸ syst‚mu
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "poŸ¡taŸa",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Vlo§te pros¡m, naform tovan£ disketu do mechaniky A:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "a stlaŸte ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY skSKSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Zoznam ni§çie, zobrazuje existuj£ce oblasti a nevyu§it‚ miesto",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "na disku vhodn‚ pre nov‚ oblasti.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  StlaŸte HORE alebo DOLE pre vìber zo zoznamu polo§iek.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte ENTER pre inçtal ciu syst‚mu ReactOS na vybran£ oblasœ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  StlaŸte C pre vytvorenie novej oblasti.",
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
        "\x07  StlaŸte D pre vymazanie existuj£cej oblasti.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "PoŸkajte, pros¡m ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY skSKFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Form tovanie oblasti",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Inçtal tor teraz naform tuje oblasœ. StlaŸte ENTER pre pokraŸovanie.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY skSKInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor nainçtaluje s£bory syst‚mu ReactOS na zvolen£ oblasœ.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Vyberte adres r kam chcete nainçtalovaœ syst‚m ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Ak chcete zmeniœ odpor£Ÿanì adres r, stlaŸte BACKSPACE a vyma§te",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "znaky. Potom nap¡çte n zov adres ra, v ktorom chcete aby bol",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "syst‚m ReactOS nainçtalovanì.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKFileCopyEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "PoŸkajte, pros¡m, kìm Inçtal tor skop¡ruje s£bory do inçtalaŸn‚ho",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        30,
        13,
        "prieŸinka pre ReactOS.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        20,
        14,
        "DokonŸenie m“§e trvaœ nieko–ko min£t.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 PoŸkajte, pros¡m ...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKBootLoaderEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor je pripravenì nainçtalovaœ zav dzaŸ operaŸn‚ho syst‚mu",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Nainçtalovaœ zav dzaŸ syst‚mu na pevnì disk (MBR a VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Nainçtalovaœ zav dzaŸ syst‚mu na pevnì disk (iba VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Nainçtalovaœ zav dzaŸ syst‚mu na disketu.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "PreskoŸiœ inçtal ciu zav dzaŸa syst‚mu.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniœ typ kl vesnice, ktorì m  byœ nainçtalovanì.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaŸte kl ves HORE alebo DOLE a vyberte po§adovanì typ kl vesnice.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   typu kl vesnice.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Pros¡m, vyberte rozlo§enie, ktor‚ sa nainçtaluje ako predvolen‚.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaŸte kl ves HORE alebo DOLE pre vìber po§adovan‚ho rozlo§enia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   kl vesnice. Potom stlaŸte ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   rozlo§enia kl vesnice.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY skSKPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Pripravuje sa kop¡rovanie s£borov syst‚mu ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vytv ra sa zoznam potrebnìch s£borov ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY skSKSelectFSEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Vyberte syst‚m s£borov zo zoznamu uveden‚ho ni§çie.",
        0
    },
    {
        8,
        19,
        "\x07  StlaŸte HORE alebo DOLE pre vìber syst‚mu s£borov.",
        0
    },
    {
        8,
        21,
        "\x07  StlaŸte ENTER pre form tovanie oblasti.",
        0
    },
    {
        8,
        23,
        "\x07  StlaŸte ESC pre vìber inej oblasti.",
        0
    },
    {
        0,
        0,
        "ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vybrali ste si odstr nenie oblasti",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  StlaŸte D pre odstr nenie oblasti.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "UPOZORNENIE: Vçetky £daje na tejto oblasti sa nen vratne stratia!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaŸte ESC pre zruçenie.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Odstr niœ oblasœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKRegistryEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Aktualizuj£ sa syst‚mov‚ nastavenia.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vytv raj£ sa polo§ky registrov ...", //registry hives
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR skSKErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "Syst‚m ReactOS nie je kompletne nainçtalovanì na Vaçom\n"
        "poŸ¡taŸi. Ak teraz preruç¡te inçtal ciu, budete musieœ\n"
        "spustiœ Inçtal tor znova, aby sa syst‚m ReactOS nainçtaloval.\n"
        "\n"
        "  \x07  StlaŸte ENTER pre pokraŸovanie v inçtal cii.\n"
        "  \x07  StlaŸte F3 pre skonŸenie inçtal cie.",
        "F3 = SkonŸiœ  ENTER = PokraŸovaœ"
    },
    {
        // ERROR_NO_HDD
        "Inçtal toru sa nepodarilo n jsœ pevnì disk.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Inçtal toru sa nepodarilo n jsœ jej zdrojov£ mechaniku.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Inçtal tor zlyhal pri nahr van¡ s£boru TXTSETUP.SIF.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Inçtal tor naçiel poçkodenì s£bor TXTSETUP.SIF.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n", //chybnì (neplatnì) podpis (znak, znaŸka, çifra)
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Inçtal tor nemohol z¡skaœ inform cie o syst‚movìch diskoch.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_WRITE_BOOT,
        "Inçtal toru sa nepodarilo nainçtalovaœ %S zav dzac¡ k¢d s£borov‚ho\n"
        "syst‚mu FAT na syst‚mov£ part¡ciu.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu typov poŸ¡taŸov.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu nastaven¡ monitora.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu typov kl vesn¡c.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu rozlo§enia kl vesn¡c.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_WARN_PARTITION,
//          "Inçtal tor zistil, §e najmenej jeden pevnì disk obsahuje nekompatibiln£\n"
          "Inçtal tor naçiel najmenej na jednom pevnom disku nekompatibiln£\n"
          "tabu–ku oblast¡, s ktorou sa ned  spr vne zaobch dzaœ!\n"
          "\n"
          "Vytvorenie alebo odstr nenie oblast¡ m“§e zniŸiœ tabu–ku oblast¡.\n"
          "\n"
          "  \x07  StlaŸte F3 pre skonŸenie inçtal cie.\n"
          "  \x07  StlaŸte ENTER pre pokraŸovanie.",
          "F3 = SkonŸiœ  ENTER = PokraŸovaœ"
    },
    {
        // ERROR_NEW_PARTITION,
        "Nem“§ete vytvoriœ nov£ oblasœ\n"
        "vo vn£tri u§ existuj£cej oblasti!\n"
        "\n"
        "  * PokraŸujte stlaŸen¡m –ubovo–n‚ho kl vesu.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "Nem“§ete odstr niœ miesto na disku, ktor‚ nie je oblasœou!\n"
        "\n"
        "  * PokraŸujte stlaŸen¡m –ubovo–n‚ho kl vesu.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Inçtal toru sa nepodarilo nainçtalovaœ %S zav dzac¡ k¢d s£borov‚ho\n"
        "syst‚mu FAT na syst‚mov£ part¡ciu.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_NO_FLOPPY,
        "V mechanike A: nie je disketa.",
        "ENTER = PokraŸovaœ"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Inçtal tor zlyhal pri aktualiz cii nastaven¡ rozlo§enia kl vesnice.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_CREATE_DIR,
        "Inçtal tor nemohol vytvoriœ inçtalaŸn‚ adres re.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Inçtal tor zlyhal pri h–adan¡ sekcie '%S'\n"
        "v s£bore TXTSETUP.SIF.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_CABINET_SECTION,
        "Inçtal tor zlyhal pri h–adan¡ sekcie '%S'\n"
        "v s£bore cabinet.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Inçtal tor nemohol vytvoriœ inçtalaŸnì adres r.", //could not = nemohol
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Inçtal tor zlyhal pri z pise do tabuliek oblast¡.\n"
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Inçtal tor zlyhal pri prid van¡ k¢dovej str nky do registrov.\n"
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Inçtal tor zlyhal pri prid van¡ rozlo§en¡ kl vesnice do registrov.\n"
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Inçtal tor nemohol nastaviœ geo id.\n"
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * PokraŸujte stlaŸen¡m –ubovo–n‚ho kl vesu.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "You can not create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "You can not create more than one extended partition per disk.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_FORMATTING_PARTITION,
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

MUI_PAGE skSKPages[] =
{
    {
        LANGUAGE_PAGE,
        skSKLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        skSKWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        skSKIntroPageEntries
    },
    {
        LICENSE_PAGE,
        skSKLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        skSKDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        skSKRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        skSKUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        skSKComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        skSKDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        skSKFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        skSKSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        skSKConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        skSKSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        skSKFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        skSKDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        skSKInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        skSKPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        skSKFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        skSKKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        skSKBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        skSKLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        skSKQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        skSKSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        skSKBootPageEntries
    },
    {
        REGISTRY_PAGE,
        skSKRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING skSKStrings[] =
{
    {STRING_PLEASEWAIT,
     "   PoŸkajte, pros¡m ..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Inçtalovaœ   C = Vytvoriœ oblasœ   F3 = SkonŸiœ"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Inçtalovaœ   D = Odstr niœ oblasœ   F3 = SkonŸiœ"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Ve–kosœ novej oblasti:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//     "Zvolili ste vytvorenie novej oblasti na"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "Zadajte, pros¡m, ve–kosœ novej oblasti v megabajtoch."},
    {STRING_CREATEPARTITION,
     "   ENTER = Vytvoriœ oblasœ   ESC = Zruçiœ   F3 = SkonŸiœ"},
    {STRING_PARTFORMAT,
    "T to oblasœ sa bude form tovaœ ako Ôalçia."},
    {STRING_NONFORMATTEDPART,
    "Zvolili ste inçtal ciu syst‚mu ReactOS na nov£ alebo nenaform tovan£ oblasœ."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Inçtal tor nainçtaluje syst‚m ReactOS na oblasœ"},
    {STRING_CHECKINGPART,
    "Inçtal tor teraz skontroluje vybran£ oblasœ."},
    {STRING_CONTINUE,
    "ENTER = PokraŸovaœ"},
    {STRING_QUITCONTINUE,
    "F3 = SkonŸiœ  ENTER = PokraŸovaœ"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Reçtart poŸ¡taŸa"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kop¡ruje sa s£bor: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Inçtal tor kop¡ruje s£bory..."},
    {STRING_REGHIVEUPDATE,
    "   Aktualizujem polo§ky registrov..."},
    {STRING_IMPORTFILE,
    "   Importujem %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Aktualizujem nastavenia obrazovky v registrov..."}, //display registry settings
    {STRING_LOCALESETTINGSUPDATE,
    "   Aktualizujem miestne nastavenia..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Aktualizujem nastavenia rozlo§enia kl vesnice..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Prid vam do registrov inform cie o k¢dovej str nke..."},
    {STRING_DONE,
    "   Hotovo..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Reçtart poŸ¡taŸa"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Nemo§no otvoriœ konzolu\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Najbe§nejçou pr¡Ÿinou tohto je pou§itie USB kl vesnice\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB kl vesnica eçte nie je plne podporovan \r\n"},
    {STRING_FORMATTINGDISK,
    "Inçtal tor form tuje V ç disk"},
    {STRING_CHECKINGDISK,
    "Inçtal tor kontroluje V ç disk"},
    {STRING_FORMATDISK1,
    " Naform tovaœ oblasœ ako syst‚m s£borov %S (rìchly form t) "},
    {STRING_FORMATDISK2,
    " Naform tovaœ oblasœ ako syst‚m s£borov %S "},
    {STRING_KEEPFORMAT,
    " Ponechaœ s£Ÿasnì syst‚m s£borov (bez zmeny) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  pevnom disku %lu  (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  pevnì disk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "na %I64u %s  pevnom disku %lu  (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "na %I64u %s  pevnom disku %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "pevnì disk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "na pevnom disku %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %styp %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  pevnì disk %lu  (Port=%hu, Bus=%hu, Id=%hu) na %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  pevnom disku %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Inçtal tor vytvoril nov£ oblasœ na"},
    {STRING_UNPSPACE,
    "    %sMiesto bez oblast¡%s             %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Nov  (Nenaform tovan )"},
    {STRING_FORMATUNUSED,
    "Nepou§it‚"},
    {STRING_FORMATUNKNOWN,
    "Nezn me"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Prid vam rozlo§enia kl vesnice"},
    {0, 0}
};
