<?php
/** Kabyle (Taqbaylit)
 *
 * @ingroup Language
 * @file
 *
 * @author Agurzil
 * @author Agzennay
 * @author Teak
 */

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Uslig',
	NS_MAIN             => '',
	NS_TALK             => 'Mmeslay',
	NS_USER             => 'Amseqdac',
	NS_USER_TALK        => 'Amyannan_umsqedac',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Amyannan_n_$1',
	NS_IMAGE            => 'Tugna',
	NS_IMAGE_TALK       => 'Amyannan_n_tugna',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Amyannan_n_MediaWiki',
	NS_TEMPLATE         => 'Talɣa',
	NS_TEMPLATE_TALK    => 'Amyannan_n_talɣa',
	NS_HELP             => 'Tallat',
	NS_HELP_TALK        => 'Amyannan_n_tallat',
	NS_CATEGORY         => 'Taggayt',
	NS_CATEGORY_TALK    => 'Amyannan_n_taggayt'
);

$namespaceAliases = array(
	'Talγa'            => NS_TEMPLATE,
	'Amyannan_n_talγa' => NS_TEMPLATE_TALK,
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Derrer izdayen:',
'tog-highlightbroken'         => 'Ssken izdayen imerẓa <a href="" class="new">akkagi</a> (neɣ: akkagi<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Err tehri ger wawalen kif-kif',
'tog-hideminor'               => 'Ffer ibeddlen ifessasen deg yibeddlen imaynuten',
'tog-extendwatchlist'         => 'Ssemɣer umuɣ n uɛessi iwakken ad muqleɣ akk n wayen zemreɣ ad beddleɣ',
'tog-usenewrc'                => 'Sselhu ibeddlen ifessasen (JavaScript)',
'tog-numberheadings'          => 'Izwal ɣur-sen imḍanen mebla ma serseɣ-iten',
'tog-showtoolbar'             => 'Ssken tanuga n dduzan n ubeddel (JavaScript)',
'tog-editondblclick'          => 'Beddel isebtar mi wekkiɣ snat n tikwal (JavaScript)',
'tog-editsection'             => 'Eǧǧ abeddel n umur s yizdayen [beddel]',
'tog-editsectiononrightclick' => 'Eǧǧ abeddel n umur mi wekkiɣ ɣef uyeffus<br /> ɣef yizwal n umur (JavaScript)',
'tog-showtoc'                 => 'Ssken agbur (i isebtar i yesɛan kter n 3 izwalen)',
'tog-rememberpassword'        => 'Cfu ɣef yisem n wemseqdac inu di uselkim-agi',
'tog-editwidth'               => 'Tankult n ubeddel tesɛa tehri ettmam',
'tog-watchcreations'          => 'Rnu isebtar i xelqeɣ deg wumuɣ n uɛessi inu',
'tog-watchdefault'            => 'Rnu isebtar i ttbeddileɣ deg wumuɣ n uɛessi inu',
'tog-watchmoves'              => 'Rnu isebtar i smimḍeɣ deg wumuɣ n uɛessi inu',
'tog-watchdeletion'           => 'Rnu isebtar i mḥiɣ deg wumuɣ n uɛessi inu',
'tog-minordefault'            => 'Rcem akk ibeddlen am ibeddlen ifessasen d ameslugen',
'tog-previewontop'            => 'Ssken pre-timeẓriwt uqbel tankult ubeddel',
'tog-previewonfirst'          => 'Ssken pre-timeẓriwt akk d ubeddel amezwaru',
'tog-nocache'                 => 'Kkes lkac n usebter',
'tog-enotifwatchlistpages'    => 'Azen-iyi-d e-mail asmi yettubeddel asebter i ttɛassaɣ',
'tog-enotifusertalkpages'     => 'Azen-iyi-d e-mail asmi sɛiɣ izen amaynut',
'tog-enotifminoredits'        => 'Azen-iyi-d e-mail ma llan ibeddlen ifessasen',
'tog-enotifrevealaddr'        => 'Ssken e-mail inu asmi yettwazen email n talɣut',
'tog-shownumberswatching'     => 'Ssken geddac yellan n yimseqdacen iɛessasen',
'tog-fancysig'                => 'Eǧǧ azmul am yettili (mebla azday otomatik)',
'tog-externaleditor'          => 'Sseqdec ambeddel n berra d ameslugen',
'tog-externaldiff'            => 'Sseqdec ambeddel n berra iwakken ad ẓreɣ imgerraden',
'tog-showjumplinks'           => 'Eǧǧ izdayen "neggez ar"',
'tog-uselivepreview'          => 'Sseqdec pre-timeẓriwt taǧiḥbuṭ (JavaScript) (Experimental)',
'tog-forceeditsummary'        => 'Ini-iyi-d mi sskecmeɣ agzul amecluc',
'tog-watchlisthideown'        => 'Ffer ibeddlen inu seg wumuɣ n uɛessi inu',
'tog-watchlisthidebots'       => 'Ffer ibeddlen n iboṭiyen seg wumuɣ n uɛessi inu',
'tog-watchlisthideminor'      => 'Ffer ibeddlen ifessasen seg wumuɣ n uɛessi inu',
'tog-nolangconversion'        => 'Kkes abeddel n yimeskilen',
'tog-ccmeonemails'            => 'Azen-iyi-d email n wayen uzneɣ i imseqdacen wiyaḍ',
'tog-diffonly'                => 'Ur temliḍ-iyi-d ara ayen yellan seddaw imgerraden',

'underline-always'  => 'Daymen',
'underline-never'   => 'Abaden',
'underline-default' => 'Browser/Explorateur ameslugen',

'skinpreview' => '(Pre-timeẓriwt)',

# Dates
'sunday'        => 'Lḥedd',
'monday'        => 'Letnayen',
'tuesday'       => 'Ttlata',
'wednesday'     => 'Larebɛa',
'thursday'      => 'Lexmis',
'friday'        => 'Lǧemɛa',
'saturday'      => 'Ssebt',
'sun'           => 'Lḥedd',
'mon'           => 'Letnayen',
'tue'           => 'Ttlata',
'wed'           => 'Larebɛa',
'thu'           => 'Lexmis',
'fri'           => 'Lǧemɛa',
'sat'           => 'Ssebt',
'january'       => 'Yennayer',
'february'      => 'Furar',
'march'         => 'Meɣres',
'april'         => 'Ibrir',
'may_long'      => 'Mayu',
'june'          => 'Yunyu',
'july'          => 'Yulyu',
'august'        => 'Ɣuct',
'september'     => 'Ctember',
'october'       => 'Tuber',
'november'      => 'Wamber',
'december'      => 'Jember',
'january-gen'   => 'Yennayer',
'february-gen'  => 'Furar',
'march-gen'     => 'Meɣres',
'april-gen'     => 'Ibrir',
'may-gen'       => 'Mayu',
'june-gen'      => 'Yunyu',
'july-gen'      => 'Yulyu',
'august-gen'    => 'Ɣuct',
'september-gen' => 'Ctember',
'october-gen'   => 'Tuber',
'november-gen'  => 'Wamber',
'december-gen'  => 'Jember',
'jan'           => 'Yennayer',
'feb'           => 'Ibrir',
'mar'           => 'Meɣres',
'apr'           => 'Ibrir',
'may'           => 'Mayu',
'jun'           => 'Yunyu',
'jul'           => 'Yulyu',
'aug'           => 'Ɣuct',
'sep'           => 'Ctember',
'oct'           => 'Tuber',
'nov'           => 'Wamber',
'dec'           => 'Jember',

# Categories related messages
'pagecategories'        => '{{PLURAL:$1|Taggayt|Taggayin}}',
'category_header'       => 'Imagraden deg taggayt "$1"',
'subcategories'         => 'Taggayin tizellumin',
'category-media-header' => 'Media deg taggayt "$1"',
'category-empty'        => "''Taggayt-agi d tilemt.''",

'about'          => 'Awal ɣef...',
'article'        => 'Ayen yella deg usebter',
'newwindow'      => '(teldi deg ttaq amaynut)',
'cancel'         => 'Eǧǧ-it am yella',
'qbfind'         => 'Af',
'qbbrowse'       => 'Ẓer isebtar',
'qbedit'         => 'Beddel',
'qbpageoptions'  => 'Asebter-agi',
'qbpageinfo'     => 'Asatal',
'qbmyoptions'    => 'isebtar inu',
'qbspecialpages' => 'isebtar usligen',
'moredotdotdot'  => 'Ugar...',
'mypage'         => 'Asebter inu',
'mytalk'         => 'Amyannan inu',
'anontalk'       => 'Amyannan n IP-yagi',
'navigation'     => 'Ẓer isebtar',
'and'            => 'u',

'errorpagetitle'    => 'Agul',
'returnto'          => 'Uɣal ar $1.',
'tagline'           => 'Seg {{SITENAME}}',
'help'              => 'Tallat',
'search'            => 'Nadi',
'searchbutton'      => 'Nadi',
'go'                => 'Ẓer',
'searcharticle'     => 'Ẓer',
'history'           => 'Amezruy n usebter',
'history_short'     => 'Amezruy',
'updatedmarker'     => 'yettubeddel segmi tarzeft taneggarut inu',
'info_short'        => 'Talɣut',
'printableversion'  => 'Tasiwelt iwakken ad timprimiḍ',
'permalink'         => 'Azday ur yettbeddil ara',
'print'             => 'Imprimi',
'edit'              => 'Beddel',
'editthispage'      => 'Beddel asebter-agi',
'delete'            => 'Mḥu',
'deletethispage'    => 'Mḥu asebter-agi',
'undelete_short'    => 'Fakk amḥay n {{PLURAL:$1|yiwen ubeddel|$1 yibeddlen}}',
'protect'           => 'Ḥrez',
'protect_change'    => 'beddel tiḥḥerzi',
'protectthispage'   => 'Ḥrez asebter-agi',
'unprotect'         => 'fakk tiḥḥerzi',
'unprotectthispage' => 'Fakk tiḥḥerzi n usebter-agi',
'newpage'           => 'Asebter amaynut',
'talkpage'          => 'Mmeslay ɣef usebter-agi',
'talkpagelinktext'  => 'Mmeslay',
'specialpage'       => 'Asebter uslig',
'personaltools'     => 'Dduzan inu',
'postcomment'       => 'Azen awennit',
'articlepage'       => 'Ẓer ayen yellan deg usebter',
'talk'              => 'Amyannan',
'views'             => 'Tuẓrin',
'toolbox'           => 'Dduzan',
'userpage'          => 'Ẓer asebter n wemseqdac',
'projectpage'       => 'Ẓer asebter n usenfar',
'imagepage'         => 'Ẓer asebter n tugna',
'mediawikipage'     => 'Ẓer asebter n izen',
'templatepage'      => 'Ẓer asebter n talɣa',
'viewhelppage'      => 'Ẓer asebter n tallat',
'categorypage'      => 'Ẓer asebter n taggayin',
'viewtalkpage'      => 'Ẓer amyannan',
'otherlanguages'    => 'S tutlayin tiyaḍ',
'redirectedfrom'    => '(Yettusmimeḍ seg $1)',
'redirectpagesub'   => 'Asebter usemmimeḍ',
'lastmodifiedat'    => 'Tikkelt taneggarut i yettubeddel asebter-agi $2, $1.', # $1 date, $2 time
'viewcount'         => 'Asebter-agi yettwakcem {{PLURAL:$1|yiwet tikelt|$1 tikwal}}.',
'protectedpage'     => 'Asebter yettwaḥerzen',
'jumpto'            => 'Neggez ar:',
'jumptonavigation'  => 'ẓer isebtar',
'jumptosearch'      => 'anadi',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Awal ɣef {{SITENAME}}',
'aboutpage'            => 'Project:Awal ɣef...',
'bugreports'           => "In'aɣ ibugiyen (bug)",
'bugreportspage'       => "Project:In'aɣ ibugiyen",
'copyright'            => 'Tzemreḍ ad twaliḍ ayen yella deg $1.',
'copyrightpagename'    => 'Copyright n {{SITENAME}}',
'currentevents'        => 'Isallen',
'currentevents-url'    => 'Project:Isallen',
'disclaimers'          => 'Iɣtalen',
'disclaimerpage'       => 'Project:Iɣtalen',
'edithelp'             => 'Tallat deg ubeddel',
'edithelppage'         => 'Help:Abeddel',
'faq'                  => 'Isteqsiyen',
'faqpage'              => 'Project:Isteqsiyen',
'helppage'             => 'Help:Agbur',
'mainpage'             => 'Asebter amenzawi',
'mainpage-description' => 'Asebter amenzawi',
'portal'               => 'Awwur n timetti',
'portal-url'           => 'Project:Awwur n timetti',
'privacy'              => 'Tudert tusligt',
'privacypage'          => 'Project:Tudert tusligt',

'badaccess'        => 'Agul n turagt',
'badaccess-group0' => 'Ur tettalaseḍ ara ad texedmeḍ tigawt i tseqsiḍ.',
'badaccess-group1' => 'Tigawt i steqsiḍ, llan ala imseqdacen n adrum n $1 i zemren a t-xedmen.',
'badaccess-group2' => 'Tigawt i steqsiḍ, llan ala imseqdacen seg yiwen n yiderman n $1 i zemren a t-xedmen.',
'badaccess-groups' => 'Tigawt i steqsiḍ, llan ala imseqdacen seg yiwen n yiderman n $1 i zemren a t-xedmen.',

'versionrequired'     => 'Yessefk ad tesɛiḍ tasiwelt $1 n MediaWiki',
'versionrequiredtext' => 'Yessefk ad tesɛiḍ tasiwelt $1 n MediaWiki iwakken ad tesseqdceḍ asebter-agi. Ẓer [[Special:Version|tasiwelt n usebter]].',

'retrievedfrom'           => 'Yettwaddem seg "$1"',
'youhavenewmessages'      => 'Ɣur-k $1 ($2).',
'newmessageslink'         => 'Izen amaynut',
'newmessagesdifflink'     => 'Abeddel aneggaru',
'youhavenewmessagesmulti' => 'Tesɛiḍ iznan imaynuten deg $1',
'editsection'             => 'beddel',
'editold'                 => 'beddel',
'editsectionhint'         => 'Beddel amur: $1',
'toc'                     => 'Agbur',
'showtoc'                 => 'Ssken',
'hidetoc'                 => 'Ffer',
'thisisdeleted'           => 'Ẓer neɣ err $1 am yella?',
'viewdeleted'             => 'Ẓer $1?',
'restorelink'             => '{{PLURAL:$1|Yiwen abeddel yettumḥan|$1 Ibeddlen yettumḥan}}',
'feedlinks'               => 'Asuddem:',
'feed-invalid'            => 'Anaw n usuddem mačči ṣaḥiḥ.',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Amagrad',
'nstab-user'      => 'Asebter n wemseqdac',
'nstab-media'     => 'Asebter n media',
'nstab-special'   => 'Uslig',
'nstab-project'   => 'Awal ɣef...',
'nstab-image'     => 'Afaylu',
'nstab-mediawiki' => 'Izen',
'nstab-template'  => 'Talɣa',
'nstab-help'      => 'Tallat',
'nstab-category'  => 'Taggayt',

# Main script and global functions
'nosuchaction'      => 'Tigawt ulac-itt',
'nosuchactiontext'  => 'Wiki ur teɛqil ara tigawt-nni n URL',
'nosuchspecialpage' => 'Asebter uslig am wagi ulac-it.',
'nospecialpagetext' => 'Tseqsiḍ ɣef usebter uslig ulac-it, tzemreḍ ad tafeḍ isebtar usligen n ṣṣeḥ deg [[Special:SpecialPages|wumuɣ n isebtar usligen]].',

# General errors
'error'                => 'Agul',
'databaseerror'        => 'Agul n database',
'dberrortext'          => 'Yella ugul n tseddast deg database.
Waqila yella bug deg software.
Query n database taneggarut hatt::
<blockquote><tt>$1</tt></blockquote>
seg tawuri  "<tt>$2</tt>".
MySQL yerra-d agul "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Yella ugul n tseddast deg database.
Query n database taneggarut hatt:
"$1"
seg tawuri "$2".
MySQL yerra-d agul "$3: $4"',
'noconnect'            => 'Suref-aɣ! Wiki-yagi tesɛa igna, ur tezmir ara ad tmeslay akk d database. <br />
$1',
'nodb'                 => 'Ur yezmir ara ad yextar database $1',
'cachederror'          => 'Wagi d alsaru n lkac n usebter, waqila ur yesɛi ara akk ibeddlen imaynuten.',
'laggedslavemode'      => 'Aɣtal: Ahat asebter ur yesɛi ara akk ibeddlen imaynuten.',
'readonly'             => 'Database d tamsekkert',
'enterlockreason'      => 'Ini ayɣer tsekkreḍ database, ini daɣen melmi ara ad ifukk asekker',
'readonlytext'         => 'Database d tamsekkert, ahat tettuseggem, qrib ad tuɣal-d.

Win (anedbal) isekker-itt yenna-d: $1',
'readonly_lag'         => 'Database d tamsekkert (weḥdes) axaṭer kra n serveur ɛeṭṭlen',
'internalerror'        => 'Agul zdaxel',
'filecopyerror'        => 'Ur yezmir ara ad yexdem alsaru n ufaylu "$1" ar "$2".',
'filerenameerror'      => 'Ur yezmir ara ad ibeddel isem ufaylu "$1" ar "$2".',
'filedeleteerror'      => 'Ur yezmir ara ad yemḥu afaylu "$1".',
'filenotfound'         => 'Ur yezmir ara ad yaf afaylu "$1".',
'unexpected'           => 'Agul: "$1"="$2".',
'formerror'            => 'Agul: ur yezmir ara ad yazen talɣa',
'badarticleerror'      => 'Ur yezmir ara ad yexdem tigawt-agi deg usebter-agi.',
'cannotdelete'         => 'Ur yezmir ara ad yemḥu asebter neɣ afaylu i tebɣiḍ. (Ahat amdan wayeḍ yemḥa-t.)',
'badtitle'             => 'Azwel ur yelhi',
'badtitletext'         => 'Asebter i testeqsiḍ fell-as mačči ṣaḥiḥ, d ilem, neɣ yella ugul deg wezday seg wikipedia s tutlayt tayeḍ neɣ deg wezday n wiki nniḍen. Ahat tesɛa asekkil ur yezmir ara ad yettuseqdac deg wezwel.',
'perfdisabled'         => 'Suref-aɣ! aḍaɣar-agi ur yettuseqdac ara tura axaṭer iɛeṭṭel aṭas database.',
'perfcached'           => 'Talɣut deg ukessar seg lkac u waqila mačči d tasiwelt taneggarut.',
'perfcachedts'         => 'Talɣut deg ukessar seg lkac, tasiwelt taneggarut n wass $1.',
'querypage-no-updates' => 'Ibeddlen n usebter-agi ur ttbanen ara tura. Tilɣa ines qrib a d-banen.',
'wrong_wfQuery_params' => 'Imsektayen mačči ṣaḥiḥ deg wfQuery()<br />
Tawuri: $1<br />
Query: $2',
'viewsource'           => 'Ẓer aɣbalu',
'viewsourcefor'        => 'n $1',
'protectedpagetext'    => 'Asebter-agi d amsekker.',
'viewsourcetext'       => 'Tzemreḍ ad twaliḍ u txedmeḍ alsaru n uɣbalu n usebter-agi:',
'protectedinterface'   => 'Asebter-agi d amsekker axaṭer yettuseqdac i weḍris n software.',
'editinginterface'     => "'''Aɣtal:''' Aqla-k tettbeddileḍ asebter i yettuseqdac i weḍris n software. Tagmett n software i tt-ẓren yimseqdacen wiyaḍ ad tbeddel akk d ibeddlen inek.",
'sqlhidden'            => '(Query n SQL tettwaffer)',
'cascadeprotected'     => 'Asebter-agi yettwaḥrez seg ubeddil, axaṭer yettusekcem deg isebtar i ttwaḥerzen ula d nutni (acercur), ahaten:',

# Login and logout pages
'logouttitle'                => 'Tuffɣa',
'logouttext'                 => '<strong>Tura teffɣeḍ.</strong><br />
Tzemreḍ ad tesseqdceḍ {{SITENAME}} d udrig, neɣ tzemreḍ ad tkecmeḍ daɣen s yisem n wemseqdac inek (neɣ nniḍen). Kra n isebtar zemren ad sskanen belli mazal-ik s yisem n wemseqdac inek armi temḥuḍ lkac.',
'welcomecreation'            => '== Anṣuf yis-k, $1! ==

Isem n wemseqdac inek yettwaxleq. Ur tettuḍ ara ad tbeddleḍ Isemyifiyen n {{SITENAME}} inek.',
'loginpagetitle'             => 'Takcemt',
'yourname'                   => 'Isem n wemseqdac',
'yourpassword'               => 'Awal n tbaḍnit',
'yourpasswordagain'          => 'Ɛiwed ssekcem awal n tbaḍnit',
'remembermypassword'         => 'Cfu ɣef wawal n tbaḍnit inu di uselkim-agi.',
'yourdomainname'             => 'Taɣult inek',
'externaldberror'            => 'Yella ugul aberrani n database neɣ ur tettalaseḍ ara ad tbeddleḍ isem an wemseqdac aberrani inek.',
'loginproblem'               => '<b>Yella ugur akk d ukcam inek.</b><br />Ɛreḍ daɣen!',
'login'                      => 'Kcem',
'nav-login-createaccount'    => 'Kcem / Xleq isem n wemseqdac',
'loginprompt'                => 'Yessefk ad teǧǧiḍ ikukiyen (cookies) iwakken ad tkecmeḍ ar {{SITENAME}}.',
'userlogin'                  => 'Kcem / Xleq isem n wemseqdac',
'logout'                     => 'Ffeɣ',
'userlogout'                 => 'Ffeɣ',
'notloggedin'                => 'Ur tekcimeḍ ara',
'nologin'                    => 'Ur tesɛiḍ ara isem n wemseqdac? $1.',
'nologinlink'                => 'Xleq isem n wemseqdac',
'createaccount'              => 'Xleq isem n wemseqdac',
'gotaccount'                 => 'Tesɛiḍ yagi isem n wemseqdac? $1.',
'gotaccountlink'             => 'Kcem',
'createaccountmail'          => 's e-mail',
'badretype'                  => 'Awal n tbaḍnit amezwaru d wis sin mačči d kif-kif.',
'userexists'                 => 'Isem n wemseqdac yeddem-as amdan wayeḍ. Fren yiwen nniḍen.',
'youremail'                  => 'E-mail *:',
'username'                   => 'Isem n wemseqdac:',
'uid'                        => 'Amseqdac ID:',
'yourrealname'               => 'Isem n ṣṣeḥ *:',
'yourlanguage'               => 'Tutlayt:',
'yourvariant'                => 'Ameskil:',
'yournick'                   => 'Isem wis sin (mačči d amenṣib):',
'badsig'                     => 'Azmul mačči d ṣaḥiḥ; Ssenqed tags n HTML.',
'prefs-help-realname'        => '* Isem n ṣṣeḥ (am tebɣiḍ): ma textareḍ a t-tefkeḍ, ad yettuseqdac iwakken ad snen medden anwa yura tikkin inek.',
'loginerror'                 => 'Agul n ukcam',
'prefs-help-email'           => '* E-mail (am tebɣiḍ): Teǧǧi imseqdacen wiyaḍ a k-aznen email mebla ma ẓren tansa email inek.',
'nocookiesnew'               => 'Isem n wemseqdac-agi yettwaxleq, meɛna ur tekcimeḍ ara. {{SITENAME}} yesseqdac ikukiyen (cookies) iwakken ad tkecmeḍ. Tekseḍ ikukiyen-nni. Eǧǧ-aten, umbeɛd kecm s yisem n wemseqdac akk d wawal n tbaḍnit inek.',
'nocookieslogin'             => '{{SITENAME}} yesseqdac ikukiyen (cookies) iwakken ad tkecmeḍ. Tekseḍ ikukiyen-nni. Eǧǧ-aten iwakken ad tkecmeḍ.',
'noname'                     => 'Ur tefkiḍ ara isem n wemseqdac ṣaḥiḥ.',
'loginsuccesstitle'          => 'Tkecmeḍ !',
'loginsuccess'               => "'''Tkecmeḍ ar {{SITENAME}} s yisem n wemseqdac \"\$1\".'''",
'nosuchuser'                 => 'Ulac isem n wemseqdac s yisem "$1". Ssenqed tira n yisem-nni, neɣ xelq isem n wemseqdac amaynut.',
'nosuchusershort'            => 'Ulac isem n wemseqdac s yisem "<nowiki>$1</nowiki>". Ssenqed tira n yisem-nni.',
'nouserspecified'            => 'Yessefk ad tefkeḍ isem n wemseqdac.',
'wrongpassword'              => 'Awal n tbaḍnit ɣaleṭ. Ɛreḍ daɣen.',
'wrongpasswordempty'         => 'Awal n tbaḍnit ulac-it. Ɛreḍ daɣen.',
'passwordtooshort'           => 'Awal n tbaḍnit inek d amecṭuḥ bezzaf. Yessefk ad yesɛu $1 isekkilen neɣ kter.',
'mailmypassword'             => 'Awal n tbaḍnit n e-mail',
'passwordremindertitle'      => 'Asmekti n wawal n tbaḍnit seg {{SITENAME}}',
'passwordremindertext'       => 'Amdan (waqila d kečč, seg tansa IP $1)
yesteqsa iwakken a nazen awal n tbaḍnit amaynut i {{SITENAME}} ($4).
Awal n tbaḍnit i wemseqdac "$2" yuɣal-d tura "$3".
Mliḥ lukan tkecmeḍ u tbeddleḍ awal n tbaḍnit tura.

Lukan mačči d kečč i yesteqsan neɣ tecfiḍ ɣef awal n tbaḍnit, tzemreḍ ad tkemmleḍ mebla ma tbeddleḍ awal n tbaḍnit.',
'noemail'                    => '"$1" ur yesɛi ara email.',
'passwordsent'               => 'Awal n tbaḍnit amaynut yettwazen i emal inek, aylaw n "$1".
G leɛnaya-k, kcem tikelt nniḍen yis-s.',
'blocked-mailpassword'       => 'Tansa n IP inek tɛekkel, ur tezmireḍ ara ad txedmeḍ abeddel,
ur tezmireḍ ara ad tesɛuḍ awal n tbaḍnit i tettuḍ.',
'eauthentsent'               => 'Yiwen e-mail yettwazen-ak iwakken ad tsenteḍ.
Qbel kulci, ḍfer ayen yenn-ak deg e-mail,
iwakken ad tbeyyneḍ belli tansa n email inek.',
'throttled-mailpassword'     => 'Asmekti n wawal n tbaḍnit yettwazen yagi deg $1 sswayeɛ i iɛeddan. Asmekti n wawal n tbaḍnit yettwazen tikelt kan mkul $1 swayeɛ.',
'mailerror'                  => 'Agul asmi yettwazen e-mail: $1',
'acct_creation_throttle_hit' => 'Surf-aɣ, txelqeḍ aṭas n yismawen n wemseqdac ($1). Ur tettalaseḍ ara ad txelqeḍ kter.',
'emailauthenticated'         => 'Tansa e-mail inek tettuɛqel deg $1.',
'emailnotauthenticated'      => 'Tansa e-mail inek mazal ur tettuɛqel. Ḥedd e-mail ur ttwazen i ulaḥedd n iḍaɣaren-agi.',
'noemailprefs'               => 'Efk tansa e-mail iwakken ad leḥḥun iḍaɣaren-nni.',
'emailconfirmlink'           => 'Sentem tansa e-mail inek',
'invalidemailaddress'        => 'Tansa e-mail-agi ur telhi, ur tesɛi ara taseddast n lɛali. Ssekcem tansa e-mail s taseddast n lɛali neɣ ur tefkiḍ acemma.',
'accountcreated'             => 'Isem n wemseqdac yettwaxleq',
'accountcreatedtext'         => 'Isem n wemseqdac i $1 yettwaxleq.',
'loginlanguagelabel'         => 'Tutlayt: $1',

# Password reset dialog
'resetpass'               => 'Iɛawed awal n tbaḍnit',
'resetpass_announce'      => 'Tkecmeḍ s ungal yettwazen-ak s e-mail (ungal-nni qrib yemmut). Iwekken tkemmleḍ, yessefk ad textareḍ awal n tbaḍnit amaynut dagi:',
'resetpass_text'          => '<!-- Rnu aḍris dagi -->',
'resetpass_header'        => 'Ɛiwed awal n tbaḍnit',
'resetpass_submit'        => 'Eg awal n tbaḍnit u kcem',
'resetpass_success'       => 'Awal n tbaḍnit yettubeddel! Qrib ad tkecmeḍ...',
'resetpass_bad_temporary' => 'Ungal mačči d ṣaḥiḥ. Ahat tbeddleḍ awal n tbaḍnit inek neɣ tetseqsiḍ ɣef wawal n tbaḍnit amaynut.',
'resetpass_forbidden'     => 'Ur tezmireḍ ara ad tbeddleḍ awal n tbaḍnit deg wiki-yagi',
'resetpass_missing'       => 'Ulac talɣut.',

# Edit page toolbar
'bold_sample'     => 'Aḍris aberbuz',
'bold_tip'        => 'Aḍris aberbuz',
'italic_sample'   => 'Aḍris aṭalyani',
'italic_tip'      => 'Aḍris aṭalyani',
'link_sample'     => 'Azwel n uzday',
'link_tip'        => 'Azday zdaxel',
'extlink_sample'  => 'http://www.example.com azwel n uzday',
'extlink_tip'     => 'Azday aberrani (cfu belli yessefk at tebduḍ s http://)',
'headline_sample' => 'Aḍris n uzwel azellum',
'headline_tip'    => 'Aswir 2 n uzwel azellum',
'math_sample'     => 'Ssekcem tasemselt dagi',
'math_tip'        => 'Tasemselt tusnakt (LaTeX)',
'nowiki_sample'   => 'Sideff da tirra bla taseddast(formatting) n wiki',
'nowiki_tip'      => 'Ttu taseddast n wiki',
'image_sample'    => 'Amedya.jpg',
'image_tip'       => 'Tugna yettussekcmen',
'media_sample'    => 'Amedya.ogg',
'media_tip'       => 'Azday n ufaylu media',
'sig_tip'         => 'Azmul inek s uzemz',
'hr_tip'          => 'Ajerriḍ aglawan (ur teččerɛiḍ ara)',

# Edit pages
'summary'                   => 'Agzul',
'subject'                   => 'Asentel/Azwel azellum',
'minoredit'                 => 'Wagi d abeddel afessas',
'watchthis'                 => 'Ɛass asebter-agi',
'savearticle'               => 'Beddel asebter',
'preview'                   => 'Pre-Ẓer',
'showpreview'               => 'Ssken pre-timeẓriwt',
'showlivepreview'           => 'Pre-timeẓriwt taǧiḥbuṭ',
'showdiff'                  => 'Ssken ibeddlen',
'anoneditwarning'           => "'''Aɣtal:''' Ur tkecmiḍ ara. Tansa IP inek ad tettusmekti deg umezruy n usebter-agi.",
'missingsummary'            => "'''Ur tettuḍ ara:''' Ur tefkiḍ ara azwel i ubeddel inek. Lukan twekkiḍ ''Smekti'' tikelt nniḍen, abeddel inek ad yettusmekti mebla azwel.",
'missingcommenttext'        => 'Ssekcem awennit deg ukessar.',
'missingcommentheader'      => "'''Ur tettuḍ ara:''' Ur tefkiḍ ara azwel-azellum i ubeddel inek. Lukan twekkiḍ ''Smekti'' tikelt nniḍen, abeddel inek ad yettusmekti mebla azwel-azellum.",
'summary-preview'           => 'Pre-timeẓriwt n ugzul',
'subject-preview'           => 'Pre-timeẓriwt asentel/azwel azellum',
'blockedtitle'              => 'Amseqdac iɛekkel',
'blockedtext'               => "<big>'''Isem n wemseqdac neɣ tansa n IP inek ɛekkelen.'''</big>

$1 iɛekkel-it u yenna-d ''$2''.

Tzemreḍ ad tmeslayeḍ akk d $1 neɣ [[{{MediaWiki:Grouppage-sysop}}|anedbal]] nniḍen iwakken ad tsmelayem ɣef uɛekkil-nni.
Lukan ur tefkiḍ ara email saḥih deg [[Special:Preferences|isemyifiyen n wemseqdac]], ur tezmireḍ ara ad tazneḍ email. Tansa n IP inek n tura d $3, ID n uɛekkil d #$5. Smekti-ten u fka-ten i unedbal-nni.",
'blockedoriginalsource'     => "Aɣablu n '''$1''' hat deg ukessar:",
'blockededitsource'         => "Aḍris n '''ubeddel inek''' i '''$1''' hat deg ukessar:",
'whitelistedittitle'        => 'Yessefk ad tkecmeḍ iwakken ad tbeddleḍ',
'whitelistedittext'         => 'Yessefk ad $1 iwakken ad tbeddleḍ isebtar.',
'confirmedittitle'          => 'Yessef ad tsentmeḍ e-mail inek iwakken ad tbeddleḍ',
'confirmedittext'           => 'Yessefk ad tsentmeḍ tansa e-mail inek uqbel abeddel. Xtar tansa e-mail di [[Special:Preferences|isemyifiyen n wemseqdac]].',
'nosuchsectiontitle'        => 'Amur ulac-it',
'nosuchsectiontext'         => 'Tɛerḍeḍ ad tbeddleḍ amur ulac-it. Ulac amur am akka deg usebter $1.',
'loginreqtitle'             => 'Yessefk ad tkecmeḍ',
'loginreqlink'              => 'Kcem',
'loginreqpagetext'          => 'Yessefk $1 iwakken ad teẓriḍ isebtar wiyaḍ.',
'accmailtitle'              => 'Awal n tbaḍnit yettwazen.',
'accmailtext'               => 'Awal n tbaḍnit n "$1" yettwazen ar $2.',
'newarticle'                => '(Amaynut)',
'newarticletext'            => 'Tḍefreḍ azday ɣer usebter mazal ur yettwaxleq ara.
Akken ad txelqeḍ asebter-nni, aru deg tenkult i tella deg ukessar
(ẓer [[{{MediaWiki:Helppage}}|asebter n tallat]] akken ad tessneḍ kter).
Ma tɣelṭeḍ, wekki kan ɣef tqeffalt "Back/Précédent" n browser/explorateur inek.',
'anontalkpagetext'          => "----''Wagi d asebter n umyennan n wemseqdac adrig. Ihi, yessef ad as nefk ID, nesseqdac tansa IP ines akken a t-neɛqel. Tansa IP nni ahat tettuseqdac sɣur aṭṭas n yimdanen. Lukan ula d kečč aqla-k amseqdac adrig u ur tebɣiḍ ara ad tettwabcreḍ izen am wigini, ihi [[Special:UserLogin|xleq isem n wemseqdac neɣ kcem]].''",
'noarticletext'             => 'Ulac aḍris deg usebter-agi, tzemreḍ ad [[Special:Search/{{PAGENAME}}|tnadiḍ ɣef wezwel n usebter-agi]] deg isebtar wiyaḍ neɣ [{{fullurl:{{FULLPAGENAME}}|action=edit}} tettbeddileḍ asebter-agi].',
'clearyourcache'            => "'''Tamawt:''' Beɛd asmekti, ahat yessefk ad temḥuḍ lkac n browser/explorateur inek akken teẓriḍ ibeddlen. '''Mozilla / Firefox / Safari:''' qqim twekkiḍ ''Shift'' u wekki ɣef ''Reload/Recharger'', neɣ wekki ɣef ''Ctrl-Shift-R'' (''Cmd-Shift-R'' deg Apple Mac); '''IE:''' qqim twekkiḍ ɣef ''Ctrl'' u wekki ɣef ''Refresh/Actualiser'', neɣ wekki ɣef ''Ctrl-F5''; '''Konqueror:''': wekki kan ɣef taqeffalt ''Reload'', neɣ wekki ɣef ''F5''; '''Opera''' yessefk ad tesseqdceḍ ''Tools→Preferences/Outils→Préférences'' akken ad temḥud akk lkac.",
'usercssjsyoucanpreview'    => "<strong>Tixidest:</strong> Sseqdec taqeffalt 'Ssken pre-timeẓriwt' iwakken ad tɛerḍeḍ CSS/JS amynut inek uqbel ad tesmektiḍ.",
'usercsspreview'            => "'''Smekti belli aql-ak twaliḍ CSS inek kan, mazal ur yettusmekti ara!'''",
'userjspreview'             => "'''Smekti belli aql-ak tɛerḍeḍ JavaScript inek kan, mazal ur yettusmekti ara!'''",
'userinvalidcssjstitle'     => '\'\'\'Aɣtal:\'\'\' Aglim "$1" ulac-it. Ur tettuḍ ara belli isebtar ".css" d ".js" i txedmeḍ sseqdacen azwel i yesɛan isekkilen imecṭuḥen, s umedya: {{ns:user}}:Foo/monobook.css akk d {{ns:user}}:Foo/Monobook.css.',
'updated'                   => '(Yettubeddel)',
'note'                      => '<strong>Tamawt:</strong>',
'previewnote'               => '<strong>Tagi d pre-timeẓriwt kan, ibeddlen mazal ur ttusmektin ara!</strong>',
'previewconflict'           => 'Pre-timeẓriwt-agi tesskan aḍris i yellan deg usawen lemmer tebɣiḍ a tt-tesmektiḍ.',
'session_fail_preview'      => '<strong>Suref-aɣ! ur nezmir ara a nesmekti abeddil inek axaṭer yella ugur.
G leɛnayek ɛreḍ tikelt nniḍen. Lukan mazal yella ugur, ffeɣ umbeɛd kcem.</strong>',
'session_fail_preview_html' => "<strong>Suref-aɣ! ur nezmir ara a nesmekti abeddel inek axaṭer yella ugur.</strong>

''Awaṭer wiki-yagi teǧǧa HTML, teffer pre-timeẓriwt akken teǧǧanez antag n JavaScript.''

<strong>Lukan abeddel agi d aḥeqqani, g leɛnayek ɛreḍ tikelt nniḍen.. Lukan mazal yella ugur, ffeɣ umbeɛd kcem.</strong>",
'editing'                   => 'Abeddel n $1',
'editingsection'            => 'Abeddel n $1 (amur)',
'editingcomment'            => 'Abeddel n $1 (awennit)',
'editconflict'              => 'Amennuɣ deg ubeddel: $1',
'explainconflict'           => "Amdan nniḍen ibeddel asebter-agi asmi telliḍ tettbeddileḍ.
Aḍris deg usawen yesɛa asebter am yella tura.
Ibeddlen inek ahaten deg ukessar.
Yesfek ad txelṭeḍ ibeddlen inek akk d usebter i yellan.
'''Ala''' aḍris deg usawen i yettusmekta asmi twekkiḍ \"Smekti asebter\".",
'yourtext'                  => 'Aḍris inek',
'storedversion'             => 'Tasiwelt yettusmketen',
'nonunicodebrowser'         => '<strong>AƔTAL: Browser/Explorateur inek ur yebil ara unicode. Nexdem akken ad tzemreḍ ad tbeddleḍ mebla amihi: isekkilin i mačči ASCII ttbanen deg tankult ubeddel s ungilen hexadecimal.</strong>',
'editingold'                => '<strong>AƔTAL: Aqlak tettbeddileḍ tasiwelt taqdimt n usebter-agi.
Ma ara t-tesmektiḍ, akk ibeddlen i yexdmen seg tasiwelt-agi ruḥen.</strong>',
'yourdiff'                  => 'Imgerraden',
'copyrightwarning'          => 'Ssen belli akk tikkin deg {{SITENAME}} hatent ttwaznen seddaw $2 (Ẓer $1 akken ad tessneḍ kter). Lukan ur tebɣiḍ ara aru inek yettubeddel neɣ yettwazen u yettwaru deg imkanen nniḍen, ihi ur t-tazneḍ ara dagi.<br />
Aqlak teggaleḍ belli tureḍ wagi d kečč, neɣ teddmiḍ-t seg taɣult azayez neɣ iɣbula tilelliyin.
<strong>UR TEFKIḌ ARA AXDAM S COPYRIGHT MEBLA TURAGT!</strong>',
'copyrightwarning2'         => 'Ssen belli akk tikkin deg {{SITENAME}} zemren ad ttubeddlen neɣ ttumḥan sɣur imdanen wiyaḍ. Lukan ur tebɣiḍ ara aru inek yettubeddel neɣ yettwazen u yettwaru deg imkanen nniḍen, ihi ur t-tazneḍ ara dagi.<br />
Aqlak teggaleḍ belli tureḍ wagi d kečč, neɣ teddmiḍ-t seg taɣult azayez neɣ iɣbula tilelliyin (ẓer $1 akken ad tessneḍ kter).
<strong>UR TEFKIḌ ARA AXDAM S COPYRIGHT MEBLA TURAGT!</strong>',
'longpagewarning'           => '<strong>AƔTAL: Asebter-agi yesɛa $1 kilobytes/kilooctets; kra n browsers/explorateur ur zemren ara ad beddlen isebtar i yesɛan 32kB/ko neɣ kter.
G leɛnayek frec asebter-nni.</strong>',
'longpageerror'             => '<strong>AGUL: Aḍris i tefkiḍ yesɛa $1 kB/ko, tiddi-yagi kter n $2 kB/ko, ur yezmir ara ad yesmekti.</strong>',
'readonlywarning'           => '<strong>AƔTAL: Database d tamsekker akken ad teddwaxdem,
ihi ur tezmireḍ ara ad tesmektiḍ ibeddlen inek tura. Smekti aḍris inek
deg afaylu nniḍen akken tesseqdceḍ-it umbeɛd.</strong>',
'protectedpagewarning'      => '<strong>AƔTAL:  Asebter-agi yettwaḥrez, ala inedbalen zemren a t-beddlen</strong>',
'semiprotectedpagewarning'  => "'''Tamawt:''' Asebter-agi yettwaḥrez, ala imseqdacen i yesɛan isem n wemseqdac zemren a t-beddlen.",
'cascadeprotectedwarning'   => "'''Aɣtal:''' Asebter-agi iɛekkel iwakken ad zemren ala inedbalen a t-beddlen, axaṭer yettwassekcem deg isebtar i yettwaḥerzen agi (acercur):",
'templatesused'             => 'Talɣiwin ttuseqdacen deg usebter-agi:',
'templatesusedpreview'      => 'Talɣiwin ttuseqdacen deg pre-timeẓriwt-agi:',
'templatesusedsection'      => 'Talɣiwin ttuseqdacen deg amur-agi:',
'template-protected'        => '(yettwaḥrez)',
'template-semiprotected'    => '(nnefṣ-yettwaḥrez)',
'edittools'                 => '<!-- Aḍris yettbanen-d seddaw talɣa n ubeddil d uzen. -->',
'nocreatetitle'             => 'Axleq n isebtar meḥdud',
'nocreatetext'              => 'Adeg n internet agi iḥedded axleq n isebtar imaynuten.
Tzemreḍ a d-uɣaleḍ u tbeddleḍ asebter i yellan, neɣ ad [[Special:UserLogin|tkecmeḍ neɣ ad txelqeḍ isem n wemseqdac]].',

# "Undo" feature
'undo-success' => 'Tzemreḍ ad tessefsuḍ abeddil. Ssenqed asidmer akken ad tessneḍ ayen tebɣiḍ ad txdmeḍ d ṣṣeḥ, umbeɛd smekti ibeddlen u tkemmleḍ ad tessefsuḍ abeddil.',
'undo-failure' => 'Ur yezmir ara ad issefu abeddel axaṭer yella amennuɣ abusari deg ubeddel.',
'undo-summary' => 'Ssefsu tasiwelt $1 sɣur [[Special:Contributions/$2|$2]] ([[User talk:$2|Meslay]])',

# Account creation failure
'cantcreateaccounttitle' => 'Ur yezmir ara ad yexleq isem n wemseqdac',

# History pages
'viewpagelogs'        => 'Ẓer aɣmis n usebter-agi',
'nohistory'           => 'Ulac amezruy n yibeddlen i usebter-agi.',
'revnotfound'         => 'Ur yezmir ara ad yaf tasiwelt',
'revnotfoundtext'     => 'Tasiwelt taqdimt n usebter-agi i testeqsiḍ ulac-it.
Ssenqed URL i tesseqdac.',
'currentrev'          => 'Tasiwelt n tura',
'revisionasof'        => 'Tasiwelt n wass $1',
'revision-info'       => 'Tasiwelt n wass $1 sɣur $2',
'previousrevision'    => '←Tasiwelt taqdimt',
'nextrevision'        => 'Tasiwelt tamaynut→',
'currentrevisionlink' => 'Tasiwelt n tura',
'cur'                 => 'tura',
'next'                => 'ameḍfir',
'last'                => 'amgirred',
'page_first'          => 'amezwaru',
'page_last'           => 'aneggaru',
'histlegend'          => 'Axtiri n umgerrad: rcem tankulin akken ad teẓreḍ imgerraden ger tisiwal u wekki ɣef enter/entrée neɣ ɣef taqeffalt deg ukessar.<br />
Tabadut: (tura) = amgirred akk d tasiwelt n tura,
(amgirred) = amgirred akk d tasiwelt ssabeq, M = abeddel afessas.',
'deletedrev'          => '[yettumḥa]',
'histfirst'           => 'Tikkin timezwura',
'histlast'            => 'Tikkin tineggura',
'historysize'         => '($1 bytes/octets)',
'historyempty'        => '(amecluc)',

# Revision feed
'history-feed-title'          => 'Amezruy n tsiwelt',
'history-feed-description'    => 'Amezruy n tsiwelt n usebter-agi deg wiki',
'history-feed-item-nocomment' => '$1 deg $2', # user at time
'history-feed-empty'          => 'Asebter i tebɣiḍ ulac-it.
Ahat yettumḥa neɣ yettbeddel isem-is.
Ɛreḍ [[Special:Search|ad tnadiḍ deg wiki]] ɣef isebtar imaynuten.',

# Revision deletion
'rev-deleted-comment'         => '(awennit yettwakes)',
'rev-deleted-user'            => '(isem n wemseqdac yettwakes)',
'rev-deleted-event'           => '(asekcem yettwakkes)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Tasiwelt-agi n tettwakkes seg weɣbar azayez.
Waqila yella kter n talɣut deg [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} aɣmis n umḥay].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Tasiwelt-agi n tettwakkes seg weɣbar azayez.
Kečč d anedbal, tzemreḍ a t-twaliḍ
Waqila yella kter n talɣut [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} aɣmis n umḥay].
</div>',
'rev-delundel'                => 'ssken/ffer',
'revisiondelete'              => 'Mḥu/kkes amḥay tisiwal',
'revdelete-nooldid-title'     => 'Ulac nnican i tasiwelt',
'revdelete-nooldid-text'      => 'Ur textareḍ ara tasiwelt nnican akken ad txedmeḍ tawuri fell-as.',
'revdelete-selected'          => "{{PLURAL:$2|Tasiwelt tettwafren|Tisiwal ttwafernen}} n '''$1:'''",
'logdelete-selected'          => '{{PLURAL:$1|Tamirt n uɣmis tettwafren|Isallen n uɣmis ttwafernen}}:',
'revdelete-text'              => 'Tisiwal i yettumḥan ad baben deg umezruy n usebter d weɣmis,
meɛna imuren seg-sen zemren imdanen a ten-ẓren.

