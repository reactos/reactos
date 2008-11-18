<?php
/** Pashto (پښتو)
 *
 * @ingroup Language
 * @file
 *
 * @author Ahmed-Najib-Biabani-Ibrahimkhel
 */

$specialPageAliases = array(
	'Shortpages'                => array( 'لنډ_مخونه' ),
	'Longpages'                 => array( 'اوږده_مخونه' ),
	'Newpages'                  => array( 'نوي_مخونه' ),
	'Protectedpages'            => array( 'ژغورلي_مخونه' ),
	'Protectedtitles'           => array( 'ژغورلي_سرليکونه' ),
	'Allpages'                  => array( 'ټول_مخونه' ),
	'Categories'                => array( 'وېشنيزې' ),
	'Mypage'                    => array( 'زما_پاڼه' ),
	'Search'                    => array( 'لټون' ),
);

$skinNames = array(
	'standard'    => 'کلاسيک',
	'nostalgia'   => 'نوستالژي',
	'cologneblue' => 'شين کلون',
	'monobook'    => 'مونوبوک',
	'myskin'      => 'زمابڼه',
	'chick'       => 'شيک',
	'simple'      => 'ساده',
	'modern'      => 'نوی',
);

$namespaceNames = array(
	NS_MEDIA          => 'رسنۍ',
	NS_SPECIAL        => 'ځانګړی',
	NS_TALK           => 'خبرې_اترې',
	NS_USER           => 'کارونکی',
	NS_USER_TALK      => 'د_کارونکي_خبرې_اترې',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => 'د_$1_خبرې_اترې',
	NS_IMAGE          => 'انځور',
	NS_IMAGE_TALK     => 'د_انځور_خبرې_اترې',
	NS_MEDIAWIKI      => 'ميډياويکي',
	NS_MEDIAWIKI_TALK => 'د_ميډياويکي_خبرې_اترې',
	NS_TEMPLATE       => 'کينډۍ',
	NS_TEMPLATE_TALK  => 'د_کينډۍ_خبرې_اترې',
	NS_HELP           => 'لارښود',
	NS_HELP_TALK      => 'د_لارښود_خبرې_اترې',
	NS_CATEGORY       => 'وېشنيزه',
	NS_CATEGORY_TALK  => 'د_وېشنيزې_خبرې_اترې',
);

$magicWords = array(
	'notoc'               => array( '0', '__بی‌نيولک__', '__NOTOC__' ),
	'nogallery'           => array( '0', '__بی‌نندارتونه__', '__NOGALLERY__' ),
	'forcetoc'            => array( '0', '__نيوليکداره__', '__FORCETOC__' ),
	'toc'                 => array( '0', '__نيوليک__', '__TOC__' ),
	'noeditsection'       => array( '0', '__بی‌برخې__', '__NOEDITSECTION__' ),
	'currentmonth'        => array( '1', 'روانه_مياشت', 'CURRENTMONTH' ),
	'currentmonthname'    => array( '1', 'دروانې_مياشت_نوم', 'CURRENTMONTHNAME' ),
	'currentmonthabbrev'  => array( '1', 'دروانې_مياشت_لنډون', 'CURRENTMONTHABBREV' ),
	'currentday'          => array( '1', 'نن', 'CURRENTDAY' ),
	'currentday2'         => array( '1', 'نن۲', 'CURRENTDAY2' ),
	'currentdayname'      => array( '1', 'دننۍورځې_نوم', 'CURRENTDAYNAME' ),
	'currentyear'         => array( '1', 'سږکال', 'CURRENTYEAR' ),
	'currenttime'         => array( '1', 'داوخت', 'CURRENTTIME' ),
	'currenthour'         => array( '1', 'دم_ګړۍ', 'CURRENTHOUR' ),
	'localmonth'          => array( '1', 'سيمه_يزه_مياشت', 'LOCALMONTH' ),
	'localmonthname'      => array( '1', 'دسيمه_يزې_مياشت_نوم', 'LOCALMONTHNAME' ),
	'localmonthabbrev'    => array( '1', 'دسيمه_يزې_مياشت_لنډون', 'LOCALMONTHABBREV' ),
	'localday'            => array( '1', 'سيمه_يزه_ورځ', 'LOCALDAY' ),
	'localday2'           => array( '1', 'سيمه_يزه_ورځ۲', 'LOCALDAY2' ),
	'localdayname'        => array( '1', 'دسيمه_يزې_ورځ_نوم', 'LOCALDAYNAME' ),
	'localyear'           => array( '1', 'سيمه_يزکال', 'LOCALYEAR' ),
	'localtime'           => array( '1', 'سيمه_يزوخت', 'LOCALTIME' ),
	'localhour'           => array( '1', 'سيمه_يزه_ګړۍ', 'LOCALHOUR' ),
	'numberofpages'       => array( '1', 'دمخونوشمېر', 'NUMBEROFPAGES' ),
	'numberofarticles'    => array( '1', 'دليکنوشمېر', 'NUMBEROFARTICLES' ),
	'numberoffiles'       => array( '1', 'ددوتنوشمېر', 'NUMBEROFFILES' ),
	'numberofusers'       => array( '1', 'دکارونکوشمېر', 'NUMBEROFUSERS' ),
	'pagename'            => array( '1', 'دمخ_نوم', 'PAGENAME' ),
	'pagenamee'           => array( '1', 'دمخ_نښه', 'PAGENAMEE' ),
	'namespace'           => array( '1', 'نوم_تشيال', 'NAMESPACE' ),
	'namespacee'          => array( '1', 'د_نوم_تشيال_نښه', 'NAMESPACEE' ),
	'talkspace'           => array( '1', 'دخبرواترو_تشيال', 'TALKSPACE' ),
	'img_right'           => array( '1', 'ښي', 'right' ),
	'img_left'            => array( '1', 'کيڼ', 'left' ),
	'img_none'            => array( '1', 'هېڅ', 'none' ),
	'img_width'           => array( '1', '$1px' ),
	'sitename'            => array( '1', 'دوېبځي_نوم', 'SITENAME' ),
	'server'              => array( '0', 'پالنګر', 'SERVER' ),
	'servername'          => array( '0', 'دپالنګر_نوم', 'SERVERNAME' ),
	'grammar'             => array( '0', 'GRAMMAR:' ),
	'currentweek'         => array( '1', 'روانه_اوونۍ', 'CURRENTWEEK' ),
	'currentdow'          => array( '1', 'داوونۍورځ', 'CURRENTDOW' ),
	'localweek'           => array( '1', 'سيمه_يزه_اوونۍ', 'LOCALWEEK' ),
	'language'            => array( '0', '#ژبه:', '#LANGUAGE:' ),
	'hiddencat'           => array( '1', '__پټه_وېشنيزه__', '__HIDDENCAT__' ),
);

$rtl = true;
$defaultUserOptionOverrides = array(
	# Swap sidebar to right side by default
	'quickbar' => 2,
	# Underlines seriously harm legibility. Force off:
	'underline' => 0,
);

