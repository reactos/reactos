// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/*
 * PROJECT:         ReactOS Setup
 * FILE:            base/setup/usetup/lang/sv-SE.h
 * PURPOSE:         Swedish resource file
 * Translation:     Jaix Bly plus perhaps GreatLord if blame and translate.reactos.se is consulted.
 */
#pragma once

static MUI_ENTRY svSESetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Please wait while the ReactOS Setup initializes itself",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "and discovers your devices...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Please wait...",
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

static MUI_ENTRY svSELanguagePageEntries[] =
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
        "Language Selection.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Then Tryck ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  This Language will be the default language for the final system.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt  F3 = Avsluta",
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

static MUI_ENTRY svSEWelcomePageEntries[] =
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
        "V\204lkommen till ReactOS Setup!",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Denna del av installationen kopierar ReactOS till eran",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "dator och f\224rbereder den andra delen av installationen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tryck p\206 R f\224r att reparera ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tryck p\206 L f\224r att l\204sa licensavtalet till ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tryck p\206 F3 f\224r att avbryta installationen av ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "F\224r mer information om ReactOS, bes\224k:",
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
        "   ENTER = Forts\204tt  R = Reparera F3 = Avbryt",
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

static MUI_ENTRY svSEIntroPageEntries[] =
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
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   F3 = Quit",
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

static MUI_ENTRY svSELicensePageEntries[] =
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
        6,
        "Licensering:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS \204r licenserad under GNU GPL med delar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "av den medf\224ljande koden licenserad under GPL-f\224renliga",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licenser s\206som X11-, BSD- och GNU LGPL-licenserna.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "All mjukvara som \204r del av ReactOS \204r publicerad",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "under GNU GPL, men \204ven den ursprungliga",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "licensen \204r uppr\204tth\206llen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Denna mjukvara har INGEN GARANTI eller begr\204nsing p\206 anv\204ndning",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "bortsett fr\206n till\204mplig lokal och internationell lag. Licenseringen av",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "ReactOS t\204cker endast distrubering till tredje part.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Om Ni av n\206gon anledning ej f\206tt en kopia av",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "GNU General Public License med ReactOS, bes\224k",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "Garanti:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Detta \204r gratis mjukvara; se k\204llkoden f\224r restriktioner ang\206ende kopiering.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "INGEN GARANTI ges; inte ens f\224r S\216LJBARHET eller PASSANDE F\231R ETT",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "SPECIELLT SYFTE. ALL ANV\216NDNING SKER P\217 EGEN RISK!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = \217terv\204nd",
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

static MUI_ENTRY svSEDevicePageEntries[] =
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
        "Listan nedanf\224r visar inst\204llningarna f\224r maskinvaran.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "       Dator:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "        Bildsk\204rm:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "       Tangentbord:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Tangentbordslayout:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "         Acceptera:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Acceptera dessa maskinvaruinst\204llningar",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "\216ndra inst\204llningarna genom att trycka p\206 UPP- och NED-piltangenterna",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "f\224r att markera en inst\204llning, och tryck p\206 ENTER f\224r att v\204lja",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "inst\204llningen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "N\204r alla inst\204llningar \204r korrekta, v\204lj \"Acceptera dessa maskinvaruinst\204llningar\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "och tryck p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   F3 = Avbryt",
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

static MUI_ENTRY svSERepairPageEntries[] =
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
        "ReactOS Setup \204r i en tidig utvecklingsfas och saknar d\204rf\224r ett antal",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "funktioner som kan f\224rv\204ntas av ett fullt anv\204ndbart setup-program.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Reparations- och uppdateringsfunktionerna fungerar ej.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tryck p\206 U f\224r att uppdatera ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tryck p\206 R f\224r \217terst\204llningskonsolen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tryck p\206 ESC f\224r att \206terv\204nda till f\224reg\206ende sida.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tryck p\206 ENTER f\224r att starta om datorn.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ESC = G\206 till f\224reg\206ende sida  ENTER = Starta om datorn",
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