Inedbalen wiyaḍ deg wiki-yagi zemren ad ẓren imuren i yettwafren u zemren a ten-mḥan, ḥaca ma llan icekkilen.',
'revdelete-legend'            => 'Eg icekkilen',
'revdelete-hide-text'         => 'Ffer aḍris n tsiwelt',
'revdelete-hide-name'         => 'Ffer tigawt d nnican',
'revdelete-hide-comment'      => 'Ffer abeddel n uwennit',
'revdelete-hide-user'         => 'Ffer Isem n wemseqdac/IP n umeskar',
'revdelete-hide-restricted'   => 'Eg icekkilen i inedbalen d yimdanen wiyaḍ',
'revdelete-suppress'          => 'Kkes talɣut seg inedbalen d yimdanen wiyaḍ',
'revdelete-hide-image'        => 'Ffer ayen yellan deg ufaylu',
'revdelete-unsuppress'        => 'Kkes icekkilen ɣef tisiwal i yuɣalen-d',
'revdelete-log'               => 'Awennit n uɣmis:',
'revdelete-submit'            => 'Eg-it i tasiwelt tettwafren',
'revdelete-logentry'          => 'asekkud n tasiwelt tettubeddel i  [[$1]]',
'logdelete-logentry'          => 'asekkud n tamirt tettubeddel i [[$1]]',
'revdelete-success'           => "'''Asekkud n tasiwelt yettuxdem.'''",
'logdelete-success'           => "'''Asekkud n tamirt yettuxdem.'''",

