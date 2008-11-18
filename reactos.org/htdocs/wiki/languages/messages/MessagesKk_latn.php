<?php
/** Kazakh (Latin) (Қазақша (Latin))
 *
 * @ingroup Language
 * @file
 *
 * @author AlefZet
 * @author Atabek
 * @author GaiJin
 */

/**
 * Bul qazaqşa tildeswiniñ jersindirw faýlı
 *
 * ŞETKİ PAÝDALANWŞILAR: BUL FAÝLDI TİKELEÝ ÖÑDEMEÑİZ!
 *
 * Bul faýldağı özgerister bağdarlamalıq jasaqtama kezekti jañartılğanda joğaltıladı.
 * Wïkïde öz baptalımdarıñızdı isteý alasız.
 * Äkimşi bop kirgeniñizde, [[Arnaýı:Barlıq xabarlar]] degen betke ötiñiz de
 * mında tizimdelingen MedïaWïkï:* sïpatı bar betterdi öñdeñiz.
 */

$fallback = 'kk-cyrl';

$separatorTransformTable = array(
	',' => "\xc2\xa0",
	'.' => ',',
);

$extraUserToggles = array(
	'nolangconversion'
);

$fallback8bitEncoding = 'windows-1254';

$namespaceNames = array(
	NS_MEDIA            => 'Taspa',
	NS_SPECIAL          => 'Arnaýı',
	NS_MAIN             => '',
	NS_TALK             => 'Talqılaw',
	NS_USER             => 'Qatıswşı',
	NS_USER_TALK        => 'Qatıswşı_talqılawı',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_talqılawı',
	NS_IMAGE            => 'Swret',
	NS_IMAGE_TALK       => 'Swret_talqılawı',
	NS_MEDIAWIKI        => 'MedïaWïkï',
	NS_MEDIAWIKI_TALK   => 'MedïaWïkï_talqılawı',
	NS_TEMPLATE         => 'Ülgi',
	NS_TEMPLATE_TALK    => 'Ülgi_talqılawı',
	NS_HELP             => 'Anıqtama',
	NS_HELP_TALK        => 'Anıqtama_talqılawı',
	NS_CATEGORY         => 'Sanat',
	NS_CATEGORY_TALK    => 'Sanat_talqılawı'
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
	'standard'    => 'Dağdılı (standard)',
	'nostalgia'   => 'Añsaw (nostalgia)',
	'cologneblue' => 'Köln zeñgirligi (cologneblue)',
	'monobook'    => 'Dara kitap (monobook)',
	'myskin'      => 'Öz mänerim (myskin)',
	'chick'       => 'Balapan (chick)',
	'simple'      => 'Kädimgi (simple)',
	'modern'      => 'Zamanawï (modern)',
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
	'redirect'               => array( 0,    '#REDIRECT', '#AÝDAW' ),
	'notoc'                  => array( 0,    '__MAZMUNSIZ__', '__MSIZ__', '__NOTOC__' ),
	'nogallery'              => array( 0,    '__QOÝMASIZ__', '__QSIZ__', '__NOGALLERY__' ),
	'forcetoc'               => array( 0,    '__MAZMUNDATQIZW__', '__MQIZW__', '__FORCETOC__' ),
	'toc'                    => array( 0,    '__MAZMUNI__', '__MZMN__', '__TOC__' ),
	'noeditsection'          => array( 0,    '__BÖLİDİMÖNDEMEW__', '__BÖLİMÖNDETKİZBEW__', '__NOEDITSECTION__' ),
	'currentmonth'           => array( 1,    'AĞIMDAĞIAÝ', 'CURRENTMONTH' ),
	'currentmonthname'       => array( 1,    'AĞIMDAĞIAÝATAWI', 'CURRENTMONTHNAME' ),
	'currentmonthnamegen'    => array( 1,    'AĞIMDAĞIAÝİLİKATAWI', 'CURRENTMONTHNAMEGEN' ),
	'currentmonthabbrev'     => array( 1,    'AĞIMDAĞIAÝJÏIR', 'AĞIMDAĞIAÝQISQA', 'CURRENTMONTHABBREV' ),
	'currentday'             => array( 1,    'AĞIMDAĞIKÜN', 'CURRENTDAY' ),
	'currentday2'            => array( 1,    'AĞIMDAĞIKÜN2', 'CURRENTDAY2' ),
	'currentdayname'         => array( 1,    'AĞIMDAĞIKÜNATAWI', 'CURRENTDAYNAME' ),
	'currentyear'            => array( 1,    'AĞIMDAĞIJIL', 'CURRENTYEAR' ),
	'currenttime'            => array( 1,    'AĞIMDAĞIWAQIT', 'CURRENTTIME' ),
	'currenthour'            => array( 1,    'AĞIMDAĞISAĞAT', 'CURRENTHOUR' ),
	'localmonth'             => array( 1,    'JERGİLİKTİAÝ', 'LOCALMONTH' ),
	'localmonthname'         => array( 1,    'JERGİLİKTİAÝATAWI', 'LOCALMONTHNAME' ),
	'localmonthnamegen'      => array( 1,    'JERGİLİKTİAÝİLİKATAWI', 'LOCALMONTHNAMEGEN' ),
	'localmonthabbrev'       => array( 1,    'JERGİLİKTİAÝJÏIR', 'JERGİLİKTİAÝQISQAŞA', 'JERGİLİKTİAÝQISQA', 'LOCALMONTHABBREV' ),
	'localday'               => array( 1,    'JERGİLİKTİKÜN', 'LOCALDAY' ),
	'localday2'              => array( 1,    'JERGİLİKTİKÜN2', 'LOCALDAY2'  ),
	'localdayname'           => array( 1,    'JERGİLİKTİKÜNATAWI', 'LOCALDAYNAME' ),
	'localyear'              => array( 1,    'JERGİLİKTİJIL', 'LOCALYEAR' ),
	'localtime'              => array( 1,    'JERGİLİKTİWAQIT', 'LOCALTIME' ),
	'localhour'              => array( 1,    'JERGİLİKTİSAĞAT', 'LOCALHOUR' ),
	'numberofpages'          => array( 1,    'BETSANI', 'NUMBEROFPAGES' ),
	'numberofarticles'       => array( 1,    'MAQALASANI', 'NUMBEROFARTICLES' ),
	'numberoffiles'          => array( 1,    'FAÝLSANI', 'NUMBEROFFILES' ),
	'numberofusers'          => array( 1,    'QATISWŞISANI', 'NUMBEROFUSERS' ),
	'numberofedits'          => array( 1,    'ÖÑDEMESANI', 'TÜZETWSANI', 'NUMBEROFEDITS' ),
	'pagename'               => array( 1,    'BETATAWI', 'PAGENAME' ),
	'pagenamee'              => array( 1,    'BETATAWI2', 'PAGENAMEE' ),
	'namespace'              => array( 1,    'ESİMAYASI', 'NAMESPACE' ),
	'namespacee'             => array( 1,    'ESİMAYASI2', 'NAMESPACEE' ),
	'talkspace'              => array( 1,    'TALQILAWAYASI', 'TALKSPACE' ),
	'talkspacee'             => array( 1,    'TALQILAWAYASI2', 'TALKSPACEE' ),
	'subjectspace'           => array( 1,    'TAQIRIPBETİ', 'MAQALABETİ', 'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'          => array( 1,    'TAQIRIPBETİ2', 'MAQALABETİ2', 'SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'           => array( 1,    'TOLIQBETATAWI', 'FULLPAGENAME' ),
	'fullpagenamee'          => array( 1,    'TOLIQBETATAWI2', 'FULLPAGENAMEE' ),
	'subpagename'            => array( 1,    'BETŞEATAWI', 'ASTIÑĞIBETATAWI', 'SUBPAGENAME' ),
	'subpagenamee'           => array( 1,    'BETŞEATAWI2', 'ASTIÑĞIBETATAWI2', 'SUBPAGENAMEE' ),
	'basepagename'           => array( 1,    'NEGİZGİBETATAWI', 'BASEPAGENAME' ),
	'basepagenamee'          => array( 1,    'NEGİZGİBETATAWI2', 'BASEPAGENAMEE' ),
	'talkpagename'           => array( 1,    'TALQILAWBETATAWI', 'TALKPAGENAME' ),
	'talkpagenamee'          => array( 1,    'TALQILAWBETATAWI2', 'TALKPAGENAMEE' ),
	'subjectpagename'        => array( 1,    'TAQIRIPBETATAWI', 'MAQALABETATAWI', 'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'       => array( 1,    'TAQIRIPBETATAWI2', 'MAQALABETATAWI2', 'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                    => array( 0,    'XBR:', 'MSG:' ),
	'subst'                  => array( 0,    'BÄDEL:', 'SUBST:' ),
	'msgnw'                  => array( 0,    'WÏKÏSİZXBR:', 'MSGNW:' ),
	'img_thumbnail'          => array( 1,    'nobaý', 'thumbnail', 'thumb' ),
	'img_manualthumb'        => array( 1,    'nobaý=$1', 'thumbnail=$1', 'thumb=$1'),
	'img_right'              => array( 1,    'oñğa', 'oñ', 'right' ),
	'img_left'               => array( 1,    'solğa', 'sol', 'left' ),
	'img_none'               => array( 1,    'eşqandaý', 'joq', 'none' ),
	'img_width'              => array( 1,    '$1 nükte', '$1px' ),
	'img_center'             => array( 1,    'ortağa', 'orta', 'center', 'centre' ),
	'img_framed'             => array( 1,    'sürmeli', 'framed', 'enframed', 'frame' ),
	'img_frameless'          => array( 1,    'sürmesiz', 'frameless' ),
	'img_page'               => array( 1,    'bet=$1', 'bet $1', 'page=$1', 'page $1' ),
	'img_upright'            => array( 1,    'tikti', 'tiktik=$1', 'tiktik $1', 'upright', 'upright=$1', 'upright $1' ),
	'img_border'             => array( 1,    'jïekti', 'border' ),
	'img_baseline'           => array( 1,    'tirekjol', 'baseline' ),
	'img_sub'                => array( 1,    'astılığı', 'ast', 'sub'),
	'img_super'              => array( 1,    'üstiligi', 'üst', 'sup', 'super', 'sup' ),
	'img_top'                => array( 1,    'üstine', 'top' ),
	'img_text_top'           => array( 1,    'mätin-üstinde', 'text-top' ),
	'img_middle'             => array( 1,    'aralığına', 'middle' ),
	'img_bottom'             => array( 1,    'astına', 'bottom' ),
	'img_text_bottom'        => array( 1,    'mätin-astında', 'text-bottom' ),
	'int'                    => array( 0,    'İŞKİ:', 'INT:' ),
	'sitename'               => array( 1,    'TORAPATAWI', 'SITENAME' ),
	'ns'                     => array( 0,    'EA:', 'ESİMAYA:', 'NS:' ),
	'localurl'               => array( 0,    'JERGİLİKTİJAÝ:', 'LOCALURL:' ),
	'localurle'              => array( 0,    'JERGİLİKTİJAÝ2:', 'LOCALURLE:' ),
	'server'                 => array( 0,    'SERVER', 'SERVER' ),
	'servername'             => array( 0,    'SERVERATAWI', 'SERVERNAME' ),
	'scriptpath'             => array( 0,    'ÄMİRJOLI', 'SCRIPTPATH' ),
	'grammar'                => array( 0,    'SEPTİGİ:', 'SEPTİK:', 'GRAMMAR:' ),
	'notitleconvert'         => array( 0,    '__TAQIRIPATINTÜRLENDİRGİZBEW__', '__TATJOQ__', '__ATAWALMASTIRĞIZBAW__', '__AABAW__', '__NOTITLECONVERT__', '__NOTC__' ),
	'nocontentconvert'       => array( 0,    '__MAĞLUMATINTÜRLENDİRGİZBEW__', '__MATJOQ__', '__MAĞLUMATALMASTIRĞIZBAW__', '__MABAW__', '__NOCONTENTCONVERT__', '__NOCC__' ),
	'currentweek'            => array( 1,    'AĞIMDAĞIAPTASI', 'AĞIMDAĞIAPTA', 'CURRENTWEEK' ),
	'currentdow'             => array( 1,    'AĞIMDAĞIAPTAKÜNİ', 'CURRENTDOW' ),
	'localweek'              => array( 1,    'JERGİLİKTİAPTASI', 'JERGİLİKTİAPTA', 'LOCALWEEK' ),
	'localdow'               => array( 1,    'JERGİLİKTİAPTAKÜNİ', 'LOCALDOW' ),
	'revisionid'             => array( 1,    'TÜZETWNÖMİRİ', 'NUSQANÖMİRİ', 'REVISIONID' ),
	'revisionday'            => array( 1,    'TÜZETWKÜNİ','NUSQAKÜNİ', 'REVISIONDAY' ),
	'revisionday2'           => array( 1,    'TÜZETWKÜNİ2', 'NUSQAKÜNİ2', 'REVISIONDAY2' ),
	'revisionmonth'          => array( 1,    'TÜZETWAÝI', 'NUSQAAÝI', 'REVISIONMONTH' ),
	'revisionyear'           => array( 1,    'TÜZETWJILI', 'NUSQAJILI', 'REVISIONYEAR' ),
	'revisiontimestamp'      => array( 1,    'TÜZETWWAQITITAÑBASI', 'NUSQAWAQITTÜÝİNDEMESİ', 'REVISIONTIMESTAMP' ),
	'plural'                 => array( 0,    'KÖPŞETÜRİ:','KÖPŞE:', 'PLURAL:' ),
	'fullurl'                => array( 0,    'TOLIQJAÝI:', 'TOLIQJAÝ:', 'FULLURL:' ),
	'fullurle'               => array( 0,    'TOLIQJAÝI2:', 'TOLIQJAÝ2:', 'FULLURLE:' ),
	'lcfirst'                => array( 0,    'KÄ1:', 'KİŞİÄRİPPEN1:', 'LCFIRST:' ),
	'ucfirst'                => array( 0,    'BÄ1:', 'BASÄRİPPEN1:', 'UCFIRST:' ),
	'lc'                     => array( 0,    'KÄ:', 'KİŞİÄRİPPEN:', 'LC:' ),
	'uc'                     => array( 0,    'BÄ:', 'BASÄRİPPEN:', 'UC:' ),
	'raw'                    => array( 0,    'QAM:', 'RAW:' ),
	'displaytitle'           => array( 1,    'KÖRİNETİNTAQIRIAPATI', 'KÖRSETİLETİNATAW', 'DISPLAYTITLE' ),
	'rawsuffix'              => array( 1,    'Q', 'R' ),
	'newsectionlink'         => array( 1,    '__JAÑABÖLİMSİLTEMESİ__', '__NEWSECTIONLINK__' ),
	'currentversion'         => array( 1,    'BAĞDARLAMANUSQASI', 'CURRENTVERSION' ),
	'urlencode'              => array( 0,    'JAÝDIMUQAMDAW:', 'URLENCODE:' ),
	'anchorencode'           => array( 0,    'JÄKİRDİMUQAMDAW', 'ANCHORENCODE' ),
	'currenttimestamp'       => array( 1,    'AĞIMDAĞIWAQITTÜÝİNDEMESİ', 'AĞIMDAĞIWAQITTÜÝİN', 'CURRENTTIMESTAMP' ),
	'localtimestamp'         => array( 1,    'JERGİLİKTİWAQITTÜÝİNDEMESİ', 'JERGİLİKTİWAQITTÜÝİN', 'LOCALTIMESTAMP' ),
	'directionmark'          => array( 1,    'BAĞITBELGİSİ', 'DIRECTIONMARK', 'DIRMARK' ),
	'language'               => array( 0,    '#TİL:', '#LANGUAGE:' ),
	'contentlanguage'        => array( 1,    'MAĞLUMATTİLİ', 'CONTENTLANGUAGE', 'CONTENTLANG' ),
	'pagesinnamespace'       => array( 1,    'ESİMAYABETSANI:', 'EABETSANI:', 'AYABETSANI:', 'PAGESINNAMESPACE:', 'PAGESINNS:' ),
	'numberofadmins'         => array( 1,    'ÄKİMŞİSANI', 'NUMBEROFADMINS' ),
	'formatnum'              => array( 0,    'SANPİŞİMİ', 'FORMATNUM' ),
	'padleft'                => array( 0,    'SOLĞAIĞIS', 'SOLIĞIS', 'PADLEFT' ),
	'padright'               => array( 0,    'OÑĞAIĞIS', 'OÑIĞIS', 'PADRIGHT' ),
	'special'                => array( 0,    'arnaýı', 'special' ),
	'defaultsort'            => array( 1,    'ÄDEPKİSURIPTAW:', 'ÄDEPKİSANATSURIPTAW:', 'ÄDEPKİSURIPTAWKİLTİ:', 'ÄDEPKİSURIP:', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:' ),
	'filepath'               => array( 0,    'FAÝLMEKENİ:', 'FILEPATH:' ),
	'tag'                    => array( 0,    'belgi', 'tag' ),
	'hiddencat'              => array( 1,    '__JASIRINSANAT__', '__HIDDENCAT__' ),
	'pagesincategory'        => array( 1,    'SANATTAĞIBETTER', 'PAGESINCATEGORY', 'PAGESINCAT' ),
	'pagesize'               => array( 1,    'BETMÖLŞERİ', 'PAGESIZE' ),
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Şınjırlı_aýdağıştar', 'Şınjırlı_aýdatwlar' ),
	'BrokenRedirects'           => array( 'Jaramsız_aýdağıştar', 'Jaramsız_aýdatwlar' ),
	'Disambiguations'           => array( 'Aýrıqtı_better' ),
	'Userlogin'                 => array( 'Qatıswşı_kirwi' ),
	'Userlogout'                => array( 'Qatıswşı_şığwı' ),
	'CreateAccount'             => array( 'Jaña_tirkelgi', 'Tirkelgi_Jaratw' ),
	'Preferences'               => array( 'Baptalımdar', 'Baptaw' ),
	'Watchlist'                 => array( 'Baqılaw_tizimi' ),
	'Recentchanges'             => array( 'Jwıqtağı_özgerister' ),
	'Upload'                    => array( 'Qotarıp_berw', 'Qotarw' ),
	'Imagelist'                 => array( 'Swret_tizimi' ),
	'Newimages'                 => array( 'Jaña_swretter' ),
	'Listusers'                 => array( 'Qatıswşılar', 'Qatıswşı_tizimi' ),
	'Listgrouprights'           => array( 'Top_quqıqtarı_tizimi' ),
	'Statistics'                => array( 'Sanaq' ),
	'Randompage'                => array( 'Kezdeýsoq', 'Kezdeýsoq_bet' ),
	'Lonelypages'               => array( 'Sayaq_better' ),
	'Uncategorizedpages'        => array( 'Sanatsız_better' ),
	'Uncategorizedcategories'   => array( 'Sanatsız_sanattar' ),
	'Uncategorizedimages'       => array( 'Sanatsız_swretter' ),
	'Uncategorizedtemplates'    => array( 'Sanatsız_ülgiler' ),
	'Unusedcategories'          => array( 'Paýdalanılmağan_sanattar' ),
	'Unusedimages'              => array( 'Paýdalanılmağan_swretter' ),
	'Wantedpages'               => array( 'Toltırılmağan_better', 'Jaramsız_siltemeler' ),
	'Wantedcategories'          => array( 'Toltırılmağan_sanattar' ),
	'Missingfiles'              => array( 'Joq_faýldar', 'Joq_swretter' ),
	'Mostlinked'                => array( 'Eñ_köp_siltengen_better' ),
	'Mostlinkedcategories'      => array( 'Eñ_köp_paýdalanılğan_sanattar', 'Eñ_köp_siltengen_sanattar' ),
	'Mostlinkedtemplates'       => array( 'Eñ_köp_paýdalanılğan_ülgiler', 'Eñ_köp_siltengen_ülgiler' ),
	'Mostcategories'            => array( 'Eñ_köp_sanattar_barı' ),
	'Mostimages'                => array( 'Eñ_köp_paýdalanılğan_swretter', 'Eñ_köp_swretter_barı' ),
	'Mostrevisions'             => array( 'Eñ_köp_tüzetilgen', 'Eñ_köp_nusqalar_barı' ),
	'Fewestrevisions'           => array( 'Eñ_az_tüzetilgen ' ),
	'Shortpages'                => array( 'Qısqa_better' ),
	'Longpages'                 => array( 'Uzın_better', 'Ülken_better' ),
	'Newpages'                  => array( 'Jaña_better' ),
	'Ancientpages'              => array( 'Eski_better' ),
	'Deadendpages'              => array( 'Tuýıq_better' ),
	'Protectedpages'            => array( 'Qorğalğan_better' ),
	'Protectedtitles'           => array( 'Qorğalğan_taqırıptar', 'Qorğalğan_atawlar' ),
	'Allpages'                  => array( 'Barlıq_better' ),
	'Prefixindex'               => array( 'Bastawış_tizimi' ) ,
	'Ipblocklist'               => array( 'Buğattalğandar' ),
	'Specialpages'              => array( 'Arnaýı_better' ),
	'Contributions'             => array( 'Ülesi' ),
	'Emailuser'                 => array( 'Xat_jiberw' ),
	'Confirmemail'              => array( 'Quptaw_xat' ),
	'Whatlinkshere'             => array( 'Mında_siltegender' ),
	'Recentchangeslinked'       => array( 'Siltengenderdiñ_özgeristeri', 'Qatıstı_özgerister' ),
	'Movepage'                  => array( 'Betti_jıljıtw' ),
	'Blockme'                   => array( 'Özdiktik_buğattaw', 'Özdik_buğattaw', 'Meni_buğattaw',),
	'Booksources'               => array( 'Kitap_qaýnarları' ),
	'Categories'                => array( 'Sanattar' ),
	'Export'                    => array( 'Sırtqa_berw' ),
	'Version'                   => array( 'Nusqası' ),
	'Allmessages'               => array( 'Barlıq_xabarlar' ),
	'Log'                       => array( 'Jwrnal', 'Jwrnaldar' ),
	'Blockip'                   => array( 'Jaýdı_buğattaw', 'IP_buğattaw'),
	'Undelete'                  => array( 'Joywdı_boldırmaw', 'Joýılğandı_qaýtarw' ),
	'Import'                    => array( 'Sırttan_alw' ),
	'Lockdb'                    => array( 'Derekqordı_qulıptaw' ),
	'Unlockdb'                  => array( 'Derekqordı_qulıptamaw' ),
	'Userrights'                => array( 'Qatıswşı_quqıqtarı' ),
	'MIMEsearch'                => array( 'MIME_türimen_izdew' ),
	'FileDuplicateSearch'       => array( 'Faýl_telnusqasın_izdew', 'Qaýtalanğan_faýldardı_izdew' ),
	'Unwatchedpages'            => array( 'Baqılanılmağan_better' ),
	'Listredirects'             => array( 'Aýdatw_tizimi' ),
	'Revisiondelete'            => array( 'Tüzetw_joyw', 'Nusqanı_joyw' ),
	'Unusedtemplates'           => array( 'Paýdalanılmağan_ülgiler' ),
	'Randomredirect'            => array( 'Kedeýsoq_aýdağış', 'Kedeýsoq_aýdatw' ),
	'Mypage'                    => array( 'Jeke_betim' ),
	'Mytalk'                    => array( 'Talqılawım' ),
	'Mycontributions'           => array( 'Ülesim' ),
	'Listadmins'                => array( 'Äkimşiler', 'Äkimşi_tizimi'),
	'Listbots'                  => array( 'Bottar', 'Bottar_tizimi' ),
	'Popularpages'              => array( 'Eñ_köp_qaralğan_better', 'Äýgili_better' ),
	'Search'                    => array( 'İzdew' ),
	'Resetpass'                 => array( 'Qupïya_sözdi_qaýtarw' ),
	'Withoutinterwiki'          => array( 'Wïkï-aralıqsızdar' ),
	'MergeHistory'              => array( 'Tarïx_biriktirw' ),
	'Filepath'                  => array( 'Faýl_mekeni' ),
	'Invalidateemail'           => array( 'Quptamaw_xatı' ),
);

#-------------------------------------------------------------------
# Default messages
#-------------------------------------------------------------------

$messages = array(
# User preference toggles
'tog-underline'               => 'Siltemeniñ astın sız:',
'tog-highlightbroken'         => 'Jaramsız siltemelerdi <a href="" class="new">bılaý sïyaqtı</a> pişimde (balaması: bılaý sïyaqtı<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Ejelerdi eni boýınşa twralaw',
'tog-hideminor'               => 'Jwıqtağı özgeristerden şağın öñdemelerdi jasır',
'tog-extendwatchlist'         => 'Baqılaw tizimdi ulğaýt (barlıq jaramdı özgeristerdi körset)',
'tog-usenewrc'                => 'Keñeýtilgen jwıqtağı özgerister (JavaScript)',
'tog-numberheadings'          => 'Bas joldardı özdiktik nomirle',
'tog-showtoolbar'             => 'Öñdew qwraldar jolağın körset (JavaScript)',
'tog-editondblclick'          => 'Qos nuqımdap öñdew (JavaScript)',
'tog-editsection'             => 'Bölimderdi [öñdew] siltemesimen öñdewin qos',
'tog-editsectiononrightclick' => 'Bölim taqırıbın oñ nuqwmen öñdewin qos (JavaScript)',
'tog-showtoc'                 => 'Mazmunın körset (3-ten arta bölimi barılarğa)',
'tog-rememberpassword'        => 'Kirgenimdi osı komp′ywterde umıtpa',
'tog-editwidth'               => 'Kiristirw ornı tolıq enimen',
'tog-watchcreations'          => 'Men bastağan betterdi baqılaw tizimime üste',
'tog-watchdefault'            => 'Men öñdegen betterdi baqılaw tizimime üste',
'tog-watchmoves'              => 'Men jıljıtqan betterdi baqılaw tizimime üste',
'tog-watchdeletion'           => 'Men joýğan betterdi baqılaw tizimime üste',
'tog-minordefault'            => 'Ädepkiden barlıq öñdemelerdi şağın dep belgile',
'tog-previewontop'            => 'Qarap şığw awmağı kiristirw ornı aldında',
'tog-previewonfirst'          => 'Birinşi öñdegende qarap şığw',
'tog-nocache'                 => 'Bet bürkemelewin öşir',
'tog-enotifwatchlistpages'    => 'Baqılanğan bet özgergende mağan xat jiber',
'tog-enotifusertalkpages'     => 'Talqılawım özgergende mağan xat jiber',
'tog-enotifminoredits'        => 'Şağın öñdeme twralı da mağan xat jiber',
'tog-enotifrevealaddr'        => 'E-poştamnıñ mekenjaýın eskertw xattarda aş',
'tog-shownumberswatching'     => 'Baqılap turğan qatıswşılardıñ sanın körset',
'tog-fancysig'                => 'Qam qoltañba (özdiktik siltemesiz)',
'tog-externaleditor'          => 'Şettik öñdewişti ädepkiden qoldan (tek sarapşılar üşin, komp′ywteriñizde arnawlı baptalımdar kerek)',
'tog-externaldiff'            => 'Şettik aýırmağıştı ädepkiden qoldan (tek sarapşılar üşin, komp′ywteriñizde arnawlı baptalımdar kerek)',
'tog-showjumplinks'           => '«Ötip ketw» qatınaw siltemelerin qos',
'tog-uselivepreview'          => 'Twra qarap şığwdı qoldanw (JavaScript) (Sınaqtama)',
'tog-forceeditsummary'        => 'Öñdemeniñ qısqaşa mazmundaması bos qalğanda mağan eskert',
'tog-watchlisthideown'        => 'Öñdemelerimdi baqılaw tizimnen jasır',
'tog-watchlisthidebots'       => 'Bot öñdemelerin baqılaw tizimnen jasır',
'tog-watchlisthideminor'      => 'Şağın öñdemelerdi baqılaw tiziminde körsetpe',
'tog-nolangconversion'        => 'Til türi awdarısın öşir',
'tog-ccmeonemails'            => 'Basqa qatıswşığa jibergen xatımnıñ köşirmesin mağan da jönelt',
'tog-diffonly'                => 'Aýırma astında bet mağlumatın körsetpe',
'tog-showhiddencats'          => 'Jasırın sanattardı körset',

'underline-always'  => 'Ärqaşan',
'underline-never'   => 'Eşqaşan',
'underline-default' => 'Şolğış boýınşa',

'skinpreview' => '(Qarap şığw)',

# Dates
'sunday'        => 'Jeksenbi',
'monday'        => 'Düýsenbi',
'tuesday'       => 'Seýsenbi',
'wednesday'     => 'Särsenbi',
'thursday'      => 'Beýsenbi',
'friday'        => 'Juma',
'saturday'      => 'Senbi',
'sun'           => 'Jek',
'mon'           => 'Düý',
'tue'           => 'Beý',
'wed'           => 'Sär',
'thu'           => 'Beý',
'fri'           => 'Jum',
'sat'           => 'Sen',
'january'       => 'qañtar',
'february'      => 'aqpan',
'march'         => 'nawrız',
'april'         => 'cäwir',
'may_long'      => 'mamır',
'june'          => 'mawsım',
'july'          => 'şilde',
'august'        => 'tamız',
'september'     => 'qırküýek',
'october'       => 'qazan',
'november'      => 'qaraşa',
'december'      => 'jeltoqsan',
'january-gen'   => 'qañtardıñ',
'february-gen'  => 'aqpannıñ',
'march-gen'     => 'nawrızdıñ',
'april-gen'     => 'säwirdiñ',
'may-gen'       => 'mamırdıñ',
'june-gen'      => 'mawsımnıñ',
'july-gen'      => 'şildeniñ',
'august-gen'    => 'tamızdıñ',
'september-gen' => 'qırküýektiñ',
'october-gen'   => 'qazannıñ',
'november-gen'  => 'qaraşanıñ',
'december-gen'  => 'jeltoqsannıñ',
'jan'           => 'qañ',
'feb'           => 'aqp',
'mar'           => 'naw',
'apr'           => 'cäw',
'may'           => 'mam',
'jun'           => 'maw',
'jul'           => 'şil',
'aug'           => 'tam',
'sep'           => 'qır',
'oct'           => 'qaz',
'nov'           => 'qar',
'dec'           => 'jel',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Sanat|Sanattar}}',
'category_header'                => '«$1» sanatındağı better',
'subcategories'                  => 'Sanatşalar',
'category-media-header'          => '«$1» sanatındağı taspalar',
'category-empty'                 => "''Bul sanatta ağımda eş bet ne taspa joq.''",
'hidden-categories'              => '{{PLURAL:$1|Jasırın sanat|Jasırın sanattar}}',
'hidden-category-category'       => 'Jasırın sanattar', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Bul sanatta tek kelesi sanatşa bar.|Bul sanatta kelesi $1 sanatşa bar (ne barlığı $2).}}',
'category-subcat-count-limited'  => 'Bul sanatta kelesi $1 sanatşa bar.',
'category-article-count'         => '{{PLURAL:$2|Bul sanatta tek kelesi bet bar.|Bul sanatta kelesi $1 bet bar (ne barlığı $2).}}',
'category-article-count-limited' => 'Ağımdağı sanatta kelesi $1 bet bar.',
'category-file-count'            => '{{PLURAL:$2|Bud sanatta tek kelesi faýl bar.|Bul sanatta kelesi $1 faýl bar (ne barlığı $2).}}',
'category-file-count-limited'    => 'Ağımdağı sanatta kelesi $1 faýl bar.',
'listingcontinuesabbrev'         => '(jalğ.)',

'mainpagetext'      => "<big>'''MedïaWïkï bwması sätti ornatıldı.'''</big>",
'mainpagedocfooter' => 'Wïkï bağdarlamalıq jasaqtamasın qalaý qoldanatın aqparatı üşin [http://meta.wikimedia.org/wiki/Help:Contents Paýdalanwşılıq nusqawlarınan] keñes alıñız.

== Bastaw üşin ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Baptalım qalawlarınıñ tizimi]
* [http://www.mediawiki.org/wiki/Manual:FAQ MedïaWïkïdiñ Jïı Qoýılğan Sawaldarı]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MedïaWïkï şığw twralı xat taratw tizimi]',

'about'          => 'Joba twralı',
'article'        => 'Mağlumat beti',
'newwindow'      => '(jaña terezede)',
'cancel'         => 'Boldırmaw',
'qbfind'         => 'Tabw',
'qbbrowse'       => 'Şolw',
'qbedit'         => 'Öñdew',
'qbpageoptions'  => 'Bul bet',
'qbpageinfo'     => 'Aýnala',
'qbmyoptions'    => 'Betterim',
'qbspecialpages' => 'Arnaýı better',
'moredotdotdot'  => 'Köbirek…',
'mypage'         => 'Jeke betim',
'mytalk'         => 'Talqılawım',
'anontalk'       => 'IP talqılawı',
'navigation'     => 'Şarlaw',
'and'            => 'jäne',

# Metadata in edit box
'metadata_help' => 'Qosımşa derekter:',

'errorpagetitle'    => 'Qatelik',
'returnto'          => '$1 degenge qaýta kelw.',
'tagline'           => '{{GRAMMAR:ablative|{{SITENAME}}}}',
'help'              => 'Anıqtama',
'search'            => 'İzdew',
'searchbutton'      => 'İzde',
'go'                => 'Ötw',
'searcharticle'     => 'Öt!',
'history'           => 'Bet tarïxı',
'history_short'     => 'Tarïxı',
'updatedmarker'     => 'soñğı kelip-ketwimnen beri jañalanğan',
'info_short'        => 'Mälimet',
'printableversion'  => 'Basıp şığarw üşin',
'permalink'         => 'Turaqtı silteme',
'print'             => 'Basıp şığarw',
'edit'              => 'Öñdew',
'create'            => 'Bastaw',
'editthispage'      => 'Betti öñdew',
'create-this-page'  => 'Jaña bet bastaw',
'delete'            => 'Joyw',
'deletethispage'    => 'Betti joyw',
'undelete_short'    => '$1 öñdeme joywın boldırmaw',
'protect'           => 'Qorğaw',
'protect_change'    => 'qorğawdı özgertw',
'protectthispage'   => 'Betti qorğaw',
'unprotect'         => 'Qorğamaw',
'unprotectthispage' => 'Betti qorğamaw',
'newpage'           => 'Jaña bet',
'talkpage'          => 'Betti talqılaw',
'talkpagelinktext'  => 'Talqılawı',
'specialpage'       => 'Arnaýı bet',
'personaltools'     => 'Jeke quraldar',
'postcomment'       => 'Mändeme jöneltw',
'articlepage'       => 'Mağlumat betin qaraw',
'talk'              => 'Talqılaw',
'views'             => 'Körinis',
'toolbox'           => 'Quraldar',
'userpage'          => 'Qatıswşı betin qaraw',
'projectpage'       => 'Joba betin qaraw',
'imagepage'         => 'Taspa betin qaraw',
'mediawikipage'     => 'Xabar betin qaraw',
'templatepage'      => 'Ülgi betin qaraw',
'viewhelppage'      => 'Anıqtama betin qaraw',
'categorypage'      => 'Sanat betin qaraw',
'viewtalkpage'      => 'Talqılaw betin qaraw',
'otherlanguages'    => 'Basqa tilderde',
'redirectedfrom'    => '($1 betinen aýdatılğan)',
'redirectpagesub'   => 'Aýdatw beti',
'lastmodifiedat'    => 'Bul bettiñ özgertilgen soñğı kezi: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Bul bet $1 ret qatınalğan.',
'protectedpage'     => 'Qorğalğan bet',
'jumpto'            => 'Mında ötw:',
'jumptonavigation'  => 'şarlaw',
'jumptosearch'      => 'izdew',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} twralı',
'aboutpage'            => 'Project:Joba twralı',
'bugreports'           => 'Qatelik bayanattarı',
'bugreportspage'       => 'Project:Qatelik eseptemeleri',
'copyright'            => 'Mağlumat $1 şartımen jetimdi.',
'copyrightpagename'    => '{{SITENAME}} awtorlıq quqıqtarı',
'copyrightpage'        => '{{ns:project}}:Awtorlıq quqıqtar',
'currentevents'        => 'Ağımdağı oqïğalar',
'currentevents-url'    => 'Project:Ağımdağı oqïğalar',
'disclaimers'          => 'Jawapkerşilikten bas tartw',
'disclaimerpage'       => 'Project:Jawapkerşilikten bas tartw',
'edithelp'             => 'Öndew anıqtaması',
'edithelppage'         => 'Help:Öñdew',
'faq'                  => 'Jïi qoýılğan sawaldar',
'faqpage'              => 'Project:Jïi qoýılğan sawaldar',
'helppage'             => 'Help:Mazmunı',
'mainpage'             => 'Bastı bet',
'mainpage-description' => 'Bastı bet',
'policy-url'           => 'Project:Erejeler',
'portal'               => 'Qawım portalı',
'portal-url'           => 'Project:Qawım portalı',
'privacy'              => 'Jeke qupïyasın saqtaw',
'privacypage'          => 'Project:Jeke qupïyasın saqtaw',

'badaccess'        => 'Ruqsat qatesi',
'badaccess-group0' => 'Suratılğan äreketiñizdi jegwiñizge ruqsat etilmeýdi.',
'badaccess-group1' => 'Suratılğan äreketiñiz $1 tobınıñ qatıswşılarına şekteledi.',
'badaccess-group2' => 'Suratılğan äreketiñiz $1 toptarı biriniñ qatwsışılarına şekteledi.',
'badaccess-groups' => 'Suratılğan äreketiñiz $1 toptarı biriniñ qatwsışılarına şekteledi.',

'versionrequired'     => 'MediaWiki $1 nusqası kerek',
'versionrequiredtext' => 'Bul betti qoldanw üşin MediaWiki $1 nusqası kerek. [[{{ns:special}}:Version|Jüýe nusqası betin]] qarañız.',

'ok'                      => 'Jaraýdı',
'pagetitle'               => '$1 — {{SITENAME}}',
'retrievedfrom'           => '«$1» betinen alınğan',
'youhavenewmessages'      => 'Sizge $1 bar ($2).',
'newmessageslink'         => 'jaña xabarlar',
'newmessagesdifflink'     => 'soñğı özgerisine',
'youhavenewmessagesmulti' => '$1 degende jaña xabarlar bar',
'editsection'             => 'öñdew',
'editold'                 => 'öñdew',
'viewsourceold'           => 'qaýnar közin qaraw',
'editsectionhint'         => 'Mına bölimdi öñdew: $1',
'toc'                     => 'Mazmunı',
'showtoc'                 => 'körset',
'hidetoc'                 => 'jasır',
'thisisdeleted'           => '$1 qaraýsız ba, ne qalpına keltiresiz be?',
'viewdeleted'             => '$1 qaraýsız ba?',
'restorelink'             => 'Joýılğan $1 öñdemeni',
'feedlinks'               => 'Arna:',
'feed-invalid'            => 'Jaramsız jazılımdı arna türi.',
'feed-unavailable'        => '{{SITENAME}} jobasında taratılatın arnalar joq',
'site-rss-feed'           => '$1 RSS arnası',
'site-atom-feed'          => '$1 Atom arnası',
'page-rss-feed'           => '«$1» — RSS arnası',
'page-atom-feed'          => '«$1» — Atom arnası',
'red-link-title'          => '$1 (äli jazılmağan)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Bet',
'nstab-user'      => 'Jeke bet',
'nstab-media'     => 'Taspa beti',
'nstab-special'   => 'Arnaýı',
'nstab-project'   => 'Joba beti',
'nstab-image'     => 'Faýl beti',
'nstab-mediawiki' => 'Xabar',
'nstab-template'  => 'Ülgi',
'nstab-help'      => 'Anıqtama',
'nstab-category'  => 'Sanat',

# Main script and global functions
'nosuchaction'      => 'Mınadaý eş äreket joq',
'nosuchactiontext'  => 'Osı URL jaýımen engizilgen äreketti osı wïkï joramaldap bilmedi.',
'nosuchspecialpage' => 'Mınadaý eş arnaýı bet joq',
'nospecialpagetext' => "<big>'''Jaramsız arnaýı betti suradıñız.'''</big>

Jaramdı arnaýı bet tizimin [[{{#special:Specialpages}}|{{int:specialpages}}]] degennen taba alasız.",

# General errors
'error'                => 'Qate',
'databaseerror'        => 'Derekqor qatesi',
'dberrortext'          => 'Derekqor suranımında söýlem jüýesiniñ qatesi boldı.
Bul bağdarlamalıq jasaqtama qatesin belgilewi mümkin.
Soñğı bolğan derekqor suranımı:
<blockquote><tt>$1</tt></blockquote>
mına jeteden «<tt>$2</tt>».
MySQL qaýtarğan qatesi «<tt>$3: $4</tt>».',
'dberrortextcl'        => 'Derekqor suranımında söýlem jüýesiniñ qatesi boldı.
Soñğı bolğan derekqor suranımı:
«$1»
mına jeteden: «$2».
MySQL qaýtarğan qatesi «$3: $4»',
'noconnect'            => 'Ğafw etiñiz! Bul wïkïde keýbir texnïkalıq qïınşılıqtar kezdesti, jäne de derekqor serverine baýlanısa almaýdı.<br />
$1',
'nodb'                 => '$1 degen derekqor bölektenbedi',
'cachederror'          => 'Tömende suralğan bettiñ bürkemelengen köşirmesi beriledi, äli jañartılmağan bolwı mümkin.',
'laggedslavemode'      => 'Qulaqtandırw: Bette jwıqtağı jañalawlar bolmawı mümkin.',
'readonly'             => 'Derekqorı qulıptalğan',
'enterlockreason'      => 'Qulıptaw sebebin, qaý waqıtqa deýin qulıptalğanın kiristirip, engiziñiz',
'readonlytext'         => 'Bul derekqor jañadan jazw jäne basqa özgerister jasawdan ağımda qulıptalınğan, mümkin künde-kün derekqordı baptaw üşin, bunı bitirgennen soñ qalıptı iske qaýtarıladı.

Qulıptağan äkimşi bunı bılaý tüsindiredi: $1',
'missing-article'      => 'Bar bolwı jön bılaý atalğan bet mätini derekqorda tabılmadı: «$1» $2.

Bul eskirgen aýırma siltemesine nemese joýılğan bet tarïxı siltemesine ergennen bola beredi.

Eger bul orındı bolmasa, bağdarlamalıq jasaqtamadağı qatege tap bolwıñız mümkin.
Bul twralı naqtı URL jaýına añğartpa jasap, äkimşige bayanattañız.',
'missingarticle-rev'   => '(tüzetw n-i: $1)',
'missingarticle-diff'  => '(Aýrm.: $1, $2)',
'readonly_lag'         => 'Jetek derekqor serverler basqısımen qadamlanğanda osı derekqor özdiktik qulıptalınğan',
'internalerror'        => 'İşki qate',
'internalerror_info'   => 'İşki qatesi: $1',
'filecopyerror'        => '«$1» faýlı «$2» faýlına köşirilmedi.',
'filerenameerror'      => '«$1» faýl atawı «$2» atawına özgertilmedi.',
'filedeleteerror'      => '«$1» faýlı joýılmaýdı.',
'directorycreateerror' => '«$1» qaltası qurılmadı.',
'filenotfound'         => '«$1» faýlı tabılmadı.',
'fileexistserror'      => '«$1» faýlğa jazw ïkemdi emes: faýl bar',
'unexpected'           => 'Kütilmegen mağına: «$1» = «$2».',
'formerror'            => 'Qatelik: pişin jöneltilmeýdi',
'badarticleerror'      => 'Osındaý äreket mına bette atqarılmaýdı.',
'cannotdelete'         => 'Aýtılmış bet ne swret joýılmaýdı.
Bunı basqa birew aldaqaşan joýğan mümkin.',
'badtitle'             => 'Jaramsız taqırıp atı',
'badtitletext'         => 'Suralğan bet taqırıbınıñ atı jaramsız, bos, tilaralıq siltemesi ne wïkï-aralıq taqırıp atı burıs engizilgen.
Mında taqırıp atında qoldalmaýtın birqatar tañbalar bolwı mümkin.',
'perfdisabled'         => 'Ğafw etiñiz! Bul mümkindik, derekqordıñ jıldamılığına äser etip, eşkimge wïkïdi paýdalanwğa bermegesin, waqıtşa öşirilgen.',
'perfcached'           => 'Kelesi derek bürkemelengen, sondıqtan tolıqtaý jañalanmağan bolwı mümkin.',
'perfcachedts'         => 'Kelesi derek bürkemelengen, soñğı jañalanlğan kezi: $1.',
'querypage-no-updates' => 'Bul bettiñ jañartılwı ağımda öşirilgen. Derekteri qazir özgertilmeýdi.',
'wrong_wfQuery_params' => 'wfQuery() fwnkcïyası üşin burıs baptalımdarı bar<br />
Jete: $1<br />
Suranım: $2',
'viewsource'           => 'Qaýnar közin qaraw',
'viewsourcefor'        => '$1 degen üşin',
'actionthrottled'      => 'Äreket bäseñdetildi',
'actionthrottledtext'  => 'Spamğa qarsı küres esebinde, osı äreketti qısqa waqıtta tım köp ret orındawıñız şektelindi, jäne bul şektew şamasınan asıp ketkensiz.
Birneşe mïnöttan qaýta baýqap köriñiz.',
'protectedpagetext'    => 'Öñdewdi qaqpaýlaw üşin bul bet qulıptalınğan.',
'viewsourcetext'       => 'Bul bettiñ qaýnar közin qarawıñızğa jäne köşirip alwñızğa boladı:',
'protectedinterface'   => 'Bul bet bağdarlamalıq jasaqtamanıñ tildesw mätinin jetistiredi, sondıqtan qïyanattı qaqpaýlaw üşin özgertwi qulıptalğan.',
'editinginterface'     => "'''Qulaqtandırw:''' Bağdarlamalıq jasaqtamanıñ tildesw mätinin jetistiretin betin öñdep jatırsız.
Bul bettiñ özgertwi basqa qatıswşılarğa paýdalanwşılıq tildeswi qalaý körinetine äser etedi.
Awdarmalar üşin, MediaWiki bağdarlamasın jersindirw [http://translatewiki.net/wiki/Main_Page?setlang=kk Betawiki jobası] arqılı qarap şeşiñiz.",
'sqlhidden'            => '(SQL suranımı jasırılğan)',
'cascadeprotected'     => 'Bul bet öñdewden qorğalğan, sebebi bul kelesi «bawlı qorğawı» qosılğan {{PLURAL:$1|bettiñ|betterdiñ}} kirikbeti:
$2',
'namespaceprotected'   => "'''$1''' esim ayasındağı betterdi öñdew üşin ruqsatıñız joq.",
'customcssjsprotected' => 'Bul betti öñdewge ruqsatıñız joq, sebebi mında özge qatıswşınıñ jeke baptawları bar.',
'ns-specialprotected'  => '{{ns:special}} esim ayasındağı better öñdelinbeýdi',
'titleprotected'       => "Bul taqırıp atı bastawdan [[{{ns:user}}:$1|$1]] qorğadı.
Keltirilgen sebebi: ''$2''.",

# Login and logout pages
'logouttitle'                => 'Qatıswşı şığwı',
'logouttext'                 => '<strong>Endi jüýeden şıqtıñız.</strong>

Jüýege kirmesten {{SITENAME}} jobasın paýdalanwın jalğastıra alasız, nemese däl sol ne özge qatıswşı bop qaýta krwiñiz mümkin.
Añğartpa: Keýbir better şolğışıñızdıñ bürkemesin tazartqanşa deýin äli de kirp qalğanıñızdaý körinwi mümkin.',
'welcomecreation'            => '== Qoş keldiñiz, $1! ==
Jaña tirkelgiñiz jasaldı.
{{SITENAME}} baptalımdarıñızdı özgertwin umıtpañız.',
'loginpagetitle'             => 'Qatıswşı kirwi',
'yourname'                   => 'Qatıswşı atıñız:',
'yourpassword'               => 'Qupïya söziñiz:',
'yourpasswordagain'          => 'Qupïya sözdi qaýtalañız:',
'remembermypassword'         => 'Meniñ kirgenimdi bul komp′ywterde umıtpa',
'yourdomainname'             => 'Jeli üýşigiñiz:',
'externaldberror'            => 'Osı arada ne şettik rastaw derekqorında qate boldı, nemese şettik tirkelgiñizdi jañalaw ruqsatı joq.',
'loginproblem'               => '<b>Kirwiñiz kezinde osında qïındıqqa tap boldıq.</b><br />Qaýta baýqap köriñiz.',
'login'                      => 'Kirw',
'nav-login-createaccount'    => 'Kirw / Tirkelgi jasaw',
'loginprompt'                => '{{SITENAME}} torabına kirwiñiz üşin «cookies» qosılwı jön.',
'userlogin'                  => 'Kirw / Tirkelgi jasaw',
'logout'                     => 'Şığw',
'userlogout'                 => 'Şığw',
'notloggedin'                => 'Kirmegensiz',
'nologin'                    => 'Kirmegensiz be? $1.',
'nologinlink'                => 'Tirkelgi jasañız',
'createaccount'              => 'Jaña tirkelgi',
'gotaccount'                 => 'Aldaqaşan tirkelgiiñiz bar ma? $1.',
'gotaccountlink'             => 'Kiriñiz',
'createaccountmail'          => 'e-poştamen',
'badretype'                  => 'Engizgen qupïya sözderiñiz bir birine säýkes emes.',
'userexists'                 => 'Engizgen qatıswşı atıñız aldaqaşan paýdalanwda.
Özge atawdı tañdañız.',
'youremail'                  => 'E-poştañız:',
'username'                   => 'Qatıswşı atıñız:',
'uid'                        => 'Qatıswşı teñdestirgişiñiz:',
'prefs-memberingroups'       => 'Kirgen {{PLURAL:$1|tobıñız|toptarıñız}}:',
'yourrealname'               => 'Naqtı atıñız:',
'yourlanguage'               => 'Tiliñiz:',
'yourvariant'                => 'Til/jazba nusqañız:',
'yournick'                   => 'Qoltañbañız:',
'badsig'                     => 'Qam qoltañbañız jaramsız; HTML belgişelerin tekseriñiz.',
'badsiglength'               => 'Laqap atıñız tım uzın;
Bul $1 tañbadan aspawı jön.',
'email'                      => 'E-poştañız',
'prefs-help-realname'        => 'Naqtı atıñız mindetti emes.
Eger bunı jetistirwdi tañdasañız, bul tüzetwiñizdiñ awtorlığın anıqtaw üşin qoldanıladı.',
'loginerror'                 => 'Kirw qatesi',
'prefs-help-email'           => 'E-poşta mekenjaýı mindetti emes, biraq jeke basıñızdı aşpaý «Qatıswşı» nemese «Qatıswşı_talqılawı» degen betteriñiz arqılı barşa sizben baýlanısa aladı.',
'prefs-help-email-required'  => 'E-poşta mekenjaýı kerek.',
'nocookiesnew'               => 'Jaña qatıswşı tirkelgisi jasaldı, biraq kirmegensiz.
Qatıswşı kirw üşin {{SITENAME}} torabında «cookie» faýldarı qoldanıladı.
Sizde «cookies» öşirilgen.
Sonı qosıñız da jaña qatıswşı atıñızdı jäne qupïya söziñizdi engizip kiriñiz.',
'nocookieslogin'             => 'Qatıswşı kirw üşin {{SITENAME}} torabında «cookies» degen qoldanıladı.
Sizde «cookies» öşirilgen.
Sonı qosıñız da kirwdi qaýta baýqap köriñiz.',
'noname'                     => 'Jaramdı qatıswşı atın engizbediñiz.',
'loginsuccesstitle'          => 'Kirwiñiz sätti ötti',
'loginsuccess'               => "'''Siz endi {{SITENAME}} jobasına «$1» retinde kirip otırsız.'''",
'nosuchuser'                 => 'Mında «$1» dep atalğan qatıswşı joq.
Emleñizdi tekseriñiz, ne jaña tirkelgi jasañız.',
'nosuchusershort'            => 'Mında «<nowiki>$1</nowiki>» dep atalğan qatıswşı joq.
Emleñizdi tekseriñiz.',
'nouserspecified'            => 'Qatıswşı atın keltirwiñiz jön.',
'wrongpassword'              => 'Burıs qupïya söz engizilgen. Qaýta baýqap köriñiz.',
'wrongpasswordempty'         => 'Qupïya söz bos bolğan. Qaýta baýqap köriñiz.',
'passwordtooshort'           => 'Qupïya söziñiz jaramsız ne tım qısqa.
Bunda eñ keminde $1 tañba bolwı jäne de qatıswşı atıñızdan özge bolwı jön.',
'mailmypassword'             => 'Qupïya sözimdi xatpen jiber',
'passwordremindertitle'      => '{{SITENAME}} üşin jaña waqıtşa qupïya söz',
'passwordremindertext'       => 'Keýbirew (IP mekenjaýı: $1, bälkim öziñiz bolarsız)
sizge {{SITENAME}} üşin jaña qupïya söz jöneletwin bizden surağan ($4).
«$2» qatıswşınıñ qupïya sözi «$3» boldı endi.
Qazir kirwiñiz jäne qupïya sözdi özgertwiñiz kerek.

Eger bul suranımdı basqa birew istese, ne qupïya sözdi eske tüsirsip endi özgertkiñiz kelmese, eski qupïya söz qoldanwın jağastırıp osı xatqa añğarmawıñızğa da boladı.',
'noemail'                    => 'Osı arada «$1» qatıswşınıñ e-poşta mekenjaýı joq.',
'passwordsent'               => 'Jaña qupïya söz «$1» üşin tirkelgen e-poşta mekenjaýına jöneltildi.
Qabıldağannan keýin kirgende sonı engiziñiz.',
'blocked-mailpassword'       => 'IP mekenjaýıñızdan öñdew buğattalğan, sondıqtan qïyanattı qaqpaýlaw üşin qupïya sözdi qalpına keltirw jetesin qoldanwına ruqsat etilmeýdi.',
'eauthentsent'               => 'Quptaw xatı aýtılmış e-poşta mekenjaýına jöneltildi.
Basqa e-poşta xatın jöneltw aldınan, tirkelgi şınınan sizdiki ekenin quptaw üşin xattağı nusqamalarğa eriwñiz jön.',
'throttled-mailpassword'     => 'Soñğı {{PLURAL:$1|sağatta|$1 sağatta}} qupïya söz eskertw xatı aldaqaşan jöneltildi.
Qïyanattı qaqpaýlaw üşin, {{PLURAL:$1|sağat|$1 sağat}} saýın tek bir ğana qupïya söz eskertw xatı jöneltiledi.',
'mailerror'                  => 'Xat jöneltw qatesi: $1',
'acct_creation_throttle_hit' => 'Ğafw etiñiz, siz aldaqaşan $1 ret tirkelgi jasapsız.
Onan artıq isteý almaýsız.',
'emailauthenticated'         => 'E-poşta mekenjaýıñız rastalğan kezi: $1.',
'emailnotauthenticated'      => 'E-poşta mekenjaýıñız äli rastalğan joq.
Kelesi ärbir mümkindikter üşin eş xat jöneltilmeýdi.',
'noemailprefs'               => 'Osı mümkindikter istewi üşin e-poşta mekenjaýıñızdı engiziñiz.',
'emailconfirmlink'           => 'E-poşta mekenjaýıñızdı quptañız',
'invalidemailaddress'        => 'Osı e-poşta mekenjaýında jaramsız pişim bolğan, qabıl etilmeýdi.
Durıs pişimdelgen mekenjaýdı engiziñiz, ne awmaqtı bos qaldırıñız.',
'accountcreated'             => 'Jaña tirkelgi jasaldı',
'accountcreatedtext'         => '$1 üşin jaña qatıswşı tirkelgisi jasaldı.',
'createaccount-title'        => '{{SITENAME}} üşin tirkelw',
'createaccount-text'         => 'Keýbirew e-poşta mekenjaýıñızdı paýdalanıp {{SITENAME}} jobasında ($4) «$2» atawımen, «$3» qupïya sözimen tirkelgi jasağan.
Jobağa kiriwiñiz jäne qupïya söziñizdi özgertwiñiz tïisti.

Eger bul tirkelgi qatelikpen jasalsa, osı xabarğa elemewiñiz mümkin.',
'loginlanguagelabel'         => 'Til: $1',

# Password reset dialog
'resetpass'               => 'Tirkelginiñ qupïya sözin özgertw',
'resetpass_announce'      => 'Xatpen jiberilgen waqıtşa kodımen kirgensiz.
Kirwiñizdi bitirw üşin, jaña qupïya söziñizdi mında engizwiñiz jön:',
'resetpass_header'        => 'Qupïya sözdi özgertw',
'resetpass_submit'        => 'Qupïya sözdi qoýıñız da kiriñiz',
'resetpass_success'       => 'Qupïya söziñiz sätti özgertildi! Endi kiriñiz…',
'resetpass_bad_temporary' => 'Waqıtşa qupïya söz jaramsız.
Mümkin qupïya söziñizdi aldaqaşan sätti özgertken bolarsız nemese jaña waqıtşa qupïya sözin suratılğansız.',
'resetpass_forbidden'     => '{{SITENAME}} jobasında qupïya sözder özgertilmeýdi',
'resetpass_missing'       => 'Eş pişin derekteri joq.',

# Edit page toolbar
'bold_sample'     => 'Jwan mätin',
'bold_tip'        => 'Jwan mätin',
'italic_sample'   => 'Qïğaş mätin',
'italic_tip'      => 'Qïğaş mätin',
'link_sample'     => 'Silteme taqırıbın atı',
'link_tip'        => 'İşki silteme',
'extlink_sample'  => 'http://www.example.com silteme taqırıbın atı',
'extlink_tip'     => 'Şettik silteme (aldınan http:// engizwin umıtpañız)',
'headline_sample' => 'Bas jol mätini',
'headline_tip'    => '2-şi deñgeýli bas jol',
'math_sample'     => 'Örnekti mında engiziñiz',
'math_tip'        => 'Matematïka örnegi (LaTeX)',
'nowiki_sample'   => 'Pişimdelinbegen mätindi mında engiziñiz',
'nowiki_tip'      => 'Wïkï pişimin elemew',
'image_tip'       => 'Endirilgen faýl',
'media_tip'       => 'Faýl siltemesi',
'sig_tip'         => 'Qoltañbañız jäne waqıt belgisi',
'hr_tip'          => 'Dereleý sızıq (ünemdi qoldanıñız)',

# Edit pages
'summary'                          => 'Tüýindemesi',
'subject'                          => 'Taqırıbı/bas jolı',
'minoredit'                        => 'Bul şağın öñdeme',
'watchthis'                        => 'Betti baqılaw',
'savearticle'                      => 'Betti saqta!',
'preview'                          => 'Qarap şığw',
'showpreview'                      => 'Qarap şıq',
'showlivepreview'                  => 'Twra qarap şıq',
'showdiff'                         => 'Özgeristerdi körset',
'anoneditwarning'                  => "'''Qulaqtandırw:''' Siz jüýege kirmegensiz.
IP mekenjaýıñız bul bettiñ tüzetw tarïxında jazılıp alınadı.",
'missingsummary'                   => "'''Eskertpe:''' Öñdemeniñ qısqaşa mazmundamasın engizbepsiz.
«Saqtaw» tüýmesin tağı bassañız, öñdenmeñiz mändemesiz saqtaladı.",
'missingcommenttext'               => 'Mändemeñizdi tömende engiziñiz.',
'missingcommentheader'             => "'''Eskertpe:''' Bul mändemege taqırıp/basjol jetistirmepsiz.
Eger tağı da Saqtaw tüýmesin nuqısañız, öñdemeñiz solsız saqtaladı.",
'summary-preview'                  => 'Qısqaşa mazmundamasın qarap şığw',
'subject-preview'                  => 'Taqırıbın/bas jolın qarap şığw',
'blockedtitle'                     => 'Qatıswşı buğattalğan',
'blockedtext'                      => "<big>'''Qatıswşı atıñız ne IP mekenjaýıñız buğattalğan.'''</big>

Osı buğattawdı $1 istegen. Keltirilgen sebebi: ''$2''.

* Buğattaw bastalğanı: $8
* Buğattaw bitetini: $6
* Buğattaw maqsatı: $7

Osı buğattawdı talqılaw üşin $1 degenmen, ne özge [[{{{{ns:mediawiki}}:grouppage-sysop}}|äkimşimen]] qatınaswıñızğa boladı.
[[{{#special:Preferences}}|Tirkelgiñiz baptalımdarın]] qoldanıp jaramdı e-poşta mekenjaýın engizgenşe deýin jäne bunı paýdalanwı buğattalmağanşa deýin «Qatıswşığa xat jazw» mümkindigin qoldana almaýsız.
Ağımdıq IP mekenjaýıñız: $3, jäne buğataw nömiri: $5. Sonıñ birewin, nemese ekewin de ärbir suranımıñızğa kiristiriñiz.",
'autoblockedtext'                  => "$1 degen burın özge qatıswşı paýdalanğan bolğasın osı IP mekenjaýıñız özdiktik buğattalğan.
Keltirilgen sebebi:

:''$2''

* Buğattaw bastalğanı: $8
* Buğattaw bitetini: $6

Osı buğattawdı talqılaw üşin $1 degenmen, ne basqa [[{{{{ns:mediawiki}}:grouppage-sysop}}|äkimşimen]] qatınaswıñızğa boladı.

Añğartpa: [[{{#special:Preferences}}|Paýdalanwşılıq baptalımdarıñızdı]] qoldanıp jaramdı e-poşta mekenjaýın engizgenşe deýin jäne bunı paýdalanwı buğattalmağanşa deýin «Qatıswşığa xat jazw» mümkindigin qoldana almaýsız. 

Buğataw nömiriñiz: $5.
Bul nömirdi ärbir suranımıñızdarğa kiristiriñiz.",
'blockednoreason'                  => 'eş sebebi keltirilmegen',
'blockedoriginalsource'            => "'''$1''' degenniñ qaýnar közi tömende körsetiledi:",
'blockededitsource'                => "'''$1''' degenge jasalğan '''öñdemeleriñizdiñ''' mätini tömende körsetiledi:",
'whitelistedittitle'               => 'Öñdew üşin kirwiñiz jön.',
'whitelistedittext'                => 'Betterdi öñdew üşin $1 jön.',
'confirmedittitle'                 => 'Quptaw xatı qaýta öñdelwi jön',
'confirmedittext'                  => 'Betterdi öñdew üşin aldın ala E-poşta mekenjaýıñızdı quptawıñız jön.
E-poşta mekenjaýıñızdı [[{{#special:Preferences}}|paýdalanwşılıq baptalımdarıñız]] arqılı qoýıñız da jaramdılığın tekserip şığıñız.',
'nosuchsectiontitle'               => 'Osındaý eş bölim joq',
'nosuchsectiontext'                => 'Joq bölimdi öñdewdi talap etipsiz.
Mında $1 degen bölim joq eken, öñdemeñizdi saqtaw üşin orın joq.',
'loginreqtitle'                    => 'Kirwiñiz kerek',
'loginreqlink'                     => 'kirw',
'loginreqpagetext'                 => 'Basqa betterdi körw üşin siz $1 bolwıñız jön.',
'accmailtitle'                     => 'Qupïya söz jöneltildi.',
'accmailtext'                      => '$2 jaýına «$1» qupïya sözi jöneltildi.',
'newarticle'                       => '(Jaña)',
'newarticletext'                   => 'Siltemege erip äli bastalmağan betke kelipsiz.
Betti bastaw üşin, tömendegi kiristirw ornında mätiniñizdi teriñiz (köbirek aqparat üşin [[{{{{ns:mediawiki}}:helppage}}|anıqtama betin]] qarañız).
Eger jañılğannan osında kelgen bolsañız, şolğışıñız «Artqa» degen batırmasın nuqıñız.',
'anontalkpagetext'                 => "----''Bul tirkelgisiz (nemese tirkelgisin qoldanbağan) qatıswşı talqılaw beti. Osı qatıswşını biz tek sandıq IP mekenjaýımen teñdestiremiz.
Osındaý IP mekenjaý birneşe qatıswşığa ortaqtastırılğan bolwı mümkin.
Eger siz tirkelgisiz qatıswşı bolsañız jäne sizge qatıssız mändemeler jiberilgenin sezseñiz, basqa tirkelgisiz qatıswşılarmen aralastırmawı üşin [[{{#special:Userlogin}}|tirkeliñiz ne kiriñiz]].''",
'noarticletext'                    => 'Bul bette ağımda eş mätin joq, basqa betterden osı bet atawın [[Special:Search/{{PAGENAME}}|izdep körwiñizge]] nemese osı betti [{{fullurl:{{FULLPAGENAME}}|action=edit}} tüzetwiñizge] boladı.',
'userpage-userdoesnotexist'        => '«$1» qatıswşı tirkelgisi jazıp alınbağan. Bul betti bastaw/öñdew talabıñızdı tekserip şığıñız.',
'clearyourcache'                   => "'''Añğartpa:''' Saqtağannan keýin, özgeristerdi körw üşin şolğış bürkemesin orağıtw ıqtïmal. '''Mozilla / Firefox / Safari:''' ''Qaýta jüktew'' batırmasın nuqığanda ''Shift'' tutıñız, ne ''Ctrl-Shift-R'' basıñız (Apple Mac — ''Cmd-Shift-R''); '''IE:''' ''Jañartw'' batırmasın nuqığanda ''Ctrl'' tutıñız, ne ''Ctrl-F5'' basıñız; '''Konqueror:''': ''Jañartw'' batırmasın jaý nuqıñız, ne ''F5'' basıñız; '''Opera''' paýdanwşıları ''Quraldar→Baptalımdar'' degenge barıp bürkemesin tolıq tazartw jön.",
'usercssjsyoucanpreview'           => '<strong>Aqıl-keñes:</strong> Jaña CSS/JS faýlın saqtaw aldında «Qarap şığw» batırmasın qoldanıp sınaqtañız.',
'usercsspreview'                   => "'''Mınaw CSS mätinin tek qarap şığw ekenin umıtpañız, ol äli saqtalğan joq!'''",
'userjspreview'                    => "'''Mınaw JavaScript qatıswşı bağdarlamasın tekserw/qarap şığw ekenin umıtpañız, ol äli saqtalğan joq!'''",
'userinvalidcssjstitle'            => "'''Qulaqtandırw:''' Osı arada «$1» degen eş mäner joq.
Qatıswşınıñ .css jäne .js faýl atawı kişi äripppen jazılw tïisti ekenin umıtpañız, mısalğa {{ns:user}}:Foo/monobook.css degendi {{ns:user}}:Foo/Monobook.css degenmen salıstırıp qarañız.",
'updated'                          => '(Jañartılğan)',
'note'                             => '<strong>Añğartpa:</strong>',
'previewnote'                      => '<strong>Mınaw tek qarap şığw ekenin umıtpañız;
özgerister äli saqtalğan joq!</strong>',
'previewconflict'                  => 'Bul qarap şığw beti joğarğı kiristirw ornındağı mätindi qamtïdı da jäne saqtalğandağı öñdi körsetpek.',
'session_fail_preview'             => '<strong>Ğafw etiñiz! Sessïya derekteri joğalwı saldarınan öñdemeñizdi bitire almaýmız.
Qaýta baýqap köriñiz. Eger bul äli istelmese, şığwdı jäne qaýta kirwdi baýqap köriñiz.</strong>',
'session_fail_preview_html'        => "<strong>Ğafw etiñiz! Sessïya derekteri joğalwı saldarınan öñdemeñizdi bitire almaýmız.</strong>

''{{SITENAME}} jobasında qam HTML qosılğan, JavaScript şabwıldardan qorğanw üşin aldın ala qarap şığw jasırılğan.''

<strong>Eger bul öñdeme adal talap bolsa, qaýta baýqap köriñiz. Eger bul äli istemese, şığwdı jäne qaýta kirwdi baýqap köriñiz.</strong>",
'token_suffix_mismatch'            => '<strong>Öñdemeñiz taýdırıldı, sebebi tutınğışıñız öñdeme derekter bwmasındağı tınıs belgilerin büldirtti.
Bet mätini bülinbew üşin öñdemeñiz taýdırıladı.
Bul keý waqıtta qatesi tolğan veb-negizinde tirkelwi joq proksï-serverdi paýdalanğan bolwı mümkin.</strong>',
'editing'                          => 'Öñdelwde: $1',
'editingsection'                   => 'Öñdelwde: $1 (bölimi)',
'editingcomment'                   => 'Öñdelwde: $1 (mändemesi)',
'editconflict'                     => 'Öñdeme qaqtığısı: $1',
'explainconflict'                  => "Osı betti siz öñdeý bastağanda basqa birew betti özgertken.
Joğarğı kiristirw ornında bettiñ ağımdıq mätini bar.
Tömengi kiristirw ornında siz özgertken mätini körsetiledi.
Özgertwiñizdi ağımdıq mätinge üstewiñiz jön.
«Betti saqta! batırmasın basqanda '''tek''' joğarğı kiristirw ornındağı mätin saqtaladı.",
'yourtext'                         => 'Mätiniñiz',
'storedversion'                    => 'Saqtalğan nusqası',
'nonunicodebrowser'                => '<strong>QULAQTANDIRW: Şolğışıñız Unicode belgilewine üýlesimdi emes, sondıqtan latın emes äripteri bar betterdi öñdew zil bolw mümkin.
Jumıs istewge ıqtïmaldıq berw üşin, tömendegi kiristirw ornında ASCII emes tañbalar onaltılıq kodımen körsetiledi</strong>.',
'editingold'                       => '<strong>QULAQTANDIRW: Osı bettiñ erterek tüzetwin öñdep jatırsız.
Bunı saqtasañız, osı tüzetwden keýingi barlıq özgerister joýıladı.</strong>',
'yourdiff'                         => 'Aýırmalar',
'copyrightwarning'                 => 'Añğartpa: {{SITENAME}} jobasına berilgen barlıq ülester $2 (köbirek aqparat üşin: $1) qujatına saý dep sanaladı.
Eger jazwıñızdıñ erkin öñdelwin jäne aqısız köpşilikke taratwın qalamasañız, mında jarïyalamawıñız jön.<br />
Tağı da, bul mağlumat öziñiz jazğanıñızğa, ne qoğam qazınasınan nemese sondaý aşıq qorlardan köşirilgenine bizge wäde beresiz.
<strong>AWTORLIQ QUQIQPEN QORĞAWLI MAĞLUMATTI RUQSATSIZ JARÏYALAMAÑIZ!</strong>',
'copyrightwarning2'                => 'Añğartpa: {{SITENAME}} jobasına berilgen barlıq ülesterdi basqa üleskerler öñdewge, özgertwge, ne alastawğa mümkin.
Eger jazwıñızdıñ erkin öñdelwin qalamasañız, mında jarïyalamawıñız jön.<br />
Tağı da, bul mağlumat öziñiz jazğanıñızğa, ne qoğam qazınasınan nemese sondaý aşıq qorlardan köşirilgenine bizge wäde beresiz (köbirek aqparat üşin $1 qwjatın qarañız).
<strong>AWTORLIQ QUQIQPEN QORĞAWLI MAĞLUMATTI RUQSATSIZ JARÏYALAMAÑIZ!</strong>',
'longpagewarning'                  => '<strong>QULAQTANDIRW: Bul bettiñ mölşeri — $1 KB;
keýbir şolğıştarda bet mölşeri 32 KB jetse ne onı assa öñdew kürdeli bolwı mümkin.
Betti birneşe kişkin bölimderge bölip köriñiz.</strong>',
'longpageerror'                    => '<strong>QATELİK: Jöneltpek mätiniñizdin mölşeri — $1 KB, eñ köbi $2 KB ruqsat etilgen mölşerinen asqan.
Bul saqtaý alınbaýdı.</strong>',
'readonlywarning'                  => '<strong>QULAQTANDIRW: Derekqor baptaw üşin qulıptalğan, sondıqtan däl qazir öñdemeñizdi saqtaý almaýsız.
Keýin qoldanw üşin mätändi qýıp alıp jäne qoýıp, mätin faýlına saqtawñızğa boladı.</strong>',
'protectedpagewarning'             => '<strong>QULAQTANDIRW: Bul bet qorğalğan. Tek äkimşi quqıqtarı bar qatıswşılar öñdeý aladı.</strong>',
'semiprotectedpagewarning'         => "'''Añğartpa:''' Bet jartılaý qorğalğan, sondıqtan osını tek tirkelgen qatıswşılar öñdeý aladı.",
'cascadeprotectedwarning'          => "'''Qulaqtandırw''': Bul bet qulıptalğan, endi tek äkimşi quqıqtarı bar qatıswşılar bunı öñdeý aladı.Bunıñ sebebi: bul bet «bawlı qorğawı» bar kelesi {{PLURAL:$1|bettiñ|betterdiñ}} kirikbeti:",
'titleprotectedwarning'            => '<strong>QULAQTANDIRW:  Bul bet qulıptalğan, sondıqtan tek birqatar qatıswşılar bunı bastaý aladı.</strong>',
'templatesused'                    => 'Bul bette qoldanılğan ülgiler:',
'templatesusedpreview'             => 'Bunı qarap şığwğa qoldanılğan ülgiler:',
'templatesusedsection'             => 'Bul bölimde qoldanılğan ülgiler:',
'template-protected'               => '(qorğalğan)',
'template-semiprotected'           => '(jartılaý qorğalğan)',
'hiddencategories'                 => 'Bul bet $1 jasırın sanattıñ müşesi:',
'nocreatetitle'                    => 'Betti bastaw şektelgen',
'nocreatetext'                     => '{{SITENAME}} jobasında jaña bet bastawı şektelgen.
Keri qaýtıp bar betti öñdewiñizge boladı, nemese [[{{#special:Userlogin}}|kirwiñizge ne tirkelwiñizge]] boladı.',
'nocreate-loggedin'                => '{{SITENAME}} jobasında jaña bet bastaw ruqsatıñız joq.',
'permissionserrors'                => 'Ruqsattar qateleri',
'permissionserrorstext'            => 'Bunı istewge ruqsatıñız joq, kelesi {{PLURAL:$1|sebep|sebepter}} boýınşa:',
'permissionserrorstext-withaction' => '$2 degenge ruqsatıñız joq, kelesi {{PLURAL:$1|sebep|sebepter}} boýınşa:',
'recreate-deleted-warn'            => "'''Qulaqtandırw: Aldında joýılğan betti qaýta bastaýın dep tursız.'''

Mına bet öñdewin jalğastırw üşin jarastığın tekserip şığwıñız jön.
Qolaýlı bolwı üşin bul bettiñ joyw jwrnalı keltirilgen:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Qulaqtandırw: Bul bette tım köp şığıs alatın qurılım taldatqış jeteleriniñ qoñıraw şalwları bar.

Bul $2 şamasınan kem bolwı jön, qazir osı arada $1.',
'expensive-parserfunction-category'       => 'Şığıs alatın qurılım taldatqış jeteleriniñ tım köp şaqırımı bar better',
'post-expand-template-inclusion-warning'  => 'Qulaqtandırw: Ülgi kiristirw mölşeri tım ülken.
Keýbir ülgiler kiristirilmeýdi.',
'post-expand-template-inclusion-category' => 'Ülgi kiristirilgen better mölşeri asıp ketti',
'post-expand-template-argument-warning'   => 'Qulaqtandırw: Bul bette tım köp ulğaýtılğan mölşeri bolğan eñ keminde bir ülgi däleli bar.
Bunıñ dälelderin qaldırıp ketken.',
'post-expand-template-argument-category'  => 'Ülgi dälelderin qaldırıp ketken better',

# "Undo" feature
'undo-success' => 'Bul öñdeme joqqa şığarılwı mümkin. Talabıñızdı quptap aldın ala tömendegi salıstırwdı tekserip şığıñız da, öñdemeni joqqa şığarwın bitirw üşin tömendegi özgeristerdi saqtañız.',
'undo-failure' => 'Bul öñdeme joqqa şığarılmaýdı, sebebi arada qaqtığıstı öñdemeler bar.',
'undo-norev'   => 'Bul öñdeme joqqa şığarılmaýdı, sebebi bul joq nemese joýılğan.',
'undo-summary' => '[[Special:Contributions/$2|$2]] ([[User_talk:$2|talqılawı]]) istegen nömir $1 nusqasın joqqa şığardı',

# Account creation failure
'cantcreateaccounttitle' => 'Jaña tirkelgi jasalmadı',
'cantcreateaccount-text' => "Bul IP jaýdan ('''$1''') jaña tirkelgi jasawın [[User:$3|$3]] buğattağan.

$3 keltirilgen sebebi: ''$2''",

# History pages
'viewpagelogs'        => 'Bul bet üşin jwrnal oqïğaların qaraw',
'nohistory'           => 'Mında bul bettiniñ tüzetw tarïxı joq.',
'revnotfound'         => 'Tüzetw tabılmadı',
'revnotfoundtext'     => 'Bul bettiñ suralğan eski tüzetwi tabılğan joq. Osı bet qatınawına paýdalanğan URL tekserip şığıñız.',
'currentrev'          => 'Ağımdıq tüzetw',
'revisionasof'        => '$1 kezindegi tüzetw',
'revision-info'       => '$1 kezindegi $2 istegen tüzetw',
'previousrevision'    => '← Eskilew tüzetwi',
'nextrevision'        => 'Jañalaw tüzetwi →',
'currentrevisionlink' => 'Ağımdıq tüzetwi',
'cur'                 => 'ağım.',
'next'                => 'kel.',
'last'                => 'soñ.',
'page_first'          => 'alğaşqısına',
'page_last'           => 'soñğısına',
'histlegend'          => 'Aýırmasın bölektew: salıstırmaq nusqalarınıñ qosw közderin belgilep <Enter> pernesin basıñız, nemese tömendegi batırmanı nuqıñız.<br />
Şarttı belgiler: (ağım.) = ağımdıq nusqamen aýırması,
(soñ.) = aldıñğı nusqamen aýırması, ş = şağın öñdeme',
'deletedrev'          => '[joýılğan]',
'histfirst'           => 'Eñ alğaşqısına',
'histlast'            => 'Eñ soñğısına',
'historysize'         => '($1 baýt)',
'historyempty'        => '(bos)',

# Revision feed
'history-feed-title'          => 'Tüzetw tarïxı',
'history-feed-description'    => 'Mına wïkïdegi bul bettiñ tüzetw tarïxı',
'history-feed-item-nocomment' => '$2 kezindegi $1 degen', # user at time
'history-feed-empty'          => 'Suratılğan bet joq boldı.
Ol mına wïkïden joýılğan, nemese atawı awıstırılğan.
Osığan qatıstı jaña betterdi [[{{#special:Search}}|bul wïkïden izdewdi]] baýqap köriñiz.',

# Revision deletion
'rev-deleted-comment'         => '(mändeme alastaldı)',
'rev-deleted-user'            => '(qatıswşı atı alastaldı)',
'rev-deleted-event'           => '(jwrnal jazbası alastaldı)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Bul bettiñ tüzetwi barşa murağattarınan alastalğan.
Mında [{{fullurl:{{#special:Log}}/delete|page={{FULLPAGENAMEE}}}} joyw jwrnalında] egjeý-tegjeý mälimetteri bolwı mümkin.</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Osı bettiñ tüzetwi barşa murağattarınan alastalğan.
{{SITENAME}} äkimşisi bop sonı köre alasız;
[{{fullurl:{{#special:Log}}/delete|page={{FULLPAGENAMEE}}}} joyw jwrnalında] egjeý-tegjeý mälmetteri bolwı mümkin.</div>',
'rev-delundel'                => 'körset/jasır',
'revisiondelete'              => 'Tüzetwlerdi joyw/joywdı boldırmaw',
'revdelete-nooldid-title'     => 'Nısana tüzetw jaramsız',
'revdelete-nooldid-text'      => 'Bul jeteni orındaw üşin nısana tüzetwin/tüzetwlerin keltirilmepsiz,
keltirilgen tüzetw joq, ne ağımdıq tüzetwdi jasırw üşin ärekettenip kördiñiz.',
'revdelete-selected'          => '[[:$1]] degenniñ bölektengen {{PLURAL:$2|tüzetwi|tüzetwleri}}:',
'logdelete-selected'          => 'Bölektengen {{PLURAL:$1|jwrnal oqïğası|jwrnal oqïğaları}}:',
'revdelete-text'              => 'Joýılğan tüzetwler men oqïğalardı äli de bet tarïxında jäne jwrnaldarda tabwğa boladı, biraq olardıñ mağlumat bölşekteri barşağa qatınalmaýdı.

{{SITENAME}} jobasınıñ basqa äkimşileri jasırın mağlumatqa qatınaý aladı, jäne qosımşa tïımdar qoýılğanşa deýin, osı tildesw arqılı joywdı boldırmawı mümkin.',
'revdelete-legend'            => 'Körinis tïımdarın qoyw:',
'revdelete-hide-text'         => 'Tüzetw mätinin jasır',
'revdelete-hide-name'         => 'Äreket pen nısanasın jasır',
'revdelete-hide-comment'      => 'Öñdeme mändemesin jasır',
'revdelete-hide-user'         => 'Öñdewşi atın (IP mekenjaýın) jasır',
'revdelete-hide-restricted'   => 'Osı tïımdardı äkimşilerge qoldanw jäne bul tildeswdi qulıptaw',
'revdelete-suppress'          => 'Derekterdi barşağa uqsas äkimşilerden de şettetw',
'revdelete-hide-image'        => 'Faýl mağlumatın jasır',
'revdelete-unsuppress'        => 'Qalpına keltirilgen tüzetwlerden tïımdardı alastaw',
'revdelete-log'               => 'Jwrnaldağı mändemesi:',
'revdelete-submit'            => 'Bölektengen tüzetwge qoldanw',
'revdelete-logentry'          => '[[$1]] degenniñ tüzetw körinisin özgertti',
'logdelete-logentry'          => '[[$1]] degenniñ oqïğa körinisin özgertti',
'revdelete-success'           => "'''Tüzetw körinisi sätti qoýıldı.'''",
'logdelete-success'           => "'''Jwrnal körinisi sätti qoýıldı.'''",
'revdel-restore'              => 'Körinisin özgertw',
'pagehist'                    => 'Bet tarïxı',
'deletedhist'                 => 'Joýılğan tarïxı',
'revdelete-content'           => 'mağlumat',
'revdelete-summary'           => 'öñdemeniñ qısqaşa mazmundaması',
'revdelete-uname'             => 'qatıswşı atı',
'revdelete-restricted'        => 'äkimşilerge tïımdar qoldadı',
'revdelete-unrestricted'      => 'äkimşilerden tïımdardı alastadı',
'revdelete-hid'               => '$1 jasırdı',
'revdelete-unhid'             => '$1 aştı',
'revdelete-log-message'       => '$2 tüzetw üşin $1',
'logdelete-log-message'       => '$2 oqïğa üşin $1',

# Suppression log
'suppressionlog'     => 'Şettetw jwrnalı',
'suppressionlogtext' => 'Tömendegi tizimde äkimşilerden jasırılğan mağlumatqa qatıstı joywlar men buğattawlar beriledi.
Ağımda ärekettegi tïım men buğattaw tizimi üşin [[{{#special:Ipblocklist}}|IP buğattaw tizimin]] qarañız.',

# History merging
'mergehistory'                     => 'Better tarïxın biriktirw',
'mergehistory-header'              => 'Bul bet tüzetwler tarïxın qaýnar bettiñ birewinen alıp jaña betke biriktirgizedi.
Osı özgeris bettiñ tarïxï jalğastırwşılığın qoştaýtınına köziñiz jetsin.',
'mergehistory-box'                 => 'Eki bettiñ tüzetwlerin biriktirw:',
'mergehistory-from'                => 'Qaýnar beti:',
'mergehistory-into'                => 'Nısana beti:',
'mergehistory-list'                => 'Biriktirletin tüzetw tarïxı',
'mergehistory-merge'               => '[[:$1]] degenniñ kelesi tüzetwleri [[:$2]] degenge biriktirilwi mümkin.
Biriktirwge tek engizilgen waqıtqa deýin jasalğan tüzetwlerdi aýırıp-qosqış bağandı qoldanıñız.
Añğartpa: bağıttaw siltemelerin qoldanğanda bul bağan qaýta qoýıladı.',
'mergehistory-go'                  => 'Biriktirletin tüzetwlerdi körset',
'mergehistory-submit'              => 'Tüzetwlerdi biriktirw',
'mergehistory-empty'               => 'Eş tüzetwler biriktirilmeýdi',
'mergehistory-success'             => '[[:$1]] degenniñ $3 tüzetwi [[:$2]] degenge sätti biriktirildi.',
'mergehistory-fail'                => 'Tarïx biriktirwin orındaw ïkemdi emes, bet pen waqıt baptalımdarın qaýta tekserip şığıñız.',
'mergehistory-no-source'           => '$1 degen qaýnar beti joq.',
'mergehistory-no-destination'      => '$1 degen nısana beti joq.',
'mergehistory-invalid-source'      => 'Qaýnar betinde jaramdı taqırıp atı bolwı jön.',
'mergehistory-invalid-destination' => 'Nısana betinde jaramdı taqırıp atı bolwı jön.',
'mergehistory-autocomment'         => '[[:$1]] degen [[:$2]] degenge biriktirildi',
'mergehistory-comment'             => '[[:$1]] degen [[:$2]] degenge biriktirildi: $3',

# Merge log
'mergelog'           => 'Biriktirw jwrnalı',
'pagemerge-logentry' => '[[$1]] degen [[$2]] degenge biriktirildi ($3 deýingi tüzetwleri)',
'revertmerge'        => 'Biriktirwdi boldırmaw',
'mergelogpagetext'   => 'Tömende bir bettiñ tarïxı özge betke biriktirw eñ soñğı tizimi keltiriledi.',

# Diffs
'history-title'           => '«$1» — tüzetw tarïxı',
'difference'              => '(Tüzetwler arasındağı aýırmaşılıq)',
'lineno'                  => 'Jol nömiri $1:',
'compareselectedversions' => 'Bölektengen nusqalardı salıstırw',
'editundo'                => 'joqqa şığarw',
'diff-multi'              => '(Aradağı $1 tüzetw körsetilmegen.)',

# Search results
'searchresults'             => 'İzdew nätïjeleri',
'searchresulttext'          => "{{SITENAME}} saytında izlew haqqında ko'birek mag'lıwmat alg'ın'ız kelse, [[{{Mediawiki:helppage}}|{{int:help}} betine]] o'tip qarap ko'rin'.",
'searchsubtitle'            => "İzdegeniñiz: '''[[:$1]]'''",
'searchsubtitleinvalid'     => "İzdegeniñiz: '''$1'''",
'noexactmatch'              => "'''Osı arada bettiñ «$1» taqırıp atı joq.'''
[[:$1|Bul betti bastaý]] alasız.",
'noexactmatch-nocreate'     => "'''Osı arada bettiñ «$1» taqırıp atı joq.'''",
'toomanymatches'            => 'Tım köp säýkes qaýtarıldı, özge suranımdı baýqap köriñiz',
'titlematches'              => 'Bet taqırıbın atı säýkes keledi',
'notitlematches'            => 'Eş bet taqırıbın atı säýkes emes',
'textmatches'               => 'Bet mätini säýkes keledi',
'notextmatches'             => 'Eş bet mätini säýkes emes',
'prevn'                     => 'aldıñğı $1',
'nextn'                     => 'kelesi $1',
'viewprevnext'              => 'Körsetilwi: ($1) ($2) ($3) jazba',
'search-result-size'        => '$1 ($2 söz)',
'search-result-score'       => 'Araqatınastılığı: $1 %',
'search-redirect'           => '(aýdağış $1)',
'search-section'            => '(bölim $1)',
'search-suggest'            => 'Bunı izdediñiz be: $1',
'search-interwiki-caption'  => 'Bawırlas jobalar',
'search-interwiki-default'  => '$1 nätïje:',
'search-interwiki-more'     => '(köbirek)',
'search-mwsuggest-enabled'  => 'usınımdarmen',
'search-mwsuggest-disabled' => 'usınımdarsız',
'search-relatedarticle'     => 'Qatıstı',
'mwsuggest-disable'         => 'AJAX usınımdarın öşir',
'searchrelated'             => 'qatıstı',
'searchall'                 => 'barlıq',
'showingresults'            => "Tömende nömir '''$2''' ornınan bastap barınşa '''$1''' nätïje körsetiledi.",
'showingresultsnum'         => "Tömende nömir '''$2''' ornınan bastap '''$3''' nätïje körsetiledi.",
'showingresultstotal'       => "Tömende {{PLURAL:$3|'''$3''' arasınan '''$1''' nätïje körsetiledi|'''$3''' arasınan '''$1 — $2''' nätïje awqımı körsetiledi}}",
'nonefound'                 => "'''Esletpe''': Defolt boyınsha tek g'ana sheklengen isimler ko'pliginen izlenedi. Barlıq mag'lıwmat tu'rin (sonın' ishinde sa'wbet betlerdi, shablonlardı h.t.b.) izlew ushın izlewin'izdi ''barlıq:'' prefiksi menen baslan', yamasa qa'legen isimler ko'pligin prefiks esabında qollanın'.",
'powersearch'               => 'Keñeýtilgen izdew',
'powersearch-legend'        => 'Keñeýtilgen izdew',
'powersearch-ns'            => "Usı isimler ko'pliginen izlew:",
'powersearch-redir'         => "Qayta bag'ıtlawshı betlerdi ko'rset",
'powersearch-field'         => "İzlenetug'ın so'z (yamasa so'z dizbegi):",
'search-external'           => 'Şettik izdegiş',
'searchdisabled'            => '{{SITENAME}} izdew qızmeti öşirilgen.
Äzirşe Google arqılı izdewge boladı.
Añğartpa: {{SITENAME}} torabınıñ mağlumat tizbeleri eskirgen bolwı mümkin.',

# Preferences page
'preferences'              => 'Baptalımdar',
'mypreferences'            => 'Baptalımdarım',
'prefs-edits'              => 'Öñdeme sanı:',
'prefsnologin'             => 'Kirmegensiz',
'prefsnologintext'         => 'Baptawıñızdı qoyw üşin [[Special:UserLogin|kirwiñiz]] tïisti.',
'prefsreset'               => 'Baptalımdar arqawdan qaýta qoýıldı.',
'qbsettings'               => 'Mäzir',
'qbsettings-none'          => 'Eşqandaý',
'qbsettings-fixedleft'     => 'Solğa bekitilgen',
'qbsettings-fixedright'    => 'Oñğa bekitilgen',
'qbsettings-floatingleft'  => 'Solğa qalqığan',
'qbsettings-floatingright' => 'Oñğa qalqığan',
'changepassword'           => 'Qupïya sözdi özgertw',
'skin'                     => 'Mänerler',
'math'                     => 'Örnekter',
'dateformat'               => 'Kün-aý pişimi',
'datedefault'              => 'Eş qalawsız',
'datetime'                 => 'Waqıt',
'math_failure'             => 'Qurılımın taldatwı sätsiz bitti',
'math_unknown_error'       => 'belgisiz qate',
'math_unknown_function'    => 'belgisiz jete',
'math_lexing_error'        => 'söz keniniñ qatesi',
'math_syntax_error'        => 'söýlem jüýesiniñ qatesi',
'math_image_error'         => 'PNG awdarısı sätsiz bitti;
latex, dvips, gs jäne convert bağdarlamalarınıñ durıs ornatwın tekserip şığıñız',
'math_bad_tmpdir'          => 'math degen waqıtşa qaltasına jazılmadı, ne qalta qurılmadı',
'math_bad_output'          => 'math degen beris qaltasına jazılmadı, ne qalta qurılmadı',
'math_notexvc'             => 'texvc atqarılmalısı tabılmadı;
baptaw üşin math/README qujatın qarañız.',
'prefs-personal'           => 'Jeke derekteri',
'prefs-rc'                 => 'Jwıqtağı özgerister',
'prefs-watchlist'          => 'Baqılaw',
'prefs-watchlist-days'     => 'Baqılaw tizimindegi künderdiñ körsetpek sanı:',
'prefs-watchlist-edits'    => 'Keñeýtilgen baqılawlardağı özgeristerdiñ barınşa körsetpek sanı:',
'prefs-misc'               => 'Ärqïlı',
'saveprefs'                => 'Saqta',
'resetprefs'               => 'Saqtalmağan özgeristerdi tazart',
'oldpassword'              => 'Ağımdıq qupïya söziñiz:',
'newpassword'              => 'Jaña qupïya söziñiz:',
'retypenew'                => 'Jaña qupïya söziñizdi qaýtalañız:',
'textboxsize'              => 'Öñdew',
'rows'                     => 'Joldar:',
'columns'                  => 'Bağandar:',
'searchresultshead'        => 'İzdew',
'resultsperpage'           => 'Bet saýın nätïje sanı:',
'contextlines'             => 'Nätïje saýın jol sanı:',
'contextchars'             => 'Jol saýın tañba sanı:',
'stub-threshold'           => '<a href="#" class="stub">Biteme siltemesin</a> pişimdew tabaldırığı (baýt):',
'recentchangesdays'        => 'Jüıqtağı özgeristerinde körsetpek kün sanı:',
'recentchangescount'       => 'Jwıqtağı özgeristerdinde, tarïx jäne jwrnal betterinde körsetpek öñdeme sanı:',
'savedprefs'               => 'Baptalımdarıñız saqtaldı.',
'timezonelegend'           => 'Waqıt beldewi',
'timezonetext'             => '¹ Jergilikti waqıtıñız ben server waqıtınıñ (UTC) arasındağı sağat sanı.',
'localtime'                => 'Jergilikti waqıt',
'timezoneoffset'           => 'Sağat ığıswı¹',
'servertime'               => 'Server waqıtı',
'guesstimezone'            => 'Şolğıştan alıp toltırw',
'allowemail'               => 'Basqadan xat qabıldawın qos',
'prefs-searchoptions'      => 'İzdew baptalımdarı',
'prefs-namespaces'         => 'Esim ayaları',
'defaultns'                => 'Mına esim ayalarda ädepkiden izdew:',
'default'                  => 'ädepki',
'files'                    => 'Faýldar',

# User rights
'userrights'                  => 'Qatıswşı quqıqtarın rettew', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Qatıswşı toptarın rettew',
'userrights-user-editname'    => 'Qatıswşı atın engiziñiz:',
'editusergroup'               => 'Qatıswşı toptarın öñdew',
'editinguser'                 => "Qatıswşı quqıqtarın özgertw: '''[[User:$1|$1]]''' ([[User_talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Qatıswşı toptarın öñdew',
'saveusergroups'              => 'Qatıswşı toptarın saqtaw',
'userrights-groupsmember'     => 'Müşeligi:',
'userrights-groups-help'      => 'Bul qatıswşı kiretin toptardı retteý alasız.
* Qusbelgi qoýılğan közi qatıswşı bul topqa kirgenin körsetedi;
* Qusbelgi alıp tastalğan köz qatıswşı bul topqa kirmegenin körsetedi;
* Keltirilgen * toptı bir üstegeninen keýin alastaý almaýtındığın, ne qarama-qarsısın körsetedi.',
'userrights-reason'           => 'Özgertw sebebi:',
'userrights-no-interwiki'     => 'Basqa wïkïlerdegi paýdalanwşı quqıqtarın öñdewge ruqsatıñız joq.',
'userrights-nodatabase'       => '$1 derekqorı joq ne jergilikti emes.',
'userrights-nologin'          => 'Qatıswşı quqıqtarın tağaýındaw üşin äkimşi tirkelgisimen [[{{#special:Userlogin}}|kirwiñiz]] jön.',
'userrights-notallowed'       => 'Qatıswşı quqıqtarın tağaýındaw üşin tirkelgiñizde ruqsat joq.',
'userrights-changeable-col'   => 'Özgerte alatın toptar',
'userrights-unchangeable-col' => 'Özgerte almaýtın toptar',

# Groups
'group'               => 'Top:',
'group-user'          => 'Qatıswşılar',
'group-autoconfirmed' => 'Özquptalğan qatıswşılar',
'group-bot'           => 'Bottar',
'group-sysop'         => 'Äkimşiler',
'group-bureaucrat'    => 'Bitikşiler',
'group-suppress'      => 'Şettetwşiler',
'group-all'           => '(barlıq)',

'group-user-member'          => 'qatıswşı',
'group-autoconfirmed-member' => 'özquptalğan qatıswşı',
'group-bot-member'           => 'bot',
'group-sysop-member'         => 'äkimşi',
'group-bureaucrat-member'    => 'bitikşi',
'group-suppress-member'      => 'şettetwşi',

'grouppage-user'          => '{{ns:project}}:Qatıswşılar',
'grouppage-autoconfirmed' => '{{ns:project}}:Özquptalğan qatıswşılar',
'grouppage-bot'           => '{{ns:project}}:Bottar',
'grouppage-sysop'         => '{{ns:project}}:Äkimşiler',
'grouppage-bureaucrat'    => '{{ns:project}}:Bitikşiler',
'grouppage-suppress'      => '{{ns:project}}:Şettetwşiler',

# Rights
'right-read'                 => 'Betterdi oqw',
'right-edit'                 => 'Betterdi öñdew',
'right-createpage'           => 'Talqılaw emes betterdi bastaw',
'right-createtalk'           => 'Talqılaw betterdi bastaw',
'right-createaccount'        => 'Jaña qatıswşı tirkelgisin jasaw',
'right-minoredit'            => 'Öñdemelerdi şağın dep belgilew',
'right-move'                 => 'Betterdi jıljıtw',
'right-move-subpages'        => 'Betterdi bulardıñ bağınıştı betterimen jıljıtw',
'right-suppressredirect'     => 'Tïisti atawğa betti jıljıtqanda aýdağıştı jasamaw',
'right-upload'               => 'Faýldardı qotarıp berw',
'right-reupload'             => 'Bar faýl üstine jazw',
'right-reupload-own'         => 'Özi qotarıp bergen faýl üstine jazw',
'right-reupload-shared'      => 'Taspa ortaq qoýmasındağı faýldardı jergiliktilermen asırw',
'right-upload_by_url'        => 'Faýldı URL mekenjaýınan qotarıp berw',
'right-purge'                => 'Betti torap bürkemesinen quptawsız tazartw',
'right-autoconfirmed'        => 'Jartılaý qorğalğan betterdi öñdew',
'right-bot'                  => 'Özdiktik üderis dep eseptelw',
'right-nominornewtalk'       => 'Talqılaw betterdegi şağın öñdemelerdi jaña xabar dep eseptemew',
'right-apihighlimits'        => 'API suranımdarınıñ joğarı şektelimderin paýdalanw',
'right-writeapi'             => 'API jazwın paýdalanw',
'right-delete'               => 'Betterdi joyw',
'right-bigdelete'            => 'Uzaq tarïxı bar betterdi joyw',
'right-deleterevision'       => 'Betterdiñ özindik tüzetwlerin joyw ne joywın boldırmaw',
'right-deletedhistory'       => 'Joýılğan tarïx danaların (baýlanıstı mätinsiz) körw',
'right-browsearchive'        => 'Joýılğan betterdi izdew',
'right-undelete'             => 'Bettiñ jywın boldırmaw',
'right-suppressrevision'     => 'Äkimşilerden jasırılğan tüzetwlerdi şolıp şığw jäne qalpına keltirw',
'right-suppressionlog'       => 'Jekelik jwrnaldardı körw',
'right-block'                => 'Basqa qatıswşılardı öñdewden buğattaw',
'right-blockemail'           => 'Qatıswşınıñ xat jöneltwin buğattaw',
'right-hideuser'             => 'Barşadan jasırıp, qatıswşı atın buğattaw',
'right-ipblock-exempt'       => 'IP buğattawlardı, özbuğattawlardı jäne awqım buğattawlardı orağıtw',
'right-proxyunbannable'      => 'Proksï serverlerdiñ özbuğattawların orağıtw',
'right-protect'              => 'Qorğaw deñgeýlerin özgertw jäne qorğalğan betterdi öñdew',
'right-editprotected'        => 'Qorğalğan betterdi öñdew (bawlı qorğawlarsız)',
'right-editinterface'        => 'Paýdalanwşılıq tildesiwin öñdew',
'right-editusercssjs'        => 'Basqa qatıswşılardıñ CSS jäne JS faýldarın öñdew',
'right-rollback'             => 'Belgili betti öñdegen soñğı qatıswşınıñ öñdemelerinen jıldam şegindirw',
'right-markbotedits'         => 'Şegindirlgen öñdemelerdi bottardiki dep belgilew',
'right-noratelimit'          => 'Eselik şektelimderi ıqpal etpeýdi',
'right-import'               => 'Basqa wïkïlerden betterdi sırttan alw',
'right-importupload'         => 'Faýl qotarıp berwimen betterdi sırttan alw',
'right-patrol'               => 'Basqarardıñ öñdemelerin zertteldi dep belgilew',
'right-autopatrol'           => 'Öz öñdemelerin zertteldi dep özdiktik belgilew',
'right-patrolmarks'          => 'Jwıqtağı özgeristerdegi zerttew belgilerin körw',
'right-unwatchedpages'       => 'Baqılanılmağan bet tizimin körw',
'right-trackback'            => 'Añıstawdı jöneltw',
'right-mergehistory'         => 'Betterdiñ tarïxın qosıp berw',
'right-userrights'           => 'Qatıswşılardıñ barlıq quqıqtarın öñdew',
'right-userrights-interwiki' => 'Basqa üïkïlerdegi qatıswşılardıñ quqıqtarın öñdew',
'right-siteadmin'            => 'Derekqordı qulıptaw jäne qulıptawın öşirw',

# User rights log
'rightslog'      => 'Qatıswşı quqıqtarı jwrnalı',
'rightslogtext'  => 'Bul qatıswşı quqıqtarın özgertw jwrnalı.',
'rightslogentry' => '$1 kirgen toptarın $2 degennen $3 degenge özgertti',
'rightsnone'     => '(eşqandaý)',

# Recent changes
'nchanges'                          => '$1 özgeris',
'recentchanges'                     => 'Jwıqtağı özgerister',
'recentchangestext'                 => 'Bul bette osı wïkïdegi bolğan jwıqtağı özgerister baýqaladı.',
'recentchanges-feed-description'    => 'Bul arnamenen wïkïdegi eñ soñğı özgerister qadağalanadı.',
'rcnote'                            => "$3 kezine deýin — tömende soñğı {{PLURAL:$2|kündegi|'''$2''' kündegi}}, soñğı '''$1''' özgeris körsetiledi.",
'rcnotefrom'                        => "'''$2''' kezinen beri — tömende '''$1''' jetkenşe deýin özgerister körsetiledi.",
'rclistfrom'                        => '$1 kezinen beri — jaña özgeristerdi körset.',
'rcshowhideminor'                   => 'Şağın öñdemelerdi $1',
'rcshowhidebots'                    => 'Bottardı $1',
'rcshowhideliu'                     => 'Kirgenderdi $1',
'rcshowhideanons'                   => 'Tirkelgisizderdi $1',
'rcshowhidepatr'                    => 'Zerttelgen öñdemelerdi $1',
'rcshowhidemine'                    => 'Öñdemelerimdi $1',
'rclinks'                           => 'Soñğı $2 künde bolğan, soñğı $1 özgeristi körset<br />$3',
'diff'                              => 'aýırm.',
'hist'                              => 'tar.',
'hide'                              => 'jasır',
'show'                              => 'körset',
'minoreditletter'                   => 'ş',
'newpageletter'                     => 'J',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[baqılağan $1 qatıswşı]',
'rc_categories'                     => 'Sanattarğa şektew ("|" belgisimen bölikteñiz)',
'rc_categories_any'                 => 'Qaýsıbir',
'newsectionsummary'                 => '/* $1 */ jaña bölim',

# Recent changes linked
'recentchangeslinked'          => 'Qatıstı özgerister',
'recentchangeslinked-title'    => '«$1» degenge qatıstı özgerister',
'recentchangeslinked-noresult' => 'Siltegen betterde keltirilgen merzimde eşqandaý özgeris bolmağan.',
'recentchangeslinked-summary'  => "Bul tizimde özindik betke siltegen betterdegi (ne özindik sanat müşelerindegi) istelgen jwıqtağı özgerister beriledi.
[[{{#special:Watchlist}}|Baqılaw tizimiñizdegi]] better '''jwan''' bolıp belgilenedi.",
'recentchangeslinked-page'     => 'Bet atawı:',
'recentchangeslinked-to'       => 'Kerisinşe, keltirilgen betke silteýtin betterdegi özgeristerdi körset',

# Upload
'upload'                      => 'Qotarıp berw',
'uploadbtn'                   => 'Qotarıp ber!',
'reupload'                    => 'Qaýta qotarıp berw',
'reuploaddesc'                => 'Qotarıp berwdi boldırmaw jäne qotarw pişinine qaýta kelw.',
'uploadnologin'               => 'Kirmegensiz',
'uploadnologintext'           => 'Faýl qotarw üşin [[Special:UserLogin|kirwiñiz]] kerek.',
'upload_directory_missing'    => 'Qotarıp bermek qaltası ($1) jetispeýdi jäne veb-server jarata almaýdı.',
'upload_directory_read_only'  => 'Qotarıp bermek qaltasına ($1) veb-server jaza almaýdı.',
'uploaderror'                 => 'Qotarıp berw qatesi',
'uploadtext'                  => "Tömendegi pişindi faýldardı qotarıp berw üşin qoldanıñız. 
Aldında qotarılıp berilgen faýldardı qaraw ne izdew üşin [[{{#special:Imagelist}}|qotarıp berilgen faýldar tizimine]] barıñız, tağı da qotarıp berwi men joywı  [[{{#special:Log}}/upload|qotarıp berw jwrnalına]] jazılıp alınadı.

Swretti betke kiristirwge, faýlğa twra siltew üşin mına pişindegi siltemeni qoldanıñız:
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|balama mätin]]</nowiki>''' ne
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki>'''.",
'upload-permitted'            => 'Ruqsat etilgen faýl türleri: $1.',
'upload-preferred'            => 'Unamdı faýl türleri $1.',
'upload-prohibited'           => 'Ruqsat etilmegen faýl türleri: $1.',
'uploadlog'                   => 'qotarıp berw jwrnalı',
'uploadlogpage'               => 'Qotarıp berw jwrnalı',
'uploadlogpagetext'           => 'Tömende eñ soñğı qotarıp berilgen faýl tizimi.',
'filename'                    => 'Faýl atawı',
'filedesc'                    => 'Tüýindemesi',
'fileuploadsummary'           => 'Tüýindemesi:',
'filestatus'                  => 'Awtorlıq quqıqtar küýi:',
'filesource'                  => 'Qaýnar közi:',
'uploadedfiles'               => 'Qotarıp berilgen faýldar',
'ignorewarning'               => 'Qulaqtandırwğa eleme de faýldı qalaýda saqta.',
'ignorewarnings'              => 'Kez kelgen qulaqtandırwlarğa eleme',
'minlength1'                  => 'Faýl atawında eñ keminde bir ärip bolwı jön.',
'illegalfilename'             => '«$1» faýl atawında bet taqırıbı atında ruqsat berilmegen tañbalar bar.
Faýldı qaýta atañız da bunı qotarıp berwdi qaýta baýqap köriñiz.',
'badfilename'                 => 'Faýldıñ atawı «$1» dep özgertildi.',
'filetype-badmime'            => '«$1» degen MIME türi bar faýldardı qotarıp berwge ruqsat etilmeýdi.',
'filetype-unwanted-type'      => "'''«.$1»''' — kütilmegen faýl türi. Unamdı faýl türleri: $2.",
'filetype-banned-type'        => "'''«.$1»''' — ruqsattalmağan faýl türi. Ruqsattalğan faýl türleri: $2.",
'filetype-missing'            => 'Bul faýldıñ («.jpg» sïyaqtı) keñeýtimi joq.',
'large-file'                  => 'Faýldıñ $1 mölşerinen aspawına kepildeme beriledi;
bul faýl mölşeri — $2.',
'largefileserver'             => 'Osı faýldıñ mölşeri serverdiñ qalawınan asıp ketken.',
'emptyfile'                   => 'Qotarıp berilgen faýlıñız bos sïyaqtı. Faýl atawı qate jazılğan mümkin.
Bul faýldı qotarıp berwi naqtı talabıñız ekenin tekserip şığıñız.',
'fileexists'                  => 'Bılaý atalğan faýl aldaqaşan bar, eger bunı özgertwge batılıñız joq bolsa <strong><tt>$1</tt></strong> degendi tekserip şığıñız.',
'filepageexists'              => 'Bul faýldıñ sïpattama beti aldaqaşan <strong><tt>$1</tt></strong> degende jasalğan, biraq ağımda bılaý atalğan eş faýl joq.
Engizgen qısqaşa mazmundamañız sïpattaması betinde körsetilmeýdi.
Qısqaşa mazmundamañız osı arada körsetilw üşin, bunı qolmen öñdemek bolıñız',
'fileexists-extension'        => 'Uqsas atawı bar faýl tabıldı:<br />
Qotarıp beriletin faýl atawı: <strong><tt>$1</tt></strong><br />
Bar bolğan faýl atawı: <strong><tt>$2</tt></strong><br />
Özge atawdı tañdañız.',
'fileexists-thumb'            => "<center>'''Bar bolğan swret'''</center>",
'fileexists-thumbnail-yes'    => 'Osı faýl — mölşeri kişiritilgen swret <i>(nobaý)</i> sïyaqtı.
Bul <strong><tt>$1</tt></strong> degen faýldı sınap şığıñız.<br />
Eger sınalğan faýl tüpnusqalı mölşeri bar dälme-däl swret bolsa, qosısmşa nobaýdı qotarıp berw keregi joq.',
'file-thumbnail-no'           => 'Faýl atawı <strong><tt>$1</tt></strong> degenmen bastaladı.
Bul — mölşeri kişiritilgen swret <i>(nobaý)</i> sïyaqtı.
Eger bul swrettiñ tolıq ajıratılımdığı bolsa, bunı qotarıp beriñiz, äýtpese faýl atawın özgertiñiz.',
'fileexists-forbidden'        => 'Osılaý atalğan faýl aldaqaşan bar;
keri qaýtıñız da, osı faýldı jaña atımen qotarıp beriñiz. [[{{ns:image}}:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Osılaý atalğan faýl ortaq qoýmada aldaqaşan bar;
keri qaýtıñız da, osı faýldı jaña atımen qotarıp beriñiz. [[{{ns:image}}:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Bul faýl kelesi {{PLURAL:$1|faýldıñ|faýldarınıñ}} telnusqası:',
'successfulupload'            => 'Sätti qotarıp berildi',
'uploadwarning'               => 'Qotarıp berw jöninde qulaqtandırw',
'savefile'                    => 'Faýldı saqtaw',
'uploadedimage'               => '«[[$1]]» faýlın qotarıp berdi',
'overwroteimage'              => '«[[$1]]» faýlınnıñ jaña nusqasın qotarıp berdi',
'uploaddisabled'              => 'Qotarıp berw öşirilgen',
'uploaddisabledtext'          => '{{SITENAME}} jobasında faýl qotarıp berwi öşirilgen.',
'uploadscripted'              => 'Bul faýlda veb şolğıştı qatelikpen taldatqızatın HTML ne ämir kodı bar.',
'uploadcorrupt'               => 'Bul faýl büldirilgen, ne burıs keñeýtimi bar.
Faýldı tekserip şığıñız da, qaýta qotarıp beriñiz.',
'uploadvirus'                 => 'Bul faýlda vïrws bar! Egjeý-tegjeýleri: $1',
'sourcefilename'              => 'Qaýnar faýl atawı:',
'destfilename'                => 'Nısana faýl atawı:',
'upload-maxfilesize'          => 'Faýldıñ eñ köp mümkin mölşeri: $1',
'watchthisupload'             => 'Bul betti baqılaw',
'filewasdeleted'              => 'Bul atawı bar faýl burın qotarıp berilgen de beri kele joýılğan.
Bunı qaýta qotarıp berw aldınan $1 degendi tekserip şığıñız.',
'upload-wasdeleted'           => "'''Qulaqtandırw: Aldında joýılğan faýldı qotarıp bermeksiz.'''

Bul faýldı qotarıp berwin jalğastırw üşin bunıñ ıñğaýlığın tekserip şığwıñız jön.
Qolaýlı bolwı üşin bul faýldıñ joyw jwrnalı keltirilgen:",
'filename-bad-prefix'         => 'Qotarıp bermek faýlıñızdıñ atawı <strong>«$1» </strong> dep bastaladı, mınadaý sïpattawsız atawdı ädette sandıq kameralar özdiktik beredi.
Faýlıñızğa sïpattılaw atawdı tañdañız.',

'upload-proto-error'      => 'Burıs xattama',
'upload-proto-error-text' => 'Şetten qotarıp berw üşin URL jaýları <code>http://</code> nemese <code>ftp://</code> degenderden bastalw jön.',
'upload-file-error'       => 'İşki qate',
'upload-file-error-text'  => 'Serverde waqıtşa faýl qurılwı işki qatesine uşırastı.
Bul jüýeniñ äkimşimen qatınasıñız.',
'upload-misc-error'       => 'Qotarıp berw kezindegi belgisiz qate',
'upload-misc-error-text'  => 'Qotarıp berw kezinde belgisiz qatege uşırastı.
URL jaramdı jäne qatınawlı ekenin tekserip şığıñız da qaýta baýqap köriñiz.
Eger bul mäsele älde de qalsa, jüýe äkimşimen qatınasıñız.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL jetilmedi',
'upload-curl-error6-text'  => 'Keltirilgen URL jetilmedi.
URL durıs ekendigin jäne torap istep turğanın qos tekseriñiz.',
'upload-curl-error28'      => 'Qotarıp berw waqıtı bitti',
'upload-curl-error28-text' => 'Toraptıñ jawap berwi tım uzaq waqıtqa sozıldı.
Bul torap iste ekenin tekserip şığıñız, azğana kidire turıñız da qaýta baýqap köriñiz.
Talabıñızdı qol tïgen kezinde qaýta baýqap körwiñiz mümkin.',

'license'            => 'Lïcenzïyalandırwı:',
'nolicense'          => 'Eşteñe bölektenbegen',
'license-nopreview'  => '(Qarap şığw jetimdi emes)',
'upload_source_url'  => ' (jaramdı, barşağa qatınawlı URL)',
'upload_source_file' => ' (komp′ywteriñizdegi faýl)',

# Special:ImageList
'imagelist-summary'     => 'Bul arnaýı bette barlıq qotarıp berilgen faýldar körsetiledi.
Soñğı qotarıp berilgen faýldar tizimde joğarğı şetimen ädepkiden körsetiledi.
Bağannıñ bas jolın nuqığanda surıptawdıñ rettewi özgertiledi.',
'imagelist_search_for'  => 'Taspa atawın izdew:',
'imgfile'               => 'faýl',
'imagelist'             => 'Faýl tizimi',
'imagelist_date'        => 'Kün-aýı',
'imagelist_name'        => 'Atawı',
'imagelist_user'        => 'Qatıswşı',
'imagelist_size'        => 'Mölşeri',
'imagelist_description' => 'Sïpattaması',

# Image description page
'filehist'                       => 'Faýl tarïxı',
'filehist-help'                  => 'Faýldıñ qaý waqıtta qalaý körinetin üşin Kün-aý/Waqıt degendi nuqıñız.',
'filehist-deleteall'             => 'barlığın joý',
'filehist-deleteone'             => 'joý',
'filehist-revert'                => 'qaýtar',
'filehist-current'               => 'ağımdağı',
'filehist-datetime'              => 'Kün-aý/Waqıt',
'filehist-user'                  => 'Qatıswşı',
'filehist-dimensions'            => 'Ölşemderi',
'filehist-filesize'              => 'Faýl mölşeri',
'filehist-comment'               => 'Mändemesi',
'imagelinks'                     => 'Siltemeler',
'linkstoimage'                   => 'Bul faýlğa kelesi {{PLURAL:$1|bet|$1 bet}} silteýdi:',
'nolinkstoimage'                 => 'Bul faýlğa eş bet siltemeýdi.',
'morelinkstoimage'               => 'Bul faýldıñ [[{{#special:Whatlinkshere}}/$1|köbirek siltemelerin]] qaraw.',
'redirectstofile'                => 'Kelesi {{PLURAL:$1|faýl|$1 faýl}} bul faýlğa aýdaýdı:',
'duplicatesoffile'               => 'Kelesi {{PLURAL:$1|faýl bul faýldıñ telnusqası|$1 faýl bul faýldıñ telnusqaları}}:',
'sharedupload'                   => 'Bul faýl ortaq qoýmağa qotarıp berilgen sondıqtan basqa jobalarda qoldanwı mümkin.',
'shareduploadwiki'               => 'Bılaýğı aqparat üşin $1 degendi qarañız.',
'shareduploadwiki-desc'          => 'Bul $1 degen faýldıñ ortaq qoýmadağı mälimetteri tömende körsetiledi.',
'shareduploadwiki-linktext'      => 'faýldıñ sïpattama beti',
'shareduploadduplicate'          => 'Bul faýl ortaq qoýmadağı $1 faýlınıñ telnusqası.',
'shareduploadduplicate-linktext' => 'özge faýl',
'shareduploadconflict'           => 'Bul faýl atawı ortaq qoýmadağı $1 faýlımen däl keledi',
'shareduploadconflict-linktext'  => 'özge faýl',
'noimage'                        => 'Bılaý atalğan faýl joq, $1 mümkindigiñiz bar.',
'noimage-linktext'               => 'bunı qotarıp ber',
'uploadnewversion-linktext'      => 'Bul faýldıñ jaña nusqasın qotarıp berw',
'imagepage-searchdupe'           => 'Faýl telnusqaların izdew',

# File reversion
'filerevert'                => '$1 degendi qaýtarw',
'filerevert-legend'         => 'Faýldı qaýtarw',
'filerevert-intro'          => "'''[[Media:$1|$1]]''' degendi [$4 $3, $2 kezindegi nusqasına] qaýtarwdasız.",
'filerevert-comment'        => 'Mändemesi:',
'filerevert-defaultcomment' => '$2, $1 kezindegi nusqasına qaýtarıldı',
'filerevert-submit'         => 'Qaýtar',
'filerevert-success'        => "'''[[Media:$1|$1]]''' degen [$4 $3, $2 kezindegi nusqasına] qaýtarıldı.",
'filerevert-badversion'     => 'Keltirilgen waqıt belgisimen bul faýldıñ aldıñğı jergilikti nusqası joq.',

# File deletion
'filedelete'                  => '$1 degendi joyw',
'filedelete-legend'           => 'Faýldı joyw',
'filedelete-intro'            => "'''[[Media:$1|$1]]''' degendi joywdasız.",
'filedelete-intro-old'        => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' — [$4 $3, $2 kezindegi nusqasın] joywdasız.</span>',
'filedelete-comment'          => 'Joyw sebebi:',
'filedelete-submit'           => 'Joý',
'filedelete-success'          => "'''$1''' degen joýıldı.",
'filedelete-success-old'      => '<span class="plainlinks">\'\'\'[[{{ns:media}}:$1|$1]]\'\'\' — $3, $2 kezindegi nusqası joýıldı.</span>',
'filedelete-nofile'           => "'''$1''' degen {{SITENAME}} jobasında joq.",
'filedelete-nofile-old'       => "Keltirilgen anıqtawıştarımen '''$1''' degenniñ murağattalğan nusqası mında joq.",
'filedelete-iscurrent'        => 'Bul faýldıñ eñ soñğı nusqasın joyw talap etkensiz.
Eñ aldınan eskilew nusqasına qaýtarıñız.',
'filedelete-otherreason'      => 'Basqa/qosımşa sebep:',
'filedelete-reason-otherlist' => 'Basqa sebep',
'filedelete-reason-dropdown'  => '* Joywdıñ jalpı sebepteri
** Awtorlıq quqıqtarın buzw
** Faýl telnusqası',
'filedelete-edit-reasonlist'  => 'Joyw sebepterin öñdew',

# MIME search
'mimesearch'         => 'Faýldı MIME türimen izdew',
'mimesearch-summary' => 'Bul bette faýldardı MIME türimen süzgilewi qosılğan.
Kirisi: mağlumat_türi/tür_tarawı, mısalı <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME türi:',
'download'           => 'qotarıp alw',

# Unwatched pages
'unwatchedpages' => 'Baqılanılmağan better',

# List redirects
'listredirects' => 'Aýdatw bet tizimi',

# Unused templates
'unusedtemplates'     => 'Paýdalanılmağan ülgiler',
'unusedtemplatestext' => 'Bul bet basqa betke kirictirilmegen ülgi esim ayaısındağı barlıq betterdi tizimdeýdi.
Ülgilerdi joyw aldınan bunıñ özge siltemelerin tekserip şığwın umıtpañız',
'unusedtemplateswlh'  => 'basqa siltemeler',

# Random page
'randompage'         => 'Kezdeýsoq bet',
'randompage-nopages' => 'Bul esim ayasında better joq.',

# Random redirect
'randomredirect'         => 'Kezdeýsoq aýdağış',
'randomredirect-nopages' => 'Bul esim ayasında eş aýdağış joq.',

# Statistics
'statistics'             => 'Sanaq',
'sitestats'              => '{{SITENAME}} sanağı',
'userstats'              => 'Qatıswşı sanağı',
'sitestatstext'          => "Derekqorda {{PLURAL:$1|'''1'''|jalpı '''$1'''}} bet bar.
Buğan «talqılaw» betteri, {{SITENAME}} jobası twralı better, tım qısqa «biteme» betteri, aýdağıştar, tağı da basqa mağlumat dep tanılmaýtın better kiristirledi.
Solardı esepten şığarğanda, mında mağlumat {{PLURAL:$2|beti|betteri}} dep sanalatın '''$2''' bet bar dep boljanadı.

'''$8''' faýl qotarıp berildi.

{{SITENAME}} ornatılğannan beri better {{PLURAL:$3|'''1'''|jalpı '''$3'''}} ret qaralğan, jäne better '''$4''' ret öñdelgen.
Bunıñ nätïjesinde ortaşa eseppen ärbir betke '''$5''' öñdeme keledi, jäne ärbir öñdemege '''$6''' qaraw keledi.

[http://www.mediawiki.org/wiki/Manual:Job_queue Tapsırımalar kezeginiñ] uzındığı: '''$7'''.",
'userstatstext'          => "Mında '''$1''' [[{{#special:Listusers}}|tirkelgen qatıswşı]] bar, sonıñ işinde '''$2''' (ne '''$4 %''') qatıswşısında $5 quqıqtarı bar",
'statistics-mostpopular' => 'Eñ köp qaralğan better',

'disambiguations'      => 'Aýrıqtı better',
'disambiguationspage'  => '{{ns:template}}:Aýrıq',
'disambiguations-text' => "Kelesi better '''aýrıqtı betke''' silteýdi.
Bunıñ ornına belgili taqırıpqa siltewi kerek.<br />
Eger [[{{ns:mediawiki}}:Disambiguationspage]] tizimindegi ülgi qoldanılsa, bet aýrıqtı dep sanaladı.",

'doubleredirects'     => 'Şınjırlı aýdağıştar',
'doubleredirectstext' => 'Bul bette basqa aýdatw betterge silteýtin better tizimdelinedi. Ärbir jolaqta birinşi jäne ekinşi aýdağışqa siltemeler bar, sonımen birge ekinşi aýdağış nısanası bar, ädette bul birinşi aýdağış bağıttaýtın «naqtı» nısana bet atawı bolwı kerek.',

'brokenredirects'        => 'Eş betke keltirmeýtin aýdağıştar',
'brokenredirectstext'    => 'Kelesi aýdağıştar joq betterge silteýdi:',
'brokenredirects-edit'   => '(öñdew)',
'brokenredirects-delete' => '(joyw)',

'withoutinterwiki'         => 'Eş tilge siltemegen better',
'withoutinterwiki-summary' => 'Kelesi better basqa tilderge siltemeýdi',
'withoutinterwiki-legend'  => 'Bastalwı:',
'withoutinterwiki-submit'  => 'Körset',

'fewestrevisions' => 'Eñ az tüzetilgen better',

# Miscellaneous special pages
'nbytes'                  => '$1 baýt',
'ncategories'             => '$1 sanat',
'nlinks'                  => '$1 silteme',
'nmembers'                => '$1 müşe',
'nrevisions'              => '$1 tüzetw',
'nviews'                  => '$1 ret qaralğan',
'specialpage-empty'       => 'Bul bayanatqa eş nätïje joq.',
'lonelypages'             => 'Eş betten siltelmegen better',
'lonelypagestext'         => 'Kelesi betterge {{SITENAME}} jobasındağı basqa better siltemeýdi.',
'uncategorizedpages'      => 'Sanatsız better',
'uncategorizedcategories' => 'Sanatsız sanattar',
'uncategorizedimages'     => 'Sanatsız faýldar',
'uncategorizedtemplates'  => 'Sanatsız ülgiler',
'unusedcategories'        => 'Paýdalanılmağan sanattar',
'unusedimages'            => 'Paýdalanılmağan faýldar',
'popularpages'            => 'Eñ köp qaralğan better',
'wantedcategories'        => 'Bastalmağan sanattar',
'wantedpages'             => 'Bastalmağan better',
'missingfiles'            => 'Joq faýldar',
'mostlinked'              => 'Eñ köp siltengen better',
'mostlinkedcategories'    => 'Eñ köp paýdalanılğan sanattar',
'mostlinkedtemplates'     => 'Eñ köp paýdalanılğan ülgiler',
'mostcategories'          => 'Eñ köp sanatı bar better',
'mostimages'              => 'Eñ köp paýdalanılğan faýldar',
'mostrevisions'           => 'Eñ köp tüzetilgen better',
'prefixindex'             => 'Ataw bastawış tizimi',
'shortpages'              => 'Eñ qısqa better',
'longpages'               => 'Eñ uzın better',
'deadendpages'            => 'Eş betke siltemeýtin better',
'deadendpagestext'        => 'Kelesi better {{SITENAME}} jobasındağı basqa betterge siltemeýdi.',
'protectedpages'          => 'Qorğalğan better',
'protectedpages-indef'    => 'Tek belgisiz qorğawlar',
'protectedpagestext'      => 'Kelesi better öñdewden nemese jıljıtwdan qorğalğan',
'protectedpagesempty'     => 'Ağımda mınadaý baptalımdarımen eşbir bet qorğalmağan',
'protectedtitles'         => 'Qorğalğan taqırıp attarı',
'protectedtitlestext'     => 'Kelesi taqırıp attarın bastawğa ruqsat berilmegen',
'protectedtitlesempty'    => 'Bul baptalımdarmen ağımda eş taqırıp attarı qorğalmağan.',
'listusers'               => 'Qatıswşı tizimi',
'newpages'                => 'Eñ jaña better',
'newpages-username'       => 'Qatıswşı atı:',
'ancientpages'            => 'Eñ eski better',
'move'                    => 'Jıljıtw',
'movethispage'            => 'Betti jıljıtw',
'unusedimagestext'        => '<p>Añğartpa: Ğalamtordağı basqa toraptar faýlğa twra URL arqılı siltewi mümkin. Sondıqtan, belsendi paýdalanwına añğarmaý, osı tizimde qalwı mümkin.</p>',
'unusedcategoriestext'    => 'Kelesi sanat betteri bar bop tur, biraq oğan eş bet ne sanat kirmeýdi.',
'notargettitle'           => 'Nısana joq',
'notargettext'            => 'Osı jete orındalatın nısana betti, ne qatıswşını engizbepsiz.',
'nopagetitle'             => 'Mınadaý eş nısana bet joq',
'nopagetext'              => 'Keltirilgen nısana betiñiz joq.',
'pager-newer-n'           => 'jañalaw $1',
'pager-older-n'           => 'eskilew $1',
'suppress'                => 'Şettetw',

# Book sources
'booksources'               => 'Kitap qaýnarları',
'booksources-search-legend' => 'Kitap qaýnarların izdew',
'booksources-go'            => 'Ötw',
'booksources-text'          => 'Tömende jaña jäne qoldanğan kitaptar satatın toraptarınıñ siltemeleri tizimdelgen. Bul toraptarda izdelgen kitaptar twralı bılaýğı aqparat bolwğa mümkin.',

# Special:Log
'specialloguserlabel'  => 'Qatıswşı:',
'speciallogtitlelabel' => 'Taqırıp atı:',
'log'                  => 'Jwrnaldar',
'all-logs-page'        => 'Barlıq jwrnaldar',
'log-search-legend'    => 'Jwrnaldardan izdew',
'log-search-submit'    => 'Öt',
'alllogstext'          => '{{SITENAME}} jobasınıñ barlıq qatınawlı jwrnaldarın biriktirip körsetwi.
Jwrnal türin, qatıswşı atın, ne tïisti betin bölektep, tarıltıp qaraý alasız.',
'logempty'             => 'Jwrnalda säýkes danalar joq.',
'log-title-wildcard'   => 'Mına mätinneñ bastalıtın taqırıp attarın izdew',

# Special:AllPages
'allpages'          => 'Barlıq better',
'alphaindexline'    => '$1 — $2',
'nextpage'          => 'Kelesi betke ($1)',
'prevpage'          => 'Aldıñğı betke ($1)',
'allpagesfrom'      => 'Mına betten bastap körsetw:',
'allarticles'       => 'Barlıq bet tizimi',
'allinnamespace'    => 'Barlıq bet ($1 esim ayası)',
'allnotinnamespace' => 'Barlıq bet ($1 esim ayasınan tıs)',
'allpagesprev'      => 'Aldıñğığa',
'allpagesnext'      => 'Kelesige',
'allpagessubmit'    => 'Ötw',
'allpagesprefix'    => 'Mınadan bastalğan betterdi körsetw:',
'allpagesbadtitle'  => 'Keltirilgen bet taqırıbın atı jaramsız bolğan, nemese til-aralıq ne wïkï-aralıq bastawı bar boldı.
Mında taqırıp atında qoldalmaýtın birqatar tañbalar bolwı mümkin.',
'allpages-bad-ns'   => '{{SITENAME}} jobasında «$1» esim ayası joq.',

# Special:Categories
'categories'                    => 'Sanattar',
'categoriespagetext'            => 'Kelesi sanattar işinde better ne taspalar bar.',
'categoriesfrom'                => 'Sanattardı mınadan bastap körsetw:',
'special-categories-sort-count' => 'sanımen surıptaw',
'special-categories-sort-abc'   => 'älipbïmen surıptaw',

# Special:ListUsers
'listusersfrom'      => 'Mına qatıswşıdan bastap körsetw:',
'listusers-submit'   => 'Körset',
'listusers-noresult' => 'Qatıswşı tabılğan joq.',

# Special:ListGroupRights
'listgrouprights'          => 'Qatıswşı tobı quqıqtarı',
'listgrouprights-summary'  => 'Kelesi tizimde bul wïkïde tağaýındalğan qatıswşı quqıqtarı (baýlanıstı qatınaw quqıqtarımen birge) körsetiledi.
Jeke quqıqtar twralı köbirek aqparattı [[{{MediaWiki:Listgrouprights-helppage}}|mında]] taba alasız.',
'listgrouprights-group'    => 'Top',
'listgrouprights-rights'   => 'Quqıqtarı',
'listgrouprights-helppage' => '{{ns:help}}:Top quqıqtarı',
'listgrouprights-members'  => '(müşe tizimi)',

# E-mail user
'mailnologin'     => 'Eş mekenjaý jöneltilgen joq',
'mailnologintext' => 'Basqa qatıswşığa xat jöneltw üşin [[Special:UserLogin|kirwiñiz]] kerek, jäne [[Special:Preferences|baptawıñızda]] jaramdı e-poşta jaýı bolwı jön.',
'emailuser'       => 'Qatıswşığa xat jazw',
'emailpage'       => 'Qatıswşığa xat jazw',
'emailpagetext'   => 'Eger bul qatıswşı baptawlarında jaramdı e-poşta mekenjaýın engizse, tömendegi pişin arqılı buğan jalğız e-poşta xatın jöneltwge boladı.
Qatıswşı baptawıñızda engizgen e-poşta mekenjaýıñız «Kimnen» degen bas jolağında körinedi, sondıqtan xat alwşısı twra jawap bere aladı.',
'usermailererror' => 'Mail nısanı qate qaýtardı:',
'defemailsubject' => '{{SITENAME}} e-poştasınıñ xatı',
'noemailtitle'    => 'Eş e-poşta mekenjaýı joq',
'noemailtext'     => 'Bul qatıswşı jaramdı E-poşta mekenjaýın keltirmegen, ne basqalardan xat qabıldawın öşirgen.',
'emailfrom'       => 'Kimnen',
'emailto'         => 'Kimge',
'emailsubject'    => 'Taqırıbı',
'emailmessage'    => 'Xat',
'emailsend'       => 'Jöneltw',
'emailccme'       => 'Xatımdıñ köşirmesin mağan da jönelt.',
'emailccsubject'  => '$1 degenge xatıñızdıñ köşirmesi: $2',
'emailsent'       => 'Xat jöneltildi',
'emailsenttext'   => 'E-poşta xatıñız jöneltildi.',

# Watchlist
'watchlist'            => 'Baqılaw tizimi',
'mywatchlist'          => 'Baqılawım',
'watchlistfor'         => "('''$1''' baqılawları)",
'nowatchlist'          => 'Baqılaw tizimiñizde eş dana joq',
'watchlistanontext'    => 'Baqılaw tizimiñizdegi danalardı qaraw, ne öñdew üşin $1 kerek.',
'watchnologin'         => 'Kirmegensiz',
'watchnologintext'     => 'Baqılaw tizimiñizdi özgertw üşin [[Special:UserLogin|kirwiñiz]] jön.',
'addedwatch'           => 'Baqılaw tizimine üsteldi',
'addedwatchtext'       => "«[[:$1]]» beti [[{{#special:Watchlist}}|baqılaw tizimiñizge]] üsteldi.
Bul bettiñ jäne baýlanıstı talqılaw betiniñ keleşektegi özgeristeri mında tizimdelinedi de, jäne bettiñ atawı jeñil tabılw üşin [[{{#special:Recentchanges}}|jwıqtağı özgerister tiziminde]] '''jwan ärpimen''' körsetiledi.",
'removedwatch'         => 'Baqılaw tizimiñizden alastaldı',
'removedwatchtext'     => '«[[:$1]]» beti baqılaw tizimiñizden alastaldı.',
'watch'                => 'Baqılaw',
'watchthispage'        => 'Betti baqılaw',
'unwatch'              => 'Baqılamaw',
'unwatchthispage'      => 'Baqılawdı toqtatw',
'notanarticle'         => 'Mağlumat beti emes',
'notvisiblerev'        => 'Tüzetw joýıldı',
'watchnochange'        => 'Körsetilgen merzimde eş baqılanğan dana öñdelgen joq.',
'watchlist-details'    => 'Talqılaw betterin sanamağanda $1 bet baqlanıladı.',
'wlheader-enotif'      => '* Eskertw xat jiberwi qosılğan.',
'wlheader-showupdated' => "* Soñğı kelip-ketwiñizden beri özgertilgen betterdi '''jwan''' qaripimen körset",
'watchmethod-recent'   => 'baqılawlı better üşin jwıqtağı özgeristerdi tekserw',
'watchmethod-list'     => 'jwıqtağı özgerister üşin baqılawlı betterdi tekserw',
'watchlistcontains'    => 'Baqılaw tizimiñizde $1 bet bar.',
'iteminvalidname'      => "'$1' danada aqaw bar — jaramsız ataw…",
'wlnote'               => "Tömende soñğı {{PLURAL:$2|sağatta|'''$2''' sağatta}} bolğan, {{PLURAL:$1|jwıqtağı özgeris|jwıqtağı '''$1''' özgeris}} körsetiledi.",
'wlshowlast'           => 'Soñğı $1 sağattağı, $2 kündegi, $3 bolğan özgeristi körsetw',
'watchlist-show-bots'  => 'Bot öñdemelerin körset',
'watchlist-hide-bots'  => 'Bot öñdemelerin jasır',
'watchlist-show-own'   => 'Öñdemelerimdi körset',
'watchlist-hide-own'   => 'Öñdemelerimdi jasır',
'watchlist-show-minor' => 'Şağın öñdemelerdi körset',
'watchlist-hide-minor' => 'Şağın öñdemelerdi jasır',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Baqılawda…',
'unwatching' => 'Baqılamawda…',

'enotif_mailer'                => '{{SITENAME}} eskertw xat jiberw qızmeti',
'enotif_reset'                 => 'Barlıq bet kelip-ketildi dep belgile',
'enotif_newpagetext'           => 'Mınaw jaña bet.',
'enotif_impersonal_salutation' => '{{SITENAME}} qatıswşısı',
'changed'                      => 'özgertti',
'created'                      => 'bastadı',
'enotif_subject'               => '{{SITENAME}} jobasında $PAGEEDITOR $PAGETITLE atawlı betti $CHANGEDORCREATED',
'enotif_lastvisited'           => 'Soñğı kelip-ketwiñizden beri bolğan özgerister üşin $1 degendi qarañız.',
'enotif_lastdiff'              => 'Osı özgeris üşin $1 degendi qarañız.',
'enotif_anon_editor'           => 'tirkelgisiz qatıswşı $1',
'enotif_body'                  => 'Qadirli $WATCHINGUSERNAME,


{{SITENAME}} jobasınıñ $PAGETITLE atawlı betti $PAGEEDITDATE kezinde $PAGEEDITOR degen $CHANGEDORCREATED, ağımdıq nusqası üşin $PAGETITLE_URL qarañız.

$NEWPAGE

Öñdewşi keltirgen qısqaşa mazmundaması: $PAGESUMMARY $PAGEMINOREDIT

Öñdewşimen qatınasw:
e-poşta: $PAGEEDITOR_EMAIL
wïkï: $PAGEEDITOR_WIKI

Bılaýğı özgerister bolğanda da osı betke kelip-ketwiñizgenşe deýin eşqandaý basqa eskertw xattar jiberilmeýdi.
Sonımen qatar baqılaw tizimiñizdegi bet eskertpelik belgisin qaýta qoýıñız.

             Sizdiñ dostıq {{SITENAME}} jobasınıñ eskertw qızmeti

----
Baqılaw tizimiñizdiñ baptawlırın özgertw üşin, mında kelip-ketiñiz:
{{fullurl:{{#special:Watchlist}}/edit}}

Sın-pikir berw jäne bılaýğı järdem alw üşin:
{{fullurl:{{{{ns:mediawiki}}:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Betti joyw',
'confirm'                     => 'Quptaw',
'excontent'                   => "bolğan mağlumatı: '$1'",
'excontentauthor'             => "bolğan mağlumatı (tek '[[{{#special:Contributions}}/$2|$2]]' ülesi): '$1'",
'exbeforeblank'               => "tazartw aldındağı bolğan mağlumatı: '$1'",
'exblank'                     => 'bet bos boldı',
'delete-confirm'              => '«$1» degendi joyw',
'delete-legend'               => 'Joyw',
'historywarning'              => 'Qulaqtandırw: Joywı közdelgen bette tarïxı bar:',
'confirmdeletetext'           => 'Betti bükil tarïxımen birge derekqordan joýywın kozdediñiz.
Osını istew nïetiñizdi, saldarın baýımdawıñızdı jäne [[{{{{ns:mediawiki}}:Policy-url}}]] degenge laýıqtı dep istemektengeñizdi quptañız.',
'actioncomplete'              => 'Äreket bitti',
'deletedtext'                 => '«<nowiki>$1</nowiki>» joýıldı.
Jwıqtağı joywlar twralı jazbaların $2 degennen qarañız.',
'deletedarticle'              => '«[[$1]]» degendi joýdı',
'suppressedarticle'           => '«[[$1]]» degendi şettetti',
'dellogpage'                  => 'Joyw_jwrnalı',
'dellogpagetext'              => 'Tömende jwıqtağı joywlardıñ tizimi berilgen.',
'deletionlog'                 => 'joyw jwrnalı',
'reverted'                    => 'Erterek tüzetwine qaýtarılğan',
'deletecomment'               => 'Joywdıñ sebebi:',
'deleteotherreason'           => 'Basqa/qosımşa sebep:',
'deletereasonotherlist'       => 'Basqa sebep',
'deletereason-dropdown'       => '* Joywdıñ jalpı sebepteri
** Awtordıñ suranımı boýınşa
** Awtorlıq quqıqtarın buzw
** Buzaqılıq',
'delete-edit-reasonlist'      => 'Joyw sebepterin öñdew',
'delete-toobig'               => 'Bul bette baýtaq tüzetw tarïxı bar, $1 tüzetwden astam.
Bundaý betterdiñ joywı {{SITENAME}} torabın äldeqalaý üzip tastawına böget salw üşin tïımdalğan.',
'delete-warning-toobig'       => 'Bul bette baýtaq tüzetw tarïxı bar, $1 tüzetwden astam.
Bunıñ joywı {{SITENAME}} torabındağı derekqor äreketterdi üzip tastawın mümkin;
bunı abaýlap ötkiziñiz.',
'rollback'                    => 'Öñdemelerdi şegindirw',
'rollback_short'              => 'Şegindirw',
'rollbacklink'                => 'şegindirw',
'rollbackfailed'              => 'Şegindirw sätsiz bitti',
'cantrollback'                => 'Öñdeme qaýtarılmadı;
soñğı üleskeri tek osı bettiñ bastawşısı boldı.',
'alreadyrolled'               => '[[{{ns:user}}:$2|$2]] ([[{{ns:user_talk}}:$2|talqılawı]]) istegen [[:$1]] soñğı öñdemesi şegindirilmedi;
basqa birew bul betti aldaqaşan öñdegen ne şegindirgen.

soñğı öñdemesin [[{{ns:user}}:$3|$3]] ([[{{ns:user_talk}}:$3|talqılawı]]) istegen.',
'editcomment'                 => 'Bolğan öñdeme mändemesi: «<i>$1</i>».', # only shown if there is an edit comment
'revertpage'                  => '[[{{#special:Contributions}}/$2|$2]] ([[{{ns:user_talk}}:$2|talqılawı]]) öñdemelerinen [[{{ns:user}}:$1|$1]] soñğı nusqasına qaýtardı', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '$1 öñdemelerinen qaýtarğan;
$2 soñğı nusqasına özgertti.',
'sessionfailure'              => 'Kirw sessïyasında şataq bolğan sïyaqtı;
sessïyağa şabwıldawdardan qorğanw üşin, osı äreket toqtatıldı.
«Artqa» degendi basıñız, jäne betti qaýta jükteñiz de, qaýta baýqap köriñiz.',
'protectlogpage'              => 'Qorğaw jwrnalı',
'protectlogtext'              => 'Tömende betterdiñ qorğaw/qorğamaw tizimi berilgen.
Ağımdağı qorğaw ärektter bar better üşin [[{{#special:Protectedpages}}|qorğalğan bet tizimin]] qarañız.',
'protectedarticle'            => '«[[$1]]» qorğaldı',
'modifiedarticleprotection'   => '«[[$1]]» qorğalw deñgeýi özgerdi',
'unprotectedarticle'          => '«[[$1]]» qorğalwı öşirildi',
'protect-title'               => '«$1» qorğaw deñgeýin özgertw',
'protect-legend'              => 'Qorğawdı quptaw',
'protectcomment'              => 'Mändemesi:',
'protectexpiry'               => 'Merzimi bitpek:',
'protect_expiry_invalid'      => 'Bitetin waqıtı jaramsız.',
'protect_expiry_old'          => 'Bitetin waqıtı ötip ketken.',
'protect-unchain'             => 'Jıljıtw ruqsattarın berw',
'protect-text'                => '<strong><nowiki>$1</nowiki></strong> betiniñ qorğaw deñgeýin qarap jäne özgertip şığa alasız.',
'protect-locked-blocked'      => 'Buğattawıñız öşirilgenşe deýin qorğaw deñgeýin özgerte almaýsız.
Mına <strong>$1</strong> bettiñ ağımdıq baptawları:',
'protect-locked-dblock'       => 'Derekqordıñ qulıptawı belsendi bolğandıqtan qorğaw deñgeýleri özgertilmeýdi.
Mına <strong>$1</strong> bettiñ ağımdıq baptawları:',
'protect-locked-access'       => 'Tirkelgiñizge bet qorğaw dengeýlerin özgertwine ruqsat joq.
Mına <strong>$1</strong> bettiñ ağımdıq baptawları:',
'protect-cascadeon'           => 'Bul bet ağımda qorğalğan, sebebi osı bet «bawlı qorğawı» bar kelesi {{PLURAL:$1|bettiñ|betterdiñ}} kirikbeti.
Bul bettiñ qorğaw deñgeýin özgerte alasız, biraq bul bawlı qorğawğa ıqpal etpeýdi.',
'protect-default'             => '(ädepki)',
'protect-fallback'            => '«$1» ruqsatı kerek',
'protect-level-autoconfirmed' => 'Tirkelgisizderge tïım',
'protect-level-sysop'         => 'Tek äkimşiler',
'protect-summary-cascade'     => 'bawlı',
'protect-expiring'            => 'merzimi bitpek: $1 (UTC)',
'protect-cascade'             => 'Bul bettiñ kirikbetterin qorğaw (bawlı qorğaw).',
'protect-cantedit'            => 'Bul bettiñ qorğaw deñgeýin özgerte almaýsız, sebebi bunı öñdewge ruqstañız joq.',
'restriction-type'            => 'Ruqsatı:',
'restriction-level'           => 'Tïımdıq deñgeýi:',
'minimum-size'                => 'Eñ az mölşeri',
'maximum-size'                => 'Eñ köp mölşeri:',
'pagesize'                    => '(baýt)',

# Restrictions (nouns)
'restriction-edit'   => 'Öñdewge',
'restriction-move'   => 'Jıljıtwğa',
'restriction-create' => 'Bastawğa',
'restriction-upload' => 'Qotarıp berwge',

# Restriction levels
'restriction-level-sysop'         => 'tolıqtaý qorğalğan',
'restriction-level-autoconfirmed' => 'jartılaý qorğalğan',
'restriction-level-all'           => 'är deñgeýde',

# Undelete
'undelete'                     => 'Joýılğan betterdi qaraw',
'undeletepage'                 => 'Joýılğan betterdi qaraw jäne qalpına keltirw',
'undeletepagetitle'            => "'''Kelesi tizim [[:$1|$1]] degenniñ joýılğan tüzetwlerinen turadı'''.",
'viewdeletedpage'              => 'Joýılğan betterdi qaraw',
'undeletepagetext'             => 'Kelesi better joýıldı dep belgilengen, biraq mağlumatı murağatta bar
jäne qalpına keltirwge mümkin. Murağat merzim boýınşa tazalanıp turwı mümkin.',
'undeleteextrahelp'            => "Bükil betti qalpına keltirw üşin, barlıq qusbelgi közderdi bosatıp '''''Qalpına keltir!''''' batırmasın nuqıñız.
Bölektewmen qalpına keltirw orındaw üşin, keltiremin degen tüzetwlerine säýkes közderge qusbelgi salıñız da, jäne '''''Qalpına keltir!''''' tüýmesin nuqıñız. '''''Qaýta qoý''''' tüýmesin nuqığanda mändeme awmağı tazartadı jäne barlıq qusbelgi közderin bosatadı.",
'undeleterevisions'            => '$1 tüzetw murağattaldı',
'undeletehistory'              => 'Eger bet mağlumatın qalpına keltirseñiz, tarïxında barlıq tüzetwler da
qaýtarıladı. Eger joywdan soñ däl solaý atawımen jaña bet bastalsa, qalpına keltirilgen tüzetwler
tarïxtıñ aldında körsetiledi. Tağı da faýl tüzetwlerin qalpına keltirgende tïımdarı joýılatın eskeriñiz.',
'undeleterevdel'               => 'Eger bul üstiñgi bette ayaqtalsa, ne faýl tüzetwi jarım-jartılaý joýılğan bolsa, joyw boldırmawı orındalmaýdı.
Osındaý jağdaýlarda, eñ jaña joýılğan tüzetwin alıp tastawıñız ne jasırwın boldırmawıñız jön.',
'undeletehistorynoadmin'       => 'Bul bet joýılğan.
Joyw sebebi aldındağı öñdegen qatıswşılar egjeý-tegjeýlerimen birge tömendegi qısqaşa mazmundamasında körsetilgen.
Mına joýılğan tüzetwlerin kökeýkesti mätini tek äkimşilerge jetimdi.',
'undelete-revision'            => '$2 kezindegi $3 joýğan $1 degenniñ joýılğan tüzetwi:',
'undeleterevision-missing'     => 'Jaramsız ne joğalğan tüzetw.
Siltemeñiz jaramsız, ne tüzetw qalpına keltirilgen, nemese murağattan alastalğan bolwı mümkin.',
'undelete-nodiff'              => 'Eş aldıñğı tüzetw tabılmadı.',
'undeletebtn'                  => 'Qalpına keltir!',
'undeletelink'                 => 'qalpına keltirw',
'undeletereset'                => 'Qaýta qoý',
'undeletecomment'              => 'Mändemesi:',
'undeletedarticle'             => '«[[$1]]» qalpına keltirildi',
'undeletedrevisions'           => '$1 tüzetw qalpına keltirildi',
'undeletedrevisions-files'     => '$1 tüzetw jäne $2 faýl qalpına keltirildi',
'undeletedfiles'               => '$1 faýl qalpına keltirildi',
'cannotundelete'               => 'Joyw boldırmawı sätsiz bitti;
basqa birew alğaşında bettiñ joywdıñ boldırmawı mümkin.',
'undeletedpage'                => "<big>'''$1 qalpına keltirildi'''</big>

Jwıqtağı joywlar men qalpına keltirwler jöninde [[{{#special:Log}}/delete|joyw jwrnalın]] qarañız.",
'undelete-header'              => 'Jwıqtağı joýılğan better jöninde [[{{#special:Log}}/delete|joyw jwrnalın]] qarañız.',
'undelete-search-box'          => 'Joýılğan betterdi izdew',
'undelete-search-prefix'       => 'Mınadan bastalğan betterdi körset:',
'undelete-search-submit'       => 'İzdew',
'undelete-no-results'          => 'Joyw murağatında eşqandaý säýkes better tabılmadı.',
'undelete-filename-mismatch'   => '$1 kezindegi faýl tüzetwiniñ joywı boldırmadı: faýl atawı säýkessiz',
'undelete-bad-store-key'       => '$1 kezindegi faýl tüzetwiniñ joywı boldırmadı: joywdıñ aldınan faýl joq bolğan.',
'undelete-cleanup-error'       => '«$1» paýdalanılmağan murağattalğan faýl joyw qatesi.',
'undelete-missing-filearchive' => 'Murağattalğan faýl (nömiri $1) qalpına keltirwi ïkemdi emes, sebebi ol derekqorda joq.
Bunıñ joywın boldırmawı aldaqaşan bolğanı mümkin.',
'undelete-error-short'         => 'Faýl joywın boldırmaw qatesi: $1',
'undelete-error-long'          => 'Faýl joywın boldırmaw kezinde mına qateler kezdesti:

$1',

# Namespace form on various pages
'namespace'      => 'Esim ayası:',
'invert'         => 'Bölektewdi kerilew',
'blanknamespace' => '(Negizgi)',

# Contributions
'contributions' => 'Qatıswşı ülesi',
'mycontris'     => 'Ülesim',
'contribsub2'   => '$1 ($2) ülesi',
'nocontribs'    => 'Osı izdew şartına säýkes özgerister tabılğan joq.',
'uctop'         => ' (üsti)',
'month'         => 'Mına aýdan (jäne erterekten):',
'year'          => 'Mına jıldan (jäne erterekten):',

'sp-contributions-newbies'     => 'Tek jaña tirkelgiden jasağan ülesterdi körset',
'sp-contributions-newbies-sub' => 'Jañadan tirkelgi jasağandar üşin',
'sp-contributions-blocklog'    => 'Buğattaw jwrnalı',
'sp-contributions-search'      => 'Üles üşin izdew',
'sp-contributions-username'    => 'IP mekenjaýı ne qatıswşı atı:',
'sp-contributions-submit'      => 'İzde',

# What links here
'whatlinkshere'            => 'Mında silteýtin better',
'whatlinkshere-title'      => '$1 degenge silteýtin better',
'whatlinkshere-page'       => 'Bet:',
'linklistsub'              => '(Siltemeler tizimi)',
'linkshere'                => "'''[[:$1]]''' degenge mına better silteýdi:",
'nolinkshere'              => "'''[[:$1]]''' degenge eş bet siltemeýdi.",
'nolinkshere-ns'           => "Tañdalğan esim ayasında '''[[:$1]]''' degenge eşqandaý bet siltemeýdi.",
'isredirect'               => 'aýdatw beti',
'istemplate'               => 'kirikbet',
'isimage'                  => 'swret siltemesi',
'whatlinkshere-prev'       => '{{PLURAL:$1|aldıñğı|aldıñğı $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|kelesi|kelesi $1}}',
'whatlinkshere-links'      => '← siltemeler',
'whatlinkshere-hideredirs' => 'aýdağıştardı $1',
'whatlinkshere-hidetrans'  => 'kirikbetterdi $1',
'whatlinkshere-hidelinks'  => 'siltemelerdi $1',
'whatlinkshere-hideimages' => 'swret siltemelerin $1',
'whatlinkshere-filters'    => 'Süzgiler',

# Block/unblock
'blockip'                     => 'Qatıswşını buğattaw',
'blockip-legend'              => 'Qatıswşını buğattaw',
'blockiptext'                 => 'Tömendegi pişin qatıswşınıñ jazw ruqsatın belgili IP mekenjaýımen ne atımen buğattaw üşin qoldanıladı.
Bunı tek buzaqılıqtı qaqpaýlaw üşin jäne de [[{{{{ns:mediawiki}}:Policy-url}}|erejeler]] boýınşa atqarwıñız jön.
Tömende tïisti sebebin toltırıp körsetiñiz (mısalı, däýekke buzaqılıqpen özgertken betterdi keltirip).',
'ipaddress'                   => 'IP mekenjaýı:',
'ipadressorusername'          => 'IP mekenjaýı ne qatıswşı atı:',
'ipbexpiry'                   => 'Merzimi bitpek:',
'ipbreason'                   => 'Sebebi:',
'ipbreasonotherlist'          => 'Basqa sebep',
'ipbreason-dropdown'          => '* Buğattawdıñ jalpı sebebteri 
** Jalğan mälimet engizw 
** Betterdegi mağlumattı alastaw 
** Şettik toraptar siltemelerin jawdırw 
** Betterge mağınasızdıq/baldırlaw kiristirw 
** Qoqandaw/qwğındaw minezqulıq 
** Birneşe ret tirkelip qïyanattaw 
** Öreskel qatıswşı atı',
'ipbanononly'                 => 'Tek tirkelgisiz qatıswşılardı buğattaw',
'ipbcreateaccount'            => 'Tirkelwdi qaqpaýlaw',
'ipbemailban'                 => 'Qatıswşı e-poştamen xat jöneltwin qaqpaýlaw',
'ipbenableautoblock'          => 'Bul qatıswşı soñğı qoldanğan IP mekenjaýın, jäne keýin öñdewge baýqap körgen är IP mekenjaýların özbuğattawı',
'ipbsubmit'                   => 'Qatıswşını buğatta',
'ipbother'                    => 'Basqa merzimi:',
'ipboptions'                  => '2 sağat:2 hours,1 kün:1 day,3 kün:3 days,1 apta:1 week,2 apta:2 weeks,1 aý:1 month,3 aý:3 months,6 aý:6 months,1 jıl:1 year,mängi:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'basqa',
'ipbotherreason'              => 'Basqa/qosımşa sebep:',
'ipbhidename'                 => 'Qatıswşı atın buğattaw jwrnalınnan, belsendi buğattaw tiziminen, qatıswşı tiziminen jasırw',
'ipbwatchuser'                => 'Bul qatıswşınıñ jeke jäne talqılaw betterin baqılaw',
'badipaddress'                => 'Jaramsız IP mekenjaýı',
'blockipsuccesssub'           => 'Buğattaw sätti ötti',
'blockipsuccesstext'          => '[[{{#special:Contributions}}/$1|$1]] degen buğattalğan.<br />
Buğattardı şolıp şığw üşin [[{{#special:Ipblocklist}}|IP buğattaw tizimin]] qarañız.',
'ipb-edit-dropdown'           => 'Buğattaw sebepterin öñdew',
'ipb-unblock-addr'            => '$1 degendi buğattamaw',
'ipb-unblock'                 => 'Qatıswşı atın nemese IP mekenjaýın buğattamaw',
'ipb-blocklist-addr'          => '$1 üşin bar buğattawlardı qaraw',
'ipb-blocklist'               => 'Bar buğattawlardı qaraw',
'unblockip'                   => 'Qatıswşını buğattamaw',
'unblockiptext'               => 'Tömendegi pişindi aldındağı IP mekenjaýımen ne atımen buğattalğan qatıswşığa jazw qatınawın qalpına keltiriwi üşin qoldanıñız.',
'ipusubmit'                   => 'Osı mekenjaýdı buğattamaw',
'unblocked'                   => '[[{{ns:user}}:$1|$1]] buğattawı öşirildi',
'unblocked-id'                => '$1 buğattaw alastaldı',
'ipblocklist'                 => 'Buğattalğan qatıswşı / IP mekenjaý tizimi',
'ipblocklist-legend'          => 'Buğattalğan qatıswşını tabw',
'ipblocklist-username'        => 'Qatıswşı atı / IP mekenjaýı:',
'ipblocklist-submit'          => 'İzde',
'blocklistline'               => '$1, $2 $3 degendi buğattadı ($4)',
'infiniteblock'               => 'mängi',
'expiringblock'               => 'merzimi bitpek: $1',
'anononlyblock'               => 'tek tirkelgisizderdi',
'noautoblockblock'            => 'özbuğattaw öşirilgen',
'createaccountblock'          => 'tirkelw buğattalğan',
'emailblock'                  => 'e-poşta buğattalğan',
'ipblocklist-empty'           => 'Buğattaw tizimi bos.',
'ipblocklist-no-results'      => 'Suratılğan IP mekenjaý ne qatıswşı atı buğattalğan emes.',
'blocklink'                   => 'buğattaw',
'unblocklink'                 => 'buğattamaw',
'contribslink'                => 'ülesi',
'autoblocker'                 => 'IP mekenjaýıñızdı jwıqta «[[{{ns:user}}:1|$1]]» paýdalanğan, sondıqtan özbuğattalğan.
$1 buğattawı üşin keltirilgen sebebi: «$2».',
'blocklogpage'                => 'Buğattaw_jwrnalı',
'blocklogentry'               => '[[$1]] degendi $2 merzimge buğattadı $3',
'blocklogtext'                => 'Bul qatıswşılardı buğattaw/buğattamaw äreketteriniñ jwrnalı.
Özdiktik buğattalğan IP mekenjaýlar osında tizimdelgemegen.
Ağımdağı belsendi tïımdar men buğattawlardı [[{{#special:Ipblocklist}}|IP buğattaw tiziminen]] qarañız.',
'unblocklogentry'             => '«$1» — buğattawın öşirdi',
'block-log-flags-anononly'    => 'tek tirkelgisizder',
'block-log-flags-nocreate'    => 'tirkelw öşirilgen',
'block-log-flags-noautoblock' => 'özbuğattaw öşirilgen',
'block-log-flags-noemail'     => 'e-poşta buğattalğan',
'range_block_disabled'        => 'Awqım buğattawların jasaw äkimşilik mümkindigi öşirilgen.',
'ipb_expiry_invalid'          => 'Bitetin waqıtı jaramsız.',
'ipb_expiry_temp'             => 'Jasırılğan qatıswşı atın buğattawı mäñgi bolwı jön.',
'ipb_already_blocked'         => '«$1» aldaqaşan buğattalğan',
'ipb_cant_unblock'            => 'Qatelik: IP $1 buğattawı tabılmadı. Onıñ buğattawı aldaqaşan öşirlgen mümkin.',
'ipb_blocked_as_range'        => 'Qatelik: IP $1 tikeleý buğattalmağan jäne buğattawı öşirilmeýdi.
Biraq, bul buğattawı öşirilwi mümkin $2 awqımı böligi bop buğattalğan.',
'ip_range_invalid'            => 'IP mekenjaý awqımı jaramsız.',
'blockme'                     => 'Özdiktik_buğattaw',
'proxyblocker'                => 'Proksï serverlerdi buğattawış',
'proxyblocker-disabled'       => 'Bul jete öşirilgen.',
'proxyblockreason'            => 'IP mekenjaýıñız aşıq proksï serverge jatatındıqtan buğattalğan.
Ïnternet qızmetin jabdıqtawşıñızben, ne texnïkalıq qoldaw qızmetimen qatınasıñız, jäne olarğa osı ote kürdeli qawıpsizdik şataq twralı aqparat beriñiz.',
'proxyblocksuccess'           => 'Bitti.',
'sorbsreason'                 => 'IP mekenjaýıñız {{SITENAME}} torabında qoldanılğan DNSBL qara tizimindegi aşıq proksï-server dep tabıladı.',
'sorbs_create_account_reason' => 'IP mekenjaýıñız {{SITENAME}} torabında qoldanılğan DNSBL qara tizimindegi aşıq proksï-server dep tabıladı.
Jaña tirkelgi jasaý almaýsız.',

# Developer tools
'lockdb'              => 'Derekqordı qulıptaw',
'unlockdb'            => 'Derekqordı qulıptamaw',
'lockdbtext'          => 'Derekqordın qulıptalwı barlıq qatıswşılardıñ bet öñdew, baptawın qalaw, baqılaw tizimin, tağı basqa derekqordı özgertetin mümkindikterin toqtata turadı.
Osı maqsatıñızdı, jäne baptaw bitkende derekqordı aşatıñızdı quptañız.',
'unlockdbtext'        => 'Derekqodın aşılwı barlıq qatıswşılardıñ bet öñdew, baptawın qalaw, baqılaw tizimin, tağı basqa derekqordı özgertetin mümkindikterin qalpına keltiredi.
Osı maqsatıñızdı quptañız.',
'lockconfirm'         => 'Ïä, derekqor qulıptawın naqtı tileýmin.',
'unlockconfirm'       => 'Ïä, derekqor qulıptamawın naqtı tileýmin.',
'lockbtn'             => 'Derekqordı qulıpta',
'unlockbtn'           => 'Derekqordı qulıptama',
'locknoconfirm'       => 'Quptaw közine qusbelgi salmağansız.',
'lockdbsuccesssub'    => 'Derekqor qulıptawı sätti ötti',
'unlockdbsuccesssub'  => 'Derekqor qulıptawı alastaldı',
'lockdbsuccesstext'   => 'Derekqor qulıptaldı.<br />
Baptaw tolıq ötkizilgennen keýin [[{{#special:Unlockdb}}|qulıptawın alastawğa]] umıtpañız.',
'unlockdbsuccesstext' => 'Qulıptalğan derekqor sätti aşıldı.',
'lockfilenotwritable' => 'Derekqor qulıptaw faýlı jazılmaýdı.
Derekqordı qulıptaw ne aşw üşin, veb-server faýlğa jazw ruqsatı bolw kerek.',
'databasenotlocked'   => 'Derekqor qulıptalğan joq.',

# Move page
'move-page'               => '$1 degendi jıljıtw',
'move-page-legend'        => 'Betti jıljıtw',
'movepagetext'            => "Tömendegi pişindi qoldanıp betterdi qaýta ataýdı, barlıq tarïxın jaña atawğa jıljıtadı.
Burınğı bet taqırıbın atı jaña taqırıp atına aýdaýtın bet boladı.
Eski taqırıp atına silteýtin siltemeler özgertilmeýdi;
jıljıtwdan soñ şınjırlı ne jaramsız aýdağıştar bar-joğın tekserip şığıñız.
Siltemeler burınğı joldawımen bılaýğı ötwin tekserwine öziñiz mindetti bolasız.

Añğartpa: Eger osı arada aldaqaşan jaña taqırıp atı bar bet bolsa, bul bos ne aýdağış bolğanşa deýin, jäne soñında tüzetw tarïxı joq bolsa, bet '''jıljıtılmaýdı'''. Osınıñ mağınası: eger betti qatelikpen qaýta atasañız, burınğı atawına qaýta atawğa boladı, jäne bar bettiñ üstine jazwıñızğa bolmaýdı.

'''QULAQTANDIRW!'''
Bul köp qaralatın betke qatañ jäne kenet özgeris jasawğa mümkin;
osınıñ saldarın baýımdawıñızdı ärekettiñ aldınan batıl bolıñız.",
'movepagetalktext'        => "Kelesi sebepter '''bolğanşa''' deýin, talqılaw beti bunımen birge özdiktik jıljıtıladı:
* Bos emes talqılaw beti jaña atawda aldaqaşan bolğanda, ne
* Tömendegi közge qusbelgi alıp tastalğanda.

Osı oraýda, qalawıñız bolsa, betti qoldan jıljıta ne qosa alasız.",
'movearticle'             => 'Jıljıtpaq bet:',
'movenotallowed'          => '{{SITENAME}} jobasında betterdi jıljıtw rwqsatıñız joq.',
'newtitle'                => 'Jaña taqırıp atına:',
'move-watch'              => 'Bul betti baqılaw',
'movepagebtn'             => 'Betti jıljıt',
'pagemovedsub'            => 'Jıljıtw sätti ayaqtaldı',
'movepage-moved'          => '<big>\'\'\'"$1" beti "$2" betine jıljıtıldı\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Osılaý atalğan bet aldaqaşan bar, ne tañdağan atawıñız jaramdı emes.
Özge atawdı tañdañız',
'cantmove-titleprotected' => 'Betti osı orınğa jıljıta almaýsız, sebebi jaña taqırıp atı bastawdan qorğalğan',
'talkexists'              => "'''Bettiñ özi sätti jıljıtıldı, biraq talqılaw beti birge jıljıtılmadı, onıñ sebebi jaña taqırıp atında birewi aldaqaşan bar.
Bunı qolmen qosıñız.'''",
'movedto'                 => 'mınağan jıljıtıldı:',
'movetalk'                => 'Qawımdastı talqılaw betin jıljıtw',
'move-subpages'           => 'Barlıq betşelerin jıljıtw, eger qoldanbalı bolsa',
'move-talk-subpages'      => 'Talqılaw betiniñ barlıq betşelerin jıljıtw, eger qoldanbalı bolsa',
'movepage-page-exists'    => '$1 degen bet aldaqaşan bar jäne üstine özdiktik jazılmaýdı.',
'movepage-page-moved'     => '$1 degen bet $2 degenge jıljıtıldı.',
'movepage-page-unmoved'   => '$1 degen bet $2 degenge jıljıtılmaýdı.',
'movepage-max-pages'      => 'Barınşa $1 bet jıljıtıldı da mınnan köbi özdiktik jıljıltılmaýdı.',
'1movedto2'               => '[[$1]] degendi [[$2]] degenge jıljıttı',
'1movedto2_redir'         => '[[$1]] degendi [[$2]] degen aýdağış üstine jıljıttı',
'movelogpage'             => 'Jıljıtw jwrnalı',
'movelogpagetext'         => 'Tömende jıljıtılğan betterdiñ tizimi berilip tur.',
'movereason'              => 'Sebebi:',
'revertmove'              => 'qaýtarw',
'delete_and_move'         => 'Joyw jäne jıljıtw',
'delete_and_move_text'    => '==Joyw kerek==
«[[:$1]]» degen nısana bet aldaqaşan bar.
Jıljıtwğa jol berw üşin bunı joyasız ba?',
'delete_and_move_confirm' => 'Ïä, bul betti joý',
'delete_and_move_reason'  => 'Jıljıtwğa jol berw üşin joýılğan',
'selfmove'                => 'Qaýnar jäne nısana taqırıp attarı birdeý;
bet öziniñ üstine jıljıtılmaýdı.',
'immobile_namespace'      => 'Qaýnar ne nısana taqırıp atı arnawlı türine jatadı;
better bul esim ayası sırtına jäne işine jıljıtılmaýdı.',
'imagenocrossnamespace'   => 'Faýl emes esim ayasına faýl jıljıtılmaýdı',
'imagetypemismatch'       => 'Faýldıñ jaña keñeýtimi bunıñ türine säýkes emes',

# Export
'export'            => 'Betterdi sırtqa berw',
'exporttext'        => 'XML pişimine qaptalğan bölek bet ne better bwması mätiniñ jäne öñdew tarïxın sırtqa bere alasız. 
MediaWiki jüýesiniñ [[{{#special:Import}}|sırttan alw betin]] paýdalanıp, bunı özge wïkïge alwğa boladı.

Betterdi sırtqa berw üşin, taqırıp attarın tömendegi mätin jolağına engiziñiz (jol saýın bir taqırıp atı), jäne de bölekteñiz: ne ağımdıq nusqasın, barlıq eski nusqaları men jäne tarïxı joldarı men birge, nemese däl ağımdıq nusqasın, soñğı öñdemew twralı aqparatı men birge.

Soñğı jağdaýda siltemeni de, mısalı «{{{{ns:mediawiki}}:Mainpage}}» beti üşin [[{{#special:Export}}/{{MediaWiki:Mainpage}}]] qoldanwğa boladı.',
'exportcuronly'     => 'Tolıq tarïxın emes, tek ağımdıq tüzetwin kiristiriñiz',
'exportnohistory'   => "----
'''Añğartpa:''' Önimdilik äseri sebepterinen, betterdiñ tolıq tarïxın bul pişinmen sırtqa berwi öşirilgen.",
'export-submit'     => 'Sırtqa ber',
'export-addcattext' => 'Mına sanattağı betterdi üstew:',
'export-addcat'     => 'Üste',
'export-download'   => 'Faýl türinde saqtaw',
'export-templates'  => 'Ülgilerdi qosa alıp',

# Namespace 8 related
'allmessages'               => 'Jüýe xabarları',
'allmessagesname'           => 'Atawı',
'allmessagesdefault'        => 'Ädepki mätini',
'allmessagescurrent'        => 'Ağımdıq mätini',
'allmessagestext'           => 'Mında {{ns:mediawiki}} esim ayasında jetimdi jüýe xabar tizimi beriledi.
Eger ämbebap MediaWiki jersindirwge üles qosqıñız kelse [http://www.mediawiki.org/wiki/Localisation MediaWiki jersindirw betine] jäne [http://translatewiki.net Betawiki jobasına] barıp şığıñız.',
'allmessagesnotsupportedDB' => "'''\$wgUseDatabaseMessages''' öşirilgen sebebinen '''{{ns:special}}:AllMessages''' beti qoldanılmaýdı.",
'allmessagesfilter'         => 'Xabardı atawımen süzgilew:',
'allmessagesmodified'       => 'Tek özgertilgendi körset',

# Thumbnails
'thumbnail-more'           => 'Ülkeýtw',
'filemissing'              => 'Joğalğan faýl',
'thumbnail_error'          => 'Nobaý qurw qatesi: $1',
'djvu_page_error'          => 'DjVu beti awmaq sırtındda',
'djvu_no_xml'              => 'DjVu faýlı üşin XML keltirwi ïkemdi emes',
'thumbnail_invalid_params' => 'Nobaýdıñ baptalımdarı jaramsız',
'thumbnail_dest_directory' => 'Nısana qaltası qurwı ïkemdi emes',

# Special:Import
'import'                     => 'Betterdi sırttan alw',
'importinterwiki'            => 'Wïkï-aparw üşin sırttan alw',
'import-interwiki-text'      => 'Sırttan alınatın wïkïdi jäne bettiñ taqırıp atın bölekteñiz.
Tüzetw kün-aýı jäne öñdewşi esimderi saqtaladı.
Wïkï-aparw üşin sırttan alw barlıq äreketter [[{{#special:Log}}/import|sırttan alw jwrnalına]] jazılıp alınadı.',
'import-interwiki-history'   => 'Bul bettiñ barlıq tarïxï nusqaların köşirw',
'import-interwiki-submit'    => 'Sırttan alw',
'import-interwiki-namespace' => 'Betterdi mına esim ayasına aparw:',
'importtext'                 => 'Qaýnar wïkïden «{{#special:Export}}» qwralın qoldanıp faýldı sırtqa beriñiz, dïskiñizge saqtañız da mında qotarıp beriñiz.',
'importstart'                => 'Betterdi sırttan alwda…',
'import-revision-count'      => '$1 tüzetw',
'importnopages'              => 'Sırttan alınatın better joq.',
'importfailed'               => 'Sırttan alw sätsiz bitti: <nowiki>$1</nowiki>',
'importunknownsource'        => 'Cırttan alınatın qaýnar türi belgisiz',
'importcantopen'             => 'Sırttan alınatın faýl aşılmaýdı',
'importbadinterwiki'         => 'Jaramsız wïkï-aralıq silteme',
'importnotext'               => 'Bul bos, nemese mätini joq',
'importsuccess'              => 'Sırttan alw ayaqtaldı!',
'importhistoryconflict'      => 'Tarïxında qaqtığıstı tüzetw bar (bul bet aldında sırttan alınğan sïyaqtı)',
'importnosources'            => 'Wïkï-aparw üşin sırttan alınatın eş qaýnar közi anıqtalmağan, jäne tarïxın tikeleý qotarıp berwi öşirilgen.',
'importnofile'               => 'Sırttan alınğan faýl qotarıp berilgen joq.',
'importuploaderrorsize'      => 'Sırttan alınğan faýldıñ qotarıp berilwi sätsiz ötti. Faýl mölşeri qotarıp berilwge rwqsat etilgennen asadı.',
'importuploaderrorpartial'   => 'Sırttan alınğan faýldıñ qotarıp berilwi sätsiz ötti. Osı faýldıñ tek bölikteri qotarılıp berildi.',
'importuploaderrortemp'      => 'Sırttan alınğan faýldıñ qotarıp berilwi sätsiz ötti. Waqıtşa qalta tabılmadı.',
'import-parse-failure'       => 'Sırttan alınğan XML faýl qurılımın taldatqanda sätsizdik boldı',
'import-noarticle'           => 'Sırttan alınatın eş bet joq!',
'import-nonewrevisions'      => 'Barlıq tüzetwleri aldında sırttan alınğan.',
'xml-error-string'           => '$1 nömir $2 jolda, bağan $3 (baýt $4): $5',
'import-upload'              => 'XML derekterin qotarıp berw',

# Import log
'importlogpage'                    => 'Sırttan alw jwrnalı',
'importlogpagetext'                => 'Betterdi tüzetw tarïxımen birge sırtqı wïkïlerden äkimşi retinde alw.',
'import-logentry-upload'           => '«[[$1]]» degendi faýl qotarıp berw arqılı sırttan aldı',
'import-logentry-upload-detail'    => '$1 tüzetw',
'import-logentry-interwiki'        => 'wïkï-aparılğan $1',
'import-logentry-interwiki-detail' => '$2 degennen $1 tüzetw',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Jeke betim',
'tooltip-pt-anonuserpage'         => 'Bul IP mekenjaýdıñ jeke beti',
'tooltip-pt-mytalk'               => 'Talqılaw betim',
'tooltip-pt-anontalk'             => 'Bul IP mekenjaý öñdemelerin talqılaw',
'tooltip-pt-preferences'          => 'Baptalımdarım',
'tooltip-pt-watchlist'            => 'Özgeristerin baqılap turğan better tizimim.',
'tooltip-pt-mycontris'            => 'Ülesterimdiñ tizimi',
'tooltip-pt-login'                => 'Kirwiñizdi usınamız, ol mindetti emes.',
'tooltip-pt-anonlogin'            => 'Kirwiñizdi usınamız, biraq, ol mindetti emes.',
'tooltip-pt-logout'               => 'Şığw',
'tooltip-ca-talk'                 => 'Mağlumat betti talqılaw',
'tooltip-ca-edit'                 => 'Bul betti öñdeý alasız. Saqtawdıñ aldında «Qarap şığw» batırmasın nuqıñız.',
'tooltip-ca-addsection'           => 'Bul talqılaw betinde jaña taraw bastaw.',
'tooltip-ca-viewsource'           => 'Bul bet qorğalğan. Qaýnar közin qaraý alasız.',
'tooltip-ca-history'              => 'Bul bettin jwıqtağı nusqaları.',
'tooltip-ca-protect'              => 'Bul betti qorğaw',
'tooltip-ca-delete'               => 'Bul betti joyw',
'tooltip-ca-undelete'             => 'Bul bettiñ joywdıñ aldındağı bolğan öñdemelerin qalpına keltirw',
'tooltip-ca-move'                 => 'Bul betti jıljıtw',
'tooltip-ca-watch'                => 'Bul betti baqılaw tizimiñizge üstew',
'tooltip-ca-unwatch'              => 'Bul betti baqılaw tizimiñizden alastaw',
'tooltip-search'                  => '{{SITENAME}} jobasında izdew',
'tooltip-search-go'               => 'Eger däl osı atawımen bolsa betke ötip ketw',
'tooltip-search-fulltext'         => 'Osı mätini bar betti izdew',
'tooltip-p-logo'                  => 'Bastı betke',
'tooltip-n-mainpage'              => 'Bastı betke kelip-ketiñiz',
'tooltip-n-portal'                => 'Joba twralı, ne istewiñizge bolatın, qaýdan tabwğa bolatın twralı',
'tooltip-n-currentevents'         => 'Ağımdağı oqïğalarğa qatıstı öñ aqparatın tabw',
'tooltip-n-recentchanges'         => 'Osı wïkïdegi jwıqtağı özgerister tizimi.',
'tooltip-n-randompage'            => 'Kezdeýsoq betti jüktew',
'tooltip-n-help'                  => 'Anıqtama tabw ornı.',
'tooltip-t-whatlinkshere'         => 'Mında siltegen barlıq betterdiñ tizimi',
'tooltip-t-recentchangeslinked'   => 'Mınnan siltengen betterdiñ jwıqtağı özgeristeri',
'tooltip-feed-rss'                => 'Bul bettiñ RSS arnası',
'tooltip-feed-atom'               => 'Bul bettiñ Atom arnası',
'tooltip-t-contributions'         => 'Osı qatıswşınıñ üles tizimin qaraw',
'tooltip-t-emailuser'             => 'Osı qatıswşığa xat jöneltw',
'tooltip-t-upload'                => 'Faýldardı qotarıp berw',
'tooltip-t-specialpages'          => 'Barlıq arnaýı better tizimi',
'tooltip-t-print'                 => 'Bul bettiñ basıp şığarışqa arnalğan nusqası',
'tooltip-t-permalink'             => 'Mına bettiñ osı nusqasınıñ turaqtı siltemesi',
'tooltip-ca-nstab-main'           => 'Mağlumat betin qaraw',
'tooltip-ca-nstab-user'           => 'Qatıswşı betin qaraw',
'tooltip-ca-nstab-media'          => 'Taspa betin qaraw',
'tooltip-ca-nstab-special'        => 'Bul arnaýı bet, bettiñ özi öñdelinbeýdi.',
'tooltip-ca-nstab-project'        => 'Joba betin qaraw',
'tooltip-ca-nstab-image'          => 'Faýl betin qaraw',
'tooltip-ca-nstab-mediawiki'      => 'Jüýe xabarın qaraw',
'tooltip-ca-nstab-template'       => 'Ülgini qaraw',
'tooltip-ca-nstab-help'           => 'Anıqtıma betin qaraw',
'tooltip-ca-nstab-category'       => 'Sanat betin qaraw',
'tooltip-minoredit'               => 'Bunı şağın öñdeme dep belgilew',
'tooltip-save'                    => 'Jasağan özgeristeriñizdi saqtaw',
'tooltip-preview'                 => 'Saqtawdıñ aldınan jasağan özgeristeriñizdi qarap şığıñız!',
'tooltip-diff'                    => 'Mätinge qandaý özgeristerdi jasağanıñızdı qaraw.',
'tooltip-compareselectedversions' => 'Bettiñ eki bölektengen nusqası aýırmasın qaraw.',
'tooltip-watch'                   => 'Bul betti baqılaw tizimiñizge üstew',
'tooltip-recreate'                => 'Bet joýılğanına qaramastan qaýta bastaw',
'tooltip-upload'                  => 'Qotarıp berwdi bastaw',

# Stylesheets
'common.css'      => '/* Mında ornalastırılğan CSS barlıq mänerlerde qoldanıladı */',
'standard.css'    => '/* Mında ornalastırılğan CSS tek «Dağdılı» (standard) mänerin paýdalanwşılarına ıqpal etedi */',
'nostalgia.css'   => '/* Mında ornalastırılğan CSS tek «Añsaw» (nostalgia) mänerin paýdalanwşılarına ıqpal etedi */',
'cologneblue.css' => '/* Mında ornalastırılğan CSS tek «Köln zeñgirligi» (cologneblue) mänerin paýdalanwşılarına ıqpal etedi skin */',
'monobook.css'    => '/* Mında ornalastırılğan CSS tek «Dara kitap» (monobook) mänerin paýdalanwşılarına ıqpal etedi */',
'myskin.css'      => '/* Mında ornalastırılğan CSS tek «Öz mänerim» (myskin) mänerin paýdalanwşılarına ıqpal etedi */',
'chick.css'       => '/* Mında ornalastırılğan CSS tek «Balapan» (chick) mänerin paýdalanwşılarına ıqpal etedi */',
'simple.css'      => '/* Mında ornalastırılğan CSS tek «Kädimgi» (simple) mänerin paýdalanwşılarına ıqpal etedi */',
'modern.css'      => '/* Mında ornalastırılğan CSS tek «Zamanawï» (modern) mänerin paýdalanwşılarına ıqpal etedi */',

# Scripts
'common.js'      => '/* Mındağı ärtürli JavaScript kez kelgen bet qotarılğanda barlıq paýdalanwşılar üşin jegiledi. */',
'standard.js'    => '/* Mındağı JavaScript tek «Dağdılı» (standard) mänerin paýdalanwşılar üşin jegiledi */',
'nostalgia.js'   => '/* Mındağı JavaScript tek «Añsaw» (nostalgia) mänerin paýdalanwşılar üşin jegiledi*/',
'cologneblue.js' => '/* Mındağı JavaScript tek «Köln zeñgirligi» (cologneblue) mänerin paýdalanwşılar üşin jegiledi */',
'monobook.js'    => '/* Mındağı JavaScript tek «Dara kitap» (monobook) mänerin paýdalanwşılar üşin jegiledi */',
'myskin.js'      => '/* Mındağı JavaScript tek «Öz mänerim» (myskin) mänerin paýdalanwşılar üşin jegiledi */',
'chick.js'       => '/* Mındağı JavaScript tek «Balapan» (chick) mänerin paýdalanwşılar üşin jegiledi */',
'simple.js'      => '/* Mındağı JavaScript tek «Kädimgi» (simple) mänerin paýdalanwşılar üşin jegiledi */',
'modern.js'      => '/* Mındağı JavaScript tek «Zamanawï» (modern) mänerin paýdalanwşılar üşin jegiledi */',

# Metadata
'nodublincore'      => 'Bul serverde «Dublin Core RDF» türi qosımşa derekteri öşirilgen.',
'nocreativecommons' => 'Bul serverde «Creative Commons RDF» türi qosımşa derekteri öşirilgen.',
'notacceptable'     => 'Tutınğışıñız oqï alatın pişimi bar derekterdi bul wïkï server jetistire almaýdı.',

# Attribution
'anonymous'        => '{{SITENAME}} tirkelgisiz qatıswşı(ları)',
'siteuser'         => '{{SITENAME}} qatıswşı $1',
'lastmodifiedatby' => 'Bul betti $3 qatıswşı soñğı özgertken kezi: $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Şığarma negizin $1 jazğan.',
'others'           => 'basqalar',
'siteusers'        => '{{SITENAME}} qatıswşı(lar) $1',
'creditspage'      => 'Betti jazğandar',
'nocredits'        => 'Bul betti jazğandar twralı aqparat joq.',

# Spam protection
'spamprotectiontitle' => '«Spam»-nan qorğaýtın süzgi',
'spamprotectiontext'  => 'Bul bettiñ saqtawın «spam» süzgisi buğattadı.
Bunıñ sebebi şettik torap siltemesinen bolwı mümkin.',
'spamprotectionmatch' => 'Kelesi «spam» mätini süzgilengen: $1',
'spambot_username'    => 'MediaWiki spam cleanup',
'spam_reverting'      => '$1 degenge siltemeleri joq soñğı nusqasına qaýtarıldı',
'spam_blanking'       => '$1 degenge siltemeleri bar barlıq tüzetwler tazartıldı',

# Info page
'infosubtitle'   => 'Bet twralı mälimet',
'numedits'       => 'Öñdeme sanı (bet): $1',
'numtalkedits'   => 'Öñdeme sanı (talqılaw beti): $1',
'numwatchers'    => 'Baqılawşı sanı: $1',
'numauthors'     => 'Ärtürli awtor sanı (bet): $1',
'numtalkauthors' => 'Ärtürli awtor sanı (talqılaw beti): $1',

# Math options
'mw_math_png'    => 'Ärqaşan PNG pişinimen körsetkiz',
'mw_math_simple' => 'Eger öte qarapaýım bolsa — HTML, äýtpese PNG',
'mw_math_html'   => 'Eger ıqtïmal bolsa — HTML, äýtpese PNG',
'mw_math_source' => 'Bunı TeX pişiminde qaldır (mätindik şolğıştarğa)',
'mw_math_modern' => 'Osı zamanğı şolğıştarına usınıladı',
'mw_math_mathml' => 'Eger ıqtïmal bolsa — MathML (sınaqtama)',

# Patrolling
'markaspatrolleddiff'                 => 'Zertteldi dep belgilew',
'markaspatrolledtext'                 => 'Bul betti zertteldi dep belgile',
'markedaspatrolled'                   => 'Zertteldi dep belgilendi',
'markedaspatrolledtext'               => 'Bölektengen tüzetw zertteldi dep belgilendi.',
'rcpatroldisabled'                    => 'Jwıqtağı özgeristerdi zerttewi öşirilgen',
'rcpatroldisabledtext'                => 'Jwıqtağı özgeristerdi zerttew mümkindigi ağımda öşirilgen.',
'markedaspatrollederror'              => 'Zertteldi dep belgilenbeýdi',
'markedaspatrollederrortext'          => 'Zertteldi dep belgilew üşin tüzetwdi keltiriñiz.',
'markedaspatrollederror-noautopatrol' => 'Öz jasağan özgeristeriñizdi zertteldi dep belgileý almaýsız.',

# Patrol log
'patrol-log-page' => 'Zerttew jwrnalı',
'patrol-log-line' => '$2 degenniñ $1 tüzetwin zertteldi dep belgiledi $3',
'patrol-log-auto' => '(özdiktik)',
'patrol-log-diff' => 'nömir $1',

# Image deletion
'deletedrevision'                 => 'Eski tüzetwin joýdı: $1',
'filedeleteerror-short'           => 'Faýl joyw qatesi: $1',
'filedeleteerror-long'            => 'Faýldı joýğanda qateler kezdesti:

$1',
'filedelete-missing'              => '«$1» faýlı joýılmaýdı, sebebi ol joq.',
'filedelete-old-unregistered'     => '«$1» faýldıñ keltirilgen tüzetwi derekqorda joq.',
'filedelete-current-unregistered' => '«$1» faýldıñ keltirilgen atawı derekqorda joq.',
'filedelete-archive-read-only'    => '«$1» murağat qaltasına veb-server jaza almaýdı.',

# Browsing diffs
'previousdiff' => '← Aldıñğı aýırm.',
'nextdiff'     => 'Kelesi aýırm. →',

# Media information
'mediawarning'         => "'''Qulaqtandırw''': Bul faýl türinde qaskünemdi kodı bar bolwı ıqtïmal; bunı jegip jüýeñizge zïyan keltirwiñiz mümkin.<hr />",
'imagemaxsize'         => 'Sïpattaması betindegi swrettiñ mölşerin şektewi:',
'thumbsize'            => 'Nobaý mölşeri:',
'widthheight'          => '$1 × $2',
'widthheightpage'      => '$1 × $2, $3 bet',
'file-info'            => 'Faýl mölşeri: $1, MIME türi: $2',
'file-info-size'       => '($1 × $2 nükte, faýl mölşeri: $3, MIME türi: $4)',
'file-nohires'         => '<small>Joğarı ajıratılımdığı jetimsiz.</small>',
'svg-long-desc'        => '(SVG faýlı, kesimdi $1 × $2 nükte, faýl mölşeri: $3)',
'show-big-image'       => 'Joğarı ajıratılımdı',
'show-big-image-thumb' => '<small>Qarap şığw mölşeri: $1 × $2 nükte</small>',

# Special:NewImages
'newimages'             => 'Jaña faýldar körmesi',
'imagelisttext'         => "Tömende $2 surıptalğan '''$1''' faýl tizimi.",
'newimages-summary'     => 'Bul arnaýı betinde soñğı qotarıp berilgen faýldar körsetiledi',
'showhidebots'          => '(bottardı $1)',
'noimages'              => 'Köretin eşteñe joq.',
'ilsubmit'              => 'İzde',
'bydate'                => 'kün-aýımen',
'sp-newimages-showfrom' => '$2, $1 kezinen beri — jaña swretterdi körset',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'video-dims'     => '$1, $2 × $3',
'minutes-abbrev' => 'mïn',
'hours-abbrev'   => 'sağ',

# Bad image list
'bad_image_list' => 'Pişimi tömendegideý:

Tek tizim danaları (* nışanımen bastalıtın joldar) esepteledi.
Joldıñ birinşi siltemesi jaramsız swretke siltew jön.
Sol joldağı keýingi ärbir siltemeler eren bolıp esepteledi, mısalı jol işindegi kezdesetin swreti bar better.',

# Metadata
'metadata'          => 'Qosımşa mälimetter',
'metadata-help'     => 'Osı faýlda qosımşa mälimetter bar. Bälkim, osı mälimetter faýldı jasap şığarw, ne sandılaw üşin paýdalanğan sandıq kamera, ne mätinalğırdan alınğan.
Eger osı faýl negizgi küýinen özgertilgen bolsa, keýbir ejeleleri özgertilgen fotoswretke laýıq bolmas.',
'metadata-expand'   => 'Egjeý-tegjeýin körset',
'metadata-collapse' => 'Egjeý-tegjeýin jasır',
'metadata-fields'   => 'Osı xabarda tizimdelgen EXIF qosımşa mälimetter awmaqtarı, swret beti körsetw kezinde qosımşa mälimetter keste jasırılığanda kiristirledi.
Basqaları ädepkiden jasırıladı.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Eni',
'exif-imagelength'                 => 'Bïiktigi',
'exif-bitspersample'               => 'Quraş saýın bït sanı',
'exif-compression'                 => 'Qısım sulbası',
'exif-photometricinterpretation'   => 'Nükte qïıswı',
'exif-orientation'                 => 'Megzewi',
'exif-samplesperpixel'             => 'Quraş sanı',
'exif-planarconfiguration'         => 'Derek rettewi',
'exif-ycbcrsubsampling'            => 'Y quraşınıñ C quraşına jarnaqtawı',
'exif-ycbcrpositioning'            => 'Y quraşı jäne C quraşı mekendewi',
'exif-xresolution'                 => 'Dereleý ajıratılımdığı',
'exif-yresolution'                 => 'Tireleý ajıratılımdığı',
'exif-resolutionunit'              => 'X jäne Y boýınşa ajıratılımdıq birligi',
'exif-stripoffsets'                => 'Swret dererekteriniñ jaýğaswı',
'exif-rowsperstrip'                => 'Beldik saýın jol sanı',
'exif-stripbytecounts'             => 'Qısımdalğan beldik saýın baýt sanı',
'exif-jpeginterchangeformat'       => 'JPEG SOI degenge ığıswı',
'exif-jpeginterchangeformatlength' => 'JPEG derekteriniñ baýt sanı',
'exif-transferfunction'            => 'Tasımaldaw jetesi',
'exif-whitepoint'                  => 'Aq nükte tüstiligi',
'exif-primarychromaticities'       => 'Alğı şeptegi tüstilikteri',
'exif-ycbcrcoefficients'           => 'Tüs ayasın tasımaldaw matrïcalıq eselikteri',
'exif-referenceblackwhite'         => 'Qara jäne aq anıqtawış qos kolemderi',
'exif-datetime'                    => 'Faýldıñ özgertilgen kün-aýı',
'exif-imagedescription'            => 'Swret taqırıbın atı',
'exif-make'                        => 'Kamera öndirwşisi',
'exif-model'                       => 'Kamera ülgisi',
'exif-software'                    => 'Qoldanılğan bağdarlamalıq jasaqtama',
'exif-artist'                      => 'Twındıgeri',
'exif-copyright'                   => 'Awtorlıq quqıqtar ïesi',
'exif-exifversion'                 => 'Exif nusqası',
'exif-flashpixversion'             => 'Qoldanğan Flashpix nusqası',
'exif-colorspace'                  => 'Tüs ayası',
'exif-componentsconfiguration'     => 'Ärqaýsı quraş mäni',
'exif-compressedbitsperpixel'      => 'Swret qısımdaw tärtibi',
'exif-pixelydimension'             => 'Swrettiñ jaramdı eni',
'exif-pixelxdimension'             => 'Swrettiñ jaramdı bïiktigi',
'exif-makernote'                   => 'Öndirwşiniñ añğartpaları',
'exif-usercomment'                 => 'Qatıswşınıñ mändemeleri',
'exif-relatedsoundfile'            => 'Qatıstı dıbıs faýlı',
'exif-datetimeoriginal'            => 'Jasalğan kezi',
'exif-datetimedigitized'           => 'Sandıqtaw kezi',
'exif-subsectime'                  => 'Jasalğan keziniñ sekwnd bölşekteri',
'exif-subsectimeoriginal'          => 'Tüpnusqa keziniñ sekwnd bölşekteri',
'exif-subsectimedigitized'         => 'Sandıqtaw keziniñ sekwnd bölşekteri',
'exif-exposuretime'                => 'Ustalım waqıtı',
'exif-exposuretime-format'         => '$1 s ($2)',
'exif-fnumber'                     => 'Sañılaw mölşeri',
'exif-exposureprogram'             => 'Ustalım bağdarlaması',
'exif-spectralsensitivity'         => 'Spektr boýınşa sezgiştigi',
'exif-isospeedratings'             => 'ISO jıldamdıq jarnaqtawı (jarıq sezgiştigi)',
'exif-oecf'                        => 'Optoelektrondı türletw ıqpalı',
'exif-shutterspeedvalue'           => 'Japqış jıldamdılığı',
'exif-aperturevalue'               => 'Sañılawlıq',
'exif-brightnessvalue'             => 'Jarıqtılıq',
'exif-exposurebiasvalue'           => 'Ustalım ötemi',
'exif-maxaperturevalue'            => 'Barınşa sañılaw aşwı',
'exif-subjectdistance'             => 'Nısana qaşıqtığı',
'exif-meteringmode'                => 'Ölşew ädisi',
'exif-lightsource'                 => 'Jarıq közi',
'exif-flash'                       => 'Jarqıldağış',
'exif-focallength'                 => 'Şoğırlaw alşaqtığı',
'exif-subjectarea'                 => 'Nısana awqımı',
'exif-flashenergy'                 => 'Jarqıldağış qarqını',
'exif-spatialfrequencyresponse'    => 'Keñistik-jïilik äserşiligi',
'exif-focalplanexresolution'       => 'X boýınşa şoğırlaw jaýpaqtıqtıñ ajıratılımdığı',
'exif-focalplaneyresolution'       => 'Y boýınşa şoğırlaw jaýpaqtıqtıñ ajıratılımdığı',
'exif-focalplaneresolutionunit'    => 'Şoğırlaw jaýpaqtıqtıñ ajıratılımdıq ölşemi',
'exif-subjectlocation'             => 'Nısana ornalaswı',
'exif-exposureindex'               => 'Ustalım aýqındawı',
'exif-sensingmethod'               => 'Sensordiñ ölşew ädisi',
'exif-filesource'                  => 'Faýl qaýnarı',
'exif-scenetype'                   => 'Saxna türi',
'exif-cfapattern'                  => 'CFA süzgi keýipi',
'exif-customrendered'              => 'Qosımşa swret öñdetwi',
'exif-exposuremode'                => 'Ustalım tärtibi',
'exif-whitebalance'                => 'Aq tüsiniñ tendestigi',
'exif-digitalzoomratio'            => 'Sandıq awqımdaw jarnaqtawı',
'exif-focallengthin35mmfilm'       => '35 mm taspasınıñ şoğırlaw alşaqtığı',
'exif-scenecapturetype'            => 'Tüsirgen saxna türi',
'exif-gaincontrol'                 => 'Saxnanı rettew',
'exif-contrast'                    => 'Aşıqtıq',
'exif-saturation'                  => 'Qanıqtıq',
'exif-sharpness'                   => 'Aýqındıq',
'exif-devicesettingdescription'    => 'Jabdıq baptaw sïpattaması',
'exif-subjectdistancerange'        => 'Saxna qaşıqtığınıñ kölemi',
'exif-imageuniqueid'               => 'Swrettiñ biregeý nömiri (ID)',
'exif-gpsversionid'                => 'GPS belgişesiniñ nusqası',
'exif-gpslatituderef'              => 'Soltüstik nemese Oñtüstik boýlığı',
'exif-gpslatitude'                 => 'Boýlığı',
'exif-gpslongituderef'             => 'Şığıs nemese Batıs endigi',
'exif-gpslongitude'                => 'Endigi',
'exif-gpsaltituderef'              => 'Bïiktik körsetwi',
'exif-gpsaltitude'                 => 'Bïiktik',
'exif-gpstimestamp'                => 'GPS waqıtı (atom sağatı)',
'exif-gpssatellites'               => 'Ölşewge pýdalanılğan Jer serikteri',
'exif-gpsstatus'                   => 'Qabıldağış küýi',
'exif-gpsmeasuremode'              => 'Ölşew tärtibi',
'exif-gpsdop'                      => 'Ölşew däldigi',
'exif-gpsspeedref'                 => 'Jıldamdılıq ölşemi',
'exif-gpsspeed'                    => 'GPS qabıldağıştıñ jıldamdılığı',
'exif-gpstrackref'                 => 'Qozğalıs bağıtın körsetwi',
'exif-gpstrack'                    => 'Qozğalıs bağıtı',
'exif-gpsimgdirectionref'          => 'Swret bağıtın körsetwi',
'exif-gpsimgdirection'             => 'Swret bağıtı',
'exif-gpsmapdatum'                 => 'Paýdalanılğan geodezïyalıq tüsirme derekteri',
'exif-gpsdestlatituderef'          => 'Nısana boýlığın körsetwi',
'exif-gpsdestlatitude'             => 'Nısana boýlığı',
'exif-gpsdestlongituderef'         => 'Nısana endigin körsetwi',
'exif-gpsdestlongitude'            => 'Nısana endigi',
'exif-gpsdestbearingref'           => 'Nısana azïmwtın körsetwi',
'exif-gpsdestbearing'              => 'Nısana azïmwtı',
'exif-gpsdestdistanceref'          => 'Nısana qaşıqtığın körsetwi',
'exif-gpsdestdistance'             => 'Nısana qaşıqtığı',
'exif-gpsprocessingmethod'         => 'GPS öñdetw ädisiniñ atawı',
'exif-gpsareainformation'          => 'GPS awmağınıñ atawı',
'exif-gpsdatestamp'                => 'GPS kün-aýı',
'exif-gpsdifferential'             => 'GPS saralanğan durıstaw',

# EXIF attributes
'exif-compression-1' => 'Ulğaýtılğan',

'exif-unknowndate' => 'Belgisiz kün-aýı',

'exif-orientation-1' => 'Qalıptı', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Dereleý şağılısqan', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° burışqa aýnalğan', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Tireleý şağılısqan', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Sağat tilşesine qarsı 90° burışqa aýnalğan jäne tireleý şağılısqan', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Sağat tilşe boýınşa 90° burışqa aýnalğan', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Sağat tilşe boýınşa 90° burışqa aýnalğan jäne tireleý şağılısqan', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Sağat tilşesine qarsı 90° burışqa aýnalğan', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'talpaq pişim',
'exif-planarconfiguration-2' => 'taýpaq pişim',

'exif-componentsconfiguration-0' => 'bar bolmadı',

'exif-exposureprogram-0' => 'Anıqtalmağan',
'exif-exposureprogram-1' => 'Qolmen',
'exif-exposureprogram-2' => 'Bağdarlamalı ädis (qalıptı)',
'exif-exposureprogram-3' => 'Sañılaw basıñqılığı',
'exif-exposureprogram-4' => 'Isırma basıñqılığı',
'exif-exposureprogram-5' => 'Öner bağdarlaması (anıqtıq terendigine sanasqan)',
'exif-exposureprogram-6' => 'Qïmıl bağdarlaması (japqış şapşandılığına sanasqan)',
'exif-exposureprogram-7' => 'Tireleý ädisi (artı şoğırlawsız tayaw tüsirmeler)',
'exif-exposureprogram-8' => 'Dereleý ädisi (artı şoğırlanğan dereleý tüsirmeler)',

'exif-subjectdistance-value' => '$1 m',

'exif-meteringmode-0'   => 'Belgisiz',
'exif-meteringmode-1'   => 'Birkelki',
'exif-meteringmode-2'   => 'Buldır daq',
'exif-meteringmode-3'   => 'BirDaqtı',
'exif-meteringmode-4'   => 'KöpDaqtı',
'exif-meteringmode-5'   => 'Örnekti',
'exif-meteringmode-6'   => 'Jırtındı',
'exif-meteringmode-255' => 'Basqa',

'exif-lightsource-0'   => 'Belgisiz',
'exif-lightsource-1'   => 'Kün jarığı',
'exif-lightsource-2'   => 'Künjarıqtı şam',
'exif-lightsource-3'   => 'Qızdırğıştı şam',
'exif-lightsource-4'   => 'Jarqıldağış',
'exif-lightsource-9'   => 'Aşıq kün',
'exif-lightsource-10'  => 'Bulınğır kün',
'exif-lightsource-11'  => 'Kölenkeli',
'exif-lightsource-12'  => 'Künjarıqtı şam (D 5700–7100 K)',
'exif-lightsource-13'  => 'Künjarıqtı şam (N 4600–5400 K)',
'exif-lightsource-14'  => 'Künjarıqtı şam (W 3900–4500 K)',
'exif-lightsource-15'  => 'Künjarıqtı şam (WW 3200–3700 K)',
'exif-lightsource-17'  => 'Qalıptı jarıq qaýnarı A',
'exif-lightsource-18'  => 'Qalıptı jarıq qaýnarı B',
'exif-lightsource-19'  => 'Qalıptı jarıq qaýnarı C',
'exif-lightsource-24'  => 'Stwdïyalıq ISO künjarıqtı şam',
'exif-lightsource-255' => 'Basqa jarıq közi',

'exif-focalplaneresolutionunit-2' => 'düým',

'exif-sensingmethod-1' => 'Anıqtalmağan',
'exif-sensingmethod-2' => '1-çïpti awmaqtı tüssezgiş',
'exif-sensingmethod-3' => '2-çïpti awmaqtı tüssezgiş',
'exif-sensingmethod-4' => '3-çïpti awmaqtı tüssezgiş',
'exif-sensingmethod-5' => 'Kezekti awmaqtı tüssezgiş',
'exif-sensingmethod-7' => '3-sızıqtı tüssezgiş',
'exif-sensingmethod-8' => 'Kezekti sızıqtı tüssezgiş',

'exif-scenetype-1' => 'Tikeleý tüsirilgen fotoswret',

'exif-customrendered-0' => 'Qalıptı öñdetw',
'exif-customrendered-1' => 'Qosımşa öñdetw',

'exif-exposuremode-0' => 'Özdiktik ustalımdaw',
'exif-exposuremode-1' => 'Qolmen ustalımdaw',
'exif-exposuremode-2' => 'Özdiktik jarqıldaw',

'exif-whitebalance-0' => 'Aq tüsi özdiktik tendestirilgen',
'exif-whitebalance-1' => 'Aq tüsi qolmen tendestirilgen',

'exif-scenecapturetype-0' => 'Qalıptalğan',
'exif-scenecapturetype-1' => 'Dereleý',
'exif-scenecapturetype-2' => 'Tireleý',
'exif-scenecapturetype-3' => 'Tüngi saxna',

'exif-gaincontrol-0' => 'Joq',
'exif-gaincontrol-1' => 'Tömen zorayw',
'exif-gaincontrol-2' => 'Joğarı zorayw',
'exif-gaincontrol-3' => 'Tömen bayawlaw',
'exif-gaincontrol-4' => 'Joğarı bayawlaw',

'exif-contrast-0' => 'Qalıptı',
'exif-contrast-1' => 'Uyan',
'exif-contrast-2' => 'Turpaýı',

'exif-saturation-0' => 'Qalıptı',
'exif-saturation-1' => 'Tömen qanıqtı',
'exif-saturation-2' => 'Joğarı qanıqtı',

'exif-sharpness-0' => 'Qalıptı',
'exif-sharpness-1' => 'Uyan',
'exif-sharpness-2' => 'Turpaýı',

'exif-subjectdistancerange-0' => 'Belgisiz',
'exif-subjectdistancerange-1' => 'Tayaw tüsirilgen',
'exif-subjectdistancerange-2' => 'Jaqın tüsirilgen',
'exif-subjectdistancerange-3' => 'Alıs tüsirilgen',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Soltüstik boýlığı',
'exif-gpslatitude-s' => 'Oñtüstik boýlığı',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Şığıs endigi',
'exif-gpslongitude-w' => 'Batıs endigi',

'exif-gpsstatus-a' => 'Ölşew ulaswda',
'exif-gpsstatus-v' => 'Ölşew özara ärekette',

'exif-gpsmeasuremode-2' => '2-bağıttıq ölşem',
'exif-gpsmeasuremode-3' => '3-bağıttıq ölşem',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'km/h',
'exif-gpsspeed-m' => 'mil/h',
'exif-gpsspeed-n' => 'knot',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Şın bağıt',
'exif-gpsdirection-m' => 'Magnïttı bağıt',

# External editor support
'edit-externally'      => 'Bul faýldı şettik qondırma arqılı öñdew',
'edit-externally-help' => 'Köbirek aqparat üşin [http://www.mediawiki.org/wiki/Manual:External_editors ornatw nusqamaların] qarañız.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'barlığın',
'imagelistall'     => 'barlığı',
'watchlistall2'    => 'barlıq',
'namespacesall'    => 'barlığı',
'monthsall'        => 'barlığı',

# E-mail address confirmation
'confirmemail'             => 'E-poşta mekenjaýın quptaw',
'confirmemail_noemail'     => '[[{{#special:Preferences}}|Paýdalanwşılıq baptalımdarıñızda]] jaramdı e-poşta mekenjaýın qoýmapsız.',
'confirmemail_text'        => '{{SITENAME}} e-poşta mümkindikterin paýdalanw üşin aldınan e-poşta mekenjaýıñızdıñ jaramdılığın tekserip şığwıñız kerek.
Öziñizdiñ mekenjaýıñızğa quptaw xatın jöneltw üşin tömendegi batırmanı nuqıñız.
Xattıñ işinde kodı bar silteme kiristirmek; 
e-poşta jaýıñızdıñ jaramdılığın quptaw üşin siltemeni şolğıştıñ mekenjaý jolağına engizip aşıñız.',
'confirmemail_pending'     => '<div class="error">Quptaw kodı aldaqaşan xatpen jiberiligen;
eger jwıqta tirkelseñiz, jaña kodın suratw aldınan xat kelwin birşama mïnöt küte turıñız.</div>',
'confirmemail_send'        => 'Quptaw kodın jöneltw',
'confirmemail_sent'        => 'Quptaw xatı jöneltildi.',
'confirmemail_oncreate'    => 'Quptaw kodı e-poşta mekenjaýıñızğa jöneltildi.
Bul belgileme kirw üdirisine keregi joq, biraq e-poşta negizindegi wïkï mümkindikterdi qosw üşin bunı jetistirwiñiz kerek.',
'confirmemail_sendfailed'  => 'Quptaw xatı jöneltilmedi.
Jaramsız tañbalar üşin mekenjaýdı tekserip şığıñız.

Poşta jibergiştiñ qaýtarğan mälimeti: $1',
'confirmemail_invalid'     => 'Quptaw kodı jaramsız.
Kod merzimi bitken şığar.',
'confirmemail_needlogin'   => 'E-poşta mekenjaýıñızdı quptaw üşin $1 kerek.',
'confirmemail_success'     => 'E-poşta mekenjaýıñız quptaldı.
Endi wïkïge kirip jumısqa kiriswge boladı',
'confirmemail_loggedin'    => 'E-poşta mekenjaýıñız endi quptaldı.',
'confirmemail_error'       => 'Quptawñızdı saqtağanda belgisiz qate boldı.',
'confirmemail_subject'     => '{{SITENAME}} torabınan e-poşta mekenjaýıñızdı quptaw xatı',
'confirmemail_body'        => 'Keýbirew, $1 degen IP mekenjaýınan, öziñiz bolwı mümkin,
{{SITENAME}} jobasında bul E-poşta mekenjaýın qoldanıp «$2» degen tirkelgi jasaptı.

Bul tirkelgi naqtı sizge tän ekenin quptaw üşin, jäne {{SITENAME}} jobasınıñ
e-poşta mümkindikterin belsendirw üşin, mına siltemeni şolğışıñızben aşıñız:

$3

Eger bul tirkelgini jasağan öziñiz *emes* bolsa, mına siltemege erip
e-poşta mekenjaýı quptawın boldırmañız:

$5

Quptaw kodı merzimi bitetin kezi: $4.',
'confirmemail_invalidated' => 'E-poşta mekenjaýın quptawı boldırılmadı',
'invalidateemail'          => 'E-poşta mekenjaýın quptawı boldırmaw',

# Scary transclusion
'scarytranscludedisabled' => '[Wïkï-aralıq kirikbetter öşirilgen]',
'scarytranscludefailed'   => '[$1 üşin ülgi keltirwi sätsiz bitti; ğafw etiñiz]',
'scarytranscludetoolong'  => '[URL tım uzın; ğafw etiñiz]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">Bul bettiñ añıstawları:

$1
</div>',
'trackbackremove'   => '([$1 Joyw])',
'trackbacklink'     => 'Añıstaw',
'trackbackdeleteok' => 'Añıstaw sätti joýıldı.',

# Delete conflict
'deletedwhileediting' => 'Qulaqtandırw: Bul betti öñdewiñizdi bastağanda, osı bet joýıldı!',
'confirmrecreate'     => "Bul betti öñdewiñizdi bastağanda [[{{ns:user}}:$1|$1]] ([[{{ns:user_talk}}:$1|talqılawı]]) osı betti joýdı, keltirgen sebebi:
: ''$2''
Osı betti qaýta bastawın naqtı tilegeniñizdi quptañız.",
'recreate'            => 'Qaýta bastaw',

'unit-pixel' => ' nükte',

# HTML dump
'redirectingto' => '[[:$1]] betine aýdatwda…',

# action=purge
'confirm_purge'        => 'Bul bettin bürkemesin tazartasız ba?

$1',
'confirm_purge_button' => 'Jaraýdı',

# AJAX search
'searchcontaining' => "''$1'' mağlumatı bar betterden izdew.",
'searchnamed'      => "''$1'' atawı bar betterden izdew.",
'articletitles'    => "''$1'' dep bastalğan betterdi",
'hideresults'      => 'Nätïjelerdi jasır',
'useajaxsearch'    => 'AJAX qoldanıp izdew',

# Multipage image navigation
'imgmultipageprev' => '← aldıñğı betke',
'imgmultipagenext' => 'kelesi betke →',
'imgmultigo'       => 'Öt!',
'imgmultigoto'     => '$1 betine ötw',

# Table pager
'ascending_abbrev'         => 'ösw',
'descending_abbrev'        => 'kemw',
'table_pager_next'         => 'Kelesi betke',
'table_pager_prev'         => 'Aldıñğı betke',
'table_pager_first'        => 'Alğaşqı betke',
'table_pager_last'         => 'Soñğı betke',
'table_pager_limit'        => 'Bet saýın $1 dana körset',
'table_pager_limit_submit' => 'Ötw',
'table_pager_empty'        => 'Eş nätïje joq',

# Auto-summaries
'autosumm-blank'   => 'Bettiñ barlıq mağlumatın alastadı',
'autosumm-replace' => "Betti '$1' degenmen almastırdı",
'autoredircomment' => '[[$1]] degenge aýdadı',
'autosumm-new'     => 'Jaña bette: $1',

# Size units
'size-bytes' => '$1 baýt',

# Live preview
'livepreview-loading' => 'Jüktewde…',
'livepreview-ready'   => 'Jüktewde… Daýın!',
'livepreview-failed'  => 'Twra qarap şığw sätsiz! Kädimgi qarap şığw ädisin baýqap köriñiz.',
'livepreview-error'   => 'Qosılw sätsiz: $1 «$2». Kädimgi qarap şığw ädisin baýqap köriñiz.',

# Friendlier slave lag warnings
'lag-warn-normal' => '$1 sekwndtan jañalaw özgerister bul tizimde körsetilmewi mümkin.',
'lag-warn-high'   => 'Derekqor serveri zor keşigwi sebebinen, $1 sekwndtan jañalaw özgerister bul tizimde körsetilmewi mümkin.',

# Watchlist editor
'watchlistedit-numitems'       => 'Baqılaw tizimiñizde, talqılaw bettersiz, $1 taqırıp atı bar.',
'watchlistedit-noitems'        => 'Baqılaw tizimiñizde eş taqırıp atı joq.',
'watchlistedit-normal-title'   => 'Baqılaw tizimdi öñdew',
'watchlistedit-normal-legend'  => 'Baqılaw tiziminen taqırıp attarın alastaw',
'watchlistedit-normal-explain' => 'Baqılaw tizimiñizdegi taqırıp attar tömende körsetiledi.
Taqırıp atın alastaw üşin, büýir közge qusbelgi salıñız, jäne «Taqırıp attarın alasta» degendi nuqıñız.
Tağı da [[{{#special:Watchlist}}/raw|qam tizimdi öñdeý]] alasız.',
'watchlistedit-normal-submit'  => 'Taqırıp attarın alasta',
'watchlistedit-normal-done'    => 'Baqılaw tizimiñizden $1 taqırıp atı alastaldı:',
'watchlistedit-raw-title'      => 'Qam baqılaw tizimdi öñdew',
'watchlistedit-raw-legend'     => 'Qam baqılaw tizimdi öñdew',
'watchlistedit-raw-explain'    => 'Baqılaw tizimiñizdegi taqırıp attarı tömende körsetiledi, jäne de tizmge üstep jäne tizmden alastap öñdelwi mümkin;
jol saýın bir taqırıp atı bolw jön.
Bitirgennen soñ «Baqılaw tizimdi jañartw» degendi nuqıñız.
Tağı da [[{{#special:Watchlist}}/edit|qalıpalğan öñdewişti paýdalana]] alasız.',
'watchlistedit-raw-titles'     => 'Taqırıp attarı:',
'watchlistedit-raw-submit'     => 'Baqılaw tizimdi jañartw',
'watchlistedit-raw-done'       => 'Baqılaw tizimiñiz jañartıldı.',
'watchlistedit-raw-added'      => '$1 taqırıp atı üsteldi:',
'watchlistedit-raw-removed'    => '$1 taqırıp atı alastaldı:',

# Watchlist editing tools
'watchlisttools-view' => 'Qatıstı özgeristerdi qaraw',
'watchlisttools-edit' => 'Baqılaw tizimdi qaraw jäne öñdew',
'watchlisttools-raw'  => 'Qam baqılaw tizimdi öñdew',

# Iranian month names
'iranian-calendar-m1'  => 'pırwardïn',
'iranian-calendar-m2'  => 'ärdïbeşt',
'iranian-calendar-m3'  => 'xırdad',
'iranian-calendar-m4'  => 'tïr',
'iranian-calendar-m5'  => 'mırdad',
'iranian-calendar-m6'  => 'şerïyar',
'iranian-calendar-m7'  => 'mer',
'iranian-calendar-m8'  => 'aban',
'iranian-calendar-m9'  => 'azar',
'iranian-calendar-m10' => 'dï',
'iranian-calendar-m11' => 'bemin',
'iranian-calendar-m12' => 'aspand',

# Hebrew month names
'hebrew-calendar-m1'      => 'tişrï',
'hebrew-calendar-m2'      => 'xışwan',
'hebrew-calendar-m3'      => 'kislw',
'hebrew-calendar-m4'      => 'tot',
'hebrew-calendar-m5'      => 'şıbat',
'hebrew-calendar-m6'      => 'adar',
'hebrew-calendar-m6a'     => 'adar',
'hebrew-calendar-m6b'     => 'wadar',
'hebrew-calendar-m7'      => 'nïsan',
'hebrew-calendar-m8'      => 'ayar',
'hebrew-calendar-m9'      => 'sïwan',
'hebrew-calendar-m10'     => 'tımoz',
'hebrew-calendar-m11'     => 'ab',
'hebrew-calendar-m12'     => 'aýlol',
'hebrew-calendar-m1-gen'  => 'tişrïdiñ',
'hebrew-calendar-m2-gen'  => 'xışwandıñ',
'hebrew-calendar-m3-gen'  => 'kislwdiñ',
'hebrew-calendar-m4-gen'  => 'tottıñ',
'hebrew-calendar-m5-gen'  => 'şıbattıñ',
'hebrew-calendar-m6-gen'  => 'adardıñ',
'hebrew-calendar-m6a-gen' => 'adardıñ',
'hebrew-calendar-m6b-gen' => 'wadardıñ',
'hebrew-calendar-m7-gen'  => 'nïsannıñ',
'hebrew-calendar-m8-gen'  => 'ayardıñ',
'hebrew-calendar-m9-gen'  => 'sïwannıñ',
'hebrew-calendar-m10-gen' => 'tımozdıñ',
'hebrew-calendar-m11-gen' => 'abtıñ',
'hebrew-calendar-m12-gen' => 'aýloldıñ',

# Core parser functions
'unknown_extension_tag' => 'Tanılmağan keñeýtpe belgisi «$1»',

# Special:Version
'version'                          => 'Jüýe nusqası', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Ornatılğan keñeýtimder',
'version-specialpages'             => 'Arnaýı better',
'version-parserhooks'              => 'Qurılımdıq taldatqıştıñ tuzaqtarı',
'version-variables'                => 'Aýnımalılar',
'version-other'                    => 'Tağı basqalar',
'version-mediahandlers'            => 'Taspa öñdetkişteri',
'version-hooks'                    => 'Jete tuzaqtarı',
'version-extension-functions'      => 'Keñeýtimder jeteleri',
'version-parser-extensiontags'     => 'Qurılımdıq taldatqış keñeýtimderiniñ belgilemeri',
'version-parser-function-hooks'    => 'Qurılımdıq taldatqış jeteleriniñ tuzaqtarı',
'version-skin-extension-functions' => 'Mäner keñeýtimderiniñ jeteleri',
'version-hook-name'                => 'Tuzaq atawı',
'version-hook-subscribedby'        => 'Tuzaq tartqıştarı',
'version-version'                  => 'Nusqası:',
'version-license'                  => 'Lïcenzïyası',
'version-software'                 => 'Ornatılğan bağdarlamalıq jasaqtama',
'version-software-product'         => 'Önim',
'version-software-version'         => 'Nusqası',

# Special:FilePath
'filepath'         => 'Faýl ornalaswı',
'filepath-page'    => 'Faýl atı:',
'filepath-submit'  => 'Ornalaswın tap',
'filepath-summary' => 'Bul arnaýı bet faýl ornalaswı tolıq jolın qaýtaradı.
Swretter tolıq ajıratılımdığımen körsetiledi, basqa faýl türlerine qatıstı bağdarlaması twra jegiledi.

Faýl atawın «{{ns:image}}:» degen bastawışsız eñgiziñiz.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Faýl telnusqaların izdew',
'fileduplicatesearch-summary'  => 'Faýl xeşi mağınası negizinde telnusqaların izdew.

Faýl atawın «{{ns:image}}:» degen bastawışsız engiziñiz.',
'fileduplicatesearch-legend'   => 'Telnusqanı izdew',
'fileduplicatesearch-filename' => 'Faýl atawı:',
'fileduplicatesearch-submit'   => 'İzde',
'fileduplicatesearch-info'     => '$1 × $2 pïksel<br />Faýl mölşeri: $3<br />MIME türi: $4',
'fileduplicatesearch-result-1' => '«$1» faýlına teñ telnusqası joq.',
'fileduplicatesearch-result-n' => '«$1» faýlına teñ $2 telnusqası bar.',

# Special:SpecialPages
'specialpages'                   => 'Arnaýı better',
'specialpages-note'              => '----
* Kädimgi arnaýı better.
* <span class="mw-specialpagerestricted">Şektelgen arnaýı better.</span>',
'specialpages-group-maintenance' => 'Baptaw bayanattarı',
'specialpages-group-other'       => 'Tağı basqa arnaýı better',
'specialpages-group-login'       => 'Kirw / tirkelw',
'specialpages-group-changes'     => 'Jwıqtağı özgerister men jwrnaldar',
'specialpages-group-media'       => 'Taspa bayanattarı jäne qotarıp berilgender',
'specialpages-group-users'       => 'Qatıswşılar jäne olardıñ quqıqtarı',
'specialpages-group-highuse'     => 'Öte köp qoldanılğan better',
'specialpages-group-pages'       => 'Better tizimi',
'specialpages-group-pagetools'   => 'Kömekşi better',
'specialpages-group-wiki'        => 'Wïkï derekteri jäne quraldarı',
'specialpages-group-redirects'   => 'Aýdaýtın arnaýı better',
'specialpages-group-spam'        => 'Spam quraldarı',

);
