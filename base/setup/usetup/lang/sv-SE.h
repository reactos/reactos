/*
 * PROJECT:         ReactOS Setup
 * FILE:            base/setup/usetup/lang/sv-SE.h  
 * PURPOSE:         Swedish resource file
 * Translation:     Jaix Bly plus perhaps GreatLord if blame and translate.reactos.se is consulted.
 */
#pragma once

MUI_LAYOUTS svSELayouts[] =
{
    { L"041D", L"0000041D" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY svSELanguagePageEntries[] =
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
        "Language Selection.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then Tryck ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  This Language will be the default language for the final system.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt  F3 = Avsluta",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEWelcomePageEntries[] =
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
        "VÑlkommen till ReactOS Setup!",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Denna del av installationen kopierar ReactOS till eran",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "dator och fîrbereder den andra delen av installationen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck pÜ ENTER fîr att installera ReactOS.",
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
        "\x07  Tryck pÜ L fîr att lÑsa licensavtalet till ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck pÜ F3 fîr att avbryta installationen av ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Fîr mer information om ReactOS, besîk:",
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
        "   ENTER = FortsÑtt  R = Reparera F3 = Avbryt",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEIntroPageEntries[] =
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
        "ReactOS Setup Ñr i en tidig utvecklingsfas och saknar dÑrfîr ett antal",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan fîrvÑntas av ett fullt anvÑndbart setup-program.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Fîljande begrÑnsningar gÑller:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Setup stîder endast filsystem av typen FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Kontrollering av hÜrddiskens filsystem stîds (Ñnnu) ej.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Tryck pÜ ENTER fîr att installera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Tryck pÜ F3 fîr att avbryta installationen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   F3 = Avbryt",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSELicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licensering:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS Ñr licenserad under GNU GPL med delar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "av den medfîljande koden licenserad under GPL-fîrenliga",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenser sÜsom X11-, BSD- och GNU LGPL-licenserna.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "All mjukvara som Ñr del av ReactOS Ñr publicerad",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "under GNU GPL, men Ñven den ursprungliga",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "licensen Ñr upprÑtthÜllen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Denna mjukvara har INGEN GARANTI eller begrÑnsing pÜ anvÑndning",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "bortsett frÜn tillÑmplig lokal och internationell lag. Licenseringen av",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS tÑcker endast distrubering till tredje part.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Om Ni av nÜgon anledning ej fÜtt en kopia av",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License med ReactOS, besîk",
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
        "Garanti:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Detta Ñr gratis mjukvara; se kÑllkoden fîr restriktioner angÜende kopiering.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "INGEN GARANTI ges; inte ens fîr SéLJBARHET eller PASSANDE FôR ETT",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "SPECIELLT SYFTE. ALL ANVéNDNING SKER Pè EGEN RISK!",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ètervÑnd",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEDevicePageEntries[] =
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
        "Listan nedanfîr visar instÑllningarna fîr maskinvaran.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "       Dator:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "        BildskÑrm:",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "       Tangentbord:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Tangentbordslayout:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "         Acceptera:",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "Acceptera dessa maskinvaruinstÑllningar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "éndra instÑllningarna genom att trycka pÜ UPP- och NED-piltangenterna",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "fîr att markera en instÑllning, och tryck pÜ ENTER fîr att vÑlja",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "instÑllningen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "NÑr alla instÑllningar Ñr korrekta, vÑlj \"Acceptera dessa maskinvaruinstÑllningar\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "och tryck pÜ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   F3 = Avbryt",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEUpgradePageEntries[] =
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

static MUI_ENTRY svSEComputerPageEntries[] =
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
        "éndra vilken typ av dator som ska installeras.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  AnvÑnd UPP- och NED-piltangenterna fîr att vÑlja înskad datortyp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryck sen pÜ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck pÜ ESC fîr att ÜtervÑnda till den fîregÜende sidan utan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   att Ñndra datortypen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   ESC = ètervÑnd   F3 = Avbryt",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Datorn fîrsÑkrar sig om att all data Ñr lagrad pÜ hÜrdisken.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Detta kommer att ta en stund.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "NÑr detta Ñr fÑrdigt kommer datorn att startas om automatiskt.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Rensar cachen",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Installationen av ReactOS har inte slutfîrts.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-lÑsare A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor frÜn CD/DVD-lÑsarna.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryck pÜ ENTER fîr att starta om datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Var god vÑnta ...",
        TEXT_TYPE_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEDisplayPageEntries[] =
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
        "éndra vilken typ av bildskÑrmsinstÑllning som ska installeras.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  AnvÑnd UPP- och NED-piltangenterna fîr att vÑlja înskad instÑllning.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryck sedan pÜ ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck pÜ ESC fîr att ÜtervÑnda till den fîregÜende sidan utan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   att Ñndra bildskÑrmsinstÑllningen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   ESC = ètervÑnd   F3 = Avbryt",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSESuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS har nu installerats pÜ datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-lÑsare A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor frÜn CD/DVD-lÑsarna.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryck pÜ ENTER fîr att starta om datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Starta om datorn",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEBootPageEntries[] =
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
        "Setup misslyckades med att installera bootloadern pÜ datorns",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "hÜrddisk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Var god sÑtt in en formatterad floppy-disk i lÑsare A: och",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "tryck pÜ ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   F3 = Avbryt",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY svSESelectPartitionEntries[] =
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
        "Lista nedan visar befintliga partitioner och oanvÑndt",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "diskutrymme fîr nya partitioner.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Tryck UPP eller NER tangenten fîr att vÑlja i listan.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck ENTER fîr att installerara ReactOS till vald partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press P to create a primary partition.",
//        "\x07  Tryck C fîr att skapa en ny partition.",
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
        "\x07  Tryck D fîr att ta bort en befintlig partititon.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Var VÑnlig VÑnta...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEConfirmDeleteSystemPartitionEntries[] =
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

static MUI_ENTRY svSEFormatPartitionEntries[] =
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
        "Formatera partition",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Partitionen kommer nu att formaters Tryck ENTER fîr att fortsÑtta.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   F3 = Avsluta",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY svSEInstallDirectoryEntries[] =
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
        "ReactOS installeras till vald partition. VÑlj en",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "mapp som du vill installera ReactOS till.:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Fîr att Ñndra den fîreslagna mappen, tryck BACKSTEG fîr att radera",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "bokstÑver och skriv sedan in mappen dit du vill att ReactOS ska bli",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "installerad.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   F3 = Avsluta",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Var vÑnlig vÑnta medans ReactOS Setup kopieras till din ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        30,
        13,
        "installationsmapp.",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "Detta kan ta flera minuter att fullfîlja.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Var VÑnlig VÑnta...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEBootLoaderEntries[] =
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
        "Setup installerar boot-loadern",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Installera bootloadern till harddisken (MBR and VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Installera bootloadern till hÜrddisken (VBR only).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Installera bootloadern till en diskett.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Skippa installation av bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   F3 = Avsluta",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEKeyboardSettingsEntries[] =
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
        "Du vill Ñndra tangentbordstyp som ska intealleras.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryck UP eller NER tangenten fîr att vÑlja înskat tangentbordstyp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryck sedan ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck ESC tangenten fîr att ÜtergÜ till fîrra sidan utan att Ñndra nÜgot.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   tangentbordstyp.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   ESC = Avbryt   F3 = Avsluta",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSELayoutSettingsEntries[] =
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
        "Var vÑnlig och vÑlj layout du vill installera som standard.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryck UPP eller NER tangenten fîr att vÑlja înskad",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    tangentbordslayout. Tryck sedan ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck ESC tangenten fîr att ÜtergÜ till fîrra sidan utan att Ñndra",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   tangentbordslayout.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   ESC = Avbryt   F3 = Avsluta",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY svSEPrepareCopyEntries[] =
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
        "Setup fîrbereder din dator fîr kopiering av ReactOS filer. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   SammanstÑller filkopieringslistan...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY svSESelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "VÑlj ett filsystem i listan nedan.",
        0
    },
    {
        8,
        19,
        "\x07  Tryck UPP or NER tangenten fîr att vÑlja filsystem.",
        0
    },
    {
        8,
        21,
        "\x07  Tryck ENTER fîr att formatera partitionen.",
        0
    },
    {
        8,
        23,
        "\x07  Tryck ESC fîr att vÑlja en annan partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = FortsÑtt   ESC = Avbryt   F3 = Avsluta",
        TEXT_TYPE_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEDeletePartitionEntries[] =
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
        "Du har valt att ta bort partitionen",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Tryck D fîr att ta bort partitionen.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "VARNING: Alla data pÜ denna partition kommer att fîrloras!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck ESC fîr att avbryta.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Tar bort Partitionen   ESC = Avbryt   F3 = Avsluta",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSERegistryEntries[] =
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
        "Setup uppdaterar systemkonfigurationen. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Skapar regististerdatafiler...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR svSEErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Success\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS installerades inte fullstÑndigt pÜ din\n"
        "dator. Om du avslutar Setup nu, kommer du att behîva\n"
        "kîra Setup igen fîr att installera ReactOS.\n"
        "\n"
        "  \x07  Tryck ENTER fîr att fortsÑtta Setup.\n"
        "  \x07  Tryck F3 fîr att avsluta Setup.",
        "F3 = Avsluta  ENTER = FortsÑtta"
    },
    {
        //ERROR_NO_HDD
        "Setup kunde inte hitta nÜgon hÜrddisk.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup kunde inte hitta sin kÑlldisk.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup misslyckades att lÑsa in filen TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup fann en korrupt TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup hittade en ogiltig signatur i TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup kunde inte lÑsa in informationen om systemenheten.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup misslyckades installera FAT bootkod pÜ systempartitionen.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup misslyckades att lÑsa datortypslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup misslyckades att lÑsa in skÑrminstÑllningslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup misslyckades att lÑsa in tangentbordstypslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup misslyckades att lÑsa in tangentbordslayoutslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_WARN_PARTITION,
        "Setup fann att minst en hÜrdisk innehÜller en partitionstabell\n"
        "inte Ñr kompatibel och inte kan hanteras korrekt!\n"
        "\n"
        "Skapa eller ta bort partitioner kan fîrstîra partitionstabellen.\n"
        "\n"
        "  \x07  Tryck F3 fîr att avsluta Setup."
        "  \x07  Tryck ENTER fîr att fortsÑtta.",
        "F3 = Avsluta  ENTER = FortsÑtt"
    },
    {
        //ERROR_NEW_PARTITION,
        "Du kan inte skapa en partition inuti\n"
        "en redat befintlig partition!\n"
        "\n"
        "  * Tryck valfri tangent fîr att fortsÑtta.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Du kan inte ta bort opartitionerrat diskutrymme!\n"
        "\n"
        "  * Tryck valfri tangent fîr att fortsÑtta.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup misslyckades att installera FAT bootkoden pÜ systempartitionen.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_NO_FLOPPY,
        "Ingen disk i enhet A:.",
        "ENTER = FortsÑtt"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup misslyckades att updatera instÑllninarna fîr tangentbordslayout.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup misslyckades att uppdatera skÑrmregisterinstÑllningen.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup misslyckades att improterea en registerdatafil.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup misslyckades att hitta registerdatafilerna.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup misslyckades att skapa registerdatafilerna.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup misslyckades att initialisera registret.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Kabinettet has inen giltig inf fil.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_CABINET_MISSING,
        "Kabinettet hittades inte.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Kabinettet har inget installationsskript.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup misslyckades att îppna filkopierningskîn.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup kunnde inte skapa installationsmapparna.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup misslyckades att hitta 'Directories' sektionen\n"
        "i TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup misslyckades att hitta 'Directories' sektionen\n"
        "i kabinettet.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup kunnde inte skapa installationsmappen.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup misslyckades att hitta 'SetupData' sektionen\n"
        "i TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup misslyckades att skriva partitionstabellen.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup misslyckades att lÑgga till vald codepage till registret.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup kunnde inte stÑlla in 'system locale'.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Setup misslyckades att lÑgga till tangentbordslayouten till registret.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Setup kunde inte stalla int 'geo id'.\n"
        "ENTER = Starta om datorn"
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
        "  * Tryck valfri tangent fîr att fortsÑtta.",
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

MUI_PAGE svSEPages[] =
{
    {
        LANGUAGE_PAGE,
        svSELanguagePageEntries
    },
    {
       WELCOME_PAGE,
       svSEWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        svSEIntroPageEntries
    },
    {
        LICENSE_PAGE,
        svSELicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        svSEDevicePageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        svSEUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        svSEComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        svSEDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        svSEFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        svSESelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        svSEConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        svSESelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        svSEFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        svSEDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        svSEInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        svSEPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        svSEFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        svSEKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        svSEBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        svSELayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        svSEQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        svSESuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        svSEBootPageEntries
    },
    {
        REGISTRY_PAGE,
        svSERegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING svSEStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Var vÑnlig vÑnta..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   P = Create Primary   E = Create Extended   F3 = Quit"},
//     "   ENTER = Installera   C = Skapa Partition   F3 = Avsluta"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Install   L = Create Logical Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Installera   D = Ta bort Partition   F3 = Avsluta"},
    {STRING_DELETEPARTITION,
     "   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Storlek pÜ den nya partitionen:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a primary partition on"},
//     "Du har valt att skapa en ny partition pÜ"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDDSIZE,
    "VÑnligen skriv in storleken av den nya partitionen i megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Skapa Partition   ESC = Avbryt   F3 = Avsluta"},
    {STRING_PARTFORMAT,
    "Denna Partition kommer att bli formaterad hÑrnÑst."},
    {STRING_NONFORMATTEDPART,
    "Du valde att installera ReactOS pÜ en oformaterad partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Setup installerar ReactOS till Partitionen"},
    {STRING_CHECKINGPART,
    "Setup undersîker nu den valda partitionen."},
    {STRING_CONTINUE,
    "ENTER = FortsÑtt"},
    {STRING_QUITCONTINUE,
    "F3 = Avsluta  ENTER = FortsÑtt"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Starta om datorn"},
    {STRING_TXTSETUPFAILED,
    "Setup misslyckades att hitta '%S' sektionen\ni TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Kopierar fil: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup kopierar filer..."},
    {STRING_REGHIVEUPDATE,
    "   Uppdaterar registerdatafiler..."},
    {STRING_IMPORTFILE,
    "   Importerar %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Uppdaterar skÑrmregisterinstÑllningar..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Uppdaterar lokala instÑllningar..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Uppdaterar tangentbordslayoutinstÑllningar..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   LÑgger till information om codepage till registret..."},
    {STRING_DONE,
    "   FÑrdigt..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Starta om datorn"},
    {STRING_CONSOLEFAIL1,
    "Det gÜr inte îppna Konsollen\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Den vanligaste orsaken till detta Ñr att ett USB tangentbord anvÑnds\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB tangentbord Ñr itne helt stîtt Ñn\r\n"},
    {STRING_FORMATTINGDISK,
    "Setup formaterar din disk"},
    {STRING_CHECKINGDISK,
    "Setup underîker din disk"},
    {STRING_FORMATDISK1,
    " Formaterar partition som %S filsystem (snabbformatering) "},
    {STRING_FORMATDISK2,
    " Formaterar partition som %S filsystem "},
    {STRING_KEEPFORMAT,
    " BehÜll nuvarande filsystem (inga fîrÑndringar) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  HÜrddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) pÜ %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  HÜrddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  HÜrddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) pÜ %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  HÜrddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "HÜrddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "pÜ HÜrddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTyp %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  HÜrddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) pÜ %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  HÜrddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup skapade en ny partition pÜ"},
    {STRING_UNPSPACE,
    "    %sOpartitionerat utrymme%s            %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Ny (Oformaterad)"},
    {STRING_FORMATUNUSED,
    "OanvÑnt"},
    {STRING_FORMATUNKNOWN,
    "OkÑnd"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "LÑgger till tangentbordslayouter"},
    {0, 0}
};
