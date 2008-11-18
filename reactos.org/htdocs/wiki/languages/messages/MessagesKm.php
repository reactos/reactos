<?php
/** Khmer (ភាសាខ្មែរ)
 *
 * @ingroup Language
 * @file
 *
 * @author Bunly
 * @author Chhorran
 * @author Kiensvay
 * @author Lovekhmer
 * @author T-Rithy
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @author គីមស៊្រុន
 * @author តឹក ប៊ុនលី
 */

$digitTransformTable = array(
	'0' => '០', # &#x17e0;
	'1' => '១', # &#x17e1;
	'2' => '២', # &#x17e2;
	'3' => '៣', # &#x17e3;
	'4' => '៤', # &#x17e4;
	'5' => '៥', # &#x17e5;
	'6' => '៦', # &#x17e6;
	'7' => '៧', # &#x17e7;
	'8' => '៨', # &#x17e8;
	'9' => '៩', # &#x17e9;
);

$separatorTransformTable = array(
	'.' => ','
);

$datePreferences = array(
	'default',
	'km',
	'ISO 8601',
);
 
$defaultDateFormat = 'km';
 
$dateFormats = array(
	'km time' => 'ម៉ោងH:i',
	'km date' => 'l ទីd F ឆ្នាំY',
	'km both' =>  'ម៉ោងH:i l ទីd F ឆ្នាំY',
);

$specialPageAliases = array(
	'DoubleRedirects'         => array( 'ការបញ្ជូនបន្តទ្វេដង' ),
	'BrokenRedirects'         => array( 'ការបញ្ជូនបន្តដែលខូច' ),
	'Userlogin'               => array( 'ការឡុកអ៊ីននៃអ្នកប្រើប្រាស់' ),
	'Userlogout'              => array( 'ការចាកចេញរបស់អ្នកប្រើប្រាស់' ),
	'CreateAccount'           => array( 'បង្កើតគណនី' ),
	'Preferences'             => array( 'ចំណង់ចំណូលចិត្ត' ),
	'Watchlist'               => array( 'បញ្ជីតាមដាន' ),
	'Recentchanges'           => array( 'បំលាស់ប្តូរថ្មីៗ' ),
	'Upload'                  => array( 'ផ្ទុកឯកសារឡើង' ),
	'Imagelist'               => array( 'បញ្ជីរូបភាព' ),
	'Newimages'               => array( 'រូបភាពថ្មីៗ' ),
	'Listusers'               => array( 'បញ្ជីឈ្មោះអ្នកប្រើប្រាស់' ),
	'Statistics'              => array( 'ស្ថិតិ' ),
	'Randompage'              => array( 'ទំព័រចៃដន្យ' ),
	'Lonelypages'             => array( 'ទំព័រកំព្រា' ),
	'Uncategorizedpages'      => array( 'ទំព័រដែលគ្មានចំនាត់ថ្នាក់ក្រុម' ),
	'Uncategorizedcategories' => array( 'ចំនាត់ថ្នាក់ក្រុមដែលគ្មានចំនាត់ថ្នាក់ក្រុម' ),
	'Uncategorizedimages'     => array( 'រូបភាពដែលគ្មានចំនាត់ថ្នាក់ក្រុម' ),
	'Uncategorizedtemplates'  => array( 'ទំព័រគំរូដែលគ្មានចំនាត់ថ្នាក់ក្រុម' ),
	'Unusedcategories'        => array( 'ចំនាត់ថ្នាក់ក្រុមដែលមិនត្រូវបានប្រើប្រាស់' ),
	'Unusedimages'            => array( 'រូបភាពដែលមិនត្រូវបានប្រើប្រាស់' ),
	'Shortpages'              => array( 'ទំព័រខ្លីៗ' ),
	'Longpages'               => array( 'ទំព័រវែងៗ' ),
	'Newpages'                => array( 'ទំព័រថ្មីៗ' ),
	'Ancientpages'            => array( 'ទំព័រចាស់ៗ' ),
	'Protectedpages'          => array( 'ទំព័របានការពារ' ),
	'Protectedtitles'         => array( 'ចំនងជើងបានការពារ' ),
	'Allpages'                => array( 'គ្រប់ទំព័រ' ),
	'Prefixindex'             => array( 'លិបិក្រមបុព្វបទ' ),
	'Ipblocklist'             => array( 'បញ្ជីហាមឃាត់IP' ),
	'Specialpages'            => array( 'ទំព័រពិសេសៗ' ),
	'Contributions'           => array( 'ការរួមចំនែក' ),
	'Emailuser'               => array( 'អ្នកប្រើប្រាស់អ៊ីមែល' ),
	'Confirmemail'            => array( 'បញ្ជាក់ទទួលស្គាល់អ៊ីមែល' ),
	'Whatlinkshere'           => array( 'អ្វីដែលភ្ជាប់មកទីនេះ' ),
	'Movepage'                => array( 'ប្តូរទីតាំងទំព័រ' ),
	'Booksources'             => array( 'ប្រភពសៀវភៅ' ),
	'Categories'              => array( 'ចំនាត់ថ្នាក់ក្រុម' ),
	'Export'                  => array( 'នាំចេញ' ),
	'Version'                 => array( 'កំណែ' ),
	'Allmessages'             => array( 'គ្រប់សារ' ),
	'Log'                     => array( 'កំណត់ហេតុ' ),
	'Blockip'                 => array( 'ហាមឃាត់IP' ),
	'Undelete'                => array( 'ឈប់លុបចេញ' ),
	'Import'                  => array( 'នាំចូល' ),
	'Lockdb'                  => array( 'ចាក់សោមូលដ្ឋានទិន្នន័យ' ),
	'Unlockdb'                => array( 'ដោះសោមូលដ្ឋានទិន្នន័យ' ),
	'Userrights'              => array( 'សិទ្ធិអ្នកប្រើប្រាស់' ),
	'FileDuplicateSearch'     => array( 'ស្វែងរកឯកសារជាន់គ្នា' ),
	'Unwatchedpages'          => array( 'ទំព័រលែងបានតាមដាន' ),
	'Listredirects'           => array( 'បញ្ជីទំព័របញ្ជូនបន្ត' ),
	'Unusedtemplates'         => array( 'ទំព័រគំរូដែលមិនត្រូវបានប្រើប្រាស់' ),
	'Randomredirect'          => array( 'ការបញ្ជូនបន្តដោយចៃដន្យ' ),
	'Mypage'                  => array( 'ទំព័ររបស់ខ្ញុំ' ),
	'Mytalk'                  => array( 'ការពិភាក្សារបស់ខ្ញុំ' ),
	'Mycontributions'         => array( 'ការរួមចំនែករបស់ខ្ញុំ' ),
	'Listadmins'              => array( 'បញ្ជីអ្នកអភិបាល' ),
	'Listbots'                => array( 'បញ្ជីរូបយន្ត' ),
	'Popularpages'            => array( 'ទំព័រដែលមានប្រជាប្រិយ' ),
	'Search'                  => array( 'ស្វែងរក' ),
	'Resetpass'               => array( 'ដាក់ពាក្យសំងាត់ថ្មីឡើងវិញ' ),
	'Withoutinterwiki'        => array( 'ដោយគ្មានអន្តរវិគី' ),
	'MergeHistory'            => array( 'ច្របាច់បញ្ជូលប្រវត្តិ' ),
	'Filepath'                => array( 'ផ្លូវនៃឯកសារ' ),
	'Invalidateemail'         => array( 'អ៊ីមែលមិនត្រឹមត្រូវ' ),
	'Blankpage'               => array( 'ទំព័រទទេ' ),
);

$skinNames = array(
	'standard'    => 'បុរាណ',
	'nostalgia'   => 'អាឡោះអាល័យ',
	'cologneblue' => 'ទឹកអប់ខៀវ',
	'monobook'    => 'សៀវភៅឯក',
	'myskin'      => 'សំបកខ្ញុំ',
	'chick'       => 'កូនមាន់',
	'simple'      => 'សាមញ្ញ',
	'modern'      => 'ទំនើប',
);

$namespaceNames = array(
	NS_MEDIA          => 'មេឌា',
	NS_SPECIAL        => 'ពិសេស',
	NS_TALK           => 'ការពិភាក្សា',
	NS_USER           => 'អ្នកប្រើប្រាស់',
	NS_USER_TALK      => 'ការពិភាក្សារបស់អ្នកប្រើប្រាស់',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => 'ការពិភាក្សាអំពី$1',
	NS_IMAGE          => 'រូបភាព',
	NS_IMAGE_TALK     => 'ការពិភាក្សាអំពីរូបភាព',
	NS_MEDIAWIKI      => 'មេឌាវិគី',
	NS_MEDIAWIKI_TALK => 'ការពិភាក្សាអំពីមេឌាវិគី',
	NS_TEMPLATE       => 'ទំព័រគំរូ',
	NS_TEMPLATE_TALK  => 'ការពិភាក្សាអំពីទំព័រគំរូ',
	NS_HELP           => 'ជំនួយ',
	NS_HELP_TALK      => 'ការពិភាក្សាអំពីជំនួយ',
	NS_CATEGORY       => 'ចំណាត់ថ្នាក់ក្រុម',
	NS_CATEGORY_TALK  => 'ការពិភាក្សាអំពីចំណាត់ថ្នាក់ក្រុម',
);

$namespaceAliases = array(
	'មីឌា'           => NS_MEDIA,
	'ពិភាក្សា'         => NS_TALK,
	'អ្នកប្រើប្រាស់-ពិភាក្សា' => NS_USER_TALK,
	'$1_ពិភាក្ស'      => NS_PROJECT_TALK,
	'រូបភាព-ពិភាក្សា'     => NS_IMAGE_TALK,
	'មីឌាវិគី'         => NS_MEDIAWIKI,
	'មីឌាវិគី-ពិភាក្សា'    => NS_MEDIAWIKI_TALK,
	'ទំព័រគំរូ-ពិភាក្សា'   => NS_TEMPLATE_TALK,
	'ជំនួយ-ពិភាក្សា'     => NS_HELP_TALK,
	'ចំណាត់ក្រុម'       => NS_CATEGORY,
	'ចំណាត់ក្រុម-ពិភាក្សា'  => NS_CATEGORY_TALK,
);

