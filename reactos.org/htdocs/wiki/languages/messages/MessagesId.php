<?php
/** Indonesian (Bahasa Indonesia)
 *
 * @ingroup Language
 * @file
 *
 * @author Borgx
 * @author Irwangatot
 * @author IvanLanin
 * @author Meursault2004
 * @author Remember the dot
 * @author Rex
 * @author לערי ריינהארט
 */

/**
 * Transform table for decimal point '.' and thousands separator ','
 */
$separatorTransformTable = array(',' => '.', '.' => ',' );

/**
 * Namespace names. NS_PROJECT is always set to $wgMetaNamespace after the
 * settings are loaded, it will be ignored even if you specify it here.
 */
$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Istimewa',
	NS_MAIN             => '',
	NS_TALK             => 'Pembicaraan',
	NS_USER             => 'Pengguna',
	NS_USER_TALK        => 'Pembicaraan_Pengguna',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Pembicaraan_$1',
	NS_IMAGE            => 'Berkas',
	NS_IMAGE_TALK       => 'Pembicaraan_Berkas',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Pembicaraan_MediaWiki',
	NS_TEMPLATE         => 'Templat',
	NS_TEMPLATE_TALK    => 'Pembicaraan_Templat',
	NS_HELP             => 'Bantuan',
	NS_HELP_TALK        => 'Pembicaraan_Bantuan',
	NS_CATEGORY         => 'Kategori',
	NS_CATEGORY_TALK    => 'Pembicaraan_Kategori'
);

/**
 * Array of namespace aliases, mapping from name to NS_xxx index
 */
$namespaceAliases = array(
	'Gambar_Pembicaraan'    => NS_IMAGE_TALK,
	'MediaWiki_Pembicaraan' => NS_MEDIAWIKI_TALK,
	'Templat_Pembicaraan'   => NS_TEMPLATE_TALK,
	'Bantuan_Pembicaraan'   => NS_HELP_TALK,
	'Kategori_Pembicaraan'  => NS_CATEGORY_TALK,
	'Gambar'                => NS_IMAGE,
	'Pembicaraan_Gambar'    => NS_IMAGE_TALK,
	'Bicara'                => NS_TALK,
	'Bicara_Pengguna'       => NS_USER_TALK,
);

/**
 * Skin names. If any key is not specified, the English one will be used.
 */
$skinNames = array(
	'standard' => 'Klasik',
	'simple'   => 'Sederhana',
);

/**
 * Default list of book sources
 */
$bookstoreList = array(
	'AddALL' => 'http://www.addall.com/New/Partner.cgi?query=$1&type=ISBN',
	'Amazon.com' => 'http://www.amazon.com/exec/obidos/ISBN=$1',
	'Barnes & Noble' => 'http://search.barnesandnoble.com/bookSearch/isbnInquiry.asp?isbn=$1',
	'Bhinneka.com bookstore' => 'http://www.bhinneka.com/Buku/Engine/search.asp?fisbn=$1',
	'Gramedia Cyberstore (via Google)' => 'http://www.google.com/search?q=%22ISBN+:+$1%22+%22product_detail%22+site:www.gramediacyberstore.com+OR+site:www.gramediaonline.com+OR+site:www.kompas.com&hl=id',
	'PriceSCAN' => 'http://www.pricescan.com/books/bookDetail.asp?isbn=$1',
);

/**
 * Magic words
 * Customisable syntax for wikitext and elsewhere
 */
$magicWords = array(
#   ID                           CASE  SYNONYMS
	'redirect'            => array( 0, '#ALIH',                    '#REDIRECT'              ),
	'notoc'               => array( 0, '__TANPADAFTARISI__',       '__NOTOC__'              ),
	'nogallery'           => array( 0, '__TANPAGALERI__',          '__NOGALLERY__'          ),
	'forcetoc'            => array( 0, '__PAKSADAFTARISI__',       '__FORCETOC__'           ),
	'toc'                 => array( 0, '__DAFTARISI__',            '__TOC__'                ),
	'noeditsection'       => array( 0, '__TANPASUNTINGANBAGIAN__', '__NOEDITSECTION__'      ),
	'currentmonth'        => array( 1, 'BULANKINI',                'CURRENTMONTH'           ),
	'currentmonthname'    => array( 1, 'NAMABULANKINI',            'CURRENTMONTHNAME'       ),
	'currentmonthnamegen' => array( 1, 'NAMASINGKATBULANKINI',     'CURRENTMONTHNAMEGEN'    ),
	'currentmonthabbrev'  => array( 1, 'BULANINISINGKAT',          'CURRENTMONTHABBREV'     ),
	'currentday'          => array( 1, 'HARIKINI',                 'CURRENTDAY'             ),
	'currentday2'         => array( 1, 'HARIKINI2',                'CURRENTDAY2'            ),
	'currentdayname'      => array( 1, 'NAMAHARIKINI',             'CURRENTDAYNAME'         ),
	'currentyear'         => array( 1, 'TAHUNKINI',                'CURRENTYEAR'            ),
	'currenttime'         => array( 1, 'WAKTUKINI',                'CURRENTTIME'            ),
	'currenthour'         => array( 1, 'JAMKINI',                  'CURRENTHOUR'            ),
	'localmonth'          => array( 1, 'BULANLOKAL',               'LOCALMONTH'             ),
	'localmonthname'      => array( 1, 'NAMABULANLOKAL',           'LOCALMONTHNAME'         ),
	'localmonthnamegen'   => array( 1, 'NAMAJENDERBULANLOKAL',     'LOCALMONTHNAMEGEN'      ),
	'localmonthabbrev'    => array( 1, 'NAMASINGKATBULANLOKAL',    'LOCALMONTHABBREV'       ),
	'localday'            => array( 1, 'HARILOKAL',                'LOCALDAY'               ),
	'localday2'           => array( 1, 'HARILOKAL2',               'LOCALDAY2'              ),
	'localdayname'        => array( 1, 'HARILOKAL',                'LOCALDAYNAME'           ),
	'localyear'           => array( 1, 'TAHUNLOKAL',               'LOCALYEAR'              ),
	'localtime'           => array( 1, 'WAKTULOKAL',               'LOCALTIME'              ),
	'localhour'           => array( 1, 'JAMLOKAL',                 'LOCALHOUR'              ),
	'numberofpages'       => array( 1, 'JUMLAHHALAMAN',            'NUMBEROFPAGES'          ),
	'numberofarticles'    => array( 1, 'JUMLAHARTIKEL',            'NUMBEROFARTICLES'       ),
	'numberoffiles'       => array( 1, 'JUMLAHBERKAS',             'NUMBEROFFILES'          ),
	'numberofusers'       => array( 1, 'JUMLAHPENGGUNA',           'NUMBEROFUSERS'          ),
	'numberofedits'       => array( 1, 'JUMLAHSUNTINGAN',          'NUMBEROFEDITS'          ),
	'pagename'            => array( 1, 'NAMAHALAMAN',              'PAGENAME'               ),
	'pagenamee'           => array( 1, 'NAMAHALAMANE',             'PAGENAMEE'              ),
	'namespace'           => array( 1, 'RUANGNAMA',                'NAMESPACE'              ),
	'namespacee'          => array( 1, 'RUANGNAMAE',               'NAMESPACEE'             ),
	'talkspace'           => array( 1, 'RUANGBICARA',              'TALKSPACE'              ),
	'talkspacee'          => array( 1, 'RUANGBICARAE',             'TALKSPACEE'              ),
	'subjectspace'        => array( 1, 'RUANGUTAMA',               'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'       => array( 1, 'RUANGUTAMAE',              'SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'        => array( 1, 'NAMALENGKAPHALAMAN',       'FULLPAGENAME'           ),
	'fullpagenamee'       => array( 1, 'NAMALENGKAPHALAMANE',      'FULLPAGENAMEE'          ),
	'subpagename'         => array( 1, 'NAMASUBHALAMAN',           'SUBPAGENAME'            ),
	'subpagenamee'        => array( 1, 'NAMASUBHALAMANE',          'SUBPAGENAMEE'           ),
	'basepagename'        => array( 1, 'NAMADASARHALAMAN',         'BASEPAGENAME'           ),
	'basepagenamee'       => array( 1, 'NAMADASARHALAMANE',        'BASEPAGENAMEE'          ),
	'talkpagename'        => array( 1, 'NAMAHALAMANBICARA',        'TALKPAGENAME'           ),
	'talkpagenamee'       => array( 1, 'NAMAHALAMANBICARAE',       'TALKPAGENAMEE'          ),
	'subjectpagename'     => array( 1, 'NAMAHALAMANARTIKEL',       'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'    => array( 1, 'NAMAHALAMANARTIKELE',      'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                 => array( 0, 'PESAN:',                   'MSG:'                   ),
	'subst'               => array( 0, 'GANTI:',                   'SUBST:'                 ),
	'img_right'           => array( 1, 'kanan',                    'right'                  ),
	'img_left'            => array( 1, 'kiri',                     'left'                   ),
	'img_none'            => array( 1, 'tanpa',                    'none'                   ),
	'img_center'          => array( 1, 'tengah',                   'center', 'centre'       ),
	'img_framed'          => array( 1, 'bingkai',                  'framed', 'enframed', 'frame' ),
	'img_frameless'       => array( 1, 'tanpabingkai',             'frameless'              ),
	'img_page'            => array( 1, 'halaman=$1',               'page=$1', 'page $1'     ),
	'img_upright'         => array( 1, 'tegak', 'tegak=$1', 'tegak $1', 'upright', 'upright=$1', 'upright $1'  ),
	'img_border'          => array( 1, 'batas',                    'border'                 ),
	'img_top'             => array( 1, 'atas',                     'top'                    ),
	'img_text_top'        => array( 1, 'atas-teks',                'text-top'               ),
	'img_middle'          => array( 1, 'tengah',                   'middle'                 ),
	'img_bottom'          => array( 1, 'bawah',                    'bottom'                 ),
	'img_text_bottom'     => array( 1, 'bawah-teks',               'text-bottom'            ),
	'sitename'            => array( 1, 'NAMASITUS',                'SITENAME'               ),
	'ns'                  => array( 0, 'RN:',                      'NS:'                    ),
	'localurl'            => array( 0, 'URLLOKAL',                 'LOCALURL:'              ),
	'localurle'           => array( 0, 'URLLOKALE',                'LOCALURLE:'             ),
	'servername'          => array( 0, 'NAMASERVER',               'SERVERNAME'             ),
	'scriptpath'          => array( 0, 'LOKASISKRIP',              'SCRIPTPATH'             ),
	'grammar'             => array( 0, 'TATABAHASA',               'GRAMMAR:'               ),
	'notitleconvert'      => array( 0, '__TANPAKONVERSIJUDUL__',   '__NOTITLECONVERT__', '__NOTC__'),
	'nocontentconvert'    => array( 0, '__TANPAKONVERSIISI__',     '__NOCONTENTCONVERT__', '__NOCC__'),
	'currentweek'         => array( 1, 'MINGGUKINI',               'CURRENTWEEK'            ),
	'currentdow'          => array( 1, 'HARIDALAMMINGGU',          'CURRENTDOW'             ),
	'localweek'           => array( 1, 'MINGGULOKAL',              'LOCALWEEK'              ),
	'localdow'            => array( 1, 'HARIDALAMMINGGULOKAL',     'LOCALDOW'               ),
	'revisionid'          => array( 1, 'IDREVISI',                 'REVISIONID'             ),
	'revisionday'         => array( 1, 'HARIREVISI',               'REVISIONDAY'            ),
	'revisionday2'        => array( 1, 'HARIREVISI2',              'REVISIONDAY2'           ),
	'revisionmonth'       => array( 1, 'BULANREVISI',              'REVISIONMONTH'          ),
	'revisionyear'        => array( 1, 'TAHUNREVISI',              'REVISIONYEAR'           ),
	'revisiontimestamp'   => array( 1, 'REKAMWAKTUREVISI',         'REVISIONTIMESTAMP'      ),
	'plural'              => array( 0, 'JAMAK:',                   'PLURAL:'                ),
	'fullurl'             => array( 0, 'URLLENGKAP:',              'FULLURL:'               ),
	'fullurle'            => array( 0, 'URLLENGKAPE',              'FULLURLE:'              ),
	'lcfirst'             => array( 0, 'AWALKECIL:',               'LCFIRST:'               ),
	'ucfirst'             => array( 0, 'AWALBESAR:',               'UCFIRST:'               ),
	'lc'                  => array( 0, 'KECIL:',                   'LC:'                    ),
	'uc'                  => array( 0, 'BESAR:',                   'UC:'                    ),
	'raw'                 => array( 0, 'MENTAH:',                  'RAW:'                   ),
	'displaytitle'        => array( 1, 'JUDULTAMPILAN',            'DISPLAYTITLE'           ),
	'rawsuffix'           => array( 1, 'M',                        'R'                      ),
	'newsectionlink'      => array( 1, '__PRANALABAGIANBARU__',    '__NEWSECTIONLINK__'     ),
	'currentversion'      => array( 1, 'VERSIKINI',                'CURRENTVERSION'         ),
	'urlencode'           => array( 0, 'KODEURL:',                 'URLENCODE:'             ),
	'anchorencode'        => array( 0, 'KODEJANGKAR',              'ANCHORENCODE'           ),
	'currenttimestamp'    => array( 1, 'STEMPELWAKTUKINI',         'CURRENTTIMESTAMP'       ),
	'localtimestamp'      => array( 1, 'STEMPELWAKTULOKAL',        'LOCALTIMESTAMP'         ),
	'directionmark'       => array( 1, 'MARKAARAH',                'DIRECTIONMARK', 'DIRMARK' ),
	'language'            => array( 0, '#BAHASA:',                 '#LANGUAGE:'             ),
	'contentlanguage'     => array( 1, 'BAHASAISI',                'CONTENTLANGUAGE', 'CONTENTLANG' ),
	'pagesinnamespace'    => array( 1, 'HALAMANDIRUANGNAMA:',      'PAGESINNAMESPACE:', 'PAGESINNS:' ),
	'numberofadmins'      => array( 1, 'JUMLAHPENGURUS',           'NUMBEROFADMINS'         ),
	'formatnum'           => array( 0, 'FORMATANGKA',              'FORMATNUM'              ),
	'padleft'             => array( 0, 'ISIKIRI',                  'PADLEFT'                ),
	'padright'            => array( 0, 'ISIKANAN',                 'PADRIGHT'               ),
	'special'             => array( 0, 'istimewa',                 'special',               ),
	'defaultsort'         => array( 1, 'URUTANBAKU:',              'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:' ),
);

/**
 * Alternate names of special pages. All names are case-insensitive. The first
 * listed alias will be used as the default.
 */
$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Pengalihan_ganda', 'Pengalihanganda' ),
	'BrokenRedirects'           => array( 'Pengalihan_rusak', 'Pengalihanrusak' ),
	'Disambiguations'           => array( 'Disambiguasi' ),
	'Userlogin'                 => array( 'Masuk_log', 'Masuklog' ),
	'Userlogout'                => array( 'Keluar_log', 'Keluarlog' ),
	'Preferences'               => array( 'Preferensi' ),
	'Watchlist'                 => array( 'Daftar_pantauan', 'Daftarpantauan' ),
	'Recentchanges'             => array( 'Perubahan_terbaru', 'Perubahanterbaru' ),
	'Upload'                    => array( 'Pemuatan' ),
	'Imagelist'                 => array( 'Daftar_berkas', 'Daftarberkas' ),
	'Newimages'                 => array( 'Berkas_baru', 'Berkasbaru' ),
	'Listusers'                 => array( 'Daftar_pengguna', 'Daftarpengguna' ),
	'Statistics'                => array( 'Statistik' ),
	'Randompage'                => array( 'Halaman_sembarang', 'Halamansembarang' ),
	'Lonelypages'               => array( 'Halaman_tak_bertuan', 'Halamantakbertuan' ),
	'Uncategorizedpages'        => array( 'Halamantakterkategori' ),
	'Uncategorizedcategories'   => array( 'Kategoritakterkategori' ),
	'Uncategorizedimages'       => array( 'Berkastakterkategori' ),
	'Uncategorizedtemplates'    => array( 'Templattakterkategori' ),
	'Unusedcategories'          => array( 'Kategoritakdigunakan' ),
	'Unusedimages'              => array( 'Berkastakdigunakan' ),
	'Wantedpages'               => array( 'Halamandiinginkan' ),
	'Wantedcategories'          => array( 'Kategoridiinginkan' ),
	'Mostlinked'                => array( 'Palingdituju' ),
	'Mostlinkedcategories'      => array( 'Kategoripalingdigunakan' ),
	'Mostlinkedtemplates'       => array( 'Templatpalingdigunakan' ),
	'Mostcategories'            => array( 'Kategoriterbanyak' ),
	'Mostimages'                => array( 'Berkastersering' ),
	'Mostrevisions'             => array( 'Perubahanterbanyak' ),
	'Fewestrevisions'           => array( 'Perubahantersedikit' ),
	'Shortpages'                => array( 'Halaman_pendek', 'Halamanpendek' ),
	'Longpages'                 => array( 'Halaman_panjang', 'Halamanpanjang' ),
	'Newpages'                  => array( 'Halaman_baru', 'Halamanbaru' ),
	'Ancientpages'              => array( 'Artikel_lama', 'Artikeltertua' ),
	'Deadendpages'              => array( 'Halaman_buntu', 'Halamanbuntu' ),
	'Protectedpages'            => array( 'Halamandilindungi' ),
	'Protectedtitles'           => array( 'Judulyangdilindungi' ),
	'Allpages'                  => array( 'Daftar_halaman', 'Daftarhalaman' ),
	'Prefixindex'               => array( 'Indeksawalan' ),
	'Ipblocklist'               => array( 'Daftar_pemblokiran', 'Daftarblokirip' ),
	'Specialpages'              => array( 'Halaman_istimewa', 'Halamanistimewa' ),
	'Contributions'             => array( 'Kontribusi_pengguna', 'Kontribusi' ),
	'Emailuser'                 => array( 'Suratepengguna' ),
	'Confirmemail'              => array( 'Konfirmasi_surat_e', 'konfirmasisurate' ),
	'Whatlinkshere'             => array( 'Pranala_balik', 'Pranalabalik' ),
	'Recentchangeslinked'       => array( 'Perubahan_terkait', 'Perubahanterkait' ),
	'Movepage'                  => array( 'Pindahkan_halaman', 'Pindahkanhalaman' ),
	'Blockme'                   => array( 'Blokirsaya' ),
	'Booksources'               => array( 'Sumber_buku', 'Sumberbuku' ),
	'Categories'                => array( 'Daftar_kategori', 'Kategori' ),
	'Export'                    => array( 'Ekspor' ),
	'Version'                   => array( 'Versi' ),
	'Allmessages'               => array( 'Pesan_sistem', 'Pesansistem' ),
	'Log'                       => array( 'Log' ),
	'Blockip'                   => array( 'Blokir_pengguna', 'Blokirip' ),
	'Undelete'                  => array( 'Pembatalan_penghapusan', 'Batalhapus' ),
	'Import'                    => array( 'Impor' ),
	'Lockdb'                    => array( 'Kuncidb' ),
	'Unlockdb'                  => array( 'Bukakuncidb' ),
	'Userrights'                => array( 'Hakpengguna' ),
	'MIMEsearch'                => array( 'Pencarian_MIME', 'CariMIME' ),
	'FileDuplicateSearch'       => array( 'PencarianDuplikatBerkas' ),
	'Unwatchedpages'            => array( 'Halamantakdipantau' ),
	'Listredirects'             => array( 'Daftar_pengalihan', 'Daftarpengalihan' ),
	'Revisiondelete'            => array( 'Hapusrevisi' ),
	'Unusedtemplates'           => array( 'Templattakdigunakan' ),
	'Randomredirect'            => array( 'Pengalihan_sembarang', 'Pengalihansembarang' ),
	'Mypage'                    => array( 'Halamansaya' ),
	'Mytalk'                    => array( 'Pembicaraansaya' ),
	'Mycontributions'           => array( 'Kontribusisaya' ),
	'Listadmins'                => array( 'Daftar_pengurus', 'Daftarpengurus' ),
	'Listbots'                  => array( 'Daftar_bot', 'Daftarbot' ),
	'Popularpages'              => array( 'Halaman_populer', 'Halamanpopuler' ),
	'Search'                    => array( 'Pencarian', 'Cari' ),
	'Resetpass'                 => array( 'Resetpass' ),
	'Withoutinterwiki'          => array( 'Tanpa_interwiki', 'Tanpainterwiki' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Garis bawahi pranala:',
'tog-highlightbroken'         => 'Format pranala patah <a href="" class="new">seperti ini</a> (pilihan: seperti ini<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Ratakan paragraf',
'tog-hideminor'               => 'Sembunyikan suntingan kecil di perubahan terbaru',
'tog-extendwatchlist'         => 'Tampilkan daftar pantauan yang menunjukkan semua perubahan',
'tog-usenewrc'                => 'Tampilan perubahan terbaru alternatif (JavaScript)',
'tog-numberheadings'          => 'Beri nomor judul secara otomatis',
'tog-showtoolbar'             => 'Perlihatkan <em>toolbar</em> (batang alat) penyuntingan',
'tog-editondblclick'          => 'Sunting halaman dengan klik ganda (JavaScript)',
'tog-editsection'             => 'Fungsikan penyuntingan sub-bagian melalui pranala [sunting]',
'tog-editsectiononrightclick' => 'Fungsikan penyuntingan sub-bagian dengan klik-kanan pada judul bagian (JavaScript)',
'tog-showtoc'                 => 'Perlihatkan daftar isi (untuk halaman yang mempunyai lebih dari 3 sub-bagian)',
'tog-rememberpassword'        => 'Ingat kata sandi pada setiap sesi',
'tog-editwidth'               => 'Kotak sunting berukuran maksimum',
'tog-watchcreations'          => 'Tambahkan halaman yang saya buat ke daftar pantauan',
'tog-watchdefault'            => 'Tambahkan halaman yang saya sunting ke daftar pantauan',
'tog-watchmoves'              => 'Tambahkan halaman yang saya pindahkan ke daftar pantauan',
'tog-watchdeletion'           => 'Tambahkan halaman yang saya hapus ke daftar pantauan',
'tog-minordefault'            => 'Tandai semua suntingan sebagai suntingan kecil secara baku',
'tog-previewontop'            => 'Perlihatkan pratayang sebelum kotak sunting dan tidak sesudahnya',
'tog-previewonfirst'          => 'Perlihatkan pratayang pada suntingan pertama',
'tog-nocache'                 => 'Matikan <em>cache</em> halaman',
'tog-enotifwatchlistpages'    => 'Surat-e saya jika suatu halaman yang saya pantau berubah',
'tog-enotifusertalkpages'     => 'Surat-e saya jika halaman pembicaraan saya berubah',
'tog-enotifminoredits'        => 'Surat-e saya juga pada perubahan kecil',
'tog-enotifrevealaddr'        => 'Berikan surat-e saya pada surat notifikasi',
'tog-shownumberswatching'     => 'Tunjukkan jumlah pemantau',
'tog-fancysig'                => 'Tanda tangan mentah (tanpa pranala otomatis)',
'tog-externaleditor'          => 'Gunakan perangkat lunak pengolah kata luar',
'tog-externaldiff'            => 'Gunakan perangkat lunak luar untuk melihat perbedaan suntingan',
'tog-showjumplinks'           => 'Aktifkan pranala pembantu "langsung ke"',
'tog-uselivepreview'          => 'Gunakan pratayang langsung (JavaScript) (eksperimental)',
'tog-forceeditsummary'        => 'Ingatkan saya bila kotak ringkasan suntingan masih kosong',
'tog-watchlisthideown'        => 'Sembunyikan suntingan saya di daftar pantauan',
'tog-watchlisthidebots'       => 'Sembunyikan suntingan bot di daftar pantauan',
'tog-watchlisthideminor'      => 'Sembunyikan suntingan kecil di daftar pantauan',
'tog-nolangconversion'        => 'Matikan konversi varian',
'tog-ccmeonemails'            => 'Kirimkan saya salinan surat-e yang saya kirimkan ke orang lain',
'tog-diffonly'                => 'Jangan tampilkan isi halaman di bawah perbedaan suntingan',
'tog-showhiddencats'          => 'Tampilkan kategori tersembunyi',

'underline-always'  => 'Selalu',
'underline-never'   => 'Tidak',
'underline-default' => 'Sesuai konfigurasi penjelajah web',

'skinpreview' => '(Pratayang)',

# Dates
'sunday'        => 'Minggu',
'monday'        => 'Senin',
'tuesday'       => 'Selasa',
'wednesday'     => 'Rabu',
'thursday'      => 'Kamis',
'friday'        => 'Jumat',
'saturday'      => 'Sabtu',
'sun'           => 'Min',
'mon'           => 'Sen',
'tue'           => 'Sel',
'wed'           => 'Rab',
'thu'           => 'Kam',
'fri'           => 'Jum',
'sat'           => 'Sab',
'january'       => 'Januari',
'february'      => 'Februari',
'march'         => 'Maret',
'april'         => 'April',
'may_long'      => 'Mei',
'june'          => 'Juni',
'july'          => 'Juli',
'august'        => 'Agustus',
'september'     => 'September',
'october'       => 'Oktober',
'november'      => 'November',
'december'      => 'Desember',
'january-gen'   => 'Januari',
'february-gen'  => 'Februari',
'march-gen'     => 'Maret',
'april-gen'     => 'April',
'may-gen'       => 'Mei',
'june-gen'      => 'Juni',
'july-gen'      => 'Juli',
'august-gen'    => 'Agustus',
'september-gen' => 'September',
'october-gen'   => 'Oktober',
'november-gen'  => 'November',
'december-gen'  => 'Desember',
'jan'           => 'Jan',
'feb'           => 'Feb',
'mar'           => 'Mar',
'apr'           => 'Apr',
'may'           => 'Mei',
'jun'           => 'Jun',
'jul'           => 'Jul',
'aug'           => 'Agu',
'sep'           => 'Sep',
'oct'           => 'Okt',
'nov'           => 'Nov',
'dec'           => 'Des',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategori|Kategori}}',
'category_header'                => 'Artikel dalam kategori "$1"',
'subcategories'                  => 'Subkategori',
'category-media-header'          => 'Media dalam kategori "$1"',
'category-empty'                 => "''Tidak terdapat artikel maupun media dalam kategori ini.''",
'hidden-categories'              => '{{PLURAL:$1|Kategori tersembunyi|Kategori tersembunyi}}',
'hidden-category-category'       => 'Kategori tersembunyi', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Kategori ini hanya memiliki satu subkategori berikut.|Kategori ini memiliki {{PLURAL:$1|subkategori|$1 subkategori}} berikut, dari total $2.}}',
'category-subcat-count-limited'  => 'Kategori ini memiliki {{PLURAL:$1|subkategori|$1 subkategori}} berikut.',
'category-article-count'         => '{{PLURAL:$2|Kategori ini hanya memiliki satu halaman berikut.|Kategori ini memiliki {{PLURAL:$1|halaman|$1 halaman}}, dari total $2.}}',
'category-article-count-limited' => 'Kategori ini memiliki {{PLURAL:$1|satu halaman|$1 halaman}} berikut.',
'category-file-count'            => '{{PLURAL:$2|Kategori ini hanya memiliki satu berkas berikut.|Kategori ini memiliki {{PLURAL:$1|berkas|$1 berkas}} berikut, dari total $2.}}',
'category-file-count-limited'    => 'Kategori ini memiliki {{PLURAL:$1|berkas|$1 berkas}} berikut.',
'listingcontinuesabbrev'         => 'samb.',

'mainpagetext'      => "<big>'''MediaWiki telah terinstal dengan sukses'''</big>.",
'mainpagedocfooter' => 'Silakan baca [http://meta.wikimedia.org/wiki/Help:Contents Panduan Pengguna] untuk informasi penggunaan perangkat lunak wiki.

== Memulai penggunaan ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Daftar pengaturan preferensi]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Milis rilis MediaWiki]',

