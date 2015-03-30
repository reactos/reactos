/*
 *      translated by Caemyr (Jan-Feb, Apr, 2008)
 *      Use ReactOS forum PM or IRC to contact me
 *      http://www.reactos.org
 *      IRC: irc.freenode.net #reactos-pl;
 *      Updated by Saibamen and Wojo664 (July, 2014)
 */

#pragma once

MUI_LAYOUTS plPLLayouts[] =
{
    { L"0415", L"00000415" },
    { L"0415", L"00010415" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY plPLLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Wyb¢r j©zyka",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Prosz© wybraÜ j©zyk dla procesu instalacji systemu",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   i nacisn•Ü ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Wybrany j©zyk b©dzie domyòlnym dla zainstalowanego systemu.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja  F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Witamy w programie instalacyjnym systemu ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        " Na tym etapie, instalator skopiuje niezb©dne pliki systemu operacyjnego",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "na tw¢j komputer i rozpocznie kolejny etap procesu instalacji.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Naciònij ENTER, aby zainstalowaÜ system ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Naciònij R, aby naprawiÜ zainstalowany system ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Naciònij L, aby zapoznaÜ si© z licencj• ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Naciònij F3, aby wyjòÜ bez instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Wi©cej informacji o systemie ReactOS moæna znale´Ü na stronie:",
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
        "ENTER = Kontynuacja  R = Naprawa F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalator ReactOS wci•æ jest we wczesnej fazie rozwoju. Nadal nie",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "obsàuguje wszystkich funkcji, niezb©dnych dla programu instalacyjnego.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Najwaæniejsze ograniczenia:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Instalator obsàuguje jedn• partycj© Podstawow• na dysku.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Instalator nie moæe skasowaÜ partycji podstawowej z dysku",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  o ile nadal znajduje si© na nim takæe partycja Rozszerzona.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Instalator nie moæe skasowaÜ pierwszej partycji Rozszerzonej z dysku",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  o ile nadal znajduj• si© na nim kolejne partycje Rozszerzone.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Instalator obsàuguje jedynie system plik¢w FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- Brakuje sprawdzenia poprawnoòci systemu plik¢w.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Naciònij ENTER, aby zainstalowaÜ system ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Naciònij F3, aby wyjòÜ bez instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyjòcie",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licencja:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "System ReactOS jest licencjonowany na warunkach licencji",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL z elementami kodu pochodz•cego z kompatybilnych",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licencji, takich jak X11 czy BSD albo GNU LGPL. Caàe",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "oprogramowanie, b©d•ce cz©òci• systemu ReactOS podlega wi©c",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "licencji GNU GPL jak i odpowiedniej licencji oryginalnej.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "To oprogramowanie wydawane jest BEZ JAKIEJKOLWIEK gwarancji",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "czy teæ ogranicze‰ w uæyciu, poza przepisami prawa lokalnego",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "b•d´ mi©dzynarodowego. Licencja systemu ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "reguluje wyà•cznie zasady dystrybucji dla os¢b trzecich.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Jeòli z jakiegoò powodu nie otrzymaàeò kopii licencji",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU GPL wraz z systemem ReactOS, prosimy odwiedziÜ stron©:",
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
        "Gwarancja:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Niniejszy program jest wolnym oprogramowaniem; szczeg¢ày",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "dotycz•ce kopiowania w ´r¢dàach. Nie ma ΩADNEJ gwarancji",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "PRZYDATNOóCI HANDLOWEJ LUB DO OKREóLONYCH ZASTOSOWA„",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Powr¢t",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Poniæsza lista zawiera obecne ustawienia sprz©tu.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "Komputer:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "Ekran:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "Klawiatura:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "Ukà. klawiatury:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "Zapisz:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "Akceptuj ustawienia sprz©tu",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Moæesz zmieniÜ poszczeg¢lne ustawienia za pomoc• klawiszy G‡RA i D‡ù,",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "aby wybraÜ kategori©. Potem naciònij ENTER, by przejòÜ do menu z ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "ustawieniami szczeg¢àowymi do wyboru.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "Gdy wszystkie ustawienia s• poprawne, wybierz: ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "\"Akceptuj ustawienia sprz©tu\" i naciònij ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalator ReactOS wci•æ jest we wczesnej fazie rozwoju. Nadal nie",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "obsàuguje wszystkich funkcji, niezb©dnych dla programu instalacyjnego.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Naprawa istniej•cej instalacji systemu nie jest jeszcze moæliwa.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Naciònij U æeby uaktualniÜ system.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Naciònij R, by uruchomiÜ Konsol© Odtwarzania.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Naciònij ESC, by powr¢ciÜ do gà¢wnego menu.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Naciònij ENTER, by zrestartowaÜ komputer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = Menu gà¢wne  ENTER = Restart",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Aby zmieniÜ typ komputera, na kt¢rym chcesz zainstalowaÜ ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Naciònij klawisz G‡RA albo D‡ù, by wybraÜ odpowiedni typ komputera.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   a nast©pnie naciònij ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciònij klawisz ESC, aby powr¢ciÜ do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   typu komputera.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "System obecnie sprawdza czy dane s• poprawnie zapisane na dysku",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "To moæe zaj•Ü minut©.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Po zako‰czeniu, system zrestartuje komputer automatycznie.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Czyszczenie pami©ci Cache",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS zostaà juæ poprawnie zainstalowany",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Upewnij si© æe w Nap©dzie A: nie ma dyskietki i",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "w nap©dach optycznych - æadnych CD-ROM¢w.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Naciònij ENTER, by zrestartowaÜ komputer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Prosz© czekaÜ ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcesz zmieniÜ rozdzielczoòÜ ekranu.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Naciònij klawisz G‡RA albo D‡ù, by wybraÜ rozdzielczoòÜ i liczb©",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   kolor¢w, a nast©pnie naciònij ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciònij ESC, aby powr¢ciÜ do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   rozdzielczoòci czy iloòci kolor¢w.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Podstawowe skàadniki systemu ReactOS zostaày zainstalowane.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Upewnij si© æe w Nap©dzie A: nie ma dyskietki i",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "w nap©dach optycznych - æadnych CD-ROM¢w.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Naciònij ENTER, by zrestartowaÜ komputer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Restart komputera",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalator ReactOS nie moæe wgraÜ bootloadera na tw¢j",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "dysk twardy",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Prosz© umieòciÜ sformatowan• dyskietk© w nap©dzie A:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "i nacisn•Ü klawisz ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Poniæsza lista pokazuje istniej•ce partycje i puste",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "miejsce na nowe partycje.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Naciònij G‡RA lub D‡ù, by wybraÜ pozycj© z listy.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciònij ENTER, by zainstalowaÜ ReactOS na wybranej partycji.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Naciònij P, by stworzyÜ partycj© podstawow•.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Naciònij E, by stworzyÜ partycj© rozszerzon•.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Naciònij D, by skasowaÜ istniej•c• partycj©.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Prosz© czekaÜ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formatowanie partycji",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Instalator sformatuje teraz partycj©. Naciònij ENTER, aby kontynuowaÜ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY plPLInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Instalator ReactOS ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalator przegra pliki systemu na wybran• partycj©. Wybierz",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "katalog, do kt¢rego chcesz zainstalowaÜ system ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Aby zmieniÜ domyòlny katalog, uæyj klawisza BACKSPACE, by skasowaÜ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "a nast©pnie wprowad´ now• òcieæk© do katalogu, do kt¢rego system",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "ma zostaÜ zainstalowany.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Prosz© czekaÜ, Instalator ReactOS kopiuje pliki do wybranego",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        30,
        13,
        "katalogu.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        20,
        14,
        "To moæe zaj•Ü kilka minut.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 Prosz© czekaÜ...    ",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalator musi teraz wgraÜ bootloader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Wgraj bootloader na dysk twardy (MBR i VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Wgraj bootloader na dysk twardy (tylko VBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        " Wgraj bootloader na dyskietk©.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        " Pomi‰ wgrywanie bootloadera.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcesz zmieniÜ typ klawiatury, jaki ma byÜ zainstalowany.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Naciònij klawisz G‡RA albo D‡ù, by wybraÜ odpowiedni typ klawiatury.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   a nast©pnie naciònij ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciònij ESC, aby powr¢ciÜ do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   typu klawiatury.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Prosz© wybraÜ domyòlnie instalowany ukàad klawiatury.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Naciònij klawisz G‡RA albo D‡ù, by wybraÜ odpowiedni ukàad",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   klawiatury. Nast©pnie naciònij ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Naciònij ESC, aby powr¢ciÜ do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ukàadu klawiatury.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalator przygotuje tw¢j komputer do skopiowania plik¢w systemu. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Tworzenie listy plik¢w do skopiowania...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Wybierz system plik¢w z listy poniæej.",
        0
    },
    {
        8,
        19,
        "\x07  Naciònij klawisz G‡RA alub D‡ù, by wybraÜ system plik¢w.",
        0
    },
    {
        8,
        21,
        "\x07  Naciònij ENTER, aby sformatowaÜ partycj©.",
        0
    },
    {
        8,
        23,
        "\x07  Naciònij ESC, aby wybraÜ inn• partycj©.",
        0
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Chcesz skasowaÜ wybran• partycj©",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Naciònij D, by skasowaÜ partycj©.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "UWAGA: Wszystkie dane, zapisane na tej partycji zostan• skasowane!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Naciònij ESC, aby anulowaÜ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = Skasowanie partycji   ESC = Anulowanie   F3 = Wyjòcie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Instalator uaktualnia wàaònie konfiguracj© systemu. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Tworzenie gaà©zi rejestru...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        // NOT_AN_ERROR
        "Sukces\n"
    },
    {
        //ERROR_NOT_INSTALLED
        "ReactOS nie zostaà w peàni zainstalowany na twoim\n"
        "komputerze. Jeòli wyjdziesz teraz, trzeba b©dzie\n"
        "ponownie uruchomiÜ instalator, by zainstalowaÜ ReactOS.\n"
        "\n"
        "  \x07  Naciònij ENTER, aby kontynuowaÜ instalacj©.\n"
        "  \x07  Naciònij F3, aby wyjòÜ z instalatora.",
        "F3= Wyjòcie  ENTER = Kontynuacja"
    },
    {
        //ERROR_NO_HDD
        "Instalator nie znalazà æadnego dysku twardego.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Instalator nie znalazà nap©du ´r¢dàowego.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Instalator nie m¢gà zaàadowaÜ pliku TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Instalator znalazà uszkodzony plik TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Instalator znalazà nieprawidàowy wpis w TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Instalator nie m¢gà odczytaÜ informacji o nap©dzie.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_WRITE_BOOT,
        "Nieudane zapisanie FAT bootcode na partycji systemowej.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Instalator nie m¢gà zaàadowaÜ listy typ¢w komputera.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Instalator nie m¢gà zaàadowaÜ listy ustawie‰ ekranu.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Instalator nie m¢gà zaàadowaÜ listy typ¢w klawiatury.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Instalator nie m¢gà zaàadowaÜ listy ukàad¢w klawiatury.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_WARN_PARTITION,
        "Instalator wykryà, æe co najmniej jeden dysk twardy zawiera niekompatybiln• \n"
        "tablic© partycji, kt¢ra nie b©dzie poprawnie obsàugiwana!\n"
        "\n"
        "Tworzenie lub kasowanie partycji moæe zniszczyÜ caà• tablic© partycji.\n"
        "\n"
        "  \x07  Naciònij F3, aby wyjòÜ z instalatora.\n"
        "  \x07  Naciònij ENTER, aby kontynuowaÜ.",
        "F3= Wyjòcie  ENTER = Kontynuacja"
    },
    {
        //ERROR_NEW_PARTITION,
        "Nie moæesz stworzyÜ nowej partycji w miejscu juæ\n"
        "istniej•cej!\n"
        "\n"
        "  * Naciònij dowolny klawisz, aby kontynuowaÜ.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Nie moæesz skasowaÜ pustej przestrzeni, gdzie nie ma æadnej partycji!\n"
        "\n"
        "  * Naciònij dowolny klawisz, aby kontynuowaÜ.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Nieudana instalacja FAT bootcode na partycji systemowej.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_NO_FLOPPY,
        "Brak dyskietki w nap©dzie A:.",
        "ENTER = Kontynuacja"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Instalator nie m¢gà zmieniÜ ukàadu klawiatury.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Instalator nie m¢gà zmieniÜ ustawie‰ ekranu w rejestrze.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Instalator nie byà w stanie zaimportowaÜ pliku gaà©zi rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_FIND_REGISTRY
        "Instalator nie byà w stanie znale´Ü plik¢w z danymi rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CREATE_HIVE,
        "Instalator nie m¢gà stworzyÜ gaà©zi rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Instalator nie byà w stanie ustawiÜ inicjalizacji rejestru.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet nie zawiera prawidàowego pliku inf.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet nie zostaà znaleziony.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet nie zawiera skryptu instalacji.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_COPY_QUEUE,
        "Instalator nie byà w stanie otworzyÜ kolejki kopiowania pliku.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CREATE_DIR,
        "Instalator nie m¢gà stworzyÜ katalog¢w.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Instalator nie byà w stanie znale´Ü sekcji 'Directories'\n"
        "w pliku TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CABINET_SECTION,
        "Instalator nie byà w stanie znale´Ü sekcji 'Directories'\n"
        "w pliku cabinet.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Instalator nie m¢gà stworzyÜ katalogu instalacji.",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Instalator nie byà w stanie znale´Ü sekcji 'SetupData'\n"
        "w pliku TXTSETUP.SIF.\n",
        "ENTER = Restart komputera"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Instalator nie m¢gà zapisaÜ zmian w tablicy partycji.\n"
        "ENTER = Restart komputera"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Instalator nie m¢gà dodaÜ strony kodowania do rejestru.\n"
        "ENTER = Restart komputera"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Instalator nie m¢gà ustawiÜ wersji j©zykowej.\n"
        "ENTER = Restart komputera"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Instalator nie m¢gà dodaÜ ukàad¢w klawiatury do rejestru.\n"
        "ENTER = Restart komputera"
    },
    {
        //ERROR_UPDATE_GEOID,
        "Instalator nie m¢gà ustawiÜ lokalizacji geograficznej.\n"
        "ENTER = Restart komputera"
    },
    {
        //ERROR_DIRECTORY_NAME,
        "Invalid directory name.\n"
        "\n"
        "  * Press any key to continue."
    },
    {
        //ERROR_INSUFFICIENT_PARTITION_SIZE,
        "The selected partition is not large enough to install ReactOS.\n"
        "The install partition must have a size of at least %lu MB.\n"
        "\n"
        "  * Naciònij dowolny klawisz, aby kontynuowaÜ.",
        NULL
    },
    {
        //ERROR_PARTITION_TABLE_FULL,
        "Nie moæna utworzyÜ na tym dysku partycji podstawowej lub\n" // FIXME
        "rozszerzonej, poniewaæ tabela partycji jest pe©na.\n"
        "\n"
        "  * Naciònij dowolny klawisz, aby kontynuowaÜ."
    },
    {
        //ERROR_ONLY_ONE_EXTENDED,
        "Nie moæesz utworzyÜ wi©cej niæ jednej partycji rozszerzonej na dysku.\n"
        "\n"
        "  * Naciònij dowolny klawisz, aby kontynuowaÜ."
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

MUI_STRING plPLStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Prosz© czekaÜ..."},
    {STRING_INSTALLCREATEPARTITION,
     "  ENTER = Instalacja   P = Partycja Podstawowa   E = Rozszerzona   F3 = Wyjòcie"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalacja   L = Utworzenie Partycji Logicznej   F3 = Wyjòcie"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalacja   D = Skasowanie Partycji   F3 = Wyjòcie"},
    {STRING_DELETEPARTITION,
     "   D = Skasowanie Partycji   F3 = Wyjòcie"},
    {STRING_PARTITIONSIZE,
     "Rozmiar nowej partycji:"},
    {STRING_CHOOSENEWPARTITION,
     "Wybrane: utworzenie nowej partycji podstawowej na"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Wybrane: utworzenie nowej partycji rozszerzonej na"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Wybrane: utworzenie nowej partycji logicznej na"},
    {STRING_HDDSIZE,
    "Prosz© wprowadziÜ rozmiar nowej partycji w megabajtach."},
    {STRING_CREATEPARTITION,
     "   ENTER = Utworzenie Partycji   ESC = Anulowanie   F3 = Wyjòcie"},
    {STRING_PARTFORMAT,
    "Nast©puj•ca partycja zostanie sformatowana."},
    {STRING_NONFORMATTEDPART,
    "Moæesz zainstalowaÜ ReactOS na nowej lub niesformatowanej partycji."},
    {STRING_INSTALLONPART,
    "Instalator kopiuje pliki systemu na wybran• partycj©."},
    {STRING_CHECKINGPART,
    "Instalator sprawdza wybran• partycj©."},
    {STRING_QUITCONTINUE,
    "F3= Wyjòcie  ENTER = Kontynuacja"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Restart komputera"},
    {STRING_TXTSETUPFAILED,
    "Instalator nie byà w stanie odnale´Ü sekcji '%S'\nw pliku TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Kopiowanie plik¢w: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Instalator kopiuje pliki..."},
    {STRING_REGHIVEUPDATE,
    "   Uaktualnianie..."},
    {STRING_IMPORTFILE,
    "   Importowanie %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Zmiana ustawie‰ ekranu w rejestrze..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Zmiana wersji j©zykowej..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Zmiana ukàadu klawiatury..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Dodawanie informacji o stronie kodowej do rejestru..."},
    {STRING_DONE,
    "   Uko‰czone..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Restart komputera"},
    {STRING_CONSOLEFAIL1,
    "Otwarcie konsoli nieudane\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Najcz©stsz• tego przyczyn• jest uæycie klawiatury USB.\r\n"},
    {STRING_CONSOLEFAIL3,
    "Nie s• obecnie w peàni obsàugiwane.\r\n"},
    {STRING_FORMATTINGDISK,
    "Instalator formatuje tw¢j dysk"},
    {STRING_CHECKINGDISK,
    "Instalator sprawdza tw¢j dysk"},
    {STRING_FORMATDISK1,
    " Sformatuj partycj© z systemem plik¢w %S (szybkie formatowanie) "},
    {STRING_FORMATDISK2,
    " Sformatuj partycj© z systemem plik¢w %S "},
    {STRING_KEEPFORMAT,
    " Zachowaj obecny system plik¢w (bez zmian) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Dysku Twardym %lu  (Port=%hu, Szyna=%hu, Id=%hu) na %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Dysk 02 Twardy %lu  (Port=%hu, Szyna=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  03Typ %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "na %I64u %s  Dysku Twardym %lu  (Port=%hu, Szyna=%hu, Id=%hu) na %wZ."},
    {STRING_HDDINFOUNK3,
    "na %I64u %s  Dysku 05 Twardym %lu  (Port=%hu, Szyna=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Dysk Twardy %lu (%I64u %s), Port=%hu, Szyna=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  07Typ %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "na Dysku Twardym %lu (%I64u %s), Port=%hu, Szyna=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c %c %s09Typ %-3u%s                      %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Dysk Twardy %lu  (Port=%hu, Szyna=%hu, Id=%hu) na %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Dysk11 Twardy %lu  (Port=%hu, Szyna=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Instalator utworzyà now• partycj©"},
    {STRING_UNPSPACE,
    "    %sMiejsce poza partycjami%s            %6lu %s"},
    {STRING_MAXSIZE,
    "MB (maks. %lu MB)"},
    {STRING_UNFORMATTED,
    "Nowa (Niesformatowana)"},
    {STRING_FORMATUNUSED,
    "Nieuæyte"},
    {STRING_FORMATUNKNOWN,
    "Nieznane"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Dodawanie ukàad¢w klawiatury"},
    {0, 0}
};