# Diffs
'difference'              => '(Imgerraden ger tisiwal)',
'lineno'                  => 'Ajerriḍ $1:',
'compareselectedversions' => 'Ẓer imgerraden ger tisiwal i textareḍ',
'editundo'                => 'ssefsu',
'diff-multi'              => '({{PLURAL:$1|Yiwen tasiwelt tabusarit|$1 n tisiwal tibusarin}} ur ttumlalent ara.)',

# Search results
'searchresults'         => 'Igmad n unadi',
'searchresulttext'      => 'Akken ad tessneḍ amek ara tnadiḍ deg {{SITENAME}}, ẓer [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => "Tnadiḍ ɣef '''[[:$1]]'''",
'searchsubtitleinvalid' => "Tnadiḍ ɣef '''$1'''",
'noexactmatch'          => "'''Asebter s yisem \"\$1\" ulac-it.''' Tzemreḍ ad [[:\$1|txelqeḍ asebter-agi]].",
'titlematches'          => 'Ayen yecban azwel n umegrad',
'notitlematches'        => 'Ulac ayen yecban azwel n umegrad',
'textmatches'           => 'Ayen yecban azwel n usebter',
'notextmatches'         => 'ulac ayen yecban azwel n usebter',
'prevn'                 => '$1 ssabeq',
'nextn'                 => '$1 ameḍfir',
'viewprevnext'          => 'Ẓer ($1) ($2) ($3).',
'showingresults'        => "Tamuli n {{PLURAL:$1|'''Yiwen''' wegmud|'''$1''' n yigmad}} seg  #'''$2'''.",
'showingresultsnum'     => "Tamuli n {{PLURAL:$3|'''Yiwen''' wegmud|'''$3''' n yigmad}} seg  #'''$2'''.",
'nonefound'             => "'''Tamawt''': S umata, asmi ur tufiḍ acemma
d ilmen awalen am \"ala\" and \"seg\",
awalen-agi mačči deg tasmult, neɣ tefkiḍ kter n yiwen n wawal (ala isebtar
i yesɛan akk awalen i banen-d).",
'powersearch'           => 'Nadi',
'searchdisabled'        => 'Anadi deg {{SITENAME}} yettwakkes. Tzemreḍ ad tnadiḍ s Google. Meɛna ur tettuḍ ara, tasmult n google taqdimt.',