'about'          => 'Perihal',
'article'        => 'Artikel',
'newwindow'      => '(buka di jendela baru)',
'cancel'         => 'Batalkan',
'qbfind'         => 'Pencarian',
'qbbrowse'       => 'Navigasi',
'qbedit'         => 'Sunting',
'qbpageoptions'  => 'Halaman ini',
'qbpageinfo'     => 'Konteks halaman',
'qbmyoptions'    => 'Halaman saya',
'qbspecialpages' => 'Halaman istimewa',
'moredotdotdot'  => 'Lainnya...',
'mypage'         => 'Halaman saya',
'mytalk'         => 'Pembicaraan saya',
'anontalk'       => 'Pembicaraan IP ini',
'navigation'     => 'Navigasi',
'and'            => 'dan',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Kesalahan',
'returnto'          => 'Kembali ke $1.',
'tagline'           => 'Dari {{SITENAME}}',
'help'              => 'Bantuan',
'search'            => 'Pencarian',
'searchbutton'      => 'Cari',
'go'                => 'Tuju ke',
'searcharticle'     => 'Tuju ke',
'history'           => 'Versi terdahulu',
'history_short'     => 'Versi terdahulu',
'updatedmarker'     => 'diubah sejak kunjungan terakhir saya',
'info_short'        => 'Informasi',
'printableversion'  => 'Versi cetak',
'permalink'         => 'Pranala permanen',
'print'             => 'Cetak',
'edit'              => 'Sunting',
'create'            => 'Buat',
'editthispage'      => 'Sunting halaman ini',
'create-this-page'  => 'Buat halaman ini',
'delete'            => 'Hapus',
'deletethispage'    => 'Hapus halaman ini',
'undelete_short'    => 'Batal hapus $1 {{PLURAL:$1|suntingan|suntingan}}',
'protect'           => 'Lindungi',
'protect_change'    => 'ubah',
'protectthispage'   => 'Lindungi halaman ini',
'unprotect'         => 'Perlindungan',
'unprotectthispage' => 'Ubah perlindungan halaman ini',
'newpage'           => 'Halaman baru',
'talkpage'          => 'Bicarakan halaman ini',
'talkpagelinktext'  => 'Bicara',
'specialpage'       => 'Halaman istimewa',
'personaltools'     => 'Peralatan pribadi',
'postcomment'       => 'Kirim komentar',
'articlepage'       => 'Lihat artikel',
'talk'              => 'Pembicaraan',
'views'             => 'Tampilan',
'toolbox'           => 'Kotak peralatan',
'userpage'          => 'Lihat halaman pengguna',
'projectpage'       => 'Lihat halaman proyek',
'imagepage'         => 'Lihat halaman berkas',
'mediawikipage'     => 'Lihat halaman pesan sistem',
'templatepage'      => 'Lihat halaman templat',
'viewhelppage'      => 'Lihat halaman bantuan',
'categorypage'      => 'Lihat halaman kategori',
'viewtalkpage'      => 'Lihat halaman pembicaran',
'otherlanguages'    => 'Bahasa lain',
'redirectedfrom'    => '(Dialihkan dari $1)',
'redirectpagesub'   => 'Halaman peralihan',
'lastmodifiedat'    => 'Halaman ini terakhir diubah pada $2, $1.', # $1 date, $2 time
'viewcount'         => 'Halaman ini telah diakses sebanyak {{PLURAL:$1|satu kali|$1 kali}}.<br />',
'protectedpage'     => 'Halaman yang dilindungi',
'jumpto'            => 'Langsung ke:',
'jumptonavigation'  => 'navigasi',
'jumptosearch'      => 'cari',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Perihal {{SITENAME}}',
'aboutpage'            => 'Project:Perihal',
'bugreports'           => 'Laporan bug',
'bugreportspage'       => 'Project:Laporan bug',
'copyright'            => 'Seluruh teks tersedia sesuai dengan $1.',
'copyrightpagename'    => 'Hak cipta {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Hak cipta',
'currentevents'        => 'Peristiwa terkini',
'currentevents-url'    => 'Project:Peristiwa terkini',
'disclaimers'          => 'Penyangkalan',
'disclaimerpage'       => 'Project:Penyangkalan umum',
'edithelp'             => 'Bantuan penyuntingan',
'edithelppage'         => 'Help:Penyuntingan',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Isi',
'mainpage'             => 'Halaman Utama',
'mainpage-description' => 'Halaman Utama',
'policy-url'           => 'Project:Kebijakan',
'portal'               => 'Portal komunitas',
'portal-url'           => 'Project:Portal komunitas',
'privacy'              => 'Kebijakan privasi',
'privacypage'          => 'Project:Kebijakan privasi',

'badaccess'        => 'Kesalahan hak akses',
'badaccess-group0' => 'Anda tidak diizinkan untuk melakukan tindakan yang Anda minta.',
'badaccess-group1' => 'Tindakan yang Anda minta dibatasi untuk pengguna kelompok $1.',
'badaccess-group2' => 'Tindakan yang Anda minta dibatasi untuk pengguna dalam kelompok $1.',
'badaccess-groups' => 'Tindakan yang Anda minta dibatasi untuk pengguna dalam kelompok $1.',

'versionrequired'     => 'Dibutuhkan MediaWiki versi $1',
'versionrequiredtext' => 'MediaWiki versi $1 dibutuhkan untuk menggunakan halaman ini. Lihat [[Special:Version|halaman versi]]',

'ok'                      => 'OK',
'retrievedfrom'           => 'Diperoleh dari "$1"',
'youhavenewmessages'      => 'Anda mempunyai $1 ($2).',
'newmessageslink'         => 'pesan baru',
'newmessagesdifflink'     => 'perubahan terakhir',
'youhavenewmessagesmulti' => 'Anda mendapat pesan-pesan baru $1',
'editsection'             => 'sunting',
'editold'                 => 'sunting',
'viewsourceold'           => 'lihat sumber',
'editsectionhint'         => 'Sunting bagian: $1',
'toc'                     => 'Daftar isi',
'showtoc'                 => 'tampilkan',
'hidetoc'                 => 'sembunyikan',
'thisisdeleted'           => 'Lihat atau kembalikan $1?',
'viewdeleted'             => 'Lihat $1?',
'restorelink'             => '$1 {{PLURAL:$1|suntingan|suntingan}} yang telah dihapus',
'feedlinks'               => 'Umpan:',
'feed-invalid'            => 'Tipe permintaan umpan tidak tepat.',
'feed-unavailable'        => 'Umpan sindikasi tidak tersedia di {{SITENAME}}',
'site-rss-feed'           => 'Umpan RSS $1',
'site-atom-feed'          => 'Umpan Atom $1',
'page-rss-feed'           => 'Umpan RSS "$1"',
'page-atom-feed'          => 'Umpan Atom "$1"',
'red-link-title'          => '$1 (belum dibuat)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Halaman',
'nstab-user'      => 'Pengguna',
'nstab-media'     => 'Media',
'nstab-special'   => 'Istimewa',
'nstab-project'   => 'Proyek',
'nstab-image'     => 'Berkas',
'nstab-mediawiki' => 'Pesan',
'nstab-template'  => 'Templat',
'nstab-help'      => 'Bantuan',
'nstab-category'  => 'Kategori',

# Main script and global functions
'nosuchaction'      => 'Tidak ada tindakan tersebut',
'nosuchactiontext'  => 'Tindakan yang dispesifikasikan oleh URL tersebut tidak dikenal oleh wiki.',
'nosuchspecialpage' => 'Tidak ada halaman istimewa tersebut',
'nospecialpagetext' => "<big>'''Anda meminta halaman istimewa yang tidak sah.'''</big>

