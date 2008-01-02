#ifndef LANG_FR_FR_H__
#define LANG_FR_FR_H__

static MUI_ENTRY frFRLanguagePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Selection de la langue.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Veuillez choisir la langue utilisee pour le processus d'installation",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   et appuyez sur ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Cette langue sera la langue par defaut pour le systeme final.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuer  F3 = Quitter",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRWelcomePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Bienvenue a l'installation de ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Cette partie de l'installation copie le Systeme d'Exploitation ReactOS",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "sur votre ordinateur et le prepare a la 2e partie de l'installation.",
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
        "\x07  Appuyez sur R pour reparer ReactOS.",
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
        "   ENTER = Continuer  R = Reparer F3 = Quitter",
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
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "L'Installation de ReactOS est en phase de developpement.",
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
        " d'installation entierement utilisable.",
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
        "- L'installation ne peut gerer plus d'une partition primaire par disque.",
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
        "- L'installation ne peut effacer la premiere partition secondaire",
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
        "- L'installation supporte uniquement le systeme de fichiers FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Les verifications de systeme de fichers ne sont pas implementees.",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        6,
        "Licence :",
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
        "Garantie :",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "La liste ci-dessous montre les reglages materiels actuels.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "     Ordinateur :",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "      Affichage :",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "        Clavier :",
        TEXT_NORMAL
    },
    {
        4,
        14,
        "Disposition clavier :",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "       Accepter :",
        TEXT_NORMAL
    },
    {
        25,
        16, "Accepter ces reglages materiels",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Vous pouvez changer les reglages materiels en appuyant sur HAUT ou BAS",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "pour selectionner une entree.",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "Appuyez sur ENTER pour choisir un autre reglage.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Quand tous les reglages sont corrects, selectionner \"Accepter",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "ces reglages materiels\" et appuyer sur ENTER.",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "L'Installation de ReactOS est en phase de developpement.",
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
        "d'installation entierement utilisable.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Les fonctions de reparation ne sont pas implementees pour l'instant.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Appuyez sur U pour mettre a jour l'OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Appuyez sur R pour la Console de Reparation.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Appuyez sur ESC pour retourner a la page principale.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Appuyez sur ENTER pour redemarrer votre ordinateur.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Page principale  ENTER = Redemarrer",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vous voulez changer le type d'ordinateur installe.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour selectionner le type d'ordinateur.",
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
        "\x07  Appuyez sur ESC pour revenir a la page precedente sans changer",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Le systeme s'assure que toutes les donnees sont ecrites sur le disque",
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
        "Quand cela sera fini, votre ordinateur redemarrera automatiquement",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS n'est pas completement installe",
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
        "Appuyez sur ENTER pour redemarrer votre ordinateur.",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vous voulez changer le type d'ecran a installer.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Appuyez sur HAUT ou BAS pour selectionner le type d'ecran.",
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
        "\x07  Appuyez sur ESC pour revenir a la page precedente sans changer",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   le type d'ecran.",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Les composants standards de ReactOS ont ete installes avec succes.",
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
        "Appuyez sur ENTER pour redemarrer votre ordinateur.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Redemarrer l'ordinateur",
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
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
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
        "Veuillez inserer une disquette formatee dans le lecteur A: et",
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

static MUI_ENTRY frFRSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "La liste suivante montre les partitions existantes et",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "l'espace disque non utilise pour de nouvelles partitions.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Appuyez sur HAUT ou BAS pour selectionner une entree de la liste.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyez sur ENTER pour installer ReactOS sur la partition choisie.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Appuyez sur C pour creer une nouvelle partition.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Appuyez sur D pour effacer une partition.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Patientez...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Formater la partition",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Setup va formater la partition. Appuyez sur ENTER pour continuer.",
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
        TEXT_NORMAL
    }
};

static MUI_ENTRY frFRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Setup installe les fichiers de ReactOS sur la partition selectionnee.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Choisissez un repertoire ou vous voulez que ReactOS soit installe :",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "Pour changer le repertoire propose, appuyez sur BACKSPACE pour effacer",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "des caracteres et ensuite tapez le repertoire ou vous voulez que",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "ReactOS soit installe",
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

static MUI_ENTRY frFRFileCopyEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "Patientez pendant que ReactOS Setup copie les fichiers",
        TEXT_NORMAL
    },
    {
        15,
        13,
        "dans le repertoire d'installation de ReactOS.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "Cela peut prendre plusieurs minutes.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Patientez...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRBootLoaderEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Setup installe le chargeur de demarrage",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Installer le chargeur de demarrage sur le disque dur (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Installer le chargeur de demarrage sur une disquette.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Ne pas installer le chargeur de demarrage.",
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

static MUI_ENTRY frFRKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vous voulez changer le type de clavier a installer.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour selectionner le type de clavier,",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   puis appuyez sur ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyez sur ESC pour revenir a la page precedente sans changer",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   le type de clavier.",
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

static MUI_ENTRY frFRLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vous voulez changer la disposition du clavier.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour selectionner la disposition",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    choisie. Puis appuyez sur ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyez sur ESC pour revenir a la page precedente sans changer",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   la disposition du clavier.",
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
    },

};