# Preferences page
'preferences'              => 'Isemyifiyen',
'mypreferences'            => 'Isemyifiyen inu',
'prefsnologin'             => 'Ur tekcimeḍ ara',
'prefsnologintext'         => 'Yessefk ad [[Special:UserLogin|tkecmeḍ]] iwakken textareḍ isemyifiyen inek.',
'prefsreset'               => 'Iɛawed ad yexdem isemyifiyen inek.',
'qbsettings'               => 'Tanuga taǧiḥbuṭ',
'qbsettings-none'          => 'Ulaḥedd',
'qbsettings-fixedleft'     => 'Aẓelmaḍ',
'qbsettings-fixedright'    => 'Ayeffus',
'qbsettings-floatingleft'  => 'Tufeg aẓelmaḍ',
'qbsettings-floatingright' => 'Tufeg ayeffus',
'changepassword'           => 'Beddel awal n tbaḍnit',
'skin'                     => 'Aglim',
'math'                     => 'Tusnakt',
'dateformat'               => 'talɣa n uzemz',
'datedefault'              => 'Ur sɛiɣ ara asemyifi',
'datetime'                 => 'Azemz d ukud',
'math_failure'             => 'Agul n tusnakt',
'math_unknown_error'       => 'Agul mačči d aḍahri',
'math_unknown_function'    => 'Tawuri mačči d taḍahrit',
'math_lexing_error'        => 'Agul n tmawalt',
'math_syntax_error'        => 'Agul n tseddast',
'math_image_error'         => 'Abeddil ɣer PNG yexser; ssenqed installation n latex, dvips, gs, umbeɛd eg abeddel',
'math_bad_tmpdir'          => 'Ur yezmir ara ad yaru ɣef/ɣer tusnakt n temp directory/dossier',
'math_bad_output'          => 'Ur yezmir ara ad yaru ɣef/ɣer tusnakt n tuffɣa directory/dossier',
'math_notexvc'             => "''texvc executable'' / ''executable texvc'' ulac-it; ẓer math/README akken a textareḍ isemyifiyen.",
'prefs-personal'           => 'Profile n wemseqdac',
'prefs-rc'                 => 'Ibeddlen imaynuten',
'prefs-watchlist'          => 'Umuɣ n uɛessi',
'prefs-watchlist-days'     => 'Geddac n wussan yessefk ad banen deg wumuɣ n uɛessi:',
'prefs-watchlist-edits'    => 'Geddac n yibeddlen yessefk ad banen deg wumuɣ n uɛessi ameqqran:',
'prefs-misc'               => 'Isemyifiyen wiyaḍ',
'saveprefs'                => 'Smekti',
'resetprefs'               => 'Reset/réinitialiser isemyifiyen',
'oldpassword'              => 'Awal n tbaḍnit aqdim:',
'newpassword'              => 'Awal n tbaḍnit amaynut:',
'retypenew'                => 'Ɛiwed ssekcem n tbaḍnit amaynut:',
'textboxsize'              => 'Abedddil',
'rows'                     => 'Ijerriḍen:',
'columns'                  => 'Tigejda:',
'searchresultshead'        => 'Anadi',
'resultsperpage'           => 'Geddac n tiririyin i mkul asebter:',
'contextlines'             => 'Geddac n ijerriḍen i mkul tiririt:',
'contextchars'             => 'Geddac n isekkilen n usatal i mkul ajjeriḍ:',
'recentchangescount'       => 'Geddac n izwal deg ibeddilen imaynuten:',
'savedprefs'               => 'Isemyifiyen inek yettusmektan.',
'timezonelegend'           => 'Iẓḍi n ukud',
'timezonetext'             => '¹Amgirred ger akud inek d akud n server (UTC) [s swayeɛ].',
'localtime'                => 'Akud inek',
'timezoneoffset'           => 'Amgirred n ukud',
'servertime'               => 'Akud n server',
'guesstimezone'            => 'Sseqdec azal n browser/explorateur',
'allowemail'               => 'Eǧǧ imseqdacen wiyaḍ a k-aznen email',
'defaultns'                => 'Nadi deg yismawen n taɣult s umeslugen:',
'default'                  => 'ameslugen',
'files'                    => 'Ifayluwen',

# User rights
'userrights'               => 'Laɛej iserfan n wemseqdac', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Laɛej iderman n yimseqdacen',
'userrights-user-editname' => 'Ssekcem isem n wemseqdac:',
'editusergroup'            => 'Beddel iderman n yimseqdacen',
'editinguser'              => "Abeddel n wemseqdac '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Beddel iderman n wemseqdac',
'saveusergroups'           => 'Smekti iderman n yimseqdacen',
'userrights-groupsmember'  => 'Amaslad deg:',
'userrights-reason'        => 'Ayɣer yettubeddel:',

# Groups
'group'       => 'Adrum:',
'group-sysop' => 'Inedbalen',
'group-all'   => '(akk)',

'group-sysop-member' => 'Anedbal',

'grouppage-sysop' => '{{ns:project}}:Inedbalen',

# User rights log
'rightslog'      => 'Aɣmis n yizerfan n wemseqdac',
'rightslogtext'  => 'Wagi d aɣmis n yibeddlen n yizerfan n wemseqdac',
'rightslogentry' => 'Yettubeddel izerfan n wemseqdac $1 seg $2 ar $3',
'rightsnone'     => '(ulaḥedd)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|Abeddel|Ibeddlen}}',
'recentchanges'                     => 'Ibeddlen imaynuten',
'recentchangestext'                 => 'Ḍfer ibeddilen imaynuten n {{SITENAME}}.',
'recentchanges-feed-description'    => 'Ḍfer ibeddilen imaynuten n wiki-yagi deg usuddem-agi.',
'rcnote'                            => "Deg ukessar {{PLURAL:$1|yella '''yiwen''' ubeddel aneggaru|llan '''$1''' n yibeddlen ineggura}} deg {{PLURAL:$2|wass aneggaru|'''$2''' wussan ineggura}}, deg uzemz $3.",
'rcnotefrom'                        => "Deg ukessar llan ibeddlen seg wasmi '''$2''' (ar '''$1''').",
'rclistfrom'                        => 'Ssken ibeddlen imaynuten seg $1',
'rcshowhideminor'                   => '$1 ibeddlen ifessasen',
'rcshowhideliu'                     => '$1 n yimseqdacen i ikecmen',
'rcshowhideanons'                   => '$1 n yimseqdacen udrigen',
'rcshowhidepatr'                    => '$1 n yibeddlen yettwassenqden',
'rcshowhidemine'                    => '$1 ibeddlen inu',
'rclinks'                           => 'Ssken $1 n yibeddlen ineggura di $2 n wussan ineggura<br />$3',
'diff'                              => 'amgirred',
'hist'                              => 'Amezruy',
'hide'                              => 'Ffer',
'show'                              => 'Ssken',
'number_of_watching_users_pageview' => '[$1 aɛessas/iɛessasen]',
'rc_categories'                     => 'Ḥedded i taggayin (ferreq s "|")',
'rc_categories_any'                 => 'Ulayɣer',

