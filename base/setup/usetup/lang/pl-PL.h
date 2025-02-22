// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/*
 *      translated by Caemyr (Jan-Feb, Apr, 2008)
 *      Use ReactOS forum PM or IRC to contact me
 *      https://reactos.org/
 *      IRC: irc.freenode.net #reactos-pl;
 *      Updated by Wojo664 (July, 2014)
 *      Updated by Saibamen (July, 2015)
 *      Updated by Piotr Hetnarowicz (June, 2021)
 */

#pragma once

static MUI_ENTRY plPLSetupInitPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Poczekaj, a\276 uruchomi si\251 Instalator systemu ReactOS",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "i wykryje wszystkie twoje urz\245dzenia...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Prosz\251 czeka\206...",
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

static MUI_ENTRY plPLLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Wyb\242r j\251zyka",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Prosz\251 wybra\206 j\251zyk dla procesu instalacji systemu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   i nacisn\245\206 ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Wybrany j\251zyk b\251dzie domy\230lnym dla zainstalowanego systemu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja  F3 = Wyj\230cie",
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

static MUI_ENTRY plPLWelcomePageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Witamy w programie instalacyjnym systemu ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        " Na tym etapie, instalator skopiuje niezb\251dne pliki systemu operacyjnego",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "na tw\242j komputer i rozpocznie kolejny etap procesu instalacji.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Naci\230nij ENTER, aby zainstalowa\206 lub uaktualni\206 system ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Naci\230nij R, aby naprawi\206 istniej\245c\245 instalacj\251 systemu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Naci\230nij L, aby zapozna\206 si\251 z licencj\245 systemu ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Naci\230nij F3, aby wyj\230\206 bez instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Wi\251cej informacji o systemie ReactOS mo\276na znale\253\206 na stronie:",
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
        "ENTER = Kontynuacja  R = Naprawa F3 = Wyj\230cie",
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

static MUI_ENTRY plPLIntroPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Stan rozwoju systemu ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "System ReactOS jest w fazie Alpha, co oznacza, \276e jest niekompletny",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "i wci\245\276 intensywnie rozwijany. Zaleca si\251 u\276ywania systemu wy\210\245cznie",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "w celach ewauluacji i testowania, nie jako system codziennego u\276ytku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Wykonaj kopi\251 zapasow\245 danych lub testuj na dodatkowym komputerze,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "je\230li pr\242bujesz uruchomi\206 system ReactOS poza maszyn\245 wirtualn\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Naci\230nij ENTER, aby zainstalowa\206 system ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Naci\230nij F3, aby wyj\230\206 bez instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licencja:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "System ReactOS jest licencjonowany na warunkach licencji",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL z elementami kodu pochodz\245cego z kompatybilnych",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licencji, takich jak X11 czy BSD albo GNU LGPL. Ca\210e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "oprogramowanie, b\251d\245ce cz\251\230ci\245 systemu ReactOS podlega wi\251c",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "licencji GNU GPL jak i odpowiedniej licencji oryginalnej.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "To oprogramowanie wydawane jest BEZ JAKIEJKOLWIEK gwarancji",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "czy te\276 ogranicze\344 w u\276yciu, poza przepisami prawa lokalnego",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "b\245d\253 mi\251dzynarodowego. Licencja systemu ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "reguluje wy\210\245cznie zasady dystrybucji dla os\242b trzecich.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Je\230li z jakiego\230 powodu nie otrzyma\210e\230 kopii licencji",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "GNU GPL wraz z systemem ReactOS, prosimy odwiedzi\206 stron\251:",
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
        "Gwarancja:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Niniejszy program jest wolnym oprogramowaniem; szczeg\242\210y",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "dotycz\245ce kopiowania w \253r\242d\210ach. Nie ma \275ADNEJ gwarancji",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "PRZYDATNO\227CI HANDLOWEJ LUB DO OKRE\227LONYCH ZASTOSOWA\343",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Powr\242t",
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

