<?php
/** Malay (Bahasa Melayu)
 *
 * @ingroup Language
 * @file
 *
 * @author Aurora
 * @author Aviator
 * @author Izzudin
 * @author Kurniasan
 * @author Putera Luqman Tunku Andre
 * @author לערי ריינהארט
 */

/**
 * CHANGELOG
 * =========
 * Init - This localisation is based on a file kindly donated by the folks at MIMOS
 * http://www.asiaosc.org/enwiki/page/Knowledgebase_Home.html
 * Sep 2007 - Rewritten by the folks at ms.wikipedia.org
 */

$defaultDateFormat = 'dmy';

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Khas',
	NS_MAIN           => '',
	NS_TALK           => 'Perbincangan',
	NS_USER           => 'Pengguna',
	NS_USER_TALK      => 'Perbincangan_Pengguna',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Perbincangan_$1',
	NS_IMAGE          => 'Imej',
	NS_IMAGE_TALK     => 'Perbincangan_Imej',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Perbincangan_MediaWiki',
	NS_TEMPLATE       => 'Templat',
	NS_TEMPLATE_TALK  => 'Perbincangan_Templat',
	NS_HELP           => 'Bantuan',
	NS_HELP_TALK      => 'Perbincangan_Bantuan',
	NS_CATEGORY       => 'Kategori',
	NS_CATEGORY_TALK  => 'Perbincangan_Kategori',
);

$namespaceAliases = array(
	'Istimewa'            => NS_SPECIAL,
	'Perbualan'           => NS_TALK,
	'Perbualan_Pengguna'  => NS_USER_TALK,
	'Perbualan_$1'        => NS_PROJECT_TALK,
	'Imej_Perbualan'      => NS_IMAGE_TALK,
	'MediaWiki_Perbualan' => NS_MEDIAWIKI_TALK,
	'Perbualan_Templat'   => NS_TEMPLATE_TALK,
	'Perbualan_Kategori'  => NS_CATEGORY_TALK,
	'Perbualan_Bantuan'   => NS_HELP_TALK,
);

$skinNames = array(
	'standard' => 'Klasik',
	'simple'   => 'Ringkas',
	'modern'   => 'Moden',
);

$specialPageAliases = array(
	'DoubleRedirects'         => array( 'Lencongan_berganda', 'Pelencongan_berganda' ),
	'BrokenRedirects'         => array( 'Lencongan_rosak', 'Pelencongan_rosak' ),
	'Disambiguations'         => array( 'Penyahtaksaan' ),
	'Userlogin'               => array( 'Log_masuk' ),
	'Userlogout'              => array( 'Log_keluar' ),
	'CreateAccount'           => array( 'Buka_akaun' ),
	'Preferences'             => array( 'Keutamaan' ),
	'Watchlist'               => array( 'Senarai_pantau' ),
	'Recentchanges'           => array( 'Perubahan_terkini' ),
	'Upload'                  => array( 'Muat_naik' ),
	'Imagelist'               => array( 'Senarai_imej' ),
	'Newimages'               => array( 'Imej_baru' ),
	'Listusers'               => array( 'Senarai_pengguna' ),
	'Listgrouprights'         => array( 'Senarai_hak_kumpulan' ),
	'Statistics'              => array( 'Statistik' ),
	'Randompage'              => array( 'Laman_rawak' ),
	'Lonelypages'             => array( 'Laman_yatim' ),
	'Uncategorizedpages'      => array( 'Laman_tanpa_kategori' ),
	'Uncategorizedcategories' => array( 'Kategori_tanpa_kategori' ),
	'Uncategorizedimages'     => array( 'Imej_tanpa_kategori' ),
	'Uncategorizedtemplates'  => array( 'Templat_tanpa_kategori' ),
	'Unusedcategories'        => array( 'Kategori_tak_digunakan' ),
	'Unusedimages'            => array( 'Imej_tak_digunakan' ),
	'Wantedpages'             => array( 'Laman_dikehendaki' ),
	'Wantedcategories'        => array( 'Kategori_dikehendaki' ),
	'Missingfiles'            => array( 'Laman_hilang' ),
	'Mostlinked'              => array( 'Laman_dipaut_terbanyak' ),
	'Mostlinkedcategories'    => array( 'Kategori_dipaut_terbanyak' ),
	'Mostlinkedtemplates'     => array( 'Templat_dipaut_terbanyak' ),
	'Mostcategories'          => array( 'Kategori_terbanyak' ),
	'Mostimages'              => array( 'Imej_terbanyak' ),
	'Mostrevisions'           => array( 'Semakan_terbanyak' ),
	'Fewestrevisions'         => array( 'Semakan_tersikit' ),
	'Shortpages'              => array( 'Laman_pendek' ),
	'Longpages'               => array( 'Laman_panjang' ),
	'Newpages'                => array( 'Laman_baru' ),
	'Ancientpages'            => array( 'Laman_lapuk' ),
	'Deadendpages'            => array( 'Laman_buntu' ),
	'Protectedpages'          => array( 'Laman_dilindungi' ),
	'Protectedtitles'         => array( 'Tajuk_dilindungi' ),
	'Allpages'                => array( 'Semua_laman' ),
	'Prefixindex'             => array( 'Indeks_awalan' ),
	'Ipblocklist'             => array( 'Sekatan_IP' ),
	'Specialpages'            => array( 'Laman_khas' ),
	'Contributions'           => array( 'Sumbangan' ),
	'Emailuser'               => array( 'E-mel_pengguna' ),
	'Confirmemail'            => array( 'Sahkan_e-mel' ),
	'Whatlinkshere'           => array( 'Pautan_ke' ),
	'Recentchangeslinked'     => array( 'Perubahan_berkaitan' ),
	'Movepage'                => array( 'Pindah_laman' ),
	'Blockme'                 => array( 'Sekat_saya' ),
	'Booksources'             => array( 'Sumber_buku' ),
	'Categories'              => array( 'Kategori' ),
	'Export'                  => array( 'Eksport' ),
	'Version'                 => array( 'Versi' ),
	'Allmessages'             => array( 'Semua_pesanan', 'Semua_mesej' ),
	'Log'                     => array( 'Log' ),
	'Blockip'                 => array( 'Sekat_IP' ),
	'Undelete'                => array( 'Nyahhapus' ),
	'Import'                  => array( 'Import' ),
	'Lockdb'                  => array( 'Kunci_pangkalan_data' ),
	'Unlockdb'                => array( 'Buka_kunci_pangkalan_data' ),
	'Userrights'              => array( 'Hak_pengguna' ),
	'MIMEsearch'              => array( 'Gelintar_MIME' ),
	'FileDuplicateSearch'     => array( 'Cari_fail_berganda' ),
	'Unwatchedpages'          => array( 'Laman_tak_dipantau' ),
	'Listredirects'           => array( 'Senarai_lencongan', 'Senarai_pelencongan' ),
	'Revisiondelete'          => array( 'Hapus_semakan' ),
	'Unusedtemplates'         => array( 'Templat_tak_digunakan' ),
	'Randomredirect'          => array( 'Lencongan_rawak', 'Pelencongan_rawak' ),
	'Mypage'                  => array( 'Laman_saya' ),
	'Mytalk'                  => array( 'Perbincangan_saya' ),
	'Mycontributions'         => array( 'Sumbangan_saya' ),
	'Listadmins'              => array( 'Senarai_pentadbir' ),
	'Listbots'                => array( 'Senarai_bot' ),
	'Popularpages'            => array( 'Laman_popular' ),
	'Search'                  => array( 'Gelintar' ),
	'Resetpass'               => array( 'Lupa_kata_laluan' ),
	'Withoutinterwiki'        => array( 'Laman_tanpa_pautan_bahasa' ),
	'MergeHistory'            => array( 'Gabung_sejarah' ),
	'Filepath'                => array( 'Laluan_fail' ),
	'Invalidateemail'         => array( 'Alamat_surat_elektronik_tidak_sah' ),
	'Blankpage'               => array( 'Laman_kosong' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Gariskan pautan:',
'tog-highlightbroken'         => 'Formatkan pautan rosak <a href="" class="new">seperti ini</a> (ataupun seperti ini<a href="" class="internal">?</a>)',
'tog-justify'                 => 'Laraskan teks perenggan',
'tog-hideminor'               => 'Sembunyikan suntingan kecil dalam laman perubahan terkini',
'tog-extendwatchlist'         => 'Kembangkan senarai pantau',
'tog-usenewrc'                => 'Pertingkatkan laman perubahan terkini (JavaScript)',
'tog-numberheadings'          => 'Nomborkan tajuk secara automatik',
'tog-showtoolbar'             => 'Tunjukkan bar sunting (JavaScript)',
'tog-editondblclick'          => 'Sunting laman ketika dwiklik (JavaScript)',
'tog-editsection'             => 'Aktifkan penyuntingan bahagian melalui pautan [sunting]',
'tog-editsectiononrightclick' => 'Aktifkan penyuntingan bahagian melalui klik kanan pada tajuk bahagian (JavaScript)',
'tog-showtoc'                 => 'Tunjukkan senarai kandungan bagi rencana melebihi 3 tajuk',
'tog-rememberpassword'        => 'Ingat status log masuk saya pada komputer ini',
'tog-editwidth'               => 'Kotak sunting mencapai lebar penuh',
'tog-watchcreations'          => 'Tambahkan laman yang saya cipta ke dalam senarai pantau',
'tog-watchdefault'            => 'Tambahkan laman yang saya sunting ke dalam senarai pantau',
'tog-watchmoves'              => 'Tambahkan laman yang saya pindahkan ke dalam senarai pantau',
'tog-watchdeletion'           => 'Tambahkan laman yang saya hapuskan ke dalam senarai pantau',
'tog-minordefault'            => 'Tandakan suntingan kecil secara lalai',
'tog-previewontop'            => 'Tunjukkan pratonton di atas kotak sunting',
'tog-previewonfirst'          => 'Tunjukkan pratonton ketika penyuntingan pertama',
'tog-nocache'                 => 'Matikan penyimpanan sementara bagi laman',
'tog-enotifwatchlistpages'    => 'E-melkan saya apabila berlaku perubahan pada laman yang dipantau',
'tog-enotifusertalkpages'     => 'E-melkan saya apabila berlaku perubahan pada laman perbincangan saya',
'tog-enotifminoredits'        => 'Juga e-melkan saya apabila berlaku penyuntingan kecil',
'tog-enotifrevealaddr'        => 'Serlahkan alamat e-mel saya dalam e-mel pemberitahuan',
'tog-shownumberswatching'     => 'Tunjukkan bilangan pemantau',
'tog-fancysig'                => 'Tandatangan mentah (tanpa pautan automatik)',
'tog-externaleditor'          => 'Gunakan penyunting luar secara lalai',
'tog-externaldiff'            => 'Gunakan pembeza luar secara lalai',
'tog-showjumplinks'           => 'Aktifkan pautan boleh capai "lompat kepada"',
'tog-uselivepreview'          => 'Gunakan pratonton langsung (JavaScript) (masih dalam uji kaji)',
'tog-forceeditsummary'        => 'Tanya saya jika ringkasan suntingan kosong',
'tog-watchlisthideown'        => 'Sembunyikan suntingan saya daripada senarai pantau',
'tog-watchlisthidebots'       => 'Sembunyikan suntingan bot daripada senarai pantau',
'tog-watchlisthideminor'      => 'Sembunyikan suntingan kecil daripada senarai pantau',
'tog-nolangconversion'        => 'Matikan penukaran kelainan',
'tog-ccmeonemails'            => 'Kirim kepada saya salinan bagi e-mel yang saya hantar kepada orang lain',
'tog-diffonly'                => 'Jangan tunjukkan kandungan laman di bawah perbezaan',
'tog-showhiddencats'          => 'Tunjukkan kategori tersembunyi',

'underline-always'  => 'Sentiasa',
'underline-never'   => 'Jangan',
'underline-default' => 'Ikut tetapan pelayar',

'skinpreview' => '(Pratonton)',

# Dates
'sunday'        => 'Ahad',
'monday'        => 'Isnin',
'tuesday'       => 'Selasa',
'wednesday'     => 'Rabu',
'thursday'      => 'Khamis',
'friday'        => 'Jumaat',
'saturday'      => 'Sabtu',
'sun'           => 'Aha',
'mon'           => 'Isn',
'tue'           => 'Sel',
'wed'           => 'Rab',
'thu'           => 'Kha',
'fri'           => 'Jum',
'sat'           => 'Sab',
'january'       => 'Januari',
'february'      => 'Februari',
'march'         => 'Mac',
'april'         => 'April',
'may_long'      => 'Mei',
'june'          => 'Jun',
'july'          => 'Julai',
'august'        => 'Ogos',
'september'     => 'September',
'october'       => 'Oktober',
'november'      => 'November',
'december'      => 'Disember',
'january-gen'   => 'Januari',
'february-gen'  => 'Februari',
'march-gen'     => 'Mac',
'april-gen'     => 'April',
'may-gen'       => 'Mei',
'june-gen'      => 'Jun',
'july-gen'      => 'Julai',
'august-gen'    => 'Ogos',
'september-gen' => 'September',
'october-gen'   => 'Oktober',
'november-gen'  => 'November',
'december-gen'  => 'Disember',
'jan'           => 'Jan',
'feb'           => 'Feb',
'mar'           => 'Mac',
'apr'           => 'Apr',
'may'           => 'Mei',
'jun'           => 'Jun',
'jul'           => 'Jul',
'aug'           => 'Ogo',
'sep'           => 'Sep',
'oct'           => 'Okt',
'nov'           => 'Nov',
'dec'           => 'Dis',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategori|Kategori}}',
'category_header'                => 'Laman-laman dalam kategori "$1"',
'subcategories'                  => 'Subkategori',
'category-media-header'          => 'Media-media dalam kategori "$1"',
'category-empty'                 => "''Kategori ini tidak mengandungi sebarang laman atau media.''",
'hidden-categories'              => '{{PLURAL:$1|Kategori|Kategori}}',
'hidden-category-category'       => 'Kategori tersembunyi', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Kategori ini mengandungi sebuah subkategori berikut.|Berikut ialah $1 daripada $2 buah subkategori dalam kategori ini.}}',
'category-subcat-count-limited'  => 'Kategori ini mengandungi $1 subkategori berikut.',
'category-article-count'         => '{{PLURAL:$2|Kategori ini mengandungi sebuah laman berikut.|Berikut ialah $1 daripada $2 buah laman dalam kategori ini.}}',
'category-article-count-limited' => '$1 laman berikut terdapat dalam kategori ini.',
'category-file-count'            => '{{PLURAL:$2|Kategori ini mengandungi sebuah fail berikut.|Berikut ialah $1 daripada $2 buah fail dalam kategori ini.}}',
'category-file-count-limited'    => '$1 fail berikut terdapat dalam kategori ini.',
'listingcontinuesabbrev'         => 'samb.',

'mainpagetext'      => "<big>'''MediaWiki telah dipasang.'''</big>",
'mainpagedocfooter' => 'Sila rujuk [http://meta.wikimedia.org/wiki/Help:Contents Panduan Penggunaan] untuk maklumat mengenai penggunaan perisian wiki ini.