$magicWords = array(
	'redirect'            => array( '0', '#REDIRECT', '#បញ្ជូនបន្ត', '#ប្តូរទីតាំងទៅ', '#ប្តូរទីតាំង' ),
	'notoc'               => array( '0', '__NOTOC__', '__លាក់មាតិកា__', '__លាក់បញ្ជីអត្ថបទ__', '__គ្មានមាតិកា__', '__គ្មានបញ្ជីអត្ថបទ__', '__កុំបង្ហាញមាតិកា__' ),
	'nogallery'           => array( '0', '__NOGALLERY__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__បង្ខំមាតិកា__', '__បង្ខំបញ្ជីអត្ថបទ__', '__បង្ខំអោយបង្ហាញមាតិកា__' ),
	'toc'                 => array( '0', '__TOC__', '__មាតិកា__', '__បញ្ជីអត្ថបទ__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__ផ្នែកមិនត្រូវកែប្រែ__', '__មិនមានផ្នែកកែប្រែ__', '__លាក់ផ្នែកកែប្រែ__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'ខែនេះ' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'ឈ្មោះខែនេះ' ),
	'currentmonthnamegen' => array( '1', 'CURRENTMONTHNAMEGEN' ),
	'currentmonthabbrev'  => array( '1', 'CURRENTMONTHABBREV' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'ថ្ងៃនេះ' ),
	'currentday2'         => array( '1', 'CURRENTDAY2' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'ឈ្មោះថ្ងៃនេះ' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'ឆ្នាំនេះ' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'ពេលនេះ' ),
	'currenthour'         => array( '1', 'CURRENTHOUR', 'ម៉ោងនេះ', 'ម៉ោងឥឡូវ' ),
	'localmonth'          => array( '1', 'LOCALMONTH' ),
	'localmonthname'      => array( '1', 'LOCALMONTHNAME' ),
	'localmonthnamegen'   => array( '1', 'LOCALMONTHNAMEGEN' ),
	'localmonthabbrev'    => array( '1', 'LOCALMONTHABBREV' ),
	'localday'            => array( '1', 'LOCALDAY' ),
	'localday2'           => array( '1', 'LOCALDAY2' ),
	'localdayname'        => array( '1', 'LOCALDAYNAME' ),
	'localyear'           => array( '1', 'LOCALDAYNAME' ),
	'localtime'           => array( '1', 'LOCALTIME', 'ពេលវេលាក្នុងតំបន់' ),
	'localhour'           => array( '1', 'LOCALHOUR', 'ម៉ោងតំបន់' ),
	'numberofpages'       => array( '1', 'NUMBEROFPAGES', 'ចំនួនទំព័រ' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'ចំនួនអត្ថបទ' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'ចំនួនឯកសារ' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'ចំនួនអ្នកប្រើប្រាស់' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'ចំនួនកំនែប្រែ' ),
	'pagename'            => array( '1', 'PAGENAME', 'ឈ្មោះទំព័រ' ),
	'pagenamee'           => array( '1', 'PAGENAMEE' ),
	'namespace'           => array( '1', 'NAMESPACE', 'លំហឈ្មោះ' ),
	'namespacee'          => array( '1', 'NAMESPACEE' ),
	'talkspace'           => array( '1', 'TALKSPACE', 'លំហឈ្មោះទំព័រពិភាក្សា' ),
	'talkspacee'          => array( '1', 'TALKSPACEE' ),
	'subjectspace'        => array( '1', 'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'       => array( '1', 'SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'        => array( '1', 'FULLPAGENAME', 'ឈ្មោះទំព័រពេញ' ),
	'fullpagenamee'       => array( '1', 'FULLPAGENAMEE' ),
	'subpagename'         => array( '1', 'SUBPAGENAME', 'ឈ្មោះទំព័ររង' ),
	'subpagenamee'        => array( '1', 'SUBPAGENAMEE' ),
	'basepagename'        => array( '1', 'BASEPAGENAME' ),
	'basepagenamee'       => array( '1', 'BASEPAGENAMEE' ),
	'talkpagename'        => array( '1', 'TALKPAGENAME', 'ឈ្មោះទំព័រពិភាក្សា' ),
	'talkpagenamee'       => array( '1', 'TALKPAGENAMEE' ),
	'subjectpagename'     => array( '1', 'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'    => array( '1', 'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                 => array( '0', 'MSG:', 'សារ:' ),
	'subst'               => array( '0', 'SUBST:' ),
	'msgnw'               => array( '0', 'MSGNW:', 'សារមិនមែនជាកូដវិគី:' ),
	'img_thumbnail'       => array( '1', 'thumbnail', 'thumb', 'រូបភាពតូច', 'រូបតូច' ),
	'img_manualthumb'     => array( '1', 'thumbnail=$1', 'thumb=$1', 'រូបភាពតូច=$1', 'រូបតូច=$1' ),
	'img_right'           => array( '1', 'right', 'ស្តាំ', 'ខាងស្តាំ' ),
	'img_left'            => array( '1', 'left', 'ធ្វេង', 'ខាងធ្វេង' ),
	'img_none'            => array( '1', 'none', 'ទទេ', 'គ្មាន' ),
	'img_width'           => array( '1', '$1px', '$1ភីកសែល', '$1ភស' ),
	'img_center'          => array( '1', 'center', 'centre', 'កណ្តាល' ),
	'img_framed'          => array( '1', 'framed', 'enframed', 'frame', 'ស៊ុម' ),
	'img_frameless'       => array( '1', 'frameless', 'គ្មានស៊ុម' ),
	'img_page'            => array( '1', 'page=$1', 'page $1', 'ទំព័រ=$1', 'ទំព័រ$1' ),
	'img_upright'         => array( '1', 'upright', 'upright=$1', 'upright $1' ),
	'img_border'          => array( '1', 'border' ),
	'img_baseline'        => array( '1', 'baseline' ),
	'img_sub'             => array( '1', 'sub' ),
	'img_super'           => array( '1', 'super', 'sup' ),
	'img_top'             => array( '1', 'top', 'ផ្នែកលើ', 'ផ្នែកខាងលើ' ),
	'img_text_top'        => array( '1', 'text-top', 'ឃ្លានៅផ្នែកខាងលើ', 'ឃ្លាផ្នែកខាងលើ' ),
	'img_middle'          => array( '1', 'middle', 'ផ្នែកកណ្តាល' ),
	'img_bottom'          => array( '1', 'bottom', 'បាត', 'ផ្នែកបាត' ),
	'img_text_bottom'     => array( '1', 'text-bottom', 'ឃ្លានៅផ្នែកបាត', 'ឃ្លាផ្នែកបាត' ),
	'int'                 => array( '0', 'INT:' ),
	'sitename'            => array( '1', 'SITENAME', 'ឈ្មោះវិបសាយ', 'ឈ្មោះគេហទំព័រ' ),
	'ns'                  => array( '0', 'NS:', 'លឈ:' ),
	'localurl'            => array( '0', 'LOCALURL:' ),
	'localurle'           => array( '0', 'LOCALURLE:' ),
	'server'              => array( '0', 'SERVER', 'ម៉ាស៊ីនបំរើសេវា' ),
	'servername'          => array( '0', 'SERVERNAME', 'ឈ្មោះម៉ាស៊ីនបំរើសេវា' ),
	'scriptpath'          => array( '0', 'SCRIPTPATH', 'ផ្លូវស្រ្គីប' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'វេយ្យាករណ៏:' ),
	'notitleconvert'      => array( '0', '__NOTITLECONVERT__', '__NOTC__' ),
	'nocontentconvert'    => array( '0', '__NOCONTENTCONVERT__', '__NOCC__' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'សប្តាហ៍នេះ' ),
	'currentdow'          => array( '1', 'CURRENTDOW' ),
	'localweek'           => array( '1', 'LOCALWEEK' ),
	'localdow'            => array( '1', 'LOCALDOW' ),
	'revisionid'          => array( '1', 'REVISIONID' ),
	'revisionday'         => array( '1', 'REVISIONDAY' ),
	'revisionday2'        => array( '1', 'REVISIONDAY2' ),
	'revisionmonth'       => array( '1', 'REVISIONMONTH' ),
	'revisionyear'        => array( '1', 'REVISIONYEAR' ),
	'revisiontimestamp'   => array( '1', 'REVISIONTIMESTAMP' ),
	'plural'              => array( '0', 'PLURAL:', 'ពហុវចនៈ:' ),
	'fullurl'             => array( '0', 'FULLURL:', 'URLពេញ:' ),
	'fullurle'            => array( '0', 'FULLURLE:' ),
	'lcfirst'             => array( '0', 'LCFIRST:' ),
	'ucfirst'             => array( '0', 'UCFIRST:' ),
	'lc'                  => array( '0', 'LC:' ),
	'uc'                  => array( '0', 'UC:' ),
	'raw'                 => array( '0', 'RAW:' ),
	'displaytitle'        => array( '1', 'DISPLAYTITLE', 'បង្ហាញចំណងជើង', 'បង្ហាញចំនងជើង' ),
	'rawsuffix'           => array( '1', 'R' ),
	'newsectionlink'      => array( '1', '__NEWSECTIONLINK__', '__តំនភ្ជាប់ផ្នែកថ្មី__', '__តំណភ្ជាប់ផ្នែកថ្មី__' ),
	'currentversion'      => array( '1', 'CURRENTVERSION' ),
	'urlencode'           => array( '0', 'URLENCODE:' ),
	'anchorencode'        => array( '0', 'ANCHORENCODE' ),
	'currenttimestamp'    => array( '1', 'CURRENTTIMESTAMP' ),
	'localtimestamp'      => array( '1', 'LOCALTIMESTAMP' ),
	'directionmark'       => array( '1', 'DIRECTIONMARK', 'DIRMARK' ),
	'language'            => array( '0', '#LANGUAGE:', '#ភាសា:' ),
	'contentlanguage'     => array( '1', 'CONTENTLANGUAGE', 'CONTENTLANG', 'កូដភាសា' ),
	'pagesinnamespace'    => array( '1', 'PAGESINNAMESPACE:', 'PAGESINNS:' ),
	'numberofadmins'      => array( '1', 'NUMBEROFADMINS', 'ចំនួនអ្នកអភិបាល', 'ចំនួនអ្នកថែទាំប្រព័ន្ធ' ),
	'formatnum'           => array( '0', 'FORMATNUM' ),
	'padleft'             => array( '0', 'PADLEFT' ),
	'padright'            => array( '0', 'PADRIGHT' ),
	'special'             => array( '0', 'special', 'ពិសេស' ),
	'defaultsort'         => array( '1', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:' ),
	'filepath'            => array( '0', 'FILEPATH:', 'ផ្លូវនៃឯកសារ:' ),
	'tag'                 => array( '0', 'tag', 'ផ្លាក', 'ស្លាក' ),
	'hiddencat'           => array( '1', '__HIDDENCAT__' ),
	'pagesincategory'     => array( '1', 'PAGESINCATEGORY', 'PAGESINCAT', 'ចំនួនទំព័រក្នុងចំនាត់ថ្នាក់ក្រុម', 'ចំនួនទំព័រក្នុងចំណាត់ថ្នាក់ក្រុម' ),
	'pagesize'            => array( '1', 'PAGESIZE', 'ទំហំទំព័រ' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'គូសបន្ទាត់ក្រោម​តំនភ្ជាប់៖',
'tog-highlightbroken'         => 'តភ្ជាប់​ទៅ​ទំព័រ​មិនទាន់មាន នឹង <a href="" class="new">ដូចនេះ</a> (បើមិនដូច្នោះ ៖ <a href="" class="internal">ដូចនេះ</a>)',
'tog-justify'                 => 'តំរឹម​កថាខណ្ឌ',
'tog-hideminor'               => 'បិទបាំង​កំនែប្រែតិចតួច​ក្នុងបញ្ជីបំលាស់ប្តូរថ្មីៗ',
'tog-extendwatchlist'         => 'ពង្រីក​បញ្ជីតាមដាន​ដើម្បីបង្ហាញ​គ្រប់បំលាស់ប្តូរ',
'tog-usenewrc'                => 'ធ្វើអោយ​បំលាស់ប្តូរ​ថ្មីៗ(JavaScript) កាន់តែប្រសើរឡើង',
'tog-numberheadings'          => 'បញ្ចូលលេខ​ចំនងជើងរង​ដោយស្វ័យប្រវត្តិ',
'tog-showtoolbar'             => 'បង្ហាញ​របារឧបករណ៍កែប្រែ (JavaScript)',
'tog-editondblclick'          => 'ចុចពីរដង​ដើម្បីកែប្រែទំព័រ (JavaScript)',
'tog-editsection'             => 'អនុញ្ញាតកែប្រែ​ផ្នែកណាមួយ​តាម​តំនភ្ជាប់[កែប្រែ]',
'tog-editsectiononrightclick' => 'អនុញ្ញាត​កែប្រែ​​ផ្នែកណាមួយ(JavaScript) ដោយ​ចុចស្តាំកណ្តុរ​លើ​ចំនងជើង​របស់វា',
'tog-showtoc'                 => 'បង្ហាញ​តារាងមាតិកា(ចំពោះទំព័រ​ដែលមាន​ចំនងជើងរង​លើសពី៣)',
'tog-rememberpassword'        => 'ចងចាំ​ការឡុកអ៊ីនរបស់ខ្ញុំ​លើកុំព្យូទ័រនេះ',
'tog-editwidth'               => 'បង្ហាញបង្អួចកែប្រែជាទទឹងពេញ',
'tog-watchcreations'          => 'បន្ថែម​ទំព័រ​ដែលខ្ញុំបង្កើត​ទៅ​បញ្ជីតាមដាន​របស់ខ្ញុំ',
'tog-watchdefault'            => 'បន្ថែម​ទំព័រ​ដែលខ្ញុំកែប្រែ​ទៅ​បញ្ជីតាមដាន​របស់ខ្ញុំ',
'tog-watchmoves'              => 'បន្ថែម​ទំព័រ​ដែលខ្ញុំប្តូរទីតាំង​ទៅ​បញ្ជីតាមដាន​របស់ខ្ញុំ',
'tog-watchdeletion'           => 'បន្ថែម​ទំព័រ​ដែលខ្ញុំលុបចេញ​ទៅ​បញ្ជីតាមដាន​របស់ខ្ញុំ',
'tog-minordefault'            => 'ចំនាំ​គ្រប់កំនែប្រែ​របស់ខ្ញុំ​ថាជា​តិចតួច',
'tog-previewontop'            => 'បង្ហាញ​ការមើលមុន​ពីលើ​ប្រអប់​កែប្រែ',
'tog-previewonfirst'          => 'បង្ហាញ​ការមើលមុន​ចំពោះ​កំនែប្រែ​ដំបូង',
'tog-nocache'                 => 'អសកម្ម​សតិភ្ជាប់​នៃ​ទំព័រ',
'tog-enotifwatchlistpages'    => 'សូមអ៊ីមែល​មកខ្ញុំ​កាលបើ​មានបំលាស់ប្តូរនៃទំព័រ​ណាមួយក្នុងបញ្ជីតាមដានរបស់ខ្ញុំ',
'tog-enotifusertalkpages'     => 'សូមអ៊ីមែល​មកខ្ញុំ​កាលបើ​មានបំលាស់ប្តូរ​នៅ​ទំព័រពិភាក្សា​របស់ខ្ញុំ',
'tog-enotifminoredits'        => 'សូមអ៊ីមែល​មកខ្ញុំ​ផងដែរ​ចំពោះ​បំលាស់ប្តូរតិចតួច​នៃ​ទំព័រនានា',
'tog-enotifrevealaddr'        => 'សូមបង្ហាញ​អាសយដ្ឋានអ៊ីមែល​របស់ខ្ញុំ​ក្នុង​​មែល​ក្រើនរំលឹក​នានា',
'tog-shownumberswatching'     => 'បង្ហាញ​ចំនួនអ្នកប្រើប្រាស់​ដែលតាមដាន​ទំព័រនេះ',
'tog-fancysig'                => 'ហត្ថលេខាឆៅ​ (គ្មានតំនភ្ជាប់​ស្វ័យប្រវត្តិ)',
'tog-externaleditor'          => 'ប្រើប្រាស់​ឧបករណ៍​កែប្រែខាងក្រៅ​តាមលំនាំដើម (សំរាប់តែអ្នកមានជំនាញប៉ុណ្ណោះ, ត្រូវការការកំនត់ពិសេសៗនៅលើកុំព្យូទ័ររបស់អ្នក)',
'tog-externaldiff'            => 'ប្រើប្រាស់​ឧបករណ៍​ប្រៀបធៀបខាងក្រៅ​តាមលំនាំដើម (សំរាប់តែអ្នកមានជំនាញប៉ុណ្ណោះ, ត្រូវការការកំនត់ពិសេសៗនៅលើកុំព្យូទ័ររបស់អ្នក)',
'tog-showjumplinks'           => 'សកម្មតំនភ្ជាប់ «ត្រាច់រក» និង «ស្វែងរក» នៅផ្នែកលើនៃទំព័រ(ចំពោះសំបក Myskin និងផ្សេងទៀត)',
'tog-uselivepreview'          => 'ប្រើប្រាស់​ការមើលមុនរហ័ស​(JavaScript) (ពិសោធ)',
'tog-forceeditsummary'        => 'សូមរំលឹកខ្ញុំ​កាលបើខ្ញុំទុកប្រអប់វិចារ​អោយទំនេរ',
'tog-watchlisthideown'        => 'បិទបាំង​កំនែប្រែ​របស់ខ្ញុំ​ពី​បញ្ជីតាមដាន',
'tog-watchlisthidebots'       => 'បិទបាំង​កំនែប្រែ​របស់​រូបយន្ត​ពី​បញ្ជីតាមដាន',
'tog-watchlisthideminor'      => 'បិទបាំង​កំនែប្រែតិចតួច​ពីបញ្ជីតាមដាន',
'tog-ccmeonemails'            => 'ផ្ញើមកខ្ញុំផងដែរនូវច្បាប់ចំលង​អ៊ីមែលដែលខ្ញុំផ្ញើទៅកាន់អ្នកប្រើប្រាស់ផ្សេងទៀត',
'tog-diffonly'                => 'សូមកុំបង្ហាញខ្លឹមសារទំព័រនៅពីក្រោមភាពខុសគ្នា',
'tog-showhiddencats'          => 'បង្ហាញចំនាត់ថ្នាក់ក្រុមដែលត្រូវបានបិទបាំង',

'underline-always'  => 'ជានិច្ច',
'underline-never'   => 'មិនដែលសោះ',
'underline-default' => 'តាមលំនាំដើម',

'skinpreview' => '(មើលជាមុន)',

# Dates
'sunday'        => 'ថ្ងៃអាទិត្យ',
'monday'        => 'ថ្ងៃច័ន្ទ',
'tuesday'       => 'ថ្ងៃអង្គារ',
'wednesday'     => 'ថ្ងៃពុធ',
'thursday'      => 'ថៃ្ងព្រហស្បតិ៍',
'friday'        => 'ថ្ងៃសុក្រ',
'saturday'      => 'ថ្ងៃសៅរ៍',
'sun'           => 'អាទិត្យ',
'mon'           => 'ច័ន្ទ',
'tue'           => 'អង្គារ',
'wed'           => 'ពុធ',
'thu'           => 'ព្រហស្បតិ៍',
'fri'           => 'សុក្រ',
'sat'           => 'សៅរ៍',
'january'       => 'ខែមករា',
'february'      => 'ខែកុម្ភៈ',
'march'         => 'ខែមិនា',
'april'         => 'ខែមេសា',
'may_long'      => 'ខែឧសភា',
'june'          => 'ខែមិថុនា',
'july'          => 'ខែកក្កដា',
'august'        => 'ខែសីហា',
'september'     => 'ខែកញ្ញា',
'october'       => 'ខែតុលា',
'november'      => 'ខែវិច្ឆិកា',
'december'      => 'ខែធ្នូ',
'january-gen'   => 'ខែមករា',
'february-gen'  => 'ខែកុម្ភៈ',
'march-gen'     => 'ខែមិនា',
'april-gen'     => 'ខែមេសា',
'may-gen'       => 'ខែឧសភា',
'june-gen'      => 'ខែមិថុនា',
'july-gen'      => 'ខែកក្កដា',
'august-gen'    => 'ខែសីហា',
'september-gen' => 'ខែកញ្ញា',
'october-gen'   => 'ខែតុលា',
'november-gen'  => 'ខែវិច្ឆិកា',
'december-gen'  => 'ខែធ្នូ',
'jan'           => 'មករា',
'feb'           => 'កុម្ភៈ',
'mar'           => 'មិនា',
'apr'           => 'មេសា',
'may'           => 'ឧសភា',
'jun'           => 'មិថុនា',
'jul'           => 'កក្កដា',
'aug'           => 'សីហា',
'sep'           => 'កញ្ញា',
'oct'           => 'តុលា',
'nov'           => 'វិច្ឆិកា',
'dec'           => 'ធ្នូ',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|ចំនាត់ថ្នាក់ក្រុម|ចំនាត់ថ្នាក់ក្រុម}}',
'category_header'                => 'ទំព័រដែលមាន​ក្នុងចំនាត់ថ្នាក់ក្រុម"$1"',
'subcategories'                  => 'ចំនាត់ថ្នាក់ក្រុមរង',
'category-media-header'          => 'ឯកសារមេឌា​ដែលមានក្នុង​ចំនាត់ថ្នាក់ក្រុម "$1"',
'category-empty'                 => "''ចំនាត់ថ្នាក់ក្រុមនេះ​មិនមានផ្ទុកអត្ថបទឬ​ឯកសារមេឌា​ណាមួយទេ។''",
'hidden-categories'              => '{{PLURAL:$1|ចំនាត់ថ្នាក់ក្រុមដែលត្រូវបានលាក់|ចំនាត់ថ្នាក់ក្រុមដែលត្រូវបានលាក់}}',
'hidden-category-category'       => 'ចំនាត់ថ្នាក់ក្រុមដែលត្រូវបានលាក់', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|ចំនាត់ថ្នាក់ក្រុមនេះមានតែចំនាត់ថ្នាក់ក្រុមរងដូចតទៅ។|ចំនាត់ថ្នាក់ក្រុមនេះមាន{{PLURAL:$1|ចំនាត់ថ្នាក់ក្រុមរង|$1 ចំនាត់ថ្នាក់ក្រុមរង}}ដូចតទៅ, ក្នុងចំនោមចំនាត់ថ្នាក់ក្រុមរងសរុប $2។}}',
'category-subcat-count-limited'  => 'ចំនាត់ថ្នាក់ក្រុមនេះមាន {{PLURAL:$1|ចំនាត់ថ្នាក់ក្រុមរង|$1 ចំនាត់ថ្នាក់ក្រុមរង}} ដូចតទៅ។',
'category-article-count'         => '{{PLURAL:$2|ចំនាត់ថ្នាក់ក្រុមនេះមានទំព័រដូចតទៅនេះ។|ទំព័រចំនួន {{PLURAL:$1|១ ទំព័រ|$1 ទំព័រ}} ក្នុងចំនោមទំព័រសរុប $2 ដូចតទៅនេះស្ថិតក្នុងចំនាត់ថ្នាក់ក្រុមនេះ។}}',
'category-article-count-limited' => 'អត្ថបទចំនួន {{PLURAL:$1|១ ទំព័រ|$1 ទំព័រ}} ដូចតទៅនេះស្ថិតនៅក្នុងចំនាត់ថ្នាក់ក្រុមនេះ។',
'category-file-count'            => '{{PLURAL:$2|ចំនាត់ថ្នាក់ក្រុមនេះមានឯកសារដូចទៅនេះ។|ឯកសារចំនួន {{PLURAL:$1|១ ឯកសារ|$1 ឯកសារ}} ក្នុងចំនោមឯកសារសរុប $2 ដូចតទៅនេះស្ថិតនៅក្នុងចំនាត់ថ្នាក់ក្រុមនេះ។}}',
'category-file-count-limited'    => 'ឯកសារ {{PLURAL:$1|១ ឯកសារ|$1 ឯកសារ}} ដូចតទៅនេះស្ថិតនៅក្នុងចំនាត់ថ្នាក់ក្រុមនេះ។',
'listingcontinuesabbrev'         => 'បន្ត',

'mainpagetext'      => "<big>'''មេឌាវិគីត្រូវបានតំលើងដោយជោគជ័យហើយ'''</big>",
'mainpagedocfooter' => 'សូមពិនិត្យមើល [http://meta.wikimedia.org/wiki/ជំនួយ៖ខ្លឹមសារណែនាំប្រើប្រាស់]សំរាប់ពត៌មានបន្ថែមចំពោះការប្រើប្រាស់ សូហ្វវែរវិគី។

== ចាប់ផ្តើមជាមួយមេឌាវិគី ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings បញ្ជីកំនត់ទំរង់]
* [http://www.mediawiki.org/wiki/Manual:FAQ/km សំនួរញឹកញាប់ MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce បញ្ជីពិភាក្សាការផ្សព្វផ្សាយរបស់ MediaWiki]',

'about'          => 'អំពី',
'article'        => 'មាតិកាអត្ថបទ',
'newwindow'      => '(បើកលើវ៉ីនដូថ្មី)',
'cancel'         => 'បោះបង់',
'qbfind'         => 'ស្វែងរក',
'qbbrowse'       => 'រាវរក',
'qbedit'         => 'កែប្រែ',
'qbpageoptions'  => 'ទំព័រនេះ',
'qbpageinfo'     => 'ពត៌មានទំព័រ',
'qbmyoptions'    => 'ទំព័ររបស់ខ្ញុំ',
'qbspecialpages' => 'ទំព័រពិសេសៗ',
'moredotdotdot'  => 'បន្ថែមទៀត...',
'mypage'         => 'ទំព័រ​របស់ខ្ញុំ',
'mytalk'         => 'ការពិភាក្សា​',
'anontalk'       => 'ពិភាក្សាចំពោះ IP នេះ',
'navigation'     => 'ទិសដៅ',
'and'            => 'និង',

# Metadata in edit box
'metadata_help' => 'ទិន្នន័យមេតា៖',

'errorpagetitle'    => 'កំហុស',
'returnto'          => "ត្រលប់ទៅ '''$1''' វិញ ។",
'tagline'           => 'ដោយ{{SITENAME}}',
'help'              => 'ជំនួយ',
'search'            => 'ស្វែងរក',
'searchbutton'      => 'ស្វែងរក',
'go'                => 'ទៅ',
'searcharticle'     => 'ទៅ',
'history'           => 'ប្រវត្តិទំព័រ',
'history_short'     => 'ប្រវត្តិ',
'updatedmarker'     => 'បានបន្ទាន់សម័យតាំងពីពេលចូលមើលចុងក្រោយរបស់ខ្ញុំ',
'info_short'        => 'ពត៌មាន',
'printableversion'  => 'ទំរង់​សំរាប់បោះពុម្ភ',
'permalink'         => 'តំនភ្ជាប់អចិន្ត្រៃយ៍',
'print'             => 'បោះពុម្ភ',
'edit'              => 'កែប្រែ',
'create'            => 'បង្កើត',
'editthispage'      => 'កែប្រែទំព័រនេះ',
'create-this-page'  => 'បង្កើតទំព័រនេះ',
'delete'            => 'លុបចេញ',
'deletethispage'    => 'លុបទំព័រនេះចេញ',
'undelete_short'    => 'ឈប់​លុបចេញ{{PLURAL:$1|១ កំនែប្រែ|$1 កំនែប្រែ}}',
'protect'           => 'ការពារ',
'protect_change'    => 'ផ្លាស់ប្តូរការការពារ',
'protectthispage'   => 'ការពារទំព័រនេះ',
'unprotect'         => 'ឈប់ការពារ',
'unprotectthispage' => 'ឈប់ការពារទំព័រនេះ',
'newpage'           => 'ទំព័រថ្មី',
'talkpage'          => 'ពិភាក្សាទំព័រនេះ',
'talkpagelinktext'  => 'ការពិភាក្សា',
'specialpage'       => 'ទំព័រពិសេស',
'personaltools'     => 'ឧបករណ៍ផ្ទាល់ខ្លួន',
'postcomment'       => 'ផ្តល់យោបល់',
'articlepage'       => 'មើលអត្ថបទ',
'talk'              => 'ការពិភាក្សា',
'views'             => 'ការមើលនានា',
'toolbox'           => 'ប្រអប់​ឧបករណ៍',
'userpage'          => 'មើលទំព័រអ្នកប្រើប្រាស់',
'projectpage'       => 'មើល​ទំព័រគំរោង',
'imagepage'         => 'មើលទំព័រមេឌា',
'mediawikipage'     => 'មើល​ទំព័រសារ',
'templatepage'      => 'មើលទំព័រគំរូ',
'viewhelppage'      => 'មើលទំព័រជំនួយ',
'categorypage'      => 'មើល​ទំព័រចំនាត់ថ្នាក់ក្រុម',
'viewtalkpage'      => 'មើលការពិភាក្សា',
'otherlanguages'    => 'ជាភាសាដទៃទៀត',
'redirectedfrom'    => '(ត្រូវបានបញ្ជូនបន្តពី $1)',
'redirectpagesub'   => 'ទំព័របញ្ជូនបន្ត',
'lastmodifiedat'    => 'ទំព័រនេះត្រូវបានកែចុងក្រោយនៅ$2 $1', # $1 date, $2 time
'viewcount'         => "ទំព័រនេះ​ត្រូវបានចូលមើល​ចំនួន'''{{PLURAL:$1|ម្ដង|$1ដង}}'''",
'protectedpage'     => 'ទំព័រដែលត្រូវបានការពារ',
'jumpto'            => 'ទៅកាន់៖',
'jumptonavigation'  => 'ទិសដៅ',
'jumptosearch'      => 'ស្វែងរក',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'អំពី{{SITENAME}}',
'aboutpage'            => 'Project:អំពី',
'bugreports'           => 'របាយការណ៍ដែលមានកំហុស',
'bugreportspage'       => 'Project:របាយការណ៍ដែលមានកំហុស',
'copyright'            => 'រក្សាសិទ្ធិគ្រប់យ៉ាងដោយ$1។',
'copyrightpagename'    => 'រក្សាសិទ្ធិដោយ{{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:រក្សាសិទ្ធិ​',
'currentevents'        => 'ព្រឹត្តិការណ៍​ថ្មីៗ',
'currentevents-url'    => 'Project:ព្រឹត្តិការណ៍​ថ្មីៗ',
'disclaimers'          => 'ការបដិសេធ',
'disclaimerpage'       => 'Project:ការបដិសេធ​ទូទៅ',
'edithelp'             => 'ជំនួយ​ក្នុងការកែប្រែ',
'edithelppage'         => 'Help:របៀបកែសំរួល',
'faq'                  => 'សំនួរដែលសួរញឹកញាប់',
'faqpage'              => 'Project:សំនួរដែលសួរញឹកញាប់',
'helppage'             => 'Help:មាតិកា',
'mainpage'             => 'ទំព័រដើម',
'mainpage-description' => 'ទំព័រដើម',
'policy-url'           => 'Project:គោលការណ៍',
'portal'               => 'ផតថលសហគមន៍',
'portal-url'           => 'Project:​ផតថលសហគមន៍',
'privacy'              => 'គោលការណ៍​ភាពឯកជន',
'privacypage'          => 'Project:គោលការណ៍ភាពឯកជន',

'badaccess'        => 'កំហុសនៃការអនុញ្ញាត',
'badaccess-group0' => 'សកម្មភាពដែលអ្នកបានស្នើមិនត្រូវបានអនុញ្ញាតទេ ។',
'badaccess-group1' => 'មានតែអ្នកប្រើប្រាស់ក្នុងក្រុម $1 ទើបអាចធ្វើសកម្មភាពដែលអ្នកបានស្នើ ។',
'badaccess-group2' => 'មានតែ​អ្នកប្រើប្រាស់​ក្នុងក្រុម១នៃក្រុម $1 ទេ ទើបអាចធ្វើសកម្មភាព​ដែលអ្នកបានស្នើ។',
'badaccess-groups' => 'មានតែ​អ្នកប្រើប្រាស់​ក្នុងក្រុម១នៃក្រុម $1 ទេ ​ទើបអាចធ្វើសកម្មភាព​ដែលអ្នកបានស្នើ។',

'versionrequired'     => 'តំរូវអោយមាន​កំនែ $1 នៃមេឌាវិគី',
'versionrequiredtext' => 'ត្រូវការកំនែ $1 នៃមេឌាវិគី (MediaWiki) ដើម្បីប្រើប្រាស់ទំព័រនេះ។ សូមមើល [[Special:Version|ទំព័រកំនែ]]។',

'ok'                      => 'យល់ព្រម',
'retrievedfrom'           => 'បានមកវិញពី "$1"',
'youhavenewmessages'      => 'អ្នកមាន $1 ($2)។',
'newmessageslink'         => 'សារថ្មីៗ',
'newmessagesdifflink'     => 'បំលាស់ប្តូរចុងក្រោយ',
'youhavenewmessagesmulti' => 'អ្នកមានសារថ្មីៗនៅ $1',
'editsection'             => 'កែប្រែ',
'editold'                 => 'កែប្រែ',
'viewsourceold'           => 'មើលកូដ',
'editsectionhint'         => "កែប្រែផ្នែក៖ '''$1'''",
'toc'                     => 'មាតិកា',
'showtoc'                 => 'បង្ហាញ',
'hidetoc'                 => 'លាក់',
'thisisdeleted'           => 'ចង់បង្ហាញ ឬ​ ទុក $1 នៅដដែល?',
'viewdeleted'             => 'មើល $1?',
'restorelink'             => '{{PLURAL:$1|កំនែប្រែត្រូវបានលុបចេញ|$1 កំនែប្រែត្រូវបានលុបចេញ}}',
'feedlinks'               => 'បំរែបំរួល៖',
'feed-invalid'            => 'ប្រភេទfeedដែលគ្មានសុពលភាព។',
'site-rss-feed'           => 'បំរែបំរួល RSS នៃ $1',
'site-atom-feed'          => 'បំរែបំរួល Atom នៃ $1',
'page-rss-feed'           => 'បំរែបំរួល RSS នៃ "$1"',
'page-atom-feed'          => 'បំរែបំរួល Atom Feed នៃ "$1"',
'red-link-title'          => '$1 (មិនទាន់​បានសរសេរ)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'អត្ថបទ',
'nstab-user'      => 'ទំព័រអ្នកប្រើប្រាស់',
'nstab-media'     => 'ទំព័រមេឌា',
'nstab-special'   => 'ពិសេស',
'nstab-project'   => 'ទំព័រគំរោង',
'nstab-image'     => 'ឯកសារ',
'nstab-mediawiki' => 'សារ',
'nstab-template'  => 'ទំព័រគំរូ',
'nstab-help'      => 'ទំព័រជំនួយ',
'nstab-category'  => 'ចំនាត់ថ្នាក់ក្រុម',

# Main script and global functions
'nosuchaction'      => 'មិនមានសកម្មភាពបែបនេះទេ',
'nosuchactiontext'  => 'សកម្មភាព​បានបង្ហាញដោយ URL មិនត្រូវបាន​ទទួលស្គាល់ដោយ​វិគី',
'nosuchspecialpage' => 'មិនមានទំព័រពិសេសបែបនេះទេ',
'nospecialpagetext' => "<big>'''អ្នកបានស្នើរក​ទំព័រពិសេសមួយ ដែលមិនត្រូវបាន​ទទួលស្គាល់​ដោយ វិគី។'''</big>

បញ្ជី​នៃ​ទំព័រពិសេស​អាចត្រូវបាន​រកឃើញ​នៅ [[Special:SpecialPages|{{int:specialpages}}]]។",

# General errors
'error'                => 'កំហុស',
'databaseerror'        => 'កំហុសមូលដ្ឋានទិន្នន័យ',
'noconnect'            => 'សូមអភ័យទោស! វិគី​កំពុង​មានបញ្ហាបច្ចេកទេសខ្លះៗ ហេតុនេះ​វាមិនអាច​ទាក់ទងទៅ​មូលដ្ឋានទិន្នន័យ​នាពេលនេះទេ។ <br />
$1',
'nodb'                 => 'មិនអាចជ្រើសយក​មូលដ្ឋានទិន្នន័យ $1',
'cachederror'          => 'ទំព័រនេះគឺជាកំនែប្រែមួយដែលឋិតនៅក្នុងការលាក់ទុក ហើយមិនអាចធ្វើឱ្យទាន់សម័យបានទេ។',
'laggedslavemode'      => 'ប្រយ័ត្ន៖ ទំព័រនេះ​អាចមិនមានផ្ទុក​បំលាស់ប្តូរ​ចុងក្រោយ​បំផុត។',
'readonly'             => 'មូលដ្ឋានទិន្នន័យត្រូវបានចាក់សោ',
'enterlockreason'      => 'សូមផ្ដល់ហេតុផលសំរាប់ការជាប់សោ រួមទាំងការប្រមាណថាតើការជាប់សោនោះនឹងត្រូវដោះនៅពេលណា',
'readonlytext'         => 'ពេលនេះ​មូលដ្ឋានទិន្នន័យ​កំពុងជាប់សោ ដើម្បីកុំអោយមាន​ការបញ្ចូល​ទិន្នន័យ​ថ្មីៗ​ឬ​ការកែប្រែ​ផ្សេងៗ។ នេះ​ប្រហែលដោយ​ហេតុផល​ថែទាំ​មូលដ្ឋានទិន្នន័យប្រចាំថ្ងៃ ដែលជាធម្មតាវានឹងវិលមកសភាពដើមវិញ​ក្នុងពេលឆាប់ៗ។
អ្នកអភិបាល​ដែលបានចាក់សោវា​បានពន្យល់ដូចតទៅ៖ $1',
'missingarticle-diff'  => '(ភាពខុសគ្នា: $1, $2)',
'readonly_lag'         => 'មូលដ្ឋានទិន្នន័យត្រូវបានចាក់សោដោយស្វ័យប្រវត្តិ ខណៈពេលដែលប្រព័ន្ឋបំរើការ(server)មូលដ្ឋានទិន្នន័យរងកំពុងទាក់ទងទៅប្រព័ន្ឋបំរើការមូលដ្ឋានទិន្នន័យមេ',
'internalerror'        => 'កំហុសផ្នែកខាងក្នុង',
'internalerror_info'   => 'កំហុសផ្នែកខាងក្នុង៖ $1',
'filecopyerror'        => 'មិនអាចចំលងឯកសារ"$1" ទៅ "$2"បានទេ។',
'filerenameerror'      => 'មិនអាចប្តូរឈ្មោះឯកសារពី"$1" ទៅ "$2"បានទេ។',
'filedeleteerror'      => 'មិនអាចលុបឯកសារ"$1"បានទេ។',
'directorycreateerror' => 'មិនអាចបង្កើតថត"$1"បានទេ។',
'filenotfound'         => 'រក​ឯកសារ "$1" មិនឃើញទេ។',
'fileexistserror'      => 'មិនអាចសរសេរ​ទៅក្នុង​ឯកសារ "$1"៖ ឯកសារមានរួចហើយ',
'unexpected'           => 'តំលៃ​មិនបានរំពឹងទុក៖ "$1"="$2"។',
'formerror'            => 'កំហុស៖ មិនអាចដាក់ស្នើ​សំនុំបែបបទ',
'badarticleerror'      => 'សកម្មភាពនេះ​មិនអាចត្រូវបានអនុវត្ត​លើទំព័រនេះទេ។',
'cannotdelete'         => 'មិនអាច​លុបចេញ ទំព័រ ឬ ឯកសារ ដែលបានសំដៅ។ វាអាច​ត្រូវបានលុបចេញហើយ​ដោយ​នរណាម្នាក់ផ្សេងទៀត។',
'badtitle'             => 'ចំនងជើង​មិនល្អ',
'badtitletext'         => 'ចំនងជើងទំព័រដែលបានស្នើ គ្មានសុពលភាព, ទទេ, ឬ ចំនងជើងតំនភ្ជាប់អន្តរភាសាឬអន្តរវិគី មិនត្រឹមត្រូវ ។ វាអាចមាន មួយ ឬ ច្រើនអក្សរ ដែលមិន​អាចប្រើប្រាស់​ក្នុង​ចំនងជើង។',
'perfdisabled'         => 'សូមអភ័យទោស!លក្ខណៈពិសេសនេះត្រូវបានបិទជាបណ្ណោះអាសន្ន ដោយហេតុថាវាធ្វើឱ្យមូលដ្ឋានទិន្នន័យធ្លាក់ចុះដល់កំរិតមួយដែលគ្មាននរណាម្នាក់អាចប្រើវីគីបាន។',
'perfcached'           => 'ទិន្នន័យទាំងនេះត្រូវបានដាក់ទៅសតិភ្ជាប់និងប្រហែលជាមិនទាន់សម័យ ។',
'perfcachedts'         => 'ទិន្នន័យ ទាំងនេះ ត្រូវបាន ដាក់ក្នុង សតិភ្ជាប់ និង បានត្រូវ បន្ទាន់សម័យ ចុងក្រោយ នៅ $1។',
'querypage-no-updates' => 'ការធ្វើឱ្យទាន់សម័យសំរាប់ទំព័រនេះគឺមិនអាចធ្វើទៅរួចទេនាពេលឥឡូវ។ ទិន្នន័យនៅទីនេះនឹងមិនត្រូវផ្លាស់ប្តូរថ្មីនាពេលបច្ចុប្បន្ន។',
'viewsource'           => 'មើល​កូដ',
'viewsourcefor'        => 'សំរាប់ $1',
'actionthrottled'      => 'សកម្មភាពត្រូវបានកំរិត',
'actionthrottledtext'  => 'ជាវិធានការប្រឆាំងស្ប៉ាម​(spam) អ្នកត្រូវបាន​គេកំហិតមិនអោយ​ធ្វើសកម្មភាពនេះ​ច្រើនដងពេកទេ​ក្នុងរយៈពេលខ្លីមួយ។

សូមព្យាយាមម្ដងទៀតក្នុងរយៈពេលប៉ុន្មាននាទីទៀត។',
'protectedpagetext'    => 'ទំព័រនេះបានត្រូវចាក់សោដើម្បីការពារមិនអោយកែប្រែ។',
'viewsourcetext'       => 'លោកអ្នកអាចមើលនិងចំលងកូដនៃទំព័រនេះ៖',
'protectedinterface'   => 'ទំព័រនេះ ផ្តល់នូវ អត្ថបទអន្តរមុខ សំរាប់ផ្នែកទន់, និង បានត្រូវចាក់សោ ដើម្បីចៀសវាង ការបំពាន ។',
'editinginterface'     => "'''សូមប្រយ័ត្ន៖''' អ្នកកំពុងតែកែប្រែទំព័រដែលបានប្រើប្រាស់​ដើម្បីផ្តល់នូវអន្តរមុខសំរាប់សូហ្វវែរ​។ បំលាស់ប្តូរចំពោះទំព័រនេះ​នឹងប៉ះពាល់ដល់ទំព័រអន្តរមុខនៃអ្នកប្រើប្រាស់​ជាច្រើន ដែលប្រើប្រាស់វិបសាយនេះ។ សំរាប់ការបកប្រែ សូមពិចារណាប្រើប្រាស់ [http://translatewiki.net/wiki/Main_Page?setlang=km Betawiki] (បេតាវិគី) គំរោង​អន្តរជាតូបនីយកម្ម​នៃមេឌាវិគី ។",
'sqlhidden'            => '(ការអង្កេត SQL ត្រូវបិទបាំង)',
'cascadeprotected'     => 'ទំព័រនេះត្រូវបានការពារពីការការប្រែដោយសារវាមាន{{PLURAL:$1|page, which is|pages, which are}} ដែលត្រូវបានការពារជាមួយ"cascading" option turned on:
$2',
'namespaceprotected'   => "អ្នកមិនមានសិទ្ធិកែប្រែទំព័រក្នុងលំហឈ្មោះ'''$1'''ទេ។",
'customcssjsprotected' => 'អ្នកមិនមាន​ការអនុញ្ញាត​ក្នុងការកែប្រែទំព័រនេះទេ ព្រោះវាផ្ទុកការកំនត់ផ្ទាល់ខ្លួនផ្សេងៗរបស់អ្នកប្រើប្រាស់ម្នាក់ផ្សេងទៀត។',
'ns-specialprotected'  => 'ទំព័រពិសេសៗមិនអាចកែប្រែបានទេ។',
'titleprotected'       => "ចំនងជើងនេះត្រូវបានការពារមិនអោយបង្កើត ដោយ [[User:$1|$1]]។
ហេតុផលលើកឡើងគឺ ''$2''។",

# Virus scanner
'virus-scanfailed'     => 'ការស្កេនបានបរាជ័យ (កូដ $1)',
'virus-unknownscanner' => 'កម្មវិធីកំចាត់មេរោគដែលមិនស្គាល់:',

# Login and logout pages
'logouttitle'                => 'ការចាកចេញរបស់អ្នកប្រើប្រាស់',
'logouttext'                 => "<strong>ឥឡូវនេះលោកអ្នកបានចាកចេញពីគណនីរបស់លោកអ្នកហើយ!</strong> 

អ្នកអាចបន្តប្រើប្រាស់{{SITENAME}}ក្នុងភាពអនាមិក ឬ [[Special:UserLogin|ឡុកអ៊ីនម្ដងទៀត]] ក្នុងនាមជាអ្នកប្រើប្រាស់ដដែលឬផ្សេងទៀត។

'''សំគាល់៖'''ទំព័រមួយចំនួនប្រហែលជានៅតែបង្ហាញលោកអ្នកនៅក្នុងភាពបានឡុកអ៊ីនចូលក្នុងគណនីរបស់លោកអ្នកដដែល។ ប្រសិនបើមានករណីនេះកើតឡើង សូមសំអាត ឃ្លាំងសំងាត់(Cache:ខាច់)នៃកម្មវិធីរុករករបស់លោកអ្នក។",
'welcomecreation'            => '== សូមស្វាគមន៍ $1! ==

គណនីរបស់អ្នកត្រូវបានបង្កើតហើយ។ 
កុំភ្លេចផ្លាស់ប្តូរ[[Special:Preferences|ចំនូលចិត្ត {{SITENAME}}]]របស់អ្នក។',
'loginpagetitle'             => 'ការឡុកអ៊ីនរបស់អ្នកប្រើប្រាស់',
'yourname'                   => 'ឈ្មោះអ្នកប្រើ៖',
'yourpassword'               => 'ពាក្យសំងាត់៖',
'yourpasswordagain'          => 'វាយពាក្យសំងាត់ម្តងទៀត៖',
'remembermypassword'         => 'ចងចាំការឡុកអ៊ីនរបស់ខ្ញុំក្នុងកុំព្យូទ័រនេះ',
'yourdomainname'             => 'ដែនរបស់អ្នក៖',
'loginproblem'               => '<b>មានបញ្ហា​ចំពោះការឡុកអ៊ីន​របស់អ្នក។</b><br />សូម​ព្យាយាមឡើងវិញ!',
'login'                      => 'ឡុកអ៊ីន',
'nav-login-createaccount'    => 'ឡុកអ៊ីនឬបង្កើតគណនី',
'loginprompt'                => 'អ្នកត្រូវតែមាន cookies ដើម្បីអាចឡុកអ៊ីនចូលទៅ {{SITENAME}}។',
'userlogin'                  => 'ឡុកអ៊ីនឬបង្កើតគណនី',
'logout'                     => 'ចាកចេញ',
'userlogout'                 => 'ចាកចេញ',
'notloggedin'                => 'មិនបានឡុកអ៊ីន',
'nologin'                    => 'តើលោកអ្នកមានគណនីសំរាប់ប្រើរឺទេ?  $1 ។',
'nologinlink'                => 'បង្កើតគណនី',
'createaccount'              => 'បង្កើតគណនី',
'gotaccount'                 => 'តើលោកអ្នកមានគណនីសំរាប់ប្រើរឺទេ?  $1។',
'gotaccountlink'             => 'ឡុកអ៊ីន',
'createaccountmail'          => 'តាមរយៈអ៊ីមែល',
'badretype'                  => 'ពាក្យសំងាត់ដែលអ្នកបានបញ្ចូលនោះ គឺមិនស៊ីគ្នាទេ។',
'userexists'                 => 'ឈ្មោះអ្នកប្រើប្រាស់ដែលបានវាយបញ្ចូល គឺឋិតនៅក្នុងការប្រើប្រាស់។ 
សូមជ្រើសរើសឈ្មោះដទៃទៀត។',
'youremail'                  => 'អ៊ីមែល៖',
'username'                   => 'ឈ្មោះអ្នកប្រើ៖',
'uid'                        => 'អត្តសញ្ញាណ៖',
'prefs-memberingroups'       => '{{PLURAL:$1|ក្រុម|ក្រុម}}សមាជិកភាព៖',
'yourrealname'               => 'ឈ្មោះពិត៖',
'yourlanguage'               => 'ភាសា៖',
'yournick'                   => 'ហត្ថលេខា៖',
'badsig'                     => 'ហត្ថលេខាឆៅមិនត្រឹមត្រូវ;ពិនិត្យមើលប្លាក HTML ។',
'badsiglength'               => 'ហត្ថលេខានេះវែងជ្រុល។

វាត្រូវតែតិចជាង $1 {{PLURAL:$1|អក្សរ|អក្សរ}}។',
'email'                      => 'អ៊ីមែល',
'prefs-help-realname'        => 'ការផ្តល់ឈ្មោះពិត​ជាជំរើសរបស់អ្នក។ បើអ្នកផ្តល់អោយ វានឹងត្រូវបានប្រើប្រាស់់ដើម្បីបញ្ជាក់ភាពជាម្ចាស់​លើការរួមចំនែក​នានា​របស់អ្នក។',
'loginerror'                 => 'កំហុសនៃការឡុកអ៊ីន',
'prefs-help-email'           => 'ការផ្ដល់អាសយដ្ឋានអ៊ីមែលឬមិនផ្ដល់ជាជំរើសរបស់អ្នក។ ប៉ុន្ដែវាអាចផ្តល់អោយពាក្យសំងាត់ត្រូវបានផ្ញើប្រសិនបើអ្នកភ្លេច។
អ្នកក៏អាចជ្រើសរើស​ការផ្ដល់លទ្ឋភាព​​អោយអ្នកដទៃទាក់ទងអ្នក​តាមរយៈ​​ទំព័រអ្នកប្រើប្រាស់​​ឬទំព័រពិភាក្សារបស់អ្នក​​ដោយមិនចាំបាច់អោយគេដឹងពីអត្តសញ្ញាណរបស់អ្នក។',
'prefs-help-email-required'  => 'អាសយដ្ឋានអ៊ីមែលត្រូវការជាចាំបាច់។',
'nocookiesnew'               => 'គណនីអ្នកប្រើប្រាស់ត្រូវបានបង្កើត ប៉ុន្តែអ្នកមិនត្រូវបានឡុកអ៊ីនទេ។ 
{{SITENAME}} ប្រើប្រាស់ cookies ដើម្បីឡុកអ៊ីន។
ប៉ុន្តែអ្នកបានអសកម្មពួកវា។
ចូរសកម្មពួកវាឡើងវិញ រួចឡុកអ៊ីនដោយប្រើឈ្មោះអ្នកប្រើប្រាស់ថ្មី  និង ពាក្យសំងាត់ថ្មីរបស់អ្នក។',
'nocookieslogin'             => '{{SITENAME}} ប្រើខូឃី(cookies)ដើម្បីឡុកអ៊ីន។ អ្នកមានខូឃីដែលមិនមានសុពលភាព។​ ចូរធ្វើឱ្យវាមានសុពលភាព រួចព្យាយាមម្តងទៀត។',
'noname'                     => 'អ្នកមិនបានកំណត់ត្រឹមត្រូវនូវឈ្មោះអ្នកប្រើប្រាស់ទេ។',
'loginsuccesstitle'          => 'ឡុកអ៊ីនដោយជោគជ័យ',
'loginsuccess'               => "'''ពេលនេះអ្នកត្រូវបានចូលទៅ{{SITENAME}}ជា \"\$1\"។'''",
'nosuchuser'                 => 'មិនមានអ្នកប្រើប្រាស់​ឈ្មោះ "$1" ទេ។ សូម​ពិនិត្យ​ក្រែង​លោ​មានកំហុស​អក្ខរាវិរុទ្ធឬ [[Special:Userlogin/signup|បង្កើត​គណនី​ថ្មី]]។',
'nosuchusershort'            => 'គ្មានអ្នកប្រើប្រាស់​ឈ្មោះ "$1" ទេ។ សូម​ពិនិត្យ​​អក្ខរាវិរុទ្ធ​របស់អ្នក ។',
'nouserspecified'            => 'អ្នកត្រូវតែ​បញ្ជាក់ឈ្មោះ​អ្នកប្រើប្រាស់។',
'wrongpassword'              => 'ពាក្យសំងាត់​ដែលបានបញ្ចូល​មិនត្រឹមត្រូវទេ។ សូមព្យាយាម​ម្តងទៀត។',
'wrongpasswordempty'         => 'ពាក្យសំងាត់ដែលបានបញ្ចូលមិនត្រូវ​ទេ។ សូមព្យាយាម​ម្តងទៀត។',
'passwordtooshort'           => 'ពាក្យសំងាត់របស់អ្នក មិនមានសុពលភាព ឬ​ ខ្លីពេក។ វាត្រូវមានយ៉ាងតិច $1 {{PLURAL:$1|1 អក្សរ|$1 អក្សរ}} និង ត្រូវផ្សេងពីឈ្មោះអ្នកប្រើប្រាស់របស់អ្នក។',
'mailmypassword'             => 'អ៊ីមែលពាក្យសំងាត់ថ្មី',
'passwordremindertitle'      => 'ពាក្យសំងាត់បណ្តោះអាសន្នថ្មីសំរាប់{{SITENAME}}',
'passwordremindertext'       => 'មានអ្នកណាម្នាក់ (ប្រហែលជាអ្នក) ពីអាស័យដ្ឋាន IP $1 បានស្នើសុំពាក្យសំងាត់ថ្មីមួយពី {{SITENAME}} ($4) ។
ពាក្យសំងាត់បណ្ដោះអាសន្នមួយសំរាប់អ្នកប្រើប្រាស់ "$2" ត្រូវបានប្ដូរទៅជា "$3" ។ បើសិនជានេះមិនមែន​ជាអ្វីដែលអ្នកចង់បានទេ សូមអ្នកឡុកអ៊ីន​ហើយជ្រើសរើសយកពាក្យសំងាត់ថ្មី។

បើមានអ្នកណាផ្សេងស្នើករណីនេះ ឬ បើអ្នកនឹកឃើញពាក្យសំងាត់ចាស់របស់អ្នក ហើយមិនចង់ផ្លាស់ប្តូរទេនោះ សូមអ្នកអាចបំភ្លេចសារនេះ ហើយបន្តប្រើប្រាស់ពាក្យសំងាត់ចាស់របស់អ្នក ។',
'noemail'                    => 'គ្មានអាសយដ្ឋានអ៊ីមែលត្រូវបានកត់ត្រាទុកសំរាប់អ្នកប្រើប្រាស់ "$1" ទេ។',
'passwordsent'               => 'ពាក្យសំងាត់ថ្មីត្រូវបានផ្ញើទៅអាសយដ្ឋានអ៊ីមែលដែលបានចុះបញ្ជីសំរាប់ "$1"។ 

សូមឡុកអ៊ីនម្តងទៀតបន្ទាប់ពីអ្នកបានទទួលវា។',
'blocked-mailpassword'       => 'អាសយដ្ឋានIPត្រូវបានហាមឃាត់ពីការកែប្រែ និងមិនអនុញ្ញាតអោយប្រើប្រាស់មុខងារសង្គ្រោះពាក្យសំងាត់ដើម្បីបង្ការការបំពាន។',
'eauthentsent'               => 'អ៊ីមែល​សំរាប់​ផ្ទៀងផ្ទាត់​ត្រូវបានផ្ញើទៅ​អាស័យដ្ឋានអ៊ីមែល​ដែលបានដាក់ឈ្មោះ។ មុននឹងមាន​អ៊ីមែលផ្សេងមួយទៀត​ត្រូវផ្ញើទៅ​គណនីនេះ អ្នកត្រូវតែ​តាមមើល​សេចក្តីណែនាំ​ក្នុងអ៊ីមែល​នេះ ដើម្បី បញ្ជាក់ថា​គណនីបច្ចុប្បន្ន​ពិតជា​របស់អ្នកពិតប្រាកដមែន។',
'throttled-mailpassword'     => 'អ៊ីមែលរំលឹកពាក្យសំងាត់ត្រូវបានផ្ញើទៅអោយអ្នកក្នុងអំឡុងពេល{{PLURAL:$1|ម៉ោង|$1ម៉ោង}}ចុងក្រោយនេះ។ 

ដើម្បីបង្ការអំពើបំពាន អ៊ីមែលរំលឹកពាក្យសំងាត់តែមួយគត់នឹងត្រូវបាន​ផ្ញើក្នុងរយៈពេល{{PLURAL:$1|ម៉ោង|$1ម៉ោង}}។',
'mailerror'                  => 'កំហុសនៃការផ្ញើសារ៖ $1',
'acct_creation_throttle_hit' => 'សូមអភ័យទោស, អ្នកបានបង្កើតគណនី $1 រួចហើយ ។ អ្នកមិនអាចធ្វើអ្វីបន្ថែមទៀតទេ​ ។',
'emailauthenticated'         => 'អាសយដ្ឋានអ៊ីមែលរបស់លោកអ្នកត្រូវបានបញ្ជាក់ថាត្រឹមត្រូវពិតប្រាកដនៅ  $1។',
'emailnotauthenticated'      => 'អាសយដ្ឋានអ៊ីមែលរបស់លោកអ្នក មិនទាន់ត្រូវបានបញ្ជាក់ថាត្រឹមត្រូវពិតប្រាកដនៅឡើយទេ។ មិនមានអ៊ីមែល ដែលនឹងត្រូវបានផ្ញើ សំរាប់មុខងារពិសេសណាមួយដូចខាងក្រោម។',
'noemailprefs'               => '<strong>បញ្ជាក់អាសយដ្ឋានអ៊ីមែលសំរាប់លក្ខណៈទាំងនេះដើម្បីធ្វើការ</strong> ។',
'emailconfirmlink'           => 'បញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែលរបស់អ្នក',
'invalidemailaddress'        => 'អាសយដ្ឋានអ៊ីមែល​នេះមិនអាចទទួលយកបានទេ​ដោយសារវាមានទំរង់​​មិនត្រឹមត្រូវ។ 

សូមបញ្ចូល​អាសយដ្ឋានមួយ​ដែលមាន​ទំរង់​ត្រឹមត្រូវ ឬមួយក៏ទុកវាលនោះអោយនៅទំនេរ​​។',
'accountcreated'             => 'គណនីរបស់លោកអ្នកត្រូវបានបង្កើតហើយ',
'accountcreatedtext'         => 'គណនី $1 ត្រូវបានបង្កើតហើយ។',
'createaccount-title'        => 'ការបង្កើតគណនីសំរាប់{{SITENAME}}',
'createaccount-text'         => 'មានអ្នកណាម្នាក់បានបង្កើតគណនីជាឈ្មោះ "$2" លើ{{SITENAME}}($4) ព្រមទាំងពាក្យសំងាត់ "$3" ។ អ្នកគួរតែឡុកអ៊ីនហើយផ្លាស់ប្តូរពាក្យសំងាត់របស់អ្នកនៅពេលនេះ។

អ្នកអាចរំលងសារនេះ ប្រសិនបើ​គណនីនេះត្រូវបានបង្កើតដោយមានបញ្ហា។',
'loginlanguagelabel'         => 'ភាសា៖ $1',

# Password reset dialog
'resetpass'               => 'បង្កើតពាក្យសំងាត់សារឡើងវិញ',
'resetpass_announce'      => 'អ្នកបានឡុកអ៊ីន​ដោយ​អក្សរកូដអ៊ីមែល​បណ្តោះអាសន្ន​មួយ​។ ដើម្បី​បញ្ចប់​ការឡុកអ៊ីន អ្នកត្រូវតែ​កំណត់​ពាក្យសំងាត់ថ្មី​មួយនៅទីនេះ ៖',
'resetpass_text'          => '<!-- បន្ថែមឃ្លានៅទីនេះ -->',
'resetpass_header'        => 'បង្កើតពាក្យសំងាត់សារឡើងវិញ',
'resetpass_submit'        => 'ដាក់ពាក្យសំងាត់និងឡុកអ៊ីន',
'resetpass_success'       => 'ពាក្យសំងាត់របស់អ្នកត្រូវបានផ្លាស់ប្តូរដោយជោគជ័យហើយ! ឥឡូវនេះកំពុងឡុកអ៊ីន...',
'resetpass_bad_temporary' => 'ពាក្យសំងាត់បណ្តោះអាសន្នមិនត្រឹមត្រូវទេ។ ប្រហែលជាអ្នកបានផ្លាស់ប្តូរពាក្យសំងាត់របស់អ្នករួចហើយ ឬបានស្នើពាក្យសំងាត់បណ្តោះអាសន្នថ្មីហើយ។',
'resetpass_forbidden'     => 'ពាក្យសំងាត់មិនអាចត្រូវបានផ្លាស់ប្តូរទេលើ{{SITENAME}}',
'resetpass_missing'       => 'ទិន្នន័យមិនត្រូវបានបញ្ចូលទេ។',

# Edit page toolbar
'bold_sample'     => 'អក្សរដិត',
'bold_tip'        => 'អក្សរដិត',
'italic_sample'   => 'អក្សរទ្រេត',
'italic_tip'      => 'អក្សរទ្រេត',
'link_sample'     => 'ចំនងជើង​តំនភ្ជាប់',
'link_tip'        => 'តំនភ្ជាប់​ខាងក្នុង',
'extlink_sample'  => 'http://www.example.com ចំនងជើង​តំនភ្ជាប់',
'extlink_tip'     => 'តំនភ្ជាប់​ខាងក្រៅ (កុំភ្លេច​ដាក់ http:// នៅពីមុខ)',
'headline_sample' => 'ចំនងជើងរងនៃអត្ថបទ',
'headline_tip'    => 'ចំនងជើងរង​កំរិត​២',
'math_sample'     => 'បញ្ចូលរូបមន្ត​នៅទីនេះ',
'math_tip'        => 'រូបមន្ត​គណិតវិទ្យា(LaTeX)',
'nowiki_sample'   => 'បញ្ចូល​អត្ថបទគ្មានទំរង់​នៅទីនេះ',
'nowiki_tip'      => 'មិនគិត​ទំរង់​នៃ​វិគី',
'image_sample'    => 'ឧទាហរណ៍.jpg',
'image_tip'       => 'រូបភាពបង្កប់',
'media_sample'    => 'ឧទាហរណ៍.ogg',
'media_tip'       => 'តំនភ្ជាប់ឯកសារ',
'sig_tip'         => 'ហត្ថលេខា​របស់អ្នកជាមួយនឹងកាលបរិច្ឆេទ',
'hr_tip'          => 'បន្ទាត់ដេក (មិនសូវប្រើទេ)',

# Edit pages
'summary'                          => 'សេចក្តីសង្ខេប',
'subject'                          => 'ប្រធានបទ/ចំនងជើងរង',
'minoredit'                        => 'នេះជា​កំនែប្រែតិចតួចប៉ុណ្ណោះ',
'watchthis'                        => 'តាមដាន​ទំព័រនេះ',
'savearticle'                      => 'រក្សាទំព័រទុក',
'preview'                          => 'មើលជាមុន',
'showpreview'                      => 'បង្ហាញ​ការមើលជាមុន',
'showlivepreview'                  => 'មើលជាមុនដោយផ្ទាល់',
'showdiff'                         => 'បង្ហាញ​បំលាស់ប្តូរ',
'anoneditwarning'                  => "'''ប្រយ័ត្ន ៖''' អ្នកមិនទាន់បានឡុកអ៊ីន​ទេ។ អាសយដ្ឋាន IP របស់អ្នក​នឹងត្រូវបាន​កត់ត្រាទុក​ក្នុងប្រវត្តិកែប្រែ​នៃទំព័រ​នេះ។",
'missingsummary'                   => "'''រំលឹក៖''' អ្នកមិនទាន់បានផ្ដល់អោយនូវសេចក្ដីសង្ខេបអំពីកំនែប្រែទេ។
បើសិនជាអ្នកចុច '''រក្សាទុក''' ម្ដងទៀតនោះកំនែប្រែរបស់អ្នកនឹងត្រូវរក្សាទុកដោយគ្មានវា។",
'missingcommenttext'               => 'សូមបញ្ចូលមួយវិចារនៅខាងក្រោម។',
'missingcommentheader'             => "'''រំលឹក៖''' អ្នកមិនទាន់បានផ្ដល់អោយនូវ ប្រធានបទ/ចំនងជើង របស់វិចារនេះទេ។
បើសិនជាអ្នកចុច '''រក្សាទុក''' ម្ដងទៀតនោះកំនែប្រែរបស់អ្នកនឹងត្រូវរក្សាទុកដោយគ្មានវា។",
'summary-preview'                  => 'ការមើលជាមុននូវសេចក្តីសង្ខេប',
'subject-preview'                  => 'ការមើលជាមុននូវប្រធានបទ/ចំនងជើង',
'blockedtitle'                     => 'អ្នកប្រើប្រាស់ត្រូវបានហាមឃាត់',
'blockedtext'                      => '<big>\'\'\'ឈ្មោះគណនី (ឬអាសយដ្ឋាន IP)របស់អ្នកត្រូវបានហាមឃាត់ហើយ។\'\'\'</big>

ការហាមឃាត់ត្រូវបានធ្វើដោយ $1 

ដោយសំអាងលើហេតុផល \'\'$2\'\'។


* ចាប់ផ្តើមការហាមឃាត់ ៖ $8
* ផុតកំនត់ការហាមឃាត់ ៖ $6
* គណនីបាននឹងត្រូវពន្យាការហាមឃាត់់ ៖ $7


អ្នកអាចទាក់ទង $1 ឬ [[{{MediaWiki:Grouppage-sysop}}|អ្នកអភិបាល]]ដទៃទៀតដើម្បីពិភាក្សាពីការហាមឃាត់់នេះ ។

អ្នកមិនអាចប្រើប្រាស់មុខងារ "អ៊ីមែលទៅអ្នកប្រើប្រាស់នេះ" បានទេ លើកលែងអាសយដ្ឋានអ៊ីមែលមានសុពលភាពមួយ​ត្រូវបានបញ្ជាក់​ក្នុង[[Special:Preferences|ចំនូលចិត្តនានានៃគណនី]]របស់លោកអ្នកហើយលោកអ្នកមិនត្រូវបានគេហាមឃាត់មិនអោយប្រើប្រាស់មុខងារនោះ។

អាសយដ្ឋាន IP បច្ចុប្បន្នរបស់លោកអ្នកគឺ $3 និងអត្តសញ្ញាណរាំងខ្ទប់គឺ  #$5 ។ សូមបញ្ចូលអាសយដ្ឋានទាំងនេះសំរាប់គ្រប់សំនួរអង្កេត។',
'autoblockedtext'                  => 'អាសយដ្ឋានIPរបស់អ្នកបានត្រូវរាំងខ្ទប់ដោយស្វ័យប្រវត្តិ ព្រោះវាត្រូវបានប្រើប្រាស់ដោយអ្នកប្រើប្រាស់ផ្សេងទៀត​ ដែលត្រូវបានរាំងខ្ទប់ដោយ $1 ។ 

មូលហេតុលើកឡើង៖

:\'\'$2\'\'

* ការចាប់ផ្តើមហាមឃាត់៖ $8
* ពេលផុតកំណត់ហាមឃាត់៖ $6
* អ្នកដែលត្រូវរាំងខ្ទប់៖ $7

អ្នកអាចទាក់ទង $1 ឬ[[{{MediaWiki:Grouppage-sysop}}|អ្នកអភិបាល]]ណាម្នាក់ ដើម្បីពិភាក្សាអំពីការរាំងខ្ទប់នេះ។

សូមកត់សំគាល់ថាអ្នកមិនអាចប្រើប្រាស់មុខងារ"អ៊ីមែលអ្នកប្រើប្រាស់នេះ"បានទេ លុះត្រាតែមានមួយអាសយដ្ឋានអ៊ីមែលដែលមានសុពលភាព បានចុះឈ្មោះ ក្នុង
[[Special:Preferences|ចំនូលចិត្ត]]របស់អ្នក ហើយអ្នកមិនត្រូវបានរាំងខ្ទប់មិនអោយប្រើប្រាស់មុខងារនោះ ។

អាសយដ្ឋាន IP បច្ចុប្បន្នរបស់អ្នកគឺ $3។ ID ដែលត្រូវបានរាំងខ្ទប់គឺ #$5។
សូមបញ្ចូលពត៌មានលំអិតខាងលើនេះ ក្នុងគ្រប់សំនួរអង្កេតដែលអ្នកបានបង្កើត។',
'blockednoreason'                  => 'គ្មានហេតុផល​ត្រូវបានលើកឡើង',
'blockedoriginalsource'            => "កូដនៃទំព័រ '''$1''' ត្រូវបានបង្ហាញដូចខាងក្រោម៖",
'blockededitsource'                => "ខ្លឹមសារ​នៃ '''កំនែប្រែ​របស់អ្នក''' ចំពោះ '''$1''' ត្រូវបាន​បង្ហាញ​ខាងក្រោម ៖",
'whitelistedittitle'               => 'តំរូវអោយឡុកអ៊ីនដើម្បីកែប្រែ',
'whitelistedittext'                => 'អ្នកត្រូវតែជា $1 ដើម្បី​កែប្រែ​ខ្លឹមសារទំព័រ។',
'confirmedittitle'                 => 'តំរូវអោយ​បញ្ជាក់ទទួលស្គាល់​អ៊ីមែល ដើម្បីកែប្រែ',
'confirmedittext'                  => 'អ្នកត្រូវតែបញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែលរបស់អ្នកមុននឹងកែប្រែខ្លឹមសារអត្ថបទ។ ចូរកំនត់និងផ្តល់សុពលភាពអោយអាសយដ្ឋានអ៊ីមែល របស់អ្នកតាម [[Special:Preferences|ចំនូលចិត្តនានារបស់អ្នកប្រើប្រាស់]] ។',
'nosuchsectiontitle'               => 'មិនមានផ្នែក​បែបនេះ',
'nosuchsectiontext'                => 'អ្នកបាន​ព្យាយាម​កែប្រែផ្នែក​មួយ​ដែលមិនទាន់មាន​នៅឡើយ ។  ដោយហេតុថា​មិនមាន​ផ្នែក $1 ម៉្លោះហើយ​គ្មានកន្លែង​សំរាប់​រក្សាទុក​កំនែប្រែ​របស់អ្នកទេ ។',
'loginreqtitle'                    => 'តំរូវអោយឡុកអ៊ីន',
'loginreqlink'                     => 'ឡុកអ៊ីន',
'loginreqpagetext'                 => 'អ្នកត្រូវតែ$1ដើម្បីមើលទំព័រដទៃផ្សេងទៀត។',
'accmailtitle'                     => 'ពាក្យសំងាត់ត្រូវបានផ្ញើរួចហើយ។',
'accmailtext'                      => 'ពាក្យសំងាត់​របស់ "$1" ត្រូវបានផ្ញើទៅ $2 ហើយ។',
'newarticle'                       => '(ថ្មី)',
'newarticletext'                   => "អ្នកបានតាម​តំនភ្ជាប់​ទៅ​ទំព័រដែលមិនទាន់មាននៅឡើយ។
ដើម្បីបង្កើតទំព័រនេះ សូមចាប់ផ្តើមវាយ​ក្នុងប្រអប់ខាងក្រោម (សូមមើល [[{{MediaWiki:Helppage}}|ទំព័រ​ជំនួយ]] សំរាប់​ពត៌មានបន្ថែម)។
បើ​អ្នកមក​ទីនេះ​ដោយច្រឡំ​ សូមចុចប៊ូតុង '''ត្រលប់ក្រោយ''' របស់ឧបករណ៍រាវរក(browser)​របស់អ្នក។",
'anontalkpagetext'                 => "----''ទំព័រពិភាក្សានេះគឺសំរាប់តែអ្នកប្រើប្រាស់អនាមិកដែលមិនទាន់បានបង្កើតគណនីតែប៉ុណ្ណោះ។ ដូច្នេះអាសយដ្ឋានលេខIPរបស់កុំព្យូទ័ររបស់លោកអ្នក​នឹងត្រូវបានបង្ហាញ ដើមី្បសំគាល់លោកអ្នក។ 

អាសយដ្ឋានIPទាំងនោះអាចនឹងត្រូវប្រើដោយមនុស្សច្រើននាក់។ 

ប្រសិនបើអ្នកជាអ្នកប្រើប្រាស់អនាមិក​ហើយ​ប្រសិនបើអ្នកឃើញមានការបញ្ចេញយោបល់​ដែល​មិន​ទាក់ទងទៅនឹងអ្វីដែល​អ្នកបាន​ធ្វើ​ សូម[[Special:UserLogin|ចូលឬបង្កើតគណនី]] ឬ [[Special:UserLogin|ឡុកអ៊ីន]] ដើម្បីចៀសវាង​ការភ័នច្រឡំ​ណាមួយជាយថាហេតុជាមួយនិងអ្នកប្រើប្រាស់អនាមិកដទៃទៀត។
''",
'noarticletext'                    => 'បច្ចុប្បន្ន គ្មានអត្ថបទណាមួយក្នុងទំព័រនេះទេ។ អ្នកអាច [[Special:Search/{{PAGENAME}}|ស្វែងរក​ចំនងជើង​នៃទំព័រនេះ]] ក្នុងទំព័រ​ផ្សេង​ ឬ [{{fullurl:{{FULLPAGENAME}}|action=edit}} កែប្រែ​ទំព័រនេះ]។',
'userpage-userdoesnotexist'        => 'គណនីអ្នកប្រើប្រាស់ "$1" មិនបានត្រូវ ចុះបញ្ជី ។ ចូរឆែកមើល តើ អ្នកចង់ បង្កើត / កែប្រែ ទំព័រ នេះ ។',
'clearyourcache'                   => "'''សំគាល់:''' បន្ទាប់ពីបានរក្សាទុករួចហើយ លោកអ្នកគួរតែសំអាត browser's cache របស់លោកអ្នកដើម្បីមើលការផ្លាស់ប្តូរ។ ខាងក្រោមនេះជាវិធីសំអាត browser's cache ចំពោះកម្មវិធីរុករក(Browser)មួយចំនួន។
* '''Mozilla / Firefox / Safari:''' សង្កត់ [Shift] អោយជាប់រួចចុចប៊ូតុង ''Reload'' ឬក៏ចុច  ''Ctrl-F5'' ឬ ''Ctrl-R'' (ចំពោះApple Mac វិញ​ចុច ''Command-R'') ។
* '''IE(Internet Explorer):''' សង្កត់ [Ctrl] អោយជាប់ រួចចុច ''Refresh''ប៊ូតុង ឬក៏ចុច ''Ctrl-F5'' ។ 
* '''Konqueror:''' ចុចប៊ូតុង  ''Reload'' ឬក៏ចុច ''F5'' 
* '''Opera:''' សូមចុច  ''[Tools]→[Preferences]'' ។",
'usercssjsyoucanpreview'           => "<strong>គន្លឹះ ៖ </strong> សូមប្រើប្រាស់​ប្រអប់ 'បង្ហាញការមើលមុន' ដើម្បី​ធ្វើតេស្ត​សន្លឹក CSS/JS ថ្មីរបស់អ្នក​មុននឹង​រក្សាទុកវា ។",
'usercsspreview'                   => "'''កុំភ្លេចថា​អ្នកគ្រាន់តែ​កំពុងមើលជាមុនសន្លឹក CSS របស់អ្នក។ 
វាមិនទាន់​ត្រូវបានរក្សាទុកទេ!'''",
'userjspreview'                    => "'កុំភ្លេចថាអ្នកគ្រាន់តែកំពុង ធ្វើតេស្ត/មើលមុន ទំព័រអ្នកប្រើប្រាស់  JavaScript របស់អ្នក។ វាមិនទាន់ត្រូវបានរក្សាទុកទេ!'''",
'userinvalidcssjstitle'            => "'''ប្រយ័ត្ន៖''' គ្មានសំបក \"\$1\"។ ចងចាំថា ទំព័រផ្ទាល់ខ្លួន .css និង .js ប្រើប្រាស់ ចំណងជើង ជាអក្សរតូច, ឧទាហរ  {{ns:user}}:Foo/monobook.css ត្រឹមត្រូវ, រីឯ {{ns:user}}:Foo/Monobook.css មិនត្រឹមត្រូវ។",
'updated'                          => '(បានបន្ទាន់សម័យ)',
'note'                             => '<strong>ចំនាំ៖</strong>',
'previewnote'                      => '<strong>នេះគ្រាន់តែជា​ការបង្ហាញការមើលជាមុនប៉ុណ្ណោះ។ បំលាស់ប្តូរ​មិនទាន់បាន​រក្សាទុកទេ!</strong>',
'previewconflict'                  => 'ការមើលមុននេះយោងតាមអត្ថបទក្នុងប្រអប់កែប្រែខាងលើ។ ទំព័រអត្ថបទនឹងបង្ហាញចេញបែបនេះប្រសិនបើអ្នកជ្រើសរើសរក្សាទុក។',
'session_fail_preview'             => '<strong>សូមអភ័យទោស! យើងមិនអាចរក្សាទុកការកែប្រែរបស់អ្នកបានទេ ដោយសារបាត់ទិន្នន័យវេនការងារ។

សូមព្យាយាមម្តងទៀត។ 

បើនៅតែមិនបានទេ សូមព្យាយាម[[Special:UserLogout|ចាកចេញពីគណនីរបស់អ្នក]] រួចឡុកអ៊ីនឡើងវិញ។</strong>',
'session_fail_preview_html'        => "<strong>សូមអភ័យទោស! យើងមិនអាចរក្សាទុកកំនែប្រែរបស់លោកអ្នកបានទេ ដោយសារបាត់ទិន្នន័យវេនការងារ។</strong>

''ដោយសារ {{SITENAME}} មានអក្សរកូដ HTMLឆៅ ត្រូវបានបើកអោយប្រើប្រាស់ ហេតុនេះទំព័រមើលមុនត្រូវបានបិទបាំង ដើម្បីចៀសវាងការវាយលុកដោយ JavaScript ។''

<strong>បើនេះជាការប៉ុនប៉ងកែប្រែសមស្រប សូមព្យាយាមម្តងទៀត។ 

បើនៅតែមិនបានទេ សូមព្យាយាម[[Special:UserLogout|ចាកចេញពីគណនីរបស់អ្នក]] រួចឡុកអ៊ីនឡើងវិញ។</strong>",
'editing'                          => 'កំពុងកែប្រែ​ $1',
'editingsection'                   => "កំពុងកែប្រែ'''$1'''(ផ្នែក)",
'editingcomment'                   => 'កែប្រែ $1 (យោបល់)',
'editconflict'                     => 'ភាពឆ្គងនៃកំនែប្រែ៖ $1',
'explainconflict'                  => 'ចាប់តាំងពីអ្នកបានបង្កើតទំព័រនេះមក មានអ្នកដទៃបានកែប្រែវាហើយ។ ផ្នែកខាងលើនៃទំព័រអត្ថបទ គឺជាកំនែប្រែថ្មី។ កំនែប្រែរបស់អ្នក គឺនៅផ្នែកខាងក្រោម។ ចូរដាក់កំនែប្រែរបស់អ្នកបញ្ចូលគ្នាជាមួយអត្ថបទដែលមាននៅផ្នែកខាងលើ។​ <strong>អត្ថបទនៅផ្នែកខខាងលើ</strong> នឹងត្រូវរក្សាទុក នៅពេលអ្នក ចុច"រក្សាទំព័រ"។',
'yourtext'                         => 'អត្ថបទរបស់អ្នក',
'storedversion'                    => 'កំណែដែលបានស្តារឡើងវិញ',
'editingold'                       => '<strong>បំរាម:អ្នកកំពុងតែកែកំនែប្រែដែលហួសសម័យរបស់ទំព័រនេះ។

ប្រសិនបើអ្នករក្សាវាទុក កំនែប្រែពីមុនទាំងប៉ុន្មាននឹងត្រូវបាត់បង់។</strong>',
'yourdiff'                         => 'ភាពខុសគ្នា',
'copyrightwarning'                 => 'សូមធ្វើការកត់សំគាល់​ថា គ្រប់ការរួមចំនែក​របស់អ្នក​នៅលើ {{SITENAME}} ត្រូវបាន​ពិចារណា​ដើម្បី​ផ្សព្វផ្សាយ​តាម​លិខិតអនុញ្ញាត $2 (សូម​មើល $1 សំរាប់​ពត៌មាន​លំអិត) ។ បើអ្នកមិនចង់អោយ​សំនេរ​របស់អ្នក​ត្រូវបានគេលុប កែប្រែ ឬក៏អ្នកមិនមានបំនងផ្សព្វផ្សាយវា សូមកុំដាក់​ស្នើវា​នៅទីនេះអី។<br />
អ្នកត្រូវសន្យាថា ​អ្នកសរសេរវា​ដោយខ្លួនអ្នក ឬបានចំលងវា​ពី​កម្មសិទ្ធិសាធារណៈឬពីប្រភពសេរី ។
<strong>មិនត្រូវ​ដាក់ស្នើ​ការងារមានរក្សាសិទ្ធិកម្មសិទ្ឋិបញ្ញាដោយគ្មានការអនុញ្ញាតទេ!</strong>',
'copyrightwarning2'                => 'សូមធ្វើការកត់សំគាល់​ថា គ្រប់ការរួមចំនែក​ទៅ {{SITENAME}} អាច​ត្រូវបាន​កែប្រែ​ ផ្លាស់ប្តូរ រឺលុបចោល ដោយអ្នករួមចំនែកដទៃទៀត។ បើអ្នកមិនចង់អោយ​សំនេររបស់អ្នក​ត្រូវបានគេកែប្រែដោយ​គ្មានអាសូរទេនោះ សូមកុំដាក់​ស្នើវា​នៅទីនេះអី។<br />
អ្នកត្រូវសន្យាជាមួយ​យើង​ខ្ញុំផងដែរថា ​អ្នកសរសេរវា​ដោយខ្លួនអ្នក ឬ បានចំលងវា​ពី​កម្មសិទ្ធិសាធារណៈឬពីប្រភពសេរី (សូមមើល $1 សំរាប់ពត៌មាន​លំអិត)។

<strong>មិនត្រូវ​ដាក់ស្នើ​ការងារមានរក្សាសិទ្ធិកម្មសិទ្ឋិបញ្ញាដោយគ្មានការអនុញ្ញាតទេ!</strong>',
'longpagewarning'                  => '<strong>ប្រយ័ត្ន ៖ ទំព័រនេះមានទំហំ $1 គីឡូបៃ។ ឧបករណ៍រាវរក(browser)ខ្លះអាចមានបញ្ហាក្នុងការកែប្រែទំព័រក្បែរឬធំជាង៣២គីឡូបៃ​។

សូមពិចារណាអំពីលទ្ឋភាពបំបែកទំព័រជាផ្នែកតូចៗ ។ </strong>',
'longpageerror'                    => '<strong>កំហុស៖ អត្ថបទ​ដែល​អ្នក​បានដាក់​ស្នើ​មានទំហំ $1 គីឡូបៃ ដែលធំជាង​ទំហំអតិបរមា $2 គីឡូបៃ។ អត្ថបទនេះ​មិនអាច​រក្សាទុកបានទេ។</strong>',
'readonlywarning'                  => '<strong>បំរាម:មូលដ្ឋានទិន្នន័យត្រូវបានចាក់សោសំរាប់ការរក្សាទុក ដូច្នេះអ្នកនឹងមិនអាចរក្សាទុករាល់កំនែប្រែរបស់អ្នកបានទេឥឡូវនេះ។ សូមអ្នកចំលងអត្ថបទ រួចដាក់ទៅក្នុងឯកសារដែលជាអត្ថបទ ហើយបន្ទាប់មករក្សាវាទុក។</strong>',
'protectedpagewarning'             => '<strong>ប្រយ័ត្ន៖ ទំព័រនេះ​ត្រូវបានចាក់សោ។ ដូច្នេះ​មានតែ​អ្នកប្រើប្រាស់​ដែល​មាន​អភ័យឯកសិទ្ឋិ​ជាអ្នកថែទាំប្រព័ន្ឋ​ (sysop) ទេទើបអាច​កែប្រែ​វាបាន។</strong>',
'semiprotectedpagewarning'         => "'''សំគាល់៖''' ទំព័រនេះ​បានត្រូវ​ចាក់សោ។ ដូច្នេះ​មានតែអ្នកប្រើប្រាស់​ដែលបានចុះឈ្មោះ​ទើបអាចកែប្រែ​វា​បាន។",
'titleprotectedwarning'            => '<strong>ប្រយ័ត្ន៖ ទំព័រនេះត្រូវបានចាក់សោ ដូច្នេះមានតែអ្នកប្រើប្រាស់មួយចំនួនប៉ុណ្ណោះអាចបង្កើតវា។</strong>',
'templatesused'                    => 'ទំព័រគំរូប្រើនៅក្នុងទំព័រនេះគឺ៖',
'templatesusedpreview'             => 'ទំព័រគំរូ​នានាដែល​បានប្រើប្រាស់​ក្នុងការមើលមុននេះ៖',
'templatesusedsection'             => 'ទំព័រគំរូដែលត្រូវបានប្រើប្រាស់ក្នុងផ្នែកនេះ៖',
'template-protected'               => '(ត្រូវបានការពារ)',
'template-semiprotected'           => '(ត្រូវបានការពារពាក់កណ្តាល)',
'hiddencategories'                 => 'ទំព័រនេះស្ថិតនៅក្នុង {{PLURAL:$1|ចំនាត់ថ្នាក់ក្រុមដែលត្រូវបានបិទបាំង១|ចំនាត់ថ្នាក់ក្រុមដែលត្រូវបានបិទបាំង $1}}:',
'nocreatetitle'                    => 'ការបង្កើតទំព័រ​ត្រូវបានកំរិត',
'nocreatetext'                     => '{{SITENAME}} បានដាក់កំហិតលទ្ធភាពបង្កើតទំព័រថ្មី ។
អ្នកអាចត្រលប់ក្រោយ និង កែប្រែទំព័រមានស្រាប់ ឬ  [[Special:UserLogin|ចូលឬបង្កើតគណនី]]។',
'nocreate-loggedin'                => 'អ្នកគ្មានការអនុញ្ញាត​បង្កើតទំព័រថ្មី​លើ {{SITENAME}} ទេ។',
'permissionserrors'                => 'កំហុសនៃការអនុញ្ញាតនានា',
'permissionserrorstext'            => 'អ្នកគ្មានការអនុញ្ញាតឱ្យធ្វើអ្វីទាំងនោះទេ សំរាប់{{PLURAL:$1|ហេតុផល|ហេតុផល}}ដូចតទៅ៖',
'permissionserrorstext-withaction' => 'អ្នកមិនត្រូវបានអនុញ្ញាតអោយ$2ទេ ដោយសារ{{PLURAL:$1|មូលហេតុ|មូលហេតុ}}ដូចខាងក្រោម:',
'recreate-deleted-warn'            => "'''ប្រយ័ត្ន ៖ អ្នកកំពុង​បង្កើតឡើងវិញ​ទំព័រដែលទើបតែ​ត្រូវបានលុបចេញ ។'''

អ្នក​គួរពិចារណាមើល​តើជាការសមស្របទេ ដែលបន្តកែប្រែ​ទំព័រនេះ ។

កំនត់ហេតុ​លុបចេញ​ចំពោះទំព័រនេះ ត្រូវបានផ្តល់​ទីនេះ​ដើម្បីងាយ​តាមដាន ៖",

# Parser/template warnings
'post-expand-template-inclusion-warning'  => 'ប្រយ័ត្ន៖ ទំព័រគំរូដែលបានបញ្ចូលមានទំហំធំពេកហើយ។

ទំព័រគំរូមួយចំនួនអាចនឹងមិនត្រូវបានបញ្ចូល។',
'post-expand-template-inclusion-category' => 'ទំព័រទាំងឡាយដែលមានបញ្ចូលទំព័រគំរូហួសចំនុះ',

# "Undo" feature
'undo-success' => 'ការកែប្រែគឺមិនអាចបញ្ចប់។ សូមពិនិត្យ​ការប្រៀបធៀបខាងក្រោមដើម្បីផ្ទៀងផ្ទាត់ថា​នេះគឺជាអ្វីដែលអ្នកចង់ធ្វើហើយបន្ទាប់មកទៀត​រក្សាបំលាស់ប្តូរខាងក្រោមទុក ដើម្បីបញ្ចប់ការកែប្រែដែលមិនទាន់រួចរាល់។',

# Account creation failure
'cantcreateaccounttitle' => 'មិនអាចបង្កើតគណនីបានទេ',
'cantcreateaccount-text' => "ការបង្កើតគណនីពីអាសយដ្ឋាន IP ('''$1''') នេះ ត្រូវបានរារាំងដោយ [[User:$3|$3]]។

ហេតុផលដែលត្រូវលើកឡើងដោយ $3 គឺ ''$2''",

# History pages
'viewpagelogs'        => 'មើលកំនត់ហេតុសំរាប់ទំព័រនេះ',
'nohistory'           => 'មិនមានប្រវត្តិកំនែប្រែ​ចំពោះទំព័រនេះ។',
'revnotfound'         => 'រកមិនឃើញ​កំនែ',
'revnotfoundtext'     => 'កំនែប្រែចាស់របស់ទំព័រដែលអ្នកស្វែងរកមិនមានទេ។ ចូរពិនិត្យURLដែលអ្នកធ្លាប់ដំណើរការទំព័រនេះ។',
'currentrev'          => 'កំនែបច្ចុប្បន្ន',
'revisionasof'        => 'កំនែ​របស់ $1',
'revision-info'       => 'កំនែ​របស់ $1 ដោយ $2',
'previousrevision'    => '← កំនែ​មុន',
'nextrevision'        => 'កំនែបន្ទាប់ →',
'currentrevisionlink' => 'កំនែប្រែបច្ចុប្បន្ន',
'cur'                 => 'បច្ចុប្បន្ន',
'next'                => 'បន្ទាប់',
'last'                => 'ចុងក្រោយ',
'page_first'          => 'ដំបូង',
'page_last'           => 'ចុងក្រោយ',
'histlegend'          => "ជំរើសផ្សេងគ្នា៖ សូមគូសក្នុងកូនប្រអប់ពីមុខកំនែ(versions)ដែលអ្នកចង់ប្រៀបធៀប រួចចុចច្នុច enter ឬប៊ូតុងនៅខាងក្រោម។<br />
'''ពាក្យតំនាង'''៖(បច្ចុប្បន្ន) = ភាពខុសគ្នាជាមួយនឹងកំនែបច្ចុប្បន្ន, (ចុងក្រោយ) = ភាពខុសគ្នារវាងកំនែប្រែពីមុន, តិច = កំនែប្រែតិចតួច",
'deletedrev'          => '[ត្រូវបាន​លុបចោល]',
'histfirst'           => 'ដំបូងៗបំផុត',
'histlast'            => 'ថ្មីៗបំផុត',
'historysize'         => '({{PLURAL:$1|១បៃ|$1បៃ}})',
'historyempty'        => '(ទទេ)',

# Revision feed
'history-feed-title'          => 'ប្រវត្តិនៃកំនែ',
'history-feed-description'    => 'ប្រវត្តិនៃកំនែទំព័រនេះលើវិគី',
'history-feed-item-nocomment' => 'ដោយ$1នៅវេលា$2', # user at time
'history-feed-empty'          => 'ទំព័រដែលអ្នកបានស្នើមិនមានទេ។
ប្រហែលជាវាត្រូវបានគេលុបចោលពីវីគីឬ​ត្រូវបានគេដាក់ឈ្មោះថ្មី។
សូមសាក [[Special:Search|ស្វែងរកនៅក្នុងវិគី]] ដើម្បីរកទំព័រថ្មីដែលមានការទាក់ទិន។',

# Revision deletion
'rev-deleted-comment'       => '(វិចារត្រូវបានដកចេញ)',
'rev-deleted-user'          => '(ឈ្មោះអ្នកប្រើប្រាស់ត្រូវបានដកចេញ)',
'rev-deleted-event'         => '(កំនត់ហេតុសកម្មភាពត្រូវបានដកចេញ)',
'rev-delundel'              => 'បង្ហាញ/លាក់',
'revisiondelete'            => 'លុបចេញ / លែងលុបចេញ កំនែនានា',
'revdelete-nooldid-title'   => 'គ្មានកំនែប្រែដែលមានគោលដៅទេ',
'revdelete-legend'          => 'ដាក់កំហិត នានា',
'revdelete-hide-text'       => 'បិទបាំងឃ្លានៃកំនែប្រែ',
'revdelete-hide-name'       => 'បិទបាំងសកម្មភាពនិងគោលដៅ',
'revdelete-hide-comment'    => 'បិទបាំងកំនែប្រែវិចារ',
'revdelete-hide-user'       => 'បិទបាំងឈ្មោះអ្នកប្រើប្រាស់​ឬអាសយដ្ឋានIPនៃអ្នករួមចំណែក',
'revdelete-hide-restricted' => 'អនុវត្តការដាក់កំហិតទាំងនេះចំពោះអ្នកថែទាំប្រព័ន្ធ និងចាក់សោអន្តរមុខនេះ',
'revdelete-suppress'        => 'លាក់ទិន្នន័យពីអ្នកថែទាំប្រព័ន្ធ ព្រមទាំងពីសមាជិកដទៃទៀតផងដែរ',
'revdelete-hide-image'      => 'បិទបាំងខ្លឹមសារនៃឯកសារ',
'revdelete-unsuppress'      => 'ដកចេញការដាក់កំហិតលើកំណដែលបានស្តារឡើងវិញ',
'revdelete-log'             => 'បញ្ចេញយោបល់:',
'revdelete-submit'          => 'អនុវត្តន៍ទៅកំនែដែលបានជ្រើសយក',
'revdelete-logentry'        => 'បានផ្លាស់ប្តូរគំហើញកំណែនៃ[[$1]]',
'logdelete-logentry'        => 'បានផ្លាស់ប្តូរគំហើញហេតុការនៃ[[$1]]',
'revdelete-success'         => "'''បានកំណត់គំហើញកំណែដោយជោគជ័យ។'''",
'logdelete-success'         => "'''បានកំណត់គំហើញកំនត់ហេតុដោយជោគជ័យ។'''",
'pagehist'                  => 'ប្រវត្តិទំព័រ',
'deletedhist'               => 'ប្រវត្តិដែលត្រូវបានលុបចោល',
'revdelete-content'         => 'ខ្លឹមសារ',
'revdelete-summary'         => 'កែប្រែសេចក្ដីសង្ខេប',
'revdelete-uname'           => 'អ្នកប្រើប្រាស់',
'revdelete-restricted'      => 'បានអនុវត្តការដាក់កំហិតចំពោះអ្នកថែទាំប្រព័ន្ធ',
'revdelete-unrestricted'    => 'បានដកការដាក់កំហិតចេញសំរាប់អ្នកថែទាំប្រព័ន្ធ',
'revdelete-hid'             => 'បានលាក់$1',
'revdelete-unhid'           => 'ឈប់លាក់$1',
'logdelete-log-message'     => '$1 ចំពោះ $2 {{PLURAL:$2|ព្រឹត្តិការណ៍|ព្រឹត្តិការណ៍}}',

# History merging
'mergehistory'                     => 'បញ្ចូលរួមគ្នាប្រវត្តិទាំងឡាយនៃទំព័រ',
'mergehistory-box'                 => 'បញ្ចូលរួមគ្នាកំណែទាំងឡាយនៃពីរទំព័រ៖',
'mergehistory-from'                => 'ទំព័រកូដ៖',
'mergehistory-into'                => 'ទំព័រគោលដៅ៖',
'mergehistory-list'                => 'ប្រវត្តិកំនែប្រែដែលអាចបញ្ចូលរួមគ្នាបាន',
'mergehistory-go'                  => 'បង្ហាញកំនែប្រែដែលអាចបញ្ចូលរួមគ្នាបាន',
'mergehistory-submit'              => 'បញ្ចូលរួមគ្នានូវកំណែនានា',
'mergehistory-empty'               => 'គ្មានកំណែ ណាមួយ អាចត្រូវបាន បញ្ចូលរួមគ្នា.',
'mergehistory-fail'                => 'មិនអាចធ្វើការប្របាច់បញ្ចូលប្រវត្តិ។ សូមពិនិត្យទំព័រនេះនិងប៉ារ៉ាម៉ែត្រពេលវេលាឡើងវិញ។',
'mergehistory-no-source'           => 'ទំព័រប្រភព $1 មិនមានទេ។',
'mergehistory-no-destination'      => 'ទំព័រគោលដៅ $1 មិនមានទេ។',
'mergehistory-invalid-source'      => 'ទំព័រ ប្រភព ត្រូវតែមាន មួយចំណងជើង បានការ។',
'mergehistory-invalid-destination' => 'ទំព័រ គោលដៅ ត្រូវតែមាន មួយចំណងជើង បានការ។',
'mergehistory-autocomment'         => 'បានរំលាយបញ្ចូល [[:$1]] ទៅក្នុង [[:$2]]',
'mergehistory-comment'             => 'រំលាយបញ្ចូល [[:$1]] ទៅក្នុង [[:$2]]: $3',

# Merge log
'mergelog'           => 'កំនត់ហេតុនៃការបញ្ចូលរួមគ្នា',
'pagemerge-logentry' => 'បានបញ្ចូល[[$1]]ជាមួយ[[$2]]រួមគ្នា (កំណែរហូតដល់$3)',
'revertmerge'        => 'បំបែកចេញ',
'mergelogpagetext'   => 'ខាងក្រោមគឺជាតារាងរបស់ការបញ្ចូលគ្នាថ្មីៗបំផុតរបស់ប្រវត្តិនៃទំព័រមួយទៅក្នុងប្រវត្តិនៃទំព័រមួយទៀត។',

# Diffs
'history-title'           => 'ប្រវត្តិកំនែប្រែនានានៃ "$1"',
'difference'              => '(ភាពខុសគ្នានៃកំនែនានា)',
'lineno'                  => 'បន្ទាត់ទី$1៖',
'compareselectedversions' => 'ប្រៀបធៀប​កំនែប្រែ​ដែលបាន​ជ្រើសយក',
'editundo'                => 'undo',
'diff-multi'              => '({{PLURAL:$1|កំនែប្រែកំរិតបង្គួរមួយ|កំនែប្រែកំរិតបង្គួរចំនួន$1}}មិនត្រូវបានបង្ហាញ)',

# Search results
'searchresults'             => 'លទ្ធផលស្វែងរក',
'searchresulttext'          => 'ចំពោះពត៌មានបន្ថែមអំពីការស្វែងរកក្នុង{{SITENAME}}, សូមមើល[[ជំនួយ:មាតិកា|ទំព័រជំនួយ]]។',
'searchsubtitle'            => 'អ្នកបានស្វែងរក \'\'\'[[:$1]]\'\'\'([[Special:Prefixindex/$1|គ្រប់ទំព័រដែលផ្ដើមដោយ "$1"]] | [[Special:WhatLinksHere/$1|គ្រប់ទំព័រដែលភ្ជាប់មក "$1"]])',
'searchsubtitleinvalid'     => "អ្នកបានស្វែងរក '''$1'''",
'noexactmatch'              => "'''គ្មានទំព័រ​ណាដែលមានចំនងជើង \"\$1\" ទេ។''' អ្នកអាច [[:\$1|បង្កើតទំព័រនេះ]]។",
'noexactmatch-nocreate'     => "'''គ្មានទំព័រ​ណាដែលមានចំនងជើង \"\$1\"ទេ។'''",
'toomanymatches'            => 'មានតំនភ្ជាប់ច្រើនណាស់ត្រូវបានបង្ហាញ ចូរព្យាយាមប្រើសំនួរផ្សេងមួយទៀត',
'titlematches'              => 'ភាពត្រូវគ្នានៃចំនងជើងទំព័រ',
'notitlematches'            => 'ពុំមានចំនងជើងទំព័រណាផ្គួរផ្គងទេ',
'textmatches'               => 'ទំព័រអត្ថបទផ្គូរផ្គងគ្នា',
'notextmatches'             => 'គ្មានទំព័រណាមួយដែលមានខ្លឹមសារផ្គួរផ្គងនឹងឃ្លាឬពាក្យនេះទេ',
'prevn'                     => 'មុន $1',
'nextn'                     => 'បន្ទាប់ $1',
'viewprevnext'              => 'មើល ($1) ($2) ($3)',
'search-result-size'        => '$1({{PLURAL:$2|១ពាក្យ|$2ពាក្យ}})',
'search-result-score'       => 'កំរិតនៃភាពទាក់ទិន៖ $1%',
'search-redirect'           => '(បញ្ជូនបន្ត $1)',
'search-section'            => '(ផ្នែក $1)',
'search-suggest'            => 'ប្រហែលជាអ្នកចង់រក៖ $1',
'search-interwiki-caption'  => 'គំរោងជាបងប្អូន',
'search-interwiki-default'  => '$1 លទ្ធផល៖',
'search-interwiki-more'     => '(បន្ថែមទៀត)',
'search-mwsuggest-enabled'  => 'មានសំនើ',
'search-mwsuggest-disabled' => 'គ្មានសំនើ',
'search-relatedarticle'     => 'ទាក់ទិន',
'searchrelated'             => 'ទាក់ទិន',
'searchall'                 => 'ទាំងអស់',
'showingresults'            => "ខាងក្រោមកំពុងបង្ហាញរហូតដល់ {{PLURAL:$1|'''១''' លទ្ឋផល|'''$1''' លទ្ឋផល}} ចាប់ផ្ដើមពីលេខ #'''$2'''។",
'showingresultsnum'         => 'កំពុងបង្ហាញ<b>$3</b>លទ្ឋផលខាងក្រោម ចាប់ផ្ដើមដោយ #<b>$2</b>។',
'showingresultstotal'       => "ខាងក្រោមកំពុងបង្ហាញលទ្ឋផលពីលេខ '''$1 - $2''' ក្នុងចំនោមលទ្ឋផលសរុប '''$3'''",
'powersearch'               => 'ស្វែងរកថ្នាក់ខ្ពស់',
'powersearch-legend'        => 'ស្វែងរកថ្នាក់ខ្ពស់',
'powersearch-ns'            => 'ស្វែងរកក្នុងលំហឈ្មោះ:',
'powersearch-redir'         => 'បញ្ជីការបញ្ជូនបន្ត',
'powersearch-field'         => 'ស្វែងរក',
'search-external'           => 'ស្វែងរកនៅខាងក្រៅ',

# Preferences page
'preferences'              => 'ចំនង់ចំនូលចិត្ត',
'mypreferences'            => 'ចំនង់ចំនូលចិត្ត​',
'prefs-edits'              => 'ចំនួនកំនែប្រែ៖',
'prefsnologin'             => 'មិនបានឡុកអ៊ីន',
'prefsnologintext'         => 'អ្នកចាំបាច់ត្រូវតែ<span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} ឡុកអ៊ីន]</span> ដើម្បីកំនត់ចំនង់ចំនូលចិត្តរបស់អ្នកប្រើប្រាស់។',
'prefsreset'               => 'ចំនូលចិត្ត​ផ្ទាល់ខ្លួនត្រូវបានធ្វើអោយដូចដើមវិញពីកំនែមុននេះហើយ។',
'qbsettings'               => 'របារទាន់ចិត្ត',
'qbsettings-none'          => 'ទទេ',
'qbsettings-fixedleft'     => 'ចុងខាងឆ្វេង',
'qbsettings-fixedright'    => 'ចុងខាងស្តាំ',
'qbsettings-floatingleft'  => 'អណ្តែតឆ្វេង',
'qbsettings-floatingright' => 'អណ្តែតស្តាំ',
'changepassword'           => 'ប្តូរពាក្យសំងាត់',
'skin'                     => 'សំបក',
'math'                     => 'គណិត',
'dateformat'               => 'ទំរង់កាលបរិច្ឆេទ',
'datedefault'              => 'គ្មានចំនូលចិត្ត',
'datetime'                 => 'កាលបរិច្ឆេទនិងល្វែងម៉ោង',
'math_failure'             => 'Failed to parse',
'math_unknown_error'       => 'កំហុសមិនស្គាល់',
'math_unknown_function'    => 'អនុគមន៍​មិន​ស្គាល់',
'math_syntax_error'        => 'កំហុសពាក្យសម្ព័ន្ធ',
'math_image_error'         => 'ការបំលែងជា PNG បានបរាជ័យ។
សូមពិនិត្យមើលតើ latex, dvips, gs, បានតំលើងត្រឹមត្រូវឬអត់ រួចបំលែង',
'math_bad_tmpdir'          => 'មិនអាចសរសេរទៅ ឬ បង្កើតថតឯកសារគណិតបណ្តោះអាសន្ន',
'math_bad_output'          => 'មិនអាច សរសេរទៅ ឬ បង្កើត ថតឯកសារ គណិត ទិន្នផល',
'prefs-personal'           => 'ប្រវត្តិរូប',
'prefs-rc'                 => 'បំលាស់ប្តូរថ្មីៗ',
'prefs-watchlist'          => 'បញ្ជីតាមដាន',
'prefs-watchlist-days'     => 'ចំនួនថ្ងៃត្រូវបង្ហាញក្នុងបញ្ជីតាមដាន៖',
'prefs-watchlist-edits'    => 'ចំនួនអតិប្បរមានៃបំលាស់ប្តូរត្រូវបង្ហាញក្នុងបញ្ជីតាមដានដែលបានពង្រីក៖',
'prefs-misc'               => 'ផ្សេងៗ',
'saveprefs'                => 'រក្សាទុក',
'resetprefs'               => 'លុបចោលបំលាស់ប្ដូរមិនបានរក្សាទុក',
'oldpassword'              => 'ពាក្យសំងាត់ចាស់៖',
'newpassword'              => 'ពាក្យសំងាត់ថ្មី៖',
'retypenew'                => 'សូមវាយពាក្យសំងាត់ថ្មី​ម្តងទៀត៖',
'textboxsize'              => 'កំនែប្រែ',
'rows'                     => 'ជួរដេក៖',
'columns'                  => 'ជួរឈរ៖',
'searchresultshead'        => 'ស្វែងរក',
'resultsperpage'           => 'ចំនួនលទ្ធផលក្នុងមួយទំព័រ៖',
'contextlines'             => 'ចំនួនបន្ទាត់ក្នុងមួយលទ្ធផល៖',
'contextchars'             => 'ចំនួនអក្សរក្នុងមួយជួរ៖',
'stub-threshold'           => 'កំរិត ចំពោះ <a href="#" class="stub">តំណភ្ជាប់​ទៅ ពង្រាង </a> (បៃ)៖',
'recentchangesdays'        => 'ចំនួនថ្ងៃបង្ហាញក្នុងទំព័របំលាស់ប្តូរថ្មីៗ៖',
'recentchangescount'       => 'ចំនួនកំនែប្រែត្រូវបង្ហាញក្នុងបំលាស់ប្តូរថ្មីៗ ប្រវត្តិនិងទំព័រកំនត់ហេតុ៖',
'savedprefs'               => 'ចំនូលចិត្តនានារបស់អ្នកត្រូវបានរក្សាទុកហើយ។',
'timezonelegend'           => 'ល្វែងម៉ោង',
'timezonetext'             => '¹ចំនួនម៉ោងដែលម៉ោងក្នុងស្រុករបស់អ្នកខុសពីម៉ោងម៉ាស៊ីនបំរើសេវា (UTC)។',
'localtime'                => 'ម៉ោងក្នុងស្រុក',
'timezoneoffset'           => 'ទូទាត់¹',
'servertime'               => 'ម៉ោងម៉ាស៊ីនបំរើសេវា',
'guesstimezone'            => 'បំពេញពីកម្មវិធីរាវរក',
'allowemail'               => 'អាចទទួលអ៊ីមែលពីអ្នកប្រើប្រាស់ដទៃទៀត',
'prefs-searchoptions'      => 'ជំរើសក្នុងការស្វែងរក',
'prefs-namespaces'         => 'លំហឈ្មោះ',
'defaultns'                => 'ស្វែងរកក្នុងលំហឈ្មោះទាំងនេះតាមលំនាំដើម៖',
'default'                  => 'លំនាំដើម',
'files'                    => 'ឯកសារ',

# User rights
'userrights'                  => 'ការគ្រប់គ្រងសិទ្ធិអ្នកប្រើប្រាស់', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'គ្រប់គ្រងក្រុមអ្នកប្រើប្រាស់',
'userrights-user-editname'    => 'បញ្ចូលឈ្មោះអ្នកប្រើប្រាស់៖',
'editusergroup'               => 'កែប្រែក្រុមអ្នកប្រើប្រាស់',
'editinguser'                 => "ការប្តូរសិទ្ធិអ្នកប្រើប្រាស់ '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'កែប្រែក្រុមអ្នកប្រើប្រាស់',
'saveusergroups'              => 'រក្សាក្រុមអ្នកប្រើប្រាស់ទុក',
'userrights-groupsmember'     => 'ក្រុមសមាជិកភាព៖',
'userrights-reason'           => 'មូលហេតុនៃការផ្លាស់ប្តូរ៖',
'userrights-no-interwiki'     => 'អ្នកមិនមានការអនុញ្ញាតិ កែប្រែសិទ្ធិ នៃអ្នកប្រើប្រាស់ លើ វិគី ផ្សេង ទេ។',
'userrights-nodatabase'       => 'មូលដ្ឋានទិន្នន័យ $1 មិនមាន ឬ ថិតនៅខាងក្រៅ។',
'userrights-nologin'          => 'អ្នកត្រូវតែ [[Special:UserLogin|ឡុកអ៊ីន]] ជាគណនីអ្នកអភិបាលដើម្បីផ្តល់សិទ្ធិអោយអ្នកប្រើប្រាស់ ។',
'userrights-notallowed'       => 'គណនីរបស់អ្នកមិនមានការអនុញ្ញាតិដើម្បីកំនត់សិទ្ធិរបស់អ្នកប្រើប្រាស់ដទៃ។',
'userrights-changeable-col'   => 'ក្រុមនានាដែលអ្នកអាចផ្លាស់ប្ដូរបាន',
'userrights-unchangeable-col' => 'ក្រុមនានាដែលអ្នកមិនអាចផ្លាស់ប្ដូរបាន',

# Groups
'group'               => 'ក្រុម៖',
'group-user'          => 'អ្នកប្រើប្រាស់',
'group-autoconfirmed' => 'អ្នកប្រើប្រាស់ទទួលស្គាល់ដោយស្វ័យប្រវត្តិ',
'group-bot'           => 'រូបយន្ត',
'group-sysop'         => 'អ្នកថែទាំប្រព័ន្ធ',
'group-bureaucrat'    => 'អ្នកការិយាល័យ',
'group-all'           => '(ទាំងអស់)',

'group-user-member'          => 'អ្នកប្រើប្រាស់',
'group-autoconfirmed-member' => 'អ្នកប្រើប្រាស់ទទួលស្គាល់ដោយស្វ័យប្រវត្តិ',
'group-bot-member'           => 'រូបយន្ត',
'group-sysop-member'         => 'អ្នកថែទាំប្រព័ន្ធ',
'group-bureaucrat-member'    => 'អ្នកការិយាល័យ',

'grouppage-user'          => '{{ns:project}}:អ្នកប្រើប្រាស់',
'grouppage-autoconfirmed' => '{{ns:project}}:អ្នកប្រើប្រាស់ទទួលស្គាល់ដោយស្វ័យប្រវត្តិ',
'grouppage-bot'           => '{{ns:project}}:រូបយន្ត',
'grouppage-sysop'         => '{{ns:project}}:អ្នកអភិបាល',
'grouppage-bureaucrat'    => '{{ns:project}}:អ្នកការិយាល័យ',

# Rights
'right-read'                 => 'អានអត្ថបទ',
'right-edit'                 => 'កែប្រែអត្ថបទ',
'right-createpage'           => 'បង្កើតទំព័រអត្ថបទ (ដែលមិនមែនជាទំព័រពិភាក្សា)',
'right-createtalk'           => 'បង្កើតទំព័រពិភាក្សា',
'right-createaccount'        => 'បង្កើតគណនីអ្នកប្រើប្រាស់ថ្មី',
'right-minoredit'            => 'កំនត់ចំនាំកំនែប្រែថាជាកំនែប្រែតិចតួច',
'right-move'                 => 'ប្ដូរទីតាំងទំព័រ',
'right-move-subpages'        => 'ប្ដូរទីតាំងទំព័ររួមជាមួយទំព័ររងរបស់វា',
'right-upload'               => 'ផ្ទុកឡើងឯកសារ',
'right-reupload'             => 'សរសេរលុបពីលើឯកសារមួយច្បាប់ដែលមានស្រាប់',
'right-upload_by_url'        => 'ភ្ទុកឡើងឯកសារមួយពីអាសយដ្ឋាន URL មួយ',
'right-autoconfirmed'        => 'កែប្រែទំព័រពាក់កណ្ដាលការពារនានា',
'right-bot'                  => 'ទុកដូចជាដំនើរការស្វ័យប្រវត្តិមួយ',
'right-delete'               => 'លុបទំព័រចោល',
'right-bigdelete'            => 'លប់ទំព័រទាំងឡាយដែលមានប្រវត្តិវែង',
'right-browsearchive'        => 'ស្វែងរកទំព័រដែលត្រូវបានលុប',
'right-undelete'             => 'ឈប់លុបទំព័រមួយ',
'right-suppressionlog'       => 'មើលកំនត់ហេតុឯកជន',
'right-block'                => 'ហាមមិនអោយអ្នកប្រើប្រាស់ដទៃទៀតធ្វើការកែប្រែ',
'right-blockemail'           => 'រាំងខ្ទប់អ្នកប្រើប្រាស់ម្នាក់មិនអោយផ្ញើអ៊ីមែល',
'right-hideuser'             => 'រាំងខ្ទប់អ្នកប្រើប្រាស់ម្នាក់ រួចលាក់មិនបង្ហាញជាសាធារណៈ',
'right-protect'              => 'ប្ដូរកំរិតការពាររួចកែប្រែទំព័រដែលបានការពារ',
'right-editprotected'        => 'កែប្រែទំព័រដែលបានការពារ (ដោយមិនរំលាយការការពារ)',
'right-editinterface'        => 'កែប្រែអន្តរមុខអ្នកប្រើប្រាស់',
'right-editusercssjs'        => 'កែប្រែឯកសារ CSS និង JS របស់អ្នកប្រើប្រាស់ផ្សេងទៀត',
'right-rollback'             => 'ត្រលប់យ៉ាងរហ័សនូវកំនែប្រែទំព័រវិសេសណាមួយ​ដែលធ្វើឡើងដោយ​អ្នកប្រើប្រាស់ចុងក្រោយគេ។',
'right-import'               => 'នាំចូលទំព័រនានាពីវិគីផ្សេងៗទៀត',
'right-importupload'         => 'នាំចូលទំព័រនានាពីឯកសារដែលបានផ្ទុកឡើង',
'right-patrol'               => 'កត់សំគាល់កំនែប្រែដ៏ទៃទៀតថាល្បាត',
'right-unwatchedpages'       => 'បង្ហាញបញ្ជីទំព័រនានាដែលមិនត្រូវបានមើល',
'right-mergehistory'         => 'រំលាយបញ្ចូលប្រវត្តិរបស់ទំព័រនានា',
'right-userrights'           => 'កែប្រែរាល់សិទ្ធិនៃអ្នកប្រើប្រាស់',
'right-userrights-interwiki' => 'កែប្រែសិទ្ធិអ្នកប្រើប្រាស់នៅលើវិគីផ្សេងៗទៀត',
'right-siteadmin'            => 'ចាក់សោនិងបើកសោមូលដ្ឋានទិន្នន័យ',

# User rights log
'rightslog'      => 'កំនត់ហេតុនៃការប្តូរសិទ្ធិអ្នកប្រើប្រាស់',
'rightslogtext'  => 'នេះជា កំណត់ហេតុ នៃបំលាស់ប្តូរ ចំពោះសិទ្ធិនានា របស់ អ្នកប្រើប្រាស់ ។',
'rightslogentry' => 'បានប្តូរក្រុមសមាជិកភាពសំរាប់$1ពី$2ទៅ$3',
'rightsnone'     => '(ទទេ)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|បំលាស់ប្តូរ|បំលាស់ប្តូរ}}',
'recentchanges'                     => 'បំលាស់ប្តូរ​ថ្មីៗ',
'recentchangestext'                 => 'តាមដានរាល់បំលាស់ប្តូរថ្មីៗបំផុតចំពោះវិគីនៅលើទំព័រនេះ។',
'recentchanges-feed-description'    => 'តាមដានបំលាស់ប្តូរថ្មីៗបំផុតនៃវិគីនេះក្នុង feed នេះ។',
'rcnote'                            => "ខាងក្រោម​នេះ​ជា{{PLURAL:$1|១បំលាស់ប្តូរ​|'''$1'''បំលាស់ប្តូរ​}}ចុងក្រោយក្នុងរយៈពេល{{PLURAL:$2|ថ្ងៃ|'''$2'''ថ្ងៃ}}ចុងក្រោយគិតត្រឹម$5 $4 ។",
'rcnotefrom'                        => "ខាងក្រោមនេះជាបំលាស់ប្តូរនានាគិតចាប់តាំងពី '''$2''' (បង្ហាញអតិបរិមា '''$1''' បំលាស់ប្តូរ)។",
'rclistfrom'                        => 'បង្ហាញបំលាស់ប្តូរថ្មីៗដែលចាប់ផ្តើមពី $1',
'rcshowhideminor'                   => '$1កំនែប្រែ​តិចតួច',
'rcshowhidebots'                    => '$1រូបយន្ត',
'rcshowhideliu'                     => '$1អ្នកប្រើប្រាស់ដែលបានឡុកអ៊ីន',
'rcshowhideanons'                   => '$1អ្នកប្រើប្រាស់អនាមិក',
'rcshowhidepatr'                    => '$1កំនែប្រែដែលបានល្បាត',
'rcshowhidemine'                    => '$1កំនែប្រែរបស់ខ្ញុំ',
'rclinks'                           => 'បង្ហាញ$1បំលាស់ប្តូរចុងក្រោយក្នុងរយៈពេល$2ថ្ងៃចុងក្រោយ<br />$3',
'diff'                              => 'ភាពខុសគ្នា',
'hist'                              => 'ប្រវត្តិ',
'hide'                              => 'លាក់',
'show'                              => 'បង្ហាញ',
'minoreditletter'                   => 'តិច',
'newpageletter'                     => 'ថ្មី',
'boteditletter'                     => 'រូបយន្ត',
'number_of_watching_users_pageview' => '[មាន{{PLURAL:$1|អ្នកប្រើប្រាស់|អ្នកប្រើប្រាស់}}$1នាក់កំពុងមើល]',
'rc_categories'                     => 'កំរិតទីតាំងចំណាត់ថ្នាក់ក្រុម(ខណ្ឌដោយសញ្ញា "|")',
'rc_categories_any'                 => 'មួយណាក៏បាន',
'newsectionsummary'                 => '/* $1 */ ផ្នែកថ្មី',

# Recent changes linked
'recentchangeslinked'          => 'បំលាស់ប្តូរពាក់ព័ន្ធ',
'recentchangeslinked-title'    => 'បំលាស់ប្តូរ​ទាក់ទិននឹង "$1"',
'recentchangeslinked-noresult' => 'គ្មានបំលាស់ប្តូរ​លើទំព័រ​ដែលត្រូវបានតភ្ជាប់ ក្នុងថេរវេលា​ដែលត្រូវបានផ្តល់អោយ ។',
'recentchangeslinked-summary'  => "នេះជាបញ្ជីបំលាស់ប្ដូរនានាដែលត្រូវបានធ្វើឡើងនាពេលថ្មីៗនេះ ទៅលើទំព័រនានាដែលមានតំនភ្ជាប់ពីទំព័រកំនត់មួយ (ឬមានតំនភ្ជាប់ទៅទំព័រក្នុងចំនាត់ថ្នាក់ក្រុមកំនត់មួយ)។ ទំព័រ​នានាលើ[[Special:Watchlist|បញ្ជីតាមដាន​របស់អ្នក]] ត្រូវបានសរសេរជា '''អក្សរដិត''' ។",
'recentchangeslinked-page'     => 'ឈ្មោះទំព័រ៖',
'recentchangeslinked-to'       => 'បង្ហាញបំលាស់ប្តូរទំព័រដែលបានតភ្ជាប់ទៅកាន់ទំព័រដែលបានផ្តល់អោយជំនួសវិញ',

# Upload
'upload'                      => 'ផ្ទុកឯកសារឡើង',
'uploadbtn'                   => 'ផ្ទុកឯកសារឡើង',
'reupload'                    => 'ផ្ទុកឡើងម្តងទៀត',
'reuploaddesc'                => 'ឈប់ផ្ទុកឡើងរួចត្រឡប់ទៅបែបបទផ្ទុកឡើងវិញ។',
'uploadnologin'               => 'មិនបានឡុកអ៊ីនទេ',
'uploadnologintext'           => 'អ្នកត្រូវតែ [[Special:UserLogin|ឡុកអ៊ីន]] ដើម្បីផ្ទុកឡើងឯកសារទាំងឡាយ។',
'uploaderror'                 => 'កំហុសផ្ទុកឡើង',
'uploadtext'                  => "សូមប្រើប្រាស់បែបបទខាងក្រោមដើម្បីផ្ទុកឯកសារ​ឡើង។

ដើមីមើល ឬស្វែងរកឯកសារដែលបានផ្ទុកឡើងពីពេលមុន សូមចូលទៅ[[Special:ImageList|បញ្ជីឯកសារដែលបានផ្ទុកឡើង]]។ ការផ្ទុកឡើងវិញ​នូវឯកសារបង្ហាញនៅក្នុង[[Special:Log/upload|កំនត់ហេតុនៃការផ្ទុកឯកសារឡើង]] និងការលុបចេញមានបង្ហាញនៅក្នុង[[Special:Log/delete|កំនត់ហេតុនៃការលុប]]។


ដើម្បីដាក់រូបភាពទៅក្នុងទំព័រ សូមប្រើប្រាស់តំនភ្ជាប់ក្នុងទំរង់ដូចខាងក្រោម៖
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:ឈ្មោះឯកសារ.jpg]]</nowiki></tt>'''ដើម្បីប្រើប្រាស់ទំរង់ពេញលេញនៃឯកសារ
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:ឈ្មោះឯកសារ.png|200px|thumb|left|ឃ្លាពិពណ៌នា]]</nowiki></tt>''' ដោយប្រើប្រាស់ទំហំ​២០០ភីកសែលក្នុងប្រអប់នៅ​គេមខាងធ្វេង​ជាមួយនឹង​ឃ្លារៀបរាប់អំពីឯកសារនេះ។
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:ឈ្មោះឯកសារ.ogg]]</nowiki></tt>''' ដើម្បីតភ្ជាប់​ដោយផ្ទាល់ទៅឯកសារនេះ​ដោយមិនបង្ហាញឯកសារ។",
'upload-permitted'            => 'ប្រភេទឯកសារដែលត្រូវបានអនុញ្ញាត៖ $1 ។',
'upload-preferred'            => 'ប្រភេទឯកសារដែលគួរប្រើប្រាស់៖ $1 ។',
'upload-prohibited'           => 'ប្រភេទឯកសារដែលត្រូវបានហាម៖ $1 ។',
'uploadlog'                   => 'កំនត់ហេតុនៃការផ្ទុកឡើង',
'uploadlogpage'               => 'កំនត់ហេតុនៃការផ្ទុកឡើង',
'uploadlogpagetext'           => 'ខាងក្រោមនេះ​ជាបញ្ជីនៃការផ្ទុកឡើង​ថ្មីបំផុត។

សូមមើល [[Special:NewImages|វិចិត្រសាលរូបភាពថ្មីៗ]] ដើម្បីមើលដោយផ្ទាល់ភ្នែក។',
'filename'                    => 'ឈ្មោះឯកសារ',
'filedesc'                    => 'សេចក្តីសង្ខេប',
'fileuploadsummary'           => 'សេចក្តីសង្ខេប៖',
'filestatus'                  => 'ស្ថានភាពរក្សាសិទ្ធិ៖',
'filesource'                  => 'ប្រភព',
'uploadedfiles'               => 'ឯកសារដែលត្រូវបានផ្ទុកឡើង',
'ignorewarning'               => 'មិនខ្វល់​ការព្រមាន ហើយរក្សាទុក​ឯកសារ​តែម្តង។',
'ignorewarnings'              => 'មិនខ្វល់​ការព្រមាន​ណាមួយ',
'minlength1'                  => 'ឈ្មោះឯកសារ​ត្រូវមាន​យ៉ាងតិច​១​អក្សរ។',
'illegalfilename'             => 'ឈ្មោះឯកសារ "$1" មាន​អក្សរ​ហាមឃាត់​​ក្នុងចំនងជើងទំព័រ។ សូម​ប្តូរឈ្មោះ​ឯកសារ ហើយ​ព្យាយាមផ្ទុកវា​ឡើង​ម្តងទៀត។',
'badfilename'                 => 'ឈ្មោះឯកសារ បានត្រូវប្តូរ ជា "$1" ។',
'filetype-badmime'            => 'ឯកសារ​ប្រភេទ MIME "$1" មិនត្រូវបាន​អនុញ្ញាត​អោយផ្ទុកឡើង។',
'filetype-unwanted-type'      => '\'".$1"\'\'\' ជាប្រភេទឯកសារមិនចង់បាន។ 

{{PLURAL:$3|ប្រភេទឯកសារ|ប្រភេទឯកសារ}}ដែលគេចង់បានគឺ $2។',
'filetype-banned-type'        => "'''\".\$1\"''' គឺជាប្រភេទឯកសារដែលមិនត្រូវបានគេអនុញ្ញាតទេ។ \$3ប្រភេទឯកសារដែលត្រូវបានគេអនុញ្ញាតគឺ \$2។",
'filetype-missing'            => 'ឯកសារ មិនមានកន្ទុយ (ដូចជា ".jpg")។',
'large-file'                  => 'ឯកសារ​គួរតែ​មាន​​ទំហំ​មិនលើសពី $1។ ឯកសារ​នេះមាន​ទំហំ $2។',
'largefileserver'             => 'ឯកសារនេះមានទំហំធំជាងទំហំដែលប្រព័ន្ឋបំរើការ(server)អនុញ្ញាត។',
'emptyfile'                   => 'ឯកសារដែលអ្នកបានដាក់បញ្ចេញ ហាក់បីដូចជាទទេ។​ នេះប្រហែលជាមកពីកំហុសនៃការសរសេរឈ្មោះឯកសារ។ ចូរពិនិត្យ ថាតើអ្នកពិតជាចង់ដាក់បញ្ចេញឯកសារនេះឬក៏អត់។',
'fileexists'                  => 'ឯកសារដែលមានឈ្មោះនេះមានរួចហើយ​ ចូរពិនិត្យ <strong><tt>$1</tt></strong> ប្រសិនបើអ្នកមិនច្បាស់ថាតើអ្នកចង់ប្តូរវាឬក៏អត់។',
'fileexists-extension'        => 'មាន​ឯកសារ​មួយ​ដែល​មាន​ឈ្មោះស្រដៀង​៖<br />
ឈ្មោះ​ឯកសារដែលបាន​ផ្ទុកឡើង​ ៖ <strong><tt>$1</tt></strong><br />
ឈ្មោះ​ឯកសារ​ដែល​មានស្រាប់​៖ <strong><tt>$2</tt></strong><br />
សូម​ជ្រើសរើសឈ្មោះ​ផ្សេងទៀត។',
'fileexists-thumb'            => "<center>'''រូបភាពមានស្រេច'''</center>",
'fileexists-thumbnail-yes'    => 'ឯកសារនេះទំនងជារូបភាពដែលបានបង្រួមទំហំ <i>(កូនរូបភាព thumbnail)</i>.
សូមពិនិត្យមើលឯកសារ <strong><tt>$1</tt></strong>។<br />
បើសិនជាឯកសារដែលអ្នកបានពិនិត្យខាងលើគឺជារូបភាពតែមួយដែលមានទំហំដើម នោះអ្នកមិនចាំបាច់ផ្ទុកឡើងនូវកូនរូបភាព (thumbnail) បន្ថែមទេ។',
'fileexists-forbidden'        => 'ឯកសារដែលមានឈ្មោះនេះមានរួចហើយ។ ចូរត្រលប់ក្រោយវិញ ហើយដាក់បញ្ចេញឯកសារដែលមានឈ្មោះថ្មីមួយ។​[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'ឯកសារដែលមានឈ្មោះនេះ មានរួចហើយនៅក្នុងកន្លែងដាក់ឯកសាររួម។

