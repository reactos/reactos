#pragma once

MUI_LAYOUTS frFRLayouts[] =
{
    { L"040C", L"0000040C" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY frFRLanguagePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "SÇlection de la langue.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Veuiller choisir la langue utilisÇe pour le processus d'installation",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   et appuyer sur ENTRêE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Cette langue sera la langue par dÇfaut pour le systäme final.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer  F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Bienvenue dans l'installation de ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Cette partie de l'installation copie le Systäme d'Exploitation ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "sur votre ordinateur et le prÇpare Ö la 2e partie de l'installation.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Appuyer sur ENTRêE pour installer ReactOS.",
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
        "\x07  Appuyer sur L pour les Termes et Conditions de Licence ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Appuyer sur F3 pour quitter sans installer ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Pour plus d'informations sur ReactOS, veuiller visiter :",
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
        "ENTRêE = Continuer  R = RÇparer  L = Licence  F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "L'installation de ReactOS est en phase de dÇveloppement.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Elle ne supporte pas encore toutes les fonctions d'une application",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        " d'installation entiärement utilisable.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Les limitations suivantes s'appliquent:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- L'installation supporte uniquement le systäme de fichiers FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Les vÇrifications de systäme de fichers ne sont pas implÇmentÇes.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Appuyer sur ENTRêE pour installer ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Appuyer sur F3 pour quitter sans installer ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licence :",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "the original license.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "If for some reason you did not receive a copy of the",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
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
        "Garantie :",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
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
        "ENTRêE = Retour",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "La liste ci-dessous montre les rÇglages matÇriels actuels.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "     Ordinateur :",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "      Affichage :",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "        Clavier :",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "RÇglage Clavier :",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "       Accepter :",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "Accepter ces rÇglages matÇriels",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Vous pouvez changer les rÇglages matÇriels en appuyant sur HAUT ou BAS",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "pour sÇlectionner une entrÇe.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "Appuyer sur ENTRêE pour choisir un autre rÇglage.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Quand tous les rÇglages sont corrects, sÇlectionner \"Accepter",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "ces rÇglages matÇriels\" et appuyer sur ENTRêE.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRUpgradePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY frFRComputerPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vous voulez changer le type d'ordinateur installÇ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Appuyer sur HAUT ou BAS pour sÇlectionner le type d'ordinateur.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Puis appuyer sur ENTRêE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyer sur êCHAP pour revenir Ö la page prÇcÇdente sans changer",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   le type d'ordinateur.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   êCHAP = Annuler   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Le systäme s'assure que toutes les donnÇes sont Çcrites sur le disque",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Cela peut prendre une minute",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Quand cela sera fini, votre ordinateur redÇmarrera automatiquement",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Vidage du cache",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS n'est pas complätement installÇ",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Enlever la disquette du lecteur A: et",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "tous les CDROMs des lecteurs de CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Appuyer sur ENTRêE pour redÇmarrer votre ordinateur.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Veuillez attendre ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vous voulez changer le type d'Çcran Ö installer.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Appuyer sur HAUT ou BAS pour sÇlectionner le type d'Çcran.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Appuyer sur ENTRêE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyer sur êCHAP pour revenir Ö la page prÇcÇdente sans changer",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   le type d'Çcran.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   êCHAP = Annuler   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Les composants standards de ReactOS ont ÇtÇ installÇs avec succäs.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Enlever la disquette du lecteur A: et",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "tous les CDROMs des lecteurs de CD.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Appuyer sur ENTRêE pour redÇmarrer votre ordinateur.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = RedÇmarrer l'ordinateur",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup ne peut installer le chargeur sur le disque dur",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "de votre ordinateur",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Veuillez insÇrer une disquette formatÇe dans le lecteur A: et",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "appuyer sur ENTRêE.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTRêE = Continuer   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "La liste suivante montre les partitions existantes et",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "l'espace disque non utilisÇ pour de nouvelles partitions.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Appuyer sur HAUT ou BAS pour sÇlectionner une entrÇe de la liste.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyer sur ENTRêE pour installer ReactOS sur la partition choisie.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Appuyer sur P pour crÇer une partition primaire.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Appuyer sur E pour crÇer une partition Çtendue.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Appuyer sur L pour crÇer une partition logique.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Appuyer sur D pour effacer une partition.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Patienter...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vous avez choisi de supprimer la partition systäme.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "La partition systäme peut contenir des programmes de diagnostic, de",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "configuration du matÇriel, des programmes pour dÇmarrer un systäme",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "d'exploitation (comme ReactOS) ou d'autres programmes fournis par le",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "constructeur du matÇriel.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "Ne supprimez la partition systäme que si vous àtes sñr qu'il n'y a aucun",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "programme dans la partition, ou bien si vous souhaitez les supprimer.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "Lorsque vous supprimez la partition systäme, vous ne pourrez peut-àtre",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        18,
        "plus dÇmarrer votre ordinateur depuis le disque dur jusqu'Ö ce que vous",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "finissiez l'installation de ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "\x07  Appuyer sur ENTRêE pour supprimer la partition systäme. Il vous sera",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "   demandÇ de confirmer la suppression de la partition plus tard.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "\x07  Appuyer sur êCHAP pour retourner Ö la page principale. La partition",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        27,
        "   ne sera pas supprimÇe.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer  êCHAP = Annuler",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formater la partition",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Setup va formater la partition. Appuyer sur ENTRêE pour continuer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY frFRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup installe les fichiers de ReactOS sur la partition sÇlectionnÇe.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Choisissez un repertoire oó vous voulez que ReactOS soit installÇ :",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Pour changer le rÇpertoire proposÇ, appuyez sur BACKSPACE pour effacer",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "des caractäres et ensuite tapez le rÇpertoire ou vous voulez que",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "ReactOS soit installÇ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Patientez pendant que ReactOS Setup copie les fichiers",
        TEXT_STYLE_NORMAL
    },
    {
        15,
        13,
        "dans le rÇpertoire d'installation de ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "Cela peut prendre plusieurs minutes.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Patientez...    ",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup installe le chargeur de dÇmarrage",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Installer le chargeur de dÇmarrage sur le disque dur (MBR et VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Installer le chargeur de dÇmarrage sur le disque dur (VBR seulement).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Installer le chargeur de dÇmarrage sur une disquette.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Ne pas installer le chargeur de dÇmarrage.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vous voulez changer le type de clavier Ö installer.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour sÇlectionner le type de clavier,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   puis appuyez sur ENTRêE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyez sur êCHAP pour revenir Ö la page prÇcÇdente sans changer",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   le type de clavier.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   êCHAP = Annuler   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Veuillez sÇlectionner une disposition Ö installer par dÇfaut.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour sÇlectionner la disposition",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    choisie. Puis appuyez sur ENTRêE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Appuyez sur êCHAP pour revenir Ö la page prÇcÇdente sans changer",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   la disposition du clavier.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTRêE = Continuer   êCHAP = Annuler   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup prÇpare votre ordinateur pour copier les fichiers de ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "PrÇpare la liste de fichiers Ö copier...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "SÇlectionnez un systäme de fichiers dans la liste suivante.",
        0
    },
    {
        8,
        19,
        "\x07  Appuyez sur HAUT ou BAS pour sÇlectionner un systäme de fichiers.",
        0
    },
    {
        8,
        21,
        "\x07  Appuyez sur ENTRêE pour formater la partition.",
        0
    },
    {
        8,
        23,
        "\x07  Appuyez sur êCHAP pour sÇlectionner une autre partition.",
        0
    },
    {
        0,
        0,
        "ENTRêE = Continuer   êCHAP = Annuler   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Vous avez choisi de supprimer la partition",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Appuyez sur D pour supprimer la partition.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ATTENTION: Toutes les donneÇes de cette partition seront perdues!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Appuyez sur êCHAP pour annuler.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Supprimer la Partition   êCHAP = Annuler   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY frFRRegistryEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup met Ö jour la configuration du systäme. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "En train de crÇer la base de registres...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR frFRErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Succäs\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS n'est pas complätement installÇ sur votre\n"
        "ordinateur. Si vous quittez Setup maintenant, vous devrez\n"
        "lancer Setup de nouveau pour installer ReactOS.\n"
        "\n"
        "  \x07  Appuyer sur ENTRêE pour continuer Setup.\n"
        "  \x07  Appuyer sur F3 pour quitter Setup.",
        "F3 = Quitter  ENTRêE = Continuer"
    },
    {
        //ERROR_NO_HDD
        "Setup n'a pu trouver un disque dur.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup n'a pu trouver son lecteur source.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup n'a pas rÇussi Ö charger le fichier TXTSETUP.SIF.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup a trouvÇ un fichier TXTSETUP.SIF corrompu.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup a trouvÇ une signature invalide dans TXTSETUP.SIF.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup n'a pu rÇcupÇrer les informations du disque systäme.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_WRITE_BOOT,
        "Echec de l'installation du code de dÇmarrage FAT sur la partition systäme.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup n'a pu charger la liste de type d'ordinateurs.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup n'a pu charger la liste de rÇglages des Çcrans.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup n'a pu charger la liste de types de claviers.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup n'a pu charger la liste de dispositions de claviers.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_WARN_PARTITION,
        "Setup a dÇtectÇ qu'au moins un disque dur contient une table\n"
        "de partition incompatible qui ne peut àtre prise en compte!\n"
        "\n"
        "CrÇer ou effacer des partitions peut dÇtruire la table de partition.\n"
        "\n"
        "  \x07  Appuyer sur F3 pour quitter Setup.\n"
        "  \x07  Appuyer sur ENTRêE pour continuer Setup.",
        "F3 = Quitter  ENTRêE = Continuer"
    },
    {
        //ERROR_NEW_PARTITION,
        "Vous ne pouvez crÇer une nouvelle Partition Ö l'intÇrieur\n"
        "d'une Partition dÇjÖ existante!\n"
        "\n"
        "  * Appuyer sur une touche pour continuer.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Vous ne pouvez supprimer de l'espace disque non partitionnÇ!\n"
        "\n"
        "  * Appuyer sur une touche pour continuer.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Echec de l'installation du code de dÇmarrage FAT sur la partition systäme.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_NO_FLOPPY,
        "Pas de disque dans le lecteur A:.",
        "ENTRêE = Continuer"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup n'a pu mettre Ö jour les rÇglages de disposition du clavier.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup n'a pu mettre Ö jour les rÇglages de l'Çcran.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup n'a pu importer un fichier ruche.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup n'a pu trouver les fichiers de la base de registres.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup n'a pu crÇer les ruches de la base de registres.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup n'a pu initialiser la base de registres.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Le Cabinet n'a pas de fichier inf valide.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet non trouvÇ.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet n'a pas de script de setup.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup n'a pu ouvrir la file d'attente de copie de fichiers.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup n'a pu crÇer les rÇpertoires d'installation.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup n'a pu trouver la section 'Directories'\n"
        "dans TXTSETUP.SIF.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup n'a pu trouver la section 'Directories\n"
        "dans le cabinet.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup n'a pu crÇer le rÇpertoire d'installation.",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup n'a pu trouver la section 'SetupData'\n"
        "dans TXTSETUP.SIF.\n",
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup n'a pu Çcrire les tables de partition.\n"
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup n'a pu ajouter la page de codes Ö la base de registres.\n"
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup n'a pu changer la langue systäme.\n"
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Setup n'a pas pu ajouter les dispositions de clavier au registre.\n"
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Setup n'a pas pu dÇfinir la geo id.\n"
        "ENTRêE = RedÇmarrer l'ordinateur"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Nom de rÇpertoire invalide.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Impossible de crÇer une nouvelle partition primaire ou Çtendue\n"
        "sur ce disque parce que sa table de partition est pleine.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Impossible de crÇer plus d'une partition Çtendue par disque.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer."
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

MUI_PAGE frFRPages[] =
{
    {
        LANGUAGE_PAGE,
        frFRLanguagePageEntries
    },
    {
        WELCOME_PAGE,
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
        UPGRADE_REPAIR_PAGE,
        frFRUpgradePageEntries
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
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        frFRConfirmDeleteSystemPartitionEntries
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
        REGISTRY_PAGE,
        frFRRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING frFRStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Veuillez patienter..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTRêE = Installer   P/E = CrÇer Partition Primaire/êtendue   F3 = Quitter"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTRêE = Installer   L = CrÇer Partition Logique   F3 = Quitter"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTRêE = Installer   D = Supprimer Partition   F3 = Quitter"},
    {STRING_DELETEPARTITION,
     "   D = Supprimer Partition   F3 = Quitter"},
    {STRING_PARTITIONSIZE,
     "Taille de la nouvelle partition :"},
    {STRING_CHOOSENEWPARTITION,
     "Vous avez choisi de crÇer une partition primaire sur"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Vous avez choisi de crÇer une partition Çtendue sur"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Vous avez choisi de crÇer une partition logique sur"},
    {STRING_HDDSIZE,
    "Veuillez entrer la taille de la nouvelle partition en mÇgaoctets."},
    {STRING_CREATEPARTITION,
     "   ENTRêE = CrÇer Partition   êCHAP = Annuler   F3 = Quitter"},
    {STRING_PARTFORMAT,
    "Cette Partition sera ensuite formatÇe."},
    {STRING_NONFORMATTEDPART,
    "Vous avez choisi d'installer ReactOS sur une nouvelle partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "The system partition is not formatted yet."},
    {STRING_NONFORMATTEDOTHERPART,
    "The new partition is not formatted yet."},
    {STRING_INSTALLONPART,
    "Setup installe ReactOS sur la partition"},
    {STRING_CHECKINGPART,
    "Setup vÇrifie la partition sÇlectionnÇe."},
    {STRING_CONTINUE,
    "ENTRêE = Continuer"},
    {STRING_QUITCONTINUE,
    "F3 = Quitter  ENTRêE = Continuer"},
    {STRING_REBOOTCOMPUTER,
    "ENTRêE = RedÇmarrer l'ordinateur"},
    {STRING_TXTSETUPFAILED,
    "Setup n'a pu trouver la section '%S'\ndans TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Copie du fichier: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup copie les fichiers..."},
    {STRING_REGHIVEUPDATE,
    "   Mise Ö jour de la base de registre..."},
    {STRING_IMPORTFILE,
    "   Importe %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Mise Ö jour des paramätres du registre pour l'Çcran..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Mise Ö jour des paramätres rÇgionaux..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Mise Ö jour des paramätres de la dispoition clavier..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Ajout des informations de pages de codes Ö la base de registres..."},
    {STRING_DONE,
    "   TerminÇ..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTRêE = RedÇmarrer l'ordinateur"},
    {STRING_CONSOLEFAIL1,
    "Impossible d'ouvrir la console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "La cause probable est l'utilisation d'un clavier USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Les claviers USB ne sont pas complätement supportÇs actuellement\r\n"},
    {STRING_FORMATTINGDISK,
    "Setup formate votre disque"},
    {STRING_CHECKINGDISK,
    "Setup vÇrifie votre disque"},
    {STRING_FORMATDISK1,
    " Formater la partition comme systäme de fichiers %S (formatage rapide) "},
    {STRING_FORMATDISK2,
    " Formater la partition comme systäme de fichiers %S "},
    {STRING_KEEPFORMAT,
    " Garder le systäme de fichiers courant (pas de changements) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Disque dur %lu  (Port=%hu, Bus=%hu, Id=%hu) sur %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Disque dur %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "sur %I64u %s  Disque dur %lu  (Port=%hu, Bus=%hu, Id=%hu) sur %wZ."},
    {STRING_HDDINFOUNK3,
    "sur %I64u %s  Disque dur %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Disque dur %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "sur Disque dur %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sType %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Disque dur %lu  (Port=%hu, Bus=%hu, Id=%hu) sur %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Disque dur %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup a crÇÇ une nouvelle partition sur"},
    {STRING_UNPSPACE,
    "    %sEspace non partitionnÇ%s            %6lu %s"},
    {STRING_MAXSIZE,
    "Mo (max. %lu Mo)"},
    {STRING_EXTENDED_PARTITION,
    "Partition êtendue"},
    {STRING_UNFORMATTED,
    "Nouveau (non formatÇ)"},
    {STRING_FORMATUNUSED,
    "Non utilisÇ"},
    {STRING_FORMATUNKNOWN,
    "Inconnu"},
    {STRING_KB,
    "Ko"},
    {STRING_MB,
    "Mo"},
    {STRING_GB,
    "Go"},
    {STRING_ADDKBLAYOUTS,
    "Ajout des dispositions clavier"},
    {0, 0}
};
