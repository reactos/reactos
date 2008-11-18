<?php
/** Kazakh (Arabic script) (‫قازاقشا (تٴوتە)‬)
 *
 * @ingroup Language
 * @file
 *
 * @author AlefZet
 * @author GaiJin
 */

/**
 * بۇل قازاقشا تىلدەسۋىنىڭ جەرسىندىرۋ فايلى
 *
 * شەتكى پايدالانۋشىلار: بۇل فايلدى تىكەلەي وڭدەمەڭىز!
 *
 * بۇل فايلداعى وزگەرىستەر باعدارلامالىق جاساقتاما كەزەكتى جاڭارتىلعاندا جوعالتىلادى.
 * ۋىيكىيدە ٴوز باپتالىمدارىڭىزدى ىستەي الاسىز.
 * اكىمشى بوپ كىرگەنىڭىزدە, [[ارنايى:بارلىق حابارلار]] دەگەن بەتكە ٴوتىڭىز دە
 * مىندا تىزىمدەلىنگەن مەدىياۋىيكىي:* سىيپاتى بار بەتتەردى وڭدەڭىز.
 */

$fallback = 'kk-cyrl';
$rtl = true;

$digitTransformTable = array(
	'0' => '۰', # &#x06f0;
	'1' => '۱', # &#x06f1;
	'2' => '۲', # &#x06f2;
	'3' => '۳', # &#x06f3;
	'4' => '۴', # &#x06f4;
	'5' => '۵', # &#x06f5;
	'6' => '۶', # &#x06f6;
	'7' => '۷', # &#x06f7;
	'8' => '۸', # &#x06f8;
	'9' => '۹', # &#x06f9;
);

$separatorTransformTable = array(
	'.' => '٫', # &#x066b;
	',' => '٬', # &#x066c;
);

$defaultUserOptionOverrides = array(
	# Swap sidebar to right side by default
	'quickbar' => 2,
	# Underlines seriously harm legibility. Force off:
	'underline' => 0,
);

$extraUserToggles = array(
	'nolangconversion'
);

$fallback8bitEncoding = 'windows-1256';

$namespaceNames = array(
	NS_MEDIA            => 'تاسپا',
	NS_SPECIAL          => 'ارنايى',
	NS_MAIN	            => '',
	NS_TALK	            => 'تالقىلاۋ',
	NS_USER             => 'قاتىسۋشى',
	NS_USER_TALK        => 'قاتىسۋشى_تالقىلاۋى',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_تالقىلاۋى',
	NS_IMAGE            => 'سۋرەت',
	NS_IMAGE_TALK       => 'سۋرەت_تالقىلاۋى',
	NS_MEDIAWIKI        => 'مەدىياۋىيكىي',
	NS_MEDIAWIKI_TALK   => 'مەدىياۋىيكىي_تالقىلاۋى',
	NS_TEMPLATE         => 'ۇلگى',
	NS_TEMPLATE_TALK    => 'ۇلگى_تالقىلاۋى',
	NS_HELP             => 'انىقتاما',
	NS_HELP_TALK        => 'انىقتاما_تالقىلاۋى',
	NS_CATEGORY         => 'سانات',
	NS_CATEGORY_TALK    => 'سانات_تالقىلاۋى'
);

$namespaceAliases = array(
	# Aliases to kk-cyrl namespaces
	'Таспа'               => NS_MEDIA,
	'Арнайы'              => NS_SPECIAL,
	'Талқылау'            => NS_TALK,
	'Қатысушы'            => NS_USER,
	'Қатысушы_талқылауы'  => NS_USER_TALK,
	'$1_талқылауы'        => NS_PROJECT_TALK,
	'Сурет'               => NS_IMAGE,
	'Сурет_талқылауы'     => NS_IMAGE_TALK,
	'МедиаУики'           => NS_MEDIAWIKI,
	'МедиаУики_талқылауы' => NS_MEDIAWIKI_TALK,
	'Үлгі'                => NS_TEMPLATE,
	'Үлгі_талқылауы'      => NS_TEMPLATE_TALK,
	'Анықтама'            => NS_HELP,
	'Анықтама_талқылауы'  => NS_HELP_TALK,
	'Санат'               => NS_CATEGORY,
	'Санат_талқылауы'     => NS_CATEGORY_TALK,

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
);

$skinNames = array(
	'standard'    => 'داعدىلى (standard)',
	'nostalgia'   => 'اڭساۋ (nostalgia)',
	'cologneblue' => 'كولن زەڭگىرلىگى (cologneblue)',
	'monobook'    => 'دارا كىتاپ (monobook)',
	'myskin'      => 'ٴوز مانەرىم (myskin)',
	'chick'       => 'بالاپان (chick)',
	'simple'      => 'كادىمگى (simple)',
	'modern'      => 'زاماناۋىي (modern)',
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
#   Please be cautious not to delete the invisible RLM from the beginning of the strings.
	'mdy time' => '‏H:i',
	'mdy date' => '‏xg j، Y "ج."',
	'mdy both' => '‏H:i، xg j، Y "ج."',

	'dmy time' => '‏H:i',
	'dmy date' => '‏j F، Y "ج."',
	'dmy both' => '‏H:i، j F، Y "ج."',

	'ymd time' => '‏H:i',
	'ymd date' => '‏Y "ج." xg j',
	'ymd both' => '‏H:i، Y "ج." xg j',

	'yyyy-mm-dd time' => 'xnH:xni:xns',
	'yyyy-mm-dd date' => 'xnY-xnm-xnd',
	'yyyy-mm-dd both' => 'xnH:xni:xns, xnY-xnm-xnd',

	'persian time' => '‏H:i',
	'persian date' => '‏xij xiF xiY', 
	'persian both' => '‏xij xiF xiY، H:i',
	
	'hebrew time' => '‏H:i',
	'hebrew date' => '‏xjj xjF xjY',
	'hebrew both' => '‏H:i، xjj xjF xjY',

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
	'redirect'               => array( 0,    '#REDIRECT', '#ايداۋ' ),
	'notoc'                  => array( 0,    '__مازمۇنسىز__', '__مسىز__', '__NOTOC__' ),
	'nogallery'              => array( 0,    '__قويماسىز__', '__قسىز__', '__NOGALLERY__' ),
	'forcetoc'               => array( 0,    '__مازمۇنداتقىزۋ__', '__مقىزۋ__', '__FORCETOC__' ),
	'toc'                    => array( 0,    '__مازمۇنى__', '__مزمن__', '__TOC__' ),
	'noeditsection'          => array( 0,    '__بولىدىموندەمەۋ__', '__بولىموندەتكىزبەۋ__', '__NOEDITSECTION__' ),
	'currentmonth'           => array( 1,    'اعىمداعىاي', 'CURRENTMONTH' ),
	'currentmonthname'       => array( 1,    'اعىمداعىاياتاۋى', 'CURRENTMONTHNAME' ),
	'currentmonthnamegen'    => array( 1,    'اعىمداعىايىلىكاتاۋى', 'CURRENTMONTHNAMEGEN' ),
	'currentmonthabbrev'     => array( 1,    'اعىمداعىايجىيىر', 'اعىمداعىايقىسقا', 'CURRENTMONTHABBREV' ),
	'currentday'             => array( 1,    'اعىمداعىكۇن', 'CURRENTDAY' ),
	'currentday2'            => array( 1,    'اعىمداعىكۇن2', 'CURRENTDAY2' ),
	'currentdayname'         => array( 1,    'اعىمداعىكۇناتاۋى', 'CURRENTDAYNAME' ),
	'currentyear'            => array( 1,    'اعىمداعىجىل', 'CURRENTYEAR' ),
	'currenttime'            => array( 1,    'اعىمداعىۋاقىت', 'CURRENTTIME' ),
	'currenthour'            => array( 1,    'اعىمداعىساعات', 'CURRENTHOUR' ),
	'localmonth'             => array( 1,    'جەرگىلىكتىاي', 'LOCALMONTH' ),
	'localmonthname'         => array( 1,    'جەرگىلىكتىاياتاۋى', 'LOCALMONTHNAME' ),
	'localmonthnamegen'      => array( 1,    'جەرگىلىكتىايىلىكاتاۋى', 'LOCALMONTHNAMEGEN' ),
	'localmonthabbrev'       => array( 1,    'جەرگىلىكتىايجىيىر', 'جەرگىلىكتىايقىسقاشا', 'جەرگىلىكتىايقىسقا', 'LOCALMONTHABBREV' ),
	'localday'               => array( 1,    'جەرگىلىكتىكۇن', 'LOCALDAY' ),
	'localday2'              => array( 1,    'جەرگىلىكتىكۇن2', 'LOCALDAY2'  ),
	'localdayname'           => array( 1,    'جەرگىلىكتىكۇناتاۋى', 'LOCALDAYNAME' ),
	'localyear'              => array( 1,    'جەرگىلىكتىجىل', 'LOCALYEAR' ),
	'localtime'              => array( 1,    'جەرگىلىكتىۋاقىت', 'LOCALTIME' ),
	'localhour'              => array( 1,    'جەرگىلىكتىساعات', 'LOCALHOUR' ),
	'numberofpages'          => array( 1,    'بەتسانى', 'NUMBEROFPAGES' ),
	'numberofarticles'       => array( 1,    'ماقالاسانى', 'NUMBEROFARTICLES' ),
	'numberoffiles'          => array( 1,    'فايلسانى', 'NUMBEROFFILES' ),
	'numberofusers'          => array( 1,    'قاتىسۋشىسانى', 'NUMBEROFUSERS' ),
	'numberofedits'          => array( 1,    'وڭدەمەسانى', 'تۇزەتۋسانى', 'NUMBEROFEDITS' ),
	'pagename'               => array( 1,    'بەتاتاۋى', 'PAGENAME' ),
	'pagenamee'              => array( 1,    'بەتاتاۋى2', 'PAGENAMEE' ),
	'namespace'              => array( 1,    'ەسىماياسى', 'NAMESPACE' ),
	'namespacee'             => array( 1,    'ەسىماياسى2', 'NAMESPACEE' ),
	'talkspace'              => array( 1,    'تالقىلاۋاياسى', 'TALKSPACE' ),
	'talkspacee'             => array( 1,    'تالقىلاۋاياسى2', 'TALKSPACEE' ),
	'subjectspace'           => array( 1,    'تاقىرىپبەتى', 'ماقالابەتى', 'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'          => array( 1,    'تاقىرىپبەتى2', 'ماقالابەتى2', 'SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'           => array( 1,    'تولىقبەتاتاۋى', 'FULLPAGENAME' ),
	'fullpagenamee'          => array( 1,    'تولىقبەتاتاۋى2', 'FULLPAGENAMEE' ),
	'subpagename'            => array( 1,    'بەتشەاتاۋى', 'استىڭعىبەتاتاۋى', 'SUBPAGENAME' ),
	'subpagenamee'           => array( 1,    'بەتشەاتاۋى2', 'استىڭعىبەتاتاۋى2', 'SUBPAGENAMEE' ),
	'basepagename'           => array( 1,    'نەگىزگىبەتاتاۋى', 'BASEPAGENAME' ),
	'basepagenamee'          => array( 1,    'نەگىزگىبەتاتاۋى2', 'BASEPAGENAMEE' ),
	'talkpagename'           => array( 1,    'تالقىلاۋبەتاتاۋى', 'TALKPAGENAME' ),
	'talkpagenamee'          => array( 1,    'تالقىلاۋبەتاتاۋى2', 'TALKPAGENAMEE' ),
	'subjectpagename'        => array( 1,    'تاقىرىپبەتاتاۋى', 'ماقالابەتاتاۋى', 'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'       => array( 1,    'تاقىرىپبەتاتاۋى2', 'ماقالابەتاتاۋى2', 'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                    => array( 0,    'حبر:', 'MSG:' ),
	'subst'                  => array( 0,    'بادەل:', 'SUBST:' ),
	'msgnw'                  => array( 0,    'ۋىيكىيسىزحبر:', 'MSGNW:' ),
	'img_thumbnail'          => array( 1,    'نوباي', 'thumbnail', 'thumb' ),
	'img_manualthumb'        => array( 1,    'نوباي=$1', 'thumbnail=$1', 'thumb=$1'),
	'img_right'              => array( 1,    'وڭعا', 'وڭ', 'right' ),
	'img_left'               => array( 1,    'سولعا', 'سول', 'left' ),
	'img_none'               => array( 1,    'ەشقانداي', 'جوق', 'none' ),
	'img_width'              => array( 1,    '$1 نۇكتە', '$1px' ),
	'img_center'             => array( 1,    'ورتاعا', 'ورتا', 'center', 'centre' ),
	'img_framed'             => array( 1,    'سۇرمەلى', 'framed', 'enframed', 'frame' ),
	'img_frameless'          => array( 1,    'سۇرمەسىز', 'frameless' ),
	'img_page'               => array( 1,    'بەت=$1', 'بەت $1', 'page=$1', 'page $1' ),
	'img_upright'            => array( 1,    'تىكتى', 'تىكتىك=$1', 'تىكتىك $1', 'upright', 'upright=$1', 'upright $1' ),
	'img_border'             => array( 1,    'جىييەكتى', 'border' ),
	'img_baseline'           => array( 1,    'تىرەكجول', 'baseline' ),
	'img_sub'                => array( 1,    'استىلىعى', 'است', 'sub'),
	'img_super'              => array( 1,    'ۇستىلىگى', 'ۇست', 'sup', 'super', 'sup' ),
	'img_top'                => array( 1,    'ۇستىنە', 'top' ),
	'img_text_top'           => array( 1,    'ماتىن-ۇستىندە', 'text-top' ),
	'img_middle'             => array( 1,    'ارالىعىنا', 'middle' ),
	'img_bottom'             => array( 1,    'استىنا', 'bottom' ),
	'img_text_bottom'        => array( 1,    'ماتىن-استىندا', 'text-bottom' ),
	'int'                    => array( 0,    'ىشكى:', 'INT:' ),
	'sitename'               => array( 1,    'توراپاتاۋى', 'SITENAME' ),
	'ns'                     => array( 0,    'ەا:', 'ەسىمايا:', 'NS:' ),
	'localurl'               => array( 0,    'جەرگىلىكتىجاي:', 'LOCALURL:' ),
	'localurle'              => array( 0,    'جەرگىلىكتىجاي2:', 'LOCALURLE:' ),
	'server'                 => array( 0,    'سەرۆەر', 'SERVER' ),
	'servername'             => array( 0,    'سەرۆەراتاۋى', 'SERVERNAME' ),
	'scriptpath'             => array( 0,    'امىرجولى', 'SCRIPTPATH' ),
	'grammar'                => array( 0,    'سەپتىگى:', 'سەپتىك:', 'GRAMMAR:' ),
	'notitleconvert'         => array( 0,    '__تاقىرىپاتىنتۇرلەندىرگىزبەۋ__', '__تاتجوق__', '__اتاۋالماستىرعىزباۋ__', '__ااباۋ__', '__NOTITLECONVERT__', '__NOTC__' ),
	'nocontentconvert'       => array( 0,    '__ماعلۇماتىنتۇرلەندىرگىزبەۋ__', '__ماتجوق__', '__ماعلۇماتالماستىرعىزباۋ__', '__ماباۋ__', '__NOCONTENTCONVERT__', '__NOCC__' ),
	'currentweek'            => array( 1,    'اعىمداعىاپتاسى', 'اعىمداعىاپتا', 'CURRENTWEEK' ),
	'currentdow'             => array( 1,    'اعىمداعىاپتاكۇنى', 'CURRENTDOW' ),
	'localweek'              => array( 1,    'جەرگىلىكتىاپتاسى', 'جەرگىلىكتىاپتا', 'LOCALWEEK' ),
	'localdow'               => array( 1,    'جەرگىلىكتىاپتاكۇنى', 'LOCALDOW' ),
	'revisionid'             => array( 1,    'تۇزەتۋنومىرٴى', 'نۇسقانومىرٴى', 'REVISIONID' ),
	'revisionday'            => array( 1,    'تۇزەتۋكۇنى','نۇسقاكۇنى', 'REVISIONDAY' ),
	'revisionday2'           => array( 1,    'تۇزەتۋكۇنى2', 'نۇسقاكۇنى2', 'REVISIONDAY2' ),
	'revisionmonth'          => array( 1,    'تۇزەتۋايى', 'نۇسقاايى', 'REVISIONMONTH' ),
	'revisionyear'           => array( 1,    'تۇزەتۋجىلى', 'نۇسقاجىلى', 'REVISIONYEAR' ),
	'revisiontimestamp'      => array( 1,    'تۇزەتۋۋاقىتىتاڭباسى', 'نۇسقاۋاقىتتۇيىندەمەسى', 'REVISIONTIMESTAMP' ),
	'plural'                 => array( 0,    'كوپشەتۇرى:','كوپشە:', 'PLURAL:' ),
	'fullurl'                => array( 0,    'تولىقجايى:', 'تولىقجاي:', 'FULLURL:' ),
	'fullurle'               => array( 0,    'تولىقجايى2:', 'تولىقجاي2:', 'FULLURLE:' ),
	'lcfirst'                => array( 0,    'كا1:', 'كىشىارىپپەن1:', 'LCFIRST:' ),
	'ucfirst'                => array( 0,    'با1:', 'باسارىپپەن1:', 'UCFIRST:' ),
	'lc'                     => array( 0,    'كا:', 'كىشىارىپپەن:', 'LC:' ),
	'uc'                     => array( 0,    'با:', 'باسارىپپەن:', 'UC:' ),
	'raw'                    => array( 0,    'قام:', 'RAW:' ),
	'displaytitle'           => array( 1,    'كورسەتىلەتىناتاۋ', 'DISPLAYTITLE' ),
	'rawsuffix'              => array( 1,    'ق', 'R' ),
	'newsectionlink'         => array( 1,    '__جاڭابولىمسىلتەمەسى__', '__NEWSECTIONLINK__' ),
	'currentversion'         => array( 1,    'باعدارلامانۇسقاسى', 'CURRENTVERSION' ),
	'urlencode'              => array( 0,    'جايدىمۇقامداۋ:', 'URLENCODE:' ),
	'anchorencode'           => array( 0,    'جاكىردىمۇقامداۋ', 'ANCHORENCODE' ),
	'currenttimestamp'       => array( 1,    'اعىمداعىۋاقىتتۇيىندەمەسى', 'اعىمداعىۋاقىتتۇيىن', 'CURRENTTIMESTAMP' ),
	'localtimestamp'         => array( 1,    'جەرگىلىكتىۋاقىتتۇيىندەمەسى', 'جەرگىلىكتىۋاقىتتۇيىن', 'LOCALTIMESTAMP' ),
	'directionmark'          => array( 1,    'باعىتبەلگىسى', 'DIRECTIONMARK', 'DIRMARK' ),
	'language'               => array( 0,    '#تىل:', '#LANGUAGE:' ),
	'contentlanguage'        => array( 1,    'ماعلۇماتتىلى', 'CONTENTLANGUAGE', 'CONTENTLANG' ),
	'pagesinnamespace'       => array( 1,    'ەسىمايابەتسانى:', 'ەابەتسانى:', 'ايابەتسانى:', 'PAGESINNAMESPACE:', 'PAGESINNS:' ),
	'numberofadmins'         => array( 1,    'اكىمشىسانى', 'NUMBEROFADMINS' ),
	'formatnum'              => array( 0,    'سانپىشىمى', 'FORMATNUM' ),
	'padleft'                => array( 0,    'سولعاىعىس', 'سولىعىس', 'PADLEFT' ),
	'padright'               => array( 0,    'وڭعاىعىس', 'وڭىعىس', 'PADRIGHT' ),
	'special'                => array( 0,    'ارنايى', 'special' ),
	'defaultsort'            => array( 1,    'ادەپكىسۇرىپتاۋ:', 'ادەپكىساناتسۇرىپتاۋ:', 'ادەپكىسۇرىپتاۋكىلتى:', 'ادەپكىسۇرىپ:', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:' ),
	'filepath'               => array( 0,    'فايلمەكەنى:', 'FILEPATH:' ), 
	'tag'                    => array( 0,    'بەلگى', 'tag' ),
	'hiddencat'              => array( 1,    '__جاسىرىنسانات__', '__HIDDENCAT__' ),
	'pagesincategory'        => array( 1,    'ساناتتاعىبەتتەر', 'PAGESINCATEGORY', 'PAGESINCAT' ),
	'pagesize'               => array( 1,    'بەتمولشەرى', 'PAGESIZE' ),
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'شىنجىرلى_ايداعىشتار', 'شىنجىرلى_ايداتۋلار' ),
	'BrokenRedirects'           => array( 'جارامسىز_ايداعىشتار', 'جارامسىز_ايداتۋلار' ),
	'Disambiguations'           => array( 'ايرىقتى_بەتتەر' ),
	'Userlogin'                 => array( 'قاتىسۋشى_كىرۋى' ),
	'Userlogout'                => array( 'قاتىسۋشى_شىعۋى' ),
	'CreateAccount'             => array( 'جاڭا_تىركەلگى', 'تىركەلگى_جاراتۋ' ),
	'Preferences'               => array( 'باپتالىمدار', 'باپتاۋ' ),
	'Watchlist'                 => array( 'باقىلاۋ_تىزىمى' ),
	'Recentchanges'             => array( 'جۋىقتاعى_وزگەرىستەر' ),
	'Upload'                    => array( 'قوتارىپ_بەرۋ', 'قوتارۋ' ),
	'Imagelist'                 => array( 'سۋرەت_تىزىمى' ),
	'Newimages'                 => array( 'جاڭا_سۋرەتتەر' ),
	'Listusers'                 => array( 'قاتىسۋشىلار', 'قاتىسۋشى_تىزىمى' ),
	'Listgrouprights'           => array( 'توپ_قۇقىقتارى_تىزىمى' ),
	'Statistics'                => array( 'ساناق' ),
	'Randompage'                => array( 'كەزدەيسوق', 'كەزدەيسوق_بەت' ),
	'Lonelypages'               => array( 'ساياق_بەتتەر' ),
	'Uncategorizedpages'        => array( 'ساناتسىز_بەتتەر' ),
	'Uncategorizedcategories'   => array( 'ساناتسىز_ساناتتار' ),
	'Uncategorizedimages'       => array( 'ساناتسىز_سۋرەتتەر' ),
	'Uncategorizedtemplates'    => array( 'ساناتسىز_ۇلگىلەر' ),
	'Unusedcategories'          => array( 'پايدالانىلماعان_ساناتتار' ),
	'Unusedimages'              => array( 'پايدالانىلماعان_سۋرەتتەر' ),
	'Wantedpages'               => array( 'تولتىرىلماعان_بەتتەر', 'جارامسىز_سىلتەمەلەر' ),
	'Wantedcategories'          => array( 'تولتىرىلماعان_ساناتتار' ),
	'Missingfiles'              => array( 'جوق_فايلدار', 'جوق_سۋرەتتەر' ),
	'Mostlinked'                => array( 'ەڭ_كوپ_سىلتەنگەن_بەتتەر' ),
	'Mostlinkedcategories'      => array( 'ەڭ_كوپ_پايدالانىلعان_ساناتتار', 'ەڭ_كوپ_سىلتەنگەن_ساناتتار' ),
	'Mostlinkedtemplates'       => array( 'ەڭ_كوپ_پايدالانىلعان_ۇلگىلەر', 'ەڭ_كوپ_سىلتەنگەن_ۇلگىلەر' ),
	'Mostcategories'            => array( 'ەڭ_كوپ_ساناتتار_بارى' ),
	'Mostimages'                => array( 'ەڭ_كوپ_پايدالانىلعان_سۋرەتتەر', 'ەڭ_كوپ_سۋرەتتەر_بارى' ),
	'Mostrevisions'             => array( 'ەڭ_كوپ_تۇزەتىلگەن', 'ەڭ_كوپ_نۇسقالار_بارى' ),
	'Fewestrevisions'           => array( 'ەڭ_از_تۇزەتىلگەن ' ),
	'Shortpages'                => array( 'قىسقا_بەتتەر' ),
	'Longpages'                 => array( 'ۇزىن_بەتتەر', 'ۇلكەن_بەتتەر' ),
	'Newpages'                  => array( 'جاڭا_بەتتەر' ),
	'Ancientpages'              => array( 'ەسكى_بەتتەر' ),
	'Deadendpages'              => array( 'تۇيىق_بەتتەر' ),
	'Protectedpages'            => array( 'قورعالعان_بەتتەر' ),
	'Protectedtitles'           => array( 'قورعالعان_تاقىرىپتار', 'قورعالعان_اتاۋلار' ),
	'Allpages'                  => array( 'بارلىق_بەتتەر' ),
	'Prefixindex'               => array( 'ٴباستاۋىش_ٴتىزىمى' ) ,
	'Ipblocklist'               => array( 'بۇعاتتالعاندار' ),
	'Specialpages'              => array( 'ارنايى_بەتتەر' ),
	'Contributions'             => array( 'ۇلەسى' ),
	'Emailuser'                 => array( 'حات_جىبەرۋ' ),
	'Confirmemail'              => array( 'قۇپتاۋ_حات' ),
	'Whatlinkshere'             => array( 'مىندا_سىلتەگەندەر' ),
	'Recentchangeslinked'       => array( 'سىلتەنگەندەردىڭ_وزگەرىستەرى' ),
	'Movepage'                  => array( 'بەتتى_جىلجىتۋ' ),
	'Blockme'                   => array( 'وزدىكتىك_بۇعاتتاۋ', 'وزدىك_بۇعاتتاۋ', 'مەنى_بۇعاتتاۋ',),
	'Booksources'               => array( 'كىتاپ_قاينارلارى' ),
	'Categories'                => array( 'ساناتتار' ),
	'Export'                    => array( 'سىرتقا_بەرۋ' ),
	'Version'                   => array( 'نۇسقاسى' ),
	'Allmessages'               => array( 'بارلىق_حابارلار' ),
	'Log'                       => array( 'جۋرنال', 'جۋرنالدار' ),
	'Blockip'                   => array( 'جايدى_بۇعاتتاۋ', 'IP_بۇعاتتاۋ'),
	'Undelete'                  => array( 'جويۋدى_بولدىرماۋ', 'جويىلعاندى_قايتارۋ' ),
	'Import'                    => array( 'سىرتتان_الۋ' ),
	'Lockdb'                    => array( 'دەرەكقوردى_قۇلىپتاۋ' ),
	'Unlockdb'                  => array( 'دەرەكقوردى_قۇلىپتاماۋ' ),
	'Userrights'                => array( 'قاتىسۋشى_قۇقىقتارى' ),
	'MIMEsearch'                => array( 'MIME_تۇرىمەن_ىزدەۋ' ),
	'Unwatchedpages'            => array( 'باقىلانىلماعان_بەتتەر' ),
	'Listredirects'             => array( 'ٴايداتۋ_ٴتىزىمى' ),
	'Revisiondelete'            => array( 'تۇزەتۋ_جويۋ', 'نۇسقانى_جويۋ' ),
	'Unusedtemplates'           => array( 'پايدالانىلماعان_ۇلگىلەر' ),
	'Randomredirect'            => array( 'Кедейсоқ_айдағыш', 'Кедейсоқ_айдату' ),
	'Mypage'                    => array( 'جەكە_بەتىم' ),
	'Mytalk'                    => array( 'تالقىلاۋىم' ),
	'Mycontributions'           => array( 'ۇلەسىم' ),
	'Listadmins'                => array( 'اكىمشىلەر', 'اكىمشى_تىزىمى'),
	'Listbots'                  => array( 'بوتتار', 'ٴبوتتار_ٴتىزىمى' ),
	'Popularpages'              => array( 'ەڭ_كوپ_قارالعان_بەتتەر', 'ايگىلى_بەتتەر' ),
	'Search'                    => array( 'ىزدەۋ' ),
	'Resetpass'                 => array( 'قۇپىيا_سوزدى_قايتارۋ' ),
	'Withoutinterwiki'          => array( 'ۋىيكىي-ارالىقسىزدار' ),
	'MergeHistory'              => array( 'تارىيح_بىرىكتىرۋ' ),
	'Invalidateemail'           => array( 'قۇپتاماۋ_حاتى' ),
);

#-------------------------------------------------------------------
# Default messages
#-------------------------------------------------------------------

$messages = array(
# User preference toggles
'tog-underline'               => 'سىلتەمەنىڭ استىن سىز:',
'tog-highlightbroken'         => 'جارامسىز سىلتەمەلەردى <a href="" class="new">بىلاي سىيياقتى</a> پىشىمدە (بالاماسى: بىلاي سىيياقتى<a href="" class="internal">?</a>).',
'tog-justify'                 => 'ەجەلەردى ەنى بويىنشا تۋرالاۋ',
'tog-hideminor'               => 'جۋىقتاعى وزگەرىستەردەن شاعىن وڭدەمەلەردى جاسىر',
'tog-extendwatchlist'         => 'باقىلاۋ ٴتىزىمدى ۇلعايت (بارلىق جارامدى وزگەرىستەردى كورسەت)',
'tog-usenewrc'                => 'كەڭەيتىلگەن جۋىقتاعى وزگەرىستەر (JavaScript)',
'tog-numberheadings'          => 'باس جولداردى وزدىكتىك نومىرلە',
'tog-showtoolbar'             => 'وڭدەۋ قۋرالدار جولاعىن كورسەت (JavaScript)',
'tog-editondblclick'          => 'قوس نۇقىمداپ وڭدەۋ (JavaScript)',
'tog-editsection'             => 'بولىمدەردى [وڭدەۋ] سىلتەمەسىمەن وڭدەۋىن قوس',
'tog-editsectiononrightclick' => 'ٴبولىم تاقىرىبىن وڭ نۇقۋمەن وڭدەۋىن قوس (JavaScript)',
'tog-showtoc'                 => 'مازمۇنىن كورسەت (3-تەن ارتا ٴبولىمى بارىلارعا)',
'tog-rememberpassword'        => 'كىرگەنىمدى وسى كومپيۋتەردە ۇمىتپا',
'tog-editwidth'               => 'كىرىستىرۋ ورنى تولىق ەنىمەن',
'tog-watchcreations'          => 'مەن باستاعان بەتتەردى باقىلاۋ تىزىمىمە ۇستە',
'tog-watchdefault'            => 'مەن وڭدەگەن بەتتەردى باقىلاۋ تىزىمىمە ۇستە',
'tog-watchmoves'              => 'مەن جىلجىتقان بەتتەردى باقىلاۋ تىزىمىمە ۇستە',
'tog-watchdeletion'           => 'مەن جويعان بەتتەردى باقىلاۋ تىزىمىمە ۇستە',
'tog-minordefault'            => 'ادەپكىدەن بارلىق وڭدەمەلەردى شاعىن دەپ بەلگىلە',
'tog-previewontop'            => 'قاراپ شىعۋ اۋماعى كىرىستىرۋ ورنى الدىندا',
'tog-previewonfirst'          => 'ٴبىرىنشى وڭدەگەندە قاراپ شىعۋ',
'tog-nocache'                 => 'بەت بۇركەمەلەۋىن ٴوشىر',
'tog-enotifwatchlistpages'    => 'باقىلانعان بەت وزگەرگەندە ماعان حات جىبەر',
'tog-enotifusertalkpages'     => 'تالقىلاۋىم وزگەرگەندە ماعان حات جىبەر',
'tog-enotifminoredits'        => 'شاعىن وڭدەمە تۋرالى دا ماعان حات جىبەر',
'tog-enotifrevealaddr'        => 'ە-پوشتامنىڭ مەكەنجايىن ەسكەرتۋ حاتتاردا اش',
'tog-shownumberswatching'     => 'باقىلاپ تۇرعان قاتىسۋشىلاردىڭ سانىن كورسەت',
'tog-fancysig'                => 'قام قولتاڭبا (وزدىكتىك سىلتەمەسىز)',
'tog-externaleditor'          => 'شەتتىك وڭدەۋىشتى ادەپكىدەن قولدان (تەك ساراپشىلار ٴۇشىن, كومپيۋتەرىڭىزدە ارناۋلى باپتالىمدار كەرەك)',
'tog-externaldiff'            => 'شەتتىك ايىرماعىشتى ادەپكىدەن قولدان (تەك ساراپشىلار ٴۇشىن, كومپيۋتەرىڭىزدە ارناۋلى باپتالىمدار كەرەك)',
'tog-showjumplinks'           => '«ٴوتىپ كەتۋ» قاتىناۋ سىلتەمەلەرىن قوس',
'tog-uselivepreview'          => 'تۋرا قاراپ شىعۋدى قولدانۋ (JavaScript) (سىناقتاما)',
'tog-forceeditsummary'        => 'وڭدەمەنىڭ قىسقاشا مازمۇنداماسى بوس قالعاندا ماعان ەسكەرت',
'tog-watchlisthideown'        => 'وڭدەمەلەرىمدى باقىلاۋ تىزىمنەن جاسىر',
'tog-watchlisthidebots'       => 'بوت وڭدەمەلەرىن باقىلاۋ تىزىمنەن جاسىر',
'tog-watchlisthideminor'      => 'شاعىن وڭدەمەلەردى باقىلاۋ تىزىمىندە كورسەتپە',
'tog-nolangconversion'        => 'ٴتىل ٴتۇرى اۋدارىسىن ٴوشىر',
'tog-ccmeonemails'            => 'باسقا قاتىسۋشىعا جىبەرگەن حاتىمنىڭ كوشىرمەسىن ماعان دا جونەلت',
'tog-diffonly'                => 'ايىرما استىندا بەت ماعلۇماتىن كورسەتپە',
'tog-showhiddencats'          => 'جاسىرىن ساناتتاردى كورسەت',

'underline-always'  => 'ارقاشان',
'underline-never'   => 'ەشقاشان',
'underline-default' => 'شولعىش بويىنشا',

'skinpreview' => '(قاراپ شىعۋ)',

# Dates
'sunday'        => 'جەكسەنبى',
'monday'        => 'دۇيسەنبى',
'tuesday'       => 'سەيسەنبى',
'wednesday'     => 'سارسەنبى',
'thursday'      => 'بەيسەنبى',
'friday'        => 'جۇما',
'saturday'      => 'سەنبى',
'sun'           => 'جەك',
'mon'           => 'ٴدۇي',
'tue'           => 'بەي',
'wed'           => 'ٴسار',
'thu'           => 'بەي',
'fri'           => 'جۇم',
'sat'           => 'سەن',
'january'       => 'قاڭتار',
'february'      => 'اقپان',
'march'         => 'ناۋرىز',
'april'         => 'cٴاۋىر',
'may_long'      => 'مامىر',
'june'          => 'ماۋسىم',
'july'          => 'شىلدە',
'august'        => 'تامىز',
'september'     => 'قىركۇيەك',
'october'       => 'قازان',
'november'      => 'قاراشا',
'december'      => 'جەلتوقسان',
'january-gen'   => 'قاڭتاردىڭ',
'february-gen'  => 'اقپاننىڭ',
'march-gen'     => 'ناۋرىزدىڭ',
'april-gen'     => 'ٴساۋىردىڭ',
'may-gen'       => 'مامىردىڭ',
'june-gen'      => 'ماۋسىمنىڭ',
'july-gen'      => 'شىلدەنىڭ',
'august-gen'    => 'تامىزدىڭ',
'september-gen' => 'قىركۇيەكتىڭ',
'october-gen'   => 'قازاننىڭ',
'november-gen'  => 'قاراشانىڭ',
'december-gen'  => 'جەلتوقساننىڭ',
'jan'           => 'قاڭ',
'feb'           => 'اقپ',
'mar'           => 'ناۋ',
'apr'           => 'cٴاۋ',
'may'           => 'مام',
'jun'           => 'ماۋ',
'jul'           => 'ٴشىل',
'aug'           => 'تام',
'sep'           => 'قىر',
'oct'           => 'قاز',
'nov'           => 'قار',
'dec'           => 'جەل',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|سانات|ساناتتار}}',
'category_header'                => '«$1» ساناتىنداعى بەتتەر',
'subcategories'                  => 'ساناتشالار',
'category-media-header'          => '«$1» ساناتىنداعى تاسپالار',
'category-empty'                 => "''بۇل ساناتتا اعىمدا ەش بەت نە تاسپا جوق.''",
'hidden-categories'              => '{{PLURAL:$1|جاسىرىن سانات|جاسىرىن ساناتتار}}',
'hidden-category-category'       => 'جاسىرىن ساناتتار', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|بۇل ساناتتا تەك كەلەسى ساناتشا بار.|بۇل ساناتتا كەلەسى $1 ساناتشا بار (نە بارلىعى $2).}}',
'category-subcat-count-limited'  => 'بۇل ساناتتا كەلەسى $1 ساناتشا بار.',
'category-article-count'         => '{{PLURAL:$2|بۇل ساناتتا تەك كەلەسى بەت بار.|بۇل ساناتتا كەلەسى $1 بەت بار (نە بارلىعى $2).}}',
'category-article-count-limited' => 'اعىمداعى ساناتتا كەلەسى $1 بەت بار.',
'category-file-count'            => '{{PLURAL:$2|بۇد ساناتتا تەك كەلەسى فايل بار.|بۇل ساناتتا كەلەسى $1 فايل بار (نە بارلىعى $2).}}',
'category-file-count-limited'    => 'اعىمداعى ساناتتا كەلەسى $1 فايل بار.',
'listingcontinuesabbrev'         => '(جالع.)',

'mainpagetext'      => "<big>'''مەدىياۋىيكىي بۋماسى ٴساتتى ورناتىلدى.'''</big>",
'mainpagedocfooter' => 'ۋىيكىي باعدارلامالىق جاساقتاماسىن قالاي قولداناتىن اقپاراتى ٴۇشىن [http://meta.wikimedia.org/wiki/Help:Contents پايدالانۋشىلىق نۇسقاۋلارىنان] كەڭەس الىڭىز.

== باستاۋ ٴۇشىن ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings باپتالىم قالاۋلارىنىڭ ٴتىزىمى]
* [http://www.mediawiki.org/wiki/Manual:FAQ مەدىياۋىيكىيدىڭ جىيى قويىلعان ساۋالدارى]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce مەدىياۋىيكىي شىعۋ تۋرالى حات تاراتۋ ٴتىزىمى]',

'about'          => 'جوبا تۋرالى',
'article'        => 'ماعلۇمات بەتى',
'newwindow'      => '(جاڭا تەرەزەدە)',
'cancel'         => 'بولدىرماۋ',
'qbfind'         => 'تابۋ',
'qbbrowse'       => 'شولۋ',
'qbedit'         => 'وڭدەۋ',
'qbpageoptions'  => 'بۇل بەت',
'qbpageinfo'     => 'اينالا',
'qbmyoptions'    => 'بەتتەرىم',
'qbspecialpages' => 'ارنايى بەتتەر',
'moredotdotdot'  => 'كوبىرەك…',
'mypage'         => 'جەكە بەتىم',
'mytalk'         => 'تالقىلاۋىم',
'anontalk'       => 'IP تالقىلاۋى',
'navigation'     => 'شارلاۋ',
'and'            => 'جانە',

# Metadata in edit box
'metadata_help' => 'قوسىمشا دەرەكتەر:',

'errorpagetitle'    => 'قاتەلىك',
'returnto'          => '$1 دەگەنگە قايتا كەلۋ.',
'tagline'           => '{{GRAMMAR:ablative|{{SITENAME}}}}',
'help'              => 'انىقتاما',
'search'            => 'ىزدەۋ',
'searchbutton'      => 'ىزدە',
'go'                => 'ٴوتۋ',
'searcharticle'     => 'ٴوت!',
'history'           => 'بەت تارىيحى',
'history_short'     => 'تارىيحى',
'updatedmarker'     => 'سوڭعى كەلىپ-كەتۋىمنەن بەرى جاڭالانعان',
'info_short'        => 'مالىمەت',
'printableversion'  => 'باسىپ شىعارۋ ٴۇشىن',
'permalink'         => 'تۇراقتى سىلتەمە',
'print'             => 'باسىپ شىعارۋ',
'edit'              => 'وڭدەۋ',
'create'            => 'باستاۋ',
'editthispage'      => 'بەتتى وڭدەۋ',
'create-this-page'  => 'جاڭا بەت باستاۋ',
'delete'            => 'جويۋ',
'deletethispage'    => 'بەتتى جويۋ',
'undelete_short'    => '$1 وڭدەمە جويۋىن بولدىرماۋ',
'protect'           => 'قورعاۋ',
'protect_change'    => 'قورعاۋدى وزگەرتۋ',
'protectthispage'   => 'بەتتى قورعاۋ',
'unprotect'         => 'قورعاماۋ',
'unprotectthispage' => 'بەتتى قورعاماۋ',
'newpage'           => 'جاڭا بەت',
'talkpage'          => 'بەتتى تالقىلاۋ',
'talkpagelinktext'  => 'تالقىلاۋى',
'specialpage'       => 'ارنايى بەت',
'personaltools'     => 'جەكە قۇرالدار',
'postcomment'       => 'ماندەمە جونەلتۋ',
'articlepage'       => 'ماعلۇمات بەتىن قاراۋ',
'talk'              => 'تالقىلاۋ',
'views'             => 'كورىنىس',
'toolbox'           => 'قۇرالدار',
'userpage'          => 'قاتىسۋشى بەتىن قاراۋ',
'projectpage'       => 'جوبا بەتىن قاراۋ',
'imagepage'         => 'تاسپا بەتىن قاراۋ',
'mediawikipage'     => 'حابار بەتىن قاراۋ',
'templatepage'      => 'ۇلگى بەتىن قاراۋ',
'viewhelppage'      => 'انىقتاما بەتىن قاراۋ',
'categorypage'      => 'سانات بەتىن قاراۋ',
'viewtalkpage'      => 'تالقىلاۋ بەتىن قاراۋ',
'otherlanguages'    => 'باسقا تىلدەردە',
'redirectedfrom'    => '($1 بەتىنەن ايداتىلعان)',
'redirectpagesub'   => 'ايداتۋ بەتى',
'lastmodifiedat'    => 'بۇل بەتتىڭ وزگەرتىلگەن سوڭعى كەزى: $2, $1.', # $1 date, $2 time
'viewcount'         => 'بۇل بەت $1 رەت قاتىنالعان.',
'protectedpage'     => 'قورعالعان بەت',
'jumpto'            => 'مىندا ٴوتۋ:',
'jumptonavigation'  => 'باعىتتاۋ',
'jumptosearch'      => 'ىزدەۋ',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} تۋرالى',
'aboutpage'            => 'Project:جوبا تۋرالى',
'bugreports'           => 'قاتەلىك باياناتتارى',
'bugreportspage'       => 'Project:قاتەلىك ەسەپتەمەلەرى',
'copyright'            => 'ماعلۇمات $1 شارتىمەن جەتىمدى.',
'copyrightpagename'    => '{{SITENAME}} اۋتورلىق قۇقىقتارى',
'copyrightpage'        => '{{ns:project}}:اۋتورلىق قۇقىقتار',
'currentevents'        => 'اعىمداعى وقىيعالار',
'currentevents-url'    => 'Project:اعىمداعى وقىيعالار',
'disclaimers'          => 'جاۋاپكەرشىلىكتەن باس تارتۋ',
'disclaimerpage'       => 'Project:جاۋاپكەرشىلىكتەن باس تارتۋ',
'edithelp'             => 'وندەۋ انىقتاماسى',
'edithelppage'         => 'Help:وڭدەۋ',
'faq'                  => 'ٴجىيى قويىلعان ساۋالدار',
'faqpage'              => 'Project:ٴجىيى قويىلعان ساۋالدار',
'helppage'             => 'Help:مازمۇنى',
'mainpage'             => 'باستى بەت',
'mainpage-description' => 'باستى بەت',
'policy-url'           => 'Project:ەرەجەلەر',
'portal'               => 'قاۋىم پورتالى',
'portal-url'           => 'Project:قاۋىم پورتالى',
'privacy'              => 'جەكە قۇپىيياسىن ساقتاۋ',
'privacypage'          => 'Project:جەكە قۇپىيياسىن ساقتاۋ',