ចូរត្រឡប់ក្រោយវិញ​ហើយដាក់បញ្ចេញឯកសារនេះឡើងវិញ​ជាមួយ​នឹងឈ្មោះថ្មី។ [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'ឯកសារនេះជាច្បាប់ចំលងរបស់ {{PLURAL:$1|ឯកសារ|ឯកសារ}}ដូចតទៅនេះ៖',
'successfulupload'            => 'ផ្ទុកឯកសារឡើងដោយជោគជ័យ',
'uploadwarning'               => 'សូមប្រុងប្រយ័ត្ន!',
'savefile'                    => 'រក្សាឯកសារទុក',
'uploadedimage'               => 'បានផ្ទុកឡើង "[[$1]]"',
'overwroteimage'              => 'ដាក់បញ្ចេញនូវកំនែប្រែថ្មីរបស់"[[$1]]"',
'uploaddisabled'              => 'ការផ្ទុកឡើង ឯកសារនានា ត្រូវអសកម្ម',
'uploaddisabledtext'          => 'ការដាក់បញ្ចេញឯកសារគឺមិនអាចធ្វើទៅរួចចំពោះ {{SITENAME}}.',
'uploadcorrupt'               => 'ឯកសារ​នេះ​ខូច​ឬ​មានកន្ទុយដែលមិនត្រឹមត្រូវ។ សូម​ពិនិត្យ​មើល​វាឡើងវិញ​ ហើយ​ផ្ទុកឡើង​ម្តងទៀត។',
'uploadvirus'                 => 'ឯកសារមានមេរោគ! សេចក្តីលំអិត៖ $1',
'sourcefilename'              => 'ឈ្មោះឯកសារប្រភព៖',
'destfilename'                => 'ឈ្មោះឯកសារគោលដៅ៖',
'upload-maxfilesize'          => 'ទំហំអតិបរិមារបស់ឯកសា៖ $1',
'watchthisupload'             => 'តាមដានទំព័រនេះ',
'filewasdeleted'              => 'ឯកសារដែលមានឈ្មោះនេះត្រូវបានដាក់បញ្ចេញមុននេះ ហើយក៏ត្រូវបានគេលុបចេញទៅវិញផងដែរ។​​​​ អ្នកគួរតែពិនិត្យ$1​មុននឹង​បន្តបញ្ចេញ​វាម្តង​ទៀត​។​',
'upload-wasdeleted'           => "'''ប្រយ័ត្ន៖ អ្នក​កំពុង​ផ្ទុក​ឡើង​នូវ​ឯកសារ​មួយ​ដែល​ត្រូវ​បានលុបចោល​មុននេះ។'''

អ្នកគួរ​ពិចារណាថាតើការផ្ទុក​ឯកសារ​នេះ​ឡើងសមរម្យឬទេ​។
ប្រវត្តិ​នៃការលុបឯកសារ​នេះ​​ត្រូវបានដាក់​នៅទីនេះ​តាមការគួរ៖",

'upload-proto-error'      => 'ពិធីការដែលមិនត្រឹមត្រូវ',
'upload-proto-error-text' => 'ការផ្ទុកឡើងពីចំងាយត្រូវការ URL ដែលចាប់ផ្ដើម <code>http://</code> ឬ <code>ftp://</code>។',
'upload-file-error'       => 'កំហុសផ្នែកខាងក្នុង',
'upload-file-error-text'  => 'កំហុសផ្នែកខាងក្នុងបានកើតឡើង​ នៅពេលព្យាយាមបង្កើតឯកសារបណ្ដោះអាសន្នមួយ​នៅក្នុងប្រព័ន្ឋបំរើការ។

សូមទំនាក់ទំនង[[Special:ListUsers/sysop|អ្នកអភិបាល]]។',
'upload-misc-error'       => 'កំហុសចំលែក​ពេលផ្ទុកឡើង',
'upload-misc-error-text'  => 'កំហុសដែលមិនស្គាល់មួយបានកើតឡើងនៅក្នុងកំឡុងពេលផ្ទុកឡើង។ 

ចូរផ្ទៀងផ្ទាត់ថា URL គឺមានសុពលភាពហើយអាចដំនើរការ រួចហើយ​ព្យាយាមម្តងទៀត។ 

ប្រសិនបើបញ្ហានៅតែកើតឡើង សូមទំនាក់ទំនង[[Special:ListUsers/sysop|អ្នកអភិបាល]]។',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'  => 'មិនអាច ចូលទៅដល់ URL',
'upload-curl-error28' => 'ផ្ទុកឡើង បានផុតកំណត់អនុញ្ញាតិ',

'license'            => 'អាជ្ញាបណ្ណ',
'nolicense'          => 'មិនបានជ្រើសរើសយកទេ',
'license-nopreview'  => '(មិនទាន់មានការបង្ហាញការមើលជាមុនទេ)',
'upload_source_url'  => ' (URL ត្រឹមត្រូវនិងបើកចំហជាសាធារណៈ)',
'upload_source_file' => ' (ឯកសារក្នុងកុំព្យូទ័ររបស់អ្នក)',

# Special:ImageList
'imagelist_search_for'  => 'ស្វែងរកឈ្មោះមេឌា៖',
'imgfile'               => 'ឯកសារ',
'imagelist'             => 'បញ្ជីរូបភាព',
'imagelist_date'        => 'កាលបរិច្ឆេទ',
'imagelist_name'        => 'ឈ្មោះ',
'imagelist_user'        => 'អ្នកប្រើប្រាស់',
'imagelist_size'        => 'ទំហំ',
'imagelist_description' => 'ការពិពណ៌នា',

# Image description page
'filehist'                       => 'ប្រវត្តិ​ឯកសារ',
'filehist-help'                  => "ចុចលើ'''ម៉ោងនិងកាលបរិច្ឆេទ'''ដើម្បីមើលឯកសារដែលបានផ្ទុកនៅពេលនោះ។",
'filehist-deleteall'             => 'លុបទាំងអស់',
'filehist-deleteone'             => 'លុបចេញ',
'filehist-revert'                => 'ត្រឡប់',
'filehist-current'               => 'បច្ចុប្បន្ន',
'filehist-datetime'              => 'ម៉ោងនិងកាលបរិច្ឆេទ',
'filehist-user'                  => 'អ្នកប្រើប្រាស់',
'filehist-dimensions'            => 'វិមាត្រ',
'filehist-filesize'              => 'ទំហំឯកសារ',
'filehist-comment'               => 'យោបល់',
'imagelinks'                     => 'តំនភ្ជាប់​',
'linkstoimage'                   => '{{PLURAL:$1|ទំព័រ​|$1 ទំព័រ}} ខាងក្រោម​មានតំនភ្ជាប់មក​ឯកសារនេះ ៖',
'nolinkstoimage'                 => 'គ្មានទំព័រណាមួយដែលតភ្ជាប់មកឯកសារនេះទេ។',
'morelinkstoimage'               => 'មើល [[Special:WhatLinksHere/$1|តំនភ្ជាប់បន្ថែមទៀត]] ដែលតភ្ជាប់មកកាន់ឯកសារនេះ។',
'redirectstofile'                => '$1ឯកសារដូចតទៅនេះបញ្ជូនបន្ដមកឯកសារនេះ៖',
'duplicatesoffile'               => '$1ឯកសារដូចតទៅនេះជាច្បាប់ចំលងរបស់ឯកសារនេះ៖',
'sharedupload'                   => 'ឯកសារនេះ​ត្រូវបានផ្ទុកឡើង​ដើម្បីចែករំលែក និង អាចត្រូវបានប្រើប្រាស់​នៅគំរោង​ដទៃ ។',
'shareduploadwiki'               => 'សូម​មើល $1 សំរាប់ពត៌មានបន្ថែម ។',
'shareduploadwiki-desc'          => 'សេចក្តីអធិប្បាយរបស់ឯកសារដែលមាននៅក្នុង$1 គឺត្រូវបានបង្ហាញដូចខាងក្រោម។',
'shareduploadwiki-linktext'      => 'ទំព័រពិពណ៌នាអំពីឯកសារ',
'shareduploadduplicate'          => 'ឯកសារនេះគឺដូចគ្នាបេះបិទនឹង $1 នៅក្នុងកន្លែងដាក់ឯកសាររួម។',
'shareduploadduplicate-linktext' => 'ឯកសារមួយទៀត',
'shareduploadconflict'           => 'ឯកសារនេះគឺមានឈ្មោះដូចគ្នានឹង $1 នៅក្នុងកន្លែងដាក់ឯកសាររួម។',
'shareduploadconflict-linktext'  => 'ឯកសារមួយទៀត',
'noimage'                        => 'គ្មានរូបភាពដែលមានឈ្មោះនេះទេ។ ប៉ុន្តែអ្នកអាច $1 ។',
'noimage-linktext'               => 'ផ្ទុកឯកសារឡើង',
'uploadnewversion-linktext'      => 'ផ្ទុកឡើងមួយកំនែថ្មីនៃឯកសារនេះ',
'imagepage-searchdupe'           => 'ស្វែងរកឯកសារដូចគ្នាបេះបិត',

# File reversion
'filerevert'         => 'ត្រឡប់ $1',
'filerevert-legend'  => 'ត្រឡប់ឯកសារ',
'filerevert-comment' => 'យោបល់៖',
'filerevert-submit'  => 'ត្រឡប់',

# File deletion
'filedelete'                  => 'លុបចេញ $1',
'filedelete-legend'           => 'លុបឯកសារចោល',
'filedelete-intro'            => "អ្នកកំពុងលុបចេញ '''[[Media:$1|$1]]'''។",
'filedelete-comment'          => 'ហេតុផលចំពោះការលុបចេញ៖',
'filedelete-submit'           => 'លុបចេញ',
'filedelete-success'          => "'''$1''' ត្រូវបានលុបចោលហើយ",
'filedelete-nofile'           => "គ្មាន '''$1''' លើ {{SITENAME}}។",
'filedelete-iscurrent'        => 'អ្នកកំពុងតែមានបំនងលុបកំនែប្រែដ៏ថ្មីបំផុតរបស់ឯកសារនេះ។ ជាដំបូងចូរអ្នកធ្វើវាឱ្យទៅជាកំនែប្រែចាស់ជាមុនសិន។',
'filedelete-otherreason'      => 'មូលហេតុបន្ថែមផ្សេងទៀត៖',
'filedelete-reason-otherlist' => 'មូលហេតុផ្សេងទៀត',
'filedelete-reason-dropdown'  => '*ហេតុផលដែលលុបជារឿយៗ
**ការបំពានទៅលើកម្មសិទ្ធិបញ្ញា
**ឯកសារដែលចម្លងតាមគំរូ',
'filedelete-edit-reasonlist'  => 'មូលហេតុនៃការលុបការកែប្រែ',

# MIME search
'mimesearch' => 'ស្វែងរក MIME',
'mimetype'   => 'ប្រភេទ MIME ៖',
'download'   => 'ដោនឡូដ',

# Unwatched pages
'unwatchedpages' => 'ទំព័រមិនត្រូវបានតាមដាន',

# List redirects
'listredirects' => 'បញ្ជីនៃការបញ្ជូនបន្ត',

# Unused templates
'unusedtemplates'    => 'ទំព័រគំរូ​ដែលលែងត្រូវបានប្រើ',
'unusedtemplateswlh' => 'តំនភ្ជាប់ផ្សេងៗទៀត',

# Random page
'randompage'         => 'ទំព័រចៃដន្យ',
'randompage-nopages' => 'គ្មានទំព័រណាមួយក្នុងលំហឈ្មោះនេះទេ។',

# Random redirect
'randomredirect'         => 'ទំព័របញ្ជូនបន្តចៃដន្យ',
'randomredirect-nopages' => 'គ្មានទំព័របញ្ជូនបន្តណាមួយនៅក្នុងលំហឈ្មោះនេះទេ។',

# Statistics
'statistics'             => 'ស្ថិតិ',
'sitestats'              => 'ស្ថិតិ{{SITENAME}}',
'userstats'              => 'ស្ថិតិអ្នកប្រើប្រាស់',
'sitestatstext'          => "*បច្ចុប្បន្នមានទំព័រសរុបចំនួន{{PLURAL:\$1|'''១'''|'''\$1'''}}នៅក្នុងទិន្នន័យ។ ស្ថិតិនេះគឺរាប់បញ្ចូលទាំងទំព័រពិភាក្សា ទំព័រគំរូ ទំព័រចំណាត់ថ្នាក់ក្រុម ទំព័រជំនួយ ទំព័រអំពី{{SITENAME}} ទំព័របញ្ជូនបន្ត និងទំព័រផ្សេងៗជាច្រើនទៀត។ ប្រសិនបើមិនរាប់បញ្ចូលទំព័រទាំងនោះទេ ទំព័រដែលមានខ្លឹមសារល្អមាន'''\$2''' (មាន'''\$2'''អត្ថបទ)។

*មាន'''\$8'''ឯកសារ(រូបភាពនិងមេឌា)ត្រូវបានផ្ទុកឡើង ។

*គ្រប់ទំព័រត្រូវបានចូលមើលសរុបចំនួន '''\$3'''{{PLURAL:\$3|ដង|ដង}}​ និងត្រូវបានកែប្រែចំនួន '''\$4'''{{PLURAL:\$4|ដង|ដង}} ចាប់តាំងពីពេលដែល{{SITENAME}}ត្រូវបានតំឡើង។

*គិតជាមធ្យមមាន '''\$5''' កំនែប្រែក្នុងមួយទំព័រ និង '''\$6'''ដងនៃការចូលមើលក្នុងមួយកំនែប្រែ។

*<span class=\"plainlinks\">[http://www.mediawiki.org/wiki/Manual:Job_queue ជួរការងារ]</span>(job queue)ឥឡូវនេះមានប្រវែង '''\$7''' ។",
'userstatstext'          => "នៅពេលនេះមាន[[Special:ListUsers|អ្នកប្រើប្រាស់]]​ដែលបានចុះឈ្មោះចំនួន '''$1'''នាក់ ដែលក្នុងនោះមាន'''$2'''នាក់(ស្មើនឹង'''$4%''') {{PLURAL:$2|មានសិទ្ធិជា|មានសិទ្ធិជា}}$5 ។",
'statistics-mostpopular' => 'ទំព័រដែលត្រូវបានមើលច្រើនបំផុត',

'disambiguations' => 'ទំព័រមានន័យច្បាស់លាស់',

'doubleredirects'            => 'ទំព័របញ្ជូនបន្តទ្វេដង',
'double-redirect-fixed-move' => '[[$1]] ត្រូវបានដកចេញ។ វាត្រូវបានបញ្ជូនបន្តទៅ [[$2]]',
'double-redirect-fixer'      => 'អ្នកជួសជុលការបញ្ជូនបន្ត',

'brokenredirects'        => 'ការបញ្ជូនបន្តដែលខូច',
'brokenredirectstext'    => 'ការបញ្ជូនបន្ដដូចតទៅនេះ​សំដៅទៅ​ទំព័រមិនមាន។',
'brokenredirects-edit'   => '(កែប្រែ)',
'brokenredirects-delete' => '(លុបចេញ)',

'withoutinterwiki'         => 'ទំព័រ​គ្មានតំនភ្ជាប់ភាសា',
'withoutinterwiki-summary' => 'ទំព័រទាំងនេះ​មិនតភ្ជាប់​ទៅទំរង់ជាភាសាដទៃ៖',
'withoutinterwiki-legend'  => 'បុព្វបទ',
'withoutinterwiki-submit'  => 'បង្ហាញ',

'fewestrevisions' => 'ទំព័រដែលត្រូវបានកែប្រែតិច​បំផុត',

# Miscellaneous special pages
'nbytes'                  => '$1បៃ',
'ncategories'             => '$1 {{PLURAL:$1|ចំនាត់ថ្នាក់ក្រុម|ចំនាត់ថ្នាក់ក្រុម}}',
'nlinks'                  => '$1 {{PLURAL:$1|តំនភ្ជាប់|តំនភ្ជាប់}}',
'nmembers'                => '$1{{PLURAL:$1|សមាជិក|សមាជិក}}',
'nviews'                  => '$1 {{PLURAL:$1|ការចូលមើល}}',
'specialpage-empty'       => 'គ្មានលទ្ធផលសំរាប់របាយណ៍នេះទេ។',
'lonelypages'             => 'ទំព័រកំព្រា',
'lonelypagestext'         => 'ទំព័រដូចតទៅនេះមិនត្រូវបានភ្ជាប់ពីទំព័រដទៃនៅក្នុង {{SITENAME}}ទេ។',
'uncategorizedpages'      => 'ទំព័រគ្មានចំនាត់ថ្នាក់ក្រុម',
'uncategorizedcategories' => 'ចំនាត់ថ្នាក់ក្រុមដែលមិនត្រូវបានចាត់ជាថ្នាក់',
'uncategorizedimages'     => 'រូបភាពគ្មានចំនាត់ថ្នាក់ក្រុម',
'uncategorizedtemplates'  => 'ទំព័រគំរូគ្មានចំនាត់ថ្នាក់ក្រុម',
'unusedcategories'        => 'ចំនាត់ថ្នាក់ក្រុមដែលមិនត្រូវបានប្រើប្រាស់',
'unusedimages'            => 'ឯកសារ(មេឌា​ រូបភាព)ដែលមិនត្រូវបានប្រើប្រាស់',
'popularpages'            => 'ទំព័រដែលមានប្រជាប្រិយ',
'wantedcategories'        => 'ចំនាត់ថ្នាក់ក្រុមដែលគ្រប់គ្នាចង់បាន',
'wantedpages'             => 'ទំព័រ​ដែល​គ្រប់គ្នា​ចង់បាន',
'missingfiles'            => 'ឯកសារដែលបាត់',
'mostlinked'              => 'ទំព័រដែលត្រូវបានតភ្ជាប់មកច្រើនបំផុត',
'mostlinkedcategories'    => 'ចំនាត់ថ្នាក់ក្រុមដែលត្រូវបានតភ្ជាប់មកច្រើនបំផុត',
'mostlinkedtemplates'     => 'ទំព័រគំរូ​ដែលត្រូវបានប្រើប្រាស់​ច្រើនបំផុត',
'mostcategories'          => 'អត្ថបទដែលមានចំនាត់ថ្នាក់ក្រុមច្រើនបំផុត',
'mostimages'              => 'រូបភាពដែលត្រូវបានតភ្ជាប់មកច្រើនបំផុត',
'mostrevisions'           => 'អត្ថបទដែលត្រូវបានកែប្រែច្រើនបំផុត',
'prefixindex'             => 'លិបិក្រមបុព្វបទ',
'shortpages'              => 'ទំព័រខ្លីៗ',
'longpages'               => 'ទំព័រវែងៗ',
'deadendpages'            => 'ទំព័រ​ទាល់',
'deadendpagestext'        => 'ទំព័រដូចតទៅនេះមិនតភ្ជាប់ទៅទំព័រដទៃទៀតក្នុង {{SITENAME}} ទេ។',
'protectedpages'          => 'ទំព័រដែលត្រូវបានការពារ',
'protectedpages-indef'    => 'ចំពោះតែការការពារដែលមិនកំនត់ប៉ុណ្ណោះ',
'protectedpagestext'      => 'ទំព័រដូចតទៅនេះត្រូវបានការពារមិនអោយប្ដូរទីតាំងឬកែប្រែ',
'protectedpagesempty'     => '​មិន​មាន​ទំព័រ​ណា​ដែល​ត្រូវបាន​ការពារ ជាមួយប៉ារ៉ាម៉ែត​ទាំងនេះទេ។',
'protectedtitles'         => 'ចំនងជើងត្រូវបានការពារ',
'protectedtitlestext'     => 'ចំនងជើងទំព័រត្រូវបានការពារមិនអោយបង្កើត',
'protectedtitlesempty'    => 'មិនមានចំនងជើងណាដែលត្រូវបានការពារជាមួយនឹងប៉ារ៉ាម៉ែតទាំងនេះទេនាពេលថ្មីៗនេះ។',
'listusers'               => 'បញ្ជីអ្នកប្រើប្រាស់',
'newpages'                => 'ទំព័រថ្មីៗ',
'newpages-username'       => 'ឈ្មោះអ្នកប្រើប្រាស់៖',
'ancientpages'            => 'ទំព័រ​ចាស់ៗ',
'move'                    => 'ប្តូរទីតាំង',
'movethispage'            => 'ប្តូរទីតាំងទំព័រនេះ',
'unusedcategoriestext'    => 'ចំនាត់ថ្នាក់ក្រុមដូចតទៅនេះមាន ប៉ុន្តែគ្មាទំព័រណាឬចំនាត់ថ្នាក់ណាដែលប្រើប្រាស់ពួកវាទេ។',
'notargettitle'           => 'គ្មានគោលដៅ',
'nopagetitle'             => 'គ្មានទំព័រគោលដៅបែបនេះទេ',
'nopagetext'              => 'ទំព័រគោលដៅដែលអ្នកបានសំដៅទៅ មិនមានទេ។',
'pager-newer-n'           => '{{PLURAL:$1|ថ្មីជាង$1}}',
'pager-older-n'           => '{{PLURAL:$1|ចាស់ជាង$1}}',

# Book sources
'booksources'               => 'ប្រភពសៀវភៅ',
'booksources-search-legend' => 'ស្វែងរកប្រភពសៀវភៅ',
'booksources-go'            => 'ទៅ',
'booksources-text'          => 'ខាងក្រោមនេះជាបញ្ជីនៃតំនភ្ជាប់ទៅសៃថ៍នានាដែលលក់​សៀវភៅថ្មីនិងជជុះ ហើយអាចផ្ដល់ពត៌មានបន្ថែមផ្សេងទៀតអំពីសៀវភៅដែលអ្នកកំពុងស្វែងរក៖',

# Special:Log
'specialloguserlabel'  => 'អ្នកប្រើប្រាស់៖',
'speciallogtitlelabel' => 'ចំនងជើង៖',
'log'                  => 'កំនត់ហេតុ',
'all-logs-page'        => 'កំនត់ហេតុទាំងអស់',
'log-search-legend'    => 'ស្វែងរកកំណត់ហេតុ',
'log-search-submit'    => 'ទៅ',
'logempty'             => 'គ្មានអ្វីក្នុងកំណត់ហេតុត្រូវនឹងទំព័រនេះទេ។',
'log-title-wildcard'   => 'ស្វែងរកចំនងជើងចាប់ផ្តើមដោយឃ្លានេះ',

# Special:AllPages
'allpages'          => 'ទំព័រទាំងអស់',
'alphaindexline'    => 'ពីទំព័រ $1 ដល់ទំព័រ $2',
'nextpage'          => 'ទំព័របន្ទាប់ ($1)',
'prevpage'          => 'ទំព័រមុន ($1)',
'allpagesfrom'      => 'បង្ហាញទំព័រផ្តើមដោយ៖',
'allarticles'       => 'គ្រប់ទំព័រ',
'allinnamespace'    => 'គ្រប់ទំព័រ(លំហឈ្មោះ$1)',
'allnotinnamespace' => 'គ្រប់ទំព័រ(មិននៅក្នុងលំហឈ្មោះ$1)',
'allpagesprev'      => 'មុន',
'allpagesnext'      => 'បន្ទាប់',
'allpagessubmit'    => 'ទៅ',
'allpagesprefix'    => 'បង្ហាញទំព័រដែលចាប់ផ្តើមដោយ ៖',
'allpagesbadtitle'  => 'ចំនងជើងទំព័រដែលត្រូវបានផ្តល់ឱ្យគឺគ្មានសុពលភាពឬក៏មានបុព្វបទដែលមានអន្តរភាសាឬអន្តរវីគី។ ប្រហែលជាវាមានអក្សរមួយឬច្រើន ដែលមិនអាចត្រូវប្រើនៅក្នុងចំនងជើង។',
'allpages-bad-ns'   => '{{SITENAME}}មិនមានលំហឈ្មោះ"$1"ទេ។',

# Special:Categories
'categories'                    => 'ចំនាត់ថ្នាក់ក្រុម',
'categoriespagetext'            => 'ចំនាត់ថ្នាក់ក្រុមខាងក្រោមនេះមានអត្ថបទឬមេឌា។

[[Special:UnusedCategories|ចំនាត់ថ្នាក់ក្រុមមិនប្រើ]]ត្រូវបានបង្ហាញទីនេះ។
សូមមើលផងដែរ [[Special:WantedCategories|ចំនាត់ថ្នាក់ក្រុមដែលគ្រប់គ្នាចង់បាន]]។',
'categoriesfrom'                => 'បង្ហាញចំនាត់ថ្នាក់ក្រុមចាប់ផ្តើមដោយ៖',
'special-categories-sort-count' => 'តំរៀបតាមចំនួន',
'special-categories-sort-abc'   => 'តំរៀបតាមអក្ខរក្រម',

# Special:ListUsers
'listusersfrom'      => 'បង្ហាញអ្នកប្រើប្រាស់ចាប់ផ្តើមដោយ៖',
'listusers-submit'   => 'បង្ហាញ',
'listusers-noresult' => 'មិនមានអ្នកប្រើប្រាស់នៅក្នុងក្រុមនេះទេ។',

# Special:ListGroupRights
'listgrouprights'          => 'សិទ្ធិនិងក្រុមអ្នកប្រើប្រាស់',
'listgrouprights-summary'  => 'ខាងក្រោមនេះជាបញ្ជីរាយឈ្មោះក្រុមអ្នកប្រើប្រាស់ដែលបានកំនត់ជាមួយនឹងសិទ្ធិរបស់គេនៅលើវិគីនេះ។ មាន[[{{MediaWiki:Listgrouprights-helppage}}|ពត៌មានបន្ថែម]] អំពីសិទ្ធិផ្ទាល់ខ្លួន។',
'listgrouprights-group'    => 'ក្រុម',
'listgrouprights-rights'   => 'សិទ្ធិ',
'listgrouprights-helppage' => 'Help:ក្រុមនិងសិទ្ធិ',
'listgrouprights-members'  => '(បញ្ជីរាយនាមសមាជិក)',

# E-mail user
'mailnologin'     => 'មិនមានអាសយដ្ឋានផ្ញើទេ',
'mailnologintext' => 'អ្នកត្រូវតែ [[Special:UserLogin|ឡុកអ៊ីន]] និង មានអាសយដ្ឋានអ៊ីមែលមានសុពលភាពមួយ ក្នុង[[Special:Preferences|ចំនូលចិត្តនានារបស់អ្នក]] ដើម្បីផ្ញើអ៊ីមែលទៅ អ្នកប្រើប្រាស់ដទៃទៀត។',
'emailuser'       => 'អ៊ីមែល​ទៅកាន់​អ្នក​ប្រើប្រាស់នេះ',
'emailpage'       => 'ទំព័រផ្ញើអ៊ីមែល',
'usermailererror' => 'កំហុសឆ្គងក្នុងចំនងជើងអ៊ីមែល៖',
'defemailsubject' => 'អ៊ីមែលពី{{SITENAME}}',
'noemailtitle'    => 'គ្មានអាសយដ្ឋានអ៊ីមែល',
'noemailtext'     => 'អ្នកប្រើប្រាស់នេះមិនបានបញ្ជាក់អំពីអាសយដ្ឋានអ៊ីមែលដែលមានសុពលភាព ឬក៏មិនបានជ្រើសយកការទទួលអ៊ីមែលពីអ្នកដទៃ។',
'emailfrom'       => 'ពី៖',
'emailto'         => 'ទៅកាន់៖',
'emailsubject'    => 'ប្រធានបទ៖',
'emailmessage'    => 'សារ៖',
'emailsend'       => 'ផ្ញើ',
'emailccme'       => 'អ៊ីមែលមកខ្ញុំនូវច្បាប់ចំលងមួយនៃសាររបស់ខ្ញុំ។',
'emailccsubject'  => 'ច្បាប់ចំលងនៃសាររបស់អ្នកចំពោះ $1 ៖ $2',
'emailsent'       => 'អ៊ីមែលត្រូវបានផ្ញើទៅហើយ',
'emailsenttext'   => 'សារអ៊ីមែលរបស់អ្នកត្រូវបានផ្ញើរួចហើយ។',
'emailuserfooter' => 'អ៊ីមែលនេះត្រូវបានផ្ញើដោយ$1ទៅកាន់$2ដោយប្រើមុខងារ"អ៊ីមែលអ្នកប្រើប្រាស់"របស់{{SITENAME}}។',

# Watchlist
'watchlist'            => 'បញ្ជីតាមដានរបស់ខ្ញុំ',
'mywatchlist'          => 'បញ្ជីតាមដាន​',
'watchlistfor'         => "(សំរាប់ '''$1''')",
'nowatchlist'          => 'គ្មានអ្វីនៅក្នុងបញ្ជីតាមដានរបស់អ្នកទេ។',
'watchlistanontext'    => 'សូម $1 ដើម្បី​មើល​ឬ​កែប្រែ​របស់​ក្នុង​បញ្ជីតាមដាន​របស់អ្នក។',
'watchnologin'         => 'មិនបានឡុកអ៊ីន',
'watchnologintext'     => 'អ្នកចាំបាច់ត្រូវតែ[[Special:UserLogin|ឡុកអ៊ីន]]ដើម្បីកែប្រែបញ្ជីតាមដានរបស់អ្នក។',
'addedwatch'           => 'បានបន្ថែមទៅបញ្ជីតាមដាន',
'addedwatchtext'       => "ទំព័រ \"[[:\$1]]\" ត្រូវបានដាក់បញ្ចូលទៅក្នុង​[[Special:Watchlist|បញ្ជីតាមដាន]]របស់លោកអ្នកហើយ ។ រាល់ការផ្លាស់ប្តូរនៃទំព័រនេះ រួមទាំងទំព័រពិភាក្សារបស់វាផងដែរ នឹងត្រូវបានដាក់បញ្ចូលក្នុងបញ្ជីនៅទីនោះ។  ទំព័រនេះនឹងបង្ហាញជា'''អក្សរដិត''' នៅក្នុង [[Special:RecentChanges|បញ្ជីបំលាស់ប្តូរថ្មីៗ]] ងាយស្រួលក្នុងការស្វែងរក។ ប្រសិនបើលោកអ្នកចង់យកវាចេញពី [[Special:Watchlist|បញ្ជីតាមដាន]]របស់លោកអ្នក សូមចុច '''ឈប់តាមដាន''' នៅលើរបារចំហៀងផ្នែកខាងលើ។",
'removedwatch'         => 'ត្រូវបានដកចេញពីបញ្ជីតាមដាន',
'removedwatchtext'     => 'ទំព័រ "[[:$1]]" ត្រូវបានដកចេញពី[[Special:Watchlist|បញ្ជីតាមដាន]]របស់លោកអ្នកហើយ ។',
'watch'                => 'តាមដាន',
'watchthispage'        => 'តាមដានទំព័រនេះ',
'unwatch'              => 'ឈប់​តាមដាន',
'unwatchthispage'      => 'ឈប់តាមដាន',
'notanarticle'         => 'មិនមែនជាទំព័រមាតិកា',
'notvisiblerev'        => 'ការកែតំរូវត្រូវបានលុបចោល',
'watchnochange'        => 'មិនមានរបស់ដែលអ្នកកំពុងតាមដានណាមួយត្រូវបានគេកែប្រែក្នុងកំឡុងពេលដូលដែលបានបង្ហាញទេ។',
'watchlist-details'    => '$1ទំព័រនៅក្នុងបញ្ជីតាមដានរបស់អ្នកដោយមិនរាប់បញ្ចូលទំព័រពិភាក្សា។',
'wlheader-enotif'      => '* អនុញ្ញាតអោយមានការផ្ដល់ដំនឹងតាមរយៈអ៊ីមែល',
'wlheader-showupdated' => "* ទំព័រដែលត្រូវបានផ្លាស់ប្តូរតាំងពីពេលចូលមើលចុងក្រោយរបស់អ្នក ត្រូវបានបង្ហាញជា '''អក្សរដិត'''",
'watchmethod-recent'   => 'ឆែកមើលកំណែប្រែថ្មីៗចំពោះទំព័រត្រូវបានតាមដាន',
'watchmethod-list'     => 'ឆែកមើលទំព័រត្រូវបានតាមដានចំពោះកំណែប្រែថ្មីៗ',
'watchlistcontains'    => 'បញ្ជីតាមដាន របស់អ្នក មាន $1 {{PLURAL:$1|ទំព័រ|ទំព័រ}}។',
'iteminvalidname'      => "មានបញ្ហាជាមួយនឹង'$1'​។ ឈ្មោះគឺមិនត្រឹមត្រូវ...",
'wlnote'               => "ខាងក្រោមនេះជា {{PLURAL:$1|បំលាស់ប្តូរចុងក្រោយ|'''$1'''បំលាស់ប្តូរចុងក្រោយ}}ក្នុងរយះពេល{{PLURAL:$2|'''$2'''ម៉ោង}}ចុងក្រោយ។",
'wlshowlast'           => 'បង្ហាញ $1ម៉ោងចុងក្រោយ $2ថ្ងៃចុងក្រោយ ឬ$3',
'watchlist-show-bots'  => 'បង្ហាញកំនែប្រែរបស់រូបយន្ត',
'watchlist-hide-bots'  => 'លាក់ការកែប្រែធ្វើឡើងដោយរូបយន្ត',
'watchlist-show-own'   => 'បង្ហាញកំនែប្រែរបស់ខ្ញុំ',
'watchlist-hide-own'   => 'លាក់កំនែប្រែរបស់ខ្ញុំ',
'watchlist-show-minor' => 'បង្ហាញកំនែប្រែតិចតួច',
'watchlist-hide-minor' => 'លាក់កំនែប្រែតិចតួច',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'កំពុង​តាមដាន...',
'unwatching' => 'ឈប់​តាមដាន...',

'enotif_mailer'                => 'ភ្នាក់ងារផ្ញើអ៊ីមែលផ្ដល់ដំនឹងរបស់ {{SITENAME}}',
'enotif_reset'                 => 'កត់សំគាល់រាល់គ្រប់ទំព័រដែលបានចូលមើល',
'enotif_newpagetext'           => 'នេះជាទំព័រថ្មី។',
'enotif_impersonal_salutation' => 'អ្នកប្រើប្រាស់ {{SITENAME}}',
'changed'                      => 'បានផ្លាស់ប្តូរ',
'created'                      => 'បានបង្កើត',
'enotif_subject'               => 'ទំព័រ $PAGETITLE នៃ {{SITENAME}} ត្រូវបាន $CHANGEDORCREATED ដោយ $PAGEEDITOR',
'enotif_lastvisited'           => 'ពិនិត្យ $1 ចំពោះគ្រប់បំលាស់ប្តូរ តាំងពីពេលចូលមើល ចុងក្រោយ។',
'enotif_lastdiff'              => 'សូមពិនិត្យ$1ដើម្បីមើលបំលាស់ប្តូរនេះ។',
'enotif_anon_editor'           => 'អ្នកប្រើប្រាស់អនាមិក $1',
'enotif_body'                  => 'ជូនចំពោះ $WATCHINGUSERNAME ជាទីរាប់អាន,


ទំព័រ $PAGETITLE នៃ {{SITENAME}} ត្រូវបាន  $CHANGEDORCREATED ថ្ងៃ $PAGEEDITDATE ដោយ $PAGEEDITOR។ សូមមើល $PAGETITLE_URL សំរាប់​កំនែបច្ចុប្បន្ន។

$NEWPAGE

សេចក្តីសង្ខេប​នៃអ្នកកែប្រែ ៖ $PAGESUMMARY $PAGEMINOREDIT

ទាក់ទង​អ្នកកែប្រែ ៖

មែល ៖ $PAGEEDITOR_EMAIL

វិគី ៖ $PAGEEDITOR_WIKI

នឹងមិនមាន​ការផ្តល់ដំណឹង​ជាលាយលក្សណ៍អក្សរ​ផ្សេងទៀតទេ លើកលែងតែ​អ្នកចូលមើល​ទំព័រនេះ។ អ្នកក៏អាចធ្វើ​អោយ​ការផ្តល់ដំណឹង​ត្រលប់ទៅលើកទីសូន្យ​ចំពោះគ្រប់ទំព័រ​នៃ​បញ្ជីតាមដាន​របស់អ្នក។

ប្រព័ន្ធផ្តល់ដំណឹង {{SITENAME}} ដ៏ស្និទ្ធស្នាល​របស់អ្នក

--
ដើម្បីផ្លាស់ប្តូរ ការកំណត់បញ្ជីតាមដាន, សូមចូលមើល
{{fullurl:{{ns:special}}:Watchlist/edit}}

ប្រតិកម្ម និង ជំនួយបន្ថែម ៖
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'លុបទំព័រចេញ',
'confirm'                     => 'បញ្ជាក់ទទួលស្គាល់',
'excontent'                   => "ខ្លឹមសារគឺ៖ '$1'",
'excontentauthor'             => "អត្ថន័យគឺ៖ '$1' (ហើយអ្នករួមចំនែកតែម្នាក់គត់គឺ '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "អត្ថន័យមុនពេលលុបចេញ៖ '$1'",
'exblank'                     => 'ទំព័រទទេ',
'delete-confirm'              => 'លុប"$1"ចេញ',
'delete-legend'               => 'លុបចេញ',
'historywarning'              => 'ប្រយ័ត្ន៖ ទំព័រដែលអ្នកទំនងជានឹងលុបមានប្រវត្តិ៖',
'confirmdeletetext'           => 'អ្នកប្រុងនឹងលុបចេញទាំងស្រុង នូវទំព័រមួយដោយរួមបញ្ចូលទាំងប្រវត្តិកែប្រែរបស់វាផង។
សូមអ្នកអះអាងថា អ្នកពិតជាមានចេតនាធ្វើបែបហ្នឹង និងថាអ្នកបានយល់ច្បាស់ពីផលវិបាកទាំងឡាយដែលអាចកើតមាន និង​សូមអះអាងថា អ្នកធ្វើស្របតាម [[{{MediaWiki:Policy-url}}|គោលការណ៍]]។',
'actioncomplete'              => 'សកម្មភាពរួចរាល់ជាស្ថាពរ',
'deletedtext'                 => '"<nowiki>$1</nowiki>"ត្រូវបានលុបរួចហើយ។ សូមមើល$2ចំពោះបញ្ជីនៃការលុបនាមពេលថ្មីៗ។',
'deletedarticle'              => 'បានលុប"[[$1]]"',
'dellogpage'                  => 'កំនត់ហេតុនៃការលុប',
'dellogpagetext'              => 'ខាងក្រោមជាបញ្ជីនៃការលុបចេញថ្មីៗបំផុត។',
'deletionlog'                 => 'កំនត់ហេតុនៃការលុប',
'reverted'                    => 'បានត្រឡប់ ទៅកំណែមុន',
'deletecomment'               => 'មូលហេតុ៖',
'deleteotherreason'           => 'មូលហេតុបន្ថែមផ្សេងទៀត៖',
'deletereasonotherlist'       => 'មូលហេតុផ្សេងទៀត',
'deletereason-dropdown'       => '*ហេតុផលលុបជាទូទៅ
** សំណើរបស់អ្នកនិពន្ធ
** បំពានសិទ្ធិអ្នកនិពន្ធ
** អំពើបំផ្លាញទ្រព្យសម្បត្តិឯកជនឬសាធារណៈ',
'delete-edit-reasonlist'      => 'ពិនិត្យផ្ទៀងផ្ទាត់ហេតុផលនៃការលុប',
'rollback'                    => 'ត្រឡប់កំនែប្រែ',
'rollback_short'              => 'ត្រឡប់',
'rollbacklink'                => 'ត្រឡប់',
'cantrollback'                => 'មិនអាចត្រឡប់កំណែប្រែ។ អ្នករួមចំណែកចុងក្រោយទើបជាអ្នកនិពន្ធ​របស់ទំព័រនេះ។',
'editcomment'                 => 'វិចារក្នុងការកែប្រែ៖ "<i>$1</i>"។', # only shown if there is an edit comment
'revertpage'                  => 'បានត្រលប់កំនែប្រែដោយ[[Special:Contributions/$2|$2]] ([[User talk:$2|Talk]]) ទៅកំនែប្រែចុងក្រោយដោយ [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectlogpage'              => 'កំនត់ហេតុនៃការការពារ',
'protectlogtext'              => 'ខាងក្រោមនេះជាបញ្ជីនៃទំព័រដែលត្រូវបានចាក់សោនិងដោះសោ។

សូមមើល [[Special:ProtectedPages|បញ្ជីទំព័រត្រូវបានការពារ]]។',
'protectedarticle'            => 'បានការពារ"[[$1]]"',
'modifiedarticleprotection'   => 'បានផ្លាស់ប្តូរកំរិតការពារនៃ"[[$1]]"',
'unprotectedarticle'          => 'បានឈប់ការពារ៖ "[[$1]]"',
'protect-title'               => 'ការពារ "$1"',
'protect-legend'              => 'បញ្ជាក់ទទួលស្គាល់ការការពារ',
'protectcomment'              => 'យោបល់៖',
'protectexpiry'               => 'ផុតកំនត់៖',
'protect_expiry_invalid'      => 'ពេលវេលាផុតកំនត់ មិនត្រឹមត្រូវ។',
'protect_expiry_old'          => 'ពេលវេលាផុតកំនត់ ឋិតក្នុងអតីតកាល។',
'protect-unchain'             => 'ឈប់ហាមឃាត់ការអនុញ្ញាតប្តូរទីតាំង',
'protect-text'                => 'លោកអ្នកអាចមើលនិងផ្លាស់ប្តូរកំរិតការពារទីនេះចំពោះទំព័រ<strong><nowiki>$1</nowiki></strong>។',
'protect-locked-blocked'      => 'អ្នកមិនអាចប្តូរកំរិតការពារនៅក្នុងកំឡុងពេលដែលត្រូវបានគេរារាំង។ នេះគឺជាការរៀបចំថ្មីៗសំរាប់ទំព័រ<strong>$1</strong>:',
'protect-locked-access'       => 'គណនីរបស់អ្នកគ្មានការអនុញ្ញាតក្នុងការផ្លាស់ប្តូរ កំរិតកាពារទំព័រ ។
នេះជាការកំនត់បច្ចុប្បន្ន ចំពោះទំព័រ <strong>$1</strong> ៖',
'protect-cascadeon'           => 'បច្ចុប្បន្ន ទំព័រនេះ ត្រូវបានការពារ ព្រោះ វាបាន ថិតក្នុង {{PLURAL:$1|ទំព័រ, ដែលមាន|ទំព័រ, ដែលមាន}} ការការពារ ជាថ្នាក់ បានសកម្ម ​។ អ្នកអាច ផ្លាស់ប្តូរ កំរិតការពារ នៃ ទំព័រ, វានឹង មិនប៉ះពាល់ ការការពារ ជាថ្នាក់ ។',
'protect-default'             => '(លំនាំដើម)',
'protect-fallback'            => 'តំរូវអោយមានការអនុញ្ញាតនៃ "$1"',
'protect-level-autoconfirmed' => 'ហាមឃាត់អ្នកប្រើប្រាស់ដែលមិនទាន់ចុះឈ្មោះ',
'protect-level-sysop'         => 'សំរាប់តែអ្នកថែទាំប្រព័ន្ធ',
'protect-summary-cascade'     => 'ការពារជា​ថ្នាក់',
'protect-expiring'            => 'ផុតកំនត់ $1 (UTC)',
'protect-cascade'             => 'ការពារគ្រប់ទំព័រដែលឋិតក្នុងទំព័រនេះ (ការពារជាថ្នាក់)',
'protect-cantedit'            => 'អ្នកមិនអាចផ្លាស់ប្តូរកំរិតការពារនៃទំព័រនេះទេ ព្រោះអ្នកគ្មានការអនុញ្ញាតក្នុងការកែប្រែវា។',
'restriction-type'            => 'ការអនុញ្ញាត៖',
'restriction-level'           => 'កំរិត​នៃ​ការដាក់កំហិត ៖',
'minimum-size'                => 'ទំហំអប្បបរិមា',
'maximum-size'                => 'ទំហំអតិបរិមា:',
'pagesize'                    => '(បៃ)',

# Restrictions (nouns)
'restriction-edit'   => 'កែប្រែ',
'restriction-move'   => 'ប្តូរទីតាំង',
'restriction-create' => 'បង្កើត',
'restriction-upload' => 'ផ្ទុកឡើង',

# Restriction levels
'restriction-level-sysop'         => 'បានការពារពេញលេញ',
'restriction-level-autoconfirmed' => 'បានការពារពាក់កណ្តាល',
'restriction-level-all'           => 'គ្រប់កំរិត',

# Undelete
'undelete'                 => 'មើលទំព័រដែលត្រូវបានលុបចេញ',
'undeletepage'             => 'មើលហើយដាក់ឡើងវិញនូវទំព័រដែលបានលុប',
'undeletepagetitle'        => "'''ខាងក្រោមនេះមានកំនែប្រែដែលត្រូវបានលុបរបស់[[:$1]]'''.",
'viewdeletedpage'          => 'មើលទំព័រដែលត្រូវបានលុបចេញ',
'undeletehistorynoadmin'   => 'ទំព័រនេះត្រូវបានលុបចេញហើយ។
មូលហេតុចំពោះការលុបចេញ​គឺត្រូវបានបង្ហាញនៅក្នុង​សេចក្តីសង្ខេបខាងក្រោម ជាមួយគ្នានឹងសេចក្តីលំអិតនៃ​អ្នកប្រើប្រាស់​ដែលបានធ្វើការកែប្រែ​ទំព័រនេះ​មុនពេលវាត្រូវបាន​លុបចេញ។ 
ឃ្លាជាការពិតនៃ​ការត្រួតពិនិត្យកំនែប្រែឡើងវិញ​​ដែលត្រូវបានលុបចេញគឺមានសុពលភាពចំពោះតែ​អ្នកអភិបាលប៉ុណ្ណោះ។',
'undelete-nodiff'          => 'គ្មានការកែតំរូវពីមុនត្រូវបានឃើញទេ។',
'undeletebtn'              => 'ស្តារឡើងវិញ',
'undeletelink'             => 'ស្តារឡើងវិញ',
'undeletereset'            => 'ធ្វើអោយដូចដើមវិញ',
'undeletecomment'          => 'យោបល់៖',
'undeletedarticle'         => 'បានស្តារ"[[$1]]"ឡើងវិញ',
'undeletedrevisions'       => 'បានស្តារឡើងវិញនូវ{{PLURAL:$1|១កំណែ|$1កំណែ}}',
'undeletedrevisions-files' => 'បានស្តារឡើងវិញនូវ{{PLURAL:$1|១កំណែ|$1កំណែ}}និង{{PLURAL:$2|១ឯកសារ|$2ឯកសារ}}',
'undeletedfiles'           => '{{PLURAL:$1|១ ឯកសារ|$1 ឯកសារ}} ត្រូវបានស្ដារឡើងវិញ',
'cannotundelete'           => 'ឈប់លប់មិនសំរេច។

ប្រហែលជាមាននរណាម្នាក់ផ្សេងទៀតបានឈប់លុបទំព័រនេះមុនអ្នក។',
'undeletedpage'            => "<big>'''$1 ត្រូវបានស្តារឡើងវិញហើយ'''</big>

សូមចូលទៅ [[Special:Log/delete|កំនត់ហេតុនៃការលុបចោល]] ដើម្បីពិនិត្យមើលកំនត់ត្រានៃការលុបចោលនិងការស្ដារឡើងវិញនានា។",
'undelete-header'          => 'មើល[[Special:Log/delete|កំនត់ហេតុនៃការលុប]]ចំពោះទំព័រដែលត្រូវបានលុបថ្មីៗ។',
'undelete-search-box'      => 'ស្វែងរកទំព័រ ដែលបានត្រូវលុប',
'undelete-search-prefix'   => 'បង្ហាញទំព័រចាប់ផ្តើមដោយ៖',
'undelete-search-submit'   => 'ស្វែងរក',
'undelete-cleanup-error'   => 'កំហុស លុបចេញ បណ្ណសារ ដែលបានលែងប្រើប្រាស់ "$1" ។',
'undelete-error-short'     => 'កំហុស លែងលុបចេញ ឯកសារ ៖  $1',
'undelete-error-long'      => 'កំហុសផ្សេងៗបានកើតឡើងក្នុងពេលកំពុងឈប់លុបឯកសារនេះ៖
$1',

# Namespace form on various pages
'namespace'      => 'លំហឈ្មោះ៖',
'invert'         => 'ដាក់បញ្ច្រាស់ជំរើស',
'blanknamespace' => '(ទូទៅ)',

# Contributions
'contributions' => 'ការរួមចំនែក​របស់អ្នកប្រើប្រាស់',
'mycontris'     => 'ការរួមចំនែក',
'contribsub2'   => 'សំរាប់ $1 ($2)',
'nocontribs'    => 'គ្មានការផ្លាស់ប្តូរត្រូវបានឃើញដូចនឹងលក្ខណៈវិនិច្ឆ័យទាំងនេះ។',
'uctop'         => '(ទាន់សម័យ)',
'month'         => 'ខែ៖',
'year'          => 'ឆ្នាំ៖',

'sp-contributions-newbies'     => 'បង្ហាញតែការរួមចំនែករបស់អ្នកប្រើប្រាស់ថ្មីៗ',
'sp-contributions-newbies-sub' => 'ចំពោះគណនីថ្មីៗ',
'sp-contributions-blocklog'    => 'កំនត់ហេតុនៃការហាមឃាត់',
'sp-contributions-search'      => 'ស្វែងរកការរួមចំនែក',
'sp-contributions-username'    => 'អាសយដ្ឋាន IP ឬឈ្មោះអ្នកប្រើ៖',
'sp-contributions-submit'      => 'ស្វែងរក',

# What links here
'whatlinkshere'            => 'អ្វី​ដែលភ្ជាប់មកទីនេះ',
'whatlinkshere-title'      => 'ទំព័រនានាដែល​តភ្ជាប់​ទៅ "$1"',
'whatlinkshere-page'       => 'ទំព័រ៖',
'linklistsub'              => '(បញ្ជី​នៃ​តំនភ្ជាប់)',
'linkshere'                => "ទំព័រដូចតទៅ​នេះតភ្ជាប់មក '''[[:$1]]''' ៖",
'nolinkshere'              => "គ្មានទំព័រណាមួយតភ្ជាប់ទៅ '''[[:$1]]''' ទេ។",
'nolinkshere-ns'           => "គ្មានទំព័រណាមួយ តភ្ជាប់ ទៅ '''[[:$1]]''' ក្នុងវាលឈ្មោះ ដែលបានជ្រើសរើស។",
'isredirect'               => 'ទំព័របញ្ជូនបន្ត',
'istemplate'               => 'ការរាប់បញ្ចូល',
'isimage'                  => 'តំនភ្ជាប់ទៅរូបភាព',
'whatlinkshere-prev'       => '{{PLURAL:$1|មុន|មុន $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|បន្ទាប់|បន្ទាប់ $1}}',
'whatlinkshere-links'      => '← តំនភ្ជាប់',
'whatlinkshere-hideredirs' => '$1ការបញ្ជូនបន្ត',
'whatlinkshere-hidelinks'  => '$1តំនភ្ជាប់',
'whatlinkshere-hideimages' => '$1តំនភ្ជាប់រូបភាព',
'whatlinkshere-filters'    => 'តំរងការពារនានា',

# Block/unblock
'blockip'                     => 'ហាមឃាត់អ្នកប្រើប្រាស់',
'blockip-legend'              => 'ហាមឃាត់អ្នកប្រើប្រាស់',
'blockiptext'                 => 'សូមប្រើប្រាស់សំនុំបែបបទខាងក្រោមដើម្បីរាំងខ្ទប់ការសរសេរពីអាសយដ្ឋាន IP ឬឈ្មោះអ្នកប្រើប្រាស់ កំនត់មួយ។
ការធ្វើបែបនេះគួរតែធ្វើឡើងក្នុងគោលបំនងបង្ការការប៉ុនប៉ងបំផ្លាញ (vandalism)ដូចដែលមានចែងក្នុង[[{{MediaWiki:Policy-url}}|គោលការណ៍]]។
សូមបំពេញមូលហេតុច្បាស់លាស់មួយខាងក្រោម (ឧទាហរណ៍៖ រាយឈ្មោះទំព័រនានាដែលត្រូវបានគេបំផ្លាញ)។',
'ipaddress'                   => 'អាសយដ្ឋាន IP ៖',
'ipadressorusername'          => 'អាសយដ្ឋាន IP ឬឈ្មោះអ្នកប្រើ៖',
'ipbexpiry'                   => 'រយៈពេលផុតកំណត់៖',
'ipbreason'                   => 'មូលហេតុ៖',
'ipbreasonotherlist'          => 'មូលហេតុផ្សេងទៀត',
'ipbreason-dropdown'          => '*មូលហេតុហាមឃាត់ជាទូទៅ
** ដាក់បញ្ចូលពត៌មានមិនពិត
** ដកខ្លឹមទាំងស្រុងពីទំព័រ
** Spamming links to external sites
** Inserting nonsense/gibberish into pages
** Intimidating behaviour/harassment
** Abusing multiple accounts
** ប្រើប្រាស់ឈ្មោះដែលមិនអាចទទួលយកបាន',
'ipbanononly'                 => 'ហាមឃាត់តែអ្នកប្រើប្រាស់ជាអនាមិកជនប៉ុណ្ណោះ',
'ipbcreateaccount'            => 'ការពារការបង្កើតគណនី',
'ipbemailban'                 => 'ការពារអ្នកប្រើប្រាស់ពីការផ្ញើរអ៊ីមែល',
'ipbsubmit'                   => 'ហាមឃាត់អ្នកប្រើប្រាស់នេះ',
'ipbother'                    => 'រយៈពេលផ្សេងទៀត៖',
'ipboptions'                  => '២ម៉ោង:2 hours,១ថ្ងៃ:1 day,៣ថ្ងៃ:3 days,១សប្តាហ៍:1 week,២សប្តាហ៍:2 weeks,១ខែ:1 month,៣ខែ:3 months,៦ខែ:6 months,១ឆ្នាំ:1 year,គ្មានកំណត់:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'ផ្សេងៗទៀត',
'ipbotherreason'              => 'មូលហេតុ(ផ្សេងទៀតឬបន្ថែម)៖',
'ipbwatchuser'                => 'តាមដានទំព័រអ្នកប្រើប្រាស់និងទំព័រពិភាក្សារបស់អ្នកប្រើប្រាស់នេះ។',
'badipaddress'                => 'អាសយដ្ឋានIPមិនត្រឹមត្រូវ',
'blockipsuccesssub'           => 'បានហាមឃាត់ដោយជោគជ័យ',
'ipb-edit-dropdown'           => 'កែប្រែ ហេតុផល រាំងខ្ទប់',
'ipb-unblock-addr'            => 'ឈប់ហាមឃាត់$1',
'ipb-unblock'                 => 'លែងរាំងខ្ទប់ អ្នកប្រើប្រាស់ ឬ អាស័យដ្ឋាន IP',
'ipb-blocklist-addr'          => 'មើលការហាមឃាត់ដែលមានស្រេចសំរាប់$1',
'ipb-blocklist'               => 'មើលការហាមឃាត់ដែលមានស្រេច',
'unblockip'                   => 'ឈប់ហាមឃាត់អ្នកប្រើប្រាស់',
'unblockiptext'               => 'សូមប្រើប្រាស់ទំរង់បែបបទខាងក្រោមនេះ ដើម្បីបើកសិទ្ឋិសរសេរឡើងវិញ សំរាប់អាសយដ្ឋានIPឬអ្នកប្រើប្រាស់ដែលត្រូវបានរាំងខ្ទប់ពីមុន។',
'ipusubmit'                   => 'លែងរាំងខ្ទប់ អាស័យដ្ឋាន នេះ',
'unblocked'                   => '[[User:$1|$1]] ត្រូវបានឈប់ហាមឃាត់',
'unblocked-id'                => '$1 ត្រូវបានឈប់ហាមឃាត់ហើយ',
'ipblocklist'                 => 'ឈ្មោះអ្នកប្រើប្រាស់ និង អាសយដ្ឋាន IP ដែលត្រូវបានរាំងខ្ទប់',
'ipblocklist-legend'          => 'រកមើល អ្នកប្រើប្រាស់ ដែលត្រូវបានរាំងខ្ទប់',
'ipblocklist-username'        => 'ឈ្មោះអ្នកប្រើឬអាសយដ្ឋានIP៖',
'ipblocklist-submit'          => 'ស្វែងរក',
'blocklistline'               => '$1, $2 បានហាមឃាត់ $3 (រយៈពេល$4)',
'infiniteblock'               => 'គ្មានកំណត់',
'expiringblock'               => 'ផុតកំណត់ $1',
'anononlyblock'               => 'សំរាប់តែអនាមិកជនប៉ុណ្ណោះ',
'noautoblockblock'            => 'ការហាមឃាត់ដោយស្វ័យប្រវត្តិមិនត្រូវបានអនុញ្ញាតទេ',
'createaccountblock'          => 'ការបង្កើតគណនីត្រូវបានហាមឃាត់',
'emailblock'                  => 'អ៊ីមែលដែលត្រូវបានហាមឃាត់',
'ipblocklist-empty'           => 'បញ្ជីរហាមឃាត់គឺទទេ។',
'ipblocklist-no-results'      => 'អាសយដ្ឋានIPឬឈ្មោះអ្នកប្រើដែលបានស្នើសុំគឺមិនត្រូវបានរារាំងទេ។',
'blocklink'                   => 'ហាមឃាត់',
'unblocklink'                 => 'ឈប់ហាមឃាត់',
'contribslink'                => 'ការរួមចំនែក',
'autoblocker'                 => 'អ្នកបានត្រូវបានហាមឃាត់ដោយស្វ័យប្រវត្តិ ពីព្រោះអាសយដ្ឋានIPរបស់អ្នកត្រូវបានប្រើប្រាស់ដោយ"[[User:$1|$1]]"។ មូលហេតុលើកឡើងចំពោះការហាមឃាត់$1គឺ៖ "$2"',
'blocklogpage'                => 'កំនត់ហេតុនៃការហាមឃាត់',
'blocklogentry'               => 'បានហាមឃាត់[[$1]]​ដោយរយៈពេលផុតកំនត់$2 $3',
'blocklogtext'                => 'នេះជាកំណត់ហេតុនៃការហាមឃាត់និងឈប់ហាមឃាត់អ្នកប្រើប្រាស់។ អាសយដ្ឋានIPដែលត្រូវបានហាមឃាត់ដោយស្វ័យប្រវត្តិមិនត្រូវបានដាក់ក្នុងបញ្ជីនេះទេ។ សូមមើល[[Special:IPBlockList|បញ្ជីនៃការហាមឃាត់IP]]ចំពោះបញ្ជីនៃហាមឃាត់នាថ្មីៗ។',
'unblocklogentry'             => 'បានឈប់ហាមឃាត់ $1',
'block-log-flags-anononly'    => 'សំរាប់តែ អ្នកប្រើប្រាស់អនាមិក',
'block-log-flags-nocreate'    => 'ការបង្កើតគណនីត្រូវបានហាមឃាត់',
'block-log-flags-noautoblock' => 'ការហាមឃាត់ដោយស្វ័យប្រវត្តិមិនត្រូវបានអនុញ្ញាតទេ',
'block-log-flags-noemail'     => 'អ៊ីមែលត្រូវបានហាមឃាត់',
'ipb_expiry_invalid'          => 'កាលបរិច្ឆេទផុតកំណត់មិនត្រឹមត្រូវទេ។',
'ipb_already_blocked'         => '"$1"ត្រូវបានរាំងខ្ទប់ហើយ',
'ip_range_invalid'            => 'ដែនកំណត់ IP គ្មានសុពលភាព។',
'blockme'                     => 'ហាមឃាត់ខ្ញុំ',
'proxyblocker-disabled'       => 'មុខងារនេះត្រូវបានអសកម្ម។',
'proxyblockreason'            => 'អាសយដ្ឋាន IP របស់អ្នកត្រូវបានរាំងខ្ទប់ហើយ ពីព្រោះវាជាប្រុកស៊ី(proxy)ចំហ។

សូមទំនាក់ទំនងអ្នកផ្ដល់សេវាអ៊ីនធឺណិតឬអ្នកបច្ចេកទេសរបស់អ្នក ហើយប្រាប់ពួកគេពីបញ្ហាសុវត្ថិភាពដ៏សំខាន់នេះ។',
'proxyblocksuccess'           => 'រួចរាល់ជាស្ថាពរ។',
'sorbsreason'                 => 'អាសយដ្ឋាន IP របស់អ្នកមានឈ្មោះក្នុងបញ្ជីប្រុកស៊ី(proxy)ចំហ នៅក្នុង DNSBL របស់ {{SITENAME}}។',
'sorbs_create_account_reason' => 'អាសយដ្ឋាន IP របស់អ្នកមានឈ្មោះក្នុងបញ្ជីប្រុកស៊ី(proxy)ចំហ នៅក្នុង DNSBL របស់ {{SITENAME}}។

អ្នកមិនអាចបង្កើតគណនីបានទេ',

# Developer tools
'lockdb'              => 'ចាក់សោមូលដ្ឋានទិន្នន័យ',
'unlockdb'            => 'ដោះសោមូលដ្ឋានទិន្នន័យ',
'lockconfirm'         => 'បាទ/ចាស, ខ្ញុំពិតជាចង់ចាក់សោមូលដ្ឋានទិន្នន័យមែន។',
'unlockconfirm'       => 'បាទ/ចាស, ខ្ញុំពិតជាចង់ដោះសោមូលដ្ឋានទិន្នន័យមែន។',
'lockbtn'             => 'ចាក់សោមូលដ្ឋានទិន្នន័យ',
'unlockbtn'           => 'ដោះសោមូលដ្ឋានទិន្នន័យ',
'locknoconfirm'       => 'អ្នកមិនបានឆែកមើលប្រអប់បញ្ជាក់ទទួលស្គាល់ទេ។',
'lockdbsuccesssub'    => 'មូលដ្ឋានទិន្នន័យត្រូវបានចាក់សោរដោយជោគជ័យ',
'unlockdbsuccesssub'  => 'សោ មូលដ្ឋានទិន្នន័យ ត្រូវបានដកចេញ',
'lockdbsuccesstext'   => 'មូលដ្ឋានទិន្នន័យត្រូវបានចាក់សោ។<br />
កុំភ្លេច [[Special:UnlockDB|ដោះសោ]] បន្ទាប់ពីបញ្ជប់ការថែទាំរបស់អ្នក។',
'unlockdbsuccesstext' => 'មូលដ្ឋានទិន្នន័យត្រូវបានដោះសោរួចហើយ។',
'databasenotlocked'   => 'មូលដ្ឋានទិន្នន័យ មិនត្រូវបានចាក់សោ។',

# Move page
'move-page'               => 'ប្តូរទីតាំង $1',
'move-page-legend'        => 'ប្តូរទីតាំងទំព័រ',
'movepagetext'            => "ការប្រើប្រាស់ទំរង់ខាងក្រោមនឹងប្តូរឈ្មោះទំព័រ  ប្តូរទីតាំងគ្រប់ប្រវត្តិរបស់វាទៅឈ្មោះថ្មី ។
ចំណងជើងចាស់នឹងក្លាយជាទំព័រប្តូរទិសទៅចំណងជើងថ្មី ។
តំណភ្ជាប់ ទៅ ចំណងជើង នៃ ទំព័រចាស់ នឹងមិនបានត្រូវ ផ្លាស់ប្តូរ; សូមឆែកមើល ការប្តូរទិស មិនបានបង្កើត ទំព័រប្តូរទិសទ្វេ ឬ ទំព័រប្តូរទិសបាក់ ។
អ្នកត្រូវតែធានាប្រាកដ ថា តំណភ្ជាប់ទាំងនោះ បន្តសំដៅ ទៅ គោលដៅបានសន្មត ។

ទំព័រចាស់ នឹង'''មិន'''ត្រូវ បានប្តូរទីតាំង កាលបើ មានទំព័រ ក្នុងចំណងជើងថ្មី ។ បើគ្មានទំព័រ ក្នុងចំណងជើងថ្មី, ទំព័ចាស់ នឹង ទទេ ឬ ជា ទំព័រប្តូរទិស និង គ្មានប្រវត្តិកំណែប្រែ ។ វាមានន័យថា អ្នកអាចប្តូរឈ្មោះទំព័រ ទៅទីតាំងដើម ករណី អ្នកបានធ្វើកំហុស, និង ដែលអ្នកមិនអាច សរសេរជាន់ពីលើ ទំព័រមានស្រាប់ ។

'''ប្រយ័ត្ន!'''
វាអាចជា បំលាស់ប្តូរដល់ឫសគល់ និង មិននឹកស្មានជាមុន ចំពោះ ទំព័រប្រជាប្រិយ ។ អ្នកត្រូវតែ ដឹងប្រាកដ អំពី ផលវិបាកទាំងអស់ មុននឹង បន្តទង្វើនេះ ។",
'movepagetalktext'        => "ទំព័រ សហពិភាក្សា, បើមាន, នឹងត្រូវបាន ប្តូរឈ្មោះ ដោយស្វ័យប្រវត្តិ   '''លើកលែងតែ ៖'''
*អ្នក ប្តូរឈ្មោះ ទំព័រ ទៅ វាលផ្សេង
*ទំព័រពិភាក្សា មានហើយ ត្រង់ ឈ្មោះថ្មី, ឬ
*អ្នក បានលែង \"ឆែក\" ប្រអប់ ខាងក្រោម ។

ក្នុងករណី ទាំងនោះ, អ្នកត្រូវតែ ប្តូរឈ្មោះទំព័រ ឬ បញ្ចូលរួមគ្នា ដោយដៃ បើចង់ ។",
'movearticle'             => 'ប្តូរទីតាំងទំព័រ៖',
'movenotallowed'          => 'អ្នកគ្មានការអនុញ្ញាតក្នុងការប្តូរទីតាំងទំព័រលើ {{SITENAME}} ទេ។',
'newtitle'                => 'ទៅចំនងជើងថ្មី៖',
'move-watch'              => 'តាមដានទំព័រនេះ',
'movepagebtn'             => 'ប្តូរទីតាំង',
'pagemovedsub'            => 'ប្តូទីតាំងដោយជោគជ័យ',
'movepage-moved'          => '<big>\'\'\'"$1"ត្រូវបានប្តូរទីតាំងទៅ"$2"\'\'\'ហើយ</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'ទំព័រដែលមានឈ្មោះបែបនេះមានរួចហើយ ឬ ឈ្មោះដែលអ្នកបានជ្រើសរើសមិនត្រឹមត្រូវ។
សូមជ្រើសរើសឈ្មោះមួយផ្សេងទៀត។',
'cantmove-titleprotected' => 'អ្នកមិនអាច​ប្តូទីតាំង ទំព័រ​ ទៅទីតាំងនេះ, ព្រោះ ចំណងជើងថ្មី បានត្រូវការពារ ចំពោះការបង្កើតវា',
'talkexists'              => "'''ទំព័រ ខ្លួនវា បានត្រូវប្តូរទីតាំង ដោយជោគជ័យ, ប៉ុន្តែ ទំព័រពិភាក្សា មិនអាចត្រូវបាន ប្តូរទីតាំង ព្រោះ នៅមាន មួយទំព័រពិភាក្សា នៅ ចំណងជើងថ្មី  ។ សូម បញ្ចូលរួមគ្នា ពួកវា ដោយដៃ ។'''",
'movedto'                 => 'បានប្តូរទីតាំងទៅ',
'movetalk'                => 'ប្តូរទីតាំងទំព័រសហពិភាក្សា',
'move-subpages'           => 'ប្តូរទីតាំងគ្រប់ទំព័ររងប្រសិនបើអាច',
'move-talk-subpages'      => 'ប្តូរទីតាំងគ្រប់ទំព័ររងនៃទំព័រពិភាក្សាប្រសិនបើអាច',
'movepage-page-moved'     => 'ទំព័រ$1ត្រូវបានប្តូរទីតាំងទៅកាន់$2ហើយ។',
'movepage-page-unmoved'   => 'ទំព័រ$1មិនអាចប្តូរទីតាំងទៅ$2បានទេ។',
'1movedto2'               => 'បានប្តូរទីតាំង [[$1]] ទៅ [[$2]]',
'1movedto2_redir'         => 'ទំព័រ [[$1]] ត្រូវបានប្តូរទីតាំងទៅ [[$2]] តាមរយៈការបញ្ជូនបន្ត។',
'movelogpage'             => 'កំនត់ហេតុនៃការប្តូរទីតាំង',
'movelogpagetext'         => 'ខាងក្រោមនេះជាបញ្ជីនៃទំព័រដែលត្រូវបានប្តូរទីតាំង។',
'movereason'              => 'មូលហេតុ៖',
'revertmove'              => 'ត្រឡប់',
'delete_and_move'         => 'លុបនិងប្តូរទីតាំង',
'delete_and_move_text'    => '==ការលុបជាចាំបាច់==
"[[:$1]]"ដែលជាទីតាំងទំព័រត្រូវបញ្ជូនទៅ មានរួចជាស្រេចហើយ។
តើអ្នកចង់លុបវាដើម្បីជាវិធីសំរាប់ប្តូរទីតាំងទេ?',
'delete_and_move_confirm' => 'បាទ/ចាស, លុបចេញទំព័រ',
'delete_and_move_reason'  => 'បានលុបដើម្បីផ្លាស់ប្តូរទីតាំង',
'selfmove'                => 'ចំនងជើងប្រភពនិងចំនងជើងគោលដៅគឺតែមួយ។

មិនអាចប្ដូរទីតាំងទំព័រមួយទៅលើខ្លួនវាបានទេ។',
'imageinvalidfilename'    => 'ឈ្មោះឯកសារគោលដៅមិនត្រឹមត្រូវ',

# Export
'export'            => 'នាំទំព័រចេញ',
'exporttext'        => 'អ្នកអាចនាំចេញ អត្ថបទ និង ប្រវត្តិកែប្រែ នៃ​ មួយទំព័រ ឬ នៃ មួយសំណុំទំព័រ ទៅ ក្នុង ឯកសារ XML ។ ឯកសារ​ទាំងនេះ អាចត្រូវបាន នាំចេញទៅ វិគី ផ្សេង ដែលមានប្រើប្រាស់ មីឌាវិគី តាម រយះ [[Special:Import|នាំចូល ទំព័រ]]។

ដើម្បី នាំចេញ ទំព័រ, អ្នកត្រូវ បញ្ចូលចំណងជើង ក្នុងប្រអប់អត្ថបទ ខាងក្រោម, មួយចំណងជើង ក្នុងមួយបន្ទាត់, និង ជ្រើសយក កំណែ តាមបំណង របស់អ្នក (កំណែចាស់ ឬ កំណែថ្មី), រួមនឹង ប្រវត្តិ នៃ​ទំព័រ, ឬ ត្រឹមតែ កំណែបច្ចុប្បន្ន ដែលមានពត៌មាន អំពី កំណែប្រែ ចុងក្រោយ។

ក្នុងករណី បន្ទាប់ អ្នកអាចប្រើប្រាស់ តំណភ្ជាប់, ដូចជា [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] ចំពោះ ទំព័រ "[[{{MediaWiki:Mainpage}}]]"។',
'export-submit'     => 'នាំចេញ',
'export-addcattext' => 'បន្ថែមទំព័រនានាពីចំណាត់ថ្នាក់ក្រុម៖',
'export-addcat'     => 'បន្ថែម',
'export-download'   => 'រក្សាទុកជាឯកសារ',
'export-templates'  => 'រួមទាំងទំព័រគំរូ',

# Namespace 8 related
'allmessages'         => 'សាររបស់ប្រព័ន្ធ',
'allmessagesname'     => 'ឈ្មោះ',
'allmessagesdefault'  => 'អត្ថបទលំនាំដើម',
'allmessagescurrent'  => 'អត្ថបទបច្ចុប្បន្ន',
'allmessagesfilter'   => 'កំរងឈ្មោះសារ៖',
'allmessagesmodified' => 'បង្ហាញតែការកែសំរួល',

# Thumbnails
'thumbnail-more'           => 'ពង្រីក',
'filemissing'              => 'ឯកសារបាត់បង់',
'thumbnail_error'          => 'កំហុស​បង្កើត​កូនរូបភាព៖ $1',
'djvu_page_error'          => 'ទំព័រ DjVu ក្រៅដែនកំណត់',
'djvu_no_xml'              => 'មិនអាចនាំយក XML សំរាប់ឯកសារ DjVu',
'thumbnail_invalid_params' => 'តួលេខ កូនទំព័រ គ្មានសុពលភាព',
'thumbnail_dest_directory' => 'មិនអាចបង្កើតថតឯកសារតាមគោលដៅបានទេ',

# Special:Import
'import'                     => 'ការនាំចូលទំព័រ',
'importinterwiki'            => 'ការនាំចូលអន្តរវិគី',
'import-interwiki-history'   => 'ចំលង គ្រប់កំណែចាស់ នៃទំព័រនេះ',
'import-interwiki-submit'    => 'នាំចូល',
'import-interwiki-namespace' => 'បញ្ជូនទំព័រទៅក្នុងលំហឈ្មោះ៖',
'importstart'                => 'កំពុងនាំចូលទំព័រ...',
'importnopages'              => 'មិមានទំព័រត្រូវនាំចូលទេ។',
'importfailed'               => 'ការនាំចូល ត្រូវបរាជ័យ ៖ <nowiki>$1</nowiki>',
'importunknownsource'        => 'មិនស្គាល់ ប្រភេទ នៃប្រភពនាំចូល',
'importcantopen'             => 'មិនអាចបើក ឯកសារនាំចូល',
'importbadinterwiki'         => 'តំនភ្ជាប់អន្តរវិគីមិនត្រឹមត្រូវ',
'importnotext'               => 'ទទេ ឬ គ្មានអត្ថបទ',
'importsuccess'              => 'នាំចូល ត្រូវបានបញ្ចប់!',
'importnofile'               => 'គ្មានឯកសារនាំចូល មួយណា ត្រូវបាន ផ្ទុកឡើង​។',
'importuploaderrorsize'      => 'ការផ្ទុកឡើងឯកសារនាំចូលបានបរាជ័យ។ ឯកសារនេះមានទំហំធំជាងទំហំដែលគេអនុញ្ញាតអោយផ្ទុកឡើង។',
'importuploaderrorpartial'   => 'ការផ្ទុកឡើងឯកសារនាំចូលបានបរាជ័យ។ ឯកសារនេះត្រូវបានផ្ទុកឡើងបានទើបតែមួយផ្នែកប៉ុណ្ណោះ។',
'importuploaderrortemp'      => 'ការផ្ទុកឡើងឯកសារនាំចូលបានបរាជ័យ។ កំពុងបាត់ថតឯកសារបណ្ដោះអាសន្នមួយ។',
'import-noarticle'           => 'គ្មានទំព័រណាមួយត្រូវនាំចូល!',
'xml-error-string'           => '$1 នៅ ជួរដេក $2, ជួរឈរ $3 (បៃ $4) ៖ $5',
'import-upload'              => 'ផ្ទុកឡើងទិន្នន័យ XML',

# Import log
'importlogpage'                    => 'កំនត់ហេតុនៃការនាំចូល',
'import-logentry-upload'           => 'បាននាំចូល [[$1]] ដោយការផ្ទុកឡើង ឯកសារ',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|កំណែ}}',
'import-logentry-interwiki'        => 'បាននាំចូល$1ពីវិគីផ្សេងទៀត',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|កំណែ}} ពី $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'ទំព័រអ្នកប្រើប្រាស់​របស់ខ្ញុំ',
'tooltip-pt-mytalk'               => 'ទំព័រពិភាក្សា​របស់ខ្ញុំ',
'tooltip-pt-anontalk'             => 'ការពិភាក្សាអំពីកំណែប្រែពីអាសយដ្ឋានIPនេះ',
'tooltip-pt-preferences'          => 'ចំនង់ចំនូលចិត្ត',
'tooltip-pt-watchlist'            => 'បញ្ជី​នៃ​ទំព័រ​ដែលអ្នកកំពុង​ត្រួតពិនិត្យ​រក​បំលាស់ប្តូរ',
'tooltip-pt-mycontris'            => 'បញ្ជីរួមចំនែក​របស់ខ្ញុំ',
'tooltip-pt-login'                => 'អ្នកត្រូវបានលើកទឹកចិត្តអោយឡុកអ៊ីន។ ប៉ុន្តែនេះមិនមែនជាការបង្ខំទេ។',
'tooltip-pt-anonlogin'            => 'អ្នកត្រូវបានលើកទឹកចិត្តអោយឡុកអ៊ីន មិនមែនជាការបង្ខំទេ។',
'tooltip-pt-logout'               => 'ចាកចេញ',
'tooltip-ca-talk'                 => 'ការពិភាក្សា​អំពីទំព័រខ្លឹមសារ​នេះ',
'tooltip-ca-edit'                 => "អ្នកអាច​កែប្រែ​ទំព័រ​នេះ ។ សូមប្រើប្រាស់​ប៊ូតុង 'បង្ហាញការមើលមុន' មុននឹង​រក្សាទុក​វា  ។",
'tooltip-ca-addsection'           => 'បន្ថែមមួយវិចារទៅការពិភាក្សានេះ។',
'tooltip-ca-viewsource'           => 'ទំព័រ​នេះ​បានត្រូវការពារ ។ អ្នកអាច​មើល​អក្សរកូដ​របស់វា ។',
'tooltip-ca-history'              => 'កំណែកន្លងមក នៃ ទំព័រនេះ ។',
'tooltip-ca-protect'              => 'ការពារ​ទំព័រនេះ',
'tooltip-ca-delete'               => 'លុបទំព័រនេះចេញ',
'tooltip-ca-move'                 => 'ប្តូរទីតាំង​ទំព័រនេះ',
'tooltip-ca-watch'                => 'បន្ថែមទំព័រនេះ​ទៅបញ្ជីតាមដាន​របស់អ្នក',
'tooltip-ca-unwatch'              => 'ដកចេញទំព័រនេះពីបញ្ជីតាមដានរបស់ខ្ញុំ',
'tooltip-search'                  => 'ស្វែងរក {{SITENAME}}',
'tooltip-search-go'               => 'ទៅទំព័រ ដែលមាន ឈ្មោះត្រឹមត្រូវតាមនេះ បើមាន',
'tooltip-search-fulltext'         => 'ស្វែងរកទំព័រនានាសំរាប់ឃ្លានេះ',
'tooltip-p-logo'                  => 'ទំព័រដើម',
'tooltip-n-mainpage'              => 'ចូលមើលទំព័រដើម',
'tooltip-n-portal'                => 'អំពីគំរោង, វិធីប្រើប្រាស់ និង ការស្វែងរកពត៌មាន',
'tooltip-n-currentevents'         => 'រកមើលពត៌មានទាក់ទិននឹងព្រឹត្តិការណ៍បច្ចុប្បន្ន',
'tooltip-n-recentchanges'         => 'បញ្ជី​នៃ​បំលាស់ប្តូរថ្មីៗ​នៅក្នុងវិគី។',
'tooltip-n-randompage'            => 'ផ្ទុក​ទំព័រចៃដន្យ​មួយទំព័រ',
'tooltip-n-help'                  => 'ជំនួយ​បន្ថែម',
'tooltip-t-whatlinkshere'         => 'បញ្ជី​ទំព័វិគី​ទាំងអស់​ដែលតភ្ជាប់​នឹងទីនេះ',
'tooltip-t-recentchangeslinked'   => 'បំលាស់ប្តូរថ្មីៗក្នុងទំព័រដែលត្រូវបានភ្ជាប់មកទំព័រនេះ',
'tooltip-feed-rss'                => 'បំរែបំរួល RSS ចំពោះទំព័រនេះ',
'tooltip-feed-atom'               => 'បំរែបំរួល Atom ចំពោះទំព័រនេះ',
'tooltip-t-contributions'         => 'បង្ហាញបញ្ជីរួមចំនែករបស់អ្នកប្រើប្រាស់នេះ',
'tooltip-t-emailuser'             => 'ផ្ញើអ៊ីមែលទៅកាន់អ្នកប្រើប្រាស់នេះ',
'tooltip-t-upload'                => 'ឯកសារផ្ទុកឡើង',
'tooltip-t-specialpages'          => 'បញ្ជីទំព័រពិសេសៗទាំងអស់',
'tooltip-t-print'                 => 'ទំរង់សំរាប់បោះពុម្ពចំពោះទំព័រនេះ',
'tooltip-t-permalink'             => 'តំនភ្ជាប់អចិន្ត្រៃយ៍ចំពោះកំនែនៃទំព័រនេះ',
'tooltip-ca-nstab-main'           => 'មើលទំព័រមាតិកា',
'tooltip-ca-nstab-user'           => 'មើលទំព័រអ្នកប្រើប្រាស់',
'tooltip-ca-nstab-media'          => 'មើលទំព័រមេឌា',
'tooltip-ca-nstab-special'        => 'នេះជាទំព័រពិសេស​មួយ។ អ្នកមិនអាចកែប្រែទំព័រនេះបានទេ។',
'tooltip-ca-nstab-project'        => 'មើលទំព័រគំរោង',
'tooltip-ca-nstab-image'          => 'មើលទំព័រ​ឯកសារ',
'tooltip-ca-nstab-mediawiki'      => 'មើលសាររបស់ប្រព័ន្ធ',
'tooltip-ca-nstab-template'       => 'មើលទំព័រគំរូ',
'tooltip-ca-nstab-help'           => 'មើលទំព័រជំនួយ',
'tooltip-ca-nstab-category'       => 'មើល​ទំព័រ​ចំនាត់ថ្នាក់ក្រុម',
'tooltip-minoredit'               => 'ចំនាំ​កំនែប្រែនេះ​ថាជា កំនែប្រែ​តិចតួច',
'tooltip-save'                    => 'រក្សាបំលាស់ប្តូររបស់អ្នកទុក',
'tooltip-preview'                 => 'មើលមុន​បំលាស់ប្តូរ​របស់អ្នក។ សូមប្រើប្រាស់​វា​មុននឹង​រក្សាទុក!',
'tooltip-diff'                    => 'បង្ហាញ​បំលាស់ប្តូរ​ដែលអ្នកបានធ្វើ​​ចំពោះអត្ថបទ។',
'tooltip-compareselectedversions' => 'មើលភាពខុសគ្នា​រវាងកំនែ​ដែលបានជ្រើសយកទាំង២ នៃទំព័រ​នេះ។',
'tooltip-watch'                   => 'បន្ថែម​ទំព័រនេះ​ទៅ​បញ្ជីតាមដាន​របស់អ្នក',
'tooltip-recreate'                => 'បង្កើតទំព័រនេះឡើងវិញ ទោះបីជាវាបានត្រូវលុបចេញក៏ដោយ',
'tooltip-upload'                  => 'ចាប់ផ្តើមផ្ទុកឡើងឯកសារ',