$messages = array(
# User preference toggles
'tog-hideminor'            => 'په وروستيو بدلونو کې وړې سمادېدنې پټول',
'tog-showtoolbar'          => 'د سمادولو توکپټه ښکاره کول (جاواسکرېپټ)',
'tog-rememberpassword'     => 'زما پټنوم پدې کمپيوټر په ياد ولره!',
'tog-watchcreations'       => 'هغه مخونه چې زه يې جوړوم، زما کتلي لړليک کې ورګډ کړه',
'tog-watchdefault'         => 'هغه مخونه چې زه يې سمادوم، زما کتلي لړليک کې ورګډ کړه',
'tog-watchmoves'           => 'هغه مخونه چې زه يې لېږدوم، زما کتلي لړليک کې ورګډ کړه',
'tog-watchdeletion'        => 'هغه مخونه چې زه يې ړنګوم، زما کتلي لړليک کې ورګډ کړه',
'tog-enotifwatchlistpages' => 'هر کله چې زما په کتلي لړليک کې يو مخ بدلون مومي نو ما ته دې برېښليک راشي.',
'tog-enotifusertalkpages'  => 'کله چې زما د خبرو اترو په مخ کې بدلون پېښېږي نو ما ته دې يو برېښليک ولېږلی شي.',
'tog-enotifminoredits'     => 'که په مخونو کې وړې سمادېدنې هم کېږي نو ماته دې برېښليک ولېږل شي.',
'tog-ccmeonemails'         => 'هغه برېښليکونه چې زه يې نورو ته لېږم، د هغو يوه کاپي دې ماته هم راشي',
'tog-showhiddencats'       => 'پټې وېشنيزې ښکاره کول',

'underline-always' => 'تل',
'underline-never'  => 'هېڅکله',

'skinpreview' => '(مخکتنه)',

# Dates
'sunday'        => 'اتوار',
'monday'        => 'ګل',
'tuesday'       => 'نهي',
'wednesday'     => 'شورو',
'thursday'      => 'زيارت',
'friday'        => 'جمعه',
'saturday'      => 'خالي',
'sun'           => 'اتوار',
'mon'           => 'ګل',
'tue'           => 'نهي',
'wed'           => 'شورو',
'thu'           => 'زيارت',
'fri'           => 'جمعه',
'sat'           => 'خالي',
'january'       => 'جنوري',
'february'      => 'فبروري',
'march'         => 'مارچ',
'april'         => 'اپرېل',
'may_long'      => 'می',
'june'          => 'جون',
'july'          => 'جولای',
'august'        => 'اګسټ',
'september'     => 'سېپتمبر',
'october'       => 'اکتوبر',
'november'      => 'نومبر',
'december'      => 'ډيسمبر',
'january-gen'   => 'جنوري',
'february-gen'  => 'فبروري',
'march-gen'     => 'مارچ',
'april-gen'     => 'اپرېل',
'may-gen'       => 'می',
'june-gen'      => 'جون',
'july-gen'      => 'جولای',
'august-gen'    => 'اګسټ',
'september-gen' => 'سېپتمبر',
'october-gen'   => 'اکتوبر',
'november-gen'  => 'نومبر',
'december-gen'  => 'ډيسمبر',
'jan'           => 'جنوري',
'feb'           => 'فبروري',
'mar'           => 'مارچ',
'apr'           => 'اپرېل',
'may'           => 'می',
'jun'           => 'جون',
'jul'           => 'جولای',
'aug'           => 'اګسټ',
'sep'           => 'سېپتمبر',
'oct'           => 'اکتوبر',
'nov'           => 'نومبر',
'dec'           => 'ډيسمبر',

# Categories related messages
'pagecategories'              => '{{PLURAL:$1|وېشنيزه|وېشنيزې}}',
'category_header'             => 'د "$1" په وېشنيزه کې شته مخونه',
'subcategories'               => 'وړې-وېشنيزې',
'category-media-header'       => '"$1" رسنۍ په وېشنيزه کې',
'category-empty'              => "''تر اوسه پورې همدا وېشنيزه هېڅ کوم مخ يا کومه رسنيزه دوتنه نلري.''",
'hidden-categories'           => '{{PLURAL:$1|پټه وېشنيزه|پټې وېشنيزې}}',
'hidden-category-category'    => 'پټې وېشنيزې', # Name of the category where hidden categories will be listed
'category-article-count'      => '{{PLURAL:$2|په همدې وېشنيزه کې يواځې دغه لاندينی مخ شته.|دا {{PLURAL:$1|لاندينی مخ|$1 لانديني مخونه}}، له ټولټال $2 مخونو نه په دې وېشنيزه کې شته.}}',
'category-file-count-limited' => 'په اوسنۍ وېشنيزه کې {{PLURAL:$1|يوه دوتنه ده|$1 دوتنې دي}}.',
'listingcontinuesabbrev'      => 'پرله پسې',

'mainpagetext'      => "<big>'''MediaWiki په برياليتوب سره نصب شو.'''</big>",
'mainpagedocfooter' => "Consult the [http://meta.wikimedia.org/wiki/Help:Contents User's Guide] for information on using the wiki software.

== پيلول ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Configuration settings list]
* [http://www.mediawiki.org/wiki/Manual:FAQ د ميډياويکي ډېرځليزې پوښتنې]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mailing list]",

'about'          => 'په اړه',
'article'        => 'د منځپانګې مخ',
'newwindow'      => '(په نوې کړکۍ کې پرانيستل کېږي)',
'cancel'         => 'کوره کول',
'qbfind'         => 'موندل',
'qbedit'         => 'سمادول',
'qbpageoptions'  => 'همدا مخ',
'qbpageinfo'     => 'متن',
'qbmyoptions'    => 'زما پاڼې',
'qbspecialpages' => 'ځانګړي مخونه',
'moredotdotdot'  => 'نور ...',
'mypage'         => 'زما پاڼه',
'mytalk'         => 'زما خبرې اترې',
'anontalk'       => 'ددې IP لپاره خبرې اترې',
'navigation'     => 'ګرځښت',
'and'            => 'او',

# Metadata in edit box
'metadata_help' => 'مېټاډاټا:',

'errorpagetitle'    => 'تېروتنه',
'returnto'          => 'بېرته $1 ته وګرځه.',
'tagline'           => 'د {{SITENAME}} لخوا',
'help'              => 'لارښود',
'search'            => 'پلټنه',
'searchbutton'      => 'پلټل',
'go'                => 'ورځه',
'searcharticle'     => 'ورځه',
'history'           => 'د مخ پېښليک',
'history_short'     => 'پېښليک',
'info_short'        => 'مالومات',
'printableversion'  => 'د چاپ بڼه',
'permalink'         => 'تلپاتې تړن',
'print'             => 'چاپ',
'edit'              => 'سمادول',
'create'            => 'جوړول',
'editthispage'      => 'دا مخ سماد کړی',
'create-this-page'  => 'همدا مخ ليکل',
'delete'            => 'ړنګول',
'deletethispage'    => 'دا مخ ړنګ کړه',
'protect'           => 'ژغورل',
'protect_change'    => 'د ژغورنې بدلون',
'protectthispage'   => 'همدا مخ ژغورل',
'unprotect'         => 'نه ژغورل',
'unprotectthispage' => 'همدا مخ نه ژغورل',
'newpage'           => 'نوی مخ',
'talkpage'          => 'په همدې مخ خبرې اترې کول',
'talkpagelinktext'  => 'خبرې اترې',
'specialpage'       => 'ځانګړې پاڼه',
'personaltools'     => 'شخصي اوزار',
'postcomment'       => 'يوه تبصره ليکل',
'articlepage'       => 'د مخ مېنځپانګه ښکاره کول',
'talk'              => 'خبرې اترې',
'views'             => 'کتنې',
'toolbox'           => 'اوزاربکس',
'userpage'          => 'د کاروونکي پاڼه ښکاره کول',
'projectpage'       => 'د پروژې مخ ښکاره کول',
'imagepage'         => 'د انځورونو مخ کتل',
'mediawikipage'     => 'د پيغامونو مخ کتل',
'templatepage'      => 'د کينډۍ مخ ښکاره کول',
'viewhelppage'      => 'د لارښود مخ کتل',
'categorypage'      => 'د وېشنيزې مخ کتل',
'viewtalkpage'      => 'خبرې اترې کتل',
'otherlanguages'    => 'په نورو ژبو کې',
'redirectedfrom'    => '(له $1 نه راګرځول شوی)',
'redirectpagesub'   => 'ورګرځېدلی مخ',
'lastmodifiedat'    => 'دا مخ وروستی ځل په $2، $1 بدلون موندلی.', # $1 date, $2 time
'viewcount'         => 'همدا مخ {{PLURAL:$1|يو وار|$1 واره}} کتل شوی.',
'protectedpage'     => 'ژغورلی مخ',
'jumpto'            => 'ورټوپ کړه:',
'jumptonavigation'  => 'ګرځښت',
'jumptosearch'      => 'پلټل',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'د {{SITENAME}} په اړه',
'aboutpage'            => 'Project:په اړه',
'bugreports'           => 'د ستونزو راپورونه',
'bugreportspage'       => 'Project:د ستونزو راپور',
'copyright'            => 'دا مېنځپانګه د $1 له مخې ستاسو لاس رسي لپاره دلته ده.',
'copyrightpagename'    => 'د {{SITENAME}} رښتې',
'copyrightpage'        => '{{ns:project}}:رښتې',
'currentevents'        => 'اوسنۍ پېښې',
'currentevents-url'    => 'Project:اوسنۍ پېښې',
'disclaimers'          => 'ردادعاليکونه',
'disclaimerpage'       => 'Project:ټولګړی ردادعاليک',
'edithelp'             => 'د لارښود سماد',
'edithelppage'         => 'Help:سمادېدنه',
'helppage'             => 'Help:لړليک',
'mainpage'             => 'لومړی مخ',
'mainpage-description' => 'لومړی مخ',
'policy-url'           => 'Project:تګلاره',
'portal'               => 'ټولګړی ورټک',
'portal-url'           => 'Project:ټولګړی ورټک',
'privacy'              => 'د محرميت تګلاره',
'privacypage'          => 'Project:د محرميت_تګلاره',

'badaccess'        => 'د لاسرسۍ تېروتنه',
'badaccess-group0' => 'تاسو د غوښتل شوې کړنې د ترسره کولو اجازه نه لرۍ.',
'badaccess-group1' => 'د کومې کړنې غوښتنه چې تاسو کړې د $1 د ډلې کارونکو پورې محدوده ده.',
'badaccess-group2' => 'د کومې کړنې غوښتنه چې تاسو کړې د هغو کارونکو پورې محدوده ده کوم چې يو د $1 د ډلې څخه دي.',
'badaccess-groups' => 'د کومې کړنې غوښتنه چې تاسو کړې د هغو کارونکو پورې محدوده ده کوم چې يو د $1 د ډلې څخه دي.',

'ok'                      => 'هو',
'retrievedfrom'           => 'همدا مخ له "$1" څخه رااخيستل شوی',
'youhavenewmessages'      => 'تاسو $1 لری  ($2).',
'newmessageslink'         => 'نوي پيغامونه',
'newmessagesdifflink'     => 'وروستی بدلون',
'youhavenewmessagesmulti' => 'ستاسو لپاره په $1 کې نوي پېغام راغلي.',
'editsection'             => 'سمادول',
'editold'                 => 'سمادول',
'viewsourceold'           => 'سرچينې کتل',
'editsectionhint'         => 'د سمادلو برخه: $1',
'toc'                     => 'نيوليک',
'showtoc'                 => 'ښکاره کول',
'hidetoc'                 => 'پټول',
'viewdeleted'             => '$1 کتل؟',
'site-rss-feed'           => '$1 د آر اس اس کتنه',
'site-atom-feed'          => '$1 د اټوم کتنه',
'page-rss-feed'           => '"$1" د آر اس اس کتنه',
'feed-rss'                => 'آر اس اس',
'red-link-title'          => '$1 (تر اوسه پورې نه دی ليکل شوی)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'ليکنه',
'nstab-user'      => 'د کارونکي پاڼه',
'nstab-media'     => 'د رسنۍ مخ',
'nstab-special'   => 'ځانګړی',
'nstab-project'   => 'د پروژې مخ',
'nstab-image'     => 'دوتنه',
'nstab-mediawiki' => 'پيغام',
'nstab-template'  => 'کينډۍ',
'nstab-help'      => 'لارښود',
'nstab-category'  => 'وېشنيزه',

# Main script and global functions
'nosuchaction'      => 'هېڅ داسې کومه کړنه نشته',
'nosuchspecialpage' => 'داسې هېڅ کوم ځانګړی مخ نشته',
'nospecialpagetext' => "<big>'''تاسو د يو ناسم ځانګړي مخ غوښتنه کړې.'''</big>

تاسو کولای شی چې د سمو ځانګړو مخونو لړليک په [[Special:SpecialPages|{{int:specialpages}}]] کې ومومۍ.",

# General errors
'error'                => 'تېروتنه',
'databaseerror'        => 'د ډاټابېز تېروتنه',
'missingarticle-diff'  => '(توپير: $1، $2)',
'internalerror'        => 'کورنۍ تېروتنه',
'internalerror_info'   => 'کورنۍ تېروتنه: $1',
'filecopyerror'        => 'د "$1" په نامه دوتنه مو "$2" ته و نه لمېسلای شوه.',
'filerenameerror'      => 'د "$1" په نامه د دوتنې نوم "$2" ته بدل نه شو.',
'filedeleteerror'      => 'د "$1" دوتنه ړنګه نه شوه.',
'directorycreateerror' => 'د "$1" په نامه ليکلړ جوړ نه شو.',
'filenotfound'         => '"$1" په نوم دوتنه مو و نه شوه موندلای.',
'fileexistserror'      => 'د "$1" په نامه دوتنه نه ليکل کېږي: دوتنه د پخوا نه دلته شته',
'badarticleerror'      => 'دا کړنه پدې مخ نه شي ترسره کېدلای.',
'cannotdelete'         => 'د اړونده مخ يا دوتنې ړنګېدنه ترسره نه شوه.  (کېدای شي چې دا د بل چا لخوا نه پخوا ړنګه شوې وي.)',
'badtitle'             => 'ناسم سرليک',
'badtitletext'         => 'ستاسو د غوښتل شوي مخ سرليک يا سم نه وو، د سرليک ځای تش وو او يا هم د ژبو خپلمنځي تړنې څخه يا د ويکي ګانو خپلمنځي سرليکونو څخه يو ناسم توری پکې کارول شوی.
کېدای شي چې ستاسو په ورکړ شوي سرليک کې يو يا ګڼ شمېر داسې توري وي چې د سرليک په توګه بايد و نه کارېږي.',
'viewsource'           => 'سرچينې کتل',
'viewsourcefor'        => 'د $1 لپاره',
'protectedpagetext'    => 'همدا مخ د سمادولو د مخنيوي په تکل تړل شوی دی.',
'viewsourcetext'       => 'تاسو د همدغه مخ توکي او سرچينې کتلی او لمېسلی شی:',
'protectedinterface'   => 'په همدې مخ کې د پوستکالي د ليدنمخ متن دی او دا متن د ناسمو کارولو د مخنيوي په تکل تړل شوی.',
'namespaceprotected'   => "تاسو ته د '''$1''' په نوم-تشيال کې د مخونو د سمادولو اجازه نشته.",
'ns-specialprotected'  => 'ځانګړې مخونه د سمادولو وړ نه دي.',

# Login and logout pages
'logouttitle'                => 'کارن-حساب نه وتنه',
'logouttext'                 => '<strong>تاسو اوس د غونډال نه ووتلی.</strong>

تاسو کولای شی چې پرته د کارن-نوم نه {{SITENAME}} په ورکنومي توګه وکاروی، او يا هم تاسو کولای شی چې په همدې کارن-نوم يا په کوم بل کارن-نوم خپلې ليکنې خپرې کړی. 
يادونه دې وي چې ځينې مخونو کې به تاسو لا تر اوسه پورې غونډال کې ننوتي ښکاری، تر څو تاسو د خپل کتنمل حافظه نه وي سپينه کړی.',
'welcomecreation'            => '==$1 ښه راغلاست! ==

ستاسو کارن-حساب جوړ شو. لطفاً د [[Special:Preferences|{{SITENAME}} غوره توبونو]] بدلول مو مه هېروی.',
'loginpagetitle'             => 'کارن-حساب ته ننوتنه',
'yourname'                   => 'کارن-نوم:',
'yourpassword'               => 'پټنوم:',
'yourpasswordagain'          => 'پټنوم بيا وليکه',
'remembermypassword'         => 'زما پټنوم پدې کمپيوټر په ياد ولره!',
'loginproblem'               => '<b>همدې غونډال ته ستاسو په ننوتنه کې يوه ستونزه راپېښه شوه!</b><br />بيا يې وآزمويۍ!',
'login'                      => 'ننوتل',
'nav-login-createaccount'    => 'ننوتل / کارن-حساب جوړول',
'loginprompt'                => 'ددې لپاره چې {{SITENAME}} کې ننوځۍ نو بايد چې ستاسو د کمپيوټر کوکيز چارن وي.',
'userlogin'                  => 'ننوتل / کارن-حساب جوړول',
'logout'                     => 'وتل',
'userlogout'                 => 'وتل',
'notloggedin'                => 'غونډال کې نه ياست ننوتي',
'nologin'                    => 'کارن نوم نه لرې ؟ $1.',
'nologinlink'                => 'يو کارن-حساب جوړول',
'createaccount'              => 'کارن-حساب جوړول',
'gotaccount'                 => 'آيا وار دمخې يو کارن-حساب لری؟ $1.',
'gotaccountlink'             => 'ننوتل',
'createaccountmail'          => 'د برېښليک له مخې',
'badretype'                  => 'دا پټنوم چې تاسو ليکلی د پخواني پټنوم سره ورته نه دی.',
'userexists'                 => 'کوم کارن نوم چې تاسو ورکړی هغه بل چا کارولی. لطفاً يو بل ډول نوم وټاکۍ.',
'youremail'                  => 'برېښليک *',
'username'                   => 'کارن-نوم:',
'uid'                        => 'د کارونکي پېژندنه:',
'prefs-memberingroups'       => 'د {{PLURAL:$1|ډلې|ډلو}} غړی:',
'yourrealname'               => 'اصلي نوم:',
'yourlanguage'               => 'ژبه:',
'yournick'                   => 'کورنی نوم:',
'badsiglength'               => 'ستاسو لاسليک ډېر اوږد دی.
پکار ده چې لاسليک مو له $1 {{PLURAL:$1|توری|تورو}} نه لږ وي.',
'email'                      => 'برېښليک',
'prefs-help-realname'        => 'د اصلي نوم ليکل ستاسو په خوښه دی خو که تاسو خپل اصلي نوم وټاکۍ پدې سره به ستاسو ټول کارونه او ونډې ستاسو د نوم په اړوندولو کې وکارېږي.',
'loginerror'                 => 'د ننوتنې ستونزه',
'prefs-help-email'           => 'د برېښليک ليکل ستاسو په خوښه دی، خو په ورکړې سره به يې نور کارونکي پدې وتوانېږي چې ستاسو سره د کارن-نوم او يا هم د کارونکي خبرې اترې لخوا، پرته له دې چې ستاسو پېژندنه وشي، اړيکې ټينګې کړي.',
'prefs-help-email-required'  => 'ستاسو د برېښليک پته پکار ده.',
'noname'                     => 'تاسو تر اوسه پورې کوم کره کارن نوم نه دی ځانګړی کړی.',
'loginsuccesstitle'          => 'ننوتل مو برياليتوب سره ترسره شوه',
'loginsuccess'               => "'''تاسو اوس {{SITENAME}} کې د \"\$1\" په نوم ننوتي ياست.'''",
'nosuchuser'                 => 'د "$1" په نامه هېڅ کوم کارونکی نه شته. مهرباني وکړی خپل ټاپ کړی نوم وګوری چې سم مو ليکلی او که نه، او يا هم  که تاسو غواړی نو په همدې نوم يو نوی کارن-حساب جوړ کړی.',
'nosuchusershort'            => 'د "<nowiki>$1</nowiki>" په نوم هېڅ کوم کارن-حساب نشته. لطفاً خپل د نوم ليکلې بڼې ته ځير شی چې پکې تېروتنه نه وي.',
'nouserspecified'            => 'تاسو ځان ته کوم کارن نوم نه دی ځانګړی کړی.',
'wrongpassword'              => 'ناسم پټنوم مو ليکلی. لطفاً يو ځل بيا يې وليکۍ.',
'wrongpasswordempty'         => 'تاسو پټنوم نه دی ليکلی. لطفاً سر له نوي يې وليکۍ.',
'passwordtooshort'           => 'ستاسو پټنوم ناسم دی او يا هم ډېر لنډ دی.
بايد چې پټنوم مو لږ تر لږه {{PLURAL:$1|1 توری|$1 توري}} ولري او پکار ده چې د پټنوم او کارن-نوم ترمېنځ مو هم توپير وي.',
'mailmypassword'             => 'پټنوم رابرېښليک کول',
'passwordremindertitle'      => 'د {{SITENAME}} لپاره نوی لنډمهاله پټنوم',
'passwordremindertext'       => 'يو چا (کېدای شي چې تاسو، د $1 IP پتې نه)
د {{SITENAME}} ($4) وېبځي لپاره د يوه نوي پټنوم د ورلېږلو غوښتنه کړې.
د "$2" په نوم کارونکي لپاره نوی پټنوم اوس "$3" دی.
تاسو بايد چې اوس غونډال ته په همدغه پټنوم ورننوځی او بيا وروسته خپل پټنوم په خپله خوښه بدل کړی.

که چېرته ستاسو نه پرته کوم بل چا دغه غوښتنه کړې وي او يا هم تاسو ته بېرته خپل پټنوم در پزړه شوی وي او تاسو د خپل د پټنوم د بدلولو هيله نه لری، نو تاسو همدا پيغام بابېزه وګڼی او د پخوا په څېر خپل هماغه پخوانی پټنوم وکاروی.',
'noemail'                    => 'د "$1" کارونکي په نامه هېڅ کومه برېښليک پته نه ده ثبته شوې.',
'passwordsent'               => 'د "$1" په نوم ثبت شوي غړي/غړې لپاره يو نوی پټنوم د هغه/هغې د برېښليک پتې ته ولېږل شو.
لطفاً کله چې پټنوم مو ترلاسه کړ نو بيا غونډال ته ننوځۍ.',
'blocked-mailpassword'       => 'ستاسو په IP پتې بنديز لګېدلی او تاسو نه شی کولای چې ليکنې وکړی، په همدې توګه تاسو نه شی کولای چې د پټنوم د پرځای کولو کړنې وکاروی دا ددې لپاره چې د وراني مخنيوی وشي.',
'eauthentsent'               => 'ستاسو ورکړ شوې برېښليک پتې ته مو يو تاييدي برېښليک درولېږی.
تر دې دمخه چې ستاسو کارن-حساب ته کوم بل برېښليک درولېږو، پکار ده چې تاسو په برېښليک کې درلېږل شوې لارښوونې پلي کړی او ددې پخلی وکړی چې همدا کارن-حساب په رښتيا ستاسو دی.',
'mailerror'                  => 'د برېښليک د لېږلو ستونزه: $1',
'acct_creation_throttle_hit' => 'اوبښۍ، تاسو وار دمخې پدغه $1 نوم کارن-حساب جوړ کړی. تاسو نه شی کولای چې نور جوړ کړی.',
'emailauthenticated'         => 'ستاسو برېښليک پته په $1 د منلو وړ وګرځېده.',
'emailnotauthenticated'      => 'ستاسو د برېښليک پته لا تر اوسه پورې د منلو وړ نه ده ګرځېدلې. د اړوندو بېلوونکو نښو په هکله تاسو ته هېڅ کوم برېښليک نه لېږل کېږي.',
'noemailprefs'               => 'ددې لپاره چې دا کړنې کار وکړي نو تاسو يو برېښليک وټاکۍ.',
'emailconfirmlink'           => 'د خپل د برېښليک پتې پخلی وکړی',
'accountcreated'             => 'کارن-حساب مو جوړ شو.',
'accountcreatedtext'         => 'د $1 لپاره يو کارن-حساب جوړ شو.',
'createaccount-title'        => 'د {{SITENAME}} د کارن-حساب جوړېدنه',
'loginlanguagelabel'         => 'ژبه: $1',

# Password reset dialog
'resetpass_bad_temporary' => 'لنډمهالی پټنوم مو سم نه دی. کېدای شي تاسو وار دمخې خپل پټنوم برياليتوب سره بدل کړی وي او يا هم د نوي لنډمهالي پټنوم غوښتنه مو کړې وي.',
'resetpass_forbidden'     => 'په {{SITENAME}} کې مو پټنوم نه شي بدلېدلای',

# Edit page toolbar
'bold_sample'     => 'روڼ ليک',
'bold_tip'        => 'روڼ ليک',
'italic_sample'   => 'کوږ ليک',
'italic_tip'      => 'کوږ ليک',
'link_sample'     => 'د تړن سرليک',
'link_tip'        => 'کورنی تړن',
'extlink_sample'  => 'http://www.example.com د تړنې سرليک',
'extlink_tip'     => 'باندنۍ تړنې (د http:// مختاړی مه هېروی)',
'headline_sample' => 'سرليک',
'headline_tip'    => 'د ۲ کچې سرليک',
'math_sample'     => 'فورمول دلته ځای کړی',
'math_tip'        => 'شمېرپوهنيز فورمول (LaTeX)',
'nowiki_sample'   => 'دلته دې بې بڼې متن ځای پر ځای شي',
'nowiki_tip'      => 'د ويکي بڼه نيونه بابېزه ګڼل',
'image_tip'       => 'خښه شوې دوتنه',
'media_tip'       => 'د دوتنې تړنه',
'sig_tip'         => 'ستاسو لاسليک د وخت د ټاپې سره',
'hr_tip'          => 'څنډيزه ليکه (ددې په کارولو کې سپما وکړۍ)',

# Edit pages
'summary'                  => 'لنډيز',
'subject'                  => 'سکالو/سرليک',
'minoredit'                => 'دا يوه وړوکې سمادېدنه ده',
'watchthis'                => 'همدا مخ کتل',
'savearticle'              => 'مخ خوندي کول',
'preview'                  => 'مخکتنه',
'showpreview'              => 'مخکتنه',
'showlivepreview'          => 'ژوندۍ مخکتنه',
'showdiff'                 => 'بدلونونه ښکاره کول',
'anoneditwarning'          => "'''يادونه:''' تاسو غونډال ته نه ياست ننوتي. ستاسو IP پته به د دې مخ د سمادولو په پېښليک کې ثبت شي.",
'missingcommenttext'       => 'لطفاً تبصره لاندې وليکۍ.',
'summary-preview'          => 'د لنډيز مخکتنه',
'subject-preview'          => 'موضوع/سرليک مخکتنه',
'blockedtitle'             => 'د کارونکي مخه نيول شوې',
'blockedtext'              => "<big>'''ستاسو د کارن-نوم يا آی پي پتې مخنيوی شوی.'''</big>

همدا بنديز د $1 له خوا پر تاسو لږېدلی. او د همدې کړنې سبب دی ''$2''.

* د مخنيوي د پېل نېټه: $8
* د مخنيوي د پای نېټه: $6
* بنديزونه دي پر: $7

تاسو کولای شی چې د $1 او يا هم د يو بل [[{{MediaWiki:Grouppage-sysop}}|پازوال]] سره اړيکې ټينګې کړی او د بنديز ستونزې مو هوارې کړی.
تاسو نه شی کولای چې د 'همدې کارونکي ته برېښلک لېږل ' کړنې نه ګټه پورته کړی تر څو چې تاسو د خپل کارن-حساب په [[Special:Preferences|غوره توبونو]] کې يوه کره برېښليک پته نه وي ځانګړې کړې او تر دې بريده چې پر تاسو د هغې د کارولو بنديز نه وي لګېدلی.
ستاسو د دم مهال آی پي پته ده $3، او ستاسو د مخنيوي پېژند #$5 دی. مهرباني وکړۍ د خپلې يادونې پر مهال د دغو دوو څخه د يوه او يا هم د دواړو ورکول مه هېروۍ.",
'blockednoreason'          => 'هېڅ سبب نه دی ورکړ شوی',
'blockedoriginalsource'    => "د '''$1''' سرچينې لاندې ښودل شوي:",
'whitelistedittitle'       => 'که د سمادولو تکل لری نو بايد غونډال ته ورننوځۍ.',
'whitelistedittext'        => 'ددې لپاره چې سمادول ترسره کړی تاسو بايد $1.',
'loginreqtitle'            => 'غونډال کې ننوتنه پکار ده',
'loginreqlink'             => 'ننوتل',
'loginreqpagetext'         => 'د نورو مخونو د کتلو لپاره تاسو بايد $1 وکړۍ.',
'accmailtitle'             => 'پټنوم ولېږل شو.',
'accmailtext'              => 'د "$1" لپاره پټنوم $2 ته ولېږل شو.',
'newarticle'               => '(نوی)',
'newarticletext'           => "تاسو د يوه داسې تړنې څارنه کړې چې لا تر اوسه پورې شتون نه لري.
که همدا مخ ليکل غواړۍ، نو په لانديني چوکاټ کې خپل متن وټاپۍ (د لا نورو مالوماتو لپاره د [[{{MediaWiki:Helppage}}|لارښود مخ]] وګورۍ).
که چېرته تاسو دلته په غلطۍ سره راغلي ياست، نو يواځې د خپل د کتنمل '''مخ پر شا''' تڼۍ مو وټوکۍ.",
'anontalkpagetext'         => "----''دا د بې نومه کارونکو لپاره چې کارن نوم يې نه دی جوړ کړی او يا هم خپل کارن نوم نه دی کارولی، د سکالو پاڼه ده. نو ددې پخاطر مونږ د هغه کارونکي/هغې کارونکې د انټرنېټ شمېره يا IP پته د نوموړي/نوموړې د پېژندلو لپاره کاروو. داسې يوه IP پته د ډېرو کارونکو لخوا هم کارېدلی شي. که تاسو يو بې نومه کارونکی ياست او تاسو ته نااړونده پېغامونه او تبصرې اشاره شوي، نو لطفاً د نورو بې نومو کارونکو او ستاسو ترمېنځ د ټکنتوب مخ نيونې لپاره [[Special:UserLogin|کارن-حساب جوړول يا ننوتنه]] وټوکۍ.''",
'noarticletext'            => 'دم مهال په همدې مخ کې هېڅ متن نشته، تاسو کولای شی چې  په نورو مخونو کې [[Special:Search/{{PAGENAME}}|د همدې سرليک لپاره پلټنه]] وکړۍ، او يا هم [{{fullurl:{{FULLPAGENAME}}|action=edit}} همدا مخ سماد کړۍ].',
'clearyourcache'           => "'''يادونه:''' د غوره توبونو د خوندي کولو وروسته، ددې لپاره چې تاسو خپل سر ته رسولي ونجونه وګورۍ نو پکار ده چې د خپل بروزر ساتل شوې حافظه تازه کړی. د '''Mozilla / Firefox / Safari:''' لپاره د ''Shift'' تڼۍ نيولې وساتی کله مو چې په ''Reload''، ټک واهه، او يا هم ''Ctrl-Shift-R'' تڼۍ کېښکاږۍ (په Apple Mac کمپيوټر باندې ''Cmd-Shift-R'' کېښکاږۍ); '''IE:''' د ''Ctrl'' تڼۍ کېښکاږۍ کله مو چې په ''Refresh'' ټک واهه، او يا هم د ''Ctrl-F5'' تڼۍ کېښکاږۍ; '''Konqueror:''' بروزر کې يواځې ''Reload'' ته ټک ورکړۍ، او يا په ''F5''; د '''Opera''' کارونکو ته پکار ده چې په بشپړه توګه د خپل کمپيوټر ساتل شوې حافظه تازه کړي چې پدې توګه کېږي ''Tools→Preferences''.",
'updated'                  => '(تازه)',
'note'                     => '<strong>يادونه:</strong>',
'previewnote'              => '<strong>دا يواځې مخکتنه ده، تاسو چې کوم بدلونونه ترسره کړي، لا تر اوسه پورې نه دي خوندي شوي!</strong>',
'editing'                  => 'سمادېدنه $1',
'editingsection'           => 'سمادېدنه $1 (برخه)',
'editconflict'             => 'په سمادولو کې خنډ: $1',
'yourtext'                 => 'ستاسو متن',
'yourdiff'                 => 'توپيرونه',
'copyrightwarning'         => 'لطفاً په پام کې وساتۍ چې ټولې هغه ونډې چې تاسو يې {{SITENAME}} کې ترسره کوی هغه د $2 له مخې د خپرولو لپاره ګڼل کېږي (د لانورو تفصيلاتو لپاره $1 وګورۍ). که تاسو نه غواړۍ چې ستاسې په ليکنو کې په بې رحمۍ سره لاسوهنې (سمادېدنې) وشي او د نورو په غوښتنه پسې لانورې هم خپرې شي، نو دلته يې مه ځای پر ځای کوی..<br />
تاسو زمونږ سره دا ژمنه هم کوی چې تاسو پخپله دا ليکنه کښلې، او يا مو د ټولګړو پاڼو او يا هم ورته وړيا سرچينو نه کاپي کړې ده <strong>لطفاً د ليکوال د اجازې نه پرته د خوندي حقونو ليکنې مه خپروی!</strong>',
'longpagewarning'          => '<strong>پاملرنه: همدا مخ $1 کيلوبايټه اوږد دی؛ کېدای شي چې ځينې براوزرونه د ۳۲ کيلوبايټ نه د اوږدو مخونو په سمادونه کې ستونزه رامېنځ ته کړي.
لطفاً د مخ په لنډولو او په وړو برخو وېشلو باندې غور وکړی.</strong>',
'longpageerror'            => '<strong>ستونزه: کوم متن چې دلته تاسو ليکلی، $1 کيلوبايټه اوږد دی او دا د همدې مخ د لوړترين ټاکلي بريده، $2 کيلوبايټه، څخه اوږد دی.
ستاسو متن نه شي خوندي کېدلای.</strong>',
'semiprotectedpagewarning' => "'''يادونه:''' همدا مخ تړل شوی دی او يواځې ثبت شوي کارونکي کولای شي چې په دې مخ کې بدلونونه راولي.",
'templatesused'            => 'په دې مخ کارېدلي کينډۍ:',
'templatesusedpreview'     => 'په دې مخکتنه کې کارېدلي کينډۍ:',
'templatesusedsection'     => 'په دې برخه کارېدلي کينډۍ:',
'template-protected'       => '(ژغورل شوی)',
'template-semiprotected'   => '(نيم-ژغورلی)',
'nocreatetext'             => '{{SITENAME}} د نوو مخونو د جوړولو وړتيا محدوده کړې.
تاسو بېرته پر شا تللای شی او په شته مخونو کې سمادېدنې ترسره کولای شی، او يا هم [[Special:UserLogin|غونډال ته ننوتلای او يو کارن-حساب جوړولای شی]].',
'recreate-deleted-warn'    => "'''ګواښ: تاسو د يو داسې مخ بياجوړونه کوی کوم چې يو ځل پخوا ړنګ شوی وو.'''

پکار ده چې تاسو په دې ځان پوه کړی چې ايا دا تاسو ته وړ ده چې د همدې مخ سمادېدنه په پرله پسې توګه وکړی.
ستاسو د اسانتياوو لپاره د همدې مخ د ړنګېدلو يادښت هم ورکړ شوی:",

# Account creation failure
'cantcreateaccounttitle' => 'کارن-حساب نه شي جوړېدای',

# History pages
'viewpagelogs'        => 'د همدغه مخ يادښتونه کتل',
'nohistory'           => 'ددې مخ لپاره د سمادېدنې هېڅ کوم پېښليک نه شته.',
'currentrev'          => 'اوسنۍ بڼه',
'revisionasof'        => 'د $1 پورې شته مخليدنه',
'revision-info'       => 'د $1 پورې شته مخليدنه، د $2 لخوا ترسره شوې',
'previousrevision'    => '← زړه بڼه',
'nextrevision'        => '← نوې بڼه',
'currentrevisionlink' => 'اوسنۍ بڼه',
'cur'                 => 'اوسنی',
'next'                => 'راتلونکي',
'last'                => 'وروستنی',
'page_first'          => 'لومړنی',
'page_last'           => 'وروستنی',
'histlegend'          => 'د توپير ټاکنه: د هرې هغې بڼې پرتلنه چې تاسو غواړۍ نو د هماغې بڼې چوکاټک په نښه کړی او بيا په لاندينۍ تڼۍ وټوکۍ.<br />
لنډيز: (اوس) = د اوسنۍ بڼې سره توپير،
(وروست) = د وروستۍ بڼې سره توپير، و = وړه سمادېدنه.',
'deletedrev'          => '[ړنګ شو]',
'histfirst'           => 'پخواني',
'histlast'            => 'تازه',
'historysize'         => '({{PLURAL:$1|1 بايټ|$1 بايټونه}})',
'historyempty'        => '(تش)',

# Revision feed
'history-feed-item-nocomment' => '$1 په $2', # user at time

# Revision deletion
'rev-delundel'    => 'ښکاره کول/ پټول',
'pagehist'        => 'د مخ پېښليک',
'revdelete-uname' => 'کارن-نوم',

# Diffs
'history-title'           => 'د "$1" د پېښليک مخليدنه',
'difference'              => '(د بڼو تر مېنځ توپير)',
'lineno'                  => '$1 کرښه:',
'compareselectedversions' => 'ټاکلې بڼې سره پرتله کول',
'editundo'                => 'ناکړ',
'diff-multi'              => '({{PLURAL:$1|يوه منځګړې مخليدنه نه ده ښکاره شوې|$1 منځګړې مخليدنې نه دي ښکاره شوي}}.)',

# Search results
'searchresults'         => 'د لټون پايلې',
'searchsubtitle'        => "تاسو د '''[[:$1]]''' لپاره لټون کړی",
'searchsubtitleinvalid' => "تاسو د '''$1''' لپاره لټون کړی",
'noexactmatch'          => "'''تر اوسه پورې د \"\$1\" په نوم هېڅ کوم مخ نشته.''' تاسو کولای شی چې [[:\$1|همدا مخ جوړ کړی]].",
'prevn'                 => 'تېر $1',
'nextn'                 => 'راتلونکي $1',
'viewprevnext'          => '($1) ($2) ($3) ښکاره کول',
'search-suggest'        => 'آيا همدا ستاسو موخه ده: $1',
'search-relatedarticle' => 'اړونده',
'searchall'             => 'ټول',
'powersearch'           => 'پرمختللې پلټنه',
'powersearch-legend'    => 'پرمختللې پلټنه',

# Preferences page
'preferences'           => 'غوره توبونه',
'mypreferences'         => 'زما غوره توبونه',
'prefs-edits'           => 'د سمادونو شمېر:',
'prefsnologin'          => 'غونډال کې نه ياست ننوتي',
'prefsnologintext'      => 'ددې لپاره چې د کارونکي غوره توبونه وټاکۍ نو تاسو ته پکار ده چې لومړی غونډال کې [[Special:UserLogin|ننوتنه]] ترسره کړی.',
'qbsettings-none'       => 'هېڅ',
'changepassword'        => 'پټنوم بدلول',
'skin'                  => 'بڼه',
'math'                  => 'شمېرپوهنه',
'dateformat'            => 'د نېټې بڼه',
'datedefault'           => 'هېڅ نه ټاکل',
'datetime'              => 'نېټه او وخت',
'math_unknown_error'    => 'ناجوته ستونزه',
'math_unknown_function' => 'ناجوته کړنه',
'prefs-personal'        => 'د کارونکي پېژنليک',
'prefs-rc'              => 'وروستي بدلونونه',
'prefs-watchlist'       => 'کتلی لړليک',
'prefs-watchlist-days'  => 'د ورځو شمېر چې په کتلي لړليک کې به ښکاري:',
'prefs-misc'            => 'بېلابېل',
'saveprefs'             => 'خوندي کول',
'resetprefs'            => 'بيا سمول',
'oldpassword'           => 'زوړ پټنوم:',
'newpassword'           => 'نوی پټنوم:',
'retypenew'             => 'نوی پټنوم بيا وليکه:',
'textboxsize'           => 'سمادېدنه',
'searchresultshead'     => 'پلټل',
'recentchangesdays'     => 'د هغو ورځو شمېر وټاکی چې په وروستي بدلونو کې يې ليدل غواړی:',
'recentchangescount'    => 'د هغو سمادونو شمېر چې په وروستي بدلونو کې يې ليدل غواړی:',
'savedprefs'            => 'ستاسو غوره توبونه خوندي شوه.',
'timezonelegend'        => 'د وخت سيمه',
'localtime'             => 'سيمه ايز وخت',
'servertime'            => 'د پالنګر وخت',
'allowemail'            => 'د نورو کارونکو لخوا د برېښليک رالېږل چارن کړه',
'defaultns'             => 'په دغو نوم-تشيالونو کې د ټاکل شوو سمونونو له مخې لټون وکړی:',
'files'                 => 'دوتنې',

# User rights
'userrights-user-editname' => 'يو کارن نوم وليکۍ:',
'userrights-editusergroup' => 'د کاروونکو ډلې سمادول',
'saveusergroups'           => 'د کارونکي ډلې خوندي کول',
'userrights-groupsmember'  => 'غړی د:',
'userrights-reason'        => 'د بدلون سبب:',

# Groups
'group'     => 'ډله:',
'group-all' => '(ټول)',

'grouppage-sysop' => '{{ns:project}}:پازوالان',

# User rights log
'rightslog'  => 'د کارونکي د رښتو يادښت',
'rightsnone' => '(هېڅ نه)',

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|بدلون|بدلونونه}}',
'recentchanges'                  => 'وروستي بدلونونه',
'recentchangestext'              => 'په همدې مخ باندې د ويکي ترټولو تازه وروستي بدلونونه وڅارۍ.',
'recentchanges-feed-description' => 'همدلته د ويکي ترټولو تازه وروستي بدلونونه وڅارۍ او وګورۍ چې څه پېښ شوي.',
'rcnote'                         => "دلته لاندې {{PLURAL:$1|وروستی '''1''' بدلون دی|وروستي '''$1''' بدلونونه دي}} چې په  {{PLURAL:$2| يوې ورځ|'''$2''' ورځو}} کې تر $4 نېټې او $5 بجو پېښ شوي.",
'rcnotefrom'                     => "په همدې ځای کې لاندې هغه بدلونونه دي چې د '''$2''' نه راپدېخوا پېښ شوي (تر '''$1''' پورې ښکاره شوي).",
'rclistfrom'                     => 'هغه بدلونونه ښکاره کړی چې له $1 نه پيلېږي',
'rcshowhideminor'                => 'وړې سمادېدنې $1',
'rcshowhidebots'                 => 'bots $1',
'rcshowhideliu'                  => 'غونډال ته ننوتي $1 کارونکي',
'rcshowhideanons'                => 'بې نومه کارونکي $1',
'rcshowhidepatr'                 => '$1 څارلي سمادېدنې',
'rcshowhidemine'                 => 'زما سمادېدنې $1',
'rclinks'                        => 'هغه وروستي $1 بدلونونه ښکاره کړی چې په $2 ورځو کې پېښ شوي<br />$3',
'diff'                           => 'توپير',
'hist'                           => 'پېښليک',
'hide'                           => 'پټول',
'show'                           => 'ښکاره کول',
'minoreditletter'                => 'و',
'newpageletter'                  => 'نوی',
'boteditletter'                  => 'باټ',
'newsectionsummary'              => '/* $1 */ نوې برخه',