== Untuk bermula ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Senarai tetapan konfigurasi]
* [http://www.mediawiki.org/wiki/Manual:FAQ Soalan Lazim MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Senarai mel bagi keluaran MediaWiki]',

'about'          => 'Perihal',
'article'        => 'Laman kandungan',
'newwindow'      => '(dibuka di tetingkap baru)',
'cancel'         => 'Batal',
'qbfind'         => 'Cari',
'qbbrowse'       => 'Semak imbas',
'qbedit'         => 'Sunting',
'qbpageoptions'  => 'Laman ini',
'qbpageinfo'     => 'Konteks',
'qbmyoptions'    => 'Laman-laman saya',
'qbspecialpages' => 'Laman khas',
'moredotdotdot'  => 'Lagi...',
'mypage'         => 'Laman saya',
'mytalk'         => 'Perbincangan saya',
'anontalk'       => 'Perbincangan bagi IP ini',
'navigation'     => 'Navigasi',
'and'            => 'dan',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Ralat',
'returnto'          => 'Kembali ke $1.',
'tagline'           => 'Daripada {{SITENAME}}.',
'help'              => 'Bantuan',
'search'            => 'Gelintar',
'searchbutton'      => 'Cari',
'go'                => 'Pergi',
'searcharticle'     => 'Pergi',
'history'           => 'Sejarah laman',
'history_short'     => 'Sejarah',
'updatedmarker'     => 'dikemaskinikan sejak kunjungan terakhir saya',
'info_short'        => 'Maklumat',
'printableversion'  => 'Versi boleh cetak',
'permalink'         => 'Pautan kekal',
'print'             => 'Cetak',
'edit'              => 'Sunting',
'create'            => 'Cipta',
'editthispage'      => 'Sunting laman ini',
'create-this-page'  => 'Cipta laman ini',
'delete'            => 'Hapus',
'deletethispage'    => 'Hapuskan laman ini',
'undelete_short'    => 'Nyahhapus {{PLURAL:$1|satu suntingan|$1 suntingan}}',
'protect'           => 'Lindung',
'protect_change'    => 'ubah',
'protectthispage'   => 'Lindungi laman ini',
'unprotect'         => 'Nyahlindung',
'unprotectthispage' => 'Nyahlindung laman ini',
'newpage'           => 'Laman baru',
'talkpage'          => 'Bincangkan laman ini',
'talkpagelinktext'  => 'Perbincangan',
'specialpage'       => 'Laman Khas',
'personaltools'     => 'Alatan peribadi',
'postcomment'       => 'Kirim komen',
'articlepage'       => 'Lihat laman kandungan',
'talk'              => 'Perbincangan',
'views'             => 'Pandangan',
'toolbox'           => 'Alatan',
'userpage'          => 'Lihat laman pengguna',
'projectpage'       => 'Lihat laman projek',
'imagepage'         => 'Lihat laman imej',
'mediawikipage'     => 'Lihat laman pesanan',
'templatepage'      => 'Lihat laman templat',
'viewhelppage'      => 'Lihat laman bantuan',
'categorypage'      => 'Lihat laman kategori',
'viewtalkpage'      => 'Lihat perbincangan',
'otherlanguages'    => 'Bahasa lain',
'redirectedfrom'    => '(Dilencongkan dari $1)',
'redirectpagesub'   => 'Laman lencongan',
'lastmodifiedat'    => 'Laman ini diubah buat kali terakhir pada $2, $1.', # $1 date, $2 time
'viewcount'         => 'Laman ini telah dilihat {{PLURAL:$1|sekali|sebanyak $1 kali}}.',
'protectedpage'     => 'Laman dilindungi',
'jumpto'            => 'Lompat ke:',
'jumptonavigation'  => 'navigasi',
'jumptosearch'      => 'gelintar',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Perihal {{SITENAME}}',
'aboutpage'            => 'Project:Perihal',
'bugreports'           => 'Laporan pepijat',
'bugreportspage'       => 'Project:Laporan pepijat',
'copyright'            => 'Kandungan disediakan dengan $1.',
'copyrightpagename'    => 'Hak cipta {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Hak cipta',
'currentevents'        => 'Peristiwa semasa',
'currentevents-url'    => 'Project:Peristiwa semasa',
'disclaimers'          => 'Penolak tuntutan',
'disclaimerpage'       => 'Project:Penolak tuntutan',
'edithelp'             => 'Bantuan menyunting',
'edithelppage'         => 'Help:Menyunting',
'faq'                  => 'Soalan Lazim',
'faqpage'              => 'Project:Soalan Lazim',
'helppage'             => 'Help:Kandungan',
'mainpage'             => 'Laman Utama',
'mainpage-description' => 'Laman Utama',
'policy-url'           => 'Project:Dasar',
'portal'               => 'Portal komuniti',
'portal-url'           => 'Project:Portal Komuniti',
'privacy'              => 'Dasar privasi',
'privacypage'          => 'Project:Dasar privasi',

'badaccess'        => 'Tidak dibenarkan',
'badaccess-group0' => 'Anda tidak dibenarkan melaksanakan tindakan ini.',
'badaccess-group1' => 'Tindakan ini hanya boleh dilakukan oleh pengguna dalam kumpulan $1.',
'badaccess-group2' => 'Tindakan ini hanya boleh dilakukan oleh pengguna dalam kumpulan $1.',
'badaccess-groups' => 'Tindakan ini hanya boleh dilakukan oleh pengguna dalam kumpulan $1.',

'versionrequired'     => 'MediaWiki versi $1 diperlukan',
'versionrequiredtext' => 'MediaWiki versi $1 diperlukan untuk menggunakan laman ini. Sila lihat [[Special:Version|laman versi]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Diambil daripada "$1"',
'youhavenewmessages'      => 'Anda mempunyai $1 ($2).',
'newmessageslink'         => 'pesanan baru',
'newmessagesdifflink'     => 'perubahan terakhir',
'youhavenewmessagesmulti' => 'Anda telah menerima pesanan baru pada $1',
'editsection'             => 'sunting',
'editold'                 => 'sunting',
'viewsourceold'           => 'lihat sumber',
'editsectionhint'         => 'Sunting bahagian: $1',
'toc'                     => 'Isi kandungan',
'showtoc'                 => 'bentang',
'hidetoc'                 => 'lipat',
'thisisdeleted'           => 'Lihat atau pulihkan $1?',
'viewdeleted'             => 'Lihat $1?',
'restorelink'             => '{{PLURAL:$1|satu|$1}} suntingan dihapuskan',
'feedlinks'               => 'Suapan:',
'feed-invalid'            => 'Jenis suapan langganan tidak sah.',
'feed-unavailable'        => 'Tiada suapan pensindiketan di {{SITENAME}}',
'site-rss-feed'           => 'Suapan RSS $1',
'site-atom-feed'          => 'Suapan Atom $1',
'page-rss-feed'           => 'Suapan RSS "$1"',
'page-atom-feed'          => 'Suapan Atom "$1"',
'red-link-title'          => '$1 (belum ditulis)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Laman',
'nstab-user'      => 'Laman pengguna',
'nstab-media'     => 'Laman media',
'nstab-special'   => 'Khas',
'nstab-project'   => 'Laman projek',
'nstab-image'     => 'Imej',
'nstab-mediawiki' => 'Pesanan',
'nstab-template'  => 'Templat',
'nstab-help'      => 'Laman bantuan',
'nstab-category'  => 'Kategori',

# Main script and global functions
'nosuchaction'      => 'Tindakan tidak dikenali',
'nosuchactiontext'  => 'Tindakan yang dinyatakan dalam URL
ini tidak dikenali oleh perisian wiki ini',
'nosuchspecialpage' => 'Laman khas tidak wujud',
'nospecialpagetext' => "<big>'''Anda telah meminta laman khas yang tidak sah.'''</big>

Senarai laman khas yang sah boleh dilihat di [[Special:SpecialPages]].",

# General errors
'error'                => 'Ralat',
'databaseerror'        => 'Ralat pangkalan data',
'dberrortext'          => 'Terdapat kesalahan pada sintaks pertanyaan pangkalan data.
Ini mungkin menandakan pepijat dalam perisian wiki ini.
Pertanyaan pangkalan data yang terakhir ialah:
<blockquote><tt>$1</tt></blockquote>
dari dalam fungsi "<tt>$2</tt>".
MySQL memulangkan ralat "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Terdapat kesalahan sintaks pada pertanyaan pangkalan data. Pertanyaan terakhir ialah: "$1" dari dalam fungsi "$2". MySQL memulangkan ralat "$3: $4".',
'noconnect'            => 'Wiki ini dilanda masalah teknikal dan tidak dapat menghubungi pelayan pangkalan data.<br />$1',
'nodb'                 => 'Tidak dapat memilih pangkalan data $1',
'cachederror'          => 'Berikut ialah salinan simpanan bagi laman yang diminta, dan barangkali bukan yang terkini.',
'laggedslavemode'      => 'Amaran: Laman ini mungkin bukan yang terkini.',
'readonly'             => 'Pangkalan data dikunci',
'enterlockreason'      => 'Sila nyatakan sebab penguncian dan jangkaan
bila kunci ini akan dibuka.',
'readonlytext'         => 'Pangkalan data sedang dikunci. Hal ini mungkin disebabkan oleh penyenggaraan rutin, dan akan dibuka semula selepas proses penyenggaraan ini siap.

Pentadbir yang menguncinya memberi penjelasan ini: $1',
'missing-article'      => 'Teks bagi laman "$1" $2 tidak dijumpai dalam pangkalan data.

Perkara ini biasanya disebabkan oleh perbuatan mengikuti pautan perbezaan yang lama atau pautan ke laman yang telah dihapuskan.

Jika bukan ini sebabnya, anda mungkin telah menjumpai pepijat dalam perisian ini.
Sila catat URL bagi laman ini dan laporkan perkara ini kepada seorang [[Special:ListUsers/sysop|pentadbir]].',
'missingarticle-rev'   => '(semakan $1)',
'missingarticle-diff'  => '(perbezaan $1-$2)',
'readonly_lag'         => 'Pangkalan data telah dikunci secara automatik sementara semua pelayan pangkalan data diselaraskan.',
'internalerror'        => 'Ralat dalaman',
'internalerror_info'   => 'Ralat dalaman: $1',
'filecopyerror'        => 'Fail "$1" tidak dapat disalin kepada "$2".',
'filerenameerror'      => 'Nama fail "$1" tidak dapat ditukarkan kepada "$2".',
'filedeleteerror'      => 'Fail "$1" tidak dapat dihapuskan.',
'directorycreateerror' => 'Directory "$1" gagal diciptakan.',
'filenotfound'         => 'Fail "$1" tidak dijumpai.',
'fileexistserror'      => 'File "$1" tidak dapat ditulis: fail telah pun wujud',
'unexpected'           => 'Nilai tanpa diduga: "$1"="$2".',
'formerror'            => 'Ralat: borang tidak dapat dikirim.',
'badarticleerror'      => 'Tindakan ini tidak boleh dilaksanakan pada laman ini.',
'cannotdelete'         => 'Laman atau imej yang dinyatakan tidak dapat dihapuskan. (Ia mungkin telah pun dihapuskan oleh orang yang lain.)',
'badtitle'             => 'Tajuk tidak sah',
'badtitletext'         => 'Tajuk laman yang diminta tidak sah, kosong, ataupun tajuk antara bahasa atau tajuk antara wiki yang salah dipaut. Ia mungkin mengandungi aksara yang tidak dibenarkan.',
'perfdisabled'         => 'Harap maaf! Ciri ini telah dipadamkan buat sementara kerana ia melambatkan pangkalan data sehingga wiki ini tidak dapat digunakan.',
'perfcached'           => 'Data berikut diambil daripada simpanan sementara dan mungkin bukan yang terkini.',
'perfcachedts'         => 'Data berikut berada dalam simpanan sementara dan dikemaskinikan buat kali terakhir pada $1.',
'querypage-no-updates' => 'Pengkemaskinian bagi laman ini dimatikan. Data yang ditunjukkan di sini tidak disegarkan semula.',
'wrong_wfQuery_params' => 'Parameter salah bagi wfQuery()<br />
Fungsi: $1<br />
Pertanyaan: $2',
'viewsource'           => 'Lihat sumber',
'viewsourcefor'        => 'bagi $1',
'actionthrottled'      => 'Tindakan dikawal',
'actionthrottledtext'  => 'Untuk mencegah spam, anda dihadkan daripada melakukan tindakan ini berulang kali dalam ruang waktu yang singkat, dan anda telah melebihi had tersebut. Sila cuba lagi selepas beberapa minit.',
'protectedpagetext'    => 'Laman ini telah dikunci untuk menghalang penyuntingan.',
'viewsourcetext'       => 'Anda boleh melihat dan menyalin sumber bagi laman ini:',
'protectedinterface'   => 'Laman ini menyediakan teks antara muka bagi perisian ini, akan tetapi dikunci untuk menghalang penyalahgunaan.',
'editinginterface'     => "'''Amaran:''' Anda sedang menyunting laman yang digunakan untuk menghasilkan teks antara muka bagi perisian ini. Sebarang perubahan terhadap laman ini akan menjejaskan rupa antara muka bagi pengguna-pengguna lain. Untuk melakukan penterjemahan, anda boleh menggunakan [http://translatewiki.net/wiki/Main_Page?setlang=ms Betawiki], sebuah projek penyetempatan MediaWiki.",
'sqlhidden'            => '(Pertanyaan SQL tidak ditunjukkan)',
'cascadeprotected'     => 'Laman ini telah dilindungi daripada penyuntingan oleh pengguna selain penyelia, kerana ia termasuk dalam {{PLURAL:$1|laman|laman-laman}} berikut, yang dilindungi dengan secara "melata": $2',
'namespaceprotected'   => "Anda tidak mempunyai keizinan untuk menyunting laman dalam ruang nama '''$1'''.",
'customcssjsprotected' => 'Anda tidak mempunyai keizinan untuk menyunting laman ini kerana ia mengandungi tetapan peribadi pengguna lain.',
'ns-specialprotected'  => 'Laman dalam ruang nama {{ns:special}} tidak boleh disunting.',
'titleprotected'       => 'Tajuk ini telah dilindungi oleh [[User:$1|$1]]. Sebab yang diberikan ialah <i>$2</i>.',

# Virus scanner
'virus-badscanner'     => 'Tatarajah rosak: pengimbas virus tidak diketahui: <i>$1</i>',
'virus-scanfailed'     => 'pengimbasan gagal (kod $1)',
'virus-unknownscanner' => 'antivirus tidak dikenali:',

# Login and logout pages
'logouttitle'                => 'Log keluar',
'logouttext'                 => "<strong>Anda telah log keluar.</strong>

Anda boleh terus menggunakan {{SITENAME}} sebagai pengguna tanpa nama, atau anda boleh [[Special:UserLogin|log masuk sekali lagi]] sebagai pengguna lain. Sila ambil perhatian bahawa sesetengah laman mungkin dipaparkan seolah-olah anda masih log masuk. Anda boleh menyelesaikan masalah tersebut dengan hanya mengosongkan data simpanan (''cache'') pelayar anda.",
'welcomecreation'            => '== Selamat datang, $1! ==

Akaun anda telah dibuka. Jangan lupa untuk mengubah keutamaan {{SITENAME}} anda.',
'loginpagetitle'             => 'Log masuk',
'yourname'                   => 'Nama pengguna:',
'yourpassword'               => 'Kata laluan:',
'yourpasswordagain'          => 'Ulangi kata laluan:',
'remembermypassword'         => 'Ingat log masuk saya di komputer ini',
'yourdomainname'             => 'Domain anda:',
'externaldberror'            => 'Berlaku ralat pangkalan data bagi pengesahan luar atau anda tidak dibenarkan mengemaskinikan akaun luar anda.',
'loginproblem'               => '<b>Berlaku sedikit masalah ketika log masuk.</b><br />Sila cuba lagi!',
'login'                      => 'Log masuk',
'nav-login-createaccount'    => 'Log masuk / buka akaun',
'loginprompt'                => "Anda mesti membenarkan ''cookies'' untuk log masuk ke dalam {{SITENAME}}.",
'userlogin'                  => 'Log masuk / buka akaun',
'logout'                     => 'Log keluar',
'userlogout'                 => 'Log keluar',
'notloggedin'                => 'Belum log masuk',
'nologin'                    => 'Belum mempunyai akaun? $1.',
'nologinlink'                => 'Buka akaun baru',
'createaccount'              => 'Buka akaun',
'gotaccount'                 => 'Sudah mempunyai akaun? $1.',
'gotaccountlink'             => 'Log masuk',
'createaccountmail'          => 'melalui e-mel',
'badretype'                  => 'Sila ulangi kata laluan dengan betul.',
'userexists'                 => 'Nama pengguna yang anda masukkan telah pun digunakan. Sila pilih nama yang lain.',
'youremail'                  => 'E-mel:',
'username'                   => 'Nama pengguna:',
'uid'                        => 'ID pengguna:',
'prefs-memberingroups'       => 'Ahli kumpulan:',
'yourrealname'               => 'Nama sebenar:',
'yourlanguage'               => 'Bahasa:',
'yourvariant'                => 'Varian',
'yournick'                   => 'Nama samaran:',
'badsig'                     => 'Tandatangan mentah tidak sah; sila semak tag HTML.',
'badsiglength'               => 'Nama samaran terlalu panjang; ia mestilah tidak melebihi $1 aksara.',
'email'                      => 'E-mel',
'prefs-help-realname'        => 'Nama sebenar adalah tidak wajib. Jika dinyatakan, ia akan digunakan untuk mengiktiraf karya anda.',
'loginerror'                 => 'Ralat log masuk',
'prefs-help-email'           => 'Alamat e-mel adalah tidak wajib. Aakan tetapi, jika anda terlupa kata laluan, anda boleh meminta kata laluan yang baru dikirim kepada e-mel anda. Anda juga boleh membenarkan orang lain menghubungi anda melalui laman pengguna atau laman perbincangan tanpa mendedahkan identiti anda.',
'prefs-help-email-required'  => 'Alamat e-mel adalah wajib.',
'nocookiesnew'               => "Akaun anda telah dibuka, tetapi anda belum log masuk. {{SITENAME}} menggunakan ''cookies'' untuk mencatat status log masuk pengguna. Sila aktifkan sokongan ''cookies'' pada pelayar anda, kemudian log masuk dengan nama pengguna dan kata laluan baru anda.",
'nocookieslogin'             => "{{SITENAME}} menggunakan ''cookies'' untuk mencatat status log masuk pengguna. Sila aktifkan sokongan ''cookies'' pada pelayar anda dan cuba lagi.",
'noname'                     => 'Nama pengguna tidak sah.',
'loginsuccesstitle'          => 'Berjaya log masuk',
'loginsuccess'               => "'''Anda telah log masuk ke dalam {{SITENAME}} sebagai \"\$1\".'''",
'nosuchuser'                 => 'Pengguna "$1" tidak wujud. Sila semak ejaan anda atau [[Special:Userlogin/signup|buka akaun baru]].',
'nosuchusershort'            => 'Pengguna "<nowiki>$1</nowiki>" tidak wujud. Sila semak ejaan anda.',
'nouserspecified'            => 'Sila nyatakan nama pengguna.',
'wrongpassword'              => 'Kata laluan yang dimasukkan adalah salah. Sila cuba lagi.',
'wrongpasswordempty'         => 'Kata laluan yang dimasukkan adalah kosong. Sila cuba lagi.',
'passwordtooshort'           => 'Kata laluan anda tidak sah atau terlalu pendek. Panjangnya mestilah sekurang-kurangnya $1 aksara dan berbeza daripada nama pengguna anda.',
'mailmypassword'             => 'E-melkan kata laluan baru',
'passwordremindertitle'      => 'Pengingat kata laluan daripada {{SITENAME}}',
'passwordremindertext'       => 'Seseorang (mungkin anda, dari alamat IP $1) meminta kami menghantar kata laluan baru untuk {{SITENAME}} ($4). Kata laluan sementara baru untuk pengguna "$2" ialah "$3". Untuk menamatkan prosedur ini, anda perlu log masuk dan tetapkan kata laluan yang baru dengan segera.

Jika anda tidak membuat permintaan ini, atau anda telah pun mengingati semula kata laluan anda dan tidak mahu menukarnya, anda boleh mengabaikan pesanan ini dan terus menggunakan kata laluan yang sedia ada.',
'noemail'                    => 'Tiada alamat e-mel direkodkan bagi pengguna "$1".',
'passwordsent'               => 'Kata laluan baru telah dikirim kepada alamat
e-mel yang didaftarkan oleh "$1".
Sila log masuk semula setelah anda menerima e-mel tersebut.',
'blocked-mailpassword'       => 'Alamat IP anda telah disekat daripada sebarang penyuntingan, oleh itu, untuk
mengelak penyalahgunaan, anda tidak dibenarkan menggunakan ciri pemulihan kata laluan.',
'eauthentsent'               => 'Sebuah e-mel pengesahan telah dikirim kepada alamat e-mel tersebut.
Sebelum e-emel lain boleh dikirim kepada alamat tersebut, anda perlu mengikuti segala arahan dalam e-mel tersebut
untuk membuktikan bahawa alamat tersebut memang milik anda.',
'throttled-mailpassword'     => 'Sebuah pengingat kata laluan telah pun
dikirim dalam $1 jam yang lalu. Untuk mengelak penyalahgunaan, hanya satu
pengingat kata laluan akan dikirim pada setiap $1 jam.',
'mailerror'                  => 'Ralat ketika mengirim e-mel: $1',
'acct_creation_throttle_hit' => 'Harap maaf, anda telah pun membuka sebanyak $1 akaun. Anda tidak boleh membuka akaun lagi.',
'emailauthenticated'         => 'Alamat e-mel anda telah disahkan pada $1.',
'emailnotauthenticated'      => 'Alamat e-mel anda belum disahkan. Oleh itu,
e-mel bagi ciri-ciri berikut tidak boleh dikirim.',
'noemailprefs'               => 'Anda perlu menetapkan alamat e-mel terlebih dahulu untuk menggunakan ciri-ciri ini.',
'emailconfirmlink'           => 'Sahkan alamat e-mel anda.',
'invalidemailaddress'        => 'Alamat e-mel tersebut tidak boleh diterima kerana ia tidak sah. Sila masukkan alamat e-mel yang betul atau kosongkan sahaja ruangan tersebut.',
'accountcreated'             => 'Akaun dibuka',
'accountcreatedtext'         => 'Akaun pengguna bagi $1 telah dibuka.',
'createaccount-title'        => 'Pembukaan akaun {{SITENAME}}',
'createaccount-text'         => 'Seseorang telah membuka akaun untuk
alamat e-mel anda di {{SITENAME}} ($4) dengan nama "$2" dan kata laluan "$3".
Anda boleh log masuk dan tukar kata laluan anda sekarang.

Sila abaikan mesej ini jika anda tidak meminta untuk membuka akaun tersebut.',
'loginlanguagelabel'         => 'Bahasa: $1',

# Password reset dialog
'resetpass'               => 'Set semula kata laluan',
'resetpass_announce'      => 'Anda sedang log masuk dengan kata laluan sementara. Untuk log masuk dengan sempurna, sila tetapkan kata laluan baru di sini:',
'resetpass_text'          => '<!-- Tambah teks di sini -->',
'resetpass_header'        => 'Set semula kata laluan',
'resetpass_submit'        => 'Tetapkan kata laluan dan log masuk',
'resetpass_success'       => 'Kata laluan anda ditukar dengan jayanya! Sila tunggu...',
'resetpass_bad_temporary' => 'Kata laluan sementara tidak sah. Anda mungkin telah pun menukar kata laluan atau meminta kata laluan sementara yang baru.',
'resetpass_forbidden'     => 'Anda tidak boleh mengubah kata laluan di {{SITENAME}}.',
'resetpass_missing'       => 'Tiada data borang.',

# Edit page toolbar
'bold_sample'     => 'Teks tebal',
'bold_tip'        => 'Teks tebal',
'italic_sample'   => 'Teks condong',
'italic_tip'      => 'Teks condong',
'link_sample'     => 'Tajuk pautan',
'link_tip'        => 'Pautan dalaman',
'extlink_sample'  => 'http://www.example.com tajuk pautan',
'extlink_tip'     => 'Pautan luar (ingat awalan http://)',
'headline_sample' => 'Teks tajuk',
'headline_tip'    => 'Tajuk peringkat 2',
'math_sample'     => 'Masukkan rumus di sini',
'math_tip'        => 'Formula matematik (LaTeX)',
'nowiki_sample'   => 'Masukkan teks tak berformat di sini',
'nowiki_tip'      => 'Abaikan pemformatan wiki',
'image_sample'    => 'Contoh.jpg',
'image_tip'       => 'Imej terbenam',
'media_sample'    => 'Contoh.ogg',
'media_tip'       => 'Pautan fail media',
'sig_tip'         => 'Tandatangan dengan cap waktu',
'hr_tip'          => 'Garis melintang (gunakan dengan hemat)',

# Edit pages
'summary'                          => 'Ringkasan',
'subject'                          => 'Tajuk',
'minoredit'                        => 'Ini adalah suntingan kecil',
'watchthis'                        => 'Pantau laman ini',
'savearticle'                      => 'Simpan',
'preview'                          => 'Pratonton',
'showpreview'                      => 'Pratonton',
'showlivepreview'                  => 'Pratonton langsung',
'showdiff'                         => 'Lihat perubahan',
'anoneditwarning'                  => "'''Amaran:''' Anda tidak log masuk. Alamat IP anda akan direkodkan dalam sejarah suntingan laman ini.",
'missingsummary'                   => "'''Peringatan:''' Anda tidak menyatakan ringkasan suntingan. Klik '''Simpan''' sekali lagi untuk menyimpan suntingan ini tanpa ringkasan.",
'missingcommenttext'               => 'Sila masukkan komen dalam ruangan di bawah.',
'missingcommentheader'             => "'''Peringatan:''' Anda tidak menyatakan tajuk bagi komen ini. Klik '''Simpan''' sekali lagi untuk menyimpan suntingan ini tanpa tajuk.",
'summary-preview'                  => 'Pratonton ringkasan',
'subject-preview'                  => 'Pratonton tajuk',
'blockedtitle'                     => 'Pengguna disekat',
'blockedtext'                      => '<big>\'\'\'Nama pengguna atau alamat IP anda telah disekat.\'\'\'</big>

Sekatan ini dilakukan oleh $1 dengan sebab \'\'$2\'\'.

* Mula: $8
* Tamat: $6
* Pengguna sasaran: $7

Sila hubungi $1 atau [[{{MediaWiki:Grouppage-sysop}}|pentadbir]] yang lain untuk untuk berunding mengenai sekatan ini.

Anda tidak boleh menggunakan ciri "e-melkan pengguna ini" kecuali sekiranya anda telah menetapkan alamat e-mel yang sah dalam [[Special:Preferences|keutamaan]] anda dan anda tidak disekat daripada menggunakannya.

Alamat IP semasa anda ialah $3, dan ID sekatan ialah #$5. Sila sertakan maklumat-maklumat ini dalam pertanyaan nanti.',
'autoblockedtext'                  => 'Alamat IP anda telah disekat secara automatik kerana ia digunakan oleh pengguna lain yang disekat oleh $1.
Berikut ialah sebab yang dinyatakan:

:\'\'$2\'\'

* Mula: $8
* Tamat: $6
* Pengguna sasaran: $7

Anda boleh menghubungi $1 atau [[{{MediaWiki:Grouppage-sysop}}|pentadbir]] lain untuk berunding mengenai sekatan ini.

Sila ambil perhatian bahawa anda tidak boleh menggunakan ciri "e-melkan pengguna ini" kecuali sekiranya anda telah menetapkan alamat e-mel yang sah dalam [[Special:Preferences|laman keutamaan]] anda dan anda tidak disekat daripada menggunakannya.

Alamat IP semasa anda ialah $3, dan ID sekatan ialah #$5. Sila sertakan maklumat-maklumat ini dalam pertanyaan nanti.',
'blockednoreason'                  => 'tiada sebab diberikan',
'blockedoriginalsource'            => "Sumber bagi '''$1'''
ditunjukkan di bawah:",
'blockededitsource'                => "Teks bagi '''suntingan anda''' terhadap '''$1''' ditunjukkan di bawah:",
'whitelistedittitle'               => 'Log masuk dahulu untuk menyunting',
'whitelistedittext'                => 'Anda hendaklah $1 terlebih dahulu untuk menyunting laman.',
'confirmedittitle'                 => 'Pengesahan e-mel diperlukan untuk menyunting',
'confirmedittext'                  => 'Anda perlu mengesahkan alamat e-mel anda terlebih dahulu untuk menyunting mana-mana laman. Sila tetapkan dan sahkan alamat e-mel anda melalui [[Special:Preferences|laman keutamaan]].',
'nosuchsectiontitle'               => 'Bahagian tidak wujud',
'nosuchsectiontext'                => 'Anda telah mencuba untuk menyunting bahagian "$1" yang tidak wujud. Oleh itu, suntingan anda tidak boleh disimpan.',
'loginreqtitle'                    => 'Log masuk diperlukan',
'loginreqlink'                     => 'log masuk',
'loginreqpagetext'                 => 'Anda harus $1 untuk dapat melihat laman yang lain.',
'accmailtitle'                     => 'Kata laluan dikirim.',
'accmailtext'                      => 'Kata laluan bagi "$1" telah dikirim kepada $2.',
'newarticle'                       => '(Baru)',
'newarticletext'                   => "Anda telah mengikuti pautan ke laman yang belum wujud.
Untuk mencipta laman ini, sila taip dalam kotak di bawah
(lihat [[{{MediaWiki:Helppage}}|laman bantuan]] untuk maklumat lanjut).
Jika anda tiba di sini secara tak sengaja, hanya klik butang '''back''' pada pelayar anda.",
'anontalkpagetext'                 => "----''Ini ialah laman perbincangan bagi pengguna tanpa nama yang belum membuka akaun atau tidak log masuk. Kami terpaksa menggunakan alamat IP untuk mengenal pasti pengguna tersebut. Alamat IP ini boleh dikongsi oleh ramai pengguna. Sekiranya anda adalah seorang pengguna tanpa nama dan berasa bahawa komen yang tidak kena mengena telah ditujui kepada anda, sila [[Special:UserLogin|buka akaun baru atau log masuk]] untuk mengelakkan sebarang kekeliruan dengan pengguna tanpa nama yang lain.''",
'noarticletext'                    => 'Tiada teks dalam laman ini pada masa sekarang. Anda boleh [[Special:Search/{{PAGENAME}}|mencari tajuk bagi laman ini]] dalam laman-laman lain atau [{{fullurl:{{FULLPAGENAME}}|action=edit}} menyunting laman ini].',
'userpage-userdoesnotexist'        => 'Akaun pengguna "$1" tidak berdaftar. Sila pastikan sama ada anda mahu mencipta/menyunting laman ini.',
'clearyourcache'                   => "'''Catatan: Selepas menyimpan laman ini, anda mungkin perlu mengosongkan fail simpanan (''cache'') pelayar anda terlebih dahulu untuk mengenakan perubahan.'''
'''Mozilla/Firefox/Safari:''' tahan ''Shift'' ketika mengklik ''Reload'' atau tekan ''Ctrl+F5'' atau tekan ''Ctrl+R'' (''Command+R'' dalam komputer Macintosh).
'''Konqueror:''' klik butang ''Reload'' atau tekan ''F5''.
'''Opera:''' kosongkan fail simpanan melalui menu ''Tools → Preferences''.
'''Internet Explorer:''' tahan ''Ctrl'' ketika mengklik ''Refresh'' atau tekan ''Ctrl+F5''.",
'usercssjsyoucanpreview'           => "<strong>Petua:</strong> Gunakan butang 'Pratonton' untuk menguji CSS/JS baru anda sebelum menyimpan.",
'usercsspreview'                   => "'''Ingat bahawa anda hanya sedang melihat pratonton CSS peribadi anda. Laman ini belum lagi disimpan!'''",
'userjspreview'                    => "'''Ingat bahawa anda hanya menguji/melihat pratonton JavaScript anda, ia belum lagi disimpan!'''",
'userinvalidcssjstitle'            => "'''Amaran:''' Rupa \"\$1\" tidak wujud. Ingat bahawa laman tempahan .css dan .js menggunakan tajuk berhuruf kecil, contohnya {{ns:user}}:Anu/monobook.css tidak sama dengan {{ns:user}}:Anu/Monobook.css.",
'updated'                          => '(Dikemaskinikan)',
'note'                             => '<strong>Catatan:</strong>',
'previewnote'                      => '<strong>Ini hanyalah pratonton. Perubahan masih belum disimpan!</strong>',
'previewconflict'                  => 'Paparan ini merupakan teks di bahagian atas dalam kotak sunting teks. Teks ini akan disimpan sekiranya anda memilih berbuat demikian.',
'session_fail_preview'             => '<strong>Kami tidak dapat memproses suntingan anda kerana kehilangan data sesi. Sila cuba lagi. Jika masalah ini berlanjutan, [[Special:UserLogout|log keluar]] dahulu, kemudian log masuk sekali lagi.</strong>',
'session_fail_preview_html'        => "<strong>Kami tidak dapat memproses suntingan anda kerana kehilangan data sesi.</strong>

''Kerana {{SITENAME}} membenarkan HTML mentah, pratonton dimatikan sebagai perlindungan daripada serangan JavaScript.''

<strong>Jika ini adalah penyuntingan yang sah, sila cuba lagi. Jika masalah ini berlanjutan, [[Special:UserLogout|log keluar]] dahulu, kemudian log masuk sekali lagi.</strong>",
'token_suffix_mismatch'            => '<strong>Suntingan anda telah ditolak kerana pelanggan anda memusnahkan aksara tanda baca
dalam token suntingan. Suntingan tersebut telah ditolak untuk menghalang kerosakan teks laman.
Hal ini kadangkala berlaku apabila anda menggunakan khidmat proksi tanpa nama berdasarkan web yang bermasalah.</strong>',
'editing'                          => 'Menyunting $1',
'editingsection'                   => 'Menyunting $1 (bahagian)',
'editingcomment'                   => 'Menyunting $1 (komen)',
'editconflict'                     => 'Percanggahan penyuntingan: $1',
'explainconflict'                  => 'Pengguna lain telah menyunting laman ini ketika anda sedang menyuntingnya.
Kawasan teks di atas mengandungi teks semasa.
Perubahan anda dipaparkan dalam kawasan teks di bawah.
Anda perlu menggabungkan perubahan anda dengan teks semasa.
<b>Hanya</b> teks dalam kawasan teks di atas akan disimpan jika anda menekan
"Simpan laman".<br />',
'yourtext'                         => 'Teks anda',
'storedversion'                    => 'Versi yang disimpan',
'nonunicodebrowser'                => '<strong>AMARAN: Pelayar anda tidak mematuhi Unicode. Aksara-aksara bukan ASCII akan dipaparkan dalam kotak sunting sebagai kod perenambelasan.</strong>',
'editingold'                       => '<strong>AMARAN: Anda sedang
menyunting sebuah semakan yang sudah ketinggalan zaman.
Jika anda menyimpannya, sebarang perubahan yang dibuat selepas tarikh semakan ini akan hilang.</strong>',
'yourdiff'                         => 'Perbezaan',
'copyrightwarning'                 => 'Sila ambil perhatian bahawa semua sumbangan kepada {{SITENAME}} akan dikeluarkan di bawah $2 (lihat $1 untuk butiran lanjut). Jika anda tidak mahu tulisan anda disunting sewenang-wenangnya oleh orang lain dan diedarkan secara bebas, maka jangan kirim di sini.<br />
Anda juga berjanji bahawa ini adalah hasil kerja tangan anda sendiri, atau disalin daripada domain awam atau mana-mana sumber bebas lain.
<strong>JANGAN KIRIM KARYA HAK CIPTA ORANG LAIN TANPA KEBENARAN!</strong>',
'copyrightwarning2'                => 'Sila ambil perhatian bahawa semua sumbangan terhadap {{SITENAME}} boleh disunting, diubah, atau dipadam oleh penyumbang lain. Jika anda tidak mahu tulisan anda disunting sewenang-wenangnya, maka jangan kirim di sini.<br />
Anda juga berjanji bahawa ini adalah hasil kerja tangan anda sendiri, atau
disalin daripada domain awam atau mana-mana sumber bebas lain (lihat $1 untuk butiran lanjut).
<strong>JANGAN KIRIM KARYA HAK CIPTA ORANG LAIN TANPA KEBENARAN!</strong>',
'longpagewarning'                  => '<strong>AMARAN: Panjang laman ini ialah $1 kilobait.
Terdapat beberapa pelayar web yang mempunyai masalah jika digunakan untuk menyunting laman yang menghampiri ataupun melebihi 32kB.
Sila bahagikan rencana ini, jika boleh.</strong>',
'longpageerror'                    => '<strong>RALAT: Panjang teks yang dikirim ialah $1 kilobait,
melebihi had maksimum $2 kilobait. Ia tidak boleh disimpan.</strong>',
'readonlywarning'                  => '<strong>AMARAN: Pangkalan data telah dikunci untuk penyenggaraan.
Justeru, anda tidak boleh menyimpan suntingan anda pada masa sekarang.
Anda boleh menyalin teks anda ke dalam komputer anda terlebih dahulu dan simpan teks tersebut di sini pada masa akan datang.</strong>',
'protectedpagewarning'             => '<strong>AMARAN: Laman ini telah dikunci supaya hanya penyelia boleh menyuntingnya.</strong>',
'semiprotectedpagewarning'         => "'''Catatan:''' Laman ini telah dikunci supaya hanya pengguna berdaftar sahaja yang boleh menyuntingnya.",
'cascadeprotectedwarning'          => "'''Amaran:''' Laman ini telah dikunci, oleh itu hanya penyelia boleh menyuntingnya. Ini kerana ia termasuk dalam {{PLURAL:$1|laman|laman-laman}} berikut yang dilindungi secara melata:",
'titleprotectedwarning'            => '<strong>AMARAN: Laman ini telah dikunci supaya sesetengah pengguna sahaja boleh menciptanya.</strong>',
'templatesused'                    => 'Templat yang digunakan dalam laman ini:',
'templatesusedpreview'             => 'Templat yang digunakan dalam pratonton ini:',
'templatesusedsection'             => 'Templat yang digunakan dalam bahagian ini:',
'template-protected'               => '(dilindungi)',
'template-semiprotected'           => '(dilindungi separa)',
'hiddencategories'                 => 'Laman ini terdapat dalam $1 kategori tersembunyi:',
'edittools'                        => '<!-- Teks di sini akan ditunjukkan bawah borang sunting dan muat naik. -->',
'nocreatetitle'                    => 'Penciptaan laman dihadkan',
'nocreatetext'                     => 'Penciptaan laman baru dihadkan pada {{SITENAME}}.
Anda boleh berundur dan menyunting laman yang sedia ada, atau [[Special:UserLogin|log masuk]].',
'nocreate-loggedin'                => 'Anda tidak mempunyai keizinan untuk mencipta laman baru dalam {{SITENAME}}.',
'permissionserrors'                => 'Tidak Dibenarkan',
'permissionserrorstext'            => 'Anda tidak mempunyai keizinan untuk berbuat demikian atas {{PLURAL:$1|sebab|sebab-sebab}} berikut:',
'permissionserrorstext-withaction' => 'Anda tidak mempunyai keizinan untuk $2, atas {{PLURAL:$1|sebab|sebab-sebab}} berikut:',
'recreate-deleted-warn'            => "'''Amaran: Anda sedang mencipta semula sebuah laman yang pernah dihapuskan.''',

Anda harus mempertimbangkan perlunya menyunting laman ini.
Untuk rujukan, berikut ialah log penghapusan bagi laman ini:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Amaran: Laman ini mengandungi terlalu banyak panggilan fungsi penghurai yang intensif.

Had panggilan ialah $2, sedangkan yang digunakan berjumlah $1.',
'expensive-parserfunction-category'       => 'Laman yang mengandungi terlalu banyak panggilan fungsi penghurai yang intensif',
'post-expand-template-inclusion-warning'  => 'Amaran: Saiz kemasukan templat terlalu besar.
Sesetengah templat tidak akan dimasukkan.',
'post-expand-template-inclusion-category' => 'Laman-laman yang melebihi had saiz kemasukan templat',
'post-expand-template-argument-warning'   => 'Amaran: Laman ini mengandungi sekurang-kurangnya satu argumen templat yang mempunyai saiz pengembangan yang terlalu besar.
Argumen-argumen ini telah ditinggalkan.',
'post-expand-template-argument-category'  => 'Laman yang mengandungi templat dengan argumen yang tidak lengkap',

# "Undo" feature
'undo-success' => 'Suntingan ini boleh dibatalkan. Sila semak perbandingan di bawah untuk mengesahkan bahawa anda betul-betul mahu melakukan tindakan ini, kemudian simpan perubahan tersebut.',
'undo-failure' => 'Suntingan tersebut tidak boleh dibatalkan kerana terdapat suntingan pertengahan yang bercanggah.',
'undo-norev'   => 'Suntingan tersebut tidak boleh dibatalkan kerana tidak wujud atau telah dihapuskan.',
'undo-summary' => 'Membatalkan semakan $1 oleh [[Special:Contributions/$2|$2]] ([[User talk:$2|Perbincangan]])',

# Account creation failure
'cantcreateaccounttitle' => 'Akaun tidak dapat dibuka',
'cantcreateaccount-text' => "Pembukaan akaun daripada alamat IP ini (<b>$1</b>) telah disekat oleh [[User:$3|$3]].

Sebab yang diberikan oleh $3 ialah ''$2''",

# History pages
'viewpagelogs'        => 'Lihat log bagi laman ini',
'nohistory'           => 'Tiada sejarah suntingan bagi laman ini.',
'revnotfound'         => 'Semakan tidak dijumpai.',
'revnotfoundtext'     => 'Semakan lama untuk laman yang anda minta tidak dapat dijumpai. Sila semak URL yang anda gunakan untuk mencapai laman ini.',
'currentrev'          => 'Semakan semasa',
'revisionasof'        => 'Semakan pada $1',
'revision-info'       => 'Semakan pada $1 oleh $2',
'previousrevision'    => '←Semakan sebelumnya',
'nextrevision'        => 'Semakan berikutnya→',
'currentrevisionlink' => 'Semakan semasa',
'cur'                 => 'kini',
'next'                => 'berikutnya',
'last'                => 'akhir',
'page_first'          => 'awal',
'page_last'           => 'akhir',
'histlegend'          => "Pemilihan perbezaan: tandakan butang radio bagi versi-versi yang ingin dibandingkan dan tekan butang ''enter'' atau butang di bawah.<br />
Petunjuk: (kini) = perbezaan dengan versi terkini,
(akhir) = perbezaan dengan versi sebelumnya, K = suntingan kecil.",
'deletedrev'          => '[dihapuskan]',
'histfirst'           => 'Terawal',
'histlast'            => 'Terkini',
'historysize'         => '($1 bait)',
'historyempty'        => '(kosong)',

# Revision feed
'history-feed-title'          => 'Sejarah semakan',
'history-feed-description'    => 'Sejarah semakan bagi laman ini',
'history-feed-item-nocomment' => '$1 pada $2', # user at time
'history-feed-empty'          => 'Laman yang diminta tidak wujud.
Mungkin ia telah dihapuskan atau namanya telah ditukar.
Cuba [[Special:Search|cari]] laman lain yang mungkin berkaitan.',

# Revision deletion
'rev-deleted-comment'         => '(komen dibuang)',
'rev-deleted-user'            => '(nama pengguna dibuang)',
'rev-deleted-event'           => '(entri dibuang)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Semakan ini telah dibuang daripada arkib awam.
Butiran lanjut boleh didapati dalam [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} log penghapusan].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Semakan ini telah dibuang daripada arkib awam.
Sebagai seorang pentadbir di {{SITENAME}}, anda boleh melihatnya.
Butiran lanjut boleh didapati dalam [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} log penghapusan].
</div>',
'rev-delundel'                => 'tunjuk/sembunyi',
'revisiondelete'              => 'Hapus/nyahhapus semakan',
'revdelete-nooldid-title'     => 'Tiada semakan sasaran',
'revdelete-nooldid-text'      => 'Anda tidak menyatakan semakan sasaran.',
'revdelete-selected'          => "{{PLURAL:$2|Versi|Versi-versi}} '''$1''' yang dipilih:",
'logdelete-selected'          => '{{PLURAL:$1|Peristiwa|Peristiwa-peristiwa}} log yang dipilih:',
'revdelete-text'              => 'Semakan dan peristiwa yang dihapuskan masih muncul dalam sejarah laman dan log,
akan tetapi kandungannya tidak boleh dilihat oleh orang awam.