# Stylesheets
'common.css'   => '/* CSS បានដាក់ទីនេះនឹងមានអនុភាពលើគ្រប់សំបកទាំងអស់ */',
'monobook.css' => '/* CSS បានដាក់ទីនេះនឹងមានអនុភាពលើអ្នកប្រើប្រាស់នៃសំបកសៀវភៅឯក */',

# Attribution
'anonymous'        => 'អ្នកប្រើប្រាស់អនាមិក នៃ {{SITENAME}}',
'siteuser'         => 'អ្នកប្រើប្រាស់$1នៃ{{SITENAME}}',
'lastmodifiedatby' => 'ទំព័រនេះត្រូវបានប្តូរចុងក្រោយដោយ$3នៅវេលា$2,$1។', # $1 date, $2 time, $3 user
'othercontribs'    => 'ផ្អែកលើការងាររបស់$1។',
'others'           => 'ផ្សេងៗទៀត',
'siteusers'        => 'អ្នកប្រើប្រាស់ $1 នៃ {{SITENAME}}',

# Spam protection
'spamprotectiontitle' => 'តំរងការពារស្ប៉ាម(Spam)',
'spamprotectiontext'  => 'ទំព័រដែលអ្នកចង់រក្សាទុកត្រូវបានរាំងខ្ទប់ដោយតំរងការពារស្ប៉ាម(spam)។

