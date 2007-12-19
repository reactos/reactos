#ifndef LANG_IT_IT_H__
#define LANG_IT_IT_H__

static MUI_ENTRY itITWelcomePageEntries[] =
{
    {
        6,
        8,
        "Benvenuto all'installazione di ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Questa parte dell'installazione copia ReactOS nel vostro computer",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "e prepara la seconda parte dell'installazione.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Premere INVIO per installare ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Premere R per riparare ReactOS.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Premere L per vedere i termini e condizioni della licenza",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_NORMAL
    },
    {
        6, 
        23,
        "Per maggiori informazioni riguardo ReactOS, visitate il sito:",
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
        "   INVIO = Continuare  R = Riparare F3 = Uscire",
        TEXT_STATUS
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
        " Installazione di ReactOS " KERNEL_VERSION_STR,
        TEXT_UNDERLINE
    },
    {
        6, 
        8, 
        "Il setup di ReactOS è ancora in una fase preliminare.",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "Non ha ancora tutte le funzioni di installazione.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "Ci sono delle limitazioni:",
        TEXT_NORMAL
    },
    {
        8, 
        13,
        "- Il setup non gestisce più di una partizione primaria per disco.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Il setup non può eliminare una partizione primaria",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  se ci sono partizioni estese nel disco duro.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Il setup non può eliminare la prima partizione estesa",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  se ci sono altre partizioni estese nel disco duro.",
        TEXT_NORMAL
    },
    {
        8, 
        18, 
        "- Il setup supporta solamente il sistema FAT.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "- La verifica dei volumi non è stata ancora implementata.",
        TEXT_NORMAL
    },
    {
        8, 
        23, 
        "\x07  Premere INVIO per installare ReactOS.",
        TEXT_NORMAL
    },
    {
        8, 
        25, 
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0, 
        "   INVIO = Continuare   F3 = Uscire",
        TEXT_STATUS
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
        6,
        6,
        "Licenza:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS adesrisce ai termini di licenza",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "GNU GPL con parti che contengono codice di altre",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licenze compatibili come la X11 o BSD e la GNU LGPL.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "Tutto il software che fa parte del sistema ReactOS viene",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "rilasciato sotto licenza GNU GPL e mantiene anche",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "la licenza originale.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "Questo software viene fornito SENZA GARANZIA o limitazioni d'uso",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "eccetto per leggi locali o internazionali applicabili. La licenza",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "di ReactOS copre solo la distribuzione a terze parti.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Se per un qualsiasi motivo non avesse ricevuto una copia",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "della licenza GNU GPL con ReactOS, visiti il sito:",
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
        "Garanzia:",
        TEXT_HIGHLIGHT
    },
    {           
        8,
        24,
        "Questo software è libero; vedere il codice per le condizioni di copia.",
        TEXT_NORMAL
    },
    {           
        8,
        25,
        "NON esiste garanzia; né di COMMERCIABILITÀ",
        TEXT_NORMAL
    },
    {           
        8,
        26,
        "o adeguatezza ad un uso particolare",
        TEXT_NORMAL
    },
    {           
        0,
        0,
        "   INVIO = Ritornare",
        TEXT_STATUS
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
        6, 
        8,
        "La lista inferiore mostra la configurazione della periferica corrente.",
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
        "        Schermo:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Tastiera:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Layout tastiera:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "      Accettare:",
        TEXT_NORMAL
    },
    {
        25, 
        16, "Accettare la configurazione delle periferiche",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Può scegliere la configurazione con i tasti SU e GIÙ",
        TEXT_NORMAL
    },
    {
        6, 
        20, 
        "Premere INVIO per modificare la configurazione",
        TEXT_NORMAL
    },
    {
        6, 
        21,
        "alternativa.",
        TEXT_NORMAL
    },
    {
        6, 
        23, 
        "Quando le impostazioni saranno corrette, scegliere \"Accettare la configurazione",
        TEXT_NORMAL
    },
    {
        6, 
        24,
        "delle periferiche\" e premere INVIO.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continuare   F3 = Uscire",
        TEXT_STATUS
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
        6, 
        8,
        "Il setup di ReactOS è ancora in una fase preliminare.",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "Non ha ancora tutte le funzioni di installazione.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "Le funzioni di ripristino non sono state ancora implementate.",
        TEXT_NORMAL
    },
    {
        8, 
        15,
        "\x07  Premere U per aggiornare il SO.",
        TEXT_NORMAL
    },
    {
        8, 
        17,
        "\x07  Premere R per la console di ripristino.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Premere ESC tornare al menù principale.",
        TEXT_NORMAL
    },
    {
        8, 
        21,
        "\x07  Premere INVIO per riavviare il computer.",
        TEXT_NORMAL
    },
    {
        0, 
        0,
        "ESC = Menù iniziale INVIO = Riavvio",
        TEXT_STATUS
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
        6,
        8,
        "Desidera modificare il computer da installare.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Premere i tasti SU e GIÙ per scegliere il tipo.",
        TEXT_NORMAL
    },    
    {
        8,
        11,
        "Dopo premere INVIO.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "il tipo di computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continuare   ESC = Annulla   F3 = Uscire",
        TEXT_STATUS
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
        10,
        6,
        "Il sistema si sta accertando che tutti i dati vengano salvati",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Questo potrebbe impiegare un minuto",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Al termine, il computer verrà riavviato automaticamente",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "Svuotando la cache",
        TEXT_STATUS
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
        10,
        6,
        "ReactOS non è stato installato completamente",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Rimuovere il diskette dall'unità A: e",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "tutti i CD-ROMs dalle unità.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Attendere prego ...",
        TEXT_STATUS,
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
        6,
        8,
        "Desidera modificare il tipo di schermo.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Premere i tasti SU e GIÙ per modificare il tipo.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Dopo premere INVIO.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere il tasto ESC per tornare alla pagina precedente",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " senza modificare il tipo di schermo.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continuare   ESC = Annullare   F3 = Uscire",
        TEXT_STATUS
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
        10,
        6,
        "I componenti base di ReactOS sono stati installati correttamente.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Rimuovere il disco dall'unità A: e",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "tutti i CD-ROMs dalle unità.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Riavviare il computer",
        TEXT_STATUS
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
        6,
        8,
        "Il Setup non ha potuto installare il bootloader nel disco",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "del vostro computer",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Inserire un diskette formattato nell'unità A: e",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "premere INVIO.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   INVIO = Continuare   F3 = Uscire",
        TEXT_STATUS
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

static MUI_ENTRY itITFormatPartitionEntries[] =
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

static MUI_ENTRY itITInstallDirectoryEntries[] =
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

static MUI_ENTRY itITFileCopyEntries[] =
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

static MUI_ENTRY itITBootLoaderEntries[] =
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

static MUI_ENTRY itITKeyboardSettingsEntries[] =
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

static MUI_ENTRY itITLayoutSettingsEntries[] =
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

static MUI_ENTRY itITPrepareCopyEntries[] =
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

static MUI_ENTRY itITSelectFSEntries[] =
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

static MUI_ENTRY itITDeletePartitionEntries[] =
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


MUI_ERROR itITErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS non è installato completamente nel vostro\n"
	     "computer. Se esce adesso, dovrà eseguire il Setup\n"
	     "nuovamente per installare ReactOS.\n"
	     "\n"
	     "  \x07  Premere INVIO per continuare il setup.\n"
	     "  \x07  Premere F3 per uscire.",
	     "F3= Uscire INVIO = Continuare"
    },
    {
        //ERROR_NO_HDD
        "Setup non ha trovato un harddisk.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup non ha trovato l'unità di origine.\n",
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
        "Setup non ha potuto recuperare le informazioni dell'unità di sistema.\n",
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
        "Setup non ha potuto caricare l'elenco di tipi di layout di tastiera.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup ha trovato che al meno un hard disk contiene una tabella delle\n"
		  "partizioni incompatibile che non può essere gestita correttamente!\n"
		  "\n"
		  "Il creare o cancellare partizioni può distruggere la tabella delle partizioni.\n"
		  "\n"
		  "  \x07  Premere F3 per uscire dal Setup."
		  "  \x07  Premere INVIO per continuare.",
          "F3= Uscire  INVIO = Continuare"
    },
    {
        //ERROR_NEW_PARTITION,
        "Non si può creare una nuova partizione all'interno\n"
		"di una partizione già esistente!\n"
		"\n"
		"  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Non si può cancellare spazio di disco non partizionato!\n"
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
        "Non c'è un disco nell'unità A:.",
        "ENTER = Continue"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup non ha potuto aggiornare la configurazione di layout di tastiera.",
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
        "Setup non ha potuto creare i hives del registro.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
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
        "Setup non ha potuto scrivere le tabelle di partizioni.\n"
        "INVIO = Riavviare il computer"
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
        LanguagePageEntries
    },
    {
       START_PAGE,
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
        -1,
        NULL
    }
};

#endif

