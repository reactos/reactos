#pragma once

static MUI_ENTRY msMYSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Please wait while the ReactOS Setup initializes itself",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "and discovers your devices...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Please wait...",
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

static MUI_ENTRY msMYLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Pemilihan Bahasa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Sila pilih Bahasa yang digunakan untuk proses pemasangan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Bahasa ini adalah Bahasa lalai untuk sistem akhir.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan F3 = Keluar",
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

static MUI_ENTRY msMYWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Selamat datang ke persediaan ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Bahagian persediaan salinan sistem pengendalian ReactOS ke",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "komputer anda dan menyediakan bahagian kedua daripada persediaan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press ENTER to install or upgrade ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tekan R untuk memperbaiki atau mengemaskini ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tekan L untuk melihat syarat-syarat dan terma-terma Pelesenan ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tekan F3 untuk keluar tanpa memasang ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Untuk maklumat lanjut mengenai ReactOS, sila layari:",
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
        "ENTER = Teruskan R = Pembaikan L = Lesen F3 = Keluar",
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

static MUI_ENTRY msMYIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
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
        "ReactOS is in Alpha stage, meaning it is not feature-complete",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "and is under heavy development. It is recommended to use it only for",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "evaluation and testing purposes and not as your daily-usage OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Backup your data or test on a secondary computer if you attempt",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "to run ReactOS on real hardware.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ENTER to continue ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   F3 = Quit",
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

static MUI_ENTRY msMYLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Pelesenan:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "the original license.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "If for some reason you did not receive a copy of the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
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
        "Jaminan:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Kembali",
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

static MUI_ENTRY msMYDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Senarai di bawah menunjukkan peranti seting semasa.",
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
        "Paparan:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Papan kekunci:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Tataletak papan kekunci:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "Terima:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "Menerima seting peranti ini",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Anda boleh menukar seting perkakasan dengan menekan butang UP atau DOWN",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "untuk pilih satu entri. Kemudian tekan kekunci ENTER untuk memilih alternatif",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "seting.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Apabila semua seting yang betul, pilih \"Menerima seting peranti ini\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "dan tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan F3 = Keluar",
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

static MUI_ENTRY msMYRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS persediaan sedang untuk fasa pembangunan awal. Ia belum lagi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "menyokong semua fungsi aplikasi persediaan sepenuhnya digunakan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Fungsi pembaikan tidak dilaksanakan lagi.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tekan U untuk mengemaskini ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tekan R bagi konsol pemulihan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tekan ESC untuk kembali ke halaman utama.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tekan ENTER untuk memulakan semula sistem komputer anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Halaman Utama  U = Mengemaskini  R = Pemulihan  ENTER = But Semula",
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

static MUI_ENTRY msMYUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "The ReactOS Setup can upgrade one of the available ReactOS installations",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "listed below, or, if a ReactOS installation is damaged, the Setup program",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "can attempt to repair it.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "The repair functions are not all implemented yet.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Press UP or DOWN to select an OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Press U for upgrading the selected OS installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Press ESC to continue with a new installation.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = Upgrade   ESC = Do not upgrade   F3 = Quit",
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

static MUI_ENTRY msMYComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda ingin menukar jenis komputer dipasang",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tekan kekunci UP atau DOWN untuk memilih jenis komputer yang anda mahu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   Kemudian tekan ENTER",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan kekunci ESC untuk kembali ke halaman sebelumnya tanpa mengubah",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   jenis komputer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY msMYFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Sistem sekarang memastikan semua data yang disimpan pada cakera anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Ini mungkin mengambil beberapa minit.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Apabila selesai, komputer akan but semula secara automatik.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Flushing cache",
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

static MUI_ENTRY msMYQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS tidak dipasang sepenuhnya.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Alih keluar cakera liut dari Drive A: dan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "semua cakera padat dari pemacu CD.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tekan ENTER untuk memulakan semula sistem komputer anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Sila tunggu...",
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

static MUI_ENTRY msMYDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda ingin tukar jenis paparan untuk dipasang.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Tekan kekunci UP atau DOWN untuk memilih",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   jenis paparan yang dikehendaki. Kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan kekunci ESC untuk kembali ke halaman sebelumnya",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   tanpa mengubah jenis paparan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY msMYSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Komponen-komponen asas ReactOS telah berjaya dipasang.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Keluarkan cakera liut A: drive dan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "semua CD-ROMs dari pemacu CD.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tekan ENTER untuk memulakan semula sistem komputer anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Memulakan semula komputer",
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

