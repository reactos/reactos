<?php
/** Kara-Kalpak (Qaraqalpaqsha)
 *
 * @ingroup Language
 * @file
 *
 * @author AlefZet
 * @author Atabek
 * @author Jiemurat
 */

$fallback = 'kk-latn';

$separatorTransformTable = array(
	',' => "\xc2\xa0",
	'.' => ',',
);

$fallback8bitEncoding = 'windows-1254';

$linkPrefixExtension = true;

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Arnawlı',
	NS_MAIN           => '',
	NS_TALK           => "Sa'wbet",
	NS_USER           => 'Paydalanıwshı',
	NS_USER_TALK      => "Paydalanıwshı_sa'wbeti",
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => "$1_sa'wbeti",
	NS_IMAGE          => "Su'wret",
	NS_IMAGE_TALK     => "Su'wret_sa'wbeti",
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => "MediaWiki_sa'wbeti",
	NS_TEMPLATE       => 'Shablon',
	NS_TEMPLATE_TALK  => "Shablon_sa'wbeti",
	NS_HELP           => 'Anıqlama',
	NS_HELP_TALK      => "Anıqlama_sa'wbeti",
	NS_CATEGORY       => 'Kategoriya',
	NS_CATEGORY_TALK  => "Kategoriya_sa'wbeti",
);

$datePreferences = array(
	'default',
	'mdy',
	'dmy',
	'ymd',
	'yyyy-mm-dd',
	'ISO 8601',
);

$defaultDateFormat = 'ymd';

$datePreferenceMigrationMap = array(
	'default',
	'mdy',
	'dmy',
	'ymd'
);

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'xg j, Y "j."',
	'mdy both' => 'H:i, xg j, Y "j."',

	'dmy time' => 'H:i',
	'dmy date' => 'j F, Y "j."',
	'dmy both' => 'H:i, j F, Y "j."',

	'ymd time' => 'H:i',
	'ymd date' => 'Y "j." xg j',
	'ymd both' => 'H:i, Y "j." xg j',

	'yyyy-mm-dd time' => 'xnH:xni:xns',
	'yyyy-mm-dd date' => 'xnY-xnm-xnd',
	'yyyy-mm-dd both' => 'xnH:xni:xns, xnY-xnm-xnd',

	'ISO 8601 time' => 'xnH:xni:xns',
	'ISO 8601 date' => 'xnY-xnm-xnd',
	'ISO 8601 both' => 'xnY-xnm-xnd"T"xnH:xni:xns',
);

$linkTrail = "/^([a-zı'ʼ’“»]+)(.*)$/sDu";

$messages = array(
# User preference toggles
'tog-underline'               => "Siltewdin' astın sız:",
'tog-highlightbroken'         => 'Jaramsız siltewlerdi <a href="" class="new">usılay</a> tuwrıla (alternativ: usınday<a href="" class="internal">?</a>).',
'tog-justify'                 => "Tekstti bettin' ken'ligi boyınsha tuwrılaw",
'tog-hideminor'               => "Aqırg'ı o'zgerislerden kishilerin jasır",
'tog-extendwatchlist'         => "Baqlaw dizimin ken'eyt (ha'mme jaramlı o'zgerislerdi ko'rset)",
'tog-usenewrc'                => "Ken'eytilgen jaqındag'ı o'zgerisler (JavaScript)",
'tog-numberheadings'          => 'Atamalardı avtomat nomerle',
'tog-showtoolbar'             => "O'zgertiw a'sbapların ko'rset (JavaScript)",
'tog-editondblclick'          => "Eki ma'rte basıp o'zgertiw (JavaScript)",
'tog-editsection'             => "Bo'limlerdi [o'zgertiw] siltew arqalı o'zgertiwdi qos",
'tog-editsectiononrightclick' => "Bo'lim atamasın on' jaqqa basıp o'zgertiwdi qos (JavaScript)",
'tog-showtoc'                 => "Mazmunın ko'rset (3-ten artıq bo'limi bar betlerge)",
'tog-rememberpassword'        => "Menin' kirgenimdi usı kompyuterde saqlap qal",
'tog-editwidth'               => "O'zgertiw maydanının' eni tolıq",
'tog-watchcreations'          => 'Men jaratqan betlerdi baqlaw dizimime qos',
'tog-watchdefault'            => "Men o'zgeris kiritken betlerdi baqlaw dizimime qos",
'tog-watchmoves'              => "Men ko'shirgen betlerdi baqlaw dizimime qos",
'tog-watchdeletion'           => "Men o'shirgen betlerdi baqlaw dizimime qos",
'tog-minordefault'            => "Defolt boyınsha barlıq o'zgerislerdi kishi dep esaplaw",
'tog-previewontop'            => "O'zgertiw maydanınan aldın ko'rip shıg'ıw maydanın ko'rset",
'tog-previewonfirst'          => "Birinshi o'zgertiwdi ko'rip shıq",
'tog-nocache'                 => "Betti keshte saqlawdı o'shir",
'tog-enotifwatchlistpages'    => "Baqlaw dizimimdegi bet o'zgertilgende mag'an xat jiber",
'tog-enotifusertalkpages'     => "Menin' sa'wbetim o'zgertilgende mag'an xat jiber",
'tog-enotifminoredits'        => "Kishi o'zgerisler haqqında da mag'an xat jiber",
'tog-enotifrevealaddr'        => "Eskertiw xatlarında e-mail adresimdi ko'rset",
'tog-shownumberswatching'     => "Baqlag'an paydalanıwshılar sanın ko'rset",
'tog-fancysig'                => 'Shala imzalar (avtomat siltewsiz)',
'tog-externaleditor'          => "Defolt boyınsha sırtqı o'zgertiwshini qollan",
'tog-externaldiff'            => 'Defoltta sırtqı parqtı qollan',
'tog-showjumplinks'           => "«O'tip ketiw» siltewlerin qos",
'tog-uselivepreview'          => "Janlı ko'rip shıg'ıwdı qollan (JavaScript) (Sınawda)",
'tog-forceeditsummary'        => "O'zgertiw juwmag'ı bos qalg'anda mag'an eskert",
'tog-watchlisthideown'        => "Baqlaw dizimindegi menin' o'zgertiwlerimdi jasır",
'tog-watchlisthidebots'       => "Baqlaw dizimindegi bot o'zgertiwlerin jasır",
'tog-watchlisthideminor'      => "Baqlaw diziminen kishi o'zgerislerdi jasır",
'tog-ccmeonemails'            => "Basqa qollanıwshılarg'a jibergen xatlarımnın' ko'shirmesin mag'an da jiber",
'tog-diffonly'                => "Bet mag'lıwmatın parqlardan to'mengi jerde ko'rsetpe",
'tog-showhiddencats'          => "Jasırın kategoriyalardı ko'rset",

'underline-always'  => "Ha'r dayım",
'underline-never'   => 'Hesh qashan',
'underline-default' => "Brawzerdin' sazlawları boyınsha",

'skinpreview' => '(Korip al)',

# Dates
'sunday'        => 'Ekshenbi',
'monday'        => "Du'yshenbi",
'tuesday'       => 'Siyshenbi',
'wednesday'     => "Sa'rshenbi",
'thursday'      => 'Piyshenbi',
'friday'        => 'Juma',
'saturday'      => 'Shenbi',
'sun'           => 'Eks',
'mon'           => 'Dsh',
'tue'           => 'Ssh',
'wed'           => "Sa'r",
'thu'           => 'Psh',
'fri'           => 'Jum',
'sat'           => 'Shn',
'january'       => 'Yanvar',
'february'      => 'Fevral',
'march'         => 'Mart',
'april'         => 'Aprel',
'may_long'      => 'May',
'june'          => 'İyun',
'july'          => 'İyul',
'august'        => 'Avgust',
'september'     => 'Sentyabr',
'october'       => 'Oktyabr',
'november'      => 'Noyabr',
'december'      => 'Dekabr',
'january-gen'   => "yanvardın'",
'february-gen'  => "fevraldın'",
'march-gen'     => "marttın'",
'april-gen'     => "apreldin'",
'may-gen'       => "maydın'",
'june-gen'      => "iyunnin'",
'july-gen'      => "iyuldin'",
'august-gen'    => "avgusttın'",
'september-gen' => "sentyabrdin'",
'october-gen'   => "oktyabrdin'",
'november-gen'  => "noyabrdin'",
'december-gen'  => "dekabrdin'",
'jan'           => 'Yan',
'feb'           => 'Fev',
'mar'           => 'Mar',
'apr'           => 'Apr',
'may'           => 'May',
'jun'           => 'İun',
'jul'           => 'İul',
'aug'           => 'Avg',
'sep'           => 'Sen',
'oct'           => 'Okt',
'nov'           => 'Noy',
'dec'           => 'Dek',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategoriya|Kategoriyalar}}',
'category_header'                => '"$1" kategoriyasindag\'ı betler',
'subcategories'                  => 'Podkategoriyalar',
'category-media-header'          => '"$1" kategoriyasindag\'ı media',
'category-empty'                 => "''Bul kategoriyada ha'zir hesh bet yamasa media joq''",
'hidden-categories'              => '{{PLURAL:$1|Jasırın kategoriya|Jasırın kategoriyalar}}',
'hidden-category-category'       => 'Jasırın kategoriyalar', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$2|Bul kategoriyada tek to'mendegi podkategoriya bar.|Bul kategoriyada $1 podkategoriya bar (barlıg'ı $2).}}",
'category-subcat-count-limited'  => "Bul kategoriyada to'mendegi {{PLURAL:$1|podkategoriya|$1 podkategoriyalar}} bar.",
'category-article-count'         => "{{PLURAL:$2|Bul kategoriyada tek to'mendegi bet bar.|Bul kategoriyada to'mendegi $1 bet bar (barlıg'ı $2).}}",
'category-article-count-limited' => "Usı kategoriyada to'mendegi $1 bet bar.",
'category-file-count'            => "{{PLURAL:$2|Bul kategoriyada tek to'mendegi fayl bar.|Bul kategoriyada to'mendegi $1 fayl bar (barlıg'ı $2).}}",
'category-file-count-limited'    => "Usı kategoriyada to'mendegi $1 fayl bar.",
'listingcontinuesabbrev'         => 'dawamı',

'linkprefix'        => '/^(.*?)([a-zıA-Zİ\\x80-\\xff]+)$/sDu',
'mainpagetext'      => "<big>'''MediaWiki tabıslı ornatıldı.'''</big>",
'mainpagedocfooter' => "Wiki bag'darlamasın qollanıw haqqındag'i mag'lıwmat usın [http://meta.wikimedia.org/wiki/Help:Contents Paydalanıwshılar qollanbasınan] ken'es alın'.

== Baslaw ushın ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Konfiguratsiya sazlaw dizimi]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWikidin' Ko'p Soralatug'ın Sorawları]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki haqqında xat tarqatıw dizimi]",

'about'          => 'Proyekt haqqında',
'article'        => "Mag'lıwmat beti",
'newwindow'      => "(jan'a aynada)",
'cancel'         => 'Biykar etiw',
'qbfind'         => 'Tabıw',
'qbbrowse'       => "Ko'riw",
'qbedit'         => "O'zgertiw",
'qbpageoptions'  => 'Usı bet',
'qbpageinfo'     => 'Kontekst',
'qbmyoptions'    => "Menin' betlerim",
'qbspecialpages' => 'Arnawlı betler',
'moredotdotdot'  => "Ja'ne...",
'mypage'         => "Menin' betim",
'mytalk'         => "Menin' sa'wbetim",
'anontalk'       => "Usı IP sa'wbeti",
'navigation'     => 'Navigatsiya',
'and'            => "ha'm",

# Metadata in edit box
'metadata_help' => "Metamag'lıwmat:",

