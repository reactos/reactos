// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
#pragma once

static MUI_ENTRY huHUSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "K\202rem v\240rjon am\241g a Reactos telep\241t\213 inicializ\240lja mag\240t",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "\202s felt\202rk\202pezi az eszk\224zeit...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "K\202rem v\240rjon...",
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

static MUI_ENTRY huHULanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Nyelv kiv\240laszt\240sa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  K\202rj\201k v\240lassza ki a telep\241t\202s folyam\240n haszn\240lni k\241v\240nt nyelvet.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Majd nyomjon ENTER-t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Ez a nyelv lesz az alap\202rtelmezett a telep\241tett rendszeren.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s  F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "\232dv\224z\224lj\201k a ReactOS telep\241t\213ben",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "A telep\241t\213nek ez a r\202sze felm\240solja a ReactOS oper\240ci\242s rendszert",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "a sz\240m\241t\242g\202pre \202s el\213k\202sz\241ti a telep\241t\202s m\240sodik szakasz\240t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Nyomjon ENTER-t a ReactOS telep\241t\202s\202hez vagy friss\241t\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Nyomjon R-t egy megl\202v\213, m\240r telep\241tett ReactOS helyre\240ll\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Nyomjon L-t a licencfelt\202telek megtekint\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Nyomjon F3-at a telep\241t\202s megszak\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Tov\240bbi inform\240ci\242k\202rt l\240togasson el a",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "https://reactos.org/ weboldalra.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s  R = Helyre\240ll\241t\240s  L = Licenc  F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS verzi\242inform\240ci\242k",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "A ReactOS alfa \240llapotban van, ami azt jelenti, hogy m\202g",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "rengeteg funkci\242 hi\240nyzik \202s er\213sen fejleszt\202s alatt \240ll.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Csak tesztel\202sre aj\240nlott, nem alkalmas mindennapi haszn\240latra.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Amennyiben nem virtu\240lis g\202pre telep\241ti, mentse le el\213tte az adatait,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "vagy tesztelje egy m\240sodlagos, nem akt\241van haszn\240lt sz\240m\241t\242g\202pen.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Nyomjon ENTER-t a telep\241t\202s folytat\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Nyomjon F3-at a telep\241t\202s megszak\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHULicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Licenc:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "A ReactOS rendszer GNU GPL licenc alatt lett k\224zreadva",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "olyan r\202szekkel, amelyek m\240s kompatibilis lincenc\373 k\242dokat",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "tartalmaznak, mint X11 vagy BSD \202s GNU LGPL licencek.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Ez\202rt az \224sszes szoftver, amely a ReactOS rendszer r\202sze,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "a GNU GPL alatt ker\201l kiad\240sra az eredeti licenc fenntart\240sa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "mellett.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Ez a szoftver GARANCIA N\220LK\232L lett k\224zreadva haszn\240lati",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "korl\240toz\240sok n\202lk\201l, kiv\202ve a vonatkoz\242 helyi \202s nemzetk\224zi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "t\224rv\202nyeket. A ReactOS licence csak a harmadik feleknek t\224rt\202n\213",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "terjeszt\202sekre vonatkozik. Ha valamilyen okb\242l nem kapta meg",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "a GNU General Public License egy p\202ld\240ny\240t, k\202rj\201k l\240togasson el",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "a http://www.gnu.org/licenses/licenses.html weboldalra.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "Garancia:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Ez szabad szoftver; a m\240sol\240si felt\202telekhez l\240sd a forr\240sk\242dot.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "Minden GARANCIA N\220LK\232L lett k\224zreadva, az ELADHAT\340S\265GRA vagy",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "VALAMELY C\220LRA VAL\340 ALKALMAZHAT\340S\265GRA val\242 sz\240rmaztatott",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "garanci\240t is bele\202rtve.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Vissza",
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

