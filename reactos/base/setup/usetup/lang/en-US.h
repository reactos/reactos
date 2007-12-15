#ifndef LANG_EN_US_H__
#define LANG_EN_US_H__

// do not translate these
static MUI_ENTRY LanguagePageEntries[] =
{
    {
        6,
        8,
        "Select your language:",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue  F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY enUSWelcomePageEntries[] =
{
    {
        6,
        8,
        "Welcome to ReactOS Setup",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "This part of the setup copies the ReactOS Operating System to your",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "computer and prepares the second part of the setup.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Press ENTER to install ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Press R to repair ReactOS.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Press L to view the ReactOS Licensing Terms and Conditions",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_NORMAL
    },
    {
        6, 
        23,
        "For more information on ReactOS, please visit:",
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
        "   ENTER = Continue  R = Repair F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY enUSIntroPageEntries[] =
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
        "ReactOS Setup is in an early development phase. It does not yet",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "support all the functions of a fully usable setup application.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "The following limitations apply:",
        TEXT_NORMAL
    },
    {
        8, 
        13,
        "- Setup can not handle more than one primary partition per disk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Setup can not delete a primary partition from a disk",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  as long as extended partitions exist on this disk.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Setup can not delete the first extended partition from a disk",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  as long as other extended partitions exist on this disk.",
        TEXT_NORMAL
    },
    {
        8, 
        18, 
        "- Setup supports FAT file systems only.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "- File system checks are not implemented yet.",
        TEXT_NORMAL
    },
    {
        8, 
        23, 
        "\x07  Press ENTER to install ReactOS.",
        TEXT_NORMAL
    },
    {
        8, 
        25, 
        "\x07  Press F3 to quit without installing ReactOS.",
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

static MUI_ENTRY enUSLicensePageEntries[] =
{
    {
        6,
        6,
        "Licensing:",
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
        "Warranty:",
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
        "   ENTER = Return",
        TEXT_STATUS
    },
    {           
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY enUSDevicePageEntries[] =
{
    {
        6, 
        8,
        "The list below shows the current device settings.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Computer:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Display:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Keyboard:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Keyboard layout:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         Accept:",
        TEXT_NORMAL
    },
    {
        25, 
        16, "Accept these device settings",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "You can change the hardware settings by pressing the UP or DOWN keys",
        TEXT_NORMAL
    },
    {
        6, 
        20, 
        "to select an entry. Then press the ENTER key to select alternative",
        TEXT_NORMAL
    },
    {
        6, 
        21,
        "settings.",
        TEXT_NORMAL
    },
    {
        6, 
        23, 
        "When all settings are correct, select \"Accept these device settings\"",
        TEXT_NORMAL
    },
    {
        6, 
        24,
        "and press ENTER.",
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

static MUI_ENTRY enUSRepairPageEntries[] =
{
    {
        6, 
        8,
        "ReactOS Setup is in an early development phase. It does not yet",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "support all the functions of a fully usable setup application.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "The repair functions are not implemented yet.",
        TEXT_NORMAL
    },
    {
        8, 
        15,
        "\x07  Press U for Updating OS.",
        TEXT_NORMAL
    },
    {
        8, 
        17,
        "\x07  Press R for the Recovery Console.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Press ESC to return to the main page.",
        TEXT_NORMAL
    },
    {
        8, 
        21,
        "\x07  Press ENTER to reboot your computer.",
        TEXT_NORMAL
    },
    {
        0, 
        0,
        "   ESC = Main page  ENTER = Reboot",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY enUSComputerPageEntries[] =
{
    {
        6,
        8,
        "You want to change the type of computer to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired computer type.",
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
        "   the computer type.",
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

static MUI_ENTRY enUSFlushPageEntries[] =
{
    {
        10,
        6,
        "The system is now making sure all data is stored on your disk",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "This may take a minute",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "When finished, your computer will reboot automatically",
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

static MUI_ENTRY enUSQuitPageEntries[] =
{
    {
        10,
        6,
        "ReactOS is not completely installed",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Remove floppy disk from Drive A: and",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "all CD-ROMs from CD-Drives.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Press ENTER to reboot your computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Please wait ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY enUSDisplayPageEntries[] =
{
    {
        6,
        8,
        "You want to change the type of display to be installed.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Press the UP or DOWN key to select the desired display type.",
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
        "   the display type.",
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

static MUI_ENTRY enUSSuccessPageEntries[] =
{
    {
        10,
        6,
        "The basic components of ReactOS have been installed successfully.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Remove floppy disk from Drive A: and",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "all CD-ROMs from CD-Drive.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Press ENTER to reboot your computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reboot computer",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY enUSBootPageEntries[] =
{
    {
        6,
        8,
        "Setup cannot install the bootloader on your computers",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "hardisk",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Please insert a formatted floppy disk in drive A: and",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "press ENTER.",
        TEXT_NORMAL,
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

MUI_ERROR enUSErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS is not completely installed on your\n"
	     "computer. If you quit Setup now, you will need to\n"
	     "run Setup again to install ReactOS.\n"
	     "\n"
	     "  \x07  Press ENTER to continue Setup.\n"
	     "  \x07  Press F3 to quit Setup.",
	     "F3= Quit  ENTER = Continue"
    },
    {
        //ERROR_NO_HDD
        "Setup could not find a harddisk.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup failed to load the file TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup found a corrupt TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup could not retrieve system drive information.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WRITE_BOOT,
        "failed to install FAT bootcode on the system partition.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup failed to load the computer type list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup failed to load the display settings list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup failed to load the keyboard type list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup failed to load the keyboard layout list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup found that at least one harddisk contains an incompatible\n"
		  "partition table that can not be handled properly!\n"
		  "\n"
		  "Creating or deleting partitions can destroy the partiton table.\n"
		  "\n"
		  "  \x07  Press F3 to quit Setup."
		  "  \x07  Press ENTER to continue.",
          "F3= Quit  ENTER = Continue"
    },
    {
        //ERROR_NEW_PARTITION,
        "You can not create a new Partition inside\n"
		"of an already existing Partition!\n"
		"\n"
		"  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "You can not delete unpartitioned disk space!\n"
        "\n"
        "  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_NO_FLOPPY,
        "No disk in drive A:.",
        "ENTER = Continue"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup failed to update keyboard layout settings.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup could not create install directories.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in the cabinet.\n", 
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup could not create the install directory.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup failed to find the 'SetupData' section\n"
		 "in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup failed to write partition tables.\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE enUSPages[] =
{
    {
        LANGUAGE_PAGE,
        LanguagePageEntries
    },
    {
       START_PAGE,
       enUSWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        enUSIntroPageEntries
    },
    {
        LICENSE_PAGE,
        enUSLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        enUSDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        enUSRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        enUSComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        enUSDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        enUSFlushPageEntries
    },
    {
        QUIT_PAGE,
        enUSQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        enUSSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        enUSBootPageEntries
    },
    {
        -1,
        NULL
    }
};

#endif
