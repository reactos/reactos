<?php
/** Bashkir (Башҡорт)
 *
 * @ingroup Language
 * @file
 *
 * @author Рустам Нурыев
 */

$fallback = 'ru';

$namespaceNames = array(
	NS_MEDIA            => 'Медиа',
	NS_SPECIAL          => 'Ярҙамсы',
	NS_MAIN             => '',
	NS_TALK             => 'Фекер_алышыу',
	NS_USER             => 'Ҡатнашыусы',
	NS_USER_TALK        => 'Ҡатнашыусы_м-н_фекер_алышыу', 
	#NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_б-са_фекер_алышыу',
	NS_IMAGE            => 'Рәсем',
	NS_IMAGE_TALK       => 'Рәсем_б-са_фекер_алышыу',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_б-са_фекер_алышыу',
	NS_TEMPLATE         => 'Ҡалып',
	NS_TEMPLATE_TALK    => 'Ҡалып_б-са_фекер_алышыу',
	NS_HELP             => 'Белешмә',
	NS_HELP_TALK        => 'Белешмә_б-са_фекер_алышыу',
	NS_CATEGORY         => 'Категория',
	NS_CATEGORY_TALK    => 'Категория_б-са_фекер_алышыу',
);

$linkTrail = '/^((?:[a-z]|а|б|в|г|д|е|ё|ж|з|и|й|к|л|м|н|о|п|р|с|т|у|ф|х|ц|ч|ш|щ|ъ|ы|ь|э|ю|я|ә|ө|ү|ғ|ҡ|ң|ҙ|ҫ|һ|“|»)+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'       => 'Һылтанмалар аҫтына һыҙыу:',
'tog-highlightbroken' => 'Бәйләнешһеҙ һылтамаларҙы <a href="" class="new">ошолай</a> күрһәтергә (юҡһа былай<a href="" class="internal">?</a>).',
'tog-justify'         => 'Һөйләмдәр теҙмәһен бит киңлегенә тигеҙләргә',
'tog-hideminor'       => 'Әһәмиәте ҙур булмаған төҙәтеүҙәрҙе һуңғы үҙгәртеүҙәр исемлегендә күрһәтмәҫкә',
'tog-extendwatchlist' => 'Барлыҡ үҙгәртеүҙәрҙе үҙ эсенә алған, киңәйтелгән күҙәтеү исемлеге',
'tog-usenewrc'        => 'Һуңғы үҙгәртеүҙәрҙәрҙең сифатлыраҡ исемлеге (JavaScript)',
'tog-watchcreations'  => 'Мин төҙөгән биттәрҙе күҙәтеү исемлегенә яҙырға',

'underline-always' => 'Һәрваҡыт',