'errorpagetitle'    => 'Qatelik',
'returnto'          => '$1 betine qaytıw.',
'tagline'           => "{{SITENAME}} mag'lıwmatı",
'help'              => 'Anıqlama',
'search'            => 'İzlew',
'searchbutton'      => 'İzle',
'go'                => "O'tiw",
'searcharticle'     => "O'tin'",
'history'           => 'Bet tariyxı',
'history_short'     => 'Tariyx',
'updatedmarker'     => "aqırg'ı kirgenimnen keyin jan'alang'anlar",
'info_short'        => "Mag'lıwmat",
'printableversion'  => 'Baspa nusqası',
'permalink'         => 'Turaqlı siltew',
'print'             => "Baspag'a shıg'arıw",
'edit'              => "O'zgertiw",
'create'            => 'Jaratıw',
'editthispage'      => "Usı betti o'zgertiw",
'create-this-page'  => 'Bul betti jaratıw',
'delete'            => "O'shiriw",
'deletethispage'    => "Usı betti o'shiriw",
'undelete_short'    => "{{PLURAL:$1|1 o'zgeristi|$1 o'zgerisin}} qayta tiklew",
'protect'           => "Qorg'aw",
'protect_change'    => "qorg'awdı o'zgertiw",
'protectthispage'   => "Bul betti qorg'aw",
'unprotect'         => "Qorg'amaw",
'unprotectthispage' => "Bul betti qorg'amaw",
'newpage'           => 'Taza bet',
'talkpage'          => 'Bul betti diskussiyalaw',
'talkpagelinktext'  => "Sa'wbet",
'specialpage'       => 'Arnawlı bet',
'personaltools'     => "Paydalanıwshı a'sbapları",
'postcomment'       => 'Kommentariy beriw',
'articlepage'       => "Mag'lıwmat betin ko'riw",
'talk'              => 'Diskussiya',
'views'             => "Ko'rinis",
'toolbox'           => "A'sbaplar",
'userpage'          => "Paydalanıwshı betin ko'riw",
'projectpage'       => "Proyekt betin ko'riw",
'imagepage'         => "Media betin ko'riw",
'mediawikipage'     => "Xabar betin ko'riw",
'templatepage'      => "Shablon betin ko'riw",
'viewhelppage'      => "Anıqlama betin ko'riw",
'categorypage'      => "Kategoriya betin ko'riw",
'viewtalkpage'      => "Diskussiyanı ko'riw",
'otherlanguages'    => 'Basqa tillerde',
'redirectedfrom'    => "($1 degennen burılg'an)",
'redirectpagesub'   => 'Burıwshı bet',
'lastmodifiedat'    => "Bul bettin' aqırg'ı ma'rte o'zgertilgen waqtı: $2, $1.", # $1 date, $2 time
'viewcount'         => "Bul bet {{PLURAL:$1|bir ma'rte|$1 ma'rte}} ko'rip shıg'ılg'an.",
'protectedpage'     => "Qorg'alg'an bet",
'jumpto'            => "Bug'an o'tiw:",
'jumptonavigation'  => 'navigatsiya',
'jumptosearch'      => 'izlew',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} haqqında',
'aboutpage'            => 'Project:Proyekt haqqında',
'bugreports'           => 'Qatelik esabatları',
'bugreportspage'       => 'Project:Qatelik esabatları',
'copyright'            => "Mag'lıwmat $1 boyınsha alıng'an.",
'copyrightpagename'    => "{{SITENAME}} proyektinin' avtorlıq huquqları",
'copyrightpage'        => '{{ns:project}}:Avtorlıq huquqları',
'currentevents'        => "Ha'zirgi ha'diyseler",
'currentevents-url'    => "Project:Ha'zirgi ha'diyseler",
'disclaimers'          => 'Juwapkershilikten bas tartıw',
'disclaimerpage'       => 'Project:Juwapkershilikten bas tartıw',
'edithelp'             => "O'zgertiw anıqlaması",
'edithelppage'         => "Help:O'zgertiw",
'faq'                  => 'KBS',
'faqpage'              => 'Project:KBS',
'helppage'             => 'Help:Mazmunı',
'mainpage'             => 'Bas bet',
'mainpage-description' => 'Bas bet',
'policy-url'           => "Project:Qag'ıydalar",
'portal'               => "Ja'miyet portalı",
'portal-url'           => "Project:Ja'miyet Portalı",
'privacy'              => "Konfidentsiallıq qag'ıydası",
'privacypage'          => "Project:Konfidentsiallıq qag'ıydası",

'badaccess'        => 'Ruxsatnama qateligi',
'badaccess-group0' => "Soralıp atırg'an ha'reketin'izdi bejeriwge ruqsatın'ız joq.",
'badaccess-group1' => "Soralıp atırg'an ha'reketin'iz $1 toparının' paydalanıwshılarına sheklengen.",
'badaccess-group2' => "Soralıp atırg'an ha'reketin'iz $1 toparlarının' birinin' paydalanıwshılarına sheklengen.",
'badaccess-groups' => "Soralıp atırg'an ha'reketin'iz $1 toparlarının' birinin' paydalanıwshılarına sheklengen.",

'versionrequired'     => "MediaWikidin' $1 nusqası kerek",
'versionrequiredtext' => "Bul betti paydalanıw ushın MediaWikidin' $1 nusqası kerek. [[Special:Version|Nusqa beti]]n qaran'.",

'ok'                      => 'OK',
'retrievedfrom'           => '"$1" saytınan alıng\'an',
'youhavenewmessages'      => 'Sizge $1 bar ($2).',
'newmessageslink'         => "jan'a xabarlar",
'newmessagesdifflink'     => "aqırg'ı o'zgeris",
'youhavenewmessagesmulti' => "$1 betinde sizge jan'a xabarlar bar",
'editsection'             => "o'zgertiw",
'editold'                 => "o'zgertiw",
'viewsourceold'           => "deregin ko'riw",
'editsectionhint'         => "$1 bo'limin o'zgertiw",
'toc'                     => 'Mazmunı',
'showtoc'                 => "ko'rset",
'hidetoc'                 => 'jasır',
'thisisdeleted'           => "$1: ko'riw yamasa qayta tiklew?",
'viewdeleted'             => "$1 ko'riw?",
'restorelink'             => "{{PLURAL:$1|bir o'shirilgen o'zgeris|$1 o'shirilgen o'zgeris}}",
'feedlinks'               => 'Jolaq:',
'feed-invalid'            => "Natuwrı jazılıw jolaqsha tu'ri.",
'feed-unavailable'        => "{{SITENAME}} saytında tarqatılatug'ın jolaqlar joq",
'site-rss-feed'           => '$1 saytının\' "RSS" jolag\'ı',
'site-atom-feed'          => '$1 saytının\' "Atom" jolag\'ı',
'page-rss-feed'           => '"$1" betinin\' "RSS" jolag\'ı',
'page-atom-feed'          => '"$1" betinin\' "Atom" jolag\'ı',
'feed-atom'               => '"Atom"',
'feed-rss'                => '"RSS"',
'red-link-title'          => "$1 (ele jaratılmag'an)",

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Bet',
'nstab-user'      => 'Paydalanıwshı',
'nstab-media'     => 'Media beti',
'nstab-special'   => 'Arnawlı',
'nstab-project'   => 'Proyekt beti',
'nstab-image'     => 'Fayl beti',
'nstab-mediawiki' => 'Xabar',
'nstab-template'  => 'Shablon',
'nstab-help'      => 'Anıqlama beti',
'nstab-category'  => 'Kategoriya',

# Main script and global functions
'nosuchaction'      => "Bunday ha'reket joq",
'nosuchactiontext'  => "Bul URLda ko'rsetilgen ha'reketti wiki tanıy almadı",
'nosuchspecialpage' => 'Bunday arnawlı bet joq',
'nospecialpagetext' => "<big>'''Siz sorag'an bunday arnawlı bet joq.'''</big>

Arnawlı betlerdin' dizimin [[Special:SpecialPages|{{int:specialpages}}]] betinen tabıwın'ızg'a boladı.",