នេះប្រហែលជាមកពីទំព័រនេះមានតំនភ្ជាប់ទៅសៃថ៍ខាងក្រៅដែលមានឈ្មោះក្នុងបញ្ជីខ្មៅ។',
'spambot_username'    => 'ការសំអាតស្ប៉ាម(spam)របស់ MediaWiki',

# Info page
'infosubtitle'   => 'ពត៌មានសំរាប់ទំព័រ',
'numedits'       => 'ចំនួននៃកំនែប្រែ (អត្ថបទ)៖ $1',
'numtalkedits'   => 'ចំនួននៃកំនែប្រែ (ទំព័រពិភាក្សា)៖ $1',
'numwatchers'    => 'ចំនួនអ្នកតាមដាន ៖ $1',
'numauthors'     => 'ចំនួនអ្នកនិពន្ឋ (អត្ថបទ): $1',
'numtalkauthors' => 'ចំនួនអ្នកនិពន្ធ (ទំព័រពិភាក្សា): $1',

# Math options
'mw_math_png'    => 'ជានិច្ចការជាPNG',
'mw_math_simple' => 'ជា HTML បើសាមញ្ញបំផុត ឬ ផ្ទុយទៅវិញ ជា PNG',
'mw_math_html'   => 'ជា HTML បើអាចទៅរួច ឬ ផ្ទុយទៅវិញ ជា PNG',
'mw_math_source' => 'ទុកអោយនៅជា TeX (ចំពោះឧបករណ៍រាវរកអត្ថបទ)',
'mw_math_modern' => 'បានផ្តល់អនុសាសន៍ចំពោះកម្មវិធីរាវរកទំនើបៗ',
'mw_math_mathml' => 'MathML បើអាចទៅរួច (ពិសោធ)',

