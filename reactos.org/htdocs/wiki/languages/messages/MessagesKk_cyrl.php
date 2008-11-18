<?php
/** Kazakh (Cyrillic) (Қазақша (Cyrillic))
 *
 * @ingroup Language
 * @file
 *
 * @author AlefZet
 * @author GaiJin
 */

/**
 * Бұл қазақша тілдесуінің жерсіндіру файлы
 *
 * ШЕТКІ ПАЙДАЛАНУШЫЛАР: БҰЛ ФАЙЛДЫ ТІКЕЛЕЙ ӨҢДЕМЕҢІЗ!
 *
 * Бұл файлдағы өзгерістер бағдарламалық жасақтама кезекті жаңартылғанда жоғалтылады.
 * Уикиде өз бапталымдарыңызды істей аласыз.
 * Әкімші боп кіргеніңізде, [[Арнайы:Барлық хабарлар]] деген бетке өтіңіз де
 * мында тізімделінген МедиаУики:* сипаты бар беттерді өңдеңіз.
 */
$separatorTransformTable = array(
	',' => "\xc2\xa0",
	'.' => ',',
);

$extraUserToggles = array(
	'nolangconversion'
);

$fallback8bitEncoding = 'windows-1251';

$namespaceNames = array(
	NS_MEDIA            => 'Таспа',
	NS_SPECIAL          => 'Арнайы',
	NS_MAIN             => '',
	NS_TALK             => 'Талқылау',
	NS_USER             => 'Қатысушы',
	NS_USER_TALK        => 'Қатысушы_талқылауы',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_талқылауы',
	NS_IMAGE            => 'Сурет',
	NS_IMAGE_TALK       => 'Сурет_талқылауы',
	NS_MEDIAWIKI        => 'МедиаУики',
	NS_MEDIAWIKI_TALK   => 'МедиаУики_талқылауы',
	NS_TEMPLATE         => 'Үлгі',
	NS_TEMPLATE_TALK    => 'Үлгі_талқылауы',
	NS_HELP             => 'Анықтама',
	NS_HELP_TALK        => 'Анықтама_талқылауы',
	NS_CATEGORY         => 'Санат',
	NS_CATEGORY_TALK    => 'Санат_талқылауы'
);

$namespaceAliases = array(
	# Aliases to kk-latn namespaces
	'Taspa'               => NS_MEDIA,
	'Arnaýı'              => NS_SPECIAL,
	'Talqılaw'            => NS_TALK,
	'Qatıswşı'            => NS_USER,
	'Qatıswşı_talqılawı'  => NS_USER_TALK,
	'$1_talqılawı'        => NS_PROJECT_TALK,
	'Swret'               => NS_IMAGE,
	'Swret_talqılawı'     => NS_IMAGE_TALK,
	'MedïaWïkï'           => NS_MEDIAWIKI,
	'MedïaWïkï_talqılawı' => NS_MEDIAWIKI_TALK,
	'Ülgi'                => NS_TEMPLATE,
	'Ülgi_talqılawı'      => NS_TEMPLATE_TALK,
	'Anıqtama'            => NS_HELP,
	'Anıqtama_talqılawı'  => NS_HELP_TALK,
	'Sanat'               => NS_CATEGORY,
	'Sanat_talqılawı'     => NS_CATEGORY_TALK,

	# Aliases to renamed kk-arab namespaces
	'مەدياۋيكي'        => NS_MEDIAWIKI,
	'مەدياۋيكي_تالقىلاۋى'  => NS_MEDIAWIKI_TALK ,
	'ٷلگٸ'        => NS_TEMPLATE ,
	'ٷلگٸ_تالقىلاۋى'    => NS_TEMPLATE_TALK,
	'ٴۇلگٴى'              => NS_TEMPLATE,
	'ٴۇلگٴى_تالقىلاۋى'    => NS_TEMPLATE_TALK,

	# Aliases to kk-arab namespaces
	'تاسپا'              => NS_MEDIA,
	'ارنايى'              => NS_SPECIAL,
	'تالقىلاۋ'            => NS_TALK,
	'قاتىسۋشى'          => NS_USER,
	'قاتىسۋشى_تالقىلاۋى'=> NS_USER_TALK,
	'$1_تالقىلاۋى'        => NS_PROJECT_TALK,
	'سۋرەت'              => NS_IMAGE,
	'سۋرەت_تالقىلاۋى'    => NS_IMAGE_TALK,
	'مەدياۋيكي'           => NS_MEDIAWIKI,
	'مەدياۋيكي_تالقىلاۋى' => NS_MEDIAWIKI_TALK,
	'ٷلگٸ'              => NS_TEMPLATE,
	'ٷلگٸ_تالقىلاۋى'    => NS_TEMPLATE_TALK,
	'انىقتاما'            => NS_HELP,
	'انىقتاما_تالقىلاۋى'  => NS_HELP_TALK,
	'سانات'              => NS_CATEGORY,
	'سانات_تالقىلاۋى'    => NS_CATEGORY_TALK,
);

$skinNames = array(
	'standard'    => 'Дағдылы (standard)',
	'nostalgia'   => 'Аңсау (nostalgia)',
	'cologneblue' => 'Көлн зеңгірлігі (cologneblue)',
	'monobook'    => 'Дара кітап (monobook)',
	'myskin'      => 'Өз мәнерім (myskin)',
	'chick'       => 'Балапан (chick)',
	'simple'      => 'Кәдімгі (simple)',
	'modern'      => 'Заманауи (modern)',
);

$datePreferences = array(
	'default',
	'mdy',
	'dmy',
	'ymd',
	'yyyy-mm-dd',
	'persian',
	'hebrew',
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
	'mdy date' => 'xg j, Y "ж."',
	'mdy both' => 'H:i, xg j, Y "ж."',

	'dmy time' => 'H:i',
	'dmy date' => 'j F, Y "ж."',
	'dmy both' => 'H:i, j F, Y "ж."',

	'ymd time' => 'H:i',
	'ymd date' => 'Y "ж." xg j',
	'ymd both' => 'H:i, Y "ж." xg j',

	'yyyy-mm-dd time' => 'xnH:xni:xns',
	'yyyy-mm-dd date' => 'xnY-xnm-xnd',
	'yyyy-mm-dd both' => 'xnH:xni:xns, xnY-xnm-xnd',

	'persian time' => 'H:i',
	'persian date' => 'xij xiF xiY', 
	'persian both' => 'xij xiF xiY, H:i',
	
	'hebrew time' => 'H:i',
	'hebrew date' => 'xjj xjF xjY',
	'hebrew both' => 'H:i, xjj xjF xjY',

	'ISO 8601 time' => 'xnH:xni:xns',
	'ISO 8601 date' => 'xnY-xnm-xnd',
	'ISO 8601 both' => 'xnY-xnm-xnd"T"xnH:xni:xns',
);

/**
 * Magic words
 * Customisable syntax for wikitext and elsewhere.
 *
 * IDs must be valid identifiers, they can't contain hyphens. 
 *
 * Note to translators:
 *   Please include the English words as synonyms.  This allows people
 *   from other wikis to contribute more easily.
 *   Please don't remove deprecated values, them should be keeped for backward compatibility.
 *
 * This array can be modified at runtime with the LanguageGetMagic hook
 */