# General errors
'error'                => "Qa'telik",
'databaseerror'        => "Mag'lıwmatlar bazası qa'tesi",
'dberrortext'          => "Mag'lıwmatlar bazası sorawında sintaksis qa'tesi sa'dir boldı.
Bul bag'darlamada qa'te barlıg'ın bildiriwi mu'mkin.
Aqırg'ı soralg'an mag'lıwmatlar bazası sorawı:
<blockquote><tt>\$1</tt></blockquote>
\"<tt>\$2</tt>\" funktsiyasınan.
MySQL qaytarg'an qa'tesi «<tt>\$3: \$4</tt>».",
'dberrortextcl'        => 'Mag\'lıwmatlar bazası sorawında sintaksis qa\'tesi sa\'dir boldı.
Aqırg\'ı soralg\'an mag\'lıwmatlar bazası sorawı:
"$1"
funktsiya: "$2".
MySQL qaytarg\'an qa\'tesi "$3: $4".',
'noconnect'            => "Keshirersiz! Wikide texnikalıq qıyınshılıqlar sa'dir bolg'anlıg'ı sebebli mag'ıwmatlar bazası serverine baylanısıw mu'mkinshiligi joq.<br />
$1",
'nodb'                 => "$1 mag'lıwmatlar bazası tabılmadı",
'cachederror'          => "To'mende soralg'an bettin' kesh nusqası ko'rsetilgen, ha'm jan'alanbag'an bolıwı mu'mkin.",
'laggedslavemode'      => "Esletpe: Bette aqırg'ı jan'alanıwlar bolmawı mu'mkin.",
'readonly'             => "Mag'lıwmatlar bazası qulplang'an",
'enterlockreason'      => "Qulıplawdın' sebebin ha'mde qansha waqıtqa esaplang'anlıg'ın ko'rsetin'",
'missingarticle-rev'   => "(du'zetiw#: $1)",
'missingarticle-diff'  => '(Ayrm.: $1, $2)',
'internalerror'        => "İshki qa'telik",
'internalerror_info'   => "İshki qa'telik: $1",
'filecopyerror'        => '"$1" faylın "$2" faylına ko\'shiriw a\'melge aspadı.',
'filerenameerror'      => '"$1" faylı atı "$2" atına o\'zgertilmedi.',
'filedeleteerror'      => '"$1" faylı o\'shirilmedi.',
'directorycreateerror' => '"$1" papkası jaratılmadı.',
'filenotfound'         => '"$1" faylı tabılmadı.',
'fileexistserror'      => '"$1" faylına jazıwg\'a bolmaydı: bunday fayl bar',
'unexpected'           => 'Ku\'tilmegen ma\'nis: "$1" = "$2".',
'formerror'            => "Qatelik: forma mag'lıwmatların jiberiw mu'mkin emes",
'badarticleerror'      => "Bunday ha'reket bul bette atqarılmaydı.",
'cannotdelete'         => "Ko'rsetilgen bet yamasa su'wret o'shirilmedi. (Bunı basqa birew aldınlaw o'shigen bolıwı mu'mkin.)",
'badtitle'             => 'Jaramsız atama',
'badtitletext'         => "Sorag'an betin'izdin' ataması natuwrı, bos, tillerara yamasa inter-wiki ataması natuwrı ko'rsetilgen. Atamada qollanıwg'a bolmaytug'ın bir yamasa bir neshe simvollar bolıwı mu'mkin.",
'wrong_wfQuery_params' => 'wfQuery() funktsiyası ushın natuwrı parametrler berilgen<br />
Funktsiya: $1<br />
Soraw: $2',
'viewsource'           => "Deregin ko'riw",
'viewsourcefor'        => '$1 ushın',
'actionthrottled'      => "Ha'reket toqtatıldı",
'protectedpagetext'    => "Bul bet o'zgertiwdin' aldın alıw ushın qulplang'an.",
'viewsourcetext'       => "Bul bettin' deregin qarawın'ızg'a ha'mde ko'shirip alıwın'ızg'a boladı:",
'editinginterface'     => "'''Esletpe:''' Siz ishinde MediaWiki sistema xabarı bar bolg'an betti o'zgertip atırsız. 
Bul bettin' o'zgeriwi basqa paydalanıwshılardın' sırtqı interfeisine ta'sir etedi.
Audarıw ushın,  MediaWiki programmasın jersindiriw [http://translatewiki.net/wiki/Main_Page?setlang=kaa Betawiki proyektisin] qarap shıg'ın'ız.",
'sqlhidden'            => "(SQL sorawı jasırılg'an)",
'namespaceprotected'   => "'''$1''' isimler ko'pligindegi betlerdi o'zgertiwge ruxsatın'ız joq.",
'customcssjsprotected' => "Bul betti o'zgertiwin'izge ruqsatın'ız joq, sebebi bul jerde basqa paydalanıwshılardın' jeke sazlawları bar.",
'ns-specialprotected'  => '"{{ns:special}}:" isimler ko\'pligindegi betler o\'zgertilmeydi',
'titleprotected'       => "Bul atamanı jaratıw [[User:$1|$1]] ta'repinen qorg'alg'an.
Keltirilgen sebep: ''$2''.",

# Login and logout pages
'logouttitle'                => "Shıg'ıw",
'logouttext'                 => "<strong>Siz endi sayttan shıqtın'ız.</strong><br />
Siz {{SITENAME}} saytınan anonim halda paydalanıwın'ız mu'mkin. Yamasa siz ja'ne ha'zirgi yaki basqa paydalanıwshı atı menen qaytadan sistemag'a kiriwin'izge boladı. Sonı este saqlan', ayrım betler sizin' brauzerin'izdin' keshi tazalanbag'anlıg'ı sebebli sistemada kirgenin'izdey ko'riniste dawam ettire beriwi mu'mkin.",
'welcomecreation'            => "== Xosh keldin'iz, $1! ==

Akkauntın'ız jaratıldı. {{SITENAME}} sazlawların'ızdı o'zgertiwdi umıtpan'.",
'loginpagetitle'             => 'Kiriw',
'yourname'                   => 'Paydalanıwshı atı:',
'yourpassword'               => 'Parol:',
'yourpasswordagain'          => "Paroldi qayta kiritin':",
'remembermypassword'         => "Menin' kirgenimdi usı kompyuterde saqlap qal",
'yourdomainname'             => "Sizin' domen:",
'loginproblem'               => "<b>Kiriw waqtında mashaqatlar sa'dir boldı.</b><br />Qaytadan kirip ko'rin'.",
'login'                      => 'Kiriw',
'nav-login-createaccount'    => 'Kiriw / akkaunt jaratıw',
'loginprompt'                => "{{SITENAME}} saytına kiriw ushın kukiler qosılg'an bolıwı kerek.",
'userlogin'                  => 'Kiriw / akkaunt jaratıw',
'logout'                     => "Shıg'ıw",
'userlogout'                 => "Shıg'ıw",
'notloggedin'                => 'Kirilmegen',
'nologin'                    => "Akkauntın'ız joqpa? $1.",
'nologinlink'                => "Akkaunt jaratın'",
'createaccount'              => 'Akkaunt jarat',
'gotaccount'                 => "Akkauntın'ız barma? $1.",
'gotaccountlink'             => 'Kir',
'createaccountmail'          => 'e-mail arqalı',
'badretype'                  => 'Siz kiritken parol tuwra kelmedi.',
'userexists'                 => "Kiritken paydalanıwshı atı ba'nt. Basqa at kiritin'.",
'youremail'                  => 'E-mail:',
'username'                   => 'Paydalanıwshı atı:',
'uid'                        => 'Paydalanıwshı IDsı:',
'prefs-memberingroups'       => "Kirgen {{PLURAL:$1|toparın'ız|toparların'ız}}:",
'yourrealname'               => "Haqıyqıy isimin'iz:",
'yourlanguage'               => 'Til:',
'yourvariant'                => "Tu'ri",
'yournick'                   => "Laqabın'ız:",
'badsig'                     => "Shala imzalar nadurıs; HTML teglerin tekserip ko'rin'.",
'badsiglength'               => "Laqap atın'ız dım uzın; 
$1 simvoldan aspawı kerek.",
'email'                      => 'E-mail',
'prefs-help-realname'        => "Haqıyqıy atın'ız (ma'jbu'riy emes): eger onı ko'rsetsen'iz, bet kim ta'repinen o'zgertilgenin ko'rsetiwde qollanıladı.",
'loginerror'                 => 'Kiriwde qatelik',
'prefs-help-email'           => "E-mail adresin'iz (ma'jbu'riy emes) basqa paydalanıwshılarg'a siz benen (adresin'izdi bilmegen xalda) baylanısıw imkaniyatın jaratadı.",
'prefs-help-email-required'  => 'E-mail adresi kerek.',
'nocookiesnew'               => "Paydalanıwshı akkauntı jaratıldı, biraq ele kirmegensiz.
Paydalanıwshılar kiriwi ushın {{SITENAME}} kukilerden paydalanadı.
Sizde kukiler o'shirilgen.
Kukilerdi qosıp, jan'a paydalanıwshı atın'ız ha'm parolin'iz arqalı kirin'.",
'nocookieslogin'             => "Paydalanıwshılar kiriwi ushın {{SITENAME}} kukilerden paydalanadı. 
Sizde kukiler o'shirilgen. 
Kukilerdi qosıp, qaytadan ko'rin'.",
'noname'                     => 'Siz kiritken paydalanıwshı atı qate.',
'loginsuccesstitle'          => "Kiriw tabıslı a'melge asırıldı",
'loginsuccess'               => "'''{{SITENAME}} saytına \"\$1\" paydalanıwshı atı menen kirdin'iz.'''",
'nosuchuser'                 => "\"\$1\" atlı paydalanıwshı joq. Tuwrı jazılg'anlıg'ın tekserin' yamasa taza akkaunt jaratın'.",
'nosuchusershort'            => '"<nowiki>$1</nowiki>" atlı paydalanıwshı joq. Tuwrı jazılg\'anlıg\'ın tekserin\'.',
'nouserspecified'            => "Siz paydalanıwshı atın ko'rsetpedin'iz.",
'wrongpassword'              => "Qate parol kiritlgen. Qaytadan kiritin'.",
'wrongpasswordempty'         => "Parol kiritilmegen. Qaytadan ha'reket etin'.",
'passwordtooshort'           => "Parolin'iz jaramsız yamasa dım qısqa. En' keminde $1 ha'rip ha'mde paydalanıwshı atın'ızdan o'zgeshe bolıwı kerek.",
'mailmypassword'             => "Paroldi e-mailg'a jiberiw",
'passwordremindertitle'      => '{{SITENAME}} ushın taza waqtınsha parol',
'passwordremindertext'       => "Kimdir (IP adresi: $1, ba'lkim o'zin'iz shıg'ar)
{{SITENAME}} ushın bizden taza parol jiberiwimizdi sorag'an ($4).
Endi «$2» paydalanıwshının' paroli «$3».
Ha'zir kirip parolin'izdi almastırıwın'ız kerek.

Eger basqa birew bunı sorag'an bolsa yamasa parolin'izdi eslegen bolsan'ız,
bul xabarg'a itibar bermey,
aldıng'ı parolin'izdi qollanıwın'ızg'a boladı.",
'noemail'                    => '"$1" paydalanıwshının\' e-mailı joq.',
'passwordsent'               => "Taza parol «$1» ushın ko'rsetilgen e-mail
adresine jiberildi.
Qabıl qılg'anın'ızdan keyin qaytadan kirin'.",
'blocked-mailpassword'       => "IP adresin'iz o'zgeris kiritiwden bloklang'an, so'nın' ushın paroldi tiklew funktsiyasın ha'm paydalana almaysız.",
'eauthentsent'               => "Tastıyıqlaw xatı e-mail adresin'izge jiberildi.
Basqa e-mail jiberiwden aldın, akkaunt shın'ınan siziki ekenin
tastıyıqlaw ushın xattag'ı ko'rsetpelerdi bejerin'.",
'throttled-mailpassword'     => "Aqırg'ı $1 saat ishinde parol eskertiw xatı jiberildi.
Jaman jolda paydalanıwdın' aldın alıw ushın, $1 saat sayın tek g'ana bir parol eskertiw xatı jiberiledi.",
'mailerror'                  => 'Xat jiberiwde qatelik juz berdi: $1',
'acct_creation_throttle_hit' => "Keshirersiz, siz aldın $1 akkaunt jaratqansız. Bunnan artıq jaratıw mu'mkinshiligin'iz joq.",
'emailauthenticated'         => "Sizin' e-mail adresin'iz tastıyıqlandı: $1.",
'emailnotauthenticated'      => "E-mail adresin'iz ele tastıyıqlanbag'an.
To'mendegi mu'mkinshilikler ushın hesh xat jiberilmeydi.",
'noemailprefs'               => "Usı mu'mkinshilikler islewi ushın e-mail adresin'izdi ko'rsetin'.",
'emailconfirmlink'           => "E-mail adresin'izdi tastıyıqlan'",
'invalidemailaddress'        => "E-mail adresin'iz nadurıs formatta bolg'anı ushın qabıl etile almaydı.
Durıs formattag'ı adresin'izdi ko'rsetin', yamasa qatardı bos qaldırın'.",
'accountcreated'             => 'Akkaunt jaratıldı',
'accountcreatedtext'         => '$1 paydalanıwshısına akkaunt jaratıldı.',
'createaccount-title'        => '{{SITENAME}} ushın akkaunt jaratıw',
'createaccount-text'         => 'Kimdir e-mail adresin\'izdi paydalanıp {{SITENAME}} saytında ($4) "$2" atı menen, "$3" paroli menen akkaunt jaratqan.
Endi saytqa kirip parolin\'izdi o\'zgertiwin\'iz kerek.

Eger bul akkaunt nadurıs jaratılg\'an bolsa, bul xabarg\'a itibar bermesen\'izde boladı.',
'loginlanguagelabel'         => 'Til: $1',

# Password reset dialog
'resetpass'               => "Akkaunt parolin aldıng'ı qa'lpine keltiriw",
'resetpass_announce'      => "E-mailin'izge jiberilgen waqtınshalıq kod penen kirdin'iz.
Kiriw protsessin juwmaqlaw ushın jan'a parolin'izdi usı jerge kiritin':",
'resetpass_header'        => "Paroldi o'zgertiw",
'resetpass_submit'        => "Paroldi kirgizin'",
'resetpass_success'       => "Parolin'iz sa'tli o'zgertildi! Endi kirin'...",
'resetpass_bad_temporary' => "Waqtinshalıq parol nadurıs.
Ba'lkim a'lle qashan parolin'izdi o'zgertken shıg'arsız yamasa jan'a waqtınshalıq parol sorag'an bolıwın'ız mu'mkin.",
'resetpass_forbidden'     => "{{SITENAME}} proyektinde parol o'zgertilmeydi",
'resetpass_missing'       => "Forma mag'lıwmatı joq.",

# Edit page toolbar
'bold_sample'     => 'Yarım juwan tekst',
'bold_tip'        => 'Yarım juwan tekst',
'italic_sample'   => 'Kursiv tekst',
'italic_tip'      => 'Kursiv tekst',
'link_sample'     => 'Siltew ataması',
'link_tip'        => 'İshki siltew',
'extlink_sample'  => 'http://www.example.com siltew ataması',
'extlink_tip'     => "Sırtqı siltew (http:// prefiksin kiritin')",
'headline_sample' => 'Atama teksti',
'headline_tip'    => "2-shi da'rejeli atama",
'math_sample'     => "Usı jerge formulanı jazın'",
'math_tip'        => 'Matematik formula (LaTeX)',
'nowiki_sample'   => "Formatlanbag'an tekstti usı jerge qoyın'",
'nowiki_tip'      => 'Wiki formatlawın esapqa almaw',
'image_tip'       => "Jaylastırılg'an fayl",
'media_tip'       => 'Fayl siltewi',
'sig_tip'         => "Sizin' imzan'iz ha'mde waqıt belgisi",
'hr_tip'          => "Gorizont bag'ıtındag'ı sızıq (dım ko'p paydalanban')",

# Edit pages
'summary'                   => 'Juwmaq',
'subject'                   => 'Ataması',
'minoredit'                 => "Bul kishi o'zgeris",
'watchthis'                 => 'Bul betti baqlaw',
'savearticle'               => 'Betti saqla',
'preview'                   => "Ko'rip shıg'ıw",
'showpreview'               => "Ko'rip shıq",
'showlivepreview'           => "Tez ko'rip shıg'ıw",
'showdiff'                  => "O'zgerislerdi ko'rset",
'anoneditwarning'           => "'''Esletpe:''' Siz kirmedin'iz. Sizin' IP adresin'iz usi bettin' o'zgeris tariyxında saqlanıp qaladı.",
'missingsummary'            => "'''Esletpe:''' O'zgeristin' qısqasha mazmunın ko'rsetpedin'iz.
\"Saqlaw\"dı ja'ne bassan'ız, o'zgerislerin'iz hesh qanday kommentariysiz saqlanadı.",
'missingcommenttext'        => "Kommentariydi to'mende kiritin'.",
'missingcommentheader'      => "'''Eskertpe:''' Bul kommentariy ushın atama ko'rsetpedin'iz.
Eger ja'ne \"Saqlaw\"dı bassan'ız, o'zgerislerin'iz olsız saqlanadı.",
'summary-preview'           => "Juwmag'ın ko'rip shıg'ıw",
'subject-preview'           => 'Atamanı aldınnan qaraw',
'blockedtitle'              => "Paydalanıwshı bloklang'an",
'blockedtext'               => "<big>'''Paydalaniwshı atın'ız yamasa IP adresin'iz bloklang'an.'''</big>

Bloklawdı \$1 a'melge asırg'an. Keltirilgen sebebi: ''\$2''.

* Bloklaw baslang'an: \$8
* Bloklaw tamamlang'an: \$6
* Bloklaw maqseti: \$7

Usı bloklawdı diskussiya qılıw ushın \$1 yamasa basqa [[{{MediaWiki:Grouppage-sysop}}|administratorlar]] menen baylanısqa shıg'ıwın'ızg'a boladı.
Siz [[Special:Preferences|akkaunt sazlawların'ızda]] haqıyqıy e-mailin'izdı ko'rsetpegenin'izshe ha'mde onı paydalanıwdan bloklang'an bolg'anısha \"Usı paydalanıwshıg'a xat jazıw\" qa'siyetinen qollana almaysız.
Sizin' ha'zirgi IP adresin'iz: \$3, bloklaw IDı: #\$5. Usılardın' birewin yamasa ekewinde ha'r bir sorawın'ızg'a qosın'.",
'blockednoreason'           => 'hesh sebep keltirilmegen',
'blockedoriginalsource'     => "'''$1''' degennin' deregi
to'mende ko'rsetilgen:",
'blockededitsource'         => "'''$1''' degennin' '''siz ozgertken''' teksti to'mende ko'rsetilgen:",
'whitelistedittitle'        => "O'zgertiw ushın sistemag'a kiriwin'iz kerek",
'whitelistedittext'         => "Betterdi o'zgertiw ushın $1 sha'rt.",
'confirmedittitle'          => "O'zgertiw ushın e-mail tastıyıqlaması kerek",
'confirmedittext'           => "Betlerge o'zgeris kiritiwin'iz ushın aldın E-pochta adresin'izdi tastıyıqlawın'ız kerek.
E-pochta adresin'izdi [[Special:Preferences|paydalanıwshı sazlawları bo'limi]] arqalı ko'rsetin' ha'm jaramlılıg'ın tekserin'.",
'nosuchsectiontitle'        => "Bunday bo'lim joq",
'nosuchsectiontext'         => "Ele jaratılmag'an bo'limdi o'zgerpekshisiz.
$1 bo'limi joq bolg'anlıg'ı sebepli sizin' o'zgertiwin'izdi saqlawg'a orın joq.",
'loginreqtitle'             => "Sistemag'a kiriw kerek",
'loginreqlink'              => 'kiriw',
'loginreqpagetext'          => "Basqa betlerdi ko'riw ushın sizge $1 kerek.",
'accmailtitle'              => 'Parol jiberildi.',
'accmailtext'               => '"$1" paroli $2 g\'a jiberildi.',
'newarticle'                => '(Taza)',
'newarticletext'            => "Siz ele jaratılmag'an betke siltew arqalı o'ttin'iz.
Betti jaratıw ushın to'mendegi aynada tekstin'izdi kiritin' (qosımsha mag'lıwmat ushın [[{{MediaWiki:Helppage}}|anıqlama betin]] qaran').
Eger bul jerge aljasıp o'tken bolsan'ız, brauzerin'izdin' «Arqag'a» knopkasın basın'.",
'noarticletext'             => "Ha'zirgi waqıtta bul bette hesh qanday mag'lıwmat joq. Basqa betlerden usı bet atamasın [[Special:Search/{{PAGENAME}}|izlep ko'riwin'izge]] yamasa usı betti [{{fullurl:{{FULLPAGENAME}}|action=edit}} jaratıwin'ızga'] boladi.",
'userpage-userdoesnotexist' => "\"\$1\" paydalanıwshı akkauntı registratsiya qılınbag'an. Bul betti jaratqın'ız yamasa o'zgertkin'iz kelse tekserip ko'rin'.",
'updated'                   => "(Jan'alang'an)",
'note'                      => '<strong>Eskertiw:</strong>',
'previewnote'               => "<strong>Bul ele tek aldınnan ko'rip shıg'ıw; o'zgerisler ele saqlanbadı!</strong>",
'editing'                   => "$1 o'zgertilmekte",
'editingsection'            => "$1 (bo'limi) o'zgertilmekte",
'editingcomment'            => "$1 (kommentariyi) o'zgertilmekte",
'editconflict'              => "O'zgertiw konflikti: $1",
'yourtext'                  => "Sizin' tekst",
'storedversion'             => "Saqlang'an nusqası",
'yourdiff'                  => 'Parqlar',
'copyrightwarning'          => "Este tutın', {{SITENAME}} proyektinde jaylastırılg'an ha'm o'zgertilgen maqalalar tekstleri $2 sha'rt tiykarında qaraladı (tolıqraq mag'lıwmat ushın: $1). Eger siz tekstin'izdin' erkin tarqatılıwın ha'mde qa'legen paydalanıwshı o'zgertiwin qa'lemesen'iz, bul jerge jaylastırmag'anın'ız maqul.<br />
Qosqan u'lesin'iz o'zin'izdin' jazg'anın'ız yamasa ashıq tu'rdegi derekten alıng'anlig'ına wa'de berin'.
<strong>AVTORLIQ HUQUQI MENEN QORG'ALG'AN MAG'LIWMATLARDI RUXSATSIZ JAYLASTIRMAN'!</strong>",
'copyrightwarning2'         => "Este tutın', {{SITENAME}} proyektindegi barlıq u'lesler basqa paydalanıwshılar arqalı o'zgertiliwi yamasa o'shiriliwi mu'mkin. Eger siz tekstin'izdin' erkin tarqatılıwın ha'mde qa'legen paydalanıwshı o'zgertiwin qa'lemesen'iz, bul jerge jaylastırmag'anın'ız maqul.<br /> Qosqan u'lesin'iz o'zin'izdin' jazg'anın'ız yamasa ashıq tu'rdegi derekten alıng'anlig'ına wa'de berin' (qosımsha mag'lıwmat ushın $1 hu'jjetin qaran'). <strong>AVTORLIQ HUQUQI MENEN QORG'ALG'AN MAG'LIWMATLARDI RUXSATSIZ JAYLASTIRMAN'!</strong>",
'longpagewarning'           => "<strong>ESLETPE: Bul bettin' ha'jmi $1 kilobayt, geybir brauzerler 32 KBqa jaqın yamasa onnan u'lken bolg'an betlerdi o'zgertiwde qıyınshılıqlarg'a tuwra keliwi mu'mkin. Betti kishi bo'leklerge bo'liw haqqında oylap ko'rin'.</strong>",
'semiprotectedpagewarning'  => "'''Eskertiw:''' Bet qulplang'an, tek registratsiyadan o'tken paydalanıwshılar g'ana o'zgerte aladı.",
'templatesused'             => "Bul bette qollanılg'an shablonlar:",
'templatesusedpreview'      => "Bul aldınnan ko'riw betinde qollanılg'an shablonlar:",
'templatesusedsection'      => "Bul bo'limde qollanılg'an shablonlar:",
'template-protected'        => "(qorg'alg'an)",
'template-semiprotected'    => "(yarım-qorg'alg'an)",
'nocreatetitle'             => 'Bet jaratıw sheklengen',
'nocreatetext'              => "{{SITENAME}} saytında taza betlerdi jaratıw sheklengen.
Arqag'a qaytıp bar betti o'zgertiwin'izge yamasa [[Special:UserLogin|kiriwin'izge / akkaunt jaratıwın'ızg'a]] boladı.",
'nocreate-loggedin'         => "{{SITENAME}} proyektinde taza betler jaratıwın'ızg'a ruxsatın'ız joq.",
'permissionserrors'         => 'Ruxsatnamalar Qatelikleri',
'recreate-deleted-warn'     => "'''Esletpe: Aldın o'shirilgen betti qayta jaratajaqsız.'''