Daftar halaman istimewa yang sah dapat dilihat di [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Kesalahan',
'databaseerror'        => 'Kesalahan basis data',
'dberrortext'          => 'Ada kesalahan sintaks pada permintaan basis data. Kesalahan ini mungkin menandakan adanya \'\'bug\'\' dalam perangkat lunak. Permintaan basis data yang terakhir adalah: <blockquote><tt>$1</tt></blockquote> dari dalam fungsi "<tt>$2</tt>". Kesalahan MySQL "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Ada kesalahan sintaks pada permintaan basis data. Permintaan basis data yang terakhir adalah: "$1" dari dalam fungsi "$2". Kesalahan MySQL "$3: $4".',
'noconnect'            => 'Wiki ini sedang mengalami masalah teknis dan tidak dapat menghubungi server basis data.<br />
$1',
'nodb'                 => 'Tidak dapat memilih basis data $1',
'cachederror'          => 'Berikut ini adalah salinan <em>cache</em> dari halaman yang diminta, yang mungkin tidak up-to-date.',
'laggedslavemode'      => 'Peringatan: Halaman mungkin tidak berisi perubahan terbaru.',
'readonly'             => 'Basis data dikunci',
'enterlockreason'      => 'Masukkan alasan penguncian, termasuk perkiraan kapan kunci akan dibuka',
'readonlytext'         => 'Basis data sedang dikunci terhadap masukan baru. Pengurus yang melakukan penguncian memberikan penjelasan sebagai berikut: <p>$1',
'missing-article'      => 'Basis data tidak dapat menemukan teks dari halaman yang seharusnya ada, yaitu "$1" $2.

Hal ini biasanya disebabkan oleh pranala usang ke revisi terdahulu halaman yang telah dihapuskan.

Jika bukan ini penyebabnya, Anda mungkin telah menemukan sebuah bug dalam perangkat lunak.
Silakan laporkan hal ini kepada salah seorang [[Special:ListUsers/sysop|Pengurus]], dengan menyebutkan alamat URL yang dituju.',
'missingarticle-rev'   => '(revisi#: $1)',
'missingarticle-diff'  => '(Beda: $1, $2)',
'readonly_lag'         => 'Basis data telah dikunci otomatis selagi basis data sekunder melakukan sinkronisasi dengan basis data utama',
'internalerror'        => 'Kesalahan internal',
'internalerror_info'   => 'Kesalahan internal: $1',
'filecopyerror'        => 'Tidak dapat menyalin berkas "$1" ke "$2".',
'filerenameerror'      => 'Tidak dapat mengubah nama berkas "$1" menjadi "$2".',
'filedeleteerror'      => 'Tidak dapat menghapus berkas "$1".',
'directorycreateerror' => 'Tidak dapat membuat direktori "$1".',
'filenotfound'         => 'Tidak dapat menemukan berkas "$1".',
'fileexistserror'      => 'Tidak dapat menulis berkas "$1": berkas sudah ada',
'unexpected'           => 'Nilai di luar jangkauan: "$1"="$2".',
'formerror'            => 'Kesalahan: Tidak dapat mengirimkan formulir',
'badarticleerror'      => 'Tindakan ini tidak dapat dilaksanakan di halaman ini.',
'cannotdelete'         => 'Tidak dapat menghapus halaman atau berkas yang diminta.',
'badtitle'             => 'Judul tidak sah',
'badtitletext'         => 'Judul halaman yang diminta tidak sah, kosong, atau judul antarbahasa atau antarwiki yang salah sambung.',
'perfdisabled'         => 'Maaf! Fitur ini dimatikan sementara karena memperlambat basis data hingga tidak ada yang dapat menggunakan wiki ini.',
'perfcached'           => 'Data berikut ini diambil dari <em>cache</em> dan mungkin bukan data mutakhir:',
'perfcachedts'         => 'Data berikut ini diambil dari <em>cache</em>, dan terakhir diperbarui pada $1.',
'querypage-no-updates' => 'Pemutakhiran dari halaman ini sedang dimatikan. Data yang ada di sini saat ini tidak akan dimuat ulang.',
'wrong_wfQuery_params' => 'Parameter salah ke wfQuery()<br />Fungsi: $1<br />Permintaan: $2',
'viewsource'           => 'Lihat sumber',
'viewsourcefor'        => 'dari $1',
'actionthrottled'      => 'Tindakan dibatasi',
'actionthrottledtext'  => 'Anda dibatasi untuk melakukan tindakan ini terlalu banyak dalam waktu pendek. Silakan mencoba lagi setelah beberapa menit.',
'protectedpagetext'    => 'Halaman ini telah dikunci untuk menghindari penyuntingan.',
'viewsourcetext'       => 'Anda dapat melihat atau menyalin sumber halaman ini:',
'protectedinterface'   => 'Halaman ini berisi teks antarmuka untuk digunakan oleh perangkat lunak dan telah dikunci untuk menghindari kesalahan.',
'editinginterface'     => "'''Peringatan:''' Anda menyunting halaman yang digunakan untuk menyediakan teks antarmuka dengan perangkat lunak. Perubahan teks ini akan mempengaruhi tampilan pada pengguna lain.",
'sqlhidden'            => '(Permintaan SQL disembunyikan)',
'cascadeprotected'     => 'Halaman ini telah dilindungi dari penyuntingan karena disertakan di {{PLURAL:$1|halaman|halaman-halaman}} berikut yang telah dilindungi dengan opsi "runtun":
$2',
'namespaceprotected'   => "Anda tak memiliki hak untuk menyunting halaman di ruang nama '''$1'''.",
'customcssjsprotected' => 'Anda tak memiliki hak menyunting halaman ini karena mengandung pengaturan pribadi pengguna lain.',
'ns-specialprotected'  => 'Halaman pada ruang nama {{ns:special}} tidak dapat disunting.',
'titleprotected'       => "Judul ini dilindungi dari pembuatan oleh [[User:$1|$1]].
Alasan yang diberikan adalah ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Kesalahan konfigurasi: pemindai virus tidak dikenal: <i>$1</i>',
'virus-scanfailed'     => 'Pemindaian gagal (kode $1)',
'virus-unknownscanner' => 'Antivirus tidak dikenal:',

# Login and logout pages
'logouttitle'                => 'Keluar log pengguna',
'logouttext'                 => 'Anda telah keluar log dari sistem. Anda dapat terus menggunakan {{SITENAME}} secara anonim, atau Anda dapat masuk log lagi sebagai pengguna yang sama atau pengguna yang lain. Perhatikan bahwa beberapa halaman mungkin masih terus menunjukkan bahwa Anda masih masuk log sampai Anda membersihkan <em>cache</em> penjelajah web Anda',
'welcomecreation'            => '== Selamat datang, $1! ==

Akun Anda telah dibuat. Jangan lupa mengatur konfigurasi {{SITENAME}} Anda.',
'loginpagetitle'             => 'Masuk log pengguna',
'yourname'                   => 'Nama pengguna:',
'yourpassword'               => 'Kata sandi:',
'yourpasswordagain'          => 'Ulangi kata sandi:',
'remembermypassword'         => 'Ingat kata sandi saya di komputer ini',
'yourdomainname'             => 'Domain Anda:',
'externaldberror'            => 'Telah terjadi kesalahan otentikasi basis data eksternal atau Anda tidak diizinkan melakukan kemaskini terhadap akun eksternal Anda.',
'loginproblem'               => '<strong>Ada masalah dengan proses masuk log Anda.</strong><br />Silakan coba lagi!',
'login'                      => 'Masuk log',
'nav-login-createaccount'    => 'Masuk log / buat akun',
'loginprompt'                => "Anda harus mengaktifkan ''cookies'' untuk dapat masuk log ke {{SITENAME}}.",
'userlogin'                  => 'Masuk log / buat akun',
'logout'                     => 'Keluar log',
'userlogout'                 => 'Keluar log',
'notloggedin'                => 'Belum masuk log',
'nologin'                    => 'Belum mempunyai akun? $1.',
'nologinlink'                => 'Daftarkan akun baru',
'createaccount'              => 'Buat akun baru',
'gotaccount'                 => 'Sudah terdaftar sebagai pengguna? $1.',
'gotaccountlink'             => 'Masuk log',
'createaccountmail'          => 'melalui surat-e',
'badretype'                  => 'Kata sandi yang Anda masukkan salah.',
'userexists'                 => 'Nama pengguna yang Anda masukkan sudah dipakai.
Silakan pilih nama yang lain.',
'youremail'                  => 'Surat elektronik:',
'username'                   => 'Nama pengguna:',
'uid'                        => 'ID pengguna:',
'prefs-memberingroups'       => 'Anggota {{PLURAL:$1|kelompok|kelompok}}:',
'yourrealname'               => 'Nama asli:',
'yourlanguage'               => 'Bahasa antarmuka:',
'yourvariant'                => 'Varian bahasa',
'yournick'                   => 'Nama samaran:',
'badsig'                     => 'Tanda tangan mentah tak sah; periksa tag HTML.',
'badsiglength'               => 'Tanda tangan terlalu panjang.
Maksimal $1 {{PLURAL:$1|karakter|karakter}}.',
'email'                      => 'Surat elektronik',
'prefs-help-realname'        => '* Nama asli bersifat opsional dan jika Anda memberikannya, nama asli Anda akan digunakan untuk memberi pengenalan atas hasil kerja Anda.',
'loginerror'                 => 'Kesalahan masuk log',
'prefs-help-email'           => 'Alamat surat-e bersifat opsional, namun bila sewaktu-waktu Anda lupa akan kata sandi Anda, kami dapat mengirimkannya melalui surat-e tersebut.
Anda juga dapat memilih untuk memungkinkan orang lain menghubungi Anda melalui halaman pengguna atau halaman pembicaraan pengguna Anda tanpa perlu membuka identitas Anda.',
'prefs-help-email-required'  => 'Alamat surat-e dibutuhkan.',
'nocookiesnew'               => "Akun pengguna telah dibuat, tetapi Anda belum masuk log. {{SITENAME}} menggunakan ''cookies'' untuk log pengguna. ''Cookies'' pada penjelajah web Anda dimatikan. Silakan aktifkan dan masuk log kembali dengan nama pengguna dan kata sandi Anda.",
'nocookieslogin'             => "{{SITENAME}} menggunakan ''cookies'' untuk log penggunanya. ''Cookies'' pada penjelajah web Anda dimatikan. Silakan aktifkan dan coba lagi.",
'noname'                     => 'Nama pengguna yang Anda masukkan tidak sah.',
'loginsuccesstitle'          => 'Berhasil masuk log',
'loginsuccess'               => "'''Anda sekarang masuk log di {{SITENAME}} sebagai \"\$1\".'''",
'nosuchuser'                 => 'Tidak ada pengguna dengan nama "$1".
Silakan periksa kembali ejaan Anda, atau [[Special:Userlogin/signup|buat akun baru]].',
'nosuchusershort'            => 'Tidak ada pengguna dengan nama "<nowiki>$1</nowiki>".
Silakan periksa kembali ejaan Anda.',
'nouserspecified'            => 'Anda harus memasukkan nama pengguna.',
'wrongpassword'              => 'Kata sandi yang Anda masukkan salah. Silakan coba lagi.',
'wrongpasswordempty'         => 'Anda tidak memasukkan kata sandi. Silakan coba lagi.',
'passwordtooshort'           => 'Kata sandi Anda tidak sah atau terlalu pendek. 
Kata sandi paling tidak harus terdiri dari {{PLURAL:$1|1 karakter|$1 karakter}} dan harus berbeda dengan nama pengguna Anda.',
'mailmypassword'             => 'Surat-e kata sandi baru',
'passwordremindertitle'      => 'Peringatan kata sandi dari {{SITENAME}}',
'passwordremindertext'       => 'Seseorang (mungkin Anda, dari alamat IP $1) meminta kami mengirimkan kata sandi yang baru untuk {{SITENAME}} ($4). Kata sandi sementara untuk pengguna "$2" telah dibuatkan dan diset menjadi "$3". Jika memang Anda yang mengajukan permintaan ini, silakan masuk log dan segera mengganti dengan kata sandi yang baru.

Jika bukan Anda yang melakukan permintaan kata sandi baru, atau Anda telah mengingat kata sandi Anda dan akan tetap menggunakan kata sandi tersebut, silakan abaikan pesan ini dan tetap gunakan kata sandi lama Anda.',
'noemail'                    => 'Tidak ada alamat surat-e yang tercatat untuk pengguna "$1".',
'passwordsent'               => 'Kata sandi baru telah dikirimkan ke surat-e yang didaftarkan untuk "$1". Silakan masuk log kembali setelah menerima surat-e tersebut.',
'blocked-mailpassword'       => 'Alamat IP Anda diblokir dari penyuntingan dan karenanya tidak diizinkan menggunakan fungsi pengingat kata sandi untuk mencegah penyalahgunaan.',
'eauthentsent'               => 'Sebuah surat elektronik untuk konfirmasi telah dikirim ke alamat surat elektronik Anda. Anda harus mengikuti instruksi di dalam surat elektronik tersebut untuk melakukan konfirmasi bahwa alamat tersebut adalah benar kepunyaan Anda. {{SITENAME}} tidak akan mengaktifkan fitur surat elektronik jika langkah ini belum dilakukan.',
'throttled-mailpassword'     => 'Suatu pengingat kata sandi telah dikirimkan dalam {{PLURAL:$1|jam|$1 jam}} terakhir. 
Untuk menghindari penyalahgunaan, hanya satu kata sandi yang akan dikirimkan setiap {{PLURAL:$1|jam|$1 jam}}.',
'mailerror'                  => 'Kesalahan dalam mengirimkan surat-e: $1',
'acct_creation_throttle_hit' => 'Alamat IP yang Anda gunakan telah membuat $1 akun dalam 24 jam terakhir, jumlah maksimum pembuatan akun yang diizinkan. Untuk sementara waktu pengguna dari alamat IP ini tidak bisa lagi membuat akun.',
'emailauthenticated'         => 'Alamat surat-e Anda telah dikonfirmasi pada $1.',
'emailnotauthenticated'      => 'Alamat surat-e Anda belum dikonfirmasi. Sebelum dikonfirmasi Anda tidak bisa menggunakan fitur surat elektronik.',
'noemailprefs'               => 'Anda harus memasukkan suatu alamat surat-e untuk dapat menggunakan fitur ini.',
'emailconfirmlink'           => 'Konfirmasikan alamat surat-e Anda',
'invalidemailaddress'        => 'Alamat surat-e ini tidak dapat diterima karena formatnya tidak sesuai.
Harap masukkan alamat surat-e dalam format yang benar atau kosongkan isian tersebut.',
'accountcreated'             => 'Akun dibuat',
'accountcreatedtext'         => 'Akun pengguna untuk $1 telah dibuat.',
'createaccount-title'        => 'Pembuatan akun untuk {{SITENAME}}',
'createaccount-text'         => 'Seseorang telah membuat sebuah akun untuk alamat surat-e Anda di {{SITENAME}} ($4) dengan nama "$2" dan kata sandi "$3". Anda dianjurkan untuk masuk log dan mengganti kata sandi Anda sekarang.

Anda dapat mengabaikan pesan nini jika akun ini dibuat karena suatu kesalahan.',
'loginlanguagelabel'         => 'Bahasa: $1',

# Password reset dialog
'resetpass'               => 'Atur ulang kata sandi akun',
'resetpass_announce'      => 'Anda telah masuk log dengan kode sementara yang dikirim melalui surat-e. Untuk melanjutkan, Anda harus memasukkan kata sandi baru di sini:',
'resetpass_text'          => '<!-- Tambahkan teks di sini -->',
'resetpass_header'        => 'Atur ulang kata sandi',
'resetpass_submit'        => 'Atur kata sandi dan masuk log',
'resetpass_success'       => 'Kata sandi Anda telah berhasil diubah! Sekarang memproses masuk log Anda...',
'resetpass_bad_temporary' => 'Kata sandi sementara salah. Anda mungkin pernah berhasil mengganti kata sandi Anda atau telah meminta kata sandi baru.',
'resetpass_forbidden'     => 'Kata sandi tidak dapat diubah di wiki ini',
'resetpass_missing'       => 'Data formulir tak dikenali.',

# Edit page toolbar
'bold_sample'     => 'Teks ini akan dicetak tebal',
'bold_tip'        => 'Cetak tebal',
'italic_sample'   => 'Teks ini akan dicetak miring',
'italic_tip'      => 'Cetak miring',
'link_sample'     => 'Judul pranala',
'link_tip'        => 'Pranala internal',
'extlink_sample'  => 'http://www.example.com judul pranala',
'extlink_tip'     => 'Pranala luar (jangan lupa awalan http:// )',
'headline_sample' => 'Teks judul',
'headline_tip'    => 'Subbagian tingkat 1',
'math_sample'     => 'Masukkan rumus di sini',
'math_tip'        => 'Rumus matematika (LaTeX)',
'nowiki_sample'   => 'Teks ini tidak akan diformat',
'nowiki_tip'      => 'Abaikan pemformatan wiki',
'image_sample'    => 'Contoh.jpg',
'image_tip'       => 'Cantumkan berkas',
'media_sample'    => 'Contoh.ogg',
'media_tip'       => 'Pranala berkas media',
'sig_tip'         => 'Tanda tangan Anda dengan tanda waktu',
'hr_tip'          => 'Garis horisontal',

# Edit pages
'summary'                          => 'Ringkasan',
'subject'                          => 'Subjek/judul',
'minoredit'                        => 'Ini adalah suntingan kecil.',
'watchthis'                        => 'Pantau halaman ini',
'savearticle'                      => 'Simpan halaman',
'preview'                          => 'Pratayang',
'showpreview'                      => 'Lihat pratayang',
'showlivepreview'                  => 'Pratayang langsung',
'showdiff'                         => 'Perlihatkan perubahan',
'anoneditwarning'                  => 'Anda tidak terdaftar masuk. Alamat IP Anda akan tercatat dalam sejarah (versi terdahulu) halaman ini.',
'missingsummary'                   => "'''Peringatan:''' Anda tidak memasukkan ringkasan penyuntingan. Jika Anda kembali menekan tombol Simpan, suntingan Anda akan disimpan tanpa ringkasan penyuntingan.",
'missingcommenttext'               => 'Harap masukkan komentar di bawah ini.',
'missingcommentheader'             => "'''Peringatan:''' Anda belum memberikan subjek atau judul untuk komentar Anda. Jika Anda kembali menekan Simpan, suntingan Anda akan disimpan tanpa komentar tersebut.",
'summary-preview'                  => 'Pratayang ringkasan',
'subject-preview'                  => 'Pratayang subyek/tajuk',
'blockedtitle'                     => 'Pengguna diblokir',
'blockedtext'                      => "<big>'''Nama pengguna atau alamat IP Anda telah diblokir.'''</big>

Blokir dilakukan oleh $1.
Alasan yang diberikan adalah ''$2''.

* Diblokir sejak: $8
* Blokir kadaluwarsa pada: $6
* Sasaran pemblokiran: $7

Anda dapat menghubungi $1 atau [[{{MediaWiki:Grouppage-sysop}}|pengurus lainnya]] untuk membicarakan hal ini.

Anda tidak dapat menggunakan fitur 'Kirim surat-e pengguna ini' kecuali Anda telah memasukkan alamat surat-e yang sah di [[Special:Preferences|preferensi akun]] dan Anda tidak diblokir untuk menggunakannya.

Alamat IP Anda adalah $3, dan ID pemblokiran adalah $5.
Tolong sertakan salah satu atau kedua informasi ini pada setiap pertanyaan yang Anda buat.",
'autoblockedtext'                  => 'Alamat IP Anda telah terblokir secara otomatis karena digunakan oleh pengguna lain, yang diblokir oleh $1. Pemblokiran dilakukan atas alasan:

:\'\'$2\'\'

* Diblokir sejak: $8
* Blokir kadaluwarsa pada: $6
* Sasaran pemblokiran: $7

Anda dapat menghubungi $1 atau [[{{MediaWiki:Grouppage-sysop}}|pengurus lainnya]] untuk membicarakan hal ini.

Anda tidak dapat menggunakan fitur "kirim surat-e pengguna ini" kecuali Anda telah memasukkan alamat surat-e yang sah di [[Special:Preferences|preferensi akun]] Anda dan Anda tidak diblokir untuk menggunakannya.

Alamat IP Anda saat ini adalah $3, dan ID pemblokiran adalah #$5.
Tolong sertakan informasi-informasi ini dalam setiap pertanyaan Anda.',
'blockednoreason'                  => 'tidak ada alasan yang diberikan',
'blockedoriginalsource'            => "Isi sumber '''$1''' ditunjukkan berikut ini:",
'blockededitsource'                => "Teks '''suntingan Anda''' terhadap '''$1''' ditunjukkan berikut ini:",
'whitelistedittitle'               => 'Perlu masuk log untuk menyunting',
'whitelistedittext'                => 'Anda harus $1 untuk dapat menyunting artikel.',
'confirmedittitle'                 => 'Konfirmasi surat-e diperlukan untuk melakukan penyuntingan',
'confirmedittext'                  => 'Anda harus mengkonfirmasikan dulu alamat surat-e Anda sebelum menyunting halaman.
Harap masukkan dan validasikan alamat surat-e Anda melalui [[Special:Preferences|halaman preferensi pengguna]] Anda.',
'nosuchsectiontitle'               => 'Subbagian tersebut tak ditemukan',
'nosuchsectiontext'                => 'Anda mencoba menyunting suatu subbagian tidak ada. Karena subbagian $1 tidak ada, suntingan Anda tak dapat disimpan.',
'loginreqtitle'                    => 'Harus masuk log',
'loginreqlink'                     => 'masuk log',
'loginreqpagetext'                 => 'Anda harus $1 untuk dapat melihat halaman lainnya.',
'accmailtitle'                     => 'Kata sandi telah terkirim.',
'accmailtext'                      => "Kata sandi untuk '$1' telah dikirimkan ke $2.",
'newarticle'                       => '(Baru)',
'newarticletext'                   => "Anda mengikuti pranala ke halaman yang belum tersedia. Untuk membuat halaman tersebut, ketiklah isi halaman di kotak di bawah ini (lihat [[{{MediaWiki:Helppage}}|halaman bantuan]] untuk informasi lebih lanjut). Jika Anda tanpa sengaja sampai ke halaman ini, klik tombol '''back''' di penjelajah web anda.",
'anontalkpagetext'                 => "----''Ini adalah halaman pembicaraan seorang pengguna anonim yang belum membuat akun atau tidak menggunakannya.
Dengan demikian, kami terpaksa harus memakai alamat IP yang bersangkutan untuk mengidentifikasikannya.
Alamat IP seperti ini mungkin dipakai bersama oleh beberapa pengguna yang berbeda.
Jika Anda adalah seorang pengguna anonim dan merasa mendapatkan komentar-komentar yang tidak relevan yang ditujukan langsung kepada Anda, silakan [[Special:UserLogin/signup|membuat akun]] atau [[Special:UserLogin|masuk log]] untuk menghindari kerancuan dengan pengguna anonim lainnya di lain waktu.''",
'noarticletext'                    => 'Saat ini tidak ada teks dalam halaman ini. Anda dapat [[Special:Search/{{PAGENAME}}|melakukan pencarian untuk judul halaman ini]] di halaman-halaman lain atau [{{fullurl:{{FULLPAGENAME}}|action=edit}} sunting halaman ini].',
'userpage-userdoesnotexist'        => 'Akun pengguna "$1" tidak terdaftar.',
'clearyourcache'                   => "'''Catatan:''' Setelah menyimpan preferensi, Anda perlu membersihkan <em>cache</em> penjelajah web Anda untuk melihat perubahan. '''Mozilla / Firefox / Safari:''' tekan ''Ctrl-Shift-R'' (''Cmd-Shift-R'' pada Apple Mac); '''IE:''' tekan ''Ctrl-F5''; '''Konqueror:''': tekan ''F5''; '''Opera''' bersihkan <em>cache</em> melalui menu ''Tools→Preferences''.",
'usercssjsyoucanpreview'           => "<strong>Tips:</strong> Gunakan tombol 'Lihat pratayang' untuk menguji CSS/JS baru Anda sebelum menyimpannya.",
'usercsspreview'                   => "'''Ingatlah bahwa Anda sedang menampilkan pratayang dari CSS Anda.
Pratayang ini belum disimpan!'''",
'userjspreview'                    => "'''Ingatlah bahwa yang Anda lihat hanyalah pratayang JavaScript Anda, dan bahwa pratayang tersebut belum disimpan!'''",
'userinvalidcssjstitle'            => "'''Peringatan:''' Kulit \"\$1\" tidak ditemukan. Harap diingat bahwa halaman .css dan .js menggunakan huruf kecil, contoh {{ns:user}}:Foo/monobook.css dan bukannya {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Diperbarui)',
'note'                             => '<strong>Catatan:</strong>',
'previewnote'                      => '<strong>Ingatlah bahwa ini hanyalah pratayang yang belum disimpan!</strong>',
'previewconflict'                  => 'Pratayang ini mencerminkan teks pada bagian atas kotak suntingan teks sebagaimana akan terlihat bila Anda menyimpannya.',
'session_fail_preview'             => '<strong>Maaf, kami tidak dapat mengolah suntingan Anda akibat terhapusnya data sesi.
Silakan coba sekali lagi.
Jika masih tidak berhasil, cobalah [[Special:UserLogout|keluar lo]]g dan masuk log kembali.</strong>',
'session_fail_preview_html'        => "<strong>Kami tidak dapat memproses suntingan Anda karena hilangnya data sesi.</strong>

''Karena {{SITENAME}} mengizinkan penggunaan HTML mentah, pratayang telah disembunyikan sebagai pencegahan terhadap serangan JavaScript.''

<strong>Jika ini merupakan upaya suntingan yang sahih, silakan coba lagi.
Jika masih tetap tidak berhasil, cobalah [[Special:UserLogout|keluar log]] dan masuk kembali.</strong>",
'token_suffix_mismatch'            => '<strong>Suntingan Anda ditolak karena aplikasi klien Anda mengubah karakter tanda baca pada suntingan. Suntingan tersebut ditolak untuk mencegah kesalahan pada artikel teks. Hal ini kadang terjadi jika Anda menggunakan layanan proxy anonim berbasis web yang bermasalah.</strong>',
'editing'                          => 'Menyunting $1',
'editingsection'                   => 'Menyunting $1 (bagian)',
'editingcomment'                   => 'Menyunting $1 (komentar)',
'editconflict'                     => 'Konflik penyuntingan: $1',
'explainconflict'                  => 'Orang lain telah menyunting halaman ini sejak Anda mulai menyuntingnya. Bagian atas teks ini mengandung teks halaman saat ini. Perubahan yang Anda lakukan ditunjukkan pada bagian bawah teks. Anda hanya perlu menggabungkan perubahan Anda dengan teks yang telah ada. <strong>Hanya</strong> teks pada bagian atas halamanlah yang akan disimpan apabila Anda menekan "Simpan halaman".<p>',
'yourtext'                         => 'Teks Anda',
'storedversion'                    => 'Versi tersimpan',
'nonunicodebrowser'                => '<strong>PERINGATAN: Penjelajah web Anda tidak mendukung Unicode, silakan ganti penjelajah web Anda sebelum menyunting artikel.</strong>',
'editingold'                       => '<strong>Peringatan:
Anda menyunting revisi lama suatu halaman.
Jika Anda menyimpannya, perubahan-perubahan yang dibuat sejak revisi ini akan hilang.</strong>',
'yourdiff'                         => 'Perbedaan',
'copyrightwarning'                 => 'Perhatikan bahwa semua kontribusi terhadap {{SITENAME}} dianggap dilisensikan sesuai dengan $2 (lihat $1 untuk informasi lebih lanjut). Jika Anda tidak ingin tulisan Anda disunting dan disebarkan ke halaman web yang lain, jangan kirimkan artikel Anda ke sini.<br />Anda juga berjanji bahwa ini adalah hasil karya Anda sendiri, atau disalin dari sumber milik umum atau sumber bebas yang lain. <strong>JANGAN KIRIMKAN KARYA YANG DILINDUNGI HAK CIPTA TANPA IZIN!</strong>',
'copyrightwarning2'                => 'Perhatikan bahwa semua kontribusi terhadap {{SITENAME}} dapat disunting, diubah, atau dihapus oleh penyumbang lainnya. Jika Anda tidak ingin tulisan Anda disunting orang lain, jangan kirimkan artikel Anda ke sini.<br />Anda juga berjanji bahwa ini adalah hasil karya Anda sendiri, atau disalin dari sumber milik umum atau sumber bebas yang lain (lihat $1 untuk informasi lebih lanjut). <strong>JANGAN KIRIMKAN KARYA YANG DILINDUNGI HAK CIPTA TANPA IZIN!</strong>',
'longpagewarning'                  => "'''PERINGATAN: Halaman ini panjangnya adalah $1 kilobita; beberapa penjelajah web mungkin mengalami masalah dalam menyunting halaman yang panjangnya 32 kb atau lebih. Harap pertimbangkan untuk memecah halaman menjadi beberapa bagian yang lebih kecil.'''",
'longpageerror'                    => '<strong>KESALAHAN: Teks yang Anda kirimkan sebesar $1 kilobita, yang berarti lebih besar dari jumlah maksimum $2 kilobita. Teks tidak dapat disimpan.</strong>',
'readonlywarning'                  => '<strong>PERINGATAN: Basis data sedang dikunci karena pemeliharaan, sehingga saat ini Anda tidak akan dapat menyimpan hasil penyuntingan Anda. Anda mungkin perlu memindahkan hasil penyuntingan Anda ini ke tempat lain untuk disimpan belakangan.</strong>',
'protectedpagewarning'             => '<strong>PERINGATAN: Halaman ini sedang dilindungi sehingga hanya pengguna dengan hak akses pengurus saja yang dapat menyuntingnya.</strong>',
'semiprotectedpagewarning'         => "'''Catatan:''' Halaman ini sedang dilindungi, sehingga hanya pengguna terdaftar yang bisa menyuntingnya.",
'cascadeprotectedwarning'          => "'''PERINGATAN:''' Halaman ini sedang dilindungi sehingga hanya pengguna dengan hak akses pengurus saja yang dapat menyuntingnya karena disertakan dalam {{PLURAL:$1|halaman|halaman-halaman}} berikut yang telah dilindungi dengan opsi 'perlindungan runtun':",
'titleprotectedwarning'            => '<strong>PERINGATAN: Halaman ini telah dikunci sehingga hanya beberapa pengguna yang dapat membuatnya.</strong>',
'templatesused'                    => 'Templat yang digunakan di halaman ini:',
'templatesusedpreview'             => 'Templat yang digunakan di pratayang ini:',
'templatesusedsection'             => 'Templat yang digunakan di bagian ini:',
'template-protected'               => '(dilindungi)',
'template-semiprotected'           => '(semi-perlindungan)',
'hiddencategories'                 => 'Halaman ini adalah anggota dari {{PLURAL:$1|1 kategori tersebunyi|$1 kategori tersebunyi}}:',
'edittools'                        => '<!-- Teks di sini akan dimunculkan di bawah isian suntingan dan pemuatan.-->',
'nocreatetitle'                    => 'Pembuatan halaman baru dibatasi',
'nocreatetext'                     => '{{SITENAME}} telah membatasi pembuatan halaman-halaman baru.
Anda dapat kembali dan menyunting halaman yang telah ada, atau silakan [[Special:UserLogin|masuk log atau membuat akun]].',
'nocreate-loggedin'                => 'Anda tak memiliki hak akses untuk membuat halaman baru pada wiki ini.',
'permissionserrors'                => 'Kesalahan Hak Akses',
'permissionserrorstext'            => 'Anda tak memiliki hak untuk melakukan hal itu karena {{PLURAL:$1|alasan|alasan-alasan}} berikut:',
'permissionserrorstext-withaction' => 'Anda tidak memiliki hak untuk $2, karena {{PLURAL:$1|alasan|alasan}} berikut:',
'recreate-deleted-warn'            => "'''Peringatan: Anda membuat ulang suatu halaman yang sudah pernah dihapus.'''

Harap pertimbangkan apakah layak untuk melanjutkan suntingan Anda.
Berikut adalah log penghapusan dari halaman ini:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Peringatan: halaman ini mengandung terlalu banyak panggilan fungsi parser.

Saat ini terdapat $1, seharusnya kurang dari $2.',
'expensive-parserfunction-category'       => 'Halaman dengan terlalu banyak panggilan fungsi parser',
'post-expand-template-inclusion-warning'  => 'Peringatan: Ukuran templat yang digunakan terlalu besar.
Beberapa templat akan diabaikan.',
'post-expand-template-inclusion-category' => 'Halaman dengan ukuran templat yang melebihi batas',
'post-expand-template-argument-warning'   => 'Peringatan: Halaman ini mengandung setidaknya satu argumen templat dengan ukuran ekspansi yang terlalu besar. Argumen-argumen tersebut telah diabaikan.',
'post-expand-template-argument-category'  => 'Halaman dengan argumen templat yang diabaikan',

# "Undo" feature
'undo-success' => 'Suntingan ini dapat dibatalkan. Tolong cek perbandingan di bawah untuk meyakinkan bahwa benar itu yang Anda ingin lakukan, lalu simpan perubahan tersebut untuk menyelesaikan pembatalan suntingan.',
'undo-failure' => 'Suntingan ini tidak dapat dibatalkan karena konflik penyuntingan antara.',
'undo-norev'   => 'Suntingan ini tidak dapat dibatalkan karena halaman tidak ditemukan atau telah dihapuskan.',
'undo-summary' => '←Membatalkan revisi $1 oleh [[Special:Contributions/$2|$2]] ([[User talk:$2|Bicara]])',

# Account creation failure
'cantcreateaccounttitle' => 'Akun tak dapat dibuat',
'cantcreateaccount-text' => "Pembuatan akun dari alamat IP ini (<strong>$1</strong>) telah diblokir oleh [[User:$3|$3]].

Alasan yang diberikan oleh $3 adalah ''$2''",

# History pages
'viewpagelogs'        => 'Lihat log halaman ini',
'nohistory'           => 'Tidak ada sejarah penyuntingan untuk halaman ini',
'revnotfound'         => 'Revisi tidak ditemukan',
'revnotfoundtext'     => 'Revisi lama halaman yang Anda minta tidak dapat ditemukan. Silakan periksa URL yang digunakan untuk mengakses halaman ini.',
'currentrev'          => 'Revisi terkini',
'revisionasof'        => 'Revisi per $1',
'revision-info'       => 'Revisi per $1; $2',
'previousrevision'    => '←Revisi sebelumnya',
'nextrevision'        => 'Revisi selanjutnya→',
'currentrevisionlink' => 'Revisi terkini',
'cur'                 => 'skr',
'next'                => 'selanjutnya',
'last'                => 'akhir',
'page_first'          => 'pertama',
'page_last'           => 'terakhir',
'histlegend'          => "Pilih dua tombol radio lalu tekan tombol ''bandingkan'' untuk membandingkan versi. Klik suatu tanggal untuk melihat versi halaman pada tanggal tersebut.<br />(skr) = perbedaan dengan versi sekarang, (akhir) = perbedaan dengan versi sebelumnya, '''k''' = suntingan kecil, '''b''' = suntingan bot, → = suntingan bagian, ← = ringkasan otomatis",
'deletedrev'          => '[dihapus]',
'histfirst'           => 'Terlama',
'histlast'            => 'Terbaru',
'historysize'         => '($1 {{PLURAL:$1|bita|bita}})',
'historyempty'        => '(kosong)',

# Revision feed
'history-feed-title'          => 'Riwayat revisi',
'history-feed-description'    => 'Riwayat revisi halaman ini di wiki',
'history-feed-item-nocomment' => '$1 pada $2', # user at time
'history-feed-empty'          => 'Halaman yang diminta tak ditemukan.
Kemungkinan telah dihapus dari wiki, atau diberi nama baru.
Coba [[Special:Search|lakukan pencarian di wiki]] untuk halaman baru yang relevan.',

# Revision deletion
'rev-deleted-comment'         => '(komentar dihapus)',
'rev-deleted-user'            => '(nama pengguna dihapus)',
'rev-deleted-event'           => '(isi dihapus)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">Riwayat revisi halaman ini telah dihapus dari arsip publik. Detil mungkin tersedia di [{{fullurl:{{ns:special}}:Log/delete|page={{FULLPAGENAMEE}}}} log penghapusan].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Riwayat revisi halaman ini telah dihapus dari arsip publik. Sebagai seorang pengurus situs, Anda dapat melihatnya; detil mungkin tersedia di [{{fullurl:{{ns:special}}:Log/delete|page={{FULLPAGENAMEE}}}} log penghapusan].</div>',
'rev-delundel'                => 'tampilkan/sembunyikan',
'revisiondelete'              => 'Hapus/batal hapus revisi',
'revdelete-nooldid-title'     => 'Target revisi tak ditemukan',
'revdelete-nooldid-text'      => 'Anda belum memberikan target revisi untuk menjalankan fungsi ini.',
'revdelete-selected'          => "{{PLURAL:$2|Revisi|Revisi-revisi}} pilihan dari '''$1'''",
'logdelete-selected'          => '{{PLURAL:$1|Log|Log-log}} pilihan untuk:',
'revdelete-text'              => 'Revisi dan tindakan yang telah dihapus akan tetap muncul di halaman versi terdahulu, tapi teks isi tidak bisa diakses publik.

Pengurus lain akan dapat mengakses isi tersebunyi dan dapat membatalkan penghapusan melalui antarmuka yang sama, kecuali jika ada pembatasan lain yang dibuat oleh operator situs',
'revdelete-legend'            => 'Atur batasan:',
'revdelete-hide-text'         => 'Sembunyikan teks revisi',
'revdelete-hide-name'         => 'Sembunyikan tindakan dan target',
'revdelete-hide-comment'      => 'Tampilkan/sembunyikan ringkasan suntingan',
'revdelete-hide-user'         => 'Sembunyikan nama pengguna/IP penyunting',
'revdelete-hide-restricted'   => 'Terapkan pembatasan bagi pengurus dan pengguna lainnya',
'revdelete-suppress'          => 'Sembunyikan juga dari pengurus',
'revdelete-hide-image'        => 'Sembunyikan isi berkas',
'revdelete-unsuppress'        => 'Hapus batasan pada revisi yang dikembalikan',
'revdelete-log'               => 'Log ringkasan:',
'revdelete-submit'            => 'Terapkan pada revisi terpilih',
'revdelete-logentry'          => 'ubah tampilan revisi untuk [[$1]]',
'logdelete-logentry'          => 'ubah aturan penyembunyian dari [[$1]]',
'revdelete-success'           => 'Aturan penyembunyian revisi berhasil diterapkan.',
'logdelete-success'           => 'Aturan penyembunyian tindakan berhasil diterapkan.',
'revdel-restore'              => 'Ubah tampilan',
'pagehist'                    => 'Sejarah halaman',
'deletedhist'                 => 'Sejarah yang dihapus',
'revdelete-content'           => 'konten',
'revdelete-summary'           => 'ringkasan',
'revdelete-uname'             => 'nama pengguna',
'revdelete-restricted'        => 'akses telah dibatasi untuk opsis',
'revdelete-unrestricted'      => 'pembatasan akses opsis dihapuskan',
'revdelete-hid'               => 'sembunyikan $1',
'revdelete-unhid'             => 'tampilkan $1',
'revdelete-log-message'       => '$1 untuk $2 {{PLURAL:$2|revisi|revisi}}',
'logdelete-log-message'       => '$1 untuk $2 {{PLURAL:$2|peristiwa|peristiwa}}',

# Suppression log
'suppressionlog'     => 'Log penyembunyian',
'suppressionlogtext' => 'Berikut adalah daftar penghapusan dan pemblokiran, termasuk konten yang disembunyikan dari para opsis.
Lihat [[Special:IPBlockList|daftar IP yang diblokir]] untuk daftar terkininya.',

# History merging
'mergehistory'                     => 'Gabung sejarah halaman',
'mergehistory-header'              => 'Halaman ini memperbolehkan Anda untuk menggabungkan revisi-revisi dari satu halaman sumber ke halaman yang lebih baru.
Pastikan bahwa perubahan ini tetap mempertahankan kontinuitas versi terdahulu halaman.',
'mergehistory-box'                 => 'Gabung revisi-revisi dari dua halaman:',
'mergehistory-from'                => 'Halaman sumber:',
'mergehistory-into'                => 'Halaman tujuan:',
'mergehistory-list'                => 'Mergeable edit history',
'mergehistory-merge'               => 'Revisi-revisi berikut dari [[:$1]] dapat digabungkan ke [[:$2]]. Gunakan tombol radio untuk menggabungkan revisi-revisi yang dibuat sebelum waktu tertentu. Perhatikan, menggunakan pranala navigasi akan mengeset ulang kolom.',
'mergehistory-go'                  => 'Tampilkan suntingan-suntingan yang dapat digabung',
'mergehistory-submit'              => 'Gabung revisi',
'mergehistory-empty'               => 'Tidak ada revisi yang dapat digabung.',
'mergehistory-success'             => '$3 {{PLURAL:$3|revisi|revisi}} dari [[:$1]] berhasil digabungkan ke [[:$2]].',
'mergehistory-fail'                => 'Tidak dapat melakukan penggabungan, harap periksa kembali halaman dan parameter waktu.',
'mergehistory-no-source'           => 'Halaman sumber $1 tidak ada.',
'mergehistory-no-destination'      => 'Halaman tujuan $1 tidak ada.',
'mergehistory-invalid-source'      => 'Judul halaman sumber haruslah judul yang berlaku.',
'mergehistory-invalid-destination' => 'Judul halaman tujuan haruslah judul yang valid.',
'mergehistory-autocomment'         => '[[:$1]] telah digabungkan ke [[:$2]]',
'mergehistory-comment'             => '[[:$1]] telah digabungkan ke [[:$2]]: $3',

# Merge log
'mergelog'           => 'Gabung log',
'pagemerge-logentry' => 'menggabungkan [[$1]] ke [[$2]] (revisi sampai dengan $3)',
'revertmerge'        => 'Batal penggabungan',
'mergelogpagetext'   => 'Di bawah ini adalah daftar penggabungan sejarah halaman ke halaman yang lain.',

# Diffs
'history-title'           => 'Riwayat revisi dari "$1"',
'difference'              => '(Perbedaan antarrevisi)',
'lineno'                  => 'Baris $1:',
'compareselectedversions' => 'Bandingkan versi terpilih',
'editundo'                => 'batalkan',
'diff-multi'              => '({{PLURAL:$1|Satu|$1}} revisi antara tak ditampilkan.)',

# Search results
'searchresults'             => 'Hasil pencarian',
'searchresulttext'          => 'Untuk informasi lebih lanjut tentang pencarian di {{SITENAME}}, lihat [[{{MediaWiki:Helppage}}|halaman bantuan]].',
'searchsubtitle'            => "Anda mencari '''[[:$1]]'''",
'searchsubtitleinvalid'     => "Anda mencari '''$1'''",
'noexactmatch'              => "'''Tidak ada halaman yang berjudul \"\$1\".''' Anda dapat [[:\$1|membuat halaman ini]].",
'noexactmatch-nocreate'     => "'''Tidak ada halaman berjudul \"\$1\".'''",
'toomanymatches'            => 'Pencarian menghasilkan terlalu banyak hasil, silakan masukkan kueri lain',
'titlematches'              => 'Judul artikel yang sama',
'notitlematches'            => 'Tidak ada judul halaman yang cocok',
'textmatches'               => 'Teks artikel yang cocok',
'notextmatches'             => 'Tidak ada teks halaman yang cocok',
'prevn'                     => '$1 sebelumnya',
'nextn'                     => '$1 berikutnya',
'viewprevnext'              => 'Lihat ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 kata|$2 kata}})',
'search-result-score'       => 'Relevansi: $1%',
'search-redirect'           => '(pengalihan $1)',
'search-section'            => '(bagian $1)',
'search-suggest'            => 'Mungkin maksud Anda adalah: $1',
'search-interwiki-caption'  => 'Proyek lain',
'search-interwiki-default'  => 'Hasil $1:',
'search-interwiki-more'     => '(selanjutnya)',
'search-mwsuggest-enabled'  => 'dengan saran',
'search-mwsuggest-disabled' => 'tidak ada saran',
'search-relatedarticle'     => 'Berkaitan',
'mwsuggest-disable'         => 'Non-aktifkan saran AJAX',
'searchrelated'             => 'berkaitan',
'searchall'                 => 'semua',
'showingresults'            => "Di bawah ini ditampilkan hingga {{PLURAL:$1|'''1''' hasil|'''$1''' hasil}}, dimulai dari #'''$2'''.",
'showingresultsnum'         => "Di bawah ini ditampilkan{{PLURAL:$3|'''1''' hasil|'''$3''' hasil}}, dimulai dari #'''$2'''.",
'showingresultstotal'       => "Hasil pencarian {{PLURAL:$3|'''$1''' dari '''$3'''|'''$1 - $2''' dari '''$3'''}}",
'nonefound'                 => "'''Catatan''': Kegagalan pencarian biasanya disebabkan oleh pencarian kata-kata umum dalam bahasa Inggris, seperti \"have\" dan \"from\", yang biasanya tidak diindeks, atau dengan menentukan lebih dari satu kriteria pencarian (hanya halaman yang mengandung semua kriteria pencarianlah yang akan ditampilkan dalam hasil pencarian)",
'powersearch'               => 'Pencarian lanjut',
'powersearch-legend'        => 'Pencarian lanjut',
'powersearch-ns'            => 'Mencari di ruang nama:',
'powersearch-redir'         => 'Daftar pengalihan',
'powersearch-field'         => 'Mencari',
'search-external'           => 'Pencarian eksternal',
'searchdisabled'            => '<p style="margin: 1.5em 2em 1em">Mesin pencari {{SITENAME}} sementara dimatikan karena masalah kinerja. Anda dapat mencari melalui Google untuk sementara waktu. <span style="font-size: 89%; display: block; margin-left: .2em">Indeks Google untuk {{SITENAME}} mungkin belum diperbaharui. Jika istilah pencarian berisi garis bawah, gantikan dengan spasi.</span></p>',

# Preferences page
'preferences'              => 'Preferensi',
'mypreferences'            => 'Preferensi saya',
'prefs-edits'              => 'Jumlah suntingan:',
'prefsnologin'             => 'Belum masuk log',
'prefsnologintext'         => 'Anda harus <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} masuk log]</span> untuk mengeset preferensi Anda.',
'prefsreset'               => 'Preferensi telah dikembalikan ke konfigurasi baku.',
'qbsettings'               => 'Pengaturan bar pintas',
'qbsettings-none'          => 'Tidak ada',
'qbsettings-fixedleft'     => 'Tetap sebelah kiri',
'qbsettings-fixedright'    => 'Tetap sebelah kanan',
'qbsettings-floatingleft'  => 'Mengambang sebelah kiri',
'qbsettings-floatingright' => 'Mengambang sebelah kanan',
'changepassword'           => 'Ganti kata sandi',
'skin'                     => 'Kulit',
'math'                     => 'Matematika',
'dateformat'               => 'Format tanggal',
'datedefault'              => 'Tak ada preferensi',
'datetime'                 => 'Tanggal dan waktu',
'math_failure'             => 'Gagal memparse',
'math_unknown_error'       => 'Kesalahan yang tidak diketahui',
'math_unknown_function'    => 'fungsi yang tidak diketahui',
'math_lexing_error'        => 'kesalahan lexing',
'math_syntax_error'        => 'kesalahan sintaks',
'math_image_error'         => 'Konversi PNG gagal; periksa apakah latex, dvips, gs, dan convert terinstal dengan benar',
'math_bad_tmpdir'          => 'Tidak dapat menulisi atau membuat direktori sementara math',
'math_bad_output'          => 'Tidak dapat menulisi atau membuat direktori keluaran math',
'math_notexvc'             => 'Executable texvc hilang; silakan lihat math/README untuk cara konfigurasi.',
'prefs-personal'           => 'Profil',
'prefs-rc'                 => 'Perubahan terbaru',
'prefs-watchlist'          => 'Pemantauan',
'prefs-watchlist-days'     => 'Jumlah hari maksimum yang ditampilkan di daftar pantauan:',
'prefs-watchlist-edits'    => 'Jumlah suntingan maksimum yang ditampilkan di daftar pantauan yang lebih lengkap:',
'prefs-misc'               => 'Lain-lain',
'saveprefs'                => 'Simpan',
'resetprefs'               => 'Batalkan perubahan',
'oldpassword'              => 'Kata sandi lama:',
'newpassword'              => 'Kata sandi baru:',
'retypenew'                => 'Ketik ulang kata sandi baru:',
'textboxsize'              => 'Penyuntingan',
'rows'                     => 'Baris:',
'columns'                  => 'Kolom:',
'searchresultshead'        => 'Pencarian',
'resultsperpage'           => 'Hasil per halaman:',
'contextlines'             => 'Baris ditampilkan per hasil:',
'contextchars'             => 'Karakter untuk konteks per baris:',
'stub-threshold'           => 'Ambang batas untuk format <a href="#" class="stub">pranala rintisan</a>:',
'recentchangesdays'        => 'Jumlah hari yang ditampilkan di perubahan terbaru:',
'recentchangescount'       => 'Jumlah suntingan yang ditampilkan di perubahan terbaru:',
'savedprefs'               => 'Preferensi Anda telah disimpan',
'timezonelegend'           => 'Zona waktu',
'timezonetext'             => 'Masukkan perbedaan waktu (dalam jam) antara waktu setempat dengan waktu server (UTC).',
'localtime'                => 'Waktu setempat',
'timezoneoffset'           => 'Perbedaan:',
'servertime'               => 'Waktu server sekarang adalah',
'guesstimezone'            => 'Isikan dari penjelajah web',
'allowemail'               => 'Ijinkan pengguna lain mengirim surat-e',
'prefs-searchoptions'      => 'Opsi pencarian',
'prefs-namespaces'         => 'Ruang nama',
'defaultns'                => 'Cari dalam ruang nama berikut ini secara baku:',
'default'                  => 'baku',
'files'                    => 'Berkas',

# User rights
'userrights'                  => 'Manajemen hak pengguna', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Mengatur kelompok pengguna',
'userrights-user-editname'    => 'Masukkan nama pengguna:',
'editusergroup'               => 'Sunting kelompok pengguna',
'editinguser'                 => "Mengganti hak akses pengguna '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Sunting kelompok pengguna',
'saveusergroups'              => 'Simpan kelompok pengguna',
'userrights-groupsmember'     => 'Anggota dari:',
'userrights-groups-help'      => 'Anda dapat mengubah kelompok pengguna ini:
* Kotak dengan tanda cek merupakan kelompok pengguna yang bersangkutan
* Kotak tanpa tanda cek berarti pengguna ini bukan anggota kelompok tersebut
* Tanda * menandai bahwa Anda tidak dapat membatalkan kelompok tersebut bila Anda telah menambahkannya, atau sebaliknya.',
'userrights-reason'           => 'Alasan pengubahan:',
'userrights-no-interwiki'     => 'Anda tidak memiliki hak untuk mengubah hak pengguna di wiki yang lain.',
'userrights-nodatabase'       => 'Basis data $1 tidak ada atau bukan lokal.',
'userrights-nologin'          => 'Anda harus [[Special:UserLogin|masuk log]] dengan menggunakan akun pengurus untuk dapat mengubah hak pengguna.',
'userrights-notallowed'       => 'Anda tidak berhak untuk mengubah hak pengguna',
'userrights-changeable-col'   => 'Kelompok yang dapat Anda ubah',
'userrights-unchangeable-col' => 'Kelompok yang tidak dapat Anda ubah',

# Groups
'group'               => 'Kelompok:',
'group-user'          => 'Pengguna',
'group-autoconfirmed' => 'Pengguna yang otomatis dikonfirmasi',
'group-bot'           => 'Bot',
'group-sysop'         => 'Pengurus',
'group-bureaucrat'    => 'Birokrat',
'group-suppress'      => 'Oversights',
'group-all'           => '(semua)',

'group-user-member'          => 'Pengguna',
'group-autoconfirmed-member' => 'Pengguna yang otomatis dikonfirmasi',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Pengurus',
'group-bureaucrat-member'    => 'Birokrat',
'group-suppress-member'      => 'Oversight',

'grouppage-user'          => '{{ns:project}}:Pengguna',
'grouppage-autoconfirmed' => '{{ns:project}}:Pengguna yang otomatis dikonfirmasi',
'grouppage-bot'           => '{{ns:project}}:Bot',
'grouppage-sysop'         => '{{ns:project}}:Pengurus',
'grouppage-bureaucrat'    => '{{ns:project}}:Birokrat',
'grouppage-suppress'      => '{{ns:project}}:Oversight',

# Rights
'right-read'                 => 'Membaca halaman',
'right-edit'                 => 'Menyunting halaman',
'right-createpage'           => 'Membuat halaman baru (yang bukan halaman pembicaraan)',
'right-createtalk'           => 'Membuat halaman pembicaraan',
'right-createaccount'        => 'Membuat akun baru',
'right-minoredit'            => 'Menandai suntingan sebagai minor',
'right-move'                 => 'Memindahkan halaman',
'right-move-subpages'        => 'Pindahkan halaman dengan seluruh sub halamannya',
'right-suppressredirect'     => 'Tidak membuat pengalihan dari nama lama ketika memindahkan halaman',
'right-upload'               => 'Memuat berkas',
'right-reupload'             => 'Menimpa berkas yang sudah ada',
'right-reupload-own'         => 'Menimpa berkas yang sudah ada yang dimuat oleh pengguna yang sama',
'right-reupload-shared'      => 'Menolak berkas-berkas pada penyimpanan media lokal bersama',
'right-upload_by_url'        => 'Memuatkan file dari sebuah alamat URL',
'right-purge'                => 'Menghapus singgahan suatu halaman tanpa halaman konfirmasi',
'right-autoconfirmed'        => 'Menyunting halaman yang semi dilindungi',
'right-bot'                  => 'Diperlakukan sebagai sebuah proses otomatis',
'right-nominornewtalk'       => 'Ketiadaan suntingan kecil di halaman pembicaraan memicu tampilan pesan baru',
'right-apihighlimits'        => 'Menggunakan batasan yang lebih tinggi dalam kueri API',
'right-writeapi'             => 'Menggunakan API penulisan',
'right-delete'               => 'Menghapus halaman',
'right-bigdelete'            => 'Menghapus halaman dengan banyak versi terdahulu',
'right-deleterevision'       => 'Menghapus dan membatalkan penghapusan revisi tertentu halaman',
'right-deletedhistory'       => 'Melihat entri-entri revisi yang dihapus, tanpa teks yang berhubungan',
'right-browsearchive'        => 'Mencari halaman yang telah dihapus',
'right-undelete'             => 'Mengembalikan halaman yang telah dihapus',
'right-suppressrevision'     => 'Memeriksa dan mengembalikan revisi-revisi yang disembunyikan dari Opsis',
'right-suppressionlog'       => 'Melihat log privat',
'right-block'                => 'Memblokir penyuntingan oleh pengguna lain',
'right-blockemail'           => 'Memblokir pengiriman surat-e oleh pengguna',
'right-hideuser'             => 'Memblokir nama pengguna dan menyembunyikannya dari publik',
'right-ipblock-exempt'       => 'Abaikan pemblokiran IP, pemblokiran otomatis, dan rentang pemblokiran',
'right-proxyunbannable'      => 'Abaikan pemblokiran otomatis atas proxy',
'right-protect'              => 'Mengubah tingkat perlindungan dan menyunting halaman yang dilindungi',
'right-editprotected'        => 'Menyunting halaman yang dilindungi (tanpa perlindungan runtun)',
'right-editinterface'        => 'Menyunting antarmuka pengguna',
'right-editusercssjs'        => 'Menyunting arsip CSS dan JS pengguna lain',
'right-rollback'             => 'Mengembalikan dengan cepat suntingan-suntingan pengguna terakhir yang menyunting halaman tertentu',
'right-markbotedits'         => 'Menandai pengembalian revisi sebagai suntingan bot',
'right-noratelimit'          => 'Tidak dipengaruhi oleh pembatasan jumlah suntingan.',
'right-import'               => 'Mengimpor halaman dari wiki lain',
'right-importupload'         => 'Mengimpor halaman dari sebuah berkas yang dimuatkan',
'right-patrol'               => 'Menandai suntingan pengguna lain sebagai terpatroli',
'right-autopatrol'           => 'Menyunting dengan status suntingan secara otomatis ditandai terpantau',
'right-patrolmarks'          => 'Melihat penandaan patroli perubahan terbaru',
'right-unwatchedpages'       => 'Melihat daftar halaman-halaman yang tidak dipantau',
'right-trackback'            => 'Mengirimkan sebuah penjejakan balik',
'right-mergehistory'         => 'Menggabungkan versi terdahulu halaman-halaman',
'right-userrights'           => 'Menyunting seluruh hak pengguna',
'right-userrights-interwiki' => 'Menyunting hak para pengguna di wiki lain',
'right-siteadmin'            => 'Mengunci dan membuka kunci basis data',

# User rights log
'rightslog'      => 'Log perubahan hak akses',
'rightslogtext'  => 'Di bawah ini adalah log perubahan terhadap hak-hak pengguna.',
'rightslogentry' => 'mengganti keanggotaan kelompok untuk $1 dari $2 menjadi $3',
'rightsnone'     => '(tidak ada)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|perubahan|perubahan}}',
'recentchanges'                     => 'Perubahan terbaru',
'recentchangestext'                 => "Temukan perubahan terbaru dalam wiki di halaman ini. Keterangan: (beda) = perubahan, (versi) = sejarah revisi, '''B''' = halaman baru, '''k''' = suntingan kecil, '''b''' = suntingan bot, (± ''bita'') = jumlah penambahan/pengurangan isi, → = suntingan bagian, ← = ringkasan otomatis.
----",
'recentchanges-feed-description'    => 'Temukan perubahan terbaru dalam wiki di umpan ini.',
'rcnote'                            => "Berikut {{PLURAL:$1|adalah '''1''' perubahan terbaru|adalah '''$1''' perubahan terbaru}} dalam {{PLURAL:$2|'''1''' hari|'''$2''' hari}} terakhir, sampai $5, $4.",
'rcnotefrom'                        => 'Di bawah ini adalah perubahan sejak <strong>$2</strong> (ditampilkan sampai <strong>$1</strong> perubahan).',
'rclistfrom'                        => 'Perlihatkan perubahan terbaru sejak $1',
'rcshowhideminor'                   => '$1 suntingan kecil',
'rcshowhidebots'                    => '$1 bot',
'rcshowhideliu'                     => '$1 pengguna masuk log',
'rcshowhideanons'                   => '$1 pengguna anon',
'rcshowhidepatr'                    => '$1 suntingan terpatroli',
'rcshowhidemine'                    => '$1 suntingan saya',
'rclinks'                           => 'Perlihatkan $1 perubahan terbaru dalam $2 hari terakhir<br />$3',
'diff'                              => 'beda',
'hist'                              => 'versi',
'hide'                              => 'Sembunyikan',
'show'                              => 'Tampilkan',
'minoreditletter'                   => 'k',
'newpageletter'                     => 'B',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|pemantau|pemantau}}]',
'rc_categories'                     => 'Batasi sampai kategori (dipisah dengan "|")',
'rc_categories_any'                 => 'Apapun',
'newsectionsummary'                 => '/* $1 */ bagian baru',