# Recent changes linked
'recentchangeslinked'          => 'اړونده بدلونونه',
'recentchangeslinked-title'    => '"$1" ته اړونده بدلونونه',
'recentchangeslinked-noresult' => 'په ورکړ شوي موده کې هېڅ کوم بدلونونه په تړل شويو مخونو کې نه دي راپېښ شوي.',
'recentchangeslinked-summary'  => "دا د هغه بدلونونو لړليک دی چې وروستۍ ځل په تړن لرونکيو مخونو کې د يوه ځانګړي مخ (او يا هم د يوې ځانګړې وېشنيزې غړو) نه رامېنځ ته شوي.
[[Special:Watchlist|ستاسو د کتلي لړليک]] مخونه په '''روڼ ليک''' کې ښکاري.",
'recentchangeslinked-page'     => 'د مخ نوم:',

# Upload
'upload'                => 'دوتنه پورته کول',
'uploadbtn'             => 'دوتنه پورته کول',
'reupload'              => 'بيا پورته کول',
'uploadnologin'         => 'غونډال کې نه ياست ننوتي',
'uploadnologintext'     => 'ددې لپاره چې دوتنې پورته کړای شۍ، تاسو ته پکار ده چې لومړی غونډال کې [[Special:UserLogin|ننوتنه]] ترسره کړی.',
'uploaderror'           => 'د پورته کولو ستونزه',
'uploadtext'            => "د دوتنو د پورته کولو لپاره د لانديني چوکاټ نه کار واخلۍ، که چېرته غواړۍ چې د پخوانيو پورته شوو انځورونو په اړه لټون وکړۍ او يا يې وکتلای شۍ نو بيا د [[Special:ImageList|پورته شوو دوتنو لړليک]] ته لاړ شی، د پورته شوو دوتنو او ړنګ شوو دوتنو يادښتونه په [[Special:Log/upload|پورته شوي يادښت]] کې کتلای شی.