'badaccess'        => 'رۇقسات قاتەسى',
'badaccess-group0' => 'سۇراتىلعان ارەكەتىڭىزدى جەگۋىڭىزگە رۇقسات ەتىلمەيدى.',
'badaccess-group1' => 'سۇراتىلعان ارەكەتىڭىز $1 توبىنىڭ قاتىسۋشىلارىنا شەكتەلەدى.',
'badaccess-group2' => 'سۇراتىلعان ارەكەتىڭىز $1 توپتارى ٴبىرىنىڭ قاتۋسىشىلارىنا شەكتەلەدى.',
'badaccess-groups' => 'سۇراتىلعان ارەكەتىڭىز $1 توپتارى ٴبىرىنىڭ قاتۋسىشىلارىنا شەكتەلەدى.',

'versionrequired'     => 'MediaWiki $1 نۇسقاسى كەرەك',
'versionrequiredtext' => 'بۇل بەتتى قولدانۋ ٴۇشىن MediaWiki $1 نۇسقاسى كەرەك. [[{{ns:special}}:Version|جۇيە نۇسقاسى بەتىن]] قاراڭىز.',

'ok'                      => 'جارايدى',
'pagetitle'               => '$1 — {{SITENAME}}',
'retrievedfrom'           => '«$1» بەتىنەن الىنعان',
'youhavenewmessages'      => 'سىزگە $1 بار ($2).',
'newmessageslink'         => 'جاڭا حابارلار',
'newmessagesdifflink'     => 'سوڭعى وزگەرىسىنە',
'youhavenewmessagesmulti' => '$1 دەگەندە جاڭا حابارلار بار',
'editsection'             => 'وڭدەۋ',
'editold'                 => 'وڭدەۋ',
'viewsourceold'           => 'قاينار كوزىن قاراۋ',
'editsectionhint'         => 'مىنا ٴبولىمدى وڭدەۋ: $1',
'toc'                     => 'مازمۇنى',
'showtoc'                 => 'كورسەت',
'hidetoc'                 => 'جاسىر',
'thisisdeleted'           => '$1 قارايسىز با, نە قالپىنا كەلتىرەسىز بە?',
'viewdeleted'             => '$1 قارايسىز با?',
'restorelink'             => 'جويىلعان $1 وڭدەمەنى',
'feedlinks'               => 'ارنا:',
'feed-invalid'            => 'جارامسىز جازىلىمدى ارنا ٴتۇرى.',
'feed-unavailable'        => '{{SITENAME}} جوباسىندا تاراتىلاتىن ارنالار جوق',
'site-rss-feed'           => '$1 RSS ارناسى',
'site-atom-feed'          => '$1 Atom ارناسى',
'page-rss-feed'           => '«$1» — RSS ارناسى',
'page-atom-feed'          => '«$1» — Atom ارناسى',
'red-link-title'          => '$1 (ٴالى جازىلماعان)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'بەت',
'nstab-user'      => 'جەكە بەت',
'nstab-media'     => 'تاسپا بەتى',
'nstab-special'   => 'ارنايى',
'nstab-project'   => 'جوبا بەتى',
'nstab-image'     => 'فايل بەتى',
'nstab-mediawiki' => 'حابار',
'nstab-template'  => 'ۇلگى',
'nstab-help'      => 'انىقتاما',
'nstab-category'  => 'سانات',

# Main script and global functions
'nosuchaction'      => 'مىناداي ەش ارەكەت جوق',
'nosuchactiontext'  => 'وسى URL جايىمەن ەنگىزىلگەن ارەكەتتى وسى ۋىيكىي جورامالداپ بىلمەدى.',
'nosuchspecialpage' => 'مىناداي ەش ارنايى بەت جوق',
'nospecialpagetext' => "<big>'''جارامسىز ارنايى بەتتى سۇرادىڭىز.'''</big>