static MUI_ENTRY svSEUpgradePageEntries[] =
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
        "The ReactOS Setup can upgrade one of the available ReactOS installations",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "listed below, or, if a ReactOS installation is damaged, the Setup program",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "can attempt to repair it.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "The repair functions are not all implemented yet.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press UP or DOWN to select an OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Press U for upgrading the selected OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ESC to continue with a new installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Upgrade   ESC = Do not upgrade   F3 = Quit",
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

static MUI_ENTRY svSEComputerPageEntries[] =
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
        "\216ndra vilken typ av dator som ska installeras.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Anv\204nd UPP- och NED-piltangenterna f\224r att v\204lja \224nskad datortyp.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Tryck sen p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryck p\206 ESC f\224r att \206terv\204nda till den f\224reg\206ende sidan utan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   att \204ndra datortypen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   ESC = \217terv\204nd   F3 = Avbryt",
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

static MUI_ENTRY svSEFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Datorn f\224rs\204krar sig om att all data \204r lagrad p\206 h\206rdisken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Detta kommer att ta en stund.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "N\204r detta \204r f\204rdigt kommer datorn att startas om automatiskt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Rensar cachen",
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

static MUI_ENTRY svSEQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Installationen av ReactOS har inte slutf\224rts.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-l\204sare A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "och tag ur alla skivor fr\206n CD/DVD-l\204sarna.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tryck p\206 ENTER f\224r att starta om datorn.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Var god v\204nta...",
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

static MUI_ENTRY svSEDisplayPageEntries[] =
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
        "\216ndra vilken typ av bildsk\204rmsinst\204llning som ska installeras.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Anv\204nd UPP- och NED-piltangenterna f\224r att v\204lja \224nskad inst\204llning.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Tryck sedan p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryck p\206 ESC f\224r att \206terv\204nda till den f\224reg\206ende sidan utan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   att \204ndra bildsk\204rmsinst\204llningen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   ESC = \217terv\204nd   F3 = Avbryt",
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

static MUI_ENTRY svSESuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS har nu installerats p\206 datorn.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-l\204sare A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "och tag ur alla skivor fr\206n CD/DVD-l\204sarna.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tryck p\206 ENTER f\224r att starta om datorn.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Starta om datorn",
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

static MUI_ENTRY svSESelectPartitionEntries[] =
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
        "Lista nedan visar befintliga partitioner och oanv\204ndt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "diskutrymme f\224r nya partitioner.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Tryck UPP eller NER tangenten f\224r att v\204lja i listan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryck ENTER f\224r att installerara ReactOS till vald partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tryck C f\224r att skapa en prim\344r/logisk partition.",
//        "\x07  Tryck C f\224r att skapa en ny partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tryck E f\224r att skapa en ut\224kad partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tryck D f\224r att ta bort en befintlig partititon.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Var V\204nlig V\204nta...",
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

static MUI_ENTRY svSEChangeSystemPartition[] =
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
        "The current system partition of your computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "on the system disk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "uses a format not supported by ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "In order to successfully install ReactOS, the Setup program must change",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "the current system partition to a new one.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "The new candidate system partition is:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  To accept this choice, press ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  To manually change the system partition, press ESC to go back to",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   the partition selection list, then select or create a new system",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   partition on the system disk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "In case there are other operating systems that depend on the original",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "system partition, you may need to either reconfigure them for the new",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "system partition, or you may need to change the system partition back",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "to the original one after finishing the installation of ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   ESC = Cancel",
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

static MUI_ENTRY svSEConfirmDeleteSystemPartitionEntries[] =
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
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   not be deleted.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
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

static MUI_ENTRY svSEFormatPartitionEntries[] =
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
        "Formatera partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Partitionen kommer nu att formaters Tryck ENTER f\224r att forts\204tta.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   F3 = Avsluta",
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

