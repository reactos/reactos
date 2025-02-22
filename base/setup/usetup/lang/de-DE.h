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
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Bitte warten Sie w\204hrend das ReactOS Setup startet",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "und Ihre Ger\204te detektiert...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Bitte warten...",
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

static MUI_ENTRY deDELanguagePageEntries[] =
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
        "Sprachauswahl",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Bitte w\204hlen Sie die Sprache, die Sie w\204hrend der Installation",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        11,
        "verwenden wollen. Best\204tigen Sie die Auswahl mit ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Diese Sprache wird sp\204ter als Standardsprache im System verwendet.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDEWelcomePageEntries[] =
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
        "Willkommen zum ReactOS-Setup",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Dieser Teil des Setups kopiert das ReactOS-Betriebssystem auf Ihren",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Computer und bereitet die n\204chsten Schritte vor.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Dr\201cken Sie ENTER, um ReactOS zu installieren / aktualisieren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Dr\201cken Sie R, um ReactOS zu reparieren. (Wiederherstellungskonsole)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Dr\201cken Sie L, um das Lizenzabkommen von ReactOS zu lesen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Dr\201cken Sie F3, um die Installation abzubrechen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Weitere Informationen erhalten Sie unter:",
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
        "ENTER = Fortsetzen   R = Reparieren   L = Lizenz   F3 = Beenden",
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

static MUI_ENTRY deDEIntroPageEntries[] =
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
        "ReactOS Version Status",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS befindet sich noch im Alpha Stadium. Noch sind nicht alle",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "wichtigen Funktionen implementiert und Sie m\201ssen mit dem Auftreten",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "schwerwiegender Fehler rechnen. Verwenden Sie es daher nur",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "zu Testzwecken auf einem Zweitcomputer, nicht als Produktivsystem!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Sichern Sie unbedingt vorher ihre bestehenden Daten!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Dr\201cken Sie ENTER, um das ReactOS Setup fortzusetzen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Dr\201cken Sie F3, um abzubrechen ohne ReactOS zu installieren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   F3 = Abbrechen",
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

static MUI_ENTRY deDELicensePageEntries[] =
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
        6,
        "Lizenz:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS ist unter den Bedingungen der GNU General Public License",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "lizenziert. Einige Teile von ReactOS stehen unter dazu kompatiblen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "Lizenzen wie der BSD- oder GNU LGPL-Lizenz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Alle Softwarebestandteile in ReactOS sind daher unter der",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "GNU GPL ver\224ffentlicht, behalten daneben aber ihre",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "urspr\201nglichen Lizenzen bei.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "ReactOS ist freie Software. Die Ver\224ffentlichung dieses Programms",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "erfolgt in der Hoffnung, dass es Ihnen von Nutzen sein wird,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "aber OHNE IRGENDEINE GARANTIE,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "sogar ohne die implizite Garantie der MARKTREIFE",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "oder der VERWENDBARKEIT F\232R EINEN BESTIMMTEN ZWECK.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "Details finden Sie in der GNU General Public License.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "Sie sollten ein Exemplar der GNU General Public License",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        23,
        "zusammen mit ReactOS erhalten haben.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Falls nicht, besuchen Sie bitte",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "http://www.gnu.org/licenses",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Zur\201ck",
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

