<?php
/** Javanese (Basa Jawa)
 *
 * @ingroup Language
 * @file
 *
 * @author Helix84
 * @author Meursault2004
 * @author לערי ריינהארט
 */

$fallback = 'id';

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Astamiwa',
	NS_MAIN             => '',
	NS_TALK             => 'Dhiskusi',
	NS_USER             => 'Panganggo',
	NS_USER_TALK        => 'Dhiskusi_Panganggo',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Dhiskusi_$1',
	NS_IMAGE            => 'Gambar',
	NS_IMAGE_TALK       => 'Dhiskusi_Gambar',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Dhiskusi_MediaWiki',
	NS_TEMPLATE         => 'Cithakan',
	NS_TEMPLATE_TALK    => 'Dhiskusi_Cithakan',
	NS_HELP             => 'Pitulung',
	NS_HELP_TALK        => 'Dhiskusi_Pitulung',
	NS_CATEGORY         => 'Kategori',
	NS_CATEGORY_TALK    => 'Dhiskusi_Kategori'
);

$namespaceAliases = array(
	'Gambar_Dhiskusi' => NS_IMAGE_TALK,
	'MediaWiki_Dhiskusi' => NS_MEDIAWIKI_TALK,
	'Cithakan_Dhiskusi' => NS_TEMPLATE_TALK,
	'Pitulung_Dhiskusi' => NS_HELP_TALK,
	'Kategori_Dhiskusi' => NS_CATEGORY_TALK,
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Garisen ngisoré pranala:',
'tog-highlightbroken'         => 'Format pranala tugel <a href="" class="new">kaya iki</a> (pilihan: kaya iki<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Ratakna paragraf',
'tog-hideminor'               => 'Delikna suntingan cilik ing owah-owahan pungkasan',
'tog-extendwatchlist'         => 'Tuduhna daftar pangawasan sing nuduhaké kabèh pangowahan',
'tog-usenewrc'                => 'Tampilan pangowahan pungkasan alternatif (JavaScript)',
'tog-numberheadings'          => 'Wènèhana nomer judul secara otomatis',
'tog-showtoolbar'             => 'Tuduhna <em>toolbar</em> (batang piranti) panyuntingan',
'tog-editondblclick'          => 'Sunting kaca nganggo klik ping loro (JavaScript)',
'tog-editsection'             => 'Fungsèkna panyuntingan sub-bagian ngliwati pranala [sunting]',
'tog-editsectiononrightclick' => 'Fungsèkna panyuntingan sub-bagian mawa klik-tengen ing judul bagian (JavaScript)',
'tog-showtoc'                 => 'Tuduhna daftar isi (kanggo kaca sing nduwé luwih saka 3 sub-bagian)',
'tog-rememberpassword'        => 'Éling tembung sandi ing saben sési',
'tog-editwidth'               => 'Kothak sunting mawa ukuran maksimum',
'tog-watchcreations'          => 'Tambahna kaca sing tak-gawé ing daftar pangawasan',
'tog-watchdefault'            => 'Tambahna kaca sing tak-sunting ing daftar pangawasan',
'tog-watchmoves'              => 'Tambahkan kaca sing tak-pindhah ing daftar pangawasan',
'tog-watchdeletion'           => 'Tambahkan kaca sing tak-busak ing daftar pangawasan',
'tog-minordefault'            => 'Tandhanana kabèh suntingan dadi suntingan cilik secara baku',
'tog-previewontop'            => 'Tuduhna pratayang sadurungé kothak sunting lan ora sawisé',
'tog-previewonfirst'          => 'Tuduhna pratayang ing suntingan kapisan',
'tog-nocache'                 => 'Patènana <em>cache</em> kaca',
'tog-enotifwatchlistpages'    => 'Kirimana aku layang e-mail yèn ana sawijining kaca sing tak-awasi owah',
'tog-enotifusertalkpages'     => 'Kirimana aku layang e-mail yèn kaca dhiskusiku owah',
'tog-enotifminoredits'        => 'Kirimana aku layang e-mail uga yèn ana pangowahan cilik',
'tog-enotifrevealaddr'        => 'Kirimana aku layang e-mail ing layang notifikasi',
'tog-shownumberswatching'     => 'Tuduhna cacahé pangawas',
'tog-fancysig'                => 'Tapak asta mentah (tanpa pranala otomatis)',
'tog-externaleditor'          => 'Nganggoa program pangolah tembung jaba (external wordprocessor)',
'tog-externaldiff'            => 'Nganggoa program njaba kanggo mirsani prabédan suntingan',
'tog-showjumplinks'           => 'Aktifna pranala pambiyantu "langsung menyang"',
'tog-uselivepreview'          => 'Nganggoa pratayang langsung (JavaScript) (eksperimental)',
'tog-forceeditsummary'        => 'Élingna aku menawa kothak ringkesan suntingan isih kosong',
'tog-watchlisthideown'        => 'Delikna suntinganku ing daftar pangawasan',
'tog-watchlisthidebots'       => 'Delikna suntingan ing daftar pangawasan',
'tog-watchlisthideminor'      => 'Delikna suntingan kecil di daftar pangawasan',
'tog-nolangconversion'        => 'Patènana konvèrsi varian',
'tog-ccmeonemails'            => 'Kirimana aku salinan layang e-mail sing tak-kirimaké menyang wong liya',
'tog-diffonly'                => 'Aja dituduhaké isi kaca ing ngisor bédané suntingan',
'tog-showhiddencats'          => 'Tuduhna kategori sing didelikaké',

'underline-always'  => 'Tansah',
'underline-never'   => 'Ora',
'underline-default' => 'Miturut konfigurasi panjlajah wèb',

'skinpreview' => '(Pratayang)',

# Dates
'sunday'        => 'Minggu',
'monday'        => 'Senèn',
'tuesday'       => 'Slasa',
'wednesday'     => 'Rebo',
'thursday'      => 'Kemis',
'friday'        => 'Jemuwah',
'saturday'      => 'Setu',
'sun'           => 'Min',
'mon'           => 'Sen',
'tue'           => 'Sel',
'wed'           => 'Rab',
'thu'           => 'Kam',
'fri'           => 'Jem',
'sat'           => 'Set',
'january'       => 'Januari',
'february'      => 'Fébruari',
'march'         => 'Maret',
'april'         => 'April',
'may_long'      => 'Méi',
'june'          => 'Juni',
'july'          => 'Juli',
'august'        => 'Agustus',
'september'     => 'September',
'october'       => 'Oktober',
'november'      => 'November',
'december'      => 'Désèmber',
'january-gen'   => 'Januari',
'february-gen'  => 'Fébruari',
'march-gen'     => 'Maret',
'april-gen'     => 'April',
'may-gen'       => 'Méi',
'june-gen'      => 'Juni',
'july-gen'      => 'Juli',
'august-gen'    => 'Agustus',
'september-gen' => 'September',
'october-gen'   => 'Oktober',
'november-gen'  => 'November',
'december-gen'  => 'Désèmber',
'jan'           => 'Jan',
'feb'           => 'Feb',
'mar'           => 'Mar',
'apr'           => 'Apr',
'may'           => 'Méi',
'jun'           => 'Jun',
'jul'           => 'Jul',
'aug'           => 'Agu',
'sep'           => 'Sep',
'oct'           => 'Okt',
'nov'           => 'Nov',
'dec'           => 'Des',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategori|Kategori}}',
'category_header'                => 'Artikel ing kategori "$1"',
'subcategories'                  => 'Subkategori',
'category-media-header'          => 'Média ing kategori "$1"',
'category-empty'                 => "''Kategori iki saiki ora ngandhut artikel utawa média.''",
'hidden-categories'              => '{{PLURAL:$1|Kategori sing didelikaké|Kategori sing didelikaké}}',
'hidden-category-category'       => 'Kategori sing didelikaké', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Kategori iki namung nduwé subkategori ing ngisor ikit.|Dituduhaké {{PLURAL:$1|subkategori|$1 subkategori}} sing kalebu ing kategori iki saka total $2.}}',
'category-subcat-count-limited'  => "Kategori iki ora duwé {{PLURAL:$1|subkategori|$1 subkategori}} ''berikut''.",
'category-article-count'         => '{{PLURAL:$2|Kategori iki namung ndarbèni kaca iki.|Dituduhaké {{PLURAL:$1|kaca|$1 kaca-kaca}} sing kalebu ing kategori iki saka gunggungé $2.}}',
'category-article-count-limited' => 'Kategori iki ngandhut {{PLURAL:$1|kaca|$1 kaca-kaca}} sing kapacak ing ngisor iki.',
'category-file-count'            => '{{PLURAL:$2|Kategori iki namung nduwé berkas iki.|Dituduhaké {{PLURAL:$1|berkas|$1 berkas-berkas}} sing kalebu ing kategori iki saka gunggungé $2.}}',
'category-file-count-limited'    => 'Kategori iki ndarbèni {{PLURAL:$1|berkas|$1 berkas-berkas}} sing kapacak ing ngisor iki.',
'listingcontinuesabbrev'         => 'samb.',

'mainpagetext'      => 'Prangkat empuk wiki wis suksès dipasang.',
'mainpagedocfooter' => "Mangga maca [http://meta.wikimedia.org/wiki/Help:Contents User's Guide] kanggo katrangan luwih langkung prakara panggunan prangkat empuk wiki
== Miwiti panggunan  ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Daftar pangaturan préférènsi]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://mail.wikimedia.org/mailman/listinfo/mediawiki-announce Milis rilis MediaWiki]",

'about'          => 'Prakara',
'article'        => 'Artikel',
'newwindow'      => '(buka ing jendhéla anyar)',
'cancel'         => 'Batalna',
'qbfind'         => 'Golèk',
'qbbrowse'       => 'Navigasi',
'qbedit'         => 'Sunting',
'qbpageoptions'  => 'Kaca iki',
'qbpageinfo'     => 'Kontèks kaca',
'qbmyoptions'    => 'Opsiku',
'qbspecialpages' => 'Kaca-kaca astaméwa',
'moredotdotdot'  => 'Liyané...',
'mypage'         => 'Kacaku',
'mytalk'         => 'Gunemanku',
'anontalk'       => 'Dhiskusi IP puniki',
'navigation'     => 'Pandhu Arah',
'and'            => 'Lan',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Kasalahan',
'returnto'          => 'Bali menyang $1.',
'tagline'           => 'Saka {{SITENAME}}',
'help'              => 'Pitulung',
'search'            => 'Panggolèkan',
'searchbutton'      => 'Golèk',
'go'                => 'Nuju menyang',
'searcharticle'     => 'Nuju menyang',
'history'           => 'Vèrsi sadurungé',
'history_short'     => 'Vèrsi lawas',
'updatedmarker'     => 'diowahi wiwit kunjungan pungkasanku',
'info_short'        => 'Informasi',
'printableversion'  => 'Versi cithak',
'permalink'         => 'Pranala permanèn',
'print'             => 'Cithak',
'edit'              => 'Sunting',
'create'            => 'Nggawé',
'editthispage'      => 'Sunting kaca iki',
'create-this-page'  => 'Nggawé kaca iki',
'delete'            => 'Busak',
'deletethispage'    => 'Busak kaca iki',
'undelete_short'    => 'Batal busak $1 {{PLURAL:$1|suntingan|suntingan}}',
'protect'           => 'Reksanen',
'protect_change'    => 'ngowahi reksanan',
'protectthispage'   => 'Reksanen kaca iki',
'unprotect'         => 'Pangreksan',
'unprotectthispage' => 'Owahana pangreksan kaca iki',
'newpage'           => 'Kaca anyar',
'talkpage'          => 'Dhiskusèkna kaca iki',
'talkpagelinktext'  => 'Wicara',
'specialpage'       => 'Kaca astaméwa',
'personaltools'     => 'Piranti pribadi',
'postcomment'       => 'Kirim komentar',
'articlepage'       => 'nDeleng artikel',
'talk'              => 'Dhiskusi',
'views'             => 'Tampilan',
'toolbox'           => 'Kothak piranti',
'userpage'          => 'Ndeleng kaca panganggo',
'projectpage'       => 'Ndeleng kaca proyèk',
'imagepage'         => 'Ndeleng kaca berkas',
'mediawikipage'     => 'Ndeleng kaca pesen sistem',
'templatepage'      => 'Ndeleng kaca cithakan',
'viewhelppage'      => 'Ndeleng kaca pitulung',
'categorypage'      => 'Ndeleng kaca kategori',
'viewtalkpage'      => 'Ndeleng kaca dhiskusi',
'otherlanguages'    => 'Basa liya',
'redirectedfrom'    => '(Dialihkan dari $1)',
'redirectpagesub'   => 'Kaca pangalihan',
'lastmodifiedat'    => 'Kaca iki diowahi pungkasané nalika $2, $1.', # $1 date, $2 time
'viewcount'         => 'Kaca iki wis tau diaksès cacahé ping {{PLURAL:$1|siji|$1}}.',
'protectedpage'     => 'Kaca sing direksa',
'jumpto'            => 'Langsung menyang:',
'jumptonavigation'  => 'navigasi',
'jumptosearch'      => 'golèk',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Prakara {{SITENAME}}',
'aboutpage'            => 'Project:Prakara',
'bugreports'           => 'Laporan bug',
'bugreportspage'       => 'Project:Laporan bug',
'copyright'            => 'Kabèh tèks kasedyakaké miturut $1.',
'copyrightpagename'    => 'Hak cipta {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Hak cipta',
'currentevents'        => 'Prastawa saiki',
'currentevents-url'    => 'Project:Prastawa saiki',
'disclaimers'          => 'Pamaidonan',
'disclaimerpage'       => 'Project:Panyangkalan umum',
'edithelp'             => 'Pitulung panyuntingan',
'edithelppage'         => 'Help:panyuntingan',
'faq'                  => 'FAQ (Pitakonan sing kerep diajokaké)',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Isi',
'mainpage'             => 'Kaca Utama',
'mainpage-description' => 'Kaca Utama',
'policy-url'           => 'Project:Kabijakan',
'portal'               => 'Gapura komunitas',
'portal-url'           => 'Project:Portal komunitas',
'privacy'              => 'Kebijakan privasi',
'privacypage'          => 'Project:Kabijakan privasi',

'badaccess'        => 'Aksès ora olèh',
'badaccess-group0' => 'Panjenengan ora pareng nglakokaké tindhakan sing panjenengan gayuh.',
'badaccess-group1' => 'Pratingkah sing panjenengan suwun namung bisa kanggo panganggo klompok $1.',
'badaccess-group2' => 'Pratingkah sing panjenengan suwun diwatesi kanggo panganggo ing klompok $1.',
'badaccess-groups' => 'Pratingkah panjenengan diwatesi tumrap panganggo ing klompoké $1.',

'versionrequired'     => 'Dibutuhaké MediaWiki vèrsi $1',
'versionrequiredtext' => 'MediaWiki vèrsi $1 dibutuhaké kanggo nggunakaké kaca iki. Mangga mirsani [[Special:Version|kaca iki]]',

'ok'                      => 'OK',
'retrievedfrom'           => 'Sumber artikel iki saka kaca situs web: "$1"',
'youhavenewmessages'      => 'Panjenengan kagungan $1 ($2).',
'newmessageslink'         => 'warta énggal',
'newmessagesdifflink'     => 'mirsani bédané saka révisi sadurungé',
'youhavenewmessagesmulti' => 'Panjenengan olèh pesen-pesen anyar $1',
'editsection'             => 'sunting',
'editold'                 => 'sunting',
'viewsourceold'           => 'deleng sumber',
'editsectionhint'         => 'Sunting bagian: $1',
'toc'                     => 'Bab lan paragraf',
'showtoc'                 => 'tuduhna',
'hidetoc'                 => 'delikna',
'thisisdeleted'           => 'Mirsani utawa mbalèkaké $1?',
'viewdeleted'             => 'Mirsani $1?',
'restorelink'             => '$1 {{PLURAL:$1|suntingan|suntingan}} sing wis kabusak',
'feedlinks'               => 'Asupan:',
'feed-invalid'            => 'Tipe permintaan asupan ora bener.',
'feed-unavailable'        => "Umpan sindikasi (''syndication feeds'') ora ana ing {{SITENAME}}",
'site-rss-feed'           => "$1 ''RSS Feed''",
'site-atom-feed'          => "$1 ''Atom Feed''",
'page-rss-feed'           => "\"\$1\" ''RSS Feed''",
'page-atom-feed'          => "\"\$1\" ''Atom Feed''",
'red-link-title'          => '$1 (durung digawé)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artikel',
'nstab-user'      => 'Panganggo',
'nstab-media'     => 'Media',
'nstab-special'   => 'Astamewa',
'nstab-project'   => 'Proyek',
'nstab-image'     => 'Gambar',
'nstab-mediawiki' => 'Pariwara',
'nstab-template'  => 'Cithak',
'nstab-help'      => 'Pitulung',
'nstab-category'  => 'Kategori',

# Main script and global functions
'nosuchaction'      => 'Ora ana pratingkah kaya ngono',
'nosuchactiontext'  => 'Pratingkah sing dirinci déning URL kaya ngono ora ditepangi déning wiki.',
'nosuchspecialpage' => 'Ora ana kaca astaméwa kaya ngono',
'nospecialpagetext' => 'Panjenengan nyuwun kaca astaméwa sing ora sah. Daftar kaca astaméwa sing sah bisa dipirsani ing [[Special:SpecialPages|daftar kaca astaméwa]].',

# General errors
'error'                => 'Kasalahan',
'databaseerror'        => 'Kasalahan database',
'dberrortext'          => 'Ana kasalahan sintaks ing panyuwunan database. Kasalah ini mbokmenawa nuduhi anané \'\'bug\'\' ing software. Panyuwunan database sing pungkasan iku: <blockquote><tt>$1</tt></blockquote> saka jeroning fungsi "<tt>$2</tt>". Kasalahan MySQL "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Ana kasalahan sintaks ing panyuwunan database. Panyuwunan database sing pungkasan iku: "$1" saka jeroning fungsi "$2". Kasalahan MySQL "$3: $4".',
'noconnect'            => 'Nuwun séwu! Wiki ngalami masalah tèknis lan ora bisa ngubungi database.<br />
$1',
'nodb'                 => 'Ora bisa milih database $1',
'cachederror'          => 'Ing ngisor iki tuladan <em>cache</em> saka kaca sing disuwun, dadi mbokmenawa ora up-to-date.',
'laggedslavemode'      => 'Pènget: Kaca iki mbokmenawa isiné dudu pangowahan pungkasan.',
'readonly'             => 'Database dikunci',
'enterlockreason'      => 'Lebokna alesan panguncèn, kalebu uga prakiran kapan kunci bakal dibuka',
'readonlytext'         => 'Database lagi dikunci marang panampan anyar. Pangurus sing ngunci mènèhi katrangan kaya mangkéné: <p>$1',
'missingarticle-rev'   => '(révisi#: $1)',
'missingarticle-diff'  => '(Béda: $1, $2)',
'readonly_lag'         => 'Database wis dikunci mawa otomatis sawetara database sékundhèr lagi nglakoni sinkronisasi mawa database utama',
'internalerror'        => 'Kasalahan internal',
'internalerror_info'   => 'Kaluputan internal: $1',
'filecopyerror'        => 'Ora bisa nulad berkas "$1" menyang "$2".',
'filerenameerror'      => 'Ora bisa ngowahi saka "$1" dadi "$2".',
'filedeleteerror'      => 'Ora bisa mbusak berkas "$1".',
'directorycreateerror' => 'Ora bisa nggawé dirèktori "$1".',
'filenotfound'         => 'Ora bisa nemokaké berkas "$1".',
'fileexistserror'      => 'Ora bisa nulis berkas "$1": berkas wis ana',
'unexpected'           => 'Biji (\'\'nilai\'\') ing njabaning jangkauan: "$1"="$2".',
'formerror'            => 'Kasalahan: Ora bisa ngirimaké formulir',
'badarticleerror'      => 'Pratingkah iku ora bisa katindhakaké ing kaca iki.',
'cannotdelete'         => 'Ora bisa mbusak kaca, gambar utawa berkas sing disuwun.',
'badtitle'             => 'Judhulé ora sah',
'badtitletext'         => 'Judhul kaca sing panjenengan ora bisa dituduhaké, kosong, utawa dadi judhul antar-basa utawa judhul antar-wiki. Iku bisa uga ana  sawijining utawa luwih aksara sing ora bisa didadèkaké judhul.',
'perfdisabled'         => 'Nuwun sèwu! Fitur iki sawetara dipatèni amerga nggawé rindhik databasé nganti ora ana sing bisa nganggo wiki iki.',
'perfcached'           => 'Data iki dijupuk saka <em>cache</em> lan mbokmenawa dudu data pungkasan:',
'perfcachedts'         => 'Data iki dijupuk saka <em>cache</em>, lan dianyaraké ing pungkasan ing $1.',
'querypage-no-updates' => 'Update saka kaca iki lagi dipatèni. Data sing ana ing kéné saiki ora bisa bakal dibalèni unggah manèh.',
'wrong_wfQuery_params' => 'Parameter salah menyang wfQuery()<br />Fungsi: $1<br />Panyuwunan: $2',
'viewsource'           => 'Tuduhna sumber',
'viewsourcefor'        => 'saka $1',
'actionthrottled'      => 'Tindakan diwatesi',
'actionthrottledtext'  => 'Minangka sawijining pepesthèn anti-spam, panjenengan diwatesi nglakoni tindhakan iki sing cacahé kakèhan ing wektu cendhak. 
Mangga dicoba manèh ing sawetara menit.',
'protectedpagetext'    => 'Kaca iki dikunci supaya ora disunting.',
'viewsourcetext'       => 'Panjenengan bisa mirsani utawa nulad sumber kaca iki:',
'protectedinterface'   => 'Kaca iki isiné tèks antarmuka sing dienggo software lan wis dikunci kanggo menghindari kasalahan.',
'editinginterface'     => "'''Pènget:''' Panjenengan nyunting kaca sing dienggo nyedyakaké tèks antarmuka mawa software. Pangowahan tèks iki bakal awèh pangaruh tampilan ing panganggo liya.",
'sqlhidden'            => '(Panyuwunan SQL didelikaké)',
'cascadeprotected'     => 'Kaca iki wis direksa saka panyuntingan amerga disertakaké ing {{PLURAL:$1|kaca|kaca-kaca}} ngisor iki sing wis direksa mawa opsi "runtun" diaktifaké:
$2',
'namespaceprotected'   => "Panjenengan ora kagungan idin kanggo nyunting kaca ing bilik nama '''$1'''.",
'customcssjsprotected' => 'Panjenengan ora kagungan idin kanggo nyunting kaca iki amerga ngandhut pangaturan pribadi panganggo liya.',
'ns-specialprotected'  => 'Kaca ing bilik nama astaméwa utawa kusus, ora bisa disunting.',
'titleprotected'       => "Irah-irahan iki direksa ora olèh digawé déning [[User:$1|$1]].
Alesané yaiku ''$2''.",

# Virus scanner
'virus-scanfailed'     => "''Pemindaian'' utawa ''scan'' gagal (kode $1)",
'virus-unknownscanner' => 'Antivirus ora ditepungi:',