Usi betti qaytadan jaratıw tuwrılıg'ın oylap ko'rin'.
Qolaylıq ushın to'mende o'shiriw jurnalı keltirilgen:",

# Account creation failure
'cantcreateaccounttitle' => 'Akkaunt jaratılmadı',
'cantcreateaccount-text' => "[[User:$3|$3]] usı IP adresten ('''$1''') akkaunt jaratıwın blokladı.

$3 keltirilgen sebebi: ''$2''",

# History pages
'viewpagelogs'        => "Usı bettin' jurnalın ko'riw",
'nohistory'           => "Bul bettin' o'zgertiw tariyxı joq.",
'revnotfound'         => 'Nusqa tabılmadı',
'currentrev'          => "Ha'zirgi nusqa",
'revisionasof'        => '$1 waqtındagı nusqası',
'revision-info'       => "$1 waqtındag'ı $2 istegen nusqası",
'previousrevision'    => '←Eskilew nusqası',
'nextrevision'        => "Jan'alaw nusqası→",
'currentrevisionlink' => "Ha'zirgi nusqa",
'cur'                 => "ha'z.",
'next'                => 'keyin.',
'last'                => 'aqır.',
'page_first'          => 'birinshi',
'page_last'           => "aqırg'ı",
'histlegend'          => "Tu'sindirme: salıstırajaq nusqaların'ızdı saylan' ha'mde <Enter> knopkasın yamasa to'mendegi knopkani basın'.<br />
Sha'rtli belgiler: (ha'z.) = ha'zirgi nusqasi menen parqı,
(aqır.) = aldıng'ı nusqasi menen parqı, k = kishi o'zgeris",
'deletedrev'          => "[o'shirilgen]",
'histfirst'           => "En' aldıng'ısı",
'histlast'            => "En' aqırg'ısı",
'historysize'         => '({{PLURAL:$1|1 bayt|$1 bayt}})',
'historyempty'        => '(bos)',

# Revision feed
'history-feed-title'          => 'Nusqa tariyxı',
'history-feed-description'    => "Usı bettin' wikidegi nusqa tariyxı",
'history-feed-item-nocomment' => "$2 waqtındag'ı $1", # user at time

# Revision deletion
'rev-deleted-comment'    => "(kommentariy o'shirildi)",
'rev-deleted-user'       => "(paydalanıwshı atı o'shirildi)",
'rev-delundel'           => "ko'rsetiw/jasırıw",
'revdelete-selected'     => "[[:$1]] {{PLURAL:$2|saylang'an nusqası|saylang'an nusqaları}}:",
'revdelete-legend'       => "Ko'rinis sheklewlerin belgilew",
'revdelete-hide-text'    => 'Nusqa tekstin jasır',
'revdelete-hide-name'    => "Ha'reket ha'm onın' obyektin jasır",
'revdelete-hide-comment' => "O'zgertiw kommentariyin jasır",
'revdelete-hide-user'    => "O'zgeriwshi atın/IP jasır",
'revdelete-hide-image'   => "Fayl mag'lıwmatın jasır",
'revdelete-log'          => 'Jurnal kommentariyi:',
'revdelete-logentry'     => "[[$1]] nusqa ko'rinisin o'zgertti",

# History merging
'mergehistory'      => 'Bet tariyxların qos',
'mergehistory-from' => 'Derek bet:',
'mergehistory-into' => 'Belgilengen bet:',
'mergehistory-go'   => "Qosılıwı mu'mkin bolg'an oz'geriserdi ko'rset",

# Merge log
'revertmerge' => 'Ajırat',

# Diffs
'history-title'           => '"$1" betinin\' nusqa tariyxı',
'difference'              => "(Nusqalar arasındag'ı ayırmashılıq)",
'lineno'                  => 'Qatar No $1:',
'compareselectedversions' => "Saylang'an nusqalardı salıstırıw",
'editundo'                => 'qaytar',
'diff-multi'              => "(Aradag'ı {{PLURAL:$1|bir nusqa|$1 nusqa}} ko'rsetilmeydi.)",

# Search results
'searchresults'         => "İzlew na'tiyjeleri",
'searchsubtitle'        => "'''[[:$1]]''' ushın izlegenin'iz",
'searchsubtitleinvalid' => "'''$1''' ushın izlegenin'iz",
'noexactmatch'          => "'''\"\$1\" atamalı bet joq.''' Bul betti [[:\$1|jaratıwın'ız]] mu'mkin.",
'notitlematches'        => 'Hesh qanday bet ataması tuwra kelmedi',
'textmatches'           => "Bet tekstinin' tuwra kelgenleri",
'notextmatches'         => 'Hesh qanday bet teksti tuwra kelmedi',
'prevn'                 => "aldıng'ı $1",
'nextn'                 => 'keyingi $1',
'viewprevnext'          => "Ko'riw: ($1) ($2) ($3)",
'powersearch'           => "Ken'eytilgen izlew",
'powersearch-legend'    => "Ken'eytilgen izlew",
'search-external'       => 'Sırtqı izlewshi',

# Preferences page
'preferences'              => 'Sazlawlar',
'mypreferences'            => "Menin' sazlawlarım",
'prefs-edits'              => "O'zgertiwler sanı:",
'prefsnologin'             => 'Kirilmegen',
'qbsettings'               => 'Navigatsiya paneli',
'qbsettings-none'          => 'Hesh qanday',
'qbsettings-fixedleft'     => 'Shepke bekitilgen',
'qbsettings-fixedright'    => "On'g'a bekitilgen",
'qbsettings-floatingleft'  => 'Shepte jıljıwshı',
'qbsettings-floatingright' => "On'da jıljıwshı",
'changepassword'           => "Paroldi o'zgertiw",
'skin'                     => "Sırtqı ko'rinis",
'math'                     => 'Formulalar',
'dateformat'               => "Sa'ne formatı",
'datedefault'              => 'Hesh sazlawlarsız',
'datetime'                 => "Sa'ne ha'm waqıt",
'math_unknown_error'       => "belgisiz qa'telik",
'math_unknown_function'    => 'belgisiz funktsiya',
'math_lexing_error'        => "leksikalıq qa'telik",
'math_syntax_error'        => "sintaksikalıq qa'telik",
'prefs-personal'           => 'Paydalanıwshı profaylı',
'prefs-rc'                 => "Aqırg'ı o'zgerisler",
'prefs-watchlist'          => 'Baqlaw dizimi',
'prefs-misc'               => 'Basqa',
'saveprefs'                => 'Saqla',
'oldpassword'              => "Aldıng'ı parol:",
'newpassword'              => 'Taza parol:',
'retypenew'                => "Taza paroldi qayta kiritin':",
'textboxsize'              => "O'zgertiw",
'rows'                     => 'Qatarlar:',
'columns'                  => "Bag'analar:",
'searchresultshead'        => 'İzlew',
'recentchangesdays'        => "Aqırg'ı o'zgerislerde ko'rsetiletug'ın ku'nler:",
'recentchangescount'       => "Aqırg'ı o'zgerislerde ko'rsetiletug'ın o'zgerisler sanı:",
'savedprefs'               => "Sizin' sazlawların'ız saqlandı.",
'timezonelegend'           => 'Waqıt zonası',
'localtime'                => 'Jergilikli waqıt',
'servertime'               => "Serverdin' waqtı",
'default'                  => 'defolt',
'files'                    => 'Fayllar',

# User rights
'userrights'               => 'Paydalanıwshı huqıqların basqarıw', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Paydalanıwshı toparların basqarıw',
'userrights-user-editname' => "Paydalanıwshı atın kiritin':",
'editusergroup'            => "Paydalanıwshı Toparların O'zgertiw",
'editinguser'              => "Paydalanıwshı <b>$1</b> o'zgertilmekte ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => "Paydalanıwshı toparların o'zgertiw",
'saveusergroups'           => 'Paydalanıwshı Toparların Saqlaw',
'userrights-groupsmember'  => "Ag'zalıq toparı:",
'userrights-reason'        => "O'zgertiw sebebi:",

# Groups
'group'               => 'Topar:',
'group-autoconfirmed' => "O'zi tastıyıqlang'anlar",
'group-bot'           => 'Botlar',
'group-sysop'         => 'Administratorlar',
'group-bureaucrat'    => 'Byurokratlar',
'group-all'           => "(ha'mmesi)",

'group-autoconfirmed-member' => "O'zi tastıyıqlang'an",
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Administrator',
'group-bureaucrat-member'    => 'Byurokrat',

'grouppage-autoconfirmed' => "{{ns:project}}:O'zi tastıyıqlang'an paydalanıwshılar",
'grouppage-bot'           => '{{ns:project}}:Botlar',
'grouppage-sysop'         => '{{ns:project}}:Administratorlar',
'grouppage-bureaucrat'    => '{{ns:project}}:Byurokratlar',