ددې لپاره چې يوه مخ ته انځور ورواچوی، نو بيا پدې ډول تړن (لېنک) وکاروی
'''<nowiki>[[</nowiki>Image:File.jpg<nowiki>]]</nowiki>''',
'''<nowiki>[[</nowiki>Image:File.png|alt text<nowiki>]]</nowiki>''' او يا هم د رسنيزو دوتنو لپاره د راساً تړن (لېنک) چې په دې ډول دی
'''<nowiki>[[</nowiki>Media:File.ogg<nowiki>]]</nowiki>''' وکاروی.",
'uploadlogpage'         => 'د پورته شويو دوتنو يادښت',
'uploadlogpagetext'     => 'دا لاندې د نوو پورته شوو دوتنو لړليک دی.',
'filename'              => 'د دوتنې نوم',
'filedesc'              => 'لنډيز',
'fileuploadsummary'     => 'لنډيز:',
'filesource'            => 'سرچينه:',
'uploadedfiles'         => 'پورته شوې دوتنې',
'ignorewarnings'        => 'هر ډول ګواښونه له پامه غورځول',
'minlength1'            => 'پکار ده چې د دوتنو نومونه لږ تر لږه يو حرف ولري.',
'badfilename'           => 'ددغې دوتنې نوم "$1" ته واوړېده.',
'filetype-badmime'      => 'د MIME بڼې "$1" د دوتنو د پورته کولو اجازه نشته.',
'fileexists'            => 'د پخوا نه پدې نوم يوه دوتنه شته، که تاسو ډاډه نه ياست او يا هم که تاسو غواړۍ چې بدلون پکې راولۍ، لطفاً <strong><tt>$1</tt></strong> وګورۍ.',
'fileexists-extension'  => 'په همدې نوم يوه بله دوتنه د پخوا نه شته:<br />
د پورته کېدونکې دوتنې نوم: <strong><tt>$1</tt></strong><br />
د پخوا نه شته دوتنه: <strong><tt>$2</tt></strong><br />
لطفاً يو داسې نوم وټاکی چې د پخوانۍ دوتنې سره توپير ولري.',
'fileexists-forbidden'  => 'د پخوا نه پدې نوم يوه دوتنه شته؛ لطفاً بېرته وګرځۍ او همدغه دوتنه بيا په يوه نوي نوم پورته کړی. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate' => 'همدا دوتنه د {{PLURAL:$1|لاندينۍ دوتنې|لاندينيو دوتنو}} غبرګه لمېسه ده:',
'savefile'              => 'دوتنه خوندي کړه',
'uploadedimage'         => '"[[$1]]" پورته شوه',
'uploaddisabled'        => 'پورته کول ناچارن شوي',
'uploadvirus'           => 'دا دوتنه ويروس لري! تفصيل: $1',
'sourcefilename'        => 'د سرچينيزې دوتنې نوم:',
'upload-maxfilesize'    => 'د دوتنې تر ټولو لويه کچه: $1',
'watchthisupload'       => 'همدا مخ کتل',

