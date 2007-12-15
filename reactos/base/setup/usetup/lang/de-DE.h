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

MUI_ERROR deDEErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS ist nicht vollstaendig auf Ihrem System installiert.\n"
	     "Wenn Sie das Setup jetzt beenden, muessen Sie das\n"
	     "Setup erneut starten, um ROS zu installieren.\n"
	     "\n"
	     "  \x07  Druecken Sie ENTER um das Setup Fortzusetzen.\n"
	     "  \x07  Druecken Sie F3 um das Setup zu beenden.",
	     "F3 = Beenden  ENTER = Fortsetzen"
    },
    {
        //ERROR_NO_HDD
        "Setup konnte keine Festplatte finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup konnte das Quelllaufwerk nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup konnte TXTSETUP.SIF nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup fand eine korrupte TXTSETUP.SIF.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup fand eine ungueltige Signatur in TXTSETUP.SIF.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup konnte keine Laufwerksinformationen abfragen.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup konnte den FAT Bootcode nicht auf der Partition installieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup konnte die Computertypenliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup konnte die Displayeinstellungsliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup konnte die Tastaturtypenliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup konnte die Tastaturlayoutliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup hat mindestens eine Festplatte mit einer inkompatiblen Partitionstabelle\n"
		  "welche nicht richtig verwendet werden koennen!\n"
		  "\n"
		  "Partitionen zu erstellen/loeschen kann die Partitionstabelle zerstoeren.\n"
		  "\n"
		  "  \x07  Druecken Sie F3 um das Setup zu beenden."
		  "  \x07  Druecken Sie ENTER um das Setup Fortzusetzen.",
          "F3 = Beenden  ENTER = Fortsetzen"
    },
    {
        //ERROR_NEW_PARTITION,
        "Sie koennen keine neue Partition in einer bereits\n"
		"vohandenen Partition erstellen!\n"
		"\n"
		"  * * Eine beliebige Taste zum Fortsetzen druecken.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Sie koennen unpartitionieren Speicher nicht loeschen!\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen druecken.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup konnte den FAT Bootcode nicht auf der Partition installieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_NO_FLOPPY,
        "Keine Diskette in Laufwerk A:.",
        "ENTER = Fortsetzen"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup konnte das Tastaturlayout nicht aktualisieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup konnte die Display-Registrywerte nicht aktualisieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup konnte keine Hive Datei importieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup konnte die Registrydateien nicht finden.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup konnte die Registry-Hives nicht erstellen.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup konnte die Registry nicht initialisieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet hat keine gueltige .inf Datei.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet nicht gefunden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet hat kein Setup Skript.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup konnte die Liste mit zu kopierenden Dateien nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup konnte die Installationspfade nicht erstellen.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup konnte die 'Ordner' Sektion in\n"
        "TXTSETUP.SIF nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup konnte die 'Ordner' Sektion im\n"
        "Cabinet nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup konnte den Installationspfad nicht erstellen.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup konnte die 'SetupData' Sektion in\n"
		 "TXTSETUP.SIF nicht finden.\n",
		 "ENTER = Computer neustarten"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup konnte die Partitionstabellen nicht schreiben.\n"
        "ENTER = Computer neustarten"
    },
    {
        NULL,
        NULL
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
