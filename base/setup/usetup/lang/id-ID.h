#pragma once

static MUI_ENTRY idIDSetupInitPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "Harap tunggu saat Penyetelan ReactOS menginisialisasi",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "dan mencari perangkat anda...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Mohon tunggu...",
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

static MUI_ENTRY idIDLanguagePageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
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
        "\x07  Mohon pilih bahasa yang digunakan pada proses instalasi.",
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
        "\x07  Bahasa ini akan menjadi pilihan asli saat ini dan seterusnya.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Lanjut  F3 = Keluar",
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

static MUI_ENTRY idIDWelcomePageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Selamat datang di Penyetelan ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "Bagian dari penyetelan ini menyalin Sistem Operasi ReactOS ke",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "komputer Anda dan mempersiapkan bagian kedua dari pengaturan ini.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tekan ENTER untuk memasang atau meningkatkan ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
     // "\x07  Tekan R untuk memperbaiki instalasi ReactOS menggunakan Konsol Pemulihan.",
        "\x07  Tekan R untuk memperbaiki instalasi ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tekan L untuk menampilkan Syarat dan Ketentuan Lisensi ReactOS.",
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
        "Untuk informasi lebih lanjut terkait ReactOS, mohon kunjungi:",
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
        "ENTER = Lanjut  R = Perbaiki  L = Lisensi  F3 = Keluar",
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

static MUI_ENTRY idIDIntroPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Status Versi ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS saat ini dalam tahap Alpha, artinya belum lengkap fitur dan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "dalam pengembangan berat. ReactOS direkomendasikan hanya untuk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "tujuan evaluasi dan percobaan, bukan untuk penggunaan OS sehari-hari.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "Cadangkan data anda atau tes pada komputer yang lain jika anda",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "berkenan untuk menjalankan ReactOS pada perangkat keras murni.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tekan ENTER untuk melanjutkan Penyetelan ReactOS.",
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
        0,
        0,
        "ENTER = Lanjut   F3 = Keluar",
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

static MUI_ENTRY idIDLicensePageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "Perizinan:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "Sistem ReactOS dilisensikan berdasarkan ketentuan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL dengan beberapa bagian kode yang termuat dari lisensi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "lain yang kompatibel seperti lisensi X11 atau BSD dan GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "Semua perangkat lunak yang menjadi bagian dari sistem ReactOS,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "karena itu dirilis di bawah GNU GPL serta mempertahankan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "lisensi asli.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Perangkat lunak ini tidak disertai GARANSI atau pembatasan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "penggunaan kecuali hukum lokal dan internasional yang berlaku.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "Lisensi ReactOS hanya mencakup distribusi ke pihak ketiga.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "Jika karena alasan tertentu Anda tidak menerima salinan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "GNU General Public License dengan ReactOS silahkan kunjungi",
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
        "Garansi:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "Ini adalah perangkat lunak gratis; lihat sumber untuk ketentuan salinan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "TIDAK ADA garansi; bahkan untuk PENJUALAN atau PENGGUNAAN UNTUK",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "TUJUAN TERTENTU",
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

static MUI_ENTRY idIDDevicePageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Daftar di bawah ini menunjukkan pengaturan perangkat saat ini.",
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
        "Tampilan:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "Papan ketik:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "Tata letak papan ketik:",
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
        16, "Terima pengaturan perangkat ini",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "Anda dapat mengubah pengaturan perangkat keras dengan menekan tombol",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "ATAS atau BAWAH untuk menunjuk daftar. Kemudian tekan tombol ENTER",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "untuk memilih pengaturan alternatif.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "Ketika semua pengaturan sesuai, pilih \"Terima pengaturan perangkat ini\"",
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
        "ENTER = Lanjut   F3 = Keluar",
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

static MUI_ENTRY idIDRepairPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan ReactOS masih dalam tahap pengembangan awal di mana masih",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "belum mendukung semua fungsi sebagai penyetelan aplikasi yang berguna penuh.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Fungsi perbaikan belum diimplementasikan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tekan U untuk meningkatkan OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tekan R untuk Konsol Pemulihan.",
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
        "\x07  Tekan ENTER untuk memulai ulang komputer anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = Halaman utama  U = Tingkatkan  R = Pulihkan  ENTER = Mulai Ulang",
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