# Recent changes linked
'recentchangeslinked'          => 'Ibeddlen imaynuten n isebtar myezdin',
'recentchangeslinked-noresult' => 'Ulac abeddel deg isebtar myezdin deg tawala i textareḍ.',

# Upload
'upload'                      => 'Azen afaylu',
'uploadbtn'                   => 'Azen afaylu',
'reupload'                    => 'Ɛiwed azen',
'reuploaddesc'                => 'Uɣal-d ar talɣa n tuznin.',
'uploadnologin'               => 'Ur tekcimeḍ ara',
'uploadnologintext'           => 'Yessefk [[Special:UserLogin|ad tkecmeḍ]]
iwakken ad tazneḍ afaylu.',
'upload_directory_read_only'  => 'Weserver/serveur Web ur yezmir ara ad yaru deg ($1).',
'uploaderror'                 => 'Agul deg usekcam',
'uploadtext'                  => "Sseqdec talɣa deg ukessar akken ad tazeneḍ tugnawin, akken ad teẓred neɣ ad tnadiḍ tugnawin yettwaznen, ruḥ ɣer [[Special:ImageList|umuɣ n usekcam n tugnawin]], Amezruy n usekcam d umḥay hatent daɣen deg [[Special:Log/upload|amezruy n usekcam]].

Akken ad tessekcmeḍ tugna deg usebter, seqdec azay am wagi
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Afaylu.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Afaylu.png|aḍris]]</nowiki>''' neɣ
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Afaylu.ogg]]</nowiki>''' akken ad iruḥ wezday qbala ar ufaylu.",
'uploadlog'                   => 'amezruy n usekcam',
'uploadlogpage'               => 'Amezruy n usekcam',
'uploadlogpagetext'           => 'Deg ukessar, yella wumuɣ n usekcam n ufayluwen imaynuten.',
'filename'                    => 'Isem n ufaylu',
'filedesc'                    => 'Agzul',
'fileuploadsummary'           => 'Agzul:',
'filestatus'                  => 'Aẓayer n copyright:',
'filesource'                  => 'Seg way yekka',
'uploadedfiles'               => 'Ifayluwen yettwaznen',
'ignorewarning'               => 'Ttu aɣtal u smekti afaylu',
'ignorewarnings'              => 'Ttu iɣtalen',
'illegalfilename'             => 'Isem n ufaylu "$1" yesɛa isekkilen ur tettalaseḍ ara a ten-tesseqdceḍ deg yizwal n isebtar. G leɛnayek beddel isem n ufaylu u azen-it tikkelt nniḍen.',
'badfilename'                 => 'Isem ufaylu yettubeddel ar "$1".',
'filetype-badmime'            => 'Ur tettalaseḍ ara ad tazneḍ ufayluwen n anaw n MIME "$1".',
'filetype-missing'            => 'Afaylu ur yesɛi ara taseggiwit (am ".jpg").',
'large-file'                  => 'Ilaq tiddi n ufayluwen ur tettili kter n $1; tiddi n ufaylu-agi $2.',
'largefileserver'             => 'Afaylu meqqer aṭṭas, server ur t-yeqbil ara.',
'emptyfile'                   => 'Afaylu i tazneḍ d ilem. Waqila tɣelṭeḍ deg isem-is. G leɛnayek ssenqed-it.',
'fileexists'                  => 'Afaylu s yisem-agi yewǧed yagi, ssenqed <strong><tt>$1</tt></strong> ma telliḍ mačči meḍmun akken a t-tbeddleḍ.',
'fileexists-extension'        => 'Afaylu s yisem-agi yewǧed:<br />
Isem n ufaylu i tazneḍ: <strong><tt>$1</tt></strong><br />
Isem n ufaylu i yewǧed: <strong><tt>$2</tt></strong><br />
Amgirred i yella kan deg isekkilen imecṭuḥen/imeqqranen deg taseggiwit (am ".jpg"/".jPg"). G leɛnayek ssenqed-it.',
'fileexists-thumb'            => "<center>'''Tugna i tewǧed'''</center>",
'fileexists-thumbnail-yes'    => 'Iban-d belli tugna-nni d tugna tamecṭuht n tugna nniḍen <i>(thumbnail)</i>. G leɛnayek ssenqed tugna-agi <strong><tt>$1</tt></strong>.<br />
Ma llant kif-kif ur tt-taznepd ara.',
'file-thumbnail-no'           => 'Isem n tugna yebda s <strong><tt>$1</tt></strong>. Waqila tugna-nni d tugna tamecṭuht n tugna nniḍen <i>(thumbnail)</i>.
Ma tesɛiḍ tugna-nni s resolution tameqqrant, azen-it, ma ulac beddel isem-is.',
'fileexists-forbidden'        => 'Tugna s yisem kif-kif tewǧed yagi; g leɛnayek uɣal u beddel isem-is. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Tugna s yisem kif-kif tewǧed yagi; g leɛnayek uɣal u beddel isem-is. [[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'Azen yekfa',
'uploadwarning'               => 'Aɣtal deg wazan n ufayluwen',
'savefile'                    => 'Smekti afaylu',
'uploadedimage'               => '"[[$1]]" yettwazen',
'uploaddisabled'              => 'Suref-aɣ, azen n ufayluwen yettwakkes',
'uploaddisabledtext'          => 'Azen n ufayluwen yettwakkes deg wiki-yagi',
'uploadscripted'              => 'Afaylu-yagi yesɛa angal n HTML/script i yexdem agula deg browser/explorateur.',
'uploadcorrupt'               => 'Afaylu-yagi yexser neɣ yesɛa taseggiwit (am ".jpg") mačči ṣaḥiḥ. G leɛnayek ssenqed-it.',
'uploadvirus'                 => 'Afaylu-nni yesɛa anfafad asenselkim (virus)! Ẓer kter: $1',
'sourcefilename'              => 'And yella afyalu',
'destfilename'                => 'Anda iruḥ afaylu',
'watchthisupload'             => 'Ɛass asebter-agi',
'filewasdeleted'              => 'Afaylu s yisem-agi yettwazen umbeɛd yettumḥa. Ssenqed $1 qbel ad tazniḍ tikelt nniḍen.',

'upload-proto-error'      => 'Agul deg protokol',
'upload-proto-error-text' => 'Assekcam yenṭerr URL i yebdan s <code>http://</code> neɣ <code>ftp://</code>.',
'upload-file-error'       => 'Agul zdaxel',
'upload-file-error-text'  => 'Agul n daxel yeḍran asmi yeɛreḍ ad yexleq afaylu temporaire deg server.  G leɛnayek, meslay akk d unedbal n system.',
'upload-misc-error'       => 'Agul mačči mechur asmi yettwazen ufaylu',
'upload-misc-error-text'  => 'Agul mačči mechur teḍra asmi yettwazen afaylu.  G leɛnayek sseqed belli URL d ṣaḥiḥ u ɛreḍ tikelt nniḍen.  Ma yella daɣen wagul, mmeslay akk d unedbal n system.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Ur yezmir ara ad yessglu URL',
'upload-curl-error6-text'  => 'Ur yezmir ara ad yessglu URL.  Ssenqed URL-nni.',
'upload-curl-error28'      => 'Yekfa wakud n wazen n ufaylu',
'upload-curl-error28-text' => 'Adeg n internet-agi iɛetṭel aṭas. G leɛnayek ssenqed adeg-nni, ggun cwiṭ umbeɛd ɛreḍ tikelt nniḍen.',

'license'            => 'Turagt',
'nolicense'          => 'Ur textareḍ acemma',
'upload_source_url'  => ' (URL saḥiḥ)',
'upload_source_file' => ' (afaylu deg uselkim inek)',

# Special:ImageList
'imagelist_search_for'  => 'Nadi ɣef yisem n tugna:',
'imgfile'               => 'afaylu',
'imagelist'             => 'Umuɣ n tugniwin',
'imagelist_date'        => 'Azemz',
'imagelist_name'        => 'Isem',
'imagelist_user'        => 'Amseqdac',
'imagelist_size'        => 'Tiddi (bytes/octets)',
'imagelist_description' => 'Aglam',

# Image description page
'filehist-current'          => 'Lux a',
'filehist-user'             => 'Amseqdac',
'imagelinks'                => 'Izdayen',
'linkstoimage'              => 'isebtar-agi sɛan azday ar afaylu-agi',
'nolinkstoimage'            => 'Ulaḥedd seg isebtar sɛan azday ar afaylu-agi.',
'sharedupload'              => 'Afaylu-yagi yettuseqdac sɣur wiki tiyaḍ.',
'shareduploadwiki'          => 'Ẓer $1 iwakken ad tessneḍ kter.',
'shareduploadwiki-linktext' => 'Asebter n weglam n ufaylu',
'noimage'                   => 'Afaylu s yisem-agi ulac-it, tzemreḍ ad $1.',
'noimage-linktext'          => 't-tazneḍ',
'uploadnewversion-linktext' => 'tazneḍ tasiwelt tamaynut n ufaylu-yagi',

# MIME search
'mimesearch'         => 'Anadi n MIME',
'mimesearch-summary' => 'Asebter-agi yeǧǧa astay n ifayluwen n unaw n MIME ines. Asekcem: ayen yella/anaw azellum, e.g. <tt>tugna/jpeg</tt>.',
'mimetype'           => 'Anaw n MIME:',
'download'           => 'Ddem-it ɣer uselkim inek',

# Unwatched pages
'unwatchedpages' => 'Isebtar mebla iɛessasen',

# List redirects
'listredirects' => 'Umuɣ isemmimḍen',

# Unused templates
'unusedtemplates'     => 'Talɣiwin mebla aseqdac',
'unusedtemplatestext' => 'Asebter-agi yesɛa umuɣ n akk isebtar n isem n taɣult s yisem "talɣa" iwumi ulac-iten deg ḥedd asebter. Ur tettuḍ ara ad tessenqdeḍ isebtar n talɣa wiyaḍ qbel ad temḥuḍ.',
'unusedtemplateswlh'  => 'izdayen wiyaḍ',

# Random page
'randompage'         => 'Asebter menwala',
'randompage-nopages' => 'Ulac isebtar deg isem n taɣult agi.',

# Random redirect
'randomredirect' => 'Asemmimeḍ menwala',

# Statistics
'statistics'             => 'Tisnaddanin',
'sitestats'              => 'Tisnaddanin n {{SITENAME}}',
'userstats'              => 'Tisnaddanin n wemseqdac',
'sitestatstext'          => "{{PLURAL:\$1|Yella '''yiwen''' usebter|Llan '''\$1''' n isebtar}} deg database.
Azwil-agi yesɛa daɣen akk isebtar \"amyannan\", d isebtar ɣef {{SITENAME}}, d isebtar \"imecṭuḥen\", isebtar ismimḍen, d wiyaḍ.
Asmi ttwakksen wigini, {{PLURAL:\$2|yella '''yiwen''' usebter|llan '''\$2''' n isebtar}} d {{PLURAL:\$2|amliḥ|imliḥen}} . 

'''\$8''' {{PLURAL:\$8|afaylu|ifayluwen}} ttwaznen.

{{PLURAL:\$3|tella|llant}} '''\$3''' n {{PLURAL:\$3|timeẓriwt|timeẓriwin}}, '''\$4''' n {{PLURAL:\$4|ubeddel|yibeddlen}} n usebtar segmi {{SITENAME}} yettwaxleq.
Ihi, {{PLURAL:\$5|yella|llan}} '''\$5''' n {{PLURAL:\$5|ubeddel|ibeddlen}} i mkul asebter, d '''\$6''' timeẓriwin i mkul abeddel.

Ṭul n [http://www.mediawiki.org/wiki/Manual:Job_queue umuti n wexdam] '''\$7'''.",
'userstatstext'          => "{{PLURAL:$1|Yella '''yiwen''' wemseqdac|Llan '''$1''' n yimseqdacen}}, seg-sen
'''$2''' (neɣ '''$4%''') {{PLURAL:$2|yesɛa|sɛan}} izerfan n $5.",
'statistics-mostpopular' => 'isebtar mmeẓren aṭṭas',

'disambiguations'      => 'isebtar n usefham',
'disambiguationspage'  => 'Template:Asefham',
'disambiguations-text' => "Isebtar-agi sɛan azday ɣer '''usebter n usefham'''. Yessefk ad sɛun azday ɣer wezwel ṣaḥiḥ mačči ɣer usebter n usefham.",

'doubleredirects'     => 'Asemmimeḍ yeḍra snat tikwal',
'doubleredirectstext' => 'Mkull ajerriḍ yesɛa azday ɣer asmimeḍ amezwaru akk d wis sin, ajerriḍ amezwaru n uḍris n usebter wis sin daɣen, iwumi yefkan asmimeḍ ṣaḥiḥ i yessefk ad sɛan isebtar azday ɣur-s.',

'brokenredirects'        => 'Isemmimḍen imerẓa',
'brokenredirectstext'    => 'Isemmimḍen-agi sɛan izdayen ar isebtar ulac-iten:',
'brokenredirects-edit'   => '(beddel)',
'brokenredirects-delete' => '(mḥu)',

'withoutinterwiki'         => 'isebtar mebla izdayen ar isebtar n wikipedia s tutlayin tiyaḍ',
'withoutinterwiki-summary' => 'isebtar-agi ur sɛan ara izdayen ar isebtar n wikipedia s tutlayin tiyaḍ:',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte/octet|bytes/octets}}',
'ncategories'             => '$1 {{PLURAL:$1|Taggayt|Taggayin}}',
'nlinks'                  => '$1 {{PLURAL:$1|azday|izdayen}}',
'nmembers'                => '$1 {{PLURAL:$1|amaslad|imasladen}}',
'nrevisions'              => '$1 {{PLURAL:$1|tasiwelt|tisiwal}}',
'nviews'                  => '$1 {{PLURAL:$1|timeẓriwt|tuẓrin}}',
'specialpage-empty'       => 'Asebter-agi d ilem.',
'lonelypages'             => 'isebtar igujilen',
'lonelypagestext'         => 'isebtar-agi ur myezdin ara seg isebtar wiyaḍ deg wiki-yagi.',
'uncategorizedpages'      => 'isebtar mebla taggayt',
'uncategorizedcategories' => 'Taggayin mebla taggayt',
'uncategorizedimages'     => 'Tugna mebla taggayt',
'uncategorizedtemplates'  => 'Talɣiwin mebla taggayt',
'unusedcategories'        => 'Taggayin ur nettwaseqdac ara',
'unusedimages'            => 'Ifayluwin ur nettwaseqdac ara',
'popularpages'            => 'Isebtar iɣerfanen',
'wantedcategories'        => 'Taggayin mmebɣant',
'wantedpages'             => 'Isebtar mmebɣan',
'mostlinked'              => 'Isebtar myezdin aṭas',
'mostlinkedcategories'    => 'Taggayin myezdint aṭas',
'mostcategories'          => 'Isebtar i yesɛan aṭṭas taggayin',
'mostimages'              => 'Tugniwin myezdin aṭas',
'mostrevisions'           => 'Isebtar i yettubedlen aṭas',
'prefixindex'             => 'Akk isebtar s yisekkilen imezwura',
'shortpages'              => 'isebtar imecṭuḥen',
'longpages'               => 'Isebtar imeqqranen',
'deadendpages'            => 'isebtar mebla izdayen',
'deadendpagestext'        => 'isebtar-agi ur sɛan ara azday ɣer isebtar wiyaḍ deg wiki-yagi.',
'protectedpages'          => 'isebtar yettwaḥerzen',
'protectedpagestext'      => 'isebtar-agi yettwaḥerzen seg ubeddel neɣ asemmimeḍ',
'protectedpagesempty'     => 'isebtar-agi ttwaḥerzen s imsektayen -agi.',
'listusers'               => 'Umuɣ n yimseqdacen',
'newpages'                => 'isebtar imaynuten',
'newpages-username'       => 'Isem n wemseqdac:',
'ancientpages'            => 'isebtar iqdimen',
'move'                    => 'Smimeḍ',
'movethispage'            => 'Smimeḍ asebter-agi',
'unusedimagestext'        => 'Ssen belli ideggen n internet sɛan izdayen ɣer tugna-agi s URL n qbala, ɣas akken tugna-nni hatt da.',
'unusedcategoriestext'    => 'Taggayin-agi weǧden meɛna ulac isebtar neɣ taggayin i sseqdacen-iten.',
'notargettitle'           => 'Ulac nnican',
'notargettext'            => 'Ur textareḍ ara asebter d nnican neɣ asebter n wemseqdac d nnican.',