# Dates
'sunday'        => 'Йәкшәмбе',
'monday'        => 'Дүшәмбе',
'tuesday'       => 'Шишәмбе',
'wednesday'     => 'Шаршамбы',
'thursday'      => 'Кесеаҙна',
'friday'        => 'Йома',
'saturday'      => 'Шәмбе',
'sun'           => 'Йәкшәмбе',
'mon'           => 'Дүшәмбе',
'tue'           => 'Шишәмбе',
'wed'           => 'Шаршамбы',
'thu'           => 'Кесеаҙна',
'fri'           => 'Йома',
'sat'           => 'Шәмбе',
'january'       => 'Ғинуар (Һыуығай)',
'february'      => 'Февраль (Шаҡай)',
'march'         => 'Март (Буранай)',
'april'         => 'Апрель (Алағарай)',
'may_long'      => 'Май (Һабанай)',
'june'          => 'Июнь (Һөтай)',
'july'          => 'Июль (Майай)',
'august'        => 'Август (Урағай)',
'september'     => 'Сентябрь (Һарысай)',
'october'       => 'Октябрь (Ҡарасай)',
'november'      => 'Ноябрь (Ҡырпағай)',
'december'      => 'Декабрь (Аҡъюлай)',
'january-gen'   => 'Ғинуар (Һыуығай)',
'february-gen'  => 'Февраль (Шаҡай)',
'march-gen'     => 'Март (Буранай)',
'april-gen'     => 'Апрель (Алағарай)',
'may-gen'       => 'Май (Һабанай)',
'june-gen'      => 'Июнь (Һөтай)',
'july-gen'      => 'Июль (Майай)',
'august-gen'    => 'Август (Урағай)',
'september-gen' => 'Сентябрь (Һарысай)',
'october-gen'   => 'Октябрь (Ҡарасай)',
'november-gen'  => 'Ноябрь (Ҡырпағай)',
'december-gen'  => 'Декабрь (Аҡъюлай)',
'jan'           => 'Ғинуар (Һыуығай)',
'feb'           => 'Февраль (Шаҡай)',
'mar'           => 'Март (Буранай)',
'apr'           => 'Апрель (Алағарай)',
'may'           => 'Май (Һабанай)',
'jun'           => 'Июнь (Һөтай)',
'jul'           => 'Июль (Майай)',
'aug'           => 'Август (Урағай)',
'sep'           => 'Сентябрь (Һарысай)',
'oct'           => 'Октябрь (Ҡарасай)',
'nov'           => 'Ноябрь (Ҡырпағай)',
'dec'           => 'Декабрь (Аҡъюлай)',

'about'          => 'Тасуирлау',
'article'        => 'Мәҡәлә',
'newwindow'      => '(яңы биттә)',
'cancel'         => 'Бөтөрөргә',
'qbfind'         => 'Эҙләү',
'qbmyoptions'    => 'Көйләү',
'qbspecialpages' => 'Махсус биттәр',
'mypage'         => 'Шәхси бит',
'mytalk'         => 'Минең менән фекер алышыу',
'navigation'     => 'Төп йүнәлештәр',
'and'            => 'һәм',

'errorpagetitle'   => 'Хата',
'returnto'         => '$1 битенә ҡайтыу.',
'help'             => 'Белешмә',
'search'           => 'Эҙләү',
'searchbutton'     => 'Табыу',
'go'               => 'Күсеү',
'searcharticle'    => 'Күсеү',
'history'          => 'Тарих',
'history_short'    => 'Тарих',
'info_short'       => 'Мәғлүмәт',
'printableversion' => 'Ҡағыҙға баҫыу өлгөһө',
'permalink'        => 'Даими һылтау',
'edit'             => 'Үҙгәртергә',
'editthispage'     => 'Был мәҡәләне үҙгәртергә',
'delete'           => 'Юҡ  итергә',
'protect'          => 'Һаҡларға',
'talkpage'         => 'Фекер алышыу',
'specialpage'      => 'Ярҙамсы бит',
'articlepage'      => 'Мәҡәләне ҡарап сығырға',
'talk'             => 'Фекер алышыу',
'toolbox'          => 'Ярҙамсы йүнәлештәр',
'otherlanguages'   => 'Башҡа телдәрҙә',
'lastmodifiedat'   => 'Был биттең һуңғы тапҡыр үҙгәртелеү ваҡыты: $2, $1 .', # $1 date, $2 time
'jumpto'           => 'Унда күсергә:',
'jumptosearch'     => 'эҙләү',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{grammar:genitive|{{SITENAME}}}}-ның тасуирламаһы',
'aboutpage'            => 'Project:Тасуирлама',
'copyright'            => '$1 ярашлы эстәлеге менән һәр кем файҙалана ала.',
'currentevents'        => 'Ағымдағы ваҡиғалар',
'currentevents-url'    => 'Project:Ағымдағы ваҡиғалар',
'disclaimers'          => 'Яуаплылыҡтан баш тартыу',
'disclaimerpage'       => 'Project:Яуаплылыҡтан баш тартыу',
'edithelp'             => 'Мөхәрирләү белешмәһе',
'mainpage'             => 'Баш бит',
'mainpage-description' => 'Баш бит',
'portal'               => 'Берләшмә',
'portal-url'           => 'Project:Берләшмә ҡоро',
'privacy'              => 'Сер һаҡлау сәйәсәте',

