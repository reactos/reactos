/*
 *      translated by Caemyr (Jan,2008)
 *      Use ReactOS forum PM or IRC to contact me
 *      http://www.reactos.org
 *      IRC: irc.freenode.net #reactos-pl;
 */


#ifndef LANG_PL_PL_H__
#define LANG_PL_PL_H__

static MUI_ENTRY plPLLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Wybór jêzyka",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Proszê wybraæ jêzyk dla procesu instalacji systemu",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   i nacisn¹æ ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Wybrany jêzyk bêdzie domyœlnym dla zainstalowanego systemu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja  F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Witamy w programie instalacyjnym systemu ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Na tym etapie, instalator skopiuje niezbêdne pliki systemu operacyjnego",
        TEXT_NORMAL
    },
    {
        6,
        12,
        " na twój komputer i przygotuje kolejny etap procesu instalacji.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Naciœnij ENTER aby zainstalowaæ system ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Naciœnij R aby naprawiæ zainstalowany system ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Naciœnij L aby zapoznaæ siê z licencj¹ i warunkami korzystania z ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Naciœnij F3 aby wyjœæ bez instalacji systemu ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Wiêcej informacji o systemie ReactOS, mo¿na znaleŸæ na stronie:",
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
        "   ENTER = Kontynuacja  R = Naprawa F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Instalator ReactOS wci¹¿ jest we wczesnej fazie rozwoju. Nadal nie",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "obs³uguje wszystkich funkcji, niezbêdnych dla programu instalacyjnego.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Najwa¿niejsze ograniczenia:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Instalator nie obs³uguje wiêcej ni¿ jednej partycji Podstawowej na dysku.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Instalator nie mo¿e skasowaæ partycji podstawowej z dysku",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  o ile nadal znajduje siê na nim tak¿e partycja Rozszerzona.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Instalator nie mo¿e skasowaæ pierwszej partycji Rozszerzonej z dysku",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  o ile nadal znajduj¹ siê na nim kolejne partycje Rozszerzone.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Instalator obs³uguje jedynie system plików FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Brakuje sprawdzenia poprawnoœci systemu plików.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Naciœnij ENTER aby zainstalowaæ system ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Naciœnij F3 aby wyjœæ bez instalacji systemu ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        6,
        "Licencja:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "System ReactOS jest licencjonowany na warunkach licencji",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "GNU GPL z elementami kodu pochodz¹cego z kompatybilnych",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licencji, takich jak X11 czy BSD albo GNU LGPL. Ca³e",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "oprogramowanie, bêd¹ce czêsci¹ system ReactOS podlega wiêc",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "licencji GNU GPL jak i odpowiedniej licencji oryginalnej.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "To oprogramowanie wydawane jest BEZ JAKIEJKOLWIEK gwarancji",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "czy te¿ ograniczeñ w u¿yciu, poza przepisami prawa lokalnego",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "b¹dŸ miêdzynarodowego. Licencja systemu ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "reguluje wy³¹cznie zasady dystrybucji dla osób trzecich.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Jeœli z jakiegoœ powodu nie otrzyma³eœ kopii licencji",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU GPL wraz z systemem ReactOS, prosimy odwiedziæ stronê:",
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
        "Gwarancja:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "Niniejszy program jest wolnym oprogramowaniem; szczegó³y",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "dotycz¹ce kopiowania w Ÿród³ach. Nie ma ¯ADNEJ gwarancji",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "PRZYDATNOŒCI HANDLOWEJ LUB DO OKREŒLONYCH ZASTOSOWAÑ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Powrót",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Poni¿sza lista zawiera obecne ustawienia sprzêtu.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Komputer:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Ekran:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Klawiatura:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Uk³ad klawiatury:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         Zaakceptuj:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Zaakceptuj te ustawienia sprzêtu",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Mo¿esz zmieniæ poszczególne ustawienia za pomoc¹ klawiszy GÓRA i DÓ£",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "aby wybraæ kategoriê. Potem naciœnij ENTER by przejœæ do menu z ",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "ustawieniami szczegó³owymi do wyboru.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Kiedy wszystkie ustawienia s¹ poprawne, wybierz \"Zaakceptuj te ustawienia sprzêtu\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "i naciœnij ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Instalator ReactOS wci¹¿ jest we wczesnej fazie rozwoju. Nadal nie",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "obs³uguje wszystkich funkcji, niezbêdnych dla programu instalacyjnego.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Naprawa istniej¹cej instalacji systemu nie jest jeszcze mo¿liwa.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Naciœnij U ¿eby uaktualniæ system.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Naciœnij R by uruchomiæ Konsolê Odtwarzania.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Naciœnij ESC by powróciæ do g³ównego menu.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Naciœnij ENTER by zrestartowaæ komputer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Menu g³ówne  ENTER = Restart",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY plPLComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Aby zmieniæ typ komputera, na którym chcesz zainstalowaæ",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  naciœnij klawisz GÓRA albo DÓ£ by wybraæ odpowiedni typ komputera.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Nastêpnie naciœnij ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciœnij ESC key aby powróciæ do poprzedniej strony bez zmiany",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu komputera.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "System obecnie sprawdza czy dane s¹ poprawnie zapisane na dysku",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "To mo¿e zaj¹æ minutê.",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Po zakoñczeniu, system zrestartuje komputer automatycznie.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Czyszczenie pamieci Cache",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS zosta³ ju¿ poprawnie zainstalowany",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Upewnij siê ze w Napêdzie A: nie ma dyskietki i",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "w napêdach optycznych - ¿adnych CD-ROMów.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Naciœnij ENTER by zrestartowaæ komputer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Proszê czekaæ ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcesz zmieniæ rozdzielczoœæ ekranu.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Naciœnij klawisz GÓRA albo DÓ£ by wybraæ odpowiedni typ komputera.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Nastêpnie naciœnij ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciœnij ESC key aby powróciæ do poprzedniej strony bez zmiany",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   rodzaju ekranu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Podstawowe sk³adniki systemu ReactOS zosta³y zainstalowane.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Upewnij siê ze w Napêdzie A: nie ma dyskietki i",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "w napêdach optycznych - ¿adnych CD-ROMów.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Naciœnij ENTER by zrestartowaæ komputer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Restart komputera",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Instalator ReactOS nie mo¿e wgraæ bootloadera na twój",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "dysk twardy",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Proszê umieœciæ sformatowan¹ dyskietkê w napêdzie A:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "i nacisn¹æ klawisz ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY plPLSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Poni¿sza lista pokazuje istniej¹ce partycje i puste",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "miejsce na nowe partycje.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Naciœnij GÓRA lub DÓ£ by wybraæ pozycjê z listy.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciœnij ENTER by zainstalowaæ ReactOS na wybranej partycji.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Naciœnij C by stworzyæ now¹ partycjê.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Naciœnij D by skasowaæ istniej¹c¹ partycjê.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Proszê czekaæ...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Formatowanie partycji",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Instalator sformatuje teraz partycjê. Naciœnij ENTER aby kontynuowaæ.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY plPLInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Instalator przegra pliki systemu na wybran¹ partycjê. Wybierz",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "katalog do którego chcesz zainstalowaæ system ReactOS:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "Aby zmieniæ domyœlny katalog, u¿yj klawisza BACKSPACE by skasowaæ",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "a nastêpnie wprowadŸ now¹ œcie¿kê do katalogu, do którego system",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "ma zostaæ zainstalowany.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "Proszê czekaæ, Instalator ReactOS kopiuje pliki do wybranego",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "katalogu.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "To mo¿e zaj¹æ kilka minut.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Proszê czekaæ...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Instalator musi teraz wgraæ bootloader",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Wgraj bootloader na dysk twardy (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Wgraj bootloader na dyskietkê.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Pomiñ wgrywanie bootloadera.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcesz zmieniæ typ klawiatury, jaki ma byæ zainstalowany.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Naciœnij klawisz GÓRA albo DÓ£ by wybraæ odpowiedni typ klawiatury.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Nastêpnie naciœnij ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciœnij ESC key aby powróciæ do poprzedniej strony bez zmiany",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu klawiatury.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcesz zmieniæ uk³ad klawiatury, jaki ma byæ zainstalowany.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Naciœnij klawisz GÓRA albo DÓ£ by wybraæ odpowiedni uk³ad",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    klawiatury. Nastêpnie naciœnij ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciœnij ESC key aby powróciæ do poprzedniej strony bez zmiany",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   uk³adu klawiatury.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY plPLPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Instalator przygotuje twój komputer do skopiowania plików systemu. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Tworzenie listy plików do skopiowania...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY plPLSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        17,
        "Wybierz system plików z listy poni¿ej.",
        0
    },
    {
        8,
        19,
        "\x07  Naciœnij klawisz GÓRa alub DÓ£ by wybraæ system plików.",
        0
    },
    {
        8,
        21,
        "\x07  Naciœnij ENTER aby sformatowaæ partycjê.",
        0
    },
    {
        8,
        23,
        "\x07  Naciœnij ESC aby wybraæ inn¹ partycjê.",
        0
    },
    {
        0,
        0,
        "   ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjœcie",
        TEXT_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcesz skasowaæ wybran¹ partycjê",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Naciœnij D by skasowaæ partycjê.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "UWAGA: Wszystkie dane, zapisane na tej partycji zostan¹ skasowane!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Naciœnij ESC aby anulowaæ.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Skasowanie partycji   ESC = Anulowanie   F3 = Wyjœcie",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY plPLRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Instalator uaktualnia w³aœnie konfiguracjê systemu. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Tworzenie ga³êzi rejestru...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR plPLErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS nie zosta³ w pe³ni zainstalowany na twoim\n"
        "komputerze. Jeœli wyjdziesz teraz, trzeba bêdzie\n"
        "ponownie uruchomiæ instalator by zainstalowaæ ReactOS.\n"
        "\n"
        "  \x07  Naciœnij ENTER aby kontynuowaæ instalacjê.\n"
        "  \x07  Naciœnij F3 aby wyjœæ z instalatora.",
        "F3= Quit  ENTER = Continue"
    },
    {
        //ERROR_NO_HDD
        "Instalator nie znalaz³ ¿adnego dysku twardego.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Instalator nie znalaz³ napêdu Ÿród³owego.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Instalator nie móg³ za³adowaæ pliku TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Instalator znalaz³ uszkodzony plik TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Instalator znalaz³ nieprawid³owy wpis w TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Instalator nie móg³ odczytaæ informacji o napêdzie.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_WRITE_BOOT,
        "Nieudane zapisanie FAT bootcode na partycji systemowej.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Instalator failed to load the computer type list.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Instalator nie móg³ za³adowaæ listy ustawieñ ekranu.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Instalator nie móg³ za³adowaæ listy typów klawiatury.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Instalator nie móg³ za³adowaæ listy uk³adów klawiatury.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_WARN_PARTITION,
        "Instalator wykry³, ¿e co najmniej jeden dysk twardy zawiera niekompatybiln¹ \n"
        "tablicê partycji, która nie bêdzie poprawnie obs³ugiwana!\n"
        "\n"
        "Tworzenie lub kasowanie partycji mo¿e zniszczyæ ca³¹ tablicê partycji.\n"
        "\n"
        "  \x07  Naciœnij F3 aby wyjœæ z instalatora."
        "  \x07  Naciœnij ENTER aby kontynuowaæ.",
        "F3= Wyjœcie  ENTER = Kontynuacja"
    },
    {
        //ERROR_NEW_PARTITION,
        "Nie mo¿esz stworzyæ nowej partycji w miejscu ju¿\n"
        "istniej¹cej!\n"
        "\n"
        "  * Naciœnij dowolny klawisz aby kontynuowaæ.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Nie mo¿esz skasowaæ pustej przestrzeni, gdzie nie ma ¿adnej partycji!\n"
        "\n"
        "  * Naciœnij dowolny klawisz aby kontynuowaæ.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Nieudana instalacja FAT bootcode na partycji systemowej.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_NO_FLOPPY,
        "Brak dyskietki w napêdzie A:.",
        "ENTER = Kontynuacja"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Instalator nie móg³ zmieniæ uk³adu klawiatury.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Instalator nie móg³ zmieniæ ustawieñ ekranu w rejestrze.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Instalator nie by³ w stanie zaimportowaæ pliku ga³êzi rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_FIND_REGISTRY
        "Instalator nie by³ w stanie znaleŸæ plików z danymi rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CREATE_HIVE,
        "Instalator nie móg³ stworzyæ ga³êzi rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Instalator nie by³ w stanie ustawic inicjalizacji rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet nie zawiera prawid³owego pliku inf.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet nie zosta³ znaleziony.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet nie zawiera skryptu instalacji.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_COPY_QUEUE,
        "Instalator nie by³ w stanie otworzyæ kolejki kopiowania pliku.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CREATE_DIR,
        "Instalator nie móg³ stworzyæ katalogów.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Instalator nie by³ w stanie znaleŸæ sekcji 'Directories'\n"
        "w pliku TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CABINET_SECTION,
        "Instalator nie by³ w stanie znaleŸæ sekcji 'Directories'\n"
        "w pliku cabinet.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Instalator nie móg³ stworzyæ katalogu instalacji.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Instalator nie by³ w stanie znaleŸæ sekcji 'SetupData'\n"
        "w pliku TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Instalator nie móg³ zapisaæ zmian w tablicy partycji.\n"
        "ENTER = Restart komputera"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE plPLPages[] =
{
    {
        LANGUAGE_PAGE,
        plPLLanguagePageEntries
    },
    {
        START_PAGE,
        plPLWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        plPLIntroPageEntries
    },
    {
        LICENSE_PAGE,
        plPLLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        plPLDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        plPLRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        plPLComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        plPLDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        plPLFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        plPLSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        plPLSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        plPLFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        plPLDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        plPLInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        plPLPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        plPLFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        plPLKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        plPLBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        plPLLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        plPLQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        plPLSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        plPLBootPageEntries
    },
    {
        REGISTRY_PAGE,
        plPLRegistryEntries
    },
    {
        -1,
        NULL
    }
};

#endif