static MUI_ENTRY huHUDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Az al\240bbi lista a jelenlegi eszk\224zbe\240ll\241t\240sokat mutatja.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "Sz\240m\241t\242g\202p:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "Kijelz\213:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Billenty\373zet:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Billenty\373zetkioszt\240s:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Elfogad\240s:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16,
        "Be\240ll\241t\240sok elfogad\240sa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "A LE / FEL gombokkal tud kijel\224lni egy elemet \202s az ENTER lenyom\240s\240val",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "tud kiv\240lasztani egy alternat\241v be\240ll\241t\240st.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Ha az \224sszes be\240ll\241t\240s megfelel\213, jel\224lje ki a \"Be\240ll\241t\240sok elfogad\240sa\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "sort, majd nyomjon ENTER-t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHURepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A ReactOS telep\241t\213 korai fejleszt\202si f\240zisban van. M\202g nem",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "t\240mogatja egy teljes \202rt\202k\373 telep\241t\213 \224sszes funkci\242j\240t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "A helyre\240ll\241t\242 funkci\242 m\202g nincs implement\240lva.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Nyomjon U-t az oper\240ci\242s rendszer friss\241t\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Nyomjon R-t a helyre\240ll\241t\240si konzol ind\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Nyomjon ESC-et a f\213oldalra val\242 visszat\202r\202shez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Nyomjon ENTER-t a sz\240m\241t\242g\202p \243jraind\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = F\213oldal  U = Friss\241t\202s  R = Helyre\240ll\241t\240s  ENTER = \351jraind\241t\240s",
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

static MUI_ENTRY huHUUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A ReactOS telep\241t\213 friss\241teni tudja az al\240bbi megl\202v\213 ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "telep\241t\202sek egyik\202t, vagy, ha egy ReactOS rendszer megs\202r\201lt,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "megpr\242b\240lhatja kijav\241tani azt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "A jav\241t\240si funkci\242 f\202lk\202sz \202s m\202g nem el\202rhet\213.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Haszn\240lja a FEL / LE gombokat egy rendszer kijel\224l\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Nyomjon U-t a kiv\240lasztott rendszer friss\241t\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Nyomjon ESC-et egy \243j telep\241t\202s ind\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Nyomjon F3-at a telep\241t\202s megszak\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Friss\241t\202s   ESC = Ne friss\241tsen   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A sz\240m\241t\242g\202p t\241pus\240nak megv\240ltoztat\240sa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Haszn\240lja a FEL / LE gombokat a sz\240m\241t\242g\202p t\241pus\240nak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   kiv\240laszt\240s\240hoz, majd nyomjon ENTER-t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Nyomjon ESC-et az el\213z\213 oldalra visszat\202r\202shez a sz\240m\241t\242g\202p",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   t\241pus\240nak megv\240ltoztat\240sa n\202lk\201l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   ESC = M\202gse   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "A rendszer most megbizonyosodik r\242la hogy minden adat",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        7,
        "ki\241r\240sra ker\201lt.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Ez egy percet vehet ig\202nybe.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        10,
        "Mikor k\202sz, a sz\240m\241t\242g\202p automatikusan \243jraindul.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Gyors\241t\242t\240r ki\201r\241t\202se",
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

static MUI_ENTRY huHUQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "A ReactOS nincs teljesen feltelep\241tve.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "T\240vol\241tsa el a floppy lemezt az A: meghajt\242b\242l \202s",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "minden CD-t a CD olvas\242kb\242l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Nyomjon ENTER-t a sz\240m\241t\242g\202p \243jraind\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "K\202rem v\240rjon...",
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

