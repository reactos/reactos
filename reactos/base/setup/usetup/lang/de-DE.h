#ifndef LANG_DE_DE_H__
#define LANG_DE_DE_H__

static MUI_ENTRY deDEWelcomePageEntries[] =
{
    {
        6,
        8,
        "Willkommen zum ReactOS Setup",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Dieser Teil der Setups kopiert das ReactOS Betriebssystem auf Ihren",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Computer und bereitet den zweiten Teil des Setups vor.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Druecken Sie ENTER, um ReactOS zu installieren.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Druecken Sie R, um ReactOS zu reparieren.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Druecken Sie L, um die Lizenzabkommen von ReactOS zu lesen",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Druecken Sie F3, um das Setup zu beenden ohne ReactOS zu installieren.",
        TEXT_NORMAL
    },
    {
        6, 
        23,
        "Fuer weitere Informationen, besuchen Sie bitte:",
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
        "   ENTER = Fortsetzen  R = Reparieren F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEIntroPageEntries[] =
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
        "Das ReactOS Setup ist noch in einer fruehen Entwicklungsphase. Es unter-",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "stuetzt noch nicht alle Funktionen eines vollstaendig nutzbaren Setups.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "Folgende Begrenzungen sind vorhanden:",
        TEXT_NORMAL
    },
    {
        8, 
        13,
        "- Setup kann nur eine primaere Partition auf einer HDD verwalten.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Setup kann keine primaere Partition von einer HDD loeschen",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  so lange erweiterte Partitionen auf dieser HDD existieren.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Setup kann die erste erweiterte Partition nicht von der HDD loeschen",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  so lange weitere erweiterte Partitionen auf dieser HDD existieren.",
        TEXT_NORMAL
    },
    {
        8, 
        18, 
        "- Setup unterstuetzt nur FAT Dateisysteme.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "- Dateisystemueberpruefung ist noch nicht implementiert.",
        TEXT_NORMAL
    },
    {
        8, 
        23, 
        "\x07  Druecken Sie ENTER, um ReactOS zu installieren.",
        TEXT_NORMAL
    },
    {
        8, 
        25, 
        "\x07  Druecken Sie F3, um das Setup zu beenden.",
        TEXT_NORMAL
    },
    {
        0,
        0, 
        "   ENTER = Fortsetzen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDELicensePageEntries[] =
{
    {
        6,
        6,
        "Lizenz:",
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
        "   ENTER = Zurueck",
        TEXT_STATUS
    },
    {           
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEDevicePageEntries[] =
{
    {
        6, 
        8,
        "Die untere Liste zeigt die derzeitigen Geraeteeinstellungen.",
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
        "     Bildschirm:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Tastatur:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " Tastaturlayout:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "    Akzeptieren:",
        TEXT_NORMAL
    },
    {
        25, 
        16, "Diese Geraeteeinstellungen akzeptieren",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Sie koennen die Einstellungen durch die HOCH- und RUNTER-Tasten auswaehlen",
        TEXT_NORMAL
    },
    {
        6, 
        20, 
        "Dann druecken Sie ENTER, um eine Alternative Einstellung auszuwaehlen",
        TEXT_NORMAL
    },
    {
        6, 
        21,
        " ",
        TEXT_NORMAL
    },
    {
        6, 
        23, 
        "Wenn alle Einstellungen korrekt sind, waehlen Sie \"Diese Geraete-",
        TEXT_NORMAL
    },
    {
        6, 
        24,
        "einstellungen akzeptieren\" und druecken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDERepairPageEntries[] =
{
    {
        6, 
        8,
        "Das ReactOS Setup ist noch in einer fruehen Entwicklungsphase. Es unter-",
        TEXT_NORMAL
    },
    {
        6, 
        9,
        "stuetzt noch nicht alle Funktionen eines vollstaendig nutzbaren Setups.",
        TEXT_NORMAL
    },
    {
        6, 
        12,
        "Die Reparaturfunktionen sind noch nicht implementiert.",
        TEXT_NORMAL
    },
    {
        8, 
        15,
        "\x07  Druecken Sie U, um ReactOS zu aktualisieren.",
        TEXT_NORMAL
    },
    {
        8, 
        17,
        "\x07  Druecken Sie R, fuer die Wiederherstellungskonsole.",
        TEXT_NORMAL
    },
    {
        8, 
        19,
        "\x07  Druecken Sie ESC, um zur Hauptseite zurueckzukehren.",
        TEXT_NORMAL
    },
    {
        8, 
        21,
        "\x07  Druecken Sie ENTER, um den Computer neuzustarten.",
        TEXT_NORMAL
    },
    {
        0, 
        0,
        "   ESC = Hauptseite  ENTER = Neustarten",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY deDEComputerPageEntries[] =
{
    {
        6,
        8,
        "Sie wollen den Computertyp aendern, der installiert wird.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Druecken Sie die HOCH- oder RUNTER-Taste um den gewuenschten",
        TEXT_NORMAL
    },    
    {
        8,
        11,
        "   Typ zu waehlen. Dann druecken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Druecken Sie ESC, um zur vorherigen Seite zurueckzukehren,",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ohne den Computertyp zu aendern.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEFlushPageEntries[] =
{
    {
        10,
        6,
        "Das System vergewissert sich nun, dass alle Daten gespeichert sind.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Dies kann einige Minuten in Anspruch nehmen.",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Der Computer wird automatisch neustarten, wenn der Vorgang beendet ist.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Cache wird geleert",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEQuitPageEntries[] =
{
    {
        10,
        6,
        "ReactOS wurde nicht vollstaendig installiert",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Entfernen Sie die Diskette aus Laufwerk A: und",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "alle CD-ROMs aus den CD-Laufwerken.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Druecken Sie ENTER, um den Computer neuzustarten.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Bitte warten ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEDisplayPageEntries[] =
{
    {
        6,
        8,
        "Sie wollen den Bildschirmtyp aendern, der installiert wird.",
        TEXT_NORMAL
    },
    {   8,
        10,
        "\x07  Druecken Sie die HOCH- oder RUNTER-Taste um den gewuenschten",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Typ zu waehlen. Dann druecken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Druecken Sie ESC, um zur vorherigen Seite zurueckzukehren, ohne",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   den Bildschirmtyp zu aendern.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDESuccessPageEntries[] =
{
    {
        10,
        6,
        "Die Standardkomponenten von ReactOS wurden erfolgreich installiert.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Entfernen Sie die Diskette aus Laufwerk A: und",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "alle CD-ROMs aus den CD-Laufwerken.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Druecken Sie ENTER, um den Computer neuzustarten.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Computer neustarten",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEBootPageEntries[] =
{
    {
        6,
        8,
        "Das Setup kann den Bootloader nicht auf der Festplatte Ihres Computers",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "installieren",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Bitte legen Sie eine formatierte Diskette in Laufwerk A: ein und",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "druecken Sie ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

MUI_PAGE deDEPages[] =
{
    {
        START_PAGE,
        deDEWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        deDEIntroPageEntries
    },
    {
        LICENSE_PAGE,
        deDELicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        deDEDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        deDERepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        deDEComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        deDEDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        deDEFlushPageEntries
    },
    {
        QUIT_PAGE,
        deDEQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        deDESuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        deDEBootPageEntries
    },
    {
        -1,
        NULL
    }
};

#endif
