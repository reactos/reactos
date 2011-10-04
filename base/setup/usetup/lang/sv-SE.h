/*
 * PROJECT:         ReactOS Setup
 * FILE:            \base\setup\usetup\lang\sv-SE.h  
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
        "   ENTER = Fortsätt  F3 = Avsluta",
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
        "Välkommen till ReactOS Setup!",
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
        "dator och förbereder den andra delen av installationen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck på ENTER för att installera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck på R för att reparera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck på L för att läsa licensavtalet till ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck på F3 för att avbryta installationen av ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "För mer information om ReactOS, besök:",
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
        "   ENTER = Fortsätt  R = Reparera F3 = Avbryt",
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
        "ReactOS Setup är i en tidig utvecklingsfas och saknar därför ett antal",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan förväntas av ett fullt användbart setup-program.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Följande begränsningar gäller:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Setup kan ej hantera mer än 1 primär partition per hårddisk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Setup kan ej radera en primär partition från en hårddisk",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  om utökade partitioner existerar på hårddisken.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Setup kan ej radera den första utökade partitionen från en hårddisk",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  om andra utökade partitioner existerar på hårddisken.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Setup stöder endast filsystem av typen FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- Kontrollering av hårddiskens filsystem stöds (ännu) ej.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Tryck på ENTER för att installera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Tryck på F3 för att avbryta installationen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   F3 = Avbryt",
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
        "ReactOS är licenserad under GNU GPL med delar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "av den medföljande koden licenserad under GPL-förenliga",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenser såsom X11-, BSD- och GNU LGPL-licenserna.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "All mjukvara som är del av ReactOS är publicerad",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "under GNU GPL, men även den ursprungliga",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "licensen är upprätthållen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Denna mjukvara har INGEN GARANTI eller begränsing på användning",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "bortsett från tillämplig lokal och internationell lag. Licenseringen av",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS täcker endast distrubering till tredje part.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Om Ni av någon anledning ej fått en kopia av",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License med ReactOS, besök",
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
        "Detta är gratis mjukvara; se källkoden för restriktioner angående kopiering.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "INGEN GARANTI ges; inte ens för SÄLJBARHET eller PASSANDE FÖR ETT",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "SPECIELLT SYFTE. ALL ANVÄNDNING SKER PÅ EGEN RISK!",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Återvänd",
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
        "Listan nedanför visar inställningarna för maskinvaran.",
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
        "        Bildskärm:",
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
        16, "Acceptera dessa maskinvaruinställningar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Ändra inställningarna genom att trycka på UPP- och NED-piltangenterna",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "för att markera en inställning, och tryck på ENTER för att välja",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "inställningen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "När alla inställningar är korrekta, välj \"Acceptera dessa maskinvaruinställningar\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "och tryck på ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   F3 = Avbryt",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Setup är i en tidig utvecklingsfas och saknar därför ett antal",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan förväntas av ett fullt användbart setup-program.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Reparations- och uppdateringsfunktionerna fungerar ej.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck på U för att uppdatera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck på R för Återställningskonsolen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck på ESC för att återvända till föregående sida.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck på ENTER för att starta om datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ESC = Gå till föregående sida  ENTER = Starta om datorn",
        TEXT_TYPE_STATUS
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
        "Ändra vilken typ av dator som ska installeras.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Använd UPP- och NED-piltangenterna för att välja önskad datortyp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryck sen på ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck på ESC för att återvända till den föregående sidan utan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   att ändra datortypen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   ESC = Återvänd   F3 = Avbryt",
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
        "Datorn försäkrar sig om att all data är lagrad på hårdisken.",
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
        "När detta är färdigt kommer datorn att startas om automatiskt.",
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
        "Installationen av ReactOS har inte slutförts.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-läsare A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor från CD/DVD-läsarna.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryck på ENTER för att starta om datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Var god vänta ...",
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
        "Ändra vilken typ av bildskärmsinställning som ska installeras.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Använd UPP- och NED-piltangenterna för att välja önskad inställning.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryck sedan på ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck på ESC för att återvända till den föregående sidan utan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   att ändra bildskärmsinställningen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   ESC = Återvänd   F3 = Avbryt",
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
        "ReactOS har nu installerats på datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-läsare A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor från CD/DVD-läsarna.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryck på ENTER för att starta om datorn.",
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
        "Setup misslyckades med att installera bootloadern på datorns",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "hårddisk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Var god sätt in en formatterad floppy-disk i läsare A: och",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "tryck på ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   F3 = Avbryt",
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
        "Lista nedan visar befintliga partitioner och oanvändt",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "diskutrymme för nya partitioner.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Tryck UPP eller NER tangenten för att välja i listan.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck ENTER för att installerara ReactOS till vald partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck C för att skapa en ny partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck D för att ta bort en befintlig partititon.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Var Vänlig Vänta...",
        TEXT_TYPE_STATUS
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
        "Partitionen kommer nu att formaters Tryck ENTER för att fortsätta.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   F3 = Avsluta",
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
        "ReactOS installeras till vald partition. Välj en",
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
        "För att ändra den föreslagna mappen, tryck BACKSTEG för att radera",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "bokstäver och skriv sedan in mappen dit du vill att ReactOS ska bli",
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
        "   ENTER = Fortsätt   F3 = Avsluta",
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
        "Var vänlig vänta medans ReactOS Setup kopieras till din ReactOS.",
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
        "Detta kan ta flera minuter att fullfölja.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Var Vänlig Vänta...    ",
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
        "Installera bootloadern till hårddisken (VBR only).",
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
        "   ENTER = Fortsätt   F3 = Avsluta",
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
        "Du vill ändra tangentbordstyp som ska intealleras.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryck UP eller NER tangenten för att välja önskat tangentbordstyp.",
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
        "\x07  Tryck ESC tangenten för att återgå till förra sidan utan att ändra något.",
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
        "   ENTER = Fortsätt   ESC = Avbryt   F3 = Avsluta",
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
        "Var vänlig och välj layout du vill installera som standard.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Tryck UPP eller NER tangenten för att välja önskad",
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
        "\x07  Tryck ESC tangenten för att återgå till förra sidan utan att ändra",
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
        "   ENTER = Fortsätt   ESC = Avbryt   F3 = Avsluta",
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
        "Setup förbereder din dator för kopiering av ReactOS filer. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Sammanställer filkopieringslistan...",
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
        "Välj ett filsystem i listan nedan.",
        0
    },
    {
        8,
        19,
        "\x07  Tryck UPP or NER tangenten för att välja filsystem.",
        0
    },
    {
        8,
        21,
        "\x07  Tryck ENTER för att formatera partitionen.",
        0
    },
    {
        8,
        23,
        "\x07  Tryck ESC för att välja en annan partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   ESC = Avbryt   F3 = Avsluta",
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
        "\x07  Tryck D för att ta bort partitionen.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "VARNING: Alla data på denna partition kommer att förloras!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck ESC för att avbryta.",
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
        //ERROR_NOT_INSTALLED
        "ReactOS installerades inte fullständigt på din\n"
        "dator. Om du avslutar Setup nu, kommer du att behöva\n"
        "köra Setup igen för att installera ReactOS.\n"
        "\n"
        "  \x07  Tryck ENTER för att fortsätta Setup.\n"
        "  \x07  Tryck F3 för att avsluta Setup.",
        "F3= Avsluta  ENTER = Fortsätta"
    },
    {
        //ERROR_NO_HDD
        "Setup kunde inte hitta någon hårddisk.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup kunde inte hitta sin källdisk.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup misslyckades att läsa in filen TXTSETUP.SIF.\n",
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
        "Setup kunde inte läsa in informationen om systemenheten.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup misslyckades installera FAT bootkod på systempartitionen.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup misslyckades att läsa datortypslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup misslyckades att läsa in skärminställningslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup misslyckades att läsa in tangentbordstypslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup misslyckades att läsa in tangentbordslayoutslistan.\n",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_WARN_PARTITION,
        "Setup fann att minst en hårdisk innehåller en partitionstabell\n"
        "inte är kompatibel och inte kan hanteras korrekt!\n"
        "\n"
        "Skapa eller ta bort partitioner kan förstöra partitionstabellen.\n"
        "\n"
        "  \x07  Tryck F3 för att avsluta Setup."
        "  \x07  Tryck ENTER för att fortsätta.",
        "F3= Avsluta  ENTER = Fortsätt"
    },
    {
        //ERROR_NEW_PARTITION,
        "Du kan inte skapa en partition inuti\n"
        "en redat befintlig partition!\n"
        "\n"
        "  * Tryck valfri tangent för att fortsätta.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Du kan inte ta bort opartitionerrat diskutrymme!\n"
        "\n"
        "  * Tryck valfri tangent för att fortsätta.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup misslyckades att installera FAT bootkoden på systempartitionen.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_NO_FLOPPY,
        "Ingen disk i enhet A:.",
        "ENTER = Fortsätt"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup misslyckades att updatera inställninarna för tangentbordslayout.",
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup misslyckades att uppdatera skärmregisterinställningen.",
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
        "Setup misslyckades att öppna filkopierningskön.\n",
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
        "Setup misslyckades att lägga till vald codepage till registret.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup kunnde inte ställa in 'system locale'.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Setup misslyckades att lägga till tangentbordslayouten till registret.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Setup kunde inte stalla int 'geo id'.\n"
        "ENTER = Starta om datorn"
    },
    {
        //ERROR_INSUFFICIENT_DISKSPACE,
        "Inte tillräckligt mycket fritt utrymme på den valda partitionen.\n"
        "  * Tryck valfri tangent för att fortsätta.",
        NULL
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
       START_PAGE,
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
     "   Var vänlig vänta..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Installera   C = Skapa Partition   F3 = Avsluta"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Installera   D = Ta bort Partition   F3 = Avsluta"},
    {STRING_PARTITIONSIZE,
     "Storlek på den nya partitionen:"},
    {STRING_CHOOSENEWPARTITION,
     "Du har valt att skapa en ny partiton på"},
    {STRING_HDDSIZE,
    "Vänligen skriv in storleken av den nya partitionen i megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Skapa Partition   ESC = Avbryt   F3 = Avsluta"},
    {STRING_PARTFORMAT,
    "Denna Partition kommer att bli formaterad härnäst."},
    {STRING_NONFORMATTEDPART,
    "Du valde att installera ReactOS på en oformaterad partition."},
    {STRING_INSTALLONPART,
    "Setup installerar ReactOS till Partitionen"},
    {STRING_CHECKINGPART,
    "Setup undersöker nu den valda partitionen."},
    {STRING_QUITCONTINUE,
    "F3= Avsluta  ENTER = Fortsätt"},
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
    "   Uppdaterar skärmregisterinställningar..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Uppdaterar lokala inställningar..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Uppdaterar tangentbordslayoutinställningar..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Lägger till information om codepage till registret..."},
    {STRING_DONE,
    "   Färdigt..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Starta om datorn"},
    {STRING_CONSOLEFAIL1,
    "Det går inte öppna Konsollen\n\n"},
    {STRING_CONSOLEFAIL2,
    "Den vanligaste orsaken till detta är att ett USB tangentbord används\n"},
    {STRING_CONSOLEFAIL3,
    "USB tangentbord är itne helt stött än\n"},
    {STRING_FORMATTINGDISK,
    "Setup formaterar din disk"},
    {STRING_CHECKINGDISK,
    "Setup underöker din disk"},
    {STRING_FORMATDISK1,
    " Formaterar partition som %S filsystem (snabbformatering) "},
    {STRING_FORMATDISK2,
    " Formaterar partition som %S filsystem "},
    {STRING_KEEPFORMAT,
    " Behåll nuvarande filsystem (inga förändringar) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Hårddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) på %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Hårddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Typ %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  Hårddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) på %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  Hårddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Hårddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Typ %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "på Hårddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Typ %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Hårddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) på %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Hårddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup skapade en ny partition på"},
    {STRING_UNPSPACE,
    "    Opartitionerat utrymme              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_UNFORMATTED,
    "Ny (Oformaterad)"},
    {STRING_FORMATUNUSED,
    "Oanvänt"},
    {STRING_FORMATUNKNOWN,
    "Okänd"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Lägger till tangentbordslayouter"},
    {0, 0}
};
