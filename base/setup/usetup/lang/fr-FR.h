// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY frFRSetupInitPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Veuillez patienter pendant que le programme d'installation de ReactOS",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "s'initialise et d\202couvre vos p\202riph\202riques...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Veuillez patienter...",
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

static MUI_ENTRY frFRLanguagePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "S\202lection de la langue.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Veuiller choisir la langue utilis\202e pour le processus d'installation",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   et appuyer sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Cette langue sera la langue par d\202faut pour le syst\212me final.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer  F3 = Quitter",
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

static MUI_ENTRY frFRWelcomePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Bienvenue dans l'installation de ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Cette partie de l'installation copie le Syst\212me d'Exploitation ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "sur votre ordinateur et le pr\202pare \205 la 2\212me partie de l'installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Appuyer sur ENTR\220E pour installer ou mettre \205 jour ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Appuyer sur R pour r\202parer ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Appuyer sur L pour les Termes et Conditions de Licence ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Appuyer sur F3 pour quitter sans installer ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Pour plus d'informations sur ReactOS, veuiller visiter :",
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
        "ENTR\220E = Continuer  R = R\202parer  L = Licence  F3 = Quitter",
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

static MUI_ENTRY frFRIntroPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Statut de version de ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS est en version Alpha, ce qui signifie qu'il n'est pas complet",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "et est en cours de d\202veloppement. Il est recommand\202 de ne l'utiliser",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "que pour \202valuation et test, pas pour un usage quotidien.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Sauvegardez vos donn\202es ou testez sur un ordinateur secondaire",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "si vous souhaitez utiliser ReactOS sur du mat\202riel.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Appuyez sur ENTR\220E pour continuer l'installation de ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Appuyez sur F3 pour quitter sans installer ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   F3 = Quitter",
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

static MUI_ENTRY frFRLicensePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licence :",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "Le syst\212me ReactOS est distribu\202 sous les termes de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "la licence GNU GPL avec des parties contenant du code sous",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "d'autres licences compatibles telles que X11, BSD et GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Tout le logiciel constituant le syst\212me ReactOS est",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "par cons\202quent distribu\202 sous licence GNU GPL tout en",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "maintenant la licence d'origine.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Ce logiciel vient SANS GARANTIE ou restriction d'usage",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "sous les lois locales ou internationales applicables.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "La licence de ReactOS ne couvre que la distribution",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "aux tierces parties.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "Si vous n'avez pas obtenu une copie de la",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "GNU General Public License avec ReactOS veuillez visiter",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        23,
        "Garantie :",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "Ce logiciel est gratuit. Voir les sources pour les",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "conditions de copie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "Il n'y a AUCUNE garantie; pas m\210me sur la QUALIT\220 ou",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "l'AD\220QUATION POUR UN USAGE EN PARTICULIER",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Retour",
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

static MUI_ENTRY frFRDevicePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "La liste ci-dessous montre les r\202glages mat\202riels actuels.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "     Ordinateur :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "      Affichage :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "        Clavier :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "R\202glage Clavier :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "       Accepter :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Accepter ces r\202glages mat\202riels",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Vous pouvez changer les r\202glages mat\202riels en appuyant sur HAUT ou BAS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "pour s\202lectionner une entr\202e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Appuyer sur ENTR\220E pour choisir un autre r\202glage.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Quand tous les r\202glages sont corrects, s\202lectionner \"Accepter",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "ces r\202glages mat\202riels\" et appuyer sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   F3 = Quitter",
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

static MUI_ENTRY frFRRepairPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "L'installation de ReactOS est en phase de d\202veloppement.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Elle ne supporte pas encore toutes les fonctions d'une application",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "d'installation enti\212rement utilisable.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Les fonctions de r\202paration ne sont pas impl\202ment\202es pour l'instant.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Appuyer sur U pour mettre \205 jour l'OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Appuyer sur R pour la Console de R\202paration.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Appuyer sur \220CHAP pour revenir \205 la page principale.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Appuyer sur ENTR\220E pour red\202marrer votre ordinateur.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "\220CHAP = Page principale  ENTR\220E = Red\202marrer",
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