جارامدى ارنايى بەت ٴتىزىمىن [[{{#special:Specialpages}}|{{int:specialpages}}]] دەگەننەن تابا الاسىز.",

# General errors
'error'                => 'قاتە',
'databaseerror'        => 'دەرەكقور قاتەسى',
'dberrortext'          => 'دەرەكقور سۇرانىمىندا سويلەم جۇيەسىنىڭ قاتەسى بولدى.
بۇل باعدارلامالىق جاساقتاما قاتەسىن بەلگىلەۋى مۇمكىن.
سوڭعى بولعان دەرەكقور سۇرانىمى:
<blockquote><tt>$1</tt></blockquote>
مىنا جەتەدەن «<tt>$2</tt>».
MySQL قايتارعان قاتەسى «<tt>$3: $4</tt>».',
'dberrortextcl'        => 'دەرەكقور سۇرانىمىندا سويلەم جۇيەسىنىڭ قاتەسى بولدى.
سوڭعى بولعان دەرەكقور سۇرانىمى:
«$1»
مىنا جەتەدەن: «$2».
MySQL قايتارعان قاتەسى «$3: $4»',
'noconnect'            => 'عافۋ ەتىڭىز! بۇل ۋىيكىيدە كەيبىر تەحنىيكالىق قىيىنشىلىقتار كەزدەستى, جانە دە دەرەكقور سەرۆەرىنە بايلانىسا المايدى.<br />
$1',
'nodb'                 => '$1 دەگەن دەرەكقور بولەكتەنبەدى',
'cachederror'          => 'تومەندە سۇرالعان بەتتىڭ بۇركەمەلەنگەن كوشىرمەسى بەرىلەدى, ٴالى جاڭارتىلماعان بولۋى مۇمكىن.',
'laggedslavemode'      => 'قۇلاقتاندىرۋ: بەتتە جۋىقتاعى جاڭالاۋلار بولماۋى مۇمكىن.',
'readonly'             => 'دەرەكقورى قۇلىپتالعان',
'enterlockreason'      => 'قۇلىپتاۋ سەبەبىن, قاي ۋاقىتقا دەيىن قۇلىپتالعانىن كىرىستىرىپ, ەنگىزىڭىز',
'readonlytext'         => 'بۇل دەرەكقور جاڭادان جازۋ جانە باسقا وزگەرىستەر جاساۋدان اعىمدا قۇلىپتالىنعان, مۇمكىن كۇندە-كۇن دەرەكقوردى باپتاۋ ٴۇشىن, بۇنى بىتىرگەننەن سوڭ قالىپتى ىسكە قايتارىلادى.

قۇلىپتاعان اكىمشى بۇنى بىلاي تۇسىندىرەدى: $1',
'missing-article'      => 'بار بولۋى ٴجون بىلاي اتالعان بەت ٴماتىنى دەرەكقوردا تابىلمادى: «$1» $2.

بۇل ەسكىرگەن ايىرما سىلتەمەسىنە نەمەسە جويىلعان بەت تارىيحى سىلتەمەسىنە ەرگەننەن بولا بەرەدى.

ەگەر بۇل ورىندى بولماسا, باعدارلامالىق جاساقتاماداعى قاتەگە تاپ بولۋىڭىز مۇمكىن.
بۇل تۋرالى ناقتى URL جايىنا اڭعارتپا جاساپ, اكىمشىگە باياناتتاڭىز.',
'missingarticle-rev'   => '(تۇزەتۋ ن-ٴى: $1)',
'missingarticle-diff'  => '(ايرم.: $1, $2)',
'readonly_lag'         => 'جەتەك دەرەكقور سەرۆەرلەر باسقىسىمەن قاداملانعاندا وسى دەرەكقور وزدىكتىك قۇلىپتالىنعان',
'internalerror'        => 'ىشكى قاتە',
'internalerror_info'   => 'ىشكى قاتەسى: $1',
'filecopyerror'        => '«$1» فايلى «$2» فايلىنا كوشىرىلمەدى.',
'filerenameerror'      => '«$1» فايل اتاۋى «$2» اتاۋىنا وزگەرتىلمەدى.',
'filedeleteerror'      => '«$1» فايلى جويىلمايدى.',
'directorycreateerror' => '«$1» قالتاسى قۇرىلمادى.',
'filenotfound'         => '«$1» فايلى تابىلمادى.',
'fileexistserror'      => '«$1» فايلعا جازۋ ىيكەمدى ەمەس: فايل بار',
'unexpected'           => 'كۇتىلمەگەن ماعىنا: «$1» = «$2».',
'formerror'            => 'قاتەلىك: ٴپىشىن جونەلتىلمەيدى',
'badarticleerror'      => 'وسىنداي ارەكەت مىنا بەتتە اتقارىلمايدى.',
'cannotdelete'         => 'ايتىلمىش بەت نە سۋرەت جويىلمايدى.
بۇنى باسقا بىرەۋ الداقاشان جويعان مۇمكىن.',
'badtitle'             => 'جارامسىز تاقىرىپ اتى',
'badtitletext'         => 'سۇرالعان بەت تاقىرىبىنىڭ اتى جارامسىز, بوس, تىلارالىق سىلتەمەسى نە ۋىيكىي-ارالىق تاقىرىپ اتى بۇرىس ەنگىزىلگەن.
مىندا تاقىرىپ اتىندا قولدالمايتىن بىرقاتار تاڭبالار بولۋى مۇمكىن.',
'perfdisabled'         => 'عافۋ ەتىڭىز! بۇل مۇمكىندىك, دەرەكقوردىڭ جىلدامىلىعىنا اسەر ەتىپ, ەشكىمگە ۋىيكىيدى پايدالانۋعا بەرمەگەسىن, ۋاقىتشا وشىرىلگەن.',
'perfcached'           => 'كەلەسى دەرەك بۇركەمەلەنگەن, سوندىقتان تولىقتاي جاڭالانماعان بولۋى مۇمكىن.',
'perfcachedts'         => 'كەلەسى دەرەك بۇركەمەلەنگەن, سوڭعى جاڭالانلعان كەزى: $1.',
'querypage-no-updates' => 'بۇل بەتتىڭ جاڭارتىلۋى اعىمدا وشىرىلگەن. دەرەكتەرى قازىر وزگەرتىلمەيدى.',
'wrong_wfQuery_params' => 'wfQuery() فۋنكتسىيياسى ٴۇشىن بۇرىس باپتالىمدارى بار<br />
جەتە: $1<br />
سۇرانىم: $2',
'viewsource'           => 'قاينار كوزىن قاراۋ',
'viewsourcefor'        => '$1 دەگەن ٴۇشىن',
'actionthrottled'      => 'ارەكەت باسەڭدەتىلدى',
'actionthrottledtext'  => 'سپامعا قارسى كۇرەس ەسەبىندە, وسى ارەكەتتى قىسقا ۋاقىتتا تىم كوپ رەت ورىنداۋىڭىز شەكتەلىندى, جانە بۇل شەكتەۋ شاماسىنان اسىپ كەتكەنسىز.
بىرنەشە ٴمىينوتتان قايتا بايقاپ كورىڭىز.',
'protectedpagetext'    => 'وڭدەۋدى قاقپايلاۋ ٴۇشىن بۇل بەت قۇلىپتالىنعان.',
'viewsourcetext'       => 'بۇل بەتتىڭ قاينار كوزىن قاراۋىڭىزعا جانە كوشىرىپ الۋڭىزعا بولادى:',
'protectedinterface'   => 'بۇل بەت باعدارلامالىق جاساقتامانىڭ تىلدەسۋ ٴماتىنىن جەتىستىرەدى, سوندىقتان قىيياناتتى قاقپايلاۋ ٴۇشىن وزگەرتۋى قۇلىپتالعان.',
'editinginterface'     => "'''قۇلاقتاندىرۋ:''' باعدارلامالىق جاساقتامانىڭ تىلدەسۋ ٴماتىنىن جەتىستىرەتىن بەتىن وڭدەپ جاتىرسىز.
بۇل بەتتىڭ وزگەرتۋى باسقا قاتىسۋشىلارعا پايدالانۋشىلىق تىلدەسۋى قالاي كورىنەتىنە اسەر ەتەدى.
اۋدارمالار ٴۇشىن, MediaWiki باعدارلاماسىن جەرسىندىرۋ [http://translatewiki.net/wiki/Main_Page?setlang=kk Betawiki جوباسى] ارقىلى قاراپ شەشىڭىز.",
'sqlhidden'            => '(SQL سۇرانىمى جاسىرىلعان)',
'cascadeprotected'     => 'بۇل بەت وڭدەۋدەن قورعالعان, سەبەبى بۇل كەلەسى «باۋلى قورعاۋى» قوسىلعان {{PLURAL:$1|بەتتىڭ|بەتتەردىڭ}} كىرىكبەتى:
$2',
'namespaceprotected'   => "'''$1''' ەسىم اياسىنداعى بەتتەردى وڭدەۋ ٴۇشىن رۇقساتىڭىز جوق.",
'customcssjsprotected' => 'بۇل بەتتى وڭدەۋگە رۇقساتىڭىز جوق, سەبەبى مىندا وزگە قاتىسۋشىنىڭ جەكە باپتاۋلارى بار.',
'ns-specialprotected'  => '{{ns:special}} ەسىم اياسىنداعى بەتتەر وڭدەلىنبەيدى',
'titleprotected'       => "بۇل تاقىرىپ اتى باستاۋدان [[{{ns:user}}:$1|$1]] قورعادى.
كەلتىرىلگەن سەبەبى: ''$2''.",

# Login and logout pages
'logouttitle'                => 'قاتىسۋشى شىعۋى',
'logouttext'                 => '<strong>ەندى جۇيەدەن شىقتىڭىز.</strong>

جۇيەگە كىرمەستەن {{SITENAME}} جوباسىن پايدالانۋىن جالعاستىرا الاسىز, نەمەسە ٴدال سول نە وزگە قاتىسۋشى بوپ قايتا كرۋىڭىز مۇمكىن.
اڭعارتپا: كەيبىر بەتتەر شولعىشىڭىزدىڭ بۇركەمەسىن تازارتقانشا دەيىن ٴالى دە كىرپ قالعانىڭىزداي كورىنۋى مۇمكىن.',
'welcomecreation'            => '== قوش كەلدىڭىز, $1! ==
جاڭا تىركەلگىڭىز جاسالدى.
{{SITENAME}} باپتالىمدارىڭىزدى وزگەرتۋىن ۇمىتپاڭىز.',
'loginpagetitle'             => 'قاتىسۋشى كىرۋى',
'yourname'                   => 'قاتىسۋشى اتىڭىز:',
'yourpassword'               => 'قۇپىييا ٴسوزىڭىز:',
'yourpasswordagain'          => 'قۇپىييا ٴسوزدى قايتالاڭىز:',
'remembermypassword'         => 'مەنىڭ كىرگەنىمدى بۇل كومپيۋتەردە ۇمىتپا',
'yourdomainname'             => 'جەلى ۇيشىگىڭىز:',
'externaldberror'            => 'وسى ارادا نە شەتتىك راستاۋ دەرەكقورىندا قاتە بولدى, نەمەسە شەتتىك تىركەلگىڭىزدى جاڭالاۋ رۇقساتى جوق.',
'loginproblem'               => '<b>كىرۋىڭىز كەزىندە وسىندا قىيىندىققا تاپ بولدىق.</b><br />قايتا بايقاپ كورىڭىز.',
'login'                      => 'كىرۋ',
'nav-login-createaccount'    => 'كىرۋ / تىركەلگى جاساۋ',
'loginprompt'                => '{{SITENAME}} تورابىنا كىرۋىڭىز ٴۇشىن «cookies» قوسىلۋى ٴجون.',
'userlogin'                  => 'كىرۋ / تىركەلگى جاساۋ',
'logout'                     => 'شىعۋ',
'userlogout'                 => 'شىعۋ',
'notloggedin'                => 'كىرمەگەنسىز',
'nologin'                    => 'كىرمەگەنسىز بە? $1.',
'nologinlink'                => 'تىركەلگى جاساڭىز',
'createaccount'              => 'جاڭا تىركەلگى',
'gotaccount'                 => 'الداقاشان تىركەلگىىڭىز بار ما? $1.',
'gotaccountlink'             => 'كىرىڭىز',
'createaccountmail'          => 'ە-پوشتامەن',
'badretype'                  => 'ەنگىزگەن قۇپىييا سوزدەرىڭىز ٴبىر بىرىنە سايكەس ەمەس.',
'userexists'                 => 'ەنگىزگەن قاتىسۋشى اتىڭىز الداقاشان پايدالانۋدا.
وزگە اتاۋدى تاڭداڭىز.',
'youremail'                  => 'ە-پوشتاڭىز:',
'username'                   => 'قاتىسۋشى اتىڭىز:',
'uid'                        => 'قاتىسۋشى تەڭدەستىرگىشىڭىز:',
'prefs-memberingroups'       => 'كىرگەن {{PLURAL:$1|توبىڭىز|توپتارىڭىز}}:',
'yourrealname'               => 'ناقتى اتىڭىز:',
'yourlanguage'               => 'ٴتىلىڭىز:',
'yourvariant'                => 'ٴتىل/ٴجازبا نۇسقاڭىز:',
'yournick'                   => 'قولتاڭباڭىز:',
'badsig'                     => 'قام قولتاڭباڭىز جارامسىز; HTML بەلگىشەلەرىن تەكسەرىڭىز.',
'badsiglength'               => 'لاقاپ اتىڭىز تىم ۇزىن;
بۇل $1 تاڭبادان اسپاۋى ٴجون.',
'email'                      => 'ە-پوشتاڭىز',
'prefs-help-realname'        => 'ناقتى اتىڭىز مىندەتتى ەمەس.
ەگەر بۇنى جەتىستىرۋدى تاڭداساڭىز, بۇل تۇزەتۋىڭىزدىڭ اۋتورلىعىن انىقتاۋ ٴۇشىن قولدانىلادى.',
'loginerror'                 => 'كىرۋ قاتەسى',
'prefs-help-email'           => 'ە-پوشتا مەكەنجايى مىندەتتى ەمەس, بىراق جەكە باسىڭىزدى اشپاي «قاتىسۋشى» نەمەسە «قاتىسۋشى_تالقىلاۋى» دەگەن بەتتەرىڭىز ارقىلى بارشا سىزبەن بايلانىسا الادى.',
'prefs-help-email-required'  => 'ە-پوشتا مەكەنجايى كەرەك.',
'nocookiesnew'               => 'جاڭا قاتىسۋشى تىركەلگىسى جاسالدى, بىراق كىرمەگەنسىز.
قاتىسۋشى كىرۋ ٴۇشىن {{SITENAME}} تورابىندا «cookie» فايلدارى قولدانىلادى.
سىزدە «cookies» وشىرىلگەن.
سونى قوسىڭىز دا جاڭا قاتىسۋشى اتىڭىزدى جانە قۇپىييا ٴسوزىڭىزدى ەنگىزىپ كىرىڭىز.',
'nocookieslogin'             => 'قاتىسۋشى كىرۋ ٴۇشىن {{SITENAME}} تورابىندا «cookies» دەگەن قولدانىلادى.
سىزدە «cookies» وشىرىلگەن.
سونى قوسىڭىز دا كىرۋدى قايتا بايقاپ كورىڭىز.',
'noname'                     => 'جارامدى قاتىسۋشى اتىن ەنگىزبەدىڭىز.',
'loginsuccesstitle'          => 'كىرۋىڭىز ٴساتتى ٴوتتى',
'loginsuccess'               => "'''ٴسىز ەندى {{SITENAME}} جوباسىنا «$1» رەتىندە كىرىپ وتىرسىز.'''",
'nosuchuser'                 => 'مىندا «$1» دەپ اتالعان قاتىسۋشى جوق.
ەملەڭىزدى تەكسەرىڭىز, نە جاڭا تىركەلگى جاساڭىز.',
'nosuchusershort'            => 'مىندا «$1» دەپ اتالعان قاتىسۋشى جوق.
ەملەڭىزدى تەكسەرىڭىز.',
'nouserspecified'            => 'قاتىسۋشى اتىن كەلتىرۋىڭىز ٴجون.',
'wrongpassword'              => 'بۇرىس قۇپىييا ٴسوز ەنگىزىلگەن. قايتا بايقاپ كورىڭىز.',
'wrongpasswordempty'         => 'قۇپىييا ٴسوز بوس بولعان. قايتا بايقاپ كورىڭىز.',
'passwordtooshort'           => 'قۇپىييا ٴسوزىڭىز جارامسىز نە تىم قىسقا.
بۇندا ەڭ كەمىندە $1 تاڭبا بولۋى جانە دە قاتىسۋشى اتىڭىزدان وزگە بولۋى ٴجون.',
'mailmypassword'             => 'قۇپىييا ٴسوزىمدى حاتپەن جىبەر',
'passwordremindertitle'      => '{{SITENAME}} ٴۇشىن جاڭا ۋاقىتشا قۇپىييا ٴسوز',
'passwordremindertext'       => 'كەيبىرەۋ (IP مەكەنجايى: $1, بالكىم ٴوزىڭىز بولارسىز)
سىزگە {{SITENAME}} ٴۇشىن جاڭا قۇپىييا ٴسوز جونەلەتۋىن بىزدەن سۇراعان ($4).
«$2» قاتىسۋشىنىڭ قۇپىييا ٴسوزى «$3» بولدى ەندى.
قازىر كىرۋىڭىز جانە قۇپىييا ٴسوزدى وزگەرتۋىڭىز كەرەك.

ەگەر بۇل سۇرانىمدى باسقا بىرەۋ ىستەسە, نە قۇپىييا ٴسوزدى ەسكە ٴتۇسىرسىپ ەندى وزگەرتكىڭىز كەلمەسە, ەسكى قۇپىييا ٴسوز قولدانۋىن جاعاستىرىپ وسى حاتقا اڭعارماۋىڭىزعا دا بولادى.',
'noemail'                    => 'وسى ارادا «$1» قاتىسۋشىنىڭ ە-پوشتا مەكەنجايى جوق.',
'passwordsent'               => 'جاڭا قۇپىييا ٴسوز «$1» ٴۇشىن تىركەلگەن ە-پوشتا مەكەنجايىنا جونەلتىلدى.
قابىلداعاننان كەيىن كىرگەندە سونى ەنگىزىڭىز.',
'blocked-mailpassword'       => 'IP مەكەنجايىڭىزدان وڭدەۋ بۇعاتتالعان, سوندىقتان قىيياناتتى قاقپايلاۋ ٴۇشىن قۇپىييا ٴسوزدى قالپىنا كەلتىرۋ جەتەسىن قولدانۋىنا رۇقسات ەتىلمەيدى.',
'eauthentsent'               => 'قۇپتاۋ حاتى ايتىلمىش ە-پوشتا مەكەنجايىنا جونەلتىلدى.
باسقا ە-پوشتا حاتىن جونەلتۋ الدىنان, تىركەلگى شىنىنان سىزدىكى ەكەنىن قۇپتاۋ ٴۇشىن حاتتاعى نۇسقامالارعا ەرىۋڭىز ٴجون.',
'throttled-mailpassword'     => 'سوڭعى {{PLURAL:$1|ساعاتتا|$1 ساعاتتا}} قۇپىييا ٴسوز ەسكەرتۋ حاتى الداقاشان جونەلتىلدى.
قىيياناتتى قاقپايلاۋ ٴۇشىن, {{PLURAL:$1|ساعات|$1 ساعات}} سايىن تەك ٴبىر عانا قۇپىييا ٴسوز ەسكەرتۋ حاتى جونەلتىلەدى.',
'mailerror'                  => 'حات جونەلتۋ قاتەسى: $1',
'acct_creation_throttle_hit' => 'عافۋ ەتىڭىز, ٴسىز الداقاشان $1 رەت تىركەلگى جاساپسىز.
ونان ارتىق ىستەي المايسىز.',
'emailauthenticated'         => 'ە-پوشتا مەكەنجايىڭىز راستالعان كەزى: $1.',
'emailnotauthenticated'      => 'ە-پوشتا مەكەنجايىڭىز ٴالى راستالعان جوق.
كەلەسى ٴاربىر مۇمكىندىكتەر ٴۇشىن ەش حات جونەلتىلمەيدى.',
'noemailprefs'               => 'وسى مۇمكىندىكتەر ىستەۋى ٴۇشىن ە-پوشتا مەكەنجايىڭىزدى ەنگىزىڭىز.',
'emailconfirmlink'           => 'ە-پوشتا مەكەنجايىڭىزدى قۇپتاڭىز',
'invalidemailaddress'        => 'وسى ە-پوشتا مەكەنجايىندا جارامسىز ٴپىشىم بولعان, قابىل ەتىلمەيدى.
دۇرىس پىشىمدەلگەن مەكەنجايدى ەنگىزىڭىز, نە اۋماقتى بوس قالدىرىڭىز.',
'accountcreated'             => 'جاڭا تىركەلگى جاسالدى',
'accountcreatedtext'         => '$1 ٴۇشىن جاڭا قاتىسۋشى تىركەلگىسى جاسالدى.',
'createaccount-title'        => '{{SITENAME}} ٴۇشىن تىركەلۋ',
'createaccount-text'         => 'كەيبىرەۋ ە-پوشتا مەكەنجايىڭىزدى پايدالانىپ {{SITENAME}} جوباسىندا ($4) «$2» اتاۋىمەن, «$3» قۇپىييا سوزىمەن تىركەلگى جاساعان.
جوباعا كىرىۋىڭىز جانە قۇپىييا ٴسوزىڭىزدى وزگەرتۋىڭىز ٴتىيىستى.

ەگەر بۇل تىركەلگى قاتەلىكپەن جاسالسا, وسى حابارعا ەلەمەۋىڭىز مۇمكىن.',
'loginlanguagelabel'         => 'ٴتىل: $1',

# Password reset dialog
'resetpass'               => 'تىركەلگىنىڭ قۇپىييا ٴسوزىن وزگەرتۋ',
'resetpass_announce'      => 'حاتپەن جىبەرىلگەن ۋاقىتشا كودىمەن كىرگەنسىز.
كىرۋىڭىزدى ٴبىتىرۋ ٴۇشىن, جاڭا قۇپىييا ٴسوزىڭىزدى مىندا ەنگىزۋىڭىز ٴجون:',
'resetpass_header'        => 'قۇپىييا ٴسوزدى وزگەرتۋ',
'resetpass_submit'        => 'قۇپىييا ٴسوزدى قويىڭىز دا كىرىڭىز',
'resetpass_success'       => 'قۇپىييا ٴسوزىڭىز ٴساتتى وزگەرتىلدى! ەندى كىرىڭىز…',
'resetpass_bad_temporary' => 'ۋاقىتشا قۇپىييا ٴسوز جارامسىز.
مۇمكىن قۇپىييا ٴسوزىڭىزدى الداقاشان ٴساتتى وزگەرتكەن بولارسىز نەمەسە جاڭا ۋاقىتشا قۇپىييا ٴسوزىن سۇراتىلعانسىز.',
'resetpass_forbidden'     => '{{SITENAME}} جوباسىندا قۇپىييا سوزدەر وزگەرتىلمەيدى',
'resetpass_missing'       => 'ەش ٴپىشىن دەرەكتەرى جوق.',

# Edit page toolbar
'bold_sample'     => 'جۋان ٴماتىن',
'bold_tip'        => 'جۋان ٴماتىن',
'italic_sample'   => 'قىيعاش ٴماتىن',
'italic_tip'      => 'قىيعاش ٴماتىن',
'link_sample'     => 'سىلتەمە تاقىرىبىن اتى',
'link_tip'        => 'ىشكى سىلتەمە',
'extlink_sample'  => 'http://www.example.com سىلتەمە تاقىرىبىن اتى',
'extlink_tip'     => 'شەتتىك سىلتەمە (الدىنان http:// ەنگىزۋىن ۇمىتپاڭىز)',
'headline_sample' => 'باس جول ٴماتىنى',
'headline_tip'    => '2-ٴشى دەڭگەيلى باس جول',
'math_sample'     => 'ورنەكتى مىندا ەنگىزىڭىز',
'math_tip'        => 'ماتەماتىيكا ورنەگى (LaTeX)',
'nowiki_sample'   => 'پىشىمدەلىنبەگەن ٴماتىندى مىندا ەنگىزىڭىز',
'nowiki_tip'      => 'ۋىيكىي ٴپىشىمىن ەلەمەۋ',
'image_tip'       => 'ەندىرىلگەن فايل',
'media_tip'       => 'فايل سىلتەمەسى',
'sig_tip'         => 'قولتاڭباڭىز جانە ۋاقىت بەلگىسى',
'hr_tip'          => 'دەرەلەي سىزىق (ۇنەمدى قولدانىڭىز)',

# Edit pages
'summary'                          => 'قىسقاشا مازمۇنداماسى',
'subject'                          => 'تاقىرىبى/باس جولى',
'minoredit'                        => 'بۇل شاعىن وڭدەمە',
'watchthis'                        => 'بەتتى باقىلاۋ',
'savearticle'                      => 'بەتتى ساقتا!',
'preview'                          => 'قاراپ شىعۋ',
'showpreview'                      => 'قاراپ شىق',
'showlivepreview'                  => 'تۋرا قاراپ شىق',
'showdiff'                         => 'وزگەرىستەردى كورسەت',
'anoneditwarning'                  => "'''قۇلاقتاندىرۋ:''' ٴسىز جۇيەگە كىرمەگەنسىز.
IP مەكەنجايىڭىز بۇل بەتتىڭ تۇزەتۋ تارىيحىندا جازىلىپ الىنادى.",
'missingsummary'                   => "'''ەسكەرتپە:''' وڭدەمەنىڭ قىسقاشا مازمۇنداماسىن ەنگىزبەپسىز.
«ساقتاۋ» تۇيمەسىن تاعى باسساڭىز, وڭدەنمەڭىز ماندەمەسىز ساقتالادى.",
'missingcommenttext'               => 'ماندەمەڭىزدى تومەندە ەنگىزىڭىز.',
'missingcommentheader'             => "'''ەسكەرتپە:''' بۇل ماندەمەگە تاقىرىپ/باسجول جەتىستىرمەپسىز.
ەگەر تاعى دا ساقتاۋ تۇيمەسىن نۇقىساڭىز, وڭدەمەڭىز سولسىز ساقتالادى.",
'summary-preview'                  => 'قىسقاشا مازمۇنداماسىن قاراپ شىعۋ',
'subject-preview'                  => 'تاقىرىبىن/باس جولىن قاراپ شىعۋ',
'blockedtitle'                     => 'قاتىسۋشى بۇعاتتالعان',
'blockedtext'                      => "<big>'''قاتىسۋشى اتىڭىز نە IP مەكەنجايىڭىز بۇعاتتالعان.'''</big>

وسى بۇعاتتاۋدى $1 ىستەگەن. كەلتىرىلگەن سەبەبى: ''$2''.

* بۇعاتتاۋ باستالعانى: $8
* بۇعاتتاۋ بىتەتىنى: $6
* بۇعاتتاۋ ماقساتى: $7

وسى بۇعاتتاۋدى تالقىلاۋ ٴۇشىن $1 دەگەنمەن, نە وزگە [[{{{{ns:mediawiki}}:grouppage-sysop}}|اكىمشىمەن]] قاتىناسۋىڭىزعا بولادى.
[[{{#special:Preferences}}|تىركەلگىڭىز باپتالىمدارىن]] قولدانىپ جارامدى ە-پوشتا مەكەنجايىن ەنگىزگەنشە دەيىن جانە بۇنى پايدالانۋى بۇعاتتالماعانشا دەيىن «قاتىسۋشىعا حات جازۋ» مۇمكىندىگىن قولدانا المايسىز.
اعىمدىق IP مەكەنجايىڭىز: $3, جانە بۇعاتاۋ ٴنومىرى: $5. سونىڭ بىرەۋىن, نەمەسە ەكەۋىن دە ٴاربىر سۇرانىمىڭىزعا كىرىستىرىڭىز.",
'autoblockedtext'                  => "$1 دەگەن بۇرىن وزگە قاتىسۋشى پايدالانعان بولعاسىن وسى IP مەكەنجايىڭىز وزدىكتىك بۇعاتتالعان.
كەلتىرىلگەن سەبەبى:

:''$2''

* بۇعاتتاۋ باستالعانى: $8
* بۇعاتتاۋ بىتەتىنى: $6

وسى بۇعاتتاۋدى تالقىلاۋ ٴۇشىن $1 دەگەنمەن, نە باسقا [[{{{{ns:mediawiki}}:grouppage-sysop}}|اكىمشىمەن]] قاتىناسۋىڭىزعا بولادى.

اڭعارتپا: [[{{#special:Preferences}}|پايدالانۋشىلىق باپتالىمدارىڭىزدى]] قولدانىپ جارامدى ە-پوشتا مەكەنجايىن ەنگىزگەنشە دەيىن جانە بۇنى پايدالانۋى بۇعاتتالماعانشا دەيىن «قاتىسۋشىعا حات جازۋ» مۇمكىندىگىن قولدانا المايسىز. 

بۇعاتاۋ ٴنومىرىڭىز: $5.
بۇل ٴنومىردى ٴاربىر سۇرانىمىڭىزدارعا كىرىستىرىڭىز.",
'blockednoreason'                  => 'ەش سەبەبى كەلتىرىلمەگەن',
'blockedoriginalsource'            => "'''$1''' دەگەننىڭ قاينار كوزى تومەندە كورسەتىلەدى:",
'blockededitsource'                => "'''$1''' دەگەنگە جاسالعان '''وڭدەمەلەرىڭىزدىڭ''' ٴماتىنى تومەندە كورسەتىلەدى:",
'whitelistedittitle'               => 'وڭدەۋ ٴۇشىن كىرۋىڭىز ٴجون.',
'whitelistedittext'                => 'بەتتەردى وڭدەۋ ٴۇشىن $1 ٴجون.',
'confirmedittitle'                 => 'قۇپتاۋ حاتى قايتا وڭدەلۋى ٴجون',
'confirmedittext'                  => 'بەتتەردى وڭدەۋ ٴۇشىن الدىن الا ە-پوشتا مەكەنجايىڭىزدى قۇپتاۋىڭىز ٴجون.
ە-پوشتا مەكەنجايىڭىزدى [[{{#special:Preferences}}|پايدالانۋشىلىق باپتالىمدارىڭىز]] ارقىلى قويىڭىز دا جارامدىلىعىن تەكسەرىپ شىعىڭىز.',
'nosuchsectiontitle'               => 'وسىنداي ەش ٴبولىم جوق',
'nosuchsectiontext'                => 'جوق ٴبولىمدى وڭدەۋدى تالاپ ەتىپسىز.
مىندا $1 دەگەن ٴبولىم جوق ەكەن, وڭدەمەڭىزدى ساقتاۋ ٴۇشىن ورىن جوق.',
'loginreqtitle'                    => 'كىرۋىڭىز كەرەك',
'loginreqlink'                     => 'كىرۋ',
'loginreqpagetext'                 => 'باسقا بەتتەردى كورۋ ٴۇشىن ٴسىز $1 بولۋىڭىز ٴجون.',
'accmailtitle'                     => 'قۇپىييا ٴسوز جونەلتىلدى.',
'accmailtext'                      => '$2 جايىنا «$1» قۇپىييا ٴسوزى جونەلتىلدى.',
'newarticle'                       => '(جاڭا)',
'newarticletext'                   => 'سىلتەمەگە ەرىپ ٴالى باستالماعان بەتكە كەلىپسىز.
بەتتى باستاۋ ٴۇشىن, تومەندەگى كىرىستىرۋ ورنىندا ٴماتىنىڭىزدى تەرىڭىز (كوبىرەك اقپارات ٴۇشىن [[{{{{ns:mediawiki}}:helppage}}|انىقتاما بەتىن]] قاراڭىز).
ەگەر جاڭىلعاننان وسىندا كەلگەن بولساڭىز, شولعىشىڭىز «ارتقا» دەگەن باتىرماسىن نۇقىڭىز.',
'anontalkpagetext'                 => "----''بۇل تىركەلگىسىز (نەمەسە تىركەلگىسىن قولدانباعان) قاتىسۋشى تالقىلاۋ بەتى. وسى قاتىسۋشىنى ٴبىز تەك ساندىق IP مەكەنجايىمەن تەڭدەستىرەمىز.
وسىنداي IP مەكەنجاي بىرنەشە قاتىسۋشىعا ورتاقتاستىرىلعان بولۋى مۇمكىن.
ەگەر ٴسىز تىركەلگىسىز قاتىسۋشى بولساڭىز جانە سىزگە قاتىسسىز ماندەمەلەر جىبەرىلگەنىن سەزسەڭىز, باسقا تىركەلگىسىز قاتىسۋشىلارمەن ارالاستىرماۋى ٴۇشىن [[{{#special:Userlogin}}|تىركەلىڭىز نە كىرىڭىز]].''",
'noarticletext'                    => 'بۇل بەتتە اعىمدا ەش ٴماتىن جوق, باسقا بەتتەردەن وسى بەت اتاۋىن [[Special:Search/{{PAGENAME}}|ىزدەپ كورۋىڭىزگە]] نەمەسە وسى بەتتى [{{fullurl:{{FULLPAGENAME}}|action=edit}} تۇزەتۋىڭىزگە] بولادى.',
'userpage-userdoesnotexist'        => '«$1» قاتىسۋشى تىركەلگىسى جازىپ الىنباعان. بۇل بەتتى باستاۋ/وڭدەۋ تالابىڭىزدى تەكسەرىپ شىعىڭىز.',
'clearyourcache'                   => "'''اڭعارتپا:''' ساقتاعاننان كەيىن, وزگەرىستەردى كورۋ ٴۇشىن شولعىش بۇركەمەسىن وراعىتۋ ىقتىيمال. '''Mozilla / Firefox / Safari:''' ''قايتا جۇكتەۋ'' باتىرماسىن نۇقىعاندا ''Shift'' تۇتىڭىز, نە ''Ctrl-Shift-R'' باسىڭىز (Apple Mac — ''Cmd-Shift-R''); '''IE:''' ''جاڭارتۋ'' باتىرماسىن نۇقىعاندا ''Ctrl'' تۇتىڭىز, نە ''Ctrl-F5'' باسىڭىز; '''Konqueror:''': ''جاڭارتۋ'' باتىرماسىن جاي نۇقىڭىز, نە ''F5'' باسىڭىز; '''Opera''' پايدانۋشىلارى ''قۇرالدار→باپتالىمدار'' دەگەنگە بارىپ بۇركەمەسىن تولىق تازارتۋ ٴجون.",
'usercssjsyoucanpreview'           => '<strong>اقىل-كەڭەس:</strong> جاڭا CSS/JS فايلىن ساقتاۋ الدىندا «قاراپ شىعۋ» باتىرماسىن قولدانىپ سىناقتاڭىز.',
'usercsspreview'                   => "'''مىناۋ CSS ٴماتىنىن تەك قاراپ شىعۋ ەكەنىن ۇمىتپاڭىز, ول ٴالى ساقتالعان جوق!'''",
'userjspreview'                    => "'''مىناۋ JavaScript قاتىسۋشى باعدارلاماسىن تەكسەرۋ/قاراپ شىعۋ ەكەنىن ۇمىتپاڭىز, ول ٴالى ساقتالعان جوق!'''",
'userinvalidcssjstitle'            => "'''قۇلاقتاندىرۋ:''' وسى ارادا «$1» دەگەن ەش مانەر جوق.
قاتىسۋشىنىڭ .css جانە .js فايل اتاۋى كىشى ارىپپپەن جازىلۋ ٴتىيىستى ەكەنىن ۇمىتپاڭىز, مىسالعا {{ns:user}}:Foo/monobook.css دەگەندى {{ns:user}}:Foo/Monobook.css دەگەنمەن سالىستىرىپ قاراڭىز.",
'updated'                          => '(جاڭارتىلعان)',
'note'                             => '<strong>اڭعارتپا:</strong>',
'previewnote'                      => '<strong>مىناۋ تەك قاراپ شىعۋ ەكەنىن ۇمىتپاڭىز;
وزگەرىستەر ٴالى ساقتالعان جوق!</strong>',
'previewconflict'                  => 'بۇل قاراپ شىعۋ بەتى جوعارعى كىرىستىرۋ ورنىنداعى ٴماتىندى قامتىيدى دا جانە ساقتالعانداعى ٴوڭدى كورسەتپەك.',
'session_fail_preview'             => '<strong>عافۋ ەتىڭىز! سەسسىييا دەرەكتەرى جوعالۋى سالدارىنان وڭدەمەڭىزدى بىتىرە المايمىز.
قايتا بايقاپ كورىڭىز. ەگەر بۇل ٴالى ىستەلمەسە, شىعۋدى جانە قايتا كىرۋدى بايقاپ كورىڭىز.</strong>',
'session_fail_preview_html'        => "<strong>عافۋ ەتىڭىز! سەسسىييا دەرەكتەرى جوعالۋى سالدارىنان وڭدەمەڭىزدى بىتىرە المايمىز.</strong>

''{{SITENAME}} جوباسىندا قام HTML قوسىلعان, JavaScript شابۋىلداردان قورعانۋ ٴۇشىن الدىن الا قاراپ شىعۋ جاسىرىلعان.''

<strong>ەگەر بۇل وڭدەمە ادال تالاپ بولسا, قايتا بايقاپ كورىڭىز. ەگەر بۇل ٴالى ىستەمەسە, شىعۋدى جانە قايتا كىرۋدى بايقاپ كورىڭىز.</strong>",
'token_suffix_mismatch'            => '<strong>وڭدەمەڭىز تايدىرىلدى, سەبەبى تۇتىنعىشىڭىز وڭدەمە دەرەكتەر بۋماسىنداعى تىنىس بەلگىلەرىن ٴبۇلدىرتتى.
بەت ٴماتىنى بۇلىنبەۋ ٴۇشىن وڭدەمەڭىز تايدىرىلادى.
بۇل كەي ۋاقىتتا قاتەسى تولعان ۆەب-نەگىزىندە تىركەلۋى جوق پروكسىي-سەرۆەردى پايدالانعان بولۋى مۇمكىن.</strong>',
'editing'                          => 'وڭدەلۋدە: $1',
'editingsection'                   => 'وڭدەلۋدە: $1 (ٴبولىمى)',
'editingcomment'                   => 'وڭدەلۋدە: $1 (ماندەمەسى)',
'editconflict'                     => 'وڭدەمە قاقتىعىسى: $1',
'explainconflict'                  => "وسى بەتتى ٴسىز وڭدەي باستاعاندا باسقا بىرەۋ بەتتى وزگەرتكەن.
جوعارعى كىرىستىرۋ ورنىندا بەتتىڭ اعىمدىق ٴماتىنى بار.
تومەنگى كىرىستىرۋ ورنىندا ٴسىز وزگەرتكەن ٴماتىنى كورسەتىلەدى.
وزگەرتۋىڭىزدى اعىمدىق ماتىنگە ۇستەۋىڭىز ٴجون.
«بەتتى ساقتا! باتىرماسىن باسقاندا '''تەك''' جوعارعى كىرىستىرۋ ورنىنداعى ٴماتىن ساقتالادى.",
'yourtext'                         => 'ٴماتىنىڭىز',
'storedversion'                    => 'ساقتالعان نۇسقاسى',
'nonunicodebrowser'                => '<strong>قۇلاقتاندىرۋ: شولعىشىڭىز Unicode بەلگىلەۋىنە ۇيلەسىمدى ەمەس, سوندىقتان لاتىن ەمەس ارىپتەرى بار بەتتەردى وڭدەۋ ٴزىل بولۋ مۇمكىن.
جۇمىس ىستەۋگە ىقتىيمالدىق بەرۋ ٴۇشىن, تومەندەگى كىرىستىرۋ ورنىندا ASCII ەمەس تاڭبالار ونالتىلىق كودىمەن كورسەتىلەدى</strong>.',
'editingold'                       => '<strong>قۇلاقتاندىرۋ: وسى بەتتىڭ ەرتەرەك تۇزەتۋىن وڭدەپ جاتىرسىز.
بۇنى ساقتاساڭىز, وسى تۇزەتۋدەن كەيىنگى بارلىق وزگەرىستەر جويىلادى.</strong>',
'yourdiff'                         => 'ايىرمالار',
'copyrightwarning'                 => 'اڭعارتپا: {{SITENAME}} جوباسىنا بەرىلگەن بارلىق ۇلەستەر $2 (كوبىرەك اقپارات ٴۇشىن: $1) قۇجاتىنا ساي دەپ سانالادى.
ەگەر جازۋىڭىزدىڭ ەركىن وڭدەلۋىن جانە اقىسىز كوپشىلىككە تاراتۋىن قالاماساڭىز, مىندا جارىييالاماۋىڭىز ٴجون.<br />
تاعى دا, بۇل ماعلۇمات ٴوزىڭىز جازعانىڭىزعا, نە قوعام قازىناسىنان نەمەسە سونداي اشىق قورلاردان كوشىرىلگەنىنە بىزگە ۋادە بەرەسىز.
<strong>اۋتورلىق قۇقىقپەن قورعاۋلى ماعلۇماتتى رۇقساتسىز جارىييالاماڭىز!</strong>',
'copyrightwarning2'                => 'اڭعارتپا: {{SITENAME}} جوباسىنا بەرىلگەن بارلىق ۇلەستەردى باسقا ۇلەسكەرلەر وڭدەۋگە, وزگەرتۋگە, نە الاستاۋعا مۇمكىن.
ەگەر جازۋىڭىزدىڭ ەركىن وڭدەلۋىن قالاماساڭىز, مىندا جارىييالاماۋىڭىز ٴجون.<br />
تاعى دا, بۇل ماعلۇمات ٴوزىڭىز جازعانىڭىزعا, نە قوعام قازىناسىنان نەمەسە سونداي اشىق قورلاردان كوشىرىلگەنىنە بىزگە ۋادە بەرەسىز (كوبىرەك اقپارات ٴۇشىن $1 قۋجاتىن قاراڭىز).
<strong>اۋتورلىق قۇقىقپەن قورعاۋلى ماعلۇماتتى رۇقساتسىز جارىييالاماڭىز!</strong>',
'longpagewarning'                  => '<strong>قۇلاقتاندىرۋ: بۇل بەتتىڭ مولشەرى — $1 KB;
كەيبىر شولعىشتاردا بەت مولشەرى 32 KB جەتسە نە ونى اسسا وڭدەۋ كۇردەلى بولۋى مۇمكىن.
بەتتى بىرنەشە كىشكىن بولىمدەرگە ٴبولىپ كورىڭىز.</strong>',
'longpageerror'                    => '<strong>قاتەلىك: جونەلتپەك ٴماتىنىڭىزدىن مولشەرى — $1 KB, ەڭ كوبى $2 KB رۇقسات ەتىلگەن مولشەرىنەن اسقان.
بۇل ساقتاي الىنبايدى.</strong>',
'readonlywarning'                  => '<strong>قۇلاقتاندىرۋ: دەرەكقور باپتاۋ ٴۇشىن قۇلىپتالعان, سوندىقتان ٴدال قازىر وڭدەمەڭىزدى ساقتاي المايسىز.
كەيىن قولدانۋ ٴۇشىن ٴماتاندى قيىپ الىپ جانە قويىپ, ٴماتىن فايلىنا ساقتاۋڭىزعا بولادى.</strong>',
'protectedpagewarning'             => '<strong>قۇلاقتاندىرۋ: بۇل بەت قورعالعان. تەك اكىمشى قۇقىقتارى بار قاتىسۋشىلار وڭدەي الادى.</strong>',
'semiprotectedpagewarning'         => "'''اڭعارتپا:''' بەت جارتىلاي قورعالعان, سوندىقتان وسىنى تەك تىركەلگەن قاتىسۋشىلار وڭدەي الادى.",
'cascadeprotectedwarning'          => "'''قۇلاقتاندىرۋ''': بۇل بەت قۇلىپتالعان, ەندى تەك اكىمشى قۇقىقتارى بار قاتىسۋشىلار بۇنى وڭدەي الادى.بۇنىڭ سەبەبى: بۇل بەت «باۋلى قورعاۋى» بار كەلەسى {{PLURAL:$1|بەتتىڭ|بەتتەردىڭ}} كىرىكبەتى:",
'titleprotectedwarning'            => '<strong>قۇلاقتاندىرۋ:  بۇل بەت قۇلىپتالعان, سوندىقتان تەك بىرقاتار قاتىسۋشىلار بۇنى باستاي الادى.</strong>',
'templatesused'                    => 'بۇل بەتتە قولدانىلعان ۇلگىلەر:',
'templatesusedpreview'             => 'بۇنى قاراپ شىعۋعا قولدانىلعان ۇلگىلەر:',
'templatesusedsection'             => 'بۇل بولىمدە قولدانىلعان ۇلگىلەر:',
'template-protected'               => '(قورعالعان)',
'template-semiprotected'           => '(جارتىلاي قورعالعان)',
'hiddencategories'                 => 'بۇل بەت $1 جاسىرىن ساناتتىڭ مۇشەسى:',
'nocreatetitle'                    => 'بەتتى باستاۋ شەكتەلگەن',
'nocreatetext'                     => '{{SITENAME}} جوباسىندا جاڭا بەت باستاۋى شەكتەلگەن.
كەرى قايتىپ بار بەتتى وڭدەۋىڭىزگە بولادى, نەمەسە [[{{#special:Userlogin}}|كىرۋىڭىزگە نە تىركەلۋىڭىزگە]] بولادى.',
'nocreate-loggedin'                => '{{SITENAME}} جوباسىندا جاڭا بەت باستاۋ رۇقساتىڭىز جوق.',
'permissionserrors'                => 'رۇقساتتار قاتەلەرى',
'permissionserrorstext'            => 'بۇنى ىستەۋگە رۇقساتىڭىز جوق, كەلەسى {{PLURAL:$1|سەبەپ|سەبەپتەر}} بويىنشا:',
'permissionserrorstext-withaction' => '$2 دەگەنگە رۇقساتىڭىز جوق, كەلەسى {{PLURAL:$1|سەبەپ|سەبەپتەر}} بويىنشا:',
'recreate-deleted-warn'            => "'''قۇلاقتاندىرۋ: الدىندا جويىلعان بەتتى قايتا باستايىن دەپ تۇرسىز.'''

مىنا بەت وڭدەۋىن جالعاستىرۋ ٴۇشىن جاراستىعىن تەكسەرىپ شىعۋىڭىز ٴجون.
قولايلى بولۋى ٴۇشىن بۇل بەتتىڭ جويۋ جۋرنالى كەلتىرىلگەن:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'قۇلاقتاندىرۋ: بۇل بەتتە تىم كوپ شىعىس الاتىن قۇرىلىم تالداتقىش جەتەلەرىنىڭ قوڭىراۋ شالۋلارى بار.

بۇل $2 شاماسىنان كەم بولۋى ٴجون, قازىر وسى ارادا $1.',
'expensive-parserfunction-category'       => 'شىعىس الاتىن قۇرىلىم تالداتقىش جەتەلەرىنىڭ تىم كوپ شاقىرىمى بار بەتتەر',
'post-expand-template-inclusion-warning'  => 'قۇلاقتاندىرۋ: ۇلگى كىرىستىرۋ مولشەرى تىم ۇلكەن.
كەيبىر ۇلگىلەر كىرىستىرىلمەيدى.',
'post-expand-template-inclusion-category' => 'ۇلگى كىرىستىرىلگەن بەتتەر مولشەرى اسىپ كەتتى',
'post-expand-template-argument-warning'   => 'قۇلاقتاندىرۋ: بۇل بەتتە تىم كوپ ۇلعايتىلعان مولشەرى بولعان ەڭ كەمىندە ٴبىر ۇلگى دالەلى بار.
بۇنىڭ دالەلدەرىن قالدىرىپ كەتكەن.',
'post-expand-template-argument-category'  => 'ۇلگى دالەلدەرىن قالدىرىپ كەتكەن بەتتەر',

# "Undo" feature
'undo-success' => 'بۇل وڭدەمە جوققا شىعارىلۋى مۇمكىن. تالابىڭىزدى قۇپتاپ الدىن الا تومەندەگى سالىستىرۋدى تەكسەرىپ شىعىڭىز دا, وڭدەمەنى جوققا شىعارۋىن ٴبىتىرۋ ٴۇشىن تومەندەگى وزگەرىستەردى ساقتاڭىز.',
'undo-failure' => 'بۇل وڭدەمە جوققا شىعارىلمايدى, سەبەبى ارادا قاقتىعىستى وڭدەمەلەر بار.',
'undo-norev'   => 'بۇل وڭدەمە جوققا شىعارىلمايدى, سەبەبى بۇل جوق نەمەسە جويىلعان.',
'undo-summary' => '[[Special:Contributions/$2|$2]] ([[User_talk:$2|تالقىلاۋى]]) ىستەگەن ٴنومىر $1 نۇسقاسىن جوققا شىعاردى',

# Account creation failure
'cantcreateaccounttitle' => 'جاڭا تىركەلگى جاسالمادى',
'cantcreateaccount-text' => "بۇل IP جايدان ('''$1''') جاڭا تىركەلگى جاساۋىن [[User:$3|$3]] بۇعاتتاعان.

$3 كەلتىرىلگەن سەبەبى: ''$2''",

# History pages
'viewpagelogs'        => 'بۇل بەت ٴۇشىن جۋرنال وقىيعالارىن قاراۋ',
'nohistory'           => 'مىندا بۇل بەتتىنىڭ تۇزەتۋ تارىيحى جوق.',
'revnotfound'         => 'تۇزەتۋ تابىلمادى',
'revnotfoundtext'     => 'بۇل بەتتىڭ سۇرالعان ەسكى تۇزەتۋى تابىلعان جوق. وسى بەت قاتىناۋىنا پايدالانعان URL تەكسەرىپ شىعىڭىز.',
'currentrev'          => 'اعىمدىق تۇزەتۋ',
'revisionasof'        => '$1 كەزىندەگى تۇزەتۋ',
'revision-info'       => '$1 كەزىندەگى $2 ىستەگەن تۇزەتۋ',
'previousrevision'    => '← ەسكىلەۋ تۇزەتۋى',
'nextrevision'        => 'جاڭالاۋ تۇزەتۋى →',
'currentrevisionlink' => 'اعىمدىق تۇزەتۋى',
'cur'                 => 'اعىم.',
'next'                => 'كەل.',
'last'                => 'سوڭ.',
'page_first'          => 'العاشقىسىنا',
'page_last'           => 'سوڭعىسىنا',
'histlegend'          => 'ايىرماسىن بولەكتەۋ: سالىستىرماق نۇسقالارىنىڭ قوسۋ كوزدەرىن بەلگىلەپ <Enter> پەرنەسىن باسىڭىز, نەمەسە تومەندەگى باتىرمانى نۇقىڭىز.<br />
شارتتى بەلگىلەر: (اعىم.) = اعىمدىق نۇسقامەن ايىرماسى,
(سوڭ.) = الدىڭعى نۇسقامەن ايىرماسى, ش = شاعىن وڭدەمە',
'deletedrev'          => '[جويىلعان]',
'histfirst'           => 'ەڭ العاشقىسىنا',
'histlast'            => 'ەڭ سوڭعىسىنا',
'historysize'         => '($1 بايت)',
'historyempty'        => '(بوس)',

# Revision feed
'history-feed-title'          => 'تۇزەتۋ تارىيحى',
'history-feed-description'    => 'مىنا ۋىيكىيدەگى بۇل بەتتىڭ تۇزەتۋ تارىيحى',
'history-feed-item-nocomment' => '$2 كەزىندەگى $1 دەگەن', # user at time
'history-feed-empty'          => 'سۇراتىلعان بەت جوق بولدى.
ول مىنا ۋىيكىيدەن جويىلعان, نەمەسە اتاۋى اۋىستىرىلعان.
وسىعان قاتىستى جاڭا بەتتەردى [[{{#special:Search}}|بۇل ۋىيكىيدەن ىزدەۋدى]] بايقاپ كورىڭىز.',

# Revision deletion
'rev-deleted-comment'         => '(ماندەمە الاستالدى)',
'rev-deleted-user'            => '(قاتىسۋشى اتى الاستالدى)',
'rev-deleted-event'           => '(جۋرنال جازباسى الاستالدى)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
بۇل بەتتىڭ تۇزەتۋى بارشا مۇراعاتتارىنان الاستالعان.
مىندا [{{fullurl:{{#special:Log}}/delete|page={{FULLPAGENAMEE}}}} جويۋ جۋرنالىندا] ەگجەي-تەگجەي مالىمەتتەرى بولۋى مۇمكىن.</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
وسى بەتتىڭ تۇزەتۋى بارشا مۇراعاتتارىنان الاستالعان.
{{SITENAME}} اكىمشىسى بوپ سونى كورە الاسىز;
[{{fullurl:{{#special:Log}}/delete|page={{FULLPAGENAMEE}}}} جويۋ جۋرنالىندا] ەگجەي-تەگجەي مالمەتتەرى بولۋى مۇمكىن.</div>',
'rev-delundel'                => 'كورسەت/جاسىر',
'revisiondelete'              => 'تۇزەتۋلەردى جويۋ/جويۋدى بولدىرماۋ',
'revdelete-nooldid-title'     => 'نىسانا تۇزەتۋ جارامسىز',
'revdelete-nooldid-text'      => 'بۇل جەتەنى ورىنداۋ ٴۇشىن نىسانا تۇزەتۋىن/تۇزەتۋلەرىن كەلتىرىلمەپسىز,
كەلتىرىلگەن تۇزەتۋ جوق, نە اعىمدىق تۇزەتۋدى جاسىرۋ ٴۇشىن ارەكەتتەنىپ كوردىڭىز.',
'revdelete-selected'          => '[[:$1]] دەگەننىڭ بولەكتەنگەن {{PLURAL:$2|تۇزەتۋى|تۇزەتۋلەرى}}:',
'logdelete-selected'          => 'بولەكتەنگەن {{PLURAL:$1|جۋرنال وقىيعاسى|جۋرنال وقىيعالارى}}:',
'revdelete-text'              => 'جويىلعان تۇزەتۋلەر مەن وقىيعالاردى ٴالى دە بەت تارىيحىندا جانە جۋرنالداردا تابۋعا بولادى, بىراق ولاردىڭ ماعلۇمات بولشەكتەرى بارشاعا قاتىنالمايدى.

{{SITENAME}} جوباسىنىڭ باسقا اكىمشىلەرى جاسىرىن ماعلۇماتقا قاتىناي الادى, جانە قوسىمشا تىيىمدار قويىلعانشا دەيىن, وسى تىلدەسۋ ارقىلى جويۋدى بولدىرماۋى مۇمكىن.',
'revdelete-legend'            => 'كورىنىس تىيىمدارىن قويۋ:',
'revdelete-hide-text'         => 'تۇزەتۋ ٴماتىنىن جاسىر',
'revdelete-hide-name'         => 'ارەكەت پەن نىساناسىن جاسىر',
'revdelete-hide-comment'      => 'وڭدەمە ماندەمەسىن جاسىر',
'revdelete-hide-user'         => 'وڭدەۋشى اتىن (IP مەكەنجايىن) جاسىر',
'revdelete-hide-restricted'   => 'وسى تىيىمداردى اكىمشىلەرگە قولدانۋ جانە بۇل تىلدەسۋدى قۇلىپتاۋ',
'revdelete-suppress'          => 'دەرەكتەردى بارشاعا ۇقساس اكىمشىلەردەن دە شەتتەتۋ',
'revdelete-hide-image'        => 'فايل ماعلۇماتىن جاسىر',
'revdelete-unsuppress'        => 'قالپىنا كەلتىرىلگەن تۇزەتۋلەردەن تىيىمداردى الاستاۋ',
'revdelete-log'               => 'جۋرنالداعى ماندەمەسى:',
'revdelete-submit'            => 'بولەكتەنگەن تۇزەتۋگە قولدانۋ',
'revdelete-logentry'          => '[[$1]] دەگەننىڭ تۇزەتۋ كورىنىسىن وزگەرتتى',
'logdelete-logentry'          => '[[$1]] دەگەننىڭ وقىيعا كورىنىسىن وزگەرتتى',
'revdelete-success'           => "'''تۇزەتۋ كورىنىسى ٴساتتى قويىلدى.'''",
'logdelete-success'           => "'''جۋرنال كورىنىسى ٴساتتى قويىلدى.'''",
'revdel-restore'              => 'كورىنىسىن وزگەرتۋ',
'pagehist'                    => 'بەت تارىيحى',
'deletedhist'                 => 'جويىلعان تارىيحى',
'revdelete-content'           => 'ماعلۇمات',
'revdelete-summary'           => 'وڭدەمەنىڭ قىسقاشا مازمۇنداماسى',
'revdelete-uname'             => 'قاتىسۋشى اتى',
'revdelete-restricted'        => 'اكىمشىلەرگە تىيىمدار قولدادى',
'revdelete-unrestricted'      => 'اكىمشىلەردەن تىيىمداردى الاستادى',
'revdelete-hid'               => '$1 جاسىردى',
'revdelete-unhid'             => '$1 اشتى',
'revdelete-log-message'       => '$2 تۇزەتۋ ٴۇشىن $1',
'logdelete-log-message'       => '$2 وقىيعا ٴۇشىن $1',

# Suppression log
'suppressionlog'     => 'شەتتەتۋ جۋرنالى',
'suppressionlogtext' => 'تومەندەگى تىزىمدە اكىمشىلەردەن جاسىرىلعان ماعلۇماتقا قاتىستى جويۋلار مەن بۇعاتتاۋلار بەرىلەدى.
اعىمدا ارەكەتتەگى تىيىم مەن بۇعاتتاۋ ٴتىزىمى ٴۇشىن [[{{#special:Ipblocklist}}|IP بۇعاتتاۋ ٴتىزىمىن]] قاراڭىز.',

# History merging
'mergehistory'                     => 'بەتتەر تارىيحىن بىرىكتىرۋ',
'mergehistory-header'              => 'بۇل بەت تۇزەتۋلەر تارىيحىن قاينار بەتتىڭ بىرەۋىنەن الىپ جاڭا بەتكە بىرىكتىرگىزەدى.
وسى وزگەرىس بەتتىڭ تارىيحىي جالعاستىرۋشىلىعىن قوشتايتىنىنا كوزىڭىز جەتسىن.',
'mergehistory-box'                 => 'ەكى بەتتىڭ تۇزەتۋلەرىن بىرىكتىرۋ:',
'mergehistory-from'                => 'قاينار بەتى:',
'mergehistory-into'                => 'نىسانا بەتى:',
'mergehistory-list'                => 'بىرىكتىرلەتىن تۇزەتۋ تارىيحى',
'mergehistory-merge'               => '[[:$1]] دەگەننىڭ كەلەسى تۇزەتۋلەرى [[:$2]] دەگەنگە بىرىكتىرىلۋى مۇمكىن.
بىرىكتىرۋگە تەك ەنگىزىلگەن ۋاقىتقا دەيىن جاسالعان تۇزەتۋلەردى ايىرىپ-قوسقىش باعاندى قولدانىڭىز.
اڭعارتپا: باعىتتاۋ سىلتەمەلەرىن قولدانعاندا بۇل باعان قايتا قويىلادى.',
'mergehistory-go'                  => 'بىرىكتىرلەتىن تۇزەتۋلەردى كورسەت',
'mergehistory-submit'              => 'تۇزەتۋلەردى بىرىكتىرۋ',
'mergehistory-empty'               => 'ەش تۇزەتۋلەر بىرىكتىرىلمەيدى',
'mergehistory-success'             => '[[:$1]] دەگەننىڭ $3 تۇزەتۋى [[:$2]] دەگەنگە ٴساتتى بىرىكتىرىلدى.',
'mergehistory-fail'                => 'تارىيح بىرىكتىرۋىن ورىنداۋ ىيكەمدى ەمەس, بەت پەن ۋاقىت باپتالىمدارىن قايتا تەكسەرىپ شىعىڭىز.',
'mergehistory-no-source'           => '$1 دەگەن قاينار بەتى جوق.',
'mergehistory-no-destination'      => '$1 دەگەن نىسانا بەتى جوق.',
'mergehistory-invalid-source'      => 'قاينار بەتىندە جارامدى تاقىرىپ اتى بولۋى ٴجون.',
'mergehistory-invalid-destination' => 'نىسانا بەتىندە جارامدى تاقىرىپ اتى بولۋى ٴجون.',
'mergehistory-autocomment'         => '[[:$1]] دەگەن [[:$2]] دەگەنگە بىرىكتىرىلدى',
'mergehistory-comment'             => '[[:$1]] دەگەن [[:$2]] دەگەنگە بىرىكتىرىلدى: $3',

# Merge log
'mergelog'           => 'بىرىكتىرۋ جۋرنالى',
'pagemerge-logentry' => '[[$1]] دەگەن [[$2]] دەگەنگە بىرىكتىرىلدى ($3 دەيىنگى تۇزەتۋلەرى)',
'revertmerge'        => 'بىرىكتىرۋدى بولدىرماۋ',
'mergelogpagetext'   => 'تومەندە ٴبىر بەتتىڭ تارىيحى وزگە بەتكە بىرىكتىرۋ ەڭ سوڭعى ٴتىزىمى كەلتىرىلەدى.',

# Diffs
'history-title'           => '«$1» — تۇزەتۋ تارىيحى',
'difference'              => '(تۇزەتۋلەر اراسىنداعى ايىرماشىلىق)',
'lineno'                  => 'جول ٴنومىرى $1:',
'compareselectedversions' => 'بولەكتەنگەن نۇسقالاردى سالىستىرۋ',
'editundo'                => 'جوققا شىعارۋ',
'diff-multi'              => '(اراداعى $1 تۇزەتۋ كورسەتىلمەگەن.)',

# Search results
'searchresults'             => 'ىزدەۋ ناتىيجەلەرى',
'searchresulttext'          => '{{SITENAME}} جوباسىندا ىزدەۋ تۋرالى كوبىرەك اقپارات ٴۇشىن, [[{{{{ns:mediawiki}}:helppage}}|{{int:help}} بەتىن]] قاراڭىز.',
'searchsubtitle'            => "ىزدەگەنىڭىز: '''[[:$1]]'''",
'searchsubtitleinvalid'     => "ىزدەگەنىڭىز: '''$1'''",
'noexactmatch'              => "'''وسى ارادا بەتتىڭ «$1» تاقىرىپ اتى جوق.'''
[[:$1|بۇل بەتتى باستاي]] الاسىز.",
'noexactmatch-nocreate'     => "'''وسى ارادا بەتتىڭ «$1» تاقىرىپ اتى جوق.'''",
'toomanymatches'            => 'تىم كوپ سايكەس قايتارىلدى, وزگە سۇرانىمدى بايقاپ كورىڭىز',
'titlematches'              => 'بەت تاقىرىبىن اتى سايكەس كەلەدى',
'notitlematches'            => 'ەش بەت تاقىرىبىن اتى سايكەس ەمەس',
'textmatches'               => 'بەت ٴماتىنى سايكەس كەلەدى',
'notextmatches'             => 'ەش بەت ٴماتىنى سايكەس ەمەس',
'prevn'                     => 'الدىڭعى $1',
'nextn'                     => 'كەلەسى $1',
'viewprevnext'              => 'كورسەتىلۋى: ($1) ($2) ($3) جازبا',
'search-result-size'        => '$1 ($2 ٴسوز)',
'search-result-score'       => 'اراقاتىناستىلىعى: $1 %',
'search-redirect'           => '(ايداعىش $1)',
'search-section'            => '(ٴبولىم $1)',
'search-suggest'            => 'بۇنى ىزدەدىڭىز بە: $1',
'search-interwiki-caption'  => 'باۋىرلاس جوبالار',
'search-interwiki-default'  => '$1 ناتىيجە:',
'search-interwiki-more'     => '(كوبىرەك)',
'search-mwsuggest-enabled'  => 'ۇسىنىمدارمەن',
'search-mwsuggest-disabled' => 'ۇسىنىمدارسىز',
'search-relatedarticle'     => 'قاتىستى',
'mwsuggest-disable'         => 'AJAX ۇسىنىمدارىن ٴوشىر',
'searchrelated'             => 'قاتىستى',
'searchall'                 => 'بارلىق',
'showingresults'            => "تومەندە ٴنومىر '''$2''' ورنىنان باستاپ بارىنشا '''$1''' ناتىيجە كورسەتىلەدى.",
'showingresultsnum'         => "تومەندە ٴنومىر '''$2''' ورنىنان باستاپ '''$3''' ناتىيجە كورسەتىلەدى.",
'showingresultstotal'       => "تومەندە {{PLURAL:$3|'''$3''' اراسىنان '''$1''' ناتىيجە كورسەتىلەدى|'''$3''' اراسىنان '''$1 — $2''' ناتىيجە اۋقىمى كورسەتىلەدى}}",
'nonefound'                 => "'''اڭعارتپا''': ادەپكىدەن تەك كەيبىر ەسىم ايالاردان ىزدەلىنەدى. بارلىق ماعلۇمات ٴتۇرىن (سونىڭ ىشىندە تالقىلاۋ بەتتەردى, ۇلگىلەردى ت.ب.) ىزدەۋ ٴۇشىن سۇرانىمىڭىزدى ''بارلىق:'' دەپ باستاڭىز, نەمەسە قالاعان ەسىم اياسىن باستاۋىش ەسەبىندە قولدانىڭىز.",
'powersearch'               => 'كەڭەيتىلگەن ىزدەۋ',
'powersearch-legend'        => 'كەڭەيتىلگەن ىزدەۋ',
'powersearch-ns'            => 'مىنا ەسىم ايالاردا ىزدەۋ:',
'powersearch-redir'         => 'ايداتۋلاردى تىزىمدەۋ',
'powersearch-field'         => 'مىنانى ىزدەمەك:',
'search-external'           => 'شەتتىك ىزدەگىش',
'searchdisabled'            => '{{SITENAME}} ىزدەۋ قىزمەتى وشىرىلگەن.
ازىرشە Google ارقىلى ىزدەۋگە بولادى.
اڭعارتپا: {{SITENAME}} تورابىنىڭ ماعلۇمات تىزبەلەرى ەسكىرگەن بولۋى مۇمكىن.',

# Preferences page
'preferences'              => 'باپتالىمدار',
'mypreferences'            => 'باپتالىمدارىم',
'prefs-edits'              => 'وڭدەمە سانى:',
'prefsnologin'             => 'كىرمەگەنسىز',
'prefsnologintext'         => 'باپتاۋىڭىزدى قويۋ ٴۇشىن [[Special:UserLogin|كىرۋىڭىز]] ٴتىيىستى.',
'prefsreset'               => 'باپتالىمدار ارقاۋدان قايتا قويىلدى.',
'qbsettings'               => 'ٴمازىر',
'qbsettings-none'          => 'ەشقانداي',
'qbsettings-fixedleft'     => 'سولعا بەكىتىلگەن',
'qbsettings-fixedright'    => 'وڭعا بەكىتىلگەن',
'qbsettings-floatingleft'  => 'سولعا قالقىعان',
'qbsettings-floatingright' => 'وڭعا قالقىعان',
'changepassword'           => 'قۇپىييا ٴسوزدى وزگەرتۋ',
'skin'                     => 'مانەرلەر',
'math'                     => 'ورنەكتەر',
'dateformat'               => 'كۇن-اي ٴپىشىمى',
'datedefault'              => 'ەش قالاۋسىز',
'datetime'                 => 'ۋاقىت',
'math_failure'             => 'قۇرىلىمىن تالداتۋى ٴساتسىز ٴبىتتى',
'math_unknown_error'       => 'بەلگىسىز قاتە',
'math_unknown_function'    => 'بەلگىسىز جەتە',
'math_lexing_error'        => 'ٴسوز كەنىنىڭ قاتەسى',
'math_syntax_error'        => 'سويلەم جۇيەسىنىڭ قاتەسى',
'math_image_error'         => 'PNG اۋدارىسى ٴساتسىز ٴبىتتى;
latex, dvips, gs جانە convert باعدارلامالارىنىڭ دۇرىس ورناتۋىن تەكسەرىپ شىعىڭىز',
'math_bad_tmpdir'          => 'math دەگەن ۋاقىتشا قالتاسىنا جازىلمادى, نە قالتا قۇرىلمادى',
'math_bad_output'          => 'math دەگەن بەرىس قالتاسىنا جازىلمادى, نە قالتا قۇرىلمادى',
'math_notexvc'             => 'texvc اتقارىلمالىسى تابىلمادى;
باپتاۋ ٴۇشىن math/README قۇجاتىن قاراڭىز.',
'prefs-personal'           => 'جەكە دەرەكتەرى',
'prefs-rc'                 => 'جۋىقتاعى وزگەرىستەر',
'prefs-watchlist'          => 'باقىلاۋ',
'prefs-watchlist-days'     => 'باقىلاۋ تىزىمىندەگى كۇندەردىڭ كورسەتپەك سانى:',
'prefs-watchlist-edits'    => 'كەڭەيتىلگەن باقىلاۋلارداعى وزگەرىستەردىڭ بارىنشا كورسەتپەك سانى:',
'prefs-misc'               => 'ارقىيلى',
'saveprefs'                => 'ساقتا',
'resetprefs'               => 'ساقتالماعان وزگەرىستەردى تازارت',
'oldpassword'              => 'اعىمدىق قۇپىييا ٴسوزىڭىز:',
'newpassword'              => 'جاڭا قۇپىييا ٴسوزىڭىز:',
'retypenew'                => 'جاڭا قۇپىييا ٴسوزىڭىزدى قايتالاڭىز:',
'textboxsize'              => 'وڭدەۋ',
'rows'                     => 'جولدار:',
'columns'                  => 'باعاندار:',
'searchresultshead'        => 'ىزدەۋ',
'resultsperpage'           => 'بەت سايىن ناتىيجە سانى:',
'contextlines'             => 'ناتىيجە سايىن جول سانى:',
'contextchars'             => 'جول سايىن تاڭبا سانى:',
'stub-threshold'           => '<a href="#" class="stub">بىتەمە سىلتەمەسىن</a> پىشىمدەۋ تابالدىرىعى (بايت):',
'recentchangesdays'        => 'جۇىقتاعى وزگەرىستەرىندە كورسەتپەك كۇن سانى:',
'recentchangescount'       => 'جۋىقتاعى وزگەرىستەردىندە, تارىيح جانە جۋرنال بەتتەرىندە كورسەتپەك وڭدەمە سانى:',
'savedprefs'               => 'باپتالىمدارىڭىز ساقتالدى.',
'timezonelegend'           => 'ۋاقىت بەلدەۋى',
'timezonetext'             => '¹ جەرگىلىكتى ۋاقىتىڭىز بەن سەرۆەر ۋاقىتىنىڭ (UTC) اراسىنداعى ساعات سانى.',
'localtime'                => 'جەرگىلىكتى ۋاقىت',
'timezoneoffset'           => 'ساعات ىعىسۋى¹',
'servertime'               => 'سەرۆەر ۋاقىتى',
'guesstimezone'            => 'شولعىشتان الىپ تولتىرۋ',
'allowemail'               => 'باسقادان حات قابىلداۋىن قوس',
'prefs-searchoptions'      => 'ىزدەۋ باپتالىمدارى',
'prefs-namespaces'         => 'ەسىم ايالارى',
'defaultns'                => 'مىنا ەسىم ايالاردا ادەپكىدەن ىزدەۋ:',
'default'                  => 'ادەپكى',
'files'                    => 'فايلدار',

# User rights
'userrights'                  => 'قاتىسۋشى قۇقىقتارىن رەتتەۋ', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'قاتىسۋشى توپتارىن رەتتەۋ',
'userrights-user-editname'    => 'قاتىسۋشى اتىن ەنگىزىڭىز:',
'editusergroup'               => 'قاتىسۋشى توپتارىن وڭدەۋ',
'editinguser'                 => "قاتىسۋشى قۇقىقتارىن وزگەرتۋ: '''[[User:$1|$1]]''' ([[User_talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'قاتىسۋشى توپتارىن وڭدەۋ',
'saveusergroups'              => 'قاتىسۋشى توپتارىن ساقتاۋ',
'userrights-groupsmember'     => 'مۇشەلىگى:',
'userrights-groups-help'      => 'بۇل قاتىسۋشى كىرەتىن توپتاردى رەتتەي الاسىز.
* قۇسبەلگى قويىلعان كوزى قاتىسۋشى بۇل توپقا كىرگەنىن كورسەتەدى;
* قۇسبەلگى الىپ تاستالعان كوز قاتىسۋشى بۇل توپقا كىرمەگەنىن كورسەتەدى;
* كەلتىرىلگەن * توپتى ٴبىر ۇستەگەنىنەن كەيىن الاستاي المايتىندىعىن, نە قاراما-قارسىسىن كورسەتەدى.',
'userrights-reason'           => 'وزگەرتۋ سەبەبى:',
'userrights-no-interwiki'     => 'باسقا ۋىيكىيلەردەگى پايدالانۋشى قۇقىقتارىن وڭدەۋگە رۇقساتىڭىز جوق.',
'userrights-nodatabase'       => '$1 دەرەكقورى جوق نە جەرگىلىكتى ەمەس.',
'userrights-nologin'          => 'قاتىسۋشى قۇقىقتارىن تاعايىنداۋ ٴۇشىن اكىمشى تىركەلگىسىمەن [[{{#special:Userlogin}}|كىرۋىڭىز]] ٴجون.',
'userrights-notallowed'       => 'قاتىسۋشى قۇقىقتارىن تاعايىنداۋ ٴۇشىن تىركەلگىڭىزدە رۇقسات جوق.',
'userrights-changeable-col'   => 'وزگەرتە الاتىن توپتار',
'userrights-unchangeable-col' => 'وزگەرتە المايتىن توپتار',

# Groups
'group'               => 'توپ:',
'group-user'          => 'قاتىسۋشىلار',
'group-autoconfirmed' => 'وزقۇپتالعان قاتىسۋشىلار',
'group-bot'           => 'بوتتار',
'group-sysop'         => 'اكىمشىلەر',
'group-bureaucrat'    => 'بىتىكشىلەر',
'group-suppress'      => 'شەتتەتۋشىلەر',
'group-all'           => '(بارلىق)',

'group-user-member'          => 'قاتىسۋشى',
'group-autoconfirmed-member' => 'وزقۇپتالعان قاتىسۋشى',
'group-bot-member'           => 'بوت',
'group-sysop-member'         => 'اكىمشى',
'group-bureaucrat-member'    => 'بىتىكشى',
'group-suppress-member'      => 'شەتتەتۋشى',

'grouppage-user'          => '{{ns:project}}:قاتىسۋشىلار',
'grouppage-autoconfirmed' => '{{ns:project}}:وزقۇپتالعان قاتىسۋشىلار',
'grouppage-bot'           => '{{ns:project}}:بوتتار',
'grouppage-sysop'         => '{{ns:project}}:اكىمشىلەر',
'grouppage-bureaucrat'    => '{{ns:project}}:بىتىكشىلەر',
'grouppage-suppress'      => '{{ns:project}}:شەتتەتۋشىلەر',

# Rights
'right-read'                 => 'بەتتەردى وقۋ',
'right-edit'                 => 'بەتتەردى وڭدەۋ',
'right-createpage'           => 'تالقىلاۋ ەمەس بەتتەردى باستاۋ',
'right-createtalk'           => 'تالقىلاۋ بەتتەردى باستاۋ',
'right-createaccount'        => 'جاڭا قاتىسۋشى تىركەلگىسىن جاساۋ',
'right-minoredit'            => 'وڭدەمەلەردى شاعىن دەپ بەلگىلەۋ',
'right-move'                 => 'بەتتەردى جىلجىتۋ',
'right-move-subpages'        => 'بەتتەردى بۇلاردىڭ باعىنىشتى بەتتەرىمەن جىلجىتۋ',
'right-suppressredirect'     => 'ٴتىيىستى اتاۋعا بەتتى جىلجىتقاندا ايداعىشتى جاساماۋ',
'right-upload'               => 'فايلداردى قوتارىپ بەرۋ',
'right-reupload'             => 'بار فايل ۇستىنە جازۋ',
'right-reupload-own'         => 'ٴوزى قوتارىپ بەرگەن فايل ۇستىنە جازۋ',
'right-reupload-shared'      => 'تاسپا ورتاق قويماسىنداعى فايلداردى جەرگىلىكتىلەرمەن اسىرۋ',
'right-upload_by_url'        => 'فايلدى URL مەكەنجايىنان قوتارىپ بەرۋ',
'right-purge'                => 'بەتتى توراپ بۇركەمەسىنەن قۇپتاۋسىز تازارتۋ',
'right-autoconfirmed'        => 'جارتىلاي قورعالعان بەتتەردى وڭدەۋ',
'right-bot'                  => 'وزدىكتىك ۇدەرىس دەپ ەسەپتەلۋ',
'right-nominornewtalk'       => 'تالقىلاۋ بەتتەردەگى شاعىن وڭدەمەلەردى جاڭا حابار دەپ ەسەپتەمەۋ',
'right-apihighlimits'        => 'API سۇرانىمدارىنىڭ جوعارى شەكتەلىمدەرىن پايدالانۋ',
'right-writeapi'             => 'API جازۋىن پايدالانۋ',
'right-delete'               => 'بەتتەردى جويۋ',
'right-bigdelete'            => 'ۇزاق تارىيحى بار بەتتەردى جويۋ',
'right-deleterevision'       => 'بەتتەردىڭ وزىندىك تۇزەتۋلەرىن جويۋ نە جويۋىن بولدىرماۋ',
'right-deletedhistory'       => 'جويىلعان تارىيح دانالارىن (بايلانىستى ٴماتىنسىز) كورۋ',
'right-browsearchive'        => 'جويىلعان بەتتەردى ىزدەۋ',
'right-undelete'             => 'بەتتىڭ جيۋىن بولدىرماۋ',
'right-suppressrevision'     => 'اكىمشىلەردەن جاسىرىلعان تۇزەتۋلەردى شولىپ شىعۋ جانە قالپىنا كەلتىرۋ',
'right-suppressionlog'       => 'جەكەلىك جۋرنالداردى كورۋ',
'right-block'                => 'باسقا قاتىسۋشىلاردى وڭدەۋدەن بۇعاتتاۋ',
'right-blockemail'           => 'قاتىسۋشىنىڭ حات جونەلتۋىن بۇعاتتاۋ',
'right-hideuser'             => 'بارشادان جاسىرىپ, قاتىسۋشى اتىن بۇعاتتاۋ',
'right-ipblock-exempt'       => 'IP بۇعاتتاۋلاردى, وزبۇعاتتاۋلاردى جانە اۋقىم بۇعاتتاۋلاردى وراعىتۋ',
'right-proxyunbannable'      => 'پروكسىي سەرۆەرلەردىڭ وزبۇعاتتاۋلارىن وراعىتۋ',
'right-protect'              => 'قورعاۋ دەڭگەيلەرىن وزگەرتۋ جانە قورعالعان بەتتەردى وڭدەۋ',
'right-editprotected'        => 'قورعالعان بەتتەردى وڭدەۋ (باۋلى قورعاۋلارسىز)',
'right-editinterface'        => 'پايدالانۋشىلىق تىلدەسىۋىن وڭدەۋ',
'right-editusercssjs'        => 'باسقا قاتىسۋشىلاردىڭ CSS جانە JS فايلدارىن وڭدەۋ',
'right-rollback'             => 'بەلگىلى بەتتى وڭدەگەن سوڭعى قاتىسۋشىنىڭ وڭدەمەلەرىنەن جىلدام شەگىندىرۋ',
'right-markbotedits'         => 'شەگىندىرلگەن وڭدەمەلەردى بوتتاردىكى دەپ بەلگىلەۋ',
'right-noratelimit'          => 'ەسەلىك شەكتەلىمدەرى ىقپال ەتپەيدى',
'right-import'               => 'باسقا ۋىيكىيلەردەن بەتتەردى سىرتتان الۋ',
'right-importupload'         => 'فايل قوتارىپ بەرۋىمەن بەتتەردى سىرتتان الۋ',
'right-patrol'               => 'باسقاراردىڭ وڭدەمەلەرىن زەرتتەلدى دەپ بەلگىلەۋ',
'right-autopatrol'           => 'ٴوز وڭدەمەلەرىن زەرتتەلدى دەپ وزدىكتىك بەلگىلەۋ',
'right-patrolmarks'          => 'جۋىقتاعى وزگەرىستەردەگى زەرتتەۋ بەلگىلەرىن كورۋ',
'right-unwatchedpages'       => 'باقىلانىلماعان بەت ٴتىزىمىن كورۋ',
'right-trackback'            => 'اڭىستاۋدى جونەلتۋ',
'right-mergehistory'         => 'بەتتەردىڭ تارىيحىن قوسىپ بەرۋ',
'right-userrights'           => 'قاتىسۋشىلاردىڭ بارلىق قۇقىقتارىن وڭدەۋ',
'right-userrights-interwiki' => 'باسقا ۇىيكىيلەردەگى قاتىسۋشىلاردىڭ قۇقىقتارىن وڭدەۋ',
'right-siteadmin'            => 'دەرەكقوردى قۇلىپتاۋ جانە قۇلىپتاۋىن ٴوشىرۋ',

# User rights log
'rightslog'      => 'قاتىسۋشى قۇقىقتارى جۋرنالى',
'rightslogtext'  => 'بۇل قاتىسۋشى قۇقىقتارىن وزگەرتۋ جۋرنالى.',
'rightslogentry' => '$1 كىرگەن توپتارىن $2 دەگەننەن $3 دەگەنگە وزگەرتتى',
'rightsnone'     => '(ەشقانداي)',

# Recent changes
'nchanges'                          => '$1 وزگەرىس',
'recentchanges'                     => 'جۋىقتاعى وزگەرىستەر',
'recentchangestext'                 => 'بۇل بەتتە وسى ۋىيكىيدەگى بولعان جۋىقتاعى وزگەرىستەر بايقالادى.',
'recentchanges-feed-description'    => 'بۇل ارنامەنەن ۋىيكىيدەگى ەڭ سوڭعى وزگەرىستەر قاداعالانادى.',
'rcnote'                            => "$3 كەزىنە دەيىن — تومەندە سوڭعى {{PLURAL:$2|كۇندەگى|'''$2''' كۇندەگى}}, سوڭعى '''$1''' وزگەرىس كورسەتىلەدى.",
'rcnotefrom'                        => "'''$2''' كەزىنەن بەرى — تومەندە '''$1''' جەتكەنشە دەيىن وزگەرىستەر كورسەتىلەدى.",
'rclistfrom'                        => '$1 كەزىنەن بەرى — جاڭا وزگەرىستەردى كورسەت.',
'rcshowhideminor'                   => 'شاعىن وڭدەمەلەردى $1',
'rcshowhidebots'                    => 'بوتتاردى $1',
'rcshowhideliu'                     => 'كىرگەندەردى $1',
'rcshowhideanons'                   => 'تىركەلگىسىزدەردى $1',
'rcshowhidepatr'                    => 'زەرتتەلگەن وڭدەمەلەردى $1',
'rcshowhidemine'                    => 'وڭدەمەلەرىمدى $1',
'rclinks'                           => 'سوڭعى $2 كۇندە بولعان, سوڭعى $1 وزگەرىستى كورسەت<br />$3',
'diff'                              => 'ايىرم.',
'hist'                              => 'تار.',
'hide'                              => 'جاسىر',
'show'                              => 'كورسەت',
'minoreditletter'                   => 'ش',
'newpageletter'                     => 'ج',
'boteditletter'                     => 'ب',
'number_of_watching_users_pageview' => '[باقىلاعان $1 قاتىسۋشى]',
'rc_categories'                     => 'ساناتتارعا شەكتەۋ ("|" بەلگىسىمەن بولىكتەڭىز)',
'rc_categories_any'                 => 'قايسىبىر',
'newsectionsummary'                 => '/* $1 */ جاڭا ٴبولىم',

# Recent changes linked
'recentchangeslinked'          => 'قاتىستى وزگەرىستەر',
'recentchangeslinked-title'    => '«$1» دەگەنگە قاتىستى وزگەرىستەر',
'recentchangeslinked-noresult' => 'سىلتەلگەن بەتتەردە كەلتىرىلگەن مەرزىمدە ەشقانداي وزگەرىس بولماعان.',
'recentchangeslinked-summary'  => "بۇل تىزىمدە وزىندىك بەتتەن سىلتەلگەن بەتتەردەگى (نە وزىندىك سانات مۇشەلەرىندەگى) ىستەلگەن جۋىقتاعى وزگەرىستەر بەرىلەدى.
[[{{#special:Watchlist}}|باقىلاۋ تىزىمىڭىزدەگى]] بەتتەر '''جۋان''' بولىپ بەلگىلەنەدى.",
'recentchangeslinked-page'     => 'بەت اتاۋى:',
'recentchangeslinked-to'       => 'بۇنىڭ ورنىنا كەلتىرىلگەن بەتكە سىلتەلگەن بەتتەردەگى وزگەرىستەردى كورسەت',

# Upload
'upload'                      => 'قوتارىپ بەرۋ',
'uploadbtn'                   => 'قوتارىپ بەر!',
'reupload'                    => 'قايتا قوتارىپ بەرۋ',
'reuploaddesc'                => 'قوتارىپ بەرۋدى بولدىرماۋ جانە قوتارۋ پىشىنىنە قايتا كەلۋ.',
'uploadnologin'               => 'كىرمەگەنسىز',
'uploadnologintext'           => 'فايل قوتارۋ ٴۇشىن [[Special:UserLogin|كىرۋىڭىز]] كەرەك.',
'upload_directory_missing'    => 'قوتارىپ بەرمەك قالتاسى ($1) جەتىسپەيدى جانە ۆەب-سەرۆەر جاراتا المايدى.',
'upload_directory_read_only'  => 'قوتارىپ بەرمەك قالتاسىنا ($1) ۆەب-سەرۆەر جازا المايدى.',
'uploaderror'                 => 'قوتارىپ بەرۋ قاتەسى',
'uploadtext'                  => "تومەندەگى ٴپىشىندى فايلداردى قوتارىپ بەرۋ ٴۇشىن قولدانىڭىز. 
الدىندا قوتارىلىپ بەرىلگەن فايلداردى قاراۋ نە ىزدەۋ ٴۇشىن [[{{#special:Imagelist}}|قوتارىپ بەرىلگەن فايلدار تىزىمىنە]] بارىڭىز, تاعى دا قوتارىپ بەرۋى مەن جويۋى  [[{{#special:Log}}/upload|قوتارىپ بەرۋ جۋرنالىنا]] جازىلىپ الىنادى.

سۋرەتتى بەتكە كىرىستىرۋگە, فايلعا تۋرا سىلتەۋ ٴۇشىن مىنا پىشىندەگى سىلتەمەنى قولدانىڭىز:
'''[[{{ns:image}}:File.jpg]]''',
'''[[{{ns:image}}:File.png|بالاما ٴماتىن]]''' نە
'''[[{{ns:media}}:File.ogg]]'''.",
'upload-permitted'            => 'رۇقسات ەتىلگەن فايل تۇرلەرى: $1.',
'upload-preferred'            => 'ۇنامدى فايل تۇرلەرى $1.',
'upload-prohibited'           => 'رۇقسات ەتىلمەگەن فايل تۇرلەرى: $1.',
'uploadlog'                   => 'قوتارىپ بەرۋ جۋرنالى',
'uploadlogpage'               => 'قوتارىپ بەرۋ جۋرنالى',
'uploadlogpagetext'           => 'تومەندە ەڭ سوڭعى قوتارىپ بەرىلگەن فايل ٴتىزىمى.',
'filename'                    => 'فايل اتاۋى',
'filedesc'                    => 'قىسقاشا مازمۇنداماسى',
'fileuploadsummary'           => 'قىسقاشا مازمۇنداماسى:',
'filestatus'                  => 'اۋتورلىق قۇقىقتار كۇيى:',
'filesource'                  => 'قاينار كوزى:',
'uploadedfiles'               => 'قوتارىپ بەرىلگەن فايلدار',
'ignorewarning'               => 'قۇلاقتاندىرۋعا ەلەمە دە فايلدى قالايدا ساقتا.',
'ignorewarnings'              => 'كەز كەلگەن قۇلاقتاندىرۋلارعا ەلەمە',
'minlength1'                  => 'فايل اتاۋىندا ەڭ كەمىندە ٴبىر ٴارىپ بولۋى ٴجون.',
'illegalfilename'             => '«$1» فايل اتاۋىندا بەت تاقىرىبى اتىندا رۇقسات بەرىلمەگەن تاڭبالار بار.
فايلدى قايتا اتاڭىز دا بۇنى قوتارىپ بەرۋدى قايتا بايقاپ كورىڭىز.',
'badfilename'                 => 'فايلدىڭ اتاۋى «$1» دەپ وزگەرتىلدى.',
'filetype-badmime'            => '«$1» دەگەن MIME ٴتۇرى بار فايلداردى قوتارىپ بەرۋگە رۇقسات ەتىلمەيدى.',
'filetype-unwanted-type'      => "'''«.$1»''' — كۇتىلمەگەن فايل ٴتۇرى. ۇنامدى فايل تۇرلەرى: $2.",
'filetype-banned-type'        => "'''«.$1»''' — رۇقساتتالماعان فايل ٴتۇرى. رۇقساتتالعان فايل تۇرلەرى: $2.",
'filetype-missing'            => 'بۇل فايلدىڭ («.jpg» سىيياقتى) كەڭەيتىمى جوق.',
'large-file'                  => 'فايلدىڭ $1 مولشەرىنەن اسپاۋىنا كەپىلدەمە بەرىلەدى;
بۇل فايل مولشەرى — $2.',
'largefileserver'             => 'وسى فايلدىڭ مولشەرى سەرۆەردىڭ قالاۋىنان اسىپ كەتكەن.',
'emptyfile'                   => 'قوتارىپ بەرىلگەن فايلىڭىز بوس سىيياقتى. فايل اتاۋى قاتە جازىلعان مۇمكىن.
بۇل فايلدى قوتارىپ بەرۋى ناقتى تالابىڭىز ەكەنىن تەكسەرىپ شىعىڭىز.',
'fileexists'                  => 'بىلاي اتالعان فايل الداقاشان بار, ەگەر بۇنى وزگەرتۋگە باتىلىڭىز جوق بولسا <strong><tt>$1</tt></strong> دەگەندى تەكسەرىپ شىعىڭىز.',
'filepageexists'              => 'بۇل فايلدىڭ سىيپاتتاما بەتى الداقاشان <strong><tt>$1</tt></strong> دەگەندە جاسالعان, بىراق اعىمدا بىلاي اتالعان ەش فايل جوق.
ەنگىزگەن قىسقاشا مازمۇنداماڭىز سىيپاتتاماسى بەتىندە كورسەتىلمەيدى.
قىسقاشا مازمۇنداماڭىز وسى ارادا كورسەتىلۋ ٴۇشىن, بۇنى قولمەن وڭدەمەك بولىڭىز',
'fileexists-extension'        => 'ۇقساس اتاۋى بار فايل تابىلدى:<br />
قوتارىپ بەرىلەتىن فايل اتاۋى: <strong><tt>$1</tt></strong><br />
بار بولعان فايل اتاۋى: <strong><tt>$2</tt></strong><br />
وزگە اتاۋدى تاڭداڭىز.',
'fileexists-thumb'            => "<center>'''بار بولعان سۋرەت'''</center>",
'fileexists-thumbnail-yes'    => 'وسى فايل — مولشەرى كىشىرىتىلگەن سۋرەت <i>(نوباي)</i> سىيياقتى.
بۇل <strong><tt>$1</tt></strong> دەگەن فايلدى سىناپ شىعىڭىز.<br />
ەگەر سىنالعان فايل تۇپنۇسقالى مولشەرى بار دالمە-ٴدال سۋرەت بولسا, قوسىسمشا نوبايدى قوتارىپ بەرۋ كەرەگى جوق.',
'file-thumbnail-no'           => 'فايل اتاۋى <strong><tt>$1</tt></strong> دەگەنمەن باستالادى.
بۇل — مولشەرى كىشىرىتىلگەن سۋرەت <i>(نوباي)</i> سىيياقتى.
ەگەر بۇل سۋرەتتىڭ تولىق اجىراتىلىمدىعى بولسا, بۇنى قوتارىپ بەرىڭىز, ايتپەسە فايل اتاۋىن وزگەرتىڭىز.',
'fileexists-forbidden'        => 'وسىلاي اتالعان فايل الداقاشان بار;
كەرى قايتىڭىز دا, وسى فايلدى جاڭا اتىمەن قوتارىپ بەرىڭىز. [[{{ns:image}}:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'وسىلاي اتالعان فايل ورتاق قويمادا الداقاشان بار;
كەرى قايتىڭىز دا, وسى فايلدى جاڭا اتىمەن قوتارىپ بەرىڭىز. [[{{ns:image}}:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'بۇل فايل كەلەسى {{PLURAL:$1|فايلدىڭ|فايلدارىنىڭ}} تەلنۇسقاسى:',
'successfulupload'            => 'ٴساتتى قوتارىپ بەرىلدى',
'uploadwarning'               => 'قوتارىپ بەرۋ جونىندە قۇلاقتاندىرۋ',
'savefile'                    => 'فايلدى ساقتاۋ',
'uploadedimage'               => '«[[$1]]» فايلىن قوتارىپ بەردى',
'overwroteimage'              => '«[[$1]]» فايلىننىڭ جاڭا نۇسقاسىن قوتارىپ بەردى',
'uploaddisabled'              => 'قوتارىپ بەرۋ وشىرىلگەن',
'uploaddisabledtext'          => '{{SITENAME}} جوباسىندا فايل قوتارىپ بەرۋى وشىرىلگەن.',
'uploadscripted'              => 'بۇل فايلدا ۆەب شولعىشتى قاتەلىكپەن تالداتقىزاتىن HTML نە ٴامىر كودى بار.',
'uploadcorrupt'               => 'بۇل فايل بۇلدىرىلگەن, نە بۇرىس كەڭەيتىمى بار.
فايلدى تەكسەرىپ شىعىڭىز دا, قايتا قوتارىپ بەرىڭىز.',
'uploadvirus'                 => 'بۇل فايلدا ۆىيرۋس بار! ەگجەي-تەگجەيلەرى: $1',
'sourcefilename'              => 'قاينار فايل اتاۋى:',
'destfilename'                => 'نىسانا فايل اتاۋى:',
'upload-maxfilesize'          => 'فايلدىڭ ەڭ كوپ مۇمكىن مولشەرى: $1',
'watchthisupload'             => 'بۇل بەتتى باقىلاۋ',
'filewasdeleted'              => 'بۇل اتاۋى بار فايل بۇرىن قوتارىپ بەرىلگەن دە بەرى كەلە جويىلعان.
بۇنى قايتا قوتارىپ بەرۋ الدىنان $1 دەگەندى تەكسەرىپ شىعىڭىز.',
'upload-wasdeleted'           => "'''قۇلاقتاندىرۋ: الدىندا جويىلعان فايلدى قوتارىپ بەرمەكسىز.'''

بۇل فايلدى قوتارىپ بەرۋىن جالعاستىرۋ ٴۇشىن بۇنىڭ ىڭعايلىعىن تەكسەرىپ شىعۋىڭىز ٴجون.
قولايلى بولۋى ٴۇشىن بۇل فايلدىڭ جويۋ جۋرنالى كەلتىرىلگەن:",
'filename-bad-prefix'         => 'قوتارىپ بەرمەك فايلىڭىزدىڭ اتاۋى <strong>«$1» </strong> دەپ باستالادى, مىناداي سىيپاتتاۋسىز اتاۋدى ادەتتە ساندىق كامەرالار وزدىكتىك بەرەدى.
فايلىڭىزعا سىيپاتتىلاۋ اتاۋدى تاڭداڭىز.',

'upload-proto-error'      => 'بۇرىس حاتتاما',
'upload-proto-error-text' => 'شەتتەن قوتارىپ بەرۋ ٴۇشىن URL جايلارى <code>http://</code> نەمەسە <code>ftp://</code> دەگەندەردەن باستالۋ ٴجون.',
'upload-file-error'       => 'ىشكى قاتە',
'upload-file-error-text'  => 'سەرۆەردە ۋاقىتشا فايل قۇرىلۋى ىشكى قاتەسىنە ۇشىراستى.
بۇل جۇيەنىڭ اكىمشىمەن قاتىناسىڭىز.',
'upload-misc-error'       => 'قوتارىپ بەرۋ كەزىندەگى بەلگىسىز قاتە',
'upload-misc-error-text'  => 'قوتارىپ بەرۋ كەزىندە بەلگىسىز قاتەگە ۇشىراستى.
URL جارامدى جانە قاتىناۋلى ەكەنىن تەكسەرىپ شىعىڭىز دا قايتا بايقاپ كورىڭىز.
ەگەر بۇل ماسەلە الدە دە قالسا, جۇيە اكىمشىمەن قاتىناسىڭىز.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL جەتىلمەدى',
'upload-curl-error6-text'  => 'كەلتىرىلگەن URL جەتىلمەدى.
URL دۇرىس ەكەندىگىن جانە توراپ ىستەپ تۇرعانىن قوس تەكسەرىڭىز.',
'upload-curl-error28'      => 'قوتارىپ بەرۋ ۋاقىتى ٴبىتتى',
'upload-curl-error28-text' => 'توراپتىڭ جاۋاپ بەرۋى تىم ۇزاق ۋاقىتقا سوزىلدى.
بۇل توراپ ىستە ەكەنىن تەكسەرىپ شىعىڭىز, ازعانا كىدىرە تۇرىڭىز دا قايتا بايقاپ كورىڭىز.
تالابىڭىزدى قول تىيگەن كەزىندە قايتا بايقاپ كورۋىڭىز مۇمكىن.',

'license'            => 'لىيتسەنزىييالاندىرۋى:',
'nolicense'          => 'ەشتەڭە بولەكتەنبەگەن',
'license-nopreview'  => '(قاراپ شىعۋ جەتىمدى ەمەس)',
'upload_source_url'  => ' (جارامدى, بارشاعا قاتىناۋلى URL)',
'upload_source_file' => ' (كومپيۋتەرىڭىزدەگى فايل)',

# Special:ImageList
'imagelist-summary'     => 'بۇل ارنايى بەتتە بارلىق قوتارىپ بەرىلگەن فايلدار كورسەتىلەدى.
سوڭعى قوتارىپ بەرىلگەن فايلدار تىزىمدە جوعارعى شەتىمەن ادەپكىدەن كورسەتىلەدى.
باعاننىڭ باس جولىن نۇقىعاندا سۇرىپتاۋدىڭ رەتتەۋى وزگەرتىلەدى.',
'imagelist_search_for'  => 'تاسپا اتاۋىن ىزدەۋ:',
'imgfile'               => 'فايل',
'imagelist'             => 'فايل ٴتىزىمى',
'imagelist_date'        => 'كۇن-ايى',
'imagelist_name'        => 'اتاۋى',
'imagelist_user'        => 'قاتىسۋشى',
'imagelist_size'        => 'مولشەرى',
'imagelist_description' => 'سىيپاتتاماسى',

# Image description page
'filehist'                       => 'فايل تارىيحى',
'filehist-help'                  => 'فايلدىڭ قاي ۋاقىتتا قالاي كورىنەتىن ٴۇشىن كۇن-اي/ۋاقىت دەگەندى نۇقىڭىز.',
'filehist-deleteall'             => 'بارلىعىن جوي',
'filehist-deleteone'             => 'جوي',
'filehist-revert'                => 'قايتار',
'filehist-current'               => 'اعىمداعى',
'filehist-datetime'              => 'كۇن-اي/ۋاقىت',
'filehist-user'                  => 'قاتىسۋشى',
'filehist-dimensions'            => 'ولشەمدەرى',
'filehist-filesize'              => 'فايل مولشەرى',
'filehist-comment'               => 'ماندەمەسى',
'imagelinks'                     => 'سىلتەمەلەر',
'linkstoimage'                   => 'بۇل فايلعا كەلەسى {{PLURAL:$1|بەت|$1 بەت}} سىلتەيدى:',
'nolinkstoimage'                 => 'بۇل فايلعا ەش بەت سىلتەمەيدى.',
'morelinkstoimage'               => 'بۇل فايلدىڭ [[{{#special:Whatlinkshere}}/$1|كوبىرەك سىلتەمەلەرىن]] قاراۋ.',
'redirectstofile'                => 'كەلەسى {{PLURAL:$1|فايل|$1 فايل}} بۇل فايلعا ايدايدى:',
'duplicatesoffile'               => 'كەلەسى {{PLURAL:$1|فايل بۇل فايلدىڭ تەلنۇسقاسى|$1 فايل بۇل فايلدىڭ تەلنۇسقالارى}}:',
'sharedupload'                   => 'بۇل فايل ورتاق قويماعا قوتارىپ بەرىلگەن سوندىقتان باسقا جوبالاردا قولدانۋى مۇمكىن.',
'shareduploadwiki'               => 'بىلايعى اقپارات ٴۇشىن $1 دەگەندى قاراڭىز.',
'shareduploadwiki-desc'          => 'بۇل $1 دەگەن فايلدىڭ ورتاق قويماداعى مالىمەتتەرى تومەندە كورسەتىلەدى.',
'shareduploadwiki-linktext'      => 'فايلدىڭ سىيپاتتاما بەتى',
'shareduploadduplicate'          => 'بۇل فايل ورتاق قويماداعى $1 فايلىنىڭ تەلنۇسقاسى.',
'shareduploadduplicate-linktext' => 'وزگە فايل',
'shareduploadconflict'           => 'بۇل فايل اتاۋى ورتاق قويماداعى $1 فايلىمەن ٴدال كەلەدى',
'shareduploadconflict-linktext'  => 'وزگە فايل',
'noimage'                        => 'بىلاي اتالعان فايل جوق, $1 مۇمكىندىگىڭىز بار.',
'noimage-linktext'               => 'بۇنى قوتارىپ بەر',
'uploadnewversion-linktext'      => 'بۇل فايلدىڭ جاڭا نۇسقاسىن قوتارىپ بەرۋ',
'imagepage-searchdupe'           => 'فايل تەلنۇسقالارىن ىزدەۋ',

# File reversion
'filerevert'                => '$1 دەگەندى قايتارۋ',
'filerevert-legend'         => 'فايلدى قايتارۋ',
'filerevert-intro'          => "'''[[Media:$1|$1]]''' دەگەندى [$4 $3, $2 كەزىندەگى نۇسقاسىنا] قايتارۋداسىز.",
'filerevert-comment'        => 'ماندەمەسى:',
'filerevert-defaultcomment' => '$2, $1 كەزىندەگى نۇسقاسىنا قايتارىلدى',
'filerevert-submit'         => 'قايتار',
'filerevert-success'        => "'''[[Media:$1|$1]]''' دەگەن [$4 $3, $2 كەزىندەگى نۇسقاسىنا] قايتارىلدى.",
'filerevert-badversion'     => 'كەلتىرىلگەن ۋاقىت بەلگىسىمەن بۇل فايلدىڭ الدىڭعى جەرگىلىكتى نۇسقاسى جوق.',

# File deletion
'filedelete'                  => '$1 دەگەندى جويۋ',
'filedelete-legend'           => 'فايلدى جويۋ',
'filedelete-intro'            => "'''[[Media:$1|$1]]''' دەگەندى جويۋداسىز.",
'filedelete-intro-old'        => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' — [$4 $3, $2 كەزىندەگى نۇسقاسىن] جويۋداسىز.</span>',
'filedelete-comment'          => 'جويۋ سەبەبى:',
'filedelete-submit'           => 'جوي',
'filedelete-success'          => "'''$1''' دەگەن جويىلدى.",
'filedelete-success-old'      => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' — $3, $2 كەزىندەگى نۇسقاسى جويىلدى.</span>',
'filedelete-nofile'           => "'''$1''' دەگەن {{SITENAME}} جوباسىندا جوق.",
'filedelete-nofile-old'       => "كەلتىرىلگەن انىقتاۋىشتارىمەن '''$1''' دەگەننىڭ مۇراعاتتالعان نۇسقاسى مىندا جوق.",
'filedelete-iscurrent'        => 'بۇل فايلدىڭ ەڭ سوڭعى نۇسقاسىن جويۋ تالاپ ەتكەنسىز.
ەڭ الدىنان ەسكىلەۋ نۇسقاسىنا قايتارىڭىز.',
'filedelete-otherreason'      => 'باسقا/قوسىمشا سەبەپ:',
'filedelete-reason-otherlist' => 'باسقا سەبەپ',
'filedelete-reason-dropdown'  => '* جويۋدىڭ جالپى سەبەپتەرى
** اۋتورلىق قۇقىقتارىن بۇزۋ
** فايل تەلنۇسقاسى',
'filedelete-edit-reasonlist'  => 'جويۋ سەبەپتەرىن وڭدەۋ',

# MIME search
'mimesearch'         => 'فايلدى MIME تۇرىمەن ىزدەۋ',
'mimesearch-summary' => 'بۇل بەتتە فايلداردى MIME تۇرىمەن سۇزگىلەۋى قوسىلعان.
كىرىسى: ماعلۇمات_تۇرى/تۇر_تاراۋى, مىسالى <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME ٴتۇرى:',
'download'           => 'قوتارىپ الۋ',

# Unwatched pages
'unwatchedpages' => 'باقىلانىلماعان بەتتەر',

# List redirects
'listredirects' => 'ايداتۋ بەت ٴتىزىمى',

# Unused templates
'unusedtemplates'     => 'پايدالانىلماعان ۇلگىلەر',
'unusedtemplatestext' => 'بۇل بەت باسقا بەتكە كىرىcتىرىلمەگەن ۇلگى ەسىم اياىسىنداعى بارلىق بەتتەردى تىزىمدەيدى.
ۇلگىلەردى جويۋ الدىنان بۇنىڭ وزگە سىلتەمەلەرىن تەكسەرىپ شىعۋىن ۇمىتپاڭىز',
'unusedtemplateswlh'  => 'باسقا سىلتەمەلەر',

# Random page
'randompage'         => 'كەزدەيسوق بەت',
'randompage-nopages' => 'بۇل ەسىم اياسىندا بەتتەر جوق.',

# Random redirect
'randomredirect'         => 'كەزدەيسوق ايداعىش',
'randomredirect-nopages' => 'بۇل ەسىم اياسىندا ەش ايداعىش جوق.',

# Statistics
'statistics'             => 'ساناق',
'sitestats'              => '{{SITENAME}} ساناعى',
'userstats'              => 'قاتىسۋشى ساناعى',
'sitestatstext'          => "دەرەكقوردا {{PLURAL:$1|'''1'''|جالپى '''$1'''}} بەت بار.
بۇعان «تالقىلاۋ» بەتتەرى, {{SITENAME}} جوباسى تۋرالى بەتتەر, تىم قىسقا «بىتەمە» بەتتەرى, ايداعىشتار, تاعى دا باسقا ماعلۇمات دەپ تانىلمايتىن بەتتەر كىرىستىرلەدى.
سولاردى ەسەپتەن شىعارعاندا, مىندا ماعلۇمات {{PLURAL:$2|بەتى|بەتتەرى}} دەپ سانالاتىن '''$2''' بەت بار دەپ بولجانادى.

'''$8''' فايل قوتارىپ بەرىلدى.

{{SITENAME}} ورناتىلعاننان بەرى بەتتەر {{PLURAL:$3|'''1'''|جالپى '''$3'''}} رەت قارالعان, جانە بەتتەر '''$4''' رەت وڭدەلگەن.
بۇنىڭ ناتىيجەسىندە ورتاشا ەسەپپەن ٴاربىر بەتكە '''$5''' وڭدەمە كەلەدى, جانە ٴاربىر وڭدەمەگە '''$6''' قاراۋ كەلەدى.

[http://www.mediawiki.org/wiki/Manual:Job_queue تاپسىرىمالار كەزەگىنىڭ] ۇزىندىعى: '''$7'''.",
'userstatstext'          => "مىندا '''$1''' [[{{#special:Listusers}}|تىركەلگەن قاتىسۋشى]] بار, سونىڭ ىشىندە '''$2''' (نە '''$4 %''') قاتىسۋشىسىندا $5 قۇقىقتارى بار",
'statistics-mostpopular' => 'ەڭ كوپ قارالعان بەتتەر',

'disambiguations'      => 'ايرىقتى بەتتەر',
'disambiguationspage'  => '{{ns:template}}:ايرىق',
'disambiguations-text' => "كەلەسى بەتتەر '''ايرىقتى بەتكە''' سىلتەيدى.
بۇنىڭ ورنىنا بەلگىلى تاقىرىپقا سىلتەۋى كەرەك.<br />
ەگەر [[{{ns:mediawiki}}:Disambiguationspage]] تىزىمىندەگى ۇلگى قولدانىلسا, بەت ايرىقتى دەپ سانالادى.",

'doubleredirects'     => 'شىنجىرلى ايداعىشتار',
'doubleredirectstext' => 'بۇل بەتتە باسقا ايداتۋ بەتتەرگە سىلتەيتىن بەتتەر تىزىمدەلىنەدى. ٴاربىر جولاقتا ٴبىرىنشى جانە ەكىنشى ايداعىشقا سىلتەمەلەر بار, سونىمەن بىرگە ەكىنشى ايداعىش نىساناسى بار, ادەتتە بۇل ٴبىرىنشى ايداعىش باعىتتايتىن «ناقتى» نىسانا بەت اتاۋى بولۋى كەرەك.',

'brokenredirects'        => 'ەش بەتكە كەلتىرمەيتىن ايداعىشتار',
'brokenredirectstext'    => 'كەلەسى ايداعىشتار جوق بەتتەرگە سىلتەيدى:',
'brokenredirects-edit'   => '(وڭدەۋ)',
'brokenredirects-delete' => '(جويۋ)',

'withoutinterwiki'         => 'ەش تىلگە سىلتeمەگەن بەتتەر',
'withoutinterwiki-summary' => 'كەلەسى بەتتەر باسقا تىلدەرگە سىلتەمەيدى',
'withoutinterwiki-legend'  => 'باستالۋى:',
'withoutinterwiki-submit'  => 'كورسەت',

'fewestrevisions' => 'ەڭ از تۇزەتىلگەن بەتتەر',

# Miscellaneous special pages
'nbytes'                  => '$1 بايت',
'ncategories'             => '$1 سانات',
'nlinks'                  => '$1 سىلتەمە',
'nmembers'                => '$1 مۇشە',
'nrevisions'              => '$1 تۇزەتۋ',
'nviews'                  => '$1 رەت قارالعان',
'specialpage-empty'       => 'بۇل باياناتقا ەش ناتىيجە جوق.',
'lonelypages'             => 'ەش بەتتەن سىلتەلمەگەن بەتتەر',
'lonelypagestext'         => 'كەلەسى بەتتەرگە {{SITENAME}} جوباسىنداعى باسقا بەتتەر سىلتەمەيدى.',
'uncategorizedpages'      => 'ساناتسىز بەتتەر',
'uncategorizedcategories' => 'ساناتسىز ساناتتار',
'uncategorizedimages'     => 'ساناتسىز فايلدار',
'uncategorizedtemplates'  => 'ساناتسىز ۇلگىلەر',
'unusedcategories'        => 'پايدالانىلماعان ساناتتار',
'unusedimages'            => 'پايدالانىلماعان فايلدار',
'popularpages'            => 'ەڭ كوپ قارالعان بەتتەر',
'wantedcategories'        => 'باستالماعان ساناتتار',
'wantedpages'             => 'باستالماعان بەتتەر',
'missingfiles'            => 'جوق فايلدار',
'mostlinked'              => 'ەڭ كوپ سىلتەلگەن بەتتەر',
'mostlinkedcategories'    => 'ەڭ كوپ پايدالانىلعان ساناتتار',
'mostlinkedtemplates'     => 'ەڭ كوپ پايدالانىلعان ۇلگىلەر',
'mostcategories'          => 'ەڭ كوپ ساناتى بار بەتتەر',
'mostimages'              => 'ەڭ كوپ پايدالانىلعان فايلدار',
'mostrevisions'           => 'ەڭ كوپ تۇزەتىلگەن بەتتەر',
'prefixindex'             => 'اتاۋ باستاۋىش ٴتىزىمى',
'shortpages'              => 'ەڭ قىسقا بەتتەر',
'longpages'               => 'ەڭ ۇزىن بەتتەر',
'deadendpages'            => 'ەش بەتكە سىلتەمەيتىن بەتتەر',
'deadendpagestext'        => 'كەلەسى بەتتەر {{SITENAME}} جوباسىنداعى باسقا بەتتەرگە سىلتەمەيدى.',
'protectedpages'          => 'قورعالعان بەتتەر',
'protectedpages-indef'    => 'تەك بەلگىسىز قورعاۋلار',
'protectedpagestext'      => 'كەلەسى بەتتەر وڭدەۋدەن نەمەسە جىلجىتۋدان قورعالعان',
'protectedpagesempty'     => 'اعىمدا مىناداي باپتالىمدارىمەن ەشبىر بەت قورعالماعان',
'protectedtitles'         => 'قورعالعان تاقىرىپ اتتارى',
'protectedtitlestext'     => 'كەلەسى تاقىرىپ اتتارىن باستاۋعا رۇقسات بەرىلمەگەن',
'protectedtitlesempty'    => 'بۇل باپتالىمدارمەن اعىمدا ەش تاقىرىپ اتتارى قورعالماعان.',
'listusers'               => 'قاتىسۋشى ٴتىزىمى',
'newpages'                => 'ەڭ جاڭا بەتتەر',
'newpages-username'       => 'قاتىسۋشى اتى:',
'ancientpages'            => 'ەڭ ەسكى بەتتەر',
'move'                    => 'جىلجىتۋ',
'movethispage'            => 'بەتتى جىلجىتۋ',
'unusedimagestext'        => '<p>اڭعارتپا: عالامتورداعى باسقا توراپتار فايلعا تۋرا URL ارقىلى سىلتەۋى مۇمكىن. سوندىقتان, بەلسەندى پايدالانۋىنا اڭعارماي, وسى تىزىمدە قالۋى مۇمكىن.</p>',
'unusedcategoriestext'    => 'كەلەسى سانات بەتتەرى بار بوپ تۇر, بىراق وعان ەش بەت نە سانات كىرمەيدى.',
'notargettitle'           => 'نىسانا جوق',
'notargettext'            => 'وسى جەتە ورىندالاتىن نىسانا بەتتى, نە قاتىسۋشىنى ەنگىزبەپسىز.',
'nopagetitle'             => 'مىناداي ەش نىسانا بەت جوق',
'nopagetext'              => 'كەلتىرىلگەن نىسانا بەتىڭىز جوق.',
'pager-newer-n'           => 'جاڭالاۋ $1',
'pager-older-n'           => 'ەسكىلەۋ $1',
'suppress'                => 'شەتتەتۋ',

# Book sources
'booksources'               => 'كىتاپ قاينارلارى',
'booksources-search-legend' => 'كىتاپ قاينارلارىن ىزدەۋ',
'booksources-go'            => 'ٴوتۋ',
'booksources-text'          => 'تومەندە جاڭا جانە قولدانعان كىتاپتار ساتاتىن توراپتارىنىڭ سىلتەمەلەرى تىزىمدەلگەن. بۇل توراپتاردا ىزدەلگەن كىتاپتار تۋرالى بىلايعى اقپارات بولۋعا مۇمكىن.',

# Special:Log
'specialloguserlabel'  => 'قاتىسۋشى:',
'speciallogtitlelabel' => 'تاقىرىپ اتى:',
'log'                  => 'جۋرنالدار',
'all-logs-page'        => 'بارلىق جۋرنالدار',
'log-search-legend'    => 'جۋرنالداردان ىزدەۋ',
'log-search-submit'    => 'ٴوت',
'alllogstext'          => '{{SITENAME}} جوباسىنىڭ بارلىق قاتىناۋلى جۋرنالدارىن بىرىكتىرىپ كورسەتۋى.
جۋرنال ٴتۇرىن, قاتىسۋشى اتىن, نە ٴتىيىستى بەتىن بولەكتەپ, تارىلتىپ قاراي الاسىز.',
'logempty'             => 'جۋرنالدا سايكەس دانالار جوق.',
'log-title-wildcard'   => 'مىنا ماتىننەڭ باستالىتىن تاقىرىپ اتتارىن ىزدەۋ',

# Special:AllPages
'allpages'          => 'بارلىق بەتتەر',
'alphaindexline'    => '$1 — $2',
'nextpage'          => 'كەلەسى بەتكە ($1)',
'prevpage'          => 'الدىڭعى بەتكە ($1)',
'allpagesfrom'      => 'مىنا بەتتەن باستاپ كورسەتۋ:',
'allarticles'       => 'بارلىق بەت ٴتىزىمى',
'allinnamespace'    => 'بارلىق بەت ($1 ەسىم اياسى)',
'allnotinnamespace' => 'بارلىق بەت ($1 ەسىم اياسىنان تىس)',
'allpagesprev'      => 'الدىڭعىعا',
'allpagesnext'      => 'كەلەسىگە',
'allpagessubmit'    => 'ٴوتۋ',
'allpagesprefix'    => 'مىنادان باستالعان بەتتەردى كورسەتۋ:',
'allpagesbadtitle'  => 'كەلتىرىلگەن بەت تاقىرىبىن اتى جارامسىز بولعان, نەمەسە ٴتىل-ارالىق نە ۋىيكىي-ارالىق باستاۋى بار بولدى.
مىندا تاقىرىپ اتىندا قولدالمايتىن بىرقاتار تاڭبالار بولۋى مۇمكىن.',
'allpages-bad-ns'   => '{{SITENAME}} جوباسىندا «$1» ەسىم اياسى جوق.',

# Special:Categories
'categories'                    => 'ساناتتار',
'categoriespagetext'            => 'كەلەسى ساناتتار ىشىندە بەتتەر نە تاسپالار بار.',
'categoriesfrom'                => 'ساناتتاردى مىنادان باستاپ كورسەتۋ:',
'special-categories-sort-count' => 'سانىمەن سۇرىپتاۋ',
'special-categories-sort-abc'   => 'الىپبىيمەن سۇرىپتاۋ',

# Special:ListUsers
'listusersfrom'      => 'مىنا قاتىسۋشىدان باستاپ كورسەتۋ:',
'listusers-submit'   => 'كورسەت',
'listusers-noresult' => 'قاتىسۋشى تابىلعان جوق.',

# Special:ListGroupRights
'listgrouprights'          => 'قاتىسۋشى توبى قۇقىقتارى',
'listgrouprights-summary'  => 'كەلەسى تىزىمدە بۇل ۋىيكىيدە تاعايىندالعان قاتىسۋشى قۇقىقتارى (بايلانىستى قاتىناۋ قۇقىقتارىمەن بىرگە) كورسەتىلەدى.
جەكە قۇقىقتار تۋرالى كوبىرەك اقپاراتتى [[{{MediaWiki:Listgrouprights-helppage}}|مىندا]] تابا الاسىز.',
'listgrouprights-group'    => 'توپ',
'listgrouprights-rights'   => 'قۇقىقتارى',
'listgrouprights-helppage' => '{{ns:help}}:توپ قۇقىقتارى',
'listgrouprights-members'  => '(مۇشە ٴتىزىمى)',

# E-mail user
'mailnologin'     => 'ەش مەكەنجاي جونەلتىلگەن جوق',
'mailnologintext' => 'باسقا قاتىسۋشىعا حات جونەلتۋ ٴۇشىن [[Special:UserLogin|كىرۋىڭىز]] كەرەك, جانە [[Special:Preferences|باپتاۋىڭىزدا]] جارامدى ە-پوشتا جايى بولۋى ٴجون.',
'emailuser'       => 'قاتىسۋشىعا حات جازۋ',
'emailpage'       => 'قاتىسۋشىعا حات جازۋ',
'emailpagetext'   => 'ەگەر بۇل قاتىسۋشى باپتاۋلارىندا جارامدى ە-پوشتا مەكەنجايىن ەنگىزسە, تومەندەگى ٴپىشىن ارقىلى بۇعان جالعىز ە-پوشتا حاتىن جونەلتۋگە بولادى.
قاتىسۋشى باپتاۋىڭىزدا ەنگىزگەن ە-پوشتا مەكەنجايىڭىز «كىمنەن» دەگەن باس جولاعىندا كورىنەدى, سوندىقتان حات الۋشىسى تۋرا جاۋاپ بەرە الادى.',
'usermailererror' => 'Mail نىسانى قاتە قايتاردى:',
'defemailsubject' => '{{SITENAME}} ە-پوشتاسىنىڭ حاتى',
'noemailtitle'    => 'ەش ە-پوشتا مەكەنجايى جوق',
'noemailtext'     => 'بۇل قاتىسۋشى جارامدى ە-پوشتا مەكەنجايىن كەلتىرمەگەن, نە باسقالاردان حات قابىلداۋىن وشىرگەن.',
'emailfrom'       => 'كىمنەن',
'emailto'         => 'كىمگە',
'emailsubject'    => 'تاقىرىبى',
'emailmessage'    => 'حات',
'emailsend'       => 'جونەلتۋ',
'emailccme'       => 'حاتىمدىڭ كوشىرمەسىن ماعان دا جونەلت.',
'emailccsubject'  => '$1 دەگەنگە حاتىڭىزدىڭ كوشىرمەسى: $2',
'emailsent'       => 'حات جونەلتىلدى',
'emailsenttext'   => 'ە-پوشتا حاتىڭىز جونەلتىلدى.',

# Watchlist
'watchlist'            => 'باقىلاۋ ٴتىزىمى',
'mywatchlist'          => 'باقىلاۋىم',
'watchlistfor'         => "('''$1''' باقىلاۋلارى)",
'nowatchlist'          => 'باقىلاۋ تىزىمىڭىزدە ەش دانا جوق',
'watchlistanontext'    => 'باقىلاۋ تىزىمىڭىزدەگى دانالاردى قاراۋ, نە وڭدەۋ ٴۇشىن $1 كەرەك.',
'watchnologin'         => 'كىرمەگەنسىز',
'watchnologintext'     => 'باقىلاۋ ٴتىزىمىڭىزدى وزگەرتۋ ٴۇشىن [[Special:UserLogin|كىرۋىڭىز]] ٴجون.',
'addedwatch'           => 'باقىلاۋ تىزىمىنە ۇستەلدى',
'addedwatchtext'       => "«[[:$1]]» بەتى [[{{#special:Watchlist}}|باقىلاۋ تىزىمىڭىزگە]] ۇستەلدى.
بۇل بەتتىڭ جانە بايلانىستى تالقىلاۋ بەتىنىڭ كەلەشەكتەگى وزگەرىستەرى مىندا تىزىمدەلىنەدى دە, جانە بەتتىڭ اتاۋى جەڭىل تابىلۋ ٴۇشىن [[{{#special:Recentchanges}}|جۋىقتاعى وزگەرىستەر تىزىمىندە]] '''جۋان ارپىمەن''' كورسەتىلەدى.",
'removedwatch'         => 'باقىلاۋ تىزىمىڭىزدەن الاستالدى',
'removedwatchtext'     => '«[[:$1]]» بەتى باقىلاۋ تىزىمىڭىزدەن الاستالدى.',
'watch'                => 'باقىلاۋ',
'watchthispage'        => 'بەتتى باقىلاۋ',
'unwatch'              => 'باقىلاماۋ',
'unwatchthispage'      => 'باقىلاۋدى توقتاتۋ',
'notanarticle'         => 'ماعلۇمات بەتى ەمەس',
'notvisiblerev'        => 'تۇزەتۋ جويىلدى',
'watchnochange'        => 'كورسەتىلگەن مەرزىمدە ەش باقىلانعان دانا وڭدەلگەن جوق.',
'watchlist-details'    => 'تالقىلاۋ بەتتەرىن ساناماعاندا $1 بەت باقلانىلادى.',
'wlheader-enotif'      => '* ەسكەرتۋ حات جىبەرۋى قوسىلعان.',
'wlheader-showupdated' => "* سوڭعى كەلىپ-كەتۋىڭىزدەن بەرى وزگەرتىلگەن بەتتەردى '''جۋان''' قارىپىمەن كورسەت",
'watchmethod-recent'   => 'باقىلاۋلى بەتتەر ٴۇشىن جۋىقتاعى وزگەرىستەردى تەكسەرۋ',
'watchmethod-list'     => 'جۋىقتاعى وزگەرىستەر ٴۇشىن باقىلاۋلى بەتتەردى تەكسەرۋ',
'watchlistcontains'    => 'باقىلاۋ تىزىمىڭىزدە $1 بەت بار.',
'iteminvalidname'      => "'$1' دانادا اقاۋ بار — جارامسىز اتاۋ…",
'wlnote'               => "تومەندە سوڭعى {{PLURAL:$2|ساعاتتا|'''$2''' ساعاتتا}} بولعان, {{PLURAL:$1|جۋىقتاعى وزگەرىس|جۋىقتاعى '''$1''' وزگەرىس}} كورسەتىلەدى.",
'wlshowlast'           => 'سوڭعى $1 ساعاتتاعى, $2 كۇندەگى, $3 بولعان وزگەرىستى كورسەتۋ',
'watchlist-show-bots'  => 'بوت وڭدەمەلەرىن كورسەت',
'watchlist-hide-bots'  => 'بوت وڭدەمەلەرىن جاسىر',
'watchlist-show-own'   => 'وڭدەمەلەرىمدى كورسەت',
'watchlist-hide-own'   => 'وڭدەمەلەرىمدى جاسىر',
'watchlist-show-minor' => 'شاعىن وڭدەمەلەردى كورسەت',
'watchlist-hide-minor' => 'شاعىن وڭدەمەلەردى جاسىر',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'باقىلاۋدا…',
'unwatching' => 'باقىلاماۋدا…',

'enotif_mailer'                => '{{SITENAME}} ەسكەرتۋ حات جىبەرۋ قىزمەتى',
'enotif_reset'                 => 'بارلىق بەت كەلىپ-كەتىلدى دەپ بەلگىلە',
'enotif_newpagetext'           => 'مىناۋ جاڭا بەت.',
'enotif_impersonal_salutation' => '{{SITENAME}} قاتىسۋشىسى',
'changed'                      => 'وزگەرتتى',
'created'                      => 'باستادى',
'enotif_subject'               => '{{SITENAME}} جوباسىندا $PAGEEDITOR $PAGETITLE اتاۋلى بەتتى $CHANGEDORCREATED',
'enotif_lastvisited'           => 'سوڭعى كەلىپ-كەتۋىڭىزدەن بەرى بولعان وزگەرىستەر ٴۇشىن $1 دەگەندى قاراڭىز.',
'enotif_lastdiff'              => 'وسى وزگەرىس ٴۇشىن $1 دەگەندى قاراڭىز.',
'enotif_anon_editor'           => 'تىركەلگىسىز قاتىسۋشى $1',
'enotif_body'                  => 'قادىرلى $WATCHINGUSERNAME,


{{SITENAME}} جوباسىنىڭ $PAGETITLE اتاۋلى بەتتى $PAGEEDITDATE كەزىندە $PAGEEDITOR دەگەن $CHANGEDORCREATED, اعىمدىق نۇسقاسى ٴۇشىن $PAGETITLE_URL قاراڭىز.

$NEWPAGE

وڭدەۋشى كەلتىرگەن قىسقاشا مازمۇنداماسى: $PAGESUMMARY $PAGEMINOREDIT

وڭدەۋشىمەن قاتىناسۋ:
ە-پوشتا: $PAGEEDITOR_EMAIL
ۋىيكىي: $PAGEEDITOR_WIKI

بىلايعى وزگەرىستەر بولعاندا دا وسى بەتكە كەلىپ-كەتۋىڭىزگەنشە دەيىن ەشقانداي باسقا ەسكەرتۋ حاتتار جىبەرىلمەيدى.
سونىمەن قاتار باقىلاۋ تىزىمىڭىزدەگى بەت ەسكەرتپەلىك بەلگىسىن قايتا قويىڭىز.

             ٴسىزدىڭ دوستىق {{SITENAME}} جوباسىنىڭ ەسكەرتۋ قىزمەتى

----
باقىلاۋ ٴتىزىمىڭىزدىڭ باپتاۋلىرىن وزگەرتۋ ٴۇشىن, مىندا كەلىپ-كەتىڭىز:
{{fullurl:{{#special:Watchlist}}/edit}}

سىن-پىكىر بەرۋ جانە بىلايعى جاردەم الۋ ٴۇشىن:
{{fullurl:{{{{ns:mediawiki}}:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'بەتتى جويۋ',
'confirm'                     => 'قۇپتاۋ',
'excontent'                   => "بولعان ماعلۇماتى: '$1'",
'excontentauthor'             => "بولعان ماعلۇماتى (تەك '[[{{#special:Contributions}}/$2|$2]]' ۇلەسى): '$1'",
'exbeforeblank'               => "تازارتۋ الدىنداعى بولعان ماعلۇماتى: '$1'",
'exblank'                     => 'بەت بوس بولدى',
'delete-confirm'              => '«$1» دەگەندى جويۋ',
'delete-legend'               => 'جويۋ',
'historywarning'              => 'قۇلاقتاندىرۋ: جويۋى كوزدەلگەن بەتتە تارىيحى بار:',
'confirmdeletetext'           => 'بەتتى بۇكىل تارىيحىمەن بىرگە دەرەكقوردان جوييۋىن كوزدەدىڭىز.
وسىنى ىستەۋ نىييەتىڭىزدى, سالدارىن بايىمداۋىڭىزدى جانە [[{{{{ns:mediawiki}}:Policy-url}}]] دەگەنگە لايىقتى دەپ ىستەمەكتەنگەڭىزدى قۇپتاڭىز.',
'actioncomplete'              => 'ارەكەت ٴبىتتى',
'deletedtext'                 => '«$1» جويىلدى.
جۋىقتاعى جويۋلار تۋرالى جازبالارىن $2 دەگەننەن قاراڭىز.',
'deletedarticle'              => '«[[$1]]» دەگەندى جويدى',
'suppressedarticle'           => '«[[$1]]» دەگەندى شەتتەتتى',
'dellogpage'                  => 'جويۋ_جۋرنالى',
'dellogpagetext'              => 'تومەندە جۋىقتاعى جويۋلاردىڭ ٴتىزىمى بەرىلگەن.',
'deletionlog'                 => 'جويۋ جۋرنالى',
'reverted'                    => 'ەرتەرەك تۇزەتۋىنە قايتارىلعان',
'deletecomment'               => 'جويۋدىڭ سەبەبى:',
'deleteotherreason'           => 'باسقا/قوسىمشا سەبەپ:',
'deletereasonotherlist'       => 'باسقا سەبەپ',
'deletereason-dropdown'       => '* جويۋدىڭ جالپى سەبەپتەرى
** اۋتوردىڭ سۇرانىمى بويىنشا
** اۋتورلىق قۇقىقتارىن بۇزۋ
** بۇزاقىلىق',
'delete-edit-reasonlist'      => 'جويۋ سەبەپتەرىن وڭدەۋ',
'delete-toobig'               => 'بۇل بەتتە بايتاق تۇزەتۋ تارىيحى بار, $1 تۇزەتۋدەن استام.
بۇنداي بەتتەردىڭ جويۋى {{SITENAME}} تورابىن الدەقالاي ٴۇزىپ تاستاۋىنا بوگەت سالۋ ٴۇشىن تىيىمدالعان.',
'delete-warning-toobig'       => 'بۇل بەتتە بايتاق تۇزەتۋ تارىيحى بار, $1 تۇزەتۋدەن استام.
بۇنىڭ جويۋى {{SITENAME}} تورابىنداعى دەرەكقور ارەكەتتەردى ٴۇزىپ تاستاۋىن مۇمكىن;
بۇنى ابايلاپ وتكىزىڭىز.',
'rollback'                    => 'وڭدەمەلەردى شەگىندىرۋ',
'rollback_short'              => 'شەگىندىرۋ',
'rollbacklink'                => 'شەگىندىرۋ',
'rollbackfailed'              => 'شەگىندىرۋ ٴساتسىز ٴبىتتى',
'cantrollback'                => 'وڭدەمە قايتارىلمادى;
سوڭعى ۇلەسكەرى تەك وسى بەتتىڭ باستاۋشىسى بولدى.',
'alreadyrolled'               => '[[{{ns:user}}:$2|$2]] ([[{{ns:user_talk}}:$2|تالقىلاۋى]]) ىستەگەن [[:$1]] سوڭعى وڭدەمەسى شەگىندىرىلمەدى;
باسقا بىرەۋ بۇل بەتتى الداقاشان وڭدەگەن نە شەگىندىرگەن.

سوڭعى وڭدەمەسىن [[{{ns:user}}:$3|$3]] ([[{{ns:user_talk}}:$3|تالقىلاۋى]]) ىستەگەن.',
'editcomment'                 => 'بولعان وڭدەمە ماندەمەسى: «<i>$1</i>».', # only shown if there is an edit comment
'revertpage'                  => '[[{{#special:Contributions}}/$2|$2]] ([[{{ns:user_talk}}:$2|تالقىلاۋى]]) وڭدەمەلەرىنەن [[{{ns:user}}:$1|$1]] سوڭعى نۇسقاسىنا قايتاردى', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '$1 وڭدەمەلەرىنەن قايتارعان;
$2 سوڭعى نۇسقاسىنا وزگەرتتى.',
'sessionfailure'              => 'كىرۋ سەسسىيياسىندا شاتاق بولعان سىيياقتى;
سەسسىيياعا شابۋىلداۋداردان قورعانۋ ٴۇشىن, وسى ارەكەت توقتاتىلدى.
«ارتقا» دەگەندى باسىڭىز, جانە بەتتى قايتا جۇكتەڭىز دە, قايتا بايقاپ كورىڭىز.',
'protectlogpage'              => 'قورعاۋ جۋرنالى',
'protectlogtext'              => 'تومەندە بەتتەردىڭ قورعاۋ/قورعاماۋ ٴتىزىمى بەرىلگەن.
اعىمداعى قورعاۋ ارەكتتەر بار بەتتەر ٴۇشىن [[{{#special:Protectedpages}}|قورعالعان بەت ٴتىزىمىن]] قاراڭىز.',
'protectedarticle'            => '«[[$1]]» قورعالدى',
'modifiedarticleprotection'   => '«[[$1]]» قورعالۋ دەڭگەيى وزگەردى',
'unprotectedarticle'          => '«[[$1]]» قورعالۋى ٴوشىرىلدى',
'protect-title'               => '«$1» قورعاۋ دەڭگەيىن وزگەرتۋ',
'protect-legend'              => 'قورعاۋدى قۇپتاۋ',
'protectcomment'              => 'ماندەمەسى:',
'protectexpiry'               => 'مەرزىمى بىتپەك:',
'protect_expiry_invalid'      => 'بىتەتىن ۋاقىتى جارامسىز.',
'protect_expiry_old'          => 'بىتەتىن ۋاقىتى ٴوتىپ كەتكەن.',
'protect-unchain'             => 'جىلجىتۋ رۇقساتتارىن بەرۋ',
'protect-text'                => '<strong>$1</strong> بەتىنىڭ قورعاۋ دەڭگەيىن قاراپ جانە وزگەرتىپ شىعا الاسىز.',
'protect-locked-blocked'      => 'بۇعاتتاۋىڭىز وشىرىلگەنشە دەيىن قورعاۋ دەڭگەيىن وزگەرتە المايسىز.
مىنا <strong>$1</strong> بەتتىڭ اعىمدىق باپتاۋلارى:',
'protect-locked-dblock'       => 'دەرەكقوردىڭ قۇلىپتاۋى بەلسەندى بولعاندىقتان قورعاۋ دەڭگەيلەرى وزگەرتىلمەيدى.
مىنا <strong>$1</strong> بەتتىڭ اعىمدىق باپتاۋلارى:',
'protect-locked-access'       => 'تىركەلگىڭىزگە بەت قورعاۋ دەنگەيلەرىن وزگەرتۋىنە رۇقسات جوق.
مىنا <strong>$1</strong> بەتتىڭ اعىمدىق باپتاۋلارى:',
'protect-cascadeon'           => 'بۇل بەت اعىمدا قورعالعان, سەبەبى وسى بەت «باۋلى قورعاۋى» بار كەلەسى {{PLURAL:$1|بەتتىڭ|بەتتەردىڭ}} كىرىكبەتى.
بۇل بەتتىڭ قورعاۋ دەڭگەيىن وزگەرتە الاسىز, بىراق بۇل باۋلى قورعاۋعا ىقپال ەتپەيدى.',
'protect-default'             => '(ادەپكى)',
'protect-fallback'            => '«$1» رۇقساتى كەرەك',
'protect-level-autoconfirmed' => 'تىركەلگىسىزدەرگە تىيىم',
'protect-level-sysop'         => 'تەك اكىمشىلەر',
'protect-summary-cascade'     => 'باۋلى',
'protect-expiring'            => 'مەرزىمى بىتپەك: $1 (UTC)',
'protect-cascade'             => 'بۇل بەتتىڭ كىرىكبەتتەرىن قورعاۋ (باۋلى قورعاۋ).',
'protect-cantedit'            => 'بۇل بەتتىڭ قورعاۋ دەڭگەيىن وزگەرتە المايسىز, سەبەبى بۇنى وڭدەۋگە رۇقستاڭىز جوق.',
'restriction-type'            => 'رۇقساتى:',
'restriction-level'           => 'تىيىمدىق دەڭگەيى:',
'minimum-size'                => 'ەڭ از مولشەرى',
'maximum-size'                => 'ەڭ كوپ مولشەرى:',
'pagesize'                    => '(بايت)',

# Restrictions (nouns)
'restriction-edit'   => 'وڭدەۋگە',
'restriction-move'   => 'جىلجىتۋعا',
'restriction-create' => 'باستاۋعا',
'restriction-upload' => 'قوتارىپ بەرۋگە',

# Restriction levels
'restriction-level-sysop'         => 'تولىقتاي قورعالعان',
'restriction-level-autoconfirmed' => 'جارتىلاي قورعالعان',
'restriction-level-all'           => 'ٴار دەڭگەيدە',

# Undelete
'undelete'                     => 'جويىلعان بەتتەردى قاراۋ',
'undeletepage'                 => 'جويىلعان بەتتەردى قاراۋ جانە قالپىنا كەلتىرۋ',
'undeletepagetitle'            => "'''كەلەسى ٴتىزىم [[:$1|$1]] دەگەننىڭ جويىلعان تۇزەتۋلەرىنەن تۇرادى'''.",
'viewdeletedpage'              => 'جويىلعان بەتتەردى قاراۋ',
'undeletepagetext'             => 'كەلەسى بەتتەر جويىلدى دەپ بەلگىلەنگەن, بىراق ماعلۇماتى مۇراعاتتا بار
جانە قالپىنا كەلتىرۋگە مۇمكىن. مۇراعات مەرزىم بويىنشا تازالانىپ تۇرۋى مۇمكىن.',
'undeleteextrahelp'            => "بۇكىل بەتتى قالپىنا كەلتىرۋ ٴۇشىن, بارلىق قۇسبەلگى كوزدەردى بوساتىپ '''''قالپىنا كەلتىر!''''' باتىرماسىن نۇقىڭىز.
بولەكتەۋمەن قالپىنا كەلتىرۋ ورىنداۋ ٴۇشىن, كەلتىرەمىن دەگەن تۇزەتۋلەرىنە سايكەس كوزدەرگە قۇسبەلگى سالىڭىز دا, جانە '''''قالپىنا كەلتىر!''''' تۇيمەسىن نۇقىڭىز. '''''قايتا قوي''''' تۇيمەسىن نۇقىعاندا ماندەمە اۋماعى تازارتادى جانە بارلىق قۇسبەلگى كوزدەرىن بوساتادى.",
'undeleterevisions'            => '$1 تۇزەتۋ مۇراعاتتالدى',
'undeletehistory'              => 'ەگەر بەت ماعلۇماتىن قالپىنا كەلتىرسەڭىز, تارىيحىندا بارلىق تۇزەتۋلەر دا
قايتارىلادى. ەگەر جويۋدان سوڭ ٴدال سولاي اتاۋىمەن جاڭا بەت باستالسا, قالپىنا كەلتىرىلگەن تۇزەتۋلەر
تارىيحتىڭ الدىندا كورسەتىلەدى. تاعى دا فايل تۇزەتۋلەرىن قالپىنا كەلتىرگەندە تىيىمدارى جويىلاتىن ەسكەرىڭىز.',
'undeleterevdel'               => 'ەگەر بۇل ۇستىڭگى بەتتە اياقتالسا, نە فايل تۇزەتۋى جارىم-جارتىلاي جويىلعان بولسا, جويۋ بولدىرماۋى ورىندالمايدى.
وسىنداي جاعدايلاردا, ەڭ جاڭا جويىلعان تۇزەتۋىن الىپ تاستاۋىڭىز نە جاسىرۋىن بولدىرماۋىڭىز ٴجون.',
'undeletehistorynoadmin'       => 'بۇل بەت جويىلعان.
جويۋ سەبەبى الدىنداعى وڭدەگەن قاتىسۋشىلار ەگجەي-تەگجەيلەرىمەن بىرگە تومەندەگى قىسقاشا مازمۇنداماسىندا كورسەتىلگەن.
مىنا جويىلعان تۇزەتۋلەرىن كوكەيكەستى ٴماتىنى تەك اكىمشىلەرگە جەتىمدى.',
'undelete-revision'            => '$2 كەزىندەگى $3 جويعان $1 دەگەننىڭ جويىلعان تۇزەتۋى:',
'undeleterevision-missing'     => 'جارامسىز نە جوعالعان تۇزەتۋ.
سىلتەمەڭىز جارامسىز, نە تۇزەتۋ قالپىنا كەلتىرىلگەن, نەمەسە مۇراعاتتان الاستالعان بولۋى مۇمكىن.',
'undelete-nodiff'              => 'ەش الدىڭعى تۇزەتۋ تابىلمادى.',
'undeletebtn'                  => 'قالپىنا كەلتىر!',
'undeletelink'                 => 'قالپىنا كەلتىرۋ',
'undeletereset'                => 'قايتا قوي',
'undeletecomment'              => 'ماندەمەسى:',
'undeletedarticle'             => '«[[$1]]» قالپىنا كەلتىرىلدى',
'undeletedrevisions'           => '$1 تۇزەتۋ قالپىنا كەلتىرىلدى',
'undeletedrevisions-files'     => '$1 تۇزەتۋ جانە $2 فايل قالپىنا كەلتىرىلدى',
'undeletedfiles'               => '$1 فايل قالپىنا كەلتىرىلدى',
'cannotundelete'               => 'جويۋ بولدىرماۋى ٴساتسىز ٴبىتتى;
باسقا بىرەۋ العاشىندا بەتتىڭ جويۋدىڭ بولدىرماۋى مۇمكىن.',
'undeletedpage'                => "<big>'''$1 قالپىنا كەلتىرىلدى'''</big>

جۋىقتاعى جويۋلار مەن قالپىنا كەلتىرۋلەر جونىندە [[{{#special:Log}}/delete|جويۋ جۋرنالىن]] قاراڭىز.",
'undelete-header'              => 'جۋىقتاعى جويىلعان بەتتەر جونىندە [[{{#special:Log}}/delete|جويۋ جۋرنالىن]] قاراڭىز.',
'undelete-search-box'          => 'جويىلعان بەتتەردى ىزدەۋ',
'undelete-search-prefix'       => 'مىنادان باستالعان بەتتەردى كورسەت:',
'undelete-search-submit'       => 'ىزدەۋ',
'undelete-no-results'          => 'جويۋ مۇراعاتىندا ەشقانداي سايكەس بەتتەر تابىلمادى.',
'undelete-filename-mismatch'   => '$1 كەزىندەگى فايل تۇزەتۋىنىڭ جويۋى بولدىرمادى: فايل اتاۋى سايكەسسىز',
'undelete-bad-store-key'       => '$1 كەزىندەگى فايل تۇزەتۋىنىڭ جويۋى بولدىرمادى: جويۋدىڭ الدىنان فايل جوق بولعان.',
'undelete-cleanup-error'       => '«$1» پايدالانىلماعان مۇراعاتتالعان فايل جويۋ قاتەسى.',
'undelete-missing-filearchive' => 'مۇراعاتتالعان فايل (ٴنومىرى $1) قالپىنا كەلتىرۋى ىيكەمدى ەمەس, سەبەبى ول دەرەكقوردا جوق.
بۇنىڭ جويۋىن بولدىرماۋى الداقاشان بولعانى مۇمكىن.',
'undelete-error-short'         => 'فايل جويۋىن بولدىرماۋ قاتەسى: $1',
'undelete-error-long'          => 'فايل جويۋىن بولدىرماۋ كەزىندە مىنا قاتەلەر كەزدەستى:

$1',

# Namespace form on various pages
'namespace'      => 'ەسىم اياسى:',
'invert'         => 'بولەكتەۋدى كەرىلەۋ',
'blanknamespace' => '(نەگىزگى)',

# Contributions
'contributions' => 'قاتىسۋشى ۇلەسى',
'mycontris'     => 'ۇلەسىم',
'contribsub2'   => '$1 ($2) ۇلەسى',
'nocontribs'    => 'وسى ىزدەۋ شارتىنا سايكەس وزگەرىستەر تابىلعان جوق.',
'uctop'         => ' (ٴۇستى)',
'month'         => 'مىنا ايدان (جانە ەرتەرەكتەن):',
'year'          => 'مىنا جىلدان (جانە ەرتەرەكتەن):',

'sp-contributions-newbies'     => 'تەك جاڭا تىركەلگىدەن جاساعان ۇلەستەردى كورسەت',
'sp-contributions-newbies-sub' => 'جاڭادان تىركەلگى جاساعاندار ٴۇشىن',
'sp-contributions-blocklog'    => 'بۇعاتتاۋ جۋرنالى',
'sp-contributions-search'      => 'ۇلەس ٴۇشىن ىزدەۋ',
'sp-contributions-username'    => 'IP مەكەنجايى نە قاتىسۋشى اتى:',
'sp-contributions-submit'      => 'ىزدە',

# What links here
'whatlinkshere'            => 'سىلتەلگەن بەتتەر',
'whatlinkshere-title'      => '$1 دەگەنگە سىلتەلگەن بەتتەر',
'whatlinkshere-page'       => 'بەت:',
'linklistsub'              => '(سىلتەمەلەر ٴتىزىمى)',
'linkshere'                => "'''[[:$1]]''' دەگەنگە مىنا بەتتەر سىلتەيدى:",
'nolinkshere'              => "'''[[:$1]]''' دەگەنگە ەش بەت سىلتەمەيدى.",
'nolinkshere-ns'           => "تاڭدالعان ەسىم اياسىندا '''[[:$1]]''' دەگەنگە ەشقانداي بەت سىلتەمەيدى.",
'isredirect'               => 'ايداتۋ بەتى',
'istemplate'               => 'كىرىكبەت',
'isimage'                  => 'سۋرەت سىلتەمەسى',
'whatlinkshere-prev'       => '{{PLURAL:$1|الدىڭعى|الدىڭعى $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|كەلەسى|كەلەسى $1}}',
'whatlinkshere-links'      => '← سىلتەمەلەر',
'whatlinkshere-hideredirs' => 'ايداعىشتاردى $1',
'whatlinkshere-hidetrans'  => 'كىرىكبەتتەردى $1',
'whatlinkshere-hidelinks'  => 'سىلتەمەلەردى $1',
'whatlinkshere-hideimages' => 'سۋرەت سىلتەمەلەرىن $1',
'whatlinkshere-filters'    => 'سۇزگىلەر',

# Block/unblock
'blockip'                     => 'قاتىسۋشىنى بۇعاتتاۋ',
'blockip-legend'              => 'قاتىسۋشىنى بۇعاتتاۋ',
'blockiptext'                 => 'تومەندەگى ٴپىشىن قاتىسۋشىنىڭ جازۋ رۇقساتىن بەلگىلى IP مەكەنجايىمەن نە اتىمەن بۇعاتتاۋ ٴۇشىن قولدانىلادى.
بۇنى تەك بۇزاقىلىقتى قاقپايلاۋ ٴۇشىن جانە دە [[{{{{ns:mediawiki}}:Policy-url}}|ەرەجەلەر]] بويىنشا اتقارۋىڭىز ٴجون.
تومەندە ٴتىيىستى سەبەبىن تولتىرىپ كورسەتىڭىز (مىسالى, دايەككە بۇزاقىلىقپەن وزگەرتكەن بەتتەردى كەلتىرىپ).',
'ipaddress'                   => 'IP مەكەنجايى:',
'ipadressorusername'          => 'IP مەكەنجايى نە قاتىسۋشى اتى:',
'ipbexpiry'                   => 'مەرزىمى بىتپەك:',
'ipbreason'                   => 'سەبەبى:',
'ipbreasonotherlist'          => 'باسقا سەبەپ',
'ipbreason-dropdown'          => '* بۇعاتتاۋدىڭ جالپى سەبەبتەرى 
** جالعان مالىمەت ەنگىزۋ 
** بەتتەردەگى ماعلۇماتتى الاستاۋ 
** شەتتىك توراپتار سىلتەمەلەرىن جاۋدىرۋ 
** بەتتەرگە ماعىناسىزدىق/بالدىرلاۋ كىرىستىرۋ 
** قوقانداۋ/قۋعىنداۋ مىنەزقۇلىق 
** بىرنەشە رەت تىركەلىپ قىيياناتتاۋ 
** ورەسكەل قاتىسۋشى اتى',
'ipbanononly'                 => 'تەك تىركەلگىسىز قاتىسۋشىلاردى بۇعاتتاۋ',
'ipbcreateaccount'            => 'تىركەلۋدى قاقپايلاۋ',
'ipbemailban'                 => 'قاتىسۋشى ە-پوشتامەن حات جونەلتۋىن قاقپايلاۋ',
'ipbenableautoblock'          => 'بۇل قاتىسۋشى سوڭعى قولدانعان IP مەكەنجايىن, جانە كەيىن وڭدەۋگە بايقاپ كورگەن ٴار IP مەكەنجايلارىن وزبۇعاتتاۋى',
'ipbsubmit'                   => 'قاتىسۋشىنى بۇعاتتا',
'ipbother'                    => 'باسقا مەرزىمى:',
'ipboptions'                  => '2 ساعات:2 hours,1 كۇن:1 day,3 كۇن:3 days,1 اپتا:1 week,2 اپتا:2 weeks,1 اي:1 month,3 اي:3 months,6 اي:6 months,1 جىل:1 year,مانگى:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'باسقا',
'ipbotherreason'              => 'باسقا/قوسىمشا سەبەپ:',
'ipbhidename'                 => 'قاتىسۋشى اتىن بۇعاتتاۋ جۋرنالىننان, بەلسەندى بۇعاتتاۋ تىزىمىنەن, قاتىسۋشى تىزىمىنەن جاسىرۋ',
'ipbwatchuser'                => 'بۇل قاتىسۋشىنىڭ جەكە جانە تالقىلاۋ بەتتەرىن باقىلاۋ',
'badipaddress'                => 'جارامسىز IP مەكەنجايى',
'blockipsuccesssub'           => 'بۇعاتتاۋ ٴساتتى ٴوتتى',
'blockipsuccesstext'          => '[[{{#special:Contributions}}/$1|$1]] دەگەن بۇعاتتالعان.<br />
بۇعاتتاردى شولىپ شىعۋ ٴۇشىن [[{{#special:Ipblocklist}}|IP بۇعاتتاۋ ٴتىزىمىن]] قاراڭىز.',
'ipb-edit-dropdown'           => 'بۇعاتتاۋ سەبەپتەرىن وڭدەۋ',
'ipb-unblock-addr'            => '$1 دەگەندى بۇعاتتاماۋ',
'ipb-unblock'                 => 'قاتىسۋشى اتىن نەمەسە IP مەكەنجايىن بۇعاتتاماۋ',
'ipb-blocklist-addr'          => '$1 ٴۇشىن بار بۇعاتتاۋلاردى قاراۋ',
'ipb-blocklist'               => 'بار بۇعاتتاۋلاردى قاراۋ',
'unblockip'                   => 'قاتىسۋشىنى بۇعاتتاماۋ',
'unblockiptext'               => 'تومەندەگى ٴپىشىندى الدىنداعى IP مەكەنجايىمەن نە اتىمەن بۇعاتتالعان قاتىسۋشىعا جازۋ قاتىناۋىن قالپىنا كەلتىرىۋى ٴۇشىن قولدانىڭىز.',
'ipusubmit'                   => 'وسى مەكەنجايدى بۇعاتتاماۋ',
'unblocked'                   => '[[{{ns:user}}:$1|$1]] بۇعاتتاۋى ٴوشىرىلدى',
'unblocked-id'                => '$1 بۇعاتتاۋ الاستالدى',
'ipblocklist'                 => 'بۇعاتتالعان قاتىسۋشى / IP مەكەنجاي ٴتىزىمى',
'ipblocklist-legend'          => 'بۇعاتتالعان قاتىسۋشىنى تابۋ',
'ipblocklist-username'        => 'قاتىسۋشى اتى / IP مەكەنجايى:',
'ipblocklist-submit'          => 'ىزدە',
'blocklistline'               => '$1, $2 $3 دەگەندى بۇعاتتادى ($4)',
'infiniteblock'               => 'مانگى',
'expiringblock'               => 'مەرزىمى بىتپەك: $1',
'anononlyblock'               => 'تەك تىركەلگىسىزدەردى',
'noautoblockblock'            => 'وزبۇعاتتاۋ وشىرىلگەن',
'createaccountblock'          => 'تىركەلۋ بۇعاتتالعان',
'emailblock'                  => 'ە-پوشتا بۇعاتتالعان',
'ipblocklist-empty'           => 'بۇعاتتاۋ ٴتىزىمى بوس.',
'ipblocklist-no-results'      => 'سۇراتىلعان IP مەكەنجاي نە قاتىسۋشى اتى بۇعاتتالعان ەمەس.',
'blocklink'                   => 'بۇعاتتاۋ',
'unblocklink'                 => 'بۇعاتتاماۋ',
'contribslink'                => 'ۇلەسى',
'autoblocker'                 => 'IP مەكەنجايىڭىزدى جۋىقتا «[[{{ns:user}}:1|$1]]» پايدالانعان, سوندىقتان وزبۇعاتتالعان.
$1 بۇعاتتاۋى ٴۇشىن كەلتىرىلگەن سەبەبى: «$2».',
'blocklogpage'                => 'بۇعاتتاۋ_جۋرنالى',
'blocklogentry'               => '[[$1]] دەگەندى $2 مەرزىمگە بۇعاتتادى $3',
'blocklogtext'                => 'بۇل قاتىسۋشىلاردى بۇعاتتاۋ/بۇعاتتاماۋ ارەكەتتەرىنىڭ جۋرنالى.
وزدىكتىك بۇعاتتالعان IP مەكەنجايلار وسىندا تىزىمدەلگەمەگەن.
اعىمداعى بەلسەندى تىيىمدار مەن بۇعاتتاۋلاردى [[{{#special:Ipblocklist}}|IP بۇعاتتاۋ تىزىمىنەن]] قاراڭىز.',
'unblocklogentry'             => '«$1» — بۇعاتتاۋىن ٴوشىردى',
'block-log-flags-anononly'    => 'تەك تىركەلگىسىزدەر',
'block-log-flags-nocreate'    => 'تىركەلۋ وشىرىلگەن',
'block-log-flags-noautoblock' => 'وزبۇعاتتاۋ وشىرىلگەن',
'block-log-flags-noemail'     => 'ە-پوشتا بۇعاتتالعان',
'range_block_disabled'        => 'اۋقىم بۇعاتتاۋلارىن جاساۋ اكىمشىلىك مۇمكىندىگى وشىرىلگەن.',
'ipb_expiry_invalid'          => 'بىتەتىن ۋاقىتى جارامسىز.',
'ipb_expiry_temp'             => 'جاسىرىلعان قاتىسۋشى اتىن بۇعاتتاۋى ماڭگى بولۋى ٴجون.',
'ipb_already_blocked'         => '«$1» الداقاشان بۇعاتتالعان',
'ipb_cant_unblock'            => 'قاتەلىك: IP $1 بۇعاتتاۋى تابىلمادى. ونىڭ بۇعاتتاۋى الداقاشان وشىرلگەن مۇمكىن.',
'ipb_blocked_as_range'        => 'قاتەلىك: IP $1 تىكەلەي بۇعاتتالماعان جانە بۇعاتتاۋى وشىرىلمەيدى.
بىراق, بۇل بۇعاتتاۋى ٴوشىرىلۋى مۇمكىن $2 اۋقىمى بولىگى بوپ بۇعاتتالعان.',
'ip_range_invalid'            => 'IP مەكەنجاي اۋقىمى جارامسىز.',
'blockme'                     => 'وزدىكتىك_بۇعاتتاۋ',
'proxyblocker'                => 'پروكسىي سەرۆەرلەردى بۇعاتتاۋىش',
'proxyblocker-disabled'       => 'بۇل جەتە وشىرىلگەن.',
'proxyblockreason'            => 'IP مەكەنجايىڭىز اشىق پروكسىي سەرۆەرگە جاتاتىندىقتان بۇعاتتالعان.
ىينتەرنەت قىزمەتىن جابدىقتاۋشىڭىزبەن, نە تەحنىيكالىق قولداۋ قىزمەتىمەن قاتىناسىڭىز, جانە ولارعا وسى وتە كۇردەلى قاۋىپسىزدىك شاتاق تۋرالى اقپارات بەرىڭىز.',
'proxyblocksuccess'           => 'ٴبىتتى.',
'sorbsreason'                 => 'IP مەكەنجايىڭىز {{SITENAME}} تورابىندا قولدانىلعان DNSBL قارا تىزىمىندەگى اشىق پروكسىي-سەرۆەر دەپ تابىلادى.',
'sorbs_create_account_reason' => 'IP مەكەنجايىڭىز {{SITENAME}} تورابىندا قولدانىلعان DNSBL قارا تىزىمىندەگى اشىق پروكسىي-سەرۆەر دەپ تابىلادى.
جاڭا تىركەلگى جاساي المايسىز.',

# Developer tools
'lockdb'              => 'دەرەكقوردى قۇلىپتاۋ',
'unlockdb'            => 'دەرەكقوردى قۇلىپتاماۋ',
'lockdbtext'          => 'دەرەكقوردىن قۇلىپتالۋى بارلىق قاتىسۋشىلاردىڭ بەت وڭدەۋ, باپتاۋىن قالاۋ, باقىلاۋ ٴتىزىمىن, تاعى باسقا دەرەكقوردى وزگەرتەتىن مۇمكىندىكتەرىن توقتاتا تۇرادى.
وسى ماقساتىڭىزدى, جانە باپتاۋ بىتكەندە دەرەكقوردى اشاتىڭىزدى قۇپتاڭىز.',
'unlockdbtext'        => 'دەرەكقودىن اشىلۋى بارلىق قاتىسۋشىلاردىڭ بەت وڭدەۋ, باپتاۋىن قالاۋ, باقىلاۋ ٴتىزىمىن, تاعى باسقا دەرەكقوردى وزگەرتەتىن مۇمكىندىكتەرىن قالپىنا كەلتىرەدى.
وسى ماقساتىڭىزدى قۇپتاڭىز.',
'lockconfirm'         => 'ٴىيا, دەرەكقور قۇلىپتاۋىن ناقتى تىلەيمىن.',
'unlockconfirm'       => 'ٴىيا, دەرەكقور قۇلىپتاماۋىن ناقتى تىلەيمىن.',
'lockbtn'             => 'دەرەكقوردى قۇلىپتا',
'unlockbtn'           => 'دەرەكقوردى قۇلىپتاما',
'locknoconfirm'       => 'قۇپتاۋ كوزىنە قۇسبەلگى سالماعانسىز.',
'lockdbsuccesssub'    => 'دەرەكقور قۇلىپتاۋى ٴساتتى ٴوتتى',
'unlockdbsuccesssub'  => 'دەرەكقور قۇلىپتاۋى الاستالدى',
'lockdbsuccesstext'   => 'دەرەكقور قۇلىپتالدى.<br />
باپتاۋ تولىق وتكىزىلگەننەن كەيىن [[{{#special:Unlockdb}}|قۇلىپتاۋىن الاستاۋعا]] ۇمىتپاڭىز.',
'unlockdbsuccesstext' => 'قۇلىپتالعان دەرەكقور ٴساتتى اشىلدى.',
'lockfilenotwritable' => 'دەرەكقور قۇلىپتاۋ فايلى جازىلمايدى.
دەرەكقوردى قۇلىپتاۋ نە اشۋ ٴۇشىن, ۆەب-سەرۆەر فايلعا جازۋ رۇقساتى بولۋ كەرەك.',
'databasenotlocked'   => 'دەرەكقور قۇلىپتالعان جوق.',

# Move page
'move-page'               => '$1 دەگەندى جىلجىتۋ',
'move-page-legend'        => 'بەتتى جىلجىتۋ',
'movepagetext'            => "تومەندەگى ٴپىشىندى قولدانىپ بەتتەردى قايتا اتايدى, بارلىق تارىيحىن جاڭا اتاۋعا جىلجىتادى.
بۇرىنعى بەت تاقىرىبىن اتى جاڭا تاقىرىپ اتىنا ايدايتىن بەت بولادى.
ەسكى تاقىرىپ اتىنا سىلتەيتىن سىلتەمەلەر وزگەرتىلمەيدى;
جىلجىتۋدان سوڭ شىنجىرلى نە جارامسىز ايداعىشتار بار-جوعىن تەكسەرىپ شىعىڭىز.
سىلتەمەلەر بۇرىنعى جولداۋىمەن بىلايعى ٴوتۋىن تەكسەرۋىنە ٴوزىڭىز مىندەتتى بولاسىز.

اڭعارتپا: ەگەر وسى ارادا الداقاشان جاڭا تاقىرىپ اتى بار بەت بولسا, بۇل بوس نە ايداعىش بولعانشا دەيىن, جانە سوڭىندا تۇزەتۋ تارىيحى جوق بولسا, بەت '''جىلجىتىلمايدى'''. وسىنىڭ ماعىناسى: ەگەر بەتتى قاتەلىكپەن قايتا اتاساڭىز, بۇرىنعى اتاۋىنا قايتا اتاۋعا بولادى, جانە بار بەتتىڭ ۇستىنە جازۋىڭىزعا بولمايدى.

'''قۇلاقتاندىرۋ!'''
بۇل كوپ قارالاتىن بەتكە قاتاڭ جانە كەنەت وزگەرىس جاساۋعا مۇمكىن;
وسىنىڭ سالدارىن بايىمداۋىڭىزدى ارەكەتتىڭ الدىنان باتىل بولىڭىز.",
'movepagetalktext'        => "كەلەسى سەبەپتەر '''بولعانشا''' دەيىن, تالقىلاۋ بەتى بۇنىمەن بىرگە وزدىكتىك جىلجىتىلادى:
* بوس ەمەس تالقىلاۋ بەتى جاڭا اتاۋدا الداقاشان بولعاندا, نە
* تومەندەگى كوزگە قۇسبەلگى الىپ تاستالعاندا.

وسى ورايدا, قالاۋىڭىز بولسا, بەتتى قولدان جىلجىتا نە قوسا الاسىز.",
'movearticle'             => 'جىلجىتپاق بەت:',
'movenotallowed'          => '{{SITENAME}} جوباسىندا بەتتەردى جىلجىتۋ رۋقساتىڭىز جوق.',
'newtitle'                => 'جاڭا تاقىرىپ اتىنا:',
'move-watch'              => 'بۇل بەتتى باقىلاۋ',
'movepagebtn'             => 'بەتتى جىلجىت',
'pagemovedsub'            => 'جىلجىتۋ ٴساتتى اياقتالدى',
'articleexists'           => 'وسىلاي اتالعان بەت الداقاشان بار, نە تاڭداعان اتاۋىڭىز جارامدى ەمەس.
وزگە اتاۋدى تاڭداڭىز',
'cantmove-titleprotected' => 'بەتتى وسى ورىنعا جىلجىتا المايسىز, سەبەبى جاڭا تاقىرىپ اتى باستاۋدان قورعالعان',
'talkexists'              => "'''بەتتىڭ ٴوزى ٴساتتى جىلجىتىلدى, بىراق تالقىلاۋ بەتى بىرگە جىلجىتىلمادى, ونىڭ سەبەبى جاڭا تاقىرىپ اتىندا بىرەۋى الداقاشان بار.
بۇنى قولمەن قوسىڭىز.'''",
'movedto'                 => 'مىناعان جىلجىتىلدى:',
'movetalk'                => 'قاۋىمداستى تالقىلاۋ بەتىن جىلجىتۋ',
'move-subpages'           => 'بارلىق بەتشەلەرىن جىلجىتۋ, ەگەر قولدانبالى بولسا',
'move-talk-subpages'      => 'تالقىلاۋ بەتىنىڭ بارلىق بەتشەلەرىن جىلجىتۋ, ەگەر قولدانبالى بولسا',
'movepage-page-exists'    => '$1 دەگەن بەت الداقاشان بار جانە ۇستىنە وزدىكتىك جازىلمايدى.',
'movepage-page-moved'     => '$1 دەگەن بەت $2 دەگەنگە جىلجىتىلدى.',
'movepage-page-unmoved'   => '$1 دەگەن بەت $2 دەگەنگە جىلجىتىلمايدى.',
'movepage-max-pages'      => 'بارىنشا $1 بەت جىلجىتىلدى دا مىننان كوبى وزدىكتىك جىلجىلتىلمايدى.',
'1movedto2'               => '[[$1]] دەگەندى [[$2]] دەگەنگە جىلجىتتى',
'1movedto2_redir'         => '[[$1]] دەگەندى [[$2]] دەگەن ايداعىش ۇستىنە جىلجىتتى',
'movelogpage'             => 'جىلجىتۋ جۋرنالى',
'movelogpagetext'         => 'تومەندە جىلجىتىلعان بەتتەردىڭ ٴتىزىمى بەرىلىپ تۇر.',
'movereason'              => 'سەبەبى:',
'revertmove'              => 'قايتارۋ',
'delete_and_move'         => 'جويۋ جانە جىلجىتۋ',
'delete_and_move_text'    => '==جويۋ كەرەك==
«[[:$1]]» دەگەن نىسانا بەت الداقاشان بار.
جىلجىتۋعا جول بەرۋ ٴۇشىن بۇنى جوياسىز با?',
'delete_and_move_confirm' => 'ٴىيا, بۇل بەتتى جوي',
'delete_and_move_reason'  => 'جىلجىتۋعا جول بەرۋ ٴۇشىن جويىلعان',
'selfmove'                => 'قاينار جانە نىسانا تاقىرىپ اتتارى بىردەي;
بەت ٴوزىنىڭ ۇستىنە جىلجىتىلمايدى.',
'immobile_namespace'      => 'قاينار نە نىسانا تاقىرىپ اتى ارناۋلى تۇرىنە جاتادى;
بەتتەر بۇل ەسىم اياسى سىرتىنا جانە ىشىنە جىلجىتىلمايدى.',
'imagenocrossnamespace'   => 'فايل ەمەس ەسىم اياسىنا فايل جىلجىتىلمايدى',
'imagetypemismatch'       => 'فايلدىڭ جاڭا كەڭەيتىمى بۇنىڭ تۇرىنە سايكەس ەمەس',

# Export
'export'            => 'بەتتەردى سىرتقا بەرۋ',
'exporttext'        => 'XML پىشىمىنە قاپتالعان بولەك بەت نە بەتتەر بۋماسى ٴماتىنىڭ جانە وڭدەۋ تارىيحىن سىرتقا بەرە الاسىز. 
MediaWiki جۇيەسىنىڭ [[{{#special:Import}}|سىرتتان الۋ بەتىن]] پايدالانىپ, بۇنى وزگە ۋىيكىيگە الۋعا بولادى.

بەتتەردى سىرتقا بەرۋ ٴۇشىن, تاقىرىپ اتتارىن تومەندەگى ٴماتىن جولاعىنا ەنگىزىڭىز (جول سايىن ٴبىر تاقىرىپ اتى), جانە دە بولەكتەڭىز: نە اعىمدىق نۇسقاسىن, بارلىق ەسكى نۇسقالارى مەن جانە تارىيحى جولدارى مەن بىرگە, نەمەسە ٴدال اعىمدىق نۇسقاسىن, سوڭعى وڭدەمەۋ تۋرالى اقپاراتى مەن بىرگە.

سوڭعى جاعدايدا سىلتەمەنى دە, مىسالى «{{{{ns:mediawiki}}:Mainpage}}» بەتى ٴۇشىن [[{{#special:Export}}/{{MediaWiki:Mainpage}}]] قولدانۋعا بولادى.',
'exportcuronly'     => 'تولىق تارىيحىن ەمەس, تەك اعىمدىق تۇزەتۋىن كىرىستىرىڭىز',
'exportnohistory'   => "----
'''اڭعارتپا:''' ونىمدىلىك اسەرى سەبەپتەرىنەن, بەتتەردىڭ تولىق تارىيحىن بۇل پىشىنمەن سىرتقا بەرۋى وشىرىلگەن.",
'export-submit'     => 'سىرتقا بەر',
'export-addcattext' => 'مىنا ساناتتاعى بەتتەردى ۇستەۋ:',
'export-addcat'     => 'ۇستە',
'export-download'   => 'فايل تۇرىندە ساقتاۋ',
'export-templates'  => 'ۇلگىلەردى قوسا الىپ',

# Namespace 8 related
'allmessages'               => 'جۇيە حابارلارى',
'allmessagesname'           => 'اتاۋى',
'allmessagesdefault'        => 'ادەپكى ٴماتىنى',
'allmessagescurrent'        => 'اعىمدىق ٴماتىنى',
'allmessagestext'           => 'مىندا {{ns:mediawiki}} ەسىم اياسىندا جەتىمدى جۇيە حابار ٴتىزىمى بەرىلەدى.
ەگەر امبەباپ MediaWiki جەرسىندىرۋگە ۇلەس قوسقىڭىز كەلسە [http://www.mediawiki.org/wiki/Localisation MediaWiki جەرسىندىرۋ بەتىنە] جانە [http://translatewiki.net Betawiki جوباسىنا] بارىپ شىعىڭىز.',
'allmessagesnotsupportedDB' => "'''\$wgUseDatabaseMessages''' وشىرىلگەن سەبەبىنەن '''{{ns:special}}:AllMessages''' بەتى قولدانىلمايدى.",
'allmessagesfilter'         => 'حاباردى اتاۋىمەن سۇزگىلەۋ:',
'allmessagesmodified'       => 'تەك وزگەرتىلگەندى كورسەت',

# Thumbnails
'thumbnail-more'           => 'ۇلكەيتۋ',
'filemissing'              => 'جوعالعان فايل',
'thumbnail_error'          => 'نوباي قۇرۋ قاتەسى: $1',
'djvu_page_error'          => 'DjVu بەتى اۋماق سىرتىنددا',
'djvu_no_xml'              => 'DjVu فايلى ٴۇشىن XML كەلتىرۋى ىيكەمدى ەمەس',
'thumbnail_invalid_params' => 'نوبايدىڭ باپتالىمدارى جارامسىز',
'thumbnail_dest_directory' => 'نىسانا قالتاسى قۇرۋى ىيكەمدى ەمەس',

# Special:Import
'import'                     => 'بەتتەردى سىرتتان الۋ',
'importinterwiki'            => 'ۋىيكىي-اپارۋ ٴۇشىن سىرتتان الۋ',
'import-interwiki-text'      => 'سىرتتان الىناتىن ۋىيكىيدى جانە بەتتىڭ تاقىرىپ اتىن بولەكتەڭىز.
تۇزەتۋ كۇن-ايى جانە وڭدەۋشى ەسىمدەرى ساقتالادى.
ۋىيكىي-اپارۋ ٴۇشىن سىرتتان الۋ بارلىق ارەكەتتەر [[{{#special:Log}}/import|سىرتتان الۋ جۋرنالىنا]] جازىلىپ الىنادى.',
'import-interwiki-history'   => 'بۇل بەتتىڭ بارلىق تارىيحىي نۇسقالارىن كوشىرۋ',
'import-interwiki-submit'    => 'سىرتتان الۋ',
'import-interwiki-namespace' => 'بەتتەردى مىنا ەسىم اياسىنا اپارۋ:',
'importtext'                 => 'قاينار ۋىيكىيدەن «{{#special:Export}}» قۋرالىن قولدانىپ فايلدى سىرتقا بەرىڭىز, دىيسكىڭىزگە ساقتاڭىز دا مىندا قوتارىپ بەرىڭىز.',
'importstart'                => 'بەتتەردى سىرتتان الۋدا…',
'import-revision-count'      => '$1 تۇزەتۋ',
'importnopages'              => 'سىرتتان الىناتىن بەتتەر جوق.',
'importfailed'               => 'سىرتتان الۋ ٴساتسىز ٴبىتتى: $1',
'importunknownsource'        => 'Cىرتتان الىناتىن قاينار ٴتۇرى بەلگىسىز',
'importcantopen'             => 'سىرتتان الىناتىن فايل اشىلمايدى',
'importbadinterwiki'         => 'جارامسىز ۋىيكىي-ارالىق سىلتەمە',
'importnotext'               => 'بۇل بوس, نەمەسە ٴماتىنى جوق',
'importsuccess'              => 'سىرتتان الۋ اياقتالدى!',
'importhistoryconflict'      => 'تارىيحىندا قاقتىعىستى تۇزەتۋ بار (بۇل بەت الدىندا سىرتتان الىنعان سىيياقتى)',
'importnosources'            => 'ۋىيكىي-اپارۋ ٴۇشىن سىرتتان الىناتىن ەش قاينار كوزى انىقتالماعان, جانە تارىيحىن تىكەلەي قوتارىپ بەرۋى وشىرىلگەن.',
'importnofile'               => 'سىرتتان الىنعان فايل قوتارىپ بەرىلگەن جوق.',
'importuploaderrorsize'      => 'سىرتتان الىنعان فايلدىڭ قوتارىپ بەرىلۋى ٴساتسىز ٴوتتى. فايل مولشەرى قوتارىپ بەرىلۋگە رۋقسات ەتىلگەننەن اسادى.',
'importuploaderrorpartial'   => 'سىرتتان الىنعان فايلدىڭ قوتارىپ بەرىلۋى ٴساتسىز ٴوتتى. وسى فايلدىڭ تەك بولىكتەرى قوتارىلىپ بەرىلدى.',
'importuploaderrortemp'      => 'سىرتتان الىنعان فايلدىڭ قوتارىپ بەرىلۋى ٴساتسىز ٴوتتى. ۋاقىتشا قالتا تابىلمادى.',
'import-parse-failure'       => 'سىرتتان الىنعان XML فايل قۇرىلىمىن تالداتقاندا ساتسىزدىك بولدى',
'import-noarticle'           => 'سىرتتان الىناتىن ەش بەت جوق!',
'import-nonewrevisions'      => 'بارلىق تۇزەتۋلەرى الدىندا سىرتتان الىنعان.',
'xml-error-string'           => '$1 ٴنومىر $2 جولدا, باعان $3 (بايت $4): $5',
'import-upload'              => 'XML دەرەكتەرىن قوتارىپ بەرۋ',

# Import log
'importlogpage'                    => 'سىرتتان الۋ جۋرنالى',
'importlogpagetext'                => 'بەتتەردى تۇزەتۋ تارىيحىمەن بىرگە سىرتقى ۋىيكىيلەردەن اكىمشى رەتىندە الۋ.',
'import-logentry-upload'           => '«[[$1]]» دەگەندى فايل قوتارىپ بەرۋ ارقىلى سىرتتان الدى',
'import-logentry-upload-detail'    => '$1 تۇزەتۋ',
'import-logentry-interwiki'        => 'ۋىيكىي-اپارىلعان $1',
'import-logentry-interwiki-detail' => '$2 دەگەننەن $1 تۇزەتۋ',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'جەكە بەتىم',
'tooltip-pt-anonuserpage'         => 'بۇل IP مەكەنجايدىڭ جەكە بەتى',
'tooltip-pt-mytalk'               => 'تالقىلاۋ بەتىم',
'tooltip-pt-anontalk'             => 'بۇل IP مەكەنجاي وڭدەمەلەرىن تالقىلاۋ',
'tooltip-pt-preferences'          => 'باپتالىمدارىم',
'tooltip-pt-watchlist'            => 'وزگەرىستەرىن باقىلاپ تۇرعان بەتتەر ٴتىزىمىم.',
'tooltip-pt-mycontris'            => 'ۇلەستەرىمدىڭ ٴتىزىمى',
'tooltip-pt-login'                => 'كىرۋىڭىزدى ۇسىنامىز, ول مىندەتتى ەمەس.',
'tooltip-pt-anonlogin'            => 'كىرۋىڭىزدى ۇسىنامىز, بىراق, ول مىندەتتى ەمەس.',
'tooltip-pt-logout'               => 'شىعۋ',
'tooltip-ca-talk'                 => 'ماعلۇمات بەتتى تالقىلاۋ',
'tooltip-ca-edit'                 => 'بۇل بەتتى وڭدەي الاسىز. ساقتاۋدىڭ الدىندا «قاراپ شىعۋ» باتىرماسىن نۇقىڭىز.',
'tooltip-ca-addsection'           => 'بۇل تالقىلاۋ بەتىندە جاڭا تاراۋ باستاۋ.',
'tooltip-ca-viewsource'           => 'بۇل بەت قورعالعان. قاينار كوزىن قاراي الاسىز.',
'tooltip-ca-history'              => 'بۇل بەتتىن جۋىقتاعى نۇسقالارى.',
'tooltip-ca-protect'              => 'بۇل بەتتى قورعاۋ',
'tooltip-ca-delete'               => 'بۇل بەتتى جويۋ',
'tooltip-ca-undelete'             => 'بۇل بەتتىڭ جويۋدىڭ الدىنداعى بولعان وڭدەمەلەرىن قالپىنا كەلتىرۋ',
'tooltip-ca-move'                 => 'بۇل بەتتى جىلجىتۋ',
'tooltip-ca-watch'                => 'بۇل بەتتى باقىلاۋ تىزىمىڭىزگە ۇستەۋ',
'tooltip-ca-unwatch'              => 'بۇل بەتتى باقىلاۋ تىزىمىڭىزدەن الاستاۋ',
'tooltip-search'                  => '{{SITENAME}} جوباسىندا ىزدەۋ',
'tooltip-search-go'               => 'ەگەر ٴدال وسى اتاۋىمەن بولسا بەتكە ٴوتىپ كەتۋ',
'tooltip-search-fulltext'         => 'وسى ٴماتىنى بار بەتتى ىزدەۋ',
'tooltip-p-logo'                  => 'باستى بەتكە',
'tooltip-n-mainpage'              => 'باستى بەتكە كەلىپ-كەتىڭىز',
'tooltip-n-portal'                => 'جوبا تۋرالى, نە ىستەۋىڭىزگە بولاتىن, قايدان تابۋعا بولاتىن تۋرالى',
'tooltip-n-currentevents'         => 'اعىمداعى وقىيعالارعا قاتىستى ٴوڭ اقپاراتىن تابۋ',
'tooltip-n-recentchanges'         => 'وسى ۋىيكىيدەگى جۋىقتاعى وزگەرىستەر ٴتىزىمى.',
'tooltip-n-randompage'            => 'كەزدەيسوق بەتتى جۇكتەۋ',
'tooltip-n-help'                  => 'انىقتاما تابۋ ورنى.',
'tooltip-t-whatlinkshere'         => 'مىندا سىلتەگەن بارلىق بەتتەردىڭ ٴتىزىمى',
'tooltip-t-recentchangeslinked'   => 'مىننان سىلتەنگەن بەتتەردىڭ جۋىقتاعى وزگەرىستەرى',
'tooltip-feed-rss'                => 'بۇل بەتتىڭ RSS ارناسى',
'tooltip-feed-atom'               => 'بۇل بەتتىڭ Atom ارناسى',
'tooltip-t-contributions'         => 'وسى قاتىسۋشىنىڭ ۇلەس ٴتىزىمىن قاراۋ',
'tooltip-t-emailuser'             => 'وسى قاتىسۋشىعا حات جونەلتۋ',
'tooltip-t-upload'                => 'فايلداردى قوتارىپ بەرۋ',
'tooltip-t-specialpages'          => 'بارلىق ارنايى بەتتەر ٴتىزىمى',
'tooltip-t-print'                 => 'بۇل بەتتىڭ باسىپ شىعارىشقا ارنالعان نۇسقاسى',
'tooltip-t-permalink'             => 'مىنا بەتتىڭ وسى نۇسقاسىنىڭ تۇراقتى سىلتەمەسى',
'tooltip-ca-nstab-main'           => 'ماعلۇمات بەتىن قاراۋ',
'tooltip-ca-nstab-user'           => 'قاتىسۋشى بەتىن قاراۋ',
'tooltip-ca-nstab-media'          => 'تاسپا بەتىن قاراۋ',
'tooltip-ca-nstab-special'        => 'بۇل ارنايى بەت, بەتتىڭ ٴوزى وڭدەلىنبەيدى.',
'tooltip-ca-nstab-project'        => 'جوبا بەتىن قاراۋ',
'tooltip-ca-nstab-image'          => 'فايل بەتىن قاراۋ',
'tooltip-ca-nstab-mediawiki'      => 'جۇيە حابارىن قاراۋ',
'tooltip-ca-nstab-template'       => 'ۇلگىنى قاراۋ',
'tooltip-ca-nstab-help'           => 'انىقتىما بەتىن قاراۋ',
'tooltip-ca-nstab-category'       => 'سانات بەتىن قاراۋ',
'tooltip-minoredit'               => 'بۇنى شاعىن وڭدەمە دەپ بەلگىلەۋ',
'tooltip-save'                    => 'جاساعان وزگەرىستەرىڭىزدى ساقتاۋ',
'tooltip-preview'                 => 'ساقتاۋدىڭ الدىنان جاساعان وزگەرىستەرىڭىزدى قاراپ شىعىڭىز!',
'tooltip-diff'                    => 'ماتىنگە قانداي وزگەرىستەردى جاساعانىڭىزدى قاراۋ.',
'tooltip-compareselectedversions' => 'بەتتىڭ ەكى بولەكتەنگەن نۇسقاسى ايىرماسىن قاراۋ.',
'tooltip-watch'                   => 'بۇل بەتتى باقىلاۋ تىزىمىڭىزگە ۇستەۋ',
'tooltip-recreate'                => 'بەت جويىلعانىنا قاراماستان قايتا باستاۋ',
'tooltip-upload'                  => 'قوتارىپ بەرۋدى باستاۋ',

# Stylesheets
'common.css'      => '/* مىندا ورنالاستىرىلعان CSS بارلىق مانەرلەردە قولدانىلادى */',
'standard.css'    => '/* مىندا ورنالاستىرىلعان CSS تەك «داعدىلى» (standard) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى */',
'nostalgia.css'   => '/* مىندا ورنالاستىرىلعان CSS تەك «اڭساۋ» (nostalgia) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى */',
'cologneblue.css' => '/* مىندا ورنالاستىرىلعان CSS تەك «كولن زەڭگىرلىگى» (cologneblue) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى skin */',
'monobook.css'    => '/* مىندا ورنالاستىرىلعان CSS تەك «دارا كىتاپ» (monobook) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى */',
'myskin.css'      => '/* مىندا ورنالاستىرىلعان CSS تەك «ٴوز مانەرىم» (myskin) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى */',
'chick.css'       => '/* مىندا ورنالاستىرىلعان CSS تەك «بالاپان» (chick) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى */',
'simple.css'      => '/* مىندا ورنالاستىرىلعان CSS تەك «كادىمگى» (simple) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى */',
'modern.css'      => '/* مىندا ورنالاستىرىلعان CSS تەك «زاماناۋىي» (modern) مانەرىن پايدالانۋشىلارىنا ىقپال ەتەدى */',

# Scripts
'common.js'      => '/* مىنداعى ٴارتۇرلى JavaScript كەز كەلگەن بەت قوتارىلعاندا بارلىق پايدالانۋشىلار ٴۇشىن جەگىلەدى. */',
'standard.js'    => '/* مىنداعى JavaScript تەك «داعدىلى» (standard) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى */',
'nostalgia.js'   => '/* مىنداعى JavaScript تەك «اڭساۋ» (nostalgia) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى*/',
'cologneblue.js' => '/* مىنداعى JavaScript تەك «كولن زەڭگىرلىگى» (cologneblue) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى */',
'monobook.js'    => '/* مىنداعى JavaScript تەك «دارا كىتاپ» (monobook) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى */',
'myskin.js'      => '/* مىنداعى JavaScript تەك «ٴوز مانەرىم» (myskin) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى */',
'chick.js'       => '/* مىنداعى JavaScript تەك «بالاپان» (chick) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى */',
'simple.js'      => '/* مىنداعى JavaScript تەك «كادىمگى» (simple) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى */',
'modern.js'      => '/* مىنداعى JavaScript تەك «زاماناۋىي» (modern) مانەرىن پايدالانۋشىلار ٴۇشىن جەگىلەدى */',

# Metadata
'nodublincore'      => 'بۇل سەرۆەردە «Dublin Core RDF» ٴتۇرى قوسىمشا دەرەكتەرى وشىرىلگەن.',
'nocreativecommons' => 'بۇل سەرۆەردە «Creative Commons RDF» ٴتۇرى قوسىمشا دەرەكتەرى وشىرىلگەن.',
'notacceptable'     => 'تۇتىنعىشىڭىز وقىي الاتىن ٴپىشىمى بار دەرەكتەردى بۇل ۋىيكىي سەرۆەر جەتىستىرە المايدى.',

# Attribution
'anonymous'        => '{{SITENAME}} تىركەلگىسىز قاتىسۋشى(لارى)',
'siteuser'         => '{{SITENAME}} قاتىسۋشى $1',
'lastmodifiedatby' => 'بۇل بەتتى $3 قاتىسۋشى سوڭعى وزگەرتكەن كەزى: $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'شىعارما نەگىزىن $1 جازعان.',
'others'           => 'باسقالار',
'siteusers'        => '{{SITENAME}} قاتىسۋشى(لار) $1',
'creditspage'      => 'بەتتى جازعاندار',
'nocredits'        => 'بۇل بەتتى جازعاندار تۋرالى اقپارات جوق.',

# Spam protection
'spamprotectiontitle' => '«سپام»-نان قورعايتىن سۇزگى',
'spamprotectiontext'  => 'بۇل بەتتىڭ ساقتاۋىن «سپام» سۇزگىسى بۇعاتتادى.
بۇنىڭ سەبەبى شەتتىك توراپ سىلتەمەسىنەن بولۋى مۇمكىن.',
'spamprotectionmatch' => 'كەلەسى «سپام» ٴماتىنى سۇزگىلەنگەن: $1',
'spambot_username'    => 'MediaWiki spam cleanup',
'spam_reverting'      => '$1 دەگەنگە سىلتەمەلەرى جوق سوڭعى نۇسقاسىنا قايتارىلدى',
'spam_blanking'       => '$1 دەگەنگە سىلتەمەلەرى بار بارلىق تۇزەتۋلەر تازارتىلدى',

# Info page
'infosubtitle'   => 'بەت تۋرالى مالىمەت',
'numedits'       => 'وڭدەمە سانى (بەت): $1',
'numtalkedits'   => 'وڭدەمە سانى (تالقىلاۋ بەتى): $1',
'numwatchers'    => 'باقىلاۋشى سانى: $1',
'numauthors'     => 'ٴارتۇرلى اۋتور سانى (بەت): $1',
'numtalkauthors' => 'ٴارتۇرلى اۋتور سانى (تالقىلاۋ بەتى): $1',

# Math options
'mw_math_png'    => 'ارقاشان PNG پىشىنىمەن كورسەتكىز',
'mw_math_simple' => 'ەگەر وتە قاراپايىم بولسا — HTML, ايتپەسە PNG',
'mw_math_html'   => 'ەگەر ىقتىيمال بولسا — HTML, ايتپەسە PNG',
'mw_math_source' => 'بۇنى TeX پىشىمىندە قالدىر (ماتىندىك شولعىشتارعا)',
'mw_math_modern' => 'وسى زامانعى شولعىشتارىنا ۇسىنىلادى',
'mw_math_mathml' => 'ەگەر ىقتىيمال بولسا — MathML (سىناقتاما)',

# Patrolling
'markaspatrolleddiff'                 => 'زەرتتەلدى دەپ بەلگىلەۋ',
'markaspatrolledtext'                 => 'بۇل بەتتى زەرتتەلدى دەپ بەلگىلە',
'markedaspatrolled'                   => 'زەرتتەلدى دەپ بەلگىلەندى',
'markedaspatrolledtext'               => 'بولەكتەنگەن تۇزەتۋ زەرتتەلدى دەپ بەلگىلەندى.',
'rcpatroldisabled'                    => 'جۋىقتاعى وزگەرىستەردى زەرتتەۋى وشىرىلگەن',
'rcpatroldisabledtext'                => 'جۋىقتاعى وزگەرىستەردى زەرتتەۋ مۇمكىندىگى اعىمدا وشىرىلگەن.',
'markedaspatrollederror'              => 'زەرتتەلدى دەپ بەلگىلەنبەيدى',
'markedaspatrollederrortext'          => 'زەرتتەلدى دەپ بەلگىلەۋ ٴۇشىن تۇزەتۋدى كەلتىرىڭىز.',
'markedaspatrollederror-noautopatrol' => 'ٴوز جاساعان وزگەرىستەرىڭىزدى زەرتتەلدى دەپ بەلگىلەي المايسىز.',

# Patrol log
'patrol-log-page' => 'زەرتتەۋ جۋرنالى',
'patrol-log-line' => '$2 دەگەننىڭ $1 تۇزەتۋىن زەرتتەلدى دەپ بەلگىلەدى $3',
'patrol-log-auto' => '(وزدىكتىك)',
'patrol-log-diff' => 'ٴنومىر $1',

# Image deletion
'deletedrevision'                 => 'ەسكى تۇزەتۋىن جويدى: $1',
'filedeleteerror-short'           => 'فايل جويۋ قاتەسى: $1',
'filedeleteerror-long'            => 'فايلدى جويعاندا قاتەلەر كەزدەستى:

$1',
'filedelete-missing'              => '«$1» فايلى جويىلمايدى, سەبەبى ول جوق.',
'filedelete-old-unregistered'     => '«$1» فايلدىڭ كەلتىرىلگەن تۇزەتۋى دەرەكقوردا جوق.',
'filedelete-current-unregistered' => '«$1» فايلدىڭ كەلتىرىلگەن اتاۋى دەرەكقوردا جوق.',
'filedelete-archive-read-only'    => '«$1» مۇراعات قالتاسىنا ۆەب-سەرۆەر جازا المايدى.',

# Browsing diffs
'previousdiff' => '← الدىڭعى ايىرم.',
'nextdiff'     => 'كەلەسى ايىرم. →',

# Media information
'mediawarning'         => "'''قۇلاقتاندىرۋ''': بۇل فايل تۇرىندە قاسكۇنەمدى كودى بار بولۋى ىقتىيمال; بۇنى جەگىپ جۇيەڭىزگە زىييان كەلتىرۋىڭىز مۇمكىن.<hr />",
'imagemaxsize'         => 'سىيپاتتاماسى بەتىندەگى سۋرەتتىڭ مولشەرىن شەكتەۋى:',
'thumbsize'            => 'نوباي مولشەرى:',
'widthheight'          => '$1 × $2',
'widthheightpage'      => '$1 × $2, $3 بەت',
'file-info'            => 'فايل مولشەرى: $1, MIME ٴتۇرى: $2',
'file-info-size'       => '($1 × $2 نۇكتە, فايل مولشەرى: $3, MIME ٴتۇرى: $4)',
'file-nohires'         => '<small>جوعارى اجىراتىلىمدىعى جەتىمسىز.</small>',
'svg-long-desc'        => '(SVG فايلى, كەسىمدى $1 × $2 نۇكتە, فايل مولشەرى: $3)',
'show-big-image'       => 'جوعارى اجىراتىلىمدى',
'show-big-image-thumb' => '<small>قاراپ شىعۋ مولشەرى: $1 × $2 نۇكتە</small>',

# Special:NewImages
'newimages'             => 'جاڭا فايلدار كورمەسى',
'imagelisttext'         => "تومەندە $2 سۇرىپتالعان '''$1''' فايل ٴتىزىمى.",
'newimages-summary'     => 'بۇل ارنايى بەتىندە سوڭعى قوتارىپ بەرىلگەن فايلدار كورسەتىلەدى',
'showhidebots'          => '(بوتتاردى $1)',
'noimages'              => 'كورەتىن ەشتەڭە جوق.',
'ilsubmit'              => 'ىزدە',
'bydate'                => 'كۇن-ايىمەن',
'sp-newimages-showfrom' => '$2, $1 كەزىنەن بەرى — جاڭا سۋرەتتەردى كورسەت',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'video-dims'     => '$1, $2 × $3',
'seconds-abbrev' => 'س',
'minutes-abbrev' => 'مىين',
'hours-abbrev'   => 'ساع',

# Bad image list
'bad_image_list' => 'ٴپىشىمى تومەندەگىدەي:

تەك ٴتىزىم دانالارى (* نىشانىمەن باستالىتىن جولدار) ەسەپتەلەدى.
جولدىڭ ٴبىرىنشى سىلتەمەسى جارامسىز سۋرەتكە سىلتەۋ ٴجون.
سول جولداعى كەيىنگى ٴاربىر سىلتەمەلەر ەرەن بولىپ ەسەپتەلەدى, مىسالى جول ىشىندەگى كەزدەسەتىن سۋرەتى بار بەتتەر.',

# Metadata
'metadata'          => 'قوسىمشا مالىمەتتەر',
'metadata-help'     => 'وسى فايلدا قوسىمشا مالىمەتتەر بار. بالكىم, وسى مالىمەتتەر فايلدى جاساپ شىعارۋ, نە ساندىلاۋ ٴۇشىن پايدالانعان ساندىق كامەرا, نە ماتىنالعىردان الىنعان.
ەگەر وسى فايل نەگىزگى كۇيىنەن وزگەرتىلگەن بولسا, كەيبىر ەجەلەلەرى وزگەرتىلگەن فوتوسۋرەتكە لايىق بولماس.',
'metadata-expand'   => 'ەگجەي-تەگجەيىن كورسەت',
'metadata-collapse' => 'ەگجەي-تەگجەيىن جاسىر',
'metadata-fields'   => 'وسى حاباردا تىزىمدەلگەن EXIF قوسىمشا مالىمەتتەر اۋماقتارى, سۋرەت بەتى كورسەتۋ كەزىندە قوسىمشا مالىمەتتەر كەستە جاسىرىلىعاندا كىرىستىرلەدى.
باسقالارى ادەپكىدەن جاسىرىلادى.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'ەنى',
'exif-imagelength'                 => 'بىيىكتىگى',
'exif-bitspersample'               => 'قۇراش سايىن بىيت سانى',
'exif-compression'                 => 'قىسىم سۇلباسى',
'exif-photometricinterpretation'   => 'نۇكتە قىيىسۋى',
'exif-orientation'                 => 'مەگزەۋى',
'exif-samplesperpixel'             => 'قۇراش سانى',
'exif-planarconfiguration'         => 'دەرەك رەتتەۋى',
'exif-ycbcrsubsampling'            => 'Y قۇراشىنىڭ C قۇراشىنا جارناقتاۋى',
'exif-ycbcrpositioning'            => 'Y قۇراشى جانە C قۇراشى مەكەندەۋى',
'exif-xresolution'                 => 'دەرەلەي اجىراتىلىمدىعى',
'exif-yresolution'                 => 'تىرەلەي اجىراتىلىمدىعى',
'exif-resolutionunit'              => 'X جانە Y بويىنشا اجىراتىلىمدىق بىرلىگى',
'exif-stripoffsets'                => 'سۋرەت دەرەرەكتەرىنىڭ جايعاسۋى',
'exif-rowsperstrip'                => 'بەلدىك سايىن جول سانى',
'exif-stripbytecounts'             => 'قىسىمدالعان بەلدىك سايىن بايت سانى',
'exif-jpeginterchangeformat'       => 'JPEG SOI دەگەنگە ىعىسۋى',
'exif-jpeginterchangeformatlength' => 'JPEG دەرەكتەرىنىڭ بايت سانى',
'exif-transferfunction'            => 'تاسىمالداۋ جەتەسى',
'exif-whitepoint'                  => 'اق نۇكتە تۇستىلىگى',
'exif-primarychromaticities'       => 'العى شەپتەگى تۇستىلىكتەرى',
'exif-ycbcrcoefficients'           => 'ٴتۇس اياسىن تاسىمالداۋ ماترىيتسالىق ەسەلىكتەرى',
'exif-referenceblackwhite'         => 'قارا جانە اق انىقتاۋىش قوس كولەمدەرى',
'exif-datetime'                    => 'فايلدىڭ وزگەرتىلگەن كۇن-ايى',
'exif-imagedescription'            => 'سۋرەت تاقىرىبىن اتى',
'exif-make'                        => 'كامەرا ٴوندىرۋشىسى',
'exif-model'                       => 'كامەرا ۇلگىسى',
'exif-software'                    => 'قولدانىلعان باعدارلامالىق جاساقتاما',
'exif-artist'                      => 'تۋىندىگەرى',
'exif-copyright'                   => 'اۋتورلىق قۇقىقتار ىييەسى',
'exif-exifversion'                 => 'Exif نۇسقاسى',
'exif-flashpixversion'             => 'قولدانعان Flashpix نۇسقاسى',
'exif-colorspace'                  => 'ٴتۇس اياسى',
'exif-componentsconfiguration'     => 'ارقايسى قۇراش ٴمانى',
'exif-compressedbitsperpixel'      => 'سۋرەت قىسىمداۋ ٴتارتىبى',
'exif-pixelydimension'             => 'سۋرەتتىڭ جارامدى ەنى',
'exif-pixelxdimension'             => 'سۋرەتتىڭ جارامدى بىيىكتىگى',
'exif-makernote'                   => 'ٴوندىرۋشىنىڭ اڭعارتپالارى',
'exif-usercomment'                 => 'قاتىسۋشىنىڭ ماندەمەلەرى',
'exif-relatedsoundfile'            => 'قاتىستى دىبىس فايلى',
'exif-datetimeoriginal'            => 'جاسالعان كەزى',
'exif-datetimedigitized'           => 'ساندىقتاۋ كەزى',
'exif-subsectime'                  => 'جاسالعان كەزىنىڭ سەكۋند بولشەكتەرى',
'exif-subsectimeoriginal'          => 'تۇپنۇسقا كەزىنىڭ سەكۋند بولشەكتەرى',
'exif-subsectimedigitized'         => 'ساندىقتاۋ كەزىنىڭ سەكۋند بولشەكتەرى',
'exif-exposuretime'                => 'ۇستالىم ۋاقىتى',
'exif-exposuretime-format'         => '$1 س ($2)',
'exif-fnumber'                     => 'ساڭىلاۋ مولشەرى',
'exif-exposureprogram'             => 'ۇستالىم باعدارلاماسى',
'exif-spectralsensitivity'         => 'سپەكتر بويىنشا سەزگىشتىگى',
'exif-isospeedratings'             => 'ISO جىلدامدىق جارناقتاۋى (جارىق سەزگىشتىگى)',
'exif-oecf'                        => 'وپتويەلەكتروندى تۇرلەتۋ ىقپالى',
'exif-shutterspeedvalue'           => 'جاپقىش جىلدامدىلىعى',
'exif-aperturevalue'               => 'ساڭىلاۋلىق',
'exif-brightnessvalue'             => 'جارىقتىلىق',
'exif-exposurebiasvalue'           => 'ۇستالىم وتەمى',
'exif-maxaperturevalue'            => 'بارىنشا ساڭىلاۋ اشۋى',
'exif-subjectdistance'             => 'نىسانا قاشىقتىعى',
'exif-meteringmode'                => 'ولشەۋ ٴادىسى',
'exif-lightsource'                 => 'جارىق كوزى',
'exif-flash'                       => 'جارقىلداعىش',
'exif-focallength'                 => 'شوعىرلاۋ الشاقتىعى',
'exif-subjectarea'                 => 'نىسانا اۋقىمى',
'exif-flashenergy'                 => 'جارقىلداعىش قارقىنى',
'exif-spatialfrequencyresponse'    => 'كەڭىستىك-جىيىلىك اسەرشىلىگى',
'exif-focalplanexresolution'       => 'ح بويىنشا شوعىرلاۋ جايپاقتىقتىڭ اجىراتىلىمدىعى',
'exif-focalplaneyresolution'       => 'Y بويىنشا شوعىرلاۋ جايپاقتىقتىڭ اجىراتىلىمدىعى',
'exif-focalplaneresolutionunit'    => 'شوعىرلاۋ جايپاقتىقتىڭ اجىراتىلىمدىق ولشەمى',
'exif-subjectlocation'             => 'نىسانا ورنالاسۋى',
'exif-exposureindex'               => 'ۇستالىم ايقىنداۋى',
'exif-sensingmethod'               => 'سەنسوردىڭ ولشەۋ ٴادىسى',
'exif-filesource'                  => 'فايل قاينارى',
'exif-scenetype'                   => 'ساحنا ٴتۇرى',
'exif-cfapattern'                  => 'CFA سۇزگى كەيىپى',
'exif-customrendered'              => 'قوسىمشا سۋرەت وڭدەتۋى',
'exif-exposuremode'                => 'ۇستالىم ٴتارتىبى',
'exif-whitebalance'                => 'اق ٴتۇسىنىڭ تەندەستىگى',
'exif-digitalzoomratio'            => 'ساندىق اۋقىمداۋ جارناقتاۋى',
'exif-focallengthin35mmfilm'       => '35 mm تاسپاسىنىڭ شوعىرلاۋ الشاقتىعى',
'exif-scenecapturetype'            => 'تۇسىرگەن ساحنا ٴتۇرى',
'exif-gaincontrol'                 => 'ساحنانى رەتتەۋ',
'exif-contrast'                    => 'اشىقتىق',
'exif-saturation'                  => 'قانىقتىق',
'exif-sharpness'                   => 'ايقىندىق',
'exif-devicesettingdescription'    => 'جابدىق باپتاۋ سىيپاتتاماسى',
'exif-subjectdistancerange'        => 'ساحنا قاشىقتىعىنىڭ كولەمى',
'exif-imageuniqueid'               => 'سۋرەتتىڭ بىرەگەي ٴنومىرى (ID)',
'exif-gpsversionid'                => 'GPS بەلگىشەسىنىڭ نۇسقاسى',
'exif-gpslatituderef'              => 'سولتۇستىك نەمەسە وڭتۇستىك بويلىعى',
'exif-gpslatitude'                 => 'بويلىعى',
'exif-gpslongituderef'             => 'شىعىس نەمەسە باتىس ەندىگى',
'exif-gpslongitude'                => 'ەندىگى',
'exif-gpsaltituderef'              => 'بىيىكتىك كورسەتۋى',
'exif-gpsaltitude'                 => 'بىيىكتىك',
'exif-gpstimestamp'                => 'GPS ۋاقىتى (اتوم ساعاتى)',
'exif-gpssatellites'               => 'ولشەۋگە پيدالانىلعان جەر سەرىكتەرى',
'exif-gpsstatus'                   => 'قابىلداعىش كۇيى',
'exif-gpsmeasuremode'              => 'ولشەۋ ٴتارتىبى',
'exif-gpsdop'                      => 'ولشەۋ دالدىگى',
'exif-gpsspeedref'                 => 'جىلدامدىلىق ولشەمى',
'exif-gpsspeed'                    => 'GPS قابىلداعىشتىڭ جىلدامدىلىعى',
'exif-gpstrackref'                 => 'قوزعالىس باعىتىن كورسەتۋى',
'exif-gpstrack'                    => 'قوزعالىس باعىتى',
'exif-gpsimgdirectionref'          => 'سۋرەت باعىتىن كورسەتۋى',
'exif-gpsimgdirection'             => 'سۋرەت باعىتى',
'exif-gpsmapdatum'                 => 'پايدالانىلعان گەودەزىييالىق تۇسىرمە دەرەكتەرى',
'exif-gpsdestlatituderef'          => 'نىسانا بويلىعىن كورسەتۋى',
'exif-gpsdestlatitude'             => 'نىسانا بويلىعى',
'exif-gpsdestlongituderef'         => 'نىسانا ەندىگىن كورسەتۋى',
'exif-gpsdestlongitude'            => 'نىسانا ەندىگى',
'exif-gpsdestbearingref'           => 'نىسانا ازىيمۋتىن كورسەتۋى',
'exif-gpsdestbearing'              => 'نىسانا ازىيمۋتى',
'exif-gpsdestdistanceref'          => 'نىسانا قاشىقتىعىن كورسەتۋى',
'exif-gpsdestdistance'             => 'نىسانا قاشىقتىعى',
'exif-gpsprocessingmethod'         => 'GPS وڭدەتۋ ٴادىسىنىڭ اتاۋى',
'exif-gpsareainformation'          => 'GPS اۋماعىنىڭ اتاۋى',
'exif-gpsdatestamp'                => 'GPS كۇن-ايى',
'exif-gpsdifferential'             => 'GPS سارالانعان دۇرىستاۋ',

# EXIF attributes
'exif-compression-1' => 'ۇلعايتىلعان',

'exif-unknowndate' => 'بەلگىسىز كۇن-ايى',

'exif-orientation-1' => 'قالىپتى', # 0th row: top; 0th column: left
'exif-orientation-2' => 'دەرەلەي شاعىلىسقان', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° بۇرىشقا اينالعان', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'تىرەلەي شاعىلىسقان', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'ساعات تىلشەسىنە قارسى 90° بۇرىشقا اينالعان جانە تىرەلەي شاعىلىسقان', # 0th row: left; 0th column: top
'exif-orientation-6' => 'ساعات تىلشە بويىنشا 90° بۇرىشقا اينالعان', # 0th row: right; 0th column: top
'exif-orientation-7' => 'ساعات تىلشە بويىنشا 90° بۇرىشقا اينالعان جانە تىرەلەي شاعىلىسقان', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'ساعات تىلشەسىنە قارسى 90° بۇرىشقا اينالعان', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'تالپاق ٴپىشىم',
'exif-planarconfiguration-2' => 'تايپاق ٴپىشىم',

'exif-componentsconfiguration-0' => 'بار بولمادى',

'exif-exposureprogram-0' => 'انىقتالماعان',
'exif-exposureprogram-1' => 'قولمەن',
'exif-exposureprogram-2' => 'باعدارلامالى ٴادىس (قالىپتى)',
'exif-exposureprogram-3' => 'ساڭىلاۋ باسىڭقىلىعى',
'exif-exposureprogram-4' => 'ىسىرما باسىڭقىلىعى',
'exif-exposureprogram-5' => 'ونەر باعدارلاماسى (انىقتىق تەرەندىگىنە ساناسقان)',
'exif-exposureprogram-6' => 'قىيمىل باعدارلاماسى (جاپقىش شاپشاندىلىعىنا ساناسقان)',
'exif-exposureprogram-7' => 'تىرەلەي ٴادىسى (ارتى شوعىرلاۋسىز تاياۋ تۇسىرمەلەر)',
'exif-exposureprogram-8' => 'دەرەلەي ٴادىسى (ارتى شوعىرلانعان دەرەلەي تۇسىرمەلەر)',

'exif-subjectdistance-value' => '$1 m',

'exif-meteringmode-0'   => 'بەلگىسىز',
'exif-meteringmode-1'   => 'بىركەلكى',
'exif-meteringmode-2'   => 'بۇلدىر داق',
'exif-meteringmode-3'   => 'بىرداقتى',
'exif-meteringmode-4'   => 'كوپداقتى',
'exif-meteringmode-5'   => 'ورنەكتى',
'exif-meteringmode-6'   => 'جىرتىندى',
'exif-meteringmode-255' => 'باسقا',

'exif-lightsource-0'   => 'بەلگىسىز',
'exif-lightsource-1'   => 'كۇن جارىعى',
'exif-lightsource-2'   => 'كۇنجارىقتى شام',
'exif-lightsource-3'   => 'قىزدىرعىشتى شام',
'exif-lightsource-4'   => 'جارقىلداعىش',
'exif-lightsource-9'   => 'اشىق كۇن',
'exif-lightsource-10'  => 'بۇلىنعىر كۇن',
'exif-lightsource-11'  => 'كولەنكەلى',
'exif-lightsource-12'  => 'كۇنجارىقتى شام (D 5700–7100 K)',
'exif-lightsource-13'  => 'كۇنجارىقتى شام (N 4600–5400 K)',
'exif-lightsource-14'  => 'كۇنجارىقتى شام (W 3900–4500 K)',
'exif-lightsource-15'  => 'كۇنجارىقتى شام (WW 3200–3700 K)',
'exif-lightsource-17'  => 'قالىپتى جارىق قاينارى A',
'exif-lightsource-18'  => 'قالىپتى جارىق قاينارى B',
'exif-lightsource-19'  => 'قالىپتى جارىق قاينارى C',
'exif-lightsource-24'  => 'ستۋدىييالىق ISO كۇنجارىقتى شام',
'exif-lightsource-255' => 'باسقا جارىق كوزى',

'exif-focalplaneresolutionunit-2' => 'ٴدۇيم',

'exif-sensingmethod-1' => 'انىقتالماعان',
'exif-sensingmethod-2' => '1-ٴتشىيپتى اۋماقتى تۇسسەزگىش',
'exif-sensingmethod-3' => '2-ٴتشىيپتى اۋماقتى تۇسسەزگىش',
'exif-sensingmethod-4' => '3-ٴتشىيپتى اۋماقتى تۇسسەزگىش',
'exif-sensingmethod-5' => 'كەزەكتى اۋماقتى تۇسسەزگىش',
'exif-sensingmethod-7' => '3-سىزىقتى تۇسسەزگىش',
'exif-sensingmethod-8' => 'كەزەكتى سىزىقتى تۇسسەزگىش',

'exif-scenetype-1' => 'تىكەلەي تۇسىرىلگەن فوتوسۋرەت',

'exif-customrendered-0' => 'قالىپتى وڭدەتۋ',
'exif-customrendered-1' => 'قوسىمشا وڭدەتۋ',

'exif-exposuremode-0' => 'وزدىكتىك ۇستالىمداۋ',
'exif-exposuremode-1' => 'قولمەن ۇستالىمداۋ',
'exif-exposuremode-2' => 'وزدىكتىك جارقىلداۋ',

'exif-whitebalance-0' => 'اق ٴتۇسى وزدىكتىك تەندەستىرىلگەن',
'exif-whitebalance-1' => 'اق ٴتۇسى قولمەن تەندەستىرىلگەن',

'exif-scenecapturetype-0' => 'قالىپتالعان',
'exif-scenecapturetype-1' => 'دەرەلەي',
'exif-scenecapturetype-2' => 'تىرەلەي',
'exif-scenecapturetype-3' => 'تۇنگى ساحنا',

'exif-gaincontrol-0' => 'جوق',
'exif-gaincontrol-1' => 'تومەن زورايۋ',
'exif-gaincontrol-2' => 'جوعارى زورايۋ',
'exif-gaincontrol-3' => 'تومەن باياۋلاۋ',
'exif-gaincontrol-4' => 'جوعارى باياۋلاۋ',

'exif-contrast-0' => 'قالىپتى',
'exif-contrast-1' => 'ۇيان',
'exif-contrast-2' => 'تۇرپايى',

'exif-saturation-0' => 'قالىپتى',
'exif-saturation-1' => 'تومەن قانىقتى',
'exif-saturation-2' => 'جوعارى قانىقتى',

'exif-sharpness-0' => 'قالىپتى',
'exif-sharpness-1' => 'ۇيان',
'exif-sharpness-2' => 'تۇرپايى',

'exif-subjectdistancerange-0' => 'بەلگىسىز',
'exif-subjectdistancerange-1' => 'تاياۋ تۇسىرىلگەن',
'exif-subjectdistancerange-2' => 'جاقىن تۇسىرىلگەن',
'exif-subjectdistancerange-3' => 'الىس تۇسىرىلگەن',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'سولتۇستىك بويلىعى',
'exif-gpslatitude-s' => 'وڭتۇستىك بويلىعى',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'شىعىس ەندىگى',
'exif-gpslongitude-w' => 'باتىس ەندىگى',

'exif-gpsstatus-a' => 'ولشەۋ ۇلاسۋدا',
'exif-gpsstatus-v' => 'ولشەۋ ٴوزارا ارەكەتتە',

'exif-gpsmeasuremode-2' => '2-باعىتتىق ولشەم',
'exif-gpsmeasuremode-3' => '3-باعىتتىق ولشەم',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'km/h',
'exif-gpsspeed-m' => 'mil/h',
'exif-gpsspeed-n' => 'knot',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'شىن باعىت',
'exif-gpsdirection-m' => 'ماگنىيتتى باعىت',

# External editor support
'edit-externally'      => 'بۇل فايلدى شەتتىك قوندىرما ارقىلى وڭدەۋ',
'edit-externally-help' => 'كوبىرەك اقپارات ٴۇشىن [http://www.mediawiki.org/wiki/Manual:External_editors ورناتۋ نۇسقامالارىن] قاراڭىز.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'بارلىعىن',
'imagelistall'     => 'بارلىعى',
'watchlistall2'    => 'بارلىق',
'namespacesall'    => 'بارلىعى',
'monthsall'        => 'بارلىعى',

# E-mail address confirmation
'confirmemail'             => 'ە-پوشتا مەكەنجايىن قۇپتاۋ',
'confirmemail_noemail'     => '[[{{#special:Preferences}}|پايدالانۋشىلىق باپتالىمدارىڭىزدا]] جارامدى ە-پوشتا مەكەنجايىن قويماپسىز.',
'confirmemail_text'        => '{{SITENAME}} ە-پوشتا مۇمكىندىكتەرىن پايدالانۋ ٴۇشىن الدىنان ە-پوشتا مەكەنجايىڭىزدىڭ جارامدىلىعىن تەكسەرىپ شىعۋىڭىز كەرەك.
ٴوزىڭىزدىڭ مەكەنجايىڭىزعا قۇپتاۋ حاتىن جونەلتۋ ٴۇشىن تومەندەگى باتىرمانى نۇقىڭىز.
حاتتىڭ ىشىندە كودى بار سىلتەمە كىرىستىرمەك; 
ە-پوشتا جايىڭىزدىڭ جارامدىلىعىن قۇپتاۋ ٴۇشىن سىلتەمەنى شولعىشتىڭ مەكەنجاي جولاعىنا ەنگىزىپ اشىڭىز.',
'confirmemail_pending'     => '<div class="error">قۇپتاۋ كودى الداقاشان حاتپەن جىبەرىلىگەن;
ەگەر جۋىقتا تىركەلسەڭىز, جاڭا كودىن سۇراتۋ الدىنان حات كەلۋىن ٴبىرشاما ٴمىينوت كۇتە تۇرىڭىز.</div>',
'confirmemail_send'        => 'قۇپتاۋ كودىن جونەلتۋ',
'confirmemail_sent'        => 'قۇپتاۋ حاتى جونەلتىلدى.',
'confirmemail_oncreate'    => 'قۇپتاۋ كودى ە-پوشتا مەكەنجايىڭىزعا جونەلتىلدى.
بۇل بەلگىلەمە كىرۋ ۇدىرىسىنە كەرەگى جوق, بىراق ە-پوشتا نەگىزىندەگى ۋىيكىي مۇمكىندىكتەردى قوسۋ ٴۇشىن بۇنى جەتىستىرۋىڭىز كەرەك.',
'confirmemail_sendfailed'  => 'قۇپتاۋ حاتى جونەلتىلمەدى.
جارامسىز تاڭبالار ٴۇشىن مەكەنجايدى تەكسەرىپ شىعىڭىز.

پوشتا جىبەرگىشتىڭ قايتارعان مالىمەتى: $1',
'confirmemail_invalid'     => 'قۇپتاۋ كودى جارامسىز.
كود مەرزىمى بىتكەن شىعار.',
'confirmemail_needlogin'   => 'ە-پوشتا مەكەنجايىڭىزدى قۇپتاۋ ٴۇشىن $1 كەرەك.',
'confirmemail_success'     => 'ە-پوشتا مەكەنجايىڭىز قۇپتالدى.
ەندى ۋىيكىيگە كىرىپ جۇمىسقا كىرىسۋگە بولادى',
'confirmemail_loggedin'    => 'ە-پوشتا مەكەنجايىڭىز ەندى قۇپتالدى.',
'confirmemail_error'       => 'قۇپتاۋڭىزدى ساقتاعاندا بەلگىسىز قاتە بولدى.',
'confirmemail_subject'     => '{{SITENAME}} تورابىنان ە-پوشتا مەكەنجايىڭىزدى قۇپتاۋ حاتى',
'confirmemail_body'        => 'كەيبىرەۋ, $1 دەگەن IP مەكەنجايىنان, ٴوزىڭىز بولۋى مۇمكىن,
{{SITENAME}} جوباسىندا بۇل ە-پوشتا مەكەنجايىن قولدانىپ «$2» دەگەن تىركەلگى جاساپتى.

بۇل تىركەلگى ناقتى سىزگە ٴتان ەكەنىن قۇپتاۋ ٴۇشىن, جانە {{SITENAME}} جوباسىنىڭ
ە-پوشتا مۇمكىندىكتەرىن بەلسەندىرۋ ٴۇشىن, مىنا سىلتەمەنى شولعىشىڭىزبەن اشىڭىز:

$3

ەگەر بۇل تىركەلگىنى جاساعان ٴوزىڭىز *ەمەس* بولسا, مىنا سىلتەمەگە ەرىپ
ە-پوشتا مەكەنجايى قۇپتاۋىن بولدىرماڭىز:

$5

قۇپتاۋ كودى مەرزىمى بىتەتىن كەزى: $4.',
'confirmemail_invalidated' => 'ە-پوشتا مەكەنجايىن قۇپتاۋى بولدىرىلمادى',
'invalidateemail'          => 'ە-پوشتا مەكەنجايىن قۇپتاۋى بولدىرماۋ',

# Scary transclusion
'scarytranscludedisabled' => '[ۋىيكىي-ارالىق كىرىكبەتتەر وشىرىلگەن]',
'scarytranscludefailed'   => '[$1 ٴۇشىن ۇلگى كەلتىرۋى ٴساتسىز ٴبىتتى; عافۋ ەتىڭىز]',
'scarytranscludetoolong'  => '[URL تىم ۇزىن; عافۋ ەتىڭىز]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">بۇل بەتتىڭ اڭىستاۋلارى:

$1
</div>',
'trackbackremove'   => '([$1 جويۋ])',
'trackbacklink'     => 'اڭىستاۋ',
'trackbackdeleteok' => 'اڭىستاۋ ٴساتتى جويىلدى.',

# Delete conflict
'deletedwhileediting' => 'قۇلاقتاندىرۋ: بۇل بەتتى وڭدەۋىڭىزدى باستاعاندا, وسى بەت جويىلدى!',
'confirmrecreate'     => "بۇل بەتتى وڭدەۋىڭىزدى باستاعاندا [[{{ns:user}}:$1|$1]] ([[{{ns:user_talk}}:$1|تالقىلاۋى]]) وسى بەتتى جويدى, كەلتىرگەن سەبەبى:
: ''$2''
وسى بەتتى قايتا باستاۋىن ناقتى تىلەگەنىڭىزدى قۇپتاڭىز.",
'recreate'            => 'قايتا باستاۋ',

'unit-pixel' => ' نۇكتە',

# HTML dump
'redirectingto' => '[[:$1]] بەتىنە ايداتۋدا…',

# action=purge
'confirm_purge'        => 'بۇل بەتتىن بۇركەمەسىن تازارتاسىز با?

$1',
'confirm_purge_button' => 'جارايدى',

# AJAX search
'searchcontaining' => "''$1'' ماعلۇماتى بار بەتتەردەن ىزدەۋ.",
'searchnamed'      => "''$1'' اتاۋى بار بەتتەردەن ىزدەۋ.",
'articletitles'    => "''$1'' دەپ باستالعان بەتتەردى",
'hideresults'      => 'ناتىيجەلەردى جاسىر',
'useajaxsearch'    => 'AJAX قولدانىپ ىزدەۋ',

# Separators for various lists, etc.
'semicolon-separator' => '؛',
'comma-separator'     => '،&#32;',

# Multipage image navigation
'imgmultipageprev' => '← الدىڭعى بەتكە',
'imgmultipagenext' => 'كەلەسى بەتكە →',
'imgmultigo'       => 'ٴوت!',
'imgmultigoto'     => '$1 بەتىنە ٴوتۋ',

# Table pager
'ascending_abbrev'         => 'ٴوسۋ',
'descending_abbrev'        => 'كەمۋ',
'table_pager_next'         => 'كەلەسى بەتكە',
'table_pager_prev'         => 'الدىڭعى بەتكە',
'table_pager_first'        => 'العاشقى بەتكە',
'table_pager_last'         => 'سوڭعى بەتكە',
'table_pager_limit'        => 'بەت سايىن $1 دانا كورسەت',
'table_pager_limit_submit' => 'ٴوتۋ',
'table_pager_empty'        => 'ەش ناتىيجە جوق',

# Auto-summaries
'autosumm-blank'   => 'بەتتىڭ بارلىق ماعلۇماتىن الاستادى',
'autosumm-replace' => "بەتتى '$1' دەگەنمەن الماستىردى",
'autoredircomment' => '[[$1]] دەگەنگە ايدادى',
'autosumm-new'     => 'جاڭا بەتتە: $1',

# Size units
'size-bytes' => '$1 بايت',

# Live preview
'livepreview-loading' => 'جۇكتەۋدە…',
'livepreview-ready'   => 'جۇكتەۋدە… دايىن!',
'livepreview-failed'  => 'تۋرا قاراپ شىعۋ ٴساتسىز! كادىمگى قاراپ شىعۋ ٴادىسىن بايقاپ كورىڭىز.',
'livepreview-error'   => 'قوسىلۋ ٴساتسىز: $1 «$2». كادىمگى قاراپ شىعۋ ٴادىسىن بايقاپ كورىڭىز.',

# Friendlier slave lag warnings
'lag-warn-normal' => '$1 سەكۋندتان جاڭالاۋ وزگەرىستەر بۇل تىزىمدە كورسەتىلمەۋى مۇمكىن.',
'lag-warn-high'   => 'دەرەكقور سەرۆەرى زور كەشىگۋى سەبەبىنەن, $1 سەكۋندتان جاڭالاۋ وزگەرىستەر بۇل تىزىمدە كورسەتىلمەۋى مۇمكىن.',

# Watchlist editor
'watchlistedit-numitems'       => 'باقىلاۋ تىزىمىڭىزدە, تالقىلاۋ بەتتەرسىز, $1 تاقىرىپ اتى بار.',
'watchlistedit-noitems'        => 'باقىلاۋ تىزىمىڭىزدە ەش تاقىرىپ اتى جوق.',
'watchlistedit-normal-title'   => 'باقىلاۋ ٴتىزىمدى وڭدەۋ',
'watchlistedit-normal-legend'  => 'باقىلاۋ تىزىمىنەن تاقىرىپ اتتارىن الاستاۋ',
'watchlistedit-normal-explain' => 'باقىلاۋ تىزىمىڭىزدەگى تاقىرىپ اتتار تومەندە كورسەتىلەدى.
تاقىرىپ اتىن الاستاۋ ٴۇشىن, ٴبۇيىر كوزگە قۇسبەلگى سالىڭىز, جانە «تاقىرىپ اتتارىن الاستا» دەگەندى نۇقىڭىز.
تاعى دا [[{{#special:Watchlist}}/raw|قام ٴتىزىمدى وڭدەي]] الاسىز.',
'watchlistedit-normal-submit'  => 'تاقىرىپ اتتارىن الاستا',
'watchlistedit-normal-done'    => 'باقىلاۋ تىزىمىڭىزدەن $1 تاقىرىپ اتى الاستالدى:',
'watchlistedit-raw-title'      => 'قام باقىلاۋ ٴتىزىمدى وڭدەۋ',
'watchlistedit-raw-legend'     => 'قام باقىلاۋ ٴتىزىمدى وڭدەۋ',
'watchlistedit-raw-explain'    => 'باقىلاۋ تىزىمىڭىزدەگى تاقىرىپ اتتارى تومەندە كورسەتىلەدى, جانە دە تىزمگە ۇستەپ جانە تىزمدەن الاستاپ وڭدەلۋى مۇمكىن;
جول سايىن ٴبىر تاقىرىپ اتى بولۋ ٴجون.
بىتىرگەننەن سوڭ «باقىلاۋ ٴتىزىمدى جاڭارتۋ» دەگەندى نۇقىڭىز.
تاعى دا [[{{#special:Watchlist}}/edit|قالىپالعان وڭدەۋىشتى پايدالانا]] الاسىز.',
'watchlistedit-raw-titles'     => 'تاقىرىپ اتتارى:',
'watchlistedit-raw-submit'     => 'باقىلاۋ ٴتىزىمدى جاڭارتۋ',
'watchlistedit-raw-done'       => 'باقىلاۋ ٴتىزىمىڭىز جاڭارتىلدى.',
'watchlistedit-raw-added'      => '$1 تاقىرىپ اتى ۇستەلدى:',
'watchlistedit-raw-removed'    => '$1 تاقىرىپ اتى الاستالدى:',

# Watchlist editing tools
'watchlisttools-view' => 'قاتىستى وزگەرىستەردى قاراۋ',
'watchlisttools-edit' => 'باقىلاۋ ٴتىزىمدى قاراۋ جانە وڭدەۋ',
'watchlisttools-raw'  => 'قام باقىلاۋ ٴتىزىمدى وڭدەۋ',

# Iranian month names
'iranian-calendar-m1'  => 'پىرۋاردىين',
'iranian-calendar-m2'  => 'اردىيبەشت',
'iranian-calendar-m3'  => 'حىرداد',
'iranian-calendar-m4'  => 'تىير',
'iranian-calendar-m5'  => 'مىرداد',
'iranian-calendar-m6'  => 'شەرىييار',
'iranian-calendar-m7'  => 'مەر',
'iranian-calendar-m8'  => 'ابان',
'iranian-calendar-m9'  => 'ازار',
'iranian-calendar-m10' => 'دىي',
'iranian-calendar-m11' => 'بەمىن',
'iranian-calendar-m12' => 'اسپاند',

# Hebrew month names
'hebrew-calendar-m1'      => 'ٴتىشرىي',
'hebrew-calendar-m2'      => 'xىشۋان',
'hebrew-calendar-m3'      => 'كىسلۋ',
'hebrew-calendar-m4'      => 'توت',
'hebrew-calendar-m5'      => 'شىبات',
'hebrew-calendar-m6'      => 'ادار',
'hebrew-calendar-m6a'     => 'ادار',
'hebrew-calendar-m6b'     => 'ۋادار',
'hebrew-calendar-m7'      => 'نىيسان',
'hebrew-calendar-m8'      => 'ايار',
'hebrew-calendar-m9'      => 'سىيۋان',
'hebrew-calendar-m10'     => 'تىموز',
'hebrew-calendar-m11'     => 'اب',
'hebrew-calendar-m12'     => 'ايلول',
'hebrew-calendar-m1-gen'  => 'ٴتىشرىيدىڭ',
'hebrew-calendar-m2-gen'  => 'حىشۋاندىڭ',
'hebrew-calendar-m3-gen'  => 'كىسلۋدىڭ',
'hebrew-calendar-m4-gen'  => 'توتتىڭ',
'hebrew-calendar-m5-gen'  => 'شىباتتىڭ',
'hebrew-calendar-m6-gen'  => 'اداردىڭ',
'hebrew-calendar-m6a-gen' => 'اداردىڭ',
'hebrew-calendar-m6b-gen' => 'ۋاداردىڭ',
'hebrew-calendar-m7-gen'  => 'نىيساننىڭ',
'hebrew-calendar-m8-gen'  => 'اياردىڭ',
'hebrew-calendar-m9-gen'  => 'سىيۋاننىڭ',
'hebrew-calendar-m10-gen' => 'تىموزدىڭ',
'hebrew-calendar-m11-gen' => 'ابتىڭ',
'hebrew-calendar-m12-gen' => 'ايلولدىڭ',

# Core parser functions
'unknown_extension_tag' => 'تانىلماعان كەڭەيتپە بەلگىسى «$1»',

# Special:Version
'version'                          => 'جۇيە نۇسقاسى', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'ورناتىلعان كەڭەيتىمدەر',
'version-specialpages'             => 'ارنايى بەتتەر',
'version-parserhooks'              => 'قۇرىلىمدىق تالداتقىشتىڭ تۇزاقتارى',
'version-variables'                => 'اينىمالىلار',
'version-other'                    => 'تاعى باسقالار',
'version-mediahandlers'            => 'تاسپا وڭدەتكىشتەرى',
'version-hooks'                    => 'جەتە تۇزاقتارى',
'version-extension-functions'      => 'كەڭەيتىمدەر جەتەلەرى',
'version-parser-extensiontags'     => 'قۇرىلىمدىق تالداتقىش كەڭەيتىمدەرىنىڭ بەلگىلەمەرى',
'version-parser-function-hooks'    => 'قۇرىلىمدىق تالداتقىش جەتەلەرىنىڭ تۇزاقتارى',
'version-skin-extension-functions' => 'مانەر كەڭەيتىمدەرىنىڭ جەتەلەرى',
'version-hook-name'                => 'تۇزاق اتاۋى',
'version-hook-subscribedby'        => 'تۇزاق تارتقىشتارى',
'version-version'                  => 'نۇسقاسى:',
'version-license'                  => 'لىيتسەنزىيياسى',
'version-software'                 => 'ورناتىلعان باعدارلامالىق جاساقتاما',
'version-software-product'         => 'ٴونىم',
'version-software-version'         => 'نۇسقاسى',

# Special:FilePath
'filepath'         => 'فايل ورنالاسۋى',
'filepath-page'    => 'فايل اتى:',
'filepath-submit'  => 'ورنالاسۋىن تاپ',
'filepath-summary' => 'بۇل ارنايى بەت فايل ورنالاسۋى تولىق جولىن قايتارادى.
سۋرەتتەر تولىق اجىراتىلىمدىعىمەن كورسەتىلەدى, باسقا فايل تۇرلەرىنە قاتىستى باعدارلاماسى تۋرا جەگىلەدى.

فايل اتاۋىن «{{ns:image}}:» دەگەن باستاۋىشسىز ەڭگىزىڭىز.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'فايل تەلنۇسقالارىن ىزدەۋ',
'fileduplicatesearch-summary'  => 'فايل حەشى ماعىناسى نەگىزىندە تەلنۇسقالارىن ىزدەۋ.

فايل اتاۋىن «{{ns:image}}:» دەگەن باستاۋىشسىز ەنگىزىڭىز.',
'fileduplicatesearch-legend'   => 'تەلنۇسقانى ىزدەۋ',
'fileduplicatesearch-filename' => 'فايل اتاۋى:',
'fileduplicatesearch-submit'   => 'ىزدە',
'fileduplicatesearch-info'     => '$1 × $2 پىيكسەل<br />فايل مولشەرى: $3<br />MIME ٴتۇرى: $4',
'fileduplicatesearch-result-1' => '«$1» فايلىنا تەڭ تەلنۇسقاسى جوق.',
'fileduplicatesearch-result-n' => '«$1» فايلىنا تەڭ $2 تەلنۇسقاسى بار.',

# Special:SpecialPages
'specialpages'                   => 'ارنايى بەتتەر',
'specialpages-note'              => '----
* كادىمگى ارنايى بەتتەر.
* <span class="mw-specialpagerestricted">شەكتەلگەن ارنايى بەتتەر.</span>',
'specialpages-group-maintenance' => 'باپتاۋ باياناتتارى',
'specialpages-group-other'       => 'تاعى باسقا ارنايى بەتتەر',
'specialpages-group-login'       => 'كىرۋ / تىركەلۋ',
'specialpages-group-changes'     => 'جۋىقتاعى وزگەرىستەر مەن جۋرنالدار',
'specialpages-group-media'       => 'تاسپا باياناتتارى جانە قوتارىپ بەرىلگەندەر',
'specialpages-group-users'       => 'قاتىسۋشىلار جانە ولاردىڭ قۇقىقتارى',
'specialpages-group-highuse'     => 'وتە كوپ قولدانىلعان بەتتەر',
'specialpages-group-pages'       => 'بەتتەر ٴتىزىمى',
'specialpages-group-pagetools'   => 'كومەكشى بەتتەر',
'specialpages-group-wiki'        => 'ۋىيكىي دەرەكتەرى جانە قۇرالدارى',
'specialpages-group-redirects'   => 'ايدايتىن ارنايى بەتتەر',
'specialpages-group-spam'        => 'سپام قۇرالدارى',

);