# Recent changes linked
'recentchangeslinked'          => 'Perubahan terkait',
'recentchangeslinked-title'    => 'Perubahan yang terkait dengan "$1"',
'recentchangeslinked-noresult' => 'Tidak terjadi perubahan pada halaman-halaman terkait selama periode yang telah ditentukan.',
'recentchangeslinked-summary'  => "Halaman istimewa ini memberikan daftar perubahan terakhir pada halaman-halaman terkait. Halaman yang Anda pantau ditandai dengan '''cetak tebal'''.",
'recentchangeslinked-page'     => 'Nama halaman:',
'recentchangeslinked-to'       => 'Perlihatkan perubahan dari halaman-halaman yang terhubung dengan halaman yang disajikan',

# Upload
'upload'                      => 'Pemuatan',
'uploadbtn'                   => 'Muatkan berkas',
'reupload'                    => 'Muat ulang',
'reuploaddesc'                => 'Kembali ke formulir pemuatan',
'uploadnologin'               => 'Belum masuk log',
'uploadnologintext'           => 'Anda harus [[Special:UserLogin|masuk log]] untuk dapat memuatkan berkas.',
'upload_directory_missing'    => 'Direktori pemuatan ($1) tidak ditemukan dan tidak dapat dibuat oleh server web.',
'upload_directory_read_only'  => 'Direktori pemuatan ($1) tidak dapat ditulis oleh server web.',
'uploaderror'                 => 'Kesalahan pemuatan',
'uploadtext'                  => "Gunakan formulir di bawah untuk memuat berkas.
Untuk menampilkan atau mencari berkas-berkas yang sebelumnya dimuatkan, gunakan [[Special:ImageList|daftar pemuatan berkas]]. Pemuatan dan pemuatan kembali juga dicatat dalam [[Special:Log/upload|log pemuatan]]. Penghapusan berkas dicatat dalam [[Special:Log/delete|log penghapusan]].

