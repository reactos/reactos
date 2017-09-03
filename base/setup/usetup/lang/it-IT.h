#pragma once

static MUI_ENTRY itITLanguagePageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Selezione della lingua.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Scegliere la lingua da usare durante l'installazione.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Poi premere invio.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Questa lingua sar… quella predefinita per il sistema finale.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua  F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Benvenuto all'installazione di ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Questa parte dell'installazione copia ReactOS nel vostro computer",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "e prepara la seconda parte dell'installazione.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Premere INVIO per installare ReactOS.",
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
        "\x07  Premere L per vedere i termini e condizioni della licenza.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Per maggiori informazioni riguardo ReactOS, visitate il sito:",
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
        "   INVIO = Continua  R = Ripara F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Il setup di ReactOS Š ancora in una fase preliminare.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Non ha ancora tutte le funzioni di installazione.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Si applicano le seguenti limitazioni:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Il setup supporta solamente il sistema FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- File system checks are not implemented yet.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Premere INVIO per installare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licenza:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS aderisce ai termini di licenza",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL con parti che contengono codice di altre",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenze compatibili come la X11 o BSD e la GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Tutto il software che fa parte del sistema ReactOS viene",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "rilasciato sotto licenza GNU GPL e mantiene anche",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "la licenza originale.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Questo software viene fornito SENZA GARANZIA o limitazioni d'uso",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "eccetto per leggi locali o internazionali applicabili. La licenza",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "di ReactOS copre solo la distribuzione a terze parti.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Se per un qualsiasi motivo non avesse ricevuto una copia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "della licenza GNU GPL con ReactOS, visiti il sito:",
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
        "Garanzia:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Questo software Š libero; vedere il codice per le condizioni di copia.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "NON esiste garanzia; n‚ di COMMERCIABILIT·",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "o adeguatezza ad un uso particolare",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Ritorna",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "L'elenco che segue mostra le impostazioni correnti delle periferiche.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "       Computer:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "        Schermo:",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "       Tastiera:",
        TEXT_STYLE_NORMAL
    },
    {
        3,
        14,
        "  Layout di tastiera:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "       Conferma:",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "Accettare queste impostazioni",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Pu• scegliere un elemento della configurazione con i tasti SU e GIë",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "e modificarlo premendo INVIO per selezionare un valore alternativo.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        " ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Quando le impostazioni saranno corrette, selezionare",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "\"Accettare queste impostazioni\" e premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
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

static MUI_ENTRY itITComputerPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Desidera modificare il computer da installare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Premere i tasti SU e GIë per scegliere il tipo.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Dopo premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "il tipo di computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Il sistema si sta accertando che tutti i dati vengano salvati",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Questo potrebbe impiegare qualche minuto",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Al termine, il computer verr… riavviato automaticamente",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Svuotamento della cache in corso",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS non Š stato installato completamente",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Rimuovere il disco floppy dall'unit… A: e",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Tutti i CD-ROMs dalle unità.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Attendere prego ...",
        TEXT_TYPE_STATUS,
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Desidera modificare il tipo di schermo.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Premere i tasti SU e GIë per modificare il tipo.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Dopo premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere il tasto ESC per tornare alla pagina precedente",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        " senza modificare il tipo di schermo.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "I componenti base di ReactOS sono stati installati correttamente.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Rimuovere il disco dall'unit… A: e",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "tutti i CD-ROMs dalle unit….",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Riavvia il computer",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITBootPageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Il Setup non ha potuto installare il bootloader nel disco",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "del vostro computer",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Inserire un disco floppy formattato nell'unit… A: e",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "premere INVIO.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "La lista seguente mostra le partizioni esistenti e lo spazio",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "libero per nuove partizioni.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Premere SU o GIë per selezionare un elemento della lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere INVIO per installare ReactOS sulla partizione selezionata.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Premere P per creare una nuova partizione.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Premere E per creare una partizione estesa.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Premere L per creare una partizione logica.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Premere D per cancellare una partizione esistente.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Attendere...",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Hai scelto di eliminare la partizione di sistema.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Le partizioni di sistema possono contenere i programmi diagnostici, configurazione hardware",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "Programmi utilizzati per l'avvio di un sistema operativo (come ReactOS) o altri.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Programmi forniti dal produttore dell'hardware.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Elimina una partizione di sistema solo quando sei sicuro che non ci siano",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "programmi sulla partizione, o quando sei sicuro di eliminarla.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "Cancellando una partizione, non sarÃ  piÃ¹ possibile avviare",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "il computer dall'harddisk fino al termine del setup di ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Primere INVIO per eliminare una partizione di sistema. Ti sarÃ  chiesto",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   di confermare ancora l'eliminazione della partizione.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Premere ESC per ritornare alla pagina precedente. La partizione",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   non verrÃ  cancellata.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "INVIO=Continua  ESC=Annulla",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formattazione della partizione",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Setup formatter… la partizione. Premere INVIO per continuare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY itITInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup installer… i file di ReactOS nella partizione selezionata.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Scegliere una cartella dove volete che ReactOS sia installato:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Per modificare la cartella suggerita premere CANC e poi digitate",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "la cartella dove volete che ReactOS sia installato.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        " ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Attendere mentre il setup di ReactOS copia i file nella",
        TEXT_STYLE_NORMAL
    },
    {
        18,
        13,
        "cartella di installazione di ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "Potrebbe richiedere alcuni minuti.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Attendere prego...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITBootLoaderEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup sta installando il bootloader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Installazione del bootloader sul disco fisso (MBR e VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Installazione del bootloader sul disco fisso (solo VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Installazione del bootloader su un disco floppy.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Salta l'installazione del bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Volete cambiare il tipo di tastiere da installare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Premere SU o GIë per selezionare il tipo di tastiera desiderato.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Poi premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   il tipo di tastiera.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Selezionare la nazionalit… predefinita della tastiera.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Premere SU o GIë per selezionare il tipo di tastiera",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    desiderato. Poi premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   la nazionalit… della tastiera.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup sta preparando il computer per la copia dei file di ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Costruzione dell'elenco dei file da copiare...",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
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
        "\x07  Premere SU o GIë per selezionare un filesystem.",
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
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Avete scelto di cancellare la partizione",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Premere D per cancellare la partizione.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "ATTENZIONE: Tutti i dati di questa partizione saranno persi!!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Premere ESC per annullare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Cancella la partizione   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup sta aggiornando la configurazione del sistema. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Creazione degli hive del registro in corso...",
        TEXT_TYPE_STATUS
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
        //ERROR_NOT_INSTALLED
        "ReactOS non Š installato completamente nel vostro\n"
        "computer. Se esce adesso, dovr… eseguire il Setup\n"
        "nuovamente per installare ReactOS.\n"
        "\n"
        "  \x07  Premere INVIO per continuare il setup.\n"
        "  \x07  Premere F3 per uscire.",
        "F3 = Uscire INVIO = Continuare"
    },
    {
        //ERROR_NO_HDD
        "Setup non ha trovato un disco fisso.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup non ha trovato l'unit… di origine.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup non ha potuto caricare il file TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup ha trovato un file TXTSETUP.SIF corrotto.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup ha trovato una firma invalida in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup non ha potuto recuperare le informazioni dell'unit… di sistema.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_WRITE_BOOT,
        "Impossibile installare il bootcode FAT nella partizione di sistema.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup non ha potuto caricare l'elenco di tipi di computer.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup non ha potuto caricare l'elenco di tipi di schermo.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup non ha potuto caricare l'elenco di tipi di tastiera.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup non ha potuto caricare l'elenco delle nazionalit… di tastiera.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_WARN_PARTITION,
        "Setup ha trovato che almeno un disco fisso contiene una tabella delle\n"
        "partizioni incompatibile che non pu• essere gestita correttamente!\n"
        "\n"
        "Il creare o cancellare partizioni pu• distruggere la tabella delle partizioni.\n"
        "\n"
        "  \x07  Premere F3 per uscire dal Setup.\n"
        "  \x07  Premere INVIO per continuare.",
        "F3 = Uscire  INVIO = Continuare"
    },
    {
        //ERROR_NEW_PARTITION,
        "Non si pu• creare una nuova partizione all'interno\n"
        "di una partizione gi… esistente!\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Non si pu• cancellare spazio in un disco non partizionato!\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Impossibile installare il bootcode FAT nella partizione di sistema.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_NO_FLOPPY,
        "Non c'Š un disco nell'unit… A:.",
        "ENTER = Continue"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup non ha potuto aggiornare la nazionalit… della tastiera.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup non ha potuto aggiornare la configurazione di registro dello schermo.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup non ha potuto importare un file hive.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup non ha potuto trovare i file di dati del registro.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup non ha potuto creare gli hive del registro.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup non ha potuto inizializzare il registro.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Il Cabinet non ha un file inf valido.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet non trovato.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Il Cabinet non ha uno script di setup.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup non ha potuto aprire la coda di copia di file.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup non ha potuto creare le cartelle d'installazione.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup non ha potuto trovare le sezioni 'Cartelle'\n"
        "in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup non ha potuto trovare le sezioni 'Cartelle'\n"
        "nel cabinet.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup non ha potuto creare la cartella d'installazione.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup non ha trovato la sezione 'SetupData'\n"
        "in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup non ha potuto scrivere le tabelle delle partizioni.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup non ha potuto aggiungere la codepage al registro.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup non ha potuto impostare la regionalizzazione del sistema.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Impossibile aggiungere le nazionalit… di tastiera al registro.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Setup non ha potuto impostare l'id geografico.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Nome della cartella non valido.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Spazio nella partizione insufficiente per installare ReactOS.\n"
        "La partizione deve avere una dimensione di almeno %lu MB.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Non Ã¨ possibile creare una partizione primaria o secondaria nella\n"
        "tabella delle partizioni del disco perchÃ¨ questa Ã¨ piena.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Impossibile creare piÃ¹ di una partizione primaria per disco.\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare."
    },
    {
        //ERROR_FORMATTING_PARTITION,
        "Setup non Ã¨ riuscito a formattare la partizione:\n"
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
        BOOT_LOADER_PAGE,
        itITBootLoaderEntries
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
        BOOT_LOADER_FLOPPY_PAGE,
        itITBootPageEntries
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
     "   INVIO = Installa   P = Crea Partizione   E = Crea Partizione Estesa   F3 = Esci"},
    {STRING_INSTALLCREATELOGICAL,
     "   INVIO = Installa   L = Crea Partizione Lgica  F3 = Esci"},
    {STRING_INSTALLDELETEPARTITION,
     "   INVIO = Installa   D = Rimuovi Partizione   F3 = Esci"},
    {STRING_DELETEPARTITION,
     "   D = Elimina Partizione   F3 = Esci"},
    {STRING_PARTITIONSIZE,
     "Dimensione della nuova partizione:"},
    {STRING_CHOOSENEWPARTITION,
     "Si Ã¨ scelto di creare una nuova partizione primaria su"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Si Ã¨ scelto di creare una nuova partizione estesa su"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Si Ã¨ scelto di creare una nuova partizione logica su"},
    {STRING_HDDSIZE,
    "Indicare la dimensione della nuova partizione in megabyte."},
    {STRING_CREATEPARTITION,
     "   INVIO = Creare la partizione   ESC = Annulla   F3 = Esci"},
    {STRING_PARTFORMAT,
    "Questa partizione sar… formattata successivamente."},
    {STRING_NONFORMATTEDPART,
    "Avete scelto di installare ReactOS su una partizione nuova o non formattata."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "La partizione di sistema non Ã¨ stata ancora formattata."},
    {STRING_NONFORMATTEDOTHERPART,
    "La nuova partizione non Ã¨ stata ancora formattata."},
    {STRING_INSTALLONPART,
    "Setup installer… ReactOS sulla partitione"},
    {STRING_CHECKINGPART,
    "Setup sta controllando la partizione selezionata."},
    {STRING_CONTINUE,
    "INVIO = Continua"},
    {STRING_QUITCONTINUE,
    "F3 = Esci  INVIO = Continua"},
    {STRING_REBOOTCOMPUTER,
    "INVIO = Riavvia il computer"},
    {STRING_TXTSETUPFAILED,
    "Setup non ha trovato la sezione '%S' \nin TXTSETUP.SIF.\n"},
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
    "   Aggiunta delle informazioni di codepage al registry..."},
    {STRING_DONE,
    "   Fatto..."},
    {STRING_REBOOTCOMPUTER2,
    "   INVIO = Riavvia il computer"},
    {STRING_CONSOLEFAIL1,
    "Impossibile aprire la console\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "La causa pi— frequente di questo Š l'uso di una tastiera USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "le tastiere USB non sono ancora completamente supportate\r\n"},
    {STRING_FORMATTINGDISK,
    "Setup sta formattando il disco"},
    {STRING_CHECKINGDISK,
    "Setup sta controllando il disco"},
    {STRING_FORMATDISK1,
    " Formatta la partizione con file system %S (formattazione rapida) "},
    {STRING_FORMATDISK2,
    " Formatta la partizione con file system %S "},
    {STRING_KEEPFORMAT,
    " Mantieni il file system attuale (nessuna modifica) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) su %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tipo 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "su %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) su %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "su %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tipo 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "su Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTipo %-3u%s                       %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) su %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Setup ha creato una nuova partizione su"},
    {STRING_UNPSPACE,
    "    %sSpazio non partizionato%s             %6lu %s"},
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
    "Aggiunta delle nazionalit… di tastiera"},
    {0, 0}
};