# User rights log
'rightslog'     => 'Paydalanıwshı huquqları jurnalı',
'rightslogtext' => "Bul paydalanıwshı huquqların o'zgertiw jurnalı.",
'rightsnone'    => '(hesh qanday)',

# Recent changes
'nchanges'                          => "{{PLURAL:$1|1 o'zgeris|$1 o'zgeris}}",
'recentchanges'                     => "Aqırg'ı o'zgerisler",
'recentchanges-feed-description'    => "Wikidin' usı ag'ımındag'ı en' aqırg'ı o'zgerislerin baqlaw.",
'rcnote'                            => "To'mende $3 waqtındag'ı aqırg'ı {{PLURAL:$2|ku'ndegi|'''$2''' ku'ndegi}} {{PLURAL:$1|'''1''' o'zgeris bar|aqırg'ı '''$1''' o'zgeris bar}}.",
'rcnotefrom'                        => "To'mende '''$2''' baslap '''$1''' shekemgi o'zgerisler ko'rsetilgen.",
'rclistfrom'                        => "$1 waqtınan baslap jan'a o'zgerisler ko'rset",
'rcshowhideminor'                   => "Kishi o'zgerislerdi $1",
'rcshowhidebots'                    => 'Botlardı $1',
'rcshowhideliu'                     => 'Kirgenlerdi $1',
'rcshowhideanons'                   => 'Anonim paydalanıwshılardı $1',
'rcshowhidepatr'                    => "Tekserilgen o'zgerislerdi $1",
'rcshowhidemine'                    => "O'zgertiwlerimdi $1",
'rclinks'                           => "Aqırg'ı $2 ku'ndegi aqırg'ı $1 o'zgeristi ko'rset<br />$3",
'diff'                              => 'parq',
'hist'                              => 'tar.',
'hide'                              => 'jasır',
'show'                              => "ko'rset",
'minoreditletter'                   => 'k',
'newpageletter'                     => 'T',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => "[Baqlag'an {{PLURAL:$1|1 paydalanıwshı|$1 paydalanıwshı}}]",
'rc_categories_any'                 => "Ha'r qanday",
'newsectionsummary'                 => "/* $1 */ taza bo'lim",

# Recent changes linked
'recentchangeslinked'          => "Baylanıslı o'zgerisler",
'recentchangeslinked-title'    => '"$1" baylanıslı o\'zgerisler',
'recentchangeslinked-noresult' => "Siltelgen betlerde berilgen waqıt dawamında hesh qanday o'zgeris bolmag'an.",
'recentchangeslinked-summary'  => "Bul arnawlı bette siltelgen betlerdegi aqırg'ı o'zgerisler dizimi ko'rsetilgen. Baqlaw dizimin'izdegi betler '''juwan''' ha'ribi menen ko'rsetilgen.",

# Upload
'upload'             => 'Fayldı aploud qılıw',
'uploadbtn'          => 'Aploud!',
'uploadnologin'      => 'Kirilmegen',
'uploadlogpage'      => 'Aploud jurnalı',
'filename'           => 'Fayl atı',
'filedesc'           => 'Juwmaq',
'fileuploadsummary'  => 'Juwmaq:',
'filestatus'         => 'Avtorlıq huqıqı statusı:',
'filesource'         => 'Fayl deregi:',
'uploadedfiles'      => "Aploud qılıng'an faillar",
'ignorewarning'      => 'Eskertiwlerdi esapqa almay fayldı saqla',
'ignorewarnings'     => 'Hesh qanday eskertiwdi esapqa alma',
'minlength1'         => "Fail atı keminde bir ha'ripten turıwı sha'rt.",
'badfilename'        => 'Fayl atı bug\'an o\'zgertildi: "$1".',
'filetype-missing'   => 'Bul faildın ken\'eytpesi (mısalı ".jpg") joq.',
'largefileserver'    => "Bul faildın mo'lsheri serverdin' ruxsatınan u'lken.",
'fileexists-thumb'   => "<center>'''Mına fayl bar'''</center>",
'successfulupload'   => 'Tabıslı aploud',
'uploadwarning'      => 'Aploud eskertiwi',
'savefile'           => 'Fayldı saqla',
'uploadedimage'      => '«[[$1]]» faylı aploud qılındı',
'uploaddisabled'     => 'Aploudqa ruxsat berilmegen',
'uploaddisabledtext' => '{{SITENAME}} proyektinde aploudqa ruxsat berilmegen.',
'uploadvirus'        => "Bul failda virus bar! Mag'lıwmat: $1",
'sourcefilename'     => "Derektin' fayl atı:",
'destfilename'       => 'Belgilengen fail atı:',
'watchthisupload'    => 'Bul betti baqlaw',

'upload-proto-error' => 'Nadurıs protokol',
'upload-file-error'  => "İshki qa'telik",
'upload-misc-error'  => 'Belgisiz aploud qatesi',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'  => 'URL tabılmadı',
'upload-curl-error28' => 'Aploudqa berilgen waqıt pitti',

'nolicense'          => "Hesh na'rse saylanbag'an",
'upload_source_file' => " (sizin' kompyuterin'izdegi fayl)",

# Special:ImageList
'imagelist_search_for'  => 'Media atamasın izlew:',
'imgfile'               => 'fayl',
'imagelist'             => 'Fayllar dizimi',
'imagelist_date'        => "Sa'ne",
'imagelist_name'        => 'Atama',
'imagelist_user'        => 'Paydalnıwshı',
'imagelist_size'        => "Ha'jim",
'imagelist_description' => 'Kommentariy',

# Image description page
'filehist'                  => 'Fayl tariyxı',
'filehist-help'             => "Aldın usı fayl qanday ko'riniste bolg'anın ko'riw ushin ku'n-ay/waqıt degendi basın'.",
'filehist-deleteall'        => "ha'mmesin o'shir",
'filehist-deleteone'        => "usını o'shiriw",
'filehist-current'          => "ha'zirgi",
'filehist-datetime'         => "Sa'ne/Waqıt",
'filehist-user'             => 'Paydalanıwshı',
'filehist-dimensions'       => "O'lshemleri",
'filehist-filesize'         => "Fayldın' ha'jmi",
'filehist-comment'          => 'Kommentariy',
'imagelinks'                => 'Siltewler',
'linkstoimage'              => "To'mendegi betler bul faylg'a siltelgen:",
'nolinkstoimage'            => "Bul faylg'a hesh bir bet siltelmegen.",
'sharedupload'              => "Bul ortalıq fayl ha'm basqa proektlerde paydalanılsa boladı.",
'shareduploadwiki-linktext' => "fayl juwmag'ının' beti",
'noimage'                   => "Bunday atı fayl joq, $1 mu'mkinshiligin'iz bar.",
'noimage-linktext'          => 'usını aploud qılıw',
'uploadnewversion-linktext' => "Bul fayldın' jan'a nusqasın aploud qılıw",

# File reversion
'filerevert-comment' => 'Kommentariy:',

# File deletion
'filedelete'             => "$1 degendi o'shiriw",
'filedelete-legend'      => "Fayldı o'shiriw",
'filedelete-intro'       => "'''[[Media:$1|$1]]''' o'shirilmekte.",
'filedelete-intro-old'   => "[$4 $3, $2] waqtındag'ı '''[[Media:$1|$1]]''' nusqası o'shirilmekte.",
'filedelete-comment'     => 'Kommentariy:',
'filedelete-submit'      => "O'shiriw",
'filedelete-success'     => "'''$1''' o'shirildi.",
'filedelete-success-old' => "<span class=\"plainlinks\">\$3, \$2 waqtındag'ı '''[[Media:\$1|\$1]]''' nusqası o'shirildi.</span>",
'filedelete-nofile'      => "'''$1''' degen {{SITENAME}} proyektinde joq.",

# MIME search
'mimesearch' => 'MIME izlew',
'mimetype'   => "MIME tu'ri:",
'download'   => 'koshirip alıw',

# Unwatched pages
'unwatchedpages' => "Baqlanbag'an betler",

# List redirects
'listredirects' => 'Burıwshılar dizimi',

# Unused templates
'unusedtemplates'    => "Paydalanılmag'an shablonlar",
'unusedtemplateswlh' => 'basqa burıwshılar',

# Random page
'randompage'         => "Qa'legen bet",
'randompage-nopages' => "Bul isimler ko'pliginde hesh bet joq.",

# Random redirect
'randomredirect'         => "Qa'legen burıwshı",
'randomredirect-nopages' => "Bul isimler ko'pliginde hesh burıwshı joq.",

# Statistics
'statistics'             => 'Statistika',
'sitestats'              => '{{SITENAME}} statistikası',
'userstats'              => 'Paydalanıwshı statistikası',
'sitestatstext'          => "Mag'lıwmatlar bazasında {{PLURAL:$1|'''1'''|ha'mmesi bolıp '''$1'''}} bet bar.
Bug'an «sa'wbet» betleri, {{SITENAME}} haqqındag'ı betler, «shala» betler, burıwshı betler, ja'ne de basqa mag'lıwmat dep tanılmaytug'ın betler kiritiledi.
Usılardı esapqa almag'anda, haqıyqıy mag'lıwmatqa iye '''$2''' bet bar dep boljanadı.

'''$8''' fayl aploud qılındı.