# Book sources
'booksources'               => 'Iɣbula n yidlisen',
'booksources-search-legend' => 'Nadi ɣef iɣbula n yidlisen',
'booksources-go'            => 'Ruḥ',
'booksources-text'          => 'Deg ukessar, yella wumuɣ n yizdayen iberraniyen izzenzen idlisen (imaynuten akk d weqdimen), yernu ahat sɛan kter talɣut ɣef idlisen i tettnadiḍ fell-asen:',

# Special:Log
'specialloguserlabel'  => 'Amseqdac:',
'speciallogtitlelabel' => 'Azwel:',
'log'                  => 'Aɣmis',
'all-logs-page'        => 'Akk iɣmisen',
'log-search-legend'    => 'Nadi ɣef yiɣmisen',
'log-search-submit'    => 'OK',
'alllogstext'          => 'Ssken akk iɣmisen n {{SITENAME}}.
Tzemreḍ ad textareḍ cwiṭ seg-sen ma tebɣiḍ.',
'logempty'             => 'Ur yufi ara deg uɣmis.',
'log-title-wildcard'   => 'Nadi ɣef izwal i yebdan s uḍris-agi',

# Special:AllPages
'allpages'          => 'Akk isebtar',
'alphaindexline'    => '$1 ar $2',
'nextpage'          => 'Asebter ameḍfir ($1)',
'prevpage'          => 'Asebter ssabeq ($1)',
'allpagesfrom'      => 'Ssken isebtar seg:',
'allarticles'       => 'Akk imagraden',
'allinnamespace'    => 'Akk isebtar ($1 isem n taɣult)',
'allnotinnamespace' => 'Akk isebtar (mačči deg $1 isem n taɣult)',
'allpagesprev'      => 'Ssabeq',
'allpagesnext'      => 'Ameḍfir',
'allpagessubmit'    => 'Ruḥ',
'allpagesprefix'    => 'Ssken isebtar s uzwir:',
'allpagesbadtitle'  => 'Azwel n usebter mačči ṣaḥiḥ neɣ yesɛa azwir inter-wiki. Waqila yesɛa isekkilen ur ttuseqdacen ara deg izwal.',
'allpages-bad-ns'   => '{{SITENAME}} ur yesɛi ara isem n taɣult "$1".',

# Special:Categories
'categories'         => 'Taggayin',
'categoriespagetext' => 'Llant taggayin-agi deg wiki-yagi.',

# Special:ListUsers
'listusersfrom'      => 'Ssken imseqdacen seg:',
'listusers-submit'   => 'Ssken',
'listusers-noresult' => 'Ur yufi ḥedd (amseqdac).',

# E-mail user
'mailnologin'     => 'Ur yufi ḥedd (tansa)',
'mailnologintext' => 'Yessefk ad [[Special:UserLogin|tkecmeḍ]] u tesɛiḍ tansa e-mail ṭaṣhiḥt deg [[Special:Preferences|isemyifiyen]] inek
iwakken ad tazneḍ email i imseqdacen wiyaḍ.',
'emailuser'       => 'Azen e-mail i wemseqdac-agi',
'emailpage'       => 'Azen e-mail i wemseqdac',
'emailpagetext'   => 'Lukan amseqdac-agi yefka-d tansa n email ṣaḥiḥ
deg imsifiyen ines, talɣa deg ukessar a t-tazen izen.
Tansa n email i tefkiḍ deg imisifyen inek ad tban-d
deg « Expéditeur» n izen inek iwakken amseqdac-nni yezmer a k-yerr.',
'usermailererror' => 'Yella ugul deg uzwel n email:',
'defemailsubject' => 'e-mail n {{SITENAME}}',
'noemailtitle'    => 'E-mail ulac-it',
'noemailtext'     => 'Amseqdac-agi ur yefki ara e-mail ṣaḥiḥ, neɣ ur yebɣi ara e-mailiyen seg medden.',
'emailfrom'       => 'Seg',
'emailto'         => 'i',
'emailsubject'    => 'Asentel',
'emailmessage'    => 'Izen',
'emailsend'       => 'Azen',
'emailccme'       => 'Azen-iyi-d e-mail n ulsaru n izen inu.',
'emailccsubject'  => 'Alsaru n izen inek i $1: $2',
'emailsent'       => 'E-mail yettwazen',
'emailsenttext'   => 'Izen n e-mail inek yettwazen.',

# Watchlist
'watchlist'            => 'Umuɣ n uɛessi inu',
'mywatchlist'          => 'Umuɣ n uɛessi inu',
'watchlistfor'         => "(n '''$1''')",
'nowatchlist'          => 'Umuɣ n uɛessi inek d ilem.',
'watchlistanontext'    => 'G leɛnaya-k $1 iwakken ad twalaḍ neɣ tbeddleḍ iferdas deg wumuɣ n uɛessi inek.',
'watchnologin'         => 'Ur tekcimeḍ ara',
'watchnologintext'     => 'Yessefk ad [[Special:UserLogin|tkecmeḍ]] iwakken ad tbeddleḍ umuɣ n uɛessi inek.',
'addedwatch'           => 'Yerna ɣer wumuɣ n uɛessi',
'addedwatchtext'       => "Asebter \"[[:\$1]]\" yettwarnu deg [[Special:Watchlist|wumuɣ n uɛessi]] inek.
Ma llan ibeddlen deg usebter-nni neɣ deg usbtar umyennan ines, ad banen dagi,
Deg [[Special:RecentChanges|wumuɣ n yibeddlen imaynuten]] ad banen s '''yisekkilen ibberbuzen''' (akken ad teẓriḍ).

Ma tebɣiḍ ad tekkseḍ asebter seg wumuɣ n uɛessi inek, wekki ɣef \"Fakk aɛessi\".",
'removedwatch'         => 'Yettwakkes seg wumuɣ n uɛessi',
'removedwatchtext'     => 'Asebter "[[:$1]]" yettwakkes seg wumuɣ n uɛessi inek.',
'watch'                => 'Ɛass',
'watchthispage'        => 'Ɛass asebter-agi',
'unwatch'              => 'Fakk aɛassi',
'unwatchthispage'      => 'Fakk aɛassi',
'notanarticle'         => 'Mačči d amagrad',
'watchnochange'        => 'Ulaḥedd n yiferdas n wumuɣ n uɛessi inek ma yettubeddel deg tawala i textareḍ.',
'watchlist-details'    => 'ttɛassaɣ {{PLURAL:$1|$1 usebter|$1 n isebtar}} mebla isebtar "amyannan".',
'wlheader-enotif'      => '* Yeǧǧa Email n talɣut.',
'wlheader-showupdated' => "* Isebtar ttubeddlen segwasmi tkecmeḍ tikelt taneggarut ttbanen-d s '''uḍris aberbuz'''",
'watchmethod-recent'   => 'yessenqed ibeddlen imaynuten n isebtar i ttɛasseɣ',
'watchmethod-list'     => 'yessenqed isebtar i ttɛassaɣ i ibeddlen imaynuten',
'watchlistcontains'    => 'Umuɣ n uɛessi inek ɣur-s $1 n {{PLURAL:$1|usebter|isebtar}}.',
'iteminvalidname'      => "Agnu akk d uferdis '$1', isem mačči ṣaḥiḥ...",
'wlnote'               => "Deg ukessar {{PLURAL:$1|yella yiwen ubeddel aneggaru|llan '''$1''' n yibeddlen ineggura}} deg {{PLURAL:$2|saɛa taneggarut|'''$2''' swayeɛ tineggura}}.",
'wlshowlast'           => 'Ssken $1 n swayeɛ $2 n wussan neɣ $3 ineggura',
'watchlist-show-bots'  => 'Ssken ibeddlen n yiboṭiyen (bots)',
'watchlist-hide-bots'  => 'Ffer ibeddlen n yiboṭiyen (bots)',
'watchlist-show-own'   => 'Ssken ibeddlen inu',
'watchlist-hide-own'   => 'Ffer ibeddlen inu',
'watchlist-show-minor' => 'Ssken ibeddlen ifessasen',
'watchlist-hide-minor' => 'Ffer ibeddlen ifessasen',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Ad iɛass...',
'unwatching' => 'Ad ifukk aɛessi...',

'enotif_mailer'                => 'Email n talɣut n {{SITENAME}}',
'enotif_reset'                 => 'Rcem akk isebtar mmeẓren',
'enotif_newpagetext'           => 'Wagi d asebter amaynut.',
'enotif_impersonal_salutation' => 'Amseqdac n {{SITENAME}}',
'changed'                      => 'yettubeddel',
'created'                      => 'yettwaxleq',
'enotif_subject'               => 'Asebter $PAGETITLE n {{SITENAME}} $CHANGEDORCREATED sɣur $PAGEEDITOR',
'enotif_lastvisited'           => 'Ẓer $1 i akk ibeddlen segwasmi tkecmeḍ tikelt taneggarut.',
'enotif_lastdiff'              => 'Ẓer $1 akken ad tmuqleḍ abeddel.',
'enotif_body'                  => 'Ay $WATCHINGUSERNAME,

Asebter n {{SITENAME}} $PAGETITLE $CHANGEDORCREATED deg wass $PAGEEDITDATE sɣur $PAGEEDITOR, ẓer $PAGETITLE_URL i tasiwelt n tura.

$NEWPAGE

Abeddel n wegzul: $PAGESUMMARY $PAGEMINOREDIT

Meslay akk d ambeddel:
email: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Ur yelli ara email n talɣut asmi llan ibeddlen deg usebter ala lukan teẓreḍ asebter-nni. Tzemreḍ ad terreḍ i zero email n talɣut i akk isebraen i tettɛasseḍ.

             email n talɣut n {{SITENAME}}

--
Akken ad tbeddleḍ n wumuɣ n uɛessi inek settings, ruḥ ɣer
{{fullurl:{{ns:special}}:Watchlist/edit}}

Tadhelt:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'              => 'Mḥu asebter',
'confirm'                 => 'Sentem',
'excontent'               => "Ayen yella: '$1'",
'excontentauthor'         => "Ayen yella: '$1' ('[[Special:Contributions/$2|$2]]' kan i yekken deg-s)",
'exbeforeblank'           => "Ayen yella uqbal ma yettumḥa: '$1'",
'exblank'                 => 'asebter yella d ilem',
'historywarning'          => 'Aɣtal: Asebter i ara temḥuḍ yesɛa amezruy:',
'actioncomplete'          => 'Axdam yekfa',
'deletedtext'             => '"<nowiki>$1</nowiki>" yettumḥa.
Ẓer $2 i aɣmis n yimḥayin imaynuten.',
'deletedarticle'          => '"[[$1]]" yettumḥa',
'dellogpage'              => 'Aɣmis n umḥay',
'dellogpagetext'          => 'Deg ukessar, yella wumuɣ n yimḥayin imaynuten.',
'deletionlog'             => 'Aɣmis n umḥay',
'reverted'                => 'Asuɣal i tasiwel taqdimt',
'deletecomment'           => 'Ayɣer tebɣiḍ ad temḥuḍ',
'cantrollback'            => 'Ur yezmir ara ad yessuɣal; yella yiwen kan amseqdac iwumi ibeddel/yexleq asebter-agi.',
'editcomment'             => 'Agzul n ubeddel yella: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'              => 'Yessuɣal ibeddlen n [[Special:Contributions/$2|$2]] ([[User talk:$2|Meslay]]); yettubeddel ɣer tasiwelt taneggarut n [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'sessionfailure'          => 'Yella ugul akk d takmect inek;
Axdam-agi yebṭel axaṭer waqila yella wemdan nniḍen i yeddem isem n wemseqdac inek.
G leɛnayek wekki ɣef taqeffalt "Back/Précédent" n browser/explorateur inek, umbeɛd wekki ɣef "Actualiser/reload" akk ad tɛerḍeḍ tikelt nniḍen.',
'protectlogpage'          => 'Aɣmis n wemḥay',
'protectedarticle'        => '"[[$1]]" yettwaḥrez',
'protect-title'           => 'Ad yeḥrez "$1"',
'protect-legend'          => 'Sentem tiḥḥerzi',
'protect-default'         => '(ameslugen)',
'protect-level-sysop'     => 'Inedbalen kan',
'protect-summary-cascade' => 'acercur',
'protect-expiring'        => 'yemmut deg $1 (UTC)',
'restriction-type'        => 'Turagt',
'minimum-size'            => 'Tiddi minimum',