static MUI_ENTRY frFRPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Setup prepare votre ordinateur pour copier les fichiers de ReactOS. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Prepare la liste de fichiers a copier...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY frFRSelectFSEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        17,
        "Selectionnez un systeme de fichiers dans la liste suivante.",
        0
    },
    {
        8,
        19,
        "\x07  Appuyez sur HAUT ou BAS pour selectionner un systeme de fichiers.",
        0
    },
    {
        8,
        21,
        "\x07  Appuyez sur ENTER pour formater la partition.",
        0
    },
    {
        8,
        23,
        "\x07  Appuyez sur ESC pour selectionner une autre partition.",
        0
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

static MUI_ENTRY frFRDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vous avez choisi de supprimer la partition",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Appuyez sur D pour supprimer la partition.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "ATTENTION: Toutes les donnees de cette partition seront perdues!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Appuyez sur ESC pour annuler.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Supprimer la Partition   ESC = Annuler   F3 = Quitter",
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
        "ReactOS n'est pas completement installe sur votre\n"
	     "ordinateur. Si vous quittez Setup maintenant, vous devrez\n"
	     "lancer Setup de nouveau pour installer ReactOS.\n"
	     "\n"
	     "  \x07  Appuyer sur ENTER pour continuer Setup.\n"
	     "  \x07  Appuyer sur F3 pour quitter Setup.",
	     "F3= Quitter  ENTER = Continuer"
    },
    {
        //ERROR_NO_HDD
        "Setup n'a pu trouver un disque dur.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup n'a pu trouver son lecteur source.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup n'a pas reussi a charger le fichier TXTSETUP.SIF.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup a trouve un fichier TXTSETUP.SIF corrompu.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup a trouve une signature invalide dans TXTSETUP.SIF.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup n'a pu recuperer les informations du disque système.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_WRITE_BOOT,
        "Echec de l'installation du code de démarrage FAT sur la partition systeme.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup n'a pu charger la liste de type d'ordinateurs.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup n'a pu charger la liste de réglages des écrans.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup n'a pu charger la liste de types de claviers.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup n'a pu charger la liste de dispositions de claviers.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup a detecte qu'au moins un dique dur contient une table\n"
		  "de partition incompatible qui ne peut etre prise en compte!\n"
		  "\n"
		  "Creer ou effacer des partitions peut detruire la table de partition.\n"
		  "\n"
		  "  \x07  Appuyer sur F3 pour quitter Setup."
		  "  \x07  Appuyer sur ENTER pour continuer Setup.",
          "F3= Quitter  ENTER = Continuer"
    },
    {
        //ERROR_NEW_PARTITION,
        "Vous ne pouvez creer une nouvelle Partition à l'interieur\n"
		"d'une Partition deja existante!\n"
		"\n"
		"  * Appuyez sur une touche pour continuer.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Vous ne pouvez supprimer de l'espace disque non partitionne!\n"
        "\n"
        "  * Appuyez sur une touche pour continuer.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Echec de l'installation du code de démarrage FAT sur la partition systeme.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_NO_FLOPPY,
        "Pas de disque dans le lecteur A:.",
        "ENTER = Continuer"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup n'a pu mettre a jour les reglages de disposition du clavier.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup n'a pu mettre a jour les reglages de l'ecran.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup n'a pu importer un fichier ruche.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup n'a pu trouver les fichiers de la base de registres.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup n'a pu creer les ruches de la base de registres.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup n'a pu initialiser la base de registres.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Le Cabinet n'a pas de fichier inf valide.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet non trouve.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet n'a pas de script de setup.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup n'a pu creer les repertoires d'installation.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup n'a pu trouver la section 'Directories'\n"
        "dans TXTSETUP.SIF.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup n'a pu trouver la section 'Directories\n"
        "dans le cabinet.\n",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup n'a pu creer le repertoire d'installation.",
        "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup n'a pu trouver la section 'SetupData'\n"
		 "in TXTSETUP.SIF.\n",
		 "ENTER = Redemarrer l'ordinateur"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup n'a pu ecrire les tables de partition.\n"
        "ENTER = Redemarrer l'ordinateur"
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
        frFRLanguagePageEntries
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
        SELECT_PARTITION_PAGE,
        frFRSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        frFRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        frFRFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        frFRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        frFRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        frFRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        frFRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        frFRKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        frFRBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        frFRLayoutSettingsEntries
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