static MUI_ENTRY frFRUpgradePageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Le programme d'installation de ReactOS peut mettre \205 jour l'une des",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "installations de ReactOS list\202es ci-dessous, ou bien, si l'une d'elles",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "est endommag\202e, le programme d'installation peut essayer de la r\202parer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Les fonctions de r\202paration ne sont pas toutes impl\202ment\202es.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Appuyer sur HAUT ou BAS pour s\202lectionner une installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Appuyer sur U pour mettre \205 jour l'installation s\202lectionn\202e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Appuyer sur \220CHAP pour effectuer une nouvelle installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Appuyer sur F3 pour quitter sans installer ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Mettre \205 jour   \220CHAP = Annuler   F3 = Quitter",
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

static MUI_ENTRY frFRComputerPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Vous voulez changer le type d'ordinateur install\202.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Appuyer sur HAUT ou BAS pour s\202lectionner le type d'ordinateur.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Puis appuyer sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Appuyer sur \220CHAP pour revenir \205 la page pr\202c\202dente sans changer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   le type d'ordinateur.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   \220CHAP = Annuler   F3 = Quitter",
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

static MUI_ENTRY frFRFlushPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Le syst\212me s'assure que toutes les donn\202es sont \202crites sur le disque.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Cela peut prendre une minute.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Quand cela sera fini, votre ordinateur red\202marrera automatiquement.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Vidage du cache",
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

static MUI_ENTRY frFRQuitPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS n'est pas compl\212tement install\202.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Enlever la disquette du lecteur A: et",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "tous les CDROMs des lecteurs de CD.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Appuyer sur ENTR\220E pour red\202marrer votre ordinateur.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Veuillez patienter...",
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

static MUI_ENTRY frFRDisplayPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Vous voulez changer le type d'\202cran \205 installer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Appuyer sur HAUT ou BAS pour s\202lectionner le type d'\202cran.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Appuyer sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Appuyer sur \220CHAP pour revenir \205 la page pr\202c\202dente sans changer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   le type d'\202cran.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   \220CHAP = Annuler   F3 = Quitter",
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

static MUI_ENTRY frFRSuccessPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Les composants standards de ReactOS ont \202t\202 install\202s avec succ\212s.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Enlever la disquette du lecteur A: et",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "tous les CDROMs des lecteurs de CD.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Appuyer sur ENTR\220E pour red\202marrer votre ordinateur.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Red\202marrer l'ordinateur",
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

static MUI_ENTRY frFRSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "La liste suivante montre les partitions existantes et",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "l'espace disque non utilis\202 pour de nouvelles partitions.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Appuyer sur HAUT ou BAS pour s\202lectionner une entr\202e de la liste.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Appuyer sur ENTR\220E pour installer ReactOS sur la partition choisie.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Appuyer sur C pour cr\202er une partition primaire/logique.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Appuyer sur E pour cr\202er une partition \202tendue.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Appuyer sur D pour effacer une partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Patienter...",
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