static MUI_ENTRY msMYSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Senarai di bawah menunjukkan partition yang sedia ada dan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "ruang cakera yang digunakan untuk partition baru.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Tekan UP atau DOWN untuk pilih entri senarai.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan ENTER untuk memasang ReactOS ke sekatan yang dipilih.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tekan C untuk mencipta partition yang utama/logik.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tekan E untuk mencipta partition yang lanjutan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tekan D untuk menghapuskan sekatan yang sedia ada.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Sila tunggu...",
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

static MUI_ENTRY msMYChangeSystemPartition[] =
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
        "The current system partition of your computer",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "on the system disk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "uses a format not supported by ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "In order to successfully install ReactOS, the Setup program must change",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "the current system partition to a new one.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "The new candidate system partition is:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  To accept this choice, press ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  To manually change the system partition, press ESC to go back to",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   the partition selection list, then select or create a new system",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   partition on the system disk.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "In case there are other operating systems that depend on the original",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "system partition, you may need to either reconfigure them for the new",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "system partition, or you may need to change the system partition back",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "to the original one after finishing the installation of ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Continue   ESC = Cancel",
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

static MUI_ENTRY msMYConfirmDeleteSystemPartitionEntries[] =
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
        "You have chosen to delete the system partition.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "System partitions can contain diagnostic programs, hardware configuration",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "programs, programs to start an operating system (like ReactOS) or other",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "programs provided by the hardware manufacturer.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Delete a system partition only when you are sure that there are no such",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "programs on the partition, or when you are sure you want to delete them.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "When you delete the partition, you might not be able to boot the",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "computer from the harddisk until you finished the ReactOS Setup.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Press ENTER to delete the system partition. You will be asked",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   to confirm the deletion of the partition again later.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Press ESC to return to the previous page. The partition will",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   not be deleted.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Continue  ESC=Cancel",
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

static MUI_ENTRY msMYFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Format partition",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Persediaan sekarang akan format partition. Tekan ENTER untuk teruskan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Teruskan   F3 = Keluar",
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

static MUI_ENTRY msMYCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Persediaan kini sedang menyemak sekatan yang dipilih.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Sila tunggu...",
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

static MUI_ENTRY msMYInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Persediaan memasang ReactOS fail ke sekatan yang dipilih. Pilih satu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "direktori di mana anda mahu ReactOS untuk dipasang:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Untuk mengubah direktori dicadangkan, tekan BACKSPACE untuk menghapuskan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "ciri-ciri dan kemudian taip direktori di mana anda mahu ReactOS untuk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "boleh dipasang.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan   F3 = Keluar",
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

static MUI_ENTRY msMYFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Sila tunggu sementara fail salinan ReactOS persediaan",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "untuk ReactOS folder pemasangan anda.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Ini mungkin mengambil beberapa minit untuk selesai.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Sila tunggu...    ",
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

static MUI_ENTRY msMYBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
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
        "Memasang mana pada cakera keras (MBR dan VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Memasang mana pada cakera keras (VBR sahaja).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Memasang mana pada cakera liut.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Langkau memasang bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan   F3 = Keluar",
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

static MUI_ENTRY msMYBootLoaderInstallPageEntries[] =
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
        "Persediaan sedang memasang bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Installing the bootloader onto the media, please wait...",
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

static MUI_ENTRY msMYBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Persediaan sedang memasang bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Sila sisipkan cakera liut diformatkan dalam pemacu A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "dan tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan   F3 = Keluar",
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

static MUI_ENTRY msMYKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda ingin menukar jenis papan kekunci dipasang.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tekan kekunci UP atau DOWN untuk memilih jenis ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   papan kekunci yang diingini. Kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan kekunci ESC untuk kembali ke halaman sebelumnya ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   tanpa mengubah jenis papan kekunci.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY msMYLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Sila pilih susun-atur yang dipasang secara lalai.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tekan kekunci UP atau DOWN untuk memilih susun atur",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    yang dikehendaki. Kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan kekunci ESC untuk kembali ke halaman sebelumnya tanpa mengubah",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   tataletak papan kekunci.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Teruskan   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY msMYPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Persediaan menyediakan komputer awda untuk menyalin fail-fail ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Membina senarai fail salinan...",
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