# Patrolling
'markaspatrolleddiff'    => 'ចំនាំថាបានល្បាត',
'markaspatrolledtext'    => 'ចំនាំទំព័រនេះថាបានល្បាត',
'markedaspatrolled'      => 'បានចំណាំថាបានល្បាត',
'rcpatroldisabled'       => 'បំលាស់ប្តូរថ្មីៗនៃការតាមដានមិនត្រូវបានអនុញ្ញាតទេ',
'markedaspatrollederror' => 'មិនអាចគូសចំនាំថាបានល្បាត',

# Patrol log
'patrol-log-page' => 'កំនត់ហេតុនៃការតាមដាន',
'patrol-log-line' => 'បានចំណាំការល្បាត $1 នៃ $2 ថា បានត្រួតពិនិត្យ $3',
'patrol-log-auto' => '(ស្វ័យប្រវត្តិ)',

# Image deletion
'deletedrevision'                 => 'កំនែចាស់ដែលត្រូវបានលុបចេញ $1',
'filedeleteerror-short'           => 'កំហុសនៃការលុបឯកសារ៖ $1',
'filedeleteerror-long'            => 'កំហុសពេលលុបឯកសារចេញ៖

$1',
'filedelete-missing'              => 'មិនអាចលុប ឯកសារ "$1"  ព្រោះ វាមិនមាន។',
'filedelete-current-unregistered' => 'ឯកសារ "$1" មិនមាន ក្នុងមូលដ្ឋានទិន្នន័យ។',
'filedelete-archive-read-only'    => 'ម៉ាស៊ីនបំរើសេវាវ៉ែប មិនអាច សរសេរទុក ថតបណ្ណសារ "$1" ។',