{{SITENAME}} ornatılg'annan beri betler {{PLURAL:$3|'''1'''|ha'mmesi bolıp '''$3'''}} ret qaralg'an, '''$4''' ret o'zgertilgen.
Bunın' na'tiyjesinde ortasha esap penen ha'r bir betke '''$5''' o'zgeris tuwrı keledi, ha'mde ha'r bir o'zgeriske '''$6''' qaraw tuwrı  keledi.

[http://www.mediawiki.org/wiki/Manual:Job_queue Tapsırımalar gezeginin'] uzınlıg'ı: '''$7'''.",
'userstatstext'          => "Bul jerde '''$1''' [[Special:ListUsers|esapqa alıng'an paydalanıwshı]] bar, solardın' ishinen '''$2''' (yag'nıy '''$4 %''') paydalanıwshısında $5 huquqları bar.",
'statistics-mostpopular' => "En' ko'p ko'rilgen betler",

'disambiguations'     => "Ko'p ma'nisli betler",
'disambiguationspage' => '{{ns:template}}:disambig',

'doubleredirects' => 'Qos burıwshılar',

'brokenredirects'        => "Hesh betke bag'ıtlamaytug'ın burıwshılar",
'brokenredirects-edit'   => "(o'zgertiw)",
'brokenredirects-delete' => "(o'shiriw)",

'withoutinterwiki' => "Hesh tilge siltemeytug'ın betler",

'fewestrevisions' => "En' az du'zetilgen betler",

# Miscellaneous special pages
'nbytes'                  => '{{PLURAL:$1|1 bayt|$1 bayt}}',
'ncategories'             => '{{PLURAL:$1|1 kategoriya|$1 kategoriya}}',
'nlinks'                  => '{{PLURAL:$1|1 siltew|$1 siltew}}',
'nmembers'                => "{{PLURAL:$1|1 ag'za|$1 ag'zalar}}",
'nrevisions'              => '{{PLURAL:$1|1 nusqa|$1 nusqa}}',
'nviews'                  => "{{PLURAL:$1|1 ma'rte|$1 ma'rte}} ko'rip shıg'ılg'an",
'lonelypages'             => 'Hesh betten siltelmegen betler',
'uncategorizedpages'      => 'Kategoriyasız betler',
'uncategorizedcategories' => 'Kategoriyasız kategoriyalar',
'uncategorizedimages'     => 'Kategoriyasız fayllar',
'uncategorizedtemplates'  => 'Kategoriyasız shablonlar',
'unusedcategories'        => "Paydalanılmag'an kategoriyalar",
'unusedimages'            => "Paydalanılmag'an fayllar",
'popularpages'            => "En' ko'p ko'rilgen betler",
'wantedcategories'        => "Talap qılıng'an kategoriyalar",
'wantedpages'             => "Talap qılıng'an betler",
'mostlinked'              => "En' ko'p siltelgen betler",
'mostlinkedcategories'    => "En' ko'p paydalanılg'an kategoriyalar",
'mostlinkedtemplates'     => "En' ko'p paydalanılg'an shablonlar",
'mostcategories'          => "En' ko'p kategoriyalang'an betler",
'mostimages'              => "En' ko'p paydalanılg'an fayllar",
'mostrevisions'           => "En' ko'p du'zetilgen betler",
'prefixindex'             => 'Atama baslaw dizimi',
'shortpages'              => "En' qısqa betler",
'longpages'               => "En' uzın betler",
'deadendpages'            => "Hesh betke siltemeytug'ın betler",
'deadendpagestext'        => "To'mendegi betler {{SITENAME}} proyektindegi basqa betlerge siltelmegen.",
'protectedpages'          => "Qorg'alg'an betler",
'protectedpagestext'      => "To'mendegi betler ko'shiriw ha'm o'zgertiwden qorg'alg'an",
'protectedpagesempty'     => "Usı parametrler menen ha'zir hesh bet qorg'almag'an",
'listusers'               => 'Paydalanıwshı dizimi',
'newpages'                => "En' taza betler",
'newpages-username'       => 'Paydalanıwshı atı:',
'ancientpages'            => "En' eski betler",
'move'                    => "Ko'shiriw",
'movethispage'            => "Bul betti ko'shiriw",
'notargettitle'           => "Nıshan ko'rsetilmegen",

# Book sources
'booksources'               => 'Kitap derekleri',
'booksources-search-legend' => 'Kitap haqqında informatsiya izlew',
'booksources-go'            => "O'tin'",

# Special:Log
'specialloguserlabel'  => 'Paydalanıwshı:',
'speciallogtitlelabel' => 'Atama:',
'log'                  => 'Jurnallar',
'all-logs-page'        => "Ha'mme jurnallar",
'log-search-legend'    => 'Jurnallardı izlew',
'log-search-submit'    => "O'tin'",
'log-title-wildcard'   => "Usı tekstten baslang'an atamalardı izlew",

# Special:AllPages
'allpages'          => "Ha'mme betler",
'alphaindexline'    => '$1 — $2',
'nextpage'          => 'Keyingi bet ($1)',
'prevpage'          => "Aldıng'ı bet ($1)",
'allpagesfrom'      => "Mına betten baslap ko'rsetiw:",
'allarticles'       => "Ha'mme betler",
'allinnamespace'    => "Ha'mme betler ($1 isimler ko'pligi)",
'allnotinnamespace' => "Ha'mme betler ($1 isimler ko'pliginen emes)",
'allpagesprev'      => "Aldıng'ı",
'allpagesnext'      => 'Keyingi',
'allpagessubmit'    => "O'tin'",
'allpagesprefix'    => "Mına prefiksten baslag'an betlerdi ko'rsetiw:",
'allpages-bad-ns'   => '{{SITENAME}} proyektinde "$1"  isimler ko\'pligi joq.',

# Special:Categories
'categories'         => 'Kategoriyalar',
'categoriespagetext' => 'Kelesi kategoriyalar ishinde betler yamasa media bar.',

# Special:ListUsers
'listusersfrom'      => "Mına paydalanıwshıdan baslap ko'rsetiw:",
'listusers-submit'   => "Ko'rset",
'listusers-noresult' => 'Paydalanıwshı tabılmadı.',

# E-mail user
'mailnologin'     => 'Jiberiwge adres tabılmadı',
'emailuser'       => 'Xat jiberiw',
'emailpage'       => "Paydalanıwshıg'a e-mail jiberiw",
'defemailsubject' => '{{SITENAME}} e-mail',
'noemailtitle'    => 'E-mail adresi joq',
'emailfrom'       => 'Kimnen',
'emailto'         => 'Kimge',
'emailmessage'    => 'Xat',
'emailsend'       => 'Jiber',
'emailccme'       => "Menin' xabarımnın' ko'shirmesin e-mailımg'a jiber.",
'emailsent'       => 'Xat jiberildi',
'emailsenttext'   => "E-mail xatın'ız jiberildi.",

# Watchlist
'watchlist'            => 'Betlerdi baqlaw dizimi',
'mywatchlist'          => "Menin' baqlaw dizimim",
'watchlistfor'         => "('''$1''' ushın)",
'nowatchlist'          => "Baqlaw dizimin'iz bos.",
'watchlistanontext'    => "Baqlaw dizimin'izdegilerdi qaraw yamasa o'zgertiw ushın $1 kerek.",
'watchnologin'         => 'Kirilmegen',
'watchnologintext'     => "Baqlaw dizimin'izdi o'zgertiw ushın [[Special:UserLogin|kiriwin'iz]] kerek.",
'addedwatch'           => 'Baqlaw dizimine qosıldı',
'addedwatchtext'       => "\"[[:\$1]]\" beti [[Special:Watchlist|baqlaw dizimin'izge]] qosıldı.
Usı ha'm og'an baylanıslı bolg'an sa'wbet betlerinde bolatug'ın keleshektegi o'zgerisler usı dizimde ko'rsetiledi ha'mde betti tabıwdı an'satlastırıw ushın [[Special:RecentChanges|taza o'zgerisler diziminde]] '''juwan ha'ripte''' ko'rsetiledi.
Eger siz bul betti baqlaw dizimin'izden o'shirmekshi bolsan'ız bettin' joqarg'ı on' jag'ındag'ı \"Baqlamaw\" jazıwın basın'.",
'removedwatch'         => "Baqlaw diziminen o'shirildi",
'removedwatchtext'     => '"[[:$1]]" beti baqlaw dizimin\'izden o\'shirildi.',
'watch'                => 'Baqlaw',
'watchthispage'        => 'Bul betti baqlaw',
'unwatch'              => 'Baqlamaw',
'unwatchthispage'      => 'Baqlawdı toqtatıw',
'notanarticle'         => "Mag'lıwmat beti emes",
'watchlist-details'    => "Baqlaw diziminde (sa'wbet betlerin esapqa almag'anda) {{PLURAL:$1|1 bet|$1 bet}} bar.",
'wlheader-enotif'      => "* E-mail arqalı eskertiw qosılg'an.",
'watchlistcontains'    => "Sizin' baqlaw dizimin'izde {{PLURAL:$1|1 bet|$1 bet}} bar.",
'wlnote'               => "To'mende aqırg'ı {{PLURAL:$2|saattag'ı|'''$2''' saattag'ı}} {{PLURAL:$1|aqırg'ı o'zgeris bar|aqırg'ı '''$1''' o'zgeris bar}}.",
'wlshowlast'           => "Aqırg'ı $1 saat, $2 ku'n, $3 ko'rset",
'watchlist-show-bots'  => "Bot o'zgertiwlerin' ko'rset",
'watchlist-hide-bots'  => "Bot o'zgertiwlerin' jasır",
'watchlist-show-own'   => "O'zgertiwlerimdi ko'rset",
'watchlist-hide-own'   => "O'zgertiwlerimdi jasır",
'watchlist-show-minor' => "Kishi o'zgerislerdi ko'rset",
'watchlist-hide-minor' => "Kishi o'zgerislerdi jasır",

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Baqlaw...',
'unwatching' => 'Baqlamaw...',

'enotif_reset'                 => "Barlıq betti ko'rip shıg'ıldı dep belgile",
'enotif_newpagetext'           => 'Bul taza bet.',
'enotif_impersonal_salutation' => '{{SITENAME}} paydalanıwshısı',
'changed'                      => "o'zgertilgen",
'created'                      => "jaratılg'an",
'enotif_lastvisited'           => "Son'g'ı kirwin'izden beri bolg'an o'zgerisler ushın $1 degendi ko'rin'iz.",
'enotif_lastdiff'              => "Usı o'zgeris ushın $1 degendi ko'rin'iz.",
'enotif_anon_editor'           => 'anonim paydalanıwshı $1',

# Delete/protect/revert
'deletepage'                  => "Betti o'shir",
'confirm'                     => 'Tastıyıqlaw',
'excontent'                   => "bolg'an mag'lıwmat: '$1'",
'excontentauthor'             => "bolg'an mag'lıwmat: '$1' (tek '[[Special:Contributions/$2|$2]]' u'lesi)",
'exblank'                     => 'bet bos edi',
'historywarning'              => "Esletpe: O'shireyin dep atırg'an betin'izdin' tariyxi bar:",
'confirmdeletetext'           => "Siz bul betti yamasa su'wretti barliq tariyxı menen mag'lıwmatlar bazasınan o'shirejaqsız.
Bunın' aqıbetin tu'singenin'izdi ha'm [[{{MediaWiki:Policy-url}}]] siyasatına ılayıqlı ekenligin tastıyıqlan'.",
'actioncomplete'              => "Ha'reket tamamlandı",
'deletedtext'                 => "\"<nowiki>\$1</nowiki>\" o'shirildi.
Aqırg'ı o'shirilgenlerdin' dizimin ko'riw ushin \$2 ni qaran'",
'deletedarticle'              => '"[[$1]]" o\'shirildi',
'dellogpage'                  => "O'shiriw jurnalı",
'dellogpagetext'              => "To'mende en' aqırg'ı o'shirilgenlerdin' dizimi keltirilgen",
'deletionlog'                 => "o'shiriw jurnalı",
'deletecomment'               => "O'shiriwdin' sebebi:",
'deleteotherreason'           => 'Basqa/qosımsha sebep:',
'deletereasonotherlist'       => 'Basqa sebep',
'rollback'                    => "O'zgerislerdi biykar etiw",
'rollback_short'              => 'Biykar etiw',
'rollbacklink'                => 'qaytarıw',
'rollbackfailed'              => "Biykar etiw sa'tsiz tamamlandı",
'editcomment'                 => 'O\'zgertiwdin\' bolg\'an kommentariyi: "<i>$1</i>".', # only shown if there is an edit comment
'protectlogpage'              => "Qorg'aw jurnalı",
'protectedarticle'            => '"[[$1]]" qorg\'alg\'an',
'modifiedarticleprotection'   => '"[[$1]]" betinin\' qorg\'aw da\'rejesi ozgertildi',
'unprotectedarticle'          => '"[[$1]]" qorg\'almag\'an',
'protect-legend'              => "Qorg'awdı tastıyıqlaw",
'protectcomment'              => 'Kommentariy:',
'protectexpiry'               => "Ku'shin joytıw waqtı:",
'protect_expiry_invalid'      => "Nadurıs ku'shin joytıw waqtı.",
'protect_expiry_old'          => "Kushin joytıw waqtı o'tip ketken.",
'protect-unchain'             => "Ko'shiriw ruxsatın beriw",
'protect-text'                => "<strong><nowiki>$1</nowiki></strong> betinin' qorg'aw da'rejesin ko're yamasa o'zgerte alasız.",
'protect-locked-access'       => "Akkauntın'ızdın' bettın' qorg'aw da'rejesin o'zgertiwge ruxsatı joq.
<strong>$1</strong> betinin' ha'zirgi sazlawları:",
'protect-cascadeon'           => "Bul bet ha'zirgi waqıtta qorg'alg'an, sebebi usı bet kaskadlı qorg'awı bar {{PLURAL:$1|betke|betlerine}} qosılg'an. Bul bettin' qorg'aw da'rejesin o'zgerte alasız, biraq bul kaskadlı qorg'awg'a ta'sir etpeydi.",
'protect-default'             => '(defolt)',
'protect-fallback'            => '"$1" ruxsatı kerek',
'protect-level-autoconfirmed' => 'Anonim paydalanıwshılardı bloklaw',
'protect-level-sysop'         => 'Tek administratorlar',
'protect-summary-cascade'     => "kaskadlang'an",
'protect-expiring'            => 'pitiw waqtı: $1 (UTC)',
'protect-cascade'             => "Bul betke qosılg'an betlerdi qorg'aw (kaskadlı qorg'aw).",
'protect-cantedit'            => "Bul bettin' qorg'aw da'rejesin o'zgerte almaysız, sebebi oni o'zgertiwge sizin' ruxsatın'ız joq.",
'restriction-type'            => 'Ruxsatnama:',
'restriction-level'           => "Sheklew da'rejesi:",
'minimum-size'                => "En' az o'lshemi",
'maximum-size'                => "En' ko'p o'lshemi:",
'pagesize'                    => '(bayt)',

# Restrictions (nouns)
'restriction-edit' => "O'zgertiw",
'restriction-move' => "Ko'shiriw",

# Restriction levels
'restriction-level-sysop'         => "tolıq qorg'alg'an",
'restriction-level-autoconfirmed' => "yarım-qorg'alg'an",
'restriction-level-all'           => "ha'mme basqısh",

# Undelete
'undelete'                 => "O'shirilgen betlerdi ko'riw",
'undeletepage'             => "O'shirilgen betlerdi ko'riw ha'm qayta tiklew",
'viewdeletedpage'          => "O'shirilgen betlerdi ko'riw",
'undelete-revision'        => "$2 waqtında $3 o'shirgen $1 nusqası:",
'undelete-nodiff'          => "Hesh aldıng'ı nusqa tabılmadı.",
'undeletebtn'              => 'Qayta tiklew',
'undeletecomment'          => 'Kommentariy:',
'undeletedarticle'         => '"[[$1]]" qayta tiklendi',
'undeletedrevisions'       => '{{PLURAL:$1|1 nusqa|$1 nusqa}} qayta tiklendi',
'undeletedrevisions-files' => "{{PLURAL:$1|1 nusqa|$1 nusqa}} ha'm {{PLURAL:$2|1 fayl|$2 fayl}} qayta tiklendi",
'undeletedfiles'           => '{{PLURAL:$1|1 fayl|$1 fayl}} qayta tiklendi',
'undelete-search-box'      => "O'shirilgen betlerdi izlew",
'undelete-search-prefix'   => "Mınadan baslag'an betlerdi ko'rsetiw:",
'undelete-search-submit'   => 'İzle',
'undelete-error-short'     => "Faildı tilkewde qa'telik: $1",

# Namespace form on various pages
'namespace'      => "İsimler ko'pligi:",
'invert'         => "Saylaw ta'rtibin almastırıw",
'blanknamespace' => '(Baslı)',

# Contributions
'contributions' => "Paydalanıwshı u'lesi",
'mycontris'     => "Menin' u'lesim",
'contribsub2'   => '$1 ushın ($2)',
'uctop'         => "(joqarg'ı)",
'month'         => "Aydag'ı (ha'm onnanda erterek):",
'year'          => "Jıldag'ı (ha'm onnanda erterek):",