Pentadbir {{SITENAME}} boleh melihat kandungan tersebut dan menyahhapuskannya
semula melalui laman ini melainkan mempunyai batasan.',
'revdelete-legend'            => 'Tetapkan batasan:',
'revdelete-hide-text'         => 'Sembunyikan teks semakan',
'revdelete-hide-name'         => 'Sembunyikan tindakan dan sasaran',
'revdelete-hide-comment'      => 'Sembunyikan komen suntingan',
'revdelete-hide-user'         => 'Sembunyikan nama pengguna/IP penyunting',
'revdelete-hide-restricted'   => 'Kenakan batasan ini ke atas semua pengguna, termasuk penyelia',
'revdelete-suppress'          => 'Sekat data daripada semua pengguna, termasuk penyelia',
'revdelete-hide-image'        => 'Sembunyikan kandungan fail',
'revdelete-unsuppress'        => 'Buang batasan pada semakan yang dipulihkan',
'revdelete-log'               => 'Komen log:',
'revdelete-submit'            => 'Kenakan ke atas versi yang dipilih',
'revdelete-logentry'          => 'menukar kebolehnampakan semakan [[$1]]',
'logdelete-logentry'          => 'menukar kebolehnampakan peristiwa bagi [[$1]]',
'revdelete-success'           => 'Kebolehnampakan semakan ditetapkan.',
'logdelete-success'           => 'Kebolehnampakan peristiwa ditetapkan.',
'revdel-restore'              => 'Tukar kebolehnampakan',
'pagehist'                    => 'Sejarah laman',
'deletedhist'                 => 'Sejarah yang dihapuskan',
'revdelete-content'           => 'kandungan',
'revdelete-summary'           => 'ringkasan',
'revdelete-uname'             => 'nama pengguna',
'revdelete-restricted'        => 'mengenakan sekatan pada penyelia',
'revdelete-unrestricted'      => 'menarik sekatan daripada penyelia',
'revdelete-hid'               => 'menyembunyikan $1',
'revdelete-unhid'             => 'memunculkan $1',
'revdelete-log-message'       => '$1 bagi {{PLURAL:$2|sebuah|$2 buah}} semakan',
'logdelete-log-message'       => '$1 bagi $2 peristiwa',

# Suppression log
'suppressionlog'     => 'Log penahanan',
'suppressionlogtext' => 'Berikut ialah senarai penghapusan dan sekatan yang membabitkan kandungan yang terselindung daripada penyelia.
Sila lihat [[Special:IPBlockList|senarai sekatan IP]] untuk senarai sekatan yang sedang dijalankan.',

# History merging
'mergehistory'                     => 'Gabungkan sejarah laman',
'mergehistory-header'              => "Anda boleh menggabungkan semua semakan dalam sejarah bagi sesebuah laman sumber ke dalam laman lain.
Sila pastikan bahawa perubahan ini akan mengekalkan kesinambungan sejarah laman.

'''Setidak-tidaknya semakan semasa bagi laman sumber akan ditinggalkan.'''",
'mergehistory-box'                 => 'Gabungkan semakan bagi dua laman:',
'mergehistory-from'                => 'Laman sumber:',
'mergehistory-into'                => 'Laman destinasi:',
'mergehistory-list'                => 'Sejarah suntingan yang boleh digabungkan',
'mergehistory-merge'               => 'Semakan-semakan bagi [[:$1]] yang berikut boleh digabungkan ke dalam [[:$2]]. Gunakan lajur butang radio sekiranya anda hanya mahu menggabungkan semakan-semakan yang dibuat pada dan sebelum waktu yang ditetapkan. Sila ambil perhatian bahawa penggunaan pautan-pautan navigasi akan mengeset semula lajur ini.',
'mergehistory-go'                  => 'Tunjukkan suntingan yang boleh digabungkan',
'mergehistory-submit'              => 'Gabungkan semakan',
'mergehistory-empty'               => 'Tiada semakan yang boleh digabungkan',
'mergehistory-success'             => '$3 semakan bagi [[:$1]] telah digabungkan ke dalam [[:$2]].',
'mergehistory-fail'                => 'Gagal melaksanakan penggabungan sejarah, sila semak semula laman tersebut dan parameter waktu.',
'mergehistory-no-source'           => 'Laman sumber $1 tidak wujud.',
'mergehistory-no-destination'      => 'Laman destinasi $1 tidak wujud.',
'mergehistory-invalid-source'      => 'Laman sumber mestilah merupakan tajuk yang sah.',
'mergehistory-invalid-destination' => 'Laman destinasi mestilah merupakan tajuk yang sah.',
'mergehistory-autocomment'         => 'Menggabungkan [[:$1]] dengan [[:$2]]',
'mergehistory-comment'             => 'Menggabungkan [[:$1]] dengan [[:$2]]: $3',

# Merge log
'mergelog'           => 'Log penggabungan',
'pagemerge-logentry' => 'menggabungkan [[$1]] ke dalam [[$2]] (semakan sehingga $3)',
'revertmerge'        => 'Batalkan',
'mergelogpagetext'   => 'Berikut ialah senarai terkini bagi penggabungan sejarah sesebuah laman ke dalam lamana yang lain.',

# Diffs
'history-title'           => 'Sejarah semakan bagi "$1"',
'difference'              => '(Perbezaan antara semakan)',
'lineno'                  => 'Baris $1:',
'compareselectedversions' => 'Bandingkan versi-versi yang dipilih',
'editundo'                => 'batal',
'diff-multi'              => '({{PLURAL:$1|Satu|$1}} semakan pertengahan tidak ditunjukkan.)',

# Search results
'searchresults'             => 'Keputusan carian',
'searchresulttext'          => 'Untuk maklumat lanjut tentang carian dalam {{SITENAME}}, sila lihat [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Anda mencari "[[$1]]"',
'searchsubtitleinvalid'     => 'Untuk pertanyaan "$1"',
'noexactmatch'              => "'''Tiada laman bertajuk \"\$1\".''' Anda boleh [[:\$1|menciptanya]].",
'noexactmatch-nocreate'     => "'''Tiada laman bertajuk \"\$1\".'''",
'toomanymatches'            => 'Terlalu banyak padanan dipulangkan, sila cuba pertanyaan lain',
'titlematches'              => 'Padanan tajuk laman',
'notitlematches'            => 'Tiada tajuk laman yang sepadan',
'textmatches'               => 'Padanan teks laman',
'notextmatches'             => 'Tiada teks laman yang sepadan',
'prevn'                     => '$1 sebelumnya',
'nextn'                     => '$1 berikutnya',
'viewprevnext'              => 'Lihat ($1) ($2) ($3)',
'search-result-size'        => '$1 ($2 patah perkataan)',
'search-result-score'       => 'Kaitan: $1%',
'search-redirect'           => '(pelencongan $1)',
'search-section'            => '(bahagian $1)',
'search-suggest'            => 'Maksud anda, $1?',
'search-interwiki-caption'  => 'Projek-projek lain',
'search-interwiki-default'  => 'Keputusan daripada $1:',
'search-interwiki-more'     => '(lagi)',
'search-mwsuggest-enabled'  => 'berserta cadangan',
'search-mwsuggest-disabled' => 'tiada cadangan',
'search-relatedarticle'     => 'Berkaitan',
'mwsuggest-disable'         => 'Matikan ciri cadangan AJAX',
'searchrelated'             => 'berkaitan',
'searchall'                 => 'semua',
'showingresults'            => "Berikut ialah '''$1''' hasil bermula daripada yang {{PLURAL:$2|pertama|ke-'''$2'''}}.",
'showingresultsnum'         => "Berikut ialah '''$3''' hasil bermula daripada yang {{PLURAL:$2|pertama|ke-'''$2'''}}.",
'showingresultstotal'       => "Berikut ialah {{PLURAL:$3|hasil '''$1'''|hasil '''$1 - $2'''}} daripada '''$3'''",
'nonefound'                 => "'''Catatan''': Kegagalan pencarian biasanya
disebabkan oleh pencarian perkataan-perkataan yang terlalu umum, seperti \"ada\"
dan \"dari\" yang tidak diindekskan, atau disebabkan oleh pencarian lebih
daripada satu kata kunci (hanya laman yang mengandungi kesemua kata kunci akan ditunjukkan).",
'powersearch'               => 'Cari',
'powersearch-legend'        => 'Gelintar maju',
'powersearch-ns'            => 'Gelintar ruang nama:',
'powersearch-redir'         => 'Termasuk lencongan',
'powersearch-field'         => 'Cari',
'search-external'           => 'Carian luar',
'searchdisabled'            => 'Ciri pencarian dalam {{SITENAME}} dimatikan. Anda boleh mencari melalui Google. Sila ambil perhatian bahawa indeks dalam Google mungkin bukan yang terkini.',