'upload-file-error' => 'کورنۍ ستونزه',

'nolicense'          => 'هېڅ نه دي ټاکل شوي',
'upload_source_file' => '(ستاسو په کمپيوټر کې يوه دوتنه)',

# Special:ImageList
'imagelist_search_for'  => 'د انځور د نوم لټون:',
'imgfile'               => 'دوتنه',
'imagelist'             => 'د دوتنو لړليک',
'imagelist_date'        => 'نېټه',
'imagelist_name'        => 'نوم',
'imagelist_user'        => 'کارونکی',
'imagelist_size'        => 'کچه (bytes)',
'imagelist_description' => 'څرګندونه',

# Image description page
'filehist'                  => 'د دوتنې پېښليک',
'filehist-help'             => 'په يوې نېټې/يوه وخت وټوکۍ چې د هماغه وخت او نېټې دوتنه چې په هماغه وخت کې څنګه ښکارېده هماغسې درښکاره شي.',
'filehist-deleteall'        => 'ټول ړنګول',
'filehist-deleteone'        => 'همدا ړنګول',
'filehist-revert'           => 'په څټ ګرځول',
'filehist-current'          => 'اوسنی',
'filehist-datetime'         => 'نېټه/وخت',
'filehist-user'             => 'کارونکی',
'filehist-dimensions'       => 'ډډې',
'filehist-filesize'         => 'د دوتنې کچه',
'filehist-comment'          => 'تبصره',
'imagelinks'                => 'تړنونه',
'linkstoimage'              => 'دا {{PLURAL:$1|لاندينی مخ|$1 لانديني مخونه}} د همدې دوتنې سره تړنې لري:',
'nolinkstoimage'            => 'داسې هېڅ کوم مخ نه شته چې د دغې دوتنې سره تړنې ولري.',
'duplicatesoffile'          => 'دا لاندينۍ {{PLURAL:$1| دوتنه د همدې دوتنې غبرګونې لمېسه ده|$1 دوتنې د همدې دوتنې غبرګونې لمېسې دي}}:',
'sharedupload'              => 'دا يوه ګډه دوتنه ده او کېدای شي چې په نورو پروژو کې به هم کارېږي.',
'shareduploadwiki'          => 'لطفاً د لا نورو مالوماتو لپاره $1 وګورۍ.',
'shareduploadwiki-linktext' => 'د دوتنې د څرګندونې مخ',
'noimage'                   => 'په دې نوم هېڅ کومه دوتنه نه شته، تاسو کولای شی چې $1.',
'noimage-linktext'          => 'همدا غونډال ته پورته کول',
'uploadnewversion-linktext' => 'د همدغې دوتنې نوې بڼه پورته کول',

# File reversion
'filerevert-comment' => 'تبصره:',
'filerevert-submit'  => 'په څټ ګرځول',

# File deletion
'filedelete'                  => '$1 ړنګول',
'filedelete-legend'           => 'دوتنه ړنګول',
'filedelete-comment'          => 'تبصره:',
'filedelete-submit'           => 'ړنګول',
'filedelete-success'          => "'''$1''' ړنګ شو.",
'filedelete-otherreason'      => 'بل/اضافه سبب:',
'filedelete-reason-otherlist' => 'بل سبب',
'filedelete-reason-dropdown'  => '*د ړنګولو ټولګړی سبب
** د رښتو نه غاړه غړونه
** کټ مټ دوه ګونې دوتنه',

# MIME search
'mimesearch' => 'MIME پلټنه',
'mimetype'   => 'MIME بڼه:',
'download'   => 'ښکته کول',

# Unwatched pages
'unwatchedpages' => 'ناکتلي مخونه',

# List redirects
'listredirects' => 'د ورګرځېدنو لړليک',

# Unused templates
'unusedtemplates'    => 'نه کارېدلي کينډۍ',
'unusedtemplateswlh' => 'نور تړنونه',

# Random page
'randompage'         => 'ناټاکلی مخ',
'randompage-nopages' => 'په همدغه نوم-تشيال کې هېڅ کوم مخ نشته.',

# Random redirect
'randomredirect' => 'ناټاکلی ورګرځېدنه',

# Statistics
'statistics'             => 'شمار',
'statistics-mostpopular' => 'تر ټولو ډېر کتل شوي مخونه',

'disambiguations' => 'د څرګندونې مخونه',

'doubleredirects' => 'دوه ځلي ورګرځېدنې',

'brokenredirects'        => 'ماتې ورګرځېدنې',
'brokenredirects-delete' => '(ړنګول)',

'withoutinterwiki'        => 'د ژبې د تړنو بې برخې مخونه',
'withoutinterwiki-submit' => 'ښکاره کول',

'fewestrevisions' => 'لږ مخليدل شوي مخونه',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|بايټ|بايټونه}}',
'ncategories'             => '$1 {{PLURAL:$1|وېشنيزه|وېشنيزې}}',
'nlinks'                  => '$1 {{PLURAL:$1|تړنه|تړنې}}',
'nmembers'                => '$1 {{PLURAL:$1|غړی|غړي}}',
'lonelypages'             => 'يتيم مخونه',
'uncategorizedpages'      => 'په وېشنيزو ناوېشلي مخونه',
'uncategorizedcategories' => 'په وېشنيزو ناوېشلې وېشنيزې',
'uncategorizedimages'     => 'په وېشنيزو ناوېشلي انځورنه',
'uncategorizedtemplates'  => 'په وېشنيزو ناوېشلې کينډۍ',
'unusedcategories'        => 'ناکارېدلې وېشنيزې',
'unusedimages'            => 'ناکارېدلې دوتنې',
'popularpages'            => 'نامتو مخونه',
'wantedcategories'        => 'غوښتلې وېشنيزې',
'wantedpages'             => 'غوښتل شوې پاڼې',
'mostlinked'              => 'د ډېرو تړنو مخونه',
'mostlinkedcategories'    => 'د ګڼ شمېر تړنو وېشنيزې',
'mostlinkedtemplates'     => 'د ډېرو تړنو کينډۍ',
'mostcategories'          => 'د ګڼ شمېر وېشنيزو لرونکي مخونه',
'mostimages'              => 'د ډېرو تړنو انځورونه',
'mostrevisions'           => 'ډېر کتل شوي مخونه',
'prefixindex'             => 'د مختاړيو ليکلړ',
'shortpages'              => 'لنډ مخونه',
'longpages'               => 'اوږده مخونه',
'deadendpages'            => 'بې پايه مخونه',
'deadendpagestext'        => 'همدا لانديني مخونه په دغه ويکي کې د نورو مخونو سره تړنې نه لري.',
'protectedpages'          => 'ژغورلي مخونه',
'protectedtitles'         => 'ژغورلي سرليکونه',
'listusers'               => 'د کارونکو لړليک',
'newpages'                => 'نوي مخونه',
'newpages-username'       => 'کارن-نوم:',
'ancientpages'            => 'تر ټولو زاړه مخونه',
'move'                    => 'لېږدول',
'movethispage'            => 'دا مخ ولېږدوه',

