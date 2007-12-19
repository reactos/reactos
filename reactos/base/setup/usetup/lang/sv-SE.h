#ifndef LANG_SV_SE_H__
#define LANG_SV_SE_H__

static MUI_ENTRY svSEWelcomePageEntries[] =
{
    {
        6,
        8,
        "Välkommen till ReactOS Setup!",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Denna del av installationen kopierar ReactOS till eran",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "dator och förbereder den andra delen av installationen.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck på ENTER för att installera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck på R för att reparera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck på L för att läsa licensavtalet till ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck på F3 för att avbryta installationen av ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "För mer information om ReactOS, besök:",
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
        "   ENTER = Fortsätt  R = Reparera F3 = Avbryt",
        TEXT_STATUS
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
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Setup är i en tidig utvecklingsfas och saknar därför ett antal",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan förväntas av ett fullt användbart setup-program.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Följande begränsningar gäller:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Setup kan ej hantera mer än 1 primär partition per hårddisk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Setup kan ej radera en primär partition från en hårddisk",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  om utökade partitioner existerar på hårddisken.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Setup kan ej radera den första utökade partitionen från en hårddisk",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  om andra utökade partitioner existerar på hårddisken.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Setup stöder endast filsystem av typen FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Kontrollering av hårddiskens filsystem stöds (ännu) ej.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Tryck på ENTER för att installera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Tryck på F3 för att avbryta installationen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   F3 = Avbryt",
        TEXT_STATUS
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
        6,
        6,
        "Licensering:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS är licenserad under GNU GPL med delar",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "av den medföljande koden licenserad under GPL-förenliga",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licenser såsom X11-, BSD- och GNU LGPL-licenserna.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "All mjukvara som är del av ReactOS är publicerad",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "under GNU GPL, men även den ursprungliga",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "licensen är upprätthållen.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "Denna mjukvara har INGEN GARANTI eller begränsing på användning",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "bortsett från tillämplig lokal och internationell lag. Licenseringen av",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "ReactOS täcker endast distrubering till tredje part.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Om Ni av någon anledning ej fått en kopia av",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License med ReactOS, besök",
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
        "Garanti:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "Detta är gratis mjukvara; se källkoden för restriktioner angående kopiering.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "INGEN GARANTI ges; inte ens för SÄLJBARHET eller PASSANDE FÖR ETT",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "SPECIELLT SYFTE. ALL ANVÄNDNING SKER PÅ EGEN RISK!",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Återvänd",
        TEXT_STATUS
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
        6,
        8,
        "Listan nedanför visar inställningarna för maskinvaran.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Dator:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Bildskärm:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Tangentbord:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Tangentbordslayout:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         Acceptera:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Acceptera dessa maskinvaruinställningar",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Ändra inställningarna genom att trycka på UPP- och NED-piltangenterna",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "för att markera en inställning, och tryck på ENTER för att välja",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "inställningen.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "När alla inställningar är korrekta, välj \"Acceptera dessa maskinvaruinställningar\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "och tryck på ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   F3 = Avbryt",
        TEXT_STATUS
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
        6,
        8,
        "ReactOS Setup är i en tidig utvecklingsfas och saknar därför ett antal",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan förväntas av ett fullt användbart setup-program.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Reparations- och uppdateringsfunktionerna fungerar ej.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck på U för att uppdatera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck på R för Återställningskonsolen.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck på ESC för att återvända till föregående sida.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck på ENTER för att starta om datorn.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Gå till föregående sida  ENTER = Starta om datorn",
        TEXT_STATUS
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
        6,
        8,
        "Ändra vilken typ av dator som ska installeras.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Använd UPP- och NED-piltangenterna för att välja önskad datortyp.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Tryck sen på ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck på ESC för att återvända till den föregående sidan utan",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   att ändra datortypen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   ESC = Återvänd   F3 = Avbryt",
        TEXT_STATUS
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
        10,
        6,
        "Datorn försäkrar sig om att all data är lagrad på hårdisken.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Detta kommer att ta en stund.",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "När detta är färdigt kommer datorn att startas om automatiskt.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Rensar cachen",
        TEXT_STATUS
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
        10,
        6,
        "Installationen av ReactOS har inte slutförts.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-läsare A:",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor från CD/DVD-läsarna.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Tryck på ENTER för att starta om datorn.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Var god vänta ...",
        TEXT_STATUS,
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
        6,
        8,
        "Ändra vilken typ av bildskärmsinställning som ska installeras.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Använd UPP- och NED-piltangenterna för att välja önskad inställning.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Tryck sedan på ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck på ESC för att återvända till den föregående sidan utan",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   att ändra bildskärmsinställningen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   ESC = Återvänd   F3 = Avbryt",
        TEXT_STATUS
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
        10,
        6,
        "ReactOS har nu installerats på datorn.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-läsare A:",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor från CD/DVD-läsarna.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Tryck på ENTER för att starta om datorn.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Starta om datorn",
        TEXT_STATUS
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
        6,
        8,
        "Setup misslyckades med att installera bootloadern på datorns",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "hårddisk",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Var god sätt in en formatterad floppy-disk i läsare A: och",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "tryck på ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Fortsätt   F3 = Avbryt",
        TEXT_STATUS
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
        "\x07  Press UP or DOWN to select a list entry.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press ENTER to install ReactOS onto the selected partition.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Press C to create a new partition.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Press D to delete an existing partition.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Please wait...",
        TEXT_STATUS
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
        6,
        8,
        "Format partition",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Setup will now format the partition. Press ENTER to continue.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY svSEInstallDirectoryEntries[] =
{
    {
        6,
        8,
        "Setup installs ReactOS files onto the selected partition. Choose a",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "directory where you want ReactOS to be installed:",
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
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
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
        11,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "installation folder.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "This may take several minutes to complete.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Please wait...    ",
        TEXT_STATUS
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
        6,
        8,
        "Setup is installing the boot loader",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Install bootloader on the harddisk (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on a floppy disk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Skip install bootloader.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
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
        6,
        8,
        "You want to change the type of keyboard to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard type.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the keyboard type.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
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
        6,
        8,
        "You want to change the keyboard layout to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    layout. Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the keyboard layout.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
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
        6,
        8,
        "Setup prepares your computer for copying the ReactOS files. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Building the file copy list...",
        TEXT_STATUS
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
        6,
        17,
        "Select a file system from the list below.",
        0
    },
    {
        8,
        19,
        "\x07  Press UP or DOWN to select a file system.",
        0
    },
    {
        8,
        21,
        "\x07  Press ENTER to format the partition.",
        0
    },
    {
        8,
        23,
        "\x07  Press ESC to select another partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
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
        6,
        8,
        "You have chosen to delete the partition",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Press D to delete the partition.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "WARNING: All data on this partition will be lost!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ESC to cancel.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Delete Partition   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};


MUI_PAGE svSEPages[] =
{
    {
        LANGUAGE_PAGE,
        LanguagePageEntries
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
        -1,
        NULL
    }
};

#endif