# Preferences page
'preferences'              => 'Keutamaan',
'mypreferences'            => 'Keutamaan saya',
'prefs-edits'              => 'Jumlah suntingan:',
'prefsnologin'             => 'Belum log masuk',
'prefsnologintext'         => 'Anda hendaklah <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} log masuk]</span> terlebih dahulu untuk menetapkan keutamaan.',
'prefsreset'               => 'Keutamaan anda telah diset semula dari storan.',
'qbsettings'               => 'Bar pantas',
'qbsettings-none'          => 'Tiada',
'qbsettings-fixedleft'     => 'Tetap sebelah kiri',
'qbsettings-fixedright'    => 'Tetap sebelah kanan',
'qbsettings-floatingleft'  => 'Berubah-ubah sebelah kiri',
'qbsettings-floatingright' => 'Berubah-ubah sebelah kanan',
'changepassword'           => 'Tukar kata laluan',
'skin'                     => 'Rupa',
'math'                     => 'Matematik',
'dateformat'               => 'Format tarikh',
'datedefault'              => 'Tiada keutamaan',
'datetime'                 => 'Tarikh dan waktu',
'math_failure'             => 'Gagal menghurai',
'math_unknown_error'       => 'ralat yang tidak dikenali',
'math_unknown_function'    => 'fungsi yang tidak dikenali',
'math_lexing_error'        => "ralat ''lexing''",
'math_syntax_error'        => 'ralat sintaks',
'math_image_error'         => 'penukaran PNG gagal; sila pastikan bahawa latex, dvips, gs dan convert dipasang dengan betul',
'math_bad_tmpdir'          => 'Direktori temp matematik tidak boleh ditulis atau dicipta',
'math_bad_output'          => 'Direktori output matematik tidak boleh ditulis atau dicipta',
'math_notexvc'             => 'Atur cara texvc hilang; sila lihat fail math/README untuk maklumat konfigurasi.',
'prefs-personal'           => 'Profil',
'prefs-rc'                 => 'Perubahan terkini',
'prefs-watchlist'          => 'Senarai pantau',
'prefs-watchlist-days'     => 'Had bilangan hari dalam senarai pantau:',
'prefs-watchlist-edits'    => 'Had maksimum perubahan untuk ditunjukkan dalam senarai pantau penuh:',
'prefs-misc'               => 'Pelbagai',
'saveprefs'                => 'Simpan',
'resetprefs'               => 'Set semula',
'oldpassword'              => 'Kata laluan lama:',
'newpassword'              => 'Kata laluan baru:',
'retypenew'                => 'Ulangi kata laluan baru:',
'textboxsize'              => 'Menyunting',
'rows'                     => 'Baris:',
'columns'                  => 'Lajur:',
'searchresultshead'        => 'Cari',
'resultsperpage'           => 'Jumlah padanan bagi setiap halaman:',
'contextlines'             => 'Bilangan baris untuk dipaparkan bagi setiap capaian:',
'contextchars'             => 'Bilangan askara konteks bagi setiap baris:',
'stub-threshold'           => 'Ambang bagi pemformatan <a href="#" class="stub">pautan ke rencana ringkas</a> (bait):',
'recentchangesdays'        => 'Bilangan hari dalam perubahan terkini:',
'recentchangescount'       => 'Bilangan suntingan dalam perubahan terkini:',
'savedprefs'               => 'Keutamaan anda disimpan.',
'timezonelegend'           => 'Zon waktu',
'timezonetext'             => 'Beza waktu dalam jam antara waktu tempatan anda dengan waktu UTC (8 untuk Kuala Lumpur).',
'localtime'                => 'Waktu tempatan',
'timezoneoffset'           => 'Imbangan¹',
'servertime'               => 'Waktu pelayan',
'guesstimezone'            => 'Gunakan tetapan pelayar saya',
'allowemail'               => 'Benarkan e-mel daripada pengguna lain',
'prefs-searchoptions'      => 'Pilihan gelintar',
'prefs-namespaces'         => 'Ruang nama',
'defaultns'                => 'Cari dalam ruang nama ini secara lalai:',
'default'                  => 'lalai',
'files'                    => 'Fail',

# User rights
'userrights'                  => 'Pengurusan hak pengguna', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Urus kumpulan pengguna',
'userrights-user-editname'    => 'Masukkan nama pengguna:',
'editusergroup'               => 'Sunting Kumpulan Pengguna',
'editinguser'                 => "Mengubah hak pengguna '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Ubah kumpulan pengguna',
'saveusergroups'              => 'Simpan Kumpulan Pengguna',
'userrights-groupsmember'     => 'Ahli bagi:',
'userrights-groups-help'      => 'Anda boleh mengubah keahlian kumpulan bagi pengguna ini:
* Petak yang bertanda bererti pengguna tersebut adalah ahli kumpulan itu.
* Petak yang tidak bertanda bererti bahawa pengguna tersebut bukan ahli kumpulan itu.
* Tanda bintang (*) menandakan bahawa anda tidak boleh melucutkan keahlian pengguna tersebut setelah anda melantiknya, dan begitulah sebaliknya.',
'userrights-reason'           => 'Sebab perubahan:',
'userrights-no-interwiki'     => 'Anda tidak mempunyai keizinan untuk mengubah hak-hak pengguna di wiki lain.',
'userrights-nodatabase'       => 'Pangkalan data $1 tiada atau bukan tempatan.',
'userrights-nologin'          => 'Anda mesti [[Special:UserLogin|log masuk]] dengan akaun pentadbir terlebih dahulu untuk memperuntukkan hak-hak pengguna.',
'userrights-notallowed'       => 'Anda tidak mempunyai keizinan untuk memperuntukkan hak-hak pengguna.',
'userrights-changeable-col'   => 'Kumpulan yang anda boleh ubah',
'userrights-unchangeable-col' => 'Kumpulan yang anda tak boleh ubah',

# Groups
'group'               => 'Kumpulan:',
'group-user'          => 'Pengguna',
'group-autoconfirmed' => 'Pengguna yang disahkan secara automatik',
'group-bot'           => 'Bot',
'group-sysop'         => 'Penyelia',
'group-bureaucrat'    => 'Birokrat',
'group-suppress'      => 'Pengawas',
'group-all'           => '(semua)',

'group-user-member'          => 'Pengguna',
'group-autoconfirmed-member' => 'Pengguna yang disahkan secara automatik',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Penyelia',
'group-bureaucrat-member'    => 'Birokrat',
'group-suppress-member'      => 'Pengawas',

'grouppage-user'          => '{{ns:project}}:Pengguna',
'grouppage-autoconfirmed' => '{{ns:project}}:Pengguna yang disahkan secara automatik',
'grouppage-bot'           => '{{ns:project}}:Bot',
'grouppage-sysop'         => '{{ns:project}}:Pentadbir',
'grouppage-bureaucrat'    => '{{ns:project}}:Birokrat',
'grouppage-suppress'      => '{{ns:project}}:Pengawas',

# Rights
'right-read'                 => 'Membaca laman',
'right-edit'                 => 'Menyunting laman',
'right-createpage'           => 'Mencipta laman (selain laman perbincangan)',
'right-createtalk'           => 'Mencipta laman perbincangan',
'right-createaccount'        => 'Membuka akaun pengguna baru',
'right-minoredit'            => 'Menanda suntingan kecil',
'right-move'                 => 'Memindah laman',
'right-move-subpages'        => 'Memindahkan laman berserta sublaman',
'right-suppressredirect'     => 'Memindahkan sesebuah laman tanpa mencipta lencongan',
'right-upload'               => 'Muat naik fail',
'right-reupload'             => 'Menulis ganti fail sedia ada',
'right-reupload-own'         => 'Menulis ganti fail sedia ada yang dimuat naik sendiri',
'right-reupload-shared'      => 'Mengatasi fail di gedung media kongsi',
'right-upload_by_url'        => 'Memuat naik fail daripada alamat URL',
'right-purge'                => 'Membersihkan fail simpanan sementara bagi sesebuah laman tanpa pengesahan',
'right-autoconfirmed'        => 'Menyunting laman yang dilindungi separa',
'right-bot'                  => 'Dianggap melakukan tugas-tugas automatik',
'right-nominornewtalk'       => 'Suntingan kecil pada laman perbincangan seseorang pengguna tidak menghidupkan isyarat pesanan baru untuk pengguna itu',
'right-apihighlimits'        => 'Meninggikan had dalam pertanyaan API',
'right-writeapi'             => 'Menggunakan API tulis',
'right-delete'               => 'Menghapuskan laman',
'right-bigdelete'            => 'Menghapuskan laman bersejarah',
'right-deleterevision'       => 'Menghapuskan dan memulihkan semula mana-mana semakan bagi sesebuah laman',
'right-deletedhistory'       => 'Melihat senarai entri sejarah yang telah dihapuskan, tetapi tanpa teks yang berkaitan',
'right-browsearchive'        => 'Menggelintar laman-laman yang telah dihapuskan',
'right-undelete'             => 'Mengembalikan laman yang telah dihapuskan (nyahhapus)',
'right-suppressrevision'     => 'Memeriksa dan memulihkan semakan yang terselindung daripada penyelia',
'right-suppressionlog'       => 'Melihat log rahsia',
'right-block'                => 'Menyekat pengguna lain daripada menyunting',
'right-blockemail'           => 'Menyekat pengguna lain daripada mengirim e-mel',
'right-hideuser'             => 'Menyekat sesebuah nama pengguna, menyembunyikannya daripada orang ramai',
'right-ipblock-exempt'       => 'Melangkau sekatan IP, sekatan automatik dan sekatan julat',
'right-proxyunbannable'      => 'Melangkau sekatan proksi automatik',
'right-protect'              => 'Menukar peringkat perlindungan dan menyunting laman yang dilindungi',
'right-editprotected'        => 'Menyunting laman yang dilindungi (tanpa perlindungan melata)',
'right-editinterface'        => 'Menyunting antara muka pengguna',
'right-editusercssjs'        => 'Menyunting fail CSS dan JavaScript pengguna lain',
'right-rollback'             => 'Mengundurkan suntigan terakhir bagi laman tertentu',
'right-markbotedits'         => 'Menanda suntingan yang diundurkan sebagai suntingan bot',
'right-noratelimit'          => 'Tidak dikenakan had kadar penyuntingan',
'right-import'               => 'Mengimport laman dari wiki lain',
'right-importupload'         => 'Mengimport laman dengan memuat naik fail',
'right-patrol'               => 'Memeriksa suntingan orang lain',
'right-autopatrol'           => 'Suntingannya ditandakan sebagai telah diperiksa secara automatik',
'right-patrolmarks'          => 'Melihat tanda pemeriksaan dalam senarai perubahan terkini',
'right-unwatchedpages'       => 'Melihat senarai laman yang tidak dipantau',
'right-trackback'            => 'Mengirim jejak balik',
'right-mergehistory'         => 'Menggabungkan sejarah laman',
'right-userrights'           => 'Menyerahkan dan menarik balik sebarang hak pengguna',
'right-userrights-interwiki' => 'Menyerahkan dan menarik balik hak pengguna di wiki lain',
'right-siteadmin'            => 'Mengunci dan membuka kunci pangkalan data',

# User rights log
'rightslog'      => 'Log hak pengguna',
'rightslogtext'  => 'Ini ialah log bagi perubahan hak pengguna.',
'rightslogentry' => 'menukar keahlian kumpulan bagi $1 daripada $2 kepada $3',
'rightsnone'     => '(tiada)',

# Recent changes
'nchanges'                          => '$1 perubahan',
'recentchanges'                     => 'Perubahan terkini',
'recentchangestext'                 => 'Jejaki perubahan terkini dalam {{SITENAME}} pada laman ini.',
'recentchanges-feed-description'    => 'Jejaki perubahan terkini dalam {{SITENAME}} pada suapan ini.',
'rcnote'                            => "Berikut ialah '''$1''' perubahan terakhir sejak '''$2''' hari yang lalu sehingga $5, $4.",
'rcnotefrom'                        => 'Berikut ialah semua perubahan sejak <b>$2</b> (sehingga <b>$1</b>).',
'rclistfrom'                        => 'Papar perubahan sejak $1',
'rcshowhideminor'                   => '$1 suntingan kecil',
'rcshowhidebots'                    => '$1 bot',
'rcshowhideliu'                     => '$1 pengguna log masuk',
'rcshowhideanons'                   => '$1 pengguna tanpa nama',
'rcshowhidepatr'                    => '$1 suntingan dirondai',
'rcshowhidemine'                    => '$1 suntingan saya',
'rclinks'                           => 'Paparkan $1 perubahan terakhir sejak $2 hari yang lalu<br />$3',
'diff'                              => 'beza',
'hist'                              => 'sej',
'hide'                              => 'Sembunyi',
'show'                              => 'Papar',
'minoreditletter'                   => 'k',
'newpageletter'                     => 'B',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 pemantau]',
'rc_categories'                     => 'Hadkan kepada kategori (asingkan dengan "|")',
'rc_categories_any'                 => 'Semua',
'newsectionsummary'                 => '/* $1 */ bahagian baru',

# Recent changes linked
'recentchangeslinked'          => 'Perubahan berkaitan',
'recentchangeslinked-title'    => 'Perubahan berkaitan dengan $1',
'recentchangeslinked-noresult' => 'Tiada perubahan pada semua laman yang dipaut dalam tempoh yang diberikan.',
'recentchangeslinked-summary'  => "Laman khas ini menyenaraikan perubahan terkini bagi laman-laman yang dipaut. Laman-laman yang terdapat dalam senarai pantau anda ditandakan dengan '''teks tebal'''.",
'recentchangeslinked-page'     => 'Nama laman:',
'recentchangeslinked-to'       => 'Paparkan perubahan pada laman yang mengandungi pautan ke laman yang diberikan',

# Upload
'upload'                      => 'Muat naik fail',
'uploadbtn'                   => 'Muat naik',
'reupload'                    => 'Muat naik sekali lagi',
'reuploaddesc'                => 'Kembali ke borang muat naik',
'uploadnologin'               => 'Belum log masuk',
'uploadnologintext'           => 'Anda perlu [[Special:UserLogin|log masuk]]
terlebih dahulu untuk memuat naik fail.',
'upload_directory_missing'    => 'Direktori muat naik ($1) hilang dan tidak dapat dicipta oleh pelayan web.',
'upload_directory_read_only'  => 'Direktori muat naik ($1) tidak boleh ditulis oleh pelayan web.',
'uploaderror'                 => 'Ralat muat naik',
'uploadtext'                  => "Gunakan borang di bawah untuk memuat naik fail. Untuk melihat atau mencari imej yang sudah dimuat naik, sila ke [[Special:ImageList|senarai fail yang dimuat naik]]. Muat naik dan penghapusan akan direkodkan dalam [[Special:Log/upload|log muat naik]].

Untuk menyertakan imej tersebut dalam sesebuah laman, sila masukkan teks
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fail.jpg]]</nowiki>''' atau
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fail.png|teks alternatif]]</nowiki>'''. Anda juga boleh menggunakan
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Fail.ogg]]</nowiki>''' untuk memaut secara terus.",
'upload-permitted'            => 'Jenis fail yang dibenarkan: $1.',
'upload-preferred'            => 'Jenis fail yang diutamakan: $1.',
'upload-prohibited'           => 'Jenis fail yang dilarang: $1.',
'uploadlog'                   => 'log muat naik',
'uploadlogpage'               => 'Log muat naik',
'uploadlogpagetext'           => 'Berikut ialah senarai terkini bagi fail yang dimuat naik.',
'filename'                    => 'Nama fail',
'filedesc'                    => 'Ringkasan',
'fileuploadsummary'           => 'Ringkasan:',
'filestatus'                  => 'Status hak cipta:',
'filesource'                  => 'Sumber:',
'uploadedfiles'               => 'Fail yang telah dimuat naik',
'ignorewarning'               => 'Abaikan amaran dan simpan juga fail ini.',
'ignorewarnings'              => 'Abaikan mana-mana amaran.',
'minlength1'                  => 'Panjang nama fail mestilah sekurang-kurangnya satu huruf.',
'illegalfilename'             => 'Nama fail "$1" mengandungi aksara yang tidak dibenarkan dalam tajuk laman. Sila tukar nama fail ini dan muat naik sekali lagi.',
'badfilename'                 => 'Nama fail telah ditukar kepada "$1".',
'filetype-badmime'            => 'Memuat naik fail jenis MIME "$1" adalah tidak dibenarkan.',
'filetype-unwanted-type'      => "'''\".\$1\"''' adalah jenis fail yang tidak dikehendaki. {{PLURAL:\$3|Jenis|Jenis-jenis}} fail yang diutamakan ialah \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' adalah jenis fail yang dilarang. {{PLURAL:\$3|Jenis|Jenis-jenis}} fail yang dibenarkan ialah \$2.",
'filetype-missing'            => 'Fail ini tidak mempunyai sambungan (contohnya ".jpg").',
'large-file'                  => 'Saiz fail ini ialah $2. Anda dinasihati supaya memuat naik fail yang tidak melebihi $1.',
'largefileserver'             => 'Fail ini telah melebihi had muat naik pelayan web.',
'emptyfile'                   => 'Fail yang dimuat naik adalah kosong. Ini mungkin disebabkan oleh kesilapan menaip nama fail. Sila pastikan bahawa anda betul-betul mahu memuat naik fail ini.',
'fileexists'                  => 'Sebuah fail dengan nama ini telah pun wujud. Sila semak <strong><tt>$1</tt></strong> sekiranya anda tidak pasti bahawa anda mahu menukarnya atau tidak.',
'filepageexists'              => 'Sebuah lama (bukan imej) dengan nama ini telah pun wujud. Sila semak <strong><tt>$1</tt></strong> sekiranya anda tidak pasti bahawa anda mahu menukarnya atau tidak.',
'fileexists-extension'        => 'Sebuah fail dengan nama yang sama telah pun wujud:<br />
Nama fail yang dimuat naik: <strong><tt>$1</tt></strong><br />
Nama fail yang sedia ada: <strong><tt>$2</tt></strong><br />
Sila pilih nama lain.',
'fileexists-thumb'            => "<center>'''Imej sedia ada'''</center>",
'fileexists-thumbnail-yes'    => 'Fail ini kelihatan seperti sebuah imej yang telah dikecilkan <i>(imej ringkas)</i>. Sila semak fail <strong><tt>$1</tt></strong>.<br />
Jika fail yang disemak itu adalah sama dengan yang saiz asal, maka anda tidak perlu memuat naik imej ringkas tambahan.',
'file-thumbnail-no'           => 'Nama fail ini bermula dengan <strong><tt>$1</tt></strong>. Barangkali ia adalah sebuah imej yang telah dikecilkan <i>(imej ringkas)</i>.
Jika anda memiliki imej ini dalam leraian penuh, sila muat naik fail tersebut. Sebaliknya, sila tukar nama fail ini.',
'fileexists-forbidden'        => 'Sebuah fail dengan nama ini telah pun wujud. Sila berundur dan muat naik fail ini dengan nama lain. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Sebuah fail dengan nama ini telah pun wujud dalam gedung fail kongsi. Jika anda masih mahu memuat naik fail ini, sila kembali ke borang muat naik dan gunakan nama lain. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Fail ini adalah salinan bagi {{PLURAL:$1|fail|fail-fail}} berikut:',
'successfulupload'            => 'Muat naik berjaya',
'uploadwarning'               => 'Amaran muat naik',
'savefile'                    => 'Simpan fail',
'uploadedimage'               => 'memuat naik "[[$1]]"',
'overwroteimage'              => 'memuat naik versi baru bagi "[[$1]]"',
'uploaddisabled'              => 'Ciri muat naik dimatikan',
'uploaddisabledtext'          => 'Ciri muat naik fail dimatikan di {{SITENAME}}.',
'uploadscripted'              => 'Fail ini mengandungi kod HTML atau skrip yang boleh disalahtafsirkan oleh pelayar web.',
'uploadcorrupt'               => 'Fail tersebut rosak atau mempunyai sambungan yang salah. Sila periksa fail tersebut dan cuba lagi.',
'uploadvirus'                 => 'Fail tersebut mengandungi virus! Butiran: $1',
'sourcefilename'              => 'Nama fail sumber:',
'destfilename'                => 'Nama fail destinasi:',
'upload-maxfilesize'          => 'Had saiz fail: $1',
'watchthisupload'             => 'Pantau laman ini',
'filewasdeleted'              => 'Sebuah fail dengan nama ini pernah dimuat naik, tetapi kemudiannya dihapuskan. Anda seharusnya menyemak $1 sebelum meneruskan percubaan untuk memuat naik fail ini.',
'upload-wasdeleted'           => "'''Amaran: Anda sedang memuat naik sebuah fail yang pernah dihapuskan.'''

Anda harus mempertimbangkan perlunya memuat naik fail ini.
Untuk rujukan, berikut ialah log penghapusan bagi fail ini:",
'filename-bad-prefix'         => 'Nama bagi fail yang dimuat naik bermula dengan <strong>"$1"</strong>, yang mana merupakan nama yang tidak deskriptif yang biasanya ditetapkan oleh kamera digital secara automatik. Sila berikan nama yang lebih deskriptif bagi fail tersebut.',

'upload-proto-error'      => 'Protokol salah',
'upload-proto-error-text' => 'Muat naik jauh memerlukan URL yang dimulakan dengan <code>http://</code> atau <code>ftp://</code>.',
'upload-file-error'       => 'Ralat dalaman',
'upload-file-error-text'  => 'Ralat dalaman telah berlaku ketika mencipta fail sementara pada komputer pelayan. Sila hubungi pentadbir sistem.',
'upload-misc-error'       => 'Ralat muat naik yang tidak diketahui',
'upload-misc-error-text'  => 'Ralat yang tidak diketahui telah berlaku ketika muat naik. Sila pastikan bahawa URL tersebut sah dan boleh dicapai kemudian cuba lagi. Jika masalah ini berterusan, sila hubungi pentadbir sistem.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL tidak dapat dicapai',
'upload-curl-error6-text'  => 'URL yang dinyatakan tidak dapat dicapai. Sila pastikan bahawa URL dan tapak web tersebut hidup.',
'upload-curl-error28'      => 'Waktu henti muat naik',
'upload-curl-error28-text' => 'Tapak web tersebut terlalu lambat bertindak balas. Sila pastikan bahawa tapak web tersebut hidup, tunggu sebentar dan cuba lagi. Anda boleh mencuba lagi pada waktu yang kurang sibuk.',

'license'            => 'Lesen:',
'nolicense'          => 'Tidak dipilih',
'license-nopreview'  => '(Tiada pratonton)',
'upload_source_url'  => ' (URL yang boleh diakses oleh orang awam)',
'upload_source_file' => ' (fail dalam komputer anda)',

# Special:ImageList
'imagelist-summary'     => 'Laman khas ini memaparkan senarai fail yang telah dimuat naik.
Klik di atas mana-mana lajur yang berkenaan untuk menukar tertib susunan.',
'imagelist_search_for'  => 'Cari nama imej:',
'imgfile'               => 'fail',
'imagelist'             => 'Senarai fail',
'imagelist_date'        => 'Tarikh',
'imagelist_name'        => 'Nama',
'imagelist_user'        => 'Pengguna',
'imagelist_size'        => 'Saiz',
'imagelist_description' => 'Huraian',