static MUI_ENTRY frFRChangeSystemPartition[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "La partition syst\212me actuelle de votre ordinateur",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "sur le disque syst\212me",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "emploie un format qui n'est pas support\202 par ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "Afin de pouvoir installer ReactOS avec succ\212s, ReactOS Setup doit changer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "la partition syst\212me actuelle par une nouvelle.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "La nouvelle partition syst\212me candidate est :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Pour accepter ce choix, appuyez sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Pour changer manuellement la partition syst\212me, appuyez sur \220CHAP",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   afin de revenir dans la liste des partitions, puis s\202lectionnez ou",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   cr\202ez une nouvelle partition syst\212me sur le disque syst\212me.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "S'il existe d'autres syst\212mes d'exploitation qui d\202pendent de la partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "syst\212me d'origine, vous devrez peut-\210tre les reconfigurer pour la nouvelle",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "partition syst\212me, ou bien changer la partition syst\212me vers celle",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "d'origine apr\212s la fin de l'installation de ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer  \220CHAP = Annuler",
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

static MUI_ENTRY frFRConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Vous avez choisi de supprimer la partition syst\212me.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "La partition syst\212me peut contenir des programmes de diagnostic, de",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "configuration du mat\202riel, des programmes pour d\202marrer un syst\212me",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "d'exploitation (comme ReactOS) ou d'autres programmes fournis par le",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "constructeur du mat\202riel.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Ne supprimez la partition syst\212me que si vous \210tes s\226r qu'il n'y a aucun",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "programme dans la partition, ou bien si vous souhaitez les supprimer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Lorsque vous supprimez la partition syst\212me, vous ne pourrez peut-\210tre",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "plus d\202marrer votre ordinateur depuis le disque dur jusqu'\205 ce que vous",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "finissiez l'installation de ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "\x07  Appuyer sur ENTR\220E pour supprimer la partition syst\212me. Il vous sera",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        23,
        "   demand\202 de confirmer la suppression de la partition plus tard.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "\x07  Appuyer sur \220CHAP pour revenir \205 la page principale. La partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "   ne sera pas supprim\202e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer  \220CHAP = Annuler",
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

static MUI_ENTRY frFRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Formater la partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Setup va formater la partition. Appuyer sur ENTR\220E pour continuer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   F3 = Quitter",
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

static MUI_ENTRY frFRCheckFSEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup v\202rifie la partition s\202lectionn\202e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Veuillez patienter...",
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

static MUI_ENTRY frFRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup installe les fichiers de ReactOS sur la partition s\202lectionn\202e.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Choisissez un r\202pertoire o\227 vous voulez que ReactOS soit install\202 :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Pour changer le r\202pertoire propos\202, appuyez sur BACKSPACE pour effacer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "des caract\212res et ensuite tapez le r\202pertoire ou vous voulez que",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "ReactOS soit install\202",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   F3 = Quitter",
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

static MUI_ENTRY frFRFileCopyEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        11,
        12,
        "Patientez pendant que ReactOS Setup copie les fichiers",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        15,
        13,
        "dans le r\202pertoire d'installation de ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        20,
        14,
        "Cela peut prendre plusieurs minutes.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "                                                           \xB3 Patientez...    ",
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

static MUI_ENTRY frFRBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Veuillez choisir o\227 Setup doit installer le chargeur de d\202marrage :",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Installer le chargeur de d\202marrage sur le disque dur (MBR et VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Installer le chargeur de d\202marrage sur le disque dur (VBR seulement).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Installer le chargeur de d\202marrage sur une disquette.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Ne pas installer le chargeur de d\202marrage.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   F3 = Quitter",
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

static MUI_ENTRY frFRBootLoaderInstallPageEntries[] =
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
        "Setup installe le chargeur de d\202marrage.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Installation du chargeur de d\202marrage, veuillez patienter...",
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

static MUI_ENTRY frFRBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup installe le chargeur de d\202marrage.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Veuillez ins\202rer une disquette format\202e dans le lecteur A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "et appuyer sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   F3 = Quitter",
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

static MUI_ENTRY frFRKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Vous voulez changer le type de clavier \205 installer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour s\202lectionner le type de clavier,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   puis appuyez sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Appuyez sur \220CHAP pour revenir \205 la page pr\202c\202dente sans changer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   le type de clavier.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   \220CHAP = Annuler   F3 = Quitter",
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

static MUI_ENTRY frFRLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Veuillez s\202lectionner une disposition \205 installer par d\202faut.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Appuyez sur HAUT ou BAS pour s\202lectionner la disposition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    choisie. Puis appuyez sur ENTR\220E.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Appuyez sur \220CHAP pour revenir \205 la page pr\202c\202dente sans changer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   la disposition du clavier.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   \220CHAP = Annuler   F3 = Quitter",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
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
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup pr\202pare votre ordinateur pour copier les fichiers de ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Pr\202pare la liste de fichiers \205 copier...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
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
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "S\202lectionnez un syst\212me de fichiers dans la liste suivante.",
        0
    },
    {
        8,
        19,
        "\x07  Appuyez sur HAUT ou BAS pour s\202lectionner un syst\212me de fichiers.",
        0
    },
    {
        8,
        21,
        "\x07  Appuyez sur ENTR\220E pour formater la partition.",
        0
    },
    {
        8,
        23,
        "\x07  Appuyez sur \220CHAP pour s\202lectionner une autre partition.",
        0
    },
    {
        0,
        0,
        "ENTR\220E = Continuer   \220CHAP = Annuler   F3 = Quitter",
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

static MUI_ENTRY frFRDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Vous avez choisi de supprimer la partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Appuyez sur L pour supprimer la partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "ATTENTION: Toutes les donne\202es de cette partition seront perdues!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Appuyez sur \220CHAP pour annuler.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Supprimer la partition   \220CHAP = Annuler   F3 = Quitter",
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

static MUI_ENTRY frFRRegistryEntries[] =
{
    {
        4,
        3,
        " Installation de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup met \205 jour la configuration du syst\212me.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "En train de cr\202er la base de registres...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
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
        "Succ\212s\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS n'est pas compl\212tement install\202 sur votre\n"
        "ordinateur. Si vous quittez Setup maintenant, vous devrez\n"
        "lancer Setup de nouveau pour installer ReactOS.\n"
        "\n"
        "  \x07  Appuyer sur ENTR\220E pour continuer Setup.\n"
        "  \x07  Appuyer sur F3 pour quitter Setup.",
        "F3 = Quitter  ENTR\220E = Continuer"
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
        "Setup n'a pu trouver un disque dur.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Setup n'a pu trouver son lecteur source.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Setup n'a pas r\202ussi \205 charger le fichier TXTSETUP.SIF.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Setup a trouv\202 un fichier TXTSETUP.SIF corrompu.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup a trouv\202 une signature invalide dans TXTSETUP.SIF.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Setup n'a pu r\202cup\202rer les informations du disque syst\212me.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_WRITE_BOOT,
        "Echec de l'installation du code de d\202marrage %S sur la partition syst\212me.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Setup n'a pu charger la liste de type d'ordinateurs.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Setup n'a pu charger la liste de r\202glages des \202crans.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Setup n'a pu charger la liste de types de claviers.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Setup n'a pu charger la liste de dispositions de claviers.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_WARN_PARTITION,
        "Setup a d\202tect\202 qu'au moins un disque dur contient une table\n"
        "de partition incompatible qui ne peut \210tre prise en compte!\n"
        "\n"
        "Cr\202er ou effacer des partitions peut d\202truire la table de partition.\n"
        "\n"
        "  \x07  Appuyer sur F3 pour quitter Setup.\n"
        "  \x07  Appuyer sur ENTR\220E pour continuer Setup.",
        "F3 = Quitter  ENTR\220E = Continuer"
    },
    {
        // ERROR_NEW_PARTITION,
        "Vous ne pouvez cr\202er une nouvelle partition \205 l'int\202rieur\n"
        "d'une partition d\202j\205 existante!\n"
        "\n"
        "  * Appuyer sur une touche pour continuer.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Echec de l'installation du code de d\202marrage %S sur la partition syst\212me.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_NO_FLOPPY,
        "Pas de disque dans le lecteur A:.",
        "ENTR\220E = Continuer"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Setup n'a pu mettre \205 jour les r\202glages de disposition du clavier.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup n'a pu mettre \205 jour les r\202glages de l'\202cran.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Setup n'a pu importer un fichier ruche.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_FIND_REGISTRY
        "Setup n'a pu trouver les fichiers de la base de registres.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_CREATE_HIVE,
        "Setup n'a pu cr\202er les ruches de la base de registres.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Setup n'a pu initialiser la base de registres.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Le Cabinet n'a pas de fichier inf valide.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cabinet non trouv\202.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Cabinet n'a pas de script de setup.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_COPY_QUEUE,
        "Setup n'a pu ouvrir la file d'attente de copie de fichiers.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_CREATE_DIR,
        "Setup n'a pu cr\202er les r\202pertoires d'installation.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Setup n'a pu trouver la section '%S'\n"
        "dans TXTSETUP.SIF.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_CABINET_SECTION,
        "Setup n'a pu trouver la section '%S\n"
        "dans le cabinet.\n",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Setup n'a pu cr\202er le r\202pertoire d'installation.",
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Setup n'a pu \202crire les tables de partition.\n"
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Setup n'a pu ajouter la page de codes au registre.\n"
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup n'a pu changer la langue syst\212me.\n"
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Setup n'a pas pu ajouter les dispositions de clavier au registre.\n"
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Setup n'a pas pu d\202finir la geo id.\n"
        "ENTR\220E = Red\202marrer l'ordinateur"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Nom de r\202pertoire invalide.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Impossible de cr\202er une nouvelle partition primaire ou \202tendue\n"
        "sur ce disque parce que sa table de partition est pleine.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Impossible de cr\202er plus d'une partition \202tendue par disque.\n"
        "\n"
        "  * Appuyer sur une touche pour continuer."
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

MUI_PAGE frFRPages[] =
{
    {
        SETUP_INIT_PAGE,
        frFRSetupInitPageEntries
    },
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
        REPAIR_INTRO_PAGE,
        frFRRepairPageEntries
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
        CHANGE_SYSTEM_PARTITION,
        frFRChangeSystemPartition
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
        CHECK_FILE_SYSTEM_PAGE,
        frFRCheckFSEntries
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
        BOOTLOADER_SELECT_PAGE,
        frFRBootLoaderSelectPageEntries
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
        BOOTLOADER_INSTALL_PAGE,
        frFRBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        frFRBootLoaderRemovableDiskPageEntries
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
     "   ENTR\220E = Installer   C/E = Cr\202er partition Primaire/\220tendue   F3 = Quitter"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTR\220E = Installer   C = Cr\202er partition Logique   F3 = Quitter"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTR\220E = Installer   D = Supprimer partition   F3 = Quitter"},
    {STRING_DELETEPARTITION,
     "   D = Supprimer partition   F3 = Quitter"},
    {STRING_PARTITIONSIZE,
     "Taille de la nouvelle partition :"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Vous avez choisi de cr\202er une partition primaire sur"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Vous avez choisi de cr\202er une partition \202tendue sur"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Vous avez choisi de cr\202er une partition logique sur"},
    {STRING_HDPARTSIZE,
    "Veuillez entrer la taille de la nouvelle partition en m\202gaoctets."},
    {STRING_CREATEPARTITION,
     "   ENTR\220E = Cr\202er partition   \220CHAP = Annuler   F3 = Quitter"},
    {STRING_NEWPARTITION,
    "Setup a cr\202\202 une nouvelle partition sur"},
    {STRING_PARTFORMAT,
    "Cette partition sera ensuite format\202e."},
    {STRING_NONFORMATTEDPART,
    "Vous avez choisi d'installer ReactOS sur une nouvelle partition."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "La partition syst\212me n'est pas encore format\202e."},
    {STRING_NONFORMATTEDOTHERPART,
    "La nouvelle partition n'est pas encore format\202e."},
    {STRING_INSTALLONPART,
    "Setup installe ReactOS sur la partition"},
    {STRING_CONTINUE,
    "ENTR\220E = Continuer"},
    {STRING_QUITCONTINUE,
    "F3 = Quitter  ENTR\220E = Continuer"},
    {STRING_REBOOTCOMPUTER,
    "ENTR\220E = Red\202marrer l'ordinateur"},
    {STRING_DELETING,
     "   Suppression du fichier : %S"},
    {STRING_MOVING,
     "   D\202placement du fichier : %S vers : %S"},
    {STRING_RENAMING,
     "   Renommage du fichier : %S vers : %S"},
    {STRING_COPYING,
     "   Copie du fichier : %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup copie les fichiers..."},
    {STRING_REGHIVEUPDATE,
    "   Mise \205 jour de la base de registres..."},
    {STRING_IMPORTFILE,
    "   Importe %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Mise \205 jour des param\212tres de registre pour l'\202cran..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Mise \205 jour des param\212tres r\202gionaux..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Mise \205 jour des param\212tres de disposition du clavier..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Ajout des informations de pages de codes..."},
    {STRING_DONE,
    "   Termin\202..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTR\220E = Red\202marrer l'ordinateur"},
    {STRING_REBOOTPROGRESSBAR,
    " Votre ordinateur va red\202marrer dans %li seconde(s)... "},
    {STRING_CONSOLEFAIL1,
    "Impossible d'ouvrir la console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "La cause probable est l'utilisation d'un clavier USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Les claviers USB ne sont pas compl\212tement support\202s actuellement\r\n"},
    {STRING_FORMATTINGPART,
    "Setup formate la partition..."},
    {STRING_CHECKINGDISK,
    "Setup v\202rifie le disque..."},
    {STRING_FORMATDISK1,
    " Formater la partition comme syst\212me de fichiers %S (formatage rapide) "},
    {STRING_FORMATDISK2,
    " Formater la partition comme syst\212me de fichiers %S "},
    {STRING_KEEPFORMAT,
    " Garder le syst\212me de fichiers courant (pas de changements) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "sur le %s."},
    {STRING_PARTTYPE,
    "Type 0x%02x"},
    {STRING_HDDINFO1,
    // "Disque dur %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Disque dur %lu (Port=%hu, Bus=%hu, Id=%hu) sur %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Disque dur %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Disque dur %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Espace non partitionn\202"},
    {STRING_MAXSIZE,
    "Mo (max. %lu Mo)"},
    {STRING_EXTENDED_PARTITION,
    "Partition \220tendue"},
    {STRING_UNFORMATTED,
    "Nouveau (non format\202)"},
    {STRING_FORMATUNUSED,
    "Non utilis\202"},
    {STRING_FORMATUNKNOWN,
    "Inconnu"},
    {STRING_KB,
    "Ko"},
    {STRING_MB,
    "Mo"},
    {STRING_GB,
    "Go"},
    {STRING_ADDKBLAYOUTS,
    "Ajout des dispositions de clavier"},
    {0, 0}
};