'editsection' => 'үҙгәртергә',
'toc'         => 'Эстәлеге',
'showtoc'     => 'күрһәтергә',
'hidetoc'     => 'йәшерергә',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Мәҡәлә',
'nstab-user'      => 'Ҡатнашыусы',
'nstab-special'   => 'Ярҙамсы бит',
'nstab-mediawiki' => 'MediaWiki белдереүе',

# General errors
'error'           => 'Хата',
'badarticleerror' => 'Был биттә ундай ғәмәл үтәргә ярамай',
'badtitle'        => 'Ярамаған исем',

# Login and logout pages
'loginpagetitle'          => 'Танышыу йәки теркәлеү',
'yourname'                => 'Ҡатнашыусы исеме',
'yourpassword'            => 'Һеҙҙең пароль',
'yourpasswordagain'       => 'Парольде ҡабаттан яҙыу',
'remembermypassword'      => 'Парольде хәтерҙә ҡалдырырға',
'yourdomainname'          => 'Һеҙҙең домен',
'login'                   => 'Танышыу йәки теркәлеү',
'nav-login-createaccount' => 'Танышыу йәки теркәлеү',
'userlogin'               => 'Танышыу йәки теркәлеү',
'logout'                  => 'Тамамлау',
'userlogout'              => 'Тамамлау',
'nologin'                 => 'Һеҙ әле теркәлмәгәнме? $1.',
'nologinlink'             => 'Иҫәп яҙыуын булдырырға',
'createaccount'           => 'Яңы ҡатнашыусыны теркәү',
'gotaccount'              => 'Әгәр Һеҙ теркәлеү үткән булһағыҙ? $1.',
'gotaccountlink'          => 'Үҙегеҙ менән таныштырығыҙ',
'createaccountmail'       => 'эл. почта буйынса',
'youremail'               => 'Электрон почта *',
'yourrealname'            => 'Һеҙҙең ысын исемегеҙ (*)',
'yourlanguage'            => 'Тышҡы күренештә ҡулланылған тел:',
'yourvariant'             => 'Тел төрө',
'yournick'                => 'Һеҙҙең уйҙырма исемегеҙ/ҡушаматығыҙ (имза өсөн):',
'prefs-help-email'        => '* Электрон почта (күрһәтмәһәң дә була) башҡа ҡатнашыусылар менән туры бәйләнешкә инергә мөмкинселек бирә.',
'loginsuccesstitle'       => 'Танышыу уңышлы үтте',
'loginsuccess'            => 'Хәҙер һеҙ $1 исеме менән эшләйһегеҙ.',
'wrongpassword'           => 'Һеҙ ҡулланған пароль ҡабул ителмәй. Яңынан яҙып ҡарағыҙ.',
'mailmypassword'          => 'Яңы пароль ебәрергә',

# Edit pages
'summary'        => 'Үҙгәртеүҙең ҡыҫҡаса тасуирламаһы',
'minoredit'      => 'Әҙ генә үҙгәрештәр',
'watchthis'      => 'Был битте күҙәтеүҙәр исемлегенә индерергә',
'savearticle'    => 'Яҙҙырып ҡуйырға',
'preview'        => 'Ҡарап сығыу',
'showpreview'    => 'Ҡарап сығырға',
'showdiff'       => 'Индерелгән үҙгәрештәр',
'previewnote'    => '<strong>Ҡарап сығыу өлгөһө, әлегә үҙгәрештәр яҙҙырылмаған!</strong>',
'editing'        => 'Мөхәрирләү  $1',
'editingsection' => 'Мөхәрирләү  $1 (секция)',
'editingcomment' => 'Мөхәрирләү $1 (комментарий)',
'yourtext'       => 'Һеҙҙең текст',
'yourdiff'       => 'Айырмалыҡтар',