# Image description page
'filehist'                       => 'Sejarah fail',
'filehist-help'                  => 'Klik pada tarikh/waktu untuk melihat rupa fail tersebut pada waktu silam.',
'filehist-deleteall'             => 'hapuskan semua',
'filehist-deleteone'             => 'hapuskan ini',
'filehist-revert'                => 'balik',
'filehist-current'               => 'semasa',
'filehist-datetime'              => 'Tarikh/Waktu',
'filehist-user'                  => 'Pengguna',
'filehist-dimensions'            => 'Ukuran',
'filehist-filesize'              => 'Saiz fail',
'filehist-comment'               => 'Komen',
'imagelinks'                     => 'Pautan',
'linkstoimage'                   => '{{PLURAL:$1|Laman|$1 buah laman}} berikut mengandungi pautan ke fail ini:',
'nolinkstoimage'                 => 'Tiada laman yang mengandungi pautan ke fail ini.',
'morelinkstoimage'               => 'Lihat [[Special:WhatLinksHere/$1|semua pautan]] ke fail ini.',
'redirectstofile'                => '{{PLURAL:$1|Fail|$1 buah fail}} berikut melencong ke fail ini:',
'duplicatesoffile'               => '{{PLURAL:$1|Fail|$1 buah fail}} berikut adalah salinan bagi fail ini:',
'sharedupload'                   => 'Fail ini adalah fail muat naik kongsi dan boleh digunakan oleh projek lain.',
'shareduploadwiki'               => 'Sila lihat $1 untuk maklumat lanjut.',
'shareduploadwiki-desc'          => 'Berikut ialah keterangan yang diambil daripada $1nya di gedung kongsi.',
'shareduploadwiki-linktext'      => 'laman keterangan fail',
'shareduploadduplicate'          => 'Fail ini adalah salinan bagi $1 di gedung kongsi.',
'shareduploadduplicate-linktext' => 'fail lain',
'shareduploadconflict'           => 'Fail ini mempunyai nama yang sama dengan $1 di gedung kongsi.',
'shareduploadconflict-linktext'  => 'fail lain',
'noimage'                        => 'Fail ini tidak wujud. Anda boleh $1.',
'noimage-linktext'               => 'memuat naik fail baru',
'uploadnewversion-linktext'      => 'Muat naik versi baru bagi fail ini',
'imagepage-searchdupe'           => 'Cari fail serupa',

# File reversion
'filerevert'                => 'Balikkan $1',
'filerevert-legend'         => 'Balikkan fail',
'filerevert-intro'          => '<span class="plainlinks">Anda sedang menmbalikkan \'\'\'[[Media:$1|$1]]\'\'\' kepada [$4 versi pada $3, $2].</span>',
'filerevert-comment'        => 'Komen:',
'filerevert-defaultcomment' => 'Dibalikkan kepada versi pada $2, $1',
'filerevert-submit'         => 'Balikkan',
'filerevert-success'        => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\' telah dibalikkan kepada [$4 versi pada $3, $2].</span>',
'filerevert-badversion'     => 'Tiada versi tempatan bagi fail ini dengan cap waktu yang dinyatakan.',

# File deletion
'filedelete'                  => 'Hapuskan $1',
'filedelete-legend'           => 'Hapuskan fail',
'filedelete-intro'            => "Anda sedang menghapuskan '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => '<span class="plainlinks">Anda sedang menghapuskan versi \'\'\'[[Media:$1|$1]]\'\'\' pada [$4 $3, $2].</span>',
'filedelete-comment'          => 'Sebab hapus:',
'filedelete-submit'           => 'Hapus',
'filedelete-success'          => "'''$1''' telah dihapuskan.",
'filedelete-success-old'      => "Versi '''[[Media:$1|$1]]''' pada $3, $2 telah dihapuskan.",
'filedelete-nofile'           => "'''$1''' tidak wujud.",
'filedelete-nofile-old'       => "Tiada versi arkib bagi '''$1''' dengan sifat-sifat yang dinyatakan.",
'filedelete-iscurrent'        => 'Anda telah mencuba untuk menghapuskan versi terkini bagi fail ini. Sila balikkannya kepada versi yang lama terlebih dahulu.',
'filedelete-otherreason'      => 'Sebab lain/tambahan:',
'filedelete-reason-otherlist' => 'Sebab lain',
'filedelete-reason-dropdown'  => '
*Sebab-sebab lazim
** Melanggar hak cipta
** Fail berulang',
'filedelete-edit-reasonlist'  => 'Ubah sebab-sebab hapus',

# MIME search
'mimesearch'         => 'Carian MIME',
'mimesearch-summary' => 'Anda boleh menggunakan laman ini untuk mencari fail mengikut jenis MIME. Format input ialah "jenis/subjenis", contohnya <tt>image/jpeg</tt>.',
'mimetype'           => 'Jenis MIME:',
'download'           => 'muat turun',

# Unwatched pages
'unwatchedpages' => 'Laman tidak dipantau',

# List redirects
'listredirects' => 'Senarai lencongan',

# Unused templates
'unusedtemplates'     => 'Templat tidak digunakan',
'unusedtemplatestext' => 'Berikut ialah senarai templat yang tidak disertakan dalam laman lain. Sila pastikan bahawa anda menyemak pautan lain ke templat-templat tersebut sebelum menghapuskannya.',
'unusedtemplateswlh'  => 'pautan-pautan lain',

# Random page
'randompage'         => 'Laman rawak',
'randompage-nopages' => 'Tiada laman dalam ruang nama ini.',

# Random redirect
'randomredirect'         => 'Lencongan rawak',
'randomredirect-nopages' => 'Tiada lencongan dalam ruang nama ini.',

# Statistics
'statistics'             => 'Statistik',
'sitestats'              => 'Statistik {{SITENAME}}',
'userstats'              => 'Statistik pengguna',
'sitestatstext'          => "Terdapat sejumlah '''\$1''' laman dalam pangkalan data kami. Jumlah ini termasuklah laman \"perbincangan\", laman mengenai {{SITENAME}}, laman ringkas,
lencongan, dan lain-lain yang tidak dikira sebagai laman kandungan. Dengan mengecualikan laman-laman ini, terdapat sejumlah '''\$2''' laman yang barangkali dianggap sah.

'''\$8''' buah fail telah dimuat naik.

Terdapat sejumlah '''\$3''' paparan laman dan '''\$4''' penyuntingan dilakukan sejak {{SITENAME}} dibuka. Secara purata, terdapat '''\$5''' suntingan bagi setiap laman, dan '''\$6''' paparan bagi setiap suntingan.