# Book sources
'booksources'               => 'د کتاب سرچينې',
'booksources-search-legend' => 'د کتابي سرچينو لټون وکړۍ',
'booksources-go'            => 'ورځه',
'booksources-text'          => 'دا لاندې د هغه وېبځايونو د تړنو لړليک دی چېرته چې نوي او زاړه کتابونه پلورل کېږي، او يا هم کېدای شي چې د هغه کتاب په هکله مالومات ولري کوم چې تاسو ورپسې لټېږۍ:',

# Special:Log
'specialloguserlabel'  => 'کارونکی:',
'speciallogtitlelabel' => 'سرليک:',
'log'                  => 'يادښتونه',
'all-logs-page'        => 'ټول يادښتونه',
'log-search-legend'    => 'د يادښتونو لپاره لټون',
'log-search-submit'    => 'ورځه',

# Special:AllPages
'allpages'          => 'ټول مخونه',
'alphaindexline'    => '$1 نه تر $2 پورې',
'nextpage'          => 'بل مخ ($1)',
'prevpage'          => 'تېر مخ ($1)',
'allpagesfrom'      => 'ښکاره دې شي هغه مخونه چې پېلېږي په:',
'allarticles'       => 'ټول مخونه',
'allinnamespace'    => 'ټول مخونه ($1 نوم-تشيال)',
'allnotinnamespace' => 'ټولې پاڼې (د $1 په نوم-تشيال کې نشته)',
'allpagesprev'      => 'پخواني',
'allpagesnext'      => 'راتلونکي',
'allpagessubmit'    => 'ورځه',
'allpagesprefix'    => 'هغه مخونه ښکاره کړه چې مختاړی يې داسې وي:',
'allpagesbadtitle'  => 'ورکړ شوی سرليک سم نه دی او يا هم د ژبو او يا د بېلابېلو ويکي ګانو مختاړی لري. ستاسو په سرليک کې يو يا څو داسې ابېڅې دي کوم چې په سرليک کې نه شي کارېدلی.',

# Special:Categories
'categories'                  => 'وېشنيزې',
'categoriespagetext'          => 'په دغه ويکي (wiki) کې همدا لاندينۍ وېشنيزې دي.',
'special-categories-sort-abc' => 'د ابېڅو له مخې اوډل',

# Special:ListUsers
'listusersfrom'      => 'هغه کارونکي ښکاره کړه چې نومونه يې پېلېږي په:',
'listusers-submit'   => 'ښکاره کول',
'listusers-noresult' => 'هېڅ کوم کارونکی و نه موندل شو.',

# Special:ListGroupRights
'listgrouprights-group'   => 'ډله',
'listgrouprights-members' => '(د غړو لړليک)',

# E-mail user
'mailnologin'     => 'هېڅ کومه لېږل شوې پته نشته',
'emailuser'       => 'همدې کارونکي ته برېښليک لېږل',
'emailpage'       => 'کارونکي ته برېښليک ولېږه',
'defemailsubject' => 'د {{SITENAME}} برېښليک',
'noemailtitle'    => 'هېڅ کومه برېښليک پته نشته.',
'emailfrom'       => 'پيغام لېږونکی',
'emailto'         => 'پيغام اخيستونکی',
'emailsubject'    => 'موضوع',
'emailmessage'    => 'پيغام',
'emailsend'       => 'لېږل',
'emailccme'       => 'زما د پيغام يوه بېلګه دې ماته هم برېښليک شي.',
'emailccsubject'  => '$1 ته ستاسو د پيغام لمېسه: $2',
'emailsent'       => 'برېښليک مو ولېږل شو',
'emailsenttext'   => 'ستاسو برېښليکي پيغام ولېږل شو.',

# Watchlist
'watchlist'            => 'زما کتلی لړليک',
'mywatchlist'          => 'زما کتلی لړليک',
'watchlistfor'         => "(د '''$1''')",
'nowatchlist'          => 'ستاسو په کتلي لړليک کې هېڅ نه شته.',
'watchnologin'         => 'غونډال کې نه ياست ننوتي.',
'watchnologintext'     => 'ددې لپاره چې خپل کتل شوي لړليک کې بدلون راولی نو تاسو ته پکار ده چې لومړی غونډال کې [[Special:UserLogin|ننوتنه]] ترسره کړی.',
'addedwatch'           => 'په کتلي لړليک کې ورګډ شو.',
'addedwatchtext'       => "د \"[[:\$1]]\" په نوم يو مخ ستاسو [[Special:Watchlist|کتلي لړليک]] کې ورګډ شو.
په راتلونکې کې چې په دغه مخ او ددغه مخ په اړونده بحث کې کوم بدلونونه راځي نو هغه به ستاسو کتلي لړليک کې وښوولی شي,
او په همدې توګه هغه مخونه به د [[Special:RecentChanges|وروستي بدلونونو]] په لړليک کې په '''روڼ''' ليک ليکل شوی وي ترڅو په اسانۍ سره څوک وپوهېږي چې په کوم کوم مخونو کې بدلونونه ترسره شوي.

که چېرته تاسو بيا وروسته غواړۍ چې کومه پاڼه د خپل کتلي لړليک نه ليرې کړۍ، نو په \"نه کتل\" تڼۍ باندې ټک ورکړۍ.",
'removedwatch'         => 'د کتلي لړليک نه لرې شو',
'removedwatchtext'     => 'د "[[:$1]]" په نامه مخ ستاسو له کتلي لړليک نه لرې شو.',
'watch'                => 'کتل',
'watchthispage'        => 'همدا مخ کتل',
'unwatch'              => 'نه کتل',
'watchlist-details'    => '{{PLURAL:$1|$1 مخ|$1 مخونه}} کتل شوي په دې کې د خبرواترو مخونه نه دي شمېر شوي.',
'wlheader-enotif'      => 'د برېښليک له لارې خبرول چارن شوی.*',
'wlheader-showupdated' => "* هغه مخونه چې وروستی ځل ستاسو د کتلو نه وروسته بدلون موندلی په '''روڼ''' ليک نښه شوي.",
'wlshowlast'           => 'وروستي $1 ساعتونه $2 ورځې $3 ښکاره کړه',
'watchlist-hide-bots'  => 'د باټ سمادېدنې پټول',
'watchlist-hide-own'   => 'زما سمادونه پټول',
'watchlist-hide-minor' => 'وړې سمادېدنې پټول',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'د کتلو په حال کې...',
'unwatching' => 'د نه کتلو په حال کې...',

'enotif_newpagetext'           => 'دا يوه نوې پاڼه ده.',
'enotif_impersonal_salutation' => '{{SITENAME}} کارونکی',
'changed'                      => 'بدل شو',
'created'                      => 'جوړ شو',
'enotif_lastvisited'           => 'د ټولو هغو بدلونونو د کتلو لپاره چې ستاسو د وروستي ځل راتګ نه وروسته پېښې شوي، $1 وګورۍ.',
'enotif_lastdiff'              => 'د همدغه بدلون د کتلو لپاره $1 وګورۍ.',
'enotif_anon_editor'           => 'ورکنومی کارونکی $1',

# Delete/protect/revert
'deletepage'                  => 'پاڼه ړنګول',
'confirm'                     => 'تاييد',
'exblank'                     => 'دا مخ تش وه',
'delete-confirm'              => '"$1" ړنګوول',
'delete-legend'               => 'ړنګول',
'historywarning'              => 'پاملرنه: کومه پاڼه چې تاسو يې د ړنګولو هڅه کوی يو پېښليک لري:',
'confirmdeletetext'           => 'تاسو د تل لپار يو مخ يا انځور د هغه ټول پېښليک سره سره د دغه ډېټابېز نه ړنګوۍ. که چېرته تاسو ددغې کړنې په پايلې پوه ياست او د دغې پاڼې د [[پروژې:تګلارې]] سره سمون خوري نو لطفاً ددغې کړنې تاييد وکړی .',
'actioncomplete'              => 'بشپړه کړنه',
'deletedtext'                 => '"<nowiki>$1</nowiki>" ړنګ شوی.
د نوو ړنګ شوو سوانحو لپاره $2 وګورۍ.',
'deletedarticle'              => 'ړنګ شو "[[$1]]"',
'dellogpage'                  => 'د ړنګولو يادښت',
'dellogpagetext'              => 'دا لاندې د نوو ړنګ شوو کړنو لړليک دی.',
'deletionlog'                 => 'د ړنګولو يادښت',
'deletecomment'               => 'د ړنګولو سبب',
'deleteotherreason'           => 'بل/اضافه سبب:',
'deletereasonotherlist'       => 'بل سبب',
'rollback_short'              => 'په شابېول',
'rollbacklink'                => 'په شابېول',
'protectlogpage'              => 'د ژغورنې يادښت',
'protectedarticle'            => '"[[$1]]" وژغورلی شو',
'protect-legend'              => 'د ژغورلو پخلی کول',
'protectcomment'              => 'تبصره:',
'protectexpiry'               => 'د پای نېټه:',
'protect_expiry_invalid'      => 'د پای وخت ناسم دی.',
'protect_expiry_old'          => 'د پای وخت په تېرمهال کې دی.',
'protect-unchain'             => 'د لېږدون اجازې ناتړل',
'protect-text'                => 'تاسو کولای شی چې د <strong><nowiki>$1</nowiki></strong> مخ لپاره د ژغورلو کچه همدلته وګورۍ او بدلون پکې راولی.',
'protect-locked-access'       => 'ستاسو کارن-حساب دا اجازه نه لري چې د پاڼو د ژغورنې په کچه کې بدلون راولي.
دلته د <strong>$1</strong> مخ لپاره اوسني شته امستنې دي:',
'protect-cascadeon'           => 'د اوسمهال لپاره همدا مخ ژغورل شوی دا ځکه چې همدا مخ په {{PLURAL:$1|لانديني مخ|لانديني مخونو}} کې ورګډ دی چې {{PLURAL:$1|ځوړاوبيزه ژغورنه يې چارنه ده|ځوړاوبيزې ژغورنې يې چارنې دي}}.
تاسو د همدې مخ د ژغورنې په کچه کې بدلون راوستلای شی، خو دا به په ځوړاوبيزه ژغورنه اغېزمنه نه کړي.',
'protect-default'             => '(اصلي بڼه)',
'protect-fallback'            => 'د "$1" اجازه پکار ده',
'protect-level-autoconfirmed' => 'د ناثبته کارونکو مخنيوی کول',
'protect-level-sysop'         => 'يواځې پازوالان',
'protect-summary-cascade'     => 'ځوړاوبيز',
'protect-expiring'            => 'په $1 (UTC) پای ته رسېږي',
'protect-cascade'             => 'په همدې مخ کې د ټولو ګډو مخونو نه ژغورنه کېږي (ځوړاوبيزه ژغورنه)',
'protect-cantedit'            => 'تاسو نه شی کولای چې د همدغه مخ د ژغورنې په کچه کې بدلون راولی، دا ځکه چې تاسو د همدغه مخ د سمادولو اجازه نه لری.',
'restriction-type'            => 'اجازه:',
'restriction-level'           => 'د بنديز کچه:',
'minimum-size'                => 'وړه کچه',

# Restrictions (nouns)
'restriction-edit'   => 'سمادول',
'restriction-move'   => 'لېږدول',
'restriction-create' => 'جوړول',

# Undelete
'undelete'               => 'ړنګ شوي مخونه کتل',
'undeletepage'           => 'ړنګ شوي مخونه کتل او بېرته پرځای کول',
'viewdeletedpage'        => 'ړنګ شوي مخونه کتل',
'undeletebtn'            => 'بېرته پرځای کول',
'undeletelink'           => 'بېرته پرځای کول',
'undeletereset'          => 'بياايښودل',
'undeletecomment'        => 'تبصره:',
'undeletedarticle'       => '"[[$1]]" بېرته پرځای شو',
'undelete-search-box'    => 'ړنګ شوي مخونه لټول',
'undelete-search-prefix' => 'هغه مخونه ښکاره کړه چې پېلېږي په:',
'undelete-search-submit' => 'پلټل',

# Namespace form on various pages
'namespace'      => 'نوم-تشيال:',
'invert'         => 'خوښونې سرچپه کول',
'blanknamespace' => '(اصلي)',

