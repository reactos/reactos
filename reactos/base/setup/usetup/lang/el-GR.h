#ifndef LANG_EL_GR_H__
#define LANG_EL_GR_H__

static MUI_ENTRY elGRWelcomePageEntries[] =
{
    {
        6,
        8,
        "Καλώς Ορίσατε στην εγκατάσταση του ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Αυτό το μέρος της εγκατάστασης αντιγράφει το λειτουργικό σύστημα ReactOS στον",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "υπολογιστή σας και προετοιμάζει το δεύτερο μέρος της εγκατάστασης.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Πατήστε ENTER για να εγκαταστήσετε το ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Πατήστε R για να επιδιορθώσετε το ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Πατήστε L για να δείτε τους όρους αδειοδότησης και τις προϋποθέσεις για το ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Πατήστε F3 για να αποχωρήσετε χωρίς να εγκαταστήσετε το ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Για περισσότερες πληροφορίες για το ReactOS, παρακαλούμε επισκεφθείτε το:",
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
        "   ENTER = Συνέχεια  R = Επιδιόρθωση F3 = Αποχώρηση",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRIntroPageEntries[] =
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
        "Η εγκατάσταση του ReactOS είναι σε αρχική φάση προγραμματισμού. Δεν υποστηρίζει ακόμα",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "όλες τις λειτουργικότητας μιας πλήρους εγκατάστασης λειτουργικού.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Οι επόμενοι περιορισμοί ισχύουν:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Η εγκατάσταση δε μπορεί να ανταπεξέλθει με πάνω από ένα primary partition ανά δίσκο.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Η εγκατάσταση δε μπορεί να διαγράψει ένα primary partition από ένα δίσκο",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  εφόσον υπάρχουν extended partitions στο δίσκο αυτό.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Η εγκατάσαση δε μπορεί να διαγράψει το πρώτο extended partition ενός δίσκου",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  εφόσον υπάρχουν κι άλλα extended partitions στο δίσκο αυτό.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Η εγκατάσταση υποστηρίζει μόνο FAT συστήματα αρχείων.",
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
        "\x07  Πατήστε ENTER για να εγκαταστήσετε το ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Πατήστε F3 για να αποχωρήσετε χωρίς να εγκαταστήσετε το ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Συνέχεια   F3 = Αποχώρηση",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRLicensePageEntries[] =
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

static MUI_ENTRY elGRDevicePageEntries[] =
{
    {
        6,
        8,
        "Η παρακάτω λίστα δείχνει τις ρυθμίσεις συσκευών.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Υπολογιστής:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Εμφάνιση:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Πληκτρολόγιο:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Διάταξη πληκτρολογίου:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         Αποδοχή:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Αποδοχή αυτών των ρυθμίσεων συσκευών",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Μπορείτε να αλλάξετε τις ρυθμίσεις υλικού πατώντας τα πλήκτρα UP ή DOWN",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "για να επιλέξετε μια ρύθμιση. Μετά πατήστε το πλήκτρο ENTER για να επιλέξετε άλλες",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "ρυθμίσεις.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Όταν όλες οι ρυθμίσεις είναι σωστές, επιλέξτε \"Αποδοχή αυτών των ρυθμίσεων συσκευών\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "και πατήστε ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Συνέχεια   F3 = Αποχώρηση",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRRepairPageEntries[] =
{
    {
        6,
        8,
        "Η εγκατάσταση του ReactOS βρίσκεται σε πρώιμο στάδιο ανάπτυξης. Δεν υποστηρίζει ακόμα",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "όλες τις λειτουργικότητες μιας πλήρους εγκατάστασης λειτουργικού.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Η λειτουργίες επιδιόρθωσης δεν έχουν υλοποιηθεί ακόμα.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Πατήστε U για ανανέωση του OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Πατήστε R για τη Recovery Console.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Πατήστε ESC για να επιστρέψετε στην κύρια σελίδα.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Πατήστε ENTER για να επανεκκινήσετε τον υπολογιστή.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Κύρια σελίδα  ENTER = Επανεκκίνηση",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY elGRComputerPageEntries[] =
{
    {
        6,
        8,
        "Θέλετε να αλλάξετε τον τύπο του υπολογιστή που θα εγκατασταθεί.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Πατήστε τα πλήκτρα UP ή DOWN για να επιλέξετε τον επιθυμητό τύπο υπολογιστή.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Μετά πατήστε ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Πατήστε το πλήκτρο ESC για να επιστρέψετε στην πορηγούμενη σελίδα χωρίς να αλλάξετε",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   τον τύπο υπολογιστή.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Συνέχεια   ESC = Ακύρωση   F3 = Αποχώρηση",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRFlushPageEntries[] =
{
    {
        10,
        6,
        "Το σύστημα επιβεβαιώνει τώρα ότι όλα τα δεδομένα αποθηκεύτηκαν στο δίσκο",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Αυτό ίσως πάρει λίγη ώρα",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Όταν ολοκληρωθεί, ο υπολογιστής σας θα επανεκκινηθεί αυτόματα",
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

static MUI_ENTRY elGRQuitPageEntries[] =
{
    {
        10,
        6,
        "Το ReactOS δεν εγκαταστάθηκε πλήρως",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Αφαιρέστε τη δισκέτα από το A: και",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "όλα τα CD-ROMs  από τα CD-Drives.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Πατήστε ENTER για να επανεκκινήσετε τον υπολογιστή.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Παρακαλώ περιμένετε ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRDisplayPageEntries[] =
{
    {
        6,
        8,
        "Θέλετε να αλλάξετε τον τύπο της εμφάνισης που θα εγκατασταθεί.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Πατήστε το πλήκτρο UP ή DOWN για να επιλέξετε τον επιθυμητό τύπο εμφάνισης.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Μετά πατήστε ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Πατήστε το πλήκτρο ESC για να επιστρέψετε στην προηγούμενη σελίδα χωρίς να αλλάξετε",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   τον τύπο εμφάνισης.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Συνέχεια   ESC = Ακύρωση   F3 = Αποχώρηση",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRSuccessPageEntries[] =
{
    {
        10,
        6,
        "Τα βασικά στοιχεία του ReactOS εγκαταστάθηκαν επιτυχώς.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Αφαιρέστε τη δισκέτα από το A: και",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "όλα τα CD-ROMs από το CD-Drive.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Πατήστε ENTER για να επανεκκινήσετε τον υπολογιστή σας.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Επανεκκίνηση υπολογιστή",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRBootPageEntries[] =
{
    {
        6,
        8,
        "Η εγκατάσταση δε μπορεί να εγκαταστήσει τον bootloader στο σκληρό δίσκο",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "του υπολογιστή σας",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Παρακαλώ εισάγετε μια διαμορφωμένη δισκέτα στο A: και",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "πατήστε ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Συνέχεια   F3 = Αποχώρηση",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY elGRSelectPartitionEntries[] =
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

static MUI_ENTRY elGRFormatPartitionEntries[] =
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

static MUI_ENTRY elGRInstallDirectoryEntries[] =
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

static MUI_ENTRY elGRFileCopyEntries[] =
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

static MUI_ENTRY elGRBootLoaderEntries[] =
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

static MUI_ENTRY elGRKeyboardSettingsEntries[] =
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

static MUI_ENTRY elGRLayoutSettingsEntries[] =
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

static MUI_ENTRY elGRPrepareCopyEntries[] =
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

static MUI_ENTRY elGRSelectFSEntries[] =
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

static MUI_ENTRY elGRDeletePartitionEntries[] =
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


MUI_ERROR elGRErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "Το ReactOS δεν εγκαταστάθηκε πλήρως στον\n"
	     "υπολογιστή σας. Αν αποχωρήσετε από την Εγκατάσταση τώρα, θα πρέπει να\n"
	     "ξανατρέξετε την Εγκατάσταση για να εγκαταστήσετ το ReactOS.\n"
	     "\n"
	     "  \x07  Πατήστε ENTER για αν συνεχίσετε την Εγκατάσταση.\n"
	     "  \x07  Πατήστε F3 για να αποχωρήσετε από την Εγκατάσταση.",
	     "F3= Αποχώρηση  ENTER = Συνέχεια"
    },
    {
        //ERROR_NO_HDD
        "Η Εγκατάσταση δε μπόρεσε να βρει κάποιον σκληρό δίσκο.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Η Εγκατάσταση δε μπόρεσε να φορτώσει το αρχείο TXTSETUP.SIF.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Η Εγκατάστση βρήκε ένα κατεστραμένο αρχείο TXTSETUP.SIF.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Η Εγκατάσταση βρήκε μια μη έγκυρη υπογραφή στο TXTSETUP.SIF.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Η Εγκατάσταση δε μπόρεσε να φορτώσει τις πληροφορίες του δίσκου συστήματος.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_WRITE_BOOT,
        "failed to install FAT bootcode on the system partition.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Η Εγκατάσταση δε μπόρεσε να φορτώσει τη λίστα τύπων υπολογιστή.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Η Εγκατάσταση δε μπόρεσε να φορτώσει τη λίστα τύπων εμφάνισης.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Η Εγκατάσταση δε μπόρεσε να φορτώσει τη λίστα τύπων πληκτρολογίου.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Η Εγκατάσταση δε μπόρεσε να φορτώσει τη λίστα διατάξεων πληκτρολογίου.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_WARN_PARTITION,
          "Η Εγκατάσταση βρήκε ότι τουλάχιστον ένας σκληρός δίσκος περιέχει ένα μη συμβατό\n"
		  "partition table που δε μπορεί να ελεγχθεί σωστά!\n"
		  "\n"
		  "Η δημιουργία ή διαγραφή partitions μπορεί να καταστρέψει το partiton table.\n"
		  "\n"
		  "  \x07  Πατήστε F3 για να αποχωρήσετε από την Εγκατάσταση."
		  "  \x07  Πατήστε ENTER για να συνεχίσετε.",
          "F3= Αποχώρηση  ENTER = Συνέχεια"
    },
    {
        //ERROR_NEW_PARTITION,
        "Δε μπορείτε να δημιουργήσετε ένα Partition μέσα σε\n"
		"ένα άλλο υπάρχον Partition!\n"
		"\n"
		"  * Πατήστε οποιοδήποτε πλήκτρο για να συνεχίσετε.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Δε μπορείτε να διαγράψετε έναν μη διαμορφωμένο χώρο δίσκου!\n"
        "\n"
        "  * Πατήστε οποιοδήποτε πλήκτρο για να συνεχίσετε.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_NO_FLOPPY,
        "Δεν υπάρχει δισκέτα στο A:.",
        "ENTER = Συνέχεια"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Η Εγκατάσαση απέτυχε να ανανεώσει τις ρυθμίσεις για τη διάταξη πληκτρολογίου.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Η Εγκατάσταση απέτυχε να ανανεώσει τις ρυθμίσεις μητρώου για την εμφάνιση.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Η Εγκατάσταση απέτυχε να φορτώσει ένα hive αρχείο.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_FIND_REGISTRY
        "Η Εγκατάσαση απέτυχα να βρει τα αρχεία δεδομένων του μητρώου.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_CREATE_HIVE,
        "Η Εγκατάσταση απέτυχε να δημιουργήσει τα registry hives.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Το cabinet δεν έχει έγκυρο αρχείο inf.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_CABINET_MISSING,
        "Το cabinet δε βρέθηκε.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Το cabinet δεν έχει κανένα σκριπτ εγκατάστασης.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_COPY_QUEUE,
        "Η Εγκατάσταση απέτυχε να ανοίξει την ουρά αρχείων προς αντιγραφή.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_CREATE_DIR,
        "Η Εγκατάσταση δε μπόρεσε να δημιουργήσει τους καταλόγους εγκατάστασης.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Η Εγκατάσταση απέτυχε να βρει τον τομέα 'Directories'\n"
        "στο TXTSETUP.SIF.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_CABINET_SECTION,
        "Η Εγκατάσταση απέτυχε να βρει τον τομέα 'Directories'\n"
        "στο cabinet.\n",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Η Εγκατάσταση δε μπόρεσε να δημιουργήσει τον κατάλογο εγκατάστασης.",
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Η Εγκατάσταση απέτυχε να βρει τον τομέα 'SetupData'\n"
		 "στο TXTSETUP.SIF.\n",
		 "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Η Εγκατάσταση απέτυχε να γράψει τα partition tables.\n"
        "ENTER = Επανεκκίνηση υπολογιστή"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE elGRPages[] =
{
    {
        LANGUAGE_PAGE,
        LanguagePageEntries
    },
    {
       START_PAGE,
       elGRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        elGRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        elGRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        elGRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        elGRRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        elGRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        elGRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        elGRFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        elGRSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        elGRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        elGRFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        elGRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        elGRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        elGRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        elGRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        elGRKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        elGRBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        elGRLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        elGRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        elGRSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        elGRBootPageEntries
    },
    {
        -1,
        NULL
    }
};

#endif