# Login and logout pages
'logouttitle'                => 'Metu log panganggo',
'logouttext'                 => "Panjenengan wis metu (oncat) saka cathetan sistem. Panjenengan bisa migunakaké {{SITENAME}} kanthi anonim, utawa panjenengan bisa mlebu manèh .
Supaya dimangertèni bilih ana kaca sing isih nganggo panjenengan kacathet ing sistém amerga panjenengan durung mbusak <em>cache</em> ''browser'' panjenengan.",
'welcomecreation'            => '== Sugeng rawuh, $1! ==

Akun panjenengan wis kacipta. Aja lali nata konfigurasi {{SITENAME}} panjenengan.',
'loginpagetitle'             => 'Mlebu log panganggo',
'yourname'                   => 'Asma pangangeman',
'yourpassword'               => 'tembung sandhi',
'yourpasswordagain'          => 'Balènana tembung sandhi',
'remembermypassword'         => 'Éling tembung sandhi',
'yourdomainname'             => 'Dhomain panjenengan',
'externaldberror'            => 'Ana kasalahan otèntikasi basis dhata èksternal utawa panjenengan ora pareng nglakoni pemutakhiran marang akun èksternal panjenengan.',
'loginproblem'               => '<strong>Ana masalah ing prosès mlebu log panjenengan.</strong><br />Sumangga nyoba manèh!',
'login'                      => 'Mlebu log',
'nav-login-createaccount'    => 'Log mlebu / nggawé rékening (akun)',
'loginprompt'                => "Panjenengan kudu ngaktifaké ''cookies'' supaya bisa mlebu (log in) ing {{SITENAME}}.",
'userlogin'                  => 'Mlebu log / gawé rékening (akun)',
'logout'                     => 'Oncat',
'userlogout'                 => 'Metu log',
'notloggedin'                => 'Durung mlebu log',
'nologin'                    => 'Durung kagungan asma panganggo? $1.',
'nologinlink'                => 'Ndaftaraké akun anyar',
'createaccount'              => 'Nggawé akun anyar',
'gotaccount'                 => 'Wis kagungan akun? $1.',
'gotaccountlink'             => 'Mlebu',
'createaccountmail'          => 'liwat layang e-mail',
'badretype'                  => 'Sandhi panjenengan ora gathuk',
'userexists'                 => 'Asma panganggo sing panjenengan pilih wis kanggo.
Mangga pilih asma liyané.',
'youremail'                  => 'Layang élèktronik (E-mail):',
'username'                   => 'Asma panganggo:',
'uid'                        => 'ID panganggo:',
'yourrealname'               => 'Asma sajatiné *',
'yourlanguage'               => 'Basa sing dienggo:',
'yourvariant'                => 'Varian basa',
'yournick'                   => 'Asma sesinglon/samaran (kagem tapak asta):',
'badsig'                     => 'Tapak astanipun klentu; cèk rambu HTML.',
'badsiglength'               => 'Tapak tangané kedawan; kudu sangisoré $1 {{PLURAL:$1|karakter|karakter}}.',
'email'                      => 'Layang élèktronik (E-mail)',
'prefs-help-realname'        => '* <strong>Asma asli</strong> (ora wajib): menawa panjenengan maringi, asma asli panjenengan bakal digunakaké kanggo mènèhi akrédhitasi kanggo kasil karya tulis panjenengan.',
'loginerror'                 => 'Kasalahan mlebu log',
'prefs-help-email'           => "* <strong>Layang élèktronik</strong> (ora wajib): Nggawé bisa wong liya ngubungi panjenengan liwat situs tanpa perlu maringi alamat e-mail panjenengan karo wong liya, lan panjenengan uga bisa nyuwun '''tembung sandhi anyar''' menawa panjenengan lali tembung sandhi panjenengan.",
'prefs-help-email-required'  => 'Alamat e-mail dibutuhaké.',
'nocookiesnew'               => "Rékening utawa akun panganggo panjenengan wis digawé, nanging panjenengan durung mlebu log. {{SITENAME}} nggunakaké ''cookies'' kanggo  log panganggo. ''Cookies'' ing panjlajah wèb panjengengan dipatèni. Mangga diaktifaké lan mlebu log manèh mawa jeneng panganggo lan tembung sandhi panjenengan.",
'nocookieslogin'             => "{{SITENAME}} nggunakaké ''cookies'' kanggo log panganggoné. ''Cookies'' ing panjlajah wèb panjenengan dipatèni. Mangga ngaktifaké manèh lan coba manèh.",
'noname'                     => 'Asma panganggo sing panjenengan pilih ora sah.',
'loginsuccesstitle'          => 'Bisa suksès mlebu log',
'loginsuccess'               => "'''Panjenengan saiki mlebu ing {{SITENAME}} kanthi asma \"\$1\".'''",
'nosuchuser'                 => 'Ora ana panganggo mawa asma "$1". Coba dipriksa manèh pasang aksarané (éjaané), utawa mangga ngagem formulir ing ngisor iki kanggo mbukak akun/rékening anyar.',
'nosuchusershort'            => 'Ora ana panganggo mawa asma "$1". Coba dipriksa manèh pasang aksarané (éjaané).',
'nouserspecified'            => 'Panjenengan kudu milih asma panganggo.',
'wrongpassword'              => 'Tembung sandhi sing dipilih salah. Mangga coba manèh.',
'wrongpasswordempty'         => 'Panjenengan ora milih tembung sandhi. Mangga dicoba manèh.',
'passwordtooshort'           => 'Tembung sandi panjenengan ora absah utawa kecendhaken. Tembung sandi kudu katulis saka paling ora $1 aksara lan kudu béda saka jeneng panganggo panjenengan.',
'mailmypassword'             => 'Kirim tembung sandhi anyar',
'passwordremindertitle'      => 'Pèngetan tembung sandhi saka {{SITENAME}}',
'passwordremindertext'       => 'Ana wong (mbokmenawa panjenengan dhéwé, saka alamat IP $1) nyuwun supaya dikirimi tembung sandhi anyar kanggo {{SITENAME}} ($4). Tembung sandhi kanggo panganggo "$2" wis digawé lan saiki "$3". Yèn panjenengan pancèn nggayuh iki, mangga sigra mlebu lan ngganti tembung sandhi.

Yèn wong liya sing nglakoni panyuwunan iki, utawa panjenengan éling tembung sandhiné, lan ora nggayuh ngowahi, panjenengan ora usah nggubris pesen iki lan bisa tetep nganggo tembung sandhi sing lawas.


Panjenengan disaranaké sigra mlebu log lan ngganti tembung sandhi.',
'noemail'                    => 'Ora ana alamat layang e-mail sing kacathet kanggo panganggo "$1".',
'passwordsent'               => 'Tembung sandhi anyar wis dikirim menyang alamat layang e-mail panjenengan sing wis didaftar kanggo "$1". Mangga mlebu log manèh sawisé nampa e-mail iku.',
'blocked-mailpassword'       => "Alamat IP panjenengan diblokir saka panyuntingan, mulané panjenengan ora olèh nganggo fungsi pèngetan tembung sandhi kanggo ''mencegah penyalahgunaan''.",
'eauthentsent'               => 'Sawijining layang élèktronik (e-mail) kanggo ndhedhes (konfirmasi) wis dikirim menyang alamat layang élèktronik panjenengan. Panjenengan kudu nuruti instruksi sajroning layang iku kanggo ndhedhes yèn alamat iku bener kagungané panjenengan. {{SITENAME}} ora bakal ngaktifaké fitur layang élèktronik yèn langkah iki durung dilakoni.',
'throttled-mailpassword'     => "Sawijining pèngetan tembung sandhi wis dikirim ing $1 jam pungkasan iki.
Kanggo ''menghindari penyalahgunaan'', mung tembung sandhi siji waé sing bisa dikirim saben $1 jam.",
'mailerror'                  => 'Kasalahan ing ngirimaké layang e-mail: $1',
'acct_creation_throttle_hit' => 'Nuwun sèwu, panjenengan wis nggawé akun $1. Panjenengan ora bisa nggawé manèh.',
'emailauthenticated'         => 'Alamat layang élèktronik (e-mail) panjenengan wis didhedhes (dikonfirmasi) ing $1.',
'emailnotauthenticated'      => 'Alamat layang élèktronik panjenengan durung didhedhes (dikonfirmasi). Sadurungé didhedhes, panjenengan ora bisa nganggo fitur layang élèktronik (e-mail).',
'noemailprefs'               => 'Panjenengan kudu milih alamat e-mail supaya bisa nganggo fitur iki.',
'emailconfirmlink'           => 'Ndhedhes (konfirmasi) alamat e-mail panjenengan',
'invalidemailaddress'        => 'Alamat e-mail iki ora bisa ditampa amerga formaté ora bener. Tulung lebokna alamat e-mail mawa format sing bener utawa kosongana isi mengkono.',
'accountcreated'             => 'Akun wis kacipta.',
'accountcreatedtext'         => 'Akun kanggo $1 wis kacipta.',
'createaccount-title'        => 'Gawé rékening kanggo {{SITENAME}}',
'createaccount-text'         => 'Ana wong sing nggawé sawijining akun utawa rékening kanggo alamat e-mail panjenengan ing {{SITENAME}} ($4) mawa jeneng "$2" lan tembung sandi "$3". Panjenengan disaranaké kanggo mlebu log lan ngganti tembung sandi panjenengan saiki.

Panjenengan bisa nglirwakaké pesen iki yèn akun utawa rékening iki digawé déné sawijining kaluputan.',
'loginlanguagelabel'         => 'Basa: $1',

# Password reset dialog
'resetpass'               => 'Nata mbalèni tembung sandhi akun',
'resetpass_announce'      => 'Panjenengan wis mlebu log mawa kodhe sementara sing dikirim mawa e-mail. Menawa kersa nglanjutaké, panjenengan kudu milih tembung sandhi anyar ing kéné:',
'resetpass_text'          => '<!-- Tambahkan teks di sini -->',
'resetpass_header'        => 'Nata mbalèni tembung sandhi',
'resetpass_submit'        => 'Nata tembung sandhi lan mlebu log',
'resetpass_success'       => 'Tembung sandhi panjenengan wis suksès diowahi! Saiki mrosès mlebu log panjenengan...',
'resetpass_bad_temporary' => 'Tembung sandhi sementara salah. Panjenengan mbokmenawa tau ngganti tembung sandhi panjenengan utawa tau nyuwun tembung sandhi anyar.',
'resetpass_forbidden'     => 'Tembung sandhi ora bisa diowahi ing wiki iki',
'resetpass_missing'       => 'Data formulir ora ditepangi.',

# Edit page toolbar
'bold_sample'     => 'Tèks iki bakal dicithak kandel',
'bold_tip'        => 'Cithak kandel',
'italic_sample'   => 'Tèks iki bakal dicithak miring',
'italic_tip'      => 'Cithak miring',
'link_sample'     => 'Judhul pranala',
'link_tip'        => 'Pranala njero',
'extlink_sample'  => 'http://www.example.com judhul pranala',
'extlink_tip'     => 'Pranala njaba (aja lali wiwitan http:// )',
'headline_sample' => 'Tèks judhul',
'headline_tip'    => 'Subbagian tingkat 1',
'math_sample'     => 'Lebokna rumus ing kéné',
'math_tip'        => 'Rumus matematika (LaTeX)',
'nowiki_sample'   => 'Tèks iki ora bakal diformat',
'nowiki_tip'      => 'Aja nganggo format wiki',
'image_sample'    => 'Conto.jpg',
'image_tip'       => 'Mènèhi gambar/berkas',
'media_sample'    => 'Conto.ogg',
'media_tip'       => 'Pranala berkas media',
'sig_tip'         => 'Tapak asta panjenengan mawa tandha wektu',
'hr_tip'          => 'Garis horisontal',

# Edit pages
'summary'                   => 'Ringkesan',
'subject'                   => 'Subyek/judhul',
'minoredit'                 => 'Iki suntingan cilik.',
'watchthis'                 => 'Awasana kaca iki',
'savearticle'               => 'Simpen kaca',
'preview'                   => 'Pratayang',
'showpreview'               => 'Mirsani pratayang',
'showlivepreview'           => 'Pratayang langsung',
'showdiff'                  => 'Tuduhna pangowahan',
'anoneditwarning'           => 'Panjenengan ora kadaftar mlebu. Alamat IP panjenengan bakal kacathet ing sajarah panyuntingan kaca iki.',
'missingsummary'            => "'''Pènget:''' Panjenengan ora nglebokaké ringkesan panyuntingan. Menawa panjenengan mencèt tombol Simpen manèh, suntingan panjenengan bakal kasimpen tanpa ringkesan panyuntingan.",
'missingcommenttext'        => 'Tulung lebokna komentar ing ngisor iki.',
'missingcommentheader'      => "'''Pènget:''' Panjenengan durung mènèhi subyèk utawa judhul kanggo komentar panjenengan. Menawa panjenengan mencèt Simpan, suntingan panjenengan bakal kasimpen tanpa komentar iku.",
'summary-preview'           => 'Pratayang ringkesan',
'subject-preview'           => 'Pratayang subyèk/judhul',
'blockedtitle'              => 'Panganggo diblokir',
'blockedtext'               => "<big>'''Asma panganggo utawa alamat IP panjenengan diblokir.'''</big>

Blokir iki sing nglakoni $1. Alesané ''$2''.

* Diblokir wiwit: $8
* Kadaluwarsa pemblokiran ing: $6
* Sing arep diblokir: $7

Panjenengan bisa ngubungi $1 utawa [[{{MediaWiki:Grouppage-sysop}}|pangurus liyané]] kanggo ngomongaké prakara iki.

Panjenengan ora bisa nggunakaké fitur 'Kirim layang e-mail panganggo iki' kejaba panjenengan wis nglebokaké alamat e-mail sing sah ing [[Special:Preferences|préferènsi]] panjenengan.

Alamat IP panjenengan iku $3, lan ID pamblokiran iku $5. Tulung salah siji saka rong informasi iki disertakaké ing saben pitakon panjenengan",
'autoblockedtext'           => 'Alamat IP panjenangan wis diblokir minangka otomatis amerga dienggo déning panganggo liyané. Pamblokiran dilakoni déning $1 mawa alesan:

:\'\'$2\'\'

* Diblokir wiwit: $8
* Blokir kadaluwarsa ing: $6

Panjenengan bisa ngubungi $1 utawa [[{{MediaWiki:Grouppage-sysop}}|pangurus liyané]] kanggo ngomongaké perkara iki.

Panjenengan ora bisa nganggo fitur "kirim e-mail panganggo iki" kejaba panjenengan wis nglebokaké alamat e-mail sing sah ing [[Special:Preferences|préferènsi]] panjenengan lan panjenengan wis diblokir kanggo nggunakaké.

ID pamblokiran panjenengan iku $5. Tulung sertakna ID iki saben ngajokaké pitakonan panjenengan. Matur nuwun.',
'blockednoreason'           => 'ora ana alesan sing diwènèhaké',
'blockedoriginalsource'     => "Isi sumber '''$1''' dituduhaké ing ngisor iki:",
'blockededitsource'         => "Tèks '''suntingan panjenengan''' tumrap ing '''$1''' dituduhaké ing ngisor iki:",
'whitelistedittitle'        => 'Prelu log mlebu kanggo nyunting',
'whitelistedittext'         => 'Panjenengan kudu $1 supaya bisa nyunting artikel.',
'confirmedittitle'          => 'Konfirmasi layang e-mail diprelokaké supaya panjenengan pareng nglakoni panyuntingan',
'confirmedittext'           => 'Panjenengan kudu ndhedhes alamat e-mail dhisik sadurungé pareng nyunting sawijining kaca. Mangga nglebokaké lan validasi alamat e-mail panjenengan sadurungé nglakoni panyuntingan. Alamat e-mail sawisé bisa diowahi liwat [[Special:Preferences|kaca préférènsi]]',
'nosuchsectiontitle'        => 'Subbagian iku ora bisa ditemokaké',
'nosuchsectiontext'         => 'Panjenengan nyoba nyunting sawijining sing ora ana. Amerga subbagian $1 ora ana, suntingan panjenengan ora bisa disimpen.',
'loginreqtitle'             => 'Mangga mlebu log',
'loginreqlink'              => 'mlebu log',
'loginreqpagetext'          => 'Panjenengan kudu $1 kanggo bisa mirsani kaca liyané.',
'accmailtitle'              => 'Tembung sandhi wis dikirim.',
'accmailtext'               => "Tembung sandhi kanggo '$1' wis dikirimaké menyang $2.",
'newarticle'                => '(Anyar)',
'newarticletext'            => "Katonané panjenengan ngetutaké pranala artikel sing durung ana.
Manawa kersa manulis artikel iki, manggaa. (Mangga mirsani [[{{MediaWiki:Helppage}}|Pitulung]] kanggo informasi sabanjuré).
Yèn ora sengaja tekan kéné, bisa ngeklik pencètan '''back''' waé ing panjlajah wèb panjenengan.",
'anontalkpagetext'          => "---- ''Iki yaiku kaca dhiskusi sawijining panganggo anonim sing durung kagungan akun utawa ora nganggo akuné, dadi kita keeksa kudu nganggo alamat IP-né kanggo nepangi. Alamat IP kaya mengkéné iki bisa dienggo déning panganggo sing séjé-séjé. Yèn panjenengan pancèn panganggo anonim lan olèh komentar-komentar miring, mangga [[Special:UserLogin|nggawé akun utawa log mlebu]] supaya ora rancu karo panganggo anonim liyané ing mangsa ngarep.''",
'noarticletext'             => 'Saiki ora ana tèks ing kaca iki. Panjenengan bisa [[Special:Search/{{PAGENAME}}|nglakoni panggolèkan kanggo judhul iki kaca iki]] ing kaca-kaca liyané utawa [{{fullurl:{{FULLPAGENAME}}|action=edit}} nyunting kaca iki].',
'userpage-userdoesnotexist' => 'Akun utawa rékening panganggo "$1" ora kadaftar.',
'clearyourcache'            => "'''Cathetan:''' Sawisé nyimpen préférènsi, panjenengan prelu ngresiki <em>cache</em> panjlajah wèb panjenengan kanggo mirsani pangowahan. '''Mozilla / Firefox / Safari:''' pencèt ''Ctrl-Shift-R'' (''Cmd-Shift-R'' pada Apple Mac); '''IE:''' tekan ''Ctrl-F5''; '''Konqueror:''': pencèt ''F5''; '''Opera''' resikana <em>cache</em> miturut menu ''Tools→Preferences''.",
'usercssjsyoucanpreview'    => "<strong>Tips:</strong> Gunakna tombol 'Deleng pratilik' kanggo ngetès CSS/JS anyar panjenengan sadurungé disimpen.",
'usercsspreview'            => "'''Pèngeten yèn panjenengan namun mirsani pratilik CSS panjenengan, lan menawa pratilik iku durung kasimpen!'''",
'userjspreview'             => "'''Pèngeten yèn sing panjenengan pirsani namung pratilik JavaScript panjenengan, lan menawa pratilik iku dèrèng kasimpen!'''",
'userinvalidcssjstitle'     => "'''Pènget:''' Kulit \"\$1\" ora ditemokaké. Muga dipèngeti yèn kaca .css lan .js nggunakaké huruf cilik, conto {{ns:user}}:Foo/monobook.css lan dudu {{ns:user}}:Foo/Monobook.css.",
'updated'                   => '(Dianyari)',
'note'                      => '<strong>Cathetan:</strong>',
'previewnote'               => '<strong>Muga digatèkaké menawa iki namung pratilik waé, durung disimpen!</strong>',
'previewconflict'           => 'Pratilik iki nuduhaké tèks ing bagian dhuwur kothak suntingan tèks kayadéné bakal katon yèn panjenengan bakal simpen.',
'session_fail_preview'      => "<strong>Nuwun sèwu, suntingan panjenengan ora bisa diolah amarga dhata sèsi kabusak.
Coba kirim dhata manèh. Yèn tetep ora bisa, coba log metua lan mlebu log manèh.</strong>'''Amerga wiki iki marengaké panggunan kodhe HTML mentah, mula pratilik didhelikaké minangka pancegahan marang serangan JavaScript.'''
<strong>Menawa iki sawijining usaha panyuntingan sing sah, mangga dicoba manèh.
Yèn isih tetep ora kasil, cobanen metu log lan mlebu manèh.</strong>",
'session_fail_preview_html' => "<strong>Nuwun sèwu! Kita ora bisa prosès suntingan panjenengan amerga data sési ilang.</strong>

''Amerga wiki iki ngidinaké panrapan HTML mentah, pratayang didelikaké minangka penggakan marang serangan Javascript.''