Jumlah [http://www.mediawiki.org/wiki/Manual:Job_queue tugas yang tertunggak] ialah '''\$7'''.",
'userstatstext'          => "Terdapat '''$1''' pengguna berdaftar. '''$2''' (atau '''$4''') daripadanya mempunyai hak $5.",
'statistics-mostpopular' => 'Laman dilihat terbanyak',

'disambiguations'      => 'Laman penyahtaksaan',
'disambiguationspage'  => 'Template:disambig',
'disambiguations-text' => "Laman-laman berikut mengandungi pautan ke '''laman penyahtaksaan'''. Pautan ini sepatutnya ditujukan kepada topik yang sepatutnya.<br />Sesebuah laman dianggap sebagai laman penyahtaksaan jika ia menggunakan templat yang dipaut dari [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Lencongan berganda',
'doubleredirectstext'        => 'Berikut ialah sebarai laman yang melencong ke laman lencongan yang lain. Setiap baris mengandungi pautan ke laman lencongan pertama dan kedua, serta baris pertama bagi teks lencongan kedua, lazimnya merupakan laman sasaran "sebenar", yang sepatutnya ditujui oleh lencongan pertama.',
'double-redirect-fixed-move' => '[[$1]] dilencongkan ke [[$2]]',
'double-redirect-fixer'      => 'Pembaiki lencongan',

'brokenredirects'        => 'Lencongan rosak',
'brokenredirectstext'    => 'Lencongan-lencongan berikut menuju ke laman yang tidak wujud:',
'brokenredirects-edit'   => '(sunting)',
'brokenredirects-delete' => '(hapus)',

'withoutinterwiki'         => 'Laman tanpa pautan bahasa',
'withoutinterwiki-summary' => 'Laman-laman berikut tidak mempunyai pautan ke versi bahasa lain:',
'withoutinterwiki-legend'  => 'Awalan',
'withoutinterwiki-submit'  => 'Tunjuk',

'fewestrevisions' => 'Laman dengan semakan tersedikit',

# Miscellaneous special pages
'nbytes'                  => '$1 bait',
'ncategories'             => '$1 kategori',
'nlinks'                  => '$1 pautan',
'nmembers'                => '$1 ahli',
'nrevisions'              => '$1 semakan',
'nviews'                  => 'Dilihat $1 kali',
'specialpage-empty'       => 'Tiada keputusan bagi laporan ini.',
'lonelypages'             => 'Laman yatim',
'lonelypagestext'         => 'Laman-laman berikut tidak dipaut dari laman lain dalam {{SITENAME}}.',
'uncategorizedpages'      => 'Laman tanpa kategori',
'uncategorizedcategories' => 'Kategori tanpa kategori',
'uncategorizedimages'     => 'Imej tanpa kategori',
'uncategorizedtemplates'  => 'Templat tanpa kategori',
'unusedcategories'        => 'Kategori tidak digunakan',
'unusedimages'            => 'Imej tidak digunakan',
'popularpages'            => 'Laman popular',
'wantedcategories'        => 'Kategori dikehendaki',
'wantedpages'             => 'Laman dikehendaki',
'missingfiles'            => 'Fail hilang',
'mostlinked'              => 'Laman dipaut terbanyak',
'mostlinkedcategories'    => 'Kategori dipaut terbanyak',
'mostlinkedtemplates'     => 'Templat dipaut terbanyak',
'mostcategories'          => 'Rencana dengan kategori terbanyak',
'mostimages'              => 'Imej dipaut terbanyak',
'mostrevisions'           => 'Rencana dengan semakan terbanyak',
'prefixindex'             => 'Indeks awalan',
'shortpages'              => 'Laman pendek',
'longpages'               => 'Laman panjang',
'deadendpages'            => 'Laman buntu',
'deadendpagestext'        => 'Laman-laman berikut tidak mengandungi pautan ke laman lain di {{SITENAME}}.',
'protectedpages'          => 'Laman dilindungi',
'protectedpages-indef'    => 'Perlindungan tanpa had sahaja',
'protectedpagestext'      => 'Laman-laman berikut dilindungi daripada pemindahan dan penyuntingan',
'protectedpagesempty'     => 'Tiada laman yang dilindungi dengan kriteria ini.',
'protectedtitles'         => 'Tajuk dilindungi',
'protectedtitlestext'     => 'Tajuk-tajuk berikut dilindungi daripada dicipta',
'protectedtitlesempty'    => 'Tiada tajuk yang dilindungi yang sepadan dengan kriteria yang diberikan.',
'listusers'               => 'Senarai pengguna',
'newpages'                => 'Laman baru',
'newpages-username'       => 'Nama pengguna:',
'ancientpages'            => 'Laman lapuk',
'move'                    => 'Alih',
'movethispage'            => 'Pindahkan laman ini',
'unusedimagestext'        => '<p>Sila ambil perhatian bahawa
mungkin terdapat tapak web lain yang mengandungi pautan ke imej ini
menggunakan URL langsung walaupun ia disenaraikan di sini.</p>',
'unusedcategoriestext'    => 'Laman-laman kategori berikut wujud walaupun tiada laman atau kategori lain menggunakannya.',
'notargettitle'           => 'Tiada sasaran',
'notargettext'            => 'Anda tidak menyatakan laman atau pengguna sebagai sasaran bagi tindakan ini.',
'nopagetitle'             => 'Laman sasaran tidak wujud',
'nopagetext'              => 'Laman sasaran yang anda nyatakan tidak wujud.',
'pager-newer-n'           => '$1 berikutnya',
'pager-older-n'           => '$1 sebelumnya',
'suppress'                => 'Kawalan',

# Book sources
'booksources'               => 'Sumber buku',
'booksources-search-legend' => 'Cari sumber buku',
'booksources-go'            => 'Pergi',
'booksources-text'          => 'Berikut ialah senarai pautan ke tapak web lain yang menjual buku baru dan terpakai,
serta mungkin mempunyai maklumat lanjut mengenai buku yang anda cari:',

# Special:Log
'specialloguserlabel'  => 'Pengguna:',
'speciallogtitlelabel' => 'Tajuk:',
'log'                  => 'Log',
'all-logs-page'        => 'Semua log',
'log-search-legend'    => 'Cari log',
'log-search-submit'    => 'Pergi',
'alllogstext'          => 'Berikut ialah gabungan bagi semua log yang ada bagi {{SITENAME}}. Anda boleh menapis senarai ini dengan memilih jenis log, nama pengguna (peka huruf besar), atau nama laman yang terjejas (juga peka huruf besar).',
'logempty'             => 'Tiada item yang sepadan dalam log.',
'log-title-wildcard'   => 'Cari semua tajuk yang bermula dengan teks ini',

# Special:AllPages
'allpages'          => 'Semua laman',
'alphaindexline'    => '$1 hingga $2',
'nextpage'          => 'Halaman berikutnya ($1)',
'prevpage'          => 'Halaman sebelumnya ($1)',
'allpagesfrom'      => 'Tunjukkan laman bermula pada:',
'allarticles'       => 'Semua laman',
'allinnamespace'    => 'Semua laman (ruang nama $1)',
'allnotinnamespace' => 'Semua laman (bukan dalam ruang nama $1)',
'allpagesprev'      => 'Sebelumnya',
'allpagesnext'      => 'Berikutnya',
'allpagessubmit'    => 'Pergi',
'allpagesprefix'    => 'Tunjukkan laman dengan awalan:',
'allpagesbadtitle'  => 'Tajuk laman yang dinyatakan tidak sah atau mempunyai awalam antara bahasa atau antara wiki. Ia mungkin mengandungi aksara yang tidak boleh digunakan dalam tajuk laman.',
'allpages-bad-ns'   => '{{SITENAME}} tidak mempunyai ruang nama "$1".',

# Special:Categories
'categories'                    => 'Kategori',
'categoriespagetext'            => 'Kategori-kategori berikut mengandungi laman-laman dan bahan-bahan media.
[[Special:UnusedCategories|Kategori yang tidak digunakan]] tidak dipaparkan di sini.
Lihat juga [[Special:WantedCategories|senarai kategori dikehendaki]].',
'categoriesfrom'                => 'Paparkan kategori bermula daripada:',
'special-categories-sort-count' => 'susun mengikut tertib bilangan',
'special-categories-sort-abc'   => 'susun mengikut tertib abjad',

# Special:ListUsers
'listusersfrom'      => 'Tunjukkan pengguna bermula pada:',
'listusers-submit'   => 'Tunjuk',
'listusers-noresult' => 'Tiada pengguna dijumpai.',

# Special:ListGroupRights
'listgrouprights'          => 'Hak kumpulan pengguna',
'listgrouprights-summary'  => 'Berikut ialah senarai kumpulan pengguna yang ditubuhkan di wiki ini dengan hak-hak masing-masing.
Anda boleh mengetahui [[{{MediaWiki:Listgrouprights-helppage}}|maklumat tambahan]] mengenai setiap hak.',
'listgrouprights-group'    => 'Kumpulan',
'listgrouprights-rights'   => 'Hak',
'listgrouprights-helppage' => 'Help:Hak kumpulan',
'listgrouprights-members'  => '(senarai ahli)',

# E-mail user
'mailnologin'     => 'Tiada alamat e-mel',
'mailnologintext' => 'Anda perlu [[Special:UserLogin|log masuk]]
terlebih dahulu dan mempunyai alamat e-mel yang sah dalam
[[Special:Preferences|laman keutamaan]] untuk mengirim e-mel kepada pengguna lain.',
'emailuser'       => 'Kirim e-mel kepada pengguna ini',
'emailpage'       => 'E-mel pengguna',
'emailpagetext'   => 'Jika pengguna ini telah memasukkan alamat e-mel yang sah dalam laman keutamaan, beliau akan menerima sebuah e-mel dengan pesanan yang diisi dalam borang di bawah. Alamat e-mel yang ditetapkan dalam [[Special:Preferences|keutamaan anda]] akan muncul dalam e-mel tersebut sebagai alamat "Daripada" supaya si penerima boleh membalasnya.',
'usermailererror' => 'Objek Mail memulangkan ralat:',
'defemailsubject' => 'E-mel {{SITENAME}}',
'noemailtitle'    => 'Tiada alamat e-mel',
'noemailtext'     => 'Pengguna ini tidak menetapkan alamat e-mel yang sah,
atau telah memilih untuk tidak menerima e-mel daripada pengguna lain.',
'emailfrom'       => 'Daripada:',
'emailto'         => 'Kepada:',
'emailsubject'    => 'Perkara:',
'emailmessage'    => 'Pesanan:',
'emailsend'       => 'Kirim',
'emailccme'       => 'Kirim salinan mesej ini kepada saya.',
'emailccsubject'  => 'Salinan bagi mesej anda kepada $1: $2',
'emailsent'       => 'E-mel dikirim',
'emailsenttext'   => 'E-mel anda telah dikirim.',
'emailuserfooter' => 'E-mel ini telah dikirim oleh $1 kepada $2 menggunakan alat "E-mel pengguna" di {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Senarai pantau',
'mywatchlist'          => 'Senarai pantau saya',
'watchlistfor'         => "(bagi '''$1''')",
'nowatchlist'          => 'Tiada item dalam senarai pantau anda.',
'watchlistanontext'    => 'Sila $1 terlebih dahulu untuk melihat atau menyunting senarai pantau anda.',
'watchnologin'         => 'Belum log masuk',
'watchnologintext'     => 'Anda mesti [[Special:UserLogin|log masuk]] terlebih dahulu untuk mengubah senarai pantau.',
'addedwatch'           => 'Senarai pantau dikemaskinikan',
'addedwatchtext'       => "Laman \"[[:\$1]]\" telah ditambahkan ke dalam [[Special:Watchlist|senarai pantau]] anda.
Semua perubahan bagi laman tersebut dan laman perbincangannya akan disenaraikan di sana,
dan tajuk laman tersebut juga akan ditonjolkan dalam '''teks tebal''' di [[Special:RecentChanges|senarai perubahan terkini]]
untuk memudahkan anda.

Jika anda mahu membuang laman tersebut daripada senarai pantau, klik \"Nyahpantau\" pada bar sisi.",
'removedwatch'         => 'Dibuang daripada senarai pantau',
'removedwatchtext'     => 'Laman "[[:$1]]" telah dibuang daripada senarai pantau anda.',
'watch'                => 'Pantau',
'watchthispage'        => 'Pantau laman ini',
'unwatch'              => 'Nyahpantau',
'unwatchthispage'      => 'Berhenti memantau',
'notanarticle'         => 'Bukan laman kandungan',
'notvisiblerev'        => 'Semakan ini telah dihapuskan',
'watchnochange'        => 'Tiada perubahan pada laman-laman yang dipantau dalam tempoh yang ditunjukkan.',
'watchlist-details'    => '$1 laman dipantau (tidak termasuk laman perbincangan).',
'wlheader-enotif'      => '* Pemberitahuan melalui e-mel diaktifkan.',
'wlheader-showupdated' => "* Laman-laman yang telah diubah sejak kunjungan terakhir anda dipaparkan dalam '''teks tebal'''",
'watchmethod-recent'   => 'menyemak laman yang dipantau dalam suntingan-suntingan terkini',
'watchmethod-list'     => 'menyemak suntingan terkini pada laman-laman yang dipantau',
'watchlistcontains'    => 'Terdapat $1 laman dalam senarai pantau anda.',
'iteminvalidname'      => "Terdapat masalah dengan item '$1', nama tidak sah...",
'wlnote'               => "Berikut ialah '''$1''' perubahan terakhir sejak '''$2''' jam yang lalu.",
'wlshowlast'           => 'Tunjukkan $1 jam / $2 hari yang lalu / $3.',
'watchlist-show-bots'  => 'Papar suntingan bot',
'watchlist-hide-bots'  => 'Sembunyi suntingan bot',
'watchlist-show-own'   => 'Papar suntingan saya',
'watchlist-hide-own'   => 'Sembunyi suntingan saya',
'watchlist-show-minor' => 'Papar suntingan kecil',
'watchlist-hide-minor' => 'Sembunyi suntingan kecil',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Memantau...',
'unwatching' => 'Menyahpantau...',

'enotif_mailer'                => 'Sistem Pemberitahuan {{SITENAME}}',
'enotif_reset'                 => 'Tandakan semua laman sebagai telah dikunjungi',
'enotif_newpagetext'           => 'Ini adalah sebuah laman baru.',
'enotif_impersonal_salutation' => 'Pengguna {{SITENAME}}',
'changed'                      => 'diubah',
'created'                      => 'dicipta',
'enotif_subject'               => 'Laman $PAGETITLE di {{SITENAME}} telah $CHANGEDORCREATED oleh $PAGEEDITOR',
'enotif_lastvisited'           => 'Lihat $1 untuk semua perubahan sejak kunjungan terakhir anda.',
'enotif_lastdiff'              => 'Rujuk $1 untuk melihat perubahan ini.',
'enotif_anon_editor'           => 'pengguna tanpa nama $1',
'enotif_body'                  => 'Saudara/saudari $WATCHINGUSERNAME,


Laman $PAGETITLE di {{SITENAME}} telah $CHANGEDORCREATED pada $PAGEEDITDATE oleh $PAGEEDITOR, sila lihat $PAGETITLE_URL untuk versi semasa.

$NEWPAGE

Ringkasan: $PAGESUMMARY $PAGEMINOREDIT

Anda boleh menghubungi si penyunting melalui:
e-mel: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Tiada pemberitahuan lain akan dikirim selagi anda tidak mengunjungi laman tersebut. Anda juga boleh mengeset semula tanda pemberitahuan bagi semua laman dalam senarai pantau anda.

         Sistem pemberitahuan {{SITENAME}}

--
Untuk mengubah tetapan senarai pantau anda, sila kunjungi
{{fullurl:{{ns:special}}:Watchlist/edit}}

Maklum balas dan bantuan:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Hapus laman',
'confirm'                     => 'Sahkan',
'excontent'                   => "kandungan: '$1'",
'excontentauthor'             => "Kandungan: '$1' (dan satu-satunya penyumbang ialah '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "kandungan sebelum pengosongan ialah: '$1'",
'exblank'                     => 'laman tersebut kosong',
'delete-confirm'              => 'Hapus "$1"',
'delete-legend'               => 'Hapus',
'historywarning'              => '<b>Amaran</b>: Laman yang ingin anda hapuskan mengandungi sejarah:',
'confirmdeletetext'           => 'Anda sudah hendak menghapuskan sebuah laman berserta semua sejarahnya.
Sila sahkan bahawa anda memang hendak berbuat demikian, anda faham akan
akibatnya, dan perbuatan anda mematuhi [[{{MediaWiki:Policy-url}}|dasar kami]].',
'actioncomplete'              => 'Tindakan berjaya',
'deletedtext'                 => '"<nowiki>$1</nowiki>" telah dihapuskan.
Sila lihat $2 untuk rekod penghapusan terkini.',
'deletedarticle'              => 'menghapuskan "[[$1]]"',
'suppressedarticle'           => 'menahan "[[$1]]"',
'dellogpage'                  => 'Log penghapusan',
'dellogpagetext'              => 'Berikut ialah senarai penghapusan terkini.',
'deletionlog'                 => 'log penghapusan',
'reverted'                    => 'Dibalikkan kepada semakan sebelumnya',
'deletecomment'               => 'Sebab penghapusan:',
'deleteotherreason'           => 'Sebab lain/tambahan:',
'deletereasonotherlist'       => 'Sebab lain',
'deletereason-dropdown'       => '
* Sebab-sebab lazim
** Permintaan pengarang
** Melanggar hak cipta
** Vandalisme',
'delete-edit-reasonlist'      => 'Ubah sebab-sebab hapus',
'delete-toobig'               => 'Laman ini mempunyai sejarah yang besar, iaitu melebihi $1 jumlah semakan. Oleh itu, laman ini dilindungi daripada dihapuskan untuk mengelak kerosakan di {{SITENAME}} yang tidak disengajakan.',
'delete-warning-toobig'       => 'Laman ini mempunyai sejarah yang besar, iaitu melebihi $1 jumlah semakan. Menghapuskannya boleh mengganggu perjalanan pangkalan data {{SITENAME}}. Sila berhati-hati.',
'rollback'                    => 'Undurkan suntingan.',
'rollback_short'              => 'Undur',
'rollbacklink'                => 'undur',
'rollbackfailed'              => 'Pengunduran gagal',
'cantrollback'                => 'Suntingan tersebut tidak dapat dibalikkan: penyumbang terakhir adalah satu-satunya pengarang bagi rencana ini.',
'alreadyrolled'               => 'Tidak dapat membalikkan suntingan terakhir bagi [[:$1]]
oleh [[User:$2|$2]] ([[User talk:$2|Perbincangan]]); terdapat pengguna yang telah berbuat demikian.

Suntingan terakhir telah dibuat oleh [[User:$3|$3]] ([[User talk:$3|Perbincangan]]).',
'editcomment'                 => 'Komen suntingan: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Membalikkan suntingan oleh [[Special:Contributions/$2|$2]] ([[User talk:$2|Perbincangan]]) kepada versi terakhir oleh [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Membalikkan suntingan oleh $1 kepada versi terakhir oleh $2.',
'sessionfailure'              => 'Terdapat sedikit masalah pada sesi log masuk anda.
Tindakan ini telah dibatalkan untuk mencegah perampasan sesi.
Sila tekan butang "back" dan muatkan semula laman yang telah anda kunjungi sebelum ini, kemudian cuba lagi.',
'protectlogpage'              => 'Log perlindungan',
'protectlogtext'              => 'Berikut ialah senarai bagi tindakan penguncian/pembukaan laman. Sila lihat [[Special:ProtectedPages|senarai laman dilindungi]] untuk rujukan lanjut.',
'protectedarticle'            => 'melindungi "[[$1]]"',
'modifiedarticleprotection'   => 'menukar peringkat perlindungan bagi "[[$1]]"',
'unprotectedarticle'          => 'menyahlindung "[[$1]]"',
'protect-title'               => 'Menetapkan peringkat perlindungan bagi "$1"',
'protect-legend'              => 'Sahkan perlindungan',
'protectcomment'              => 'Komen:',
'protectexpiry'               => 'Tamat pada:',
'protect_expiry_invalid'      => 'Waktu tamat tidak sah.',
'protect_expiry_old'          => 'Waktu tamat telah berlalu.',
'protect-unchain'             => 'Buka kunci keizinan pemindahan',
'protect-text'                => 'Anda boleh melihat dan menukar peringkat perlindungan bagi laman <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Anda telah disekat, justeru tidak boleh menukar peringkat perlindungan.
Ini adalah tetapan semasa bagi laman <strong>$1</strong>:',
'protect-locked-dblock'       => 'Anda tidak boleh menukar peringkat perlindungan kerana pangkalan data sedang dikunci.
Ini adalah tetapan semasa bagi laman <strong>$1</strong>:',
'protect-locked-access'       => 'Anda tidak mempunyai keizinan untuk menukar peringkat perlindungan.
Ini adalah tetapan semasa bagi laman <strong>$1</strong>:',
'protect-cascadeon'           => 'Laman ini dilindungi kerana ia terkandung dalam {{PLURAL:$1|laman|laman-laman}} berikut, yang dilindungi secara melata. Anda boleh menukar peringkat perlindunan laman ini, akan tetapi ia tidak akan menjejaskan perlindungan melata tersebut.',
'protect-default'             => '(lalai)',
'protect-fallback'            => 'Perlukan keizinan "$1"',
'protect-level-autoconfirmed' => 'Sekat pengguna-pengguna tidak berdaftar',
'protect-level-sysop'         => 'Penyelia sahaja',
'protect-summary-cascade'     => 'melata',
'protect-expiring'            => 'tamat pada $1 (UTC)',
'protect-cascade'             => 'Lindungi semua laman yang terkandung dalam laman ini (perlindungan melata)',
'protect-cantedit'            => 'Anda tidak dibenarkan menukar peringkat perlindungan bagi laman ini.',
'restriction-type'            => 'Keizinan:',
'restriction-level'           => 'Peringkat pembatasan:',
'minimum-size'                => 'Saiz minimum',
'maximum-size'                => 'Saiz maksimum',
'pagesize'                    => '(bait)',

# Restrictions (nouns)
'restriction-edit'   => 'Sunting',
'restriction-move'   => 'Pindah',
'restriction-create' => 'Cipta',
'restriction-upload' => 'Muat naik',

# Restriction levels
'restriction-level-sysop'         => 'perlindungan penuh',
'restriction-level-autoconfirmed' => 'perlindungan separa',
'restriction-level-all'           => 'semua peringkat',

# Undelete
'undelete'                     => 'Lihat laman-laman yang dihapuskan',
'undeletepage'                 => 'Lihat dan pulihkan laman yang dihapuskan',
'undeletepagetitle'            => "'''Berikut ialah semakan-semakan [[:$1|$1]] yang telah dihapuskan'''.",
'viewdeletedpage'              => 'Lihat laman-laman yang dihapuskan',
'undeletepagetext'             => 'Laman-laman berikut telah dihapuskan tetapi masih disimpan dalam arkib dan
masih boleh dipulihkan. Arkib tersebut akan dibersihkan dari semasa ke semasa.',
'undelete-fieldset-title'      => 'Pulihkan semakan',
'undeleteextrahelp'            => "Untuk memulihkan keseluruhan laman, biarkan semua kotak semak dan klik '''''Pulih'''''. Untuk melaksanakan pemulihan tertentu, tanda di setiap kotak yang bersebelahan dengan semakan untuk dipulihkan dan klik '''''Pulih'''''. Klik '''''Set semula''''' untuk mengosongkan ruangan komen dan membuang tanda semua kotak.",
'undeleterevisions'            => '$1 semakan telah diarkibkan.',
'undeletehistory'              => 'Jika anda memulihkan laman tersebut, semua semakan akan dipulihkan kepada sejarahnya. Jika sebuah laman baru yang mempunyai nama yang sama telah dicipta sejak penghapusan, semakan yang dipulihkan akan muncul dalam sejarah terdahulu.',
'undeleterevdel'               => 'Penyahhapusan tidak akan dilaksanakan sekiranya ia menyebabkan sebahagian semakan puncak dihapuskan.
Dalam hal tersebut, anda perlu membuang semak atau menyemak semakan yang baru dihapuskan. Semakan fail
yang anda tidak dibenarkan melihatnya tidak akan dipulihkan.',
'undeletehistorynoadmin'       => 'Rencana ini telah dihapuskan. Sebab penghapusan
ditunjukkan dalam ringkasan di bawah, berserta butiran bagi pengguna-pengguna yang telah menyunting laman ini
sebelum penghapusan. Teks sebenar bagi semua semakan yang dihapuskan hanya boleh dilihat oleh para pentadbir.',
'undelete-revision'            => 'Menghapuskan semakan bagi $1 (pada $2) oleh $3:',
'undeleterevision-missing'     => 'Semakan tersebut tidak sah atau tidak dijumpai. Mungkin anda telah mengikuti pautan yang rosak
atau semakan tersebut telah dipulihkan atau dibuang daripada arkib.',
'undelete-nodiff'              => 'Tiada semakan sebelumnya.',
'undeletebtn'                  => 'Pulihkan',
'undeletelink'                 => 'pulih',
'undeletereset'                => 'set semula',
'undeletecomment'              => 'Komen:',
'undeletedarticle'             => '"[[$1]]" telah dipulihkan',
'undeletedrevisions'           => '$1 semakan dipulihkan',
'undeletedrevisions-files'     => '$1 semakan dan $2 fail dipulihkan',
'undeletedfiles'               => '$1 fail dipulihkan',
'cannotundelete'               => 'Penyahhapusan gagal; mungkin orang lain telah pun mengnyahhapuskannya.',
'undeletedpage'                => "<big>'''$1 telah dipulihkan'''</big>

Sila rujuk [[Special:Log/delete|log penghapusan]] untuk rekod penghapusan terkini.",
'undelete-header'              => 'Lihat [[Special:Log/delete|log penghapusan]] untuk laman-laman yang baru dihapuskan.',
'undelete-search-box'          => 'Cari laman yang dihapuskan',
'undelete-search-prefix'       => 'Tunjukkan laman bermula dengan:',
'undelete-search-submit'       => 'Cari',
'undelete-no-results'          => 'Tiada laman yang sepadan dijumpai dalam arkib penghapusan.',
'undelete-filename-mismatch'   => 'Semakan pada $1 tidak boleh dinyahhapuskan: nama fail tidak sepadan',
'undelete-bad-store-key'       => 'Semakan pada $1 tidak boleh dinyahhapuskan: fail telah hilang.',
'undelete-cleanup-error'       => 'Ralat ketika menyahhhapuskan fail "$1" dalam arkib yang tidak digunakan.',
'undelete-missing-filearchive' => 'Arkib fail dengan ID $1 tidak dapat dipulihkan kerana tiada dalam pangkalan data. Ia mungkin telah pun dinyahhapuskan.',
'undelete-error-short'         => 'Ralat ketika menyahhapuskan fail: $1',
'undelete-error-long'          => 'Berlaku ralat ketika menyahhapuskan fail tersebut:

$1',

# Namespace form on various pages
'namespace'      => 'Ruang nama:',
'invert'         => 'Kecualikan pilihan',
'blanknamespace' => '(Utama)',

# Contributions
'contributions' => 'Sumbangan',
'mycontris'     => 'Sumbangan saya',
'contribsub2'   => 'Oleh $1 ($2)',
'nocontribs'    => 'Tiada sebarang perubahan yang sepadan dengan kriteria-kriteria ini.',
'uctop'         => ' (puncak)',
'month'         => 'Sebelum bulan:',
'year'          => 'Sebelum tahun:',

'sp-contributions-newbies'     => 'Tunjuk sumbangan daripada akaun baru sahaja',
'sp-contributions-newbies-sub' => 'Bagi akaun-akaun baru',
'sp-contributions-blocklog'    => 'Log sekatan',
'sp-contributions-search'      => 'Cari sumbangan',
'sp-contributions-username'    => 'Alamat IP atau nama pengguna:',
'sp-contributions-submit'      => 'Cari',

# What links here
'whatlinkshere'            => 'Pautan ke laman ini',
'whatlinkshere-title'      => 'Laman yang mengandungi pautan ke "$1"',
'whatlinkshere-page'       => 'Laman:',
'linklistsub'              => '(Senarai pautan masuk)',
'linkshere'                => "Laman-laman berikut mengandungi pautan ke '''[[:$1]]''':",
'nolinkshere'              => "Tiada laman yang mengandungi pautan ke '''[[:$1]]'''.",
'nolinkshere-ns'           => "Tiada laman yang mengandungi pautan ke '''[[:$1]]''' dalam ruang nama yang dinyatakan.",
'isredirect'               => 'laman lencongan',
'istemplate'               => 'penyertaan',
'isimage'                  => 'pautan imej',
'whatlinkshere-prev'       => '{{PLURAL:$1|sebelumnya|$1 sebelumnya}}',
'whatlinkshere-next'       => '{{PLURAL:$1|berikutnya|$1 berikutnya}}',
'whatlinkshere-links'      => '← pautan',
'whatlinkshere-hideredirs' => '$1 pelencongan',
'whatlinkshere-hidetrans'  => '$1 kemasukan',
'whatlinkshere-hidelinks'  => '$1 pautan',
'whatlinkshere-hideimages' => '$1 pautan imej',
'whatlinkshere-filters'    => 'Tapis',

# Block/unblock
'blockip'                         => 'Sekat pengguna',
'blockip-legend'                  => 'Sekat pengguna',
'blockiptext'                     => 'Gunakan borang di bawah untuk menyekat
penyuntingan daripada alamat IP atau pengguna tertentu.
Tindakan ini perlu dilakukan untuk menentang vandalisme sahaja dan selaras
dengan [[{{MediaWiki:Policy-url}}|dasar {{SITENAME}}]].
Sila masukkan sebab sekatan di bawah (umpamannya, sebutkan laman yang telah
dirosakkan).',
'ipaddress'                       => 'Alamat IP:',
'ipadressorusername'              => 'Alamat IP atau nama pengguna:',
'ipbexpiry'                       => 'Tempoh:',
'ipbreason'                       => 'Sebab:',
'ipbreasonotherlist'              => 'Lain-lain',
'ipbreason-dropdown'              => '
*Sebab lazim
** Memasukkan maklumat palsu
** Membuang kandungan daripada laman
** Memmasukkan pautan spam ke tapak web luar
** Memasukkan karut-marut ke dalam laman
** Mengugut/mengganggu pengguna lain
** Menyalahgunakan berbilang akaun
** Nama pengguna yang tidak sesuai',
'ipbanononly'                     => 'Sekat pengguna tanpa nama sahaja',
'ipbcreateaccount'                => 'Tegah pembukaan akaun',
'ipbemailban'                     => 'Halang pengguna tersebut daripada mengirim e-mel',
'ipbenableautoblock'              => 'Sekat alamat IP terakhir dan mana-mana alamat berikutnya yang digunakan oleh pengguna ini secara automatik',
'ipbsubmit'                       => 'Sekat pengguna ini',
'ipbother'                        => 'Waktu lain:',
'ipboptions'                      => '2 jam:2 hours,1 hari:1 day,3 hari:3 days,1 minggu:1 week,2 minggu:2 weeks,1 bulan:1 month,3 bulan:3 months,6 bulan:6 months,1 tahun:1 year,selama-lamanya:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'lain',
'ipbotherreason'                  => 'Sebab tambahan/lain:',
'ipbhidename'                     => 'Sembunyikan nama pengguna/alamat IP daripada log sekatan, senarai sekatan aktif, dan senarai pengguna',
'ipbwatchuser'                    => 'Pantau laman pengguna dan laman perbincangan bagi pengguna ini',
'badipaddress'                    => 'Alamat IP tidak sah',
'blockipsuccesssub'               => 'Sekatan berjaya',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] telah disekat.
<br />Sila lihat [[Special:IPBlockList|senarai sekatan IP]] untuk maklumat lanjut.',
'ipb-edit-dropdown'               => 'Sunting sebab sekatan',
'ipb-unblock-addr'                => 'Nyahsekat $1',
'ipb-unblock'                     => 'Nyahsekat nama pengguna atau alamat IP',
'ipb-blocklist-addr'              => 'Lihat sekatan sedia ada bagi $1',
'ipb-blocklist'                   => 'Lihat sekatan sedia ada',
'unblockip'                       => 'Nyahsekat pengguna',
'unblockiptext'                   => 'Gunakan borang di bawah untuk membuang sekatan bagialamat IP atau nama pengguna yang telah disekat.',
'ipusubmit'                       => 'Nyahsekat alamat ini.',
'unblocked'                       => '[[User:$1|$1]] telah dinyahsekat',
'unblocked-id'                    => 'Sekatan $1 telah dibuang',
'ipblocklist'                     => 'Alamat IP dan nama pengguna yang disekat',
'ipblocklist-legend'              => 'Cari pengguna yang disekat',
'ipblocklist-username'            => 'Nama pengguna atau alamat IP:',
'ipblocklist-submit'              => 'Cari',
'blocklistline'                   => '$1, $2 menyekat $3 ($4)',
'infiniteblock'                   => 'selama-lamanya',
'expiringblock'                   => 'sehingga $1',
'anononlyblock'                   => 'pengguna tanpa nama sahaja',
'noautoblockblock'                => 'sekatan automatik dipadamkan',
'createaccountblock'              => 'pembukaan akaun baru disekat',
'emailblock'                      => 'e-mail disekat',
'ipblocklist-empty'               => 'Senarai sekatan adalah kosong.',
'ipblocklist-no-results'          => 'Alamat IP atau nama pengguna tersebut tidak disekat.',
'blocklink'                       => 'sekat',
'unblocklink'                     => 'nyahsekat',
'contribslink'                    => 'sumb.',
'autoblocker'                     => 'Disekat secara automatik kerana baru-baru ini alamat IP anda digunakan oleh "[[User:$1|$1]]". Sebab sekatan $1 ialah: "$2"',
'blocklogpage'                    => 'Log sekatan',
'blocklogentry'                   => 'menyekat [[$1]] sehingga $2 $3',
'blocklogtext'                    => 'Ini adalah log bagi sekatan dan penyahsekatan.
Alamat IP yang disekat secara automatik tidak disenaraikan di sini. Sila lihat
[[Special:IPBlockList|senarai sekatan IP]] untuk mengetahui sekatan-sekatan yang sedang dijalankan.',
'unblocklogentry'                 => 'menyahsekat $1',
'block-log-flags-anononly'        => 'pengguna tanpa nama sahaja',
'block-log-flags-nocreate'        => 'pembukaan akaun dimatikan',
'block-log-flags-noautoblock'     => 'sekatan automatik dimatikan',
'block-log-flags-noemail'         => 'e-mail disekat',
'block-log-flags-angry-autoblock' => 'sekatan automatik tambahan diaktifkan',
'range_block_disabled'            => 'Kebolehan penyelia untuk membuat sekatan julat dimatikan.',
'ipb_expiry_invalid'              => 'Waktu tamat tidak sah.',
'ipb_expiry_temp'                 => 'Sekatan nama pengguna terselindung sepatutnya kekal.',
'ipb_already_blocked'             => '"$1" telah pun disekat',
'ipb_cant_unblock'                => 'Ralat: ID sekatan $1 tidak dijumpai. Barangkali ia telah pun dinyahsekat.',
'ipb_blocked_as_range'            => 'Ralat: IP $1 tidak boleh dinyahsekat kerana ia tidak disekat secara langsung. Sebaliknya, ia disekat kerana merupakan sebahagian daripada sekatan julat $2, yang mana boleh dinyahsekat.',
'ip_range_invalid'                => 'Julat IP tidak sah.',
'blockme'                         => 'Sekat saya',
'proxyblocker'                    => 'Sekatan proksi',
'proxyblocker-disabled'           => 'Fungsi ini dimatikan.',
'proxyblockreason'                => 'Alamat IP anda telah disekat kerana ia merupakan proksi terbuka. Sila hubungi penyedia perkhidmatan Internet anda atau pihak sokongan teknikal dan beritahu mereka mengenai masalah berat ini.',
'proxyblocksuccess'               => 'Berjaya.',
'sorbsreason'                     => 'Alamat IP anda telah disenaraikan sebagai proksi terbuka dalam DNSBL yang digunakan oleh {{SITENAME}}.',
'sorbs_create_account_reason'     => 'Alamat IP anda telah disenaraikan sebagai proksi terbuka dalam DNSBL yang digunakan oleh {{SITENAME}}. Oleh itu, anda tidak dibenarkan membuka akaun baru.',

# Developer tools
'lockdb'              => 'Kunci pangkalan data',
'unlockdb'            => 'Buka kunci pangkalan data.',
'lockdbtext'          => 'Penguncian pangkalan data akan membekukan kebolehan semua
pengguna untuk menyunting laman, mengubah keutamaan, menyunting senarai
sekatan, dan perkara lain yang memerlukan perubahan dalam pangkalan data.
Sila sahkan bahawa anda memang berniat untuk melakukan tindakan ini, dan
bahawa anda akan membuka semula kunci pangkalan data ini setelah penyenggaraan selesai.',
'unlockdbtext'        => 'Pembukaan kunci pangkalan data boleh
memulihkan kebolehan semua pengguna untuk menyunting laman, keutamaan, senarai
pantau dan sebagainya yang melibatkan perubahan dalam pangkalan data. Sila
sahkan bahawa anda betul-betul mahu melakukan tindakan ini.',
'lockconfirm'         => 'Ya, saya mahu mengunci pangkalan data ini.',
'unlockconfirm'       => 'Ya, saya betul-betul mahu membuka kunci pangkalan data.',
'lockbtn'             => 'Kunci pangkalan data',
'unlockbtn'           => 'Buka kunci pangkalan data',
'locknoconfirm'       => 'Anda tidak menyemak kotak pengesahan.',
'lockdbsuccesssub'    => 'Pangkalan data dikunci.',
'unlockdbsuccesssub'  => 'Kunci pangkalan data dibuka.',
'lockdbsuccesstext'   => 'Pangkalan data telah dikunci.
<br />Pastikan anda [[Special:UnlockDB|membukanya semula]] selepas penyelenggaraan selesai.',
'unlockdbsuccesstext' => 'Kunci pangkalan data {{SITENAME}} telah dibuka.',
'lockfilenotwritable' => 'Fail kunci pangkalan data tidak boleh ditulis. Untuk mengunci atau membuka kunci pangkalan data, fail ini perlu diubah suai supaya boleh ditulis oleh pelayan web ini.',
'databasenotlocked'   => 'Pangkalan data tidak dikunci.',

# Move page
'move-page'               => 'Pindah $1',
'move-page-legend'        => 'Pindah laman',
'movepagetext'            => "Gunakan borang di bawah untuk menukar nama laman dan memindahkan semua maklumat sejarahnya ke nama baru. Tajuk yang lama akan dijadikan lencongan ke tajuk yang baru. Anda juga boleh mengemaskinikan semua lencongan yang menuju ke tajuk asal supaya menuju ke tajuk baru. Sebaliknya, anda boleh menyemak sekiranya terdapat [[Special:DoubleRedirects|lencongan berganda]] atau [[Special:BrokenRedirects|lencongan rosak]]. Anda bertanggungjawab memastikan semua pautan bersambung ke laman yang sepatutnya.

Sila ambil perhatian bahawa laman tersebut '''tidak''' akan dipindahkan sekiranya laman dengan tajuk yang baru tadi telah wujud, melainkan apabila
laman tersebut kosong atau merupakan laman lencongan dan tidak mempunyai sejarah penyuntingan. Ini bermakna anda boleh menukar semula nama sesebuah
laman kepada nama yang asal jika anda telah melakukan kesilapan, dan anda tidak boleh menulis ganti laman yang telah wujud.

'''AMARAN!''' Tindakan ini boleh menjadi perubahan yang tidak dijangka dan drastik bagi laman popular. Oleh itu, sila pastikan anda faham akibat yang mungkin timbul sebelum meneruskannya.",
'movepagetalktext'        => "Laman perbincangan yang berkaitan, jika ada, akan dipindahkan bersama-sama laman ini secara automatik '''kecuali''':
* Sebuah laman perbincangan dengan nama baru telah pun wujud, atau
* Anda membuang tanda kotak di bawah.