$magicWords = array(
#   ID                                 CASE  SYNONYMS
	'redirect'               => array( 0,    '#REDIRECT', '#АЙДАУ' ),
	'notoc'                  => array( 0,    '__МАЗМҰНСЫЗ__', '__МСЫЗ__', '__NOTOC__' ),
	'nogallery'              => array( 0,    '__ҚОЙМАСЫЗ__', '__ҚСЫЗ__', '__NOGALLERY__' ),
	'forcetoc'               => array( 0,    '__МАЗМҰНДАТҚЫЗУ__', '__МҚЫЗУ__', '__FORCETOC__' ),
	'toc'                    => array( 0,    '__МАЗМҰНЫ__', '__МЗМН__', '__TOC__' ),
	'noeditsection'          => array( 0,    '__БӨЛІДІМӨНДЕМЕУ__', '__БӨЛІМӨНДЕТКІЗБЕУ__', '__NOEDITSECTION__' ),
	'currentmonth'           => array( 1,    'АҒЫМДАҒЫАЙ', 'CURRENTMONTH' ),
	'currentmonthname'       => array( 1,    'АҒЫМДАҒЫАЙАТАУЫ', 'CURRENTMONTHNAME' ),
	'currentmonthnamegen'    => array( 1,    'АҒЫМДАҒЫАЙІЛІКАТАУЫ', 'CURRENTMONTHNAMEGEN' ),
	'currentmonthabbrev'     => array( 1,    'АҒЫМДАҒЫАЙЖИЫР', 'АҒЫМДАҒЫАЙҚЫСҚА', 'CURRENTMONTHABBREV' ),
	'currentday'             => array( 1,    'АҒЫМДАҒЫКҮН', 'CURRENTDAY' ),
	'currentday2'            => array( 1,    'АҒЫМДАҒЫКҮН2', 'CURRENTDAY2' ),
	'currentdayname'         => array( 1,    'АҒЫМДАҒЫКҮНАТАУЫ', 'CURRENTDAYNAME' ),
	'currentyear'            => array( 1,    'АҒЫМДАҒЫЖЫЛ', 'CURRENTYEAR' ),
	'currenttime'            => array( 1,    'АҒЫМДАҒЫУАҚЫТ', 'CURRENTTIME' ),
	'currenthour'            => array( 1,    'АҒЫМДАҒЫСАҒАТ', 'CURRENTHOUR' ),
	'localmonth'             => array( 1,    'ЖЕРГІЛІКТІАЙ', 'LOCALMONTH' ),
	'localmonthname'         => array( 1,    'ЖЕРГІЛІКТІАЙАТАУЫ', 'LOCALMONTHNAME' ),
	'localmonthnamegen'      => array( 1,    'ЖЕРГІЛІКТІАЙІЛІКАТАУЫ', 'LOCALMONTHNAMEGEN' ),
	'localmonthabbrev'       => array( 1,    'ЖЕРГІЛІКТІАЙЖИЫР', 'ЖЕРГІЛІКТІАЙҚЫСҚАША', 'ЖЕРГІЛІКТІАЙҚЫСҚА', 'LOCALMONTHABBREV' ),
	'localday'               => array( 1,    'ЖЕРГІЛІКТІКҮН', 'LOCALDAY' ),
	'localday2'              => array( 1,    'ЖЕРГІЛІКТІКҮН2', 'LOCALDAY2'  ),
	'localdayname'           => array( 1,    'ЖЕРГІЛІКТІКҮНАТАУЫ', 'LOCALDAYNAME' ),
	'localyear'              => array( 1,    'ЖЕРГІЛІКТІЖЫЛ', 'LOCALYEAR' ),
	'localtime'              => array( 1,    'ЖЕРГІЛІКТІУАҚЫТ', 'LOCALTIME' ),
	'localhour'              => array( 1,    'ЖЕРГІЛІКТІСАҒАТ', 'LOCALHOUR' ),
	'numberofpages'          => array( 1,    'БЕТСАНЫ', 'NUMBEROFPAGES' ),
	'numberofarticles'       => array( 1,    'МАҚАЛАСАНЫ', 'NUMBEROFARTICLES' ),
	'numberoffiles'          => array( 1,    'ФАЙЛСАНЫ', 'NUMBEROFFILES' ),
	'numberofusers'          => array( 1,    'ҚАТЫСУШЫСАНЫ', 'NUMBEROFUSERS' ),
	'numberofedits'          => array( 1,    'ӨҢДЕМЕСАНЫ', 'ТҮЗЕТУСАНЫ', 'NUMBEROFEDITS' ),
	'pagename'               => array( 1,    'БЕТАТАУЫ', 'PAGENAME' ),
	'pagenamee'              => array( 1,    'БЕТАТАУЫ2', 'PAGENAMEE' ),
	'namespace'              => array( 1,    'ЕСІМАЯСЫ', 'NAMESPACE' ),
	'namespacee'             => array( 1,    'ЕСІМАЯСЫ2', 'NAMESPACEE' ),
	'talkspace'              => array( 1,    'ТАЛҚЫЛАУАЯСЫ', 'TALKSPACE' ),
	'talkspacee'             => array( 1,    'ТАЛҚЫЛАУАЯСЫ2', 'TALKSPACEE' ),
	'subjectspace'           => array( 1,    'ТАҚЫРЫПБЕТІ', 'МАҚАЛАБЕТІ', 'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'          => array( 1,    'ТАҚЫРЫПБЕТІ2', 'МАҚАЛАБЕТІ2', 'SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'           => array( 1,    'ТОЛЫҚБЕТАТАУЫ', 'FULLPAGENAME' ),
	'fullpagenamee'          => array( 1,    'ТОЛЫҚБЕТАТАУЫ2', 'FULLPAGENAMEE' ),
	'subpagename'            => array( 1,    'БЕТШЕАТАУЫ', 'АСТЫҢҒЫБЕТАТАУЫ', 'SUBPAGENAME' ),
	'subpagenamee'           => array( 1,    'БЕТШЕАТАУЫ2', 'АСТЫҢҒЫБЕТАТАУЫ2', 'SUBPAGENAMEE' ),
	'basepagename'           => array( 1,    'НЕГІЗГІБЕТАТАУЫ', 'BASEPAGENAME' ),
	'basepagenamee'          => array( 1,    'НЕГІЗГІБЕТАТАУЫ2', 'BASEPAGENAMEE' ),
	'talkpagename'           => array( 1,    'ТАЛҚЫЛАУБЕТАТАУЫ', 'TALKPAGENAME' ),
	'talkpagenamee'          => array( 1,    'ТАЛҚЫЛАУБЕТАТАУЫ2', 'TALKPAGENAMEE' ),
	'subjectpagename'        => array( 1,    'ТАҚЫРЫПБЕТАТАУЫ', 'МАҚАЛАБЕТАТАУЫ', 'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'       => array( 1,    'ТАҚЫРЫПБЕТАТАУЫ2', 'МАҚАЛАБЕТАТАУЫ2', 'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                    => array( 0,    'ХБР:', 'MSG:' ),
	'subst'                  => array( 0,    'БӘДЕЛ:', 'SUBST:' ),
	'msgnw'                  => array( 0,    'УИКИСІЗХБР:', 'MSGNW:' ),
	'img_thumbnail'          => array( 1,    'нобай', 'thumbnail', 'thumb' ),
	'img_manualthumb'        => array( 1,    'нобай=$1', 'thumbnail=$1', 'thumb=$1'),
	'img_right'              => array( 1,    'оңға', 'оң', 'right' ),
	'img_left'               => array( 1,    'солға', 'сол', 'left' ),
	'img_none'               => array( 1,    'ешқандай', 'жоқ', 'none' ),
	'img_width'              => array( 1,    '$1 нүкте', '$1px' ),
	'img_center'             => array( 1,    'ортаға', 'орта', 'center', 'centre' ),
	'img_framed'             => array( 1,    'сүрмелі', 'framed', 'enframed', 'frame' ),
	'img_frameless'          => array( 1,    'сүрмесіз', 'frameless' ),
	'img_page'               => array( 1,    'бет=$1', 'бет $1', 'page=$1', 'page $1' ),
	'img_upright'            => array( 1,    'тікті', 'тіктік=$1', 'тіктік $1', 'upright', 'upright=$1', 'upright $1' ),
	'img_border'             => array( 1,    'жиекті', 'border' ),
	'img_baseline'           => array( 1,    'тірекжол', 'baseline' ),
	'img_sub'                => array( 1,    'астылығы', 'аст', 'sub'),
	'img_super'              => array( 1,    'үстілігі', 'үст', 'sup', 'super', 'sup' ),
	'img_top'                => array( 1,    'үстіне', 'top' ),
	'img_text_top'           => array( 1,    'мәтін-үстінде', 'text-top' ),
	'img_middle'             => array( 1,    'аралығына', 'middle' ),
	'img_bottom'             => array( 1,    'астына', 'bottom' ),
	'img_text_bottom'        => array( 1,    'мәтін-астында', 'text-bottom' ),
	'int'                    => array( 0,    'ІШКІ:', 'INT:' ),
	'sitename'               => array( 1,    'ТОРАПАТАУЫ', 'SITENAME' ),
	'ns'                     => array( 0,    'ЕА:', 'ЕСІМАЯ:', 'NS:' ),
	'localurl'               => array( 0,    'ЖЕРГІЛІКТІЖАЙ:', 'LOCALURL:' ),
	'localurle'              => array( 0,    'ЖЕРГІЛІКТІЖАЙ2:', 'LOCALURLE:' ),
	'server'                 => array( 0,    'СЕРВЕР', 'SERVER' ),
	'servername'             => array( 0,    'СЕРВЕРАТАУЫ', 'SERVERNAME' ),
	'scriptpath'             => array( 0,    'ӘМІРЖОЛЫ', 'SCRIPTPATH' ),
	'grammar'                => array( 0,    'СЕПТІГІ:', 'СЕПТІК:', 'GRAMMAR:' ),
	'notitleconvert'         => array( 0,    '__ТАҚЫРЫПАТЫНТҮРЛЕНДІРГІЗБЕУ__', '__ТАТЖОҚ__', '__АТАУАЛМАСТЫРҒЫЗБАУ__', '__ААБАУ__', '__NOTITLECONVERT__', '__NOTC__' ),
	'nocontentconvert'       => array( 0,    '__МАҒЛҰМАТЫНТҮРЛЕНДІРГІЗБЕУ__', '__МАТЖОҚ__', '__МАҒЛҰМАТАЛМАСТЫРҒЫЗБАУ__', '__МАБАУ__', '__NOCONTENTCONVERT__', '__NOCC__' ),
	'currentweek'            => array( 1,    'АҒЫМДАҒЫАПТАСЫ', 'АҒЫМДАҒЫАПТА', 'CURRENTWEEK' ),
	'currentdow'             => array( 1,    'АҒЫМДАҒЫАПТАКҮНІ', 'CURRENTDOW' ),
	'localweek'              => array( 1,    'ЖЕРГІЛІКТІАПТАСЫ', 'ЖЕРГІЛІКТІАПТА', 'LOCALWEEK' ),
	'localdow'               => array( 1,    'ЖЕРГІЛІКТІАПТАКҮНІ', 'LOCALDOW' ),
	'revisionid'             => array( 1,    'ТҮЗЕТУНӨМІРІ', 'НҰСҚАНӨМІРІ', 'REVISIONID' ),
	'revisionday'            => array( 1,    'ТҮЗЕТУКҮНІ','НҰСҚАКҮНІ', 'REVISIONDAY' ),
	'revisionday2'           => array( 1,    'ТҮЗЕТУКҮНІ2', 'НҰСҚАКҮНІ2', 'REVISIONDAY2' ),
	'revisionmonth'          => array( 1,    'ТҮЗЕТУАЙЫ', 'НҰСҚААЙЫ', 'REVISIONMONTH' ),
	'revisionyear'           => array( 1,    'ТҮЗЕТУЖЫЛЫ', 'НҰСҚАЖЫЛЫ', 'REVISIONYEAR' ),
	'revisiontimestamp'      => array( 1,    'ТҮЗЕТУУАҚЫТЫТАҢБАСЫ', 'НҰСҚАУАҚЫТТҮЙІНДЕМЕСІ', 'REVISIONTIMESTAMP' ),
	'plural'                 => array( 0,    'КӨПШЕТҮРІ:','КӨПШЕ:', 'PLURAL:' ),
	'fullurl'                => array( 0,    'ТОЛЫҚЖАЙЫ:', 'ТОЛЫҚЖАЙ:', 'FULLURL:' ),
	'fullurle'               => array( 0,    'ТОЛЫҚЖАЙЫ2:', 'ТОЛЫҚЖАЙ2:', 'FULLURLE:' ),
	'lcfirst'                => array( 0,    'КӘ1:', 'КІШІӘРІППЕН1:', 'LCFIRST:' ),
	'ucfirst'                => array( 0,    'БӘ1:', 'БАСӘРІППЕН1:', 'UCFIRST:' ),
	'lc'                     => array( 0,    'КӘ:', 'КІШІӘРІППЕН:', 'LC:' ),
	'uc'                     => array( 0,    'БӘ:', 'БАСӘРІППЕН:', 'UC:' ),
	'raw'                    => array( 0,    'ҚАМ:', 'RAW:' ),
	'displaytitle'           => array( 1,    'КӨРІНЕТІНТАҚЫРЫАПАТЫ', 'КӨРСЕТІЛЕТІНАТАУ', 'DISPLAYTITLE' ),
	'rawsuffix'              => array( 1,    'Қ', 'R' ),
	'newsectionlink'         => array( 1,    '__ЖАҢАБӨЛІМСІЛТЕМЕСІ__', '__NEWSECTIONLINK__' ),
	'currentversion'         => array( 1,    'БАҒДАРЛАМАНҰСҚАСЫ', 'CURRENTVERSION' ),
	'urlencode'              => array( 0,    'ЖАЙДЫМҰҚАМДАУ:', 'URLENCODE:' ),
	'anchorencode'           => array( 0,    'ЖӘКІРДІМҰҚАМДАУ', 'ANCHORENCODE' ),
	'currenttimestamp'       => array( 1,    'АҒЫМДАҒЫУАҚЫТТҮЙІНДЕМЕСІ', 'АҒЫМДАҒЫУАҚЫТТҮЙІН', 'CURRENTTIMESTAMP' ),
	'localtimestamp'         => array( 1,    'ЖЕРГІЛІКТІУАҚЫТТҮЙІНДЕМЕСІ', 'ЖЕРГІЛІКТІУАҚЫТТҮЙІН', 'LOCALTIMESTAMP' ),
	'directionmark'          => array( 1,    'БАҒЫТБЕЛГІСІ', 'DIRECTIONMARK', 'DIRMARK' ),
	'language'               => array( 0,    '#ТІЛ:', '#LANGUAGE:' ),
	'contentlanguage'        => array( 1,    'МАҒЛҰМАТТІЛІ', 'CONTENTLANGUAGE', 'CONTENTLANG' ),
	'pagesinnamespace'       => array( 1,    'ЕСІМАЯБЕТСАНЫ:', 'ЕАБЕТСАНЫ:', 'АЯБЕТСАНЫ:', 'PAGESINNAMESPACE:', 'PAGESINNS:' ),
	'numberofadmins'         => array( 1,    'ӘКІМШІСАНЫ', 'NUMBEROFADMINS' ),
	'formatnum'              => array( 0,    'САНПІШІМІ', 'FORMATNUM' ),
	'padleft'                => array( 0,    'СОЛҒАЫҒЫС', 'СОЛЫҒЫС', 'PADLEFT' ),
	'padright'               => array( 0,    'ОҢҒАЫҒЫС', 'ОҢЫҒЫС', 'PADRIGHT' ),
	'special'                => array( 0,    'арнайы', 'special' ),
	'defaultsort'            => array( 1,    'ӘДЕПКІСҰРЫПТАУ:', 'ӘДЕПКІСАНАТСҰРЫПТАУ:', 'ӘДЕПКІСҰРЫПТАУКІЛТІ:', 'ӘДЕПКІСҰРЫП:', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:' ),
	'filepath'               => array( 0,    'ФАЙЛМЕКЕНІ:', 'FILEPATH:' ),
	'tag'                    => array( 0,    'белгі', 'tag' ),
	'hiddencat'              => array( 1,    '__ЖАСЫРЫНСАНАТ__', '__HIDDENCAT__' ),
	'pagesincategory'        => array( 1,    'САНАТТАҒЫБЕТТЕР', 'PAGESINCATEGORY', 'PAGESINCAT' ),
	'pagesize'               => array( 1,    'БЕТМӨЛШЕРІ', 'PAGESIZE' ),
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Шынжырлы_айдағыштар', 'Шынжырлы_айдатулар' ),
	'BrokenRedirects'           => array( 'Жарамсыз_айдағыштар', 'Жарамсыз_айдатулар' ),
	'Disambiguations'           => array( 'Айрықты_беттер' ),
	'Userlogin'                 => array( 'Қатысушы_кіруі' ),
	'Userlogout'                => array( 'Қатысушы_шығуы' ),
	'CreateAccount'             => array( 'Жаңа_тіркелгі', 'Тіркелгі_Жарату' ),
	'Preferences'               => array( 'Бапталымдар', 'Баптау' ),
	'Watchlist'                 => array( 'Бақылау_тізімі' ),
	'Recentchanges'             => array( 'Жуықтағы_өзгерістер' ),
	'Upload'                    => array( 'Қотарып_беру', 'Қотару' ),
	'Imagelist'                 => array( 'Сурет_тізімі' ),
	'Newimages'                 => array( 'Жаңа_суреттер' ),
	'Listusers'                 => array( 'Қатысушылар', 'Қатысушы_тізімі' ),
	'Listgrouprights'           => array( 'Топ_құқықтары_тізімі' ),
	'Statistics'                => array( 'Санақ' ),
	'Randompage'                => array( 'Кездейсоқ', 'Кездейсоқ_бет' ),
	'Lonelypages'               => array( 'Саяқ_беттер' ),
	'Uncategorizedpages'        => array( 'Санатсыз_беттер' ),
	'Uncategorizedcategories'   => array( 'Санатсыз_санаттар' ),
	'Uncategorizedimages'       => array( 'Санатсыз_суреттер' ),
	'Uncategorizedtemplates'    => array( 'Санатсыз_үлгілер' ),
	'Unusedcategories'          => array( 'Пайдаланылмаған_санаттар' ),
	'Unusedimages'              => array( 'Пайдаланылмаған_суреттер' ),
	'Wantedpages'               => array( 'Толтырылмаған_беттер', 'Жарамсыз_сілтемелер' ),
	'Wantedcategories'          => array( 'Толтырылмаған_санаттар' ),
	'Missingfiles'              => array( 'Жоқ_файлдар', 'Жоқ_суреттер' ),
	'Mostlinked'                => array( 'Ең_көп_сілтенген_беттер' ),
	'Mostlinkedcategories'      => array( 'Ең_көп_пайдаланылған_санаттар', 'Ең_көп_сілтенген_санаттар' ),
	'Mostlinkedtemplates'       => array( 'Ең_көп_пайдаланылған_үлгілер', 'Ең_көп_сілтенген_үлгілер' ),
	'Mostcategories'            => array( 'Ең_көп_санаттар_бары' ),
	'Mostimages'                => array( 'Ең_көп_пайдаланылған_суреттер', 'Ең_көп_суреттер_бары' ),
	'Mostrevisions'             => array( 'Ең_көп_түзетілген', 'Ең_көп_нұсқалар_бары' ),
	'Fewestrevisions'           => array( 'Ең_аз_түзетілген ' ),
	'Shortpages'                => array( 'Қысқа_беттер' ),
	'Longpages'                 => array( 'Ұзын_беттер', 'Үлкен_беттер' ),
	'Newpages'                  => array( 'Жаңа_беттер' ),
	'Ancientpages'              => array( 'Ескі_беттер' ),
	'Deadendpages'              => array( 'Тұйық_беттер' ),
	'Protectedpages'            => array( 'Қорғалған_беттер' ),
	'Protectedtitles'           => array( 'Қорғалған_тақырыптар', 'Қорғалған_атаулар' ),
	'Allpages'                  => array( 'Барлық_беттер' ),
	'Prefixindex'               => array( 'Бастауыш_тізімі' ) ,
	'Ipblocklist'               => array( 'Бұғатталғандар' ),
	'Specialpages'              => array( 'Арнайы_беттер' ),
	'Contributions'             => array( 'Үлесі' ),
	'Emailuser'                 => array( 'Хат_жіберу' ),
	'Confirmemail'              => array( 'Құптау_хат' ),
	'Whatlinkshere'             => array( 'Мында_сілтегендер' ),
	'Recentchangeslinked'       => array( 'Сілтенгендердің_өзгерістері', 'Қатысты_өзгерістер' ),
	'Movepage'                  => array( 'Бетті_жылжыту' ),
	'Blockme'                   => array( 'Өздіктік_бұғаттау', 'Өздік_бұғаттау', 'Мені_бұғаттау',),
	'Booksources'               => array( 'Кітап_қайнарлары' ),
	'Categories'                => array( 'Санаттар' ),
	'Export'                    => array( 'Сыртқа_беру' ),
	'Version'                   => array( 'Нұсқасы' ),
	'Allmessages'               => array( 'Барлық_хабарлар' ),
	'Log'                       => array( 'Журнал', 'Журналдар' ),
	'Blockip'                   => array( 'Жайды_бұғаттау', 'IP_бұғаттау'),
	'Undelete'                  => array( 'Жоюды_болдырмау', 'Жойылғанды_қайтару' ),
	'Import'                    => array( 'Сырттан_алу' ),
	'Lockdb'                    => array( 'Дерекқорды_құлыптау' ),
	'Unlockdb'                  => array( 'Дерекқорды_құлыптамау' ),
	'Userrights'                => array( 'Қатысушы_құқықтары' ),
	'MIMEsearch'                => array( 'MIME_түрімен_іздеу' ),
	'FileDuplicateSearch'       => array( 'Файл_телнұсқасын_іздеу', 'Қайталанған_файлдарды_іздеу' ),
	'Unwatchedpages'            => array( 'Бақыланылмаған_беттер' ),
	'Listredirects'             => array( 'Айдату_тізімі' ),
	'Revisiondelete'            => array( 'Түзету_жою', 'Нұсқаны_жою' ),
	'Unusedtemplates'           => array( 'Пайдаланылмаған_үлгілер' ),
	'Randomredirect'            => array( 'Кедейсоқ_айдағыш', 'Кедейсоқ_айдату' ),
	'Mypage'                    => array( 'Жеке_бетім' ),
	'Mytalk'                    => array( 'Талқылауым' ),
	'Mycontributions'           => array( 'Үлесім' ),
	'Listadmins'                => array( 'Әкімшілер', 'Әкімші_тізімі'),
	'Listbots'                  => array( 'Боттар', 'Боттар_тізімі' ),
	'Popularpages'              => array( 'Ең_көп_қаралған_беттер', 'Әйгілі_беттер' ),
	'Search'                    => array( 'Іздеу' ),
	'Resetpass'                 => array( 'Құпия_сөзді_қайтару' ),
	'Withoutinterwiki'          => array( 'Уики-аралықсыздар' ),
	'MergeHistory'              => array( 'Тарих_біріктіру' ),
	'Filepath'                  => array( 'Файл_мекені' ),
	'Invalidateemail'           => array( 'Құптамау_хаты' ),
);

#-------------------------------------------------------------------
# Default messages
#-------------------------------------------------------------------

$messages = array(
# User preference toggles
'tog-underline'               => 'Сілтеменің астын сыз:',
'tog-highlightbroken'         => 'Жарамсыз сілтемелерді <a href="" class="new">былай сияқты</a> пішімде (баламасы: былай сияқты<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Ежелерді ені бойынша туралау',
'tog-hideminor'               => 'Жуықтағы өзгерістерден шағын өңдемелерді жасыр',
'tog-extendwatchlist'         => 'Бақылау тізімді ұлғайт (барлық жарамды өзгерістерді көрсет)',
'tog-usenewrc'                => 'Кеңейтілген жуықтағы өзгерістер (JavaScript)',
'tog-numberheadings'          => 'Бас жолдарды өздіктік номірле',
'tog-showtoolbar'             => 'Өңдеу қуралдар жолағын көрсет (JavaScript)',
'tog-editondblclick'          => 'Қос нұқымдап өңдеу (JavaScript)',
'tog-editsection'             => 'Бөлімдерді [өңдеу] сілтемесімен өңдеуін қос',
'tog-editsectiononrightclick' => 'Бөлім тақырыбын оң нұқумен өңдеуін қос (JavaScript)',
'tog-showtoc'                 => 'Мазмұнын көрсет (3-тен арта бөлімі барыларға)',
'tog-rememberpassword'        => 'Кіргенімді осы компьютерде ұмытпа',
'tog-editwidth'               => 'Кірістіру орны толық енімен',
'tog-watchcreations'          => 'Мен бастаған беттерді бақылау тізіміме үсте',
'tog-watchdefault'            => 'Мен өңдеген беттерді бақылау тізіміме үсте',
'tog-watchmoves'              => 'Мен жылжытқан беттерді бақылау тізіміме үсте',
'tog-watchdeletion'           => 'Мен жойған беттерді бақылау тізіміме үсте',
'tog-minordefault'            => 'Әдепкіден барлық өңдемелерді шағын деп белгіле',
'tog-previewontop'            => 'Қарап шығу аумағы кірістіру орны алдында',
'tog-previewonfirst'          => 'Бірінші өңдегенде қарап шығу',
'tog-nocache'                 => 'Бет бүркемелеуін өшір',
'tog-enotifwatchlistpages'    => 'Бақыланған бет өзгергенде маған хат жібер',
'tog-enotifusertalkpages'     => 'Талқылауым өзгергенде маған хат жібер',
'tog-enotifminoredits'        => 'Шағын өңдеме туралы да маған хат жібер',
'tog-enotifrevealaddr'        => 'Е-поштамның мекенжайын ескерту хаттарда аш',
'tog-shownumberswatching'     => 'Бақылап тұрған қатысушылардың санын көрсет',
'tog-fancysig'                => 'Қам қолтаңба (өздіктік сілтемесіз)',
'tog-externaleditor'          => 'Шеттік өңдеуішті әдепкіден қолдан (тек сарапшылар үшін, компьютеріңізде арнаулы бапталымдар керек)',
'tog-externaldiff'            => 'Шеттік айырмағышты әдепкіден қолдан (тек сарапшылар үшін, компьютеріңізде арнаулы бапталымдар керек)',
'tog-showjumplinks'           => '«Өтіп кету» қатынау сілтемелерін қос',
'tog-uselivepreview'          => 'Тура қарап шығуды қолдану (JavaScript) (Сынақтама)',
'tog-forceeditsummary'        => 'Өңдеменің қысқаша мазмұндамасы бос қалғанда маған ескерт',
'tog-watchlisthideown'        => 'Өңдемелерімді бақылау тізімнен жасыр',
'tog-watchlisthidebots'       => 'Бот өңдемелерін бақылау тізімнен жасыр',
'tog-watchlisthideminor'      => 'Шағын өңдемелерді бақылау тізімінде көрсетпе',
'tog-nolangconversion'        => 'Тіл түрі аударысын өшір',
'tog-ccmeonemails'            => 'Басқа қатысушыға жіберген хатымның көшірмесін маған да жөнелт',
'tog-diffonly'                => 'Айырма астында бет мағлұматын көрсетпе',
'tog-showhiddencats'          => 'Жасырын санаттарды көрсет',

'underline-always'  => 'Әрқашан',
'underline-never'   => 'Ешқашан',
'underline-default' => 'Шолғыш бойынша',

'skinpreview' => '(Қарап шығу)',

# Dates
'sunday'        => 'Жексенбі',
'monday'        => 'Дүйсенбі',
'tuesday'       => 'Сейсенбі',
'wednesday'     => 'Сәрсенбі',
'thursday'      => 'Бейсенбі',
'friday'        => 'Жұма',
'saturday'      => 'Сенбі',
'sun'           => 'Жек',
'mon'           => 'Дүй',
'tue'           => 'Бей',
'wed'           => 'Сәр',
'thu'           => 'Бей',
'fri'           => 'Жұм',
'sat'           => 'Сен',
'january'       => 'қаңтар',
'february'      => 'ақпан',
'march'         => 'наурыз',
'april'         => 'cәуір',
'may_long'      => 'мамыр',
'june'          => 'маусым',
'july'          => 'шілде',
'august'        => 'тамыз',
'september'     => 'қыркүйек',
'october'       => 'қазан',
'november'      => 'қараша',
'december'      => 'желтоқсан',
'january-gen'   => 'қаңтардың',
'february-gen'  => 'ақпанның',
'march-gen'     => 'наурыздың',
'april-gen'     => 'сәуірдің',
'may-gen'       => 'мамырдың',
'june-gen'      => 'маусымның',
'july-gen'      => 'шілденің',
'august-gen'    => 'тамыздың',
'september-gen' => 'қыркүйектің',
'october-gen'   => 'қазанның',
'november-gen'  => 'қарашаның',
'december-gen'  => 'желтоқсанның',
'jan'           => 'қаң',
'feb'           => 'ақп',
'mar'           => 'нау',
'apr'           => 'cәу',
'may'           => 'мам',
'jun'           => 'мау',
'jul'           => 'шіл',
'aug'           => 'там',
'sep'           => 'қыр',
'oct'           => 'қаз',
'nov'           => 'қар',
'dec'           => 'жел',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Санат|Санаттар}}',
'category_header'                => '«$1» санатындағы беттер',
'subcategories'                  => 'Санатшалар',
'category-media-header'          => '«$1» санатындағы таспалар',
'category-empty'                 => "''Бұл санатта ағымда еш бет не таспа жоқ.''",
'hidden-categories'              => '{{PLURAL:$1|Жасырын санат|Жасырын санаттар}}',
'hidden-category-category'       => 'Жасырын санаттар', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Бұл санатта тек келесі санатша бар.|Бұл санатта келесі $1 санатша бар (не барлығы $2).}}',
'category-subcat-count-limited'  => 'Бұл санатта келесі $1 санатша бар.',
'category-article-count'         => '{{PLURAL:$2|Бұл санатта тек келесі бет бар.|Бұл санатта келесі $1 бет бар (не барлығы $2).}}',
'category-article-count-limited' => 'Ағымдағы санатта келесі $1 бет бар.',
'category-file-count'            => '{{PLURAL:$2|Бұд санатта тек келесі файл бар.|Бұл санатта келесі $1 файл бар (не барлығы $2).}}',
'category-file-count-limited'    => 'Ағымдағы санатта келесі $1 файл бар.',
'listingcontinuesabbrev'         => '(жалғ.)',

'mainpagetext'      => "<big>'''МедиаУики бумасы сәтті орнатылды.'''</big>",
'mainpagedocfooter' => 'Уики бағдарламалық жасақтамасын қалай қолданатын ақпараты үшін [http://meta.wikimedia.org/wiki/Help:Contents Пайдаланушылық нұсқауларынан] кеңес алыңыз.

== Бастау үшін ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Бапталым қалауларының тізімі]
* [http://www.mediawiki.org/wiki/Manual:FAQ МедиаУикидің Жиы Қойылған Сауалдары]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce МедиаУики шығу туралы хат тарату тізімі]',

'about'          => 'Жоба туралы',
'article'        => 'Мағлұмат беті',
'newwindow'      => '(жаңа терезеде)',
'cancel'         => 'Болдырмау',
'qbfind'         => 'Табу',
'qbbrowse'       => 'Шолу',
'qbedit'         => 'Өңдеу',
'qbpageoptions'  => 'Бұл бет',
'qbpageinfo'     => 'Айнала',
'qbmyoptions'    => 'Беттерім',
'qbspecialpages' => 'Арнайы беттер',
'moredotdotdot'  => 'Көбірек…',
'mypage'         => 'Жеке бетім',
'mytalk'         => 'Талқылауым',
'anontalk'       => 'IP талқылауы',
'navigation'     => 'Шарлау',
'and'            => 'және',

# Metadata in edit box
'metadata_help' => 'Қосымша деректер:',

'errorpagetitle'    => 'Қателік',
'returnto'          => '$1 дегенге қайта келу.',
'tagline'           => '{{GRAMMAR:ablative|{{SITENAME}}}}',
'help'              => 'Анықтама',
'search'            => 'Іздеу',
'searchbutton'      => 'Ізде',
'go'                => 'Өту',
'searcharticle'     => 'Өт!',
'history'           => 'Бет тарихы',
'history_short'     => 'Тарихы',
'updatedmarker'     => 'соңғы келіп-кетуімнен бері жаңаланған',
'info_short'        => 'Мәлімет',
'printableversion'  => 'Басып шығару үшін',
'permalink'         => 'Тұрақты сілтеме',
'print'             => 'Басып шығару',
'edit'              => 'Өңдеу',
'create'            => 'Бастау',
'editthispage'      => 'Бетті өңдеу',
'create-this-page'  => 'Жаңа бет бастау',
'delete'            => 'Жою',
'deletethispage'    => 'Бетті жою',
'undelete_short'    => '$1 өңдеме жоюын болдырмау',
'protect'           => 'Қорғау',
'protect_change'    => 'қорғауды өзгерту',
'protectthispage'   => 'Бетті қорғау',
'unprotect'         => 'Қорғамау',
'unprotectthispage' => 'Бетті қорғамау',
'newpage'           => 'Жаңа бет',
'talkpage'          => 'Бетті талқылау',
'talkpagelinktext'  => 'Талқылауы',
'specialpage'       => 'Арнайы бет',
'personaltools'     => 'Жеке құралдар',
'postcomment'       => 'Мәндеме жөнелту',
'articlepage'       => 'Мағлұмат бетін қарау',
'talk'              => 'Талқылау',
'views'             => 'Көрініс',
'toolbox'           => 'Құралдар',
'userpage'          => 'Қатысушы бетін қарау',
'projectpage'       => 'Жоба бетін қарау',
'imagepage'         => 'Таспа бетін қарау',
'mediawikipage'     => 'Хабар бетін қарау',
'templatepage'      => 'Үлгі бетін қарау',
'viewhelppage'      => 'Анықтама бетін қарау',
'categorypage'      => 'Санат бетін қарау',
'viewtalkpage'      => 'Талқылау бетін қарау',
'otherlanguages'    => 'Басқа тілдерде',
'redirectedfrom'    => '($1 бетінен айдатылған)',
'redirectpagesub'   => 'Айдату беті',
'lastmodifiedat'    => 'Бұл беттің өзгертілген соңғы кезі: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Бұл бет $1 рет қатыналған.',
'protectedpage'     => 'Қорғалған бет',
'jumpto'            => 'Мында өту:',
'jumptonavigation'  => 'шарлау',
'jumptosearch'      => 'іздеу',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} туралы',
'aboutpage'            => 'Project:Жоба туралы',
'bugreports'           => 'Қателік баянаттары',
'bugreportspage'       => 'Project:Қателік баянаттары',
'copyright'            => 'Мағлұмат $1 шартымен жетімді.',
'copyrightpagename'    => '{{SITENAME}} ауторлық құқықтары',
'copyrightpage'        => '{{ns:project}}:Ауторлық құқықтар',
'currentevents'        => 'Ағымдағы оқиғалар',
'currentevents-url'    => 'Project:Ағымдағы оқиғалар',
'disclaimers'          => 'Жауапкершіліктен бас тарту',
'disclaimerpage'       => 'Project:Жауапкершіліктен бас тарту',
'edithelp'             => 'Өндеу анықтамасы',
'edithelppage'         => 'Help:Өңдеу',
'faq'                  => 'Жиі қойылған сауалдар',
'faqpage'              => 'Project:Жиі қойылған сауалдар',
'helppage'             => 'Help:Мазмұны',
'mainpage'             => 'Басты бет',
'mainpage-description' => 'Басты бет',
'policy-url'           => 'Project:Ережелер',
'portal'               => 'Қауым порталы',
'portal-url'           => 'Project:Қауым порталы',
'privacy'              => 'Жеке құпиясын сақтау',
'privacypage'          => 'Project:Жеке құпиясын сақтау',

'badaccess'        => 'Рұқсат қатесі',
'badaccess-group0' => 'Сұратылған әрекетіңізді жегуіңізге рұқсат етілмейді.',
'badaccess-group1' => 'Сұратылған әрекетіңіз $1 тобының қатысушыларына шектеледі.',
'badaccess-group2' => 'Сұратылған әрекетіңіз $1 топтары бірінің қатусышыларына шектеледі.',
'badaccess-groups' => 'Сұратылған әрекетіңіз $1 топтары бірінің қатусышыларына шектеледі.',

'versionrequired'     => 'MediaWiki $1 нұсқасы керек',
'versionrequiredtext' => 'Бұл бетті қолдану үшін MediaWiki $1 нұсқасы керек. [[Special:Version|Жүйе нұсқасы бетін]] қараңыз.',

'ok'                      => 'Жарайды',
'pagetitle'               => '$1 — {{SITENAME}}',
'retrievedfrom'           => '«$1» бетінен алынған',
'youhavenewmessages'      => 'Сізге $1 бар ($2).',
'newmessageslink'         => 'жаңа хабарлар',
'newmessagesdifflink'     => 'соңғы өзгерісіне',
'youhavenewmessagesmulti' => '$1 дегенде жаңа хабарлар бар',
'editsection'             => 'өңдеу',
'editold'                 => 'өңдеу',
'viewsourceold'           => 'қайнар көзін қарау',
'editsectionhint'         => 'Мына бөлімді өңдеу: $1',
'toc'                     => 'Мазмұны',
'showtoc'                 => 'көрсет',
'hidetoc'                 => 'жасыр',
'thisisdeleted'           => '$1 қарайсыз ба, не қалпына келтіресіз бе?',
'viewdeleted'             => '$1 қарайсыз ба?',
'restorelink'             => 'Жойылған $1 өңдемені',
'feedlinks'               => 'Арна:',
'feed-invalid'            => 'Жарамсыз жазылымды арна түрі.',
'feed-unavailable'        => '{{SITENAME}} жобасында таратылатын арналар жоқ',
'site-rss-feed'           => '$1 RSS арнасы',
'site-atom-feed'          => '$1 Atom арнасы',
'page-rss-feed'           => '«$1» — RSS арнасы',
'page-atom-feed'          => '«$1» — Atom арнасы',
'red-link-title'          => '$1 (әлі жазылмаған)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Бет',
'nstab-user'      => 'Жеке бет',
'nstab-media'     => 'Таспа беті',
'nstab-special'   => 'Арнайы',
'nstab-project'   => 'Жоба беті',
'nstab-image'     => 'Файл беті',
'nstab-mediawiki' => 'Хабар',
'nstab-template'  => 'Үлгі',
'nstab-help'      => 'Анықтама',
'nstab-category'  => 'Санат',

# Main script and global functions
'nosuchaction'      => 'Мынадай еш әрекет жоқ',
'nosuchactiontext'  => 'Осы URL жайымен енгізілген әрекетті осы уики жорамалдап білмеді.',
'nosuchspecialpage' => 'Мынадай еш арнайы бет жоқ',
'nospecialpagetext' => "<big>'''Жарамсыз арнайы бетті сұрадыңыз.'''</big>

Жарамды арнайы бет тізімін [[{{#special:Specialpages}}|{{int:specialpages}}]] дегеннен таба аласыз.",

# General errors
'error'                => 'Қате',
'databaseerror'        => 'Дерекқор қатесі',
'dberrortext'          => 'Дерекқор сұранымында сөйлем жүйесінің қатесі болды.
Бұл бағдарламалық жасақтама қатесін белгілеуі мүмкін.
Соңғы болған дерекқор сұранымы:
<blockquote><tt>$1</tt></blockquote>
мына жетеден «<tt>$2</tt>».
MySQL қайтарған қатесі «<tt>$3: $4</tt>».',
'dberrortextcl'        => 'Дерекқор сұранымында сөйлем жүйесінің қатесі болды.
Соңғы болған дерекқор сұранымы:
«$1»
мына жетеден: «$2».
MySQL қайтарған қатесі «$3: $4»',
'noconnect'            => 'Ғафу етіңіз! Бұл уикиде кейбір техникалық қиыншылықтар кездесті, және де дерекқор серверіне байланыса алмайды.<br />
$1',
'nodb'                 => '$1 деген дерекқор бөлектенбеді',
'cachederror'          => 'Төменде сұралған беттің бүркемеленген көшірмесі беріледі, әлі жаңартылмаған болуы мүмкін.',
'laggedslavemode'      => 'Құлақтандыру: Бетте жуықтағы жаңалаулар болмауы мүмкін.',
'readonly'             => 'Дерекқоры құлыпталған',
'enterlockreason'      => 'Құлыптау себебін, қай уақытқа дейін құлыпталғанын кірістіріп, енгізіңіз',
'readonlytext'         => 'Бұл дерекқор жаңадан жазу және басқа өзгерістер жасаудан ағымда құлыпталынған, мүмкін күнде-күн дерекқорды баптау үшін, бұны бітіргеннен соң қалыпты іске қайтарылады.

Құлыптаған әкімші бұны былай түсіндіреді: $1',
'missing-article'      => 'Бар болуы жөн былай аталған бет мәтіні дерекқорда табылмады: «$1» $2.

Бұл ескірген айырма сілтемесіне немесе жойылған бет тарихы сілтемесіне ергеннен бола береді.

Егер бұл орынды болмаса, бағдарламалық жасақтамадағы қатеге тап болуыңыз мүмкін.
Бұл туралы нақты URL жайына аңғартпа жасап, әкімшіге баянаттаңыз.',
'missingarticle-rev'   => '(түзету н-і: $1)',
'missingarticle-diff'  => '(Айрм.: $1, $2)',
'readonly_lag'         => 'Жетек дерекқор серверлер басқысымен қадамланғанда осы дерекқор өздіктік құлыпталынған',
'internalerror'        => 'Ішкі қате',
'internalerror_info'   => 'Ішкі қатесі: $1',
'filecopyerror'        => '«$1» файлы «$2» файлына көшірілмеді.',
'filerenameerror'      => '«$1» файл атауы «$2» атауына өзгертілмеді.',
'filedeleteerror'      => '«$1» файлы жойылмайды.',
'directorycreateerror' => '«$1» қалтасы құрылмады.',
'filenotfound'         => '«$1» файлы табылмады.',
'fileexistserror'      => '«$1» файлға жазу икемді емес: файл бар',
'unexpected'           => 'Күтілмеген мағына: «$1» = «$2».',
'formerror'            => 'Қателік: пішін жөнелтілмейді',
'badarticleerror'      => 'Осындай әрекет мына бетте атқарылмайды.',
'cannotdelete'         => 'Айтылмыш бет не сурет жойылмайды.
Бұны басқа біреу алдақашан жойған мүмкін.',
'badtitle'             => 'Жарамсыз тақырып аты',
'badtitletext'         => 'Сұралған бет тақырыбының аты жарамсыз, бос, тіларалық сілтемесі не уики-аралық тақырып аты бұрыс енгізілген.
Мында тақырып атында қолдалмайтын бірқатар таңбалар болуы мүмкін.',
'perfdisabled'         => 'Ғафу етіңіз! Бұл мүмкіндік, дерекқордың жылдамылығына әсер етіп, ешкімге уикиді пайдалануға бермегесін, уақытша өшірілген.',
'perfcached'           => 'Келесі дерек бүркемеленген, сондықтан толықтай жаңаланмаған болуы мүмкін.',
'perfcachedts'         => 'Келесі дерек бүркемеленген, соңғы жаңаланлған кезі: $1.',
'querypage-no-updates' => 'Бұл беттің жаңартылуы ағымда өшірілген. Деректері қазір өзгертілмейді.',
'wrong_wfQuery_params' => 'wfQuery() функциясы үшін бұрыс бапталымдары бар<br />
Жете: $1<br />
Сұраным: $2',
'viewsource'           => 'Қайнар көзін қарау',
'viewsourcefor'        => '$1 деген үшін',
'actionthrottled'      => 'Әрекет бәсеңдетілді',
'actionthrottledtext'  => 'Спамға қарсы күрес есебінде, осы әрекетті қысқа уақытта тым көп рет орындауыңыз шектелінді, және бұл шектеу шамасынан асып кеткенсіз.
Бірнеше минөттан қайта байқап көріңіз.',
'protectedpagetext'    => 'Өңдеуді қақпайлау үшін бұл бет құлыпталынған.',
'viewsourcetext'       => 'Бұл беттің қайнар көзін қарауыңызға және көшіріп алуңызға болады:',
'protectedinterface'   => 'Бұл бет бағдарламалық жасақтаманың тілдесу мәтінін жетістіреді, сондықтан қиянатты қақпайлау үшін өзгертуі құлыпталған.',
'editinginterface'     => "'''Құлақтандыру:''' Бағдарламалық жасақтаманың тілдесу мәтінін жетістіретін бетін өңдеп жатырсыз.
Бұл беттің өзгертуі басқа қатысушыларға пайдаланушылық тілдесуі қалай көрінетіне әсер етеді.
Аудармалар үшін, MediaWiki бағдарламасын жерсіндіру [http://translatewiki.net/wiki/Main_Page?setlang=kk Betawiki жобасы] арқылы қарап шешіңіз.",
'sqlhidden'            => '(SQL сұранымы жасырылған)',
'cascadeprotected'     => 'Бұл бет өңдеуден қорғалған, себебі бұл келесі «баулы қорғауы» қосылған {{PLURAL:$1|беттің|беттердің}} кірікбеті:
$2',
'namespaceprotected'   => "'''$1''' есім аясындағы беттерді өңдеу үшін рұқсатыңыз жоқ.",
'customcssjsprotected' => 'Бұл бетті өңдеуге рұқсатыңыз жоқ, себебі мында өзге қатысушының жеке баптаулары бар.',
'ns-specialprotected'  => '{{ns:special}} есім аясындағы беттер өңделінбейді',
'titleprotected'       => "Бұл тақырып аты бастаудан [[{{ns:user}}:$1|$1]] қорғады.
Келтірілген себебі: ''$2''.",

# Login and logout pages
'logouttitle'                => 'Қатысушы шығуы',
'logouttext'                 => '<strong>Енді жүйеден шықтыңыз.</strong>

Жүйеге кірместен {{SITENAME}} жобасын пайдалануын жалғастыра аласыз, немесе дәл сол не өзге қатысушы боп қайта круіңіз мүмкін.
Аңғартпа: Кейбір беттер шолғышыңыздың бүркемесін тазартқанша дейін әлі де кірп қалғаныңыздай көрінуі мүмкін.',
'welcomecreation'            => '== Қош келдіңіз, $1! ==
Жаңа тіркелгіңіз жасалды.
{{SITENAME}} бапталымдарыңызды өзгертуін ұмытпаңыз.',
'loginpagetitle'             => 'Қатысушы кіруі',
'yourname'                   => 'Қатысушы атыңыз:',
'yourpassword'               => 'Құпия сөзіңіз:',
'yourpasswordagain'          => 'Құпия сөзді қайталаңыз:',
'remembermypassword'         => 'Менің кіргенімді бұл компьютерде ұмытпа',
'yourdomainname'             => 'Желі үйшігіңіз:',
'externaldberror'            => 'Осы арада не шеттік растау дерекқорында қате болды, немесе шеттік тіркелгіңізді жаңалау рұқсаты жоқ.',
'loginproblem'               => '<b>Кіруіңіз кезінде осында қиындыққа тап болдық.</b><br />Қайта байқап көріңіз.',
'login'                      => 'Кіру',
'nav-login-createaccount'    => 'Кіру / Тіркелгі жасау',
'loginprompt'                => '{{SITENAME}} торабына кіруіңіз үшін «cookies» қосылуы жөн.',
'userlogin'                  => 'Кіру / Тіркелгі жасау',
'logout'                     => 'Шығу',
'userlogout'                 => 'Шығу',
'notloggedin'                => 'Кірмегенсіз',
'nologin'                    => 'Кірмегенсіз бе? $1.',
'nologinlink'                => 'Тіркелгі жасаңыз',
'createaccount'              => 'Жаңа тіркелгі',
'gotaccount'                 => 'Алдақашан тіркелгііңіз бар ма? $1.',
'gotaccountlink'             => 'Кіріңіз',
'createaccountmail'          => 'е-поштамен',
'badretype'                  => 'Енгізген құпия сөздеріңіз бір біріне сәйкес емес.',
'userexists'                 => 'Енгізген қатысушы атыңыз алдақашан пайдалануда.
Өзге атауды таңдаңыз.',
'youremail'                  => 'Е-поштаңыз:',
'username'                   => 'Қатысушы атыңыз:',
'uid'                        => 'Қатысушы теңдестіргішіңіз:',
'prefs-memberingroups'       => 'Кірген {{PLURAL:$1|тобыңыз|топтарыңыз}}:',
'yourrealname'               => 'Нақты атыңыз:',
'yourlanguage'               => 'Тіліңіз:',
'yourvariant'                => 'Тіл/жазба нұсқаңыз:',
'yournick'                   => 'Қолтаңбаңыз:',
'badsig'                     => 'Қам қолтаңбаңыз жарамсыз; HTML белгішелерін тексеріңіз.',
'badsiglength'               => 'Лақап атыңыз тым ұзын;
Бұл $1 таңбадан аспауы жөн.',
'email'                      => 'Е-поштаңыз',
'prefs-help-realname'        => 'Нақты атыңыз міндетті емес.
Егер бұны жетістіруді таңдасаңыз, бұл түзетуіңіздің ауторлығын анықтау үшін қолданылады.',
'loginerror'                 => 'Кіру қатесі',
'prefs-help-email'           => 'Е-пошта мекенжайы міндетті емес, бірақ жеке басыңызды ашпай «Қатысушы» немесе «Қатысушы_талқылауы» деген беттеріңіз арқылы барша сізбен байланыса алады.',
'prefs-help-email-required'  => 'Е-пошта мекенжайы керек.',
'nocookiesnew'               => 'Жаңа қатысушы тіркелгісі жасалды, бірақ кірмегенсіз.
Қатысушы кіру үшін {{SITENAME}} торабында «cookie» файлдары қолданылады.
Сізде «cookies» өшірілген.
Соны қосыңыз да жаңа қатысушы атыңызды және құпия сөзіңізді енгізіп кіріңіз.',
'nocookieslogin'             => 'Қатысушы кіру үшін {{SITENAME}} торабында «cookies» деген қолданылады.
Сізде «cookies» өшірілген.
Соны қосыңыз да кіруді қайта байқап көріңіз.',
'noname'                     => 'Жарамды қатысушы атын енгізбедіңіз.',
'loginsuccesstitle'          => 'Кіруіңіз сәтті өтті',
'loginsuccess'               => "'''Сіз енді {{SITENAME}} жобасына «$1» ретінде кіріп отырсыз.'''",
'nosuchuser'                 => 'Мында «$1» деп аталған қатысушы жоқ.
Емлеңізді тексеріңіз, не жаңа тіркелгі жасаңыз.',
'nosuchusershort'            => 'Мында «<nowiki>$1</nowiki>» деп аталған қатысушы жоқ.
Емлеңізді тексеріңіз.',
'nouserspecified'            => 'Қатысушы атын келтіруіңіз жөн.',
'wrongpassword'              => 'Бұрыс құпия сөз енгізілген. Қайта байқап көріңіз.',
'wrongpasswordempty'         => 'Құпия сөз бос болған. Қайта байқап көріңіз.',
'passwordtooshort'           => 'Құпия сөзіңіз жарамсыз не тым қысқа.
Бұнда ең кемінде $1 таңба болуы және де қатысушы атыңыздан өзге болуы жөн.',
'mailmypassword'             => 'Құпия сөзімді хатпен жібер',
'passwordremindertitle'      => '{{SITENAME}} үшін жаңа уақытша құпия сөз',
'passwordremindertext'       => 'Кейбіреу (IP мекенжайы: $1, бәлкім өзіңіз боларсыз)
сізге {{SITENAME}} үшін жаңа құпия сөз жөнелетуін бізден сұраған ($4).
«$2» қатысушының құпия сөзі «$3» болды енді.
Қазір кіруіңіз және құпия сөзді өзгертуіңіз керек.

Егер бұл сұранымды басқа біреу істесе, не құпия сөзді еске түсірсіп енді өзгерткіңіз келмесе, ескі құпия сөз қолдануын жағастырып осы хатқа аңғармауыңызға да болады.',
'noemail'                    => 'Осы арада «$1» қатысушының е-пошта мекенжайы жоқ.',
'passwordsent'               => 'Жаңа құпия сөз «$1» үшін тіркелген е-пошта мекенжайына жөнелтілді.
Қабылдағаннан кейін кіргенде соны енгізіңіз.',
'blocked-mailpassword'       => 'IP мекенжайыңыздан өңдеу бұғатталған, сондықтан қиянатты қақпайлау үшін құпия сөзді қалпына келтіру жетесін қолдануына рұқсат етілмейді.',
'eauthentsent'               => 'Құптау хаты айтылмыш е-пошта мекенжайына жөнелтілді.
Басқа е-пошта хатын жөнелту алдынан, тіркелгі шынынан сіздікі екенін құптау үшін хаттағы нұсқамаларға еріуңіз жөн.',
'throttled-mailpassword'     => 'Соңғы {{PLURAL:$1|сағатта|$1 сағатта}} құпия сөз ескерту хаты алдақашан жөнелтілді.
Қиянатты қақпайлау үшін, {{PLURAL:$1|сағат|$1 сағат}} сайын тек бір ғана құпия сөз ескерту хаты жөнелтіледі.',
'mailerror'                  => 'Хат жөнелту қатесі: $1',
'acct_creation_throttle_hit' => 'Ғафу етіңіз, сіз алдақашан $1 рет тіркелгі жасапсыз.
Онан артық істей алмайсыз.',
'emailauthenticated'         => 'Е-пошта мекенжайыңыз расталған кезі: $1.',
'emailnotauthenticated'      => 'Е-пошта мекенжайыңыз әлі расталған жоқ.
Келесі әрбір мүмкіндіктер үшін еш хат жөнелтілмейді.',
'noemailprefs'               => 'Осы мүмкіндіктер істеуі үшін е-пошта мекенжайыңызды енгізіңіз.',
'emailconfirmlink'           => 'Е-пошта мекенжайыңызды құптаңыз',
'invalidemailaddress'        => 'Осы е-пошта мекенжайында жарамсыз пішім болған, қабыл етілмейді.
Дұрыс пішімделген мекенжайды енгізіңіз, не аумақты бос қалдырыңыз.',
'accountcreated'             => 'Жаңа тіркелгі жасалды',
'accountcreatedtext'         => '$1 үшін жаңа қатысушы тіркелгісі жасалды.',
'createaccount-title'        => '{{SITENAME}} үшін тіркелу',
'createaccount-text'         => 'Кейбіреу е-пошта мекенжайыңызды пайдаланып {{SITENAME}} жобасында ($4) «$2» атауымен, «$3» құпия сөзімен тіркелгі жасаған.
Жобаға кіріуіңіз және құпия сөзіңізді өзгертуіңіз тиісті.

Егер бұл тіркелгі қателікпен жасалса, осы хабарға елемеуіңіз мүмкін.',
'loginlanguagelabel'         => 'Тіл: $1',

# Password reset dialog
'resetpass'               => 'Тіркелгінің құпия сөзін өзгерту',
'resetpass_announce'      => 'Хатпен жіберілген уақытша кодымен кіргенсіз.
Кіруіңізді бітіру үшін, жаңа құпия сөзіңізді мында енгізуіңіз жөн:',
'resetpass_header'        => 'Құпия сөзді өзгерту',
'resetpass_submit'        => 'Құпия сөзді қойыңыз да кіріңіз',
'resetpass_success'       => 'Құпия сөзіңіз сәтті өзгертілді! Енді кіріңіз…',
'resetpass_bad_temporary' => 'Уақытша құпия сөз жарамсыз.
Мүмкін құпия сөзіңізді алдақашан сәтті өзгерткен боларсыз немесе жаңа уақытша құпия сөзін сұратылғансыз.',
'resetpass_forbidden'     => '{{SITENAME}} жобасында құпия сөздер өзгертілмейді',
'resetpass_missing'       => 'Еш пішін деректері жоқ.',

# Edit page toolbar
'bold_sample'     => 'Жуан мәтін',
'bold_tip'        => 'Жуан мәтін',
'italic_sample'   => 'Қиғаш мәтін',
'italic_tip'      => 'Қиғаш мәтін',
'link_sample'     => 'Сілтеме тақырыбын аты',
'link_tip'        => 'Ішкі сілтеме',
'extlink_sample'  => 'http://www.example.com сілтеме тақырыбын аты',
'extlink_tip'     => 'Шеттік сілтеме (алдынан http:// енгізуін ұмытпаңыз)',
'headline_sample' => 'Бас жол мәтіні',
'headline_tip'    => '2-ші деңгейлі бас жол',
'math_sample'     => 'Өрнекті мында енгізіңіз',
'math_tip'        => 'Математика өрнегі (LaTeX)',
'nowiki_sample'   => 'Пішімделінбеген мәтінді мында енгізіңіз',
'nowiki_tip'      => 'Уики пішімін елемеу',
'image_tip'       => 'Ендірілген файл',
'media_tip'       => 'Файл сілтемесі',
'sig_tip'         => 'Қолтаңбаңыз және уақыт белгісі',
'hr_tip'          => 'Дерелей сызық (үнемді қолданыңыз)',

# Edit pages
'summary'                          => 'Түйіндемесі',
'subject'                          => 'Тақырыбы/бас жолы',
'minoredit'                        => 'Бұл шағын өңдеме',
'watchthis'                        => 'Бетті бақылау',
'savearticle'                      => 'Бетті сақта!',
'preview'                          => 'Қарап шығу',
'showpreview'                      => 'Қарап шық',
'showlivepreview'                  => 'Тура қарап шық',
'showdiff'                         => 'Өзгерістерді көрсет',
'anoneditwarning'                  => "'''Құлақтандыру:''' Сіз жүйеге кірмегенсіз.
IP мекенжайыңыз бұл беттің түзету тарихында жазылып алынады.",
'missingsummary'                   => "'''Ескертпе:''' Өңдеменің қысқаша мазмұндамасын енгізбепсіз.
«Сақтау» түймесін тағы бассаңыз, өңденмеңіз мәндемесіз сақталады.",
'missingcommenttext'               => 'Мәндемеңізді төменде енгізіңіз.',
'missingcommentheader'             => "'''Ескертпе:''' Бұл мәндемеге тақырып/басжол жетістірмепсіз.
Егер тағы да Сақтау түймесін нұқысаңыз, өңдемеңіз солсыз сақталады.",
'summary-preview'                  => 'Қысқаша мазмұндамасын қарап шығу',
'subject-preview'                  => 'Тақырыбын/бас жолын қарап шығу',
'blockedtitle'                     => 'Қатысушы бұғатталған',
'blockedtext'                      => "<big>'''Қатысушы атыңыз не IP мекенжайыңыз бұғатталған.'''</big>

Осы бұғаттауды $1 істеген. Келтірілген себебі: ''$2''.

* Бұғаттаудың басталғаны: $8
* Бұғаттаудың бітетіні: $6
* Бұғаттау нысанасы: $7

Осы бұғаттауды талқылау үшін $1, не өзге [[{{MediaWiki:Grouppage-sysop}}|әкімшімен]] қатынасуыңызға болады.
[[Special:Preferences|Тіркелгі бапталымдары]]ңызда жарамды е-пошта мекенжайын ұсынып және де оны пайдаланудан бұғатталмаған жағдайыңызда ғана «Қатысушыға хат жазу» қызметін қолдана аласыз.
Ағымдық IP мекенжайыңыз: $3, және бұғатау нөмірі: $5. 
Сұраным жасағанда осының екеуін де кірістіруіңізді сұраймыз.",
'autoblockedtext'                  => "$1 деген бұрын өзге қатысушы пайдаланған болғасын осы IP мекенжайыңыз өздіктік бұғатталған.
Келтірілген себебі:

:''$2''

* Бұғаттау басталғаны: $8
* Бұғаттау бітетіні: $6

Осы бұғаттауды талқылау үшін $1 дегенмен, не басқа [[{{{{ns:mediawiki}}:grouppage-sysop}}|әкімшімен]] қатынасуыңызға болады.

Аңғартпа: [[{{#special:Preferences}}|Пайдаланушылық бапталымдарыңызды]] қолданып жарамды е-пошта мекенжайын енгізгенше дейін және бұны пайдалануы бұғатталмағанша дейін «Қатысушыға хат жазу» мүмкіндігін қолдана алмайсыз. 

Бұғатау нөміріңіз: $5.
Бұл нөмірді әрбір сұранымыңыздарға кірістіріңіз.",
'blockednoreason'                  => 'еш себебі келтірілмеген',
'blockedoriginalsource'            => "'''$1''' дегеннің қайнар көзі төменде көрсетіледі:",
'blockededitsource'                => "'''$1''' дегенге жасалған '''өңдемелеріңіздің''' мәтіні төменде көрсетіледі:",
'whitelistedittitle'               => 'Өңдеу үшін кіруіңіз жөн.',
'whitelistedittext'                => 'Беттерді өңдеу үшін $1 жөн.',
'confirmedittitle'                 => 'Құптау хаты қайта өңделуі жөн',
'confirmedittext'                  => 'Беттерді өңдеу үшін алдын ала Е-пошта мекенжайыңызды құптауыңыз жөн.
Е-пошта мекенжайыңызды [[{{#special:Preferences}}|пайдаланушылық бапталымдарыңыз]] арқылы қойыңыз да жарамдылығын тексеріп шығыңыз.',
'nosuchsectiontitle'               => 'Осындай еш бөлім жоқ',
'nosuchsectiontext'                => 'Жоқ бөлімді өңдеуді талап етіпсіз.
Мында $1 деген бөлім жоқ екен, өңдемеңізді сақтау үшін орын жоқ.',
'loginreqtitle'                    => 'Кіруіңіз керек',
'loginreqlink'                     => 'кіру',
'loginreqpagetext'                 => 'Басқа беттерді көру үшін сіз $1 болуыңыз жөн.',
'accmailtitle'                     => 'Құпия сөз жөнелтілді.',
'accmailtext'                      => '$2 жайына «$1» құпия сөзі жөнелтілді.',
'newarticle'                       => '(Жаңа)',
'newarticletext'                   => 'Сілтемеге еріп әлі басталмаған бетке келіпсіз.
Бетті бастау үшін, төмендегі кірістіру орнында мәтініңізді теріңіз (көбірек ақпарат үшін [[{{{{ns:mediawiki}}:helppage}}|анықтама бетін]] қараңыз).
Егер жаңылғаннан осында келген болсаңыз, шолғышыңыз «Артқа» деген батырмасын нұқыңыз.',
'anontalkpagetext'                 => "----''Бұл тіркелгісіз (немесе тіркелгісін қолданбаған) қатысушы талқылау беті. Осы қатысушыны біз тек сандық IP мекенжайымен теңдестіреміз.
Осындай IP мекенжай бірнеше қатысушыға ортақтастырылған болуы мүмкін.
Егер сіз тіркелгісіз қатысушы болсаңыз және сізге қатыссыз мәндемелер жіберілгенін сезсеңіз, басқа тіркелгісіз қатысушылармен араластырмауы үшін [[{{#special:Userlogin}}|тіркеліңіз не кіріңіз]].''",
'noarticletext'                    => 'Бұл бетте ағымда еш мәтін жоқ, дегенмен басқа беттерден [[{{#special:Search}}/{{PAGENAME}}|бұл бет тақырыбы атын іздей]] не [{{fullurl:{{FULLPAGENAME}}|action=edit}} бұл бетті өңдей] аласыз.',
'userpage-userdoesnotexist'        => '«$1» қатысушы тіркелгісі жазып алынбаған. Бұл бетті бастау/өңдеу талабыңызды тексеріп шығыңыз.',
'clearyourcache'                   => "'''Аңғартпа:''' Сақтағаннан кейін, өзгерістерді көру үшін шолғыш бүркемесін орағыту ықтимал. '''Mozilla / Firefox / Safari:''' ''Қайта жүктеу'' батырмасын нұқығанда ''Shift'' тұтыңыз, не ''Ctrl-Shift-R'' басыңыз (Apple Mac — ''Cmd-Shift-R''); '''IE:''' ''Жаңарту'' батырмасын нұқығанда ''Ctrl'' тұтыңыз, не ''Ctrl-F5'' басыңыз; '''Konqueror:''': ''Жаңарту'' батырмасын жай нұқыңыз, не ''F5'' басыңыз; '''Opera''' пайданушылары ''Құралдар→Бапталымдар'' дегенге барып бүркемесін толық тазарту жөн.",
'usercssjsyoucanpreview'           => '<strong>Ақыл-кеңес:</strong> Жаңа CSS/JS файлын сақтау алдында «Қарап шығу» батырмасын қолданып сынақтаңыз.',
'usercsspreview'                   => "'''Мынау CSS мәтінін тек қарап шығу екенін ұмытпаңыз, ол әлі сақталған жоқ!'''",
'userjspreview'                    => "'''Мынау JavaScript қатысушы бағдарламасын тексеру/қарап шығу екенін ұмытпаңыз, ол әлі сақталған жоқ!'''",
'userinvalidcssjstitle'            => "'''Құлақтандыру:''' Осы арада «$1» деген еш мәнер жоқ.
Қатысушының .css және .js файл атауы кіші әріпппен жазылу тиісті екенін ұмытпаңыз, мысалға {{ns:user}}:Foo/monobook.css дегенді {{ns:user}}:Foo/Monobook.css дегенмен салыстырып қараңыз.",
'updated'                          => '(Жаңартылған)',
'note'                             => '<strong>Аңғартпа:</strong>',
'previewnote'                      => '<strong>Мынау тек қарап шығу екенін ұмытпаңыз;
өзгерістер әлі сақталған жоқ!</strong>',
'previewconflict'                  => 'Бұл қарап шығу беті жоғарғы кірістіру орнындағы мәтінді қамтиды да және сақталғандағы өңді көрсетпек.',
'session_fail_preview'             => '<strong>Ғафу етіңіз! Сессия деректері жоғалуы салдарынан өңдемеңізді бітіре алмаймыз.
Қайта байқап көріңіз. Егер бұл әлі істелмесе, шығуды және қайта кіруді байқап көріңіз.</strong>',
'session_fail_preview_html'        => "<strong>Ғафу етіңіз! Сессия деректері жоғалуы салдарынан өңдемеңізді бітіре алмаймыз.</strong>

''{{SITENAME}} жобасында қам HTML қосылған, JavaScript шабуылдардан қорғану үшін алдын ала қарап шығу жасырылған.''

<strong>Егер бұл өңдеме адал талап болса, қайта байқап көріңіз. Егер бұл әлі істемесе, шығуды және қайта кіруді байқап көріңіз.</strong>",
'token_suffix_mismatch'            => '<strong>Өңдемеңіз тайдырылды, себебі тұтынғышыңыз өңдеме деректер бумасындағы тыныс белгілерін бүлдіртті.
Бет мәтіні бүлінбеу үшін өңдемеңіз тайдырылады.
Бұл кей уақытта қатесі толған веб-негізінде тіркелуі жоқ прокси-серверді пайдаланған болуы мүмкін.</strong>',
'editing'                          => 'Өңделуде: $1',
'editingsection'                   => 'Өңделуде: $1 (бөлімі)',
'editingcomment'                   => 'Өңделуде: $1 (мәндемесі)',
'editconflict'                     => 'Өңдеме қақтығысы: $1',
'explainconflict'                  => "Осы бетті сіз өңдей бастағанда басқа біреу бетті өзгерткен.
Жоғарғы кірістіру орнында беттің ағымдық мәтіні бар.
Төменгі кірістіру орнында сіз өзгерткен мәтіні көрсетіледі.
Өзгертуіңізді ағымдық мәтінге үстеуіңіз жөн.
«Бетті сақта! батырмасын басқанда '''тек''' жоғарғы кірістіру орнындағы мәтін сақталады.",
'yourtext'                         => 'Мәтініңіз',
'storedversion'                    => 'Сақталған нұсқасы',
'nonunicodebrowser'                => '<strong>ҚҰЛАҚТАНДЫРУ: Шолғышыңыз Unicode белгілеуіне үйлесімді емес, сондықтан латын емес әріптері бар беттерді өңдеу зіл болу мүмкін.
Жұмыс істеуге ықтималдық беру үшін, төмендегі кірістіру орнында ASCII емес таңбалар оналтылық кодымен көрсетіледі</strong>.',
'editingold'                       => '<strong>ҚҰЛАҚТАНДЫРУ: Осы беттің ертерек түзетуін өңдеп жатырсыз.
Бұны сақтасаңыз, осы түзетуден кейінгі барлық өзгерістер жойылады.</strong>',
'yourdiff'                         => 'Айырмалар',
'copyrightwarning'                 => 'Аңғартпа: {{SITENAME}} жобасына берілген барлық үлестер $2 (көбірек ақпарат үшін: $1) құжатына сай деп саналады.
Егер жазуыңыздың еркін өңделуін және ақысыз көпшілікке таратуын қаламасаңыз, мында жарияламауыңыз жөн.<br />
Тағы да, бұл мағлұмат өзіңіз жазғаныңызға, не қоғам қазынасынан немесе сондай ашық қорлардан көшірілгеніне бізге уәде бересіз.
<strong>АУТОРЛЫҚ ҚҰҚЫҚПЕН ҚОРҒАУЛЫ МАҒЛҰМАТТЫ РҰҚСАТСЫЗ ЖАРИЯЛАМАҢЫЗ!</strong>',
'copyrightwarning2'                => 'Аңғартпа: {{SITENAME}} жобасына берілген барлық үлестерді басқа үлескерлер өңдеуге, өзгертуге, не аластауға мүмкін.
Егер жазуыңыздың еркін өңделуін қаламасаңыз, мында жарияламауыңыз жөн.<br />
Тағы да, бұл мағлұмат өзіңіз жазғаныңызға, не қоғам қазынасынан немесе сондай ашық қорлардан көшірілгеніне бізге уәде бересіз (көбірек ақпарат үшін $1 қужатын қараңыз).
<strong>АУТОРЛЫҚ ҚҰҚЫҚПЕН ҚОРҒАУЛЫ МАҒЛҰМАТТЫ РҰҚСАТСЫЗ ЖАРИЯЛАМАҢЫЗ!</strong>',
'longpagewarning'                  => '<strong>ҚҰЛАҚТАНДЫРУ: Бұл беттің мөлшері — $1 KB;
кейбір шолғыштарда бет мөлшері 32 KB жетсе не оны асса өңдеу күрделі болуы мүмкін.
Бетті бірнеше кішкін бөлімдерге бөліп көріңіз.</strong>',
'longpageerror'                    => '<strong>ҚАТЕЛІК: Жөнелтпек мәтініңіздін мөлшері — $1 KB, ең көбі $2 KB рұқсат етілген мөлшерінен асқан.
Бұл сақтай алынбайды.</strong>',
'readonlywarning'                  => '<strong>ҚҰЛАҚТАНДЫРУ: Дерекқор баптау үшін құлыпталған, сондықтан дәл қазір өңдемеңізді сақтай алмайсыз.
Кейін қолдану үшін мәтәнді қйып алып және қойып, мәтін файлына сақтауңызға болады.</strong>',
'protectedpagewarning'             => '<strong>ҚҰЛАҚТАНДЫРУ: Бұл бет қорғалған. Тек әкімші құқықтары бар қатысушылар өңдей алады.</strong>',
'semiprotectedpagewarning'         => "'''Аңғартпа:''' Бет жартылай қорғалған, сондықтан осыны тек тіркелген қатысушылар өңдей алады.",
'cascadeprotectedwarning'          => "'''Құлақтандыру''': Бұл бет құлыпталған, енді тек әкімші құқықтары бар қатысушылар бұны өңдей алады.Бұның себебі: бұл бет «баулы қорғауы» бар келесі {{PLURAL:$1|беттің|беттердің}} кірікбеті:",
'titleprotectedwarning'            => '<strong>ҚҰЛАҚТАНДЫРУ:  Бұл бет құлыпталған, сондықтан тек бірқатар қатысушылар бұны бастай алады.</strong>',
'templatesused'                    => 'Бұл бетте қолданылған үлгілер:',
'templatesusedpreview'             => 'Бұны қарап шығуға қолданылған үлгілер:',
'templatesusedsection'             => 'Бұл бөлімде қолданылған үлгілер:',
'template-protected'               => '(қорғалған)',
'template-semiprotected'           => '(жартылай қорғалған)',
'hiddencategories'                 => 'Бұл бет $1 жасырын санаттың мүшесі:',
'nocreatetitle'                    => 'Бетті бастау шектелген',
'nocreatetext'                     => '{{SITENAME}} жобасында жаңа бет бастауы шектелген.
Кері қайтып бар бетті өңдеуіңізге болады, немесе [[{{#special:Userlogin}}|кіруіңізге не тіркелуіңізге]] болады.',
'nocreate-loggedin'                => '{{SITENAME}} жобасында жаңа бет бастау рұқсатыңыз жоқ.',
'permissionserrors'                => 'Рұқсаттар қателері',
'permissionserrorstext'            => 'Бұны істеуге рұқсатыңыз жоқ, келесі {{PLURAL:$1|себеп|себептер}} бойынша:',
'permissionserrorstext-withaction' => '$2 дегенге рұқсатыңыз жоқ, келесі {{PLURAL:$1|себеп|себептер}} бойынша:',
'recreate-deleted-warn'            => "'''Құлақтандыру: Алдында жойылған бетті қайта бастайын деп тұрсыз.'''

Мына бет өңдеуін жалғастыру үшін жарастығын тексеріп шығуыңыз жөн.
Қолайлы болуы үшін бұл беттің жою журналы келтірілген:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Құлақтандыру: Бұл бетте тым көп шығыс алатын құрылым талдатқыш жетелерінің қоңырау шалулары бар.

Бұл $2 шамасынан кем болуы жөн, қазір осы арада $1.',
'expensive-parserfunction-category'       => 'Шығыс алатын құрылым талдатқыш жетелерінің тым көп шақырымы бар беттер',
'post-expand-template-inclusion-warning'  => 'Құлақтандыру: Үлгі кірістіру мөлшері тым үлкен.
Кейбір үлгілер кірістірілмейді.',
'post-expand-template-inclusion-category' => 'Үлгі кірістірілген беттер мөлшері асып кетті',
'post-expand-template-argument-warning'   => 'Құлақтандыру: Бұл бетте тым көп ұлғайтылған мөлшері болған ең кемінде бір үлгі дәлелі бар.
Бұның дәлелдерін қалдырып кеткен.',
'post-expand-template-argument-category'  => 'Үлгі дәлелдерін қалдырып кеткен беттер',

# "Undo" feature
'undo-success' => 'Бұл өңдеме жоққа шығарылуы мүмкін. Талабыңызды құптап алдын ала төмендегі салыстыруды тексеріп шығыңыз да, өңдемені жоққа шығаруын бітіру үшін төмендегі өзгерістерді сақтаңыз.',
'undo-failure' => 'Бұл өңдеме жоққа шығарылмайды, себебі арада қақтығысты өңдемелер бар.',
'undo-norev'   => 'Бұл өңдеме жоққа шығарылмайды, себебі бұл жоқ немесе жойылған.',
'undo-summary' => '[[{{#special:Contributions}}/$2|$2]] ([[{{ns:user_talk}}:$2|талқылауы]]) істеген нөмір $1 түзетуін жоққа шығарды',

# Account creation failure
'cantcreateaccounttitle' => 'Жаңа тіркелгі жасалмады',
'cantcreateaccount-text' => "Бұл IP мекенжайдан ('''$1''') жаңа тіркелгі жасауын [[{{ns:user}}:$3|$3]] бұғаттаған.

$3 келтірілген себебі: ''$2''",

# History pages
'viewpagelogs'        => 'Бұл бет үшін журнал оқиғаларын қарау',
'nohistory'           => 'Мында бұл беттінің түзету тарихы жоқ.',
'revnotfound'         => 'Түзету табылмады',
'revnotfoundtext'     => 'Бұл беттің сұралған ескі түзетуі табылған жоқ. Осы бет қатынауына пайдаланған URL тексеріп шығыңыз.',
'currentrev'          => 'Ағымдық түзету',
'revisionasof'        => '$1 кезіндегі түзету',
'revision-info'       => '$1 кезіндегі $2 істеген түзету',
'previousrevision'    => '← Ескілеу түзетуі',
'nextrevision'        => 'Жаңалау түзетуі →',
'currentrevisionlink' => 'Ағымдық түзетуі',
'cur'                 => 'ағым.',
'next'                => 'кел.',
'last'                => 'соң.',
'page_first'          => 'алғашқысына',
'page_last'           => 'соңғысына',
'histlegend'          => 'Айырмасын бөлектеу: салыстырмақ нұсқаларының қосу көздерін белгілеп <Enter> пернесін басыңыз, немесе төмендегі батырманы нұқыңыз.<br />
Шартты белгілер: (ағым.) = ағымдық нұсқамен айырмасы,
(соң.) = алдыңғы нұсқамен айырмасы, ш = шағын өңдеме',
'deletedrev'          => '[жойылған]',
'histfirst'           => 'Ең алғашқысына',
'histlast'            => 'Ең соңғысына',
'historysize'         => '($1 байт)',
'historyempty'        => '(бос)',

# Revision feed
'history-feed-title'          => 'Түзету тарихы',
'history-feed-description'    => 'Мына уикидегі бұл беттің түзету тарихы',
'history-feed-item-nocomment' => '$2 кезіндегі $1 деген', # user at time
'history-feed-empty'          => 'Сұратылған бет жоқ болды.
Ол мына уикиден жойылған, немесе атауы ауыстырылған.
Осыған қатысты жаңа беттерді [[{{#special:Search}}|бұл уикиден іздеуді]] байқап көріңіз.',

# Revision deletion
'rev-deleted-comment'         => '(мәндеме аласталды)',
'rev-deleted-user'            => '(қатысушы аты аласталды)',
'rev-deleted-event'           => '(журнал жазбасы аласталды)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Бұл беттің түзетуі барша мұрағаттарынан аласталған.
Мында [{{fullurl:{{#special:Log}}/delete|page={{FULLPAGENAMEE}}}} жою журналында] егжей-тегжей мәліметтері болуы мүмкін.</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Осы беттің түзетуі барша мұрағаттарынан аласталған.
{{SITENAME}} әкімшісі боп соны көре аласыз;
[{{fullurl:{{#special:Log}}/delete|page={{FULLPAGENAMEE}}}} жою журналында] егжей-тегжей мәлметтері болуы мүмкін.</div>',
'rev-delundel'                => 'көрсет/жасыр',
'revisiondelete'              => 'Түзетулерді жою/жоюды болдырмау',
'revdelete-nooldid-title'     => 'Нысана түзету жарамсыз',
'revdelete-nooldid-text'      => 'Бұл жетені орындау үшін нысана түзетуін/түзетулерін келтірілмепсіз,
келтірілген түзету жоқ, не ағымдық түзетуді жасыру үшін әрекеттеніп көрдіңіз.',
'revdelete-selected'          => '[[:$1]] дегеннің бөлектенген {{PLURAL:$2|түзетуі|түзетулері}}:',
'logdelete-selected'          => 'Бөлектенген {{PLURAL:$1|журнал оқиғасы|журнал оқиғалары}}:',
'revdelete-text'              => 'Жойылған түзетулер мен оқиғаларды әлі де бет тарихында және журналдарда табуға болады, бірақ олардың мағлұмат бөлшектері баршаға қатыналмайды.

{{SITENAME}} жобасының басқа әкімшілері жасырын мағлұматқа қатынай алады, және қосымша тиымдар қойылғанша дейін, осы тілдесу арқылы жоюды болдырмауы мүмкін.',
'revdelete-legend'            => 'Көрініс тиымдарын қою:',
'revdelete-hide-text'         => 'Түзету мәтінін жасыр',
'revdelete-hide-name'         => 'Әрекет пен нысанасын жасыр',
'revdelete-hide-comment'      => 'Өңдеме мәндемесін жасыр',
'revdelete-hide-user'         => 'Өңдеуші атын (IP мекенжайын) жасыр',
'revdelete-hide-restricted'   => 'Осы тиымдарды әкімшілерге қолдану және бұл тілдесуді құлыптау',
'revdelete-suppress'          => 'Деректерді баршаға ұқсас әкімшілерден де шеттету',
'revdelete-hide-image'        => 'Файл мағлұматын жасыр',
'revdelete-unsuppress'        => 'Қалпына келтірілген түзетулерден тиымдарды аластау',
'revdelete-log'               => 'Журналдағы мәндемесі:',
'revdelete-submit'            => 'Бөлектенген түзетуге қолдану',
'revdelete-logentry'          => '[[$1]] дегеннің түзету көрінісін өзгертті',
'logdelete-logentry'          => '[[$1]] дегеннің оқиға көрінісін өзгертті',
'revdelete-success'           => "'''Түзету көрінісі сәтті қойылды.'''",
'logdelete-success'           => "'''Журнал көрінісі сәтті қойылды.'''",
'revdel-restore'              => 'Көрінісін өзгерту',
'pagehist'                    => 'Бет тарихы',
'deletedhist'                 => 'Жойылған тарихы',
'revdelete-content'           => 'мағлұмат',
'revdelete-summary'           => 'өңдеменің қысқаша мазмұндамасы',
'revdelete-uname'             => 'қатысушы аты',
'revdelete-restricted'        => 'әкімшілерге тиымдар қолдады',
'revdelete-unrestricted'      => 'әкімшілерден тиымдарды аластады',
'revdelete-hid'               => '$1 жасырды',
'revdelete-unhid'             => '$1 ашты',
'revdelete-log-message'       => '$2 түзету үшін $1',
'logdelete-log-message'       => '$2 оқиға үшін $1',

# Suppression log
'suppressionlog'     => 'Шеттету журналы',
'suppressionlogtext' => 'Төмендегі тізімде әкімшілерден жасырылған мағлұматқа қатысты жоюлар мен бұғаттаулар беріледі.
Ағымда әрекеттегі тиым мен бұғаттау тізімі үшін [[{{#special:Ipblocklist}}|IP бұғаттау тізімін]] қараңыз.',

# History merging
'mergehistory'                     => 'Беттер тарихын біріктіру',
'mergehistory-header'              => 'Бұл бет түзетулер тарихын қайнар беттің біреуінен алып жаңа бетке біріктіргізеді.
Осы өзгеріс беттің тарихи жалғастырушылығын қоштайтынына көзіңіз жетсін.',
'mergehistory-box'                 => 'Екі беттің түзетулерін біріктіру:',
'mergehistory-from'                => 'Қайнар беті:',
'mergehistory-into'                => 'Нысана беті:',
'mergehistory-list'                => 'Біріктірлетін түзету тарихы',
'mergehistory-merge'               => '[[:$1]] дегеннің келесі түзетулері [[:$2]] дегенге біріктірілуі мүмкін.
Біріктіруге тек енгізілген уақытқа дейін жасалған түзетулерді айырып-қосқыш бағанды қолданыңыз.
Аңғартпа: бағыттау сілтемелерін қолданғанда бұл баған қайта қойылады.',
'mergehistory-go'                  => 'Біріктірлетін түзетулерді көрсет',
'mergehistory-submit'              => 'Түзетулерді біріктіру',
'mergehistory-empty'               => 'Еш түзетулер біріктірілмейді',
'mergehistory-success'             => '[[:$1]] дегеннің $3 түзетуі [[:$2]] дегенге сәтті біріктірілді.',
'mergehistory-fail'                => 'Тарих біріктіруін орындау икемді емес, бет пен уақыт бапталымдарын қайта тексеріп шығыңыз.',
'mergehistory-no-source'           => '$1 деген қайнар беті жоқ.',
'mergehistory-no-destination'      => '$1 деген нысана беті жоқ.',
'mergehistory-invalid-source'      => 'Қайнар бетінде жарамды тақырып аты болуы жөн.',
'mergehistory-invalid-destination' => 'Нысана бетінде жарамды тақырып аты болуы жөн.',
'mergehistory-autocomment'         => '[[:$1]] деген [[:$2]] дегенге біріктірілді',
'mergehistory-comment'             => '[[:$1]] деген [[:$2]] дегенге біріктірілді: $3',

# Merge log
'mergelog'           => 'Біріктіру журналы',
'pagemerge-logentry' => '[[$1]] деген [[$2]] дегенге біріктірілді ($3 дейінгі түзетулері)',
'revertmerge'        => 'Біріктіруді болдырмау',
'mergelogpagetext'   => 'Төменде бір беттің тарихы өзге бетке біріктіру ең соңғы тізімі келтіріледі.',

# Diffs
'history-title'           => '«$1» — түзету тарихы',
'difference'              => '(Түзетулер арасындағы айырмашылық)',
'lineno'                  => 'Жол нөмірі $1:',
'compareselectedversions' => 'Бөлектенген нұсқаларды салыстыру',
'editundo'                => 'жоққа шығару',
'diff-multi'              => '(Арадағы $1 түзету көрсетілмеген.)',

# Search results
'searchresults'             => 'Іздеу нәтижелері',
'searchresulttext'          => '{{SITENAME}} жобасында іздеу туралы көбірек ақпарат үшін, [[{{{{ns:mediawiki}}:helppage}}|{{int:help}} бетін]] қараңыз.',
'searchsubtitle'            => "Іздегеніңіз: '''[[:$1]]'''",
'searchsubtitleinvalid'     => "Іздегеніңіз: '''$1'''",
'noexactmatch'              => "'''Осы арада беттің «$1» тақырып аты жоқ.'''
[[:$1|Бұл бетті бастай]] аласыз.",
'noexactmatch-nocreate'     => "'''Осы арада беттің «$1» тақырып аты жоқ.'''",
'toomanymatches'            => 'Тым көп сәйкес қайтарылды, өзге сұранымды байқап көріңіз',
'titlematches'              => 'Бет тақырыбын аты сәйкес келеді',
'notitlematches'            => 'Еш бет тақырыбын аты сәйкес емес',
'textmatches'               => 'Бет мәтіні сәйкес келеді',
'notextmatches'             => 'Еш бет мәтіні сәйкес емес',
'prevn'                     => 'алдыңғы $1',
'nextn'                     => 'келесі $1',
'viewprevnext'              => 'Көрсетілуі: ($1) ($2) ($3) жазба',
'search-result-size'        => '$1 ($2 сөз)',
'search-result-score'       => 'Арақатынастылығы: $1 %',
'search-redirect'           => '(айдағыш $1)',
'search-section'            => '(бөлім $1)',
'search-suggest'            => 'Бұны іздедіңіз бе: $1',
'search-interwiki-caption'  => 'Бауырлас жобалар',
'search-interwiki-default'  => '$1 нәтиже:',
'search-interwiki-more'     => '(көбірек)',
'search-mwsuggest-enabled'  => 'ұсынымдармен',
'search-mwsuggest-disabled' => 'ұсынымдарсыз',
'search-relatedarticle'     => 'Қатысты',
'mwsuggest-disable'         => 'AJAX ұсынымдарын өшір',
'searchrelated'             => 'қатысты',
'searchall'                 => 'барлық',
'showingresults'            => "Төменде нөмір '''$2''' орнынан бастап барынша '''$1''' нәтиже көрсетіледі.",
'showingresultsnum'         => "Төменде нөмір '''$2''' орнынан бастап '''$3''' нәтиже көрсетіледі.",
'showingresultstotal'       => "Төменде {{PLURAL:$3|'''$3''' арасынан '''$1''' нәтиже көрсетіледі|'''$3''' арасынан '''$1 — $2''' нәтиже ауқымы көрсетіледі}}",
'nonefound'                 => "'''Аңғартпа''': Әдепкіден тек кейбір есім аялардан ізделінеді. Барлық мағлұмат түрін (соның ішінде талқылау беттерді, үлгілерді т.б.) іздеу үшін сұранымыңызды ''барлық:'' деп бастаңыз, немесе қалаған есім аясын бастауыш есебінде қолданыңыз.",
'powersearch'               => 'Кеңейтілген іздеу',
'powersearch-legend'        => 'Кеңейтілген іздеу',
'powersearch-ns'            => 'Мына есім аяларда іздеу:',
'powersearch-redir'         => 'Айдатуларды тізімдеу',
'powersearch-field'         => 'Мынаны іздемек:',
'search-external'           => 'Шеттік іздегіш',
'searchdisabled'            => '{{SITENAME}} іздеу қызметі өшірілген.
Әзірше Google арқылы іздеуге болады.
Аңғартпа: {{SITENAME}} торабының мағлұмат тізбелері ескірген болуы мүмкін.',

# Preferences page
'preferences'              => 'Бапталымдар',
'mypreferences'            => 'Бапталымдарым',
'prefs-edits'              => 'Өңдеме саны:',
'prefsnologin'             => 'Кірмегенсіз',
'prefsnologintext'         => 'Пайдаланушылық бапталымдарыңызды қою үшін [[{{#special:Userlogin}}|кіруіңіз]] жөн.',
'prefsreset'               => 'Бапталымдар арқаудан қайта қойылды.',
'qbsettings'               => 'Мәзір',
'qbsettings-none'          => 'Ешқандай',
'qbsettings-fixedleft'     => 'Солға бекітілген',
'qbsettings-fixedright'    => 'Оңға бекітілген',
'qbsettings-floatingleft'  => 'Солға қалқыған',
'qbsettings-floatingright' => 'Оңға қалқыған',
'changepassword'           => 'Құпия сөзді өзгерту',
'skin'                     => 'Мәнерлер',
'math'                     => 'Өрнектер',
'dateformat'               => 'Күн-ай пішімі',
'datedefault'              => 'Еш қалаусыз',
'datetime'                 => 'Уақыт',
'math_failure'             => 'Құрылымын талдатуы сәтсіз бітті',
'math_unknown_error'       => 'белгісіз қате',
'math_unknown_function'    => 'белгісіз жете',
'math_lexing_error'        => 'сөз кенінің қатесі',
'math_syntax_error'        => 'сөйлем жүйесінің қатесі',
'math_image_error'         => 'PNG аударысы сәтсіз бітті;
latex, dvips, gs және convert бағдарламаларының дұрыс орнатуын тексеріп шығыңыз',
'math_bad_tmpdir'          => 'math деген уақытша қалтасына жазылмады, не қалта құрылмады',
'math_bad_output'          => 'math деген беріс қалтасына жазылмады, не қалта құрылмады',
'math_notexvc'             => 'texvc атқарылмалысы табылмады;
баптау үшін math/README құжатын қараңыз.',
'prefs-personal'           => 'Жеке деректері',
'prefs-rc'                 => 'Жуықтағы өзгерістер',
'prefs-watchlist'          => 'Бақылау',
'prefs-watchlist-days'     => 'Бақылау тізіміндегі күндердің көрсетпек саны:',
'prefs-watchlist-edits'    => 'Кеңейтілген бақылаулардағы өзгерістердің барынша көрсетпек саны:',
'prefs-misc'               => 'Әрқилы',
'saveprefs'                => 'Сақта',
'resetprefs'               => 'Сақталмаған өзгерістерді тазарт',
'oldpassword'              => 'Ағымдық құпия сөзіңіз:',
'newpassword'              => 'Жаңа құпия сөзіңіз:',
'retypenew'                => 'Жаңа құпия сөзіңізді қайталаңыз:',
'textboxsize'              => 'Өңдеу',
'rows'                     => 'Жолдар:',
'columns'                  => 'Бағандар:',
'searchresultshead'        => 'Іздеу',
'resultsperpage'           => 'Бет сайын нәтиже саны:',
'contextlines'             => 'Нәтиже сайын жол саны:',
'contextchars'             => 'Жол сайын таңба саны:',
'stub-threshold'           => '<a href="#" class="stub">Бітеме сілтемесін</a> пішімдеу табалдырығы (байт):',
'recentchangesdays'        => 'Жүықтағы өзгерістерінде көрсетпек күн саны:',
'recentchangescount'       => 'Жуықтағы өзгерістердінде, тарих және журнал беттерінде көрсетпек өңдеме саны:',
'savedprefs'               => 'Бапталымдарыңыз сақталды.',
'timezonelegend'           => 'Уақыт белдеуі',
'timezonetext'             => '¹ Жергілікті уақытыңыз бен сервер уақытының (UTC) арасындағы сағат саны.',
'localtime'                => 'Жергілікті уақыт',
'timezoneoffset'           => 'Сағат ығысуы¹',
'servertime'               => 'Сервер уақыты',
'guesstimezone'            => 'Шолғыштан алып толтыру',
'allowemail'               => 'Басқадан хат қабылдауын қос',
'prefs-searchoptions'      => 'Іздеу бапталымдары',
'prefs-namespaces'         => 'Есім аялары',
'defaultns'                => 'Мына есім аяларда әдепкіден іздеу:',
'default'                  => 'әдепкі',
'files'                    => 'Файлдар',

# User rights
'userrights'                  => 'Қатысушы құқықтарын реттеу', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Қатысушы топтарын реттеу',
'userrights-user-editname'    => 'Қатысушы атын енгізіңіз:',
'editusergroup'               => 'Қатысушы топтарын өңдеу',
'editinguser'                 => "Қатысушы құқықтарын өзгерту: '''[[{{ns:user}}:$1|$1]]''' ([[{{ns:user_talk}}:$1|{{int:talkpagelinktext}}]] | [[{{#special:Contributions}}/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Қатысушы топтарын өңдеу',
'saveusergroups'              => 'Қатысушы топтарын сақтау',
'userrights-groupsmember'     => 'Мүшелігі:',
'userrights-groups-help'      => 'Бұл қатысушы кіретін топтарды реттей аласыз.
* Құсбелгі қойылған көзі қатысушы бұл топқа кіргенін көрсетеді;
* Құсбелгі алып тасталған көз қатысушы бұл топқа кірмегенін көрсетеді;
* Келтірілген * топты бір үстегенінен кейін аластай алмайтындығын, не қарама-қарсысын көрсетеді.',
'userrights-reason'           => 'Өзгерту себебі:',
'userrights-no-interwiki'     => 'Басқа уикилердегі пайдаланушы құқықтарын өңдеуге рұқсатыңыз жоқ.',
'userrights-nodatabase'       => '$1 дерекқоры жоқ не жергілікті емес.',
'userrights-nologin'          => 'Қатысушы құқықтарын тағайындау үшін әкімші тіркелгісімен [[{{#special:Userlogin}}|кіруіңіз]] жөн.',
'userrights-notallowed'       => 'Қатысушы құқықтарын тағайындау үшін тіркелгіңізде рұқсат жоқ.',
'userrights-changeable-col'   => 'Өзгерте алатын топтар',
'userrights-unchangeable-col' => 'Өзгерте алмайтын топтар',

# Groups
'group'               => 'Топ:',
'group-user'          => 'Қатысушылар',
'group-autoconfirmed' => 'Өзқұпталған қатысушылар',
'group-bot'           => 'Боттар',
'group-sysop'         => 'Әкімшілер',
'group-bureaucrat'    => 'Бітікшілер',
'group-suppress'      => 'Шеттетушілер',
'group-all'           => '(барлық)',

'group-user-member'          => 'қатысушы',
'group-autoconfirmed-member' => 'өзқұпталған қатысушы',
'group-bot-member'           => 'бот',
'group-sysop-member'         => 'әкімші',
'group-bureaucrat-member'    => 'бітікші',
'group-suppress-member'      => 'шеттетуші',

'grouppage-user'          => '{{ns:project}}:Қатысушылар',
'grouppage-autoconfirmed' => '{{ns:project}}:Өзқұпталған қатысушылар',
'grouppage-bot'           => '{{ns:project}}:Боттар',
'grouppage-sysop'         => '{{ns:project}}:Әкімшілер',
'grouppage-bureaucrat'    => '{{ns:project}}:Бітікшілер',
'grouppage-suppress'      => '{{ns:project}}:Шеттетушілер',

# Rights
'right-read'                 => 'Беттерді оқу',
'right-edit'                 => 'Беттерді өңдеу',
'right-createpage'           => 'Талқылау емес беттерді бастау',
'right-createtalk'           => 'Талқылау беттерді бастау',
'right-createaccount'        => 'Жаңа қатысушы тіркелгісін жасау',
'right-minoredit'            => 'Өңдемелерді шағын деп белгілеу',
'right-move'                 => 'Беттерді жылжыту',
'right-move-subpages'        => 'Беттерді бұлардың бағынышты беттерімен жылжыту',
'right-suppressredirect'     => 'Тиісті атауға бетті жылжытқанда айдағышты жасамау',
'right-upload'               => 'Файлдарды қотарып беру',
'right-reupload'             => 'Бар файл үстіне жазу',
'right-reupload-own'         => 'Өзі қотарып берген файл үстіне жазу',
'right-reupload-shared'      => 'Таспа ортақ қоймасындағы файлдарды жергіліктілермен асыру',
'right-upload_by_url'        => 'Файлды URL мекенжайынан қотарып беру',
'right-purge'                => 'Бетті торап бүркемесінен құптаусыз тазарту',
'right-autoconfirmed'        => 'Жартылай қорғалған беттерді өңдеу',
'right-bot'                  => 'Өздіктік үдеріс деп есептелу',
'right-nominornewtalk'       => 'Талқылау беттердегі шағын өңдемелерді жаңа хабар деп есептемеу',
'right-apihighlimits'        => 'API сұранымдарының жоғары шектелімдерін пайдалану',
'right-writeapi'             => 'API жазуын пайдалану',
'right-delete'               => 'Беттерді жою',
'right-bigdelete'            => 'Ұзақ тарихы бар беттерді жою',
'right-deleterevision'       => 'Беттердің өзіндік түзетулерін жою не жоюын болдырмау',
'right-deletedhistory'       => 'Жойылған тарих даналарын (байланысты мәтінсіз) көру',
'right-browsearchive'        => 'Жойылған беттерді іздеу',
'right-undelete'             => 'Беттің жюын болдырмау',
'right-suppressrevision'     => 'Әкімшілерден жасырылған түзетулерді шолып шығу және қалпына келтіру',
'right-suppressionlog'       => 'Жекелік журналдарды көру',
'right-block'                => 'Басқа қатысушыларды өңдеуден бұғаттау',
'right-blockemail'           => 'Қатысушының хат жөнелтуін бұғаттау',
'right-hideuser'             => 'Баршадан жасырып, қатысушы атын бұғаттау',
'right-ipblock-exempt'       => 'IP бұғаттауларды, өзбұғаттауларды және ауқым бұғаттауларды орағыту',
'right-proxyunbannable'      => 'Прокси серверлердің өзбұғаттауларын орағыту',
'right-protect'              => 'Қорғау деңгейлерін өзгерту және қорғалған беттерді өңдеу',
'right-editprotected'        => 'Қорғалған беттерді өңдеу (баулы қорғауларсыз)',
'right-editinterface'        => 'Пайдаланушылық тілдесіуін өңдеу',
'right-editusercssjs'        => 'Басқа қатысушылардың CSS және JS файлдарын өңдеу',
'right-rollback'             => 'Белгілі бетті өңдеген соңғы қатысушының өңдемелерінен жылдам шегіндіру',
'right-markbotedits'         => 'Шегіндірлген өңдемелерді боттардікі деп белгілеу',
'right-noratelimit'          => 'Еселік шектелімдері ықпал етпейді',
'right-import'               => 'Басқа уикилерден беттерді сырттан алу',
'right-importupload'         => 'Файл қотарып беруімен беттерді сырттан алу',
'right-patrol'               => 'Басқарардың өңдемелерін зерттелді деп белгілеу',
'right-autopatrol'           => 'Өз өңдемелерін зерттелді деп өздіктік белгілеу',
'right-patrolmarks'          => 'Жуықтағы өзгерістердегі зерттеу белгілерін көру',
'right-unwatchedpages'       => 'Бақыланылмаған бет тізімін көру',
'right-trackback'            => 'Аңыстауды жөнелту',
'right-mergehistory'         => 'Беттердің тарихын қосып беру',
'right-userrights'           => 'Қатысушылардың барлық құқықтарын өңдеу',
'right-userrights-interwiki' => 'Басқа үикилердегі қатысушылардың құқықтарын өңдеу',
'right-siteadmin'            => 'Дерекқорды құлыптау және құлыптауын өшіру',

# User rights log
'rightslog'      => 'Қатысушы құқықтары журналы',
'rightslogtext'  => 'Бұл қатысушы құқықтарын өзгерту журналы.',
'rightslogentry' => '$1 кірген топтарын $2 дегеннен $3 дегенге өзгертті',
'rightsnone'     => '(ешқандай)',

# Recent changes
'nchanges'                          => '$1 өзгеріс',
'recentchanges'                     => 'Жуықтағы өзгерістер',
'recentchangestext'                 => 'Бұл бетте осы уикидегі болған жуықтағы өзгерістер байқалады.',
'recentchanges-feed-description'    => 'Бұл арнаменен уикидегі ең соңғы өзгерістер қадағаланады.',
'rcnote'                            => "$3 кезіне дейін — төменде соңғы {{PLURAL:$2|күндегі|'''$2''' күндегі}}, соңғы '''$1''' өзгеріс көрсетіледі.",
'rcnotefrom'                        => "'''$2''' кезінен бері — төменде '''$1''' жеткенше дейін өзгерістер көрсетіледі.",
'rclistfrom'                        => '$1 кезінен бері — жаңа өзгерістерді көрсет.',
'rcshowhideminor'                   => 'Шағын өңдемелерді $1',
'rcshowhidebots'                    => 'Боттарды $1',
'rcshowhideliu'                     => 'Кіргендерді $1',
'rcshowhideanons'                   => 'Тіркелгісіздерді $1',
'rcshowhidepatr'                    => 'Зерттелген өңдемелерді $1',
'rcshowhidemine'                    => 'Өңдемелерімді $1',
'rclinks'                           => 'Соңғы $2 күнде болған, соңғы $1 өзгерісті көрсет<br />$3',
'diff'                              => 'айырм.',
'hist'                              => 'тар.',
'hide'                              => 'жасыр',
'show'                              => 'көрсет',
'minoreditletter'                   => 'ш',
'newpageletter'                     => 'Ж',
'boteditletter'                     => 'б',
'number_of_watching_users_pageview' => '[бақылаған $1 қатысушы]',
'rc_categories'                     => 'Санаттарға шектеу ("|" белгісімен бөліктеңіз)',
'rc_categories_any'                 => 'Қайсыбір',
'newsectionsummary'                 => '/* $1 */ жаңа бөлім',

# Recent changes linked
'recentchangeslinked'          => 'Қатысты өзгерістер',
'recentchangeslinked-title'    => '«$1» дегенге қатысты өзгерістер',
'recentchangeslinked-noresult' => 'Сілтеген беттерде келтірілген мерзімде ешқандай өзгеріс болмаған.',
'recentchangeslinked-summary'  => "Бұл тізімде өзіндік бетке сілтеген беттердегі (не өзіндік санат мүшелеріндегі) істелген жуықтағы өзгерістер беріледі.
[[{{#special:Watchlist}}|Бақылау тізіміңіздегі]] беттер '''жуан''' болып белгіленеді.",
'recentchangeslinked-page'     => 'Бет атауы:',
'recentchangeslinked-to'       => 'Керісінше, келтірілген бетке сілтейтін беттердегі өзгерістерді көрсет',

# Upload
'upload'                      => 'Қотарып беру',
'uploadbtn'                   => 'Қотарып бер!',
'reupload'                    => 'Қайта қотарып беру',
'reuploaddesc'                => 'Қотарып беруді болдырмау және қотару пішініне қайта келу.',
'uploadnologin'               => 'Кірмегенсіз',
'uploadnologintext'           => 'Файлдарды қотарып беру үшін [[{{#special:Userlogin}}|кіруіңіз]] жөн.',
'upload_directory_missing'    => 'Қотарып бермек қалтасы ($1) жетіспейді және веб-сервер жарата алмайды.',
'upload_directory_read_only'  => 'Қотарып бермек қалтасына ($1) веб-сервер жаза алмайды.',
'uploaderror'                 => 'Қотарып беру қатесі',
'uploadtext'                  => "Төмендегі пішінді файлдарды қотарып беру үшін қолданыңыз. 
Алдында қотарылып берілген файлдарды қарау не іздеу үшін [[{{#special:Imagelist}}|қотарып берілген файлдар тізіміне]] барыңыз, тағы да қотарып беруі мен жоюы  [[{{#special:Log}}/upload|қотарып беру журналына]] жазылып алынады.

Суретті бетке кірістіруге, файлға тура сілтеу үшін мына пішіндегі сілтемені қолданыңыз:
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|балама мәтін]]</nowiki>''' не
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki>'''.",
'upload-permitted'            => 'Рұқсат етілген файл түрлері: $1.',
'upload-preferred'            => 'Ұнамды файл түрлері $1.',
'upload-prohibited'           => 'Рұқсат етілмеген файл түрлері: $1.',
'uploadlog'                   => 'қотарып беру журналы',
'uploadlogpage'               => 'Қотарып беру журналы',
'uploadlogpagetext'           => 'Төменде ең соңғы қотарып берілген файл тізімі.',
'filename'                    => 'Файл атауы',
'filedesc'                    => 'Түйіндемесі',
'fileuploadsummary'           => 'Түйіндемесі:',
'filestatus'                  => 'Ауторлық құқықтар күйі:',
'filesource'                  => 'Қайнар көзі:',
'uploadedfiles'               => 'Қотарып берілген файлдар',
'ignorewarning'               => 'Құлақтандыруға елеме де файлды қалайда сақта.',
'ignorewarnings'              => 'Кез келген құлақтандыруларға елеме',
'minlength1'                  => 'Файл атауында ең кемінде бір әріп болуы жөн.',
'illegalfilename'             => '«$1» файл атауында бет тақырыбы атында рұқсат берілмеген таңбалар бар.
Файлды қайта атаңыз да бұны қотарып беруді қайта байқап көріңіз.',
'badfilename'                 => 'Файлдың атауы «$1» деп өзгертілді.',
'filetype-badmime'            => '«$1» деген MIME түрі бар файлдарды қотарып беруге рұқсат етілмейді.',
'filetype-unwanted-type'      => "'''«.$1»''' — күтілмеген файл түрі. Ұнамды файл түрлері: $2.",
'filetype-banned-type'        => "'''«.$1»''' — рұқсатталмаған файл түрі. Рұқсатталған файл түрлері: $2.",
'filetype-missing'            => 'Бұл файлдың («.jpg» сияқты) кеңейтімі жоқ.',
'large-file'                  => 'Файлдың $1 мөлшерінен аспауына кепілдеме беріледі;
бұл файл мөлшері — $2.',
'largefileserver'             => 'Осы файлдың мөлшері сервердің қалауынан асып кеткен.',
'emptyfile'                   => 'Қотарып берілген файлыңыз бос сияқты. Файл атауы қате жазылған мүмкін.
Бұл файлды қотарып беруі нақты талабыңыз екенін тексеріп шығыңыз.',
'fileexists'                  => 'Былай аталған файл алдақашан бар, егер бұны өзгертуге батылыңыз жоқ болса <strong><tt>$1</tt></strong> дегенді тексеріп шығыңыз.',
'filepageexists'              => 'Бұл файлдың сипаттама беті алдақашан <strong><tt>$1</tt></strong> дегенде жасалған, бірақ ағымда былай аталған еш файл жоқ.
Енгізген қысқаша мазмұндамаңыз сипаттамасы бетінде көрсетілмейді.
Қысқаша мазмұндамаңыз осы арада көрсетілу үшін, бұны қолмен өңдемек болыңыз',
'fileexists-extension'        => 'Ұқсас атауы бар файл табылды:<br />
Қотарып берілетін файл атауы: <strong><tt>$1</tt></strong><br />
Бар болған файл атауы: <strong><tt>$2</tt></strong><br />
Өзге атауды таңдаңыз.',
'fileexists-thumb'            => "<center>'''Бар болған сурет'''</center>",
'fileexists-thumbnail-yes'    => 'Осы файл — мөлшері кішірітілген сурет <i>(нобай)</i> сияқты.
Бұл <strong><tt>$1</tt></strong> деген файлды сынап шығыңыз.<br />
Егер сыналған файл түпнұсқалы мөлшері бар дәлме-дәл сурет болса, қосысмша нобайды қотарып беру керегі жоқ.',
'file-thumbnail-no'           => 'Файл атауы <strong><tt>$1</tt></strong> дегенмен басталады.
Бұл — мөлшері кішірітілген сурет <i>(нобай)</i> сияқты.
Егер бұл суреттің толық ажыратылымдығы болса, бұны қотарып беріңіз, әйтпесе файл атауын өзгертіңіз.',
'fileexists-forbidden'        => 'Осылай аталған файл алдақашан бар;
кері қайтыңыз да, осы файлды жаңа атымен қотарып беріңіз. [[{{ns:image}}:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Осылай аталған файл ортақ қоймада алдақашан бар;
кері қайтыңыз да, осы файлды жаңа атымен қотарып беріңіз. [[{{ns:image}}:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Бұл файл келесі {{PLURAL:$1|файлдың|файлдарының}} телнұсқасы:',
'successfulupload'            => 'Сәтті қотарып берілді',
'uploadwarning'               => 'Қотарып беру жөнінде құлақтандыру',
'savefile'                    => 'Файлды сақтау',
'uploadedimage'               => '«[[$1]]» файлын қотарып берді',
'overwroteimage'              => '«[[$1]]» файлынның жаңа нұсқасын қотарып берді',
'uploaddisabled'              => 'Қотарып беру өшірілген',
'uploaddisabledtext'          => '{{SITENAME}} жобасында файл қотарып беруі өшірілген.',
'uploadscripted'              => 'Бұл файлда веб шолғышты қателікпен талдатқызатын HTML не әмір коды бар.',
'uploadcorrupt'               => 'Бұл файл бүлдірілген, не бұрыс кеңейтімі бар.
Файлды тексеріп шығыңыз да, қайта қотарып беріңіз.',
'uploadvirus'                 => 'Бұл файлда вирус бар! Егжей-тегжейлері: $1',
'sourcefilename'              => 'Қайнар файл атауы:',
'destfilename'                => 'Нысана файл атауы:',
'upload-maxfilesize'          => 'Файлдың ең көп мүмкін мөлшері: $1',
'watchthisupload'             => 'Бұл бетті бақылау',
'filewasdeleted'              => 'Бұл атауы бар файл бұрын қотарып берілген де бері келе жойылған.
Бұны қайта қотарып беру алдынан $1 дегенді тексеріп шығыңыз.',
'upload-wasdeleted'           => "'''Құлақтандыру: Алдында жойылған файлды қотарып бермексіз.'''

Бұл файлды қотарып беруін жалғастыру үшін бұның ыңғайлығын тексеріп шығуыңыз жөн.
Қолайлы болуы үшін бұл файлдың жою журналы келтірілген:",
'filename-bad-prefix'         => 'Қотарып бермек файлыңыздың атауы <strong>«$1» </strong> деп басталады, мынадай сипаттаусыз атауды әдетте сандық камералар өздіктік береді.
Файлыңызға сипаттылау атауды таңдаңыз.',

'upload-proto-error'      => 'Бұрыс хаттама',
'upload-proto-error-text' => 'Шеттен қотарып беру үшін URL жайлары <code>http://</code> немесе <code>ftp://</code> дегендерден басталу жөн.',
'upload-file-error'       => 'Ішкі қате',
'upload-file-error-text'  => 'Серверде уақытша файл құрылуы ішкі қатесіне ұшырасты.
Бұл жүйенің әкімшімен қатынасыңыз.',
'upload-misc-error'       => 'Қотарып беру кезіндегі белгісіз қате',
'upload-misc-error-text'  => 'Қотарып беру кезінде белгісіз қатеге ұшырасты.
URL жарамды және қатынаулы екенін тексеріп шығыңыз да қайта байқап көріңіз.
Егер бұл мәселе әлде де қалса, жүйе әкімшімен қатынасыңыз.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL жетілмеді',
'upload-curl-error6-text'  => 'Келтірілген URL жетілмеді.
URL дұрыс екендігін және торап істеп тұрғанын қос тексеріңіз.',
'upload-curl-error28'      => 'Қотарып беру уақыты бітті',
'upload-curl-error28-text' => 'Тораптың жауап беруі тым ұзақ уақытқа созылды.
Бұл торап істе екенін тексеріп шығыңыз, азғана кідіре тұрыңыз да қайта байқап көріңіз.
Талабыңызды қол тиген кезінде қайта байқап көруіңіз мүмкін.',

'license'            => 'Лицензияландыруы:',
'nolicense'          => 'Ештеңе бөлектенбеген',
'license-nopreview'  => '(Қарап шығу жетімді емес)',
'upload_source_url'  => ' (жарамды, баршаға қатынаулы URL)',
'upload_source_file' => ' (компьютеріңіздегі файл)',

# Special:ImageList
'imagelist-summary'     => 'Бұл арнайы бетте барлық қотарып берілген файлдар көрсетіледі.
Соңғы қотарып берілген файлдар тізімде жоғарғы шетімен әдепкіден көрсетіледі.
Бағанның бас жолын нұқығанда сұрыптаудың реттеуі өзгертіледі.',
'imagelist_search_for'  => 'Таспа атауын іздеу:',
'imgfile'               => 'файл',
'imagelist'             => 'Файл тізімі',
'imagelist_date'        => 'Күн-айы',
'imagelist_name'        => 'Атауы',
'imagelist_user'        => 'Қатысушы',
'imagelist_size'        => 'Мөлшері',
'imagelist_description' => 'Сипаттамасы',

# Image description page
'filehist'                       => 'Файл тарихы',
'filehist-help'                  => 'Файлдың қай уақытта қалай көрінетін үшін Күн-ай/Уақыт дегенді нұқыңыз.',
'filehist-deleteall'             => 'барлығын жой',
'filehist-deleteone'             => 'жой',
'filehist-revert'                => 'қайтар',
'filehist-current'               => 'ағымдағы',
'filehist-datetime'              => 'Күн-ай/Уақыт',
'filehist-user'                  => 'Қатысушы',
'filehist-dimensions'            => 'Өлшемдері',
'filehist-filesize'              => 'Файл мөлшері',
'filehist-comment'               => 'Мәндемесі',
'imagelinks'                     => 'Сілтемелер',
'linkstoimage'                   => 'Бұл файлға келесі {{PLURAL:$1|бет|$1 бет}} сілтейді:',
'nolinkstoimage'                 => 'Бұл файлға еш бет сілтемейді.',
'morelinkstoimage'               => 'Бұл файлдың [[{{#special:Whatlinkshere}}/$1|көбірек сілтемелерін]] қарау.',
'redirectstofile'                => 'Келесі {{PLURAL:$1|файл|$1 файл}} бұл файлға айдайды:',
'duplicatesoffile'               => 'Келесі {{PLURAL:$1|файл бұл файлдың телнұсқасы|$1 файл бұл файлдың телнұсқалары}}:',
'sharedupload'                   => 'Бұл файл ортақ қоймаға қотарып берілген сондықтан басқа жобаларда қолдануы мүмкін.',
'shareduploadwiki'               => 'Былайғы ақпарат үшін $1 дегенді қараңыз.',
'shareduploadwiki-desc'          => 'Бұл $1 деген файлдың ортақ қоймадағы мәліметтері төменде көрсетіледі.',
'shareduploadwiki-linktext'      => 'файлдың сипаттама беті',
'shareduploadduplicate'          => 'Бұл файл ортақ қоймадағы $1 файлының телнұсқасы.',
'shareduploadduplicate-linktext' => 'өзге файл',
'shareduploadconflict'           => 'Бұл файл атауы ортақ қоймадағы $1 файлымен дәл келеді',
'shareduploadconflict-linktext'  => 'өзге файл',
'noimage'                        => 'Былай аталған файл жоқ, $1 мүмкіндігіңіз бар.',
'noimage-linktext'               => 'бұны қотарып бер',
'uploadnewversion-linktext'      => 'Бұл файлдың жаңа нұсқасын қотарып беру',
'imagepage-searchdupe'           => 'Файл телнұсқаларын іздеу',

# File reversion
'filerevert'                => '$1 дегенді қайтару',
'filerevert-legend'         => 'Файлды қайтару',
'filerevert-intro'          => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' дегенді [$4 $3, $2 кезіндегі нұсқасына] қайтарудасыз.</span>',
'filerevert-comment'        => 'Мәндемесі:',
'filerevert-defaultcomment' => '$2, $1 кезіндегі нұсқасына қайтарылды',
'filerevert-submit'         => 'Қайтар',
'filerevert-success'        => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' деген [$4 $3, $2 кезіндегі нұсқасына] қайтарылды.</span>',
'filerevert-badversion'     => 'Келтірілген уақыт белгісімен бұл файлдың алдыңғы жергілікті нұсқасы жоқ.',

# File deletion
'filedelete'                  => '$1 дегенді жою',
'filedelete-legend'           => 'Файлды жою',
'filedelete-intro'            => "'''[[{{ns:media}}:$1|$1]]''' дегенді жоймақсыз.",
'filedelete-intro-old'        => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' — [$4 $3, $2 кезіндегі нұсқасын] жоюдасыз.</span>',
'filedelete-comment'          => 'Жою себебі:',
'filedelete-submit'           => 'Жой',
'filedelete-success'          => "'''$1''' деген жойылды.",
'filedelete-success-old'      => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' — $3, $2 кезіндегі нұсқасы жойылды.</span>',
'filedelete-nofile'           => "'''$1''' деген {{SITENAME}} жобасында жоқ.",
'filedelete-nofile-old'       => "Келтірілген анықтауыштарымен '''$1''' дегеннің мұрағатталған нұсқасы мында жоқ.",
'filedelete-iscurrent'        => 'Бұл файлдың ең соңғы нұсқасын жою талап еткенсіз.
Ең алдынан ескілеу нұсқасына қайтарыңыз.',
'filedelete-otherreason'      => 'Басқа/қосымша себеп:',
'filedelete-reason-otherlist' => 'Басқа себеп',
'filedelete-reason-dropdown'  => '* Жоюдың жалпы себептері
** Ауторлық құқықтарын бұзу
** Файл телнұсқасы',
'filedelete-edit-reasonlist'  => 'Жою себептерін өңдеу',

# MIME search
'mimesearch'         => 'Файлды MIME түрімен іздеу',
'mimesearch-summary' => 'Бұл бетте файлдарды MIME түрімен сүзгілеуі қосылған.
Кірісі: мағлұмат_түрі/түр_тарауы, мысалы <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME түрі:',
'download'           => 'қотарып алу',

# Unwatched pages
'unwatchedpages' => 'Бақыланылмаған беттер',

# List redirects
'listredirects' => 'Айдату бет тізімі',

# Unused templates
'unusedtemplates'     => 'Пайдаланылмаған үлгілер',
'unusedtemplatestext' => 'Бұл бет басқа бетке кіріcтірілмеген үлгі есім аяысындағы барлық беттерді тізімдейді.
Үлгілерді жою алдынан бұның өзге сілтемелерін тексеріп шығуын ұмытпаңыз',
'unusedtemplateswlh'  => 'басқа сілтемелер',

# Random page
'randompage'         => 'Кездейсоқ бет',
'randompage-nopages' => 'Бұл есім аясында беттер жоқ.',

# Random redirect
'randomredirect'         => 'Кездейсоқ айдағыш',
'randomredirect-nopages' => 'Бұл есім аясында еш айдағыш жоқ.',

# Statistics
'statistics'             => 'Санақ',
'sitestats'              => '{{SITENAME}} санағы',
'userstats'              => 'Қатысушы санағы',
'sitestatstext'          => "Дерекқорда {{PLURAL:$1|'''1'''|жалпы '''$1'''}} бет бар.
Бұған «талқылау» беттері, {{SITENAME}} жобасы туралы беттер, тым қысқа «бітеме» беттері, айдағыштар, тағы да басқа мағлұмат деп танылмайтын беттер кірістірледі.
Соларды есептен шығарғанда, мында мағлұмат {{PLURAL:$2|беті|беттері}} деп саналатын '''$2''' бет бар деп болжанады.

'''$8''' файл қотарып берілді.

{{SITENAME}} орнатылғаннан бері беттер {{PLURAL:$3|'''1'''|жалпы '''$3'''}} рет қаралған, және беттер '''$4''' рет өңделген.
Бұның нәтижесінде орташа есеппен әрбір бетке '''$5''' өңдеме келеді, және әрбір өңдемеге '''$6''' қарау келеді.

[http://www.mediawiki.org/wiki/Manual:Job_queue Тапсырымалар кезегінің] ұзындығы: '''$7'''.",
'userstatstext'          => "Мында '''$1''' [[{{#special:Listusers}}|тіркелген қатысушы]] бар, соның ішінде '''$2''' (не '''$4 %''') қатысушысында $5 құқықтары бар",
'statistics-mostpopular' => 'Ең көп қаралған беттер',

'disambiguations'      => 'Айрықты беттер',
'disambiguationspage'  => '{{ns:template}}:Айрық',
'disambiguations-text' => "Келесі беттер '''айрықты бетке''' сілтейді.
Бұның орнына белгілі тақырыпқа сілтеуі керек.<br />
Егер [[{{ns:mediawiki}}:Disambiguationspage]] тізіміндегі үлгі қолданылса, бет айрықты деп саналады.",

'doubleredirects'     => 'Шынжырлы айдағыштар',
'doubleredirectstext' => 'Бұл бетте басқа айдату беттерге сілтейтін беттер тізімделінеді. Әрбір жолақта бірінші және екінші айдағышқа сілтемелер бар, сонымен бірге екінші айдағыш нысанасы бар, әдетте бұл бірінші айдағыш бағыттайтын «нақты» нысана бет атауы болуы керек.',

'brokenredirects'        => 'Еш бетке келтірмейтін айдағыштар',
'brokenredirectstext'    => 'Келесі айдағыштар жоқ беттерге сілтейді:',
'brokenredirects-edit'   => '(өңдеу)',
'brokenredirects-delete' => '(жою)',

'withoutinterwiki'         => 'Еш тілге сілтeмеген беттер',
'withoutinterwiki-summary' => 'Келесі беттер басқа тілдерге сілтемейді',
'withoutinterwiki-legend'  => 'Басталуы:',
'withoutinterwiki-submit'  => 'Көрсет',

'fewestrevisions' => 'Ең аз түзетілген беттер',

# Miscellaneous special pages
'nbytes'                  => '$1 байт',
'ncategories'             => '$1 санат',
'nlinks'                  => '$1 сілтеме',
'nmembers'                => '$1 мүше',
'nrevisions'              => '$1 түзету',
'nviews'                  => '$1 рет қаралған',
'specialpage-empty'       => 'Бұл баянатқа еш нәтиже жоқ.',
'lonelypages'             => 'Еш беттен сілтелмеген беттер',
'lonelypagestext'         => 'Келесі беттерге {{SITENAME}} жобасындағы басқа беттер сілтемейді.',
'uncategorizedpages'      => 'Санатсыз беттер',
'uncategorizedcategories' => 'Санатсыз санаттар',
'uncategorizedimages'     => 'Санатсыз файлдар',
'uncategorizedtemplates'  => 'Санатсыз үлгілер',
'unusedcategories'        => 'Пайдаланылмаған санаттар',
'unusedimages'            => 'Пайдаланылмаған файлдар',
'popularpages'            => 'Ең көп қаралған беттер',
'wantedcategories'        => 'Басталмаған санаттар',
'wantedpages'             => 'Басталмаған беттер',
'missingfiles'            => 'Жоқ файлдар',
'mostlinked'              => 'Ең көп сілтенген беттер',
'mostlinkedcategories'    => 'Ең көп пайдаланылған санаттар',
'mostlinkedtemplates'     => 'Ең көп пайдаланылған үлгілер',
'mostcategories'          => 'Ең көп санаты бар беттер',
'mostimages'              => 'Ең көп пайдаланылған файлдар',
'mostrevisions'           => 'Ең көп түзетілген беттер',
'prefixindex'             => 'Атау бастауыш тізімі',
'shortpages'              => 'Ең қысқа беттер',
'longpages'               => 'Ең ұзын беттер',
'deadendpages'            => 'Еш бетке сілтемейтін беттер',
'deadendpagestext'        => 'Келесі беттер {{SITENAME}} жобасындағы басқа беттерге сілтемейді.',
'protectedpages'          => 'Қорғалған беттер',
'protectedpages-indef'    => 'Тек белгісіз қорғаулар',
'protectedpagestext'      => 'Келесі беттер өңдеуден немесе жылжытудан қорғалған',
'protectedpagesempty'     => 'Ағымда мынадай бапталымдарымен ешбір бет қорғалмаған',
'protectedtitles'         => 'Қорғалған тақырып аттары',
'protectedtitlestext'     => 'Келесі тақырып аттарын бастауға рұқсат берілмеген',
'protectedtitlesempty'    => 'Бұл бапталымдармен ағымда еш тақырып аттары қорғалмаған.',
'listusers'               => 'Қатысушы тізімі',
'newpages'                => 'Ең жаңа беттер',
'newpages-username'       => 'Қатысушы аты:',
'ancientpages'            => 'Ең ескі беттер',
'move'                    => 'Жылжыту',
'movethispage'            => 'Бетті жылжыту',
'unusedimagestext'        => '<p>Аңғартпа: Ғаламтордағы басқа тораптар файлға тура URL арқылы сілтеуі мүмкін. Сондықтан, белсенді пайдалануына аңғармай, осы тізімде қалуы мүмкін.</p>',
'unusedcategoriestext'    => 'Келесі санат беттері бар боп тұр, бірақ оған еш бет не санат кірмейді.',
'notargettitle'           => 'Нысана жоқ',
'notargettext'            => 'Осы жете орындалатын нысана бетті, не қатысушыны енгізбепсіз.',
'nopagetitle'             => 'Мынадай еш нысана бет жоқ',
'nopagetext'              => 'Келтірілген нысана бетіңіз жоқ.',
'pager-newer-n'           => 'жаңалау $1',
'pager-older-n'           => 'ескілеу $1',
'suppress'                => 'Шеттету',

# Book sources
'booksources'               => 'Кітап қайнарлары',
'booksources-search-legend' => 'Кітап қайнарларын іздеу',
'booksources-go'            => 'Өту',
'booksources-text'          => 'Төменде жаңа және қолданған кітаптар сататын тораптарының сілтемелері тізімделген. Бұл тораптарда ізделген кітаптар туралы былайғы ақпарат болуға мүмкін.',

# Special:Log
'specialloguserlabel'  => 'Қатысушы:',
'speciallogtitlelabel' => 'Тақырып аты:',
'log'                  => 'Журналдар',
'all-logs-page'        => 'Барлық журналдар',
'log-search-legend'    => 'Журналдардан іздеу',
'log-search-submit'    => 'Өт',
'alllogstext'          => '{{SITENAME}} жобасының барлық қатынаулы журналдарын біріктіріп көрсетуі.
Журнал түрін, қатысушы атын, не тиісті бетін бөлектеп, тарылтып қарай аласыз.',
'logempty'             => 'Журналда сәйкес даналар жоқ.',
'log-title-wildcard'   => 'Мына мәтіннең басталытын тақырып аттарын іздеу',

# Special:AllPages
'allpages'          => 'Барлық беттер',
'alphaindexline'    => '$1 — $2',
'nextpage'          => 'Келесі бетке ($1)',
'prevpage'          => 'Алдыңғы бетке ($1)',
'allpagesfrom'      => 'Мына беттен бастап көрсету:',
'allarticles'       => 'Барлық бет тізімі',
'allinnamespace'    => 'Барлық бет ($1 есім аясы)',
'allnotinnamespace' => 'Барлық бет ($1 есім аясынан тыс)',
'allpagesprev'      => 'Алдыңғыға',
'allpagesnext'      => 'Келесіге',
'allpagessubmit'    => 'Өту',
'allpagesprefix'    => 'Мынадан басталған беттерді көрсету:',
'allpagesbadtitle'  => 'Келтірілген бет тақырыбын аты жарамсыз болған, немесе тіл-аралық не уики-аралық бастауы бар болды.
Мында тақырып атында қолдалмайтын бірқатар таңбалар болуы мүмкін.',
'allpages-bad-ns'   => '{{SITENAME}} жобасында «$1» есім аясы жоқ.',

# Special:Categories
'categories'                    => 'Санаттар',
'categoriespagetext'            => 'Келесі санаттар ішінде беттер не таспалар бар.',
'categoriesfrom'                => 'Санаттарды мынадан бастап көрсету:',
'special-categories-sort-count' => 'санымен сұрыптау',
'special-categories-sort-abc'   => 'әліпбимен сұрыптау',

# Special:ListUsers
'listusersfrom'      => 'Мына қатысушыдан бастап көрсету:',
'listusers-submit'   => 'Көрсет',
'listusers-noresult' => 'Қатысушы табылған жоқ.',

# Special:ListGroupRights
'listgrouprights'          => 'Қатысушы тобы құқықтары',
'listgrouprights-summary'  => 'Келесі тізімде бұл уикиде тағайындалған қатысушы құқықтары (байланысты қатынау құқықтарымен бірге) көрсетіледі.
Жеке құқықтар туралы көбірек ақпаратты [[{{MediaWiki:Listgrouprights-helppage}}|мында]] таба аласыз.',
'listgrouprights-group'    => 'Топ',
'listgrouprights-rights'   => 'Құқықтары',
'listgrouprights-helppage' => '{{ns:help}}:Топ құқықтары',
'listgrouprights-members'  => '(мүше тізімі)',

# E-mail user
'mailnologin'     => 'Еш мекенжай жөнелтілген жоқ',
'mailnologintext' => 'Басқа қатысушыға хат жөнелту үшін [[{{#special:Userlogin}}|кіруіңіз]] жөн, және [[{{#special:Preferences}}|бапталымдарыңызда]] жарамды е-пошта мекенжайы болуы жөн.',
'emailuser'       => 'Қатысушыға хат жазу',
'emailpage'       => 'Қатысушыға хат жазу',
'emailpagetext'   => 'Егер бұл қатысушы баптауларында жарамды е-пошта мекенжайын енгізсе, төмендегі пішін арқылы бұған жалғыз е-пошта хатын жөнелтуге болады.
Қатысушы баптауыңызда енгізген е-пошта мекенжайыңыз «Кімнен» деген бас жолағында көрінеді, сондықтан хат алушысы тура жауап бере алады.',
'usermailererror' => 'Mail нысаны қате қайтарды:',
'defemailsubject' => '{{SITENAME}} е-поштасының хаты',
'noemailtitle'    => 'Еш е-пошта мекенжайы жоқ',
'noemailtext'     => 'Бұл қатысушы жарамды Е-пошта мекенжайын келтірмеген, не басқалардан хат қабылдауын өшірген.',
'emailfrom'       => 'Кімнен',
'emailto'         => 'Кімге',
'emailsubject'    => 'Тақырыбы',
'emailmessage'    => 'Хат',
'emailsend'       => 'Жөнелту',
'emailccme'       => 'Хатымдың көшірмесін маған да жөнелт.',
'emailccsubject'  => '$1 дегенге хатыңыздың көшірмесі: $2',
'emailsent'       => 'Хат жөнелтілді',
'emailsenttext'   => 'Е-пошта хатыңыз жөнелтілді.',

# Watchlist
'watchlist'            => 'Бақылау тізімі',
'mywatchlist'          => 'Бақылауым',
'watchlistfor'         => "('''$1''' бақылаулары)",
'nowatchlist'          => 'Бақылау тізіміңізде еш дана жоқ',
'watchlistanontext'    => 'Бақылау тізіміңіздегі даналарды қарау, не өңдеу үшін $1 керек.',
'watchnologin'         => 'Кірмегенсіз',
'watchnologintext'     => 'Бақылау тізіміңізді өзгерту үшін [[{{#special:Userlogin}}|кіруіңіз]] жөн.',
'addedwatch'           => 'Бақылау тізіміне үстелді',
'addedwatchtext'       => "«[[:$1]]» беті [[{{#special:Watchlist}}|бақылау тізіміңізге]] үстелді.
Бұл беттің және байланысты талқылау бетінің келешектегі өзгерістері мында тізімделінеді де, және беттің атауы жеңіл табылу үшін [[{{#special:Recentchanges}}|жуықтағы өзгерістер тізімінде]] '''жуан әрпімен''' көрсетіледі.",
'removedwatch'         => 'Бақылау тізіміңізден аласталды',
'removedwatchtext'     => '«[[:$1]]» беті бақылау тізіміңізден аласталды.',
'watch'                => 'Бақылау',
'watchthispage'        => 'Бетті бақылау',
'unwatch'              => 'Бақыламау',
'unwatchthispage'      => 'Бақылауды тоқтату',
'notanarticle'         => 'Мағлұмат беті емес',
'notvisiblerev'        => 'Түзету жойылды',
'watchnochange'        => 'Көрсетілген мерзімде еш бақыланған дана өңделген жоқ.',
'watchlist-details'    => 'Талқылау беттерін санамағанда $1 бет бақланылады.',
'wlheader-enotif'      => '* Ескерту хат жіберуі қосылған.',
'wlheader-showupdated' => "* Соңғы келіп-кетуіңізден бері өзгертілген беттерді '''жуан''' қаріпімен көрсет",
'watchmethod-recent'   => 'бақылаулы беттер үшін жуықтағы өзгерістерді тексеру',
'watchmethod-list'     => 'жуықтағы өзгерістер үшін бақылаулы беттерді тексеру',
'watchlistcontains'    => 'Бақылау тізіміңізде $1 бет бар.',
'iteminvalidname'      => "'$1' данада ақау бар — жарамсыз атау…",
'wlnote'               => "Төменде соңғы {{PLURAL:$2|сағатта|'''$2''' сағатта}} болған, {{PLURAL:$1|жуықтағы өзгеріс|жуықтағы '''$1''' өзгеріс}} көрсетіледі.",
'wlshowlast'           => 'Соңғы $1 сағаттағы, $2 күндегі, $3 болған өзгерісті көрсету',
'watchlist-show-bots'  => 'Бот өңдемелерін көрсет',
'watchlist-hide-bots'  => 'Бот өңдемелерін жасыр',
'watchlist-show-own'   => 'Өңдемелерімді көрсет',
'watchlist-hide-own'   => 'Өңдемелерімді жасыр',
'watchlist-show-minor' => 'Шағын өңдемелерді көрсет',
'watchlist-hide-minor' => 'Шағын өңдемелерді жасыр',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Бақылауда…',
'unwatching' => 'Бақыламауда…',

'enotif_mailer'                => '{{SITENAME}} ескерту хат жіберу қызметі',
'enotif_reset'                 => 'Барлық бет келіп-кетілді деп белгіле',
'enotif_newpagetext'           => 'Мынау жаңа бет.',
'enotif_impersonal_salutation' => '{{SITENAME}} қатысушысы',
'changed'                      => 'өзгертті',
'created'                      => 'бастады',
'enotif_subject'               => '{{SITENAME}} жобасында $PAGEEDITOR $PAGETITLE атаулы бетті $CHANGEDORCREATED',
'enotif_lastvisited'           => 'Соңғы келіп-кетуіңізден бері болған өзгерістер үшін $1 дегенді қараңыз.',
'enotif_lastdiff'              => 'Осы өзгеріс үшін $1 дегенді қараңыз.',
'enotif_anon_editor'           => 'тіркелгісіз қатысушы $1',
'enotif_body'                  => 'Қадірлі $WATCHINGUSERNAME,


{{SITENAME}} жобасының $PAGETITLE атаулы бетті $PAGEEDITDATE кезінде $PAGEEDITOR деген $CHANGEDORCREATED, ағымдық нұсқасы үшін $PAGETITLE_URL қараңыз.

$NEWPAGE

Өңдеуші келтірген қысқаша мазмұндамасы: $PAGESUMMARY $PAGEMINOREDIT

Өңдеушімен қатынасу:
е-пошта: $PAGEEDITOR_EMAIL
уики: $PAGEEDITOR_WIKI

Былайғы өзгерістер болғанда да осы бетке келіп-кетуіңізгенше дейін ешқандай басқа ескерту хаттар жіберілмейді.
Сонымен қатар бақылау тізіміңіздегі бет ескертпелік белгісін қайта қойыңыз.

             Сіздің достық {{SITENAME}} жобасының ескерту қызметі

----
Бақылау тізіміңіздің баптаулырын өзгерту үшін, мында келіп-кетіңіз:
{{fullurl:{{#special:Watchlist}}/edit}}

Сын-пікір беру және былайғы жәрдем алу үшін:
{{fullurl:{{{{ns:mediawiki}}:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Бетті жою',
'confirm'                     => 'Құптау',
'excontent'                   => "болған мағлұматы: '$1'",
'excontentauthor'             => "болған мағлұматы (тек '[[{{#special:Contributions}}/$2|$2]]' үлесі): '$1'",
'exbeforeblank'               => "тазарту алдындағы болған мағлұматы: '$1'",
'exblank'                     => 'бет бос болды',
'delete-confirm'              => '«$1» дегенді жою',
'delete-legend'               => 'Жою',
'historywarning'              => 'Құлақтандыру: Жоюы көзделген бетте тарихы бар:',
'confirmdeletetext'           => 'Бетті бүкіл тарихымен бірге дерекқордан жойюын коздедіңіз.
Осыны істеу ниетіңізді, салдарын байымдауыңызды және [[{{{{ns:mediawiki}}:Policy-url}}]] дегенге лайықты деп істемектенгеңізді құптаңыз.',
'actioncomplete'              => 'Әрекет бітті',
'deletedtext'                 => '«<nowiki>$1</nowiki>» жойылды.
Жуықтағы жоюлар туралы жазбаларын $2 дегеннен қараңыз.',
'deletedarticle'              => '«[[$1]]» дегенді жойды',
'suppressedarticle'           => '«[[$1]]» дегенді шеттетті',
'dellogpage'                  => 'Жою_журналы',
'dellogpagetext'              => 'Төменде жуықтағы жоюлардың тізімі берілген.',
'deletionlog'                 => 'жою журналы',
'reverted'                    => 'Ертерек түзетуіне қайтарылған',
'deletecomment'               => 'Жоюдың себебі:',
'deleteotherreason'           => 'Басқа/қосымша себеп:',
'deletereasonotherlist'       => 'Басқа себеп',
'deletereason-dropdown'       => '* Жоюдың жалпы себептері
** Аутордың сұранымы бойынша
** Ауторлық құқықтарын бұзу
** Бұзақылық',
'delete-edit-reasonlist'      => 'Жою себептерін өңдеу',
'delete-toobig'               => 'Бұл бетте байтақ түзету тарихы бар, $1 түзетуден астам.
Бұндай беттердің жоюы {{SITENAME}} торабын әлдеқалай үзіп тастауына бөгет салу үшін тиымдалған.',
'delete-warning-toobig'       => 'Бұл бетте байтақ түзету тарихы бар, $1 түзетуден астам.
Бұның жоюы {{SITENAME}} торабындағы дерекқор әрекеттерді үзіп тастауын мүмкін;
бұны абайлап өткізіңіз.',
'rollback'                    => 'Өңдемелерді шегіндіру',
'rollback_short'              => 'Шегіндіру',
'rollbacklink'                => 'шегіндіру',
'rollbackfailed'              => 'Шегіндіру сәтсіз бітті',
'cantrollback'                => 'Өңдеме қайтарылмады;
соңғы үлескері тек осы беттің бастаушысы болды.',
'alreadyrolled'               => '[[{{ns:user}}:$2|$2]] ([[{{ns:user_talk}}:$2|талқылауы]]) істеген [[:$1]] соңғы өңдемесі шегіндірілмеді;
басқа біреу бұл бетті алдақашан өңдеген не шегіндірген.

соңғы өңдемесін [[{{ns:user}}:$3|$3]] ([[{{ns:user_talk}}:$3|талқылауы]]) істеген.',
'editcomment'                 => 'Болған өңдеме мәндемесі: «<i>$1</i>».', # only shown if there is an edit comment
'revertpage'                  => '[[{{#special:Contributions}}/$2|$2]] ([[{{ns:user_talk}}:$2|талқылауы]]) өңдемелерінен [[{{ns:user}}:$1|$1]] соңғы нұсқасына қайтарды', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '$1 өңдемелерінен қайтарған;
$2 соңғы нұсқасына өзгертті.',
'sessionfailure'              => 'Кіру сессиясында шатақ болған сияқты;
сессияға шабуылдаудардан қорғану үшін, осы әрекет тоқтатылды.
«Артқа» дегенді басыңыз, және бетті қайта жүктеңіз де, қайта байқап көріңіз.',
'protectlogpage'              => 'Қорғау журналы',
'protectlogtext'              => 'Төменде беттердің қорғау/қорғамау тізімі берілген.
Ағымдағы қорғау әректтер бар беттер үшін [[{{#special:Protectedpages}}|қорғалған бет тізімін]] қараңыз.',
'protectedarticle'            => '«[[$1]]» қорғалды',
'modifiedarticleprotection'   => '«[[$1]]» қорғалу деңгейі өзгерді',
'unprotectedarticle'          => '«[[$1]]» қорғалуы өшірілді',
'protect-title'               => '«$1» қорғау деңгейін өзгерту',
'protect-legend'              => 'Қорғауды құптау',
'protectcomment'              => 'Мәндемесі:',
'protectexpiry'               => 'Мерзімі бітпек:',
'protect_expiry_invalid'      => 'Бітетін уақыты жарамсыз.',
'protect_expiry_old'          => 'Бітетін уақыты өтіп кеткен.',
'protect-unchain'             => 'Жылжыту рұқсаттарын беру',
'protect-text'                => '<strong><nowiki>$1</nowiki></strong> бетінің қорғау деңгейін қарап және өзгертіп шыға аласыз.',
'protect-locked-blocked'      => 'Бұғаттауыңыз өшірілгенше дейін қорғау деңгейін өзгерте алмайсыз.
Мына <strong>$1</strong> беттің ағымдық баптаулары:',
'protect-locked-dblock'       => 'Дерекқордың құлыптауы белсенді болғандықтан қорғау деңгейлері өзгертілмейді.
Мына <strong>$1</strong> беттің ағымдық баптаулары:',
'protect-locked-access'       => 'Тіркелгіңізге бет қорғау денгейлерін өзгертуіне рұқсат жоқ.
Мына <strong>$1</strong> беттің ағымдық баптаулары:',
'protect-cascadeon'           => 'Бұл бет ағымда қорғалған, себебі осы бет «баулы қорғауы» бар келесі {{PLURAL:$1|беттің|беттердің}} кірікбеті.
Бұл беттің қорғау деңгейін өзгерте аласыз, бірақ бұл баулы қорғауға ықпал етпейді.',
'protect-default'             => '(әдепкі)',
'protect-fallback'            => '«$1» рұқсаты керек',
'protect-level-autoconfirmed' => 'Тіркелгісіздерге тиым',
'protect-level-sysop'         => 'Тек әкімшілер',
'protect-summary-cascade'     => 'баулы',
'protect-expiring'            => 'мерзімі бітпек: $1 (UTC)',
'protect-cascade'             => 'Бұл беттің кірікбеттерін қорғау (баулы қорғау).',
'protect-cantedit'            => 'Бұл беттің қорғау деңгейін өзгерте алмайсыз, себебі бұны өңдеуге рұқстаңыз жоқ.',
'restriction-type'            => 'Рұқсаты:',
'restriction-level'           => 'Тиымдық деңгейі:',
'minimum-size'                => 'Ең аз мөлшері',
'maximum-size'                => 'Ең көп мөлшері:',
'pagesize'                    => '(байт)',

# Restrictions (nouns)
'restriction-edit'   => 'Өңдеуге',
'restriction-move'   => 'Жылжытуға',
'restriction-create' => 'Бастауға',
'restriction-upload' => 'Қотарып беруге',

# Restriction levels
'restriction-level-sysop'         => 'толықтай қорғалған',
'restriction-level-autoconfirmed' => 'жартылай қорғалған',
'restriction-level-all'           => 'әр деңгейде',

# Undelete
'undelete'                     => 'Жойылған беттерді қарау',
'undeletepage'                 => 'Жойылған беттерді қарау және қалпына келтіру',
'undeletepagetitle'            => "'''Келесі тізім [[:$1|$1]] дегеннің жойылған түзетулерінен тұрады'''.",
'viewdeletedpage'              => 'Жойылған беттерді қарау',
'undeletepagetext'             => 'Келесі беттер жойылды деп белгіленген, бірақ мағлұматы мұрағатта бар
және қалпына келтіруге мүмкін. Мұрағат мерзім бойынша тазаланып тұруы мүмкін.',
'undeleteextrahelp'            => "Бүкіл бетті қалпына келтіру үшін, барлық құсбелгі көздерді босатып '''''Қалпына келтір!''''' батырмасын нұқыңыз.
Бөлектеумен қалпына келтіру орындау үшін, келтіремін деген түзетулеріне сәйкес көздерге құсбелгі салыңыз да, және '''''Қалпына келтір!''''' түймесін нұқыңыз. '''''Қайта қой''''' түймесін нұқығанда мәндеме аумағы тазартады және барлық құсбелгі көздерін босатады.",
'undeleterevisions'            => '$1 түзету мұрағатталды',
'undeletehistory'              => 'Егер бет мағлұматын қалпына келтірсеңіз, тарихында барлық түзетулер да
қайтарылады. Егер жоюдан соң дәл солай атауымен жаңа бет басталса, қалпына келтірілген түзетулер
тарихтың алдында көрсетіледі. Тағы да файл түзетулерін қалпына келтіргенде тиымдары жойылатын ескеріңіз.',
'undeleterevdel'               => 'Егер бұл үстіңгі бетте аяқталса, не файл түзетуі жарым-жартылай жойылған болса, жою болдырмауы орындалмайды.
Осындай жағдайларда, ең жаңа жойылған түзетуін алып тастауыңыз не жасыруын болдырмауыңыз жөн.',
'undeletehistorynoadmin'       => 'Бұл бет жойылған.
Жою себебі алдындағы өңдеген қатысушылар егжей-тегжейлерімен бірге төмендегі қысқаша мазмұндамасында көрсетілген.
Мына жойылған түзетулерін көкейкесті мәтіні тек әкімшілерге жетімді.',
'undelete-revision'            => '$2 кезіндегі $3 жойған $1 дегеннің жойылған түзетуі:',
'undeleterevision-missing'     => 'Жарамсыз не жоғалған түзету.
Сілтемеңіз жарамсыз, не түзету қалпына келтірілген, немесе мұрағаттан аласталған болуы мүмкін.',
'undelete-nodiff'              => 'Еш алдыңғы түзету табылмады.',
'undeletebtn'                  => 'Қалпына келтір!',
'undeletelink'                 => 'қалпына келтіру',
'undeletereset'                => 'Қайта қой',
'undeletecomment'              => 'Мәндемесі:',
'undeletedarticle'             => '«[[$1]]» қалпына келтірілді',
'undeletedrevisions'           => '$1 түзету қалпына келтірілді',
'undeletedrevisions-files'     => '$1 түзету және $2 файл қалпына келтірілді',
'undeletedfiles'               => '$1 файл қалпына келтірілді',
'cannotundelete'               => 'Жою болдырмауы сәтсіз бітті;
басқа біреу алғашында беттің жоюдың болдырмауы мүмкін.',
'undeletedpage'                => "<big>'''$1 қалпына келтірілді'''</big>

Жуықтағы жоюлар мен қалпына келтірулер жөнінде [[{{#special:Log}}/delete|жою журналын]] қараңыз.",
'undelete-header'              => 'Жуықтағы жойылған беттер жөнінде [[{{#special:Log}}/delete|жою журналын]] қараңыз.',
'undelete-search-box'          => 'Жойылған беттерді іздеу',
'undelete-search-prefix'       => 'Мынадан басталған беттерді көрсет:',
'undelete-search-submit'       => 'Іздеу',
'undelete-no-results'          => 'Жою мұрағатында ешқандай сәйкес беттер табылмады.',
'undelete-filename-mismatch'   => '$1 кезіндегі файл түзетуінің жоюы болдырмады: файл атауы сәйкессіз',
'undelete-bad-store-key'       => '$1 кезіндегі файл түзетуінің жоюы болдырмады: жоюдың алдынан файл жоқ болған.',
'undelete-cleanup-error'       => '«$1» пайдаланылмаған мұрағатталған файл жою қатесі.',
'undelete-missing-filearchive' => 'Мұрағатталған файл (нөмірі $1) қалпына келтіруі икемді емес, себебі ол дерекқорда жоқ.
Бұның жоюын болдырмауы алдақашан болғаны мүмкін.',
'undelete-error-short'         => 'Файл жоюын болдырмау қатесі: $1',
'undelete-error-long'          => 'Файл жоюын болдырмау кезінде мына қателер кездесті:

$1',

# Namespace form on various pages
'namespace'      => 'Есім аясы:',
'invert'         => 'Бөлектеуді керілеу',
'blanknamespace' => '(Негізгі)',

# Contributions
'contributions' => 'Қатысушы үлесі',
'mycontris'     => 'Үлесім',
'contribsub2'   => '$1 ($2) үлесі',
'nocontribs'    => 'Осы іздеу шартына сәйкес өзгерістер табылған жоқ.',
'uctop'         => ' (үсті)',
'month'         => 'Мына айдан (және ертеректен):',
'year'          => 'Мына жылдан (және ертеректен):',

'sp-contributions-newbies'     => 'Тек жаңа тіркелгіден жасаған үлестерді көрсет',
'sp-contributions-newbies-sub' => 'Жаңадан тіркелгі жасағандар үшін',
'sp-contributions-blocklog'    => 'Бұғаттау журналы',
'sp-contributions-search'      => 'Үлес үшін іздеу',
'sp-contributions-username'    => 'IP мекенжайы не қатысушы аты:',
'sp-contributions-submit'      => 'Ізде',

# What links here
'whatlinkshere'            => 'Мында сілтейтін беттер',
'whatlinkshere-title'      => '$1 дегенге сілтейтін беттер',
'whatlinkshere-page'       => 'Бет:',
'linklistsub'              => '(Сілтемелер тізімі)',
'linkshere'                => "'''[[:$1]]''' дегенге мына беттер сілтейді:",
'nolinkshere'              => "'''[[:$1]]''' дегенге еш бет сілтемейді.",
'nolinkshere-ns'           => "Таңдалған есім аясында '''[[:$1]]''' дегенге ешқандай бет сілтемейді.",
'isredirect'               => 'айдату беті',
'istemplate'               => 'кірікбет',
'isimage'                  => 'сурет сілтемесі',
'whatlinkshere-prev'       => '{{PLURAL:$1|алдыңғы|алдыңғы $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|келесі|келесі $1}}',
'whatlinkshere-links'      => '← сілтемелер',
'whatlinkshere-hideredirs' => 'айдағыштарды $1',
'whatlinkshere-hidetrans'  => 'кірікбеттерді $1',
'whatlinkshere-hidelinks'  => 'сілтемелерді $1',
'whatlinkshere-hideimages' => 'сурет сілтемелерін $1',
'whatlinkshere-filters'    => 'Сүзгілер',

# Block/unblock
'blockip'                     => 'Қатысушыны бұғаттау',
'blockip-legend'              => 'Қатысушыны бұғаттау',
'blockiptext'                 => 'Төмендегі пішін қатысушының жазу рұқсатын белгілі IP мекенжайымен не атымен бұғаттау үшін қолданылады.
Бұны тек бұзақылықты қақпайлау үшін және де [[{{{{ns:mediawiki}}:Policy-url}}|ережелер]] бойынша атқаруыңыз жөн.
Төменде тиісті себебін толтырып көрсетіңіз (мысалы, дәйекке бұзақылықпен өзгерткен беттерді келтіріп).',
'ipaddress'                   => 'IP мекенжайы:',
'ipadressorusername'          => 'IP мекенжайы не қатысушы аты:',
'ipbexpiry'                   => 'Мерзімі бітпек:',
'ipbreason'                   => 'Себебі:',
'ipbreasonotherlist'          => 'Басқа себеп',
'ipbreason-dropdown'          => '* Бұғаттаудың жалпы себебтері 
** Жалған мәлімет енгізу 
** Беттердегі мағлұматты аластау 
** Шеттік тораптар сілтемелерін жаудыру 
** Беттерге мағынасыздық/балдырлау кірістіру 
** Қоқандау/қуғындау мінезқұлық 
** Бірнеше рет тіркеліп қиянаттау 
** Өрескел қатысушы аты',
'ipbanononly'                 => 'Тек тіркелгісіз қатысушыларды бұғаттау',
'ipbcreateaccount'            => 'Тіркелуді қақпайлау',
'ipbemailban'                 => 'Қатысушы е-поштамен хат жөнелтуін қақпайлау',
'ipbenableautoblock'          => 'Бұл қатысушы соңғы қолданған IP мекенжайын, және кейін өңдеуге байқап көрген әр IP мекенжайларын өзбұғаттауы',
'ipbsubmit'                   => 'Қатысушыны бұғатта',
'ipbother'                    => 'Басқа мерзімі:',
'ipboptions'                  => '2 сағат:2 hours,1 күн:1 day,3 күн:3 days,1 апта:1 week,2 апта:2 weeks,1 ай:1 month,3 ай:3 months,6 ай:6 months,1 жыл:1 year,мәнгі:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'басқа',
'ipbotherreason'              => 'Басқа/қосымша себеп:',
'ipbhidename'                 => 'Қатысушы атын бұғаттау журналыннан, белсенді бұғаттау тізімінен, қатысушы тізімінен жасыру',
'ipbwatchuser'                => 'Бұл қатысушының жеке және талқылау беттерін бақылау',
'badipaddress'                => 'Жарамсыз IP мекенжайы',
'blockipsuccesssub'           => 'Бұғаттау сәтті өтті',
'blockipsuccesstext'          => '[[{{#special:Contributions}}/$1|$1]] деген бұғатталған.<br />
Бұғаттарды шолып шығу үшін [[{{#special:Ipblocklist}}|IP бұғаттау тізімін]] қараңыз.',
'ipb-edit-dropdown'           => 'Бұғаттау себептерін өңдеу',
'ipb-unblock-addr'            => '$1 дегенді бұғаттамау',
'ipb-unblock'                 => 'Қатысушы атын немесе IP мекенжайын бұғаттамау',
'ipb-blocklist-addr'          => '$1 үшін бар бұғаттауларды қарау',
'ipb-blocklist'               => 'Бар бұғаттауларды қарау',
'unblockip'                   => 'Қатысушыны бұғаттамау',
'unblockiptext'               => 'Төмендегі пішінді алдындағы IP мекенжайымен не атымен бұғатталған қатысушыға жазу қатынауын қалпына келтіріуі үшін қолданыңыз.',
'ipusubmit'                   => 'Осы мекенжайды бұғаттамау',
'unblocked'                   => '[[{{ns:user}}:$1|$1]] бұғаттауы өшірілді',
'unblocked-id'                => '$1 бұғаттау аласталды',
'ipblocklist'                 => 'Бұғатталған қатысушы / IP мекенжай тізімі',
'ipblocklist-legend'          => 'Бұғатталған қатысушыны табу',
'ipblocklist-username'        => 'Қатысушы аты / IP мекенжайы:',
'ipblocklist-submit'          => 'Ізде',
'blocklistline'               => '$1, $2 $3 дегенді бұғаттады ($4)',
'infiniteblock'               => 'мәнгі',
'expiringblock'               => 'мерзімі бітпек: $1',
'anononlyblock'               => 'тек тіркелгісіздерді',
'noautoblockblock'            => 'өзбұғаттау өшірілген',
'createaccountblock'          => 'тіркелу бұғатталған',
'emailblock'                  => 'е-пошта бұғатталған',
'ipblocklist-empty'           => 'Бұғаттау тізімі бос.',
'ipblocklist-no-results'      => 'Сұратылған IP мекенжай не қатысушы аты бұғатталған емес.',
'blocklink'                   => 'бұғаттау',
'unblocklink'                 => 'бұғаттамау',
'contribslink'                => 'үлесі',
'autoblocker'                 => 'IP мекенжайыңызды жуықта «[[{{ns:user}}:1|$1]]» пайдаланған, сондықтан өзбұғатталған.
$1 бұғаттауы үшін келтірілген себебі: «$2».',
'blocklogpage'                => 'Бұғаттау_журналы',
'blocklogentry'               => '[[$1]] дегенді $2 мерзімге бұғаттады $3',
'blocklogtext'                => 'Бұл қатысушыларды бұғаттау/бұғаттамау әрекеттерінің журналы.
Өздіктік бұғатталған IP мекенжайлар осында тізімделгемеген.
Ағымдағы белсенді тиымдар мен бұғаттауларды [[{{#special:Ipblocklist}}|IP бұғаттау тізімінен]] қараңыз.',
'unblocklogentry'             => '«$1» — бұғаттауын өшірді',
'block-log-flags-anononly'    => 'тек тіркелгісіздер',
'block-log-flags-nocreate'    => 'тіркелу өшірілген',
'block-log-flags-noautoblock' => 'өзбұғаттау өшірілген',
'block-log-flags-noemail'     => 'е-пошта бұғатталған',
'range_block_disabled'        => 'Ауқым бұғаттауларын жасау әкімшілік мүмкіндігі өшірілген.',
'ipb_expiry_invalid'          => 'Бітетін уақыты жарамсыз.',
'ipb_expiry_temp'             => 'Жасырылған қатысушы атын бұғаттауы мәңгі болуы жөн.',
'ipb_already_blocked'         => '«$1» алдақашан бұғатталған',
'ipb_cant_unblock'            => 'Қателік: IP $1 бұғаттауы табылмады. Оның бұғаттауы алдақашан өшірлген мүмкін.',
'ipb_blocked_as_range'        => 'Қателік: IP $1 тікелей бұғатталмаған және бұғаттауы өшірілмейді.
Бірақ, бұл бұғаттауы өшірілуі мүмкін $2 ауқымы бөлігі боп бұғатталған.',
'ip_range_invalid'            => 'IP мекенжай ауқымы жарамсыз.',
'blockme'                     => 'Өздіктік_бұғаттау',
'proxyblocker'                => 'Прокси серверлерді бұғаттауыш',
'proxyblocker-disabled'       => 'Бұл жете өшірілген.',
'proxyblockreason'            => 'IP мекенжайыңыз ашық прокси серверге жататындықтан бұғатталған.
Интернет қызметін жабдықтаушыңызбен, не техникалық қолдау қызметімен қатынасыңыз, және оларға осы оте күрделі қауыпсіздік шатақ туралы ақпарат беріңіз.',
'proxyblocksuccess'           => 'Бітті.',
'sorbsreason'                 => 'IP мекенжайыңыз {{SITENAME}} торабында қолданылған DNSBL қара тізіміндегі ашық прокси-сервер деп табылады.',
'sorbs_create_account_reason' => 'IP мекенжайыңыз {{SITENAME}} торабында қолданылған DNSBL қара тізіміндегі ашық прокси-сервер деп табылады.
Жаңа тіркелгі жасай алмайсыз.',

# Developer tools
'lockdb'              => 'Дерекқорды құлыптау',
'unlockdb'            => 'Дерекқорды құлыптамау',
'lockdbtext'          => 'Дерекқордын құлыпталуы барлық қатысушылардың бет өңдеу, баптауын қалау, бақылау тізімін, тағы басқа дерекқорды өзгертетін мүмкіндіктерін тоқтата тұрады.
Осы мақсатыңызды, және баптау біткенде дерекқорды ашатыңызды құптаңыз.',
'unlockdbtext'        => 'Дерекқодын ашылуы барлық қатысушылардың бет өңдеу, баптауын қалау, бақылау тізімін, тағы басқа дерекқорды өзгертетін мүмкіндіктерін қалпына келтіреді.
Осы мақсатыңызды құптаңыз.',
'lockconfirm'         => 'Иә, дерекқор құлыптауын нақты тілеймін.',
'unlockconfirm'       => 'Иә, дерекқор құлыптамауын нақты тілеймін.',
'lockbtn'             => 'Дерекқорды құлыпта',
'unlockbtn'           => 'Дерекқорды құлыптама',
'locknoconfirm'       => 'Құптау көзіне құсбелгі салмағансыз.',
'lockdbsuccesssub'    => 'Дерекқор құлыптауы сәтті өтті',
'unlockdbsuccesssub'  => 'Дерекқор құлыптауы аласталды',
'lockdbsuccesstext'   => 'Дерекқор құлыпталды.<br />
Баптау толық өткізілгеннен кейін [[{{#special:Unlockdb}}|құлыптауын аластауға]] ұмытпаңыз.',
'unlockdbsuccesstext' => 'Құлыпталған дерекқор сәтті ашылды.',
'lockfilenotwritable' => 'Дерекқор құлыптау файлы жазылмайды.
Дерекқорды құлыптау не ашу үшін, веб-сервер файлға жазу рұқсаты болу керек.',
'databasenotlocked'   => 'Дерекқор құлыпталған жоқ.',

# Move page
'move-page'               => '$1 дегенді жылжыту',
'move-page-legend'        => 'Бетті жылжыту',
'movepagetext'            => "Төмендегі пішінді қолданып беттерді қайта атайды, барлық тарихын жаңа атауға жылжытады.
Бұрынғы бет тақырыбын аты жаңа тақырып атына айдайтын бет болады.
Ескі тақырып атына сілтейтін сілтемелер өзгертілмейді;
жылжытудан соң шынжырлы не жарамсыз айдағыштар бар-жоғын тексеріп шығыңыз.
Сілтемелер бұрынғы жолдауымен былайғы өтуін тексеруіне өзіңіз міндетті боласыз.

Аңғартпа: Егер осы арада алдақашан жаңа тақырып аты бар бет болса, бұл бос не айдағыш болғанша дейін, және соңында түзету тарихы жоқ болса, бет '''жылжытылмайды'''. Осының мағынасы: егер бетті қателікпен қайта атасаңыз, бұрынғы атауына қайта атауға болады, және бар беттің үстіне жазуыңызға болмайды.

'''ҚҰЛАҚТАНДЫРУ!'''
Бұл көп қаралатын бетке қатаң және кенет өзгеріс жасауға мүмкін;
осының салдарын байымдауыңызды әрекеттің алдынан батыл болыңыз.",
'movepagetalktext'        => "Келесі себептер '''болғанша''' дейін, талқылау беті бұнымен бірге өздіктік жылжытылады:
* Бос емес талқылау беті жаңа атауда алдақашан болғанда, не
* Төмендегі көзге құсбелгі алып тасталғанда.

Осы орайда, қалауыңыз болса, бетті қолдан жылжыта не қоса аласыз.",
'movearticle'             => 'Жылжытпақ бет:',
'movenotallowed'          => '{{SITENAME}} жобасында беттерді жылжыту руқсатыңыз жоқ.',
'newtitle'                => 'Жаңа тақырып атына:',
'move-watch'              => 'Бұл бетті бақылау',
'movepagebtn'             => 'Бетті жылжыт',
'pagemovedsub'            => 'Жылжыту сәтті аяқталды',
'movepage-moved'          => '<big>\'\'\'"$1" беті "$2" бетіне жылжытылды\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Осылай аталған бет алдақашан бар, не таңдаған атауыңыз жарамды емес.
Өзге атауды таңдаңыз',
'cantmove-titleprotected' => 'Бетті осы орынға жылжыта алмайсыз, себебі жаңа тақырып аты бастаудан қорғалған',
'talkexists'              => "'''Беттің өзі сәтті жылжытылды, бірақ талқылау беті бірге жылжытылмады, оның себебі жаңа тақырып атында біреуі алдақашан бар.
Бұны қолмен қосыңыз.'''",
'movedto'                 => 'мынаған жылжытылды:',
'movetalk'                => 'Қауымдасты талқылау бетін жылжыту',
'move-subpages'           => 'Барлық бетшелерін жылжыту, егер қолданбалы болса',
'move-talk-subpages'      => 'Талқылау бетінің барлық бетшелерін жылжыту, егер қолданбалы болса',
'movepage-page-exists'    => '$1 деген бет алдақашан бар және үстіне өздіктік жазылмайды.',
'movepage-page-moved'     => '$1 деген бет $2 дегенге жылжытылды.',
'movepage-page-unmoved'   => '$1 деген бет $2 дегенге жылжытылмайды.',
'movepage-max-pages'      => 'Барынша $1 бет жылжытылды да мыннан көбі өздіктік жылжылтылмайды.',
'1movedto2'               => '[[$1]] дегенді [[$2]] дегенге жылжытты',
'1movedto2_redir'         => '[[$1]] дегенді [[$2]] деген айдағыш үстіне жылжытты',
'movelogpage'             => 'Жылжыту журналы',
'movelogpagetext'         => 'Төменде жылжытылған беттердің тізімі беріліп тұр.',
'movereason'              => 'Себебі:',
'revertmove'              => 'қайтару',
'delete_and_move'         => 'Жою және жылжыту',
'delete_and_move_text'    => '==Жою керек==
«[[:$1]]» деген нысана бет алдақашан бар.
Жылжытуға жол беру үшін бұны жоясыз ба?',
'delete_and_move_confirm' => 'Иә, бұл бетті жой',
'delete_and_move_reason'  => 'Жылжытуға жол беру үшін жойылған',
'selfmove'                => 'Қайнар және нысана тақырып аттары бірдей;
бет өзінің үстіне жылжытылмайды.',
'immobile_namespace'      => 'Қайнар не нысана тақырып аты арнаулы түріне жатады;
беттер бұл есім аясы сыртына және ішіне жылжытылмайды.',
'imagenocrossnamespace'   => 'Файл емес есім аясына файл жылжытылмайды',
'imagetypemismatch'       => 'Файлдың жаңа кеңейтімі бұның түріне сәйкес емес',
'imageinvalidfilename'    => 'Нысана файл атауы жарамсыз',

# Export
'export'            => 'Беттерді сыртқа беру',
'exporttext'        => 'XML пішіміне қапталған бөлек бет не беттер бумасы мәтінің және өңдеу тарихын сыртқа бере аласыз. 
MediaWiki жүйесінің [[{{#special:Import}}|сырттан алу бетін]] пайдаланып, бұны өзге уикиге алуға болады.

Беттерді сыртқа беру үшін, тақырып аттарын төмендегі мәтін жолағына енгізіңіз (жол сайын бір тақырып аты), және де бөлектеңіз: не ағымдық нұсқасын, барлық ескі нұсқалары мен және тарихы жолдары мен бірге, немесе дәл ағымдық нұсқасын, соңғы өңдемеу туралы ақпараты мен бірге.

Соңғы жағдайда сілтемені де, мысалы «{{{{ns:mediawiki}}:Mainpage}}» беті үшін [[{{#special:Export}}/{{MediaWiki:Mainpage}}]] қолдануға болады.',
'exportcuronly'     => 'Толық тарихын емес, тек ағымдық түзетуін кірістіріңіз',
'exportnohistory'   => "----
'''Аңғартпа:''' Өнімділік әсері себептерінен, беттердің толық тарихын бұл пішінмен сыртқа беруі өшірілген.",
'export-submit'     => 'Сыртқа бер',
'export-addcattext' => 'Мына санаттағы беттерді үстеу:',
'export-addcat'     => 'Үсте',
'export-download'   => 'Файл түрінде сақтау',
'export-templates'  => 'Үлгілерді қоса алып',

# Namespace 8 related
'allmessages'               => 'Жүйе хабарлары',
'allmessagesname'           => 'Атауы',
'allmessagesdefault'        => 'Әдепкі мәтіні',
'allmessagescurrent'        => 'Ағымдық мәтіні',
'allmessagestext'           => 'Мында {{ns:mediawiki}} есім аясында жетімді жүйе хабар тізімі беріледі.
Егер әмбебап MediaWiki жерсіндіруге үлес қосқыңыз келсе [http://www.mediawiki.org/wiki/Localisation MediaWiki жерсіндіру бетіне] және [http://translatewiki.net Betawiki жобасына] барып шығыңыз.',
'allmessagesnotsupportedDB' => "'''\$wgUseDatabaseMessages''' өшірілген себебінен '''{{#special:AllMessages}}''' беті қолданылмайды.",
'allmessagesfilter'         => 'Хабарды атауымен сүзгілеу:',
'allmessagesmodified'       => 'Тек өзгертілгенді көрсет',

# Thumbnails
'thumbnail-more'           => 'Үлкейту',
'filemissing'              => 'Жоғалған файл',
'thumbnail_error'          => 'Нобай құру қатесі: $1',
'djvu_page_error'          => 'DjVu беті аумақ сыртындда',
'djvu_no_xml'              => 'DjVu файлы үшін XML келтіруі икемді емес',
'thumbnail_invalid_params' => 'Нобайдың бапталымдары жарамсыз',
'thumbnail_dest_directory' => 'Нысана қалтасы құруы икемді емес',

# Special:Import
'import'                     => 'Беттерді сырттан алу',
'importinterwiki'            => 'Уики-апару үшін сырттан алу',
'import-interwiki-text'      => 'Сырттан алынатын уикиді және беттің тақырып атын бөлектеңіз.
Түзету күн-айы және өңдеуші есімдері сақталады.
Уики-апару үшін сырттан алу барлық әрекеттер [[{{#special:Log}}/import|сырттан алу журналына]] жазылып алынады.',
'import-interwiki-history'   => 'Бұл беттің барлық тарихи нұсқаларын көшіру',
'import-interwiki-submit'    => 'Сырттан алу',
'import-interwiki-namespace' => 'Беттерді мына есім аясына апару:',
'importtext'                 => 'Қайнар уикиден «{{#special:Export}}» қуралын қолданып файлды сыртқа беріңіз, дискіңізге сақтаңыз да мында қотарып беріңіз.',
'importstart'                => 'Беттерді сырттан алуда…',
'import-revision-count'      => '$1 түзету',
'importnopages'              => 'Сырттан алынатын беттер жоқ.',
'importfailed'               => 'Сырттан алу сәтсіз бітті: <nowiki>$1</nowiki>',
'importunknownsource'        => 'Cырттан алынатын қайнар түрі белгісіз',
'importcantopen'             => 'Сырттан алынатын файл ашылмайды',
'importbadinterwiki'         => 'Жарамсыз уики-аралық сілтеме',
'importnotext'               => 'Бұл бос, немесе мәтіні жоқ',
'importsuccess'              => 'Сырттан алу аяқталды!',
'importhistoryconflict'      => 'Тарихында қақтығысты түзету бар (бұл бет алдында сырттан алынған сияқты)',
'importnosources'            => 'Уики-апару үшін сырттан алынатын еш қайнар көзі анықталмаған, және тарихын тікелей қотарып беруі өшірілген.',
'importnofile'               => 'Сырттан алынған файл қотарып берілген жоқ.',
'importuploaderrorsize'      => 'Сырттан алынған файлдың қотарып берілуі сәтсіз өтті. Файл мөлшері қотарып берілуге руқсат етілгеннен асады.',
'importuploaderrorpartial'   => 'Сырттан алынған файлдың қотарып берілуі сәтсіз өтті. Осы файлдың тек бөліктері қотарылып берілді.',
'importuploaderrortemp'      => 'Сырттан алынған файлдың қотарып берілуі сәтсіз өтті. Уақытша қалта табылмады.',
'import-parse-failure'       => 'Сырттан алынған XML файл құрылымын талдатқанда сәтсіздік болды',
'import-noarticle'           => 'Сырттан алынатын еш бет жоқ!',
'import-nonewrevisions'      => 'Барлық түзетулері алдында сырттан алынған.',
'xml-error-string'           => '$1 нөмір $2 жолда, баған $3 (байт $4): $5',
'import-upload'              => 'XML деректерін қотарып беру',

# Import log
'importlogpage'                    => 'Сырттан алу журналы',
'importlogpagetext'                => 'Беттерді түзету тарихымен бірге сыртқы уикилерден әкімші ретінде алу.',
'import-logentry-upload'           => '«[[$1]]» дегенді файл қотарып беру арқылы сырттан алды',
'import-logentry-upload-detail'    => '$1 түзету',
'import-logentry-interwiki'        => 'уики-апарылған $1',
'import-logentry-interwiki-detail' => '$2 дегеннен $1 түзету',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Жеке бетім',
'tooltip-pt-anonuserpage'         => 'Бұл IP мекенжайдың жеке беті',
'tooltip-pt-mytalk'               => 'Талқылау бетім',
'tooltip-pt-anontalk'             => 'Бұл IP мекенжай өңдемелерін талқылау',
'tooltip-pt-preferences'          => 'Бапталымдарым',
'tooltip-pt-watchlist'            => 'Өзгерістерін бақылап тұрған беттер тізімім.',
'tooltip-pt-mycontris'            => 'Үлестерімдің тізімі',
'tooltip-pt-login'                => 'Кіруіңізді ұсынамыз, ол міндетті емес.',
'tooltip-pt-anonlogin'            => 'Кіруіңізді ұсынамыз, бірақ, ол міндетті емес.',
'tooltip-pt-logout'               => 'Шығу',
'tooltip-ca-talk'                 => 'Мағлұмат бетті талқылау',
'tooltip-ca-edit'                 => 'Бұл бетті өңдей аласыз. Сақтаудың алдында «Қарап шығу» батырмасын нұқыңыз.',
'tooltip-ca-addsection'           => 'Бұл талқылау бетінде жаңа тарау бастау.',
'tooltip-ca-viewsource'           => 'Бұл бет қорғалған. Қайнар көзін қарай аласыз.',
'tooltip-ca-history'              => 'Бұл беттін жуықтағы нұсқалары.',
'tooltip-ca-protect'              => 'Бұл бетті қорғау',
'tooltip-ca-delete'               => 'Бұл бетті жою',
'tooltip-ca-undelete'             => 'Бұл беттің жоюдың алдындағы болған өңдемелерін қалпына келтіру',
'tooltip-ca-move'                 => 'Бұл бетті жылжыту',
'tooltip-ca-watch'                => 'Бұл бетті бақылау тізіміңізге үстеу',
'tooltip-ca-unwatch'              => 'Бұл бетті бақылау тізіміңізден аластау',
'tooltip-search'                  => '{{SITENAME}} жобасында іздеу',
'tooltip-search-go'               => 'Егер дәл осы атауымен болса бетке өтіп кету',
'tooltip-search-fulltext'         => 'Осы мәтіні бар бетті іздеу',
'tooltip-p-logo'                  => 'Басты бетке',
'tooltip-n-mainpage'              => 'Басты бетке келіп-кетіңіз',
'tooltip-n-portal'                => 'Жоба туралы, не істеуіңізге болатын, қайдан табуға болатын туралы',
'tooltip-n-currentevents'         => 'Ағымдағы оқиғаларға қатысты өң ақпаратын табу',
'tooltip-n-recentchanges'         => 'Осы уикидегі жуықтағы өзгерістер тізімі.',
'tooltip-n-randompage'            => 'Кездейсоқ бетті жүктеу',
'tooltip-n-help'                  => 'Анықтама табу орны.',
'tooltip-t-whatlinkshere'         => 'Мында сілтеген барлық беттердің тізімі',
'tooltip-t-recentchangeslinked'   => 'Мыннан сілтенген беттердің жуықтағы өзгерістері',
'tooltip-feed-rss'                => 'Бұл беттің RSS арнасы',
'tooltip-feed-atom'               => 'Бұл беттің Atom арнасы',
'tooltip-t-contributions'         => 'Осы қатысушының үлес тізімін қарау',
'tooltip-t-emailuser'             => 'Осы қатысушыға хат жөнелту',
'tooltip-t-upload'                => 'Файлдарды қотарып беру',
'tooltip-t-specialpages'          => 'Барлық арнайы беттер тізімі',
'tooltip-t-print'                 => 'Бұл беттің басып шығарышқа арналған нұсқасы',
'tooltip-t-permalink'             => 'Мына беттің осы нұсқасының тұрақты сілтемесі',
'tooltip-ca-nstab-main'           => 'Мағлұмат бетін қарау',
'tooltip-ca-nstab-user'           => 'Қатысушы бетін қарау',
'tooltip-ca-nstab-media'          => 'Таспа бетін қарау',
'tooltip-ca-nstab-special'        => 'Бұл арнайы бет, беттің өзі өңделінбейді.',
'tooltip-ca-nstab-project'        => 'Жоба бетін қарау',
'tooltip-ca-nstab-image'          => 'Файл бетін қарау',
'tooltip-ca-nstab-mediawiki'      => 'Жүйе хабарын қарау',
'tooltip-ca-nstab-template'       => 'Үлгіні қарау',
'tooltip-ca-nstab-help'           => 'Анықтыма бетін қарау',
'tooltip-ca-nstab-category'       => 'Санат бетін қарау',
'tooltip-minoredit'               => 'Бұны шағын өңдеме деп белгілеу',
'tooltip-save'                    => 'Жасаған өзгерістеріңізді сақтау',
'tooltip-preview'                 => 'Сақтаудың алдынан жасаған өзгерістеріңізді қарап шығыңыз!',
'tooltip-diff'                    => 'Мәтінге қандай өзгерістерді жасағаныңызды қарау.',
'tooltip-compareselectedversions' => 'Беттің екі бөлектенген нұсқасы айырмасын қарау.',
'tooltip-watch'                   => 'Бұл бетті бақылау тізіміңізге үстеу',
'tooltip-recreate'                => 'Бет жойылғанына қарамастан қайта бастау',
'tooltip-upload'                  => 'Қотарып беруді бастау',

# Stylesheets
'common.css'      => '/* Мында орналастырылған CSS барлық мәнерлерде қолданылады */',
'standard.css'    => '/* Мында орналастырылған CSS тек «Дағдылы» (standard) мәнерін пайдаланушыларына ықпал етеді */',
'nostalgia.css'   => '/* Мында орналастырылған CSS тек «Аңсау» (nostalgia) мәнерін пайдаланушыларына ықпал етеді */',
'cologneblue.css' => '/* Мында орналастырылған CSS тек «Көлн зеңгірлігі» (cologneblue) мәнерін пайдаланушыларына ықпал етеді skin */',
'monobook.css'    => '/* Мында орналастырылған CSS тек «Дара кітап» (monobook) мәнерін пайдаланушыларына ықпал етеді */',
'myskin.css'      => '/* Мында орналастырылған CSS тек «Өз мәнерім» (myskin) мәнерін пайдаланушыларына ықпал етеді */',
'chick.css'       => '/* Мында орналастырылған CSS тек «Балапан» (chick) мәнерін пайдаланушыларына ықпал етеді */',
'simple.css'      => '/* Мында орналастырылған CSS тек «Кәдімгі» (simple) мәнерін пайдаланушыларына ықпал етеді */',
'modern.css'      => '/* Мында орналастырылған CSS тек «Заманауи» (modern) мәнерін пайдаланушыларына ықпал етеді */',

# Scripts
'common.js'      => '/* Мындағы әртүрлі JavaScript кез келген бет қотарылғанда барлық пайдаланушылар үшін жегіледі. */',
'standard.js'    => '/* Мындағы JavaScript тек «Дағдылы» (standard) мәнерін пайдаланушылар үшін жегіледі */',
'nostalgia.js'   => '/* Мындағы JavaScript тек «Аңсау» (nostalgia) мәнерін пайдаланушылар үшін жегіледі*/',
'cologneblue.js' => '/* Мындағы JavaScript тек «Көлн зеңгірлігі» (cologneblue) мәнерін пайдаланушылар үшін жегіледі */',
'monobook.js'    => '/* Мындағы JavaScript тек «Дара кітап» (monobook) мәнерін пайдаланушылар үшін жегіледі */',
'myskin.js'      => '/* Мындағы JavaScript тек «Өз мәнерім» (myskin) мәнерін пайдаланушылар үшін жегіледі */',
'chick.js'       => '/* Мындағы JavaScript тек «Балапан» (chick) мәнерін пайдаланушылар үшін жегіледі */',
'simple.js'      => '/* Мындағы JavaScript тек «Кәдімгі» (simple) мәнерін пайдаланушылар үшін жегіледі */',
'modern.js'      => '/* Мындағы JavaScript тек «Заманауи» (modern) мәнерін пайдаланушылар үшін жегіледі */',

# Metadata
'nodublincore'      => 'Бұл серверде «Dublin Core RDF» түрі қосымша деректері өшірілген.',
'nocreativecommons' => 'Бұл серверде «Creative Commons RDF» түрі қосымша деректері өшірілген.',
'notacceptable'     => 'Тұтынғышыңыз оқи алатын пішімі бар деректерді бұл уики сервер жетістіре алмайды.',

# Attribution
'anonymous'        => '{{SITENAME}} тіркелгісіз қатысушы(лары)',
'siteuser'         => '{{SITENAME}} қатысушы $1',
'lastmodifiedatby' => 'Бұл бетті $3 қатысушы соңғы өзгерткен кезі: $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Шығарма негізін $1 жазған.',
'others'           => 'басқалар',
'siteusers'        => '{{SITENAME}} қатысушы(лар) $1',
'creditspage'      => 'Бетті жазғандар',
'nocredits'        => 'Бұл бетті жазғандар туралы ақпарат жоқ.',

# Spam protection
'spamprotectiontitle' => '«Спам»-нан қорғайтын сүзгі',
'spamprotectiontext'  => 'Бұл беттің сақтауын «спам» сүзгісі бұғаттады.
Бұның себебі шеттік торап сілтемесінен болуы мүмкін.',
'spamprotectionmatch' => 'Келесі «спам» мәтіні сүзгіленген: $1',
'spambot_username'    => 'MediaWiki spam cleanup',
'spam_reverting'      => '$1 дегенге сілтемелері жоқ соңғы нұсқасына қайтарылды',
'spam_blanking'       => '$1 дегенге сілтемелері бар барлық түзетулер тазартылды',

# Info page
'infosubtitle'   => 'Бет туралы мәлімет',
'numedits'       => 'Өңдеме саны (бет): $1',
'numtalkedits'   => 'Өңдеме саны (талқылау беті): $1',
'numwatchers'    => 'Бақылаушы саны: $1',
'numauthors'     => 'Әртүрлі аутор саны (бет): $1',
'numtalkauthors' => 'Әртүрлі аутор саны (талқылау беті): $1',

# Math options
'mw_math_png'    => 'Әрқашан PNG пішінімен көрсеткіз',
'mw_math_simple' => 'Егер өте қарапайым болса — HTML, әйтпесе PNG',
'mw_math_html'   => 'Егер ықтимал болса — HTML, әйтпесе PNG',
'mw_math_source' => 'Бұны TeX пішімінде қалдыр (мәтіндік шолғыштарға)',
'mw_math_modern' => 'Осы заманғы шолғыштарына ұсынылады',
'mw_math_mathml' => 'Егер ықтимал болса — MathML (сынақтама)',

# Patrolling
'markaspatrolleddiff'                 => 'Зерттелді деп белгілеу',
'markaspatrolledtext'                 => 'Бұл бетті зерттелді деп белгіле',
'markedaspatrolled'                   => 'Зерттелді деп белгіленді',
'markedaspatrolledtext'               => 'Бөлектенген түзету зерттелді деп белгіленді.',
'rcpatroldisabled'                    => 'Жуықтағы өзгерістерді зерттеуі өшірілген',
'rcpatroldisabledtext'                => 'Жуықтағы өзгерістерді зерттеу мүмкіндігі ағымда өшірілген.',
'markedaspatrollederror'              => 'Зерттелді деп белгіленбейді',
'markedaspatrollederrortext'          => 'Зерттелді деп белгілеу үшін түзетуді келтіріңіз.',
'markedaspatrollederror-noautopatrol' => 'Өз жасаған өзгерістеріңізді зерттелді деп белгілей алмайсыз.',

# Patrol log
'patrol-log-page' => 'Зерттеу журналы',
'patrol-log-line' => '$2 дегеннің $1 түзетуін зерттелді деп белгіледі $3',
'patrol-log-auto' => '(өздіктік)',
'patrol-log-diff' => 'нөмір $1',

# Image deletion
'deletedrevision'                 => 'Ескі түзетуін жойды: $1',
'filedeleteerror-short'           => 'Файл жою қатесі: $1',
'filedeleteerror-long'            => 'Файлды жойғанда қателер кездесті:

$1',
'filedelete-missing'              => '«$1» файлы жойылмайды, себебі ол жоқ.',
'filedelete-old-unregistered'     => '«$1» файлдың келтірілген түзетуі дерекқорда жоқ.',
'filedelete-current-unregistered' => '«$1» файлдың келтірілген атауы дерекқорда жоқ.',
'filedelete-archive-read-only'    => '«$1» мұрағат қалтасына веб-сервер жаза алмайды.',

# Browsing diffs
'previousdiff' => '← Алдыңғы айырм.',
'nextdiff'     => 'Келесі айырм. →',

# Media information
'mediawarning'         => "'''Құлақтандыру''': Бұл файл түрінде қаскүнемді коды бар болуы ықтимал; бұны жегіп жүйеңізге зиян келтіруіңіз мүмкін.<hr />",
'imagemaxsize'         => 'Сипаттамасы бетіндегі суреттің мөлшерін шектеуі:',
'thumbsize'            => 'Нобай мөлшері:',
'widthheight'          => '$1 × $2',
'widthheightpage'      => '$1 × $2, $3 бет',
'file-info'            => 'Файл мөлшері: $1, MIME түрі: $2',
'file-info-size'       => '($1 × $2 нүкте, файл мөлшері: $3, MIME түрі: $4)',
'file-nohires'         => '<small>Жоғары ажыратылымдығы жетімсіз.</small>',
'svg-long-desc'        => '(SVG файлы, кесімді $1 × $2 нүкте, файл мөлшері: $3)',
'show-big-image'       => 'Жоғары ажыратылымды',
'show-big-image-thumb' => '<small>Қарап шығу мөлшері: $1 × $2 нүкте</small>',

# Special:NewImages
'newimages'             => 'Жаңа файлдар көрмесі',
'imagelisttext'         => "Төменде $2 сұрыпталған '''$1''' файл тізімі.",
'newimages-summary'     => 'Бұл арнайы бетінде соңғы қотарып берілген файлдар көрсетіледі',
'showhidebots'          => '(боттарды $1)',
'noimages'              => 'Көретін ештеңе жоқ.',
'ilsubmit'              => 'Ізде',
'bydate'                => 'күн-айымен',
'sp-newimages-showfrom' => '$2, $1 кезінен бері — жаңа суреттерді көрсет',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'video-dims'     => '$1, $2 × $3',
'seconds-abbrev' => 'с',
'minutes-abbrev' => 'мин',
'hours-abbrev'   => 'сағ',

# Bad image list
'bad_image_list' => 'Пішімі төмендегідей:

Тек тізім даналары (* нышанымен басталытын жолдар) есептеледі.
Жолдың бірінші сілтемесі жарамсыз суретке сілтеу жөн.
Сол жолдағы кейінгі әрбір сілтемелер ерен болып есептеледі, мысалы жол ішіндегі кездесетін суреті бар беттер.',

# Metadata
'metadata'          => 'Қосымша мәліметтер',
'metadata-help'     => 'Осы файлда қосымша мәліметтер бар. Бәлкім, осы мәліметтер файлды жасап шығару, не сандылау үшін пайдаланған сандық камера, не мәтіналғырдан алынған.
Егер осы файл негізгі күйінен өзгертілген болса, кейбір ежелелері өзгертілген фотосуретке лайық болмас.',
'metadata-expand'   => 'Егжей-тегжейін көрсет',
'metadata-collapse' => 'Егжей-тегжейін жасыр',
'metadata-fields'   => 'Осы хабарда тізімделген EXIF қосымша мәліметтер аумақтары, сурет беті көрсету кезінде қосымша мәліметтер кесте жасырылығанда кірістірледі.
Басқалары әдепкіден жасырылады.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Ені',
'exif-imagelength'                 => 'Биіктігі',
'exif-bitspersample'               => 'Құраш сайын бит саны',
'exif-compression'                 => 'Қысым сұлбасы',
'exif-photometricinterpretation'   => 'Нүкте қиысуы',
'exif-orientation'                 => 'Мегзеуі',
'exif-samplesperpixel'             => 'Құраш саны',
'exif-planarconfiguration'         => 'Дерек реттеуі',
'exif-ycbcrsubsampling'            => 'Y құрашының C құрашына жарнақтауы',
'exif-ycbcrpositioning'            => 'Y құрашы және C құрашы мекендеуі',
'exif-xresolution'                 => 'Дерелей ажыратылымдығы',
'exif-yresolution'                 => 'Тірелей ажыратылымдығы',
'exif-resolutionunit'              => 'X және Y бойынша ажыратылымдық бірлігі',
'exif-stripoffsets'                => 'Сурет дереректерінің жайғасуы',
'exif-rowsperstrip'                => 'Белдік сайын жол саны',
'exif-stripbytecounts'             => 'Қысымдалған белдік сайын байт саны',
'exif-jpeginterchangeformat'       => 'JPEG SOI дегенге ығысуы',
'exif-jpeginterchangeformatlength' => 'JPEG деректерінің байт саны',
'exif-transferfunction'            => 'Тасымалдау жетесі',
'exif-whitepoint'                  => 'Ақ нүкте түстілігі',
'exif-primarychromaticities'       => 'Алғы шептегі түстіліктері',
'exif-ycbcrcoefficients'           => 'Түс аясын тасымалдау матрицалық еселіктері',
'exif-referenceblackwhite'         => 'Қара және ақ анықтауыш қос колемдері',
'exif-datetime'                    => 'Файлдың өзгертілген күн-айы',
'exif-imagedescription'            => 'Сурет тақырыбын аты',
'exif-make'                        => 'Камера өндірушісі',
'exif-model'                       => 'Камера үлгісі',
'exif-software'                    => 'Қолданылған бағдарламалық жасақтама',
'exif-artist'                      => 'Туындыгері',
'exif-copyright'                   => 'Ауторлық құқықтар иесі',
'exif-exifversion'                 => 'Exif нұсқасы',
'exif-flashpixversion'             => 'Қолданған Flashpix нұсқасы',
'exif-colorspace'                  => 'Түс аясы',
'exif-componentsconfiguration'     => 'Әрқайсы құраш мәні',
'exif-compressedbitsperpixel'      => 'Сурет қысымдау тәртібі',
'exif-pixelydimension'             => 'Суреттің жарамды ені',
'exif-pixelxdimension'             => 'Суреттің жарамды биіктігі',
'exif-makernote'                   => 'Өндірушінің аңғартпалары',
'exif-usercomment'                 => 'Қатысушының мәндемелері',
'exif-relatedsoundfile'            => 'Қатысты дыбыс файлы',
'exif-datetimeoriginal'            => 'Жасалған кезі',
'exif-datetimedigitized'           => 'Сандықтау кезі',
'exif-subsectime'                  => 'Жасалған кезінің секунд бөлшектері',
'exif-subsectimeoriginal'          => 'Түпнұсқа кезінің секунд бөлшектері',
'exif-subsectimedigitized'         => 'Сандықтау кезінің секунд бөлшектері',
'exif-exposuretime'                => 'Ұсталым уақыты',
'exif-exposuretime-format'         => '$1 с ($2)',
'exif-fnumber'                     => 'Саңылау мөлшері',
'exif-exposureprogram'             => 'Ұсталым бағдарламасы',
'exif-spectralsensitivity'         => 'Спектр бойынша сезгіштігі',
'exif-isospeedratings'             => 'ISO жылдамдық жарнақтауы (жарық сезгіштігі)',
'exif-oecf'                        => 'Оптоелектронды түрлету ықпалы',
'exif-shutterspeedvalue'           => 'Жапқыш жылдамдылығы',
'exif-aperturevalue'               => 'Саңылаулық',
'exif-brightnessvalue'             => 'Жарықтылық',
'exif-exposurebiasvalue'           => 'Ұсталым өтемі',
'exif-maxaperturevalue'            => 'Барынша саңылау ашуы',
'exif-subjectdistance'             => 'Нысана қашықтығы',
'exif-meteringmode'                => 'Өлшеу әдісі',
'exif-lightsource'                 => 'Жарық көзі',
'exif-flash'                       => 'Жарқылдағыш',
'exif-focallength'                 => 'Шоғырлау алшақтығы',
'exif-subjectarea'                 => 'Нысана ауқымы',
'exif-flashenergy'                 => 'Жарқылдағыш қарқыны',
'exif-spatialfrequencyresponse'    => 'Кеңістік-жиілік әсершілігі',
'exif-focalplanexresolution'       => 'Х бойынша шоғырлау жайпақтықтың ажыратылымдығы',
'exif-focalplaneyresolution'       => 'Y бойынша шоғырлау жайпақтықтың ажыратылымдығы',
'exif-focalplaneresolutionunit'    => 'Шоғырлау жайпақтықтың ажыратылымдық өлшемі',
'exif-subjectlocation'             => 'Нысана орналасуы',
'exif-exposureindex'               => 'Ұсталым айқындауы',
'exif-sensingmethod'               => 'Сенсордің өлшеу әдісі',
'exif-filesource'                  => 'Файл қайнары',
'exif-scenetype'                   => 'Сахна түрі',
'exif-cfapattern'                  => 'CFA сүзгі кейіпі',
'exif-customrendered'              => 'Қосымша сурет өңдетуі',
'exif-exposuremode'                => 'Ұсталым тәртібі',
'exif-whitebalance'                => 'Ақ түсінің тендестігі',
'exif-digitalzoomratio'            => 'Сандық ауқымдау жарнақтауы',
'exif-focallengthin35mmfilm'       => '35 mm таспасының шоғырлау алшақтығы',
'exif-scenecapturetype'            => 'Түсірген сахна түрі',
'exif-gaincontrol'                 => 'Сахнаны реттеу',
'exif-contrast'                    => 'Ашықтық',
'exif-saturation'                  => 'Қанықтық',
'exif-sharpness'                   => 'Айқындық',
'exif-devicesettingdescription'    => 'Жабдық баптау сипаттамасы',
'exif-subjectdistancerange'        => 'Сахна қашықтығының көлемі',
'exif-imageuniqueid'               => 'Суреттің бірегей нөмірі (ID)',
'exif-gpsversionid'                => 'GPS белгішесінің нұсқасы',
'exif-gpslatituderef'              => 'Солтүстік немесе Оңтүстік бойлығы',
'exif-gpslatitude'                 => 'Бойлығы',
'exif-gpslongituderef'             => 'Шығыс немесе Батыс ендігі',
'exif-gpslongitude'                => 'Ендігі',
'exif-gpsaltituderef'              => 'Биіктік көрсетуі',
'exif-gpsaltitude'                 => 'Биіктік',
'exif-gpstimestamp'                => 'GPS уақыты (атом сағаты)',
'exif-gpssatellites'               => 'Өлшеуге пйдаланылған Жер серіктері',
'exif-gpsstatus'                   => 'Қабылдағыш күйі',
'exif-gpsmeasuremode'              => 'Өлшеу тәртібі',
'exif-gpsdop'                      => 'Өлшеу дәлдігі',
'exif-gpsspeedref'                 => 'Жылдамдылық өлшемі',
'exif-gpsspeed'                    => 'GPS қабылдағыштың жылдамдылығы',
'exif-gpstrackref'                 => 'Қозғалыс бағытын көрсетуі',
'exif-gpstrack'                    => 'Қозғалыс бағыты',
'exif-gpsimgdirectionref'          => 'Сурет бағытын көрсетуі',
'exif-gpsimgdirection'             => 'Сурет бағыты',
'exif-gpsmapdatum'                 => 'Пайдаланылған геодезиялық түсірме деректері',
'exif-gpsdestlatituderef'          => 'Нысана бойлығын көрсетуі',
'exif-gpsdestlatitude'             => 'Нысана бойлығы',
'exif-gpsdestlongituderef'         => 'Нысана ендігін көрсетуі',
'exif-gpsdestlongitude'            => 'Нысана ендігі',
'exif-gpsdestbearingref'           => 'Нысана азимутын көрсетуі',
'exif-gpsdestbearing'              => 'Нысана азимуты',
'exif-gpsdestdistanceref'          => 'Нысана қашықтығын көрсетуі',
'exif-gpsdestdistance'             => 'Нысана қашықтығы',
'exif-gpsprocessingmethod'         => 'GPS өңдету әдісінің атауы',
'exif-gpsareainformation'          => 'GPS аумағының атауы',
'exif-gpsdatestamp'                => 'GPS күн-айы',
'exif-gpsdifferential'             => 'GPS сараланған дұрыстау',

# EXIF attributes
'exif-compression-1' => 'Ұлғайтылған',

'exif-unknowndate' => 'Белгісіз күн-айы',

'exif-orientation-1' => 'Қалыпты', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Дерелей шағылысқан', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° бұрышқа айналған', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Тірелей шағылысқан', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Сағат тілшесіне қарсы 90° бұрышқа айналған және тірелей шағылысқан', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Сағат тілше бойынша 90° бұрышқа айналған', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Сағат тілше бойынша 90° бұрышқа айналған және тірелей шағылысқан', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Сағат тілшесіне қарсы 90° бұрышқа айналған', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'талпақ пішім',
'exif-planarconfiguration-2' => 'тайпақ пішім',

'exif-componentsconfiguration-0' => 'бар болмады',

'exif-exposureprogram-0' => 'Анықталмаған',
'exif-exposureprogram-1' => 'Қолмен',
'exif-exposureprogram-2' => 'Бағдарламалы әдіс (қалыпты)',
'exif-exposureprogram-3' => 'Саңылау басыңқылығы',
'exif-exposureprogram-4' => 'Ысырма басыңқылығы',
'exif-exposureprogram-5' => 'Өнер бағдарламасы (анықтық терендігіне санасқан)',
'exif-exposureprogram-6' => 'Қимыл бағдарламасы (жапқыш шапшандылығына санасқан)',
'exif-exposureprogram-7' => 'Тірелей әдісі (арты шоғырлаусыз таяу түсірмелер)',
'exif-exposureprogram-8' => 'Дерелей әдісі (арты шоғырланған дерелей түсірмелер)',

'exif-subjectdistance-value' => '$1 m',

'exif-meteringmode-0'   => 'Белгісіз',
'exif-meteringmode-1'   => 'Біркелкі',
'exif-meteringmode-2'   => 'Бұлдыр дақ',
'exif-meteringmode-3'   => 'БірДақты',
'exif-meteringmode-4'   => 'КөпДақты',
'exif-meteringmode-5'   => 'Өрнекті',
'exif-meteringmode-6'   => 'Жыртынды',
'exif-meteringmode-255' => 'Басқа',

'exif-lightsource-0'   => 'Белгісіз',
'exif-lightsource-1'   => 'Күн жарығы',
'exif-lightsource-2'   => 'Күнжарықты шам',
'exif-lightsource-3'   => 'Қыздырғышты шам',
'exif-lightsource-4'   => 'Жарқылдағыш',
'exif-lightsource-9'   => 'Ашық күн',
'exif-lightsource-10'  => 'Бұлынғыр күн',
'exif-lightsource-11'  => 'Көленкелі',
'exif-lightsource-12'  => 'Күнжарықты шам (D 5700–7100 K)',
'exif-lightsource-13'  => 'Күнжарықты шам (N 4600–5400 K)',
'exif-lightsource-14'  => 'Күнжарықты шам (W 3900–4500 K)',
'exif-lightsource-15'  => 'Күнжарықты шам (WW 3200–3700 K)',
'exif-lightsource-17'  => 'Қалыпты жарық қайнары A',
'exif-lightsource-18'  => 'Қалыпты жарық қайнары B',
'exif-lightsource-19'  => 'Қалыпты жарық қайнары C',
'exif-lightsource-24'  => 'Студиялық ISO күнжарықты шам',
'exif-lightsource-255' => 'Басқа жарық көзі',

'exif-focalplaneresolutionunit-2' => 'дүйм',

'exif-sensingmethod-1' => 'Анықталмаған',
'exif-sensingmethod-2' => '1-чипті аумақты түссезгіш',
'exif-sensingmethod-3' => '2-чипті аумақты түссезгіш',
'exif-sensingmethod-4' => '3-чипті аумақты түссезгіш',
'exif-sensingmethod-5' => 'Кезекті аумақты түссезгіш',
'exif-sensingmethod-7' => '3-сызықты түссезгіш',
'exif-sensingmethod-8' => 'Кезекті сызықты түссезгіш',

'exif-scenetype-1' => 'Тікелей түсірілген фотосурет',

'exif-customrendered-0' => 'Қалыпты өңдету',
'exif-customrendered-1' => 'Қосымша өңдету',

'exif-exposuremode-0' => 'Өздіктік ұсталымдау',
'exif-exposuremode-1' => 'Қолмен ұсталымдау',
'exif-exposuremode-2' => 'Өздіктік жарқылдау',

'exif-whitebalance-0' => 'Ақ түсі өздіктік тендестірілген',
'exif-whitebalance-1' => 'Ақ түсі қолмен тендестірілген',

'exif-scenecapturetype-0' => 'Қалыпталған',
'exif-scenecapturetype-1' => 'Дерелей',
'exif-scenecapturetype-2' => 'Тірелей',
'exif-scenecapturetype-3' => 'Түнгі сахна',

'exif-gaincontrol-0' => 'Жоқ',
'exif-gaincontrol-1' => 'Төмен зораю',
'exif-gaincontrol-2' => 'Жоғары зораю',
'exif-gaincontrol-3' => 'Төмен баяулау',
'exif-gaincontrol-4' => 'Жоғары баяулау',

'exif-contrast-0' => 'Қалыпты',
'exif-contrast-1' => 'Ұян',
'exif-contrast-2' => 'Тұрпайы',

'exif-saturation-0' => 'Қалыпты',
'exif-saturation-1' => 'Төмен қанықты',
'exif-saturation-2' => 'Жоғары қанықты',

'exif-sharpness-0' => 'Қалыпты',
'exif-sharpness-1' => 'Ұян',
'exif-sharpness-2' => 'Тұрпайы',

'exif-subjectdistancerange-0' => 'Белгісіз',
'exif-subjectdistancerange-1' => 'Таяу түсірілген',
'exif-subjectdistancerange-2' => 'Жақын түсірілген',
'exif-subjectdistancerange-3' => 'Алыс түсірілген',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Солтүстік бойлығы',
'exif-gpslatitude-s' => 'Оңтүстік бойлығы',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Шығыс ендігі',
'exif-gpslongitude-w' => 'Батыс ендігі',

'exif-gpsstatus-a' => 'Өлшеу ұласуда',
'exif-gpsstatus-v' => 'Өлшеу өзара әрекетте',

'exif-gpsmeasuremode-2' => '2-бағыттық өлшем',
'exif-gpsmeasuremode-3' => '3-бағыттық өлшем',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'km/h',
'exif-gpsspeed-m' => 'mil/h',
'exif-gpsspeed-n' => 'knot',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Шын бағыт',
'exif-gpsdirection-m' => 'Магнитты бағыт',

# External editor support
'edit-externally'      => 'Бұл файлды шеттік қондырма арқылы өңдеу',
'edit-externally-help' => 'Көбірек ақпарат үшін [http://www.mediawiki.org/wiki/Manual:External_editors орнату нұсқамаларын] қараңыз.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'барлығын',
'imagelistall'     => 'барлығы',
'watchlistall2'    => 'барлық',
'namespacesall'    => 'барлығы',
'monthsall'        => 'барлығы',

# E-mail address confirmation
'confirmemail'             => 'Е-пошта мекенжайын құптау',
'confirmemail_noemail'     => '[[{{#special:Preferences}}|Пайдаланушылық бапталымдарыңызда]] жарамды е-пошта мекенжайын қоймапсыз.',
'confirmemail_text'        => '{{SITENAME}} е-пошта мүмкіндіктерін пайдалану үшін алдынан е-пошта мекенжайыңыздың жарамдылығын тексеріп шығуыңыз керек.
Өзіңіздің мекенжайыңызға құптау хатын жөнелту үшін төмендегі батырманы нұқыңыз.
Хаттың ішінде коды бар сілтеме кірістірмек; 
е-пошта жайыңыздың жарамдылығын құптау үшін сілтемені шолғыштың мекенжай жолағына енгізіп ашыңыз.',
'confirmemail_pending'     => '<div class="error">Құптау коды алдақашан хатпен жіберіліген;
егер жуықта тіркелсеңіз, жаңа кодын сұрату алдынан хат келуін біршама минөт күте тұрыңыз.</div>',
'confirmemail_send'        => 'Құптау кодын жөнелту',
'confirmemail_sent'        => 'Құптау хаты жөнелтілді.',
'confirmemail_oncreate'    => 'Құптау коды е-пошта мекенжайыңызға жөнелтілді.
Бұл белгілеме кіру үдірісіне керегі жоқ, бірақ е-пошта негізіндегі уики мүмкіндіктерді қосу үшін бұны жетістіруіңіз керек.',
'confirmemail_sendfailed'  => 'Құптау хаты жөнелтілмеді.
Жарамсыз таңбалар үшін мекенжайды тексеріп шығыңыз.

Пошта жібергіштің қайтарған мәліметі: $1',
'confirmemail_invalid'     => 'Құптау коды жарамсыз.
Код мерзімі біткен шығар.',
'confirmemail_needlogin'   => 'Е-пошта мекенжайыңызды құптау үшін $1 керек.',
'confirmemail_success'     => 'Е-пошта мекенжайыңыз құпталды.
Енді уикиге кіріп жұмысқа кірісуге болады',
'confirmemail_loggedin'    => 'Е-пошта мекенжайыңыз енді құпталды.',
'confirmemail_error'       => 'Құптауңызды сақтағанда белгісіз қате болды.',
'confirmemail_subject'     => '{{SITENAME}} торабынан е-пошта мекенжайыңызды құптау хаты',
'confirmemail_body'        => 'Кейбіреу, $1 деген IP мекенжайынан, өзіңіз болуы мүмкін,
{{SITENAME}} жобасында бұл Е-пошта мекенжайын қолданып «$2» деген тіркелгі жасапты.

Бұл тіркелгі нақты сізге тән екенін құптау үшін, және {{SITENAME}} жобасының
е-пошта мүмкіндіктерін белсендіру үшін, мына сілтемені шолғышыңызбен ашыңыз:

$3

Егер бұл тіркелгіні жасаған өзіңіз *емес* болса, мына сілтемеге еріп
е-пошта мекенжайы құптауын болдырмаңыз:

$5

Құптау коды мерзімі бітетін кезі: $4.',
'confirmemail_invalidated' => 'Е-пошта мекенжайын құптауы болдырылмады',
'invalidateemail'          => 'Е-пошта мекенжайын құптауы болдырмау',

# Scary transclusion
'scarytranscludedisabled' => '[Уики-аралық кірікбеттер өшірілген]',
'scarytranscludefailed'   => '[$1 үшін үлгі келтіруі сәтсіз бітті; ғафу етіңіз]',
'scarytranscludetoolong'  => '[URL тым ұзын; ғафу етіңіз]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">Бұл беттің аңыстаулары:

$1
</div>',
'trackbackremove'   => '([$1 Жою])',
'trackbacklink'     => 'Аңыстау',
'trackbackdeleteok' => 'Аңыстау сәтті жойылды.',

# Delete conflict
'deletedwhileediting' => 'Құлақтандыру: Бұл бетті өңдеуіңізді бастағанда, осы бет жойылды!',
'confirmrecreate'     => "Бұл бетті өңдеуіңізді бастағанда [[{{ns:user}}:$1|$1]] ([[{{ns:user_talk}}:$1|талқылауы]]) осы бетті жойды, келтірген себебі:
: ''$2''
Осы бетті қайта бастауын нақты тілегеніңізді құптаңыз.",
'recreate'            => 'Қайта бастау',

'unit-pixel' => ' нүкте',

# HTML dump
'redirectingto' => '[[:$1]] бетіне айдатуда…',

# action=purge
'confirm_purge'        => 'Бұл беттін бүркемесін тазартасыз ба?

$1',
'confirm_purge_button' => 'Жарайды',

# AJAX search
'searchcontaining' => "''$1'' мағлұматы бар беттерден іздеу.",
'searchnamed'      => "''$1'' атауы бар беттерден іздеу.",
'articletitles'    => "''$1'' деп басталған беттерді",
'hideresults'      => 'Нәтижелерді жасыр',
'useajaxsearch'    => 'AJAX қолданып іздеу',

# Separators for various lists, etc.
'semicolon-separator' => ';',
'comma-separator'     => ',&#32;',
'colon-separator'     => ':&#32;',

# Multipage image navigation
'imgmultipageprev' => '← алдыңғы бетке',
'imgmultipagenext' => 'келесі бетке →',
'imgmultigo'       => 'Өт!',
'imgmultigoto'     => '$1 бетіне өту',

# Table pager
'ascending_abbrev'         => 'өсу',
'descending_abbrev'        => 'кему',
'table_pager_next'         => 'Келесі бетке',
'table_pager_prev'         => 'Алдыңғы бетке',
'table_pager_first'        => 'Алғашқы бетке',
'table_pager_last'         => 'Соңғы бетке',
'table_pager_limit'        => 'Бет сайын $1 дана көрсет',
'table_pager_limit_submit' => 'Өту',
'table_pager_empty'        => 'Еш нәтиже жоқ',

# Auto-summaries
'autosumm-blank'   => 'Беттің барлық мағлұматын аластады',
'autosumm-replace' => "Бетті '$1' дегенмен алмастырды",
'autoredircomment' => '[[$1]] дегенге айдады',
'autosumm-new'     => 'Жаңа бетте: $1',

# Size units
'size-bytes' => '$1 байт',

# Live preview
'livepreview-loading' => 'Жүктеуде…',
'livepreview-ready'   => 'Жүктеуде… Дайын!',
'livepreview-failed'  => 'Тура қарап шығу сәтсіз! Кәдімгі қарап шығу әдісін байқап көріңіз.',
'livepreview-error'   => 'Қосылу сәтсіз: $1 «$2». Кәдімгі қарап шығу әдісін байқап көріңіз.',

# Friendlier slave lag warnings
'lag-warn-normal' => '$1 секундтан жаңалау өзгерістер бұл тізімде көрсетілмеуі мүмкін.',
'lag-warn-high'   => 'Дерекқор сервері зор кешігуі себебінен, $1 секундтан жаңалау өзгерістер бұл тізімде көрсетілмеуі мүмкін.',

# Watchlist editor
'watchlistedit-numitems'       => 'Бақылау тізіміңізде, талқылау беттерсіз, $1 тақырып аты бар.',
'watchlistedit-noitems'        => 'Бақылау тізіміңізде еш тақырып аты жоқ.',
'watchlistedit-normal-title'   => 'Бақылау тізімді өңдеу',
'watchlistedit-normal-legend'  => 'Бақылау тізімінен тақырып аттарын аластау',
'watchlistedit-normal-explain' => 'Бақылау тізіміңіздегі тақырып аттар төменде көрсетіледі.
Тақырып атын аластау үшін, бүйір көзге құсбелгі салыңыз, және «Тақырып аттарын аласта» дегенді нұқыңыз.
Тағы да [[{{#special:Watchlist}}/raw|қам тізімді өңдей]] аласыз.',
'watchlistedit-normal-submit'  => 'Тақырып аттарын аласта',
'watchlistedit-normal-done'    => 'Бақылау тізіміңізден $1 тақырып аты аласталды:',
'watchlistedit-raw-title'      => 'Қам бақылау тізімді өңдеу',
'watchlistedit-raw-legend'     => 'Қам бақылау тізімді өңдеу',
'watchlistedit-raw-explain'    => 'Бақылау тізіміңіздегі тақырып аттары төменде көрсетіледі, және де тізмге үстеп және тізмден аластап өңделуі мүмкін;
жол сайын бір тақырып аты болу жөн.
Бітіргеннен соң «Бақылау тізімді жаңарту» дегенді нұқыңыз.
Тағы да [[{{#special:Watchlist}}/edit|қалыпалған өңдеуішті пайдалана]] аласыз.',
'watchlistedit-raw-titles'     => 'Тақырып аттары:',
'watchlistedit-raw-submit'     => 'Бақылау тізімді жаңарту',
'watchlistedit-raw-done'       => 'Бақылау тізіміңіз жаңартылды.',
'watchlistedit-raw-added'      => '$1 тақырып аты үстелді:',
'watchlistedit-raw-removed'    => '$1 тақырып аты аласталды:',

# Watchlist editing tools
'watchlisttools-view' => 'Қатысты өзгерістерді қарау',
'watchlisttools-edit' => 'Бақылау тізімді қарау және өңдеу',
'watchlisttools-raw'  => 'Қам бақылау тізімді өңдеу',

# Iranian month names
'iranian-calendar-m1'  => 'пыруардин',
'iranian-calendar-m2'  => 'әрдибешт',
'iranian-calendar-m3'  => 'хырдад',
'iranian-calendar-m4'  => 'тир',
'iranian-calendar-m5'  => 'мырдад',
'iranian-calendar-m6'  => 'шерияр',
'iranian-calendar-m7'  => 'мер',
'iranian-calendar-m8'  => 'абан',
'iranian-calendar-m9'  => 'азар',
'iranian-calendar-m10' => 'ди',
'iranian-calendar-m11' => 'бемін',
'iranian-calendar-m12' => 'аспанд',

# Hebrew month names
'hebrew-calendar-m1'      => 'тішри',
'hebrew-calendar-m2'      => 'xышуан',
'hebrew-calendar-m3'      => 'кіслу',
'hebrew-calendar-m4'      => 'тот',
'hebrew-calendar-m5'      => 'шыбат',
'hebrew-calendar-m6'      => 'адар',
'hebrew-calendar-m6a'     => 'адар',
'hebrew-calendar-m6b'     => 'уадар',
'hebrew-calendar-m7'      => 'нисан',
'hebrew-calendar-m8'      => 'аяр',
'hebrew-calendar-m9'      => 'сиуан',
'hebrew-calendar-m10'     => 'тымоз',
'hebrew-calendar-m11'     => 'аб',
'hebrew-calendar-m12'     => 'айлол',
'hebrew-calendar-m1-gen'  => 'тішридің',
'hebrew-calendar-m2-gen'  => 'хышуандың',
'hebrew-calendar-m3-gen'  => 'кіслудің',
'hebrew-calendar-m4-gen'  => 'тоттың',
'hebrew-calendar-m5-gen'  => 'шыбаттың',
'hebrew-calendar-m6-gen'  => 'адардың',
'hebrew-calendar-m6a-gen' => 'адардың',
'hebrew-calendar-m6b-gen' => 'уадардың',
'hebrew-calendar-m7-gen'  => 'нисанның',
'hebrew-calendar-m8-gen'  => 'аярдың',
'hebrew-calendar-m9-gen'  => 'сиуанның',
'hebrew-calendar-m10-gen' => 'тымоздың',
'hebrew-calendar-m11-gen' => 'абтың',
'hebrew-calendar-m12-gen' => 'айлолдың',

# Core parser functions
'unknown_extension_tag' => 'Танылмаған кеңейтпе белгісі «$1»',

# Special:Version
'version'                          => 'Жүйе нұсқасы', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Орнатылған кеңейтімдер',
'version-specialpages'             => 'Арнайы беттер',
'version-parserhooks'              => 'Құрылымдық талдатқыштың тұзақтары',
'version-variables'                => 'Айнымалылар',
'version-other'                    => 'Тағы басқалар',
'version-mediahandlers'            => 'Таспа өңдеткіштері',
'version-hooks'                    => 'Жете тұзақтары',
'version-extension-functions'      => 'Кеңейтімдер жетелері',
'version-parser-extensiontags'     => 'Құрылымдық талдатқыш кеңейтімдерінің белгілемері',
'version-parser-function-hooks'    => 'Құрылымдық талдатқыш жетелерінің тұзақтары',
'version-skin-extension-functions' => 'Мәнер кеңейтімдерінің жетелері',
'version-hook-name'                => 'Тұзақ атауы',
'version-hook-subscribedby'        => 'Тұзақ тартқыштары',
'version-version'                  => 'Нұсқасы:',
'version-license'                  => 'Лицензиясы',
'version-software'                 => 'Орнатылған бағдарламалық жасақтама',
'version-software-product'         => 'Өнім',
'version-software-version'         => 'Нұсқасы',

# Special:FilePath
'filepath'         => 'Файл орналасуы',
'filepath-page'    => 'Файл аты:',
'filepath-submit'  => 'Орналасуын тап',
'filepath-summary' => 'Бұл арнайы бет файл орналасуы толық жолын қайтарады.
Суреттер толық ажыратылымдығымен көрсетіледі, басқа файл түрлеріне қатысты бағдарламасы тура жегіледі.

Файл атауын «{{ns:image}}:» деген бастауышсыз еңгізіңіз.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Файл телнұсқаларын іздеу',
'fileduplicatesearch-summary'  => 'Файл хеші мағынасы негізінде телнұсқаларын іздеу.

Файл атауын «{{ns:image}}:» деген бастауышсыз енгізіңіз.',
'fileduplicatesearch-legend'   => 'Телнұсқаны іздеу',
'fileduplicatesearch-filename' => 'Файл атауы:',
'fileduplicatesearch-submit'   => 'Ізде',
'fileduplicatesearch-info'     => '$1 × $2 пиксел<br />Файл мөлшері: $3<br />MIME түрі: $4',
'fileduplicatesearch-result-1' => '«$1» файлына тең телнұсқасы жоқ.',
'fileduplicatesearch-result-n' => '«$1» файлына тең $2 телнұсқасы бар.',

# Special:SpecialPages
'specialpages'                   => 'Арнайы беттер',
'specialpages-note'              => '----
* Кәдімгі арнайы беттер.
* <span class="mw-specialpagerestricted">Шектелген арнайы беттер.</span>',
'specialpages-group-maintenance' => 'Баптау баянаттары',
'specialpages-group-other'       => 'Тағы басқа арнайы беттер',
'specialpages-group-login'       => 'Кіру / тіркелу',
'specialpages-group-changes'     => 'Жуықтағы өзгерістер мен журналдар',
'specialpages-group-media'       => 'Таспа баянаттары және қотарып берілгендер',
'specialpages-group-users'       => 'Қатысушылар және олардың құқықтары',
'specialpages-group-highuse'     => 'Өте көп қолданылған беттер',
'specialpages-group-pages'       => 'Беттер тізімі',
'specialpages-group-pagetools'   => 'Көмекші беттер',
'specialpages-group-wiki'        => 'Уики деректері және құралдары',
'specialpages-group-redirects'   => 'Айдайтын арнайы беттер',
'specialpages-group-spam'        => 'Спам құралдары',

# Special:BlankPage
'blankpage'              => 'Бос бет',
'intentionallyblankpage' => 'Бұл бет әдейі бос қалдырылған',

);
