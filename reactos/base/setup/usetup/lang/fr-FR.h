#ifndef LANG_FR_FR_H__
#define LANG_FR_FR_H__

static MUI_ENTRY frFRWelcomePageEntries[] =
{
    {
        6,
        8,
        "Bienvenue à l'installation de ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Cette partie de l'installation copie le Système d'Exploitation ReactOS",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "sur votre ordinateur et le prépare à la 2e partie de l'installation.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Appuyez sur ENTER pour installer ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Appuyez sur R pour réparer ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Appuyez sur L pour les Termes et Conditions de Licence ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Appuyez sur F3 pour quitter sans installer ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Pour plus d'informations sur ReactOS, veuillez visiter :",
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
        "   ENTER = Continuer  R = Réparer F3 = Quitter",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRIntroPageEntries[] =
{
    {
        4,
        3,
        "Installation de ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "L'Installation de ReactOS est en phase de développement.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Elle ne supporte pas encore toutes les fonctions d'une application",
        TEXT_NORMAL
    },
    {
        6,
        10,
        " d'installation entièrement utilisable.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Les limitations suivantes s'appliquent:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- L'installation ne peut gérer plus d'une partition primaire par disque.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- L'installation ne peut effacer une partition primaire d'un disque",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  tant que des partitions secondaires existent sur ce disque.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- L'installation ne peut effacer la première partition secondaire",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  tant que des autres partitions secondaires existent sur ce disque.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- L'installation supporte uniquement le système de fichiers FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Les vérifications de système de fichers ne sont pas implémentées.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Appuyez sur ENTER pour installer ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Appuyez sur F3 pour quitter sans installer ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuer   F3 = Quitter",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRLicensePageEntries[] =
{
    {
        6,
        6,
        "Licence:",
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
        "   ENTER = Retour",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRDevicePageEntries[] =
{
    {
        6,
        8,
        "La liste ci-dessous montre les réglages matériels actuels.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "     Ordinateur:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "      Affichage:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "        Clavier:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Réglage Clavier:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "       Accepter:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Accepter ces réglages matériels",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Vous pouvez changer les réglages matériels en appuyant sur HAUT ou BAS",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "pour selectionner une entrée.",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "Appuyez sur ENTER pour choisir un autre réglage.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Quand tous les réglages sont corrects, sélectionner \"Accepter",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "ces réglages matériels\" et appuyer sur ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuer   F3 = Quitter",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRRepairPageEntries[] =
{
    {
        6,
        8,
        "L'Installation de ReactOS est en phase de développement.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Elle ne supporte pas encore toutes les fonctions d'une application",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "d'installation entièrement utilisable.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Les fonctions de réparation ne sont pas implémentées pour l'instant.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Appuyez sur U pour mettre à jour l'OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Appuyez sur R pour la Console de Réparation.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Appuyez sur ESC pour retourner à la page principale.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Appuyez sur ENTER pour redémarrer votre ordinateur.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Page principale  ENTER = Redémarrer",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY frFRComputerPageEntries[] =
{
    {
        6,
        8,
        "Vous voulez changer le type d'ordinateur installé.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour sélectionner le type d'ordinateur.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Puis appuyez sur ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyez sur ESC pour revenir à la page précédente sans changer",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   le type d'ordinateur.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuer   ESC = Annuler   F3 = Quitter",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRFlushPageEntries[] =
{
    {
        10,
        6,
        "Le système s'assure quer toutes les données sont écrites sur le disque",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Cela peut prendre une minute",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Quand cela sera fini, votre ordinateur redémarrera automatiquement",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Vidage du cache",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRQuitPageEntries[] =
{
    {
        10,
        6,
        "ReactOS n'est pas complètement installé",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Enlever la disquette du lecteur A: et",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "tous les CDROMs des lecteurs de CD.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Appuyez sur ENTER pour redémarrer votre ordinateur.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Veuillez attendre ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRDisplayPageEntries[] =
{
    {
        6,
        8,
        "Vous voulez changer le type d'écran à installer.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Appuyez sur HAUT ou BAS pour sélectionner le type d'écran.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Appuez sur ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyez sur ESC pour revenir à la page précédente sans changer",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   le type d'écran.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuer   ESC = Annuler   F3 = Quitter",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRSuccessPageEntries[] =
{
    {
        10,
        6,
        "Les composants standards de ReactOS ont été installés avec succès.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Enlever la disquette du lecteur A: et",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "tous les CDROMs des lecteurs de CD.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Appuyez sur ENTER pour redémarrer votre ordinateur.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Redémarrer l'ordinateur",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRBootPageEntries[] =
{
    {
        6,
        8,
        "Setup ne peut installer le chargeur sur le disque dur",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "de votre ordinateur",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Veuillez insérer une disquette formatée dans le lecteur A: et",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "appuyez sur ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Continuer   F3 = Quitter",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

MUI_ERROR frFRErrorEntries[] =
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


MUI_PAGE frFRPages[] =
{
    {
        LANGUAGE_PAGE,
        LanguagePageEntries
    },
    {
       START_PAGE,
       frFRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        frFRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        frFRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        frFRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        frFRRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        frFRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        frFRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        frFRFlushPageEntries
    },
    {
        QUIT_PAGE,
        frFRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        frFRSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        frFRBootPageEntries
    },
    {
        -1,
        NULL
    }
};

#endif