<strong>Yèn iki sawijining upaya suntingan sing absah, mangga dicoba manèh. Yèn isih tetep ora kasil, cobanen metu log utawa oncat lan mlebua manèh.</strong>",
'token_suffix_mismatch'     => '<strong>Suntingan panjenengan ditulak amerga aplikasi klièn panjenengan ngowahi karakter tandha wewacan ing suntingan. Suntingan iku ditulak kanggo untuk menggak kaluputan ing tèks artikel. Prekara iki kadhangkala dumadi yèn panjenengan ngangem dines layanan proxy anonim adhedhasar situs wèb sing duwé masalah.</strong>',
'editing'                   => 'Nyunting $1',
'editingsection'            => 'Nyunting $1 (bagian)',
'editingcomment'            => 'Nyunting $1 (komentar)',
'editconflict'              => 'Konflik panyuntingan: $1',
'explainconflict'           => 'Wong liya wis nyunting kaca iki wiwit panjenengan mau nyunting. Bagian dhuwur tèks iki ngamot tèks kaca vèrsi saiki. Pangowahan sing panjenengan lakoni dituduhaké ing bagian ngisor tèks. Panjenengan namung prelu nggabungaké pangowahan panjenengan karo tèks sing wis ana. <strong>Namung</strong> tèks ing bagian dhuwur kaca sing bakal kasimpen menawa panjenengan mencèt "Simpen kaca".<p>',
'yourtext'                  => 'Tèks panjenengan',
'storedversion'             => 'Versi sing kasimpen',
'nonunicodebrowser'         => '<strong>PÈNGET: Panjlajah wèb panjenengan ora ndhukung Unicode, mangga gantènana panjlajah wèb panjenengan sadurungé nyunting artikel.</strong>',
'editingold'                => "'''PÈNGET:''' Panjenengan nyunting revisi lawas sawijining kaca. Yèn versi iki panjenengan simpen, mengko pangowahan-pangowahan sing wis digawé wiwit revisi iki bakal ilang.",
'yourdiff'                  => 'Prabédan',
'copyrightwarning'          => 'Tulung dipun-gatèkaké menawa kabèh sumbangsih utawa kontribusi kanggo {{SITENAME}} iku dianggep wis diluncuraké miturut $2 GNU (mangga priksanen $1 kanggo ditèlé).
Menawa panjenengan ora kersa menawa tulisan panjenengan bakal disunting karo disebar, aja didokok ing kéné.<br />
Panjenengan uga janji menawa apa-apa sing katulis ing kéné, iku karyané panjenengan dhéwé, utawa disalin saka sumber bébas. <strong>AJA NDOKOK KARYA SING DIREKSA DÉNING UNDHANG-UNDHANG HAK CIPTA TANPA IDIN!</strong>',
'copyrightwarning2'         => 'Mangga digatèkaké yèn kabèh kontribusi marang  {{SITENAME}} bisa disunting, diowahi, utawa dibusak déning penyumbang liyané. Yèn panjenengan ora kersa yèn tulisan panjenengan bisa disunting wong liya, aja ngirim artikel panjenengan ing kéné.<br />Panjenengan uga janji yèn tulisan panjenengan iku kasil karya panjenengan dhéwé, utawa disalin saka sumber umum utawa sumber bébas liyané (mangga delengen $1 kanggo informasi sabanjuré). <strong>AJA NGIRIM KARYA SING DIREKSA DÉNING UNDHANG-UNDHANG HAK CIPTA TANPA IDIN!</strong>',
'longpagewarning'           => "'''PÈNGET: Kaca iki dawané $1 kilobita; sawetara panjlajah wèb mbokmenawa ngalami masalah kanggo nyunting kaca sing dawané 32 kb utawa luwih. Muga digalih dhisik mbokmenawa kaca iki bisa dipérang dadi pirang-pirang kaca sing luwih cilik.'''",
'longpageerror'             => '<strong>KALUPUTAN: Tèks sing panjenengan kirim gedhéné $1 kilobita, sing tegesé luwih gedhé tinimbang cacah maksimum $2 kilobita. Tèks iki ora bisa disimpen.</strong>',
'readonlywarning'           => '<strong>PÈNGET: Basis data lagi dikunci amerga ana pangopènan, dadi saiki panjenengan ora bisa nyimpen kasil panyuntingan panjenengan. Panjenengan mbokmenawa prelu mindhahaké kasil panyuntingan panjenengan iki menyang panggonan liya kanggo disimpen bésuk.</strong>',
'protectedpagewarning'      => '<strong>PÈNGET:  Kaca iki wis dikunci dadi namung panganggo sing nduwé hak aksès pangurus baé sing bisa nyunting.</strong>',
'semiprotectedpagewarning'  => "'''Cathetan:''' Kaca iki lagi direksa, dadi namung panganggo kadaftar sing bisa nyunting.",
'cascadeprotectedwarning'   => "'''PÈNGET:''' Kaca iki wis dikunci dadi namung panganggo mawa hak aksès pangurus waé sing bisa nyunting, amerga kalebu {{PLURAL:$1|kaca|kaca-kaca}} ing ngisor iki sing wis direksa mawa opsi 'pangreksan runtun' diaktifaké:",
'titleprotectedwarning'     => '<strong>PÈNGET: Kaca iki wis dikunci dadi namung sawetara panganggo waé sing bisa nggawé.</strong>',
'templatesused'             => 'Cithakan kang digunakaké ing kaca iki:',
'templatesusedpreview'      => 'Cithakan kang digunakaké ing pratilik iki:',
'templatesusedsection'      => 'Cithakan kang digunakaké ing bagian iki:',
'template-protected'        => '(direksa)',
'template-semiprotected'    => '(semi-pangreksan)',
'hiddencategories'          => 'Kaca iki sawijining anggota saka {{PLURAL:$1|1 kategori ndelik|$1 kategori-kategori ndelik}}:',
'edittools'                 => '<!-- Tèks ing ngisor iki bakal ditudhuhaké ing ngisoring isènan suntingan lan pangemotan.-->',
'nocreatetitle'             => 'Panggawéan kaca anyar diwatesi',
'nocreatetext'              => 'Situs iki ngwatesi kemampuan kanggo nggawé kaca anyar. Panjenengan bisa bali lan nyunting kaca sing wis ana, utawa mangga [[Special:UserLogin|mlebua log utawa ndaftar]]',
'nocreate-loggedin'         => 'Panjenengan ora kagungan idin kanggo nggawé kaca anyar ing wiki iki.',
'permissionserrors'         => 'Kaluputan Idin Aksès',
'permissionserrorstext'     => 'Panjengan ora kagungan idin kanggo nglakoni sing panjenengan gayuh amerga {{PLURAL:$1|alesan|alesan-alesan}} iki:',
'recreate-deleted-warn'     => "'''Pènget: Panjenengan nggawé manèh sawijining kaca sing wis tau dibusak.''',

Mangga digagas manèh apa suntingan panjenengan iki layak ora.
Ing ngisor iki kapacak log pambusakan saka kaca iki:",

# Parser/template warnings
'expensive-parserfunction-warning'  => "Pènget: Kaca iki ngandhut kakèhan panggunan fungsi ''parser'' sing larang.

Sajatiné kuduné duwé kurang saka $2, saiki ana $1.",
'expensive-parserfunction-category' => "Kaca-kaca mawa panggunan fungsi ''parser'' sing kakèhan",

# "Undo" feature
'undo-success' => 'Suntingan iki bisa dibatalaké. Tulung priksa prabandhingan ing ngisor iki kanggo mesthèkaké yèn prakara iki pancèn sing bener panjenengan pèngin lakoni, banjur simpenen pangowahan iku kanggo ngrampungaké pambatalan suntingan.',
'undo-failure' => 'Suntingan iki ora bisa dibatalakén amerga ana konflik panyuntingan antara.',
'undo-norev'   => 'Suntingan iki ora bisa dibatalaké amerga ora ana utawa wis dibusak.',
'undo-summary' => '←Mbatalaké revisi $1 déning [[Special:Contributions/$2|$2]] ([[User talk:$2|Dhiskusi]])',

# Account creation failure
'cantcreateaccounttitle' => 'Akun ora bisa digawé',
'cantcreateaccount-text' => "Saka alamat IP iki ('''$1''') ora diparengaké nggawé akun utawa rékening. Sing mblokir utawa ora marengaké iku [[User:$3|$3]].

Alesané miturut $3 yaiku ''$2''",

# History pages
'viewpagelogs'        => 'Mirsani log kaca iki',
'nohistory'           => 'Ora ana sajarah panyuntingan kanggo kaca iki',
'revnotfound'         => 'Revisi ora ditemokaké',
'revnotfoundtext'     => 'Revisi lawas kaca sing panjenengan suwun ora bisa ditemokaké. Mangga priksanen URL sing digunakaké kanggo ngaksès kaca iki.',
'currentrev'          => 'Revisi saiki',
'revisionasof'        => 'Revisi per $1',
'revision-info'       => 'Revisi per $1; $2',
'previousrevision'    => '←Revisi sadurungé',
'nextrevision'        => 'Revisi sabanjuré→',
'currentrevisionlink' => 'Revisi saiki',
'cur'                 => 'saiki',
'next'                => 'sabanjuré',
'last'                => 'akir',
'page_first'          => 'kapisan',
'page_last'           => 'pungkasan',
'histlegend'          => "Pilihen rong tombol radhio banjur pencèten tombol ''bandhingna'' kanggo mbandhingaké versi. Klik sawijining tanggal kanggo ndeleng versi kaca ing tanggal iku.<br />(skr) = prabédan karo vèrsi saiki, (akir) = prabédan karo vèrsi sadurungé, '''s''' = suntingan sithik, '''b''' = suntingan bot, → = suntingan bagian, ← = ringkesan otomatis",
'deletedrev'          => '[dibusak]',
'histfirst'           => 'Suwé dhéwé',
'histlast'            => 'Anyar dhéwé',
'historysize'         => '($1 {{PLURAL:$1|bita|bita}})',
'historyempty'        => '(kosong)',

# Revision feed
'history-feed-title'          => 'Riwayat revisi',
'history-feed-description'    => 'Riwayat revisi kaca iki ing wiki',
'history-feed-item-nocomment' => '$1 ing $2', # user at time
'history-feed-empty'          => 'Kaca sing disuwun ora ditemokaké. Mbokmenawa wis dibusak saka wiki, utawa diwènèhi jeneng anyar. Coba [[Special:Search|golèka ing wiki]] kanggo kaca anyar sing rélevan.',

# Revision deletion
'rev-deleted-comment'         => '(komentar dibusak)',
'rev-deleted-user'            => '(jeneng panganggo dibusak)',
'rev-deleted-event'           => '(isi dibusak)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">Riwayat revisi kaca iki wis dibusak saka arsip umum.
Detil mbokmenawa kasedyakaké ing  [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} log pambusakan].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Riwayat revisi kaca iki wis dibusak saka arsip umum.
Minangka sawijning pangurus situs, panjenengan bisa mirsani; detil mbokmenawa kasedyakaké ing [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} log pambusakan].</div>',
'rev-delundel'                => 'tuduhna/delikna',
'revisiondelete'              => 'Busak/batal busak revisi',
'revdelete-nooldid-title'     => 'Target revisi ora ditemokaké',
'revdelete-nooldid-text'      => 'Panjenengan durung mènèhi target revisi kanggo nglakoni fungsi iki.',
'revdelete-selected'          => "{{PLURAL:$2|Revisi kapilih|Revisi kapilih}} dari '''$1'''",
'logdelete-selected'          => '{{PLURAL:$1|Log kapilih|Log kapilih}} kanggo:',
'revdelete-text'              => 'Revisi lan tindhakan sing wis kabusak bakal tetep muncul ing kaca versi sadurungé, nanging tèks iki ora bisa diaksès minangka umum.

Pengurus liyané bakal tetep bisa ngaksès isi sing kadhelikaké iku lan bisa mbatalaké pambusakan ngliwati antarmuka sing padha, kejaba yèn ana pawatesan liyané sing digawé déning operator situs',
'revdelete-legend'            => 'Atur watesan:',
'revdelete-hide-text'         => 'Dhelikna tèks revisi',
'revdelete-hide-name'         => 'Dhelikna tindhakan lan targèt',
'revdelete-hide-comment'      => 'Tudhuhna/dhelikan ringkesan suntingan',
'revdelete-hide-user'         => 'Dhelikan jeneng panganggo/IP penyunting',
'revdelete-hide-restricted'   => 'Trapna pawatesan kanggo pangurus lan panganggo liyané',
'revdelete-suppress'          => 'Uga dhelikan saka pangurus',
'revdelete-hide-image'        => 'Dhelikna isi berkas',
'revdelete-unsuppress'        => 'Busak watesan ing revisi sing dibalèkaké',
'revdelete-log'               => 'Log ringkesan:',
'revdelete-submit'            => 'Trapna ing revisi kapilih',
'revdelete-logentry'          => 'owahna tampilan revisi kanggo [[$1]]',
'logdelete-logentry'          => 'owahna aturan pandhelikan saka [[$1]]',
'revdelete-success'           => 'Aturan pandhelikan revisi bisa kasil ditrapaké.',
'logdelete-success'           => 'Aturan pandhelikan tindhakan bisa kasil ditrapaké.',
'revdel-restore'              => 'Ngowahi visiblitas (pangatonan)',
'pagehist'                    => 'Sajarah kaca',
'deletedhist'                 => 'Sajarah sing dibusak',
'revdelete-content'           => 'isi',
'revdelete-summary'           => 'ringkesan suntingan',
'revdelete-uname'             => 'jeneng panganggo',
'revdelete-restricted'        => 'rèstriksi ditrapaké marang para opsis',
'revdelete-unrestricted'      => 'rèstriksi marang para opsis dijabel',
'revdelete-hid'               => 'delikaké $1',
'revdelete-unhid'             => 'buka pandelikan $1',
'revdelete-log-message'       => '$1 kanggo $2 {{PLURAL:$2|révisi|révisi}}',
'logdelete-log-message'       => '$1 kanggo $2 {{PLURAL:$2|prastawa|prastawa}}',

# Suppression log
'suppressionlog'     => "Log barang-barang sing didelikaké (''oversight'')",
'suppressionlogtext' => "Ing ngisor iki kapacak daftar pambusakan lan pamblokiran pungkasan sing uga nyangkut isi sing didelikaké saka para opsis. Mangga mirsani [[Special:IPBlockList|daftar pamblokiran IP]] kanggo daftar pambuwangan (''ban'') lan pamblokiran sing saiki lagi operasional.",

# History merging
'mergehistory'                     => 'Gabung sejarah kaca',
'mergehistory-header'              => 'Ing kaca iki panjenengan bisa nggabung révisi-révisi sajarah saka sawijining kaca sumber menyang kaca anyar.
Pastèkna yèn owah-owahan iki bakal netepaké kasinambungan sajarah kaca.',
'mergehistory-box'                 => 'Gabungna revisi-revisi saka rong kaca:',
'mergehistory-from'                => 'Kaca sumber:',
'mergehistory-into'                => 'Kaca tujuan:',
'mergehistory-list'                => 'Sejarah suntingan bisa digabung',
'mergehistory-merge'               => 'Révisi-révisi sing kapacak ing ngisor iki saka [[:$1]] bisa digabungaké menyang [[:$2]].
Gunakna tombol radio kanggo nggabungaké révisi-révisi sing digawé sadurungé wektu tartamtu. Gatèkna, menawa nganggo pranala navigasi bakal ngesèt ulang kolom iki.',
'mergehistory-go'                  => 'Tuduhna suntingan-suntingan sing bisa digabung',
'mergehistory-submit'              => 'Gabung revisi',
'mergehistory-empty'               => 'Ora ana revisi sing bisa digabung.',
'mergehistory-success'             => '$3 {{PLURAL:$1|révisi|révisi}} saka [[:$1]] bisa suksès digabung menyang [[:$2]].',
'mergehistory-fail'                => 'Ora bisa nggabung sajarah, coba dipriksa manèh kacané lan paramèter wektuné.',
'mergehistory-no-source'           => 'Kaca sumber $1 ora ana.',
'mergehistory-no-destination'      => 'Kaca tujuan $1 ora ana.',
'mergehistory-invalid-source'      => 'Irah-irahan kaca sumber kudu irah-irahan utawa judhul sing bener.',
'mergehistory-invalid-destination' => 'Irah-irahan kaca tujuan kudu irah-irahan utawa judhul sing bener.',
'mergehistory-autocomment'         => 'Nggabung [[:$1]] menyang [[:$2]]',
'mergehistory-comment'             => 'Nggabung [[:$1]] menyang [[:$2]]: $3',

# Merge log
'mergelog'           => 'Gabung log',
'pagemerge-logentry' => 'nggabungaké [[$1]] menyang [[$2]] (révisi nganti tekan $3)',
'revertmerge'        => 'Batalna panggabungan',
'mergelogpagetext'   => 'Ing ngisor iki kapacak daftar panggabungan sajarah kaca ing kaca liyané.',

# Diffs
'history-title'           => 'Sajarah revisi saka "$1"',
'difference'              => '(Prabédan antarrevisi)',
'lineno'                  => 'Baris $1:',
'compareselectedversions' => 'Bandhingna vèrsi kapilih',
'editundo'                => 'batalna',
'diff-multi'              => '({{PLURAL:$1|Sawiji|$1}} revisi antara sing ora dituduhaké.)',

# Search results
'searchresults'             => 'Pituwas panggolèkan',
'searchresulttext'          => 'Kanggo informasi sabanjuré ngenani nggolèki apa-apa ing {{SITENAME}}, mangga mirsani [[{{MediaWiki:Helppage}}|kaca pitulung]].',
'searchsubtitle'            => "Panjenengan nggolèki '''[[:$1]]'''",
'searchsubtitleinvalid'     => "Panjenengan nggolèki '''$1'''",
'noexactmatch'              => "'''Ora ana kaca mawa irah-irahan utawa judhul \"\$1\".''' Panjenengan bisa [[:\$1|nggawé kaca iki]].",
'noexactmatch-nocreate'     => "'''Ora ana kaca mawa irah-irahan utawa judhul \"\$1\".'''",
'toomanymatches'            => "Olèhé panjenengan golèk ngasilaké kakèhan pituwas, mangga nglebokaké ''query'' liyané",
'titlematches'              => 'Irah-irahan artikel sing cocog',
'notitlematches'            => 'Ora ana irah-irahan artikel sing cocog',
'textmatches'               => 'Tèks artikel sing cocog',
'notextmatches'             => 'Ora ana tèks kaca sing cocog',
'prevn'                     => '$1 sadurungé',
'nextn'                     => '$1 sabanjuré',
'viewprevnext'              => 'Deleng ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 tembung|$2 tembung}})',
'search-result-score'       => 'Relevansi: $1%',
'search-redirect'           => '(pangalihan $1)',
'search-section'            => '(sèksi $1)',
'search-suggest'            => 'Apa panjenengan kersané: $1',
'search-interwiki-caption'  => 'Proyèk-proyèk kagandhèng',
'search-interwiki-default'  => 'Pituwas $1:',
'search-interwiki-more'     => '(luwih akèh)',
'search-mwsuggest-enabled'  => 'mawa sugèsti',
'search-mwsuggest-disabled' => 'ora ana sugèsti',
'search-relatedarticle'     => 'Kagandhèng',
'mwsuggest-disable'         => 'Patènana sugèsti AJAX',
'searchrelated'             => 'kagandhèng',
'searchall'                 => 'kabèh',
'showingresults'            => "Ing ngisor iki dituduhaké {{PLURAL:$1|'''1''' kasil|'''$1''' kasil}}, wiwitané saking #<strong>$2</strong>.",
'showingresultsnum'         => "Ing ngisor iki dituduhaké {{PLURAL:$3|'''1''' kasil|'''$3''' kasil}}, wiwitané saka #<strong>$2</strong>.",
'showingresultstotal'       => "Ing ngisor iki kapacak pituwas '''$1 - $2''' of '''$3'''",
'nonefound'                 => "'''Cathetan''': Namung sawetara bilik nama sing digolèki sacara baku. Coba seselana mawa awalan ''all:'' kanggo golèk kabèh isi (kalebu kaca dhiskusi, cithakan lsp.), utawa nganggo bilik nama sing dipèngèni minangka préfiks.",
'powersearch'               => 'Golèk',
'powersearch-legend'        => "Panggolèkan sabanjuré (''advance search'')",
'search-external'           => 'Panggolèkan èkstèrnal',
'searchdisabled'            => 'Sawetara wektu iki panjenengan ora bisa nggolèk mawa fungsi golèk {{SITENAME}}. Kanggo saiki mangga panjenengan bisa golèk nganggo Google. Nanging isi indèks Google kanggo {{SITENAME}} bisa waé lawas lan durung dianyari.',

# Preferences page
'preferences'              => 'Konfigurasi',
'mypreferences'            => 'Préferènsiku',
'prefs-edits'              => 'Gunggungé suntingan:',
'prefsnologin'             => 'Durung mlebu log',
'prefsnologintext'         => 'Panjenengan kudu [[Special:UserLogin|mlebu log]] kanggo nyimpen préferènsi njenengan.',
'prefsreset'               => 'Préferènsi wis dibalèkaké menyang konfigurasi baku.',
'qbsettings'               => 'Pengaturan bar sidhatan',
'qbsettings-none'          => 'Ora ana',
'qbsettings-fixedleft'     => 'Tetep sisih kiwa',
'qbsettings-fixedright'    => 'Tetep sisih tengen',
'qbsettings-floatingleft'  => 'Ngambang sisih kiwa',
'qbsettings-floatingright' => 'Ngambang sisih tengen',
'changepassword'           => 'Ganti tembung sandhi',
'skin'                     => 'Kulit',
'math'                     => 'Matématika',
'dateformat'               => 'Format tanggal',
'datedefault'              => 'Ora ana préferènsi',
'datetime'                 => 'Wektu',
'math_failure'             => 'Gagal nglakoni parse',
'math_unknown_error'       => 'Kaluputan sing ora dimangertèni',
'math_unknown_function'    => 'fungsi sing ora dimangertèni',
'math_lexing_error'        => "kaluputan ''lexing''",
'math_syntax_error'        => "''syntax error'' (kaluputan sintaksis)",
'math_image_error'         => 'Konversi PNG gagal; priksa apa latex, dvips, gs, lan convert wis diinstalasi sing bener',
'math_bad_tmpdir'          => 'Ora bisa nulis utawa nggawé dirèktori sauntara math',
'math_bad_output'          => 'Ora bisa nulis utawa nggawé dirèktori paweton math',
'math_notexvc'             => 'Executable texvc ilang;
mangga delengen math/README kanggo cara konfigurasi.',
'prefs-personal'           => 'Profil',
'prefs-rc'                 => 'Owah-owahan pungkasan',
'prefs-watchlist'          => 'Pandelengan',
'prefs-watchlist-days'     => 'Cacahé dina sing dituduhaké ing daftar pangawasan:',
'prefs-watchlist-edits'    => 'Cacahé suntingan maksimum sing dituduhaké ing daftar pangawasan sing luwih jangkep:',
'prefs-misc'               => 'Liya-liya',
'saveprefs'                => 'Simpen',
'resetprefs'               => 'Resikana owah-owahan sing ora disimpen',
'oldpassword'              => 'Tembung sandi lawas:',
'newpassword'              => 'Tembung sandi anyar:',
'retypenew'                => 'Ketik ulang tembung sandi anyar:',
'textboxsize'              => 'Panyuntingan',
'rows'                     => 'Baris:',
'columns'                  => 'Kolom:',
'searchresultshead'        => 'Panggolèkan',
'resultsperpage'           => 'Pituwas (kasil) per kaca:',
'contextlines'             => 'Baris dituduhaké per pituwas (kasil):',
'contextchars'             => 'Karakter kanggo kontèks per baris:',
'stub-threshold'           => 'Ambang wates kanggo format <a href="#" class="stub">pranala rintisan</a>:',
'recentchangesdays'        => 'Cacahé dina sing dituduhaké ing owah-owahan pungkasan:',
'recentchangescount'       => 'Cacahé suntingan sing dituduhaké ing kaca owah-owahan pungkasan:',
'savedprefs'               => 'Préferènsi Panjenengan wis disimpen',
'timezonelegend'           => 'Zona wektu',
'timezonetext'             => '¹Lebokna prabédan wektu (ing jam) antara wektu saenggonan karo wektu server (UTC).',
'localtime'                => 'Wektu saenggon',
'timezoneoffset'           => 'Prabédan:',
'servertime'               => 'Wektu server saiki iku',
'guesstimezone'            => 'Isinen saka panjlajah wèb',
'allowemail'               => 'Marengaké panganggo liyané ngirim layang èlèktronik (email).',
'prefs-searchoptions'      => 'Opsi-opsi panggolèkan',
'defaultns'                => "Golèk ing bilik jeneng (''namespace'') iki mawa baku:",
'default'                  => 'baku',
'files'                    => 'Berkas',

# User rights
'userrights'                  => 'Manajemen hak panganggo', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Ngatur kelompok panganggo',
'userrights-user-editname'    => 'Lebokna jeneng panganggo:',
'editusergroup'               => 'Sunting kelompok panganggo',
'editinguser'                 => "Ngowahi hak-hak aksès panganggo saka panganggo '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Sunting kelompok panganggo',
'saveusergroups'              => 'Simpen kelompok panganggo',
'userrights-groupsmember'     => 'Anggota saka:',
'userrights-groups-help'      => 'Panjenengan bisa ngowahi grup-grup sing ana panganggoné iki.
* Kothak sing dicenthang tegesé panganggo iki ana sajroné grup iku.
* Kothak sing ora dicenthang tegesé panganggo iku ora ana ing grup iku.
* Tandha bintang * tegesé panjenengan ora bisa ngilangi grup iku yèn wis tau nambah, utawa sawalikané.',
'userrights-reason'           => 'Alesané ngowahi:',
'userrights-no-interwiki'     => 'Panjenengan ora ana hak kanggo ngowahi hak panganggo ing wiki liyané.',
'userrights-nodatabase'       => 'Basis data $1 ora ana utawa ora lokal.',
'userrights-nologin'          => 'Panjenengan kudu [[Special:UserLogin|mlebu log]] mawa nganggo akun utawa rékening pangurus supaya bisa ngowahi hak panganggo.',
'userrights-notallowed'       => 'Panjenengan ora ndarbèni hak kanggo ngowahi hak panganggo.',
'userrights-changeable-col'   => 'Grup sing bisa panjenengan owahi',
'userrights-unchangeable-col' => 'Grup sing ora bisa diowahi panjenengan',