# Contributions
'contributions' => 'د کارونکي ونډې',
'mycontris'     => 'زما ونډې',
'contribsub2'   => 'د $1 لپاره ($2)',
'uctop'         => '(سرپاڼه)',
'month'         => 'له ټاکلې مياشتې نه راپدېخوا (او تر دې پخواني):',
'year'          => 'له ټاکلي کال نه راپدېخوا (او تر دې پخواني):',

'sp-contributions-newbies'     => 'د نوو کارن-حسابونو ونډې ښکاره کول',
'sp-contributions-newbies-sub' => 'د نوو کارن-حسابونو لپاره',
'sp-contributions-blocklog'    => 'د مخنيوي يادښت',
'sp-contributions-search'      => 'د ونډو لټون',
'sp-contributions-username'    => 'IP پته يا کارن-نوم:',
'sp-contributions-submit'      => 'پلټل',

# What links here
'whatlinkshere'       => 'د همدې پاڼې تړنونه',
'whatlinkshere-title' => 'هغه مخونه چې د "$1" سره تړنې لري',
'whatlinkshere-page'  => 'مخ:',
'linklistsub'         => '(د تړنونو لړليک)',
'linkshere'           => "دغه لانديني مخونه د '''[[:$1]]''' سره تړنې لري:",
'nolinkshere'         => "د '''[[:$1]]''' سره هېڅ يو مخ هم تړنې نه لري .",
'isredirect'          => 'ورګرځېدلی مخ',
'istemplate'          => 'ورګډېدنه',
'whatlinkshere-prev'  => '{{PLURAL:$1|پخوانی|پخواني $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|راتلونکی|راتلونکي $1}}',
'whatlinkshere-links' => '← تړنې',

# Block/unblock
'blockip'                  => 'د کاروونکي مخه نيول',
'ipaddress'                => 'IP پته',
'ipadressorusername'       => 'IP پته يا کارن نوم',
'ipbreason'                => 'سبب',
'ipbreasonotherlist'       => 'بل لامل',
'ipbother'                 => 'بل وخت:',
'ipboptions'               => '2 ساعتونه:2 hours,1 ورځ:1 day,3 ورځې:3 days,1 اوونۍ:1 week,2 اوونۍ:2 weeks,1 مياشت:1 month,3 مياشتې:3 months,6 مياشتې:6 months,1 کال:1 year,لامحدوده:infinite', # display1:time1,display2:time2,...
'ipbotherreason'           => 'بل/اضافه سبب:',
'badipaddress'             => 'ناسمه IP پته',
'blockipsuccesssub'        => 'مخنيوی په برياليتوب سره ترسره شو',
'blockipsuccesstext'       => 'د [[Special:Contributions/$1|$1]] مخه نيول شوې.
<br />د مخنيول شويو خلکو د کتنې لپاره، د [[Special:IPBlockList|مخنيول شويو IP لړليک]] وګورۍ.',
'ipblocklist'              => 'د مخنيول شويو آی پي پتو او کارن نومونو لړليک',
'ipblocklist-username'     => 'کارن-نوم يا IP پته:',
'ipblocklist-submit'       => 'پلټل',
'infiniteblock'            => 'لامحدوده',
'anononlyblock'            => 'يواځې ورکنومی',
'blocklink'                => 'مخه نيول',
'unblocklink'              => 'نامخنيول',
'contribslink'             => 'ونډې',
'autoblocker'              => 'په اتوماتيک ډول ستاسو مخنيوی شوی دا ځکه چې ستاسو IP پته وروستی ځل د "[[User:$1|$1]]" له خوا کارېدلې. او د $1 د مخنيوي سبب دا دی: "$2"',
'blocklogpage'             => 'د مخنيوي يادښت',
'blocklogentry'            => 'د [[$1]] مخنيوی شوی چې د بنديز د پای وخت يې $2 $3 دی',
'block-log-flags-anononly' => 'يواځې ورکنومي کارونکي',
'block-log-flags-nocreate' => 'د کارن-حساب جوړول ناچارن شوې',
'block-log-flags-noemail'  => 'ددې برېښليک مخه نيول شوی',
'proxyblocksuccess'        => 'ترسره شو.',

# Move page
'move-page-legend'        => 'مخ لېږدول',
'movepagetext'            => "د لاندينۍ فورمې په کارولو سره تاسو د يوه مخ نوم بدلولی شی، چې په همدې توګه به د يوه مخ ټول پېښليک د هغه د نوي نوم سرليک ته ولېږدېږي.
د يوه مخ، پخوانی نوم به د نوي نوم ورګرځونکی مخ وګرځي او نوي سرليک ته به وګرځولی شي.
هغه تړنې چې په زاړه مخ کې دي په هغو کې به هېڅ کوم بدلون را نه شي;
[[Special:BrokenRedirects|د ماتو ورګرځونو]] يا [[Special:DoubleRedirects|دوه ځله ورګرځونو]] د ستونزو د پېښېدو په خاطر ځان ډاډه کړی چې ستاسې ورګرځونې ماتې يا دوه ځله نه وي.
دا ستاسو پازه ده چې ځان په دې هم ډاډمن کړی چې آيا هغه تړنې کوم چې د يو مخ سره پکار دي چې وي، همداسې په پرله پسې توګه پېيلي او خپل موخن ځايونو سره اړونده دي.

په ياد مو اوسه چې يو مخ به '''هېڅکله''' و نه لېږدېږي که چېرته د پخوا نه په هماغه نوم يو مخ شتون ولري، خو که چېرته يو مخ تش وه او يا هم يو ورګرځېدلی مخ وه چې پېښليک کې يې بدلون نه وي راغلی. نو دا په دې مانا ده چې تاسو کولای شی چې د يو مخ نوم بدل کړی بېرته هماغه پخواني نوم ته چې د پخوا نه يې درلوده، که چېرته تاسو تېرووځۍ نو په داسې حال کې تاسو نه شی کولای چې د يوه مخ پر سر يو څه وليکۍ.

'''ګواښنه!'''
يوه نوي نوم ته د مخونو د نوم بدلون کېدای شي چې په نامتو مخونو کې بنسټيزه او نه اټکل کېدونکی بدلونونه رامېنځ ته کړي;
مخکې له دې نه چې پرمخ ولاړ شی، مهرباني وکړۍ لومړی خپل ځان په دې ډاډه کړی چې تاسو ددغې کړنې په پايلو ښه پوهېږۍ.",
'movepagetalktext'        => "همدې مخ ته اړونده د خبرواترو مخ هم په اتوماتيک ډول لېږدول کېږي '''خو که چېرته:'''
*په نوي نوم د پخوا نه د خبرواترو يو مخ شتون ولري، او يا هم
*تاسو ته لاندې ورکړ شوی څلورڅنډی په نښه شوی وي.