static MUI_ENTRY deDEDevicePageEntries[] =
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
        "Die untere Liste zeigt die derzeitigen Ger\204teeinstellungen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Computertyp:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Anzeige:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Tastatur:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Tastaturlayout:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
  /*{
        24,
        16,
        "Akzeptieren:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },*/
    {
        25,
        16, "Diese Ger\204teeinstellungen akzeptieren",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Sie k\224nnen die Einstellungen durch die PFEILTASTEN ausw\204hlen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "Dr\201cken Sie ENTER, um eine Einstellung zu \204ndern.",
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
        "Wenn alle Einstellungen korrekt sind, w\204hlen Sie \"Diese Ger\204te-",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "einstellungen akzeptieren\" und best\204tigen mit ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDERepairPageEntries[] =
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
        "Der Installationsassistent ist noch der Entwicklungsphase.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Er unterst\201tzt noch nicht alle Funktionen eines vollst\204ndig",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "nutzbaren Setups.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Die Reparaturfunktionen sind noch nicht implementiert.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "\x07  Dr\201cken Sie U, um ReactOS zu aktualisieren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Dr\201cken Sie R, f\201r die Wiederherstellungskonsole.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Dr\201cken Sie ESC, um zur Hauptseite zur\201ckzukehren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "\x07  Dr\201cken Sie ENTER, um den Computer neu zu starten.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Zur\201ck   U = Aktualisieren   R = Wiederherst.   ENTER = Neustart",
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

static MUI_ENTRY deDEUpgradePageEntries[] =
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
        "Das ReactOS Setup kann eine der unten aufgef\201hrten Installationen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "aktualisieren, oder versuchen,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "eine bestehende ReactOS Installation zu reparieren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Die Reparaturfunktionen sind noch nicht alle implementiert.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  W\204hlen Sie mit den PFEILTASTEN die gew\201nschte Installation aus.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Dr\201cken Sie U, um die gew\204hlte Installation zu aktualisieren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Dr\201cken Sie ESC, um eine Neuinstallation vorzunehmen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Dr\201cken Sie F3, um abzubrechen ohne ReactOS zu installieren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Aktualisieren   ESC = Neuinstallation   F3 = Abbrechen",
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

static MUI_ENTRY deDEComputerPageEntries[] =
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
        "Den zu installierenden Computertyp einstellen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Dr\201cken Sie die PFEILTASTEN, um den gew\201nschten",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Typ zu w\204hlen. Best\204tigen Sie mit ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   ohne den Computertyp zu \204ndern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   ESC = Zur\201ck   F3 = Installation abbrechen",
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

static MUI_ENTRY deDEFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Die geschrieben Daten werden \201berpr\201ft.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Dies kann einige Zeit in Anspruch nehmen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Der PC startet automatisch neu, sobald der Vorgang beendet ist.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Der Zwischenspeicher wird geleert",
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

static MUI_ENTRY deDEQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS wurde nicht vollst\204ndig installiert.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Entfernen Sie alle Datentr\204ger aus den CD-Laufwerken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Dr\201cken Sie ENTER, um den Computer neu zu starten.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Bitte warten...",
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

static MUI_ENTRY deDEDisplayPageEntries[] =
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
        "Sie wollen den zu installierenden Bildschirmtyp \204ndern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Benutzen Sie die PFEILTASTEN, um den gew\201nschten",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Typ zu w\204hlen. Best\204tigen Sie mit ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren, ohne",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   den Bildschirmtyp zu \204ndern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   ESC = Zur\201ck   F3 = Installation abbrechen",
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

static MUI_ENTRY deDESuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Die Grundkomponenten von ReactOS wurden erfolgreich installiert.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Entfernen Sie alle Datentr\204ger aus den CD-Laufwerken.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Dr\201cken Sie ENTER, um den Computer neu zu starten.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Computer neu starten",
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

static MUI_ENTRY deDESelectPartitionEntries[] =
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
        "Diese Liste zeigt die existierenden Partitionen und ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "freien Speicherplatz f\201r neue Partitionen auf der Festplatte an.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Benutzen Sie die PFEILTASTEN, um eine Partition auszuw\204hlen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  ENTER best\204tigt Ihre Auswahl.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  C erstellt eine prim\204re/logische Partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  E erstellt eine erweiterte Partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  D l\224scht eine vorhandene Partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Bitte warten...",
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

static MUI_ENTRY deDEChangeSystemPartition[] =
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
        "Die derzeitige aktive Partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "auf dem Systemlaufwerk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "verwendet eine von ReactOS nicht unterst\201tzte Formatierung.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "Um ReactOS installieren zu k\224nnen, muss das Setup Programm",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "die Partition \204ndern, die derzeit als aktiv markiert ist.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Folgende Partition wird k\201nftig die aktive Systempartition sein:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Dr\201cken Sie ENTER, um diesen Vorschlag zu akzeptieren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Dr\201cken Sie ESC, um manuell eine andere Systempartition festzulegen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   Erstellen Sie dazu eine neue Partition oder w\204hlen eine existierende in der folgenden Liste",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   der Partitionen auf dem Systemlaufwerk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "Falls sich noch weitere Betriebssysteme auf die alte Systempartition beziehen,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "k\224nnen Sie entweder manuell versuchen, diese nachtr\204glich anzupassen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "oder Sie markieren die alte aktive Partition wieder als aktiv",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "nachdem das ReactOS Setup abgeschlossen wurde.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   ESC = Abbrechen",
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

static MUI_ENTRY deDEConfirmDeleteSystemPartitionEntries[] =
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
        "Sind Sie sicher, dass Sie die aktive Systempartition l\224schen wollen?",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Systempartitionen enthalten oft wichtige Diagnose-Programme oder",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Ger\204tetreiber, die ben\224tigt werden, um den Computer starten",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "oder einen fr\201heren Zustand wiederherstellen zu k\224nnen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "L\224schen Sie die aktive Systempartition nur falls Sie sicher sind,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "dass sich keine derartigen Programme auf dieser Partition befinden oder",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "dass Sie keinerlei Daten von dieser Partition l\204nger ben\224tigen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Erst nach Abschluss des Setup wird der Computer wieder startf\204hig sein.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Dr\201cken Sie ENTER, um die aktive Systempartition zu l\224schen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   Das L\224schen der Partition muss ein weiteres Mal best\204tigt werden.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   Die Partition wird nicht gel\224scht.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   ESC = Abbrechen",
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

static MUI_ENTRY deDEFormatPartitionEntries[] =
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
        "Formatierung der Partition",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Die gew\201nschte Partition wird nun formatiert.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        6,
        12,
        "Dr\201cken Sie ENTER, um fortzusetzen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDECheckFSEntries[] =
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
        "Die ausgew\204hlte Partition wird \201berpr\201ft.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Bitte warten...",
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

static MUI_ENTRY deDEInstallDirectoryEntries[] =
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
        "Die Installationsdateien werden auf die ausgew\204hlte Partition kopiert.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "W\204hlen Sie ein Installationsverzeichnis f\201r ReactOS:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Geben Sie den Namen des Verzeichnisses an.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Benutzen Sie die R\201ck-TASTE, um Zeichen zu l\224schen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Best\204tigen Sie die Eingabe mit ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDEFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Die ben\224tigten Dateien werden in das Installationsverzeichnis kopiert.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "Dieser Vorgang kann einige Zeit in Anspruch nehmen -",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Bitte haben Sie einen Moment Geduld.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Bitte warten...    ",
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

static MUI_ENTRY deDEBootLoaderSelectPageEntries[] =
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
        "Bestimmen Sie, wo Setup den Bootloader installieren soll:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "Bootloader auf der Festplatte installieren (MBR und VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Bootloader auf der Festplatte installieren (nur VBR)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Bootloader auf einer Diskette installieren",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Bootloader nicht installieren",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   F3 = Abbrechen",
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

static MUI_ENTRY deDEBootLoaderInstallPageEntries[] =
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
        "Setup installiert den Bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Bootloader wird installiert. Bitte warten...",
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

static MUI_ENTRY deDEBootLoaderRemovableDiskPageEntries[] =
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
        "Setup installiert den Bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Bitte legen Sie eine formatierte Diskette in Laufwerk A: ein",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "und dr\201cken Sie ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDEKeyboardSettingsEntries[] =
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
        "Sie wollen den zu installierenden Tastaturtyp \204ndern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Benutzen Sie die PFEILTASTEN, um den gew\201nschten Typ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   zu w\204hlen. Best\204tigen Sie Ihre Auswahl mit ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   ohne den Tastaturtyp zu \204ndern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   ESC = Abbrechen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDELayoutSettingsEntries[] =
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
        "Bitte w\204hlen Sie das Standardtastaturlayout aus.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Benutzen Sie die PFEILTASTEN, um den gew\201nschten Typ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   zu w\204hlen. Best\204tigen Sie Ihre Auswahl mit ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Dr\201cken Sie ESC, um zur vorherigen Seite zur\201ckzukehren,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   ohne das Tastaturlayout zu \204ndern.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Fortsetzen   ESC = Abbrechen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDEPrepareCopyEntries[] =
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
        "Der Computer wird f\201r die Installation vorbereitet.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Erstelle Liste der zu kopierenden Dateien...",
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

static MUI_ENTRY deDESelectFSEntries[] =
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
        "\x07  Dr\201cken Sie ENTER, um die Partition zu formatieren.",
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
        "ENTER = Fortsetzen   ESC = Zur\201ck   F3 = Installation abbrechen",
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

static MUI_ENTRY deDEDeletePartitionEntries[] =
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
        "Sie haben sich entschieden, diese Partition zu l\224schen",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Dr\201cken Sie L, um die Partition zu l\224schen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "Warnung: Alle Daten auf dieser Partition werden gel\224scht!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ESC um abzubrechen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = L\224sche Partition   ESC = Abbrechen   F3 = Installation abbrechen",
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

static MUI_ENTRY deDERegistryEntries[] =
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
        "Systemkonfiguration wird aktualisiert.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Registrierungseintr\204ge erstellen...",
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
        "  \x07  Dr\201cken Sie ENTER, um die Installation fortzusetzen.\n"
        "  \x07  Dr\201cken Sie F3, um die Installation zu beenden.",
        "F3 = Beenden   ENTER = Fortsetzen"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Die Installationspfade f\201r das ReactOS Installationsverzeichnis konnten nicht erstellt werden!\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_SOURCE_PATH
        "Die Partition kann nicht gel\224scht werden, weil sie die Installationsquellen beherbergt!\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_SOURCE_DIR
        "ReactOS kann nicht in dasselbe Verzeichnis installiert werden, in dem die Installationsquellen liegen!\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_NO_HDD
        "Es konnte keine Festplatte gefunden werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Es konnte kein Installationsmedium gefunden werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "TXTSETUP.SIF konnte nicht gefunden werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "TXTSETUP.SIF scheint besch\204digt zu sein.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Es wurde eine ung\201ltige Signatur in TXTSETUP.SIF gefunden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Es konnten keine Laufwerksinformationen abgefragt werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_WRITE_BOOT,
        "Der %S-Bootcode konnte nicht auf der Partition installiert werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Computertypenliste konnte nicht geladen werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Displayeinstellungsliste konnte nicht geladen werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Tastaturtypenliste konnte nicht geladen werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Die Liste der Tastaturlayouts konnte nicht geladen werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_WARN_PARTITION,
        "Es wurde mindestens eine Festplatte mit einer inkompatiblen\n"
        "Partitionstabelle gefunden, die nicht richtig verwendet werden kann.\n"
        "\n"
        "\216nderungen an den Partitionen k\224nnen die Partitionstabelle zerst\224ren!\n"
        "\n"
        "  \x07  Dr\201cken Sie F3, um die Installation zu beenden.\n"
        "  \x07  Dr\201cken Sie ENTER, um die Installation fortzusetzen.",
        "F3 = Beenden   ENTER = Fortsetzen"
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
        // ERROR_INSTALL_BOOTCODE,
        "Der %S-Bootcode konnte nicht auf der Partition installiert werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_NO_FLOPPY,
        "Keine Diskette in Laufwerk A: gefunden.",
        "ENTER = Fortsetzen"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Das Tastaturlayout konnte nicht aktualisiert werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Die Registrierungseintr\204ge der Anzeigeeinstellungen\n"
        "konnten nicht aktualisiert werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Es konnte keine Hive-Datei importiert werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_FIND_REGISTRY
        "Die Registrierungsdateien konnten nicht gefunden werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_CREATE_HIVE,
        "Die Zweige in der Registrierung konnten nicht erstellt werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Die Registrierung konnte nicht initialisiert werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Das CAB-Archiv besitzt keine g\201ltige INF-Datei.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_CABINET_MISSING,
        "Das CAB-Archiv wurde nicht gefunden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Das CAB-Archiv enth\204lt kein Setup-Skript.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_COPY_QUEUE,
        "Die Liste mit den zu kopierenden Dateien\n"
        "konnte nicht gefunden werden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_CREATE_DIR,
        "Die Installationspfade konnten nicht erstellt werden.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Setup konnte die '%S'-Sektion in\n"
        "TXTSETUP.SIF nicht finden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_CABINET_SECTION,
        "Setup konnte die '%S'-Sektion im\n"
        "CAB-Archiv nicht finden.\n",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Setup konnte den Installationspfad nicht erstellen.",
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Die Partitionstabellen konnten nicht geschrieben werden.\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Es konnte kein Codepage-Eintrag hinzugef\201gt werden.\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Die Systemsprache konnte nicht eingestellt werden.\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Die Tastaturlayouts konnten nicht in die Registrierung\n"
        "eingetragen werden.\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Der geografische Standort konnte nicht eingestellt werden.\n"
        "ENTER = Computer neu starten"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Unzul\204ssiger Verzeichnisname.\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen dr\201cken."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Die gew\204hlte Partition ist nicht gro\341 genug, um ReactOS zu installieren.\n"
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
        "Setup konnte die Partition nicht formatieren:\n"
        " %S\n"
        "\n"
        "ENTER = Computer neu starten"
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
        CHANGE_SYSTEM_PARTITION,
        deDEChangeSystemPartition
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
        CHECK_FILE_SYSTEM_PAGE,
        deDECheckFSEntries
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
        BOOTLOADER_SELECT_PAGE,
        deDEBootLoaderSelectPageEntries
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
        BOOTLOADER_INSTALL_PAGE,
        deDEBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        deDEBootLoaderRemovableDiskPageEntries
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
     "  ENTER = Installieren   C = Prim\204re   E = Erweiterte   F3 = Installation abbr."},
    {STRING_INSTALLCREATELOGICAL,
     "  ENTER = Installieren   C = Logische Partition   F3 = Installation abbrechen"},
    {STRING_INSTALLDELETEPARTITION,
     "  ENTER = Installieren   D = Partition l\224schen   F3 = Installation abbrechen"},
    {STRING_DELETEPARTITION,
     "   D = Partition l\224schen   F3 = Installation abbrechen"},
    {STRING_PARTITIONSIZE,
     "Gr\224\341e der neuen Partition:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Eine prim\204re Partition soll hier erstellt werden:"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Eine erweiterte Partition soll hier erstellt werden:"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Eine logische Partition soll hier erstellt werden:"},
    {STRING_HDPARTSIZE,
    "Bitte geben Sie die Gr\224\341e der neuen Partition in Megabyte ein."},
    {STRING_CREATEPARTITION,
     "  ENTER = Partition erstellen   ESC = Abbrechen   F3 = Installation abbrechen"},
    {STRING_NEWPARTITION,
    "Setup erstellte eine neue Partition auf"},
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
    {STRING_CONTINUE,
    "ENTER = Fortsetzen"},
    {STRING_QUITCONTINUE,
    "F3 = Beenden   ENTER = Fortsetzen"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Computer neu starten"},
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
    "   ENTER = Computer neu starten"},
    {STRING_REBOOTPROGRESSBAR,
    " Der Computer wird in %li Sekunde(n) neugestartet... "},
    {STRING_CONSOLEFAIL1,
    "Konsole konnte nicht ge\224ffnet werden\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Der h\204ufigste Grund hierf\201r ist die Verwendung einer USB-Tastatur\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB-Tastaturen werden noch nicht vollst\204ndig unterst\201tzt\r\n"},
    {STRING_FORMATTINGPART,
    "Die Partition wird formatiert..."},
    {STRING_CHECKINGDISK,
    "Die Festplatte wird \201berpr\201ft..."},
    {STRING_FORMATDISK1,
    " Partition mit dem %S-Dateisystem formatieren (Schnell) "},
    {STRING_FORMATDISK2,
    " Partition mit dem %S-Dateisystem formatieren "},
    {STRING_KEEPFORMAT,
    " Dateisystem beibehalten (Keine Ver\204nderungen) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "auf %s."},
    {STRING_PARTTYPE,
    "Typ 0x%02x"},
    {STRING_HDDINFO1,
    // "Festplatte %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Festplatte %lu (Port=%hu, Bus=%hu, Id=%hu) auf %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Festplatte %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Festplatte %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Unpartitionierter Speicher"},
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
