/* TRANSLATOR:  M·rio KaËm·r /Mario Kacmar/ aka Kario (kario@szm.sk)
 * DATE OF TR:  22-01-2008
 */

#ifndef LANG_SK_SK_H__
#define LANG_SK_SK_H__

static MUI_ENTRY skSKLanguagePageEntries[] =
{
    {
        4,
        3,
         " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "V˝ber jazyka.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Zvoæte si jazyk, ktor˝ sa pouûije poËas inötal·cie.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaËte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Tento jazyk bude predvolen˝m jazykom nainötalovanÈho systÈmu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   F3 = SkonËiù",
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
         " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "VÌta V·s Inötal·tor systÈmu ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Tento stupeÚ Inötal·tora skopÌruje operaËn˝ systÈm ReactOS na V·ö",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "poËÌtaË a pripravÌ druh˝ stupeÚ Inötal·tora.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  StlaËte ENTER pre nainötalovanie systÈmu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaËte R pre opravu systÈmu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  StlaËte L, ak chcete zobraziù licenËnÈ podmienky systÈmu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaËte F3 pre skonËenie inötal·cie bez nainötalovania systÈmu ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Pre viac inform·ciÌ o systÈme ReactOS, navötÌvte prosÌm:",
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
        "   ENTER = PokraËovaù   R = Opraviù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inötal·tor systÈmu ReactOS je v zaËiatoËnom öt·diu v˝voja. Zatiaæ",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "nepodporuje vöetky funkcie plne vyuûÌvaj˙ce program Inötal·tor.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "M· nasleduj˙ce obmedzenia:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Inötal·tor nepracuje s viac ako 1 prim·rnou oblasùou na 1 disk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Inötal·tor nevie odstr·niù prim·rnu oblasù z disku,",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  pokiaæ existuj˙ rozöÌrenÈ oblasti na disku.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Inötal·tor nevie odstr·niù prv˙ rozöÌren˙ oblasù z disku,",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  pokiaæ existuj˙ inÈ rozöÌrenÈ oblasti na disku.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Inötal·tor podporuje iba s˙borov˝ systÈm FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Kontrola s˙borovÈho systÈmu zatiaæ nie je implementovan·.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  StlaËte ENTER pre nainötalovanie systÈmu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  StlaËte F3 pre skonËenie inötal·cie bez nainötalovania systÈmu ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
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
        "Z·ruka:",
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
        "   ENTER = N·vrat",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Zoznam niûöie, zobrazuje s˙ËasnÈ nastavenia zariadenÌ.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "        PoËÌtaË:",
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
        "     Kl·vesnica:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " Rozloûenie kl.:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "     Akceptovaù:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Akceptovaù tieto nastavenia zariadenÌ",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "MÙûete zmeniù hardvÈrovÈ nastavenia stlaËenÌm kl·vesov HORE alebo DOLE",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "pre v˝ber poloûky. Potom stlaËte kl·ves ENTER pre v˝ber alternatÌvnych",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "nastavenÌ.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Ak s˙ vöetky nastavenia spr·vne, vyberte \"Akceptovaù tieto nastavenia zariadenÌ\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "a stlaËte ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inötal·tor systÈmu ReactOS je v zaËiatoËnom öt·diu v˝voja. Zatiaæ",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "nepodporuje vöetky funkcie plne vyuûÌvaj˙ce program Inötal·tor.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Funkcie na opravu systÈmu zatiaæ nie s˙ implementovanÈ.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  StlaËte U pre aktualiz·ciu OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaËte R pre z·chrann˙ konzolu.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  StlaËte ESC pre n·vrat na hlavn˙ str·nku.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaËte ENTER pre reötart poËÌtaËa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Hlavn· str·nka  ENTER = Reötart",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniù typ poËÌtaËa, ktor˝ m· byù nainötalovan˝.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaËte kl·ves HORE alebo DOLE pre v˝ber poûadovanÈho typu poËÌtaËa.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaËte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaËte kl·ves ESC pre n·vrat na predch·dzaj˙cu str·nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu poËÌtaËa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   ESC = Zruöiù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "SystÈm pr·ve overuje vöetky uloûenÈ ˙daje na Vaöom disku",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "To mÙûe trvaù niekoæko min˙t",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "KeÔ skonËÌ, poËÌtaË sa automaticky reötartuje",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "SystÈm ReactOS nie je nainötalovan˝ kompletne",
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
        "vöetky mÈdi· CD-ROM z CD mechanÌk.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "StlaËte ENTER pre reötart poËÌtaËa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   PoËkajte, prosÌm ...",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniù typ monitora, ktor˝ m· byù nainötalovan˝.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  StlaËte kl·ves HORE alebo DOLE pre v˝ber poûadovanÈho typu monitora.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaËte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaËte kl·ves ESC pre n·vrat na predch·dzaj˙cu str·nku bez zmeny",
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
        "   ENTER = PokraËovaù   ESC = Zruöiù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Z·kladnÈ s˙ËastÌ systÈmu ReactOS boli ˙speöne nainötalovanÈ.",
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
        "vöetky mÈdi· CD-ROM z CD mechanÌk.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "StlaËte ENTER pre reötart poËÌtaËa.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reötart poËÌtaËa",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inötal·tor nemÙûe nainötalovaù zav·dzaË systÈmu na pevn˝ disk V·öho", //bootloader = zav·dzaË systÈmu
        TEXT_NORMAL
    },
    {
        6,
        9,
        "poËÌtaËa",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Vloûte prosÌm, naform·tovan˙ disketu do mechaniky A:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "a stlaËte ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
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
        "\x07  StlaËte HORE alebo DOLE pre v˝ber zo zoznamu poloûiek.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaËte ENTER to install ReactOS onto the selected partition.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  StlaËte C pre vytvorenie novej oblasti.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  StlaËte D pre vymazanie existuj˙cej oblasti.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   PoËkajte, prosÌm ...",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Form·tovanie oblasti",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Inötal·tor teraz naform·tuje oblasù. StlaËte ENTER pre pokraËovanie.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inötal·tor inötaluje s˙bory systÈmu ReactOS na zvolen˙ oblasù.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Vyberte adres·r kam chcete nainötalovaù systÈm ReactOS:",
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
        "   ENTER = PokraËovaù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "PoËkajte, prosÌm, k˝m Inötal·tor systÈmu ReactOS skopÌruje s˙bory",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "do V·öho inötalaËnÈho prieËinka pre ReactOS.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "DokonËenie mÙûe trvaù niekoæko min˙t.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 PoËkajte, prosÌm ...    ",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Inötal·tor inötaluje zav·dzaË operaËnÈho systÈmu",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Nainötalovaù zav·dzaË systÈmu na pevn˝ disk (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Nainötalovaù zav·dzaË systÈmu na disketu.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "PreskoËiù inötal·ciu zav·dzaËa systÈmu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniù typ kl·vesnice, ktor˝ m· byù nainötalovan˝.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaËte kl·ves HORE alebo DOLE pre v˝ber poûadovanÈho typu kl·vesnice.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stlaËte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaËte kl·ves ESC pre n·vrat na predch·dzaj˙cu str·nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu kl·vesnice.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   ESC = Zruöiù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeniù rozloûenie kl·vesnice, ktorÈ m· byù nainötalovanÈ.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  StlaËte kl·ves HORE alebo DOLE pre v˝ber poûadovanÈho rozloûenia",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    kl·vesnice. Potom stlaËte ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  StlaËte kl·ves ESC pre n·vrat na predch·dzaj˙cu str·nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   rozloûenia kl·vesnice.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   ESC = Zruöiù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Pripravuje sa kopÌrovanie s˙borov systÈmu ReactOS. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Vytv·ra sa zoznam potrebn˝ch s˙borov ...",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        17,
        "Vyberte systÈm s˙borov zo zoznamu uvedenÈho niûöie.",
        0
    },
    {
        8,
        19,
        "\x07  StlaËte HORE alebo DOLE pre v˝ber systÈmu s˙borov.",
        0
    },
    {
        8,
        21,
        "\x07  StlaËte ENTER pre form·tovanie oblasti.",
        0
    },
    {
        8,
        23,
        "\x07  StlaËte ESC pre v˝ber inej oblasti.",
        0
    },
    {
        0,
        0,
        "   ENTER = PokraËovaù   ESC = Zruöiù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vybrali Ste si odstr·nenie oblasti",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  StlaËte D pre odstr·nenie oblasti.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "UPOZORNENIE: Vöetky ˙daje na tejto oblasti sa nen·vratne stratia!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  StlaËte ESC pre zruöenie.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Odstr·niù oblasù   ESC = Zruöiù   F3 = SkonËiù",
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
        " Inötal·tor systÈmu ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Aktualizuj˙ sa systÈmovÈ nastavenia.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Vytv·raj˙ sa poloûky registrov ...", //registry hives
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
        "SystÈm ReactOS nie je kompletne nainötalovan˝ na V·ö\n"
        "poËÌtaË. Ak teraz preruöÌte inötal·ciu, budete musieù\n"
        "spustiù Inötal·tor znova, aby sa systÈm ReactOS nainötaloval.\n"
        "\n"
        "  \x07  StlaËte ENTER pre pokraËovanie v inötal·cii.\n"
        "  \x07  StlaËte F3 pre skonËenie inötal·cie.",
        "F3 = SkonËiù  ENTER = PokraËovaù"
    },
    {
        //ERROR_NO_HDD
        "Inötal·toru sa nepodarilo n·jsù pevn˝ disk.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Inötal·toru sa nepodarilo n·jsù jej zdrojov˙ mechaniku.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Inötal·tor zlyhal pri nahr·vanÌ s˙boru TXTSETUP.SIF.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Inötal·tor naöiel poökoden˝ s˙bor TXTSETUP.SIF.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n", //chybn˝ (neplatn˝) podpis (znak, znaËka, öifra)
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup could not retrieve system drive information.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Inötal·tor zlyhal pri nahr·vanÌ zoznamu typov poËÌtaËov.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Inötal·tor zlyhal pri nahr·vanÌ zoznamu nastavenÌ monitora.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Inötal·tor zlyhal pri nahr·vanÌ zoznamu typov kl·vesnÌc.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Inötal·tor zlyhal pri nahr·vanÌ zoznamu rozloûenia kl·vesnÌc.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup found that at least one harddisk contains an incompatible\n"
          "partition table that can not be handled properly!\n"
          "\n"
          "Vytvorenie alebo odstr·nenie oblastÌ mÙûe zniËiù tabuæku oblastÌ.\n"
          "\n"
          "  \x07  StlaËte F3 pre skonËenie inötal·cie."
          "  \x07  StlaËte ENTER pre pokraËovanie.",
          "F3 = SkonËiù  ENTER = PokraËovaù"
    },
    {
        //ERROR_NEW_PARTITION,
        "NemÙûete vytvoriù nov˙ oblasù\n"
        "vo vn˙tri uû existuj˙cej oblasti!\n"
        "\n"
        "  * PokraËujte stlaËenÌm æubovoænÈho kl·vesu.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "NemÙûete odstr·niù miesto na disku, ktorÈ nie je oblasùou!\n"
        "\n"
        "  * PokraËujte stlaËenÌm æubovoænÈho kl·vesu.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_NO_FLOPPY,
        "V mechanike A: nie je disketa.",
        "ENTER = PokraËovaù"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup failed to update keyboard layout settings.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup could not create install directories.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in the cabinet.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup could not create the install directory.",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Inötal·tor zlyhal pri hæadanÌ sekcie 'SetupData'\n"
        "v s˙bore TXTSETUP.SIF.\n",
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Inötal·tor zlyhal pri z·pise do tabuliek oblastÌ.\n"
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Reötart poËÌtaËa"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reötart poËÌtaËa"
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

#endif