Untuk menampilkan atau menyertakan berkas/gambar pada suatu halaman, gunakan pranala dengan salah satu format di bawah ini:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Berkas.jpg]]</nowiki></tt>''' untuk menampilkan berkas dalam ukuran aslinya
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Berkas.png|200px|thumb|left|teks alternatif]]</nowiki></tt>''' untuk menampilkan berkas dengan lebar 200px dalam sebuah kotak di kiri artikel dengan 'teks alternatif' sebagai keterangan gambar
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Berkas.ogg]]</nowiki></tt>''' sebagai pranala langsung ke berkas yang dimaksud tanpa menampilkan berkas tersebut melalui wiki",
'upload-permitted'            => 'Jenis berkas yang diijinkan: $1.',
'upload-preferred'            => 'Jenis berkas yang disarankan: $1.',
'upload-prohibited'           => 'Jenis berkas yang dilarang: $1.',
'uploadlog'                   => 'log pemuatan',
'uploadlogpage'               => 'Log pemuatan',
'uploadlogpagetext'           => 'Di bawah ini adalah log pemuatan berkas. Semua waktu yang ditunjukkan adalah waktu server.',
'filename'                    => 'Nama berkas',
'filedesc'                    => 'Ringkasan',
'fileuploadsummary'           => 'Ringkasan:',
'filestatus'                  => 'Status hak cipta:',
'filesource'                  => 'Sumber:',
'uploadedfiles'               => 'Berkas yang telah dimuat',
'ignorewarning'               => 'Abaikan peringatan dan langsung simpan berkas.',
'ignorewarnings'              => 'Abaikan peringatan apapun',
'minlength1'                  => 'Nama berkas paling tidak harus terdiri dari satu huruf.',
'illegalfilename'             => 'Nama berkas "$1" mengandung aksara yang tidak diperbolehkan ada dalam judul halaman. Silakan ubah nama berkas tersebut dan cobalah memuatkannya kembali.',
'badfilename'                 => 'Nama berkas telah diubah menjadi "$1".',
'filetype-badmime'            => 'Berkas dengan tipe MIME "$1" tidak diperkenankan untuk dimuat.',
'filetype-unwanted-type'      => "'''\".\$1\"''' termasuk jenis berkas yang tidak diijinkan.
{{PLURAL:\$3|Jenis berkas yang disarankan adalah|Jenis berkas yang disarankan adalah}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' termasuk dalam jenis berkas yang tidak diijinkan.
{{PLURAL:\$3|Jenis berkas yang diijinkan adalah|Jenis berkas yang diijinkan adalah}} \$2.",
'filetype-missing'            => 'Berkas tak memiliki ekstensi (misalnya ".jpg").',
'large-file'                  => 'Ukuran berkas disarankan untuk tidak melebihi $1 bita; berkas ini berukuran $2 bita.',
'largefileserver'             => 'Berkas ini lebih besar dari pada yang diizinkan server.',
'emptyfile'                   => 'Berkas yang Anda muatkan kelihatannya kosong. Hal ini mungkin disebabkan karena adanya kesalahan ketik pada nama berkas. Silakan pastikan apakah Anda benar-benar ingin memuatkan berkas ini.',
'fileexists'                  => 'Suatu berkas dengan nama tersebut telah ada, harap periksa <strong><tt>$1</tt></strong> jika Anda tidak yakin untuk mengubahnya.',
'filepageexists'              => 'Halaman deskripsi untuk berkas ini telah dibuat di <strong><tt>$1</tt></strong>, tapi saat ini tak ditemukan berkas dengan nama tersebut. Ringkasan yang Anda masukkan tidak akan tampil pada halaman deskripsi. Untuk memunculkannya, Anda perlu untuk menyuntingnya secara manual',
'fileexists-extension'        => 'Berkas dengan nama serupa telah ada:<br />
Nama berkas yang akan dimuat: <strong><tt>$1</tt></strong><br />
Nama berkas yang telah ada: <strong><tt>$2</tt></strong><br />
Satu-satunya perbedaan adalah pada kapitalisasi ekstensi. Harap cek apakah berkas tersebut sama.',
'fileexists-thumb'            => "<center>'''Berkas yang tersedia'''</center>",
'fileexists-thumbnail-yes'    => 'Berkas ini tampaknya merupakan gambar yang ukurannya diperkecil <i>(miniatur)</i>.
Harap periksa berkas <strong><tt>$1</tt></strong> tersebut.<br />
Jika berkas tersebut memang merupakan gambar dalam ukuran aslinya, Anda tidak perlu untuk memuat kembali miniatur lainnya.',
'file-thumbnail-no'           => 'Nama berkas dimulai dengan <strong><tt>$1</tt></strong>.
Tampaknya berkas ini merupakan gambar dengan ukuran diperkecil <i>(miniatur)</i>.
Jika Anda memiliki versi resolusi penuh dari gambar ini, harap muatkan berkas tersebut. Jika tidak, harap ubah nama berkas ini.',
'fileexists-forbidden'        => 'Ditemukan berkas dengan nama yang sama;
harap kembali dan muatkan berkas dengan nama lain. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Ditemukan berkas lain dengan nama yang sama di repositori bersama.
Jika Anda tetap ingin memuatkan berkas Anda, harap kembali dan gunakan nama lain. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Berkas ini berduplikasi dengan {{PLURAL:$1|berkas|berkas-berkas}} berikut:',
'successfulupload'            => 'Berhasil dimuat',
'uploadwarning'               => 'Peringatan pemuatan',
'savefile'                    => 'Simpan berkas',
'uploadedimage'               => 'memuat "[[$1]]"',
'overwroteimage'              => 'memuat versi baru dari "[[$1]]"',
'uploaddisabled'              => 'Maaf, fasilitas pemuatan dimatikan.',
'uploaddisabledtext'          => 'Pemuatan berkas di tidak diizinkan di wiki ini.',
'uploadscripted'              => 'Berkas ini mengandung HTML atau kode yang dapat diinterpretasikan dengan keliru oleh penjelajah web.',
'uploadcorrupt'               => 'Berkas tersebut rusak atau ekstensinya salah. Silakan periksa berkas tersebut dan muatkan kembali.',
'uploadvirus'                 => 'Berkas tersebut mengandung virus! Detil: $1',
'sourcefilename'              => 'Nama berkas sumber:',
'destfilename'                => 'Nama berkas tujuan:',
'upload-maxfilesize'          => 'Ukuran file maksimum: $1',
'watchthisupload'             => 'Pantau halaman ini',
'filewasdeleted'              => 'Suatu berkas dengan nama ini pernah dimuat dan selanjutnya dihapus. Harap cek $1 sebelum memuat lagi berkas tersebut.',
'upload-wasdeleted'           => "'''Peringatan: Anda memuat suatu berkas yang telah pernah dihapus.'''

Anda harus mempertimbangkan apakah perlu untuk melanjutkan pemuatan berkas ini.
Log penghapusan berkas adalah sebagai berikut:",
'filename-bad-prefix'         => 'Nama berkas yang Anda muat diawali dengan <strong>"$1"</strong>, yang merupakan nama non-deskriptif yang biasanya diberikan secara otomatis oleh kamera digital. Harap pilih nama lain yang lebih deskriptif untuk berkas Anda.',

'upload-proto-error'      => 'Protokol tak tepat',
'upload-proto-error-text' => 'Pemuatan jarak jauh membutuhkan URL yang diawali dengan <code>http://</code> atau <code>ftp://</code>.',
'upload-file-error'       => 'Kesalahan internal',
'upload-file-error-text'  => 'Suatu kesalahan internal terjadi sewaktu mencoba membuat berkas temporer di server. Silakan kontak administrator sistem.',
'upload-misc-error'       => 'Kesalahan pemuatan yang tak dikenal',
'upload-misc-error-text'  => 'Suatu kesalahan yang tak dikenal terjadi sewaktu pemuatan. Harap pastikan bahwa URL tersebut valid dan dapat diakses dan silakan coba lagi. Jika masalah ini tetap terjadi, kontak administrator sistem.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL tidak dapat dihubungi',
'upload-curl-error6-text'  => 'URL yang diberikan tak dapat dihubungi. Harap periksa ulang bahwa URL tersebut tepat dan situs itu sedang aktif.',
'upload-curl-error28'      => 'Pemuatan lewat waktu',
'upload-curl-error28-text' => 'Situs yang dituju terlalu lambat merespon. Tolong cek apakah situs tersebut aktif, tunggu sebentar, dan coba lagi. Mungkin Anda perlu mencobanya di saat yang lebih longgar.',

'license'            => 'Jenis lisensi:',
'nolicense'          => 'Belum dipilih',
'license-nopreview'  => '(Pratayang tak tersedia)',
'upload_source_url'  => ' (suatu URL valid yang dapat diakses publik)',
'upload_source_file' => ' (suatu berkas di komputer Anda)',

# Special:ImageList
'imagelist-summary'     => 'Halaman istimewa ini menampilkan semua berkas yang telah dimuat.
Secara baku, berkas yang terakhir dimuat berada pada urutan teratas.
Klik pada kepala kolom untuk mengubah urutan.',
'imagelist_search_for'  => 'Cari nama berkas:',
'imgfile'               => 'berkas',
'imagelist'             => 'Daftar berkas',
'imagelist_date'        => 'Tanggal',
'imagelist_name'        => 'Nama',
'imagelist_user'        => 'Pengguna',
'imagelist_size'        => 'Besar',
'imagelist_description' => 'Deskripsi',

# Image description page
'filehist'                       => 'Riwayat berkas',
'filehist-help'                  => 'Klik pada tanggal/waktu untuk melihat berkas ini pada saat tersebut.',
'filehist-deleteall'             => 'hapus semua',
'filehist-deleteone'             => 'hapus ini',
'filehist-revert'                => 'kembalikan',
'filehist-current'               => 'saat ini',
'filehist-datetime'              => 'Tanggal/Waktu',
'filehist-user'                  => 'Pengguna',
'filehist-dimensions'            => 'Dimensi',
'filehist-filesize'              => 'Besar berkas',
'filehist-comment'               => 'Komentar',
'imagelinks'                     => 'Pranala',
'linkstoimage'                   => 'Halaman berikut memiliki {{PLURAL:$1|pranala|$1 pranala}} ke berkas ini:',
'nolinkstoimage'                 => 'Tidak ada halaman yang memiliki pranala ke berkas ini.',
'morelinkstoimage'               => 'Lihat [[Special:WhatLinksHere/$1|pranala lainnya]] ke berkas ini.',
'redirectstofile'                => 'Berkas berikut {{PLURAL:$1|dialihkan|$1 dialihkan}} ke berkas ini:',
'duplicatesoffile'               => 'Berkas berikut {{PLURAL:$1|merupakan duplikat|$1 merupakan duplikat}} dari berkas ini:',
'sharedupload'                   => 'Berkas ini adalah pemuatan bersama yang mungkin juga dipakai oleh proyek lain.',
'shareduploadwiki'               => 'Lihat $1 untuk informasi lebih lanjut.',
'shareduploadwiki-desc'          => 'Deskripsi pada $1 ditampilkan di bawah.',
'shareduploadwiki-linktext'      => 'halaman deskripsi berkas',
'shareduploadduplicate'          => 'Berkas ini berduplikasi dengan $1 dari tempat penyimpanan bersama.',
'shareduploadduplicate-linktext' => 'berkas lain',
'shareduploadconflict'           => 'Berkas ini memiliki nama yang sama dengan $1 dari tempat penyimpanan bersama.',
'shareduploadconflict-linktext'  => 'berkas lain',
'noimage'                        => 'Tidak ada berkas dengan nama tersebut, tetapi Anda dapat $1.',
'noimage-linktext'               => 'memuat berkas',
'uploadnewversion-linktext'      => 'Muatkan versi yang lebih baru dari berkas ini',
'imagepage-searchdupe'           => 'Cari berkas duplikat',

# File reversion
'filerevert'                => 'Kembalikan $1',
'filerevert-legend'         => 'Kembalikan berkas',
'filerevert-intro'          => '<span class="plainlinks">Anda mengembalikan \'\'\'[[Media:$1|$1]]\'\'\' ke [versi $4 pada $3, $2].</span>',
'filerevert-comment'        => 'Komentar:',
'filerevert-defaultcomment' => 'Dikembalikan ke versi pada $2, $1',
'filerevert-submit'         => 'Kembalikan',
'filerevert-success'        => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\' telah dikembalikan ke [versi $4 pada $3, $2].</span>',
'filerevert-badversion'     => 'Tidak ada versi lokal terdahulu dari berkas ini dengan stempel waktu yang dimaksud.',

# File deletion
'filedelete'                  => 'Menghapus $1',
'filedelete-legend'           => 'Menghapus berkas',
'filedelete-intro'            => "Anda menghapus '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => '<span class="plainlinks">Anda menghapus versi \'\'\'[[Media:$1|$1]]\'\'\' hingga [$4 $3, $2].</span>',
'filedelete-comment'          => 'Komentar:',
'filedelete-submit'           => 'Hapus',
'filedelete-success'          => "'''$1''' telah dihapus.",
'filedelete-success-old'      => "Berkas '''[[Media:$1|$1]]''' versi $3, $2 telah dihapus.",
'filedelete-nofile'           => "'''$1''' tak ditemukan pada situs ini.",
'filedelete-nofile-old'       => "Tak ditemukan arsip versi dari '''$1''' dengan atribut yang diberikan.",
'filedelete-iscurrent'        => 'Anda mencoba menghapus versi terakhir berkas ini. Harap kembalikan dulu ke versi lama.',
'filedelete-otherreason'      => 'Alasan lain:',
'filedelete-reason-otherlist' => 'Alasan lain',
'filedelete-reason-dropdown'  => '*Alasan penghapusan
** Pelanggaran hak cipta
** Berkas duplikat',
'filedelete-edit-reasonlist'  => 'Alasan penghapusan suntingan',

# MIME search
'mimesearch'         => 'Pencarian MIME',
'mimesearch-summary' => 'Halaman ini menyediakan fasilitas menyaring berkas berdasarkan tipe MIME nya. Masukkan: contenttype/subtype, misalnya <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipe MIME:',
'download'           => 'unduh',

# Unwatched pages
'unwatchedpages' => 'Halaman yang tak dipantau',

# List redirects
'listredirects' => 'Daftar pengalihan',

# Unused templates
'unusedtemplates'     => 'Templat yang tak digunakan',
'unusedtemplatestext' => 'Daftar berikut adalah halaman pada ruang nama templat yang tidak dipakai di halaman manapun. Cek dahulu pranala lain ke templat tersebut sebelum menghapusnya.',
'unusedtemplateswlh'  => 'pranala lain',

# Random page
'randompage'         => 'Halaman sembarang',
'randompage-nopages' => 'Tak terdapat halaman pada ruang nama ini.',

# Random redirect
'randomredirect'         => 'Pengalihan sembarang',
'randomredirect-nopages' => 'Tak terdapat pengalihan pada ruang nama ini.',

# Statistics
'statistics'             => 'Statistik',
'sitestats'              => 'Statistik situs',
'userstats'              => 'Statistik pengguna',
'sitestatstext'          => "{{SITENAME}} saat ini memiliki '''$2''' {{PLURAL:$1|halaman|halaman}} yang termasuk artikel yang sah. Jumlah tersebut tidak memperhitungkan halaman pembicaraan, halaman tentang {{SITENAME}}, halaman rintisan minimum, halaman peralihan, dan halaman-halaman lain yang tidak masuk dalam kriteria artikel. Jika termasuk halaman-halaman ini, terdapat total '''$1''' halaman dalam basis data.

Telah terjadi sejumlah '''$3''' penampilan halaman dan '''$4''' penyuntingan sejak {{SITENAME}} dimulai. Ini berarti rata-rata '''$5''' suntingan per halaman, dan '''$6''' penampilan per penyuntingan.

Telah dimuat sejumlah '''$8''' berkas dan sedang terjadi '''$7''' [http://www.mediawiki.org/wiki/Manual:Job_queue antrian pekerjaan].",
'userstatstext'          => "Terdapat {{PLURAL:$1|'''1''' [[Special:ListUsers|pengguna terdaftar]]|'''$1''' [[Special:ListUsers|pengguna terdaftar]]}}, dengan '''$2''' (atau '''$4%''') di antaranya memiliki hak akses $5.",
'statistics-mostpopular' => 'Halaman yang paling banyak ditampilkan',

'disambiguations'      => 'Halaman disambiguasi',
'disambiguationspage'  => 'Template:Disambig',
'disambiguations-text' => "Halaman-halaman berikut memiliki pranala ke suatu '''halaman disambiguasi'''.
Halaman-halaman tersebut seharusnya berpaut ke topik-topik yang sesuai.<br />
Suatu halaman dianggap sebagai halaman disambiguasi apabila halaman tersebut menggunakan templat yang terhubung ke [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Pengalihan ganda',
'doubleredirectstext'        => 'Halaman ini memuat daftar halaman yang beralih ke halaman pengalihan yang lain. Setiap baris memuat pranala ke pengalihan pertama dan pengalihan kedua serta target dari pengalihan kedua yang umumnya adalah halaman yang sebenarnya. Halaman peralihan pertama seharusnya dialihkan ke halaman target tersebut.',
'double-redirect-fixed-move' => '[[$1]] telah dipindahkan menjadi halaman peralihan ke [[$2]]',
'double-redirect-fixer'      => 'Revisi pengalihan',

'brokenredirects'        => 'Pengalihan rusak',
'brokenredirectstext'    => 'Halaman-halaman berikut dialihkan ke halaman yang tidak ada.',
'brokenredirects-edit'   => '(sunting)',
'brokenredirects-delete' => '(hapus)',

'withoutinterwiki'         => 'Halaman tanpa interwiki',
'withoutinterwiki-summary' => 'Halaman-halaman berikut tidak memiliki pranala ke versi dalam bahasa lain:',
'withoutinterwiki-legend'  => 'Prefiks',
'withoutinterwiki-submit'  => 'Tampilkan',

'fewestrevisions' => 'Artikel dengan perubahan tersedikit',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bita|bita}}',
'ncategories'             => '$1 {{PLURAL:$1|kategori|kategori}}',
'nlinks'                  => '$1 {{PLURAL:$1|pranala|pranala}}',
'nmembers'                => '$1 {{PLURAL:$1|isi|isi}}',
'nrevisions'              => '$1 {{PLURAL:$1|revisi|revisi}}',
'nviews'                  => '$1 {{PLURAL:$1|penampilan|penampilan}}',
'specialpage-empty'       => 'Tak ada yang perlu dilaporkan.',
'lonelypages'             => 'Halaman tanpa pranala balik',
'lonelypagestext'         => 'Halaman-halaman berikut tidak memiliki pranala dari halaman manapun di wiki ini.',
'uncategorizedpages'      => 'Halaman yang tak terkategori',
'uncategorizedcategories' => 'Kategori yang tak terkategori',
'uncategorizedimages'     => 'Berkas yang tak terkategori',
'uncategorizedtemplates'  => 'Templat yang tak terkategori',
'unusedcategories'        => 'Kategori yang tak digunakan',
'unusedimages'            => 'Berkas yang tak digunakan',
'popularpages'            => 'Halaman populer',
'wantedcategories'        => 'Kategori yang diinginkan',
'wantedpages'             => 'Halaman yang diinginkan',
'missingfiles'            => 'File-file hilang',
'mostlinked'              => 'Halaman yang tersering dituju',
'mostlinkedcategories'    => 'Kategori yang tersering digunakan',
'mostlinkedtemplates'     => 'Templat yang tersering digunakan',
'mostcategories'          => 'Artikel dengan kategori terbanyak',
'mostimages'              => 'Berkas yang tersering digunakan',
'mostrevisions'           => 'Artikel dengan perubahan terbanyak',
'prefixindex'             => 'Daftar halaman dengan awalan',
'shortpages'              => 'Halaman pendek',
'longpages'               => 'Halaman panjang',
'deadendpages'            => 'Halaman buntu',
'deadendpagestext'        => 'Halaman-halaman berikut tidak memiliki pranala ke halaman manapun di wiki ini.',
'protectedpages'          => 'Halaman yang dilindungi',
'protectedpages-indef'    => 'Hanya untuk perlindungan dengan jangka waktu tak terbatas',
'protectedpagestext'      => 'Halaman-halaman berikut dilindungi dari pemindahan atau penyuntingan.',
'protectedpagesempty'     => 'Saat ini tidak ada halaman yang sedang dilindungi dengan parameter-parameter tersebut.',
'protectedtitles'         => 'Judul yang dilindungi',
'protectedtitlestext'     => 'Judul berikut ini dilindungi dari pembuatan',
'protectedtitlesempty'    => 'Tidak ada judul yang dilindungi.',
'listusers'               => 'Daftar pengguna',
'newpages'                => 'Halaman baru',
'newpages-username'       => 'Nama pengguna:',
'ancientpages'            => 'Artikel lama',
'move'                    => 'Pindahkan',
'movethispage'            => 'Pindahkan halaman ini',
'unusedimagestext'        => '<p>Perhatikan bahwa situs web lain mungkin dapat berpaut ke sebuah berkas secara langsung, dan berkas-berkas seperti itu mungkin terdapat dalam daftar ini meskipun masih digunakan oleh situs web lain.',
'unusedcategoriestext'    => 'Kategori berikut ada walaupun tidak ada artikel atau kategori lain yang menggunakannya.',
'notargettitle'           => 'Tidak ada sasaran',
'notargettext'            => 'Anda tidak menentukan halaman atau pengguna tujuan fungsi ini.',
'nopagetitle'             => 'Halaman tujuan tidak ditemukan',
'nopagetext'              => 'Halaman yang Anda tuju tidak ditemukan.',
'pager-newer-n'           => '{{PLURAL:$1|1 lebih baru|$1 lebih baru}}',
'pager-older-n'           => '{{PLURAL:$1|1 lebih lama|$1 lebih lama}}',
'suppress'                => 'Oversight',

# Book sources
'booksources'               => 'Sumber buku',
'booksources-search-legend' => 'Cari di sumber buku',
'booksources-go'            => 'Cari',
'booksources-text'          => 'Di bawah ini adalah daftar pranala ke situs lain yang menjual buku baru dan bekas, dan mungkin juga mempunyai informasi lebih lanjut mengenai buku yang sedang Anda cari:',

# Special:Log
'specialloguserlabel'  => 'Pengguna:',
'speciallogtitlelabel' => 'Judul:',
'log'                  => 'Log',
'all-logs-page'        => 'Semua log',
'log-search-legend'    => 'Pencarian log',
'log-search-submit'    => 'Cari',
'alllogstext'          => 'Gabungan tampilan semua log yang tersedia di {{SITENAME}}.
Anda dapat melakukan pembatasan tampilan dengan memilih jenis log, nama pengguna (sensitif kapital), atau judul halaman (juga sensitif kapital).',
'logempty'             => 'Tidak ditemukan entri log yang sesuai.',
'log-title-wildcard'   => 'Cari judul yang diawali dengan teks tersebut',

# Special:AllPages
'allpages'          => 'Daftar halaman',
'alphaindexline'    => '$1 ke $2',
'nextpage'          => 'Halaman selanjutnya ($1)',
'prevpage'          => 'Halaman sebelumnya ($1)',
'allpagesfrom'      => 'Tampilkan halaman dimulai dari:',
'allarticles'       => 'Daftar artikel',
'allinnamespace'    => 'Daftar halaman (ruang nama $1)',
'allnotinnamespace' => 'Daftar halaman (bukan ruang nama $1)',
'allpagesprev'      => 'Sebelumnya',
'allpagesnext'      => 'Selanjutnya',
'allpagessubmit'    => 'Cari',
'allpagesprefix'    => 'Tampilkan halaman dengan awalan:',
'allpagesbadtitle'  => 'Judul halaman yang diberikan tidak sah atau memiliki awalan antar-bahasa atau antar-wiki. Judul tersebut mungkin juga mengandung satu atau lebih aksara yang tidak dapat digunakan dalam judul.',
'allpages-bad-ns'   => '{{SITENAME}} tidak memiliki ruang nama "$1".',

# Special:Categories
'categories'                    => 'Daftar kategori',
'categoriespagetext'            => 'Terdapat halaman-halaman atau media dalam kategori-kategori berikut.
[[Special:UnusedCategories|Kategori-kategori tanpa isi]] tidak ditampilkan di sini.
Lihat pula [[Special:WantedCategories|daftar kategori yang dibutuhkan]].',
'categoriesfrom'                => 'Tampilkan kategori-kategori dimulai dengan:',
'special-categories-sort-count' => 'urutkan menurut jumlah',
'special-categories-sort-abc'   => 'urutkan menurut abjad',

# Special:ListUsers
'listusersfrom'      => 'Tampilkan pengguna diawali dengan:',
'listusers-submit'   => 'Tampilkan',
'listusers-noresult' => 'Pengguna tidak ditemukan.',

# Special:ListGroupRights
'listgrouprights'          => 'Hak-hak grup pengguna',
'listgrouprights-summary'  => 'Berikut adalah daftar kelompok pengguna yang terdapat di wiki ini, dengan daftar hak akses mereka masing-masing. Informasi lebih lanjut mengenai hak masing-masing dapat ditemukan di [[{{MediaWiki:Listgrouprights-helppage}}|halaman bantuan hak pengguna]].',
'listgrouprights-group'    => 'Kelompok',
'listgrouprights-rights'   => 'Hak',
'listgrouprights-helppage' => 'Help:Hak kelompok',
'listgrouprights-members'  => '(daftar anggota)',

# E-mail user
'mailnologin'     => 'Tidak ada alamat surat-e',
'mailnologintext' => 'Anda harus [[Special:UserLogin|masuk log]] dan mempunyai alamat surat-e yang sah di dalam [[Special:Preferences|preferensi]] untuk mengirimkan surat-e kepada pengguna lain.',
'emailuser'       => 'Surat-e pengguna',
'emailpage'       => 'Kirimi pengguna ini surat-e',
'emailpagetext'   => 'Jika pengguna ini memasukkan alamat surat-e yang sah dalam preferensinya, formulir di bawah ini akan mengirimkan sebuah surat-e.
Alamat surat-e yg terdapat pada [[Special:Preferences|preferensi Anda]] akan muncul sebagai alamat "Dari" dalam surat-e tersebut, sehingga penerima dapat membalas surat-e tersebut langsung kepada Anda.',
'usermailererror' => 'Kesalahan objek surat:',
'defemailsubject' => 'Surat-e {{SITENAME}}',
'noemailtitle'    => 'Tidak ada alamat surat-e',
'noemailtext'     => 'Pengguna ini tidak memasukkan alamat surat-e yang sah, atau memilih untuk tidak menerima surat-e dari pengguna yang lain.',
'emailfrom'       => 'Dari:',
'emailto'         => 'Untuk:',
'emailsubject'    => 'Perihal:',
'emailmessage'    => 'Pesan:',
'emailsend'       => 'Kirim',
'emailccme'       => 'Kirimi saya salinan pesan saya.',
'emailccsubject'  => 'Salinan pesan Anda untuk $1: $2',
'emailsent'       => 'Surat-e terkirim',
'emailsenttext'   => 'Surat-e Anda telah dikirimkan.',
'emailuserfooter' => 'Surat-e ini dikirimkan oleh $1 kepada $2 menggunakan fungsi "Suratepengguna" di {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Daftar pantauan',
'mywatchlist'          => 'Pantauan saya',
'watchlistfor'         => "(untuk '''$1''')",
'nowatchlist'          => 'Daftar pantauan Anda kosong.',
'watchlistanontext'    => 'Silakan $1 untuk melihat atau menyunting daftar pantauan Anda.',
'watchnologin'         => 'Belum masuk log',
'watchnologintext'     => 'Anda harus [[Special:UserLogin|masuk log]] untuk mengubah daftar pantauan Anda.',
'addedwatch'           => 'Telah ditambahkan ke daftar pantauan',
'addedwatchtext'       => "Halaman \"[[:\$1]]\" telah ditambahkan ke [[Special:Watchlist|daftar pantauan]] Anda.
Perubahan-perubahan berikutnya pada halaman tersebut dan halaman pembicaraan terkaitnya akan tercantum di sini, dan halaman itu akan ditampilkan '''tebal''' pada [[Special:RecentChanges|daftar perubahan terbaru]] agar lebih mudah terlihat.",
'removedwatch'         => 'Telah dihapus dari daftar pantauan',
'removedwatchtext'     => 'Halaman "<nowiki>$1</nowiki>" telah dihapus dari daftar pantauan.',
'watch'                => 'Pantau',
'watchthispage'        => 'Pantau halaman ini',
'unwatch'              => 'Batal pantau',
'unwatchthispage'      => 'Batal pantau halaman ini',
'notanarticle'         => 'Bukan sebuah artikel',
'notvisiblerev'        => 'Revisi telah dihapus',
'watchnochange'        => 'Tak ada halaman pantauan Anda yang telah berubah dalam jangka waktu yang dipilih.',
'watchlist-details'    => 'Terdapat {{PLURAL:$1|$1 halaman|$1 halaman}} di daftar pantauan Anda, tidak termasuk halaman pembicaraan.',
'wlheader-enotif'      => '* Notifikasi surat-e diaktifkan.',
'wlheader-showupdated' => "* Halaman-halaman yang telah berubah sejak kunjungan terakhir Anda ditampilkan dengan '''huruf tebal'''",
'watchmethod-recent'   => 'periksa daftar perubahan terbaru terhadap halaman yang dipantau',
'watchmethod-list'     => 'periksa halaman yang dipantau terhadap perubahan terbaru',
'watchlistcontains'    => 'Daftar pantauan Anda berisi $1 {{PLURAL:$1|halaman|halaman}}.',
'iteminvalidname'      => "Ada masalah dengan '$1', namanya tidak sah...",
'wlnote'               => "Di bawah ini adalah $1 {{PLURAL:$1|perubahan|perubahan}} terakhir dalam '''$2''' jam terakhir.",
'wlshowlast'           => 'Tampilkan $1 jam $2 hari $3 terakhir',
'watchlist-show-bots'  => 'Tampilkan suntingan bot',
'watchlist-hide-bots'  => 'Sembunyikan suntingan bot',
'watchlist-show-own'   => 'Tampilkan suntingan saya',
'watchlist-hide-own'   => 'Sembunyikan suntingan saya',
'watchlist-show-minor' => 'Tampilkan suntingan kecil',
'watchlist-hide-minor' => 'Sembunyikan suntingan kecil',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Memantau...',
'unwatching' => 'Menghilangkan pemantauan...',

'enotif_mailer'                => 'Pengirim Notifikasi {{SITENAME}}',
'enotif_reset'                 => 'Tandai semua halaman sebagai telah dikunjungi',
'enotif_newpagetext'           => 'Ini adalah halaman baru.',
'enotif_impersonal_salutation' => 'Pengguna {{SITENAME}}',
'changed'                      => 'diubah',
'created'                      => 'dibuat',
'enotif_subject'               => 'Halaman $PAGETITLE di {{SITENAME}} telah $CHANGEDORCREATED oleh $PAGEEDITOR',
'enotif_lastvisited'           => 'Lihat $1 untuk semua perubahan sejak kunjungan terakhir Anda.',
'enotif_lastdiff'              => 'Kunjungi $1 untuk melihat perubahan ini.',
'enotif_anon_editor'           => 'pengguna anonim $1',
'enotif_body'                  => 'Dear $WATCHINGUSERNAME,

Halaman $PAGETITLE di {{SITENAME}} telah $CHANGEDORCREATED pada $PAGEEDITDATE oleh $PAGEEDITOR, lihat $PAGETITLE_URL untuk versi terakhir.

$NEWPAGE

Riwayat suntingan: $PAGESUMMARY $PAGEMINOREDIT

Hubungi penyunting:
mail: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Kami tidak akan mengirimkan pemberitahuan lain jika terjadi perubahan lagi, kecuali Anda jika Anda telah mengunjungi halaman tersebut. Anda juga dapat menghapus tanda notifikasi untuk semua halaman pantauan Anda pada daftar pantauan Anda.

             Sistem notifikasi {{SITENAME}}

--
Untuk mengubah preferensi daftar pantauan Anda, kunjungi
{{fullurl:{{ns:special}}:Watchlist/edit}}

Umpan balik dan bantuan lanjutan:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Hapus halaman',
'confirm'                     => 'Konfirmasikan',
'excontent'                   => "isi sebelumnya: '$1'",
'excontentauthor'             => "isinya hanya berupa: '$1' (dan satu-satunya penyumbang adalah '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "isi sebelum dikosongkan: '$1'",
'exblank'                     => 'halaman kosong',
'delete-confirm'              => 'Hapus "$1"',
'delete-legend'               => 'Hapus',
'historywarning'              => 'Peringatan: Halaman yang ingin Anda hapus mempunyai sejarah:',
'confirmdeletetext'           => 'Anda akan menghapus halaman atau berkas ini secara permanen berikut semua sejarahnya dari basis data. Pastikan bahwa Anda memang ingin melakukannya, mengetahui segala akibatnya, dan apa yang Anda lakukan ini adalah sejalan dengan [[{{MediaWiki:Policy-url}}|kebijakan {{SITENAME}}]].',
'actioncomplete'              => 'Proses selesai',
'deletedtext'                 => '"<nowiki>$1</nowiki>" telah dihapus. Lihat $2 untuk log terkini halaman yang telah dihapus.',
'deletedarticle'              => 'menghapus "[[$1]]"',
'suppressedarticle'           => '"[[$1]]" disembunyikan',
'dellogpage'                  => 'Log penghapusan',
'dellogpagetext'              => 'Di bawah ini adalah log penghapusan halaman. Semua waktu yang ditunjukkan adalah waktu server.',
'deletionlog'                 => 'log penghapusan',
'reverted'                    => 'Dikembalikan ke revisi sebelumnya',
'deletecomment'               => 'Alasan penghapusan',
'deleteotherreason'           => 'Lainnya/alasan tambahan:',
'deletereasonotherlist'       => 'Alasan lain',
'deletereason-dropdown'       => '*Alasan penghapusan
** Permintaan pengguna
** Pelanggaran hak cipta
** Vandalisme',
'delete-edit-reasonlist'      => 'Sunting alasan penghapusan',
'delete-toobig'               => 'Halaman ini memiliki sejarah penyuntingan yang panjang, melebihi {{PLURAL:$1|revisi|revisi}}.
Penghapusan halaman dengan sejarah penyuntingan yang panjang tidak diperbolehkan untuk mencegah kerusakan di {{SITENAME}}.',
'delete-warning-toobig'       => 'Halaman ini memiliki sejarah penyuntingan yang panjang, melebihi {{PLURAL:$1|revisi|revisi}}.
Menghapus halaman ini dapat menyebabkan masalah dalam operasional basis data {{SITENAME}}.',
'rollback'                    => 'Kembalikan suntingan',
'rollback_short'              => 'Kembalikan',
'rollbacklink'                => 'kembalikan',
'rollbackfailed'              => 'Pengembalian gagal dilakukan',
'cantrollback'                => 'Tidak dapat mengembalikan suntingan; pengguna terakhir adalah satu-satunya penulis artikel ini.',
'alreadyrolled'               => 'Tidak dapat melakukan pengembalian ke revisi terakhir [[:$1]] oleh [[User:$2|$2]] ([[User talk:$2|bicara]] | [[Special:Contributions/$2|{{int:contribslink}}]]);
pengguna lain telah menyunting atau melakukan pengembalian terhadap revisi tersebut.

Suntingan terakhir dilakukan oleh [[User:$3|$3]] ([[User talk:$3|bicara]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Komentar penyuntingan adalah: "<em>$1</em>".', # only shown if there is an edit comment
'revertpage'                  => 'Suntingan [[Special:Contributions/$2|$2]] ([[User talk:$2|bicara]]) dikembalikan ke versi terakhir oleh [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Pengembalian suntingan oleh $1; dikembalikan ke versi terakhir oleh $2.',
'sessionfailure'              => 'Sepertinya ada masalah dengan sesi log anda; log anda telah dibatalkan untuk mencegah pembajakan. Silahkan tekan tombol "back" dan muat kembali halaman sebelum anda masuk, lalu coba lagi.',
'protectlogpage'              => 'Log perlindungan',
'protectlogtext'              => 'Berikut adalah log perlindungan halaman dan pembatalannya.
Lihat [[Special:ProtectedPages|daftar halaman yang dilindungi]] untuk daftar terkini.',
'protectedarticle'            => 'melindungi "[[$1]]"',
'modifiedarticleprotection'   => 'mengubah tingkat perlindungan "[[$1]]"',
'unprotectedarticle'          => 'menghilangkan perlindungan "[[$1]]"',
'protect-title'               => 'Melindungi "$1"',
'protect-legend'              => 'Konfirmasi perlindungan',
'protectcomment'              => 'Komentar:',
'protectexpiry'               => 'Kadaluwarsa:',
'protect_expiry_invalid'      => 'Waktu kadaluwarsa tidak sah.',
'protect_expiry_old'          => 'Waktu kadaluwarsa adalah pada masa lampau.',
'protect-unchain'             => 'Buka perlindungan pemindahan',
'protect-text'                => 'Anda dapat melihat atau mengganti tingkatan perlindungan untuk halaman <strong><nowiki>$1</nowiki></strong> di sini.',
'protect-locked-blocked'      => 'Anda tak dapat mengganti tingkat perlindungan selagi diblokir. Berikut adalah konfigurasi saat ini untuk halaman <strong>$1</strong>:',
'protect-locked-dblock'       => 'Tingkat perlindungan tak dapat diganti karena aktifnya penguncian basis data. Berikut adalah konfigurasi saat ini untuk halaman <strong>$1</strong>:',
'protect-locked-access'       => 'Akun Anda tidak dapat memiliki hak untuk mengganti tingkat perlindungan halaman. Berikut adalah konfigurasi saat ini untuk halaman <strong>$1</strong>:',
'protect-cascadeon'           => 'Halaman ini sedang dilindungi karena disertakan dalam {{PLURAL:$1|halaman|halaman-halaman}} berikut yang telah dilindungi dengan pilihan perlindungan runtun diaktifkan. Anda dapat mengganti tingkat perlindungan untuk halaman ini, tapi hal tersebut tidak akan mempengaruhi perlindungan runtun.',
'protect-default'             => '(baku)',
'protect-fallback'            => 'Memerlukan hak akses "$1"',
'protect-level-autoconfirmed' => 'Hanya pengguna terdaftar',
'protect-level-sysop'         => 'Hanya pengurus',
'protect-summary-cascade'     => 'runtun',
'protect-expiring'            => 'kadaluwarsa $1 (UTC)',
'protect-cascade'             => 'Lindungi semua halaman yang termasuk dalam halaman ini (perlindungan runtun)',
'protect-cantedit'            => 'Anda tidak dapat mengubah tingkatan perlindungan halaman ini karena Anda tidak memiliki hak untuk itu.',
'restriction-type'            => 'Perlindungan:',
'restriction-level'           => 'Tingkatan:',
'minimum-size'                => 'Ukuran minimum',
'maximum-size'                => 'Ukuran maksimum',
'pagesize'                    => '(bita)',

# Restrictions (nouns)
'restriction-edit'   => 'Penyuntingan',
'restriction-move'   => 'Pemindahan',
'restriction-create' => 'Buat',
'restriction-upload' => 'Pemuatan',

# Restriction levels
'restriction-level-sysop'         => 'perlindungan penuh',
'restriction-level-autoconfirmed' => 'perlindungan semi',
'restriction-level-all'           => 'semua tingkatan',

# Undelete
'undelete'                     => 'Pembatalan penghapusan',
'undeletepage'                 => 'Pembatalan penghapusan',
'undeletepagetitle'            => "'''Berikut daftar revisi yang dihapus dari [[:$1]]'''.",
'viewdeletedpage'              => 'Lihat halaman yang telah dihapus',
'undeletepagetext'             => 'Halaman-halaman berikut ini telah dihapus tapi masih ada di dalam arsip dan dapat dikembalikan. Arsip tersebut mungkin akan dibersihkan secara berkala.',
'undelete-fieldset-title'      => 'Mengembalikan revisi',
'undeleteextrahelp'            => "Untuk mengembalikan seluruh revisi-revisi terdahulu halaman, biarkan seluruh kotak cek tidak terpilih dan klik '''''Kembalikan'''''.
Untuk melakukan pengembalian selektif, cek kotak revisi yang diinginkan dan klik '''''Kembalikan'''''.
Menekan tombol '''''Reset''''' akan mengosongkan isian komentar dan semua kotak cek.",
'undeleterevisions'            => '$1 {{PLURAL:$1|revisi|revisi}} diarsipkan',
'undeletehistory'              => 'Jika Anda mengembalikan halaman tersebut, semua revisi juga akan dikembalikan ke dalam daftar versi terdahulu halaman.
Jika sebuah halaman baru dengan nama yang sama telah dibuat sejak penghapusan, revisi-revisi yang dikembalikan tersebut akan ditampilkan dalam daftar versi terdahulu.',
'undeleterevdel'               => 'Pembatalan penghapusan tidak akan dilakukan jika hal tersebut akan mengakibatkan revisi terkini halaman terhapus sebagian. Pada kondisi tersebut, Anda harus menghilangkan cek atau menghilangkan penyembunyian revisi yang dihapus terakhir. Revisi berkas yang tidak dapat Anda lihat tidak akan dipulihkan.',
'undeletehistorynoadmin'       => 'Artikel ini telah dihapus. Alasan penghapusan diberikan pada ringkasan di bawah ini, berikut detil pengguna yang telah melakukan penyuntingan pada halaman ini sebelum dihapus. Isi terakhir dari revisi yang telah dihapus ini hanya tersedia untuk pengurus.',
'undelete-revision'            => 'Revisi yang telah dihapus dari $1 (sampai $2) oleh $3:',
'undeleterevision-missing'     => 'Revisi salah atau tak ditemukan. Anda mungkin mengikuti pranala yang salah, atau revisi tersebut telah dipulihkan atau dibuang dari arsip.',
'undelete-nodiff'              => 'Tidak ada revisi yang lebih lama.',
'undeletebtn'                  => 'Kembalikan!',
'undeletelink'                 => 'kembalikan',
'undeletereset'                => 'Reset',
'undeletecomment'              => 'Komentar:',
'undeletedarticle'             => '"$1" telah dikembalikan',
'undeletedrevisions'           => '$1 {{PLURAL:$1|revisi|revisi}} telah dikembalikan',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|revisi|revisi}} and $2 berkas dikembalikan',
'undeletedfiles'               => '$1 {{PLURAL:$1|berkas|berkas}} dikembalikan',
'cannotundelete'               => 'Pembatalan penghapusan gagal; mungkin ada orang lain yang telah terlebih dahulu melakukan pembatalan.',
'undeletedpage'                => "<big>'''$1 berhasil dikembalikan'''</big>

Lihat [[Special:Log/delete|log penghapusan]] untuk data penghapusan dan pengembalian.",
'undelete-header'              => 'Lihat [[Special:Log/delete|log penghapusan]] untuk daftar halaman yang baru dihapus.',
'undelete-search-box'          => 'Cari halaman yang dihapus',
'undelete-search-prefix'       => 'Tampilkan halaman dimulai dari:',
'undelete-search-submit'       => 'Cari',
'undelete-no-results'          => 'Tidak ditemukan halaman yang sesuai di arsip penghapusan.',
'undelete-filename-mismatch'   => 'Tidak dapat membatalkan penghapusan revisi berkas dengan tanda waktu $1: nama berkas tak sesuai',
'undelete-bad-store-key'       => 'Tidak dapat membatalkan penghapusan revisi berkas dengan tanda waktu $1: berkas hilang sebelum dihapus.',
'undelete-cleanup-error'       => 'Kesalahan sewaktu menghapus arsip berkas "$1" yang tak digunakan.',
'undelete-missing-filearchive' => 'Tidak dapat mengembalikan arsip berkas dengan ID $1 karena tak ada di basis data. Berkas tersebut mungkin telah dihapus..',
'undelete-error-short'         => 'Kesalahan membatalkan penghapusan: $1',
'undelete-error-long'          => 'Terjadi kesalahan sewaktu membatalkan penghapusan berkas:

$1',

# Namespace form on various pages
'namespace'      => 'Ruang nama:',
'invert'         => 'Balikkan pilihan',
'blanknamespace' => '(Utama)',

# Contributions
'contributions' => 'Kontribusi pengguna',
'mycontris'     => 'Kontribusi saya',
'contribsub2'   => 'Untuk $1 ($2)',
'nocontribs'    => 'Tidak ada perubahan yang sesuai dengan kriteria tersebut.',
'uctop'         => ' (atas)',
'month'         => 'Sejak bulan (dan sebelumnya):',
'year'          => 'Sejak tahun (dan sebelumnya):',

'sp-contributions-newbies'     => 'Hanya pengguna-pengguna baru',
'sp-contributions-newbies-sub' => 'Untuk pengguna baru',
'sp-contributions-blocklog'    => 'Log pemblokiran',
'sp-contributions-search'      => 'Cari kontribusi',
'sp-contributions-username'    => 'Alamat IP atau nama pengguna:',
'sp-contributions-submit'      => 'Cari',

# What links here
'whatlinkshere'            => 'Pranala balik',
'whatlinkshere-title'      => 'Halaman yang memiliki pranala ke "$1"',
'whatlinkshere-page'       => 'Halaman:',
'linklistsub'              => '(Daftar pranala)',
'linkshere'                => "Halaman-halaman berikut ini memiliki pranala ke '''[[:$1]]''':",
'nolinkshere'              => "Tidak ada halaman yang memiliki pranala ke '''[[:$1]]'''.",
'nolinkshere-ns'           => "Tidak ada halaman yang memiliki pranala ke '''[[:$1]]''' pada ruang nama yang dipilih.",
'isredirect'               => 'halaman peralihan',
'istemplate'               => 'dengan templat',
'isimage'                  => 'pranala berkas',
'whatlinkshere-prev'       => '$1 {{PLURAL:$1|sebelumnya|sebelumnya}}',
'whatlinkshere-next'       => '$1 {{PLURAL:$1|selanjutnya|selanjutnya}}',
'whatlinkshere-links'      => '← pranala',
'whatlinkshere-hideredirs' => '$1 pengalihan',
'whatlinkshere-hidetrans'  => '$1 transklusi',
'whatlinkshere-hidelinks'  => '$1 pranala',
'whatlinkshere-hideimages' => '$1 pranala berkas',
'whatlinkshere-filters'    => 'Filter',

# Block/unblock
'blockip'                         => 'Blokir pengguna',
'blockip-legend'                  => 'Blokir pengguna',
'blockiptext'                     => 'Gunakan formulir di bawah untuk memblokir akses penulisan dari sebuah alamat IP atau pengguna tertentu.
Ini hanya boleh dilakukan untuk mencegah vandalisme, dan sejalan dengan [[{{MediaWiki:Policy-url}}|kebijakan]].
Masukkan alasan Anda di bawah (contoh, menuliskan nama halaman yang telah divandalisasi).',
'ipaddress'                       => 'Alamat IP:',
'ipadressorusername'              => 'Alamat IP atau nama pengguna:',
'ipbexpiry'                       => 'Kadaluwarsa:',
'ipbreason'                       => 'Alasan:',
'ipbreasonotherlist'              => 'Alasan lain',
'ipbreason-dropdown'              => '
*Alasan umum
** Memberikan informasi palsu
** Menghilangkan isi halaman
** Spam pranala ke situs luar
** Memasukkan omong kosong ke halaman
** Perilaku intimidasi/pelecehan
** Menyalahgunakan beberapa akun
** Nama pengguna tak layak',
'ipbanononly'                     => 'Hanya blokir pengguna anonim',
'ipbcreateaccount'                => 'Cegah pembuatan akun',
'ipbemailban'                     => 'Cegah pengguna mengirimkan surat-e',
'ipbenableautoblock'              => 'Blokir alamat IP terakhir yang digunakan pengguna ini secara otomatis, dan semua alamat berikutnya yang mereka coba gunakan untuk menyunting.',
'ipbsubmit'                       => 'Kirimkan',
'ipbother'                        => 'Waktu lain:',
'ipboptions'                      => '2 jam:2 hours,1 hari:1 day,3 hari:3 days,1 minggu:1 week,2 minggu:2 weeks,1 bulan:1 month,3 bulan:3 months,6 bulan:6 months,1 tahun:1 year,selamanya:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'lainnya',
'ipbotherreason'                  => 'Alasan lain/tambahan:',
'ipbhidename'                     => 'Sembunyikan nama pengguna atau IP dari log pemblokiran, daftar blokir aktif, serta daftar pengguna',
'ipbwatchuser'                    => 'Pantau halaman pengguna dan pembicaraan pengguna ini',
'badipaddress'                    => 'Format alamat IP atau nama pengguna salah.',
'blockipsuccesssub'               => 'Pemblokiran sukses',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] telah diblokir.<br />
Lihat [[Special:IPBlockList|Daftar IP]] untuk meninjau kembali pemblokiran.',
'ipb-edit-dropdown'               => 'Sunting alasan pemblokiran',
'ipb-unblock-addr'                => 'Hilangkan blokir $1',
'ipb-unblock'                     => 'Hilangkan blokir seorang pengguna atau suatu alamat IP',
'ipb-blocklist-addr'              => 'Lihat blokir yang diterapkan untuk $1',
'ipb-blocklist'                   => 'Lihat blokir yang diterapkan',
'unblockip'                       => 'Hilangkan blokir terhadap alamat IP atau pengguna',
'unblockiptext'                   => 'Gunakan formulir di bawah untuk mengembalikan kemampuan menulis sebuah alamat IP atau pengguna yang sebelumnya telah diblokir.',
'ipusubmit'                       => 'Hilangkan blokir terhadap alamat ini',
'unblocked'                       => 'Blokir terhadap [[User:$1|$1]] telah dicabut',
'unblocked-id'                    => 'Blokir $1 telah dicabut',
'ipblocklist'                     => 'Daftar pemblokiran alamat IP dan nama penguna',
'ipblocklist-legend'              => 'Cari pengguna yang diblokir',
'ipblocklist-username'            => 'Nama pengguna atau alamat IP:',
'ipblocklist-submit'              => 'Cari',
'blocklistline'                   => '$1, $2 memblokir $3 ($4)',
'infiniteblock'                   => 'tak terbatas',
'expiringblock'                   => 'kadaluwarsa $1',
'anononlyblock'                   => 'hanya pengguna anonim',
'noautoblockblock'                => 'pemblokiran otomatis dimatikan',
'createaccountblock'              => 'pembuatan akun diblokir',
'emailblock'                      => 'surat-e diblokir',
'ipblocklist-empty'               => 'Daftar pemblokiran kosong.',
'ipblocklist-no-results'          => 'alamat IP atau pengguna yang diminta tidak diblokir.',
'blocklink'                       => 'blokir',
'unblocklink'                     => 'hilangkan blokir',
'contribslink'                    => 'kontrib',
'autoblocker'                     => 'Diblokir secara otomatis karena Anda berbagi alamat IP dengan "$1". Alasan "$2".',
'blocklogpage'                    => 'Log pemblokiran',
'blocklogentry'                   => 'memblokir [[$1]] dengan waktu kadaluwarsa $2 $3',
'blocklogtext'                    => 'Di bawah ini adalah log pemblokiran dan pembukaan blokir terhadap pengguna.
Alamat IP yang diblokir secara otomatis tidak terdapat di dalam daftar ini.
Lihat [[Special:IPBlockList|daftar alamat IP yang diblokir]] untuk daftar pemblokiran terkini.',
'unblocklogentry'                 => 'menghilangkan blokir "$1"',
'block-log-flags-anononly'        => 'hanya pengguna anonim',
'block-log-flags-nocreate'        => 'pembuatan akun dimatikan',
'block-log-flags-noautoblock'     => 'blokir otomatis dimatikan',
'block-log-flags-noemail'         => 'surat-e diblokir',
'block-log-flags-angry-autoblock' => 'peningkatan sistem pemblokiran otomatis telah diaktifkan',
'range_block_disabled'            => 'Kemampuan pengurus dalam membuat blokir blok IP dimatikan.',
'ipb_expiry_invalid'              => 'Waktu kadaluwarsa tidak sah.',
'ipb_expiry_temp'                 => 'Pemblokiran atas nama pengguna yang disembunyikan harus permanen.',
'ipb_already_blocked'             => '"$1" telah diblokir',
'ipb_cant_unblock'                => 'Kesalahan: Blokir dengan ID $1 tidak ditemukan. Blokir tersebut kemungkinan telah dibuka.',
'ipb_blocked_as_range'            => 'Kesalahan: IP $1 tidak diblok secara langsung dan tidak dapat dilepaskan. IP $1 diblok sebagai bagian dari pemblokiran kelompok IP $2, yang dapat dilepaskan.',
'ip_range_invalid'                => 'Blok IP tidak sah.',
'blockme'                         => 'Blokir saya',
'proxyblocker'                    => 'Pemblokir proxy',
'proxyblocker-disabled'           => 'Fitur ini sedang tidak diakfifkan.',
'proxyblockreason'                => 'Alamat IP Anda telah diblokir karena alamat IP Anda adalah proxy terbuka. Silakan hubungi penyedia jasa internet Anda atau dukungan teknis dan beritahukan mereka masalah keamanan serius ini.',
'proxyblocksuccess'               => 'Selesai.',
'sorbsreason'                     => 'Alamat IP anda terdaftar sebagai proxy terbuka di DNSBL.',
'sorbs_create_account_reason'     => 'Alamat IP anda terdaftar sebagai proxy terbuka di DNSBL. Anda tidak dapat membuat akun.',