# Groups
'group'               => 'Kelompok:',
'group-user'          => 'Para panganggo',
'group-autoconfirmed' => 'Panganggo sing otomatis didhedhes (dikonfirmasi)',
'group-bot'           => 'Bot',
'group-sysop'         => 'Pangurus',
'group-bureaucrat'    => 'Birokrat',
'group-suppress'      => "Para pangawas (''oversight'')",
'group-all'           => '(kabèh)',

'group-user-member'          => 'Panganggo',
'group-autoconfirmed-member' => 'Panganggo sing otomatis didhedhes (dikonfirmasi)',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Pangurus',
'group-bureaucrat-member'    => 'Birokrat',
'group-suppress-member'      => "Pangawas (''oversight'')",

'grouppage-user'          => '{{ns:project}}:Para panganggo',
'grouppage-autoconfirmed' => '{{ns:project}}:Panganggo sing otomatis didhedhes (dikonfirmasi)',
'grouppage-bot'           => '{{ns:project}}:Bot',
'grouppage-sysop'         => '{{ns:project}}:Pangurus',
'grouppage-bureaucrat'    => '{{ns:project}}:Birokrat',
'grouppage-suppress'      => '{{ns:project}}:Oversight',

# Rights
'right-read'                 => 'Maca kaca-kaca',
'right-edit'                 => 'Nyunting kaca-kaca',
'right-createpage'           => 'Nggawé kaca (sing dudu kaca dhiskusi)',
'right-createtalk'           => 'Nggawé kaca dhiskusi',
'right-createaccount'        => 'Nggawé rékening (akun) panganggo anyar',
'right-minoredit'            => 'Tandhanan suntingan minangka minor',
'right-move'                 => 'Pindhahna kaca',
'right-suppressredirect'     => 'Aja nggawé pangalihan saka kaca sing lawas yèn mindhah sawijining kaca',
'right-upload'               => 'Ngunggahaké berkas-berkas',
'right-reupload'             => 'Timpanana sawijining berkas sing wis ana',
'right-reupload-own'         => 'Nimpa sawijining berkas sing wis ana lan diunggahaké déning panganggo sing padha',
'right-reupload-shared'      => 'Timpanana berkas-berkas ing khazanah binagi sacara lokal',
'right-upload_by_url'        => 'Ngunggahaké berkas saka sawijining alamat URL',
'right-purge'                => "Kosongna ''cache'' situs iki kanggo sawijining kaca tanpa kaca konfirmasi",
'right-autoconfirmed'        => 'Sunting kaca-kaca sing disémi-reksa',
'right-bot'                  => 'Anggepen minangka prosès otomatis',
'right-nominornewtalk'       => "Suntingan sithik (''minor'') ora ngwetokaké prompt pesen anyar",
'right-apihighlimits'        => 'Nganggo wates sing luwih dhuwur ing kwéri API',
'right-delete'               => 'Busak kaca-kaca',
'right-bigdelete'            => 'Busak kaca-kaca mawa sajarah panyuntingan sing gedhé',
'right-deleterevision'       => 'Busak lan batal busak révisi tartamtu kaca-kaca',
'right-deletedhistory'       => 'Ndeleng sajarah èntri-èntri kabusak, tanpa bisa ndeleng apa sing dibusak',
'right-browsearchive'        => 'Golèk kaca-kaca sing wis dibusak',
'right-undelete'             => 'Batal busak sawijining kaca',
'right-suppressrevision'     => 'Ndeleng lan mbalèkaké révisi-révisi sing didelikaké saka para opsis',
'right-suppressionlog'       => 'Ndeleng log-log pribadi',
'right-block'                => 'Blokir panganggo-panganggo liya saka panyuntingan',
'right-blockemail'           => 'Blokir sawijining panganggo saka ngirim e-mail',
'right-hideuser'             => 'Blokir jeneng panganggo, lan delikna saka umum',
'right-ipblock-exempt'       => 'Bypass pamblokiran IP, pamblokiran otomatis lan pamblokiran rangkéan',
'right-proxyunbannable'      => 'Bypass pamblokiran otomatis proxy-proxy',
'right-protect'              => 'Ganti tingkatan pangreksan lan sunting kaca-kaca sing direksa',
'right-editprotected'        => 'Sunting kaca-kaca sing direksa (tanpa pangreksan runtun)',
'right-editinterface'        => 'Sunting interface (antarmuka) panganggo',
'right-editusercssjs'        => 'Sunting berkas-berkas CSS lan JS panganggo liya',
'right-rollback'             => 'Sacara gelis mbalèkaké panganggo pungkasan sing nyunting kaca tartamtu',
'right-markbotedits'         => 'Tandhanana suntingan pambalèkan minangka suntingan bot',
'right-import'               => 'Impor kaca-kaca saka wiki liya',
'right-importupload'         => 'Impor kaca-kaca saka sawijining pangunggahan berkas',
'right-patrol'               => 'Tandhanana suntingan minangka wis dipatroli',
'right-autopatrol'           => 'Gawé supaya suntingan-suntingan ditandhani minangka wis dipatroli',
'right-patrolmarks'          => 'Ndeleng tandha-tandha patroli owah-owahan anyar',
'right-unwatchedpages'       => 'Tuduhna daftar kaca-kaca sing ora diawasi',
'right-trackback'            => 'Kirimna trackback',
'right-mergehistory'         => 'Gabungna sajarah kaca-kaca',
'right-userrights'           => 'Sunting kabèh hak-hak panganggo',
'right-userrights-interwiki' => 'Sunting hak-hak para panganggo ing situs-situs wiki liya',
'right-siteadmin'            => 'Kunci lan buka kunci basis data',

# User rights log
'rightslog'      => 'Log pangowahan hak aksès',
'rightslogtext'  => 'Ing ngisor iki kapacak log pangowahan marang hak-hak panganggo.',
'rightslogentry' => 'ngganti kaanggotan kelompok kanggo $1 saka $2 dadi $3',
'rightsnone'     => '(ora ana)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|pangowahan|owah-owahan}}',
'recentchanges'                     => 'Owah-owahan',
'recentchangestext'                 => 'Runutna owah-owahan pungkasan ing wiki iki ing kaca iki.',
'recentchanges-feed-description'    => "Urutna owah-owahan anyar ing wiki ing ''feed'' iki.",
'rcnote'                            => 'Ing ngisor iki kapacak {{PLURAL:$1|pangowahan|owah-owahan}} pungkasan ing  <strong>$2</strong> dina pungkasan nganti $3.',
'rcnotefrom'                        => 'Ing ngisor iki owah-owahan wiwit <strong>$2</strong> (kapacak nganti <strong>$1</strong> owah-owahan).',
'rclistfrom'                        => 'Saiki nuduhaké owah-owahan wiwit tanggal $1',
'rcshowhideminor'                   => '$1 suntingan sithik',
'rcshowhidebots'                    => '$1 bot',
'rcshowhideliu'                     => '$1 panganggo mlebu log',
'rcshowhideanons'                   => '$1 panganggo anonim',
'rcshowhidepatr'                    => '$1 suntingan sing dipatroli',
'rcshowhidemine'                    => '$1 suntinganku',
'rclinks'                           => 'Tuduhna owah-owahan pungkasan $1 ing $2 dina pungkasan iki.<br />$3',
'diff'                              => 'béda',
'hist'                              => 'sajarah',
'hide'                              => 'Delikna',
'show'                              => 'Tuduhna',
'minoreditletter'                   => 's',
'newpageletter'                     => 'A',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|cacahé sing ngawasi|cacahé sing ngawasi}}]',
'rc_categories'                     => 'Watesana nganti kategori (dipisah karo "|")',
'rc_categories_any'                 => 'Apa waé',
'newsectionsummary'                 => '/* $1 */ bagéyan anyar',

# Recent changes linked
'recentchangeslinked'          => 'Pranala Pilihan',
'recentchangeslinked-title'    => 'Owah-owahan sing ana gandhèngané karo "$1"',
'recentchangeslinked-noresult' => 'Ora ana owah-owahan ing kaca-kaca kagandhèng iki salawasé periode sing wis ditemtokaké.',
'recentchangeslinked-summary'  => "Kaca astaméwa (kaca kusus) iki mènèhi daftar owah-owahan pungkasan ing kaca-kaca sing kagandhèng (utawa anggota sawijining kateogri). Kaca sing [[Special:Watchlist|panjenengan awasi]] ditandhani '''kandel'''.",
'recentchangeslinked-page'     => 'Jeneng kaca:',
'recentchangeslinked-to'       => 'Nuduhaké owah-owahan menyang kaca sing disambung menyang kaca-kaca iki',

# Upload
'upload'                      => 'Unggah',
'uploadbtn'                   => 'Unggahna berkas',
'reupload'                    => 'Unggah ulang',
'reuploaddesc'                => 'Bali ing formulir pamotan',
'uploadnologin'               => 'Durung mlebu log',
'uploadnologintext'           => 'Panjenengan kudu [[Special:UserLogin|mlebu log]] supaya olèh ngunggahaké gambar utawa berkas liyané.',
'upload_directory_read_only'  => 'Dirèktori pangunggahan ($1) ora bisa ditulis déning server wèb.',
'uploaderror'                 => 'Kaluputan pangunggahan berkas',
'uploadtext'                  => "Enggonen formulir ing ngisor iki kanggo ngunggahaké berkas. Gunakna [[Special:ImageList|daftar berkas]] utawa [[Special:Log/upload|log pangunggahan]] kanggo nuduhaké utawa nggolèk berkas utawa gambar sing wis diunggahaké sadurungé.