'sp-contributions-newbies'     => "Tek taza akkauntlar u'leslerin ko'rset",
'sp-contributions-newbies-sub' => 'Taza akkauntlar ushın',
'sp-contributions-blocklog'    => 'Bloklaw jurnalı',
'sp-contributions-search'      => "U'lesi boyınsha izlew",
'sp-contributions-username'    => 'IP Adres yamasa paydalanıwshı atı:',
'sp-contributions-submit'      => 'İzle',

# What links here
'whatlinkshere'       => 'Siltelgen betler',
'whatlinkshere-title' => '$1 degenge siltelgen betler',
'whatlinkshere-page'  => 'Bet:',
'linklistsub'         => '(Siltewler dizimi)',
'linkshere'           => "To'mendegi betler mınag'an siltelgen: '''[[:$1]]''':",
'nolinkshere'         => "'''[[:$1]]''' degenge hesh bet siltemeydi.",
'isredirect'          => 'burıwshı bet',
'istemplate'          => 'qosıw',
'whatlinkshere-prev'  => "{{PLURAL:$1|aldıng'ı|aldıng'ı $1}}",
'whatlinkshere-next'  => '{{PLURAL:$1|keyingi|keyingi $1}}',
'whatlinkshere-links' => '← siltewler',

# Block/unblock
'blockip'                     => 'Paydalanıwshını bloklaw',
'ipaddress'                   => 'IP Adres:',
'ipadressorusername'          => 'IP Adres yamasa paydalanıwshı atı:',
'ipbexpiry'                   => "Ku'shin joytıw waqtı:",
'ipbreason'                   => 'Sebep:',
'ipbreasonotherlist'          => 'Basqa sebep',
'ipbanononly'                 => 'Tek anonim paydalanıwshılardı bloklaw',
'ipbcreateaccount'            => "Akkaunt jaratıwdı qadag'an etiw",
'ipbemailban'                 => "Paydalanıwshını e-mail jiberiwden qadag'alaw",
'ipbsubmit'                   => 'Bul paydalanıwshını bloklaw',
'ipbother'                    => 'Basqa waqıt:',
'ipboptions'                  => "2 saat:2 hours,1 ku'n:1 day,3 ku'n:3 days,1 ha'pte:1 week,2 h'apte:2 weeks,1 ay:1 month,3 ay:3 months,6 ay:6 months,1 jil:1 year,sheksiz:infinite", # display1:time1,display2:time2,...
'ipbotheroption'              => 'basqa',
'ipbotherreason'              => 'Basqa/qosımsha sebep:',
'badipaddress'                => 'Jaramsız IP adres',
'blockipsuccesssub'           => 'Tabıslı qulplaw',
'blockipsuccesstext'          => "[[Special:Contributions/$1|$1]] bloklang'an.<br />
Basqa bloklawlar ushın [[Special:IPBlockList|IP bloklaw dizimin]] ko'rip shıg'ın'iz.",
'ipb-edit-dropdown'           => "Bloklaw sebeplerin o'zgertiw",
'ipb-unblock-addr'            => '$1 degennin qulpın sheshiw',
'ipb-unblock'                 => "Paydalanıwshının' yamasa IP adrestin' qulpın shesh",
'unblockip'                   => "Paydalanıwshının' qulpın sheshiw",
'ipusubmit'                   => "Bul adrestin' qulpın shesh",
'unblocked-id'                => "$1 bloklawı o'shirildi",
'ipblocklist'                 => "Bloklang'an paydalanıwshı / IP adres dizimi",
'ipblocklist-legend'          => "Bloklang'an paydalanıwshını tabıw",
'ipblocklist-username'        => 'Paydalanıwshı atı yamasa IP adres:',
'ipblocklist-submit'          => 'İzle',
'blocklistline'               => '$1, $2 waqıtında $3 blokladı ($4)',
'infiniteblock'               => 'sheksiz',
'expiringblock'               => "Ku'shin joytıw waqtı: $1",
'anononlyblock'               => 'tek anon.',
'noautoblockblock'            => "avtoqulplaw o'shirilgen",
'createaccountblock'          => "Akkaunt jaratıw qadag'alang'an",
'emailblock'                  => "e-mail bloklang'an",
'ipblocklist-empty'           => 'Bloklaw dizimi bos.',
'blocklink'                   => 'bloklaw',
'unblocklink'                 => 'bloklamaw',
'contribslink'                => "u'lesi",
'blocklogpage'                => 'Bloklaw jurnalı',
'blocklogentry'               => "[[$1]] $2 waqıt aralıg'ına bloklandı $3",
'block-log-flags-anononly'    => 'tek anonim paydalanıwshılar',
'block-log-flags-nocreate'    => "Akkaunt jaratıw o'shirilgen",
'block-log-flags-noautoblock' => "Avtoqulplaw o'shirilgen",
'block-log-flags-noemail'     => "e-mail bloklang'an",
'ipb_expiry_invalid'          => "Ku'shin joytıw waqtı nadurıs.",
'ipb_already_blocked'         => '"$1" aldın qulplang\'an',
'proxyblocker-disabled'       => "Bul funktsiya o'shirilgen.",
'proxyblocksuccess'           => 'Tamamlandı.',

# Developer tools
'lockdb'              => "Mag'lıwmatlar bazasın qulpla",
'unlockdb'            => "Mag'lıwmatlar bazasının' qulpın shesh",
'lockconfirm'         => "Awa, men mag'lıwmatlar bazasın qulplayman.",
'unlockconfirm'       => "Awa, men mag'lıwmatlar bazasının' qulpın sheshemen.",
'lockbtn'             => "Mag'lıwmatlar bazasın qulpla",
'unlockbtn'           => "Mag'lıwmatlar bazasının' qulpın shesh",
'locknoconfirm'       => "Tastıyıqlaw belgisin qoymadın'ız.",
'lockdbsuccesssub'    => "Mag'lıwmatlar bazasın qulplaw tabıslı tamamlandı",
'unlockdbsuccesssub'  => "Mag'lıwmatlar bazasının' qulpı sheshildi",
'unlockdbsuccesstext' => "Mag'lıwmatlar bazasının' qulpı sheshildi",
'databasenotlocked'   => "Mag'lıwmatlar bazası qulplanbag'an",

# Move page
'move-page-legend'        => "Betti ko'shiriw",
'movepagetext'            => "To'mendegi forma bettin' atamasın o'zgertedi, onın' barlıq tariyxın taza atamag'a ko'shiredi.
Burıng'ı bet ataması taza atamag'a qayta bag'ıtlang'an bet bolıp qaladı.
Eski atamag'a silteytug'ın siltewler o'zgertilmeydi, ko'shiriwden son' shınjırlı yamasa natuwrı qayta bag'ıtlang'an betlerdin' bar-joqlıg'ınj tekserip ko'rin'.
Siltewlerdin' tuwrı islewine siz juwapker bolasız.

Itibar berin': eger taza atamalı bet aldınnan bar bolsa ha'm son'g'ı o'zgertiw tariyxısız bos bet yamasa qayta bag'ıtlandırıwshı bolg'anına deyin bet '''ko'shirilmeydi'''.
Bul degeni, eger betti aljasıp qayta atasan'iz aldıng'ı atamag'a qaytıwın'ızg'a boladı, biraq bar bettin' u'stine jazıwın'ızg'a bolmaydi.

'''ESTE TUTIN'!'''
Bul ko'p qaralatug'ın betke qatan' ha'm ku'tilmegen o'zgerisler alıp keliwi mu'mkin;
dawam ettiriwden aldın qanday aqıbetlerge alıp keliwin oylap ko'rin'.",
'movepagetalktext'        => "To'mendegi sebepler bar '''bolg'anısha''', sa'wbet beti avtomatik halda ko'shiriledi:
* Bos emes sa'wbet beti jan'a atamada bar bolg'anda yaki
* To'mendegi qutını belgilemegen'izde.

Bul jag'daylarda eger qa'lesen'iz betti qoldan ko'shiriwin'iz yamasa qosıwın'izg'a boladı.",
'movearticle'             => "Ko'shiriletug'ın bet:",
'newtitle'                => 'Taza atama:',
'move-watch'              => 'Bul betti baqlaw',
'movepagebtn'             => "Betti ko'shir",
'pagemovedsub'            => "Tabıslı ko'shirildi",
'articleexists'           => "Bunday atamalı bet bar yamasa natuwrı atama sayladın'ız.
Basqa atama saylan'",
'talkexists'              => "'''Bettin' o'zi a'wmetli ko'shirildi, biraq sa'wbet beti ko'shirilmedi sebebi jan'a atamanın' sa'wbet beti bar eken. Olardı o'zin'iz qoldan qosın'.'''",
'movedto'                 => "betke ko'shirildi",
'movetalk'                => "Baylanıslı sa'wbet betin ko'shiriw",
'1movedto2'               => "[[$1]] beti [[$2]] degenge ko'shirildi",
'1movedto2_redir'         => "[[$1]] beti [[$2]] degen burıwshıg'a ko'shirildi",
'movelogpage'             => "Ko'shiriw jurnalı",
'movelogpagetext'         => "To'mende ko'shirilgen betlerdin' dizimi keltirilgen.",
'movereason'              => 'Sebep:',
'revertmove'              => 'qaytarıw',
'delete_and_move'         => "O'shiriw ha'm ko'shiriw",
'delete_and_move_confirm' => "Awa, bul betti o'shiriw",
'delete_and_move_reason'  => "Ko'shiriwge jol beriw ushın o'shirilgen",

# Export
'export'            => 'Betlerdi eksport qılıw',
'export-submit'     => 'Eksport',
'export-addcattext' => 'Mına kategoriyadan betlerdi qosıw:',
'export-addcat'     => 'Qos',

# Namespace 8 related
'allmessages'         => 'Sistema xabarları',
'allmessagesname'     => 'Atama',
'allmessagesdefault'  => 'Defolt tekst',
'allmessagescurrent'  => "Ha'zirgi tekst",
'allmessagestext'     => "Bul {{ns:mediawiki}} isimler ko'pligindegi bar bolg'an sistema xabarları dizimi.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.",
'allmessagesfilter'   => 'Xabar atamasın filtrlew:',
'allmessagesmodified' => "Tek o'zgertilgenlerdi ko'rset",

# Thumbnails
'thumbnail-more'           => "U'lkeytiw",
'filemissing'              => 'Fayl tabılmadı',
'thumbnail_error'          => "Miniatyura jaratıw qa'teligi: $1",
'thumbnail_invalid_params' => 'Miniatyura sazlawları natuwrı',

# Special:Import
'import'                  => 'Betlerdi import qılıw',
'import-interwiki-submit' => 'Import',
'importstart'             => 'Betler import qılınbaqta...',
'import-revision-count'   => '{{PLURAL:$1|1 nusqa|$1 nusqa}}',
'importnopages'           => "Import qılınatug'ın betler joq.",
'importunknownsource'     => "Import qılıw derek tu'ri belgisiz",
'importnotext'            => 'Bos yamasa tekstsiz',

# Import log
'importlogpage'                    => 'Import qılıw jurnalı',
'import-logentry-upload-detail'    => '{{PLURAL:$1|1 nusqa|$1 nusqa}}',
'import-logentry-interwiki-detail' => '$2 degennen {{PLURAL:$1|1 nusqa|$1 nusqa}}',

