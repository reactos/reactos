#pragma once

static MUI_ENTRY itITSetupInitPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY itITLanguagePageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Selezione della lingua.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Scegliere la lingua da usare durante l'installazione.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Poi premere invio.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Questa lingua sar\x85 quella predefinita per il sistema installato.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua  F3 = Termina",
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

static MUI_ENTRY itITWelcomePageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Benvenuto all'installazione di ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Questa parte dell'installazione copia ReactOS nel vostro computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "e prepara la seconda parte dell'installazione.",
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
        "\x07  Premere R per riparare ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Premere L per vedere i termini e condizioni della licenza.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Per maggiori informazioni riguardo ReactOS, visitate il sito:",
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
        "   INVIO = Continua  R = Ripara F3 = Termina",
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

static MUI_ENTRY itITIntroPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Status versione ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS \x8A nello stadio di sviluppo Alpha, significa che non tutte le",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "funzioni sono operative e cambia continuamente. \x90 adatto solamente per",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "scopi di valutazione, test ed \x8A inadatto per le attivit\x85 quotidiane.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Eseguire un backup dei dati oppure installate il sistema in un computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "senza sistema operativo e dati se volete eseguirlo su un hardware fisico.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Premere INVIO per continuare l'installazione di ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "INVIO = Continua   F3 = Termina",
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

static MUI_ENTRY itITLicensePageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licenza:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS aderisce ai termini di licenza",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL con parti che contengono codice di altre",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licenze compatibili come la X11 o BSD e la GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Tutto il software che fa parte del sistema ReactOS viene",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "rilasciato sotto licenza GNU GPL e mantiene anche",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "la licenza originale.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Questo software viene fornito SENZA GARANZIA o limitazioni d'uso",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "eccetto per leggi locali o internazionali applicabili. La licenza",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "di ReactOS copre solo la distribuzione a terze parti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Se per un qualsiasi motivo non avesse ricevuto una copia",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "della licenza GNU GPL con ReactOS, visiti il sito:",
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
        "Garanzia:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Questo software \x8A libero; vedere il codice per le condizioni di copia.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "NON esiste garanzia; n\x8A di COMMERCIABILIT\x85",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "o adeguatezza ad un uso particolare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Ritorna",
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

static MUI_ENTRY itITDevicePageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "L'elenco che segue mostra le impostazioni correnti delle periferiche.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "       Computer:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "        Schermo:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "       Tastiera:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        3,
        14,
        "  Layout di tastiera:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "       Conferma:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Accettare queste impostazioni",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Pu\x95 scegliere un elemento della configurazione con i tasti SU e GI\xEB",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "e modificarlo premendo INVIO per selezionare un valore alternativo.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        " ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Quando le impostazioni saranno corrette, selezionare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\"Accettare queste impostazioni\" e premere INVIO.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
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

static MUI_ENTRY itITRepairPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Il setup di ReactOS \x8A ancora in una fase preliminare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Non ha ancora tutte le funzioni di installazione.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Le funzioni di ripristino non sono state ancora implementate.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Premere U per aggiornare il SO.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Premere R per la console di ripristino.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Premere ESC tornare al men\x97 principale.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Men\x97 iniziale INVIO = Riavvio",
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

static MUI_ENTRY itITUpgradePageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
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

static MUI_ENTRY itITComputerPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Desidera modificare il computer da installare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Premere i tasti SU e GI\xEB per scegliere il tipo.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Dopo premere INVIO.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "il tipo di computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
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

static MUI_ENTRY itITFlushPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Il sistema si sta accertando che tutti i dati vengano salvati.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Questo potrebbe impiegare qualche minuto.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Al termine, il computer verr\x85 riavviato automaticamente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Svuotamento della cache in corso",
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

static MUI_ENTRY itITQuitPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS non \x8A stato installato completamente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Rimuovere il disco floppy dall'unit\x85 A: e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Tutti i CD-ROMs dalle unit\x85.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Attendere prego ...",
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

static MUI_ENTRY itITDisplayPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Desidera modificare il tipo di schermo.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Premere i tasti SU e GI\xEB per modificare il tipo.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Dopo premere INVIO.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Premere il tasto ESC per tornare alla pagina precedente",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        " senza modificare il tipo di schermo.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
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