# Restrictions (nouns)
'restriction-edit' => 'Beddel',
'restriction-move' => 'Smimeḍ',

# Undelete
'viewdeletedpage'        => 'Ẓer isebtar yettumḥan',
'undeletecomment'        => 'Awennit:',
'undelete-header'        => 'Ẓer [[Special:Log/delete|aɣmis n umḥay]] i isebtar ttumḥan tura.',
'undelete-search-box'    => 'Nadi ɣef isebtar yettumḥan',
'undelete-search-prefix' => 'Ssken isebtar i yebdan s:',
'undelete-search-submit' => 'Nadi',
'undelete-no-results'    => 'Ur yufi ara ulaḥedd n wawalen i tnadiḍ ɣef isebtar deg iɣbaren.',

# Namespace form on various pages
'namespace'      => 'Isem n taɣult:',
'invert'         => 'Snegdam ayen textareḍ',
'blanknamespace' => '(Amenzawi)',

# Contributions
'contributions' => 'Tikkin n wemseqdac',
'mycontris'     => 'Tikkin inu',
'contribsub2'   => 'n $1 ($2)',
'nocontribs'    => 'Ur yufi ara abddel i tebɣiḍ.',
'uctop'         => '(taneggarut)',

'sp-contributions-newbies'     => 'Ssken tikkin n yimseqdacen imaynuten kan',
'sp-contributions-newbies-sub' => 'I yisem yimseqdacen imaynuten',
'sp-contributions-blocklog'    => 'Aɣmis n uɛeṭṭil',
'sp-contributions-search'      => 'Nadi i tikkin',
'sp-contributions-username'    => 'Tansa IP neɣ isem n wemseqdac:',
'sp-contributions-submit'      => 'Nadi',

# What links here
'whatlinkshere'       => 'Ayen i d-yettawi ɣer da',
'linklistsub'         => '(Umuɣ n yizdayen)',
'linkshere'           => "Isebtar-agi sɛan azday ɣer '''[[:$1]]''':",
'nolinkshere'         => "Ulac asebter i yesɛan azday ɣer '''[[:$1]]'''.",
'nolinkshere-ns'      => "Ulac asebter i yesɛan azday ɣer '''[[:$1]]''' deg yisem n taɣult i textareḍ.",
'isredirect'          => 'Asebter n usemmimeḍ',
'istemplate'          => 'asekcam',
'whatlinkshere-prev'  => '{{PLURAL:$1|ssabeq|$1 ssabeq}}',
'whatlinkshere-next'  => '{{PLURAL:$1|ameḍfir|$1 imeḍfiren}}',
'whatlinkshere-links' => '← izdayen',

# Block/unblock
'blockip'                     => 'Ɛekkel amseqdac',
'ipaddress'                   => 'Tansa IP',
'ipadressorusername'          => 'Tansa IP neɣ isem n wemseqdac',
'ipbreason'                   => 'Ayɣer',
'ipbsubmit'                   => 'Ɛekkel amseqdac-agi',
'ipbotheroption'              => 'nniḍen',
'badipaddress'                => 'Tansa IP mačči d ṣaḥiḥ',
'ipblocklist-submit'          => 'Nadi',
'blocklink'                   => 'ɛekkel',
'contribslink'                => 'tikkin',
'block-log-flags-anononly'    => 'Imseqdacen udrigen kan',
'proxyblockreason'            => 'Tansa n IP inek teɛkel axaṭer nettat "open proxy". G leɛnayek, meslay akk d provider inek.',
'proxyblocksuccess'           => 'D ayen.',
'sorbsreason'                 => 'Tansa n IP inek teɛkel axaṭer nettat "open proxy" deg DNSBL yettuseqdac da.',
'sorbs_create_account_reason' => 'Tansa n IP inek teɛkel axaṭer nettat "open proxy" deg DNSBL yettuseqdac da. Ur tezmireḍ ara ad txelqeḍ isem n wemseqdac',

# Developer tools
'lockdb' => 'Sekker database',

# Move page
'move-page-legend'        => 'Smimeḍ asebter',
'movepagetext'            => "Mi tedsseqdceḍ talɣa deg ukessar ad ibddel isem n usebter, yesmimeḍ akk umezruy-is ɣer isem amaynut.
Azwel aqdim ad yuɣal azady n wesmimeḍ ɣer azwel amaynut.
Izdayen ɣer azwel aqdim ur ttubeddlen ara;
ssenqd-iten u ssenqed izdayen n snat d tlata tikkwal.
D kečč i yessefk a ten-yessenqed.

Meɛna, ma yella amagrad deg azwel amaynut neɣ azday n wamsmimeḍ mebla amezruy, asebter-inek '''ur''' yettusmimeḍ '''ara'''.
Yernu, tzemreḍ ad tesmimeḍ asebter ɣer isem-is aqdim ma tɣelṭeḍ.",
'movepagetalktext'        => "Asebter \"Amyannan\" yettusmimeḍ ula d netta '''ma ulac:'''
*Yella asebter \"Amyannan\" deg isem amaynut, neɣ
*Trecmeḍ tankult deg ukessar.

Lukan akka, yessefk a t-tedmeḍ weḥdek.",
'movearticle'             => 'Smimeḍ asebter',
'newtitle'                => 'Ar azwel amaynut',
'move-watch'              => 'Ɛass asebter-agi',
'movepagebtn'             => 'Smimeḍ asebter',
'pagemovedsub'            => 'Asemmimeḍ yekfa',
'articleexists'           => 'Yella yagi yisem am wagi, neɣ 
isem ayen textareḍ mačči d ṣaḥiḥ.
Xtar yiwen nniḍen.',
'talkexists'              => "'''Asemmimeḍ n usebter yekfa, meɛna asebter n umyannan ines ur yettusemmimeḍ ara axaṭer yella yagi yiwen s yisem kif-kif. G leɛnayek, xdem-it weḥd-ek.'''",
'movedto'                 => 'yettusmimeḍ ar',
'movetalk'                => 'Smimeḍ asebter n umyannan (n umagrad-nni)',
'1movedto2'               => '[[$1]] yettusmimeḍ ar [[$2]]',
'1movedto2_redir'         => '[[$1]] yettusmimeḍ ar [[$2]] s redirect',
'movelogpage'             => 'Aɣmis n usemmimeḍ',
'movelogpagetext'         => 'Akessar yella wumuɣ n isebtar yettusmimeḍen.',
'movereason'              => 'Ayɣer',
'revertmove'              => 'Uɣal ar tasiwelt ssabeq',
'delete_and_move'         => 'Mḥu u smimeḍ',
'delete_and_move_text'    => '==Amḥay i tebɣiḍ==

Anda tebɣiḍ tesmimeḍ "[[:$1]]" yella yagi. tebɣiḍ ad temḥuḍ iwakken yeqqim-d wemkan i usmimeḍ?',
'delete_and_move_confirm' => 'Ih, mḥu asebter',
'delete_and_move_reason'  => 'Mḥu iwakken yeqqim-d wemkan i usmimeḍ',
'selfmove'                => 'Izwal amezwaru d uneggaru kif-kif; ur yezmir ara ad yesmimeḍ asebter ɣur iman-is.',
'immobile_namespace'      => 'Azwel n uɣbalu neɣ anda tebɣiḍ tesmimeḍ d anaw aslig; ur yezmir ara ad yesmimeḍ isebtar seg/ɣer isem n taɣult-agi.',

# Export
'export'            => 'Ssufeɣ isebtar',
'exportcuronly'     => 'Ssekcem tasiwelt n tura kan, mačči akk amezruy-is',
'export-submit'     => 'Ssufeɣ',
'export-addcattext' => 'Rnu isebtar seg taggayt:',
'export-addcat'     => 'Rnu',

# Namespace 8 related
'allmessages'               => 'Izen n system',
'allmessagesname'           => 'Isem',
'allmessagesdefault'        => 'Aḍris ameslugen',
'allmessagescurrent'        => 'Aḍris n tura',
'allmessagestext'           => 'Wagi d umuɣ n izen n system i yellan deg yisem n taɣult.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' ut yezmir ara ad yettuseqdac axaṭer '''\$wgUseDatabaseMessages''' yettwakkes.",
'allmessagesfilter'         => 'Tastayt n yisem n izen:',
'allmessagesmodified'       => 'Ssken win yettubeddlen kan',

# Thumbnails
'thumbnail-more'  => 'Ssemɣer',
'filemissing'     => 'Afaylu ulac-it',
'thumbnail_error' => 'Agul asmi yexleq tugna tamecṭuḥt: $1',

# Special:Import
'import'                     => 'Ssekcem isebtar',
'importinterwiki'            => 'Assekcem n transwiki',
'import-interwiki-history'   => 'Xdem alsaru n akk tisiwal umezruy n usebter-agi',
'import-interwiki-submit'    => 'Ssekcem',
'import-interwiki-namespace' => 'Azen isebtar ar isem n taɣult:',
'importstart'                => 'Asekcem n isebtar...',
'import-revision-count'      => '$1 {{PLURAL:$1|tasiwelt|tisiwal}}',
'importnopages'              => 'Ulac isebtar iwakken ad ttussekcmen.',
'importfailed'               => 'Asekcem yexser: $1',
'importunknownsource'        => 'Anaw n uɣbalu n usekcem mačči d mechur',
'importcantopen'             => 'Ur yezmir ara ad yexdem asekcem n ufaylu',
'importbadinterwiki'         => 'Azday n interwiki ur yelhi',
'importnotext'               => 'D ilem neɣ ulac aḍris',
'importsuccess'              => 'Asekcem yekfa!',
'importhistoryconflict'      => 'Amennuɣ ger tisiwal n umezruy (ahat asebter-agi yettwazen yagi)',
'importnosources'            => 'Asekcam n transwiki ur yexdim ara u amezruy n usekcam yettwakkes.',
'importnofile'               => 'ulaḥedd afaylu usekcam ur yettwazen.',

# Import log
'importlogpage'                    => 'Aɣmis n usekcam',
'importlogpagetext'                => 'Adeblan n usekcam n isebtar i yesɛan amezruy ubeddel seg wiki tiyaḍ.',
'import-logentry-upload'           => 'Yessekcem [[$1]] s usekcam n ufaylu',
'import-logentry-upload-detail'    => '$1 tasiwelt(tisiwal)',
'import-logentry-interwiki'        => '$1 s transwiki',
'import-logentry-interwiki-detail' => '$1 tasiwelt(tisiwal) seg $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Asebter n wemseqdac inu',
'tooltip-pt-anonuserpage'         => 'Asebter n wemseqdac n IP wukud tekkiḍ',
'tooltip-pt-mytalk'               => 'Asebter n wemyannan inu',
'tooltip-pt-anontalk'             => 'Amyannan ɣef yibeddlen n tansa ip-yagi',
'tooltip-pt-preferences'          => 'Isemyifiyen inu',
'tooltip-pt-watchlist'            => 'Umuɣ n uɛessi n isebtar i ttɛessaɣ',
'tooltip-pt-mycontris'            => 'Umuɣ n tikkin inu',
'tooltip-pt-login'                => 'Lukan tkecmeḍ xir, meɛna am tebɣiḍ.',
'tooltip-pt-anonlogin'            => 'Lukan tkecmeḍ xir, meɛna am tebɣiḍ.',
'tooltip-pt-logout'               => 'Ffeɣ',
'tooltip-ca-talk'                 => 'Amyannan ɣef wayen yella deg usebter',
'tooltip-ca-edit'                 => 'Tzemreḍ ad tbeddleḍ asebter-agi. Sseqdec pre-timeẓriwt qbel.',
'tooltip-ca-addsection'           => 'Rnu awennit i amyannan-agi.',
'tooltip-ca-viewsource'           => 'Asebter-agi yettwaḥrez. Tzemreḍ ad twaliḍ aɣbalu-ines.',
'tooltip-ca-history'              => 'Tisiwal ssabeq n usebter-agi.',
'tooltip-ca-protect'              => 'Ḥrez asebter-agi',
'tooltip-ca-delete'               => 'Mḥu asebter-agi',
'tooltip-ca-undelete'             => 'Err akk ibeddlen n usebter-agi i yellan uqbel ad yettwamḥu usebter',
'tooltip-ca-move'                 => 'Smimeḍ asebter-agi',
'tooltip-ca-watch'                => 'Rnu asebter-agi i wumuɣ n uɛessi inek',
'tooltip-ca-unwatch'              => 'Kkes asebter-agi seg wumuɣ n uɛessi inek',
'tooltip-search'                  => 'Nadi {{SITENAME}}',
'tooltip-p-logo'                  => 'Asebter amenzawi',
'tooltip-n-mainpage'              => 'Ẓer asebter amenzawi',
'tooltip-n-portal'                => 'Ɣef usenfar, ayen tzemrḍ ad txedmeḍ, anda tafeḍ tiɣawsiwin',
'tooltip-n-currentevents'         => 'Af ayen yeḍran tura',
'tooltip-n-recentchanges'         => 'Umuɣ n yibeddlen imaynuten deg wiki.',
'tooltip-n-randompage'            => 'Ẓer asebter menwala',
'tooltip-n-help'                  => 'Amkan ideg tafeḍ.',
'tooltip-t-whatlinkshere'         => 'Umuɣ n akk isebtar i yesɛan azday ar dagi',
'tooltip-t-recentchangeslinked'   => 'Ibeddlen imaynuten deg isebtar myezdin seg usebter-agi',
'tooltip-feed-rss'                => 'RSS feed n usebter-agi',
'tooltip-feed-atom'               => 'Atom feed n usebter-agi',
'tooltip-t-contributions'         => 'Ẓer umuɣ n tikkin n wemseqdac-agi',
'tooltip-t-emailuser'             => 'Azen e-mail i wemseqdac-agi',
'tooltip-t-upload'                => 'Azen tugna neɣ afaylu nniḍen',
'tooltip-t-specialpages'          => 'Umuɣ n akk isebtar usligen',
'tooltip-ca-nstab-main'           => 'Ẓer ayen yellan deg usebter',
'tooltip-ca-nstab-user'           => 'Ẓer asebter n wemseqdac',
'tooltip-ca-nstab-media'          => 'Ẓer asebter n media',
'tooltip-ca-nstab-special'        => 'Wagi d asebter uslig, ur tezmireḍ ara a t-tbeddleḍ',
'tooltip-ca-nstab-project'        => 'Ẓer asebter n usenfar',
'tooltip-ca-nstab-image'          => 'Ẓer asebter n tugna',
'tooltip-ca-nstab-mediawiki'      => 'Ẓer izen n system',
'tooltip-ca-nstab-template'       => 'Ẓer talɣa',
'tooltip-ca-nstab-help'           => 'Ẓer asebter n tallat',
'tooltip-ca-nstab-category'       => 'Ẓer asebter n taggayt',
'tooltip-minoredit'               => 'Wagi d abeddel afessas',
'tooltip-save'                    => 'Smekti ibeddlen inek',
'tooltip-preview'                 => 'G leɛnaya-k, pre-ẓer ibeddlen inek uqbel ad tesmektiḍ!',
'tooltip-diff'                    => 'Ssken ayen tbeddleḍ deg uḍris.',
'tooltip-compareselectedversions' => 'Ẓer amgirred ger snat tisiwlini (i textareḍ) n usebter-agi.',
'tooltip-watch'                   => 'Rnu asebter-agi i wumuɣ n uɛessi inu',
'tooltip-recreate'                => 'Ɛiwed xleq asebter ɣas akken yettumḥu',