static MUI_ENTRY plPLDevicePageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Poni\276sza lista zawiera obecne ustawienia sprz\251tu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Komputer:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Monitor:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Klawiatura:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Uk\210. klawiatury:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Zapisz:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Akceptuj ustawienia sprz\251tu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Mo\276esz zmieni\206 poszczeg\242lne ustawienia za pomoc\245 klawiszy G\340RA i D\340\235,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "aby wybra\206 kategori\251. Potem naci\230nij ENTER, by przej\230\206 do menu z ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "ustawieniami szczeg\242\210owymi do wyboru.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Gdy wszystkie ustawienia s\245 poprawne, wybierz: ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "\"Akceptuj ustawienia sprz\251tu\" i naci\230nij ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLRepairPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalator systemu ReactOS wci\245\276 jest we wczesnej fazie rozwoju. Nadal nie",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "obs\210uguje wszystkich funkcji, niezb\251dnych dla programu instalacyjnego.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Naprawa istniej\245cej instalacji systemu nie jest jeszcze mo\276liwa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Naci\230nij U \276eby uaktualni\206 system.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Naci\230nij R, by uruchomi\206 Konsol\251 Odtwarzania.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Naci\230nij ESC, by powr\242ci\206 do g\210\242wnego menu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Naci\230nij ENTER, by uruchomi\206 ponownie komputer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Menu g\210\242wne  ENTER = Ponowne uruchomienie komputera",
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

static MUI_ENTRY plPLUpgradePageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR "  ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalator mo\276e uaktualni\206 jedn\245 z poni\276szych instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "Je\230li instalacja systemu ReactOS jest uszkodzona, instalator mo\276e",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "podj\245\206 pr\242b\251 jej naprawy.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Funkcje naprawcze nie s\245 jeszcze zaimplementowane.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  U\276ywaj\245c klawiszy G\340RA lub D\340\235, wybierz instalacj\251 systemu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Naci\230nij U, aby uaktualni\206 wybran\245 instalacj\251 systemu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Naci\230nij ESC, aby wykona\206 now\245 instalacj\251 systemu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Naci\230nij F3, aby wyj\230\206 bez instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Uaktualnienie   ESC = Nowa instalacja   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Aby zmieni\206 typ komputera, na kt\242rym chcesz zainstalowa\206 system ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  U\276ywaj\245c klawiszy G\340RA lub D\340\235, wybierz odpowiedni typ komputera.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   a nast\251pnie naci\230nij ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Naci\230nij klawisz ESC, aby powr\242ci\206 do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   typu komputera.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLFlushPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "System ReactOS sprawdza, czy dane s\245 poprawnie zapisane na dysku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "To mo\276e potrwa\206 kilka minut.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Po zako\344czeniu, komputer automatycznie zostanie ponownie uruchomiony.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Czyszczenie pami\251ci podr\251cznej",
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

static MUI_ENTRY plPLQuitPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "System ReactOS nie zosta\210 zainstalowany na tym komputerze.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Upewnij si\251 \276e w nap\251dzie A: nie ma dyskietki, a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "w nap\251dach optycznych - \276adnych p\210yt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Naci\230nij ENTER, by ponownie uruchomi\206 komputer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Prosz\251 czeka\206...",
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

static MUI_ENTRY plPLDisplayPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcesz zmieni\206 rozdzielczo\230\206 monitora.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  U\276ywaj\245c klawiszy G\340RA lub D\340\235, wybierz rozdzielczo\230\206 i llo\230\206",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   kolor\242w, a nast\251pnie naci\230nij ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Naci\230nij ESC, aby powr\242ci\206 do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   rozdzielczo\230ci czy ilo\230ci kolor\242w.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLSuccessPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Podstawowe sk\210adniki systemu ReactOS zosta\210y zainstalowane.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Upewnij si\251 \276e w nap\251dzie A: nie ma dyskietki, a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "w nap\251dach optycznych - \276adnych p\210yt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Naci\230nij ENTER, by ponownie uruchomi\206 komputer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Ponowne uruchomienie komputera",
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

static MUI_ENTRY plPLSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Poni\276sza lista pokazuje istniej\245ce partycje i nieprzydzielone",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "miejsce na nowe partycje.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  U\276ywaj\245c klawiszy G\340RA lub D\340\235, wybierz pozycj\251 z listy.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Naci\230nij ENTER, by zainstalowa\206 system ReactOS na wybranej partycji.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Naci\230nij C, by utworzy\206 partycj\251 podstawow\245/logiczn\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Naci\230nij E, by utworzy\206 partycj\251 rozszerzon\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Naci\230nij D, by usun\245\206 istniej\245c\245 partycj\251.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Prosz\251 czeka\206...",
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