static MUI_ENTRY svSECheckFSEntries[] =
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
        "Setup unders\224ker nu den valda partitionen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Var v\204nlig v\204nta...",
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

static MUI_ENTRY svSEInstallDirectoryEntries[] =
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
        "ReactOS installeras till vald partition. V\204lj en",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "mapp som du vill installera ReactOS till.:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "F\224r att \204ndra den f\224reslagna mappen, tryck BACKSTEG f\224r att radera",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "bokst\204ver och skriv sedan in mappen dit du vill att ReactOS ska bli",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "installerad.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   F3 = Avsluta",
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

static MUI_ENTRY svSEFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        11,
        12,
        "Var v\204nlig v\204nta medans ReactOS Setup kopieras till din ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        30,
        13,
        "installationsmapp.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        20,
        14,
        "Detta kan ta flera minuter att fullf\224lja.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "                                                           \xB3 Var V\204nlig V\204nta...    ",
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

static MUI_ENTRY svSEBootLoaderSelectPageEntries[] =
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
        "Please select where Setup should install the bootloader:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Installera bootloadern till harddisken (MBR and VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Installera bootloadern till h\206rddisken (VBR only).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Installera bootloadern till en diskett.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Skippa installation av bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   F3 = Avsluta",
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

static MUI_ENTRY svSEBootLoaderInstallPageEntries[] =
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
        "Setup installerar boot-loadern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Installing the bootloader onto the media, please wait...",
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

static MUI_ENTRY svSEBootLoaderRemovableDiskPageEntries[] =
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
        "Setup installerar boot-loadern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Var god s\204tt in en formatterad floppy-disk i l\204sare A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "och tryck p\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   F3 = Avbryt",
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

static MUI_ENTRY svSEKeyboardSettingsEntries[] =
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
        "Du vill \204ndra tangentbordstyp som ska intealleras.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tryck UP eller NER tangenten f\224r att v\204lja \224nskat tangentbordstyp.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Tryck sedan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryck ESC tangenten f\224r att \206terg\206 till f\224rra sidan utan att \204ndra n\206got.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   tangentbordstyp.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   ESC = Avbryt   F3 = Avsluta",
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