# Developer tools
'lockdb'              => 'Kunci basis data',
'unlockdb'            => 'Buka kunci basis data',
'lockdbtext'          => 'Mengunci basis data akan menghentikan kemampuan semua pengguna dalam menyunting halaman, mengubah preferensi pengguna, menyunting daftar pantauan mereka, dan hal-hal lain yang memerlukan perubahan terhadap basis data. Pastikan bahwa ini adalah yang ingin Anda lakukan, dan bahwa Anda akan membuka kunci basis data setelah pemeliharaan selesai.',
'unlockdbtext'        => 'Membuka kunci basis data akan mengembalikan kemampuan semua pengguna dalam menyunting halaman, mengubah preferensi pengguna, menyunting daftar pantauan mereka, dan hal-hal lain yang memerlukan perubahan terhadap basis data.  Pastikan bahwa ini adalah yang ingin Anda lakukan.',
'lockconfirm'         => 'Ya, saya memang ingin mengunci basis data.',
'unlockconfirm'       => 'Ya, saya memang ingin membuka kunci basis data.',
'lockbtn'             => 'Kunci basis data',
'unlockbtn'           => 'Buka kunci basis data',
'locknoconfirm'       => 'Anda tidak memberikan tanda cek pada kotak konfirmasi.',
'lockdbsuccesssub'    => 'Penguncian basis data berhasil',
'unlockdbsuccesssub'  => 'Pembukaan kunci basis data berhasil',
'lockdbsuccesstext'   => 'Basis data telah dikunci.<br />
Pastikan Anda [[Special:UnlockDB|membuka kuncinya]] setelah pemeliharaan selesai.',
'unlockdbsuccesstext' => 'Kunci basis data telah dibuka.',
'lockfilenotwritable' => 'Berkas kunci basis data tidak dapat ditulis. Untuk mengunci atau membuka basis data, berkas ini harus dapat ditulis oleh server web.',
'databasenotlocked'   => 'Basis data tidak terkunci.',