static MUI_ENTRY huHUDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Kijelz\213 t\241pus\240nak megv\240ltoztat\240sa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {   8,
        10,
         "\x07  Haszn\240lja a FEL / LE gombokat a kijelz\213 t\241pus\240nak kijel\224l\202s\202hez.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Majd nyomjon ENTER-t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Nyomjon ESC-et az el\213z\213 oldalra visszat\202r\202shez a kijelz\213 t\241pus\240nak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   megv\240ltoztat\240sa n\202lk\201l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   ESC = M\202gse   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "A ReactOS alapvet\213 \224sszetev\213i sikeresen feltelep\201ltek.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "T\240vol\241tsa el a floppy lemezt az A: meghajt\242b\242l \202s",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "minden CD-t a CD olvas\242kb\242l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Nyomjon ENTER-t a sz\240m\241t\242g\202p \243jraind\241t\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa",
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

static MUI_ENTRY huHUSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Az al\240bbi list\240ban l\240that\242ak a megl\202v\213 part\241ci\242k \202s a nem haszn\240lt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "t\240rhely \243j part\241ci\242k l\202trehoz\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Haszn\240lja a FEL / LE gombokat egy elem kijel\224l\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Nyomjon ENTER-t a ReactOS kijel\224lt part\241ci\242ra val\242 telep\241t\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Nyomjon C-t egy els\213dleges/logikai part\241ci\242 l\202trehoz\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Nyomjon E-t egy kiterjesztett part\241ci\242 l\202trehoz\240s\240hoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Nyomjon D-t egy megl\202v\213 part\241ci\242 t\224rl\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "K\202rem v\240rjon...",
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

static MUI_ENTRY huHUConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A rendszerpart\241ci\242 t\224rl\202s\202t v\240lasztotta.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "A rendszerpart\241ci\242kon lehetnek diagnosztikai programok, hardver",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "konfigur\240l\242 programok, programok, melyek egy oper\240ci\242s rendszert",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "(pl. ReactOS) ind\241tanak vagy egy\202b szoftverek, melyeket",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "a hardver gy\240rt\242ja adott.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Csak akkor t\224r\224lj\224n egy rendszerpart\241ci\242t, ha biztos benne hogy",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "nincsenek rajta ilyen programok, vagy ha biztos benne hogy t\224r\224lni",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "szeretn\202. Ha t\224rli a part\241ci\242t, lehet hogy nem fogja tudni elind\241tani",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "a sz\240m\241t\242g\202pet a merevlemezr\213l am\241g nem v\202gez a ReactOS telep\241t\202s\202vel.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Nyomjon ENTER-t a rendszerpart\241ci\242 t\224rl\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "   K\202s\213bb \243jra meg kell er\213s\241tenie a part\241ci\242 t\224rl\202s\202t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Nyomjon ESC-et az el\213z\213 oldalra val\242 visszat\202r\202shez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "   A part\241ci\242 nem lesz t\224r\224lve.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s  ESC = M\202gse",
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

static MUI_ENTRY huHUFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Part\241ci\242 form\240z\240sa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "A telep\241t\213 form\240zni fogja a part\241ci\242t. Nyomjon ENTER-t a folytat\240shoz.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A telep\241t\213 most ellen\213rzi a kijel\224lt part\241ci\242t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "K\202rem v\240rjon...",
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

static MUI_ENTRY huHUInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A telep\241t\213 felm\240solja a ReactOS f\240jljait a kijel\224lt part\241ci\242ra.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "V\240lasszon egy mapp\240t ahov\240 a ReactOS-t telep\241teni szeretn\202:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Az aj\240nlott mappa megv\240ltoztat\240s\240hoz nyomjon BACKSPACE-t, hogy",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "kit\224r\224lje a karaktereket, majd g\202pelje be a k\241v\240nt mappa nev\202t ahov\240",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "a ReactOS-t telep\241teni szeretn\202.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "K\202rem v\240rjon, am\241g a ReactOS telep\241t\213 felm\240solja a f\240jlokat a ReactOS",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "rendszer mapp\240ba.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Ez p\240r percet vehet ig\202nybe.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 K\202rem v\240rjon...    ",
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

static MUI_ENTRY huHUBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
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
        "Rendszerbet\224lt\213 merevlemezre telep\241t\202se (MBR \202s VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Rendszerbet\224lt\213 merevlemezre telep\241t\202se (csak VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Rendszerbet\224lt\213 floppy lemezre telep\241t\202se.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Rendszerbet\224lt\213 telep\241t\202s\202nek kihagy\240sa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUBootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Bootloader telep\241t\202se.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Bootloader telep\241t\202se az eszk\224zre, k\202rem v\240rjon...",
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

static MUI_ENTRY huHUBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Bootloader telep\241t\202se.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "K\202rj\201k helyezzen be egy megform\240zott floppy lemezt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "az A: meghajt\242ba, majd nyomjon ENTER-t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A billenty\373zet t\241pus\240nak megv\240ltoztat\240sa.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Haszn\240lja a FEL / LE gombokat a billenty\373zet t\241pus\240nak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   kijel\224l\202s\202hez, majd nyomjon ENTER-t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Nyomjon ESC-et az el\213z\213 oldalra visszat\202r\202shez a billenty\373zet",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   t\241pus\240nak megv\240ltoztat\240sa n\202lk\201l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   ESC = M\202gse   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHULayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "K\202rj\201k v\240lassza ki az alap\202rtelmezett billenty\373zetkioszt\240st.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Haszn\240lja a FEL / LE gombokat a v\240lasztott billenty\373zetkioszt\240s",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   kijel\213l\202s\202hez, majd nyomjon ENTER-t.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Nyomjon ESC-et az el\213z\213 oldalra visszat\202r\202shez",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   a billenty\373zetkioszt\240s megv\240ltoztat\240sa n\202lk\201l.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   ESC = M\202gse   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A telep\241t\213 el\213k\202sz\241ti a sz\240m\241t\242g\202pet a ReactOS f\240jlok m\240sol\240s\240ra.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "M\240soland\242 f\240jlok list\240j\240nak l\202trehoz\240sa...",
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

static MUI_ENTRY huHUSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "V\240lasszon egy f\240jlrendszert az al\240bbi list\240b\242l.",
        0
    },
    {
        8,
        19,
        "\x07  Haszn\240lja a FEL / LE gombokat egy f\240jlrendszer kijel\224l\202s\202hez.",
        0
    },
    {
        8,
        21,
        "\x07  Nyomjon ENTER-t a part\241ci\242 form\240z\240s\240hoz.",
        0
    },
    {
        8,
        23,
        "\x07  Nyomjon ESC-et egy m\240sik part\241ci\242 kijel\224l\202s\202hez.",
        0
    },
    {
        0,
        0,
        "ENTER = Folytat\240s   ESC = M\202gse   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHUDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A part\241ci\242 t\224rl\202s\202t v\240lasztotta",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Nyomjon L-t a part\241ci\242 t\224rl\202s\202hez.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "FIGYELEM: A part\241ci\242n l\202v\213 \224sszes adat el fog veszni!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Nyomjon ESC-et ha m\202gsem szeretn\202 t\224r\224lni.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Part\241ci\242 t\224rl\202se   ESC = M\202gse   F3 = Kil\202p\202s",
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

static MUI_ENTRY huHURegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " telep\241t\213 ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "A telep\241t\213 a rendszerbe\240ll\241t\240sokat friss\241ti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Regisztr\240ci\242s adatb\240zis gy\373jt\213k l\202trehoz\240sa...",
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

MUI_ERROR huHUErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Siker\201lt\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "A ReactOS nincs teljesen telep\241tve a sz\240m\241t\242g\202pre.\n"
        "Ha most kil\202p a telep\241t\213b\213l, \243jra futtatnia kell azt\n"
        "a ReactOS telep\241t\202s\202hez.\n"
        "\n"
        "  \x07 Nyomjon ENTER-t a telep\241t\202s folytat\240s\240hoz.\n"
        "  \x07 Nyomjon F3-at a kil\202p\202shez.\n",
        "F3 = Kil\202p\202s  ENTER = Folytat\240s"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Nem siker\201lt fel\202p\241teni a telep\241t\202si \243tvonalakat a ReactOS telep\241t\202si mapp\240hoz!\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_SOURCE_PATH
        "Nem lehet a telep\241t\202si forr\240st tartalmaz\242 part\241ci\242t t\224r\224lni!\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_SOURCE_DIR
        "Nem lehet a ReactOS-t a telep\241t\202si forr\240s mapp\240j\240ba telep\241teni!\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_NO_HDD
        "A telep\241t\213 nem tal\240lt merevlemezt.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "A telep\241t\213 nem tal\240lja a forr\240s meghajt\242j\240t.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "A telep\241t\213 nem tudja bet\224lteni a TXTSETUP.SIF f\240jlt.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "A telep\241t\213 egy s\202r\201lt TXTSETUP.SIF f\240jlt tal\240lt.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "A telep\241t\213 egy \202rv\202nytelen al\240\241r\240st tal\240lt a TXTSETUP.SIF f\240jlban.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "A telep\241t\213 nem tudta beolvasni a rendszermeghajt\242 inform\240ci\242it.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_WRITE_BOOT,
        "Nem siker\201lt a %S bootcode telep\241t\202se a rendszerpart\241ci\242ra.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "A telep\241t\213 nem tudta bet\224lteni a sz\240m\241t\242g\202p t\241pusok list\240j\240t.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "A telep\241t\213 nem tudta bet\224lteni a kijelz\213 be\240ll\241t\240sok list\240j\240t.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "A telep\241t\213 nem tudta bet\224lteni a billenty\373zet t\241pusok list\240j\240t.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "A telep\241t\213 nem tudta bet\224lteni a billenty\373zetkioszt\240sok list\240j\240t.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_WARN_PARTITION,
        "A telep\241t\213 legal\240bb egy olyan merevlemezt tal\240lt amely nem kompatibilis\n"
        "part\241ci\242s t\240bl\240t tartalmaz, amelyet nem tud rendesen kezelni!\n"
        "\n"
        "Part\241ci\242k l\202trehoz\240sa vagy t\224rl\202se t\224nkreteheti a part\241ci\242s t\240bl\240t.\n"
        "\n"
        "  \x07  Nyomjon F3-at a telep\241t\213b\213l val\242 kil\202p\202shez.\n"
        "  \x07  Nyomjon ENTER-t a folytat\240shoz.",
        "F3 = Kil\202p\202s  ENTER = Folytat\240s"
    },
    {
        // ERROR_NEW_PARTITION,
        "Nem tud l\202trehozni \243j part\241ci\242t\n"
        "egy m\240r l\202tez\213 part\241ci\242n bel\201l!\n"
        "\n"
        "  * Nyomjon meg egy gombot a folytat\240shoz.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Nem siker\201lt a %S bootcode telep\241t\202se a rendszerpart\241ci\242ra.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_NO_FLOPPY,
        "Nincs lemez a A: meghajt\242ban.",
        "ENTER = Folytat\240s"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Nem siker\201lt friss\241teni a billenty\373zetkioszt\240s be\240ll\241t\240sait.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Nem siker\201lt friss\241teni a kijelz\213 be\240l\241t\240sokat.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Nem siker\201lt import\240lni egy regisztr\240ci\242s adatb\240zis gy\373jt\213f\240jlt.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_FIND_REGISTRY
        "A telep\241t\213 nem tal\240lja a regisztr\240ci\242s adatb\240zis f\240jlokat.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_CREATE_HIVE,
        "Nem siker\201lt l\202trehozni a regisztr\240ci\242s adatb\240zis gy\373jt\213ket.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Nem siker\201lt a regisztr\240ci\242s adatb\240zis inicializ\240l\240sa.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "A kabinetf\240jlban nincs \202rv\202nyes inf f\240jl.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_CABINET_MISSING,
        "A kabinetf\240jl nem tal\240lhat\242.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "A kabinetf\240jlban nincs telep\241t\213 parancsf\240jl.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_COPY_QUEUE,
        "Nem siker\201lt megnyitni a m\240soland\242 f\240jlok list\240j\240t.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_CREATE_DIR,
        "Nem siker\201lt l\202trehozni a telep\241t\202si mapp\240kat.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "A telep\241t\213 nem tal\240lja a '%S' r\202szt\n"
        "a TXTSETUP.SIF f\240jlban.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_CABINET_SECTION,
        "A telep\241t\213 nem tal\240lja a '%S' r\202szt\n"
        "a kabinetf\240jlban.\n",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Nem siker\201lt l\202trehozni a telep\241t\202si mapp\240t.",
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Nem siker\201lt a part\241ci\242s t\240bla \241r\240sa.\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Nem siker\201lt a k\242dlap hozz\240ad\240sa a regisztr\240ci\242s adatb\240zishoz.\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Nem siker\201lt a ter\201leti be\240ll\241t\240s ment\202se.\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Nem siker\201lt a billenyt\373zetkioszt\240sok hozz\240ad\240sa\n"
        "a regisztr\240ci\242s adatb\240zishoz.\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Nem siker\201lt be\240ll\241tani a geo id-t.\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "\220rv\202nytelen mappa n\202v.\n"
        "\n"
        "  * Nyomjon meg egy gombot a folytat\240shoz."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "A kijel\224lt part\241ci\242 nem el\202g nagy a ReactOS telep\241t\202s\202hez.\n"
        "A telep\241t\202si part\241ci\242nak legal\240bb %lu MB-nak kell lennie.\n"
        "\n"
        "  * Nyomjon meg egy gombot a folytat\240shoz.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Nem lehet \243j els\213dleges vagy kiterjesztett part\241ci\242t l\202trehozni\n"
        "ezen a lemezen, mert megtelt a part\241ci\242s t\240bla.\n"
        "\n"
        "  * Nyomjon meg egy gombot a folytat\240shoz."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Nem lehet egyn\202l t\224bb kiterjesztett part\241ci\242t l\202trehozni lemezenk\202nt.\n"
        "\n"
        "  * Nyomjon meg egy gombot a folytat\240shoz."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "A telep\241t\213 nem tudja form\240zni a part\241ci\242t:\n"
        " %S\n"
        "\n"
        "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE huHUPages[] =
{
    {
        SETUP_INIT_PAGE,
        huHUSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        huHULanguagePageEntries
    },
    {
        WELCOME_PAGE,
        huHUWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        huHUIntroPageEntries
    },
    {
        LICENSE_PAGE,
        huHULicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        huHUDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        huHURepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        huHUUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        huHUComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        huHUDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        huHUFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        huHUSelectPartitionEntries
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        huHUConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        huHUSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        huHUFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        huHUCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        huHUDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        huHUInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        huHUPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        huHUFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        huHUKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        huHUBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        huHULayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        huHUQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        huHUSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        huHUBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        huHUBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        huHURegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING huHUStrings[] =
{
    {STRING_PLEASEWAIT,
     "   K\202rem v\240rjon..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Telep\241t\202s   C = \351j els\213dleges   E = \351j kiterjesztett   F3 = Kil\202p\202s"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Telep\241t\202s   C = Logikai part\241ci\242 l\202trehoz\240sa   F3 = Kil\202p\202s"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Telep\241t\202s   D = Part\241ci\242 t\224rl\202se   F3 = Kil\202p\202s"},
    {STRING_DELETEPARTITION,
     "   D = Part\241ci\242 t\224rl\202se   F3 = Kil\202p\202s"},
    {STRING_PARTITIONSIZE,
     "Az \243j part\241ci\242 m\202rete:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Egy els\213dleges part\241ci\242 l\202trehoz\240s\240t v\240lasztotta itt:"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Egy kiterjesztett part\241ci\242 l\202trehoz\240s\240t v\240lasztotta itt:"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Egy logikai part\241ci\242 l\202trehoz\240s\240t v\240lasztotta itt:"},
    {STRING_HDPARTSIZE,
    "K\202rem \241rja be az \243j part\241ci\242 m\202ret\202t megab\240jtban."},
    {STRING_CREATEPARTITION,
     "   ENTER = Part\241ci\242 l\202trehoz\240sa   ESC = M\202gse   F3 = Kil\202p\202s"},
    {STRING_NEWPARTITION,
    "A telep\241t\213 egy \243j part\241ci\242t hozott l\202tre itt:"},
    {STRING_PARTFORMAT,
    "K\224vetkez\213 l\202p\202sk\202nt ez a part\241ci\242 form\240zva lesz."},
    {STRING_NONFORMATTEDPART,
    "A ReactOS egy \243j / nem form\240zott part\241ci\242ra telep\241t\202s\202t v\240lasztotta."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "A rendszerpart\241ci\242 m\202g nincs form\240zva."},
    {STRING_NONFORMATTEDOTHERPART,
    "Az \243j part\241ci\242 m\202g nincs form\240zva."},
    {STRING_INSTALLONPART,
    "A telep\241t\213 az al\240bbi part\241ci\242ra telep\241ti a ReactOS-t:"},
    {STRING_CONTINUE,
    "ENTER = Folytat\240s"},
    {STRING_QUITCONTINUE,
    "F3 = Kil\202p\202s  ENTER = Folytat\240s"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"},
    {STRING_DELETING,
     "   F\240jl t\224rl\202se: %S"},
    {STRING_MOVING,
     "   F\240jl mozgat\240sa: %S ide: %S"},
    {STRING_RENAMING,
     "   F\240jl \240tnevez\202se: %S erre: %S"},
    {STRING_COPYING,
     "   F\240jl m\240sol\240sa: %S"},
    {STRING_SETUPCOPYINGFILES,
     "A telep\241t\213 f\240jlokat m\240sol..."},
    {STRING_REGHIVEUPDATE,
    "   Regisztr\240ci\242s adatb\240zis gy\373jt\213k friss\241t\202se..."},
    {STRING_IMPORTFILE,
    "   %S import\240l\240sa..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Kijelz\213 be\240ll\241t\240sok friss\241t\202se..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Ter\201leti be\240ll\241t\240sok friss\241t\202se..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Billenty\373zetkioszt\240s be\240ll\241t\240sok friss\241t\202se..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   K\242dlap inform\240ci\242k hozz\240ad\240sa..."},
    {STRING_DONE,
    "   K\202sz..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Sz\240m\241t\242g\202p \243jraind\241t\240sa"},
    {STRING_REBOOTPROGRESSBAR,
    " A sz\240m\241t\242g\202p %li m\240sodperc m\243lva \243jraindul... "},
    {STRING_CONSOLEFAIL1,
    "Nem lehet megnyitni a konzolt\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Ennek leggyakoribb oka az USB-s billenty\373zet haszn\240lata\r\n"},
    {STRING_CONSOLEFAIL3,
    "Az USB-s billenty\373zetek m\202g nincsenek teljesen t\240mogatva\r\n"},
    {STRING_FORMATTINGPART,
    "A telep\241t\213 form\240zza a part\241ci\242t..."},
    {STRING_CHECKINGDISK,
    "A telep\241t\213 ellen\213rzi a merevlemezt..."},
    {STRING_FORMATDISK1,
    " Part\241ci\242 form\240z\240sa %S f\240jlrendszerrel (gyorsform\240z\240s) "},
    {STRING_FORMATDISK2,
    " Part\241ci\242 form\240z\240sa %S f\240jlrendszerrel "},
    {STRING_KEEPFORMAT,
    " Jelenlegi f\240jlrendszer megtart\240sa (nincs v\240ltoztat\240s) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "itt: %s."},
    {STRING_PARTTYPE,
    "T\241pus 0x%02x"},
    {STRING_HDDINFO1,
    // "%lu. merevlemez (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s %lu. merevlemez (Port=%hu, Bus=%hu, Id=%hu) itt: %wZ [%s]"},
    {STRING_HDDINFO2,
    // "%lu. merevlemez (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s %lu. merevlemez (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Nem particion\240lt ter\201let"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Kiterjesztett part\241ci\242"},
    {STRING_UNFORMATTED,
    "\351j (nem form\240zott)"},
    {STRING_FORMATUNUSED,
    "Nem haszn\240lt"},
    {STRING_FORMATUNKNOWN,
    "Ismeretlen"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Billenty\373zetkioszt\240sok hozz\240ad\240sa"},
    {0, 0}
};