# Preferences page
'preferences' => 'Көйләүҙәр',

# User rights
'editinguser' => "Мөхәрирләү  '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",

# Groups
'group-all' => '(бөтә)',

# Recent changes
'recentchanges'     => 'Һуңғы үҙгәртеүҙәр',
'recentchangestext' => '{{grammar:genitive|{{SITENAME}}}}. биттәрендә индерелгән һуңғы үҙгәртеүҙәр исемлеге',

# Recent changes linked
'recentchangeslinked' => 'Бәйле үҙгәртеүҙәр',

# Special:ImageList
'imagelist_user' => 'Ҡатнашыусы',

# MIME search
'mimesearch' => 'MIME буйынса эҙләү',

# Unwatched pages
'unwatchedpages' => 'Бер кем дә күҙәтмәгән биттәр',

# Random page
'randompage' => 'Осраҡлы мәҡәлә',

# Statistics
'userstatstext' => "Бөтәһе '''$1''' ҡатнашыусы теркәлгән, шуларҙан '''$2''' ($4 %) хәким бурыстарын үтәй.",

# Miscellaneous special pages
'listusers'         => 'Ҡатнашыусылар исемлеге',
'newpages-username' => 'Ҡатнашыусы:',
'ancientpages'      => 'Иң иҫке мәҡәләләр',
'move'              => 'Яңы исем биреү',

# Special:Log
'specialloguserlabel' => 'Ҡатнашыусы:',

# Special:AllPages
'allpages'          => 'Бөтә биттәр',
'alphaindexline'    => '$1 алып $2 тиклем',
'allpagesfrom'      => 'Ошондай хәрефтәрҙән башланған биттәрҙе күрһәтергә:',
'allarticles'       => 'Бөтә мәҡәләләр',
'allinnamespace'    => 'Бөтә биттәр (Исемдәре «$1» арауығында)',
'allnotinnamespace' => 'Бөтә биттәр («$1» исемдәр арауығынан башҡа)',
'allpagesprev'      => 'Алдағы',
'allpagesnext'      => 'Киләһе',
'allpagessubmit'    => 'Үтәргә',

# E-mail user
'emailuser'    => 'Ҡатнашыусыға хат',
'emailfrom'    => 'Кемдән',
'emailto'      => 'Кемгә:',
'emailmessage' => 'Хәбәр',

# Watchlist
'watchlist'    => 'Күҙәтеү исемлеге',
'mywatchlist'  => 'Күҙәтеү исемлеге',
'watchnologin' => 'Үҙегеҙҙе танытырға кәрәк',
'addedwatch'   => 'Күҙәтеү исемлегенә өҫтәлде',
'watch'        => 'Күҙәтергә',
'unwatch'      => 'Күҙәтмәҫкә',
'notanarticle' => 'Мәҡәлә түгел',

'enotif_newpagetext' => 'Был яңы бит.',
'changed'            => 'үҙгәртелгән',

# Delete/protect/revert
'actioncomplete' => 'Ғәмәл үтәлде',

# Namespace form on various pages
'namespace'      => 'Исемдәр арауығы:',
'blanknamespace' => 'Мәҡәләләр',

# Contributions
'contributions' => 'Ҡатнашыусы өлөшө',
'mycontris'     => 'ҡылған эштәр',

# What links here
'whatlinkshere' => 'Бында һылтанмалар',

# Block/unblock
'blockip' => 'Ҡатнашыусыны ябыу',

# Namespace 8 related
'allmessagesname' => 'Хәбәр',

# Attribution
'siteuser'  => '{{grammar:genitive|{{SITENAME}}}} - ла ҡатнашыусы $1',
'siteusers' => '{{grammar:genitive|{{SITENAME}}}} - ла ҡатнашыусы (-лар) $1',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'бөтә',
'imagelistall'     => 'бөтә',
'watchlistall2'    => 'бөтә',
'namespacesall'    => 'бөтә',

# Special:SpecialPages
'specialpages' => 'Махсус биттәр',

);