static MUI_ENTRY msMYSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Pilih sistem fail dari senarai di bawah.",
        0
    },
    {
        8,
        19,
        "\x07  Tekan UP atau DOWN untuk pilih sistem fail.",
        0
    },
    {
        8,
        21,
        "\x07  Tekan ENTER untuk memformat partition.",
        0
    },
    {
        8,
        23,
        "\x07  Tekan ESC untuk memilih partition yang lain.",
        0
    },
    {
        0,
        0,
        "ENTER = Teruskan   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY msMYDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda telah memilih untuk menghapuskan partition itu",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Tekan L untuk menghapuskan partition itu.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "AMARAN: Semua data pada partition ini akan hilang!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  Tekan ESC untuk membatalkan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = Menghapuskan partition   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY msMYRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Persediaan ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Persediaan sedang mengemaskini konfigurasi sistem.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Mencipta daftaran gatal...",
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

MUI_ERROR msMYErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Berjaya\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS tidak benar-benar dipasang pada\n"
        "komputer anda. Jika anda berhenti persediaan sekarang, anda akan perlu\n"
        "untuk menjalankan persediaan untuk memasang ReactOS lagi.\n"
        "\n"
        "  \x07  Tekan ENTER untuk meneruskan persediaan.\n"
        "  \x07  Tekan F3 untuk keluar persediaan.",
        "F3 = Keluar  ENTER = Teruskan"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Failed to build the installation paths for the ReactOS installation directory!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_PATH
        "You cannot delete the partition containing the installation source!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_SOURCE_DIR
        "You cannot install ReactOS within the installation source directory!\n"
        "ENTER = Reboot computer"
    },
    {
        // ERROR_NO_HDD
        "Persediaan tidak dapat mencari cakera keras.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Persediaan tidak dapat mencari sumber cakera.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Persediaan gagal untuk memuatkan fail TXTSETUP.SIF.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Persediaan mendapati TXTSETUP.SIF yang rosak.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Persediaan ditemui untuk tandatangan tidak sah dalam TXTSETUP.SIF.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Persediaan tidak dapat mendapatkan semula maklumat sistem cakera.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_WRITE_BOOT,
        "Persediaan gagal memasang bootcode %S pada partition sistem.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Persediaan gagal untuk memuatkan senarai jenis komputer.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Persediaan gagal untuk memuatkan pengesetan paparan senarai.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Persediaan gagal untuk memuatkan senarai jenis papan kekunci.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Persediaan gagal untuk memuatkan senarai susun atur papan kekunci.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_WARN_PARTITION,
          "Persediaan mendapati bahawa sekurang-kurangnya satu cakera keras mengandungi\n"
          "jadual partition yang sesuai untuk yang tidak boleh dikendalikan dengan betul!\n"
          "\n"
          "Mencipta atau menghapuskan sekatan boleh memusnahkan semula jadual partition.\n"
          "\n"
          "  \x07  Tekan F3 untuk keluar persediaan.\n"
          "  \x07  Tekan ENTER untuk teruskan.",
          "F3 = Keluar  ENTER = Teruskan"
    },
    {
        // ERROR_NEW_PARTITION,
        "Anda tidak boleh mencipta satu partition baru\n"
        "dalam partition yang telah sedia ada!\n"
        "\n"
        "  * Tekan sebarang kunci untuk meneruskan.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Persediaan gagal memasang bootcode %S pada partition sistem.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_NO_FLOPPY,
        "Tiada cakera dalam cakera A:.",
        "ENTER = Teruskan"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Persediaan gagal mengemaskini seting susun atur papan kekunci.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Persediaan gagal mengemaskini daftaran seting paparan.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Persediaan gagal untuk mengimport fail gatal.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_FIND_REGISTRY
        "Persediaan gagal untuk mencari pendaftaran fail data.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_CREATE_HIVE,
        "Persediaan gagal mencipta gatal daftaran.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Persediaan gagal untuk diawalkan daftaran.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Kabinet mempunyai fail inf tidak sah.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_CABINET_MISSING,
        "Kabinet tidak dijumpai.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Kabinet mempunyai skrip tiada persediaan.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_COPY_QUEUE,
        "Persediaan gagal membuka baris gilir fail salinan.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_CREATE_DIR,
        "Persediaan tidak dapat mencipta direktori pasang.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Persediaan gagal untuk mencari bahagian '%S'\n"
        "dalam TXTSETUP. SIF.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_CABINET_SECTION,
        "Persediaan gagal menemui bahagian '%S'\n"
        "dalam kabinet.\n",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Persediaan tidak dapat mencipta direktori pasang.",
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Persediaan gagal menulis jadual partition.\n"
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Persediaan gagal untuk menambah codepage ke daftaran.\n"
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Persediaan tidak dapat menyediakan penempatan sistem.\n"
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Persediaan gagal untuk menambah susun atur papan kekunci daftaran.\n"
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Persediaan tidak dapat disetkan geo id.\n"
        "ENTER = Memulakan semula komputer"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Nama direktori tidak sah.\n"
        "\n"
        "  * Tekan sebarang kunci untuk meneruskan."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Partition yang dipilih adalah tidak cukup besar untuk memasang ReactOS.\n"
        "Partition yang pasang hendaklah mempunyai saiz sekurang-kurangnya %lu MB.\n"
        "\n"
        "  * Tekan sebarang kunci untuk meneruskan.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Anda tidak boleh mencipta satu partition baru utama atau yang dilanjutkan\n"
        "dalam jadual partition cakera ini kerana jadual partition telah penuh.\n"
        "\n"
        "  * Tekan sebarang kunci untuk meneruskan."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Anda tidak boleh mencipta lebih daripada satu petak lanjutan setiap cakera.\n"
        "\n"
        "  * Tekan sebarang kunci untuk meneruskan."
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE msMYPages[] =
{
    {
        SETUP_INIT_PAGE,
        msMYSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        msMYLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        msMYWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        msMYIntroPageEntries
    },
    {
        LICENSE_PAGE,
        msMYLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        msMYDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        msMYRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        msMYUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        msMYComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        msMYDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        msMYFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        msMYSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        msMYChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        msMYConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        msMYSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        msMYFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        msMYCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        msMYDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        msMYInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        msMYPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        msMYFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        msMYKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        msMYBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        msMYLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        msMYQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        msMYSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        msMYBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        msMYBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        msMYRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING msMYStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Sila tunggu..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Pasang   C = Mencipta Utama   E = Mencipta Dilanjutkan   F3 = Keluar"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Pasang   C = Mencipta Partition Logik   F3 = Keluar"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Pasang   D = Menghapuskan Partition   F3 = Keluar"},
    {STRING_DELETEPARTITION,
     "   D = Menghapuskan Partition   F3 = Keluar"},
    {STRING_PARTITIONSIZE,
     "Saiz partition yang baru:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Anda telah memilih untuk mencipta partition yang utama padanya"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Anda telah memilih untuk mencipta partition yang lanjut padanya"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Anda telah memilih untuk mencipta partition yang logik padanya"},
    {STRING_HDPARTSIZE,
    "Sila masukkan saiz partition yang baru dalam megabait."},
    {STRING_CREATEPARTITION,
     "   ENTER = Mencipta Partition   ESC = Batalkan   F3 = Keluar"},
    {STRING_NEWPARTITION,
    "Persediaan yang dicipta partition yang baru di"},
    {STRING_PARTFORMAT,
    "Partition ini akan diformat seterusnya."},
    {STRING_NONFORMATTEDPART,
    "Anda telah memilih untuk memasang ReactOS pada Partition yang baru atau tidak diformat."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Partition sistem tidak diformat lagi."},
    {STRING_NONFORMATTEDOTHERPART,
    "Partition baru tidak diformat lagi."},
    {STRING_INSTALLONPART,
    "Persediaan memasang ReactOS ke Partition"},
    {STRING_CONTINUE,
    "ENTER = Teruskan"},
    {STRING_QUITCONTINUE,
    "F3 = Keluar  ENTER = Teruskan"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Memulakan semuala komputer"},
    {STRING_DELETING,
     "   Deleting file: %S"},
    {STRING_MOVING,
     "   Moving file: %S to: %S"},
    {STRING_RENAMING,
     "   Renaming file: %S to: %S"},
    {STRING_COPYING,
     "   Menyalin fail: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Persediaan sedang menyalin fail..."},
    {STRING_REGHIVEUPDATE,
    "   Mengemaskini daftaran gatal..."},
    {STRING_IMPORTFILE,
    "   Mengimport %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Mengemaskini seting paparan pendaftaran..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Mengemaskini seting tempatan..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Mengemaskini seting susun atur papan kekunci..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Menambah maklumat codepage..."},
    {STRING_DONE,
    "   Siap..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Memulakan semuala komputer"},
    {STRING_REBOOTPROGRESSBAR,
    " Your computer will reboot in %li second(s)... "},
    {STRING_CONSOLEFAIL1,
    "Tidak dapat membuka konsol\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Punca paling biasa ini menggunakan papan kekunci yang USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Papan kekunci USB tidak disokong sepenuhnya lagi\r\n"},
    {STRING_FORMATTINGPART,
    "Persediaan sedang format partition..."},
    {STRING_CHECKINGDISK,
    "Persediaan sedang menyemak cakera..."},
    {STRING_FORMATDISK1,
    " Format partition seperti sistem fail %S (format ringkas) "},
    {STRING_FORMATDISK2,
    " Format partition seperti sistem fail %S "},
    {STRING_KEEPFORMAT,
    " Memastikan sistem fail semasa (tiada perubahan) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "on %s."},
    {STRING_PARTTYPE,
    "Type 0x%02x"},
    {STRING_HDDINFO1,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) on %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Ruang unpartitioned"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Partition dilanjutkan"},
    {STRING_UNFORMATTED,
    "Baru (Tidak diformat)"},
    {STRING_FORMATUNUSED,
    "Tidak digunakan"},
    {STRING_FORMATUNKNOWN,
    "Tidak diketahui"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Menambah susun atur papan kekunci"},
    {0, 0}
};