static MUI_ENTRY itITSuccessPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "I componenti base di ReactOS sono stati installati correttamente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Rimuovere il disco dall'unit\x85 A: e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "tutti i CD-ROMs dalle unit\x85.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Riavvia il computer",
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

static MUI_ENTRY itITSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "La lista seguente mostra le partizioni esistenti e lo spazio",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "libero per nuove partizioni.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Premere SU o GI\xEB per selezionare un elemento della lista.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Premere INVIO per installare ReactOS sulla partizione selezionata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Premere C per creare una partizione primaria/logica.",
//        "\x07  Premere C per creare una nuova partizione.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Premere E per creare una partizione estesa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Premere D per cancellare una partizione esistente.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Attendere...",
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

static MUI_ENTRY itITChangeSystemPartition[] =
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

static MUI_ENTRY itITConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Hai scelto di eliminare la partizione di sistema.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Le partizioni di sistema possono contenere i programmi diagnostici,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "configurazione hardware, programmi utilizzati per l'avvio di un",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "sistema operativo (come ReactOS o altri), programmi forniti",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "dal produttore dell'hardware.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Elimina una partizione di sistema solo quando sei sicuro che non ci",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "siano programmi sulla partizione, o quando sei sicuro di eliminarla.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Cancellando una partizione, non sar\x85 pi\x97 possibile avviare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "il computer dall'harddisk fino al termine dell'installazione di ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Primere INVIO per eliminare una partizione di sistema. Ti sar\x85",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "   chiesto di confermare ancora l'eliminazione della partizione.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Premere ESC per ritornare alla pagina precedente. La partizione",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "   non verr\x85 cancellata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "INVIO=Continua  ESC=Annulla",
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

static MUI_ENTRY itITFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Formattazione della partizione",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Setup formatter\x85 la partizione. Premere INVIO per continuare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
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

static MUI_ENTRY itITCheckFSEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup sta controllando la partizione selezionata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Attendere...",
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

static MUI_ENTRY itITInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Il setup installer\x85 i file di ReactOS nella partizione selezionata.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Scegliere una cartella dove si vuole che ReactOS venga installato:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Per modificare la cartella suggerita premere CANC e poi digitate",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "la cartella desiderata per l'installazione di ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        " ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
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

static MUI_ENTRY itITFileCopyEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        11,
        12,
        "Attendere mentre il setup di ReactOS copia i file nella",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        18,
        13,
        "cartella di installazione di ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        20,
        14,
        "Potrebbe richiedere alcuni minuti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "                                                           \xB3 Attendere prego...    ",
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

static MUI_ENTRY itITBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
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
        "Installazione del bootloader sul disco fisso (MBR e VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Installazione del bootloader sul disco fisso (solo VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Installazione del bootloader su un disco floppy.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Salta l'installazione del bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
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

static MUI_ENTRY itITBootLoaderInstallPageEntries[] =
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
        "Setup sta installando il bootloader.",
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

static MUI_ENTRY itITBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup sta installando il bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Inserire un disco floppy formattato nell'unit\x85 A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "e premere INVIO.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
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

static MUI_ENTRY itITKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Volete cambiare il tipo di tastiere da installare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Premere SU o GI\xEB per selezionare il tipo di tastiera desiderato.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Poi premere INVIO.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   il tipo di tastiera.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
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

static MUI_ENTRY itITLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Selezionare la nazionalit\x85 predefinita della tastiera.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Premere SU o GI\xEB per selezionare il tipo di tastiera",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    desiderato. Poi premere INVIO.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   la nazionalit\x85 della tastiera.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
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

static MUI_ENTRY itITPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup sta preparando il computer per la copia dei file di ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Costruzione dell'elenco dei file da copiare...",
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