# Tooltip help for the actions
'tooltip-pt-userpage'             => "Menin' paydalanıwshı betim",
'tooltip-pt-anonuserpage'         => 'Bul IP adres paydalanıwshı beti',
'tooltip-pt-mytalk'               => "Menin' sa'wbetim",
'tooltip-pt-anontalk'             => "Bul IP adresten kiritilgen o'zgerisler haqqında diskussiya",
'tooltip-pt-preferences'          => "Menin' sazlawlarım",
'tooltip-pt-watchlist'            => "O'zgerislerin baqlap turg'an betler dizimi",
'tooltip-pt-mycontris'            => "Menin' u'lesler dizimim",
'tooltip-pt-login'                => "Kiriwin'iz usınıladı, biraq ma'jbu'riy bolmag'an xalda.",
'tooltip-pt-anonlogin'            => "Kiriwin'iz usınıladı, biraq ma'jbu'riy bolmag'an xalda.",
'tooltip-pt-logout'               => "Shıg'ıw",
'tooltip-ca-talk'                 => "Mag'lıwmat beti haqqında diskussiya",
'tooltip-ca-edit'                 => "Siz bul betti o'zgertiwin'izge boladi. Iltimas betti saqlawdan aldın ko'rip shig'ıw knopkasın paydalanın'.",
'tooltip-ca-addsection'           => "Bul diskussiyag'a kommentariy qosıw.",
'tooltip-ca-viewsource'           => "Bul bet qorg'alg'an. Biraq ko'rip shıg'ıwın'ızg'a boladı.",
'tooltip-ca-history'              => "Bul bettin' aqırg'ı nusqaları.",
'tooltip-ca-protect'              => "Bul betti qorg'aw",
'tooltip-ca-delete'               => "Bul betti o'shiriw",
'tooltip-ca-undelete'             => "Bul bettin' o'shiriwden aldın bolg'an o'zgertiwlerin qaytarıw",
'tooltip-ca-move'                 => "Bul betti ko'shiriw",
'tooltip-ca-watch'                => "Bul betti menin' baqlaw dizimime qosiw",
'tooltip-ca-unwatch'              => "Bul betti menin' baqlaw dizimimnen alıp tasla",
'tooltip-search'                  => '{{SITENAME}} saytınan izlew',
'tooltip-search-fulltext'         => 'Usı tekst ushın betlerdi izlew',
'tooltip-p-logo'                  => 'Bas bet',
'tooltip-n-mainpage'              => "Bas betke o'tiw",
'tooltip-n-portal'                => "Proyekt haqqında, nelerdi islewin'izge boladi, qayaqtan tabıwın'ızg'a boladi",
'tooltip-n-currentevents'         => "Ha'zirgi ha'diyseler haqqında mag'lıwmat tabıw",
'tooltip-n-recentchanges'         => "Wikidegi aqırg'ı o'zgerislerdin' dizimi.",
'tooltip-n-randompage'            => "Qa'legen betti ju'klew",
'tooltip-n-help'                  => 'Anıqlama tabıw ornı.',
'tooltip-t-whatlinkshere'         => 'Usı betke siltelgen barlıq betler dizimi',
'tooltip-t-recentchangeslinked'   => "Bul betten siltengen betlerdegi aqırg'ı o'zgerisler",
'tooltip-feed-rss'                => 'Bul bettin\' "RSS" jolag\'ı',
'tooltip-feed-atom'               => 'Bul bettin\' "Atom" jolag\'ı',
'tooltip-t-contributions'         => "Usı paydalanıwshının' u'lesler dizimin ko'riw",
'tooltip-t-emailuser'             => "Usı paydalanıwshıg'a e-mail jiberiw",
'tooltip-t-upload'                => 'Fayllardı aploud qılıw',
'tooltip-t-specialpages'          => 'Barlıq arnawlı betler dizimi',
'tooltip-t-print'                 => "Bul bettin' baspa nusqası",
'tooltip-t-permalink'             => "Bul bettegi usı nusqasının' turaqlı siltewi",
'tooltip-ca-nstab-main'           => "Mag'lıwmat betin ko'riw",
'tooltip-ca-nstab-user'           => "Paydalanıwshı betin ko'riw",
'tooltip-ca-nstab-media'          => "Media betin ko'riw",
'tooltip-ca-nstab-special'        => "Bul arnawlı bet, onı o'zgerte almaysız.",
'tooltip-ca-nstab-project'        => "Proyekt betin ko'riw",
'tooltip-ca-nstab-image'          => "Fayl betin ko'riw",
'tooltip-ca-nstab-mediawiki'      => "Sistema xabarın ko'riw",
'tooltip-ca-nstab-template'       => "Shablondı ko'riw",
'tooltip-ca-nstab-help'           => "Anıqlama betin ko'riw",
'tooltip-ca-nstab-category'       => "Kategoriya betin ko'riw",
'tooltip-minoredit'               => "Kishi o'zgeris dep belgilew",
'tooltip-save'                    => "O'zgertiwlerin'izdi saqla",
'tooltip-preview'                 => "Saqlawdan aldın kiritken o'zgerislerin'izdi ko'rip shıg'ın'!",
'tooltip-diff'                    => "Tekstke qanday o'zgeris kiritkenin'izdi ko'rsetiw",
'tooltip-compareselectedversions' => "Bettin' eki nusqasının' ayırmashılıg'ın qaraw.",
'tooltip-watch'                   => "Bul betti baqlaw dizimin'izge qosıw",
'tooltip-upload'                  => 'Aploudtı basla',

# Attribution
'anonymous'        => '{{SITENAME}} anonim paydalanıwshı(ları)',
'siteuser'         => '{{SITENAME}} paydalanıwshısı $1',
'lastmodifiedatby' => "Bul bettin' aqırg'ı ma'rte $3 o'zgertken waqtı: $2, $1.", # $1 date, $2 time, $3 user
'others'           => 'basqalar',
'siteusers'        => '{{SITENAME}} paydalanıwshı(ları) $1',
'creditspage'      => 'Bet avtorları',

# Info page
'infosubtitle' => "Bet haqqında mag'lıwmat",
'numedits'     => "O'zgerisler sanı (bet): $1",
'numtalkedits' => "O'zgerisler sanı (diskussiya beti): $1",
'numwatchers'  => 'Baqlawshılar sanı: $1',

# Patrol log
'patrol-log-auto' => '(avtomatlasqan)',

# Image deletion
'deletedrevision'       => "$1 eski nusqasın o'shirdi",
'filedeleteerror-short' => "Fayl o'shiriw qateligi: $1",

# Browsing diffs
'previousdiff' => "← Aldıng'ı parq",
'nextdiff'     => 'Keyingi parq →',

# Media information
'thumbsize'            => "Miniatyuranın' ha'jmi:",
'widthheight'          => '$1 × $2',
'widthheightpage'      => '$1 × $2, $3 bet',
'file-info'            => "(fayldın' ha'jmi: $1, MIME tu'ri: $2)",
'file-info-size'       => "($1 × $2 piksel, fayldın' ha'jmi: $3, MIME tu'ri: $4)",
'file-nohires'         => '<small>Bunnan joqarı imkaniyatlı tabılmadı.</small>',
'svg-long-desc'        => "(SVG fayl, $1 × $2 piksel belgilengen, fayldın' ha'jmi: $3)",
'show-big-image'       => 'Joqarı imkaniyatlı',
'show-big-image-thumb' => "<small>Bul aldinnan ko'riwdin' ha'jmi: $1 × $2 piksel</small>",

# Special:NewImages
'newimages'             => 'Taza fayllar galereyasi',
'showhidebots'          => '(botlardı $1)',
'noimages'              => "Ko'riwge su'wret joq.",
'ilsubmit'              => 'İzle',
'bydate'                => "sa'ne boyınsha",
'sp-newimages-showfrom' => "$2, $1 baslap taza fayllardı ko'rset",

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'video-dims'   => '$1, $2 × $3',
'hours-abbrev' => 'st',

# Bad image list
'bad_image_list' => "Formatı to'mendegishe:

Tek dizim elementleri (* menen baslanatug'ın qatarlar) esaplanadi. 
Qatardın' birinshi siltewi natuwrı faylg'a siltewi sha'rt.
Sol qatardag'ı keyingi ha'r bir siltewler tısqarı qabıl etiledi, mısalı qatar ishindegi ushırasatug'ın faylı bar betler.",

# Metadata
'metadata'          => "Metamag'lıwmat",
'metadata-help'     => "Usı faylda a'dette sanlı kamera yamasa skaner arqalı qosılatug'ın qosımsha mag'lıwmat bar.
Eger fayl jaratılg'anınan keyin o'zgertilgen bolsa, geybir parametrleri o'zgertilgen faylg'a tuwra kelmewi mu'mkin.",
'metadata-expand'   => "Qosımsha mag'lıwmatlardı ko'rset",
'metadata-collapse' => "Qosımsha mag'lıwmatlardi jasır",
'metadata-fields'   => "Usı xabarda ko'rsetilgen EXIF metamag'lıwmat qatarları metamag'lıwmat kestesi jasırılg'anda su'wret betinde ko'rsetiledi. Basqalar defolt boyınsha jasırılg'an.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'       => 'Yeni:',
'exif-imagelength'      => "Uzunlıg'ı",
'exif-imagedescription' => "Su'wret ataması",
'exif-artist'           => 'Avtor',

# External editor support
'edit-externally'      => "Bul fayldı sırtqı bag'darlama arqalı o'zgertiw",
'edit-externally-help' => "Ko'birek mag'lıwmat ushın [http://www.mediawiki.org/wiki/Manual:External_editors ornatıw jolların] qaran'.",

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => "ha'mmesin",
'imagelistall'     => "ha'mme",
'watchlistall2'    => "ha'mmesin",
'namespacesall'    => "ha'mmesi",
'monthsall'        => "ha'mme",

# E-mail address confirmation
'confirmemail'            => 'E-mail adresin tastıyıqlaw',
'confirmemail_send'       => 'Tastıyıqlaw kodın jiberiw',
'confirmemail_sent'       => 'Tastıyıqlaw xatı jiberildi.',
'confirmemail_oncreate'   => "Tastıyıqlaw kodı e-mail adresin'izge jiberildi.
Bul kod kiriw ushın talap etilmeydi, biraq wikidin' e-mail mu'mkinshiliklerinen paydalanıwın'ız ushın kodtı ko'rsetiwin'iz kerek.",
'confirmemail_sendfailed' => "Tastıyıqlaw xatı jiberilmedi.
Adresin'izde jaramsız simvollar bolmawına tekserip shıg'ın'.

Xat jiberiwshinin' qaytarg'an mag'lıwmatı: $1",
'confirmemail_invalid'    => "Tastıyıqlaw kodı nadurıs.
Kodtın' jaramlılıq waqtı pitken bolıwı mu'mkin.",
'confirmemail_needlogin'  => "E-mail adresin'izdi tastıyıqlaw ushın $1 kerek.",
'confirmemail_success'    => "Sizin' e-mail adresin'iz tastıyıqlandı, endi wikige kiriwin'iz mu'mkin.",
'confirmemail_loggedin'   => "Sizin' e-mail adresin'iz endi tastıyıqlandı.",
'confirmemail_error'      => "Tastıyıqlawın'ızdı saqlaw waqtında belgisiz qa'te ju'z berdi.",
'confirmemail_subject'    => '{{SITENAME}} e-pochta adresi tastıyıqaw xatı',
'confirmemail_body'       => "Geybirew, ba'lkimiz o'zin'iz shıg'ar, $1 IP adresinen,
{{SITENAME}} saytında bul E-pochta adresin qollanıp «$2» degen akkaunt jarattı.

Usı akkaunt shın'ınan ha'm siziki ekenin tastıyıqlaw ushın ha'mde {{SITENAME}} saytının'
e-pochta mu'mkinshiliklerin paydalanıw ushın, to'mendegi siltewdi brauzerin'izde ashın':

$3

Eger bul akkauntti jaratpag'an bolsan'ız, to'mendegi siltewge o'tip
e-pochta adresin'izdin' tastıyıqlawın o'shirsen'iz boladı:

$5

Bul tastıyıqlaw kodının' pitetug'ın waqtı: $4.",

# Trackbacks
'trackbackremove' => " ([$1 O'shir])",

# Delete conflict
'recreate' => 'Qaytadan jaratıw',

'unit-pixel' => ' px',

# HTML dump
'redirectingto' => '[[:$1]] degenge burılmaqta...',

# action=purge
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "''$1'' mag'lıwmatı bar betlerdi izlew.",
'searchnamed'      => "''$1'' ataması bar betlerdi izlew.",
'articletitles'    => "''$1'' degen menen baslag'an betlerdi",
'hideresults'      => "Na'tiyjelerdi jasır",

# Multipage image navigation
'imgmultipageprev' => "← aldıng'ı bet",
'imgmultipagenext' => 'keyingi bet →',
'imgmultigo'       => "O'tin'!",

# Table pager
'ascending_abbrev'         => "o's.",
'descending_abbrev'        => 'kem.',
'table_pager_next'         => 'Keyingi bet',
'table_pager_prev'         => "Aldıng'ı bet",
'table_pager_first'        => 'Birinshi bet',
'table_pager_last'         => "Aqırg'ı bet",
'table_pager_limit_submit' => "O'tin'",
'table_pager_empty'        => "Na'tiyjeler joq",

# Auto-summaries
'autosumm-blank'   => "Bettin' barlıq mag'lıwmatın o'shırıw",
'autosumm-replace' => "Betti '$1' penen almastırıw",
'autoredircomment' => '[[$1]] degenge burıw',
'autosumm-new'     => 'Taza bet: $1',

# Friendlier slave lag warnings
'lag-warn-normal' => "Usı dizimde $1 sekundtan jan'alaw bolg'an o'zgerisler ko'rsetilmewi mu'mkin.",

# Watchlist editor
'watchlistedit-numitems'      => "Sizin' baqlaw dizimin'izde, sa'wbet betlerin esapqa almag'anda {{PLURAL:$1|1 atama|$1 atama}} bar.",
'watchlistedit-noitems'       => "Baqlaw dizimin'izde atamalar joq.",
'watchlistedit-normal-title'  => "Baqlaw dizimin o'zgertiw",
'watchlistedit-normal-legend' => "Baqlaw diziminen atamalardi o'shıriw",
'watchlistedit-normal-submit' => "Atamalardı O'shir",
'watchlistedit-normal-done'   => "Baqlaw dizimin'izden {{PLURAL:$1|1 atama|$1 atama}} o'shirildi:",
'watchlistedit-raw-title'     => '"Shiyki" baqlaw dizimin o\'zgertiw',
'watchlistedit-raw-legend'    => '"Shiyki" baqlaw dizimin o\'zgertiw',
'watchlistedit-raw-titles'    => 'Atamalar:',
'watchlistedit-raw-submit'    => "Baqlaw dizimin jan'ala",
'watchlistedit-raw-done'      => "Baqılaw dizimin'iz jan'alandı.",
'watchlistedit-raw-added'     => "{{PLURAL:$1|1 atama|$1 atama}} qosilg'an:",
'watchlistedit-raw-removed'   => "{{PLURAL:$1|1 atama|$1 atama}} o'shirildi:",

# Watchlist editing tools
'watchlisttools-view' => "Baylanıslı o'zgerislerdi qaraw",
'watchlisttools-edit' => "Baqlaw dizimin ko'riw ha'm o'zgertiw",
'watchlisttools-raw'  => '"Shiyki" baqlaw dizimin o\'zgertiw',

# Special:Version
'version' => "MediaWikidin' nusqası", # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath' => 'Fayl jolı',

# Special:SpecialPages
'specialpages' => 'Arnawlı betler',

);