# Attribution
'anonymous'        => 'Amseqdac udrig (Imseqdacen udrigen) n {{SITENAME}}',
'siteuser'         => '{{SITENAME}} amseqdac $1',
'lastmodifiedatby' => 'Tikkelt taneggarut asmi yettubeddel asebter-agi $2, $1 sɣur $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Tikkin n wemseqdac-agi.',
'others'           => 'wiyaḍ',
'siteusers'        => '{{SITENAME}} amseqdac(imseqdacen) $1',
'creditspage'      => 'Win ixedmen asebter',
'nocredits'        => 'Ulac talɣut ɣef wayen ixedmen asebter-agi.',

# Spam protection
'spamprotectiontitle' => 'Aḥraz amgel "Spam"',
'spamprotectiontext'  => "Asebter i tebɣiḍ ad tesmektiḍ iɛekkel-it ''aḥraz mgel \"Spam\"''. Ahat yella wezday aberrani.",
'spamprotectionmatch' => 'Aḍris-agi ur t-iɛeǧ \'\'"aḥraz mgel "Spam"\'\': $1',
'spam_reverting'      => 'Asuɣal i tasiwel taneggarut i ur tesɛi ara izdayen ɣer $1',
'spam_blanking'       => 'Akk tisiwal sɛan izdayen ɣer $1, ad yemḥu',

# Info page
'infosubtitle'   => 'Talɣut n usebter',
'numedits'       => 'Geddac n yibeddlen (amagrad): $1',
'numtalkedits'   => 'Geddac n yibeddlen (asebter n wemyannan): $1',
'numwatchers'    => 'Geddac n yiɛessasen: $1',
'numauthors'     => 'Geddac n yimseqdacen i yuran (amagrad): $1',
'numtalkauthors' => 'Geddac n yimsedac i yuran (asebter n wemyennan): $1',

# Math options
'mw_math_png'    => 'Daymen err-it PNG',
'mw_math_simple' => 'HTML ma yella amraḍi, ma ulac PNG',
'mw_math_html'   => 'HTML ma yezmer neɣ PNG ma ulac',
'mw_math_source' => 'Eǧǧ-it s TeX (i browsers/explorateurs n weḍris)',
'mw_math_modern' => 'Mliḥ i browsers/explorateurs imaynuten',
'mw_math_mathml' => 'MathML ma yezmer (experimental)',

# Patrolling
'markaspatrolleddiff'                 => 'Rcem "yettwassenqden"',
'markaspatrolledtext'                 => 'Rcem amagrad-agi "yettwassenqden"',
'markedaspatrolled'                   => 'Rcem belli yettwasenqed',
'markedaspatrolledtext'               => 'Tasiwelt i textareḍ tettwassenqed.',
'rcpatroldisabled'                    => 'Yettwakkes asenqad n ibeddlen imaynuten',
'rcpatroldisabledtext'                => 'Yettwakkes asenqad n ibeddlen imaynuten',
'markedaspatrollederror'              => 'Ur yezmir ara ad yercem "yettwassenqden"',
'markedaspatrollederrortext'          => 'Yessefk ad textareḍ tasiwelt akken a tt-trecmeḍ "yettwassenqden".',
'markedaspatrollederror-noautopatrol' => 'Ur tezmireḍ ara ad trecmeḍ ibeddilen inek "yettwassenqden".',

# Patrol log
'patrol-log-page' => 'Aɣmis n usenqad',
'patrol-log-line' => 'Yercem tasiwelt $1 n $2 "yettwassenqden" $3',
'patrol-log-auto' => '(otomatik)',

# Image deletion
'deletedrevision' => 'Tasiwelt taqdimt $1 tettumḥa.',

# Browsing diffs
'previousdiff' => '← Amgirred ssabeq',
'nextdiff'     => 'Amgirred ameḍfir →',

# Media information
'mediawarning'         => "'''Aɣtal''': Waqila afaylu-yagi yesɛa angal aḥraymi, lukan a t-tesseqdceḍ yezmer ad ixesser aselkim inek.<hr />",
'imagemaxsize'         => 'Ḥedded tiddi n tugniwin deg yiglamen n tugniwim i:',
'thumbsize'            => 'Tiddi n tugna tamecṭuḥt:',
'file-info'            => '(tiddi n ufaylu: $1, anaw n MIME: $2)',
'file-info-size'       => '($1 × $2 pixel, tiddi n ufaylu: $3, anaw n MIME: $4)',
'file-nohires'         => '<small>Ulac resolution i tameqqrant fell-as.</small>',
'show-big-image'       => 'Resolution tameqqrant',
'show-big-image-thumb' => '<small>Tiddi n pre-timeẓriwt-agi: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'             => 'Umuɣ n ifayluwen imaynuten',
'imagelisttext'         => "Deg ukessar yella wumuɣ n '''$1''' {{PLURAL:$1|ufaylu|yifayluwen}} $2.",
'noimages'              => 'Tugna ulac-itt.',
'ilsubmit'              => 'Nadi',
'bydate'                => 's uzemz',
'sp-newimages-showfrom' => 'Ssken tugniwin timaynutin seg $1',

# EXIF tags
'exif-imagewidth' => 'Tehri',

'exif-meteringmode-255' => 'Nniḍen',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometr deg ssaɛa',

# External editor support
'edit-externally'      => 'Beddel afaylu-yagi s usnas aberrani.',
'edit-externally-help' => 'Ẓer [http://www.mediawiki.org/wiki/Manual:External_editors taknut] iwakken ad tessneḍ kter.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'akk',
'imagelistall'     => 'akk',
'watchlistall2'    => 'akk',
'namespacesall'    => 'akk',
'monthsall'        => 'akk',

# E-mail address confirmation
'confirmemail'            => 'Sentem tansa n e-mail',
'confirmemail_noemail'    => 'Ur tesɛiḍ ara tansa n email ṣaḥiḥ deg [[Special:Preferences|isemyifiyen n wemseqdac]] inek.',
'confirmemail_text'       => 'Deg wiki-yagi, yessefk ad tvalidiḍ tansa n email inek
qbel ad tesseqdceḍ iḍaɣaren n email. Tella taqeffalt d akessar, wekki fell-as
iwakken yettwazen ungal n usentem semail. Email-nni yesɛa azady, ldi-t.',
'confirmemail_pending'    => '<div class="error">
Yettwazen-ak yagi ungal n usentem; lukan txelqeḍ isem wemseqdac tura kan,
ahat yessefk ad tegguniḍ cwiṭ qbel ad tɛreḍeḍ ad testeqsiḍ ɣef ungal amaynut.
</div>',
'confirmemail_send'       => 'Azen-iyi-d angal n usentem s e-mail iwakken ad snetmeɣ.',
'confirmemail_sent'       => 'E-mail yettwazen iwakken ad tsentmeḍ.',
'confirmemail_oncreate'   => 'Angal n usentem yettwazen ar tansa n e-mail inek.
Yessefk ad tesseqdceḍ angal-agi iwakken ad tkecmeḍ, meɛna yessefk a t-tefkeḍ
iwakken ad xedmen yiḍaɣaren n email deg wiki-yagi.',
'confirmemail_sendfailed' => 'Ur yezmir ara ad yazen asentem n email. Ssenqed tansa n email inek.

Email yuɣal-d: $1',
'confirmemail_invalid'    => 'Angal n usentem mačči ṣaḥiḥ. Waqila yemmut.',
'confirmemail_needlogin'  => 'Yessefk $1 iwakken tesnetmeḍ tansa n email inek.',
'confirmemail_success'    => 'Asentem n tansa n email inek yekfa. Tura tzemreḍ ad tkecmeḍ.',
'confirmemail_loggedin'   => 'Asentem n tansa n email inek yekfa tura.',
'confirmemail_error'      => 'Yella ugur s usmekti n usentem inek.',
'confirmemail_subject'    => 'Asentem n tansa n email seg {{SITENAME}}',
'confirmemail_body'       => 'Amdan, waqila d kečč, seg tansa IP $1, yexleq
isem n wemseqdac "$2" s tansa n e-mail deg {{SITENAME}}.

Iwakken tbeyyneḍ belli isem n wemseqdac inek u terreḍ
iḍaɣaren n email ad xdemen deg {{SITENAME}}, ldi azday agi:

$3

Lukan mačči d *kečč*, ur teḍfireḍ ara azday. Angal n usentem-agi
ad yemmut ass $4.',

# Scary transclusion
'scarytranscludedisabled' => '[Yettwakkes assekcam n isebtar seg wiki tiyaḍ]',
'scarytranscludefailed'   => '[Ur yezmir ara a d-yawi talɣa n $1; suref-aɣ]',
'scarytranscludetoolong'  => '[URL d aɣezfan bezzaf; suref-aɣ]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Izdayen n zdeffir n umagrad-agi:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Mḥu])',
'trackbacklink'     => 'Azday n zdeffir',
'trackbackdeleteok' => 'Azday n zdeffir yettumḥa.',

# Delete conflict
'deletedwhileediting' => 'Aɣtal: Asebter-agi yettumḥa qbel ad tebdiḍ a t-tbeddleḍ!',
'confirmrecreate'     => "Amseqdac [[User:$1|$1]] ([[User talk:$1|Meslay]]) yemḥu asebter-agi beɛd ad tebdiḍ abeddel axaṭer:
: ''$2''
G leɛnaya-k sentem belli ṣaḥḥ tebɣiḍ ad tɛiwedeḍ axlaq n usebter-agi.",
'recreate'            => 'Ɛiwed xleq',

# HTML dump
'redirectingto' => 'Asemmimeḍ ar [[:$1]]...',

# action=purge
'confirm_purge' => 'Mḥu lkac n usebter-agi?

$1',

# AJAX search
'searchcontaining' => "Inadi isebtar i yesɛan ''$1''.",
'searchnamed'      => "Nadi ɣef imagraden ttusemman ''$1''.",
'articletitles'    => "Imagraden i yebdan s ''$1''",
'hideresults'      => 'Ffer igmad',

# Multipage image navigation
'imgmultipageprev' => '← asebter ssabeq',
'imgmultipagenext' => 'asebter ameḍfir →',
'imgmultigo'       => 'Ruḥ!',

# Table pager
'ascending_abbrev'         => 'asawen',
'descending_abbrev'        => 'akessar',
'table_pager_next'         => 'Asebtar ameḍfir',
'table_pager_prev'         => 'Asebtar ssabeq',
'table_pager_first'        => 'Asebtar amezwaru',
'table_pager_last'         => 'Asebtar aneggaru',
'table_pager_limit'        => 'Ssken $1 n yiferdas di mkul asebtar',
'table_pager_limit_submit' => 'Ruḥ',
'table_pager_empty'        => 'Ulac igmad',

# Auto-summaries
'autosumm-blank'   => 'Yekkes akk aḍris seg usebter',
'autosumm-replace' => "Ibeddel asebtar s '$1'",
'autoredircomment' => 'Asemmimeḍ ar [[$1]]',
'autosumm-new'     => 'Asebtar amaynut: $1',

# Size units
'size-bytes'     => '$1 B/O',
'size-kilobytes' => '$1 KB/KO',
'size-megabytes' => '$1 MB/MO',
'size-gigabytes' => '$1 GB/GO',

# Live preview
'livepreview-loading' => 'Assisi…',
'livepreview-ready'   => 'Assisi… D ayen!',
'livepreview-failed'  => 'Pre-timeẓriwt taǧiḥbuṭ texser!
Ɛreḍ pre-timeẓriwt tamagnut.',
'livepreview-error'   => 'Pre-timeẓriwt taǧiḥbuṭ texser: $1 "$2"
Ɛreḍ pre-timeẓriwt tamagnut.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Ibeddlen imaynuten ɣef $1 tisinin ahat ur ttbanen ara deg wumuɣ-agi.',
'lag-warn-high'   => 'Database tɛeṭṭel aṭas, ibeddlen imaynuten ɣef $1 tisinin ahat ur ttbanen ara deg wumuɣ-agi.',

# Watchlist editor
'watchlistedit-numitems'       => 'Mebla isebtar "Amyannan", umuɣ n uɛessi inek ɣur-s {{PLURAL:$1|1 wezwel|$1 yizwalen}}.',
'watchlistedit-noitems'        => 'Umuɣ n uɛessi inek ur yesɛi ḥedd izwal.',
'watchlistedit-normal-title'   => 'Beddel umuɣ n uɛessi',
'watchlistedit-normal-legend'  => 'Kkes izwal seg wumuɣ n uɛessi',
'watchlistedit-normal-explain' => 'Izwal deg wumuɣ n uɛessi ttbanen-d deg ukessar. Akken ad tekkseḍ yiwen wezwel, wekki ɣef tenkult i zdat-s, umbeɛd wekki ɛef "Kkes izwal". Tzemreḍ daɣen [[Special:Watchlist/raw|ad tbeddleḍ umuɣ n uɛessi (raw)]].',
'watchlistedit-normal-submit'  => 'Kkes izwal',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 wezwel yettwakkes|$1 yizwal ttwakksen}} seg wumuɣ n uɛessi inek:',
'watchlistedit-raw-title'      => 'Beddel umuɣ n uɛessi (raw)',
'watchlistedit-raw-legend'     => 'Beddel umuɣ n uɛessi (raw)',
'watchlistedit-raw-titles'     => 'Izwal:',
'watchlistedit-raw-done'       => 'Umuɣ n uɛessi inek yettubeddel.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 wezwel |$1 yizwal}} nnernan:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 wezwel yettwakkes|$1 yizwal ttwakksen}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Umuɣ n uɛessi',
'watchlisttools-edit' => 'Ẓer u beddel umuɣ n uɛessi',
'watchlisttools-raw'  => 'Beddel umuɣ n uɛessi (raw)',

# Special:Version
'version' => 'Tasiwelt', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'isebtar usligen',

);
