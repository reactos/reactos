/* TRANSLATOR:  M rio KaŸm r /Mario Kacmar/ aka Kario (kario@szm.sk)
 * DATE OF TR:  22-01-2008
 * Encoding  :  Latin II (852)
 */

#ifndef LANG_SK_SK_H__
#define LANG_SK_SK_H__

static MUI_ENTRY skSKLanguagePageEntries[] =
{
    {
        4,
        3,
         " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vìber jazyka.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Zvo–te si jazyk, ktorì sa pou§ije poŸas inçtal cie.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Tento jazyk bude predvolenìm jazykom nainçtalovan‚ho syst‚mu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "V¡ta V s Inçtal tor syst‚mu ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Tento stupeå Inçtal tora skop¡ruje operaŸnì syst‚m ReactOS na V ç",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "poŸ¡taŸ a priprav¡ druhì stupeå Inçtal tora.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  StlaŸte ENTER pre nainçtalovanie syst‚mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaŸte R pre opravu syst‚mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  StlaŸte L, ak chcete zobraziœ licenŸn‚ podmienky syst‚mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaŸte F3 pre skonŸenie inçtal cie bez nainçtalovania syst‚mu ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Pre viac inform ci¡ o syst‚me ReactOS, navçt¡vte pros¡m:",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org",
        TEXT_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   R = Opraviœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor syst‚mu ReactOS je v zaŸiatoŸnom çt diu vìvoja. Zatia–",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "nepodporuje vçetky funkcie plne vyu§¡vaj£ce program Inçtal tor.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "M  nasleduj£ce obmedzenia:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Inçtal tor nepracuje s viac ako 1 prim rnou oblasœou na 1 disku.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Inçtal tor nevie odstr niœ prim rnu oblasœ z disku,",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  pokia– existuj£ rozç¡ren‚ oblasti na disku.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Inçtal tor nevie odstr niœ prv£ rozç¡ren£ oblasœ z disku,",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  pokia– existuj£ in‚ rozç¡ren‚ oblasti na disku.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Inçtal tor podporuje iba s£borovì syst‚m FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Kontrola s£borov‚ho syst‚mu zatia– nie je implementovan .",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  StlaŸte ENTER pre nainçtalovanie syst‚mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  StlaŸte F3 pre skonŸenie inçtal cie bez nainçtalovania syst‚mu ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        6,
        "Licencia:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "the original license.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "If for some reason you did not receive a copy of the",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
        TEXT_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_HIGHLIGHT
    },
    {
        8,
        22,
        "Z ruka:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = N vrat",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Zoznam ni§çie, zobrazuje s£Ÿasn‚ nastavenia zariaden¡.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "        PoŸ¡taŸ:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Monitor:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "     Kl vesnica:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " Rozlo§enie kl.:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "     Akceptovaœ:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Akceptovaœ tieto nastavenia zariaden¡",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "M“§ete zmeniœ hardv‚rov‚ nastavenia stlaŸen¡m kl vesov HORE alebo DOLE",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "pre vìber polo§ky. Potom stlaŸte kl ves ENTER pre vìber alternat¡vnych",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "nastaven¡.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Ak s£ vçetky nastavenia spr vne, vyberte polo§ku",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "\"Akceptovaœ tieto nastavenia zariaden¡\" a stlaŸte ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor syst‚mu ReactOS je v zaŸiatoŸnom çt diu vìvoja. Zatia–",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "nepodporuje vçetky funkcie plne vyu§¡vaj£ce program Inçtal tor.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Funkcie na opravu syst‚mu zatia– nie s£ implementovan‚.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  StlaŸte U pre aktualiz ciu OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaŸte R pre z chrann£ konzolu.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  StlaŸte ESC pre n vrat na hlavn£ str nku.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaŸte ENTER pre reçtart poŸ¡taŸa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Hlavn  str nka  ENTER = Reçtart",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniœ typ poŸ¡taŸa, ktorì m  byœ nainçtalovanì.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaŸte kl ves HORE alebo DOLE pre vìber po§adovan‚ho typu poŸ¡taŸa.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu poŸ¡taŸa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Syst‚m pr ve overuje vçetky ulo§en‚ £daje na Vaçom disku",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "To m“§e trvaœ nieko–ko min£t",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "KeÔ skonŸ¡, poŸ¡taŸ sa automaticky reçtartuje",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Flushing cache",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Syst‚m ReactOS nie je nainçtalovanì kompletne",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "vçetky m‚di  CD-ROM z CD mechan¡k.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "StlaŸte ENTER pre reçtart poŸ¡taŸa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   PoŸkajte, pros¡m ...",
        TEXT_STATUS,
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniœ typ monitora, ktorì m  byœ nainçtalovanì.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  StlaŸte kl ves HORE alebo DOLE pre vìber po§adovan‚ho typu monitora.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu monitora.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Z kladn‚ s£Ÿast¡ syst‚mu ReactOS boli £speçne nainçtalovan‚.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "vçetky m‚di  CD-ROM z CD mechan¡k.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "StlaŸte ENTER pre reçtart poŸ¡taŸa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reçtart poŸ¡taŸa",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor nem“§e nainçtalovaœ zav dzaŸ syst‚mu na pevnì disk V çho", //bootloader = zav dzaŸ syst‚mu
        TEXT_NORMAL
    },
    {
        6,
        9,
        "poŸ¡taŸa",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Vlo§te pros¡m, naform tovan£ disketu do mechaniky A:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "a stlaŸte ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "The list below shows existing partitions and unused disk",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "space for new partitions.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  StlaŸte HORE alebo DOLE pre vìber zo zoznamu polo§iek.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte ENTER to install ReactOS onto the selected partition.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  StlaŸte C pre vytvorenie novej oblasti.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaŸte D pre vymazanie existuj£cej oblasti.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   PoŸkajte, pros¡m ...",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Form tovanie oblasti",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Inçtal tor teraz naform tuje oblasœ. StlaŸte ENTER pre pokraŸovanie.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY skSKInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Inçtal tor syst‚mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor nainçtaluje s£bory syst‚mu ReactOS na zvolen£ oblasœ.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Vyberte adres r kam chcete nainçtalovaœ syst‚m ReactOS:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "To change the suggested directory, press BACKSPACE to delete",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "characters and then type the directory where you want ReactOS to",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "be installed.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "PoŸkajte, pros¡m, kìm Inçtal tor skop¡ruje s£bory do inçtalaŸn‚ho",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "prieŸinka pre ReactOS.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "DokonŸenie m“§e trvaœ nieko–ko min£t.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                     \xB3 PoŸkajte, pros¡m ...    ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inçtal tor inçtaluje zav dzaŸ operaŸn‚ho syst‚mu",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Nainçtalovaœ zav dzaŸ syst‚mu na pevnì disk (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Nainçtalovaœ zav dzaŸ syst‚mu na disketu.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "PreskoŸiœ inçtal ciu zav dzaŸa syst‚mu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniœ typ kl vesnice, ktorì m  byœ nainçtalovanì.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaŸte kl ves HORE alebo DOLE a vyberte po§adovanì typ kl vesnice.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaŸte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu kl vesnice.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniœ rozlo§enie kl vesnice, ktor‚ m  byœ nainçtalovan‚.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaŸte kl ves HORE alebo DOLE pre vìber po§adovan‚ho rozlo§enia",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   kl vesnice. Potom stlaŸte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaŸte kl ves ESC pre n vrat na predch dzaj£cu str nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   rozlo§enia kl vesnice.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Pripravuje sa kop¡rovanie s£borov syst‚mu ReactOS. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Vytv ra sa zoznam potrebnìch s£borov ...",
        TEXT_STATUS
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
        TEXT_UNDERLINE
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
        "   ENTER = PokraŸovaœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vybrali Ste si odstr nenie oblasti",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  StlaŸte D pre odstr nenie oblasti.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "UPOZORNENIE: Vçetky £daje na tejto oblasti sa nen vratne stratia!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaŸte ESC pre zruçenie.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Odstr niœ oblasœ   ESC = Zruçiœ   F3 = SkonŸiœ",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Aktualizuj£ sa syst‚mov‚ nastavenia.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Vytv raj£ sa polo§ky registrov ...", //registry hives
        TEXT_STATUS
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
        //ERROR_NOT_INSTALLED
        "Syst‚m ReactOS nie je kompletne nainçtalovanì na V çom\n"
        "poŸ¡taŸi. Ak teraz preruç¡te inçtal ciu, budete musieœ\n"
        "spustiœ Inçtal tor znova, aby sa syst‚m ReactOS nainçtaloval.\n"
        "\n"
        "  \x07  StlaŸte ENTER pre pokraŸovanie v inçtal cii.\n"
        "  \x07  StlaŸte F3 pre skonŸenie inçtal cie.",
        "F3 = SkonŸiœ  ENTER = PokraŸovaœ"
    },
    {
        //ERROR_NO_HDD
        "Inçtal toru sa nepodarilo n jsœ pevnì disk.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Inçtal toru sa nepodarilo n jsœ jej zdrojov£ mechaniku.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Inçtal tor zlyhal pri nahr van¡ s£boru TXTSETUP.SIF.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Inçtal tor naçiel poçkodenì s£bor TXTSETUP.SIF.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n", //chybnì (neplatnì) podpis (znak, znaŸka, çifra)
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup could not retrieve system drive information.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu typov poŸ¡taŸov.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu nastaven¡ monitora.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu typov kl vesn¡c.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Inçtal tor zlyhal pri nahr van¡ zoznamu rozlo§enia kl vesn¡c.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup found that at least one harddisk contains an incompatible\n"
          "partition table that can not be handled properly!\n"
          "\n"
          "Vytvorenie alebo odstr nenie oblast¡ m“§e zniŸiœ tabu–ku oblast¡.\n"
          "\n"
          "  \x07  StlaŸte F3 pre skonŸenie inçtal cie."
          "  \x07  StlaŸte ENTER pre pokraŸovanie.",
          "F3 = SkonŸiœ  ENTER = PokraŸovaœ"
    },
    {
        //ERROR_NEW_PARTITION,
        "Nem“§ete vytvoriœ nov£ oblasœ\n"
        "vo vn£tri u§ existuj£cej oblasti!\n"
        "\n"
        "  * PokraŸujte stlaŸen¡m –ubovo–n‚ho kl vesu.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Nem“§ete odstr niœ miesto na disku, ktor‚ nie je oblasœou!\n"
        "\n"
        "  * PokraŸujte stlaŸen¡m –ubovo–n‚ho kl vesu.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_NO_FLOPPY,
        "V mechanike A: nie je disketa.",
        "ENTER = PokraŸovaœ"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup failed to update keyboard layout settings.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup could not create install directories.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in the cabinet.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup could not create the install directory.",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Inçtal tor zlyhal pri h–adan¡ sekcie 'SetupData'\n"
        "v s£bore TXTSETUP.SIF.\n",
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Inçtal tor zlyhal pri z pise do tabuliek oblast¡.\n"
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Reçtart poŸ¡taŸa"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reçtart poŸ¡taŸa"
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
        START_PAGE,
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
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Inçtalovaœ   C = Vytvoriœ oblasœ   F3 = SkonŸiœ"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Inçtalovaœ   D = Odstr niœ oblasœ   F3 = SkonŸiœ"},
    {STRING_PARTITIONSIZE,
     "Ve–kosœ novej oblasti:"},
    {STRING_PLEASEWAIT,
     "   PoŸkajte, pros¡m ..."},
    {STRING_CHOOSENEWPARTITION,
     "Zvolili Ste vytvorenie novej oblasti na"},
    {STRING_CREATEPARTITION,
     "   ENTER = Vytvoriœ oblasœ   ESC = Zruçiœ   F3 = SkonŸiœ"},
    {STRING_COPYING,
     "                                              \xB3 Kop¡ruje sa s£bor: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Inçtal tor kop¡ruje s£bory..."},
    {STRING_PAGEDMEM,
     "Str nkovan  pam„œ"},
    {STRING_NONPAGEDMEM,
     "Nestr nkovan  pam„œ"},
    {STRING_FREEMEM,
     "Vo–n  pam„œ"},
    {0, 0}
};

#endif
