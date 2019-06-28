// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY deDESetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        20,
        "Please wait while the ReactOS Setup initializes itself",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        21,
        "and discovers your devices...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        0,
        "Please wait...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDELanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Sprachauswahl",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Bitte w\204hlen Sie die Sprache, die Sie w\204hrend der Installation",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        11,
        "verwenden wollen.  Best\204tigen Sie die Auswahl mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Diese Sprache wird sp\204ter als Standardsprache im System verwendet.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortsetzen  F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Willkommen zum ReactOS-Setup",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Dieser Teil des Setups kopiert das ReactOS-Betriebssystem auf Ihren",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Computer und bereitet die n\204chsten Schritte vor.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Dr\201cken Sie R, um ReactOS zu reparieren.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Dr\201cken Sie L, um die Lizenzabkommen von ReactOS zu lesen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Dr\201cken Sie F3, um die Installation abzubrechen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Weitere Informationen erhalten Sie unter:",
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
        "EINGABETASTE = Fortsetzen  R = Reparieren  L = Lizenz  F3 = Beenden",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
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
        "ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Lizenz:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS ist unter den Bedingungen der GNU General Public License",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "lizenziert. Einige Teile von ReactOS stehen unter dazu kompatiblen",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "Lizenzen wie der BSD- oder GNU LGPL-Lizenz.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "S\204mtliche Software in ReactOS daher unter der",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "GNU GPL ver\224ffentlicht, behalten daneben aber ihre",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "urspr\201nglichen Lizenzen bei.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "ReactOS ist freie Software. Die Ver\224ffentlichung dieses Programms",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "erfolgt in der Hoffnung, dass es Ihnen von Nutzen sein wird,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "aber OHNE IRGENDEINE GARANTIE,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "sogar ohne die implizite Garantie der MARKTREIFE",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "oder der VERWENDBARKEIT F\232R EINEN BESTIMMTEN ZWECK.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "Details finden Sie in der GNU General Public License.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "Sie sollten ein Exemplar der GNU General Public License",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "zusammen mit ReactOS erhalten haben.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "Falls nicht, besuchen Sie bitte",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "http://www.gnu.org/licenses",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "EINGABETASTE = Zur\201ck",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Die untere Liste zeigt die derzeitigen Ger\204teeinstellungen.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Computertyp:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Anzeige:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Tastatur:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Tastaturlayout:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
  /*{
        24,
        16,
        "Akzeptieren:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },*/
    {
        25,
        16, "Diese Ger\204teeinstellungen akzeptieren",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Sie k\224nnen die Einstellungen durch die PFEILTASTEN ausw\204hlen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "Dr\201cken Sie die EINGABETASTE, um eine Einstellung zu \204ndern.",
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
        "Wenn alle Einstellungen korrekt sind, w\204hlen Sie \"Diese Ger\204te-",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "einstellungen akzeptieren\" und best\204tigen mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortsetzen   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Der Installationsassistent ist noch der Entwicklungsphase.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Er unterst\201tzt noch nicht alle Funktionen eines vollst\204ndig",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "nutzbaren Setups.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Die Reparaturfunktionen sind noch nicht implementiert.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "\x07  Dr\201cken Sie U, um ReactOS zu aktualisieren.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Dr\201cken Sie R, f\201r die Wiederherstellungskonsole.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Dr\201cken Sie ESC, um zur Hauptseite zur\201ckzukehren.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        22,
        "\x07  Dr\201cken Sie die EINGABETASTE, um den Computer neu zu starten.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = Zur\201ck  U = Aktualisieren  R = Wiederherst.  EINGABETASTE = Neustart",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
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
        "\x07  Press ESC to continue with a new installation.",
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

static MUI_ENTRY deDEComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Den zu installierenden Computertyp einstellen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Dr\201cken Sie die PFEILTASTEN, um den gew\201nschten",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Typ zu w\204hlen. Best\204tigen Sie mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ohne den Computertyp zu \204ndern.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortsetzen   ESC = Zur\201ck  F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Die geschrieben Daten werden \201berpr\201ft.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Dies kann einige Zeit in Anspruch nehmen.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Der PC wird automatisch neu gestartet, sobald der Vorgang beendet ist.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Der Zwischenspeicher wird geleert",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS wurde nicht vollst\204ndig installiert.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Entfernen Sie alle Datentr\204ger aus den CD-Laufwerken.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Dr\201cken Sie die EINGABETASTE, um den Computer neu zu starten.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Bitte warten...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Sie wollen den zu installierenden Bildschirmtyp \204ndern.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
        "\x07  Benutzen Sie die PFEILTASTEN, um den gew\201nschten",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Typ zu w\204hlen. Best\204tigen Sie mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren, ohne",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   den Bildschirmtyp zu \204ndern.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EEINGABETASTE = Fortsetzen   ESC = Zur\201ck  F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Die Grundkomponenten von ReactOS wurden erfolgreich installiert.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Entfernen Sie alle Datentr\204ger aus den CD-Laufwerken.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Dr\201cken Sie die EINGABETASTE, um den Computer neu zu starten.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Computer neu starten",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Der Bootsektor konnte nicht auf der",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Festplatte Ihres Computers installiert werden.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Bitte legen Sie eine formatierte Diskette in Laufwerk A: ein und",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "dr\201cken Sie die EINGABETASTE.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "EINGABETASTE = Fortsetzen   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY deDESelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Diese Liste zeigt die existierenden Partitionen und ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "freien Speicherplatz f\201r neue Partitionen auf der Festplatte an.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Benutzen Sie die PFEILTASTEN, um eine Partition auszuw\204hlen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Best\204tigen Sie Ihre Auswahl mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  P erstellt eine prim\204re Partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  E erstellt eine erweiterte Partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  L erstellt eine logische Partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  D l\224scht eine vorhandene Partition.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Bitte warten...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "   not be deleted.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formatierung der Partition",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        10,
        "Die gew\201nschte Partition wird nun formatiert.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        11,
        "Dr\201cken Sie die EINGABETASTE, um fortzufahren.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortfahren   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY deDEInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Die Installationsdateien werden auf die ausgew\204hlte Partition kopiert.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "W\204hlen Sie ein Installationsverzeichnis f\201r ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Geben Sie den Namen des Verzeichnisses an.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "Benutzen Sie die R\201ck-TASTE, um Zeichen zu l\224schen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        17,
        "Best\204tigen Sie die Eingabe mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortfahren   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "Die ben\224tigten Dateien werden in das Installationsverzeichnis kopiert.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "Dieser Vorgang kann einige Zeit in Anspruch nehmen -",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "Bitte haben Sie einen Moment Geduld.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Bitte warten...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Bestimmen Sie, wo der Bootloader installiert werden soll:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Bootloader auf der Festplatte installieren (MBR und VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Bootloader auf der Festplatte installieren (nur VBR)",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Bootloader auf einer Diskette installieren",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Bootloader nicht installieren",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortfahren   F3 = Abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Sie wollen den zu installierenden Tastaturtyp \204ndern.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Benutzen Sie die PFEILTASTEN, um den gew\201nschten Typ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    zu w\204hlen. Best\204tigen Sie Ihre Auswahl mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ohne den Tastaturtyp zu \204ndern.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortfahren   ESC = Abbrechen   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDELayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Bitte w\204hlen Sie das Standardtastaturlayout aus.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Benutzen Sie die PFEILTASTEN, um den gew\201nschten Typ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   zu w\204hlen. Best\204tigen Sie Ihre Auswahl mit der EINGABETASTE.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren,",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ohne das Tastaturlayout zu \204ndern.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "EINGABETASTE = Fortfahren   ESC = Abbrechen   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY deDEPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Der Computer wird f\201r die Installation vorbereitet.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Erstelle Liste der zu kopierenden Dateien...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY deDESelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "W\204hlen Sie ein Dateisystem aus der folgenden Liste aus.",
        0
    },
    {
        8,
        19,
        "\x07  Dr\201cken Sie die PFEILTASTEN, um das Dateisystem zu \204ndern.",
        0
    },
    {
        8,
        21,
        "\x07  Dr\201cken Sie die EINGABETASTE, um die Partition zu formatieren.",
        0
    },
    {
        8,
        23,
        "\x07  Dr\201cken Sie ESC, um eine andere Partition auszuw\204hlen.",
        0
    },
    {
        0,
        0,
        "EINGABETASTE = Fortfahren   ESC = Zur\201ck   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Sie haben sich entschieden, diese Partition zu l\224schen",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Dr\201cken Sie L, um die Partition zu l\224schen.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "Warnung: Alle Daten auf dieser Partition werden gel\224scht!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ESC um abzubrechen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "L = L\224sche Partition   ESC = Abbrechen   F3 = Installation abbrechen",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDERegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Systemkonfiguration wird aktualisiert.",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "Registrierungseintr\204ge erstellen...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR deDEErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Erfolg\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS wurde nicht vollst\204ndig auf Ihrem System installiert.\n"
        "Wenn Sie die Installation jetzt beenden, m\201ssen Sie diese\n"
        "erneut starten, um ReactOS zu installieren.\n"
        "\n"
        "  \x07  Dr\201cken Sie die EINGABETASTE, um die Installation fortzusetzen.\n"
        "  \x07  Dr\201cken Sie F3, um die Installation zu beenden.",
        "F3 = Beenden  EINGABETASTE = Fortsetzen"
    },
    {
        // ERROR_NO_HDD
        "Es konnte keine Festplatte gefunden werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Es konnte kein Installationsmedium gefunden werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "TXTSETUP.SIF konnte nicht gefunden werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "TXTSETUP.SIF scheint besch\204digt zu sein.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Es wurde eine ung\201ltige Signatur in TXTSETUP.SIF gefunden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Es konnten keine Laufwerksinformationen abgefragt werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_WRITE_BOOT,
        "Der %S-Bootcode konnte nicht auf der Partition installiert werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Computertypenliste konnte nicht geladen werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Displayeinstellungsliste konnte nicht geladen werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Tastaturtypenliste konnte nicht geladen werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Die Liste der Tastaturlayouts konnte nicht geladen werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_WARN_PARTITION,
        "Es wurde mindestens eine Festplatte mit einer inkompatiblen\n"
        "Partitionstabelle gefunden, die nicht richtig verwendet werden kann.\n"
        "\n"
        "\216nderungen an den Partitionen k\224nnen die Partitionstabelle zerst\224ren!\n"
        "\n"
        "  \x07  Dr\201cken Sie F3, um die Installation zu beenden.\n"
        "  \x07  Dr\201cken Sie die EINGABETASTE, um die Installation fortzusetzen.",
        "F3 = Beenden  ENTER = EINGABETASTE"
    },
    {
        // ERROR_NEW_PARTITION,
        "Sie k\224nnen keine neue Partition in einer bereits\n"
        "vorhandenen Partition erstellen!\n"
        "\n"
        "  * * Eine beliebige Taste zum Fortsetzen dr\201cken.",
        NULL
    },
    {
        // ERROR_DELETE_SPACE,
        "Sie k\224nnen unpartitionierten Speicher nicht l\224schen!\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen dr\201cken.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Der %S-Bootcode konnte nicht auf der Partition installiert werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_NO_FLOPPY,
        "Keine Diskette in Laufwerk A: gefunden.",
        "EINGABETASTE = Fortsetzen"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Das Tastaturlayout konnte nicht aktualisiert werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Die Registrierungseintr\204ge der Anzeigeeinstellungen\n"
        "konnten nicht aktualisiert werden.",
        "EINGABETASTER = Computer neu starten"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Es konnte keine Hive-Datei importiert werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_FIND_REGISTRY
        "Die Registrierungsdateien konnten nicht gefunden werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_CREATE_HIVE,
        "Die Zweige in der Registrierung konnten nicht erstellt werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Die Registrierung konnte nicht initialisiert werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Das CAB-Archiv besitzt keine g\201ltige INF-Datei.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_CABINET_MISSING,
        "Das CAB-Archiv wurde nicht gefunden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Das CAB-Archiv enth\204lt kein Setup-Skript.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_COPY_QUEUE,
        "Die Liste mit den zu kopierenden Dateien\n"
        "konnte nicht gefunden werden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_CREATE_DIR,
        "Die Installationspfade konnten nicht erstellt werden.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Setup konnte die '%S'-Sektion in\n"
        "TXTSETUP.SIF nicht finden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_CABINET_SECTION,
        "Setup konnte die '%S'-Sektion im\n"
        "CAB-Archiv nicht finden.\n",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Setup konnte den Installationspfad nicht erstellen.",
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Die Partitionstabellen konnten nicht geschrieben werden.\n"
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Es konnte kein Codepage-Eintrag hinzugef\201gt werden.\n"
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Die Systemsprache konnte nicht eingestellt werden.\n"
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Die Tastaturlayouts konnten nicht in die Registrierung\n"
        "eingetragen werden.\n"
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Der geografische Standort konnte nicht eingestellt werden.\n"
        "EINGABETASTE = Computer neu starten"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Unzul\204ssiger Verzeichnisname.\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen dr\201cken."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Die gew\204hlten Partition ist nicht gro\341 genug, um ReactOS zu installieren.\n"
        "Die Installationspartition muss mindestens %lu MB gro\341 sein.\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen dr\201cken.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Sie k\224nnen keine weitere prim\204re oder erweiterte Partition in\n"
        "der Partitionstabelle erstellen, weil die Tabelle voll ist.\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen dr\201cken."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Sie k\224nnen nur eine erweiterte Partition auf jeder Festplatte anlegen.\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen dr\201cken."
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

MUI_PAGE deDEPages[] =
{
    {
        SETUP_INIT_PAGE,
        deDESetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        deDELanguagePageEntries
    },
    {
        WELCOME_PAGE,
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
        UPGRADE_REPAIR_PAGE,
        deDEUpgradePageEntries
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
        SELECT_PARTITION_PAGE,
        deDESelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        deDEConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        deDESelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        deDEFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        deDEDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        deDEInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        deDEPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        deDEFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        deDEKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        deDEBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        deDELayoutSettingsEntries
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
        REGISTRY_PAGE,
        deDERegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING deDEStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Bitte warten..."},
    {STRING_INSTALLCREATEPARTITION,
     "  EINGABETASTE = Installieren  P = Prim\204re  E = Erweiterte  F3 = Installation abbr."},
    {STRING_INSTALLCREATELOGICAL,
     "  EINGABETASTE = Installieren  L = Logisches Laufwerk  F3 = Installation abbr."},
    {STRING_INSTALLDELETEPARTITION,
     "  EINGABETASTE = Installieren  D = Partition l\224schen  F3 = Installation abbr."},
    {STRING_DELETEPARTITION,
     "   D = Partition l\224schen  F3 = Installation abbrechen"},
    {STRING_PARTITIONSIZE,
     "Gr\224\341e der neuen Partition:"},
    {STRING_CHOOSENEWPARTITION,
     "Eine prim\204re Partition soll hier erstellt werden:"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Eine erweiterte Partition soll hier erstellt werden:"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Ein logisches Laufwerk soll hier erstellt werden:"},
    {STRING_HDDSIZE,
    "Bitte geben Sie die Gr\224\341e der neuen Partition in Megabyte ein."},
    {STRING_CREATEPARTITION,
     "  EINGABETASTE = Partition erstellen  ESC = Abbrechen  F3 = Installation abbr."},
    {STRING_PARTFORMAT,
    "Diese Partition wird als n\204chstes formatiert."},
    {STRING_NONFORMATTEDPART,
    "Sie wollen ReactOS auf einer neuen/unformatierten Partition installieren."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Die Systempartition ist noch nicht formartiert."},
    {STRING_NONFORMATTEDOTHERPART,
    "Die neue Partition ist noch nicht formatiert."},
    {STRING_INSTALLONPART,
    "ReactOS wird auf dieser Partition installiert."},
    {STRING_CHECKINGPART,
    "Die ausgew\204hlte Partition wird \201berpr\201ft."},
    {STRING_CONTINUE,
    "EINGABETASTE = Fortsetzen"},
    {STRING_QUITCONTINUE,
    "F3 = Beenden  EINGABETASTE = Fortsetzen"},
    {STRING_REBOOTCOMPUTER,
    "EINGABETASTE = Computer neu starten"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Kopiere Datei: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Dateien werden kopiert..."},
    {STRING_REGHIVEUPDATE,
    "   Registrierungseintr\204ge werden aktualisiert..."},
    {STRING_IMPORTFILE,
    "   Importiere %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Anzeigeeinstellungen werden aktualisiert..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Lokalisierungseinstellungen werden aktualisiert..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Tastaturlayouteinstellungen werden aktualisiert..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Codepage-Informationen werden hinzugef\201gt..."},
    {STRING_DONE,
    "   Fertig..."},
    {STRING_REBOOTCOMPUTER2,
    "   EINGABETASTE = Computer neu starten"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Konsole konnte nicht ge\224ffnet werden\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Der h\204ufigste Grund hierf\201r ist die Verwendung einer USB-Tastatur\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB-Tastaturen werden noch nicht vollst\204ndig unterst\201tzt\r\n"},
    {STRING_FORMATTINGDISK,
    "Ihre Festplatte wird formatiert"},
    {STRING_CHECKINGDISK,
    "Ihre Festplatte wird \201berpr\201ft"},
    {STRING_FORMATDISK1,
    " Partition mit dem %S-Dateisystem formatieren (Schnell) "},
    {STRING_FORMATDISK2,
    " Partition mit dem %S-Dateisystem formatieren "},
    {STRING_KEEPFORMAT,
    " Dateisystem beibehalten (Keine Ver\204nderungen) "},
    {STRING_HDINFOPARTCREATE_1,
    "%I64u %s  Festplatte %lu  (Port=%hu, Bus=%hu, Id=%hu) auf %wZ [%s]."},
    {STRING_HDINFOPARTCREATE_2,
    "%I64u %s  Festplatte %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTDELETE_1,
    "auf %I64u %s  Festplatte %lu  (Port=%hu, Bus=%hu, Id=%hu) auf %wZ [%s]."},
    {STRING_HDINFOPARTDELETE_2,
    "auf %I64u %s  Festplatte %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]."},
    {STRING_HDINFOPARTZEROED_1,
    "Festplatte %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK4,
    "%c%c  Typ 0x%02X    %I64u %s"},
    {STRING_HDINFOPARTEXISTS_1,
    "auf Festplatte %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %sTyp %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT_1,
    "%6lu %s  Festplatte %lu  (Port=%hu, Bus=%hu, Id=%hu) auf %wZ [%s]"},
    {STRING_HDINFOPARTSELECT_2,
    "%6lu %s  Festplatte %lu  (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_NEWPARTITION,
    "Setup erstellte eine neue Partition auf"},
    {STRING_UNPSPACE,
    "    %sUnpartitionierter Speicher%s     %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Erweiterte Partition"},
    {STRING_UNFORMATTED,
    "Neu (Unformatiert)"},
    {STRING_FORMATUNUSED,
    "Ungenutzt"},
    {STRING_FORMATUNKNOWN,
    "Unbekannt"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Tastaturlayout hinzuf\201gen"},
    {0, 0}
};