Dalam kes tersebut, anda terpaksa melencongkan atau menggabungkan laman secara manual, jika perlu.",
'movearticle'             => 'Pindah laman:',
'movenotallowed'          => 'Anda tidak mempunyai keizinan untuk memindahkan laman.',
'newtitle'                => 'Kepada tajuk baru:',
'move-watch'              => 'Pantau laman ini',
'movepagebtn'             => 'Pindah laman',
'pagemovedsub'            => 'Pemindahan berjaya',
'movepage-moved'          => '<big>\'\'\'"$1" telah dipindahkan ke "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Laman dengan nama tersebut telah pun wujud,
atau nama yang anda pilih tidak sah.
Sila pilih nama lain.',
'cantmove-titleprotected' => 'Anda tidak boleh memindah sebarang laman ke sini kerana tajuk ini telah dilindungi daripada dicipta',
'talkexists'              => "'''Laman tersebut berjaya dipindahkan, akan tetapi laman perbincangannya tidak dapat dipindahkan kerana laman dengan tajuk baru tersebut telah pun wujud. Anda perlu menggabungkannya secara manual.'''",
'movedto'                 => 'dipindahkan ke',
'movetalk'                => 'Pindahkan laman perbincangan yang berkaitan',
'move-subpages'           => 'Pindahkan semua sublaman sekali, jika boleh',
'move-talk-subpages'      => 'Pindahkan semua sublaman bagi laman perbincangan sekali, jika boleh',
'movepage-page-exists'    => 'Laman $1 telah pun wujud dan tidak boleh ditulis ganti secara automatik.',
'movepage-page-moved'     => 'Laman $1 telah dipindahkan ke $2.',
'movepage-page-unmoved'   => 'Laman $1 tidak dapat dipindahkan ke $2.',
'movepage-max-pages'      => 'Jumlah maksimum $1 laman telah dipindahkan secara automatik.',
'1movedto2'               => '[[$1]] dipindahkan ke [[$2]]',
'1movedto2_redir'         => '[[$1]] dipindahkan ke [[$2]] menerusi pelencongan',
'movelogpage'             => 'Log pemindahan',
'movelogpagetext'         => 'Berikut ialah senarai pemindahan laman.',
'movereason'              => 'Sebab:',
'revertmove'              => 'balik',
'delete_and_move'         => 'Hapus dan pindah',
'delete_and_move_text'    => '==Penghapusan diperlukan==

Laman destinasi "[[:$1]]" telah pun wujud. Adakah anda mahu menghapuskannya supaya laman ini dapat dipindahkan?',
'delete_and_move_confirm' => 'Ya, hapuskan laman ini',
'delete_and_move_reason'  => 'Dihapuskan supaya laman lain dapat dipindahkan',
'selfmove'                => 'Tajuk sumber dan tajuk destinasi tidak boleh sama.',
'immobile_namespace'      => 'Tajuk sumber atau destinasi adalah jenis khas. Anda tidak memindahkan laman ke luar atau dalam ruang nama tersebut.',
'imagenocrossnamespace'   => 'Tidak boleh memindah fail ke ruang nama lain',
'imagetypemismatch'       => 'Sambungan baru fail tersebut tidak sepadan dengan jenisnya',
'imageinvalidfilename'    => 'Nama fail imej sasaran tidak sah',
'fix-double-redirects'    => 'Kemas kinikan semua lencongan yang menuju ke tajuk asal',

# Export
'export'            => 'Eksport laman',
'exporttext'        => 'Anda boleh mengeksport teks dan sejarah suntingan untuk laman-laman tertentu yang ke dalam fail XML.
Fail ini boleh diimport ke dalam wiki lain yang menggunakan perisian MediaWiki melalui [[Special:Import|laman import]].

Untuk mengeksport laman, masukkan tajuk dalam kotak teks di bawah (satu tajuk bagi setiap baris) dan pilih sama ada anda mahukan semua versi dan catatan sejarah atau hanya versi semasa berserta maklumat mengenai suntingan terakhir.

Dalam pilihan kedua tadi, anda juga boleh menggunakan pautan, umpamanya [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] untuk laman "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Hanya eksport semakan semasa, bukan keseluruhan sejarah.',
'exportnohistory'   => "----
'''Catatan:''' Ciri eksport sejarah penuh laman melalui borang ini telah dimatikan atas sebab-sebab prestasi.",
'export-submit'     => 'Eksport',
'export-addcattext' => 'Tambah laman daripada kategori:',
'export-addcat'     => 'Tambah',
'export-download'   => 'Simpan sebagai fail',
'export-templates'  => 'Sertakan templat',