static MUI_ENTRY idIDUpgradePageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan ReactOS dapat meningkatkan salah satu dari instalasi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "ReactOS yang tersedia di bawah ini, atau, jika instalasi ReactOS",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "ini rusak, program penyetelan ini dapat mengupayakan perbaikan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "Fungsi perbaikan belum semuanya diimplementasikan.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tekan ATAS atau BAWAH untuk memilih instalasi OS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tekan U untuk meningkatkan instalasi OS terpilih.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tekan ESC untuk lanjut dengan instalasi yang baru.",
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
        0,
        0,
        "U = Tingkatkan   ESC = Jangan tingkatkan   F3 = Keluar",
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

static MUI_ENTRY idIDComputerPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda ingin mengubah jenis komputer yang akan diinstal.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tekan tombol ATAS atau BAWAH untuk memilih jenis komputer yang",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   diinginkan. Kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan tombol ESC untuk kembali ke halaman sebelumnya tanpa mengubah",
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
        "ENTER = Lanjut   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY idIDFlushPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Sistem sedang memastikan semua data tersimpan dalam cakram anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Ini membutuhkan waktu beberapa menit.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "Ketika selesai, komputer anda akan dimulai ulang secara otomatis.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Membersihkan cache",
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

static MUI_ENTRY idIDQuitPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS tidak terpasang sepenuhnya.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Cabut cakram disket dari Drive A: dan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "semua CD-ROM dari Drive CD.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tekan ENTER untuk memulai ulang komputer anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Mohon tunggu...",
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

static MUI_ENTRY idIDDisplayPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda ingin mengubah jenis tampilan yang akan dipasang.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  Tekan tombol ATAS atau BAWAH untuk memilih jenis tampilan.",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   yang diinginkan. Kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan tombol ESC untuk kembali ke halaman sebelumnya tanpa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   mengubah jenis tampilan.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Lanjut   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY idIDSuccessPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "Komponen dasar ReactOS berhasil dipasang.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "Cabut cakram disket dari Drive A: dan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "semua CD-ROM dari CD-Drive.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "Tekan ENTER untuk memulai ulang komputer anda.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Mulai ulang komputer",
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

static MUI_ENTRY idIDSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Daftar di bawah ini menunjukkan partisi yang tersedia dan ukuran cakram",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "yang belum dikunakan untuk partisi baru.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  Tekan ATAS atau BAWAH untuk memilih daftar entri.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan ENTER untuk memasang ReactOS pada partisi terpilih.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  Tekan C untuk membuat partisi primary/logical.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Tekan E untuk membuat partisi extended.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  Tekan D untuk menghapus partisi yang tersedia.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Mohon tunggu...",
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