static MUI_ENTRY plPLChangeSystemPartition[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Bie\276\245ca partycja systemowa twojego komputera",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "na dysku systemowym",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "u\276ywa formatu nie obs\210ugiwanego przez system ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "Aby pomy\230lnie zainstalowa\206 system ReactOS, Instalator musi zmieni\206",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "bi\251\276\245c\245 partycj\251 systemow\245 na now\245.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Nowa proponowana partycja systemowa to:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Naci\230nij ENTER, aby zaakceptowa\206 ten wyb\242r.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Je\230li chcesz r\251cznie zmieni\206 partycj\251 systemow\245, nacisnij ESC, by powr\242ci\206",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   do listy wyboru partycji, i wybierz istniej\245c\245 lub utw\242rz now\245 partycj\251",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   systemow\245 na dysku systemowym.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "Je\230li na twoim komputerze s\245 zainstalowane inne systemy operacyjne, kt\242re",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "wymagaj\245 poprzedniej partycji systemowej, mo\276e zaistnie\206 potrzeba",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "skonfigurowania ich dla nowej partycji systemowej, albo zmiana partycji",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "systemowej do poprzednej po zako\344czeniu instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie",
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

static MUI_ENTRY plPLConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Za\276\245dano usuni\251cia partycji systemowej",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "Partycje systemowe mog\245 zawiera\206 programy diagnostyczne, konfiguruj\245ce",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "urz\245dzenia, programy uruchamiaj\245ce systemy operacyjne (na przyk\210ad",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "ReactOS) i inne programy dostarczane przez producent\242w.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Partycj\251 systemow\245 mo\276esz usun\245\206 tylko wtedy, gdy masz pewno\230\206, \276e nie",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "zawiera ona takich program\242w albo gdy chcesz je usun\245\206. Usuni\251cie",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "partycji systemowej mo\276e uniemo\276liwi\206 uruchomienie komputera z dysku",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "twardego do czasu zako\344czenia instalacji systemu ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Naci\230nij ENTER, aby usun\245\206 t\251 partycj\251. Instalator wy\230wietli monit",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   o potwierdzenie, przed usuni\251ciem tej partycji.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Naci\230nij ESC, aby powr\242ci\206 do poprzedniego ekranu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   nie usuwaj\245c partycji.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja  ESC = Anulowanie",
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

static MUI_ENTRY plPLFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Formatowanie partycji",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Instalator sformatuje teraz partycj\251. Naci\230nij ENTER, aby kontynuowa\206.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyj\230cie",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        0
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY plPLCheckFSEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalator sprawdza wybran\245 partycj\251.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Prosz\251 czeka\206...",
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

static MUI_ENTRY plPLInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalator skopiuje pliki systemu na wybran\245 partycj\251. Wybierz",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "katalog, do kt\242rego chcesz zainstalowa\206 system ReactOS:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Aby zmieni\206 domy\230lny katalog, u\276yj klawisza BACKSPACE, by usun\245\206",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "a nast\251pnie wprowad\253 now\245 \230cie\276k\251 do katalogu, do kt\242rego system",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "ma zosta\206 zainstalowany.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLFileCopyEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        11,
        12,
        "Prosz\251 czeka\206, Instalator systemu ReactOS kopiuje pliki do wybranego",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        30,
        13,
        "katalogu.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        20,
        14,
        "To mo\276e potrwa\206 kilka minut.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Prosz\251 czeka\206...    ",
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

static MUI_ENTRY plPLBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
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
        "Instaluj mened\276er rozruchu na dysku twardym (MBR i VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Instaluj mened\276er rozruchu na dysku twardym (tylko VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Instaluj mened\276er rozruchu na dyskietce.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Pomi\344 instalacj\251 mened\276era rozruchu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLBootLoaderInstallPageEntries[] =
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
        "Instalator musi teraz zainstalowa\206 mened\276er rozruchu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Instalacja mened\276era rozruchu na no\230niku, prosz\251 czeka\206...",
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

static MUI_ENTRY plPLBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalator musi teraz zainstalowa\206 mened\276er rozruchu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "W\210\242\276 sformatowan\245 dyskietk\251 do nap\251du A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "i nacisnij klawisz ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcesz zmieni\206 typ klawiatury, jaki ma by\206 zainstalowany.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  U\276ywaj\245c klawiszy G\340RA lub D\340\235, wybierz odpowiedni typ klawiatury.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   a nast\251pnie naci\230nij ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Naci\230nij ESC, aby powr\242ci\206 do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   typu klawiatury.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Wybierz domy\230lnie instalowany uk\210ad klawiatury.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  U\276ywaj\245c klawiszy G\340RA lub D\340\235, wybierz odpowiedni uk\210ad",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   klawiatury. Nast\251pnie naci\230nij ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Naci\230nij ESC, aby powr\242ci\206 do poprzedniej strony bez zmiany",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   uk\210adu klawiatury.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalator przygotuje tw\242j komputer do skopiowania plik\242w systemu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Tworzenie listy plik\242w do skopiowania...",
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

static MUI_ENTRY plPLSelectFSEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Wybierz system plik\242w z listy poni\276ej.",
        0
    },
    {
        8,
        19,
        "\x07  U\276ywaj\245c klawiszy G\340RA lub D\340\235, wybierz system plik\242w.",
        0
    },
    {
        8,
        21,
        "\x07  Naci\230nij ENTER, aby sformatowa\206 partycj\251.",
        0
    },
    {
        8,
        23,
        "\x07  Naci\230nij ESC, aby wybra\206 inn\245 partycj\251.",
        0
    },
    {
        0,
        0,
        "ENTER = Kontynuacja   ESC = Anulowanie   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Chcesz usun\245\206 wybran\245 partycj\251.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Naci\230nij L, by usun\245\206 partycj\251.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "UWAGA: Wszystkie dane, zapisane na tej partycji zostan\245 utracone!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Naci\230nij ESC, aby anulowa\206.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Usuni\251cie partycji   ESC = Anulowanie   F3 = Wyj\230cie",
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

static MUI_ENTRY plPLRegistryEntries[] =
{
    {
        4,
        3,
        " Instalator ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Instalator uaktualnia w\210a\230nie konfiguracj\251 systemu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Tworzenie ga\210\251zi rejestru...",
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

MUI_ERROR plPLErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Sukces\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "System ReactOS nie zosta\210 w pe\210ni zainstalowany na twoim\n"
        "komputerze. Je\230li wyjdziesz teraz, trzeba b\251dzie\n"
        "ponownie uruchomi\206 instalatora, by zainstalowa\206 system ReactOS.\n"
        "\n"
        "  \x07  Naci\230nij ENTER, aby kontynuowa\206 instalacj\251.\n"
        "  \x07  Naci\230nij F3, aby wyj\230\206 z instalatora.",
        "F3 = Wyj\230cie  ENTER = Kontynuacja"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Nie mo\276na utworzy\206 scie\276ek instalacji dla katalogu instalacji systemu ReactOS!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_PATH
        "Nie mo\276na usun\245\206 partycji zawieraj\245cej \253r\242d\210o instalacji!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_DIR
        "Nie mo\276na zainstalowa\206 systemu ReactOS w katalogu \253r\242d\210owym instalacji!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_NO_HDD
        "Instalator nie wykry\210 \276adnego dysku twardego.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Instalator nie wykry\210 nap\251du \253r\242d\210owego.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Instalator nie m\242g\210 za\210adowa\206 pliku TXTSETUP.SIF.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Instalator wykry\210, \276e TXTSETUP.SIF jest uszkodzony.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Instalator wykry\210 nieprawid\210owy wpis w pliku TXTSETUP.SIF.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Instalator nie m\242g\210 odczyta\206 informacji o nap\251dzie.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_WRITE_BOOT,
        "Instalator nie m\242g\210 zapisa\206 kodu rozruchowego FAT na partycji systemowej.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Instalator nie m\242g\210 za\210adowa\206 listy typ\242w komputera.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Instalator nie m\242g\210 za\210adowa\206 listy ustawie\344 monitora.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Instalator nie m\242g\210 za\210adowa\206 listy typ\242w klawiatury.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Instalator nie m\242g\210 za\210adowa\206 listy uk\210ad\242w klawiatury.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_WARN_PARTITION,
        "Instalator wykry\210, \276e co najmniej jeden dysk twardy zawiera niekompatybiln\245\n"
        "tabel\251 partycji, kt\242ra nie b\251dzie poprawnie obs\210ugiwana!\n"
        "\n"
        "Tworzenie lub usuwanie partycji mo\276e nieodwracalnie uszkodi\206 tabel\251 partycji.\n"
        "\n"
        "  \x07  Naci\230nij F3, aby wyj\230\206 z instalatora.\n"
        "  \x07  Naci\230nij ENTER, aby kontynuowa\206.",
        "F3 = Wyj\230cie  ENTER = Kontynuacja"
    },
    {
        // ERROR_NEW_PARTITION,
        "Nie mo\276na utworzy\206 nowej partycji w miejscu ju\276\n"
        "istniej\245cej!\n"
        "\n"
        "  * Naci\230nij dowolny klawisz, aby kontynuowa\206.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Instalator nie m\242g\210 zapisa\206 kodu rozruchowego %S na partycji systemowej.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_NO_FLOPPY,
        "Brak dyskietki w nap\251dzie A:.",
        "ENTER = Kontynuacja"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Instalator nie m\242g\210 zmieni\206 uk\210adu klawiatury.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Instalator nie m\242g\210 zmieni\206 ustawie\344 monitora w rejestrze.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Instalator nie by\210 w stanie zaimportowa\206 pliku ga\210\251zi rejestru.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_FIND_REGISTRY
        "Instalator nie m\242g\210 odnale\253\206 plik\242w z danymi rejestru.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_CREATE_HIVE,
        "Instalator nie m\242g\210 utworzy\206 ga\210\251zi rejestru.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Instalator nie m\242g\210 ustawi\206 inicjalizacji rejestru.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Plik Cabinet nie zawiera prawid\210owego pliku informacji instalatora.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_CABINET_MISSING,
        "Plik Cabinet nie zosta\210 odnaleziony.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Plik Cabinet nie zawiera skryptu instalacji.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_COPY_QUEUE,
        "Instalator nie m\242g\210 otworzy\206 kolejki kopiowania pliku.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_CREATE_DIR,
        "Instalator nie m\242g\210 utworzy\206 katalog\242w.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Instalator nie m\242g\210 odnale\253\206 sekcji '%S'\n"
        "w pliku TXTSETUP.SIF.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_CABINET_SECTION,
        "Instalator nie m\242g\210 odnale\253\206 sekcji '%S'\n"
        "w pliku Cabinet.\n",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Instalator nie m\242g\210 utworzy\206 katalogu instalacji.",
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Instalator nie m\242g\210 zapisa\206 zmian w tablicy partycji.\n"
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Instalator nie m\242g\210 doda\206 strony kodowania do rejestru.\n"
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Instalator nie m\242g\210 ustawi\206 wersji j\251zykowej.\n"
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Instalator nie m\242g\210 doda\206 uk\210ad\242w klawiatury do rejestru.\n"
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Instalator nie m\242g\210 ustawi\206 lokalizacji geograficznej.\n"
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Nieprawid\210owa nazwa katalogu.\n"
        "\n"
        "  * Naci\230nij dowolny klawisz, aby kontynuowa\206."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Wybrana partycja nie jest wystarczaj\245co du\276a, aby zainstalowa\206 system ReactOS.\n"
        "Partycja instalacyjna musi mie\206 rozmiar co najmniej %lu MB.\n"
        "\n"
        "  * Naci\230nij dowolny klawisz, aby kontynuowa\206.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Nie mo\276na utworzy\206 partycji podstawowej lub rozszerzonej,\n" // FIXME
        "poniewa\276 tabela partycji na tym dysku jest pe\210na.\n"
        "\n"
        "  * Naci\230nij dowolny klawisz, aby kontynuowa\206."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Nie mo\276na utworzy\206 wi\251cej ni\276 jednej partycji rozszerzonej na dysku.\n"
        "\n"
        "  * Naci\230nij dowolny klawisz, aby kontynuowa\206."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Instalator nie m\242g\210 sformatowa\206 partycji:\n"
        " %S\n"
        "\n"
        "ENTER = Ponowne uruchomienie komputera"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE plPLPages[] =
{
    {
        SETUP_INIT_PAGE,
        plPLSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        plPLLanguagePageEntries
    },
    {
        WELCOME_PAGE,
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
        UPGRADE_REPAIR_PAGE,
        plPLUpgradePageEntries
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
        CHANGE_SYSTEM_PARTITION,
        plPLChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        plPLConfirmDeleteSystemPartitionEntries
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
        CHECK_FILE_SYSTEM_PAGE,
        plPLCheckFSEntries
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
        BOOTLOADER_SELECT_PAGE,
        plPLBootLoaderSelectPageEntries
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
        BOOTLOADER_INSTALL_PAGE,
        plPLBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        plPLBootLoaderRemovableDiskPageEntries
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
     "   Prosz\251 czeka\206..."},
    {STRING_INSTALLCREATEPARTITION,
     "  ENTER = Instalacja   C = Partycja podstawowa   E = Rozszerzona   F3 = Wyj\230cie"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Instalacja   C = Utworzenie partycji logicznej   F3 = Wyj\230cie"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Instalacja   D = Usuni\251cie partycji   F3 = Wyj\230cie"},
    {STRING_DELETEPARTITION,
     "   D = Usuni\251cie partycji   F3 = Wyj\230cie"},
    {STRING_PARTITIONSIZE,
     "Rozmiar nowej partycji:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Wybrane: utworzenie nowej partycji podstawowej na"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Wybrane: utworzenie nowej partycji rozszerzonej na"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Wybrane: utworzenie nowej partycji logicznej na"},
    {STRING_HDPARTSIZE,
    "Prosz\251 wprowadzi\206 rozmiar nowej partycji w megabajtach."},
    {STRING_CREATEPARTITION,
     "   ENTER = Utworzenie partycji   ESC = Anulowanie   F3 = Wyj\230cie"},
    {STRING_NEWPARTITION,
    "Instalator utworzy\210 now\245 partycj\251"},
    {STRING_PARTFORMAT,
    "Nast\251puj\245ca partycja zostanie sformatowana."},
    {STRING_NONFORMATTEDPART,
    "Mo\276esz zainstalowa\206 system ReactOS na nowej lub niesformatowanej partycji."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Partycja systemowa nie jest jeszcze sformatowana."},
    {STRING_NONFORMATTEDOTHERPART,
    "Nowa partycja nie jest jeszcze sformatowana."},
    {STRING_INSTALLONPART,
    "Instalator kopiuje pliki systemu na wybran\245 partycj\251."},
    {STRING_CONTINUE,
    "ENTER = Kontynuacja"},
    {STRING_QUITCONTINUE,
    "F3 = Wyj\230cie  ENTER = Kontynuacja"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Ponowne uruchomienie komputera"},
    {STRING_DELETING,
     "   Usuwanie pliku: %S"},
    {STRING_MOVING,
     "   Przenoszenie pliku z: %S do: %S"},
    {STRING_RENAMING,
     "   Zmienianie nazwy pliku z: %S na: %S"},
    {STRING_COPYING,
     "   Kopiowanie plik\242w: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Instalator kopiuje pliki..."},
    {STRING_REGHIVEUPDATE,
    "   Uaktualnianie..."},
    {STRING_IMPORTFILE,
    "   Importowanie %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Zmiana ustawie\344 monitora w rejestrze..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Zmiana wersji j\251zykowej..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Zmiana uk\210adu klawiatury..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Dodawanie informacji o stronie kodowej..."},
    {STRING_DONE,
    "   Uko\344czone..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Ponowne uruchomienie komputera"},
    {STRING_REBOOTPROGRESSBAR,
    " Komputer zostanie uruchomiony ponownie za %li sekund(y)... "},
    {STRING_CONSOLEFAIL1,
    "Otwarcie konsoli nieudane\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Najcz\251stsz\245 tego przyczyn\245 jest u\276ycie klawiatury USB.\r\n"},
    {STRING_CONSOLEFAIL3,
    "Nie s\245 obecnie w pe\210ni obs\210ugiwane.\r\n"},
    {STRING_FORMATTINGPART,
    "Instalator formatuje partycj\251..."},
    {STRING_CHECKINGDISK,
    "Instalator sprawdza dysk..."},
    {STRING_FORMATDISK1,
    " Formatuj partycj\251 w systemie plik\242w %S (szybkie formatowanie) "},
    {STRING_FORMATDISK2,
    " Formatuj partycj\251 w systemie plik\242w %S "},
    {STRING_KEEPFORMAT,
    " Zachowaj obecny system plik\242w (bez zmian) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "na: %s."},
    {STRING_PARTTYPE,
    "Typ 0x%02x"},
    {STRING_HDDINFO1,
    // "Dysk twardy %lu (%I64u %s), Port=%hu, Szyna=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Dysk twardy %lu (Port=%hu, Szyna=%hu, Id=%hu) na %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Dysk twardy %lu (%I64u %s), Port=%hu, Szyna=%hu, Id=%hu [%s]"
    "%I64u %s Dysk twardy %lu (Port=%hu, Szyna=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Miejsce poza partycjami"},
    {STRING_MAXSIZE,
    "MB (maks. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Partycja rozszerzona"},
    {STRING_UNFORMATTED,
    "Nowa (niesformatowana)"},
    {STRING_FORMATUNUSED,
    "Nieu\276yte"},
    {STRING_FORMATUNKNOWN,
    "Nieznane"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Dodawanie uk\210ad\242w klawiatury"},
    {0, 0}
};