# Namespace 8 related
'allmessages'               => 'Pesanan sistem',
'allmessagesname'           => 'Nama',
'allmessagesdefault'        => 'Teks lalai',
'allmessagescurrent'        => 'Teks semasa',
'allmessagestext'           => 'Ini ialah senarai pesanan sistem yang terdapat dalam ruang nama MediaWiki.
Sila lawat [http://www.mediawiki.org/wiki/Localisation Penyetempatan MediaWiki] dan [http://translatewiki.net Betawiki] sekiranya anda mahu menyumbang dalam menyetempatkan dan menterjemah perisian MediaWiki.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' tidak boleh digunakan kerana '''\$wgUseDatabaseMessages''' dipadamkan.",
'allmessagesfilter'         => 'Tapis nama mesej:',
'allmessagesmodified'       => 'Hanya tunjukkan yang telah diubah',

# Thumbnails
'thumbnail-more'           => 'Besarkan',
'filemissing'              => 'Fail hilang',
'thumbnail_error'          => 'Berlaku ralat ketika mencipta imej ringkas: $1',
'djvu_page_error'          => 'Laman DjVu di luar julat',
'djvu_no_xml'              => 'Gagal mendapatkan data XML bagi fail DjVu',
'thumbnail_invalid_params' => 'Parameter imej ringkas tidak sah',
'thumbnail_dest_directory' => 'Direktori destinasi gagal diciptakans',

# Special:Import
'import'                     => 'Import laman',
'importinterwiki'            => 'Import transwiki',
'import-interwiki-text'      => 'Sila pilih wiki dan tajuk laman yang ingin diimport.
Semua tarikh semakan dan nama penyunting akan dikekalkan.
Semua tindakan import transwiki dicatatkan dalam [[Special:Log/import|log import]].',
'import-interwiki-history'   => 'Salin semua versi sejarah bagi laman ini',
'import-interwiki-submit'    => 'Import',
'import-interwiki-namespace' => 'Pindahkan laman ke dalam ruang nama:',
'importtext'                 => 'Sila eksport fail daripada sumber wiki menggunakan kemudahan Special:Export, simpan dalam komputer anda dan muat naik di sini.',
'importstart'                => 'Mengimport laman...',
'import-revision-count'      => '$1 semakan',
'importnopages'              => 'Tiada laman untuk diimport.',
'importfailed'               => 'Import gagal: $1',
'importunknownsource'        => 'Jenis sumber import tidak dikenali',
'importcantopen'             => 'Fail import tidak dapat dibuka',
'importbadinterwiki'         => 'Pautan antara wiki rosak',
'importnotext'               => 'Kosong atau tiada teks',
'importsuccess'              => 'Import selesai!',
'importhistoryconflict'      => 'Terdapat percanggahan semakan sejarah (mungkin laman ini pernah diimport sebelum ini)',
'importnosources'            => 'Tiada sumber import transwiki ditentunkan dan ciri muat naik sejarah secara terus dimatikan.',
'importnofile'               => 'Tiada fail import dimuat naik.',
'importuploaderrorsize'      => 'Fail import tidak dapat dimuat naik kerana melebihi had muat naik yang dibenarkan.',
'importuploaderrorpartial'   => 'Fail import tidak dapat dimuat naik kerana tidak dimuat naik sampai habis.',
'importuploaderrortemp'      => 'Fail import tidak dapat dimuat naik kerana tiada direktori sementara.',
'import-parse-failure'       => 'Gagal menghurai fail XML yang diimport',
'import-noarticle'           => 'Tiada laman untuk diimport!',
'import-nonewrevisions'      => 'Semua semakan telah pun diimport sebelum ini.',
'xml-error-string'           => '$1 pada baris $2, lajur $3 (bait $4): $5',
'import-upload'              => 'Muat naik data XML',

# Import log
'importlogpage'                    => 'Log import',
'importlogpagetext'                => 'Senarai tindakan import laman dengan keseluruhan sejarah suntingannya daripada wiki lain.',
'import-logentry-upload'           => 'mengimport [[$1]] dengan memuat naik fail',
'import-logentry-upload-detail'    => '$1 semakan',
'import-logentry-interwiki'        => '$1 dipindahkan ke wiki lain',
'import-logentry-interwiki-detail' => '$1 semakan daripada $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Laman pengguna saya',
'tooltip-pt-anonuserpage'         => 'Laman pengguna bagi alamat IP anda',
'tooltip-pt-mytalk'               => 'Laman perbincangan saya',
'tooltip-pt-anontalk'             => 'Perbincangan mengenai penyuntingan daripada alamat IP anda',
'tooltip-pt-preferences'          => 'Keutamaan saya',
'tooltip-pt-watchlist'            => 'Senarai laman yang anda pantau',
'tooltip-pt-mycontris'            => 'Senarai sumbangan saya',
'tooltip-pt-login'                => 'Walaupun tidak wajib, anda digalakkan supaya log masuk.',
'tooltip-pt-anonlogin'            => 'Walaupun tidak wajib, anda digalakkan supaya log masuk.',
'tooltip-pt-logout'               => 'Log keluar',
'tooltip-ca-talk'                 => 'Perbincangan mengenai laman kandungan',
'tooltip-ca-edit'                 => 'Anda boleh menyunting laman ini. Sila lihat pratonton terlebih dahulu sebelum menyimpan.',
'tooltip-ca-addsection'           => 'Tambah komen bagi perbincangan ini.',
'tooltip-ca-viewsource'           => 'Laman ini dilindungi. Anda boleh melihat sumbernya.',
'tooltip-ca-history'              => 'Versi-versi terdahulu bagi laman ini.',
'tooltip-ca-protect'              => 'Lindungi laman ini',
'tooltip-ca-delete'               => 'Hapuskan laman ini',
'tooltip-ca-undelete'             => 'Balikkan suntingan yang dilakukan kepada laman ini sebelum ia dihapuskan',
'tooltip-ca-move'                 => 'Pindahkan laman ini',
'tooltip-ca-watch'                => 'Tambahkan laman ini ke dalam senarai pantau anda',
'tooltip-ca-unwatch'              => 'Buang laman ini daripada senarai pantau anda',
'tooltip-search'                  => 'Cari dalam {{SITENAME}}',
'tooltip-search-go'               => 'Pergi ke laman dengan nama tepat ini, jika ada',
'tooltip-search-fulltext'         => 'Cari laman dengan teks ini',
'tooltip-p-logo'                  => 'Laman Utama',
'tooltip-n-mainpage'              => 'Kunjungi Laman Utama',
'tooltip-n-portal'                => 'Maklumat mengenai projek ini',
'tooltip-n-currentevents'         => 'Cari maklumat latar belakang mengenai peristiwa semasa',
'tooltip-n-recentchanges'         => 'Senarai perubahan terkini dalam wiki ini.',
'tooltip-n-randompage'            => 'Buka laman rawak',
'tooltip-n-help'                  => 'Tempat mencari jawapan.',
'tooltip-t-whatlinkshere'         => 'Senarai laman wiki yang mengandungi pautan ke laman ini',
'tooltip-t-recentchangeslinked'   => 'Perubahan terkini bagi semua laman yang dipaut dari laman ini',
'tooltip-feed-rss'                => 'Suapan RSS bagi laman ini',
'tooltip-feed-atom'               => 'Suapan Atom bagi laman ini',
'tooltip-t-contributions'         => 'Lihat senarai sumbangan pengguna ini',
'tooltip-t-emailuser'             => 'Kirim e-mel kepada pengguna ini',
'tooltip-t-upload'                => 'Muat naik imej atau fail media',
'tooltip-t-specialpages'          => 'Senarai laman khas',
'tooltip-t-print'                 => 'Versi boleh cetak bagi laman ini',
'tooltip-t-permalink'             => 'Pautan kekal ke versi ini',
'tooltip-ca-nstab-main'           => 'Lihat laman kandungan',
'tooltip-ca-nstab-user'           => 'Lihat laman pengguna',
'tooltip-ca-nstab-media'          => 'Lihat laman media',
'tooltip-ca-nstab-special'        => 'Ini adalah sebuah laman khas, anda tidak boleh menyunting laman ini secara terus.',
'tooltip-ca-nstab-project'        => 'Lihat laman projek',
'tooltip-ca-nstab-image'          => 'Lihat laman imej',
'tooltip-ca-nstab-mediawiki'      => 'Lihat pesanan sistem',
'tooltip-ca-nstab-template'       => 'Lihat templat',
'tooltip-ca-nstab-help'           => 'Lihat laman bantuan',
'tooltip-ca-nstab-category'       => 'Lihat laman kategori',
'tooltip-minoredit'               => 'Tandakan sebagai suntingan kecil',
'tooltip-save'                    => 'Simpan perubahan',
'tooltip-preview'                 => 'Pratonton perubahan yang anda lakukan, sila gunakan butang ini sebelum menyimpan!',
'tooltip-diff'                    => 'Tunjukkan perubahan yang anda telah lakukan kepada teks ini.',
'tooltip-compareselectedversions' => 'Lihat perbezaan antara dua versi laman ini yang dipilih.',
'tooltip-watch'                   => 'Tambahkan laman ini ke dalam senarai pantau anda',
'tooltip-recreate'                => 'Cipta semula laman ini walaupun ia telah dihapuskan',
'tooltip-upload'                  => 'Muat naik',

# Metadata
'nodublincore'      => 'Metadata RDF Dublin Core dipadamkan bagi pelayan ini.',
'nocreativecommons' => 'Metadata RDF Creative Commons RDF dipadamkan bagi pelayan ini.',
'notacceptable'     => 'Pelayan wiki ini tidak mampu menyediakan data dalam format yang boleh dibaca oleh pelanggan anda.',

# Attribution
'anonymous'        => 'Penguna {{SITENAME}} tanpa nama',
'siteuser'         => 'Pengguna {{SITENAME}}, $1',
'lastmodifiedatby' => 'Laman ini diubah buat kali terakhir pada $2, $1 oleh $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Berdasarkan karya $1.',
'others'           => 'lain-lain',
'siteusers'        => 'Pengguna-pengguna {{SITENAME}}, $1',
'creditspage'      => 'Penghargaan',
'nocredits'        => 'Tiada maklumat penghargaan bagi laman ini.',

# Spam protection
'spamprotectiontitle' => 'Penapis spam',
'spamprotectiontext'  => 'Laman yang anda ingin simpan telah dihalang oleh penapis spam. Hal ini mungkin disebabkan oleh pautan ke tapak web luar yang telah disenaraihitamkan.',
'spamprotectionmatch' => 'Teks berikut dikesan oleh penapis spam kami: $1',
'spambot_username'    => 'Pembersihan spam MediaWiki',
'spam_reverting'      => 'Membalikkan kepada versi terakhir yang tidak mengandungi pautan ke $1',
'spam_blanking'       => 'Mengosongkan semua semakan yang mengandungi pautan ke $1',

# Info page
'infosubtitle'   => 'Maklumat laman',
'numedits'       => 'Jumlah suntingan (laman): $1',
'numtalkedits'   => 'Jumlah suntingan (laman perbincangan): $1',
'numwatchers'    => 'Bilangan pemantau: $1',
'numauthors'     => 'Bilangan pengarang (page): $1',
'numtalkauthors' => 'Bilangan pengarang (laman perbincangan): $1',

# Math options
'mw_math_png'    => 'Sentiasa lakar PNG',
'mw_math_simple' => 'HTML jika ringkas, sebaliknya PNG',
'mw_math_html'   => 'HTML jika boleh, sebaliknya PNG',
'mw_math_source' => 'Biarkan sebagai TeX (untuk pelayar teks)',
'mw_math_modern' => 'Dicadangkan untuk pelayar moden',
'mw_math_mathml' => 'MathML jika boleh (sedang dalam uji kaji)',

# Patrolling
'markaspatrolleddiff'                 => 'Tanda ronda',
'markaspatrolledtext'                 => 'Tanda ronda laman ini',
'markedaspatrolled'                   => 'Tanda ronda',
'markedaspatrolledtext'               => 'Semakan tersebut telah ditanda ronda.',
'rcpatroldisabled'                    => 'Rondaan Perubahan Terkini dimatikan',
'rcpatroldisabledtext'                => 'Ciri Rondaan Perubahan Terkini dimatikan.',
'markedaspatrollederror'              => 'Tidak boleh menanda ronda',
'markedaspatrollederrortext'          => 'Anda perlu menyatakan semakan untuk ditanda ronda.',
'markedaspatrollederror-noautopatrol' => 'Anda tidak dibenarkan menanda ronda perubahan anda sendiri.',

# Patrol log
'patrol-log-page'   => 'Log pemeriksaan',
'patrol-log-header' => 'Berikut ialah log rondaan bagi semakan.',
'patrol-log-line'   => 'menandakan $1 bagi $2 sebagai telah diperiksa $3',
'patrol-log-auto'   => '(automatik)',
'patrol-log-diff'   => 's$1',

# Image deletion
'deletedrevision'                 => 'Menghapuskan semakan lama $1.',
'filedeleteerror-short'           => 'Ralat ketika menghapuskan fail: $1',
'filedeleteerror-long'            => 'Berlaku ralat ketika menghapuskan fail tersebut:

$1',
'filedelete-missing'              => 'Fail "$1" tidak boleh dihapuskan kerana ia tidak wujud.',
'filedelete-old-unregistered'     => 'Semakan fail "$1" tiada dalam pangkalan data.',
'filedelete-current-unregistered' => 'Fail "$1" tiada dalam pangkalan data.',
'filedelete-archive-read-only'    => 'Direktori arkib "$1" tidak boleh ditulis oleh pelayan web.',

# Browsing diffs
'previousdiff' => '← Suntingan sebelumnya',
'nextdiff'     => 'Suntingan berikutnya →',

# Media information
'mediawarning'         => "'''Amaran''': Fail ini boleh mengandungi kod yang berbahaya dan merosakkan komputer anda.<hr />",
'imagemaxsize'         => 'Had saiz imej di laman keterangannya:',
'thumbsize'            => 'Saiz imej ringkas:',
'widthheightpage'      => '$1×$2, $3 halaman',
'file-info'            => '(saiz file: $1, jenis MIME: $2)',
'file-info-size'       => '($1 × $2 piksel, saiz fail: $3, jenis MIME: $4)',
'file-nohires'         => '<small>Tiada leraian lebih besar.</small>',
'svg-long-desc'        => '(Fail SVG, ukuran dasar $1 × $2 piksel, saiz fail: $3)',
'show-big-image'       => 'Leraian penuh',
'show-big-image-thumb' => '<small>Saiz pratonton ini: $1 × $2 piksel</small>',

# Special:NewImages
'newimages'             => 'Galeri fail baru',
'imagelisttext'         => "Berikut ialah senarai bagi '''$1''' fail yang disusun secara $2.",
'newimages-summary'     => 'Laman khas ini memaparkan senarai fail muat naik terakhir.',
'showhidebots'          => '($1 bot)',
'noimages'              => 'Tiada imej.',
'ilsubmit'              => 'Cari',
'bydate'                => 'mengikut tarikh',
'sp-newimages-showfrom' => 'Tunjukkan imej baru bermula daripada $2, $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'hours-abbrev' => 'j',

# Bad image list
'bad_image_list' => 'Berikut adalah format yang digunakan:

Hanya item senarai (baris yang dimulakan dengan *) diambil kira. Pautan pertama pada sesebuah baris mestilah merupakan pautan ke sebuah imej rosak.
Sebarang pautan berikutnya pada baris yang sama dikira sebagai pengecualian (rencana yang dibenarkan disertakan imej).',

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => 'Fail ini mengandungi maklumat tambahan daripada kamera digital atau pengimbas yang digunakan untuk menghasilkannya. Jika fail ini telah diubah suai daripada rupa asalnya, beberapa butiran dalam maklumat ini mungkin sudah tidak relevan.',
'metadata-expand'   => 'Tunjukkan butiran penuh',
'metadata-collapse' => 'Sembunyikan butiran penuh',
'metadata-fields'   => 'Ruangan metadata EXIF yang disenaraikan dalam mesej ini
akan ditunjukkan pada laman imej apabila jadual metadata dikecilkan.
Ruangan lain akan disembunyikan.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Lebar',
'exif-imagelength'                 => 'Tinggi',
'exif-bitspersample'               => 'Bit sekomponen',
'exif-compression'                 => 'Skema pemampatan',
'exif-photometricinterpretation'   => 'Komposisi piksel',
'exif-orientation'                 => 'Haluan',
'exif-samplesperpixel'             => 'Bilangan komponen',
'exif-planarconfiguration'         => 'Penyusunan data',
'exif-ycbcrsubsampling'            => 'Nisbah subpensampelan Y kepada C',
'exif-ycbcrpositioning'            => 'Kedudukan Y dan C',
'exif-xresolution'                 => 'Leraian mengufuk',
'exif-yresolution'                 => 'Leraian menegak',
'exif-resolutionunit'              => 'Unit leraian X dan Y',
'exif-stripoffsets'                => 'Lokasi data imej',
'exif-rowsperstrip'                => 'Baris sejalur',
'exif-stripbytecounts'             => 'Bait sejalur termampat',
'exif-jpeginterchangeformat'       => 'Ofset ke SOI JPEG',
'exif-jpeginterchangeformatlength' => 'Jumlah bait bagi data JPEG',
'exif-transferfunction'            => 'Fungsi pindah',
'exif-whitepoint'                  => 'Kekromatan takat putih',
'exif-primarychromaticities'       => 'Kekromatan warna primer',
'exif-ycbcrcoefficients'           => 'Pekali matriks penukaran ruang warna',
'exif-referenceblackwhite'         => 'Nilai rujukan pasangan hitam dan putih',
'exif-datetime'                    => 'Tarikh dan waktu fail diubah',
'exif-imagedescription'            => 'Tajuk imej',
'exif-make'                        => 'Pengilang kamera',
'exif-model'                       => 'Model kamera',
'exif-software'                    => 'Perisian digunakan',
'exif-artist'                      => 'Artis',
'exif-copyright'                   => 'Pemegang hak cipta',
'exif-exifversion'                 => 'Versi exif',
'exif-flashpixversion'             => 'Versi Flashpix yang disokong',
'exif-colorspace'                  => 'Ruang warna',
'exif-componentsconfiguration'     => 'Maksud setiap komponen',
'exif-compressedbitsperpixel'      => 'Mod pemampatan imej',
'exif-pixelydimension'             => 'Lebar imej',
'exif-pixelxdimension'             => 'Tinggi imej',
'exif-makernote'                   => 'Catatan pengilang',
'exif-usercomment'                 => 'Komen pengguna',
'exif-relatedsoundfile'            => 'Fail audio berkaitan',
'exif-datetimeoriginal'            => 'Tarikh dan waktu penjanaan data',
'exif-datetimedigitized'           => 'Tarikh dan waktu pendigitan',
'exif-subsectime'                  => 'TarikhWaktu subsaat',
'exif-subsectimeoriginal'          => 'TarikhWaktuAsal subsaat',
'exif-subsectimedigitized'         => 'TarikhWaktuPendigitan subsaat',
'exif-exposuretime'                => 'Tempoh pendedahan',
'exif-exposuretime-format'         => '$1 saat ($2)',
'exif-fnumber'                     => 'Nombor F',
'exif-exposureprogram'             => 'Atur cara pendedahan',
'exif-spectralsensitivity'         => 'Kepekaan spektrum',
'exif-isospeedratings'             => 'Penilaian kelajuan ISO',
'exif-oecf'                        => 'Faktor penukaran optoelektronik',
'exif-shutterspeedvalue'           => 'Kelajuan pengatup',
'exif-aperturevalue'               => 'Bukaan',
'exif-brightnessvalue'             => 'Kecerahan',
'exif-exposurebiasvalue'           => 'Kecenderungan pendedahan',
'exif-maxaperturevalue'            => 'Bukaan tanah maksimum',
'exif-subjectdistance'             => 'Jarak subjek',
'exif-meteringmode'                => 'Mod permeteran',
'exif-lightsource'                 => 'Sumber cahaya',
'exif-flash'                       => 'Denyar',
'exif-focallength'                 => 'Panjang fokus kanta',
'exif-subjectarea'                 => 'Luas subjek',
'exif-flashenergy'                 => 'Tenaga denyar',
'exif-spatialfrequencyresponse'    => 'Sambutan frekuensi ruang',
'exif-focalplanexresolution'       => 'Leraian X satah fokus',
'exif-focalplaneyresolution'       => 'Leraian Y satah fokus',
'exif-focalplaneresolutionunit'    => 'Unit leraian satah fokus',
'exif-subjectlocation'             => 'Lokasi subjek',
'exif-exposureindex'               => 'Indeks pendedahan',
'exif-sensingmethod'               => 'Kaedah penderiaan',
'exif-filesource'                  => 'Sumber fail',
'exif-scenetype'                   => 'Jenis latar',
'exif-cfapattern'                  => 'Corak CFA',
'exif-customrendered'              => 'Pemprosesan imej tempahan',
'exif-exposuremode'                => 'Mod pendedahan',
'exif-whitebalance'                => 'Imbangan warna putih',
'exif-digitalzoomratio'            => 'Nisbah zum digital',
'exif-focallengthin35mmfilm'       => 'Panjang fokus dalam filem 35 mm',
'exif-scenecapturetype'            => 'Jenis penangkapan latar',
'exif-gaincontrol'                 => 'Kawalan latar',
'exif-contrast'                    => 'Kontras',
'exif-saturation'                  => 'Kepekatan',
'exif-sharpness'                   => 'Ketajaman',
'exif-devicesettingdescription'    => 'Huraian tetapan peranti',
'exif-subjectdistancerange'        => 'Julat jarak subjek',
'exif-imageuniqueid'               => 'ID unik imej',
'exif-gpsversionid'                => 'Versi label GPS',
'exif-gpslatituderef'              => 'Latitud utara atau selatan',
'exif-gpslatitude'                 => 'Latitud',
'exif-gpslongituderef'             => 'Logitud timur atau barat',
'exif-gpslongitude'                => 'Longitud',
'exif-gpsaltituderef'              => 'Rujukan ketinggian',
'exif-gpsaltitude'                 => 'Ketinggian',
'exif-gpstimestamp'                => 'Waktu GPS (jam atom)',
'exif-gpssatellites'               => 'Satelit yang digunakan untuk pengukuran',
'exif-gpsstatus'                   => 'Status penerima',
'exif-gpsmeasuremode'              => 'Mod pengukuran',
'exif-gpsdop'                      => 'Kepersisan pengukuran',
'exif-gpsspeedref'                 => 'Unit kelajuan',
'exif-gpsspeed'                    => 'Kelajuan penerima GPS',
'exif-gpstrackref'                 => 'Rujukan bagi arah pergerakan',
'exif-gpstrack'                    => 'Arah pergerakan',
'exif-gpsimgdirectionref'          => 'Rujukan bagi arah imej',
'exif-gpsimgdirection'             => 'Arah imej',
'exif-gpsmapdatum'                 => 'Data ukur geodesi yang digunakan',
'exif-gpsdestlatituderef'          => 'Rujukan bagi latitud destinasi',
'exif-gpsdestlatitude'             => 'Latitud destinasi',
'exif-gpsdestlongituderef'         => 'Rujukan bagi longitud destinasi',
'exif-gpsdestlongitude'            => 'Longitud destinasi',
'exif-gpsdestbearingref'           => 'Rujukan bagi bearing destinasi',
'exif-gpsdestbearing'              => 'Bearing destinasi',
'exif-gpsdestdistanceref'          => 'Rujukan bagi jarak destinasi',
'exif-gpsdestdistance'             => 'Jarak destinasi',
'exif-gpsprocessingmethod'         => 'Nama kaedah pemprosesan GPS',
'exif-gpsareainformation'          => 'Nama kawasan GPS',
'exif-gpsdatestamp'                => 'Tarikh GPS',
'exif-gpsdifferential'             => 'Pembetulan pembezaan GPS',

# EXIF attributes
'exif-compression-1' => 'Tidak dimampat',

'exif-unknowndate' => 'Tarikh tidak diketahui',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Dibalikkan secara mengufuk', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Diputar 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Dibalikkan secara menegak', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Diputarkan 90° melawan arah jam dan dibalikkan secara menegak', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Diputarkan 90° mengikut arah jam', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Diputarkan 90° mengikut arah jam dan dibalikkan secara menegak', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Diputarkan 90° melawan arah jam', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'format besar',
'exif-planarconfiguration-2' => 'format satah',

'exif-componentsconfiguration-0' => 'tiada',

'exif-exposureprogram-0' => 'Tidak ditentukan',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Atur cara normal',
'exif-exposureprogram-3' => 'Mengutamakan bukaan',
'exif-exposureprogram-4' => 'Mengutamakan pengatup',
'exif-exposureprogram-5' => 'Atur cara kreatif (cenderung kepada kedalaman lapangan)',
'exif-exposureprogram-6' => 'Atur cara aksi (cenderung kepada kelajuan pengatup yang tinggi)',
'exif-exposureprogram-7' => 'Mod potret (untuk foto jarak dekat dengan latar belakang kabur)',
'exif-exposureprogram-8' => 'Mod landskap (untuk foto landskap dengan latar belakang terfokus)',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0'   => 'Tidak diketahui',
'exif-meteringmode-1'   => 'Purata',
'exif-meteringmode-2'   => 'Purata cenderung ke pusat',
'exif-meteringmode-3'   => 'Titik',
'exif-meteringmode-4'   => 'Berbilang titik',
'exif-meteringmode-5'   => 'Corak',
'exif-meteringmode-6'   => 'Separa',
'exif-meteringmode-255' => 'Lain-lain',

'exif-lightsource-0'   => 'Tidak diketahui',
'exif-lightsource-1'   => 'Cahaya siang',
'exif-lightsource-2'   => 'Pendarfluor',
'exif-lightsource-3'   => 'Tungsten (lampu pijar)',
'exif-lightsource-4'   => 'Denyar',
'exif-lightsource-9'   => 'Cuaca cerah',
'exif-lightsource-10'  => 'Cuaca mendung',
'exif-lightsource-11'  => 'Gelap',
'exif-lightsource-12'  => 'Pendarfluor cahaya siang (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Pendarfluor putih siang (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Pendarfluor putih sejuk (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Pendarfluor putih (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Cahaya standard A',
'exif-lightsource-18'  => 'Cahaya standard B',
'exif-lightsource-19'  => 'Cahaya standard C',
'exif-lightsource-24'  => 'Tungsten studio ISO',
'exif-lightsource-255' => 'Sumber cahaya lain',

'exif-focalplaneresolutionunit-2' => 'inci',

'exif-sensingmethod-1' => 'Tidak ditentukan',
'exif-sensingmethod-2' => 'Penderia kawasan warna cip tunggal',
'exif-sensingmethod-3' => 'Penderia kawasan warna dwicip',
'exif-sensingmethod-4' => 'Penderia kawasan warna tricip',
'exif-sensingmethod-5' => 'Penderia kawasan warna berjujukan',
'exif-sensingmethod-7' => 'Penderia trilinear',
'exif-sensingmethod-8' => 'Penderia linear warna berjujukan',

'exif-scenetype-1' => 'Gambar yang diambil secara terus',

'exif-customrendered-0' => 'Proses biasa',
'exif-customrendered-1' => 'Proses tempahan',

'exif-exposuremode-0' => 'Pendedahan automatik',
'exif-exposuremode-1' => 'Pendedahan manual',
'exif-exposuremode-2' => 'Braket automatik',

'exif-whitebalance-0' => 'Imbangan warna putih automatik',
'exif-whitebalance-1' => 'Imbangan warna putih manual',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Landskap',
'exif-scenecapturetype-2' => 'Potret',
'exif-scenecapturetype-3' => 'Malam',

'exif-gaincontrol-0' => 'Tiada',
'exif-gaincontrol-1' => 'Gandaan rendah atas',
'exif-gaincontrol-2' => 'Gandaan tinggi atas',
'exif-gaincontrol-3' => 'Gandaan rendah bawah',
'exif-gaincontrol-4' => 'Gandaan tinggi bawah',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Lembut',
'exif-contrast-2' => 'Keras',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Kepekatan rendah',
'exif-saturation-2' => 'Kepekatan tinggi',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Lembut',
'exif-sharpness-2' => 'Keras',

'exif-subjectdistancerange-0' => 'Tidak diketahui',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Pandangan dekat',
'exif-subjectdistancerange-3' => 'Pandangan jauh',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitud utara',
'exif-gpslatitude-s' => 'Latitud selatan',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Longitud timur',
'exif-gpslongitude-w' => 'Longitud barat',

'exif-gpsstatus-a' => 'Pengukuran sedang dijalankan',
'exif-gpsstatus-v' => 'Interoperabiliti pengukuran',

'exif-gpsmeasuremode-2' => 'Pengukuran dua dimensi',
'exif-gpsmeasuremode-3' => 'Pengukuran tiga dimensi',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometer sejam',
'exif-gpsspeed-m' => 'Batu sejam',
'exif-gpsspeed-n' => 'Knot',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Arah benar',
'exif-gpsdirection-m' => 'Arah magnet',

# External editor support
'edit-externally'      => 'Sunting fail ini menggunakan perisian luar',
'edit-externally-help' => 'Lihat [http://www.mediawiki.org/wiki/Manual:External_editors arahan pemasangan] untuk maklumat lanjut.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'semua',
'imagelistall'     => 'semua',
'watchlistall2'    => 'semua',
'namespacesall'    => 'semua',
'monthsall'        => 'semua',

# E-mail address confirmation
'confirmemail'             => 'Sahkan alamat e-mel',
'confirmemail_noemail'     => 'Anda belum menetapkan alamat e-mel yang sah dalam [[Special:Preferences|laman keutamaan]] anda.',
'confirmemail_text'        => '{{SITENAME}} menghendaki supaya anda mengesahkan alamat e-mel anda sebelum menggunakan ciri-ciri e-mel.
Aktifkan butang di bawah untuk mengirim e-mel pengesahan kepada alamat e-mel
anda. E-mel tersebut akan mengandungi sebuah pautan yang mengandungi sebuah
kod; buka pautan tersebut di pelayar anda untuk mengesahkan bahawa alamat e-mel anda.',
'confirmemail_pending'     => '<div class="error">
Sebuah kod pengesahan telah pun di-e-melkan kepada anda. Jika anda baru sahaja
membuka akaun, sila tunggu kehadiran e-mel tersebut selama beberapa minit
sebelum meminta kod baru.
</div>',
'confirmemail_send'        => 'E-melkan kod pengesahan',
'confirmemail_sent'        => 'E-mel pengesahan dikirim.',
'confirmemail_oncreate'    => 'Sebuah kod pengesahan telah dikirm kepada alamat e-mel anda.
Kod ini tidak diperlukan untuk log masuk, akan tetapi anda perlu menyediakannya untuk
mengaktifkan ciri-ciri e-mel yang terdapat dalam wiki ini.',
'confirmemail_sendfailed'  => '{{SITENAME}} tidak dapat menghantar e-mel pengesahan anda. Sila semak alamat e-mel tersebut.

Pelayan mel memulangkan: $1',
'confirmemail_invalid'     => 'Kod pengesahan tidak sah. Kod tersebut mungkin sudah luput.',
'confirmemail_needlogin'   => 'Anda perlu $1 terlebih dahulu untuk mengesahkan alamat e-mel anda.',
'confirmemail_success'     => 'Alamat e-mel anda telah disahkan. Sekarang anda boleh melog masuk dan berseronok di wiki ini.',
'confirmemail_loggedin'    => 'Alamat e-mel anda telah disahkan.',
'confirmemail_error'       => 'Sesuatau yang tidak kena berlaku ketika kami menyimpan pengesahan anda.',
'confirmemail_subject'     => 'Pengesahan alamat e-mel di {{SITENAME}}',
'confirmemail_body'        => 'Seseorang, barangkali anda, dari alamat IP $1, telah mendaftarkan akaun "$2" dengan alamat e-mel ini di {{SITENAME}}.

Untuk mengesahkan bahawa akaun ini milik anda dan untuk mengaktifkan kemudahan e-mel di {{SITENAME}}, sila buka pautan ini dalam pelayar web anda:

$3

Jika anda tidak mendaftar di {{SITENAME}} (atau anda telah mendaftar menggunakan alamat e-mel lain), ikuti pautan ini untuk membatalkan pengesahan alamat e-mel:

$5

Kod pengesahan ini akan luput pada $4.',
'confirmemail_invalidated' => 'Pengesahan alamat e-mel telah dibatalkan',
'invalidateemail'          => 'Batalkan pengesahan e-mel',

# Scary transclusion
'scarytranscludedisabled' => '[Kemasukan pautan interwiki dimatikan]',
'scarytranscludefailed'   => '[Gagal mendapatkan templat $1]',
'scarytranscludetoolong'  => '[URL terlalu panjang]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Jejak balik bagi laman ini:<br />
$1
</div>',
'trackbackremove'   => ' ([Hapus $1])',
'trackbacklink'     => 'Jejak balik',
'trackbackdeleteok' => 'Jejak balik dihapuskan.',

# Delete conflict
'deletedwhileediting' => "'''Amaran''': Laman ini dihapuskan ketika anda sedang menyuntingnya!",
'confirmrecreate'     => "Pengguna [[User:$1|$1]] ([[User talk:$1|perbincangan]]) telah menghapuskan laman ini ketika anda sedang menyunting atas sebab berikut:
: ''$2''
Sila sahkan bahawa anda mahu mencipta semula laman ini.",
'recreate'            => 'Cipta semula',

# HTML dump
'redirectingto' => 'Melencong ke [[:$1]]...',

# action=purge
'confirm_purge'        => 'Kosongkan fail simpanan bagi laman ini?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Cari laman mengandungi ''$1''.",
'searchnamed'      => "Cari laman bernama ''$1''.",
'articletitles'    => "Laman bermula dengan ''$1''",
'hideresults'      => 'Sembunyikan keputusan',
'useajaxsearch'    => 'Gunakan carian AJAX',

# Multipage image navigation
'imgmultipageprev' => '← halaman sebelumnya',
'imgmultipagenext' => 'halaman berikutnya →',
'imgmultigo'       => 'Pergi!',
'imgmultigoto'     => 'Pergi ke halaman $1',

# Table pager
'ascending_abbrev'         => 'menaik',
'descending_abbrev'        => 'menurun',
'table_pager_next'         => 'Laman berikutnya',
'table_pager_prev'         => 'Laman sebelumnya',
'table_pager_first'        => 'Halaman pertama',
'table_pager_last'         => 'Halaman terakhir',
'table_pager_limit'        => 'Tunjukkan $1 item setiap halaman',
'table_pager_limit_submit' => 'Pergi',
'table_pager_empty'        => 'Tiada keputusan',

# Auto-summaries
'autosumm-blank'   => 'Membuang semua kandungan daripada laman',
'autosumm-replace' => "Menggantikan laman dengan '$1'",
'autoredircomment' => 'Melencong ke [[$1]]',
'autosumm-new'     => 'Laman baru: $1',

# Live preview
'livepreview-loading' => 'Memuat …',
'livepreview-ready'   => 'Memuat … Sedia!',
'livepreview-failed'  => 'Pratonton langsung gagal! Sila gunakan pratonton biasa.',
'livepreview-error'   => 'Gagal membuat sambungan: $1 "$2". Sila gunakan pratonton biasa.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Sebarang perubahan baru yang melebihi $1 saat mungkin tidak ditunjukkan dalam senarai ini.',
'lag-warn-high'   => 'Disebabkan oleh kelambatan pelayan pangkalan data, sebarang perubahan baru yang melebihi $1 saat mungkin tidak ditunjukkan dalam senarai ini.',

# Watchlist editor
'watchlistedit-numitems'       => 'Senarai pantau anda mengandungi $1 tajuk (tidak termasuk laman perbincangan).',
'watchlistedit-noitems'        => 'Tiada tajuk dalam senarai pantau anda.',
'watchlistedit-normal-title'   => 'Sunting senarai pantau',
'watchlistedit-normal-legend'  => 'Buang tajuk daripada senarai pantau',
'watchlistedit-normal-explain' => 'Berikut ialah tajuk-tajuk dalam senarai pantau anda. Untuk membuang mana-mana tajuk, tanda
kotak yang terletak di sebelahnya, dan klik Buang Tajuk. Anda juga boleh [[Special:Watchlist/raw|menyunting senarai mentah]].',
'watchlistedit-normal-submit'  => 'Buang Tajuk',
'watchlistedit-normal-done'    => '$1 tajuk dibuang daripada senarai pantau anda:',
'watchlistedit-raw-title'      => 'Sunting senarai pantau mentah',
'watchlistedit-raw-legend'     => 'Sunting senarai pantau mentah',
'watchlistedit-raw-explain'    => 'Berikut ialah tajuk-tajuk dalam senarai pantau anda. Anda boleh menyunting mana-mana tajuk
dengan menambah atau membuang daripada senarai tersebut, satu tajuk bagi setiap baris. Apabila selesai, klik Kemas Kini Senarai Pantau.
Anda juga boleh [[Special:Watchlist/edit|menggunakan penyunting standard]].',
'watchlistedit-raw-titles'     => 'Tajuk:',
'watchlistedit-raw-submit'     => 'Kemas Kini Senarai Pantau',
'watchlistedit-raw-done'       => 'Senarai pantau anda telah dikemaskinikan.',
'watchlistedit-raw-added'      => '$1 tajuk ditambah:',
'watchlistedit-raw-removed'    => '$1 tajuk telah dibuang:',

# Watchlist editing tools
'watchlisttools-view' => 'Lihat perubahan',
'watchlisttools-edit' => 'Sunting senarai pantau',
'watchlisttools-raw'  => 'Sunting senarai pantau mentah',

# Hijri month names
'hijri-calendar-m1'  => 'Muharam',
'hijri-calendar-m2'  => 'Safar',
'hijri-calendar-m3'  => 'Rabiulawal',
'hijri-calendar-m4'  => 'Rabiulakhir',
'hijri-calendar-m5'  => 'Jamadilawal',
'hijri-calendar-m6'  => 'Jamadilakhir',
'hijri-calendar-m7'  => 'Rejab',
'hijri-calendar-m8'  => 'Syaaban',
'hijri-calendar-m9'  => 'Ramadan',
'hijri-calendar-m10' => 'Syawal',
'hijri-calendar-m11' => 'Zulkaedah',
'hijri-calendar-m12' => 'Zulhijah',

# Core parser functions
'unknown_extension_tag' => 'Tag penyambung "$1" tidak dikenali',

# Special:Version
'version'                          => 'Versi', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Penyambung yang dipasang',
'version-specialpages'             => 'Laman khas',
'version-parserhooks'              => 'Penyangkuk penghurai',
'version-variables'                => 'Pemboleh ubah',
'version-other'                    => 'Lain-lain',
'version-mediahandlers'            => 'Pengelola media',
'version-hooks'                    => 'Penyangkuk',
'version-extension-functions'      => 'Fungsi penyambung',
'version-parser-extensiontags'     => 'Tag penyambung penghurai',
'version-parser-function-hooks'    => 'Penyangkuk fungsi penghurai',
'version-skin-extension-functions' => 'Fungsi penyangkuk rupa',
'version-hook-name'                => 'Nama penyangkuk',
'version-hook-subscribedby'        => 'Dilanggan oleh',
'version-version'                  => 'Versi',
'version-license'                  => 'Lesen',
'version-software'                 => 'Perisian yang dipasang',
'version-software-product'         => 'Produk',
'version-software-version'         => 'Versi',

# Special:FilePath
'filepath'         => 'Laluan fail',
'filepath-page'    => 'Fail:',
'filepath-submit'  => 'Laluan',
'filepath-summary' => 'Laman khas ini mengembalikan laluan penuh bagi sesebuah fail.
Imej ditunjuk dalam leraian penuh, jenis fail yang lain dibuka dengan atur cara yang berkenaan secara terus.

Sila masukkan nama fail tanpa awalan "{{ns:image}}:".',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Cari fail serupa',
'fileduplicatesearch-summary'  => 'Anda boleh mencari fail serupa berdasarkan nilai cincangannya.

Sila masukkan nama fail tanpa awalan "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'Cari fail serupa',
'fileduplicatesearch-filename' => 'Nama fail:',
'fileduplicatesearch-submit'   => 'Gelintar',
'fileduplicatesearch-info'     => '$1 × $2 piksel<br />Saiz fail: $3<br />Jenis MIME: $4',
'fileduplicatesearch-result-1' => 'Tiada fail yang serupa dengan "$1".',
'fileduplicatesearch-result-n' => 'Terdapat $2 fail yang serupa dengan "$1".',

# Special:SpecialPages
'specialpages'                   => 'Laman khas',
'specialpages-note'              => '----
* Laman khas biasa.
* <span class="mw-specialpagerestricted">Laman khas terhad.</span>',
'specialpages-group-maintenance' => 'Laporan penyenggaraan',
'specialpages-group-other'       => 'Laman khas lain',
'specialpages-group-login'       => 'Log masuk / daftar',
'specialpages-group-changes'     => 'Perubahan terkini dan log',
'specialpages-group-media'       => 'Laporan media dan muat naik',
'specialpages-group-users'       => 'Pengguna dan hak',
'specialpages-group-highuse'     => 'Laman popular',
'specialpages-group-pages'       => 'Senarai laman',
'specialpages-group-pagetools'   => 'Alatan laman',
'specialpages-group-wiki'        => 'Data dan alatan wiki',
'specialpages-group-redirects'   => 'Laman khas yang melencong',
'specialpages-group-spam'        => 'Alatan spam',

# Special:BlankPage
'blankpage'              => 'Laman kosong',
'intentionallyblankpage' => 'Laman ini sengaja dibiarkan kosong dan digunakan untuk kerja-kerja ujian dan sebagainya.',

);