static MUI_ENTRY svSELayoutSettingsEntries[] =
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
        "Var v\204nlig och v\204lj layout du vill installera som standard.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tryck UPP eller NER tangenten f\224r att v\204lja \224nskad",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    tangentbordslayout. Tryck sedan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tryck ESC tangenten f\224r att \206terg\206 till f\224rra sidan utan att \204ndra",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   tangentbordslayout.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   ESC = Avbryt   F3 = Avsluta",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
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
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup f\224rbereder din dator f\224r kopiering av ReactOS filer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Sammanst\204ller filkopieringslistan...",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
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
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "V\204lj ett filsystem i listan nedan.",
        0
    },
    {
        8,
        19,
        "\x07  Tryck UPP or NER tangenten f\224r att v\204lja filsystem.",
        0
    },
    {
        8,
        21,
        "\x07  Tryck ENTER f\224r att formatera partitionen.",
        0
    },
    {
        8,
        23,
        "\x07  Tryck ESC f\224r att v\204lja en annan partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = Forts\204tt   ESC = Avbryt   F3 = Avsluta",
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

static MUI_ENTRY svSEDeletePartitionEntries[] =
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
        "Du har valt att ta bort partitionen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Tryck L f\224r att ta bort partitionen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "VARNING: Alla data p\206 denna partition kommer att f\224rloras!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tryck ESC f\224r att avbryta.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   L = Tar bort Partitionen   ESC = Avbryt   F3 = Avsluta",
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

static MUI_ENTRY svSERegistryEntries[] =
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
        "Setup uppdaterar systemkonfigurationen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Skapar regististerdatafiler...",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
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
        // ERROR_NOT_INSTALLED
        "ReactOS installerades inte fullst\204ndigt p\206 din\n"
        "dator. Om du avslutar Setup nu, kommer du att beh\224va\n"
        "k\224ra Setup igen f\224r att installera ReactOS.\n"
        "\n"
        "  \x07  Tryck ENTER f\224r att forts\204tta Setup.\n"
        "  \x07  Tryck F3 f\224r att avsluta Setup.",
        "F3 = Avsluta  ENTER = Forts\204tta"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Failed to build the installation paths for the ReactOS installation directory!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_PATH
        "You cannot delete the partition containing the installation source!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_DIR
        "You cannot install ReactOS within the installation source directory!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_NO_HDD
        "Setup kunde inte hitta n\206gon h\206rddisk.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Setup kunde inte hitta sin k\204lldisk.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Setup misslyckades att l\204sa in filen TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Setup fann en korrupt TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup hittade en ogiltig signatur i TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Setup kunde inte l\204sa in informationen om systemenheten.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_WRITE_BOOT,
        "Setup misslyckades installera %S bootkod p\206 systempartitionen.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Setup misslyckades att l\204sa datortypslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Setup misslyckades att l\204sa in sk\204rminst\204llningslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Setup misslyckades att l\204sa in tangentbordstypslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Setup misslyckades att l\204sa in tangentbordslayoutslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_WARN_PARTITION,
        "Setup fann att minst en h\206rdisk inneh\206ller en partitionstabell\n"
        "inte \204r kompatibel och inte kan hanteras korrekt!\n"
        "\n"
        "Skapa eller ta bort partitioner kan f\224rst\224ra partitionstabellen.\n"
        "\n"
        "  \x07  Tryck F3 f\224r att avsluta Setup."
        "  \x07  Tryck ENTER f\224r att forts\204tta.",
        "F3 = Avsluta  ENTER = Forts\204tt"
    },
    {
        // ERROR_NEW_PARTITION,
        "Du kan inte skapa en partition inuti\n"
        "en redat befintlig partition!\n"
        "\n"
        "  * Tryck valfri tangent f\224r att forts\204tta.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Setup misslyckades att installera %S bootkoden p\206 systempartitionen.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_NO_FLOPPY,
        "Ingen disk i enhet A:.",
        "ENTER = Forts\204tt"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Setup misslyckades att updatera inst\204llninarna f\224r tangentbordslayout.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup misslyckades att uppdatera sk\204rmregisterinst\204llningen.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Setup misslyckades att improterea en registerdatafil.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_FIND_REGISTRY
        "Setup misslyckades att hitta registerdatafilerna.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_CREATE_HIVE,
        "Setup misslyckades att skapa registerdatafilerna.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Setup misslyckades att initialisera registret.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Kabinettet has inen giltig inf fil.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_CABINET_MISSING,
        "Kabinettet hittades inte.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Kabinettet har inget installationsskript.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_COPY_QUEUE,
        "Setup misslyckades att \224ppna filkopierningsk\224n.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_CREATE_DIR,
        "Setup kunnde inte skapa installationsmapparna.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Setup misslyckades att hitta '%S' sektionen\n"
        "i TXTSETUP.SIF.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_CABINET_SECTION,
        "Setup misslyckades att hitta '%S' sektionen\n"
        "i kabinettet.\n",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Setup kunnde inte skapa installationsmappen.",
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Setup misslyckades att skriva partitionstabellen.\n"
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Setup misslyckades att l\204gga till vald codepage till registret.\n"
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup kunnde inte st\204lla in 'system locale'.\n"
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Setup misslyckades att l\204gga till tangentbordslayouten till registret.\n"
        "ENTER = Starta om datorn"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Setup kunde inte stalla int 'geo id'.\n"
        "ENTER = Starta om datorn"
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
        "  * Tryck valfri tangent f\224r att forts\204tta.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "You cannot create a new primary or extended partition in the\n"
        "partition table of this disk because the partition table is full.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "You cannot create more than one extended partition per disk.\n"
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

MUI_PAGE svSEPages[] =
{
    {
        SETUP_INIT_PAGE,
        svSESetupInitPageEntries
    },
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
        REPAIR_INTRO_PAGE,
        svSERepairPageEntries
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
        CHANGE_SYSTEM_PARTITION,
        svSEChangeSystemPartition
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
        CHECK_FILE_SYSTEM_PAGE,
        svSECheckFSEntries
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
        BOOTLOADER_SELECT_PAGE,
        svSEBootLoaderSelectPageEntries
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
        BOOTLOADER_INSTALL_PAGE,
        svSEBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        svSEBootLoaderRemovableDiskPageEntries
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
     "   Var v\204nlig v\204nta..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Installera   C = Skapa Prim\344r Partition   E = Skapa Ut\224kad Partition   F3 = Avsluta"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Installera   C = Skapa Logisk Partition   F3 = Avsluta"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Installera   D = Ta bort Partition   F3 = Avsluta"},
    {STRING_DELETEPARTITION,
     "   D = Ta bort Partition   F3 = Avsluta"},
    {STRING_PARTITIONSIZE,
     "Storlek p\206 den nya partitionen:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "You have chosen to create a primary partition on"},
//     "Du har valt att skapa en ny partition p†"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "You have chosen to create an extended partition on"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "You have chosen to create a logical partition on"},
    {STRING_HDPARTSIZE,
    "V\204nligen skriv in storleken av den nya partitionen i megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Skapa Partition   ESC = Avbryt   F3 = Avsluta"},
    {STRING_NEWPARTITION,
    "Setup skapade en ny partition p\206"},
    {STRING_PARTFORMAT,
    "Denna Partition kommer att bli formaterad h\204rn\204st."},
    {STRING_NONFORMATTEDPART,
    "Du valde att installera ReactOS p\206 en oformaterad partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Setup installerar ReactOS till Partitionen"},
    {STRING_CONTINUE,
    "ENTER = Forts\204tt"},
    {STRING_QUITCONTINUE,
    "F3 = Avsluta  ENTER = Forts\204tt"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Starta om datorn"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kopierar fil: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup kopierar filer..."},
    {STRING_REGHIVEUPDATE,
    "   Uppdaterar registerdatafiler..."},
    {STRING_IMPORTFILE,
    "   Importerar %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Uppdaterar sk\204rmregisterinst\204llningar..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Uppdaterar lokala inst\204llningar..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Uppdaterar tangentbordslayoutinst\204llningar..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   L\204gger till information om codepage..."},
    {STRING_DONE,
    "   F\204rdigt..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Starta om datorn"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Det g\206r inte \224ppna Konsollen\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Den vanligaste orsaken till detta \204r att ett USB tangentbord anv\204nds\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB tangentbord \204r itne helt st\224tt \204n\r\n"},
    {STRING_FORMATTINGPART,
    "Setup formaterar partitionen..."},
    {STRING_CHECKINGDISK,
    "Setup unders\224ker disken..."},
    {STRING_FORMATDISK1,
    " Formaterar partition som %S filsystem (snabbformatering) "},
    {STRING_FORMATDISK2,
    " Formaterar partition som %S filsystem "},
    {STRING_KEEPFORMAT,
    " Beh\206ll nuvarande filsystem (inga f\224r\204ndringar) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "p\206 %s."},
    {STRING_PARTTYPE,
    "Typ 0x%02x"},
    {STRING_HDDINFO1,
    // "H\206rddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s H\206rddisk %lu (Port=%hu, Bus=%hu, Id=%hu) p\206 %wZ [%s]"},
    {STRING_HDDINFO2,
    // "H\206rddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s H\206rddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Opartitionerat utrymme"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Extended Partition"},
    {STRING_UNFORMATTED,
    "Ny (Oformaterad)"},
    {STRING_FORMATUNUSED,
    "Oanv\204nt"},
    {STRING_FORMATUNKNOWN,
    "Ok\204nd"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "L\204gger till tangentbordslayouter"},
    {0, 0}
};