Kanggo nuduhaké utawa nyertakaké berkas utawa gambar ing sawijining kaca, gunakna pranala mawa format
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Berkas.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Berkas.png|tèks alternatif]]</nowiki>''' utawa
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Berkas.ogg]]</nowiki>''' kanggo langsung tumuju berkas sing dikarepaké.",
'upload-permitted'            => 'Jenis berkas sing diidinaké: $1.',
'upload-preferred'            => 'Jenis berkas sing disaranaké: $1.',
'upload-prohibited'           => 'Jenis berkas sing dilarang: $1.',
'uploadlog'                   => 'log pangunggahan',
'uploadlogpage'               => 'Log pangunggahan',
'uploadlogpagetext'           => 'Ing ngisor iki kapacak log pangunggahan berkas sing anyar dhéwé.',
'filename'                    => 'Jeneng berkas',
'filedesc'                    => 'Ringkesan',
'fileuploadsummary'           => 'Ringkesan:',
'filestatus'                  => 'Status hak cipta',
'filesource'                  => 'Sumber',
'uploadedfiles'               => 'Berkas sing wis diamot',
'ignorewarning'               => 'Lirwakna pèngetan lan langsung simpen berkas.',
'ignorewarnings'              => 'Lirwakna pèngetan apa waé',
'minlength1'                  => 'Jeneng berkas paling ora minimal kudu awujud saaksara.',
'illegalfilename'             => 'Jeneng berkas "$1" ngandhut aksara sing ora diparengaké ana sajroning irah-irahan kaca. Mangga owahana jeneng berkas iku lan cobanen  diunggahaké manèh.',
'badfilename'                 => 'Berkas wis diowahi dados "$1".',
'filetype-badmime'            => 'Berkas mawa tipe MIME "$1" ora pareng diunggahaké.',
'filetype-unwanted-type'      => "'''\".\$1\"''' kalebu jenis berkas sing ora diidinaké. Jenis berkas sing disaranaké iku \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' kalebu jenis berkas sing ora diidinaké. Jenis berkas sing diidinaké yaiku \$2.",
'filetype-missing'            => 'Berkas ini ora duwé ekstènsi (contoné ".jpg").',
'large-file'                  => 'Ukuran berkas disaranaké supaya ora ngluwihi $1 bita; berkas iki ukurané $2 bita.',
'largefileserver'             => 'Berkas iki luwih gedhé tinimbang sing bisa kaparengaké server.',
'emptyfile'                   => 'Berkas sing panjenengan unggahaké katoné kosong. Mbokmenawa iki amerga anané salah ketik ing jeneng berkas. Mangga dipastèkaké apa panjenengan pancèn kersa ngunggahaké berkas iki.',
'fileexists'                  => 'Sawijining berkas mawa jeneng iku wis ana, mangga dipriksa <strong><tt>$1</tt></strong> yèn panjenengan ora yakin sumedya ngowahiné.',
'filepageexists'              => 'Kaca dèskripsi kanggo berkas iki wis digawé ing <strong><tt>$1</tt></strong>, nanging saiki iki ora ditemokaké berkas mawa jeneng iku. Ringkesan sing panjenengan lebokaké ora bakal metu ing kaca dèskripsi. Kanggo ngetokaké dèskripsi iki, panjenengan kudu nyunting sacara manual',
'fileexists-extension'        => 'Berkas mawa jeneng sing padha wis ana:<br />
Jeneng berkas sing bakal diunggahaké: <strong><tt>$1</tt></strong><br />
Jeneng berkas sing wis ana: <strong><tt>$2</tt></strong><br />
Mangga milih jeneng liya.',
'fileexists-thumb'            => "<center>'''Berkas sing wis ana'''</center>",
'fileexists-thumbnail-yes'    => 'Berkas iki katoné gambar mawa ukuran sing luwih cilik <em>(thumbnail)</em>. 
Tulung dipriksa berkas <strong><tt>$1</tt></strong>.<br />
Yèn berkas sing wis dipriksa iku padha, ora perlu panjenengan ngunggahaké vèrsi cilik liyané manèh.',
'file-thumbnail-no'           => 'Jeneng berkas diwiwiti mawa <strong><tt>$1</tt></strong>. Katoné berkas iki sawijining gambar mawa ukuran sing luwih cilik <em>(thumbnail)</em>.
Yèn panjenengan kagungan vèrsi mawa résolusi kebak saka gambar iki, mangga vèrsi iku diunggahaké. Yèn ora, tulung jeneng berkas iki diganti.',
'fileexists-forbidden'        => 'Berkas mawa jeneng sing padha wis ana; 
tulung berkasé diunggahaké manèh mawa jeneng liya. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Wis ana berkas liyané mawa jeneng sing padha ing papan gudhang berkas bebarengan;
mangga berkas diunggahaké ulang mawa jeneng liya. [[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'Kasil diamot',
'uploadwarning'               => 'Pèngetan pangunggahan berkas',
'savefile'                    => 'Simpen berkas',
'uploadedimage'               => 'gambar "[[$1]]" kaunggahaké',
'overwroteimage'              => 'ngunggahaké vèrsi anyar saka "[[$1]]"',
'uploaddisabled'              => 'Nuwun sèwu, fasilitas pangunggahan dipatèni.',
'uploaddisabledtext'          => 'Pangunggahan berkas ora diidinaké ing {{SITENAME}}.',
'uploadscripted'              => 'Berkas iki ngandhut HTML utawa kode sing bisa diinterpretasi salah déning panjlajah wèb.',
'uploadcorrupt'               => 'Berkasé rusak utawa èkstènsiné salah. Mangga dipriksa dhisik berkas iki lan diunggahaké manèh.',
'uploadvirus'                 => 'Berkas iki ngamot virus! Détil: $1',
'sourcefilename'              => 'Jeneng berkas sumber',
'destfilename'                => 'Jeneng berkas sing dituju',
'upload-maxfilesize'          => 'Ukuran maksimal berkas: $1',
'watchthisupload'             => 'Awasana kaca iki',
'filewasdeleted'              => 'Sawijining berkas mawa jeneng iki wis tau diunggahaké lan sawisé dibusak. 
Mangga priksanen $1 sadurungé ngunggahaké berkas iku manèh.',
'upload-wasdeleted'           => "'''PÈNGET: Panjenengan ngunggahaké sawijining berkas sing wis tau dibusak.'''

Panjenengan kudu nggalih perlu utawa ora mbanjuraké pangunggahan berkas ini.
Log pambusakan berkas iki kaya mangkéné:",
'filename-bad-prefix'         => 'Jeneng berkas sing panjenengan unggahaké, diawali mawa <strong>"$1"</strong>, sing sawijining jeneng non-dèskriptif sing biasané diwènèhaké sacara otomatis déning kamera digital. Mangga milih jeneng liyané sing luwih dèskriptif kanggo berkas panjenengan.',

'upload-proto-error'      => 'Protokol ora bener',
'upload-proto-error-text' => 'Pangunggahan jarah adoh mbutuhaké URL sing diawali karo <code>http://</code> utawa <code>ftp://</code>.',
'upload-file-error'       => 'Kaluputan internal',
'upload-file-error-text'  => "Ana sawijining kaluputan internal nalika nyoba ngunggahaké berkas sauntara (''temporary file'') ing server. Mangga kontak pangurus sistém iki.",
'upload-misc-error'       => 'Kaluputan pamunggahan sing ora dimangertèni',
'upload-misc-error-text'  => 'Ana sawijining kaluputan sing ora ditepungi dumadi nalika pangunggahan. Mangga dipastèkaké yèn URL kasebut iku absah lan bisa diaksès. Sawisé iku cobanen manèh. Yèn masalah iki isih ana, mangga kontak pangurus sistém.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL-é ora bisa dihubungi',
'upload-curl-error6-text'  => 'URL sing diwènèhaké ora bisa dihubungi. 
Mangga dipriksa manèh yèn URL iku pancèn bener lan situs iki lagi aktif.',
'upload-curl-error28'      => 'Pangunggahan ngliwati wektu',
'upload-curl-error28-text' => 'Situsé kesuwèn sadurungé réaksi.
Mangga dipriksa menawa situsé aktif, nunggu sedélok lan coba manèh.
Mbok-menawa panjenengan bisa nyoba manèh ing wektu sing luwih longgar.',

'license'            => 'Jenis lisènsi:',
'nolicense'          => 'Durung ana sing dipilih',
'license-nopreview'  => '(Pratayang ora sumedya)',
'upload_source_url'  => ' (sawijining URL absah sing bisa diaksès publik)',
'upload_source_file' => ' (sawijining berkas ing komputeré panjenengan)',

# Special:ImageList
'imagelist-summary'     => 'Kaca astaméwa utawa kusus iki nuduhaké kabèh berkas sing wis diunggahaké.
Sacara baku, berkas pungkasan sing diunggahaké dituduhaké ing urutan dhuwur dhéwé.
Klik sirahé kolom kanggo ngowahi urutan.',
'imagelist_search_for'  => 'Golèk jeneng gambar:',
'imgfile'               => 'gambar',
'imagelist'             => 'Daftar gambar',
'imagelist_date'        => 'Tanggal',
'imagelist_name'        => 'Jeneng',
'imagelist_user'        => 'Panganggo',
'imagelist_size'        => 'Ukuran (bita)',
'imagelist_description' => 'Dèskripsi',

# Image description page
'filehist'                       => 'Sajarah berkas',
'filehist-help'                  => 'Klik ing tanggal/wektu kanggo deleng berkas iki ing wektu iku.',
'filehist-deleteall'             => 'busaken kabèh',
'filehist-deleteone'             => 'busaken iki',
'filehist-revert'                => 'balèkna',
'filehist-current'               => 'saiki iki',
'filehist-datetime'              => 'Tanggal/Wektu',
'filehist-user'                  => 'Panganggo',
'filehist-dimensions'            => 'Ukuran',
'filehist-filesize'              => 'Gedhené berkas',
'filehist-comment'               => 'Komentar',
'imagelinks'                     => 'Pranala',
'linkstoimage'                   => 'Kaca-kaca sing kapacak iki duwé pranala menyang berkas iki:',
'nolinkstoimage'                 => 'Ora ana kaca sing nyambung menyang berkas iki.',
'morelinkstoimage'               => 'Ndeleng [[Special:WhatLinksHere/$1|luwih akèh pranala]] menyang berkas iki.',
'redirectstofile'                => 'Berkas-berkas iki duwé pangalihan menyang berkas iki:',
'sharedupload'                   => 'Berkas iki sawijining pangunggahan bebarengan sing uga bisa dienggo déning proyèk-proyèk liyané.',
'shareduploadwiki'               => 'Mangga mirsani $1 kanggo informasi sabanjuré.',
'shareduploadwiki-desc'          => 'Dèskripsi ing $1 sajroning khazanah binagi dituduhaké ing ngisor iki.',
'shareduploadwiki-linktext'      => 'kaca dèskripsi berkas',
'shareduploadduplicate'          => 'Berkas iki sawijining duplikat $1 saka khazanah binagi.',
'shareduploadduplicate-linktext' => 'Berkas liya',
'shareduploadconflict'           => 'Berkas iki duwé jeneng padha karo $1 saka khazanah binagi.',
'shareduploadconflict-linktext'  => 'Berkas liya',
'noimage'                        => 'Ora ana berkas mawa jeneng iku, panjenengan bisa $1.',
'noimage-linktext'               => 'ngunggah gambar',
'uploadnewversion-linktext'      => 'Unggahna vèrsi sing luwih anyar tinimbang gambar iki',
'imagepage-searchdupe'           => 'Golèk berkas duplikat',

# File reversion
'filerevert'                => 'Balèkna $1',
'filerevert-legend'         => 'Balèkna berkas',
'filerevert-intro'          => "Panjenengan mbalèkaké '''[[Media:$1|$1]]''' menyang [vèrsi $4 ing $3, $2].",
'filerevert-comment'        => 'Komentar:',
'filerevert-defaultcomment' => 'Dibalèkaké menyang vèrsi ing $2, $1',
'filerevert-submit'         => 'Balèkna',
'filerevert-success'        => "'''[[Media:$1|$1]]''' wis dibalèkaké menyang [vèrsi $4 ing $3, $2].",
'filerevert-badversion'     => 'Ora ana vèrsi lokal sadurungé saka berkas iki mawa stèmpel wektu sing dikarepaké.',

# File deletion
'filedelete'                  => 'Mbusak $1',
'filedelete-legend'           => 'Mbusak berkas',
'filedelete-intro'            => "Panjenengan mbusak '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Panjenengan mbusak vèrsi '''[[Media:$1|$1]]''' per [$4 $3, $2].",
'filedelete-comment'          => 'Alesan mbusak:',
'filedelete-submit'           => 'Busak',
'filedelete-success'          => "'''$1''' wis dibusak.",
'filedelete-success-old'      => '<span class="plainlinks">Vèrsi \'\'\'[[Media:$1|$1]]\'\'\' ing $3, $2 wis dibusak.</span>',
'filedelete-nofile'           => "'''$1''' ora ana ing {{SITENAME}}.",
'filedelete-nofile-old'       => "Ora ditemokaké arsip vèrsi saka '''$1''' mawa atribut sing diwènèhaké.",
'filedelete-iscurrent'        => 'Panjenengan nyoba mbusak vèrsi pungkasan berkas iki.
Mangga bali ing vèrsi sing luwih lawas dhisik.',
'filedelete-otherreason'      => 'Alesan tambahan/liya:',
'filedelete-reason-otherlist' => 'Alesan liya',
'filedelete-reason-dropdown'  => '*Alesan pambusakan
** Nglanggar hak cipta
** Berkas duplikat',
'filedelete-edit-reasonlist'  => 'Sunting alesan pambusakan',

# MIME search
'mimesearch'         => 'Panggolèkan MIME',
'mimesearch-summary' => 'Kaca iki nyedyaké fasilitas nyaring berkas miturut tipe MIME-né. Lebokna: contenttype/subtype, contoné <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipe MIME:',
'download'           => 'undhuh',

# Unwatched pages
'unwatchedpages' => 'Kaca-kaca sing ora diawasi',

# List redirects
'listredirects' => 'Daftar pengalihan',

# Unused templates
'unusedtemplates'     => 'Cithakan sing ora dienggo',
'unusedtemplatestext' => 'Daftar iki ngandhut kaca-kaca ing bilik nama cithakan sing ora dienggo ing kaca ngendi waé. Priksanen dhisik pranala-pranala menyang cithakan iki sadurungé mbusak.',
'unusedtemplateswlh'  => 'pranala liya-liyané',

# Random page
'randompage'         => 'Sembarang kaca',
'randompage-nopages' => 'Ora ana kaca ing bilik jeneng iki.',

# Random redirect
'randomredirect'         => 'Pangalihan sembarang',
'randomredirect-nopages' => 'Ing bilik nama iki ora ana pangalihan.',

# Statistics
'statistics'             => 'Statistik',
'sitestats'              => 'Statistik situs',
'userstats'              => 'Statistik panganggo',
'sitestatstext'          => "{{SITENAME}} saiki iki duwèni '''\$2''' {{PLURAL:\$1|kaca|kaca}} artikel sing absah. 

Saliyané iku saiki gunggungé ana {{PLURAL:\$1|kaca|kaca}} ''database''. Ing iku kalebu kaca-kaca dhiskusi, prakara {{SITENAME}}, artikel \"stub\" (rintisan), kaca pangalihan (''redirect''), karo kaca-kaca sing dudu kaca isi.

Banjur wis ana '''\$8''' berkas sing diunggahaké.

Wis tau ana '''\$3''' kaca dituduhaké karo '''\$4''' kaca tau disunting sawisé wiki iki diadegaké.

Dadi tegesé rata-rata ana '''\$5''' suntingan per kaca karo '''\$6''' tayangan per suntingan.

Dawané [http://www.mediawiki.org/wiki/Manual:Job_queue antrian tugas] ana '''\$7'''.",
'userstatstext'          => "Ana '''$1''' [[Special:ListUsers|{{PLURAL:$1|panganggo|panganggo}}]] sing wis ndaftar. '''$2''' (utawa '''$4%''') antarané iku {{PLURAL:$2|duwé|duwé}} hak aksès $5.",
'statistics-mostpopular' => 'Kaca sing paling akèh dituduhaké',

'disambiguations'      => 'Kaca disambiguasi',
'disambiguationspage'  => 'Template:Disambig',
'disambiguations-text' => "Kaca-kaca iki ndarbèni pranala menyang sawijining ''kaca disambiguasi''.
Kaca-kaca iku sajatiné kuduné nyambung menyang topik-topik sing bener.<br />
Sawijining kaca dianggep minangka kaca disambiguasi yèn kaca iku nganggo cithakan sing nyambung menyang [[MediaWiki:Disambiguationspage]].",

'doubleredirects'     => 'Pangalihan dobel',
'doubleredirectstext' => 'Kaca iki ngandhut daftar kaca sing ngalih ing kaca pangalihan liyané. Saben baris ngandhut pranala menyang pangalihan kapisan lan pangalihan kapindho serta tujuan saka pangalihan kapindho sing biasané kaca tujuan sing "sajatiné". Kaca pangalihan kapisan samesthiné kudu dialihaké menyang kaca tujuan iku.',

'brokenredirects'        => 'Pangalihan rusak',
'brokenredirectstext'    => 'Pengalihanipun kaca punika mboten kepanggih sambunganipun.',
'brokenredirects-edit'   => '(sunting)',
'brokenredirects-delete' => '(busak)',

'withoutinterwiki'         => 'Kaca tanpa pranala antarbasa',
'withoutinterwiki-summary' => 'Kaca-kaca iki ora nduwé pranala menyang vèrsi ing  basa liyané:',
'withoutinterwiki-legend'  => 'Préfiks',
'withoutinterwiki-submit'  => 'Tuduhna',

'fewestrevisions' => 'Artikel mawa owah-owahan sithik dhéwé',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bita|bita}}',
'ncategories'             => '$1 {{PLURAL:$1|kategori|kategori}}',
'nlinks'                  => '$1 {{PLURAL:$1|pranala|pranala}}',
'nmembers'                => '$1 {{PLURAL:$1|anggota|anggota}}',
'nrevisions'              => '$1 {{PLURAL:$1|revisi|revisi}}',
'nviews'                  => 'Wis kaping $1 {{PLURAL:$1|dituduhaké|dituduhaké}}',
'specialpage-empty'       => 'Ora ana sing perlu dilaporaké.',
'lonelypages'             => 'Kaca tanpa dijagani',
'lonelypagestext'         => 'Kaca-kaca sing kapacak ing ngisor iki ora ana sing nyambung saka kaca liyané siji-sijia ing {{SITENAME}}.',
'uncategorizedpages'      => 'Kaca sing ora dikategorisasi',
'uncategorizedcategories' => 'Kategori sing ora dikategorisasi',
'uncategorizedimages'     => 'Berkas sing ora dikategorisasi',
'uncategorizedtemplates'  => 'Cithakan sing ora dikategorisasi',
'unusedcategories'        => 'Kategori sing ora dienggo',
'unusedimages'            => 'Berkas sing ora dienggo',
'popularpages'            => 'Kaca populèr',
'wantedcategories'        => 'Kategori sing diperlokaké',
'wantedpages'             => 'Kaca sing dipèngini',
'mostlinked'              => 'Kaca sing kerep dhéwé dituju',
'mostlinkedcategories'    => 'Kategori sing kerep dhéwé dienggo',
'mostlinkedtemplates'     => 'Cithakan sing kerep dhéwé dienggo',
'mostcategories'          => 'Kaca sing kategoriné akèh dhéwé',
'mostimages'              => 'Berkas sing kerep dhéwé dienggo',
'mostrevisions'           => 'Kaca mawa pangowahan sing akèh dhéwé',
'prefixindex'             => 'Indeks awalan',
'shortpages'              => 'Kaca cendhak',
'longpages'               => 'Kaca dawa',
'deadendpages'            => 'Kaca-kaca buntu (tanpa pranala)',
'deadendpagestext'        => 'kaca-kaca iki ora nduwé pranala tekan ngendi waé ing wiki iki..',
'protectedpages'          => 'Kaca sing direksa',
'protectedpages-indef'    => 'Namung pangreksan ora langgeng waé',
'protectedpagestext'      => 'Kaca-kaca sing kapacak iki direksa déning pangalihan utawa panyuntingan.',
'protectedpagesempty'     => 'Saat ini tidak ada halaman yang sedang dilindungi.',
'protectedtitles'         => 'Irah-irahan sing direksa',
'protectedtitlestext'     => 'Irah-irahan sing kapacak ing ngisor iki direksa lan ora bisa digawé',
'protectedtitlesempty'    => 'Ora ana irah-irahan utawa judhul sing direksa karo paramèter-paramèter iki.',
'listusers'               => 'Daftar panganggo',
'newpages'                => 'Kaca énggal',
'newpages-username'       => 'Asma panganggo:',
'ancientpages'            => 'Kaca-kaca langkung sepuh',
'move'                    => 'Pindhahen',
'movethispage'            => 'Pindhahna kaca iki',
'unusedimagestext'        => '<p>Gatèkna yèn situs wèb liyané mbok-menawa bisa nyambung ing sawijining berkas sacara langsung, lan berkas-berkas kaya mengkéné iku mbok-menawa ana ing daftar iki senadyan isih dienggo déning situs wèb liya.',
'unusedcategoriestext'    => 'Kategori iki ana senadyan ora ana artikel utawa kategori liyané sing nganggo.',
'notargettitle'           => 'Ora ana sasaran',
'notargettext'            => 'Panjenengan ora nemtokaké kaca utawa panganggo tujuan fungsi iki.',
'pager-newer-n'           => '{{PLURAL:$1|1 luwih anyar|$1 luwih anyar}}',
'pager-older-n'           => '{{PLURAL:$1|1 luwih lawas|$1 luwih lawas}}',
'suppress'                => "Pangawas (''oversight'')",

# Book sources
'booksources'               => 'Sumber buku',
'booksources-search-legend' => 'Golèk ing sumber buku',
'booksources-go'            => 'Golèk',
'booksources-text'          => 'Ing ngisor iki kapacak daftar pranala menyang situs liyané sing ngadol buku anyar lan bekas, lan mbok-menawa uga ndarbèni informasi sabanjuré ngenani buku-buku sing lagi panjenengan golèki:',

# Special:Log
'specialloguserlabel'  => 'Panganggo:',
'speciallogtitlelabel' => 'Irah-irahan (judhul):',
'log'                  => 'Log',
'all-logs-page'        => 'Kabèh log',
'log-search-legend'    => 'Golèk log',
'log-search-submit'    => 'Golèk',
'alllogstext'          => 'Ing ngisor iki kapacak gabungan log impor, pamblokiran, pamindhahan, pangunggahan, pambusakan, pangreksan, pangowahan hak aksès, lan liya-liyané ing {{SITENAME}}. 
Panjenengan bisa ngwatesi panuduhan mawa milih jenis log, jeneng panganggo, utawa irah-irahan kaca sing dipengaruhi.',
'logempty'             => 'Ora ditemokaké èntri log sing pas.',
'log-title-wildcard'   => 'Golèk irah-irahan utawa judhul sing diawali mawa tèks kasebut',

# Special:AllPages
'allpages'          => 'Kabèh kaca',
'alphaindexline'    => '$1 tumuju $2',
'nextpage'          => 'Kaca sabanjuré ($1)',
'prevpage'          => 'Kaca sadurungé ($1)',
'allpagesfrom'      => 'Kaca-kaca kawiwitan kanthi:',
'allarticles'       => 'Kabèh artikel',
'allinnamespace'    => 'Kabeh kaca ($1 namespace)',
'allnotinnamespace' => 'Sedaya kaca (mboten panggènan asma $1)',
'allpagesprev'      => 'Sadèrèngipun',
'allpagesnext'      => 'Sabanjuré',
'allpagessubmit'    => 'Madosi',
'allpagesprefix'    => 'Kapacak kaca-kaca ingkang mawi ater-ater:',
'allpagesbadtitle'  => 'Irah-irahan (judhul) ingkang dipun-gunaaken boten sah utawi nganggé ater-ater (awalan) antar-basa utawi antar-wiki. Irah-irahan punika saged ugi nganggé setunggal aksara utawi luwih ingkang boten saged kagunaaken dados irah-irahan.',
'allpages-bad-ns'   => '{{SITENAME}} ora duwé bilik nama "$1".',

# Special:Categories
'categories'                    => 'Daftar kategori',
'categoriespagetext'            => 'Kategori-kategori punika wonten ing wiki.',
'special-categories-sort-count' => 'urutna miturut angka',
'special-categories-sort-abc'   => 'urutna miturut abjad',

# Special:ListUsers
'listusersfrom'      => 'Tuduhna panganggo sing diawali karo:',
'listusers-submit'   => 'Tuduhna',
'listusers-noresult' => 'Panganggo ora ditemokaké.',

# Special:ListGroupRights
'listgrouprights'          => 'Hak-hak grup panganggo',
'listgrouprights-summary'  => 'Ing ngisor iki kapacak daftar grup panganggo sing didéfinisi ing wiki iki, karo hak-hak aksès gandhèngané.
Informasi tambahan perkara hak-hak individual bisa ditemokaké ing [[{{MediaWiki:Listgrouprights-helppage}}|kéné]].',
'listgrouprights-group'    => 'Grup',
'listgrouprights-rights'   => 'Hak-hak',
'listgrouprights-helppage' => 'Help:Hak-hak grup',
'listgrouprights-members'  => '(daftar anggota)',

# E-mail user
'mailnologin'     => 'Ora ana alamat layang e-mail',
'mailnologintext' => 'Panjenengan kudu [[Special:UserLogin|mlebu log]] lan kagungan alamat e-mail sing sah ing [[Special:Preferences|preféèrensi]] yèn kersa ngirim layang e-mail kanggo panganggo liya.',
'emailuser'       => 'Kirim e-mail panganggo iki',
'emailpage'       => 'Kirimi panganggo iki layang e-mail',
'emailpagetext'   => 'Yèn panganggo iki nglebokaké alamat layang e-mailé sing absah sajroning préferènsiné, formulir ing ngisor iki bakal ngirimaké sawijining layang e-mail. Alamat e-mail sing ana ing préferènsi panjenengan bakal metu minangka alamat "Saka" ing layang e-mail iku, dadi sing nampa bisa mbales layang e-mail panjenengan.',
'usermailererror' => 'Kaluputan obyèk layang:',
'defemailsubject' => 'Layang e-mail {{SITENAME}}',
'noemailtitle'    => 'Ora ana alamat layang e-mail',
'noemailtext'     => 'Panganggo iki ora nglebokaké alamat layang e-mail sing absah, utawa milih ora gelem nampa layang e-mail saka panganggo liyané.',
'emailfrom'       => 'Saka',
'emailto'         => 'Kanggo',
'emailsubject'    => 'Prekara',
'emailmessage'    => 'Pesen',
'emailsend'       => 'Kirim',
'emailccme'       => 'Kirimana aku salinan pesenku.',
'emailccsubject'  => 'Salinan pesen panjenengan kanggo $1: $2',
'emailsent'       => 'Layang e-mail wis dikirim',
'emailsenttext'   => 'Layang e-mail panjenengan wis dikirim.',

# Watchlist
'watchlist'            => 'Daftar artikel pilihan',
'mywatchlist'          => 'Daftar pangawasanku',
'watchlistfor'         => "(kanggo '''$1''')",
'nowatchlist'          => 'Daftar pangawasan panjenengan kosong.',
'watchlistanontext'    => 'Mangga $1 kanggo mirsani utawa nyunting daftar pangawasan panjenengan.',
'watchnologin'         => 'Durung mlebu log',
'watchnologintext'     => 'Panjenengan kudu [[Special:UserLogin|mlebu log]] kanggo ngowahi daftar artikel pilihan.',
'addedwatch'           => 'Sampun katambahaken wonten ing daftar artikel pilihan.',
'addedwatchtext'       => "Kaca \"[[:\$1]]\" wis ditambahaké menyang [[Special:Watchlist|daftar pangawasan]].
Owah-owahan sing dumadi ing tembé ing kaca iku lan kaca dhiskusi sing kagandhèng, bakal dipacak ing kéné, lan kaca iku bakal dituduhaké '''kandel''' ing [[Special:RecentChanges|daftar owah-owahan iku]] supados luwih gampang katon.",
'removedwatch'         => 'Wis dibusak saka daftar pangawasan',
'removedwatchtext'     => 'Kaca "<nowiki>$1</nowiki>" wis dibusak saka daftar pangawasan.',
'watch'                => 'tutana',
'watchthispage'        => 'Periksa kaca iki',
'unwatch'              => 'Ora usah ngawasaké manèh',
'unwatchthispage'      => 'Batalna olèhé ngawasi kaca iki',
'notanarticle'         => 'Dudu kaca artikel',
'notvisiblerev'        => 'Révisi wis dibusak',
'watchnochange'        => 'Ora ana kaca ing daftar pangawasan panjenengan sing diowahi ing mangsa wektu sing dipilih.',
'watchlist-details'    => 'Ngawasaké {{PLURAL:$1|$1 kaca|$1 kaca}}, ora kalebu kaca-kaca dhiskusi.',
'wlheader-enotif'      => '* Notifikasi e-mail diaktifaké.',
'wlheader-showupdated' => "* Kaca-kaca sing wis owah wiwit ditiliki panjenengan kaping pungkasan, dituduhaké mawa '''aksara kandel'''",
'watchmethod-recent'   => 'priksa daftar owah-owahan anyar kanggo kaca sing diawasi',
'watchmethod-list'     => 'priksa kaca sing diawasi kanggo owah-owahan anyar',
'watchlistcontains'    => 'Daftar pangawasan panjenengan isiné ana $1 {{PLURAL:$1|kaca|kaca}}.',
'iteminvalidname'      => "Ana masalah karo '$1', jenengé ora absah...",
'wlnote'               => "Ing ngisor iki kapacak $1 {{PLURAL:$1|owah-owahan|owah-owahan}} pungkasan ing '''$2''' jam kapungkur.",
'wlshowlast'           => 'Tuduhna $1 jam $2 dina $3 pungkasan',
'watchlist-show-bots'  => 'Tuduhna suntingan bot',
'watchlist-hide-bots'  => 'Delikna suntingan bot',
'watchlist-show-own'   => 'Tuduhna suntinganku',
'watchlist-hide-own'   => 'Delikna suntinganku',
'watchlist-show-minor' => 'Tuduhna suntingan cilik',
'watchlist-hide-minor' => 'Delikna suntingan cilik',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Ngawasi...',
'unwatching' => 'Ngilangi pangawasan...',

'enotif_mailer'                => 'Pangirim Notifikasi {{SITENAME}}',
'enotif_reset'                 => 'Tandhanana kabèh kaca sing wis ditiliki',
'enotif_newpagetext'           => 'Iki sawijining kaca anyar.',
'enotif_impersonal_salutation' => 'Panganggo {{SITENAME}}',
'changed'                      => 'kaubah',
'created'                      => 'kadamel',
'enotif_subject'               => 'Kaca $PAGETITLE ing {{SITENAME}} wis $CHANGEDORCREATED déning $PAGEEDITOR',
'enotif_lastvisited'           => 'Deleng $1 kanggo kabèh owah-owahan wiwit pungkasan panjenengan niliki.',
'enotif_lastdiff'              => 'Tilikana $1 kanggo mirsani owah-owahan iki.',
'enotif_anon_editor'           => 'panganggo anonim $1',
'enotif_body'                  => 'Sing minulya $WATCHINGUSERNAME,

Kaca $PAGETITLE ing {{SITENAME}} wis $CHANGEDORCREATED ing $PAGEEDITDATE déning $PAGEEDITOR, mangga mirsani $PAGETITLE_URL kanggo vèrsi pungkasan.

$NEWPAGE

Sajarah suntingan: $PAGESUMMARY $PAGEMINOREDIT

Hubungana panyunting:
mail: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Kita ora bakal ngandhani manèh yèn diowahi manèh, kejaba panjenengan wis mirsani kaca iku. Panjenengan uga bisa mbusak tandha notifikasi kanggo kabèh kaca pangawasan ing daftar pangawasan panjenengan.

             Sistém notifikasi {{SITENAME}}

--
Kanggo ngowahi préferènsi ing daftar pangawasan panjenengan, mangga mirsani
{{fullurl:{{ns:special}}:Watchlist/edit}}

Umpan balik lan pitulung sabanjuré:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Busak kaca',
'confirm'                     => 'Dhedhes (konfirmasi)',
'excontent'                   => "isi sadurungé: '$1'",
'excontentauthor'             => "isiné mung arupa: '$1' (lan siji-sijiné sing nyumbang yaiku '$2')",
'exbeforeblank'               => "isi sadurungé dikosongaké: '$1'",
'exblank'                     => 'kaca kosong',
'delete-confirm'              => 'Busak "$1"',
'delete-legend'               => 'Busak',
'historywarning'              => 'Pènget: Kaca sing bakal panjenengan busak ana sajarahé:',
'confirmdeletetext'           => 'Panjenengan bakal mbusak kaca utawa berkas iki minangka permanèn karo kabèh sajarahé saka basis data. Pastèkna dhisik menawa panjenengan pancèn nggayuh iki, ngerti kabèh akibat lan konsekwènsiné, lan apa sing bakal panjenengan tumindak iku cocog karo [[{{MediaWiki:Policy-url}}|kawicaksanan {{SITENAME}}]].',
'actioncomplete'              => 'Proses tuntas',
'deletedtext'                 => '"<nowiki>$1</nowiki>" sampun kabusak. Coba pirsani $2 kanggé log paling énggal kaca ingkang kabusak.',
'deletedarticle'              => 'mbusak "[[$1]]"',
'suppressedarticle'           => 'ndelikaké "[[$1]]"',
'dellogpage'                  => 'Cathetan pambusakan',
'dellogpagetext'              => 'Ing ngisor iki kapacak log pambusakan kaca sing anyar dhéwé.',
'deletionlog'                 => 'Cathetan sing dibusak',
'reverted'                    => 'Dibalèkaké ing revisi sadurungé',
'deletecomment'               => 'Alesan dibusak:',
'deleteotherreason'           => 'Alesan liya utawa tambahan:',
'deletereasonotherlist'       => 'Alesan liya',
'deletereason-dropdown'       => '*Alesan pambusakan
** Disuwun sing nulis
** Nglanggar hak cipta
** Vandalisme',
'delete-edit-reasonlist'      => 'Sunting alesan pambusakan',
'delete-toobig'               => 'Kaca iki ndarbèni sajarah panyuntingan sing dawa, yaiku ngluwihi $1 révisi. Pambusakan kaca mawa sajarah panyuntingan sing dawa ora diparengaké kanggo menggak anané karusakan ing {{SITENAME}}.',
'delete-warning-toobig'       => 'Kaca iki duwé sajarang panyuntingan sing dawa, luwih saka $1 révisi.
Mbusak kaca iki bisa nyebabaké masalah operasional basis data {{SITENAME}};
mangga digalih manèh kersa nerusaké ora.',
'rollback'                    => 'Mangsulaken suntingan',
'rollback_short'              => 'Balèkna',
'rollbacklink'                => 'balèaké',
'rollbackfailed'              => 'Pambalèkan gagal dilakoni',
'cantrollback'                => 'Ora bisa mbalèkaké suntingan; panganggo pungkasan iku siji-sijiné penulis artikel iki.',
'alreadyrolled'               => 'Ora bisa mbalèkaké menyang suntingan pungkasan [[:$1]] déning [[User:$2|$2]] ([[User talk:$2|Wicara]]); 
wong liya wis nyunting utawa mbalèkaké kaca artikel iku. 

Suntingan pungkasan dilakoni déning [[User:$3|$3]] ([[User talk:$3|Wicara]]).',
'editcomment'                 => 'Komentar panyuntingané yaiku: "<em>$1</em>".', # only shown if there is an edit comment
'revertpage'                  => 'Suntingan [[Special:Contributions/$2|$2]] ([[User talk:$2|dhiskusi]]) dipunwangsulaken dhateng ing vèrsi pungkasan déning [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Suntingan dibalèkaké déning $1;
diowahi bali menyang vèrsi pungkasan déning $2.',
'sessionfailure'              => 'Katoné ana masalah karo sèsi log panjenengan; log panjenengan wis dibatalaké kanggo nyegah pambajakan. Mangga mencèt tombol "back" lan unggahaké manèh kaca sadurungé mlebu log, lan coba manèh.',
'protectlogpage'              => 'Log pangreksan',
'protectlogtext'              => 'Ing ngisor iki kapacak log pangreksan lan panjabelan reksa kaca.
Mangga mirsani [[Special:ProtectedPages|daftar kaca sing direksa]] kanggo daftar pangreksan kaca pungkasan.',
'protectedarticle'            => 'ngreksa "[[$1]]"',
'modifiedarticleprotection'   => 'ngowahi tingkat pangreksan "[[$1]]"',
'unprotectedarticle'          => 'ngilangi pangreksan "[[$1]]"',
'protect-title'               => 'Ngowahi tingkatan pangreksan kanggo "$1"',
'protect-legend'              => 'Konfirmasi pangreksan',
'protectcomment'              => 'Komentar:',
'protectexpiry'               => 'Kadaluwarsa:',
'protect_expiry_invalid'      => 'Wektu kadaluwarsané ora sah.',
'protect_expiry_old'          => 'Wektu kadaluwarsané kuwi ana ing jaman biyèn.',
'protect-unchain'             => 'Bukak pangreksan pamindhahan',
'protect-text'                => 'Panjenengan bisa mirsani utawa ngganti tingkatan pangreksan kanggo kaca <strong><nowiki>$1</nowiki></strong> ing kéné.',
'protect-locked-blocked'      => 'Panjenengan ora bisa ngganti tingkat pangreksan yèn lagi diblokir.
Ing ngisor iki kapacak konfigurasi saiki iki kanggo kaca <strong>$1</strong>:',
'protect-locked-dblock'       => 'Tingkat pangreksan ora bisa diganti amerga anané panguncèn aktif basis data.
Ing ngisor iki kapacak konfigurasi kanggo kaca <strong>$1</strong>:',
'protect-locked-access'       => 'Akun utawa rékening panjenengan ora awèh idin kanggo ngganti tingkat pangreksan kaca. Ing ngisor iki kapacak konfigurasi saiki iki kanggo kaca <strong>$1</strong>:',
'protect-cascadeon'           => 'Kaca iki lagi direksa amerga disertakaké ing {{PLURAL:$1|kaca|kaca-kaca}} sing wis direksa mawa pilihan pangreksan runtun diaktifaké. Panjenengan bisa ngganti tingkat pangreksan kanggo kaca iki, nanging perkara iku ora awèh pengaruh pangreksan runtun.',
'protect-default'             => '(baku)',
'protect-fallback'            => 'Perlu idin hak aksès "$1"',
'protect-level-autoconfirmed' => 'Blokir panganggo sing ora kadaftar',
'protect-level-sysop'         => 'Namung opsis (operator sistem)',
'protect-summary-cascade'     => 'runtun',
'protect-expiring'            => 'kadaluwarsa $1 (UTC)',
'protect-cascade'             => 'Reksanen kabèh kaca sing kalebu ing kaca iki (pangreksan runtun).',
'protect-cantedit'            => 'Panjenengan ora pareng ngowahi tingkatan pangreksan kaca iki amerga panjenengan ora kagungan idin nyunting kaca iki.',
'restriction-type'            => 'Pangreksan:',
'restriction-level'           => 'Tingkatan pambatesan:',
'minimum-size'                => 'Ukuran minimum',
'maximum-size'                => 'Ukuran maksimum:',
'pagesize'                    => '(bita)',

# Restrictions (nouns)
'restriction-edit'   => 'Panyuntingan',
'restriction-move'   => 'Pamindhahan',
'restriction-create' => 'Gawé',
'restriction-upload' => 'Unggah',

# Restriction levels
'restriction-level-sysop'         => 'pangreksan kebak',
'restriction-level-autoconfirmed' => 'pangreksan sémi',
'restriction-level-all'           => 'kabèh tingkatan',

# Undelete
'undelete'                     => 'Kembalikan halaman yang telah dihapus',
'undeletepage'                 => 'Lihat dan kembalikan halaman yang telah dihapus',
'undeletepagetitle'            => "'''Ing ngisor iki kapacak daftar révisi sing dibusak saka [[:$1]]'''.",
'viewdeletedpage'              => 'Deleng kaca sing wis dibusak',
'undeletepagetext'             => 'Kaca-kaca sing kapacak ing ngisor iki wis dibusak, nanging isih ana sajroning arsip lan bisa dibalèkaké.
Nanging arsipé bisa diresiki sakala-kala.',
'undeleteextrahelp'            => "Kanggo mbalèkaké kaca sacara kabèh, lirwakna kabèh kothak cèk ora dipilih siji-sijia lan kliken '''''Balèkna'''''. Kanggo nglakoni pambalèkan sèlèktif, cèk kothak révisi sing dipéngini lan kliken '''''Balèkna'''''. Yèn mencèt tombol '''''Reset''''' bakal ngosongaké isi komentar lan kabèh kothak cèk.",
'undeleterevisions'            => '$1 {{PLURAL:$1|révisi|révisi}} diarsipaké',
'undeletehistory'              => 'Jika Anda mengembalikan halaman tersebut, semua revisi akan dikembalikan ke dalam sejarah. Jika sebuah halaman baru dengan nama yang sama telah dibuat sejak penghapusan, revisi yang telah dikembalikan akan kelihatan dalam sejarah dahulu, dan revisi terkini halaman tersebut tidak akan ditimpa secara otomatis.',
'undeleterevdel'               => 'Pambatalan pambusakan ora bakal dilakokaké yèn bab iku bakal ngakibataké révisi pungkasan kaca dadi sabagéyan kabusak.
Ing kasus kaya mengkono, panjenengan kudu ngilangaké cèk utawa mbusak pandelikan révisi kabusak sing anyar dhéwé.',
'undeletehistorynoadmin'       => 'Kaca iki wis dibusak.
Alesané dituduhaké ing ringkesan ing ngisor iki, karo détail para panganggo sing wis nyunting kaca iki sadurungé dibusak.
Isi pungkasan tèks iki wis dibusak lan namung bisa dideleng para pangurus.',
'undelete-revision'            => 'Révisi sing wis dibusak saka $1 (nganti $2) déning $3:',
'undeleterevision-missing'     => 'Revisi salah utawa ora ditemokaké. 
Panjenengan mbokmenawa ngetutaké pranala sing salah, utawa revisi iku wis dipulihaké utawa diguwang saka arsip.',
'undelete-nodiff'              => 'Ora ditemokaké révisi sing luwih lawas.',
'undeletebtn'                  => 'Balèkna!',
'undeletelink'                 => 'balèkna',
'undeletereset'                => "''Reset''",
'undeletecomment'              => 'Komentar:',
'undeletedarticle'             => '"$1" wis dibalèkaké',
'undeletedrevisions'           => '$1 {{PLURAL:$1|révisi|révisi}} wis dibalèkaké',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|révisi|révisi}} lan $2 berkas dibalèkaké',
'undeletedfiles'               => '$1 {{PLURAL:$1|berkas|berkas}} dibalèkaké',
'cannotundelete'               => 'Olèhé mbatalaké pambusakan gagal; 
mbokmenawa wis ana wong liya sing luwih dhisik nglakoni pambatalan.',
'undeletedpage'                => "<big>'''$1 bisa dibalèkaké'''</big>