static MUI_ENTRY itITSelectFSEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Scegliere un file system dalla lista seguente.",
        0
    },
    {
        8,
        19,
        "\x07  Premere SU o GI\xEB per selezionare un filesystem.",
        0
    },
    {
        8,
        21,
        "\x07  Premere INVIO per formattare una partizione.",
        0
    },
    {
        8,
        23,
        "\x07  Premere ESC per selezionare un'altra partizione.",
        0
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
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

static MUI_ENTRY itITDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Avete scelto di cancellare la partizione",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Premere L per cancellare la partizione.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "ATTENZIONE: Tutti i dati di questa partizione saranno persi!!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Premere ESC per annullare.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   L = Cancella la partizione   ESC = Annulla   F3 = Termina",
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

static MUI_ENTRY itITRegistryEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Setup sta aggiornando la configurazione del sistema.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "   Creazione degli hive del registro in corso...",
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

MUI_ERROR itITErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Successo\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS non \x8A stato installato completamente nel\n"
        "vostro computer. Se esce adesso, dovr\x85 eseguire il\n"
        "Setup nuovamente per installare ReactOS.\n"
        "\n"
        "  \x07  Premere INVIO per continuare il setup.\n"
        "  \x07  Premere F3 per uscire.",
        "F3 = Uscire INVIO = Continuare"
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
        "Setup non ha trovato un disco fisso.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Setup non ha trovato l'unit\x85 di origine.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Setup non ha potuto caricare il file TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Setup ha trovato un file TXTSETUP.SIF corrotto.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup ha trovato una firma invalida in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Setup non ha potuto recuperare le informazioni dell'unit\x85 di sistema.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_WRITE_BOOT,
        "Impossibile installare il bootcode %S nella partizione di sistema.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Setup non ha potuto caricare l'elenco di tipi di computer.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Setup non ha potuto caricare l'elenco di tipi di schermo.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Setup non ha potuto caricare l'elenco di tipi di tastiera.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Setup non ha potuto caricare l'elenco delle nazionalit\x85 di tastiera.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_WARN_PARTITION,
        "Setup ha trovato che almeno un disco fisso contiene una tabella delle\n"
        "partizioni incompatibile che non pu\x95 essere gestita correttamente!\n"
        "\n"
        "Il creare o cancellare partizioni pu\x95 distruggere la tabella delle partizioni.\n"
        "\n"
        "  \x07  Premere F3 per uscire dal Setup.\n"
        "  \x07  Premere INVIO per continuare.",
        "F3 = Uscire  INVIO = Continuare"
    },
    {
        // ERROR_NEW_PARTITION,
        "Non si pu\x95 creare una nuova partizione all'interno\n"
        "di una partizione gi\x85 esistente!\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Impossibile installare il bootcode %S nella partizione di sistema.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_NO_FLOPPY,
        "Non c'\x8A un disco nell'unit\x85 A:.",
        "ENTER = Continue"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Setup non ha potuto aggiornare la nazionalit\x85 della tastiera.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup non ha potuto aggiornare la configurazione di registro dello schermo.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Setup non ha potuto importare un file hive.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_FIND_REGISTRY
        "Setup non ha potuto trovare i file di dati del registro.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_CREATE_HIVE,
        "Setup non ha potuto creare gli hive del registro.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Setup non ha potuto inizializzare il registro.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Il Cabinet non ha un file inf valido.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_CABINET_MISSING,
        "Cabinet non trovato.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Il Cabinet non ha uno script di setup.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_COPY_QUEUE,
        "Setup non ha potuto aprire la coda di copia di file.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_CREATE_DIR,
        "Setup non ha potuto creare le cartelle d'installazione.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Setup non ha potuto trovare le sezioni '%S'\n"
        "in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_CABINET_SECTION,
        "Setup non ha potuto trovare le sezioni '%S'\n"
        "nel cabinet.\n",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Setup non ha potuto creare la cartella d'installazione.",
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Setup non ha potuto scrivere le tabelle delle partizioni.\n"
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Setup non ha potuto aggiungere la codepage al registro.\n"
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Setup non ha potuto impostare la regionalizzazione del sistema.\n"
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Impossibile aggiungere le nazionalit\x85 di tastiera al registro.\n"
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Setup non ha potuto impostare l'id geografico.\n"
        "INVIO = Riavviare il computer"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Nome della cartella non valido.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Spazio nella partizione insufficiente per installare ReactOS.\n"
        "La partizione deve avere una dimensione di almeno %lu MB.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Non \x8A possibile creare una partizione primaria o secondaria nella\n"
        "tabella delle partizioni del disco perch\x8A questa \x8A piena.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Impossibile creare pi\x97 di una partizione primaria per disco.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Setup non \x8A riuscito a formattare la partizione:\n"
        " %S\n"
        "\n"
        "ENTER = Riavvia il computer"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE itITPages[] =
{
    {
        SETUP_INIT_PAGE,
        itITSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        itITLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        itITWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        itITIntroPageEntries
    },
    {
        LICENSE_PAGE,
        itITLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        itITDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        itITRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        itITUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        itITComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        itITDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        itITFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        itITSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        itITChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        itITConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        itITSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        itITFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        itITCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        itITDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        itITInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        itITPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        itITFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        itITKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        itITBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        itITLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        itITQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        itITSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        itITBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        itITBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        itITRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING itITStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Attendere..."},
    {STRING_INSTALLCREATEPARTITION,
     "   INVIO = Installa   C = Crea Partizione   E = Crea Partizione Estesa   F3 = Esci"},
    {STRING_INSTALLCREATELOGICAL,
     "   INVIO = Installa   C = Crea Partizione Lgica  F3 = Esci"},
    {STRING_INSTALLDELETEPARTITION,
     "   INVIO = Installa   D = Rimuovi Partizione   F3 = Esci"},
    {STRING_DELETEPARTITION,
     "   D = Elimina Partizione   F3 = Esci"},
    {STRING_PARTITIONSIZE,
     "Dimensione nuova partizione:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Si \x8A scelto di creare una nuova partizione primaria su"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Si \x8A scelto di creare una nuova partizione estesa su"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Si \x8A scelto di creare una nuova partizione logica su"},
    {STRING_HDPARTSIZE,
    "Indicare la dimensione della nuova partizione in megabyte."},
    {STRING_CREATEPARTITION,
     "   INVIO = Creare la partizione   ESC = Annulla   F3 = Esci"},
    {STRING_NEWPARTITION,
    "Setup ha creato una nuova partizione su"},
    {STRING_PARTFORMAT,
    "Questa partizione sar\x85 formattata successivamente."},
    {STRING_NONFORMATTEDPART,
    "Avete scelto di installare ReactOS su una partizione nuova o non formattata."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "La partizione di sistema non \x8A stata ancora formattata."},
    {STRING_NONFORMATTEDOTHERPART,
    "La nuova partizione non \x8A stata ancora formattata."},
    {STRING_INSTALLONPART,
    "Il setup installer\x85 ReactOS sulla partitione"},
    {STRING_CONTINUE,
    "INVIO = Continua"},
    {STRING_QUITCONTINUE,
    "F3 = Esci  INVIO = Continua"},
    {STRING_REBOOTCOMPUTER,
    "INVIO = Riavvia il computer"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Copia di: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Copia dei file in corso..."},
    {STRING_REGHIVEUPDATE,
    "   Aggiornamento degli hives del registro..."},
    {STRING_IMPORTFILE,
    "   Importazione di %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Aggiornamento delle impostazioni dello schermo nel registro..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Aggiornamento delle impostazioni di regionalizzazione..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Aggiornamento delle impostazioni della tastiera..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Aggiunta delle informazioni di codepage..."},
    {STRING_DONE,
    "   Fatto..."},
    {STRING_REBOOTCOMPUTER2,
    "   INVIO = Riavvia il computer"},
    {STRING_REBOOTPROGRESSBAR,
    " Il computer si riavvier\x85 in %li secondi... "},
    {STRING_CONSOLEFAIL1,
    "Impossibile aprire la console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "La causa pi\x97 frequente di questo \x8A l'uso di una tastiera USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "le tastiere USB non sono ancora completamente supportate\r\n"},
    {STRING_FORMATTINGPART,
    "Setup sta formattando la partizione..."},
    {STRING_CHECKINGDISK,
    "Setup sta controllando il disco..."},
    {STRING_FORMATDISK1,
    " Formatta la partizione con file system %S (formattazione rapida) "},
    {STRING_FORMATDISK2,
    " Formatta la partizione con file system %S "},
    {STRING_KEEPFORMAT,
    " Mantieni il file system attuale (nessuna modifica) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "su %s."},
    {STRING_PARTTYPE,
    "Tipo 0x%02x"},
    {STRING_HDDINFO1,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) su %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Spazio non partizionato"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Partizione estesa"},
    {STRING_UNFORMATTED,
    "Nuova (Non formattata)"},
    {STRING_FORMATUNUSED,
    "Non usata"},
    {STRING_FORMATUNKNOWN,
    "Sconsciuta"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Aggiunta delle nazionalit\x85 di tastiera"},
    {0, 0}
};