# Move page
'move-page'               => 'Pindahkan $1',
'move-page-legend'        => 'Pindahkan halaman',
'movepagetext'            => "Formulir di bawah ini digunakan untuk mengubah nama suatu halaman dan memindahkan semua data sejarah ke nama baru. Judul yang lama akan menjadi halaman peralihan menuju judul yang baru. Pranala kepada judul lama tidak akan berubah. Pastikan untuk memeriksa terhadap peralihan halaman yang rusak atau berganda setelah pemindahan. Anda bertanggung jawab untuk memastikan bahwa pranala terus menyambung ke halaman yang seharusnya.

Perhatikan bahwa halaman '''tidak''' akan dipindah apabila telah ada halaman yang menggunakan judul yang baru, kecuali bila halaman tersebut kosong atau merupakan halaman peralihan dan tidak mempunyai sejarah penyuntingan. Ini berarti Anda dapat mengubah nama halaman kembali seperti semula apabila Anda membuat kesalahan, dan Anda tidak dapat menimpa halaman yang telah ada.

'''Peringatan:''' Ini dapat mengakibatkan perubahan yang tak terduga dan drastis  bagi halaman yang populer. Pastikan Anda mengerti konsekuensi dari perbuatan ini sebelum melanjutkan.",
'movepagetalktext'        => "Halaman pembicaraan yang berkaitan juga akan dipindahkan secara otomatis '''kecuali apabila:'''

*Sebuah halaman pembicaraan yang tidak kosong telah ada di bawah judul baru, atau
*Anda tidak memberi tanda cek pada kotak di bawah ini

Dalam kasus tersebut, apabila diinginkan, Anda dapat memindahkan atau menggabungkan halaman secara manual.",
'movearticle'             => 'Pindahkan halaman:',
'movenotallowed'          => 'Anda tak memiliki hak akses untuk memindahkan halaman pada wiki ini.',
'newtitle'                => 'Ke judul baru:',
'move-watch'              => 'Pantau halaman ini',
'movepagebtn'             => 'Pindahkan halaman',
'pagemovedsub'            => 'Pemindahan berhasil',
'movepage-moved'          => '<big>\'\'\'"$1" telah dipindahkan ke "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Halaman dengan nama tersebut telah ada atau nama yang dipilih tidak sah. Silakan pilih nama lain.',
'cantmove-titleprotected' => 'Anda tidak dapat memindahkan halaman ke lokasi ini, karena judul tujuan sedang dilindungi dari pembuatan',
'talkexists'              => 'Halaman tersebut berhasil dipindahkan, tetapi halaman pembicaraan dari halaman tersebut tidak dapat dipindahkan karena telah ada halaman pembicaraan pada judul yang baru. Silakan gabungkan halaman-halaman pembicaraan tersebut secara manual.',
'movedto'                 => 'dipindahkan ke',
'movetalk'                => 'Pindahkan halaman pembicaraan yang terkait.',
'move-subpages'           => 'Pindahkan semua sub-halaman, bila ada',
'move-talk-subpages'      => 'Pindahkan semua sub-halaman pembicaraan, bila ada',
'movepage-page-exists'    => 'Halaman $1 telah ada dan tidak dapat ditimpa secara otomatis.',
'movepage-page-moved'     => 'Halaman $1 telah dipindahkan ke $2.',
'movepage-page-unmoved'   => 'Halaman $1 tidak dapat dipindahkan ke $2.',
'movepage-max-pages'      => 'Sejumlah maksimum $1 {{PLURAL:$1|halaman|halaman}} telah dipindahkan dan tidak ada lagi yang akan dipindahkan secara otomatis.',
'1movedto2'               => 'memindahkan [[$1]] ke [[$2]]',
'1movedto2_redir'         => 'memindahkan [[$1]] ke [[$2]] melalui peralihan',
'movelogpage'             => 'Log pemindahan',
'movelogpagetext'         => 'Di bawah ini adalah log pemindahan halaman.',
'movereason'              => 'Alasan:',
'revertmove'              => 'kembalikan',
'delete_and_move'         => 'Hapus dan pindahkan',
'delete_and_move_text'    => '==Penghapusan diperlukan==

Artikel yang dituju, "[[:$1]]", telah mempunyai isi. Apakah Anda hendak menghapusnya untuk memberikan ruang bagi pemindahan?',
'delete_and_move_confirm' => 'Ya, hapus halaman tersebut',
'delete_and_move_reason'  => 'Dihapus untuk mengantisipasikan pemindahan halaman',
'selfmove'                => 'Pemindahan halaman tidak dapat dilakukan karena judul sumber dan judul tujuan sama.',
'immobile_namespace'      => 'Judul sumber atau tujuan termasuk tipe khusus; tidak dapat memindahkan halaman ke ruang nama tersebut.',
'imagenocrossnamespace'   => 'Tidak dapat memindahkan berkas ke ruang nama non-berkas',
'imagetypemismatch'       => 'Ekstensi yang diberikan tidak cocok dengan tipe berkas',
'imageinvalidfilename'    => 'Nama berkas tujuan tidak sah',
'fix-double-redirects'    => 'Perbaiki semua pengalihan ganda yang mungkin terjadi',

# Export
'export'            => 'Ekspor halaman',
'exporttext'        => 'Anda dapat mengekspor teks dan sejarah penyuntingan suatu halaman tertentu atau suatu set halaman dalam bentuk XML tertentu.
Hasil ekspor ini selanjutnya dapat diimpor ke wiki lainnya yang menggunakan perangkat lunak MediaWiki, dengan menggunakan fasilitas [[Special:Import|halaman impor]].

Untuk mengekspor halaman-halaman artikel, masukkan judul-judul dalam kotak teks di bawah ini, satu judul per baris, dan pilih apakah Anda ingin mengekspor lengkap dengan versi terdahulunya, atau hanya versi terbaru dengan catatan penyuntingan terakhir.

Jika Anda hanya ingin mengimpor versi terbaru, Anda melakukannya lebih cepat dengan cara menggunakan pranala khusus, sebagai contoh: [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] untuk mengekspor artikel "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Hanya ekspor revisi sekarang, bukan seluruh versi terdahulu',
'exportnohistory'   => "----
'''Catatan:''' Mengekspor keseluruhan riwayat suntingan halaman melalui isian ini telah dinon-aktifkan karena alasan kinerja.",
'export-submit'     => 'Ekspor',
'export-addcattext' => 'Tambahkan halaman dari kategori:',
'export-addcat'     => 'Tambahkan',
'export-download'   => 'Tawarkan untuk menyimpan sebagai suatu berkas',
'export-templates'  => 'Termasuk templat',