Delengen [[Special:Log/delete|log pambusakan]] kanggo data pambusakan lan pambalèkan.",
'undelete-header'              => 'Mangga mirsani [[Special:Log/delete|log pambusakan]] kanggo daftar kaca sing lagi waé dibusak.',
'undelete-search-box'          => 'Golèk kaca-kaca sing wis dibusak',
'undelete-search-prefix'       => 'Tuduhna kaca sing diwiwiti karo:',
'undelete-search-submit'       => 'Golèk',
'undelete-no-results'          => 'Ora ditemokaké kaca sing cocog ing arsip pambusakan.',
'undelete-filename-mismatch'   => 'Ora bisa mbatalaké pambusakan révisi berkas mawa tandha wektu $1: jeneng berkas ora padha',
'undelete-bad-store-key'       => 'Ora bisa mbatalaké pambusakan révisi berkas mawa tandha wektu $1: berkas ilang sadurungé dibusak.',
'undelete-cleanup-error'       => 'Ana kaluputan nalika mbusak arsip berkas "$1" sing ora dienggo.',
'undelete-missing-filearchive' => 'Ora bisa mbalèkaké arsip bekas mawa ID $1 amerga ora ana ing basis data.
Berkas iku mbok-menawa wis dibusak.',
'undelete-error-short'         => 'Kaluputan olèhé mbatalaké pambusakan: $1',
'undelete-error-long'          => 'Ana kaluputan nalika mbatalaké pambusakan berkas:

$1',

# Namespace form on various pages
'namespace'      => 'Bilik nama (bilik jeneng):',
'invert'         => 'Balèkna pilihan',
'blanknamespace' => '(Utama)',

# Contributions
'contributions' => 'Sumbangan panganggo',
'mycontris'     => 'Kontribusiku',
'contribsub2'   => 'Kagem $1 ($2)',
'nocontribs'    => 'Ora ditemokaké owah-owahan sing cocog karo kritéria kasebut iku.',
'uctop'         => ' (dhuwur)',
'month'         => 'Wiwit sasi (lan sadurungé):',
'year'          => 'Wiwit taun (lan sadurungé):',

'sp-contributions-newbies'     => 'Namung panganggo-panganggo anyar',
'sp-contributions-newbies-sub' => 'Kanggo panganggo anyar',
'sp-contributions-blocklog'    => 'Log pemblokiran',
'sp-contributions-search'      => 'Golèk kontribusi',
'sp-contributions-username'    => 'Alamat IP utawa jeneng panganggo:',
'sp-contributions-submit'      => 'Golèk',

# What links here
'whatlinkshere'            => 'Pranala balik',
'whatlinkshere-title'      => 'Kaca-kaca sing duwé pranala menyang $1',
'whatlinkshere-page'       => 'Kaca:',
'linklistsub'              => '(Daftar pranala)',
'linkshere'                => "Kaca-kaca iki nduwé pranala menyang '''[[:$1]]''':",
'nolinkshere'              => "Ora ana kaca sing nduwé pranala menyang '''[[:$1]]'''.",
'nolinkshere-ns'           => " Ora ana kaca sing nduwé pranala menyang '''[[:$1]]''' ing bilik jeneng sing kapilih.",
'isredirect'               => 'kaca pangalihan',
'istemplate'               => 'karo cithakan',
'isimage'                  => 'pranala berkas',
'whatlinkshere-prev'       => '{{PLURAL:$1|sadurungé|$1 sadurungé}}',
'whatlinkshere-next'       => '{{PLURAL:$1|sabanjuré|$1 sabanjuré}}',
'whatlinkshere-links'      => '← pranala',
'whatlinkshere-hideredirs' => '$1 pangalihan-pangalihan',
'whatlinkshere-hidetrans'  => '$1 transklusi',
'whatlinkshere-hidelinks'  => 'pranala-pranala $1',
'whatlinkshere-hideimages' => '$1 pranala-pranala berkas',
'whatlinkshere-filters'    => 'Filter-filter',

# Block/unblock
'blockip'                     => 'Blokir panganggo',
'blockip-legend'              => 'Blokir panganggo',
'blockiptext'                 => 'Enggonen formulir ing ngisor iki kanggo mblokir sawijining alamat IP utawa panganggo supaya ora bisa nyunting kaca.
Prekara iki perlu dilakoni kanggo menggak vandalisme, lan miturut [[{{MediaWiki:Policy-url}}|kawicaksanan {{SITENAME}}]].
Lebokna alesan panjenengan ing ngisor iki (contoné njupuk conto kaca sing wis tau dirusak).',
'ipaddress'                   => 'Alamat IP',
'ipadressorusername'          => 'Alamat IP utawa jeneng panganggo',
'ipbexpiry'                   => 'Kadaluwarsa',
'ipbreason'                   => 'Alesan',
'ipbreasonotherlist'          => 'Alesan liya',
'ipbreason-dropdown'          => '*Alesan umum mblokir panganggo
** Mènèhi informasi palsu
** Ngilangi isi kaca
** Spam pranala menyang situs njaba
** Nglebokaké tulisan ngawur ing kaca
** Tumindak intimidasi/nglècèhaké
** Nyalahgunakaké sawetara akun utawa rékening
** Jeneng panganggo ora layak',
'ipbanononly'                 => 'Blokir panganggo anonim waé',
'ipbcreateaccount'            => 'Penggak nggawé akun utawa rékening',
'ipbemailban'                 => 'Penggak panganggo ngirim layang e-mail',
'ipbenableautoblock'          => 'Blokir alamat IP pungkasan sing dienggo déning pengguna iki sacara otomatis, lan kabèh alamat sabanjuré sing dicoba arep dienggo nyunting.',
'ipbsubmit'                   => 'Kirimna',
'ipbother'                    => 'Wektu liya',
'ipboptions'                  => '2 jam:2 hours,1 dina:1 day,3 dina:3 days,1 minggu:1 week,2 minggu:2 weeks,1 sasi:1 month,3 sasi:3 months,6 sasi:6 months,1 taun:1 year,tanpa wates:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'liyané',
'ipbotherreason'              => 'Alesan liya/tambahan',
'ipbhidename'                 => 'Delikna jeneng panganggo utawa alamat IP saka log pamblokiran, daftar blokir aktif, sarta daftar panganggo',
'ipbwatchuser'                => 'Ngawasi kaca panganggo lan kaca-kaca dhiskusi panganggo iki',
'badipaddress'                => 'Alamat IP klèntu',
'blockipsuccesssub'           => 'Pemblokiran suksès',
'blockipsuccesstext'          => 'Alamat IP utawa panganggo "$1" wis diblokir. <br />Delengen [[Special:IPBlockList|Daftar IP lan panganggo diblokir]] kanggo ndeleng manèh pemblokiran.',
'ipb-edit-dropdown'           => 'Sunting alesan pamblokiran',
'ipb-unblock-addr'            => 'Ilangna blokir $1',
'ipb-unblock'                 => 'Ilangna blokir sawijining panganggo utawa alamat IP',
'ipb-blocklist-addr'          => 'Ndeleng blokir sing ditrapaké kanggo $1',
'ipb-blocklist'               => 'Ndeleng blokir sing lagi ditrapaké',
'unblockip'                   => 'Jabel blokir marang alamat IP utawa panganggo',
'unblockiptext'               => 'Nggonen formulir ing ngisor iki kanggo mbalèkaké aksès nulis sawijining alamt IP utawa panganggo sing sadurungé diblokir.',
'ipusubmit'                   => 'Ilangna blokir ing alamat iki',
'unblocked'                   => 'Blokir marang [[User:$1|$1]] wis dijabel',
'unblocked-id'                => 'Blokir $1 wis dijabel',
'ipblocklist'                 => 'Daftar pemblokiran',
'ipblocklist-legend'          => 'Golèk panganggo sing diblokir',
'ipblocklist-username'        => 'Jeneng panganggo utawa alamat IP:',
'ipblocklist-submit'          => 'Golèk',
'blocklistline'               => '$1, $2 mblokir $3 ($4)',
'infiniteblock'               => 'salawasé',
'expiringblock'               => 'kadaluwarsa $1',
'anononlyblock'               => 'namung anon',
'noautoblockblock'            => 'pamblokiran otomatis dipatèni',
'createaccountblock'          => 'ndamelipun akun dipunblokir',
'emailblock'                  => 'layang e-mail diblokir',
'ipblocklist-empty'           => 'Daftar pamblokiran kosong.',
'ipblocklist-no-results'      => 'alamat IP utawa panganggo sing disuwun ora diblokir.',
'blocklink'                   => 'blokir',
'unblocklink'                 => 'jabel blokir',
'contribslink'                => 'sumbangan',
'autoblocker'                 => 'Panjenengan otomatis dipun-blok amargi nganggé alamat protokol internet (IP) ingkang sami kaliyan "[[User:$1|$1]]". Alesanipun $1 dipun blok inggih punika "\'\'\'$2\'\'\'"',
'blocklogpage'                => 'Log pamblokiran',
'blocklogentry'               => 'mblokir "[[$1]]" dipun watesi wedalipun $2 $3',
'blocklogtext'                => 'Ing ngisor iki kapacak log pamblokiran lan panjabelan blokir panganggo. 
Alamat IP sing diblokir sacara otomatis ora ana ing daftar iki. 
Mangga mirsani [[Special:IPBlockList|daftar alamat IP sing diblokir]] kanggo daftar blokir pungkasan.',
'unblocklogentry'             => 'njabel blokir "$1"',
'block-log-flags-anononly'    => 'namung panganggo anonim waé',
'block-log-flags-nocreate'    => 'opsi nggawé akun utawa rékening dipatèni',
'block-log-flags-noautoblock' => 'blokir otomatis dipatèni',
'block-log-flags-noemail'     => 'e-mail diblokir',
'range_block_disabled'        => 'Fungsi pamblokir blok IP kanggo para opsis dipatèni.',
'ipb_expiry_invalid'          => 'Wektu kadaluwarsa ora absah.',
'ipb_already_blocked'         => '"$1" wis diblokir',
'ipb_cant_unblock'            => 'Kaluputan: Blokir mawa ID $1 ora ditemokaké. Blokir iku mbok-menawa wis dibuka.',
'ipb_blocked_as_range'        => 'Kaluputan: IP $1 ora diblokir sacara langsung lan ora bisa dijabel blokiré. IP $1 diblokir mawa bagéyan saka pamblokiran kelompok IP $2, sing bisa dijabel pamblokirané.',
'ip_range_invalid'            => 'Blok IP ora absah.',
'blockme'                     => 'Blokiren aku',
'proxyblocker'                => 'Pamblokir proxy',
'proxyblocker-disabled'       => 'Fungsi iki saiki lagi dipatèni.',
'proxyblockreason'            => "Alamat IP panjenengan wis diblokir amerga alamat IP panjenengan iku ''open proxy''. 
Mangga ngubungi sing nyedyakaké dines internèt panjenengan utawa pitulungan tèknis lan aturana masalah kaamanan sérius iki.",
'proxyblocksuccess'           => 'Bubar.',
'sorbsreason'                 => "Alamat IP panjenengan didaftar minangka ''open proxy'' ing DNSBL.",
'sorbs_create_account_reason' => "Alamat IP panjenengan didaftar minangka ''open proxy'' ing DNSBL. Panjenengan ora bisa nggawé akun utawa rékening.",

# Developer tools
'lockdb'              => 'Kunci basis data',
'unlockdb'            => 'Buka kunci basis data',
'lockdbtext'          => 'Ngunci basis data bakal menggak kabèh panganggo kanggo nyunting kaca, ngowahi préferènsi panganggo, nyunting daftar pangawasan, lan prekara-prekara liyané sing merlokaké owah-owahan basis data. Pastèkna yèn iki pancèn panjenengan gayuh, lan yèn panjenengan ora lali mbuka kunci basis data sawisé pangopènan rampung.',
'unlockdbtext'        => 'Mbuka kunci basis data bakal mbalèkaké kabèh panganggo bisa nyunting kaca manèh, ngowahi préferènsi panganggo, nyunting daftar pangawasan, lan prekara-prekara liyané sing merlokaké pangowahan marang basis data. 
Tulung pastèkna yèn iki pancèn sing panjenengan gayuh.',
'lockconfirm'         => 'Iya, aku pancèn péngin ngunci basis data.',
'unlockconfirm'       => 'Iya, aku pancèn péngin tmbuka kunci basis data.',
'lockbtn'             => 'Kunci basis data',
'unlockbtn'           => 'Buka kunci basis data',
'locknoconfirm'       => 'Panjenengan ora mènèhi tandha cèk ing kothak konfirmasi.',
'lockdbsuccesssub'    => 'Bisa kasil ngunci basis data',
'unlockdbsuccesssub'  => 'Bisa kasil buka kunci basis data',
'lockdbsuccesstext'   => 'Basis data wis dikunci.
<br />Pastèkna panjenengan [[Special:UnlockDB|mbuka kunciné]] sawisé pangopènan bubar.',
'unlockdbsuccesstext' => 'Kunci basis data wis dibuka.',
'lockfilenotwritable' => 'Berkas kunci basis data ora bisa ditulis. Kanggo ngunci utawa mbuka basis data, berkas iki kudu ditulis déning server wèb.',
'databasenotlocked'   => 'Basis data ora dikunci.',

# Move page
'move-page'               => 'Pindhahna $1',
'move-page-legend'        => 'Mindhah kaca',
'movepagetext'            => "Formulir ing ngisor iki bakal ngowahi jeneng sawijining kaca, mindhah kabèh sajarahé menyang kaca sing anyar. Irah-irahan utawa judhul sing lawas bakal dadi kaca pangalihan menyang irah-irahan sing anyar. Pranala menyang kaca sing lawas ora bakal diowahi; dadi pastèkna dhisik mriksa pangalihan dobel utawa pangalihan sing rusak sawisé pamindhahan. Panjenengan sing tanggung jawab mastèkaké menawa kabèh pranala-pranala tetep nyambung ing kaca panujon kaya samesthiné.

Gatèkna yèn kaca iki '''ora''' bakal dipindhah yèn wis ana kaca liyané sing nganggo irah-irahan sing anyar, kejaba kaca iku kosong utawa ora nduwé sajarah panyuntingan. Dadi tegesé panjenengan bisa ngowahi jeneng kaca iku manèh kaya sedyakala menawa panjenengan luput, lan panjenengan ora bisa nimpani kaca sing wis ana.

'''PÈNGET:''' Perkara iki bisa ngakibataké owah-owahan sing drastis lan ora kaduga kanggo kaca-kaca sing populèr. Pastekaké dhisik panjenengan ngerti konsekwènsi saka panggayuh panjenengan sadurungé dibanjuraké.",
'movepagetalktext'        => "Kaca dhiskusi sing kagandhèng uga bakal dipindhahaké sacara otomatis '''kejaba yèn:'''

*Sawijining kaca dhiskusi sing ora kosong wis ana sangisoring irah-irahan (judhul) anyar, utawa
*Panjenengan ora maringi tandha cèk ing kothak ing ngisor iki.