نو په هغه وخت کې پکار ده چې د خبرواترو د مخ لېږدونه او د نوي مخ سره د يوځای کولو کړنه په لاسي توګه ترسره کړی.",
'movearticle'             => 'مخ لېږدول',
'newtitle'                => 'يو نوي سرليک ته:',
'move-watch'              => 'همدا مخ کتل',
'movepagebtn'             => 'مخ لېږدول',
'pagemovedsub'            => 'لېږدول په برياليتوب سره ترسره شوه',
'movepage-moved'          => '<big>\'\'\'د "$1" په نامه دوتنه، "$2" ته ولېږدېده\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'په همدې نوم يوه بله پاڼه د پخوا نه شته او يا خو دا نوم چې تاسو ټاکلی سم نه دی. لطفاً يو بل نوم وټاکۍ.',
'talkexists'              => "'''همدا مخ په برياليتوب سره نوي سرليک ته ولېږدېده، خو د خبرواترو مخ يې و نه لېږدول شو دا ځکه چې نوی سرليک له پخوا نه ځانته د خبرواترو يو مخ لري.
مهرباني وکړۍ د خبرواترو دا دواړه مخونه په لاسي توګه سره يو ځای کړی.'''",
'movedto'                 => 'ته ولېږدول شو',
'movetalk'                => 'د خبرو اترو اړونده مخ ورسره لېږدول',
'1movedto2'               => '[[$1]]، [[$2]] ته ولېږدېده',
'movelogpage'             => 'د لېږدولو يادښت',
'movelogpagetext'         => 'دا لاندې د لېږدول شوو مخونو لړليک دی.',
'movereason'              => 'سبب',
'revertmove'              => 'په څټ ګرځول',
'delete_and_move'         => 'ړنګول او لېږدول',
'delete_and_move_confirm' => 'هو, دا مخ ړنګ کړه',

# Export
'export'            => 'مخونه صادرول',
'export-addcattext' => 'مخونو د ورګډولو وېشنيزه:',
'export-addcat'     => 'ورګډول',

# Namespace 8 related
'allmessages'               => 'د غونډال پيغامونه',
'allmessagesname'           => 'نوم',
'allmessagesdefault'        => 'ټاکل شوی متن',
'allmessagescurrent'        => 'اوسنی متن',
'allmessagestext'           => 'دا د مېډيا ويکي په نوم-تشيال کې د غونډال د شته پيغامونو لړليک دی.',
'allmessagesnotsupportedDB' => "'''Special:Allmessages''' ترېنه کار نه اخيستل کېږي ځکه چې '''\$wgUseDatabaseMessages''' مړ دی.",
'allmessagesfilter'         => 'د پيغامونو د نوم فلتر:',
'allmessagesmodified'       => 'يواځې بدلون خوړلي توکي ښکاره کول',

# Thumbnails
'thumbnail-more'  => 'لويول',
'filemissing'     => 'دوتنه ورکه ده',
'thumbnail_error' => 'د  بټنوک د جوړېدنې ستونزه: $1',

# Import log
'importlogpage' => 'د واردولو يادښت',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'زما کارن مخ',
'tooltip-pt-mytalk'               => 'زما د خبرواترو مخ',
'tooltip-pt-preferences'          => 'زما غوره توبونه',
'tooltip-pt-watchlist'            => 'د هغه مخونو لړليک چې تاسو يې د بدلون لپاره څاری',
'tooltip-pt-mycontris'            => 'زما د ونډو لړليک',
'tooltip-pt-login'                => 'تاسو ته په غونډال کې د ننوتلو سپارښتنه کوو، که څه هم چې دا يو اړين کار نه دی.',
'tooltip-pt-anonlogin'            => 'تاسو ته په غونډال کې د ننوتنې سپارښتنه کوو، که څه هم چې دا يو اړين کار نه دی.',
'tooltip-pt-logout'               => 'وتل',
'tooltip-ca-talk'                 => 'د مخ د مېنځپانګې په اړه خبرې اترې',
'tooltip-ca-edit'                 => 'تاسو همدا مخ سمادولای شی. مهرباني وکړی د ليکنې د خوندي کولو دمخه مو د همدې ليکنې مخکتنه وګورۍ.',
'tooltip-ca-addsection'           => 'د خبرواترو همدغه مخ کې يوه تبصره ورګډول.',
'tooltip-ca-viewsource'           => 'همدا مخ ژغورل شوی. تاسو کولای شی چې د همدې مخ سرجينه وګورۍ.',
'tooltip-ca-protect'              => 'همدا مخ ژغورل',
'tooltip-ca-delete'               => 'همدا مخ ړنګول',
'tooltip-ca-move'                 => 'همدا مخ لېږدول',
'tooltip-ca-watch'                => 'همدا مخ پخپل کتلي لړليک کې ګډول',
'tooltip-ca-unwatch'              => 'همدا مخ خپل د کتلي لړليک نه لرې کول',
'tooltip-search'                  => 'د {{SITENAME}} لټون',
'tooltip-p-logo'                  => 'لومړی مخ',
'tooltip-n-mainpage'              => 'لومړي مخ ته ورتلل',
'tooltip-n-portal'                => 'د پروژې په اړه، تاسو څه کولای شی، چېرته کولای شی چې شيان ومومۍ',
'tooltip-n-currentevents'         => 'د اوسنيو پېښو اړونده د هغوی د شاليد مالومات موندل',
'tooltip-n-recentchanges'         => 'په ويکي کې د وروستي بدلونو لړليک.',
'tooltip-n-randompage'            => 'يو ناټاکلی مخ ښکاره کوي',
'tooltip-n-help'                  => 'هغه ځای چېرته چې راڅرګندولای شو.',
'tooltip-t-whatlinkshere'         => 'د ويکي د ټولو هغو مخونو لړليک چې دلته تړنې لري',
'tooltip-t-contributions'         => 'د همدې کارونکي د ونډو لړليک کتل',
'tooltip-t-emailuser'             => 'همدې کارونکي ته يو برېښليک لېږل',
'tooltip-t-upload'                => 'دوتنې پورته کول',
'tooltip-t-specialpages'          => 'د ټولو ځانګړو پاڼو لړليک',
'tooltip-t-print'                 => 'د همدې مخ چاپي بڼه',
'tooltip-ca-nstab-user'           => 'د کارونکي مخ کتل',
'tooltip-ca-nstab-special'        => 'همدا يو ځانګړی مخ دی، تاسو نه شی کولای چې دا مخ سماد کړی.',
'tooltip-ca-nstab-project'        => 'د پروژې مخ کتل',
'tooltip-ca-nstab-image'          => 'د دوتنې مخ کتل',
'tooltip-ca-nstab-mediawiki'      => 'د غونډال پيغامونه ښکاره کول',
'tooltip-ca-nstab-template'       => 'کينډۍ ښکاره کول',
'tooltip-ca-nstab-help'           => 'د لارښود مخ کتل',
'tooltip-ca-nstab-category'       => 'د وېشنيزې مخ ښکاره کول',
'tooltip-minoredit'               => 'دا لکه يوه وړه سمادېدنه په نښه کوي[alt-i]',
'tooltip-save'                    => 'ستاسو بدلونونه خوندي کوي',
'tooltip-preview'                 => 'ستاسو بدلونونه ښکاره کوي, لطفاً دا کړنه د خوندي کولو دمخه وکاروۍ! [alt-p]',
'tooltip-diff'                    => 'دا هغه بدلونونه چې تاسو په متن کې ترسره کړي، ښکاره کوي. [alt-v]',
'tooltip-compareselectedversions' => 'د همدې مخ د دوو ټاکل شويو بڼو تر مېنځ توپيرونه وګورۍ.',
'tooltip-watch'                   => 'همدا مخ ستاسو کتلي لړليک کې ورګډوي [alt-w]',

# Attribution
'lastmodifiedatby' => 'دا مخ وروستی ځل د $3 لخوا په $2، $1 بدلون موندلی.', # $1 date, $2 time, $3 user

# Info page
'infosubtitle' => 'د مخ مالومات',

# Patrol log
'patrol-log-auto' => '(خپلسر)',

# Image deletion
'filedeleteerror-short' => 'د دوتنې د ړنګولو ستونزه: $1',

# Browsing diffs
'previousdiff' => 'تېر توپير →',
'nextdiff'     => '← بل توپير',

# Media information
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|مخ|مخونه}}',
'file-info-size'       => '($1 × $2 پېکسل, د دوتنې کچه: $3, MIME بڼه: $4)',
'file-nohires'         => '<small>تر دې کچې لوړې بېلن نښې نشته.</small>',
'svg-long-desc'        => '(SVG دوتنه، نومېنلي $1 × $2 پېکسل، د دوتنې کچه: $3)',
'show-big-image'       => 'بشپړه بېلن نښې',
'show-big-image-thumb' => '<small>د همدې مخکتنې کچه: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'             => 'د نوو دوتنو نندارتون',
'imagelisttext'         => "دلته لاندې د '''$1''' {{PLURAL:$1|دوتنه|دوتنې}} يو لړليک دی چې اوډل شوي $2.",
'newimages-summary'     => 'همدا ځانګړی مخ، وروستنۍ پورته شوې دوتنې ښکاره کوي.',
'noimages'              => 'د کتلو لپاره څه نشته.',
'ilsubmit'              => 'پلټل',
'bydate'                => 'د نېټې له مخې',
'sp-newimages-showfrom' => 'هغه نوې دوتنې چې په $1 په $2 بجو پيلېږي ښکاره کول',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'hours-abbrev' => 'ساعتونه',

# Bad image list
'bad_image_list' => 'بڼه يې په لاندې توګه ده:

يواځې د هغو توکيو لړليک راوړل (هغه کرښې چې پېلېږي پر *) کوم چې ګڼل کېږي.
بايد چې په يوه کرښه کې لومړنۍ تړنه د يوې خرابې دوتنې سره وي.
په يوې کرښې باندې هر ډول وروستۍ تړنې به د استثنا په توګه وګڼلای شي، د ساري په توګه هغه مخونو کې چې يوه دوتنه پرليکه پرته وي.',

# Metadata
'metadata'          => 'مېټاډاټا',
'metadata-help'     => 'همدا دوتنه نور اضافه مالومات هم لري، چې کېدای شي ستاسو د ګڼياليزې کامرې او يا هم د ځيرڅار په کارولو سره د ګڼيالېدنې په وخت کې ورسره مل شوي.
که همدا دوتنه د خپل آرني دريځ څخه بدله شوې وي نو ځينې تفصيلونه به په بدل شوي دوتنه کې په بشپړه توګه نه وي.',
'metadata-expand'   => 'غځېدلی تفصيل ښکاره کړی',
'metadata-collapse' => 'غځېدلی تفصيل پټ کړی',
'metadata-fields'   => 'د EXIF ميټاډاټا ډګرونه چې لړليک يې په همدې پيغام کې په لاندې توګه راغلی د انځوريز مخ په ښکارېدنه کې به هغه وخت ورګډ شي کله چې د مېټاډاټا چوکاټ پرانيستل کېږي.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagedescription' => 'د انځور سرليک',
'exif-model'            => 'د کامرې ماډل',
'exif-artist'           => 'ليکوال',
'exif-usercomment'      => 'د کارونکي تبصرې',
'exif-filesource'       => 'د دوتنې سرچينه',

'exif-unknowndate' => 'نامالومه نېټه',

'exif-orientation-1' => 'نورمال', # 0th row: top; 0th column: left

'exif-componentsconfiguration-0' => 'نشته دی',

'exif-subjectdistance-value' => '$1 متره',

'exif-meteringmode-0'   => 'ناجوت',
'exif-meteringmode-255' => 'نور',

'exif-lightsource-0' => 'ناجوت',

'exif-focalplaneresolutionunit-2' => 'انچه',

'exif-gaincontrol-0' => 'هېڅ',

'exif-contrast-0' => 'نورمال',

'exif-saturation-0' => 'نورمال',

'exif-sharpness-0' => 'نورمال',

'exif-subjectdistancerange-0' => 'ناجوت',

# External editor support
'edit-externally'      => 'د باندنيو پروګرامونو په کارولو سره دا دوتنه سمادول',
'edit-externally-help' => 'د نورو مالوماتو لپاره د [http://www.mediawiki.org/wiki/Manual:External_editors امستنو لارښوونې] وګورۍ.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'ټول',
'imagelistall'     => 'ټول',
'watchlistall2'    => 'ټول',
'namespacesall'    => 'ټول',
'monthsall'        => 'ټول',

# E-mail address confirmation
'confirmemail'           => 'د برېښليک پتې پخلی وکړی',
'confirmemail_noemail'   => 'تاسو يوه سمه برېښناليک پته نه ده ثبته کړې مهرباني وکړی [[Special:Preferences|د کارونکي غوره توبونه]] کې مو بدلون راولی.',
'confirmemail_oncreate'  => 'ستاسو د برېښناليک پتې ته يو تاييدي کوډ درولېږل شو.
ددې لپاره چې تاسو غونډال ته ورننوځی تاسو ته د همدغه کوډ اړتيا نشته، خو تاسو ته د همدغه کوډ اړتيا په هغه وخت کې پکارېږي کله چې په ويکي کې خپلې برېښناليکي کړنې چارن کول غواړی.',
'confirmemail_needlogin' => 'تاسو ته پکار ده چې $1 ددې لپاره چې ستاسو د برېښليک پتې پخلی وشي.',
'confirmemail_loggedin'  => 'اوس ستاسو د برېښناليک د پتې پخلی وشو.',
'confirmemail_error'     => 'ستاسو د برېښليک پتې د تاييد په خوندي کولو کې يوه ستونزه رامېنڅ ته شوه.',

# Scary transclusion
'scarytranscludetoolong' => '[اوبخښۍ؛ URL مو ډېر اوږد دی]',

# Trackbacks
'trackbackremove' => '([$1 ړنګول])',

# action=purge
'confirm_purge'        => 'په رښتيا د همدې مخ حافظه سپينول غواړۍ؟

$1',
'confirm_purge_button' => 'ښه/هو',

# AJAX search
'searchcontaining' => "د هغو ليکنو لټون چې ''$1'' په کې شته.",
'searchnamed'      => "د هغې ليکنې لټون چې نوم يې ''$1'' دی.",
'articletitles'    => "هغه ليکنې چې په ''$1'' پيلېږي",
'hideresults'      => 'پايلې پټول',

# Multipage image navigation
'imgmultipageprev' => '← پخوانی مخ',
'imgmultipagenext' => 'راتلونکی مخ →',
'imgmultigo'       => 'ورځه!',
'imgmultigoto'     => 'د $1 مخ ته ورځه',

# Table pager
'ascending_abbrev'         => 'ختند',
'descending_abbrev'        => 'مخښکته',
'table_pager_next'         => 'بل مخ',
'table_pager_prev'         => 'تېر مخ',
'table_pager_first'        => 'لومړی مخ',
'table_pager_last'         => 'وروستی مخ',
'table_pager_limit'        => 'په يوه مخ $1 توکي ښکاره کړی',
'table_pager_limit_submit' => 'ورځه',
'table_pager_empty'        => 'هېڅ پايلې نه شته',

# Auto-summaries
'autosumm-blank'   => 'د مخ ټوله مېنځپانګه ليرې کول',
'autosumm-replace' => "دا مخ د '$1' پرځای راوستل",
'autoredircomment' => '[[$1]] ته وګرځولی شو',
'autosumm-new'     => 'نوی مخ: $1',

# Live preview
'livepreview-loading' => 'د برسېرېدلو په حال کې...',

# Watchlist editor
'watchlistedit-noitems'    => 'ستاسو په کتلي لړليک کې هېڅ کوم سرليک نشته.',
'watchlistedit-raw-title'  => 'خام کتلی لړليک سمادول',
'watchlistedit-raw-legend' => 'خام کتلی لړليک سمادول',
'watchlistedit-raw-titles' => 'سرليکونه:',
'watchlistedit-raw-submit' => 'کتلی لړليک تازه کول',
'watchlistedit-raw-done'   => 'ستاسو کتلی لړليک تازه شو.',

# Watchlist editing tools
'watchlisttools-view' => 'اړونده بدلونونه کتل',
'watchlisttools-edit' => 'کتلی لړليک ليدل او سمادول',
'watchlisttools-raw'  => 'خام کتلی لړليک سمادول',

# Iranian month names
'iranian-calendar-m1'  => 'وری',
'iranian-calendar-m2'  => 'غويی',
'iranian-calendar-m3'  => 'غبرګولی',
'iranian-calendar-m4'  => 'چنګاښ',
'iranian-calendar-m5'  => 'زمری',
'iranian-calendar-m6'  => 'وږی',
'iranian-calendar-m7'  => 'تله',
'iranian-calendar-m8'  => 'لړم',
'iranian-calendar-m9'  => 'ليندۍ',
'iranian-calendar-m10' => 'مرغومی',
'iranian-calendar-m11' => 'سلواغه',
'iranian-calendar-m12' => 'کب',

# Special:Version
'version'              => 'بڼه', # Not used as normal message but as header for the special page itself
'version-specialpages' => 'ځانګړي مخونه',
'version-other'        => 'بل',

# Special:FilePath
'filepath-page' => 'دوتنه:',

# Special:FileDuplicateSearch
'fileduplicatesearch-filename' => 'د دوتنې نوم:',
'fileduplicatesearch-submit'   => 'پلټل',

# Special:SpecialPages
'specialpages'                 => 'ځانګړي مخونه',
'specialpages-group-other'     => 'نور ځانګړي مخونه',
'specialpages-group-login'     => 'ننوتل / کارن-حساب جوړول',
'specialpages-group-changes'   => 'وروستي بدلونونه او يادښتونه',
'specialpages-group-users'     => 'کارونکي او رښتې',
'specialpages-group-highuse'   => 'ډېر کارېدونکي مخونه',
'specialpages-group-pages'     => 'د مخونو لړليک',
'specialpages-group-pagetools' => 'د مخ اوزارونه',
'specialpages-group-wiki'      => 'ويکيډاټا او اوزارونه',

# Special:BlankPage
'blankpage'              => 'تش مخ',
'intentionallyblankpage' => 'همدا مخ په لوی لاس تش پرېښودل شوی دی',

);