# Namespace 8 related
'allmessages'               => 'Pesan sistem',
'allmessagesname'           => 'Nama',
'allmessagesdefault'        => 'Teks baku',
'allmessagescurrent'        => 'Teks sekarang',
'allmessagestext'           => 'Ini adalah daftar semua pesan sistem yang tersedia dalam ruang nama MediaWiki:',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' tidak didukung karena '''\$wgUseDatabaseMessages''' dimatikan.",
'allmessagesfilter'         => 'Filter nama pesan:',
'allmessagesmodified'       => 'Hanya tampilkan yang diubah',

# Thumbnails
'thumbnail-more'           => 'Perbesar',
'filemissing'              => 'Berkas tak ditemukan',
'thumbnail_error'          => 'Gagal membuat miniatur: $1',
'djvu_page_error'          => 'Halaman DjVu di luar rentang',
'djvu_no_xml'              => 'XML untuk berkas DjVu tak dapat diperoleh',
'thumbnail_invalid_params' => 'Kesalahan parameter miniatur',
'thumbnail_dest_directory' => 'Direktori tujuan tak dapat dibuat',

# Special:Import
'import'                     => 'Impor halaman',
'importinterwiki'            => 'Impor transwiki',
'import-interwiki-text'      => 'Pilih suatu wiki dan judul halaman yang akan di impor.
Tanggal revisi dan nama penyunting akan dipertahankan.
Semua aktivitas impor transwiki akan dicatat di [[Special:Log/import|log impor]].',
'import-interwiki-history'   => 'Salin semua versi terdahulu dari halaman ini',
'import-interwiki-submit'    => 'Impor',
'import-interwiki-namespace' => 'Transfer halaman ke dalam ruang nama:',
'importtext'                 => 'Silakan ekspor berkas dari wiki asal dengan menggunakan [[Special:Export|fasilitas ekspor]].
Simpan ke komputer Anda lalu muatkan di sini.',
'importstart'                => 'Mengimpor halaman...',
'import-revision-count'      => '$1 {{PLURAL:$1|revisi|revisi}}',
'importnopages'              => 'Tidak ada halaman untuk diimpor.',
'importfailed'               => 'Impor gagal: $1',
'importunknownsource'        => 'Sumber impor tidak dikenali',
'importcantopen'             => 'Berkas impor tidak dapat dibuka',
'importbadinterwiki'         => 'Pranala interwiki rusak',
'importnotext'               => 'Kosong atau tidak ada teks',
'importsuccess'              => 'Impor sukses!',
'importhistoryconflict'      => 'Terjadi konflik revisi sejarah (mungkin pernah mengimpor halaman ini sebelumnya)',
'importnosources'            => 'Tidak ada sumber impor transwiki yang telah dibuat dan pemuatan riwayat secara langsung telah di non-aktifkan.',
'importnofile'               => 'Tidak ada berkas sumber impor yang telah dimuat.',
'importuploaderrorsize'      => 'Pemuatan berkas impor gagal. Ukuran berkas melebihi ukuran yang diperbolehkan.',
'importuploaderrorpartial'   => 'Pemuatan berkas impor gagal. Hanya sebagian berkas yang berhasil dimuat.',
'importuploaderrortemp'      => 'Pemuatan berkas gagal. Sebuah direktori sementara dibutuhkan.',
'import-parse-failure'       => 'Proses impor XML gagal',
'import-noarticle'           => 'Tak ada halaman yang dapat diimpor!',
'import-nonewrevisions'      => 'Semua revisi telah pernah diimpor sebelumnya.',
'xml-error-string'           => '$1 pada baris $2, kolom $3 (bita $4): $5',
'import-upload'              => 'Memuat data XML',

# Import log
'importlogpage'                    => 'Log impor',
'importlogpagetext'                => 'Di bawah ini adalah log impor administratif dari halaman-halaman, berikut riwayat suntingannya dari wiki lain.',
'import-logentry-upload'           => 'mengimpor [[$1]] melalui pemuatan berkas',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|revisi|revisi}}',
'import-logentry-interwiki'        => 'men-transwiki $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|revisi}} dari $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Halaman pengguna saya',
'tooltip-pt-anonuserpage'         => 'Halaman pengguna IP Anda',
'tooltip-pt-mytalk'               => 'Halaman pembicaraan saya',
'tooltip-pt-anontalk'             => 'Pembicaraan tentang suntingan dari alamat IP ini',
'tooltip-pt-preferences'          => 'Preferensi saya',
'tooltip-pt-watchlist'            => 'Daftar halaman yang saya pantau.',
'tooltip-pt-mycontris'            => 'Daftar kontribusi saya',
'tooltip-pt-login'                => 'Anda disarankan untuk masuk log, meskipun hal itu tidak diwajibkan.',
'tooltip-pt-anonlogin'            => 'Anda disarankan untuk masuk log, meskipun hal itu tidak diwajibkan.',
'tooltip-pt-logout'               => 'Keluar log',
'tooltip-ca-talk'                 => 'Pembicaraan halaman isi',
'tooltip-ca-edit'                 => 'Sunting halaman ini. Gunakan tombol pratayang sebelum menyimpan.',
'tooltip-ca-addsection'           => 'Tambahkan komentar ke halaman pembicaraan ini.',
'tooltip-ca-viewsource'           => 'Halaman ini dilindungi. Anda hanya dapat melihat sumbernya.',
'tooltip-ca-history'              => 'Versi-versi sebelumnya dari halaman ini.',
'tooltip-ca-protect'              => 'Lindungi halaman ini',
'tooltip-ca-delete'               => 'Hapus halaman ini',
'tooltip-ca-undelete'             => 'Kembalikan suntingan ke halaman ini sebelum halaman ini dihapus',
'tooltip-ca-move'                 => 'Pindahkan halaman ini',
'tooltip-ca-watch'                => 'Tambahkan halaman ini ke daftar pantauan Anda',
'tooltip-ca-unwatch'              => 'Hapus halaman ini dari daftar pantauan Anda',
'tooltip-search'                  => 'Cari dalam wiki ini',
'tooltip-search-go'               => 'Cari suatu halaman dengan nama yang persis seperti ini jika tersedia',
'tooltip-search-fulltext'         => 'Cari halaman yang memiliki teks seperti ini',
'tooltip-p-logo'                  => 'Halaman Utama',
'tooltip-n-mainpage'              => 'Kunjungi Halaman Utama',
'tooltip-n-portal'                => 'Tentang proyek, apa yang dapat anda lakukan, di mana mencari sesuatu',
'tooltip-n-currentevents'         => 'Temukan informasi tentang peristiwa terkini',
'tooltip-n-recentchanges'         => 'Daftar perubahan terbaru dalam wiki.',
'tooltip-n-randompage'            => 'Tampilkan sembarang halaman',
'tooltip-n-help'                  => 'Tempat mencari bantuan.',
'tooltip-t-whatlinkshere'         => 'Daftar semua halaman wiki yang memiliki pranala ke halaman ini',
'tooltip-t-recentchangeslinked'   => 'Perubahan terbaru halaman-halaman yang memiliki pranala ke halaman ini',
'tooltip-feed-rss'                => 'Umpan RSS untuk halaman ini',
'tooltip-feed-atom'               => 'Umpan Atom untuk halaman ini',
'tooltip-t-contributions'         => 'Lihat daftar kontribusi pengguna ini',
'tooltip-t-emailuser'             => 'Kirimkan surat-e kepada pengguna ini',
'tooltip-t-upload'                => 'Muatkan gambar atau berkas media',
'tooltip-t-specialpages'          => 'Daftar semua halaman istimewa',
'tooltip-t-print'                 => 'Versi cetak halaman ini',
'tooltip-t-permalink'             => 'Pranala permanen untuk revisi halaman ini',
'tooltip-ca-nstab-main'           => 'Lihat halaman artikel',
'tooltip-ca-nstab-user'           => 'Lihat halaman pengguna',
'tooltip-ca-nstab-media'          => 'Lihat halaman media',
'tooltip-ca-nstab-special'        => 'Ini adalah halaman istimewa yang tidak dapat disunting.',
'tooltip-ca-nstab-project'        => 'Lihat halaman proyek',
'tooltip-ca-nstab-image'          => 'Lihat halaman berkas',
'tooltip-ca-nstab-mediawiki'      => 'Lihat pesan sistem',
'tooltip-ca-nstab-template'       => 'Lihat templat',
'tooltip-ca-nstab-help'           => 'Lihat halaman bantuan',
'tooltip-ca-nstab-category'       => 'Lihat halaman kategori',
'tooltip-minoredit'               => 'Tandai ini sebagai suntingan kecil',
'tooltip-save'                    => 'Simpan perubahan Anda',
'tooltip-preview'                 => 'Pratayang perubahan Anda, harap gunakan ini sebelum menyimpan!',
'tooltip-diff'                    => 'Lihat perubahan yang telah Anda lakukan.',
'tooltip-compareselectedversions' => 'Lihat perbedaan antara dua versi halaman yang dipilih.',
'tooltip-watch'                   => 'Tambahkan halaman ini ke daftar pantauan Anda',
'tooltip-recreate'                => 'Buat ulang halaman walaupun sebenarnya telah dihapus',
'tooltip-upload'                  => 'Mulai pemuatan',

# Stylesheets
'common.css'   => '/* CSS yang ada di sini akan diterapkan untuk semua kulit. */',
'monobook.css' => '/* CSS yang ada di sini akan diterapkan untuk kulit Monobook. */',

# Scripts
'common.js'   => '/* JavaScript yang ada di sini akan diterapkan untuk semua kulit. */',
'monobook.js' => '/* Semua JavaScript di sini akan dimuatkan untuk para pengguna yang menggunakan kulit MonoBook */',

# Metadata
'nodublincore'      => 'Metadata Dublin Core RDF dimatikan di server ini.',
'nocreativecommons' => 'Metadata Creative Commons RDF dimatikan di server ini.',
'notacceptable'     => 'Server wiki tidak dapat menyediakan data dalam format yang dapat dibaca oleh client Anda.',

# Attribution
'anonymous'        => 'Pengguna(-pengguna) anonim {{SITENAME}}',
'siteuser'         => 'Pengguna {{SITENAME}} $1',
'lastmodifiedatby' => 'Halaman ini terakhir kali diubah $2, $1 oleh $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Didasarkan pada karya $1.',
'others'           => 'lainnya',
'siteusers'        => 'Pengguna(-pengguna) {{SITENAME}} $1',
'creditspage'      => 'Penghargaan halaman',
'nocredits'        => 'Tidak ada informasi penghargaan yang tersedia untuk halaman ini.',

# Spam protection
'spamprotectiontitle' => 'Filter pencegah spam',
'spamprotectiontext'  => 'Halaman yang ingin Anda simpan telah diblokir oleh filter spam.
Ini mungkin disebabkan oleh pranala ke situs luar yang termasuk dalam daftar hitam.',
'spamprotectionmatch' => 'Teks berikut ini memancing filter spam kami: $1',
'spambot_username'    => 'Pembersihan span MediaWiki',
'spam_reverting'      => 'Mengembalikan ke versi terakhir yang tak memiliki pranala ke $1',
'spam_blanking'       => 'Semua revisi yang memiliki pranala ke $1, pengosongan',

# Info page
'infosubtitle'   => 'Informasi halaman',
'numedits'       => 'Jumlah penyuntingan (artikel): $1',
'numtalkedits'   => 'Jumlah penyuntingan (halaman pembicaraan): $1',
'numwatchers'    => 'Jumlah pengamat: $1',
'numauthors'     => 'Jumlah pengarang yang berbeda (artikel): $1',
'numtalkauthors' => 'Jumlah pengarang yang berbeda (halaman pembicaraan): $1',

# Math options
'mw_math_png'    => 'Selalu buat PNG',
'mw_math_simple' => 'HTML jika sangat sederhana atau PNG',
'mw_math_html'   => 'HTML jika mungkin atau PNG',
'mw_math_source' => 'Biarkan sebagai TeX (untuk penjelajah web teks)',
'mw_math_modern' => 'Disarankan untuk penjelajah web modern',
'mw_math_mathml' => 'MathML jika mungkin (percobaan)',

# Patrolling
'markaspatrolleddiff'                 => 'Tandai telah dipatroli',
'markaspatrolledtext'                 => 'Tandai artikel ini telah dipatroli',
'markedaspatrolled'                   => 'Ditandai telah dipatroli',
'markedaspatrolledtext'               => 'Revisi yang dipilih telah ditandai terpatroli',
'rcpatroldisabled'                    => 'Patroli perubahan terbaru dimatikan',
'rcpatroldisabledtext'                => 'Fitur patroli perubahan terbaru sedang dimatikan.',
'markedaspatrollederror'              => 'Tidak dapat menandai telah dipatroli',
'markedaspatrollederrortext'          => 'Anda harus menentukan satu revisi untuk ditandai sebagai yang dipatroli.',
'markedaspatrollederror-noautopatrol' => 'Anda tidak diizinkan menandai suntingan Anda sendiri dipatroli.',

# Patrol log
'patrol-log-page'   => 'Log patroli',
'patrol-log-header' => 'Ini adalah log revisi terpatroli.',
'patrol-log-line'   => 'menandai $1 dari $2 terpatroli $3',
'patrol-log-auto'   => '(otomatis)',

# Image deletion
'deletedrevision'                 => 'Revisi lama yang dihapus $1',
'filedeleteerror-short'           => 'Kesalahan waktu menghapus berkas: $1',
'filedeleteerror-long'            => 'Terjadi kesalahan sewaktu menghapus berkas:\\n\\n$1\\n',
'filedelete-missing'              => 'Berkas "$1" tak dapat dihapus karena tak ditemukan.',
'filedelete-old-unregistered'     => 'Revisi berkas "$1" yang diberikan tidak ada dalam basis data.',
'filedelete-current-unregistered' => 'Berkas yang diberikan "$1" tidak ada dalam basis data.',
'filedelete-archive-read-only'    => 'Direktori arsip "$1" tak dapat ditulis oleh server web.',

# Browsing diffs
'previousdiff' => '← Revisi sebelumnya',
'nextdiff'     => 'Revisi selanjutnya →',

# Media information
'mediawarning'         => "'''Peringatan:''' Berkas ini mungkin mengandung kode berbahaya yang jika dijalankan dapat mempengaruhi sistem Anda.<hr />",
'imagemaxsize'         => 'Batasi ukuran gambar dalam halaman deskripsi berkas sampai:',
'thumbsize'            => 'Ukuran miniatur:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|halaman|halaman}}',
'file-info'            => '(ukuran berkas: $1, tipe MIME: $2)',
'file-info-size'       => '($1 × $2 piksel, ukuran berkas: $3, tipe MIME: $4)',
'file-nohires'         => '<small>Tak tersedia resolusi yang lebih tinggi.</small>',
'svg-long-desc'        => '(Berkas SVG, nominal $1 × $2 piksel, besar berkas: $3)',
'show-big-image'       => 'Resolusi penuh',
'show-big-image-thumb' => '<small>Ukuran pratayang ini: $1 × $2 piksel</small>',

# Special:NewImages
'newimages'             => 'Berkas baru',
'imagelisttext'         => "Di bawah ini adalah daftar '''$1''' {{PLURAL:$1|berkas|berkas}} diurutkan $2.",
'newimages-summary'     => 'Halaman istimewa berikut menampilkan daftar berkas yang terakhir dimuat',
'showhidebots'          => '($1 bot)',
'noimages'              => 'Tidak ada yang dilihat.',
'ilsubmit'              => 'Cari',
'bydate'                => 'berdasarkan tanggal',
'sp-newimages-showfrom' => 'Tampilkan berkas baru dimulai dari $2, $1',

# Bad image list
'bad_image_list' => 'Formatnya sebagai berikut:

Hanya butir daftar (baris yang diawali dengan tanda *) yang diperhitungkan. Pranala pertama pada suatu baris haruslah pranala ke berkas yang buruk.
Pranala-pranala selanjutnya pada baris yang sama dianggap sebagai pengecualian, yaitu artikel yang dapat menampilkan berkas tersebut.',

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => 'Berkas ini mengandung informasi tambahan yang mungkin ditambahkan oleh kamera digital atau pemindai yang digunakan untuk membuat atau mendigitalisasi berkas. Jika berkas ini telah mengalami modifikasi, detil yang ada mungkin tidak secara penuh merefleksikan informasi dari gambar yang sudah dimodifikasi ini.',
'metadata-expand'   => 'Tampilkan detil tambahan',
'metadata-collapse' => 'Sembunyikan detil tambahan',
'metadata-fields'   => 'Entri metadata EXIF berikut akan ditampilkan pada halaman informasi gambar jika tabel metadata disembunyikan. Entri lain secara baku akan disembunyikan
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Lebar',
'exif-imagelength'                 => 'Tinggi',
'exif-bitspersample'               => 'Bit per komponen',
'exif-compression'                 => 'Skema kompresi',
'exif-photometricinterpretation'   => 'Komposisi piksel',
'exif-orientation'                 => 'Orientasi',
'exif-samplesperpixel'             => 'Jumlah komponen',
'exif-planarconfiguration'         => 'Pengaturan data',
'exif-ycbcrsubsampling'            => 'Rasio subsampling Y ke C',
'exif-ycbcrpositioning'            => 'Penempatan Y dan C',
'exif-xresolution'                 => 'Resolusi horizontal',
'exif-yresolution'                 => 'Resolusi vertikal',
'exif-resolutionunit'              => 'Satuan resolusi X dan Y',
'exif-stripoffsets'                => 'Lokasi data gambar',
'exif-rowsperstrip'                => 'Jumlah baris per strip',
'exif-stripbytecounts'             => 'Bita per strip kompresi',
'exif-jpeginterchangeformat'       => 'Ofset ke JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Bita data JPEG',
'exif-transferfunction'            => 'Fungsi transfer',
'exif-whitepoint'                  => 'Kromatisitas titik putih',
'exif-primarychromaticities'       => 'Kromatisitas warna primer',
'exif-ycbcrcoefficients'           => 'Koefisien matriks transformasi ruang warna',
'exif-referenceblackwhite'         => 'Nilai referensi pasangan hitam putih',
'exif-datetime'                    => 'Tanggal dan waktu perubahan berkas',
'exif-imagedescription'            => 'Judul gambar',
'exif-make'                        => 'Produsen kamera',
'exif-model'                       => 'Model kamera',
'exif-software'                    => 'Perangkat lunak',
'exif-artist'                      => 'Pembuat',
'exif-copyright'                   => 'Pemilik hak cipta',
'exif-exifversion'                 => 'Versi Exif',
'exif-flashpixversion'             => 'Dukungan versi Flashpix',
'exif-colorspace'                  => 'Ruang warna',
'exif-componentsconfiguration'     => 'Arti tiap komponen',
'exif-compressedbitsperpixel'      => 'Mode kompresi gambar',
'exif-pixelydimension'             => 'Lebar gambar yang sah',
'exif-pixelxdimension'             => 'Tinggi gambar yang sah',
'exif-makernote'                   => 'Catatan produsen',
'exif-usercomment'                 => 'Komentar pengguna',
'exif-relatedsoundfile'            => 'Berkas audio yang berhubungan',
'exif-datetimeoriginal'            => 'Tanggal dan waktu pembuatan data',
'exif-datetimedigitized'           => 'Tanggal dan waktu digitalisasi',
'exif-subsectime'                  => 'Subdetik DateTime',
'exif-subsectimeoriginal'          => 'Subdetik DateTimeOriginal',
'exif-subsectimedigitized'         => 'Subdetik DateTimeDigitized',
'exif-exposuretime'                => 'Waktu pajanan',
'exif-exposuretime-format'         => '$1 detik ($2)',
'exif-fnumber'                     => 'Nilai F',
'exif-exposureprogram'             => 'Program pajanan',
'exif-spectralsensitivity'         => 'Sensitivitas spektral',
'exif-isospeedratings'             => 'Rating kecepatan ISO',
'exif-oecf'                        => 'Faktor konversi optoelektronik',
'exif-shutterspeedvalue'           => 'Kecepatan rana',
'exif-aperturevalue'               => 'Bukaan',
'exif-brightnessvalue'             => 'Kecerahan',
'exif-exposurebiasvalue'           => 'Bias pajanan',
'exif-maxaperturevalue'            => 'Bukaan tanah maksimum',
'exif-subjectdistance'             => 'Jarak subjek',
'exif-meteringmode'                => 'Mode pengukuran',
'exif-lightsource'                 => 'Sumber cahaya',
'exif-flash'                       => 'Kilas',
'exif-focallength'                 => 'Jarak fokus lensa',
'exif-subjectarea'                 => 'Wilayah subjek',
'exif-flashenergy'                 => 'Energi kilas',
'exif-spatialfrequencyresponse'    => 'Respons frekuensi spasial',
'exif-focalplanexresolution'       => 'Resolusi bidang fokus X',
'exif-focalplaneyresolution'       => 'Resolusi bidang fokus Y',
'exif-focalplaneresolutionunit'    => 'Unit resolusi bidang fokus',
'exif-subjectlocation'             => 'Lokasi subjek',
'exif-exposureindex'               => 'Indeks pajanan',
'exif-sensingmethod'               => 'Metode penginderaan',
'exif-filesource'                  => 'Sumber berkas',
'exif-scenetype'                   => 'Tipe pemandangan',
'exif-cfapattern'                  => 'Pola CFA',
'exif-customrendered'              => 'Proses buatan gambar',
'exif-exposuremode'                => 'Mode pajanan',
'exif-whitebalance'                => 'Keseimbangan putih',
'exif-digitalzoomratio'            => 'Rasio pembesaran digital',
'exif-focallengthin35mmfilm'       => 'Panjang fokus pada fil 35 mm',
'exif-scenecapturetype'            => 'Tipe penangkapan',
'exif-gaincontrol'                 => 'Kontrol pemandangan',
'exif-contrast'                    => 'Kontras',
'exif-saturation'                  => 'Saturasi',
'exif-sharpness'                   => 'Ketajaman',
'exif-devicesettingdescription'    => 'Deskripsi pengaturan alat',
'exif-subjectdistancerange'        => 'Jarak subjek',
'exif-imageuniqueid'               => 'ID unik gambar',
'exif-gpsversionid'                => 'Versi tag GPS',
'exif-gpslatituderef'              => 'Lintang Utara atau Selatan',
'exif-gpslatitude'                 => 'Lintang',
'exif-gpslongituderef'             => 'Bujur Timur atau Barat',
'exif-gpslongitude'                => 'Bujur',
'exif-gpsaltituderef'              => 'Referensi ketinggian',
'exif-gpsaltitude'                 => 'Ketinggian',
'exif-gpstimestamp'                => 'Waktu GPS (jam atom)',
'exif-gpssatellites'               => 'Satelit untuk pengukuran',
'exif-gpsstatus'                   => 'Status penerima',
'exif-gpsmeasuremode'              => 'Mode pengukuran',
'exif-gpsdop'                      => 'Ketepatan pengukuran',
'exif-gpsspeedref'                 => 'Unit kecepatan',
'exif-gpsspeed'                    => 'Kecepatan penerima GPS',
'exif-gpstrackref'                 => 'Referensi arah gerakan',
'exif-gpstrack'                    => 'Arah gerakan',
'exif-gpsimgdirectionref'          => 'Referensi arah gambar',
'exif-gpsimgdirection'             => 'Arah gambar',
'exif-gpsmapdatum'                 => 'Data survei geodesi',
'exif-gpsdestlatituderef'          => 'Referensi lintang dari tujuan',
'exif-gpsdestlatitude'             => 'Lintang tujuan',
'exif-gpsdestlongituderef'         => 'Referensi bujur dari tujuan',
'exif-gpsdestlongitude'            => 'Bujur tujuan',
'exif-gpsdestbearingref'           => 'Referensi bearing tujuan',
'exif-gpsdestbearing'              => 'Bearing tujuan',
'exif-gpsdestdistanceref'          => 'Referensi jarak dari tujuan',
'exif-gpsdestdistance'             => 'Jarak dari tujuan',
'exif-gpsprocessingmethod'         => 'Nama metode proses GPS',
'exif-gpsareainformation'          => 'Nama wilayah GPS',
'exif-gpsdatestamp'                => 'Tanggal GPS',
'exif-gpsdifferential'             => 'Koreksi diferensial GPS',

# EXIF attributes
'exif-compression-1' => 'Tak terkompresi',

'exif-unknowndate' => 'Tanggal tak diketahui',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Dibalik horisontal', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Diputar 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Dibalik vertikal', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Diputar 90° CCW dan dibalik vertikal', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Diputar 90° CW', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Diputar 90° CW dan dibalik vertikal', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Diputar 90° CCW', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'format chunky',
'exif-planarconfiguration-2' => 'format planar',

'exif-componentsconfiguration-0' => 'tak tersedia',

'exif-exposureprogram-0' => 'Tak terdefinisi',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Program normal',
'exif-exposureprogram-3' => 'Prioritas bukaan',
'exif-exposureprogram-4' => 'Prioritas penutup',
'exif-exposureprogram-5' => 'Program kreatif (condong ke kedalaman ruang)',
'exif-exposureprogram-6' => 'Program aksi (condong ke kecepatan rana)',
'exif-exposureprogram-7' => 'Modus potret (untuk foto closeup dengan latar belakang tak fokus)',
'exif-exposureprogram-8' => 'Modus pemandangan (untuk foto pemandangan dengan latar belakang fokus)',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0'   => 'Tak diketahui',
'exif-meteringmode-1'   => 'Rerata',
'exif-meteringmode-2'   => 'RerataBerbobot',
'exif-meteringmode-3'   => 'Terpusat',
'exif-meteringmode-4'   => 'BanyakPusat',
'exif-meteringmode-5'   => 'Pola',
'exif-meteringmode-6'   => 'Parsial',
'exif-meteringmode-255' => 'Lain-lain',

'exif-lightsource-0'   => 'Tak diketahui',
'exif-lightsource-1'   => 'Cahaya siang',
'exif-lightsource-2'   => 'Pendarflour',
'exif-lightsource-3'   => 'Wolfram (cahaya pijar)',
'exif-lightsource-4'   => 'Kilas',
'exif-lightsource-9'   => 'Cuaca baik',
'exif-lightsource-10'  => 'Cuaca berkabut',
'exif-lightsource-11'  => 'Bayangan',
'exif-lightsource-12'  => 'Pendarflour cahaya siang (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Pendarflour putih siang (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Pendarflour putih teduh (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Pendarflour putih (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Cahaya standar A',
'exif-lightsource-18'  => 'Cahaya standar B',
'exif-lightsource-19'  => 'Cahaya standar C',
'exif-lightsource-24'  => 'studio ISO tungsten',
'exif-lightsource-255' => 'Sumber cahaya lain',

'exif-focalplaneresolutionunit-2' => 'inci',

'exif-sensingmethod-1' => 'Tak terdefinisi',
'exif-sensingmethod-2' => 'Sensor area warna satu keping',
'exif-sensingmethod-3' => 'Sensor area warna dua keping',
'exif-sensingmethod-4' => 'Sensor area warna tiga keping',
'exif-sensingmethod-5' => 'Sensor area warna berurut',
'exif-sensingmethod-7' => 'Sensor trilinear',
'exif-sensingmethod-8' => 'Sensor linear warna berurut',

'exif-scenetype-1' => 'Gambar foto langsung',

'exif-customrendered-0' => 'Proses normal',
'exif-customrendered-1' => 'Proses kustom',

'exif-exposuremode-0' => 'Pajanan otomatis',
'exif-exposuremode-1' => 'Pajanan manual',
'exif-exposuremode-2' => 'Braket otomatis',

'exif-whitebalance-0' => 'Keseimbangan putih otomatis',
'exif-whitebalance-1' => 'Keseimbangan putih manual',

'exif-scenecapturetype-0' => 'Standar',
'exif-scenecapturetype-1' => 'Melebar',
'exif-scenecapturetype-2' => 'Potret',
'exif-scenecapturetype-3' => 'Pemandangan malam',

'exif-gaincontrol-0' => 'Tak ada',
'exif-gaincontrol-1' => 'Naikkan fokus rendah',
'exif-gaincontrol-2' => 'Naikkan fokus tinggi',
'exif-gaincontrol-3' => 'Turunkan fokus rendah',
'exif-gaincontrol-4' => 'Turunkan fokus tinggi',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Lembut',
'exif-contrast-2' => 'Keras',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Saturasi rendah',
'exif-saturation-2' => 'Saturasi tinggi',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Lembut',
'exif-sharpness-2' => 'Keras',

'exif-subjectdistancerange-0' => 'Tak diketahui',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Tampak dekat',
'exif-subjectdistancerange-3' => 'Tampak jauh',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Lintang utara',
'exif-gpslatitude-s' => 'Lintang selatan',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Bujur timur',
'exif-gpslongitude-w' => 'Bujur barat',

'exif-gpsstatus-a' => 'Pengukuran sedang berlangsung',
'exif-gpsstatus-v' => 'Interoperabilitas pengukuran',

'exif-gpsmeasuremode-2' => 'Pengukuran 2-dimensi',
'exif-gpsmeasuremode-3' => 'Pengukuran 3-dimensi',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometer per jam',
'exif-gpsspeed-m' => 'Mil per jam',
'exif-gpsspeed-n' => 'Knot',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Arah sejati',
'exif-gpsdirection-m' => 'Arah magnetis',

# External editor support
'edit-externally'      => 'Sunting berkas ini dengan aplikasi luar',
'edit-externally-help' => 'Lihat [http://www.mediawiki.org/wiki/Manual:External_editors instruksi pengaturan] untuk informasi lebih lanjut.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'semua',
'imagelistall'     => 'semua',
'watchlistall2'    => 'semua',
'namespacesall'    => 'semua',
'monthsall'        => 'semua',

# E-mail address confirmation
'confirmemail'             => 'Konfirmasi alamat surat-e',
'confirmemail_noemail'     => 'Anda tidak memberikan alamat surat-e yang sah di [[Special:Preferences|preferensi pengguna]] Anda.',
'confirmemail_text'        => 'Wiki ini mengharuskan Anda untuk melakukan konfirmasi atas alamat surat elektronik Anda sebelum fitur-fitur surat elektronik dapat digunakan. Tekan tombol di bawah ini untuk mengirimi Anda sebuah surat elektronik yang berisi kode konfirmasi yang berupa sebuah alamat internet. Salin alamat tersebut ke penjelajah web Anda dan buka alamat tersebut untuk melakukan konfirmasi sehingga menginformasikan bahwa alamat surat elektronik Anda valid.',
'confirmemail_pending'     => '<div class="error">Suatu kode konfirmasi telah dikirimkan kepada Anda; jika Anda baru saja membuat akun Anda, silakan tunggu beberapa menit untuk surat tersebut tiba sebelum mencoba untuk meminta satu kode baru.</div>',
'confirmemail_send'        => 'Kirim kode konfirmasi',
'confirmemail_sent'        => 'Surat elektronik berisi kode konfirmasi telah dikirim.',
'confirmemail_oncreate'    => 'Suatu kode konfirmasi telah dikirimkan ke alamat surat-e Anda. Kode ini tidak dibutuhkan untuk masuk log, tapi dibutuhkan sebelum menggunakan semua fitur yang menggunakan surat-e di wiki ini.',
'confirmemail_sendfailed'  => '{{SITENAME}} tidak berhasil mengirimkan surat konfirmasi Anda.
Harap cek kemungkinan karakter ilegal pada alamat surat-e.

Aplikasi pengiriman surat-e menginformasikan: $1',
'confirmemail_invalid'     => 'Kode konfirmasi salah. Kode tersebut mungkin sudah kadaluwarsa.',
'confirmemail_needlogin'   => 'Anda harus melakukan $1 untuk mengkonfirmasikan alamat surat-e Anda.',
'confirmemail_success'     => 'Alamat surat-e Anda telah dikonfirmasi. Sekarang Anda dapat masuk log dan mulai menggunakan wiki.',
'confirmemail_loggedin'    => 'Alamat surat elektronik Anda telah dikonfirmasi.',
'confirmemail_error'       => 'Terjadi kesalahan sewaktu menyimpan konfirmasi Anda.',
'confirmemail_subject'     => 'Konfirmasi alamat surat-e {{SITENAME}}',
'confirmemail_body'        => 'Seseorang, mungkin Anda, dari alamat IP $1, telah mendaftarkan akun "$2" dengan alamat surat-e ini di {{SITENAME}}.

Untuk mengkonfirmasikan bahwa akun ini benar dimiliki oleh Anda sekaligus mengaktifkan fitur surat-e di {{SITENAME}}, ikuti pranala berikut pada penjelajah web Anda:

$3

Jika Anda merasa *tidak pernah* mendaftar, jangan ikuti pranala di atas.
Klik pada pranala ini untuk membatalkan konfirmasi alamat surat-e:

$5

Kode konfirmasi ini akan kadaluwarsa pada $4.',
'confirmemail_invalidated' => 'Konfirmasi alamat surat-e dibatalkan',
'invalidateemail'          => 'Batalkan konfirmasi surat-e',

# Scary transclusion
'scarytranscludedisabled' => '[Transklusi interwiki dimatikan]',
'scarytranscludefailed'   => '[Pengambilan templat $1 gagal]',
'scarytranscludetoolong'  => '[URL terlalu panjang]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Pelacakan balik untuk artikel ini:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Hapus])',
'trackbacklink'     => 'Lacak balik',
'trackbackdeleteok' => 'Pelacakan balik berhasil dihapus.',

# Delete conflict
'deletedwhileediting' => "'''Peringatan''': Halaman ini telah dihapus setelah Anda mulai melakukan penyuntingan!",
'confirmrecreate'     => "Pengguna [[User:$1|$1]] ([[User talk:$1|bicara]]) telah menghapus halaman selagi Anda mulai melakukan penyuntingan dengan alasan:
: ''$2''
Silakan konfirmasi jika Anda ingin membuat ulang halaman ini.",
'recreate'            => 'Buat ulang',

# HTML dump
'redirectingto' => 'Sedang dialihkan ke [[:$1]]...',

# action=purge
'confirm_purge'        => "Hapus ''cache'' halaman ini?

$1",
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Mencari artikel yang mengandung ''$1''.",
'searchnamed'      => "Mencari artikel yang berjudul ''$1''.",
'articletitles'    => "Artikel yang diawali ''$1''",
'hideresults'      => 'Sembunyikan hasil',
'useajaxsearch'    => 'Gunakan pencarian AJAX',

# Multipage image navigation
'imgmultipageprev' => '&larr; halaman sebelumnya',
'imgmultipagenext' => 'halaman selanjutnya &rarr;',
'imgmultigo'       => 'Cari!',
'imgmultigoto'     => 'Pergi ke halaman $1',

# Table pager
'ascending_abbrev'         => 'naik',
'descending_abbrev'        => 'turun',
'table_pager_next'         => 'Halaman selanjutnya',
'table_pager_prev'         => 'Halaman sebelumnya',
'table_pager_first'        => 'Halaman pertama',
'table_pager_last'         => 'Halaman terakhir',
'table_pager_limit'        => 'Tampilkan $1 entri per halaman',
'table_pager_limit_submit' => 'Cari',
'table_pager_empty'        => 'Tidak ditemukan',

# Auto-summaries
'autosumm-blank'   => '←Mengosongkan halaman',
'autosumm-replace' => "←Mengganti halaman dengan '$1'",
'autoredircomment' => '←Mengalihkan ke [[$1]]',
'autosumm-new'     => "←Membuat halaman berisi '$1'",

# Live preview
'livepreview-loading' => 'Memuat…',
'livepreview-ready'   => 'Memuat… Selesai!',
'livepreview-failed'  => 'Pratayang langsung gagal!
Coba dengan pratayang normal.',
'livepreview-error'   => 'Gagal tersambung: $1 "$2".
Coba dengan pratayang normal.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Perubahan yang lebih baru dari $1 {{PLURAL:$1|detik|detik}} mungkin tidak muncul di daftar ini.',
'lag-warn-high'   => 'Karenanya besarnya keterlambatan basis data server, perubahan yang lebih baru dari $1 {{PLURAL:$1|detik|detik}} mungkin tidak muncul di daftar ini.',

# Watchlist editor
'watchlistedit-numitems'       => 'Daftar pantauan Anda berisi {{PLURAL:$1|1 judul|$1 judul}}, tidak termasuk halaman pembicaraan.',
'watchlistedit-noitems'        => 'Daftar pantauan Anda kosong.',
'watchlistedit-normal-title'   => 'Sunting daftar pantauan',
'watchlistedit-normal-legend'  => 'Hapus judul dari daftar pantauan',
'watchlistedit-normal-explain' => 'Judul-judul pada daftar pantauan Anda ditampilkan di bawah ini.
Untuk menghapus suatu judul, berikan tanda cek pada kotak di sampingnya, dan klik Hapus Judul.
Anda juga dapat [[Special:Watchlist/raw|menyunting daftar mentahnya]].',
'watchlistedit-normal-submit'  => 'Hapus judul',
'watchlistedit-normal-done'    => '{{PLURAL:$1|satu|$1}} judul telah dihapus dari daftar pantauan Anda:',
'watchlistedit-raw-title'      => 'Sunting daftar mentah',
'watchlistedit-raw-legend'     => 'Sunting daftar mentah',
'watchlistedit-raw-explain'    => 'Judul-judul pada daftar pantauan Anda ditampilkan di bawah ini, dan dapat diubah dengan menambahkan atau menghapus daftar; satu judul pada setiap barisnya. Jika telah selesai, klik Perbarui daftar pantauan. Anda juga dapat [[Special:Watchlist/edit|menggunakan editor standar Anda]].',
'watchlistedit-raw-titles'     => 'Judul:',
'watchlistedit-raw-submit'     => 'Perbarui daftar pantauan',
'watchlistedit-raw-done'       => 'Daftar pantauan Anda telah diperbarui.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 judul telah|$1 judul telah}} ditambahkan:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 judul telah|$1 judul telah}} dikeluarkan:',

# Watchlist editing tools
'watchlisttools-view' => 'Tampilkan perubahan terkait',
'watchlisttools-edit' => 'Tampilkan dan sunting daftar pantauan',
'watchlisttools-raw'  => 'Sunting daftar pantauan mentah',

# Core parser functions
'unknown_extension_tag' => 'Tag ekstensi tidak dikenal "$1"',

# Special:Version
'version'                          => 'Versi', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Ekstensi terinstal',
'version-specialpages'             => 'Halaman istimewa',
'version-parserhooks'              => 'Kait parser',
'version-variables'                => 'Variabel',
'version-other'                    => 'Lainnya',
'version-mediahandlers'            => 'Penanganan media',
'version-hooks'                    => 'Kait',
'version-extension-functions'      => 'Fungsi ekstensi',
'version-parser-extensiontags'     => 'Tag ekstensi parser',
'version-parser-function-hooks'    => 'Kait fungsi parser',
'version-skin-extension-functions' => 'Fungsi ekstensi kulit',
'version-hook-name'                => 'Nama kait',
'version-hook-subscribedby'        => 'Dilanggani oleh',
'version-version'                  => 'Versi',
'version-license'                  => 'Lisensi',
'version-software'                 => 'Perangkat lunak terinstal',
'version-software-product'         => 'Produk',
'version-software-version'         => 'Versi',

# Special:FilePath
'filepath'         => 'Lokasi berkas',
'filepath-page'    => 'Berkas:',
'filepath-submit'  => 'Lokasi',
'filepath-summary' => 'Halaman istimewa ini menampilkan jalur lengkap untuk suatu berkas.
Gambar ditampilkan dalam resolusi penuh dan tipe lain berkas akan dibuka langsung dengan program terkaitnya.

Masukkan nama berkas tanpa prefiks "{{ns:image}}:"-nya.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Pencarian berkas duplikat',
'fileduplicatesearch-summary'  => 'Pencarian duplikat berkas berdasarkan nilai hash-nya.

Masukkan nama berkas tanpa prefiks "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'Cari duplikat',
'fileduplicatesearch-filename' => 'Nama berkas:',
'fileduplicatesearch-submit'   => 'Cari',
'fileduplicatesearch-info'     => '$1 × $2 piksel<br />Besar berkas: $3<br />Tipe MIME: $4',
'fileduplicatesearch-result-1' => 'Berkas "$1" tidak memiliki duplikat identik.',
'fileduplicatesearch-result-n' => 'Berkas "$1" memiliki {{PLURAL:$2|1 duplikat identik|$2 duplikat identik}}.',

# Special:SpecialPages
'specialpages'                   => 'Halaman istimewa',
'specialpages-note'              => '----
Keterangan tampilan:
* Halaman istimewa normal
* <span class="mw-specialpagerestricted">Halaman istimewa terbatas</span>',
'specialpages-group-maintenance' => 'Laporan pemeliharaan',
'specialpages-group-other'       => 'Halaman istimewa lainnya',
'specialpages-group-login'       => 'Masuk log / mendaftar',
'specialpages-group-changes'     => 'Perubahan terbaru dan log',
'specialpages-group-media'       => 'Laporan dan pemuatan berkas',
'specialpages-group-users'       => 'Pengguna dan hak pengguna',
'specialpages-group-highuse'     => 'Frekuensi tinggi',
'specialpages-group-pages'       => 'Daftar halaman',
'specialpages-group-pagetools'   => 'Peralatan halaman',
'specialpages-group-wiki'        => 'Data dan peralatan wiki',
'specialpages-group-redirects'   => 'Mengalihkan halaman istimewa',
'specialpages-group-spam'        => 'Peralatan spam',

# Special:BlankPage
'blankpage'              => 'Halaman kosong',
'intentionallyblankpage' => 'Halaman ini sengaja dibiarkan kosong dan digunakan di antaranya untuk pengukuran kinerja, dan lain-lain.',

);