# Browsing diffs
'previousdiff' => '← កំនែប្រែមុននេះ',
'nextdiff'     => 'កំនែប្រែបន្ទាប់ →',

# Media information
'mediawarning'         => "'''បំរាម''' ៖ ឯកសារនេះអាចមានផ្ទុកកូដពិសពុល កុំព្យូទ័ររបស់អ្នកអាចមានគ្រោះថ្នាក់បើអោយវាមានដំណើរការ។<hr />",
'imagemaxsize'         => 'កំណត់ទំហំរូបភាពលើទំព័រពិពណ៌នារូបភាពត្រឹម៖',
'thumbsize'            => 'ទំហំកូនរូបភាព៖',
'widthheightpage'      => '$1×$2, $3{{PLURAL:$3|ទំព័រ|ទំព័រ}}',
'file-info'            => '(ទំហំឯកសារ៖ $1, ប្រភេទ MIME ៖ $2)',
'file-info-size'       => '($1 × $2 ភីកសែល ទំហំឯកសារ៖ $3 ប្រភេទ MIME៖ $4)',
'file-nohires'         => '<small>គ្មានភាពម៉ត់ ដែលខ្ពស់ជាង។</small>',
'svg-long-desc'        => '(ឯកសារប្រភេទSVG  $1 × $2 ភីកសែល ទំហំឯកសារ៖ $3)',
'show-big-image'       => 'រូបភាពពេញ',
'show-big-image-thumb' => '<small>ទំហំ​នៃការមើលជាមុននេះ៖ $1 × $2 ភីកសែល</small>',