static MUI_ENTRY idIDChangeSystemPartition[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Partisi sistem saat ini pada komputer anda",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "pada cakram sistem",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "menggunakan format yang tidak didukung oleh ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "Agar instalasi ReactOS berhasil, program Penyetelan harus",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "mengubah partisi sistem saat ini menjadi yang baru.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "Kandidat baru partisi sistem adalah:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  Untuk menerima pilihan ini, tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  Untuk mengubah partisi sistem secara manual, tekan ESC untuk kembali",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   ke daftar pemilihan partisi, kemusian pilih atau buat partisi sistem",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   baru pada cakram sistem.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "Jika ada sistem operasi lain yang ada pada partisi sistem asli,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "Anda mungkin perlu mengkonfigurasi ulang untuk partisi sistem baru,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "atau Anda mungkin perlu mengubah partisi sistem kembali ke yang",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "asli setelah menyelesaikan instalasi ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Lanjut   ESC = Batal",
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

static MUI_ENTRY idIDConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda telah memilih untuk menghapus partisi sistem. Partisi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "sistem dapat berisi program diagnostik, program konfigurasi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "perangkat keras, program untuk memulai sistem operasi (seperti",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "ReactOS) atau program lain yang disediakan oleh pabrik perangkat keras.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Hapus partisi sistem hanya jika Anda yakin tidak ada program seperti",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "itu di partisi, atau ketika Anda yakin ingin menghapusnya.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Ketika Anda menghapus partisi, Anda mungkin tidak dapat mem-boot",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "komputer dari harddisk hingga Anda menyelesaikan Penyetelan ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  Tekan ENTER untuk menghapus partisi sistem. Anda akan diminta",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   mengonfirmasi penghapusan partisi lagi nanti.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  Tekan ESC untuk kembali ke halaman sebelumnya. Partisi tidak akan",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   dihapus.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=Lanjut  ESC=Batal",
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

static MUI_ENTRY idIDFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Format partisi",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "Penyetelan akan memformat partisi. Tekan ENTER untuk lanjut.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = Lanjut   F3 = Keluar",
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

static MUI_ENTRY idIDCheckFSEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan sekarang memeriksa partisi terpilih.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Mohon tunggu...",
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

static MUI_ENTRY idIDInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan memasang berkas ReactOS pada partisi terpilih. Pilih",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "direktori yang anda inginkan untuk dipasang:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "Untuk mengubah direktori yang disarankan, tekan BACKSPACE untuk",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "menghapus beberapa karakter dan kemudian ketikkan direktori yang",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "anda inginkan untuk memasang ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Lanjut   F3 = Keluar",
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

static MUI_ENTRY idIDFileCopyEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "Harap tunggu sementara Penyetelan ReactOS menyalin Berkas ke",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "folder instalasi ReactOS Anda.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "Mungkin butuh beberapa waktu untuk penyelesaiannya.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 Mohon tunggu...    ",
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

static MUI_ENTRY idIDBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        "Penyetelan ReactOS " KERNEL_VERSION_STR " ",
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
        "Pasang bootloader pada cakram keras (MBR dan VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "Pasang bootloader pada cakram keras (hanya VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "Pasang bootloader pada cakram disket.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "Lewati instalasi bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Lanjut   F3 = Keluar",
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

static MUI_ENTRY idIDBootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan sedang memasang the bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Memasang bootloader pada media, harap tunggu...",
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

static MUI_ENTRY idIDBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan sedang memasang the bootloader.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "Mohon masukkan cakram disket yang terformat di drive A:",
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
        "ENTER = Lanjut   F3 = Keluar",
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

static MUI_ENTRY idIDKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda ingin mengubah jenis papan ketik untuk dipasang.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tekan tombol ATAS atau BAWAH untuk memilih jenis papan ketik",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   yang diinginkan. Kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan tombol ESC untuk kembali ke halaman sebelumnya tanpa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   mengubah jenis papan ketik.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Lanjut   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY idIDLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Silahkan pilih tata letak yang akan dipasang seperti standar.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  Tekan tombol ATAS dan Bawah untuk memilih tata letak",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    papan ketik yang diinginkan. kemudian tekan ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  Tekan tombol ESC untuk kembali ke halaman sebelumnya tanpa",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   mengubah tata letak papan ketik.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = Lanjut   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY idIDPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan menpersiapkan komputer Anda untuk menyalin berkas ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Menyusun daftar salinan berkas...",
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

static MUI_ENTRY idIDSelectFSEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "Pilih sistem berkas dari daftar di bawah ini.",
        0
    },
    {
        8,
        19,
        "\x07  Tekan ATAS atau BAWAH untuk memilih sistem berkas.",
        0
    },
    {
        8,
        21,
        "\x07  Tekan ENTER untuk memformat partisi.",
        0
    },
    {
        8,
        23,
        "\x07  Tekan ESC untuk memilih partisi yang lain.",
        0
    },
    {
        0,
        0,
        "ENTER = Lanjut   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY idIDDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Anda telah memilih untuk menghapus partisi ini",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  Tekan L untuk menghapuskan partisi tersebut.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "PERINGATAN: Semua data pada partisi ini akan terhapus!",
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
        "L = Hapus Partisi   ESC = Batal   F3 = Keluar",
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

static MUI_ENTRY idIDRegistryEntries[] =
{
    {
        4,
        3,
        " Penyetelan ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "Penyetelan sedang memperbarui konfigurasi sistem.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "Membuat kumpulan registri...",
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

MUI_ERROR idIDErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "Sukses\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS tidak terpasang sepenuhnya pada komputer\n"
        "anda. Jika anda keluar sekarang, anda akan memulai\n"
        "lagi Penyetelan untuk memasang ReactOS.\n"
        "\n"
        "  \x07  Tekan ENTER untuk melanjutkan Penyetelan.\n"
        "  \x07  Tekan F3 untuk keluar dari Penyetelan.",
        "F3 = Keluar  ENTER = Lanjut"
    },
    {
        // ERROR_NO_BUILD_PATH
        "Gagal membangun jalur instalasi untuk direktori instalasi ReactOS!\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_SOURCE_PATH
        "Anda tidak dapat menghapus partisi yang memuat sumber instalasi!\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_SOURCE_DIR
        "Anda tidak dapat memasang ReactOS yang di dalamnya berisi direktori sumber instalasi!\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_NO_HDD
        "Penyetelan tidak dapat menemukan harddisk.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "Penyetelan tidak dapat menemukan drive sumber.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "Penyetelan gagal memuat berkas TXTSETUP.SIF.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "Penyetelan menemukan TXTSETUP.SIF yang rusak.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "Penyetelan menemukan tanda tangan yang tidak sah pada TXTSETUP.SIF.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "Penyetelan tidak dapat mengambil informasi drive sistem.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_WRITE_BOOT,
        "Penyetelan gagal memasang bootcode %S pada partisi sistem.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "Penyetelan gagal memuat daftar jenis komputer.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "Penyetelan gagal memuat daftar pengaturan tampilan.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "Penyetelan gagal memuat daftar jenis papan ketik.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "Penyetelan gagal memuat daftar tata letak papan ketik.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_WARN_PARTITION,
        "Penyetelan menemukan bahwa setidaknya satu harddisk berisi tabel partisi\n"
        "yang tidak kompatibel di mana tidak dapat ditangani dengan benar!\n"
        "\n"
        "Membuat atau menghapus partisi bisa merusak tabel partisi.\n"
        "\n"
        "  \x07  Tekan F3 untuk keluar dari Penyetelan.\n"
        "  \x07  Tekan ENTER untuk lanjut.",
        "F3 = Keluar  ENTER = Lanjut"
    },
    {
        // ERROR_NEW_PARTITION,
        "Anda tidak dapat membuat partisi baru di dalam\n"
        "partisi yang sudah ada!\n"
        "\n"
        "  * Tekan tombol apapun untuk lanjut.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "Penyetelan gagal memasang bootcode %S pada partisi sistem.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_NO_FLOPPY,
        "Tidak ada cakram di drive A:.",
        "ENTER = Lanjut"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "Penyetelan gagal memperbarui pengaturan tata letak papan ketik.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "Penyetelan gagal memperbarui registri pengaturan tampilan.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_IMPORT_HIVE,
        "Penyetelan gagal memasukkan kumpulan berkas.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_FIND_REGISTRY
        "Penyetelan gagal menemukan berkas data registri.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_CREATE_HIVE,
        "Penyetelan gagal membuat kumpulan registri.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "Penyetelan gagal menginisialisasi registri.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "Kabinet tidak punya berkas inf yang sah.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_CABINET_MISSING,
        "Kabinet tidak ditemukan.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "Kabinet tidak punya naskah penyetelan.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_COPY_QUEUE,
        "Penyetelan gagal membuka salinan antrean berkas.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_CREATE_DIR,
        "Penyetelan tidak dapat membuat direktori instalasi.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "Penyetelan gagal menemukan bidang '%S'\n"
        "dalam TXTSETUP.SIF.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_CABINET_SECTION,
        "Penyetelan gagal menemukan bidang '%S'\n"
        "di dalam kabinet.\n",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "Penyetelan tidak dapat membuat direktori instalasi.",
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_WRITE_PTABLE,
        "Penyetelan gagal menulis tabel partisi.\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "Penyetelan gagal menambahkan codepage ke registri.\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "Penyetelan tidak dapat mengatur lokal sistem.\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "Penyetelan gagal menambahkan tata letak papan ketik ke registri.\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_UPDATE_GEOID,
        "Penyetelan tidak dapat mengatur geo id.\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "Nama direktori tidak sah.\n"
        "\n"
        "  * Tekan tombol apapun untuk lanjut."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "Partisi yang dipilih tidak cukup besar untuk memasang ReactOS.\n"
        "Partisi instalasi harus memiliki ukuran setidaknya %lu MB.\n"
        "\n"
        "  * Tekan tombol apapun untuk lanjut.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "Anda tidak dapat membuat partisi primary atau extended baru di\n"
        "tabel partisi cakram ini karena tabel partisi ini penuh.\n"
        "\n"
        "  * Tekan tombol apapun untuk lanjut."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "Anda tidak dapat membuat lebih dari satu partisi extended per satu cakram.\n"
        "\n"
        "  * Tekan tombol apapun untuk lanjut."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "Penyetelan tidak dapat memformat partisi:\n"
        " %S\n"
        "\n"
        "ENTER = Mulai ulang komputer"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE idIDPages[] =
{
    {
        SETUP_INIT_PAGE,
        idIDSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        idIDLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        idIDWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        idIDIntroPageEntries
    },
    {
        LICENSE_PAGE,
        idIDLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        idIDDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        idIDRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        idIDUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        idIDComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        idIDDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        idIDFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        idIDSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        idIDChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        idIDConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        idIDSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        idIDFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        idIDCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        idIDDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        idIDInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        idIDPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        idIDFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        idIDKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        idIDBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        idIDLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        idIDQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        idIDSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        idIDBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        idIDBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        idIDRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING idIDStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Mohon tunggu..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Pasang   C = Buat Primary   E = Buat Extended   F3 = Keluar"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = Pasang   C = Buat Partisi Logical   F3 = Keluar"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Pasang   D = Hapus Partisi   F3 = Keluar"},
    {STRING_DELETEPARTITION,
     "   D = Hapus Partisi   F3 = Keluar"},
    {STRING_PARTITIONSIZE,
     "Ukuran partisi baru:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "Anda telah memilih untuk membuat partisi primary pada"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "Anda telah memilih untuk membuat partisi extended pada"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "Anda telah memilih untuk membuat partisi logical pada"},
    {STRING_HDPARTSIZE,
    "Mohon masukkan ukuran pada partisi baru dalam bentuk megabyte."},
    {STRING_CREATEPARTITION,
     "   ENTER = Buat Partisi   ESC = Batal   F3 = Keluar"},
    {STRING_NEWPARTITION,
    "Penyetelan membuat partisi baru pada"},
    {STRING_PARTFORMAT,
    "Partisi ini selanjutnya akan diformat."},
    {STRING_NONFORMATTEDPART,
    "Pilihlah untuk memasang ReactOS pada partisi baru atau yang belum diformat."},
    {STRING_NONFORMATTEDSYSTEMPART,
    "Partisi sistem ini belum diformat."},
    {STRING_NONFORMATTEDOTHERPART,
    "Partisi baru ini belum diformat."},
    {STRING_INSTALLONPART,
    "Penyetelan memasang ReactOS pada Partisi"},
    {STRING_CONTINUE,
    "ENTER = Lanjut"},
    {STRING_QUITCONTINUE,
    "F3 = Keluar  ENTER = Lanjut"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Mulai ulang komputer"},
    {STRING_DELETING,
     "   Menghapus berkas: %S"},
    {STRING_MOVING,
     "   Memindahkan berkas: %S ke: %S"},
    {STRING_RENAMING,
     "   Menamai berkas: %S ke: %S"},
    {STRING_COPYING,
     "   Menyalin berkas: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Penyetelan sedang menyalin berkas..."},
    {STRING_REGHIVEUPDATE,
    "   Memperbarui kumpulan registri..."},
    {STRING_IMPORTFILE,
    "   Memasukkan %S..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   Memperbarui pengaturan registri tampilan..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Memperbarui pengaturan lokal..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Memperbarui pengaturan tata letak papan ketik..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Menambahkan informasi halaman kode..."},
    {STRING_DONE,
    "   Selesai..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Mulai ulang komputer"},
    {STRING_REBOOTPROGRESSBAR,
    " Komputer anda akan dimulai ulang pada %li detik... "},
    {STRING_CONSOLEFAIL1,
    "Tidak dapat membuka konsol\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "Secara umum penyebab dari ini adalah menggunakan papan ketik USB\r\n"},
    {STRING_CONSOLEFAIL3,
    "Papan ketik USB belum didukung sepenuhnya\r\n"},
    {STRING_FORMATTINGPART,
    "Penyetelan sedang memformat partisi..."},
    {STRING_CHECKINGDISK,
    "Penyetelan sedang memeriksa cakram..."},
    {STRING_FORMATDISK1,
    " Format partisi sebagai sistem berkas %S (format cepat) "},
    {STRING_FORMATDISK2,
    " Format partisi sebagai sistem berkas %S  "},
    {STRING_KEEPFORMAT,
    " Tetapkan sistem berkas seperti ini (tanpa perubahan) "},
    {STRING_HDDISK1,
    "%s."},
    {STRING_HDDISK2,
    "pada %s."},
    {STRING_PARTTYPE,
    "Jenis 0x%02x"},
    {STRING_HDDINFO1,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ) [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) pada %wZ [%s]"},
    {STRING_HDDINFO2,
    // "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu [%s]"
    "%I64u %s Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) [%s]"},
    {STRING_UNPSPACE,
    "Ukuran yang belum dipartisi"},
    {STRING_MAXSIZE,
    "MB (maks. %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "Partisi Extended"},
    {STRING_UNFORMATTED,
    "Baru (Belum Diformat)"},
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
    "Menambah tata letak papan ketik"},
    {0, 0}
};