Ing kasus-kasus iku, yèn panjenengan gayuh, panjenengan bisa mindhahaké utawa nggabung kaca iku sacara manual.",
'movearticle'             => 'Pindhah kaca',
'movenotallowed'          => 'Panjenengan ora pareng ngalihaké kaca.',
'newtitle'                => 'Menyang irah-irahan utawa judhul anyar:',
'move-watch'              => 'Awasna kaca iki',
'movepagebtn'             => 'Pindhahna kaca',
'pagemovedsub'            => 'Bisa kasil dipindhahaké',
'movepage-moved'          => '<big>\'\'\'"$1" dipindhahaké menyang "$2".\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Satunggalipun kaca kanthi asma punika sampun wonten, utawi asma ingkang panjenengan pendhet mboten leres. Sumangga nyobi asma sanèsipun.',
'cantmove-titleprotected' => 'Panjenengan ora bisa mindhahaké kaca iki menyang lokasi iki, amerga irah-irahan tujuan lagi direksa; ora olèh digawé',
'talkexists'              => 'Kaca iku kasil dipindhahaké, nanging kaca dhiskusi saka kaca iku ora bisa dipindhahaké amerga wis ana kaca dhiskusi ing irah-irahan (judhul) sing anyar. Mangga kaca-kaca dhiskusi wau digabung sacara manual.',
'movedto'                 => 'dipindhah menyang',
'movetalk'                => 'Pindahna kaca dhiskusi sing ana gandhèngané.',
'1movedto2'               => '$1 dialihaké menyang $2',
'1movedto2_redir'         => '[[$1]] dipunalihaken menyang [[$2]] via pangalihan',
'movelogpage'             => 'Log pamindhahan',
'movelogpagetext'         => 'Ing ngisor iki kapacak log pangalihan kaca.',
'movereason'              => 'Alesan:',
'revertmove'              => 'balèkaké',
'delete_and_move'         => 'busak lan kapindahaken',
'delete_and_move_text'    => '== Perlu mbusak ==

Artikel sing dituju, "[[:$1]]", wis ana isiné. 
Apa panjenengan kersa mbusak iku supaya kacané bisa dialihaké?',
'delete_and_move_confirm' => 'Ya, busak kaca iku.',
'delete_and_move_reason'  => 'Dibusak kanggo antisipasi pangalihan kaca',
'selfmove'                => 'Pangalihan kaca ora bisa dilakoni amerga irah-irahan utawa judhul sumber lan tujuané padha.',
'immobile_namespace'      => 'Irah-irahan sumber utawa tujuan kalebu tipe kusus; 
ora bisa mindhahaké kaca saka lan menyang bilik nama iku.',
'imagenocrossnamespace'   => 'Ora bisa mindhahaké gambar menyang bilik nama non-gambar',
'imagetypemismatch'       => 'Èkstènsi anyar berkas ora cocog karo jenisé',

# Export
'export'            => 'Ekspor kaca',
'exporttext'        => 'Panjenengan bisa ngèkspor tèks lan sajarah panyuntingan sawijining kaca tartamtu utawa sawijining sèt kaca awujud XML tartamtu. Banjur iki bisa diimpor ing wiki liyané nganggo MediaWiki nganggo fasilitas [[Special:Import|impor kaca]].

Kanggo ngèkspor kaca-kaca artikel, lebokna irah-irahan utawa judhul sajroning kothak tèks ing ngisor iki, irah-irahan utawa judhul siji per baris, lan pilihen apa panjenengan péngin ngèkspor jangkep karo vèrsi sadurungé, utawa namung vèrsi saiki mawa cathetan panyuntingan pungkasan.

Yèn panjenengan namun péngin ngimpor vèrsi pungkasan, panjenengan uga bisa nganggo pranala kusus, contoné [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] kanggo ngèkspor artikel "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Namung èkspor révisi saiki, dudu kabèh vèrsi lawas',
'exportnohistory'   => "----
'''Cathetan:''' Ngèkspor kabèh sajarah suntingan kaca ngliwati formulir iki wis dinon-aktifaké déning alesan kinerja.",
'export-submit'     => 'Èkspor',
'export-addcattext' => 'Tambahna kaca saka kategori:',
'export-addcat'     => 'Tambahna',
'export-download'   => 'Simpen minangka berkas',
'export-templates'  => 'Kalebu cithakan-cithakan',

# Namespace 8 related
'allmessages'               => 'Kabèh laporan sistém',
'allmessagesname'           => 'Asma (jeneng)',
'allmessagesdefault'        => 'Tèks baku',
'allmessagescurrent'        => 'Tèks saiki',
'allmessagestext'           => 'Punika pesen-pesen saking sistem ingkang kacawisaken wonten ing MediaWiki namespace.',
'allmessagesnotsupportedDB' => "Kaca iki ora bisa dienggo amerga '''\$wgUseDatabaseMessages''' dipatèni.",
'allmessagesfilter'         => 'Saringan jeneng pesen:',
'allmessagesmodified'       => 'Namung tampilanipun ingkang owah',

# Thumbnails
'thumbnail-more'           => 'Gedhèkna',
'filemissing'              => 'Berkas ora ditemokaké',
'thumbnail_error'          => "Kaluputan nalika nggawé gambar cilik (''thumbnail''): $1",
'djvu_page_error'          => "Kaca DjVu ana ing sajabaning ranggèhan (''range'')",
'djvu_no_xml'              => 'Ora bisa njupuk XML kanggo berkas DjVu',
'thumbnail_invalid_params' => "Paramèter gambar cilik (''thumbnail'') ora absah",
'thumbnail_dest_directory' => 'Ora bisa nggawé dirèktori tujuan',

# Special:Import
'import'                     => 'Impor kaca',
'importinterwiki'            => 'Impor transwiki',
'import-interwiki-text'      => 'Pilih sawijining wiki lan irah-irahan kaca sing arep diimpor. 
Tanggal révisi lan jeneng panyunting bakal dilestarèkaké. 
Kabèh aktivitas impor transwiki bakal dilog ing [[Special:Log/import|log impor]].',
'import-interwiki-history'   => 'Tuladen kabèh vèrsi lawas saka kaca iki',
'import-interwiki-submit'    => 'Impor',
'import-interwiki-namespace' => 'Pindhahna kaca ing bilik nama:',
'importtext'                 => "Mangga ngèkspor berkasa saka wiki sumber nganggo piranti Special:Export, simpenen ing cakram padhet (''harddisk'') lan unggahna ing kéné.",
'importstart'                => 'Ngimpor kaca...',
'import-revision-count'      => '$1 {{PLURAL:$1|révisi|révisi-révisi}}',
'importnopages'              => 'Ora ana kaca kanggo diimpor.',
'importfailed'               => 'Impor gagal: $1',
'importunknownsource'        => 'Sumber impor ora ditepungi',
'importcantopen'             => 'Berkas impor ora bisa dibukak',
'importbadinterwiki'         => 'Pranala interwiki rusak',
'importnotext'               => 'Kosong utawa ora ana tèks',
'importsuccess'              => 'Impor suksès!',
'importhistoryconflict'      => 'Ana konflik révisi sajarah (mbok-menawa tau ngimpor kaca iki sadurungé)',
'importnosources'            => 'Ora ana sumber impor transwiki sing wis digawé lan pangunggahan sajarah sacara langsung wis dinon-aktifaké.',
'importnofile'               => 'Ora ana berkas sumber impor sing wis diunggahaké.',
'importuploaderrorsize'      => 'Pangunggahan berkas impor gagal. Ukuran berkas ngluwihi ukuran sing diidinaké.',
'importuploaderrorpartial'   => 'Pangunggahan berkas impor gagal. Namung sabagéyan berkas sing kasil bisa diunggahaké.',
'importuploaderrortemp'      => 'Pangunggahan berkas gagal. Sawijining dirèktori sauntara sing dibutuhaké ora ana.',
'import-parse-failure'       => 'Prosès impor XML gagal',
'import-noarticle'           => 'Ora ana kaca sing bisa diimpor!',
'import-nonewrevisions'      => 'Kabèh révisi sadurungé wis tau diimpor.',
'xml-error-string'           => '$1 ing baris $2, kolom $3 (bita $4): $5',

# Import log
'importlogpage'                    => 'Log impor',
'importlogpagetext'                => 'Impor administratif kaca-kaca mawa sajarah panyuntingan saka wiki liya.',
'import-logentry-upload'           => 'ngimpor [[$1]] mawa pangunggahan berkas',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|révisi|révisi}}',
'import-logentry-interwiki'        => 'wis nge-transwiki $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|révisi}} saka $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Kaca panganggoku',
'tooltip-pt-anonuserpage'         => 'Kaca panganggo IP panjenengan',
'tooltip-pt-mytalk'               => 'Kaca gunemanku',
'tooltip-pt-anontalk'             => 'Dhiskusi perkara suntingan saka alamat IP iki',
'tooltip-pt-preferences'          => 'Préferènsiku',
'tooltip-pt-watchlist'            => 'Daftar kaca sing tak-awasi.',
'tooltip-pt-mycontris'            => 'Daftar kontribusiku',
'tooltip-pt-login'                => 'Panjenengan diaturi mlebu log, nanging ora dikudokaké.',
'tooltip-pt-anonlogin'            => 'Panjenengan disaranaké mlebu log, nanging ora diwajibaké.',
'tooltip-pt-logout'               => 'Log metu (oncat)',
'tooltip-ca-talk'                 => 'Dhiskusi perkara isi',
'tooltip-ca-edit'                 => 'Sunting kaca iki. Nganggoa tombol pratayang sadurungé nyimpen.',
'tooltip-ca-addsection'           => 'Tambah komentar ing kaca dhiskusi iki.',
'tooltip-ca-viewsource'           => 'Kaca iki direksa. Panjenengan namung bisa mirsani sumberé.',
'tooltip-ca-history'              => 'Vèrsi-vèrsi sadurungé saka kaca iki.',
'tooltip-ca-protect'              => 'Reksa kaca iki',
'tooltip-ca-delete'               => 'Busak kaca iki',
'tooltip-ca-undelete'             => 'Balèkna suntingan ing kaca iki sadurungé kaca iki dibusak',
'tooltip-ca-move'                 => 'Pindhahen kaca iki',
'tooltip-ca-watch'                => 'Tambahna kaca iki ing daftar pangawasan panjenengan',
'tooltip-ca-unwatch'              => 'Busak kaca iki saka daftar pangawasan panjenengan',
'tooltip-search'                  => 'Golek ing situs {{SITENAME}} iki',
'tooltip-search-go'               => 'Lungaa ing kaca mawa jeneng persis iki, yèn anaa',
'tooltip-search-fulltext'         => 'Golèk kaca sing duwé tèks kaya mangkéné',
'tooltip-p-logo'                  => 'Kaca Utama',
'tooltip-n-mainpage'              => 'Nuwèni Kaca Utama',
'tooltip-n-portal'                => 'Perkara proyèk, apa sing bisa panjenengan gayuh, lan ing ngendi golèk apa-apa',
'tooltip-n-currentevents'         => 'Temokna informasi perkara prastawa anyar',
'tooltip-n-recentchanges'         => 'Daftar owah-owahan anyar ing wiki.',
'tooltip-n-randompage'            => 'Tuduhna sembarang kaca',
'tooltip-n-help'                  => 'Papan kanggo golèk pitulung.',
'tooltip-t-whatlinkshere'         => 'Daftar kabèh kaca wiki sing nyambung menyang kaca iki',
'tooltip-t-recentchangeslinked'   => 'Owah-owahan pungkasan kaca-kaca sing duwé pranala menyang kaca iki',
'tooltip-feed-rss'                => "''RSS feed'' kanggo kaca iki",
'tooltip-feed-atom'               => "''Atom feed'' kanggo kaca iki",
'tooltip-t-contributions'         => 'Deleng daftar kontribusi panganggo iki',
'tooltip-t-emailuser'             => 'Kirimna e-mail menyang panganggo iki',
'tooltip-t-upload'                => 'Ngunggah gambar utawa berkas média',
'tooltip-t-specialpages'          => 'Daftar kabèh kaca astaméwa (kaca kusus)',
'tooltip-t-print'                 => 'Vèrsi cithak kaca iki',
'tooltip-t-permalink'             => 'Pranala permanèn kanggo révisi kaca iki',
'tooltip-ca-nstab-main'           => 'Ndeleng kaca artikel',
'tooltip-ca-nstab-user'           => 'Deleng kaca panganggo',
'tooltip-ca-nstab-media'          => 'Ndeleng kaca média',
'tooltip-ca-nstab-special'        => 'Iki kaca astaméwa utawa kaca kusus sing ora bisa disunting',
'tooltip-ca-nstab-project'        => 'Deleng kaca proyèk',
'tooltip-ca-nstab-image'          => 'Deleng kaca berkas',
'tooltip-ca-nstab-mediawiki'      => 'Ndeleng pesenan sistém',
'tooltip-ca-nstab-template'       => 'Deleng cithakan',
'tooltip-ca-nstab-help'           => 'Mirsani kaca pitulung',
'tooltip-ca-nstab-category'       => 'Deleng kaca kategori',
'tooltip-minoredit'               => 'Tandhanana minangka suntingan cilik',
'tooltip-save'                    => 'Simpen owah-owahan panjenengan',
'tooltip-preview'                 => 'Pratayang owah-owahan panjenengan, tulung nganggo fungsi iki sadurungé nyimpen!',
'tooltip-diff'                    => 'Tuduhna owah-owahan panjenengan ing tèks iki.',
'tooltip-compareselectedversions' => 'Delengen prabédan antara rong vèrsi kaca iki sing dipilih.',
'tooltip-watch'                   => 'Tambahna kaca iki ing daftar pangawasan panjenengan',
'tooltip-recreate'                => 'Gawéa kaca iki manèh senadyan tau dibusak',
'tooltip-upload'                  => 'Miwiti pangunggahan',

# Metadata
'nodublincore'      => 'Metadata Dublin Core RDF dipatèni ing server iki.',
'nocreativecommons' => 'Metadata Creative Commons RDF dipatèni ing server iki.',
'notacceptable'     => 'Server wiki ora bisa nyedyakaké data sajroning format sing bisa diwaca déning klièn panjenengan.',

# Attribution
'anonymous'        => 'Panganggé {{SITENAME}} ingkang mboten kinawruhan.',
'siteuser'         => 'Panganggo {{SITENAME}} $1',
'lastmodifiedatby' => 'Kaca iki pungkasan diowahi  $2, $1 déning $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Adhedhasar karyané $1.',
'others'           => 'liya-liyané',
'siteusers'        => 'Panganggo(-panganggo) {{SITENAME}} $1',
'creditspage'      => 'Informasi para panulis kaca',
'nocredits'        => 'Ora ana informasi ngenani para panulis ing kaca iki.',

# Spam protection
'spamprotectiontitle' => 'Filter anti-spam',
'spamprotectiontext'  => 'Kaca sing arep disimpen panjenengan diblokir déning filter spam. 
Mbok-menawa iki disebabaké anané pranala jaba tartamtu.',
'spamprotectionmatch' => 'Tèks sing kapacak iki mancing filter spam kita: $1',
'spambot_username'    => 'Resik-resik spam MediaWiki',
'spam_reverting'      => 'Mbalèkaké menyang vèrsi pungkasan sing ora ana pranalané menyang $1',
'spam_blanking'       => 'Kabèh révisi sing duwé pranala menyang $1, pangosongan',

# Info page
'infosubtitle'   => 'Informasi kanggo kaca',
'numedits'       => 'Cacahé panyuntingan (artikel): $1',
'numtalkedits'   => 'Cacahé panyuntingan (kaca dhiskusi): $1',
'numwatchers'    => 'Cacahé sing ngawasi: $1',
'numauthors'     => 'Cacahé pangarang sing béda-béda (artikel): $1',
'numtalkauthors' => 'Cacahé pangarang sing béda-béda (kaca dhiskusi): $1',

# Math options
'mw_math_png'    => 'Mesthi nggawé PNG',
'mw_math_simple' => 'HTML yèn prasaja banget utawa yèn ora PNG',
'mw_math_html'   => 'HTML yèn bisa utawa PNG',
'mw_math_source' => 'Dijarna waé minangka TeX (kanggo panjlajah wèb tèks)',
'mw_math_modern' => 'Disaranaké kanggo panjlajah wèb modèrn',
'mw_math_mathml' => 'MathML yèn bisa (pracoban)',

# Patrolling
'markaspatrolleddiff'                 => 'Tandhanana wis dipatroli',
'markaspatrolledtext'                 => 'Tandhanana artikel iki wis dipatroli',
'markedaspatrolled'                   => 'Ditandhani wis dipatroli',
'markedaspatrolledtext'               => 'Révisi sing dipilih wis ditandhani minangka dipatroli.',
'rcpatroldisabled'                    => 'Patroli owah-owahan pungkasan dipatèni',
'rcpatroldisabledtext'                => 'Fitur patroli owah-owahan pungkasan lagi dipatèni.',
'markedaspatrollederror'              => 'Ora bisa awèh tandha wis dipatroli',
'markedaspatrollederrortext'          => 'Panjenengan kudu nentokaké sawijining révisi kanggo ditandhani minangka sing dipatroli.',
'markedaspatrollederror-noautopatrol' => 'Panjenengan ora pareng nandhani suntingan panjenengan dhéwé minangka dipatroli.',

# Patrol log
'patrol-log-page' => 'Log patroli',
'patrol-log-line' => 'nandhani $1 saka $2 sing dipatroli $3',
'patrol-log-auto' => '(otomatis)',

# Image deletion
'deletedrevision'                 => 'Revisi lawas sing dibusak $1.',
'filedeleteerror-short'           => 'Kaluputan nalika mbusak berkas: $1',
'filedeleteerror-long'            => 'Ana kaluputan nalika mbusak berkas:\\n\\n$1\\n',
'filedelete-missing'              => 'Berkas "$1" ora bisa dibusak amerga ora ditemokaké.',
'filedelete-old-unregistered'     => 'Révisi berkas "$1" sing diwènèhaké ora ana sajroning basis data.',
'filedelete-current-unregistered' => 'Berkas sing dispésifikasi "$1" ora ana sajroning basis data.',
'filedelete-archive-read-only'    => 'Dirèktori arsip "$1" ora bisa ditulis déning server wèb.',

# Browsing diffs
'previousdiff' => '← Panyuntingan sadurungé',
'nextdiff'     => 'Panyuntingan sing luwih anyar →',

# Media information
'mediawarning'         => "'''Pènget:''' Berkas iki mbokmenawa ngandhut kode sing bebayani, yèn dilakokaké sistém panjenengan bisa kena pangaruh ala.<hr />",
'imagemaxsize'         => 'Watesana ukuran gambar ing kaca dèskripsi berkas dadi:',
'thumbsize'            => 'Ukuran gambar cilik (thumbnail):',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|kaca|kaca}}',
'file-info'            => '(ukuran berkas: $1, tipe MIME: $2)',
'file-info-size'       => '($1 × $2 piksel, ukuran berkas: $3, tipe MIME: $4)',
'file-nohires'         => '<small>Ora ana résolusi sing luwih dhuwur.</small>',
'svg-long-desc'        => '(Berkas SVG, nominal $1 × $2 piksel, gedhené berkas: $3)',
'show-big-image'       => 'Résolusi kebak',
'show-big-image-thumb' => '<small>Ukuran pratayang iki: $1 × $2 piksel</small>',

# Special:NewImages
'newimages'             => 'Galeri berkas anyar',
'imagelisttext'         => "Ing ngisor iki kapacak daftar '''$1''' {{PLURAL:$1|berkas|berkas}} sing diurutaké $2.",
'newimages-summary'     => 'Kaca astaméwa utawa kusus iki nuduhaké daftar berkas anyar dhéwé sing diunggahaké.',
'showhidebots'          => '($1 bot)',
'noimages'              => 'Ora ana sing dideleng.',
'ilsubmit'              => 'Golek',
'bydate'                => 'miturut tanggal',
'sp-newimages-showfrom' => 'Tuduhna gambar anyar wiwit saka $2, $1',

# Bad image list
'bad_image_list' => "Formaté kaya mengkéné:

Namung butir daftar (baris sing diawali mawa tandha *) sing mèlu diitung. Pranala kapisan ing sawijining baris kudu pranala ing berkas sing ala. 
Pranala-pranala sabanjuré ing baris sing padha dianggep minangka ''pengecualian'', yaiku artikel sing bisa nuduhaké berkas iku.",

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => "Berkas iki ngandhut informasi tambahan sing mbokmenawa ditambahaké déning kamera digital utawa ''scanner'' sing dipigunakaké kanggo nggawé utawa olèhé digitalisasi berkas. Yèn berkas iki wis dimodifikasi, detail sing ana mbokmenawa ora sacara kebak nuduhaké informasi saka gambar sing wis dimodifikasi iki.",
'metadata-expand'   => 'Tuduhna detail tambahan',
'metadata-collapse' => 'Delikna detail tambahan',
'metadata-fields'   => 'Entri metadata EXIF sing kapacak iki bakal dituduhaké ing kaca informasi gambar yèn tabèl metadata didelikaké. Entri liyané minangka baku bakal didelikaké.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Jembar',
'exif-imagelength'                 => 'Dhuwur',
'exif-bitspersample'               => 'Bit per komponèn',
'exif-compression'                 => 'Skéma komprèsi',
'exif-photometricinterpretation'   => 'Komposisi piksel',
'exif-orientation'                 => 'Orièntasi',
'exif-samplesperpixel'             => 'Cacah komponèn',
'exif-planarconfiguration'         => 'Pangaturan data',
'exif-ycbcrsubsampling'            => 'Rasio subsampling Y ke C',
'exif-ycbcrpositioning'            => 'Pandokokan Y lan C',
'exif-xresolution'                 => 'Résolusi horisontal',
'exif-yresolution'                 => 'Résolusi vèrtikal',
'exif-resolutionunit'              => 'Unit résolusi X lan Y',
'exif-stripoffsets'                => 'Lokasi data gambar',
'exif-rowsperstrip'                => 'Cacah baris per strip',
'exif-stripbytecounts'             => 'Bita per strip komprèsi',
'exif-jpeginterchangeformat'       => 'Ofset menyang JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Bita data JPEG',
'exif-transferfunction'            => 'Fungsi transfer',
'exif-whitepoint'                  => 'Kromatisitas titik putih',
'exif-primarychromaticities'       => 'Kromatisitas werna primer',
'exif-ycbcrcoefficients'           => 'Koèfisièn matriks transformasi papan werna',
'exif-referenceblackwhite'         => 'Wiji réferènsi pasangan ireng putih',
'exif-datetime'                    => 'Tanggal lan wektu pangowahan berkas',
'exif-imagedescription'            => 'Judhul gambar',
'exif-make'                        => 'Produsèn kamera',
'exif-model'                       => 'Modhèl kamera',
'exif-software'                    => 'Perangkat lunak',
'exif-artist'                      => 'Prodhusèn',
'exif-copyright'                   => 'Sing ndarbèni hak cipta',
'exif-exifversion'                 => 'Vèrsi Exif',
'exif-flashpixversion'             => 'Dukungan versi Flashpix',
'exif-colorspace'                  => 'Papan werna',
'exif-componentsconfiguration'     => 'Teges saben komponèn',
'exif-compressedbitsperpixel'      => 'Modhe komprèsi gambar',
'exif-pixelydimension'             => 'Jembar gambar sing sah',
'exif-pixelxdimension'             => 'Dhuwur gambar sing sah',
'exif-makernote'                   => 'Cathetan prodhusèn',
'exif-usercomment'                 => 'Komentar panganggo',
'exif-relatedsoundfile'            => 'Berkas audio sing kagandhèng',
'exif-datetimeoriginal'            => 'Tanggal lan wektu nggawé data',
'exif-datetimedigitized'           => 'Tanggal lan wektu dhigitalisasi',
'exif-subsectime'                  => 'Subdetik DateTime',
'exif-subsectimeoriginal'          => 'Subdetik DateTimeOriginal',
'exif-subsectimedigitized'         => 'Subdetik DateTimeDigitized',
'exif-exposuretime'                => 'Wektu pajanan',
'exif-exposuretime-format'         => '$1 detik ($2)',
'exif-fnumber'                     => 'Wiji F',
'exif-exposureprogram'             => 'Program pajanan',
'exif-spectralsensitivity'         => 'Sènsitivitas spèktral',
'exif-isospeedratings'             => 'Rating kacepetan ISO',
'exif-oecf'                        => 'Faktor konvèrsi optoélèktronik',
'exif-shutterspeedvalue'           => 'Kacepatan rana',
'exif-aperturevalue'               => 'Bukaan',
'exif-brightnessvalue'             => 'Kacerahan',
'exif-exposurebiasvalue'           => 'Bias pajanan',
'exif-maxaperturevalue'            => 'Bukaan tanah maksimum',
'exif-subjectdistance'             => 'Jarak subjèk',
'exif-meteringmode'                => 'Modhe pangukuran',
'exif-lightsource'                 => 'Sumber cahya',
'exif-flash'                       => 'Kilas',
'exif-focallength'                 => 'Jarak fokus lènsa',
'exif-subjectarea'                 => 'Wilayah subjèk',
'exif-flashenergy'                 => 'Énèrgi kilas',
'exif-spatialfrequencyresponse'    => 'Respons frekwènsi spasial',
'exif-focalplanexresolution'       => 'Résolusi bidang fokus X',
'exif-focalplaneyresolution'       => 'Résolusi bidang fokus Y',
'exif-focalplaneresolutionunit'    => 'Unit résolusi bidang fokus',
'exif-subjectlocation'             => 'Lokasi subjèk',
'exif-exposureindex'               => 'Indhèks pajanan',
'exif-sensingmethod'               => 'Métodhe pangindran',
'exif-filesource'                  => 'Sumber berkas',
'exif-scenetype'                   => 'Tipe panyawangan',
'exif-cfapattern'                  => 'Pola CFA',
'exif-customrendered'              => 'Prosès nggawé gambar',
'exif-exposuremode'                => 'Modhe pajanan',
'exif-whitebalance'                => 'Kaseimbangan putih',
'exif-digitalzoomratio'            => 'Rasio pambesaran digital',
'exif-focallengthin35mmfilm'       => 'Dhawa fokus ing fil 35 mm',
'exif-scenecapturetype'            => 'Tipe panangkepan',
'exif-gaincontrol'                 => 'Kontrol panyawangan',
'exif-contrast'                    => 'Kontras',
'exif-saturation'                  => 'Saturasi',
'exif-sharpness'                   => 'Kalandhepan',
'exif-devicesettingdescription'    => 'Dhèskripsi pangaturan piranti',
'exif-subjectdistancerange'        => 'Jarak subjèk',
'exif-imageuniqueid'               => 'ID unik gambar',
'exif-gpsversionid'                => 'Vèrsi tag GPS',
'exif-gpslatituderef'              => 'Lintang Lor utawa Kidul',
'exif-gpslatitude'                 => 'Lintang',
'exif-gpslongituderef'             => 'Bujur Wétan utawa Kulon',
'exif-gpslongitude'                => 'Bujur',
'exif-gpsaltituderef'              => 'Réferènsi dhuwur',
'exif-gpsaltitude'                 => 'Dhuwuré',
'exif-gpstimestamp'                => 'Wektu GPS (jam atom)',
'exif-gpssatellites'               => 'Satelit kanggo pangukuran',
'exif-gpsstatus'                   => 'Status panrima',
'exif-gpsmeasuremode'              => 'Modhe pangukuran',
'exif-gpsdop'                      => 'Katepatan pangukuran',
'exif-gpsspeedref'                 => 'Unit kacepetan',
'exif-gpsspeed'                    => 'Kacepetan panrima GPS',
'exif-gpstrackref'                 => 'Réferènsi arah obah',
'exif-gpstrack'                    => 'Arah obah',
'exif-gpsimgdirectionref'          => 'Réferènsi arah gambar',
'exif-gpsimgdirection'             => 'Arah gambar',
'exif-gpsmapdatum'                 => 'Data survéi géodèsi',
'exif-gpsdestlatituderef'          => 'Réferènsi lintang saka patujon',
'exif-gpsdestlatitude'             => 'Lintang tujuan',
'exif-gpsdestlongituderef'         => 'Réferènsi bujur saka patujon',
'exif-gpsdestlongitude'            => 'Bujur tujuan',
'exif-gpsdestbearingref'           => 'Réferènsi bearing of destination',
'exif-gpsdestbearing'              => 'Arah tujuan',
'exif-gpsdestdistanceref'          => 'Réferènsi jarak saka patujon',
'exif-gpsdestdistance'             => 'Jarak saka patujon',
'exif-gpsprocessingmethod'         => 'Jeneng métodhe prosès GPS',
'exif-gpsareainformation'          => 'Jeneng wilayah GPS',
'exif-gpsdatestamp'                => 'Tanggal GPS',
'exif-gpsdifferential'             => 'Korèksi diférènsial GPS',

# EXIF attributes
'exif-compression-1' => 'Ora dikomprèsi',

'exif-unknowndate' => 'Tanggal ora dingertèni',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Baliken sacara horisontal', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Diputer 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Baliken sacara vèrtikal', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Diputer 90° nglawan arah dom jam dan dibalik sacara vèrtikal', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Diputer 90° miturut arah dom jam', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Diputer 90° miturut arah dom jam lan diwalik sacara vèrtikal', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Diputer 90° miturut lawan arah dom jam', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => "format ''chunky'' (kumothak)",
'exif-planarconfiguration-2' => 'format planar',

'exif-componentsconfiguration-0' => 'ora ana',

'exif-exposureprogram-0' => 'Ora didéfinisi',
'exif-exposureprogram-1' => 'Mawa tangan (manual)',
'exif-exposureprogram-2' => 'Program normal',
'exif-exposureprogram-3' => 'Prioritas diafragma',
'exif-exposureprogram-4' => 'Prioritas panutup',
'exif-exposureprogram-5' => "Program kréatif (condong menyang jroning bilik (''depth of field''))",
'exif-exposureprogram-6' => 'Program aksi (condhong marang kacepetan rana)',
'exif-exposureprogram-7' => "Modus potret (kanggo foto ''closeup'' mawa latar wuri ora fokus)",
'exif-exposureprogram-8' => "Modus pamandhangan (''landscape'') (kanggo foto pamandhangan mawa latar wuri fokus)",

'exif-subjectdistance-value' => '$1 mèter',

'exif-meteringmode-0'   => 'Ora dingertèni',
'exif-meteringmode-1'   => 'Rata-rata',
'exif-meteringmode-2'   => 'Rata-rataAbobot',
'exif-meteringmode-3'   => 'Spot',
'exif-meteringmode-4'   => 'MultiSpot',
'exif-meteringmode-5'   => 'Pola utawa patron multi-sègmèn',
'exif-meteringmode-6'   => 'Parsial (sabagéyan)',
'exif-meteringmode-255' => 'Liya-liyané',

'exif-lightsource-0'   => 'Ora dingertèni',
'exif-lightsource-1'   => 'Cahya srengéngé',
'exif-lightsource-2'   => 'Cahya néon',
'exif-lightsource-3'   => 'Wolfram (cahya pijer)',
'exif-lightsource-4'   => 'Blitz',
'exif-lightsource-9'   => 'Hawa apik',
'exif-lightsource-10'  => 'Hawa apedhut',
'exif-lightsource-11'  => 'Bayangan',
'exif-lightsource-12'  => 'Fluorescent cahya pepadhang awan (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluorescent putih pepadhang awan (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluorescent putih éyup (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Fluorescent putih (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Cahya standar A',
'exif-lightsource-18'  => 'Cahya standar B',
'exif-lightsource-19'  => 'Cahya standar C',
'exif-lightsource-24'  => 'ISO studio tungsten',
'exif-lightsource-255' => 'Sumber cahya liya',

'exif-focalplaneresolutionunit-2' => 'inci',

'exif-sensingmethod-1' => 'Ora didéfinisi',
'exif-sensingmethod-2' => 'Sènsor aréa werna sa-tugelan',
'exif-sensingmethod-3' => 'Sènsor aréa werna rong tugelan',
'exif-sensingmethod-4' => 'Sènsor aréa werna telung tugelan',
'exif-sensingmethod-5' => 'Sènsor aréa werna urut-urutan',
'exif-sensingmethod-7' => 'Sènsor trilinéar',
'exif-sensingmethod-8' => 'Sènsor linéar werna urut-urutan',

'exif-scenetype-1' => 'Gambar foto langsung',

'exif-customrendered-0' => 'Prosès normal',
'exif-customrendered-1' => 'Prosès kustom',

'exif-exposuremode-0' => 'Pajanan (èkspos) otomatis',
'exif-exposuremode-1' => 'Pajanan (èkspos) manual',
'exif-exposuremode-2' => 'Brakèt otomatis',

'exif-whitebalance-0' => "Kababagan (''kasaimbangan'') putih otomatis",
'exif-whitebalance-1' => 'Kababagan (kasaimbangan) putih manual',

'exif-scenecapturetype-0' => 'Standar',
'exif-scenecapturetype-1' => "Dawa (''landscape'')",
'exif-scenecapturetype-2' => 'Potrèt',
'exif-scenecapturetype-3' => 'Pamandhangan wengi',

'exif-gaincontrol-0' => 'Ora ana',
'exif-gaincontrol-1' => 'Puncak-puncak ngisor munggah',
'exif-gaincontrol-2' => 'Puncak-puncak dhuwur munggah',
'exif-gaincontrol-3' => 'Puncak-puncak ngisor medhun',
'exif-gaincontrol-4' => 'Puncak-puncak dhuwur medhun',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Lembut',
'exif-contrast-2' => 'Atos',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Saturasi ngisor',
'exif-saturation-2' => 'Saturasi dhuwur',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Lembut',
'exif-sharpness-2' => 'Atos',

'exif-subjectdistancerange-0' => 'Ora dimangertèni',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Katon cedhak',
'exif-subjectdistancerange-3' => 'Katon adoh',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Lintang lor',
'exif-gpslatitude-s' => 'Lintang kidul',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Bujur wétan',
'exif-gpslongitude-w' => 'Bujur kulon',

'exif-gpsstatus-a' => 'Pangukuran lagi dilakoni',
'exif-gpsstatus-v' => 'Interoperabilitas pangukuran',

'exif-gpsmeasuremode-2' => 'Pangukuran 2-dimènsi',
'exif-gpsmeasuremode-3' => 'Pangukuran 3-dimènsi',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilométer per jam',
'exif-gpsspeed-m' => 'Mil per jam',
'exif-gpsspeed-n' => 'Knot',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Arah sejati',
'exif-gpsdirection-m' => 'Arah magnètis',

# External editor support
'edit-externally'      => 'Sunting berkas iki mawa aplikasi jaba',
'edit-externally-help' => 'Deleng [http://www.mediawiki.org/wiki/Manual:External_editors instruksi pangaturan] kanggo informasi sabanjuré.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'kabèh',
'imagelistall'     => 'kabèh',
'watchlistall2'    => 'kabèh',
'namespacesall'    => 'kabèh',
'monthsall'        => 'kabèh',

# E-mail address confirmation
'confirmemail'             => 'Konfirmasi alamat e-mail',
'confirmemail_noemail'     => 'Panjenengan ora maringi alamat e-mail sing absah ing [[Special:Preferences|préferènsi]] panjenengan.',
'confirmemail_text'        => '{{SITENAME}} ngwajibaké panjenengan ndhedhes utawa konfirmasi alamat e-mail panjenengan sadurungé bisa nganggo fitur-fitur e-mail.
Pencèten tombol ing ngisor iki kanggo ngirim sawijining kode konfirmasi arupa sawijining pranala;
Tuladen pranala iki ing panjlajah wèb panjenengan kanggo ndhedhes yèn alamat e-mail panjenengan pancèn bener.',
'confirmemail_pending'     => '<div class="error">Sawijining kode konfirmasi wis dikirim menyang alamat e-mail panjenengan;
yèn panjenengan lagi waé nggawé akun utawa rékening panjenengan, mangga nunggu sawetara menit nganti layang iku tekan sadurungé nyuwun kode anyar manèh.</div>',
'confirmemail_send'        => 'Kirim kode konfirmasi',
'confirmemail_sent'        => 'E-mail mawa kode konfirmasi wis dikirim.',
'confirmemail_oncreate'    => 'Sawijining kode pandhedhesan (konfirmasi) wis dikirim menyang alamat e-mail panjenengan.
Kode iki ora dibutuhaké kanggo log mlebu, nanging dibutuhaké sadurungé nganggo kabèh fitur sing nganggo e-mail ing wiki iki.',
'confirmemail_sendfailed'  => 'Layang e-mail konfirmasi ora kasil dikirim. 
Mangga dipriksa mbok-menawa ana aksara ilegal ing alamat e-mail panjenengan. 

Pangirim mènèhi informasi: $1',
'confirmemail_invalid'     => 'Kode konfirmasi salah. Kode iku mbok-menawa wis kadaluwarsa.',
'confirmemail_needlogin'   => 'Panjenengan kudu ndhedhes (konfirmasi) $1 alamat layang e-mail panjenengan.',
'confirmemail_success'     => 'Alamat e-mail panjenengan wis dikonfirmasi.
Saiki panjenengan bisa log mlebu lan wiwit nganggo wiki.',
'confirmemail_loggedin'    => 'Alamat e-mail panjenengan wis dikonfirmasi.',
'confirmemail_error'       => 'Ana kaluputan nalika nyimpen konfirmasi panjenengan.',
'confirmemail_subject'     => 'Konfirmasi alamat e-mail {{SITENAME}}',
'confirmemail_body'        => 'Sawijining wong, mbokmenawa panjenengan dhéwé, saka alamat IP $1, wis ndaftaraké akun "$2" mawa alamat e-mail iki ing {{SITENAME}}. Bukaka pranala iki ing panjlajah wèb panjenengan.

$3

Yèn panjenengan *ora tau* ndaftar akun iki, tutna pranala ing ngisor iki kanggo mbatalaké konfirmasi alamat e-mail:

$5

Konfirmasi iki bakal kadaluwarsa ing $4.',
'confirmemail_invalidated' => 'Pandhedhesan (konfirmasi) alamat e-mail batal',
'invalidateemail'          => 'Batalna pandhedhesan (konfirmasi) e-mail',

# Scary transclusion
'scarytranscludedisabled' => '[Transklusi cithakan interwiki dipatèni]',
'scarytranscludefailed'   => '[Olèhé njupuk cithakan $1 gagal; nuwun sèwu]',
'scarytranscludetoolong'  => '[URL-é kedawan; nuwun sèwu]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Ngrunut bali kanggo artikel iki:<br />
$1
</div>',
'trackbackremove'   => '([$1 Busak])',
'trackbacklink'     => 'Lacak balik',
'trackbackdeleteok' => 'Pelacakan balik bisa dibusak.',

# Delete conflict
'deletedwhileediting' => 'Wara-wara: Kaca punika sampun kabusak sasampunipun panjenengan miwiti nyunting!',
'confirmrecreate'     => "Panganggo [[User:$1|$1]] ([[User talk:$1|Wicara]]) wis mbusak kaca iki nalika panjenengan miwiti panyuntingan mawa alesan:
: ''$2''
Mangga didhedhes (dikonfirmasi) menawa panjenengan kersa nggawé ulang kaca iki.",
'recreate'            => 'Gawé ulang',

# HTML dump
'redirectingto' => 'Dipun-alihaken tumuju [[:$1]]...',

# action=purge
'confirm_purge'        => "Busak ''cache'' kaca iki?$1",
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Golèk artikel sing ngamot ''$1''.",
'searchnamed'      => "Golèk artikel sing ajudhul ''$1''.",
'articletitles'    => "Artikel sing diawali ''$1''",
'hideresults'      => 'Delikna pituwas',
'useajaxsearch'    => 'Nganggoa panggolèkan AJAX',

# Multipage image navigation
'imgmultipageprev' => '&larr; kaca sadurungé',
'imgmultipagenext' => 'kaca sabanjuré →',
'imgmultigo'       => 'Golèk!',
'imgmultigoto'     => 'Lungaa menyang kaca $1',

# Table pager
'ascending_abbrev'         => 'unggah',
'descending_abbrev'        => 'mudhun',
'table_pager_next'         => 'Kaca sabanjuré',
'table_pager_prev'         => 'Kaca sadurungé',
'table_pager_first'        => 'Kaca kapisan',
'table_pager_last'         => 'Kaca pungkasan',
'table_pager_limit'        => 'Tuduhna $1 entri per kaca',
'table_pager_limit_submit' => 'Golèk',
'table_pager_empty'        => 'Ora ditemokaké',

# Auto-summaries
'autosumm-blank'   => '←Ngosongaké kaca',
'autosumm-replace' => "←Ngganti kaca karo '$1'",
'autoredircomment' => '←Ngalihaké menyang [[$1]]',
'autosumm-new'     => "←Nggawé kaca sing isiné '$1'",

# Live preview
'livepreview-loading' => 'Ngunggahaké…',
'livepreview-ready'   => 'Ngunggahaké… Rampung!',
'livepreview-failed'  => 'Pratayang langsung gagal! Coba karo pratayang normal.',
'livepreview-error'   => 'Gagal nyambung: $1 "$2"
Cobanen mawa pratayang normal.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Owah-owahan pungkasan sing luwih anyar tinimbang $1 detik mbokmenawa ora muncul ing daftar iki.',
'lag-warn-high'   => "Amerga gedhéné ''lag'' basis data server, owah-owahan pungkasan sing luwih anyar saka $1 detik mbokmenawa ora muncul ing daftar iki.",

# Watchlist editor
'watchlistedit-numitems'       => 'Daftar pangawasan panjenengan ngandhut {{PLURAL:$1|1 irah-irahan|$1 irah-irahan}}, ora kalebu kaca-kaca dhiskusi.',
'watchlistedit-noitems'        => 'Daftar pangawasan panjenengan kosong.',
'watchlistedit-normal-title'   => 'Sunting daftar pangawasan',
'watchlistedit-normal-legend'  => 'Busak irah-irahan saka daftar pangawasan',
'watchlistedit-normal-explain' => 'Irah-irahan utawa judhul ing daftar pangawasan panjenengan kapacak ing ngisor iki. 
Kanggo mbusak sawijining irah-irahan, kliken kothak ing pinggiré, lan banjur kliken "Busak judhul". 
Panjenengan uga bisa [[Special:Watchlist/raw|nyunting daftar mentah]].',
'watchlistedit-normal-submit'  => 'Busak irah-irahan',
'watchlistedit-normal-done'    => 'Irah-irahan {{PLURAL:$1|siji|$1}} wis dibusak saka daftar pangawasan panjenengan:',
'watchlistedit-raw-title'      => 'Sunting daftar mentah',
'watchlistedit-raw-legend'     => 'Sunting daftar mentah',
'watchlistedit-raw-explain'    => 'Irah-irahan ing daftar pangawasan panjenengan kapacak ing ngisor iki, lan bisa diowahi mawa nambahaké utawa mbusak daftar; sairah-irahan saban barisé. 
Yèn wis rampung, anyarana kaca daftar pangawasan iki. 
Panjenengan uga bisa [[Special:Watchlist/edit|nganggo éditor standar panjenengan]].',
'watchlistedit-raw-titles'     => 'Irah-irahan:',
'watchlistedit-raw-submit'     => 'Anyarana daftar pangawasan',
'watchlistedit-raw-done'       => 'Daftar pangawasan panjenengan wis dianyari.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 irah-irahan wis|$1 irah-irahan wis}} ditambahaké:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 irah-irahan wis|$1 irah-irahan wis}} diwetokaké:',

# Watchlist editing tools
'watchlisttools-view' => 'Tuduhna owah-owahan sing ana gandhèngané',
'watchlisttools-edit' => 'Tuduhna lan sunting daftar pangawasan',
'watchlisttools-raw'  => 'Sunting daftar pangawasan mentah',

# Core parser functions
'unknown_extension_tag' => 'Tag èkstènsi ora ditepungi "$1"',

# Special:Version
'version'                          => 'Versi', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Èkstènsi sing wis diinstalasi',
'version-specialpages'             => 'Kaca astaméwa (kaca kusus)',
'version-parserhooks'              => 'Canthèlan parser',
'version-variables'                => 'Variabel',
'version-other'                    => 'Liyané',
'version-mediahandlers'            => 'Pananganan média',
'version-hooks'                    => 'Canthèlan-canthèlan',
'version-extension-functions'      => 'Fungsi-fungsi èkstènsi',
'version-parser-extensiontags'     => 'Rambu èkstènsi parser',
'version-parser-function-hooks'    => 'Canthèlan fungsi parser',
'version-skin-extension-functions' => 'Fungsi èkstènsi kulit',
'version-hook-name'                => 'Jeneng canthèlan',
'version-hook-subscribedby'        => 'Dilanggani déning',
'version-version'                  => 'Vèrsi',
'version-license'                  => 'Lisènsi',
'version-software'                 => "''Software'' wis diinstalasi",
'version-software-product'         => 'Prodhuk',
'version-software-version'         => 'Vèrsi',

# Special:FilePath
'filepath'         => 'Lokasi berkas',
'filepath-page'    => 'Berkas:',
'filepath-submit'  => 'Lokasi',
'filepath-summary' => 'Kaca astaméwa utawa kusus iki nuduhaké jalur pepak sawijining berkas.
Gambar dituduhaké mawa résolusi kebak lan tipe liyané berkas bakal dibuka langsung mawa program kagandhèng.

Lebokna jeneng berkas tanpa imbuhan awalan "{{ns:image}}:".',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Golèk berkas duplikat',
'fileduplicatesearch-summary'  => 'Golèk duplikat berkas adhedhasar biji hash-é.

Lebokna jeneng berkas tanpa imbuhan awal "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'Golèk duplikat',
'fileduplicatesearch-filename' => 'Jeneng berkas:',
'fileduplicatesearch-submit'   => 'Golèk',
'fileduplicatesearch-info'     => '$1 × $2 piksel<br />Ukuran berkas: $3<br />Tipe MIME: $4',
'fileduplicatesearch-result-1' => 'Berkas "$1" ora duwé duplikat idèntik.',
'fileduplicatesearch-result-n' => 'Berkas "$1" ora ndarbèni {{PLURAL:$2|1 duplikat idèntik|$2 duplikat idèntik}}.',

# Special:SpecialPages
'specialpages'                   => 'Kaca astaméwa',
'specialpages-group-maintenance' => 'Lapuran pangopènan',
'specialpages-group-other'       => 'Kaca-kaca astaméwa liyané',
'specialpages-group-login'       => 'Mlebu log / ndaftar',
'specialpages-group-changes'     => 'Owah-owahan pungkasan lan log',
'specialpages-group-media'       => 'Lapuran média lan pangunggahan',
'specialpages-group-users'       => 'Panganggo lan hak-haké',
'specialpages-group-highuse'     => 'Kaca-kaca sing akèh dienggo',

# Special:BlankPage
'blankpage' => 'Kaca kosong',

);