# Special:NewImages
'newimages'             => 'វិចិត្រសាលរូបភាពថ្មីៗ',
'imagelisttext'         => "ខាងក្រោមនេះជាបញ្ជី'''$1'''{{PLURAL:$1|ឯកសារ|ឯកសារ}}បានរៀបតាមលំដាប់$2។",
'newimages-summary'     => 'ទំព័រពិសេសនេះបង្ហាញឯកសារដែលផ្ទុកឡើងចុងក្រោយគេ។',
'showhidebots'          => '($1រូបយន្ត)',
'noimages'              => 'គ្មានឃើញអី សោះ។',
'ilsubmit'              => 'ស្វែងរក',
'bydate'                => 'តាមកាលបរិច្ឆេទ',
'sp-newimages-showfrom' => 'បង្ហាញឯកសារថ្មីៗចាប់ពី$2 $1',

# Bad image list
'bad_image_list' => 'ទំរង់ ដូចតទៅ ៖

មានតែ បញ្ជីរាយមុខរបស់ (ឃ្លា ផ្តើមដោយ *) ត្រូវបាន យកជាការ ។ តំណភ្ជាប់ដំបូង នៃឃ្លា ត្រូវតែ ជាតំណភ្ជាប់ ទៅ មួយរូបភាពអន់ ។
តំណភ្ជាប់បន្ទាប់ លើឃ្លាតែមួយ ត្រូវបានយល់ថា ជា ករណីលើកលែង, ឧទាហរ ទំព័រ ដែលលើនោះ រូបភាព អាចនឹងលេចឡើង ។',

# Metadata
'metadata'          => 'ទិន្នន័យ​មេតា',
'metadata-help'     => 'ឯកសារនេះ​មាន​ពត៌មានបន្ថែម​ដែល​ទំនងជា​បានបន្ថែម​ពី ឧបករណ៍ថតរូបឌីជីថល ឬ ស្កេននើរ ដែលត្រូវបាន​ប្រើប្រាស់​ដើម្បីបង្កើត ឬ ធ្វើ​វា​ជា​ឌីជីថល។ បើសិនឯកសារ​បានត្រូវ​កែប្រែ​ពី ស្ថានភាពដើម នោះសេចក្តីលំអិតខ្លះ​អាចនឹងមិនអាច​​ឆ្លុះ​បញ្ចាំង​ពេញលេញទៅឯកសារ​ដែលបានកែប្រែទេ។',
'metadata-expand'   => 'បង្ហាញភាពលំអិត',
'metadata-collapse' => 'លាក់ភាពលំអិតដែលបានពន្លាត',
'metadata-fields'   => 'វាលទិន្នន័យមេតា EXIF ដែលបានរាយ​ក្នុងសារនេះ​នឹងត្រូវដាក់ក្នុង​ទំព័រ​ពិពណ៌នារូបភាព ពេល​តារាង​ទិន្នន័យមេតា​ត្រូវបានបង្រួមតូច ។ ពត៌មាន​ដទៃទៀត​នឹងត្រូវបាន បិទបាំង​តាមលំនាំដើម ។
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'ទទឹង',
'exif-imagelength'                 => 'កំពស់',
'exif-bitspersample'               => 'Bits per component',
'exif-orientation'                 => 'Orientation',
'exif-planarconfiguration'         => 'ការរៀបចំទិន្នន័យ',
'exif-stripoffsets'                => 'ទីតាំងទិន្នន័យរូបភាព',
'exif-jpeginterchangeformatlength' => 'ទំហំជាបៃនៃទិន្នន័យJPEG',
'exif-datetime'                    => 'ការផ្លាស់ប្តូរឯកសារ ថ្ងៃហើយនិងពេលវេលា',
'exif-imagedescription'            => 'ចំណងជើងរូបភាព',
'exif-make'                        => 'ក្រុមហ៊ុនផលិតកាមេរ៉ា',
'exif-model'                       => 'ម៉ូដែលកាមេរ៉ា',
'exif-software'                    => 'សូហ្វវែរត្រូវបានប្រើប្រាស់',
'exif-artist'                      => 'អ្នកនិពន្ធ',
'exif-exifversion'                 => 'កំណែ នៃ Exif',
'exif-flashpixversion'             => 'បានគាំទ្រ កំណែ Flashpix',
'exif-colorspace'                  => 'លំហពណ៌',
'exif-compressedbitsperpixel'      => 'កំរិតហាប់ នៃរូបភាព (ប៊ិត/ចំណុច)',
'exif-pixelydimension'             => 'ទទឹងសមស្រប នៃរូបភាព',
'exif-pixelxdimension'             => 'កំពស់សមស្រប នៃរូបភាព',
'exif-makernote'                   => 'កំនត់​ត្រារបស់​អ្នកផលិត',
'exif-usercomment'                 => 'យោបល់របស់អ្នកប្រើប្រាស់',
'exif-relatedsoundfile'            => 'ឯកសារសំលេងពាក់ព័ន្ធ',
'exif-datetimeoriginal'            => 'ពេលវេលានិងកាលបរិច្ឆេទបង្កើតទិន្នន័យ',
'exif-datetimedigitized'           => 'ពេលវេលានិងការបរិច្ឆេទធ្វើជាឌីជីថល',
'exif-exposuretime-format'         => '$1វិនាទី($2)',
'exif-lightsource'                 => 'ប្រភពពន្លឺ',
'exif-focallength'                 => 'ប្រវែង​កំនុំ​ឡង់ទី',
'exif-filesource'                  => 'ប្រភពឯកសារ',
'exif-saturation'                  => 'តិត្ថិភាព',
'exif-gpslatituderef'              => 'រយៈទទឹង​ខាងជើងឬខាងត្បូង',
'exif-gpslatitude'                 => 'រយៈទទឹង',
'exif-gpslongituderef'             => 'រយៈបណ្ដោយ​ខាងកើតឬខាងលិច',
'exif-gpslongitude'                => 'រយៈបណ្តោយ',
'exif-gpsaltitude'                 => 'រយៈកំពស់',
'exif-gpsdestdistance'             => 'ចំងាយ​ទៅ​គោលដៅ',
'exif-gpsareainformation'          => 'ឈ្មោះ នៃ តំបន់ GPS',
'exif-gpsdatestamp'                => 'កាលបរិច្ឆេទ GPS',

# EXIF attributes
'exif-compression-1' => 'លែងបានបង្ហាប់',

'exif-unknowndate' => 'មិនដឹងកាលបរិច្ឆេទ',

'exif-orientation-1' => 'ធម្មតា', # 0th row: top; 0th column: left
'exif-orientation-3' => 'ត្រូវបាន​បង្វិល 180°', # 0th row: bottom; 0th column: right
'exif-orientation-6' => 'បានបង្វិល 90° តាមទិសទ្រនិចនាឡិកា', # 0th row: right; 0th column: top
'exif-orientation-8' => 'បានបង្វិល 90° ច្រាស់ទិសទ្រនិចនាឡិកា', # 0th row: left; 0th column: bottom

'exif-componentsconfiguration-0' => 'មិនមាន',

'exif-exposureprogram-0' => 'មិនត្រូវបានកំណត់',

'exif-subjectdistance-value' => '$1ម៉ែត្រ',

'exif-meteringmode-0'   => 'មិនបានស្គាល់',
'exif-meteringmode-1'   => 'មធ្យម',
'exif-meteringmode-255' => 'ផ្សេង',

'exif-lightsource-0'   => 'មិនដឹង',
'exif-lightsource-1'   => 'ពន្លឺថ្ងៃ',
'exif-lightsource-2'   => 'អំពូលម៉ែត',
'exif-lightsource-3'   => 'អំពូលតឹងស្តែន (ចង្កៀងរង្គុំ)',
'exif-lightsource-255' => 'ប្រភពពន្លឺដទៃ',

'exif-focalplaneresolutionunit-2' => 'អ៊ិន្ឈ៍',

'exif-sensingmethod-1' => 'មិនត្រូវបានកំណត់',

'exif-scenecapturetype-0' => 'ស្តង់ដារ',
'exif-scenecapturetype-1' => 'រូបផ្តេក',
'exif-scenecapturetype-2' => 'រូបបញ្ឈរ',

'exif-gaincontrol-0' => 'ទទេ',

'exif-contrast-0' => 'ធម្មតា',
'exif-contrast-1' => 'ស្រទន់',

'exif-saturation-0' => 'ធម្មតា',

'exif-sharpness-0' => 'ធម្មតា',
'exif-sharpness-1' => 'ស្រទន់',

'exif-subjectdistancerange-0' => 'មិនដឹង',
'exif-subjectdistancerange-1' => 'ម៉ាក្រូ',
'exif-subjectdistancerange-2' => 'បិទការមើល',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'ខាងជើង',
'exif-gpslatitude-s' => 'ខាងត្បូង',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'ខាងកើត',
'exif-gpslongitude-w' => 'ខាងលិច',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'គីឡូម៉ែត្រក្នុងមួយម៉ោង',
'exif-gpsspeed-m' => 'ម៉ាយល៍ក្នុងមួយម៉ោង',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'ខាងជើងពិត',
'exif-gpsdirection-m' => 'ខាងជើងម៉ាញេទិក',

# External editor support
'edit-externally'      => 'កែប្រែ​ឯកសារ​នេះដោយប្រើប្រាស់​កម្មវិធី​ខាងក្រៅ',
'edit-externally-help' => 'សូមមើល[http://www.mediawiki.org/wiki/Manual:External_editors ណែនាំ​ប្រើប្រាស់]សំរាប់​ពត៌មានបន្ថែម ។',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'ទាំងអស់',
'imagelistall'     => 'ទាំងអស់',
'watchlistall2'    => 'ទាំងអស់',
'namespacesall'    => 'ទាំងអស់',
'monthsall'        => 'ទាំងអស់',

# E-mail address confirmation
'confirmemail'             => 'បញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែល',
'confirmemail_noemail'     => 'អ្នកមិនមានអាសយដ្ឋានអ៊ីមែលត្រឹមត្រូវមួយ ដាក់នៅក្នុង[[Special:Preferences|ចំនង់ចំនូលចិត្ត]]របស់អ្នកទេ។',
'confirmemail_send'        => 'ផ្ញើកូដបញ្ជាក់ការទទួលស្គាល់',
'confirmemail_sent'        => 'ការបញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែលត្រូវបានផ្ញើទៅរួចហើយ។',
'confirmemail_invalid'     => 'កូដបញ្ជាក់ទទួលស្គាល់មិនត្រឹមត្រូវទេ។
កូដនេះប្រហែលជាផុតកំនត់ហើយ។',
'confirmemail_needlogin'   => 'អ្នកត្រូវការ$1ដើម្បីបញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែលរបស់អ្នក។',
'confirmemail_success'     => 'អាសយដ្ឋានអ៊ីមែលរបស់អ្នកត្រូវបានបញ្ជាក់ទទួលស្គាល់ហើយ។ ពេលនេះអ្នកអាចឡុកអ៊ីន និងចូលរួមសប្បាយរីករាយជាមួយវិគីបានហើយ។',
'confirmemail_loggedin'    => 'អាសយដ្ឋានអ៊ីមែលរបស់អ្នកត្រូវបានបញ្ជាក់ទទួលស្គាល់ហើយនាពេលនេះ។',
'confirmemail_error'       => 'រក្សាទុក ​ការបញ្ជាក់ទទួលស្គាល់ របស់អ្នក មានបញ្ហា ។',
'confirmemail_subject'     => 'ការបញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែល{{SITENAME}}',
'confirmemail_body'        => 'នរណាម្នាក់ ប្រហែលជាអ្នកពីអាសយដ្ឋានIP $1,
បានចុះបញ្ជីគណនី "$2" ជាមួយនឹងអាសយដ្ឋានអ៊ីមែលនេះនៅលើ{{SITENAME}}។

ដើម្បីបញ្ជាក់ថានេះពិតជាគណនីផ្ទាល់របស់អ្នកមែន សូមធ្វើអោយអ៊ីមែលរបស់អ្នកមានដំណើរការឡើងនៅលើ{{SITENAME}} ដោយបើកតំនភ្ជាប់ខាងក្រោមនេះក្នុងកម្មវិធីរុករករបស់អ្នក៖

$3

ប្រសិនបើអ្នក*មិនបាន*ចុះបញ្ជីគណនីនេះទេ សូមបើកតំនភ្ជាប់ខាងក្រោម ដើម្បីបោះបង់ចោលនូវការបញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែលនេះ៖

$5

កូដដើម្បីទទួលស្គាល់នេះនឹងផុតកំនត់នៅ  $4 ។',
'confirmemail_invalidated' => 'ការអះអាងបញ្ជាក់ទទួលស្គាល់អាសយដ្ឋានអ៊ីមែលបានបោះបង់ចោលហើយ',
'invalidateemail'          => 'បោះបង់ចោលការបញ្ជាក់ទទួលស្គាល់អ៊ីមែល',

# Scary transclusion
'scarytranscludetoolong' => '[URL វែងជ្រុល]',

# Trackbacks
'trackbackremove' => ' ([$1 លុបចេញ])',

# Delete conflict
'deletedwhileediting' => "'''ប្រយ័ត្ន''' ៖ ទំព័រនេះបានត្រូវលុបចោល បន្ទាប់ពីអ្នកបានចាប់ផ្តើមកែប្រែ!",
'confirmrecreate'     => "អ្នកប្រើប្រាស់ [[User:$1|$1]] ([[User talk:$1|talk]]) បានលុបទំព័រនេះចោលបន្ទាប់ពីអ្នកចាប់ផ្ដើមកែប្រែវា ដោយមានហេតុផលថា៖

៖ ''$2''

សូមអះអាងថាអ្នកពិតជាចង់បង្កើតទំព័រនេះឡើងវិញពិតប្រាកដមែន។",
'recreate'            => 'បង្កើតឡើងវិញ',

# HTML dump
'redirectingto' => 'កំពុងប្តូរទិស ទៅ [[:$1]]...',

# action=purge
'confirm_purge'        => 'សំអាតឃ្លាំងសំងាត់(cache)នៃទំព័រនេះ?

$1',
'confirm_purge_button' => 'យល់ព្រម',

# AJAX search
'searchcontaining' => "ស្វែងរកអត្ថបទដែលផ្ទុក ''$1'' ។",
'searchnamed'      => "ស្វែងរកអត្ថបទដែលមានឈ្មោះ ''$1'' ។",
'articletitles'    => "អត្ថបទផ្តើមដោយ ''$1''",
'hideresults'      => 'លាក់លទ្ធផល',
'useajaxsearch'    => 'ប្រើប្រាស់ការស្វែងរករបស់ AJAX',

# Multipage image navigation
'imgmultipageprev' => '← ទំព័រមុន',
'imgmultipagenext' => 'ទំព័របន្ទាប់ →',
'imgmultigo'       => 'ទៅ!',
'imgmultigoto'     => 'ទៅកាន់ទំព័រ$1',

# Table pager
'ascending_abbrev'         => 'លំដាប់ឡើង',
'descending_abbrev'        => 'លំដាប់ចុះ',
'table_pager_next'         => 'ទំព័របន្ទាប់',
'table_pager_prev'         => 'ទំព័រមុន',
'table_pager_first'        => 'ទំព័រដំបូង',
'table_pager_last'         => 'ទំព័រចុងក្រោយ',
'table_pager_limit'        => "បង្ហាញ'''$1'''ចំនុចក្នុងមួយទំព័រ",
'table_pager_limit_submit' => 'ទៅ',
'table_pager_empty'        => 'មិនមានលទ្ធផលទេ',

# Auto-summaries
'autosumm-blank'   => 'ដកចេញខ្លឹមសារទាំងអស់ពីទំព័រ',
'autosumm-replace' => "ជំនួសខ្លឹមសារនៃទំព័រដោយ '$1'",
'autoredircomment' => 'បញ្ជូនបន្តទៅ [[$1]]',
'autosumm-new'     => 'ទំព័រថ្មី៖ $1',

# Size units
'size-bytes'     => '$1បៃ',
'size-kilobytes' => '$1គីឡូបៃ',
'size-megabytes' => '$1មេកាបៃ',
'size-gigabytes' => '$1ជីកាបៃ',

# Live preview
'livepreview-loading' => 'កំពុងផ្ទុក…',
'livepreview-ready'   => 'កំពុងផ្ទុក… ហើយ!',
'livepreview-failed'  => 'ការមើលជាមុនដោយផ្ទាល់មិនទទួលបានជោគជ័យទេ! សូមសាកល្បងជាមួយនឹងការមើលជាមុនតាមធម្មតា។',
'livepreview-error'   => 'មិនអាចទាក់ទងទៅ៖ $1 "$2" បានទេ។

សូមសាកល្បងប្រើការមើលមុនធម្មតា។',

# Friendlier slave lag warnings
'lag-warn-normal' => 'បំលាស់ប្តូរថ្មីជាង$1វិនាទីអាចមិនត្រូវបានបង្ហាញក្នុងបញ្ជីនេះ។',

# Watchlist editor
'watchlistedit-numitems'       => 'បញ្ជីតាមដានរបស់អ្នកមាន{{PLURAL:$1|១ចំណងជើង|$1ចំណងជើង}}ដោយមិនរាប់បញ្ចូលទំព័រពិភាក្សាទេ។',
'watchlistedit-noitems'        => 'បញ្ជីតាមដាន របស់អ្នក គ្មានផ្ទុក ចំណងជើង។',
'watchlistedit-normal-title'   => 'កែប្រែបញ្ជីតាមដាន',
'watchlistedit-normal-legend'  => 'ដកចំណងជើងចេញពីបញ្ជីតាមដាន',
'watchlistedit-normal-explain' => 'ចំណងជើងក្នុងបញ្ជីតាមដានរបស់អ្នកត្រូវបានបង្ហាញខាងក្រោម។

ដើម្បីដកចេញនូវចំណងជើងណាមួយ សូមគូសឆែកប្រអប់ក្បែរវា រួចចុចលើដកចំណងជើងចេញ។

អ្នកអាច[[Special:Watchlist/raw|កែប្រែបញ្ជីឆៅ]]ផងដែរ។',
'watchlistedit-normal-submit'  => 'ដកចំណងជើងចេញ',
'watchlistedit-normal-done'    => '{{PLURAL:$1|១ចំណងជើង|$1ចំណងជើង}}ត្រូវបានដកចេញពីបញ្ជីតាមដានរបស់អ្នក៖',
'watchlistedit-raw-title'      => 'កែប្រែបញ្ជីតាមដានឆៅ',
'watchlistedit-raw-legend'     => 'កែប្រែបញ្ជីតាមដានឆៅ',
'watchlistedit-raw-explain'    => 'ចំណងជើង នានា លើ បញ្ជីតាមដាន របស់អ្នក ត្រូវបាន បង្ហាញខាងក្រោម, និង អាចត្រូវបាន កែប្រែ ដោយបន្ថែម ទៅ ឬ ដកចេញ ពី បញ្ជី, មួយ​ចំណងជើង ក្នុង មួយបន្ទាត់ ។ ពេល បានបញ្ចប់, ចុច បន្ទាន់សម័យ បញ្ជីតាមដាន ។
អ្នក អាចផងដែរ [[Special:Watchlist/edit|ប្រើប្រាស់ ឧបករកែប្រែ គំរូ]] ។',
'watchlistedit-raw-titles'     => 'ចំណងជើង៖',
'watchlistedit-raw-submit'     => 'បន្ទាន់សម័យបញ្ជីតាមដាន',
'watchlistedit-raw-done'       => 'បញ្ជីតាមដានរបស់អ្នកត្រូវបានធ្វើអោយទាន់សម័យហើយ។',
'watchlistedit-raw-added'      => '$1ចំនងជើងបានត្រូវដាក់បន្ថែម៖',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|១ចំណងជើងបានត្រូវ|$1ចំណងជើងបានត្រូវ}}ដកចេញ៖',

# Watchlist editing tools
'watchlisttools-view' => 'មើលបំលាស់ប្តូរពាក់ព័ន្ធ',
'watchlisttools-edit' => 'មើលនិងកែប្រែបញ្ជីតាមដាន',
'watchlisttools-raw'  => 'កែប្រែបញ្ជីតាមដានឆៅ',

# Core parser functions
'unknown_extension_tag' => 'ប្លាកនៃផ្នែកបន្ថែម "$1" មិនស្គាល់',

# Special:Version
'version'                          => 'Version', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'ផ្នែកបន្ថែមដែលបានតំឡើង',
'version-specialpages'             => 'ទំព័រពិសេសៗ',
'version-variables'                => 'អថេរ',
'version-other'                    => 'ផ្សេង',
'version-extension-functions'      => 'មុខងារផ្នែកបន្ថែម',
'version-skin-extension-functions' => 'មុខងារផ្នែកបន្ថែមនៃសំបក',
'version-hook-subscribedby'        => 'បានជាវ ជាប្រចាំ ដោយ',
'version-version'                  => 'កំណែ',
'version-license'                  => 'អាជ្ញាបណ្ណ',
'version-software'                 => 'សូហ្វវែរបានតំឡើង',
'version-software-product'         => 'ផលិតផល',
'version-software-version'         => 'Version',

# Special:FilePath
'filepath'         => 'ផ្លូវនៃឯកសារ',
'filepath-page'    => 'ឯកសារ៖',
'filepath-submit'  => 'ផ្លូវ',
'filepath-summary' => 'ទំព័រពិសេសនេះ បង្ហាញផ្លូវពេញលេញ នៃ មួយឯកសារ។
រូបភាពត្រូវបានបង្ហាញ ជាភាពម៉ត់ខ្ពស់, ប្រភេទឯកសារ ដទៃទៀត ធ្វើការដោយផ្ទាល់ ជាមួយ សហកម្មវិធី ។

បញ្ចូល ឈ្មោះឯកសារ ដោយគ្មានការភ្ជាប់ "{{ns:image}}:" នៅពីមុខវា ។',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'ស្វែងរកឯកសារដូចគ្នាបេះបិត',
'fileduplicatesearch-legend'   => 'ស្វែងរកឯកសារដូចគ្នាបេះបិត',
'fileduplicatesearch-filename' => 'ឈ្មោះឯកសារ៖',
'fileduplicatesearch-submit'   => 'ស្វែងរក',
'fileduplicatesearch-info'     => '$1 × $2 ភីកសែល<br />ទំហំឯកសារ:$3<br />ប្រភេទMIME:$4',
'fileduplicatesearch-result-1' => 'គ្មានឯកសារដែលដូចគ្នាបេះបិតទៅនឹងឯកសារ "$1" ទេ។',
'fileduplicatesearch-result-n' => 'មាន {{PLURAL:$2|1 ឯកសារដូចគ្នាបេះបិត|$2 ឯកសារដូចគ្នាបេះបិត}}ទៅនឹងឯកសារ "$1"។',

# Special:SpecialPages
'specialpages'                   => 'ទំព័រ​ពិសេស​ៗ',
'specialpages-note'              => '----
* ទំព័រពិសេសៗធម្មតាដែលអ្នកប្រើប្រាស់គ្រប់រូបអាចប្រើប្រាស់បាន។
* <span class="mw-specialpagerestricted">ទំព័រពិសេសៗដែលត្រូវបានដាក់កំហិត អ្នកប្រើប្រាស់ធម្មតាមិនអាចប្រើប្រាស់បាន។</span>',
'specialpages-group-maintenance' => 'របាយការណ៍នានាអំពីតំហែទាំ',
'specialpages-group-other'       => 'ទំព័រពិសេសៗផ្សេងៗទៀត',
'specialpages-group-login'       => 'ឡុកអ៊ីន / ចុះឈ្មោះ',
'specialpages-group-changes'     => 'បំលាស់ប្តូរថ្មីៗនិងកំនត់ហេតុ',
'specialpages-group-media'       => 'របាយការណ៍មេឌានិងការផ្ទុកឯកសារ',
'specialpages-group-users'       => 'អ្នកប្រើប្រាស់និងសិទ្ធិ',
'specialpages-group-highuse'     => 'ទំព័រដែលត្រូវបានប្រើច្រើន',
'specialpages-group-pages'       => 'បញ្ជីទំព័រនានា',
'specialpages-group-pagetools'   => 'ឧបករណ៍ទំព័រ',
'specialpages-group-wiki'        => 'ទិន្នន័យនិងឧបករណ៍វិគី',
'specialpages-group-redirects'   => 'ទំព័របញ្ជូនបន្តពិសេសៗ',
'specialpages-group-spam'        => 'ឧបករណ៍ស្ព៊ែម',

# Special:BlankPage
'blankpage'              => 'ទំព័រទទេ',
'intentionallyblankpage' => 'ទំព័រនេះត្រូវបានទុកចោលអោយនៅទំនេរដោយចេតនា',

);
