<?php
/** Amharic (አማርኛ)
 *
 * @ingroup Language
 * @file
 *
 * @author Codex Sinaiticus
 * @author Teferra
 */

$namespaceNames = array(
	'NS_MEDIA'          => 'ፋይል',
	'NS_SPECIAL'        => 'ልዩ',
	'NS_TALK'           => 'ውይይት',
	'NS_USER'           => 'አባል',
	'NS_USER_TALK'      => 'አባል_ውይይት',
	'NS_PROJECT_TALK'   => '$1_ውይይት',
	'NS_IMAGE'          => 'ስዕል',
	'NS_IMAGE_TALK'     => 'ስዕል_ውይይት',
	'NS_MEDIAWIKI'      => 'መልዕክት',
	'NS_MEDIAWIKI_TALK' => 'መልዕክት_ውይይት',
	'NS_TEMPLATE'       => 'መልጠፊያ',
	'NS_TEMPLATE_TALK'  => 'መልጠፊያ_ውይይት',
	'NS_HELP'           => 'እርዳታ',
	'NS_HELP_TALK'      => 'እርዳታ_ውይይት',
	'NS_CATEGORY'       => 'መደብ',
	'NS_CATEGORY_TALK'  => 'መደብ_ውይይት',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'በመያያዣ ስር አስምር',
'tog-highlightbroken'         => 'የተሰበረ (ቀይ) መያያዣን <a href="" class="new">እንዲህ</a>? አለዚያ: እንዲህ<a href="" class="internal">?</a>',
'tog-justify'                 => 'አንቀጾችን አስተካክል',
'tog-hideminor'               => 'በቅርብ ጊዜ የተደረጉ አነስተኛ እርማቶችን ደብቅ',
'tog-extendwatchlist'         => 'የሚደረጉ ለውጦችን ለማሳየት መቆጣጠሪያ-ዝርዝርን ዘርጋ',
'tog-usenewrc'                => 'የተሻሻሉ የቅርብ ለውጦች (JavaScript)',
'tog-numberheadings'          => 'አርዕስቶችን በራስገዝ ቁጥር ስጥ',
'tog-showtoolbar'             => 'አርም ትዕዛዝ-ማስጫ ይታይ (JavaScript)',
'tog-editondblclick'          => 'ሁለቴ መጫን ገጹን ማረም ያስችል (JavaScript)',
'tog-editsection'             => 'በ[አርም] መያያዣ ክፍል ማረምን አስችል',
'tog-editsectiononrightclick' => 'የክፍል አርዕስት ላይ በቀኝ በመጫን ክፍል ማረምን አስችል (JavaScript)',
'tog-showtoc'                 => 'ከ3 አርዕስቶች በላይ ሲሆን የማውጫ ሰንጠረዥ ይታይ',
'tog-rememberpassword'        => 'መግባቴን እዚህ አስሊ ላይ አስታውስ',
'tog-editwidth'               => 'የማረሚያ ሳጥን ሙሉ ስፋት አለው',
'tog-watchcreations'          => 'እኔ የፈጠርኳቸውን ገጾች ወደምከታተላቸው ገጾች ዝርዝር ውስጥ ጨምር',
'tog-watchdefault'            => 'ያረምኳቸውን ገጾች ወደምከታተላቸው ገጾች ዝርዝር ውስጥ ጨምር',
'tog-watchmoves'              => 'ያዛወርኳቸውን ገጾች ወደምከታተላቸው ገጾች ዝርዝር ውስጥ ጨምር',
'tog-watchdeletion'           => 'የሰረዝኳቸውን ገጾች ወደምከታተላቸው ገጾች ዝርዝር ውስጥ ጨምር',
'tog-minordefault'            => 'ሁሉም እርማቶች በቀዳሚነት አነስተኛ ይባሉ',
'tog-previewontop'            => 'ከማረሚያው ሳጥን በፊት ቅድመ-ዕይታ አሳይ',
'tog-previewonfirst'          => 'በመጀመሪያ እርማት ቅድመ-ዕይታ ይታይ',
'tog-nocache'                 => 'ገጽ መቆጠብን አታስችል',
'tog-enotifwatchlistpages'    => 'የምከታተለው ገጽ ሲቀየር ኤመልዕክት ይላክልኝ',
'tog-enotifusertalkpages'     => 'የተጠቃሚ መወያያ ገጼ ሲቀየር ኤመልዕክት ይላክልኝ',
'tog-enotifminoredits'        => 'ለአነስተኛ የገጽ እርማቶችም ኤመልዕክት ይላክልኝ',
'tog-enotifrevealaddr'        => 'ኤመልዕክት አድራሻዬን በማሳወቂያ መልዕክቶች ውስጥ አሳይ',
'tog-shownumberswatching'     => 'የሚከታተሉ ተጠቃሚዎችን ቁጥር አሳይ',
'tog-fancysig'                => 'ጥሬ ፊርማ (ያለራስገዝ ማያያዣ)',
'tog-externaleditor'          => 'በቀዳሚነት ውጪያዊ አራሚን ተጠቀም',
'tog-externaldiff'            => 'በቀዳሚነት የውጭ ልዩነት-ማሳያን ተጠቀም',
'tog-showjumplinks'           => 'የ"ዝለል" አቅላይ መያያዣዎችን አስችል',
'tog-uselivepreview'          => 'ቀጥታ ቅድመ-ዕይታን ይጠቀሙ (JavaScript) (የሙከራ)',
'tog-forceeditsummary'        => 'ማጠቃለያው ባዶ ከሆነ ማስታወሻ ይስጠኝ',
'tog-watchlisthideown'        => 'የራስዎ ለውጦች ከሚከታተሉት ገጾች ይደበቁ',
'tog-watchlisthidebots'       => 'የቦት (መሣርያ) ለውጦች ከሚከታተሉት ገጾች ይደበቁ',
'tog-watchlisthideminor'      => 'ጥቃቅን ለውጦች ከሚከታተሉት ገጾች ይደበቁ',
'tog-ccmeonemails'            => 'ወደ ሌላ ተጠቃሚ የምልከው ኢሜል ቅጂ ለኔም ይላክ',
'tog-diffonly'                => 'ከለውጦቹ ስር የገጽ ይዞታ አታሳይ',
'tog-showhiddencats'          => 'የተደበቁ መደቦች ይታዩ',

'underline-always'  => 'ሁሌም ይህን',
'underline-never'   => 'ሁሌም አይሁን',
'underline-default' => 'የቃኝ ቀዳሚ ባህሪዎች',

'skinpreview' => '(ቅድመ-ዕይታ)',

# Dates
'sunday'        => 'እሑድ',
'monday'        => 'ሰኞ',
'tuesday'       => 'ማክሰኞ',
'wednesday'     => 'ረቡዕ',
'thursday'      => 'ሐሙስ',
'friday'        => 'ዓርብ',
'saturday'      => 'ቅዳሜ',
'sun'           => 'እሑድ',
'mon'           => 'ሰኞ',
'tue'           => 'ማክሰኞ',
'wed'           => 'ረቡዕ',
'thu'           => 'ሐሙስ',
'fri'           => 'ዓርብ',
'sat'           => 'ቅዳሜ',
'january'       => 'ጃንዩዌሪ',
'february'      => 'ፌብሩዌሪ',
'march'         => 'ማርች',
'april'         => 'ኤይፕርል',
'may_long'      => 'ሜይ',
'june'          => 'ጁን',
'july'          => 'ጁላይ',
'august'        => 'ኦገስት',
'september'     => 'ሰፕቴምበር',
'october'       => 'ኦክቶበር',
'november'      => 'ኖቬምበር',
'december'      => 'ዲሴምበር',
'january-gen'   => 'ጃንዩዌሪ',
'february-gen'  => 'ፌብሩዌሪ',
'march-gen'     => 'ማርች',
'april-gen'     => 'ኤይፕርል',
'may-gen'       => 'ሜይ',
'june-gen'      => 'ጁን',
'july-gen'      => 'ጁላይ',
'august-gen'    => 'ኦገስት',
'september-gen' => 'ሰፕቴምበር',
'october-gen'   => 'ኦክቶበር',
'november-gen'  => 'ኖቬምበር',
'december-gen'  => 'ዲሴምበር',
'jan'           => 'ጃንዩ.',
'feb'           => 'ፌብሩ.',
'mar'           => 'ማርች',
'apr'           => 'ኤፕሪ.',
'may'           => 'ሜይ',
'jun'           => 'ጁን',
'jul'           => 'ጁላይ',
'aug'           => 'ኦገስት',
'sep'           => 'ሴፕቴ.',
'oct'           => 'ኦክቶ.',
'nov'           => 'ኖቬም.',
'dec'           => 'ዲሴም.',

# Categories related messages
'pagecategories'           => '{{PLURAL:$1|ምድብ|ምድቦች}}',
'category_header'          => 'በምድብ «$1» ውስጥ የሚገኙ ገጾች',
'subcategories'            => 'ንዑስ-ምድቦች',
'category-media-header'    => 'በመደቡ «$1» የተገኙ ፋይሎች፦',
'category-empty'           => 'ይህ መደብ አሁን ባዶ ነው።',
'hidden-categories'        => '{{PLURAL:$1|የተደበቀ መደብ|የተደበቁ መደቦች}}',
'hidden-category-category' => 'የተደበቁ መደቦች', # Name of the category where hidden categories will be listed
'category-subcat-count'    => '{{PLURAL:$2|በዚሁ መደብ ውስጥ አንድ ንዑስ-መደብ አለ|በዚሁ መደብ ውስጥ {{PLURAL:$1|የሚከተለው ንዕስ-መደብ አለ|የሚከተሉት $1 ንዑስ-መደቦች አሉ}} (በጠቅላላም ከነስውር መደቦች $2 አሉ)}}፦',
'listingcontinuesabbrev'   => '(ተቀጥሏል)',

'mainpagetext'      => "<big>'''MediaWiki በትክክል ማስገባቱ ተከናወነ።'''</big>",
'mainpagedocfooter' => "ስለ ዊኪ ሶፍትዌር ጥቅም ለመረዳት፣ [http://meta.wikimedia.org/wiki/Help:Contents User's Guide] ያንብቡ።

== ለመጀመር ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Configuration settings list]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mailing list]",

'about'          => 'ስለ',
'article'        => 'መጣጥፍ',
'newwindow'      => '(ባዲስ መስኮት ውስጥ ይከፈታል።)',
'cancel'         => 'ሰርዝ',
'qbfind'         => 'አግኝ',
'qbbrowse'       => 'ቃኝ',
'qbedit'         => 'አርም',
'qbpageoptions'  => 'ይህ ገጽ',
'qbpageinfo'     => 'አግባብ',
'qbmyoptions'    => 'የኔ ገጾች',
'qbspecialpages' => 'ልዩ ገጾች',
'moredotdotdot'  => 'ተጨማሪ...',
'mypage'         => 'የኔ ገጽ',
'mytalk'         => 'የኔ ውይይት',
'anontalk'       => 'ውይይት ለዚሁ ቁ. አድራሻ',
'navigation'     => 'መቃኘት',
'and'            => 'እና',

# Metadata in edit box
'metadata_help' => 'ተጨማሪ መረጃ:',

'errorpagetitle'    => 'ስህተት',
'returnto'          => '(ወደ $1 ለመመለስ)',
'tagline'           => 'ከ{{SITENAME}}',
'help'              => 'እርዳታ ገጽ',
'search'            => 'ፈልግ',
'searchbutton'      => 'ፈልግ',
'go'                => 'ሂድ',
'searcharticle'     => 'ሂድ',
'history'           => 'የገጽ ታሪክ',
'history_short'     => 'ታሪክ',
'updatedmarker'     => 'ከመጨረሻው ጉብኝቴ በኋላ የተሻሻለ',
'info_short'        => 'መረጃ',
'printableversion'  => 'የህትመት ዝርያ',
'permalink'         => 'ቋሚ መያያዣ',
'print'             => 'ይታተም',
'edit'              => 'አርም',
'create'            => 'ለመፍጠር',
'editthispage'      => 'ይህን ገጽ አርም',
'delete'            => 'ይጥፋ',
'deletethispage'    => 'ይህን ገጽ ሰርዝ',
'undelete_short'    => '{{PLURAL:$1|አንድ ዕትም|$1 ዕትሞች}} ለመመልስ',
'protect'           => 'ጠብቅ',
'protect_change'    => 'የመቆለፍ ደረጃ ለመቀይር',
'protectthispage'   => 'ይህን ገጽ ለመቆለፍ',
'unprotect'         => 'አለመቆለፍ',
'unprotectthispage' => 'ይህን ገጽ ለመፍታት',
'newpage'           => 'አዲስ ገጽ',
'talkpage'          => 'ስለዚሁ ገጽ ለመወያየት',
'talkpagelinktext'  => 'ውይይት',
'specialpage'       => 'ልዩ ገጽ',
'personaltools'     => 'የኔ መሣርያዎች',
'postcomment'       => 'አስተያየት ለማቅረብ',
'articlepage'       => 'መጣጥፉን ለማየት',
'talk'              => 'ውይይት',
'views'             => 'ዕይታዎች',
'toolbox'           => 'ትዕዛዝ ማስጫ',
'userpage'          => 'የአባል መኖሪያ ገጽ ለማየት',
'projectpage'       => 'ግብራዊ ገጹን ለማየት',
'imagepage'         => 'የፋይሉን ገጽ ለማየት',
'mediawikipage'     => 'የመልእክቱን ገጽ ለማየት',
'templatepage'      => 'የመልጠፊያውን ገጽ ለማየት',
'viewhelppage'      => 'የእርዳታ ገጽ ለማየት',
'categorypage'      => 'የመደቡን ገጽ ለማየት',
'viewtalkpage'      => 'ውይይቱን ለማየት',
'otherlanguages'    => 'በሌሎች ቋንቋዎች',
'redirectedfrom'    => '(ከ$1 የተዛወረ)',
'redirectpagesub'   => 'መምሪያ መንገድ',
'lastmodifiedat'    => 'ይህ ገጽ መጨረሻ የተቀየረው እ.ኣ.አ በ$2፣ $1 ዓ.ም. ነበር።', # $1 date, $2 time
'viewcount'         => 'ይህ ገጽ {{PLURAL:$1|አንዴ|$1 ጊዜ}} ታይቷል።',
'protectedpage'     => 'የተቆለፈ ገጽ',
'jumpto'            => 'ዘልለው ለመሐድ፦',
'jumptonavigation'  => 'የማውጫ ቁልፎች',
'jumptosearch'      => 'ፍለጋ',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'ስለ {{SITENAME}} መርሃግብር',
'aboutpage'            => 'Project:ስለ',
'bugreports'           => 'ተውሳክ ማመልከቻ',
'bugreportspage'       => 'Project:ተውሳክ ማመልከቻ',
'copyright'            => 'ይዘቱ በ$1 ሥር ይገኛል።',
'copyrightpagename'    => '{{SITENAME}} የቅጂ መብት',
'copyrightpage'        => '{{ns:project}}:የማብዛት መብት ደንብ',
'currentevents'        => 'ወቅታዊ ጉዳዮች',
'currentevents-url'    => 'Project:ወቅታዊ ጉዳዮች',
'disclaimers'          => 'የኃላፊነት ማስታወቂያ',
'disclaimerpage'       => 'Project:አጠቃላይ የሕግ ነጥቦች',
'edithelp'             => 'የማረም መመሪያ',
'edithelppage'         => 'Help:የማዘጋጀት እርዳታ',
'faq'                  => 'ብጊየጥ (ብዙ ጊዜ የሚጠየቁ ጥያቀዎች)',
'faqpage'              => 'Project:ብጊየጥ',
'helppage'             => 'Help:ይዞታ',
'mainpage'             => 'ዋና ገጽ',
'mainpage-description' => 'ዋና ገጽ',
'policy-url'           => 'Project:መርመርያዎች',
'portal'               => 'የኅብረተሠቡ መረዳጃ',
'portal-url'           => 'Project:የኅብረተሠብ መረዳጃ',
'privacy'              => 'የሚስጥር ፖሊሲ',
'privacypage'          => 'Project:የግልነት ድንጋጌ',

'badaccess'        => 'ያልተፈቀደ - አይቻልም',
'badaccess-group0' => 'የጠየቁት አድራጎት እንዲፈጸም ፈቃድ የለዎም።',
'badaccess-group1' => 'የጠየቁት አድራጎት ለ$1 ማዕረግ ላላቸው አባላት ብቻ ይፈቀዳል።',
'badaccess-group2' => 'የጠየቁት አድራጎት ለ$1 ማዕረጎች ላሏቸው አባላት ብቻ ይፈቀዳል።',
'badaccess-groups' => 'የጠየቁት አድራጎት ለ$1 ማዕረጎች ላሏቸው አባላት ብቻ ይፈቀዳል።',

'versionrequired'     => 'የMediaWiki ዝርያ $1 ያስፈልጋል።',
'versionrequiredtext' => 'ይህንን ገጽ ለመጠቀም የMediaWiki ዝርያ $1 ያስፈልጋል። [[Special:Version|የዝርያውን ገጽ]] ይዩ።',

'ok'                      => 'እሺ',
'retrievedfrom'           => 'ከ «$1» ተወሰደ',
'youhavenewmessages'      => '$1 አሉዎት ($2)።',
'newmessageslink'         => 'አዲስ መልእክቶች',
'newmessagesdifflink'     => 'የመጨረሻ ለውጥ',
'youhavenewmessagesmulti' => 'በ$1 አዲስ መልእክቶች አሉዎት',
'editsection'             => 'አርም',
'editold'                 => 'አርም',
'editsectionhint'         => 'ክፍሉን «$1» ለማስተካከል',
'toc'                     => 'ማውጫ',
'showtoc'                 => 'አሳይ',
'hidetoc'                 => 'ደብቅ',
'thisisdeleted'           => '($1ን ለመመልከት ወይም ለመመለስ)',
'viewdeleted'             => '$1 ይታይ?',
'restorelink'             => '{{PLURAL:$1|የጠፋ ዕትም|$1 የጠፉት ዕትሞች}}',
'feedlinks'               => 'ማጉረስ (feed)፦',
'feed-invalid'            => 'የማይገባ የማጉረስ አይነት።',
'feed-unavailable'        => 'ማጉረስ በ{{SITENAME}} የለም።',
'site-rss-feed'           => '$1 R.S.S. Feed',
'site-atom-feed'          => '$1 አቶም Feed',
'page-rss-feed'           => '"$1" R.S.S. Feed',
'page-atom-feed'          => '"$1" አቶም Feed',
'red-link-title'          => '$1 (ገና አልተጻፈም)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'ገጽ',
'nstab-user'      => 'የአባል ገጽ',
'nstab-media'     => 'ፋይል',
'nstab-special'   => 'ልዩ',
'nstab-project'   => 'የፕሮጀክት ገጽ',
'nstab-image'     => 'ፋይል',
'nstab-mediawiki' => 'መልዕክት',
'nstab-template'  => 'መልጠፊያ',
'nstab-help'      => 'የመመሪያ ገጽ',
'nstab-category'  => 'ምድብ',

# Main script and global functions
'nosuchaction'      => 'የማይሆን ተግባር',
'nosuchactiontext'  => 'በURL የተወሰነው ተግባር በዚህ ዊኪ አይታወቀም።',
'nosuchspecialpage' => 'እንዲህ የተባለ ልዩ ገጽ የለም',
'nospecialpagetext' => "<big>'''ለማይኖር ልዩ ገጽ ጠይቀዋል።'''</big>

የሚኖሩ ልዩ ገጾች ዝርዝር በ[[Special:SpecialPages|{{int:specialpages}}]] ሊገኝ ይችላል።",

# General errors
'error'                => 'ስኅተት',
'databaseerror'        => 'የመረጃ-ቤት ስህተት',
'dberrortext'          => 'የመረጃ-ቤት ጥያቄ ስዋሰው ስህተት ሆኗል። ይህ ምናልባት በሶፍትዌሩ ወስጥ ያለ ተውሳክ ሊጠቆም ይችላል። መጨረሻ የተሞከረው መረጃ-ቤት ጥያቄ <blockquote><tt>$1</tt></blockquote> ከተግባሩ «<tt>$2</tt>» ውስጥ ነበረ። MySQL ስህተት «<tt>$3: $4</tt>» መለሰ።',
'dberrortextcl'        => 'የመረጃ-ቤት ጥያቄ ስዋሰው ስህተት ሆኗል። መጨረሻ የተሞከረው መረጃ-ቤት ጥያቄ <blockquote><tt>$1</tt></blockquote> ከተግባሩ «<tt>$2</tt>» ውስጥ ነበረ። MySQL ስህተት «<tt>$3: $4</tt>» መለሰ።',
'noconnect'            => 'ይቅርታ! ይህ ዊኪ አሁን የተግባር ችግር ስላጋጠመው የመረጃ-ቤቱን ሰርቨር ሊያገናኝ አይችልም። <br />
$1',
'nodb'                 => 'መረጃ-ቤት $1 ለመምረጥ አልተቻለም',
'cachederror'          => 'የሚከተለው ለተጠየቀው ገጽ የቆጠበው ቅጂ ሆኖ ምናልባት የታደሠ አይሆንም።',
'laggedslavemode'      => 'ማስጠንቀቂያ፦ ምናልባት የቅርብ ለውጦች በገጹ ላይ አይታዩም።',
'readonly'             => 'መረጃ-ቤት ተቆልፏል',
'enterlockreason'      => 'የመቆለፉን ምክንያትና የሚያልቅበትን ሰዓት (በግምት) ይጻፉ።',
'readonlytext'         => 'መረጃ-ቤቱ አሁን ከመቀየር ተቆልፏል። ይህ ለተራ አጠባበቅ ብቻ መሆኑ አይቀርም። ከዚያ በኋላ እንደ ወትሮ ሁኔታ ይኖራል።

የቆለፉት መጋቢ ይህንን መግለጫ አቀረቡ፦ $1',
'readonly_lag'         => 'ተከታይ ሰርቨሮች ለቀዳሚው እስከሚደርሱ ድረስ መረጃ-ቤቱ በቀጥታ ተቆልፏል።',
'internalerror'        => 'የውስጥ ስህተት',
'internalerror_info'   => 'የውስጥ ስህተት፦ $1',
'filecopyerror'        => 'ፋይሉን «$1» ወደ «$2» መቅዳት አልተቻለም።',
'filerenameerror'      => 'የፋይሉን ስም ከ«$1» ወደ «$2» መቀየር አተቻለም።',
'filedeleteerror'      => 'ፋይሉን «$1» ለማጥፋት አልተቻለም።',
'directorycreateerror' => 'ዶሴ «$1» መፍጠር አልተቻለም።',
'filenotfound'         => '«$1» የሚባል ፋይል አልተገኘም።',
'fileexistserror'      => 'ወደ ፋይሉ «$1» መጻፍ አይቻልም፦ ፋይሉ ይኖራል',
'unexpected'           => 'ያልተጠበቀ ዕሴት፦ «$1»=«$2»።',
'formerror'            => 'ስኅተት፦ ማመልከቻ ለማቅረብ አልተቻለም',
'badarticleerror'      => 'ይህ ተግባር በዚሁ ገጽ ላይ ሊደረግ አይቻልም።',
'cannotdelete'         => 'የተወሰነው ገጽ ወይም ፋይል ለማጥፋት አልተቻለም። (ምናልባት በሌላ ሰው እጅ ገና ጠፍቷል።)',
'badtitle'             => 'መጥፎ አርዕስት',
'badtitletext'         => 'የፈለጉት አርዕስት ልክ አልነበረም። ምናልባት ለአርዕስት የማይሆን የፊደል ምልክት አለበት።',
'perfdisabled'         => 'ይቅርታ! ማንም ዊኪውን ለመጠቀም እስከማይችል ድረስ መረጃ-ቤቱን ስለሚያዘገይ ይህ ተግባር ለግዜው እንደማይሰራ ተደርጓል።',
'perfcached'           => 'ማስታወቂያ፡ ይህ መረጃ በየጊዜ የሚታደስ ስለሆነ ዘመናዊ ሳይሆን የቆየ ሊሆን ይችላል።',
'perfcachedts'         => 'የሚቀጥለው መረጃ ተቆጥቧል፣ መጨረሻ የታደሠው $1 እ.ኤ.አ. ነው።',
'querypage-no-updates' => 'ይህ ገጽ አሁን የታደሠ አይደለም። ወደፊትም መታደሱ ቀርቷል። በቅርብ ግዜ አይታደስም።',
'wrong_wfQuery_params' => 'ለwfQuery() ትክክለኛ ያልሆነ ግቤት<br />
ተግባር፦ $1<br />
ጥያቄ፦ $2',
'viewsource'           => 'ምንጩን ተመልከት',
'viewsourcefor'        => 'ለ«$1»',
'actionthrottled'      => 'ተግባሩ ተቋረጠ',
'actionthrottledtext'  => 'የስፓም መብዛት ለመቃወም፣ በአጭር ጊዜ ውስጥ ይህን ተግባር ብዙ ጊዜ ከመፈጽም ተክለክለዋል። አሁንም ከመጠኑ በላይ በልጠዋል። እባክዎ ከጥቂት ደቂቃ በኋላ እንደገና ይሞክሩ።',
'protectedpagetext'    => 'ይኸው ገጽ እንዳይታረም ተጠብቋል።',
'viewsourcetext'       => 'የዚህን ገጽ ምንጭ ማየትና መቅዳት ይችላሉ።',
'protectedinterface'   => 'ይህ ገጽ ለስልቱ ገጽታ ጽሑፍን ያቀርባል፣፡ ስለዚህ እንዳይበላሽ ተጠብቋል።',
'editinginterface'     => "'''ማስጠንቀቂያ፦''' ይህ ገጽ ለድረገጹ መልክ ጽሕፈት ይሰጣል። በዊኪ ሁሉ ላይ መላውን የድረገጽ መልክ በቀላል ለማስተርጎም [http://translatewiki.net/wiki/Main_Page?setlang=am Betawiki] ይጎብኙ።",
'sqlhidden'            => '(የመደበኛ-የመጠይቅ-ቋንቋ (SQL) ጥያቄ ተደበቀ)',
'cascadeprotected'     => "'''ማስጠንቀቂያ፦''' ይህ አርእስት ሊፈጠር ወይም ሊቀየር አይቻልም። ምክንያቱም ወደ {{PLURAL:$1|ተከታተለው አርዕስት|ተከታተሉት አርእስቶች}} ተጨምሯል።
$2",
'namespaceprotected'   => "በ'''$1''' ክፍለ-ዊኪ ያሉትን ገጾች ለማዘጋጀት ፈቃድ የለዎም።",
'customcssjsprotected' => 'ይህ ገጽ የሌላ ተጠቃሚ ምርጫዎች ስላሉበት እሱን ለማዘጋጀት ፈቃድ የለዎም።',
'ns-specialprotected'  => 'ልዩ ገጾችን ማረም አይፈቀድም።',
'titleprotected'       => "ይህ አርዕስት እንዳይፈጠር በ[[User:$1|$1]] ተጠብቋል። የተሰጠው ምክንያት ''$2'' ነው።",

# Login and logout pages
'logouttitle'                => 'የአባል መውጫ',
'logouttext'                 => '<strong>አሁን ወጥተዋል።</strong><br /> አሁንም በቁጥር መታወቂያዎ ማዘጋጀት ይቻላል። ወይም ደግሞ እንደገና በብዕር ስምዎ መግባት ይችላሉ።
----
በጥቂት ሴኮንድ ውስጥ ወደሚከተለው ገጽ በቀጥታ ይመለሳል፦',
'welcomecreation'            => '== ሰላምታ፣ $1! ==

የብዕር ስምዎ ተፈጥሯል። ምርጫዎችዎን ለማስተካከል ይችላሉ።',
'loginpagetitle'             => 'የአባል መግቢያ',
'yourname'                   => 'Username / የብዕር ስም:',
'yourpassword'               => 'Password / መግቢያ ቃል',
'yourpasswordagain'          => 'መግቢያ ቃልዎን ዳግመኛ ይስጡ',
'remembermypassword'         => '(መግቢያዎ እንዲታወስ ምልክት እዚህ ያድርጉ)',
'yourdomainname'             => 'የእርስዎ ከባቢ (domain)፦',
'externaldberror'            => 'ወይም አፍአዊ የማረጋገጫ መረጃ-ቤት ስኅተት ነበረ፣ ወይም አፍአዊ አባልነትዎን ማሳደስ አልተፈቀዱም።',
'loginproblem'               => '<b>በመግባትዎ አንድ ችግር ኖሯል። </b><br />እንደገና ይሞክሩ!',
'login'                      => 'ለመግባት',
'nav-login-createaccount'    => 'መግቢያ',
'loginprompt'                => '(You must have cookies enabled to log in to {{SITENAME}}.)',
'userlogin'                  => 'መግቢያ',
'logout'                     => 'ከብዕር ስምዎ ለመውጣት',
'userlogout'                 => 'መውጫ',
'notloggedin'                => 'አልገቡም',
'nologin'                    => 'የብዕር ስም ገና የለዎም? $1!',
'nologinlink'                => 'አዲስ የብዕር ስም ያውጡ',
'createaccount'              => 'አዲስ አባል ለመሆን',
'gotaccount'                 => '(አባልነት አሁን ካለዎ፥ $1 ይግቡ)',
'gotaccountlink'             => 'በዚህ',
'createaccountmail'          => 'በኢ-ሜል',
'badretype'                  => 'የጻፉት መግቢያ ቃሎች አይስማሙም።',
'userexists'                 => 'ይህ ብዕር ስም አሁን ይኖራል። እባክዎ፣ ሌላ ብዕር ስም ይምረጡ።',
'youremail'                  => 'ኢ-ሜል *',
'username'                   => 'የብዕር ስም:',
'uid'                        => 'የገባበት ቁ.: #',
'yourrealname'               => 'ዕውነተኛ ስም፦',
'yourlanguage'               => 'የመልኩ ቋንቋ',
'yournick'                   => 'ቁልምጫ ስም (ለፊርማ)',
'badsig'                     => 'ትክክለኛ ያልሆነ ጥሬ ፊርማ፤ HTML ተመልከት።',
'badsiglength'               => 'ያ ቁልምጫ ስም ከመጠን በላይ ይረዝማል፤ ከ$1 ፊደል በታች መሆን አለበት።',
'email'                      => 'ኢ-ሜል',
'prefs-help-realname'        => 'ዕውነተኛ ስምዎን መግለጽ አስፈላጊነት አይደለም። ለመግለጽ ከመረጡ ለሥራዎ ደራሲነቱን ለማስታወቅ ይጠቅማል።',
'loginerror'                 => 'የመግባት ስኅተት',
'prefs-help-email'           => 'ኢሜል አድራሻን ማቅረብዎ አስፈላጊ አይደለም። ቢያቅርቡት ሌሎች አባላት አድራሻውን ሳያውቁ በፕሮግራሙ አማካኝነት ሊገናኙዎት ተቻለ።',
'prefs-help-email-required'  => 'የኢ-ሜል አድራሻ ያስፈልጋል።',
'nocookiesnew'               => 'ብዕር ስም ተፈጠረ፣ እርስዎ ግን ገና አልገቡም። በ{{SITENAME}} ተጠቃሚዎች ለመግባት የቃኚ-ማስታወሻ (cookie) ይጠቀማል። በርስዎ ኮምፒውተር ግን የቃኚ-ማስታወሻ እንዳይሠራ ተደርጓል። እባክዎ እንዲሠራ ያድርጉና በአዲስ ብዕር ስምና መግቢያ ቃልዎ ይግቡ።።',
'nocookieslogin'             => 'በ{{SITENAME}} ተጠቃሚዎች ለመግባት የቃኚ-ማስታወሻ (cookie) ይጠቀማል። በርስዎ ኮምፒውተር ግን የቃኚ-ማስታወሻ እንዳይሠራ ተደርጓል። እባክዎ እንዲሠራ ያድርጉና እንደገና ይሞክሩ።',
'noname'                     => 'የተወሰነው ብዕር ስም ትክክለኛ አይደለም።',
'loginsuccesstitle'          => 'መግባትዎ ተከናወነ!',
'loginsuccess'               => 'እንደ «$1» ሆነው አሁን {{SITENAME}}ን ገብተዋል።',
'nosuchuser'                 => '«$1» የሚል ብዕር ስም አልተገኘም። አጻጻፉን ይመልከቱ ወይም አዲስ ብዕር ስም ያውጡ።',
'nosuchusershort'            => '«<nowiki>$1</nowiki>» የሚል ብዕር ስም አልተገኘም። አጻጻፉን ይመልከቱ።',
'nouserspecified'            => 'አንድ ብዕር ስም መጠቆም ያስፈልጋል።',
'wrongpassword'              => 'የተሰጠው መግቢያ ቃል ልክ አልነበረም። ዳግመኛ ይሞክሩ።',
'wrongpasswordempty'         => 'ምንም መግቢያ ቃል አልተሰጠም። ዳግመኛ ይሞክሩ።',
'passwordtooshort'           => 'የመረጡት መግቢያ ቃል ልክ አይሆንም። ቢያንስ $1 ፊደላትና ከብዕር ስምዎ የተለየ መሆን አለበት።',
'mailmypassword'             => 'Mail me a new password / መግቢያ ቃሌን ረስቼ አዲስ በኔ email ይላክልኝ።',
'passwordremindertitle'      => 'አዲስ ግዜያዊ መግቢያ ቃል (PASSWORD) ለ{{SITENAME}}',
'passwordremindertext'       => 'አንድ ሰው (ከቁጥር አድራሻ #$1 ሆኖ እርስዎ ይሆናሉ) አዲስ መግቢያ ቃል ለ{{SITENAME}} ጠይቋል ($4).
ለ«$2» ይሆነው መግቢያ ቃል አሁን «$3» ነው። አሁን በዚህ መግቢያ ቃል ገብተው ወደ አዲስ መግቢያ ቃል መቀየር ይሻሎታል።

ይህ ጥያቄ የእርስዎ ካልሆነ፣ ወይም መግቢያ ቃልዎን ያስታወሱ እንደ ሆነ፣ ይህንን መልእክት ቸል ማለት ይችላሉ። የቆየው መግቢያ ቃል ከዚህ በኋላ ተግባራዊ ሆኖ ይቀጥላል።',
'noemail'                    => 'ለብዕር ስም «$1» የተመዘገበ ኢ-ሜል የለም።',
'passwordsent'               => 'አዲስ መግቢያ ቃል ለ«$1» ወደ ተመዘገበው ኢ-ሜል ተልኳል። እባክዎ ከተቀበሉት በኋላ ዳግመኛ ይግቡ።',
'blocked-mailpassword'       => 'የርስዎ ቁጥር አድራሻ ከማዘጋጀት ታግዷልና፣ እንግዲህ ተንኮል ለመከልከል የመግቢያ ቃል ማግኘት ዘዴ ለመጠቀም አይፈቀደም።',
'eauthentsent'               => 'የማረጋገጫ ኢ-ሜል ወዳቀረቡት አድራሻ ተልኳል። ያው አድራሻ በውነት የርስዎ እንደሆነ ለማረጋገጥ፣ እባክዎ በዚያ ደብዳቤ ውስጥ የተጻፈውን መያያዣ ይጫኑ። ከዚያ ቀጥሎ ኢ-ሜል ከሌሎች ተጠቃሚዎች መቀበል ይችላሉ።',
'throttled-mailpassword'     => 'የመግቢያ ቃል ማስታወሻ ገና አሁን ባለፉት $1 ሰዓቶች ተልኳል። ተንኮልን ለመከልከል፣ በየ$1 ሰዓቶቹ አንድ የመግቢያ ቃል ማስታወሻ ብቻ ይላካል።',
'mailerror'                  => 'ኢ-ሜልን የመላክ ስኅተት፦ $1',
'acct_creation_throttle_hit' => 'ይቅርታ! $1 ብዕር ስሞች ከዚህ በፊት ፈጥረዋል። ከዚያ በላይ ለመፍጠር አይችሉም።',
'emailauthenticated'         => 'የርስዎ ኢ-ሜል አድራሻ በ$1 ተረጋገጠ።',
'emailnotauthenticated'      => 'ያቀረቡት አድራሻ ገና አልተረጋገጠምና ከሌሎች ተጠቃሚዎች ኢሜል መቀበል አይችሉም።',
'noemailprefs'               => '(በ{{SITENAME}} በኩል ኢሜል ለመቀበል፣ የራስዎን አድራሻ አስቀድመው ማቅረብ ያስፈልጋል።)',
'emailconfirmlink'           => 'አድራሻዎን ለማረጋገጥ',
'invalidemailaddress'        => 'ያው ኢ-ሜል አድራሻ ትክክለኛ አይመስልምና ልንቀበለው አይቻልም። እባክዎ ትክክለኛ አድራሻ ያስግቡ ወይም አለዚያ ጥያቄው ባዶ ይሁን።',
'accountcreated'             => 'ብዕር ስም ተፈጠረ',
'accountcreatedtext'         => 'ለ$1 ብዕር ስም ተፈጥሯል።',
'createaccount-title'        => 'ለ{{SITENAME}} የብዕር ስም መፍጠር',
'createaccount-text'         => 'አንድ ሰው ለኢሜል አድራሻዎ {{SITENAME}} ($4) «$2» የተባለውን ብዕር ስም በመግቢያ ቃል «$3» ፈጥሯል። አሁን ገብተው የመግቢያ ቃልዎን መቀየር ይቫልዎታል።

ይህ ብዕር ስም በስህተት ከተፈጠረ፣ ይህን መልእክት ቸል ማለት ይችላሉ።',
'loginlanguagelabel'         => 'ቋምቋ፦ $1',

# Password reset dialog
'resetpass'               => 'የአባል መግቢያ ቃል ለመቀየር',
'resetpass_announce'      => 'በኢ-ሜል በተላከ ጊዜያዊ ኮድ ገብተዋል። መግባትዎን ለመጨርስ፣ አዲስ መግቢያ ቃል እዚህ መምረጥ አለብዎ።',
'resetpass_header'        => 'መግቢያ ቃል ለመቀየር',
'resetpass_submit'        => 'መግቢያ ቃል ለመቀየርና ለመግባት',
'resetpass_success'       => 'የመግቢያ ቃልዎ መቀየሩ ተከናወነ! አሁን መግባት ይደረግልዎታል......',
'resetpass_bad_temporary' => 'ትክክለኛ ያልሆነ ጊዜያዊ መግቢያ ቃል። ምናልባት ከዚህ በፊት መግቢያ ቃልዎን በመከናወን ቀየሩ፤ ወይም አዲስ ጊዜያዊ መግቢያ ቃል ጠይቀዋል።',
'resetpass_forbidden'     => 'በ{{SITENAME}} የመግቢያ ቃል መቀየር አይቻልም።',
'resetpass_missing'       => 'የማመልከቻ መረጃ የለም።',

# Edit page toolbar
'bold_sample'     => 'ጨለማ ጽሕፈት',
'bold_tip'        => 'ያመለከቱትን ቃላት በጨለማ ጽሕፈት ለማድረግ',
'italic_sample'   => 'ያንጋደደ ጽሕፈት',
'italic_tip'      => 'ያመለከቱትን ቃላት ባንጋደደ (ኢታሊክ) ለማድረግ',
'link_sample'     => 'የመያያዣ ስም',
'link_tip'        => 'ባመለከቱት ቃላት ላይ የዊኪ-ማያያዣ ለማድረግ',
'extlink_sample'  => 'http://www.example.com የውጭ መያያዣ',
'extlink_tip'     => "የውጭ መያያዣ ለመፍጠር (በ'http://' የሚቀደም)",
'headline_sample' => 'ንዑስ ክፍል',
'headline_tip'    => 'የንዑስ-ክፍል አርዕስት ለመፍጠር',
'math_sample'     => 'የሒሳብ ቀመር በዚህ ይግባ',
'math_tip'        => 'የሒሳብ ቀመር (LaTeX) ለመጨመር',
'nowiki_sample'   => 'በዚህ ውስጥ የሚከተት ሁሉ የዊኪ-ሥርአተ ቋንቋን ቸል ይላል',
'nowiki_tip'      => 'የዊኪ-ሥርአተ ቋንቋን ቸል ለማድረግ',
'image_tip'       => 'የስዕል መያያዣ ለመፍጠር',
'media_tip'       => 'የድምጽ ፋይል መያያዣ ለመፍጠር',
'sig_tip'         => 'ፊርማዎ ከነሰዓቱ (4x ~)',
'hr_tip'          => "አድማሳዊ መስመር (በ'----') ለመፍጠር",

# Edit pages
'summary'                   => 'ማጠቃለያ',
'subject'                   => 'ጥቅል ርዕስ',
'minoredit'                 => 'ይህ ለውጥ ጥቃቅን ነው።',
'watchthis'                 => 'ይህንን ገጽ ለመከታተል',
'savearticle'               => 'ገጹን አስቀምጥ',
'preview'                   => 'ሙከራ / preview',
'showpreview'               => 'ቅድመ እይታ',
'showlivepreview'           => 'የቀጥታ ቅድመ-ዕይታ',
'showdiff'                  => 'ማነጻጸሪያ',
'anoneditwarning'           => "'''ማስታወቂያ:''' እርስዎ አሁን በአባል ስምዎ ያልገቡ ነዎት። ማዘጋጀት ይቻሎታል፤ ነገር ግን ለውጦችዎ በአባል ስም ሳይሆን በቁጥር አድራሻዎ ይመዘገባሉ። ከፈለጉ፥ በአባልነት [[Special:UserLogin|መግባት]] ይችላሉ።",
'missingsummary'            => "'''ማስታወሻ፦''' ማጠቃለያ ገና አላቀረቡም። እንደገና «ገጹን ለማቅረብ» ቢጫኑ፣ ያለ ማጠቃለያ ይላካል።",
'missingcommenttext'        => 'እባክዎ አስተያየት ከዚህ በታች ያስግቡ።',
'missingcommentheader'      => "'''ማስታወሻ፦''' ለዚሁ አስተያየት ምንም አርእስት አላቀረቡም። 'ለማቅረብ' እንደገና ቢጫኑ ለውጥዎ ያለ አርዕስት ይሆናል።",
'summary-preview'           => 'የማጠቃለያ ቅድመ እይታ',
'subject-preview'           => 'የአርእስት ቅድመ-ዕይታ',
'blockedtitle'              => 'አባል ተከለክሏል',
'blockedtext'               => "<big>'''የርስዎ ብዕር ስም ወይም ቁጥር አድራሻ ከማዘጋጀት ተከለክሏል።'''</big>

በእርስዎ ላይ ማገጃ የጣለው መጋቢ $1 ነበረ። ምክንያቱም፦ ''$2''

* ማገጃ የጀመረበት ግዜ፦ $8
* ማገጃ የሚያልቅበት ግዜ፦ $6
* የታገደው ተጠቃሚ፦ $7

$1ን ወይም ማንም ሌላ [[{{MediaWiki:Grouppage-sysop}}|መጋቢ]] ስለ ማገጃ ለመጠይቅ ይችላሉ። ነገር ግን በ[[Special:Preferences|ምርጫዎችዎ]] ትክክለኛ ኢሜል ካልኖረ ከጥቅሙም ካልተከለከሉ በቀር ለሰው ኢሜል ለመላክ አይችሉም። የአሁኑኑ ቁጥር አድራሻዎ $3 ህኖ የማገጃው ቁጥር #$5 ነው። ምንም ጥያቄ ካለዎ ይህን ቁጥር ይጨምሩ።",
'autoblockedtext'           => "የእርስዎ ቁጥር አድራሻ በቀጥታ ታግዷል። በ$1 የተገደ ተጠቃሚ ስለ ተጠቀመ ነው። የተሰጠው ምክንያት እንዲህ ነው፦

:''$2''

* ማገጃ የጀመረበት፦ $8
* ማገጃ ያለቀበት፦ $6

ስለ ማገጃው ለመወያየት፣ $1 ወይም ማንምን ሌላ [[{{MediaWiki:Grouppage-sysop}}|መጋቢ]] መጠይቅ ይችላሉ።

በ[[Special:Preferences|ምርጫዎችዎ]] ትክክለኛ ኢ-ሜል አድራሻ ካልሰጡ፣ ወይም ከጥቅሙ ከታገዱ፣ ወደ ሌላ ሰው ኢ-ሜል መላክ እንዳልተቻለዎ ያስታውሱ።

የማገጃዎ ቁጥር # $5 ነው። እባክዎ በማንኛውም ጥያቄ ይህን ቁጥር ይሰጡ።",
'blockednoreason'           => 'ምንም ምክንያት አልተሰጠም',
'blockedoriginalsource'     => "የ'''$1''' ጥሬ ኮድ ምንጭ ከዚህ ታች ይታያል፦",
'blockededitsource'         => "በ'''$1''' ላይ '''የእርስዎ ለውጦች''' ጽሕፈት ከዚህ ታች ይታያሉ፦",
'whitelistedittitle'        => 'ለማዘጋጀት መግባት አስቀድሞ ያስፈልጋል',
'whitelistedittext'         => 'ገጾችን ለማዘጋጀት $1 አስቀድሞ ያስፈልግዎታል።',
'confirmedittitle'          => 'ለማዘጋጀት የኢ-ሜል ማረጋገጫ ያስፈልጋል።',
'confirmedittext'           => 'ገጽ ማዘጋጀት ሳይችሉ፣ አስቀድመው የኢ-ሜል አድራሻዎን ማረጋገጥ አለብዎት። እባክዎ፣ በ[[Special:Preferences|ምርጫዎችዎ]] በኩል ኢ-ሜል አድራሻዎን ያረጋግጡ።',
'nosuchsectiontitle'        => 'የማይኖር ክፍል',
'nosuchsectiontext'         => 'የማይኖር ክፍል ለማዘጋጀት ሞክረዋል። ክፍሉ $1 ስለማይኖር፣ ለውጥዎን ለማስቀመጥ ምንም ሥፍራ የለም።',
'loginreqtitle'             => 'መግባት ያስፈልጋል።',
'loginreqlink'              => 'መግባት',
'loginreqpagetext'          => 'ሌሎች ገጾች ለመመልከት $1 ያስፈልግዎታል።',
'accmailtitle'              => 'የመግቢያ ቃል ተላከ።',
'accmailtext'               => 'የመግቢያ ቃል ለ«$1» ወደ $2 ተልኳል።',
'newarticle'                => '(አዲስ)',
'newarticletext'            => 'እርስዎ የተከተሉት መያያዣ እስካሁን ወደማይኖር ገጽ የሚወስድ ነው። ገጹን አሁን ለመፍጠር፣ ከታች በሚገኘው ሳጥን ውስጥ መተየብ ይጀምሩ። ለተጨማሪ መረጃ፣ [[{{MediaWiki:Helppage}}|የእርዳታ ገጽን]] ይመልከቱ።

ወደዚህ በስሕተት ከሆነ የመጡት፣ የቃኝውን «Back» ቁልፍ ይጫኑ።',
'anontalkpagetext'          => "----''ይኸው ገጽ ገና ያልገባ ወይም ብዕር ስም የሌለው ተጠቃሚ ውይይት ገጽ ነው። መታወቂያው በ[[ቁጥር አድራሻ]] እንዲሆን ያስፈልጋል። አንዳንዴ ግን አንድ የቁጥር አድራሻ በሁለት ወይም በብዙ ተጠቃሚዎች የጋራ ሊሆን ይችላል። ስለዚህ ለርስዎ የማይገባ ውይይት እንዳይደርስልዎ፣ [[Special:UserLogin|«መግቢያ»]] በመጫን የብዕር ስም ለማውጣት ይችላሉ።''",
'noarticletext'             => 'በአሁኑ ወቅት በዚህ ገጽ ላይ ምንም ጽሑፍ የለም፤ በሌላ ገጾች [[Special:Search/{{PAGENAME}}|የዚህን ገጽ አርዕስት መፈለግ]] ወይም [{{fullurl:{{FULLPAGENAME}}|action=edit}} አዲስ ገፅ ማዘጋጀት ይችላሉ].',
'userpage-userdoesnotexist' => 'የብዕር ስም «$1» አልተመዘገበም። እባክዎ ይህን ገጽ ለመፍጠር/ ለማስተካከል የፈለጉ እንደ ሆነ ያረጋግጡ።',
'usercssjsyoucanpreview'    => "<strong>ምክር፦</strong> ሳይቆጠብ አዲስ CSS/JSዎን ለመሞከር 'ቅድመ እይታ' የሚለውን ይጫኑ።",
'usercsspreview'            => "'''ማስታወሻ፦ CSS-ዎን ለሙከራ ብቻ እያዩ ነው፤ ገና አልተቆጠበም!'''",
'userjspreview'             => "'''ማስታወሻ፦ JavaScriptዎን ለሙከራ ብቻ እያዩ ነው፤ ገና አልተቆጠበም!'''",
'userinvalidcssjstitle'     => "'''ማስጠንቀቂያ፦''' «$1» የሚባል መልክ የለም። ልዩ .css እና .js ገጾች በትንንሽ እንግሊዝኛ ፊደል መጀመር እንዳለባቸው ያስታውሱ። ለምሳሌ፦  {{ns:user}}:Foo/monobook.css ልክ ነው እንጂ {{ns:user}}:Foo/Monobook.css አይደለም።",
'updated'                   => '(የታደሰ)',
'note'                      => '<strong>ማሳሰቢያ፦</strong>',
'previewnote'               => 'ማስታወቂያ፦ <strong><big>ይህ ለሙከራው ብቻ ነው የሚታየው -- ምንም ለውጦች ገና አልተላኩም!</big></strong>',
'previewconflict'           => 'ለማስቀምጥ የመረጡ እንደ ሆነ እንደሚታይ፣ ይህ ቅድመ-ዕይታ በላይኛ ጽሕፈት ማዘጋጀት ክፍል ያለውን ጽሕፈት ያንጸባርቃል።',
'session_fail_preview'      => '<strong>ይቅርታ! ገጹን ለማቅረብ ስንሂድ፣ አንድ ትንሽ ችግር በመረቡ መረጃ ውስጥ ድንገት ገብቶበታል። እባክዎ፣ እንደገና ገጹን ለማቅረብ አንዴ ይሞክሩ። ከዚያ ገና ካልሠራ፣ ምናልባት ከአባል ስምዎ መውጣትና እንደገና መግባት ይሞክሩ።</strong>',
'editing'                   => '«$1» ማዘጋጀት / ማስተካከል',
'editingsection'            => '«$1» (ክፍል) ማዘጋጀት / ማስተካከል',
'editingcomment'            => '$1 ማዘጋጀት (ውይይት መጨመር)',
'editconflict'              => 'ተቃራኒ ለውጥ፦ $1',
'explainconflict'           => "ይህን ገጽ ለማዘጋጀት ከጀመሩ በኋላ የሌላ ሰው ለውጥ ገብቷል። ላይኛው ጽሕፈት የአሁኑ እትም ያሳያል፤ የርስዎም እትም ከዚያ በታች ይገኛል። ለውጦችዎን በአሁኑ ጽሕፈት ውስጥ ማዋሐድ ይኖርብዎታል። ገጹንም ባቀረቡበት ግዜ በላይኛው ክፍል ያለው ጽሕፈት '''ብቻ''' ይቀርባል።",
'yourtext'                  => 'የእርስዎ እትም',
'storedversion'             => 'የተቆጠበው እትም',
'editingold'                => '<strong>ማስጠንቀቂያ፦
ይህ እትም የአሁኑ አይደለም፣ ከዚህ ሁናቴ ታድሷል።
ይህንን እንዳቀረቡ ከዚህ እትም በኋላ የተቀየረው ለውጥ ሁሉ ያልፋል።</strong>',
'yourdiff'                  => 'ልዩነቶች',
'copyrightwarning'          => "*<big> '''መጣጥፎችን ለመፍጠርና ለማሻሻል አይፈሩ''!''''' — </big>ሥራዎ ትክክለኛ ካልሆነ፣ በሌሎቹ አዘጋጆች ሊታረም ይችላል።",
'copyrightwarning2'         => 'ወደ {{SITENAME}} የሚላከው አስተዋጽኦ ሁሉ በሌሎች ተጠቃሚዎች ሊታረም፣ ሊለወጥ፣ ወይም ሊጠፋ እንደሚቻል ያስታውሱ። ጽሕፈትዎ እንዲታረም ካልወደዱ፣ ወደዚህ አይልኩት።<br />
ደግሞ ይህ የራስዎ ጽሕፈት ወይም ከነጻ ምንጭ የተቀዳ ጽሕፈት መሁኑን ያረጋግጣሉ። (ለዝርዝር $1 ይዩ)።
<strong>አለፈቃድ፡ መብቱ የተጠበቀውን ሥራ አይልኩት!</strong>',
'longpagewarning'           => '<strong>ማስጠንቀቂያ፦ የዚሁ ገጽ መጠን እስከ $1 kilobyte ድረስ ደርሷል፤ አንድ ጽሑፍ ከ32 kilobyte የበለጠ ሲሆን ይህ ግዙፍነት ለአንዳንድ ተጠቃሚ ዌብ-ብራውዘር ያስቸግራል። እባክዎን፣ ገጹን ወደ ተለያዩ ገጾች ማከፋፈልን ያስቡበት። </strong>',
'longpageerror'             => '<strong>ስህተት፦ ያቀረቡት ጽሕፈት $1 kb ነው፤ ይህም ከተፈቀደው ወሰን $2 kb በላይ ነው። ሊቆጠብ አይችልም።</strong>',
'readonlywarning'           => ':<strong>ማስታወቂያ፦</strong> {{SITENAME}} አሁን ለአጭር ግዜ ተቆልፎ ገጹን ለማቅረብ አይቻልም። ጥቂት ደቂቃ ቆይተው እባክዎ እንደገና ይሞክሩት!
:(The database has been temporarily locked for maintenance, so you cannot save your edits at this time. You may wish to cut-&-paste the text into another file, and try again in a moment or two.)',
'protectedpagewarning'      => '<strong>ማስጠንቀቂያ፦ ይህ ገጽ ከመጋቢ በስተቀር በማንም እንዳይለወጥ ተቆልፏል።</strong>',
'semiprotectedpagewarning'  => "'''ማስታወቂያ፦''' ይኸው ገጽ ከቋሚ አዛጋጆች በተቀር በማንም እንዳይለወጥ ተቆልፏል።",
'cascadeprotectedwarning'   => "'''ማስጠንቀቂያ፦''' ይህ ገጽ በመጋቢ ብቻ እንዲታረም ተቆልፏል። ምክንያቱም {{PLURAL:$1|በሚከተለው በውስጡ የሚያቆልፍ ገጽ|በሚከተሉ በውስጡ ይሚያቆልፉ ገጾች}} ውስጥ ይገኛል።",
'titleprotectedwarning'     => '<strong>ማስጠንቀቂያ፦ ይህ ገጽ አንዳንድ ተጠቃሚ ብቻ ሊፈጠር እንዲችል ተቆልፏል።</strong>',
'templatesused'             => 'በዚሁ ገጽ ላይ የሚገኙት መልጠፊያዎች እነዚህ ናቸው፦',
'templatesusedpreview'      => 'በዚሁ ቅድመ-እይታ የሚገኙት መልጠፊያዎች እነዚህ ናቸው፦',
'templatesusedsection'      => 'በዚሁ ክፍል የተጠቀሙት መልጠፊያዎች፦',
'template-protected'        => '(የተቆለፈ)',
'template-semiprotected'    => '(በከፊል የተቆለፈ)',
'hiddencategories'          => 'ይህ ገጽ በ{{PLURAL:$1|1 የተደበቀ መደብ|$1 የተደበቁ መድቦች}} ውስጥ ይገኛል።',
'nocreatetitle'             => 'የገጽ መፍጠር ተወሰነ',
'nocreatetext'              => '{{SITENAME}} አዳዲስ ገጾችን ለመፍጠር ያሚያስችል ሁኔታ ከለክሏል። ተመልሰው የቆየውን ገጽ ማዘጋጀት ይችላሉ፤ አለዚያ [[Special:UserLogin|በብዕር ስም መግባት]] ይችላሉ።',
'nocreate-loggedin'         => 'አዲስ ገጽ በ{{SITENAME}} ለመፍጠር ፈቃድ የለዎም።',
'permissionserrors'         => 'የፈቃድ ስሕተቶች',
'permissionserrorstext'     => 'ያ አድራጎት አይቻልም - {{PLURAL:$1|ምክንያቱም|ምክንያቶቹም}}፦',
'recreate-deleted-warn'     => ":<strong><big>'''ማስጠንቀቂያ፦ ይኸው አርእስት ከዚህ በፊት የጠፋ ገጽ ነው!'''</big></strong>

*እባክዎ፥ ገጹ እንደገና እንዲፈጠር የሚገባ መሆኑን ያረጋግጡ።

*የገጹ መጥፋት ዝርዝር ከዚህ ታች ይታያል።",

# "Undo" feature
'undo-success' => "ያ ለውጥ በቀጥታ ሊገለበጥ ይቻላል። እባክዎ ከታች ያለውን ማነጻጸርያ ተመልክተው ይህ እንደሚፈልጉ ያረጋግጡና ለውጡ እንዲገለበጥ '''ገጹን ለማቅረብ''' ይጫኑ።",
'undo-failure' => 'ከዚሁ ለውጥ በኋላ ቅራኔ ለውጦች ስለ ገቡ ሊገለበጥ አይቻልም።',
'undo-summary' => 'አንድ ለውጥ $1 ከ[[Special:Contributions/$2|$2]] ([[User talk:$2|ውይይት]]) ገለበጠ',

# Account creation failure
'cantcreateaccounttitle' => 'ብዕር ስም ለመፍጠር አይቻልም',
'cantcreateaccount-text' => "ከዚሁ የቁጥር አድራሻ ('''$1''') የብዕር ስም መፍጠር በ[[User:$3|$3]] ታግዷል።

በ$3 የተሰጠው ምክንያት ''$2'' ነው።",

# History pages
'viewpagelogs'        => 'መዝገቦች ለዚሁ ገጽ',
'nohistory'           => 'ለዚሁ ገጽ የዕትሞች ታሪክ የለም።',
'revnotfound'         => 'እትም አልተገኘም',
'revnotfoundtext'     => 'ለዚህ ገጽ የጠየቁት የቆየው ዕትም ሊገኝ አልቻለም። እባክዎ ይህን ገጽ ለማግኘት የተጠቀመው URL ይመልከቱ።',
'currentrev'          => 'የአሁኑ እትም',
'revisionasof'        => 'እትም በ$1',
'revision-info'       => 'የ$1 ዕትም (ከ$2 ተዘጋጅቶ)',
'previousrevision'    => '← የፊተኛው እትም',
'nextrevision'        => 'የሚከተለው እትም →',
'currentrevisionlink' => '«የአሁኑን እትም ለመመልከት»',
'cur'                 => 'ከአሁን',
'next'                => 'ቀጥሎ',
'last'                => 'ካለፈው',
'page_first'          => 'ፊተኞች',
'page_last'           => 'ኋለኞች',
'histlegend'          => "ከ2 እትሞች መካከል ልዩነቶቹን ለመናበብ፦ በ2 ክብ ነገሮች ውስጥ ምልክት አድርገው «የተመረጡትን እትሞች ለማነፃፀር» የሚለውን ተጭነው የዛኔ በቀጥታ ይሄዳሉ።<br /> መግለጫ፦ (ከአሁን) - ከአሁኑ እትም ያለው ልዩነት፤ (ካለፈው) - ቀጥሎ ከቀደመው እትም ያለው ልዩነት፤<br /> «'''ጥ'''» ማለት ጥቃቅን ለውጥ ነው።",
'deletedrev'          => '[የተደለዘ]',
'histfirst'           => 'ቀድመኞች',
'histlast'            => 'ኋለኞች',
'historysize'         => '($1 byte)',
'historyempty'        => '(ባዶ)',

# Revision feed
'history-feed-title'          => 'የዕትሞች ታሪክ',
'history-feed-description'    => 'በዊኪ ላይ የዕትሞች ታሪክ ለዚሁ ገጽ',
'history-feed-item-nocomment' => '$1 በ$2', # user at time
'history-feed-empty'          => 'የተጠየቀው ገጽ አይኖርም። ምናልባት ከዊኪው ጠፍቷል፣ ወይም ወደ አዲስ ስም ተዛወረ። ለተመሳሳይ አዲስ ገጽ [[Special:Search|ፍለጋ]] ይሞክሩ።',

# Revision deletion
'rev-deleted-comment'     => '(ማጠቃልያ ተደለዘ)',
'rev-deleted-user'        => '(ብዕር ስም ተደለዘ)',
'rev-deleted-event'       => '(መዝገቡ ድርጊት ተወግዷል)',
'rev-delundel'            => 'ይታይ/ይደበቅ',
'revdelete-nooldid-title' => 'የማይሆን ግብ እትም',
'revdelete-nooldid-text'  => 'ይህ ተግባር የሚፈጸምበት ግብ (አላማ) እትም አልወሰኑም።',
'revdelete-selected'      => 'ከ [[:$1]] {{PLURAL:$2|የተመረጡ ዝርያዎች|የተመረጡ ዝርያዎች}}:',
'logdelete-selected'      => '{{PLURAL:$1|የተመረጠ መዝገብ ድርጊት|የተመረጡ መዝገብ ድርጊቶች}}፦',
'revdelete-hide-text'     => 'የእትሙ ጽሕፈት ይደበቅ',
'revdelete-hide-name'     => 'ድርጊትና ግቡ ይደበቅ',
'revdelete-hide-comment'  => 'ማጠቃለያ ይደበቅ',
'revdelete-hide-user'     => 'የአዘጋጁ ብዕር ስም ወይም ቁ. አድርሻ ይደበቅ',
'revdelete-hide-image'    => 'የፋይሉ ይዞታ ይደበቅ',
'revdelete-log'           => 'የመዝገቡ ማጠቃለያ፦',

# History merging
'mergehistory'                     => 'የገጽ ታሪኮች ለመዋሐድ',
'mergehistory-box'                 => 'የሁለት ገጾች እትሞች ለማዋሐድ፦',
'mergehistory-from'                => 'መነሻው ገጽ፦',
'mergehistory-into'                => 'መድረሻው ገጽ፦',
'mergehistory-list'                => 'መዋሐድ የሚችሉ እትሞች ታሪክ',
'mergehistory-go'                  => 'መዋሐድ የሚችሉ እትሞች ይታዩ',
'mergehistory-submit'              => 'እትሞቹን ለማዋሐድ',
'mergehistory-empty'               => 'ምንም ዕትም ማዋሐድ አይቻልም።',
'mergehistory-success'             => 'ከ[[:$1]] $3 {{PLURAL:$3|እትም|እትሞች}} ወደ [[:$2]] መዋሐዱ ተከናወነ።',
'mergehistory-fail'                => 'የታሪክ መዋሐድ አይቻልም፤ እባክዎ የገጽና የጊዜ ግቤቶች እንደገና ይመለከቱ።',
'mergehistory-no-source'           => 'መነሻው ገጽ $1 አይኖርም።',
'mergehistory-no-destination'      => 'መድረሻው ገጽ $1 አይኖርም።',
'mergehistory-invalid-source'      => 'መነሻው ገጽ ትክክለኛ አርእስት መሆን አለበት።',
'mergehistory-invalid-destination' => 'መድረሻው ገጽ ትክክለኛ አርእስት መሆን አለበት።',

# Merge log
'mergelog'           => 'የመዋሐድ መዝገብ',
'pagemerge-logentry' => '[[$1]]ን ወደ [[$2]] አዋሐደ (እትሞች እስከ $3 ድረስ)',
'revertmerge'        => 'መዋሐዱን ለመገልበጥ',
'mergelogpagetext'   => 'የአንድ ገጽ ታሪክ ወደ ሌላው ሲዋሐድ ከዚህ ታች ያለው ዝርዝር ያሳያል።',

# Diffs
'history-title'           => 'የ«$1» እትሞች ታሪክ',
'difference'              => '(በ2ቱ እትሞቹ ዘንድ ያለው ልዩነት)',
'lineno'                  => 'መስመር፡ $1፦',
'compareselectedversions' => 'የተመረጡትን እትሞች ለማነፃፀር',
'editundo'                => 'ለውጡ ይገለበጥ',
'diff-multi'              => '(ከነዚህ 2 እትሞች መካከል {{PLURAL:$1|አንድ ለውጥ ነበር|$1 ለውጦች ነበሩ}}።)',

# Search results
'searchresults'         => 'የፍለጋ ውጤቶች',
'searchresulttext'      => 'በተጨማሪ ስለ ፍለጋዎች ለመረዳት፣ [[{{MediaWiki:Helppage}}]] ያንብቡ።',
'searchsubtitle'        => "'''ፍለጋ ለ[[:$1]]፦'''",
'searchsubtitleinvalid' => "ለ'''$1''' ፈለጉ",
'noexactmatch'          => "በ«$1» አርዕስት የሚሰየም መጣጥፍ '''አልተገኘም'''፤ እርሶ ግን [[:$1|ሊፈጥሩት ይችላሉ]]... ።",
'noexactmatch-nocreate' => "'''«$1» የሚባል ገጽ የለም።'''",
'toomanymatches'        => 'ከመጠን በላይ ያሉ ስምምነቶች ተመለሱ፤ እባክዎ ሌላ ጥያቄ ይሞክሩ።',
'titlematches'          => 'የሚስማሙ አርዕስቶች',
'notitlematches'        => 'የሚስማሙ አርዕስቶች የሉም',
'textmatches'           => 'ጽሕፈት የሚስማማባቸው ገጾች',
'notextmatches'         => 'ጽሕፈት የሚስማማባቸው ገጾች የሉም',
'prevn'                 => 'ፊተኛ $1',
'nextn'                 => 'ቀጥሎ $1',
'viewprevnext'          => 'በቁጥር ለማየት፡ ($1) ($2) ($3).',
'search-result-size'    => '$1 ({{PLURAL:$2|1 ቃል|$2 ቃላት}})',
'search-result-score'   => 'ተገቢነት፦ $1%',
'showingresults'        => 'ከ ቁ.#<b>$2</b> ጀምሮ እስከ <b>$1</b> ውጤቶች ድረስ ከዚህ በታች ይታያሉ።',
'showingresultsnum'     => "ከ#'''$2''' ጀምሮ {{PLURAL:$3|'''1''' ውጤት|'''$3''' ውጤቶች}} ከዚህ ታች ማየት ይቻላል።",
'powersearch'           => 'ፍለጋ',
'powersearch-legend'    => 'ተጨማሪ ፍለጋ',
'searchdisabled'        => '{{SITENAME}} ፍለጋ አሁን እንዳይሠራ ተደርጓል። ለጊዜው ግን በGoogle ላይ መፈልግ ይችላሉ። የ{{SITENAME}} ይዞታ ማውጫ በዚያ እንዳልታደሰ ማቻሉ ያስታውሱ።',

# Preferences page
'preferences'              => 'ምርጫዎች፤',
'mypreferences'            => 'ምርጫዎች፤',
'prefs-edits'              => 'የለውጦች ቁጥር:',
'prefsnologin'             => 'ገና አልገቡም',
'prefsnologintext'         => 'ምርጫዎችዎን ለማስተካከል አስቀድሞ [[Special:UserLogin|መግባት]] ያስፈልግዎታል።',
'prefsreset'               => 'ምርጫዎች ከመቆጠቢያ ታድሰዋል።',
'qbsettings-none'          => 'የለም',
'qbsettings-fixedleft'     => 'በግራ የተለጠፈ',
'qbsettings-fixedright'    => 'በቀኝ የተለጠፈ',
'qbsettings-floatingleft'  => 'በግራ ተንሳፋፊ',
'qbsettings-floatingright' => 'በቀኝ ተንሳፋፊ',
'changepassword'           => 'መግቢያ ቃልዎን ለመቀየር',
'skin'                     => 'የድህረ-ገጽ መልክ',
'math'                     => 'የሂሳብ መልክ',
'dateformat'               => 'ያውሮፓ አቆጣጠር ዘመን ሥርዓት',
'datedefault'              => 'ግድ የለኝም',
'datetime'                 => 'ዘመንና ሰዓት',
'math_failure'             => 'ዘርዛሪው ተሳነው',
'math_unknown_error'       => 'የማይታወቅ ስኅተት',
'math_unknown_function'    => 'የማይታወቅ ተግባር',
'math_lexing_error'        => 'የlexing ስህተት',
'math_syntax_error'        => 'የሰዋሰው ስህተት',
'math_bad_output'          => 'ወደ math ውጤት ዶሴ መጻፍ ወይም መፍጠር አይቻልም',
'prefs-personal'           => 'ያባል ዶሴ',
'prefs-rc'                 => 'የቅርቡ ለውጦች ዝርዝር',
'prefs-watchlist'          => 'የሚከታተሉ ገጾች',
'prefs-watchlist-days'     => 'በሚከታተሉት ገጾች ዝርዝር ስንት ቀን ይታይ፤',
'prefs-watchlist-edits'    => 'በተደረጁት ዝርዝር ስንት ለውጥ ይታይ፤',
'prefs-misc'               => 'ልዩ ልዩ ምርጫዎች',
'saveprefs'                => 'ይቆጠብ',
'resetprefs'               => 'እንደ በፊቱ ይታደስ',
'oldpassword'              => 'የአሁኑ መግቢያ ቃልዎ',
'newpassword'              => 'አዲስ መግቢያ ቃል',
'retypenew'                => 'አዲስ መግቢያ ቃል ዳግመኛ',
'textboxsize'              => 'የማዘጋጀት ምርጫዎች',
'rows'                     => 'በማዘጋጀቱ ሰንጠረዥ ስንት ተርታዎች?',
'columns'                  => 'ስንት ዓምዶችስ?',
'searchresultshead'        => 'ፍለጋ',
'resultsperpage'           => 'ስንት ውጤቶች በየገጹ?',
'contextlines'             => 'ስንት መስመሮች በየውጤቱ?',
'contextchars'             => 'ስንት ፊደላት በየመስመሩ?',
'recentchangesdays'        => 'በቅርቡ ለውጦች ዝርዝር ስንት ቀን ይታይ?',
'recentchangescount'       => 'በዝርዝርዎ ላይ ስንት ለውጥ ይታይ? (እስከ 500)',
'savedprefs'               => 'ምርጫዎችህ ተቆጥበዋል።',
'timezonelegend'           => 'የሰዓት ክልል',
'timezonetext'             => '¹ከ Server time (UTC) ያለው ልዩነት (በሰዓቶች ቁጥር) (እንደ ኢትዮጵያ ጊዜ ለማድረግ እንደገና ስድስት ሰዓት ይጨምሩ።)',
'localtime'                => 'የክልሉ ሰዓት (Local time)',
'timezoneoffset'           => 'ኦፍ ሰት¹',
'servertime'               => 'የሰርቨሩ ሰዓት',
'guesstimezone'            => 'ከኮምፒውተርዎ መዝገብ ልዩነቱ ይገኝ',
'allowemail'               => 'ኢሜል ከሌሎች ተጠቃሚዎች ለመፍቀድ',
'defaultns'                => 'በመጀመርያው ፍለጋዎ በነዚህ ክፍለ-ዊኪዎች ብቻ ይደረግ:',
'default'                  => 'ቀዳሚ',
'files'                    => 'የስዕሎች መጠን',

# User rights
'userrights'               => 'የአባል መብቶች ለማስተዳደር', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'የ1 አባል ማዕረግ ለማስተዳደር',
'userrights-user-editname' => 'ለዚሁ ብዕር ስም፦',
'editusergroup'            => 'የአባሉ ማዕረግ ለማስተካከል',
'editinguser'              => "ይህ ማመልከቻ ለብዕር ስም '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) መብቶቹን ለመቀየር ነው።",
'userrights-editusergroup' => 'የአባሉ ማዕረግ ለማስተካከል',
'saveusergroups'           => 'ለውጦቹ ይቆጠቡ',
'userrights-groupsmember'  => 'አሁን ያሉባቸው ማዕረጎች፦',
'userrights-reason'        => 'የመቀየሩ ምክንያት፦',
'userrights-no-interwiki'  => 'ማዕረጎችን በሌላ ዊኪ ላይ ለማስተካከል ፈቃድ የለዎም።',
'userrights-nodatabase'    => 'መረጃ-ቤቱ $1 አይኖርም ወይም የቅርብ አካባቢ አይደለም።',
'userrights-nologin'       => 'የአባል መብቶች ለመወሰን መጋቢ ሆነው [[Special:UserLogin|መግባት]] ያስፈልግዎታል።',
'userrights-notallowed'    => 'የአባል መብቶች ለማስተካከል ፈቃድ የለዎም።',

# Groups
'group'               => 'ደረጃ፦',
'group-autoconfirmed' => 'የተረጋገጡ አባላት',
'group-bot'           => 'BOTS',
'group-sysop'         => 'መጋቢ',
'group-bureaucrat'    => 'አስተዳዳሪዎች',
'group-all'           => '(ሁሉ)',

'group-autoconfirmed-member' => 'የተረጋገጠ ተጠቃሚ',
'group-bot-member'           => 'BOT',
'group-sysop-member'         => 'መጋቢ',
'group-bureaucrat-member'    => 'አስተዳዳሪ',

'grouppage-autoconfirmed' => '{{ns:project}}:የተረጋገጡ ተጠቃሚዎች',
'grouppage-bot'           => '{{ns:project}}:BOTS',
'grouppage-sysop'         => '{{ns:project}}:መጋቢዎች',
'grouppage-bureaucrat'    => '{{ns:project}}:አስተዳዳሪዎች',

# User rights log
'rightslog'      => 'የአባል መብቶች መዝገብ',
'rightslogtext'  => 'ይህ መዝገብ የአባል መብቶች ሲለወጡ ይዘረዝራል።',
'rightslogentry' => 'የ$1 ማዕረግ ከ$2 ወደ $3 ለወጠ',
'rightsnone'     => '(የለም)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|ለውጥ|ለውጦች}}',
'recentchanges'                     => 'በቅርብ ጊዜ የተለወጡ',
'recentchangestext'                 => "በዚሁ ገጽ ላይ በቅርብ ጊዜ የወጡ አዳዲስ ለውጦች ለመከታተል ይችላሉ። <br /> ('''ጥ'''፦ ጥቃቅን ለውጥ፤ '''አ'''፦ አዲስ ገጽ)",
'recentchanges-feed-description'    => 'በዚህ ዊኪ ላይ በቅርብ ግዜ የተለወጠውን በዚሁ feed መከታተል ይችላሉ',
'rcnote'                            => "ከ$5 $4 እ.ኤ.አ. {{PLURAL:$2|ባለፈው 1 ቀን|ባለፉት '''$2''' ቀኖች}} {{PLURAL:$1|የተደረገው '''1''' ለውት እታች ይገኛል|የተደረጉት '''$1''' መጨረሻ ለውጦች እታች ይገኛሉ}}።",
'rcnotefrom'                        => "ከ'''$2''' ጀምሮ የተቀየሩትን ገጾች (እስከ '''$1''' ድረስ) ክዚህ በታች ይታያሉ።",
'rclistfrom'                        => '(ከ $1 ጀምሮ አዲስ ለውጦቹን ለማየት)',
'rcshowhideminor'                   => 'ጥቃቅን ለውጦች $1',
'rcshowhidebots'                    => 'bots $1',
'rcshowhideliu'                     => 'ያባላት ለውጦች $1',
'rcshowhideanons'                   => 'የቁ. አድራሻ ለውጦች $1',
'rcshowhidepatr'                    => 'የተቆጣጠሩ ለውጦች $1',
'rcshowhidemine'                    => 'የኔ $1',
'rclinks'                           => 'ባለፉት $2 ቀን ውስጥ የወጡት መጨረሻ $1 ለውጦች ይታዩ።<br />($3)',
'diff'                              => 'ለውጡ',
'hist'                              => 'ታሪክ',
'hide'                              => 'ይደበቁ',
'show'                              => 'ይታዩ',
'minoreditletter'                   => 'ጥ',
'newpageletter'                     => 'አ',
'boteditletter'                     => 'B',
'number_of_watching_users_pageview' => '[$1 የሚከታተሉ {{PLURAL:$1|ተጠቃሚ|ተጠቃሚዎች}}]',
'rc_categories_any'                 => 'ማንኛውም',
'newsectionsummary'                 => '/* $1 */ አዲስ ክፍል',

# Recent changes linked
'recentchangeslinked'          => 'የተዛመዱ ለውጦች',
'recentchangeslinked-title'    => 'በ«$1» በተዛመዱ ገጾች ቅርብ ለውጦች',
'recentchangeslinked-noresult' => 'በተመለከተው ጊዜ ውስጥ ከዚህ በተያየዙት ገጾች ላይ ምንም ለውጥ አልነበረም።',
'recentchangeslinked-summary'  => "ከዚሁ ገጽ የተያየዙት ሌሎች ጽሑፎች ቅርብ ለውጦች ከታች ይዘረዝራሉ።

በሚከታተሉት ገጾች መካከል ያሉት ሁሉ በ'''ጨለማ ጽሕፈት''' ይታያሉ።",

# Upload
'upload'                      => 'ፋይል / ሥዕል ለመላክ',
'uploadbtn'                   => 'ፋይሉ ይላክ',
'reupload'                    => 'እንደገና ለመላክ',
'reuploaddesc'                => 'ለመሰረዝና ወደ መላኪያ ማመልከቻ ለመመለስ',
'uploadnologin'               => 'ገና አልገቡም',
'uploadnologintext'           => 'ፋይል ለመላክ አስቀድሞ [[Special:UserLogin|መግባት]] ያስፈልግዎታል።',
'uploaderror'                 => 'የመላክ ስሕተት',
'uploadtext'                  => "በዚህ ማመልከቻ ላይ ፋይል ለመላክ ይችላሉ። ቀድሞ የተላኩት ስዕሎች [[Special:ImageList|በፋይል / ሥዕሎች ዝርዝር]] ናቸው፤ ከዚህ በላይ የሚጨመረው ፋይል ሁሉ [[Special:Log/upload|በፋይሎች መዝገብ]] ይዘረዝራሉ።

ስዕልዎ በጽሑፍ እንዲታይ '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Filename.jpg]]</nowiki>''' ወይም
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Filename.png|thumb|ሌላ ጽሑፍ]]</nowiki>''' በሚመስል መልክ ይጠቅሙ።",
'upload-permitted'            => 'የተፈቀዱት የፋይል አይነቶች፦ $1 ብቻ ናቸው።',
'upload-preferred'            => 'የተመረጡት የፋይል አይነቶች፦  $1።',
'upload-prohibited'           => 'ያልተፈቀዱት የፋይል አይነቶች፦ $1።',
'uploadlog'                   => 'የፋይሎች መዝገብ',
'uploadlogpage'               => 'የፋይሎች መዝገብ',
'uploadlogpagetext'           => 'ይህ መዝገብ በቅርቡ የተላኩት ፋይሎች ሁሉ ያሳያል።',
'filename'                    => 'የፋይል ስም',
'filedesc'                    => 'ማጠቃለያ',
'fileuploadsummary'           => 'ማጠቃለያ፦',
'filestatus'                  => 'የማብዛት መብት ሁኔታ፦',
'filesource'                  => 'መነሻ፦',
'uploadedfiles'               => 'የተላኩ ፋይሎች',
'ignorewarning'               => 'ማስጠንቀቂያውን ቸል በማለት ፋይሉ ይላክ።',
'ignorewarnings'              => 'ማስጠንቀቂያ ቸል ይበል',
'minlength1'                  => 'የፋይል ስም ቢያንስ አንድ ፊደል መሆን አለበት።',
'illegalfilename'             => 'የፋይሉ ስም «$1» በአርእስት ያልተፈቀደ ፊደል ወይም ምልክት አለበት። እባክዎ፣ ለፋይሉ አዲስ ስም ያውጡና እንደገና ይልኩት።',
'badfilename'                 => 'የፋይል ስም ወደ «$1» ተቀይሯል።',
'filetype-badmime'            => 'የMIME አይነት «$1» ፋይሎች ሊላኩ አይፈቀዱም።',
'filetype-unwanted-type'      => "'''\".\$1\"''' ያልተፈለገ ፋይል አይነት ነው። የተመረጡት ፋይል አይነቶች \$2 ናቸው።",
'filetype-banned-type'        => "'''«.$1»''' ያልተፈቀደ ፋይል አይነት ነው። የተፈቀዱት ፋይል አይነቶች $2 ናቸው።",
'filetype-missing'            => 'ፋይሉ ምንም ቅጥያ (ለምሳሌ «.jpg») የለውም።',
'large-file'                  => 'የፋይል መጠን ከ$1 በላይ እንዳይሆን ይመከራል፤ የዚህ ፋይል መጠን $2 ነው።',
'largefileserver'             => 'ይህ ፋይል ሰርቨሩ ከሚችለው መጠን በላይ ነው።',
'emptyfile'                   => 'የላኩት ፋይል ባዶ እንደ ሆነ ይመስላል። ይህ ምናልባት በፋይሉ ስም አንድ ግድፋት ስላለ ይሆናል። እባክዎ ይህን ፋይል በውኑ መላክ እንደ ፈለጉ ያረጋግጡ።',
'fileexists'                  => 'ይህ ስም ያለው ፋይል አሁን ይኖራል፤ እባክዎ እሱም ለመቀየር እንደፈለጉ እርግጥኛ ካልሆኑ <strong><tt>$1</tt></strong> ይመለከቱ።',
'filepageexists'              => 'የዚሁ ፋኡል መግለጫ ገጽ ከዚህ በፊት በ<strong><tt>$1</tt></strong> ተፈጥሯል፤ ነገር ግን ይህ ስም ያለበት ፋይል አሁን አይኖርም። ስለዚህ ያቀረቡት ማጠቃለያ በመግለጫው ገጽ አይታይም። መግለጫዎ በዚያ እንዲታይ በእጅ ማስገባት ይኖርብዎታል።',
'fileexists-extension'        => 'ተመሳሳይ ስም ያለበት ፋይል ይኖራል፦<br />
የሚላክ ፋይል ስም፦ <strong><tt>$1</tt></strong><br />
የሚኖር (የቆየው) ፋይል ስም፦ <strong><tt>$2</tt></strong><br />
እባክዎ ሌላ ስም ይምረጡ።',
'fileexists-thumb'            => "<center>'''የሚኖር ፋይል'''</center>",
'fileexists-thumbnail-yes'    => 'ፋይሉ የተቀነሰ መጠን ያለበት ስዕል <i>(ናሙና)</i> እንደ ሆነ ይመስላል። እባክዎ ፋይሉን <strong><tt>$1</tt></strong> ይመለከቱ።<br /> ያው ፋይል ለዚሁ ፋይል አንድ አይነት በኦሪጂናሉ መጠን ቢሆን ኖሮ፣ ተጨማሪ ናሙና መላክ አያስፈልግም።',
'file-thumbnail-no'           => 'የፋይሉ ስም በ<strong><tt>$1</tt></strong> ይጀመራል። የተቀነሰ መጠን ያለበት ስዕል <i>(ናሙና)</i> እንደ ሆነ ይመስላል። ይህን ስዕል በሙሉ ማጉላት ካለዎ፣ ይህን ይላኩ፤ አለዚያ እባክዎ የፋይሉን ስም ይቀይሩ።',
'fileexists-forbidden'        => 'በዚህ ስም የሚኖር ፋይል ገና አለ፤ እባክዎ ተመልሰው ይህን ፋይል በአዲስ ስም ስር ይልኩት። [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'ይህ ስም ያለበት ፋይል አሁን በጋራ ፋይል ምንጭ ይኖራል፤ እባክዎ ተመልሰው ፋይሉን በሌላ ስም ስር ይላኩት። [[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'መላኩ ተከናወነ',
'uploadwarning'               => 'የመላክ ማስጠንቀቂያ',
'savefile'                    => 'ፋይሉ ለመቆጠብ',
'uploadedimage'               => '«[[$1]]» ላከ',
'overwroteimage'              => 'የ«[[$1]]» አዲስ ዕትም ላከ',
'uploaddisabled'              => 'ፋይል መላክ አይቻልም',
'uploaddisabledtext'          => 'ፋይል መላክ በ{{SITENAME}} አይቻልም።',
'uploadcorrupt'               => 'ይህ ፋይል ብልሹ ነው፤ ወይም ትክክለኛ ያልሆነ ቅጥያ አለው። እባክዎ ፋይሉን ተመልክተው እንደገና ይላኩት።',
'uploadvirus'                 => 'ፋይሉ ቫይረስ አለበት! ዝርዝር፦ $1',
'sourcefilename'              => 'የቆየው የፋይሉ ስም፦',
'destfilename'                => 'የፋይሉ አዲስ ስም፦',
'watchthisupload'             => 'ይህንን ገጽ ለመከታተል',
'filewasdeleted'              => 'በዚሁ ስም ያለው ፋይል ከዚህ በፊት ተልኮ እንደገና ጠፍቷል።  ዳግመኛ ሳይልኩት $1 ማመልከት ያሻላል።',
'upload-wasdeleted'           => "'''ማስጠንቀቂያ፦ ቀድሞ የተደለዘ ፋይል እየላኩ ነው።'''

ይህን ፋይል መላክ የሚገባ መሆኑን ይቆጠሩ። የፋይሉ ማጥፋት መዝገብ ከዚህ ታች ይታያል፦",
'filename-bad-prefix'         => 'የሚልኩት ፋይል ስም በ<strong>«$1»</strong> ይጀመራል፤ ይህ ብዙ ጊዜ በቁጥራዊ ካሜራ የተወሰነ ገላጭ ያልሆነ ስም ይሆናል። እባክዎ ለፋይልዎ ገላጭ የሆነ ስም ይምረጡ።',

'upload-proto-error'      => 'ትክክለኛ ያልሆነ ወግ (protocol)',
'upload-proto-error-text' => 'የሩቅ መላክ እንዲቻል URL በ<code>http://</code> ወይም በ<code>ftp://</code> መጀመር አለበት።',
'upload-file-error'       => 'የውስጥ ስህተት',
'upload-misc-error'       => 'ያልታወቀ የመላክ ስህተት',
'upload-misc-error-text'  => 'በተላከበት ጊዜ ያልታወቀ ስህተት ተነሣ። እባክዎ URL ትክክለኛና የሚገኝ መሆኑን አረጋግጠው እንደገና ይሞክሩ። ችግሩ ቢቀጠል፣ መጋቢን ይጠይቁ።',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'  => 'URLን መድረስ አልተቻለም',
'upload-curl-error28' => 'የመላክ ጊዜ አልቋል',

'license'            => 'የፈቃድ አይነት፦',
'nolicense'          => 'ምንም አልተመረጠም',
'license-nopreview'  => '(ቅድመ-ዕይታ አይገኝም)',
'upload_source_url'  => ' (ትክክለኛ፣ በግልጽ የሚገኝ URL)',
'upload_source_file' => ' (በኮምፒውተርዎ ላይ ያለበት ፋይል)',

# Special:ImageList
'imagelist_search_for'  => 'ለMedia ፋይል ስም ፍለጋ፦',
'imgfile'               => 'ፋይሉ',
'imagelist'             => 'የፋይል / ሥዕሎች ዝርዝር',
'imagelist_date'        => 'ቀን እ.ኤ.አ',
'imagelist_name'        => 'የፋይል ስም',
'imagelist_user'        => 'አቅራቢው',
'imagelist_size'        => 'መጠን (byte)',
'imagelist_description' => 'ማጠቃለያ',

# Image description page
'filehist'                  => 'የፋይሉ ታሪክ',
'filehist-help'             => 'የቀድሞው ዕትም ካለ ቀን/ሰዓቱን በመጫን መመልከት ይቻላል።',
'filehist-deleteall'        => 'ሁሉን ለማጥፋት',
'filehist-deleteone'        => 'ይህን ለማጥፋት',
'filehist-revert'           => 'ወዲህ ይገለበጥ',
'filehist-current'          => 'ያሁኑኑ',
'filehist-datetime'         => 'ቀን /ሰዓት',
'filehist-user'             => 'አቅራቢው',
'filehist-dimensions'       => 'ክልሉ (በpixel)',
'filehist-filesize'         => 'መጠን',
'filehist-comment'          => 'ማጠቃለያ',
'imagelinks'                => 'መያያዣዎች',
'linkstoimage'              => 'የሚከተሉ ገጾች ወደዚሁ ፋይል ተያይዘዋል።',
'nolinkstoimage'            => 'ወዲህ ፋይል የተያያዘ ገጽ የለም።',
'sharedupload'              => 'ይህ ፋይል ከጋራ ምንጭ (Commons) የተቀሰመ ነው። በማንኛውም ዊኪ ላይ ሊጠቅም ይቻላል።',
'shareduploadwiki'          => 'በተጨማሪ ለመረዳት $1 ይዩ።',
'shareduploadwiki-desc'     => 'በዚያ በ$1 የሚታየው መግለጫ እንዲህ ይላል፦',
'shareduploadwiki-linktext' => 'ፋይል መግለጫ ገጹ',
'noimage'                   => 'በዚህ ስም የሚታወቅ ፋይል የለም፤ እርስዎ ግን $1 ይችላሉ።',
'noimage-linktext'          => 'ሊልኩት',
'uploadnewversion-linktext' => 'ለዚሁ ፋይል አዲስ ዕትም ለመላክ',

# File reversion
'filerevert'                => '$1 ማገልበጥ',
'filerevert-legend'         => 'ፋይል ማገልበጥ',
'filerevert-comment'        => 'ማጠቃለያ፦',
'filerevert-defaultcomment' => 'በ$2፣ $1 ወደ ነበረው ዕትም መለሰው',
'filerevert-submit'         => 'ማገልበጥ',
'filerevert-success'        => "'''[[Media:$1|$1]]''' [በ$3፣ $2 ወደ ነበረው $4 እትም] ተመልሷል።",

# File deletion
'filedelete'                  => '$1 ለማጥፋት',
'filedelete-legend'           => 'ፋይልን ለማጥፋት',
'filedelete-intro'            => "'''[[Media:$1|$1]]''' ሊያጥፉ ነው።",
'filedelete-intro-old'        => "በ[$4 $3፣ $2] እ.ኤ.አ. የነበረው የ'''[[Media:$1|$1]]''' እትም ሊያጥፉ ነው።",
'filedelete-comment'          => 'የማጥፋቱ ምክንያት፦',
'filedelete-submit'           => 'ይጥፋ',
'filedelete-success'          => "'''$1''' ጠፍቷል።",
'filedelete-success-old'      => '<span class="plainlinks">በ$3፣ $2 የነበረው የ\'\'\'[[Media:$1|$1]]\'\'\' ዕትም ጠፍቷል።</span>',
'filedelete-nofile'           => "'''$1''' በ{{SITENAME}} የለም።",
'filedelete-otherreason'      => 'ሌላ / ተጨማሪ ምክንያት፦',
'filedelete-reason-otherlist' => 'ሌላ ምክንያት',
'filedelete-reason-dropdown'  => '*ተራ የማጥፋት ምክንያቶች
** የማብዛት ፈቃድ አለመኖር
** የተዳገመ ፋይል ቅጂ',
'filedelete-edit-reasonlist'  => "'ተራ የማጥፋት ምክንያቶች' ለማስተካከል",

# MIME search
'mimesearch' => 'የMIME ፍለጋ',
'mimetype'   => 'የMIME አይነት፦',
'download'   => 'አውርድ',

# Unwatched pages
'unwatchedpages' => 'ያልተከታተሉ ገጾች',

# List redirects
'listredirects' => 'መምሪያ መንገዶች ሁሉ',

# Unused templates
'unusedtemplates'     => 'ያልተለጠፉ መልጠፊያዎች',
'unusedtemplatestext' => 'እነኚህ መልጠፊያዎች አሁን ባንዳችም ገጽ ላይ አልተለጠፉም።',
'unusedtemplateswlh'  => 'ሌሎች መያያዣዎች',

# Random page
'randompage'         => 'ማናቸውንም ለማየት',
'randompage-nopages' => 'በዚህ ክፍለ-ዊኪ ምንም ገጽ የለም።',

# Random redirect
'randomredirect'         => 'ማናቸውም መምሪያ መንገድ',
'randomredirect-nopages' => 'በዚህ ክፍለ-ዊኪ ምንም መምሪያ መንገድ የለም።',

# Statistics
'statistics'             => 'የዚሁ ሥራ እቅድ ዝርዝር ቁጥሮች',
'sitestats'              => 'የዚህ {{SITENAME}} ዝርዝር ቁጥሮች (Statistics)',
'userstats'              => 'ያባላት ዝርዝር ቁጥሮች',
'sitestatstext'          => "በጠቅላላው '''$1''' ገጾች በዚህ ሥራ ዕቅድ አሉ። ይኸኛው ድምር ቁጥር የሚጠቅልለው ውይይት ገጾች፣ ልዩ ገጾች፣ አጫጭር ፅሑፎች፣ መምሪያ ገጾች፣ እንዲሁም ሌሎች ይዞታ የሌለባቸው ገጾች ሁሉ ይሆናል። ከነዚህ ውጭ '''$2''' ይዞታ ያላቸው ተገቢ ፅሑፎች ይኖራሉ። 

ይህ ዊኪፔድያ ከተመሰረተ ጀምሮ '''$4''' ለውጦች ተደርገዋል። ስለዚህ ባማካኝ '''$5''' ለውጦች በየገጹ ይሆናል።",
'userstatstext'          => "እስከ ዛሬ ድረስ '''$1''' አባላት ገብተዋል። ከዚህ ቁጥር መካከል፣ '''$2''' (ማለት '''$4%''') መጋቢዎች ናቸው። There are '''$1''' registered users, of whom '''$2''' (or '''$4%''') {{PLURAL:$2|has|have}} $5 rights.",
'statistics-mostpopular' => 'ከሁሉ የታዩት ገጾች',

'disambiguations'      => 'ወደ መንታ መንገድ የሚያያይዝ',
'disambiguationspage'  => 'Template:መንታ',
'disambiguations-text' => "የሚከተሉት ጽሑፎች ወደ '''መንታ መንገድ''' እየተያያዙ ነውና ብዙ ጊዜ እንዲህ ሳይሆን ወደሚገባው ርዕስ ቢወስዱ ይሻላል። <br />
መንታ መንገድ ማለት የመንታ መልጠፊያ ([[MediaWiki:Disambiguationspage]]) ሲኖርበት ነው።",

'doubleredirects'     => 'ድርብ መምሪያ መንገዶች',
'doubleredirectstext' => 'ይህ ድርብ መምሪያ መንገዶች ይዘርዘራል።

ድርብ መምሪያ መንገድ ካለ ወደ መጨረሻ መያያዣ እንዲሄድ ቢስተካከል ይሻላል።',

'brokenredirects'        => 'ሰባራ መምሪያ መንገዶች',
'brokenredirectstext'    => 'እነዚህ መምሪያ መንገዶች ወደማይኖር ጽሑፍ ይመራሉ።',
'brokenredirects-edit'   => '(ለማስተካከል)',
'brokenredirects-delete' => '(ለማጥፋት)',

'withoutinterwiki'         => 'በሌሎች ቋንቋዎች ያልተያያዙ',
'withoutinterwiki-summary' => 'እነዚህ ጽሑፎች «በሌሎች ቋንቋዎች» ሥር ወደሆኑት ሌሎች ትርጉሞች ገና አልተያያዙም።',
'withoutinterwiki-submit'  => 'ይታዩ',

'fewestrevisions' => 'ለውጦች ያነሱላቸው መጣጥፎች',

# Miscellaneous special pages
'nbytes'                  => '$1 byte',
'ncategories'             => '$1 {{PLURAL:$1|መደብ|መደቦች}}',
'nlinks'                  => '$1 መያያዣዎች',
'nmembers'                => '$1 {{PLURAL:$1|መጣጥፍ|መጣጥፎች}}',
'nrevisions'              => '$1 ለውጦች',
'nviews'                  => '$1 {{PLURAL:$1|ዕይታ|ዕይታዎች}}',
'specialpage-empty'       => '(ይህ ገጽ ባዶ ነው።)',
'lonelypages'             => 'ያልተያያዙ ፅሑፎች',
'lonelypagestext'         => 'የሚቀጥሉት ገጾች በ{{SITENAME}} ውስጥ ከሚገኙ ሌሎች ገጾች ጋር አልተያያዙም።',
'uncategorizedpages'      => 'ገና ያልተመደቡ ጽሑፎች',
'uncategorizedcategories' => 'ያልተመደቡ መደቦች (ንዑስ ያልሆኑ)',
'uncategorizedimages'     => 'ያልተመደቡ ፋይሎች',
'uncategorizedtemplates'  => 'ያልተመደቡ መልጠፊያዎች',
'unusedcategories'        => 'ባዶ መደቦች',
'unusedimages'            => 'ያልተያያዙ ፋይሎች',
'popularpages'            => 'የሚወደዱ ገጾች',
'wantedcategories'        => 'ቀይ መያያዣዎች የበዙላቸው መደቦች',
'wantedpages'             => 'ቀይ መያያዣዎች የበዙላቸው አርእስቶች',
'mostlinked'              => 'መያያዣዎች የበዙላቸው ገጾች',
'mostlinkedcategories'    => 'መያያዣዎች የበዙላቸው መደቦች',
'mostlinkedtemplates'     => 'መያያዣዎች የበዙላቸው መልጠፊያዎች',
'mostcategories'          => 'መደቦች የበዙላቸው መጣጥፎች',
'mostimages'              => 'መያያዣዎች የበዙላቸው ስዕሎች',
'mostrevisions'           => 'ለውጦች የበዙላቸው መጣጥፎች',
'prefixindex'             => 'ገጾች በፊደል ለመፈልግ',
'shortpages'              => 'ጽሁፎች ካጭሩ ተደርድረው',
'longpages'               => 'ጽሁፎች ከረጅሙ ተደርድረው',
'deadendpages'            => 'መያያዣ የሌለባቸው ፅሑፎች',
'deadendpagestext'        => 'የሚቀጥሉት ገጾች በ{{SITENAME}} ውስጥ ከሚገኙ ሌሎች ገጾች ጋር አያያይዙም።',
'protectedpages'          => 'የተቆለፉ ገጾች',
'protectedpagestext'      => 'የሚከተሉት ገጾች ከመዛወር ወይም ከመታረም ተቆልፈዋል።',
'protectedpagesempty'     => 'በዚያ ግቤት የሚቆለፍ ገጽ አሁን የለም።',
'protectedtitles'         => 'የተቆለፉ አርዕስቶች',
'protectedtitlestext'     => 'የሚከተሉት አርዕስቶች ከመፈጠር ተጠብቀዋል።',
'protectedtitlesempty'    => 'እንደዚህ አይነት አርእስት አሁን የሚቆለፍ ምንም የለም።',
'listusers'               => 'አባላት',
'newpages'                => 'አዳዲስ መጣጥፎች',
'newpages-username'       => 'በአቅራቢው፦',
'ancientpages'            => 'የቈዩ ፅሑፎች (በተለወጠበት ሰአት)',
'move'                    => 'ለማዛወር',
'movethispage'            => 'ይህንን ገጽ ለማዛወር',
'unusedimagestext'        => 'እነኚህ ፋይሎች ከ{{SITENAME}} አልተያያዙም። ሆኖም ሳያጥፏቸው ከ{{SITENAME}} ውጭ በቀጥታ ተያይዘው የሚገኙ ድረ-ገጾች መኖራቸው እንደሚቻል ይገንዝቡ።',
'unusedcategoriestext'    => 'እነዚህ መደብ ገጾች ባዶ ናቸው። ምንም ጽሑፍ ወይም ግንኙነት የለባቸውም።',
'notargettitle'           => 'ምንም ግብ የለም',
'notargettext'            => 'ይህ ተግባር የሚፈጽምበት ምንም ግብ (አላማ) ገጽ ወይም አባል አልወሰኑም።',
'pager-newer-n'           => '{{PLURAL:$1|ኋለኛ 1|ኋለኛ $1}}',
'pager-older-n'           => '{{PLURAL:$1|ፊተኛ 1|ፊተኛ $1}}',

# Book sources
'booksources'               => 'የመጻሕፍት ቤቶችና ሸጪዎች',
'booksources-search-legend' => 'የመጽሐፍ ቦታ ፍለጋ',
'booksources-isbn'          => 'የመጽሐፉ ISBN #:',
'booksources-go'            => 'ይሂድ',
'booksources-text'          => 'ከዚህ ታች ያሉት ውጭ መያያዦች መጻሕፍት ይሸጣሉ፤ ስለ ተፈለጉት መጻሕፍት ተጨማሪ መረጃ እዚያ እንደሚገኝ ይሆናል።',

# Special:Log
'specialloguserlabel'  => 'ብዕር ስም፡',
'speciallogtitlelabel' => 'አርዕስት፡',
'log'                  => 'Logs / መዝገቦች',
'all-logs-page'        => 'All logs - መዝገቦች ሁሉ',
'log-search-legend'    => 'ለመዝገቦች መፈለግ',
'log-search-submit'    => 'ሂድ',
'alllogstext'          => 'ይኸው መዝገብ ሁሉንም ያጠቅልላል። 1) የፋይሎች መዝገብ 2) የማጥፋት መዝገብ 3) የመቆለፍ መዝገብ 4) የማገድ መዝገብ 5) የመጋቢ አድራጎት መዝገቦች በያይነቱ ናቸው።

ከሳጥኑ የተወሰነ መዝገብ አይነት መምረጥ ይችላሉ። ከዚያ ጭምር በብዕር ስም ወይም በገጽ ስም መፈለግ ይቻላል።',
'logempty'             => '(በመዝገቡ ምንም የለም...)',
'log-title-wildcard'   => 'ከዚህ ፊደል ጀምሮ አርዕስቶችን ለመፈልግ',

# Special:AllPages
'allpages'          => 'ገጾች ሁሉ በሙሉ',
'alphaindexline'    => '$1 እስከ $2 ድረስ',
'nextpage'          => 'የሚቀጥለው ገጽ (ከ$1 ጀምሮ)',
'prevpage'          => 'ፊተኛው ገጽ (ከ$1 ጀምሮ)',
'allpagesfrom'      => 'ገጾች ከዚሁ ፊደል ጀምሮ ይታዩ፦',
'allarticles'       => 'የመጣጥፎች ማውጫ በሙሉ፣',
'allinnamespace'    => 'ገጾች ሁሉ (ክፍለ-ዊኪ፡$1)',
'allnotinnamespace' => 'ገጾች ሁሉ (በክፍለ-ዊኪ፡$1 ያልሆኑት)',
'allpagesprev'      => 'ቀድመኛ',
'allpagesnext'      => 'ቀጥሎ',
'allpagessubmit'    => 'ይታይ',
'allpagesprefix'    => 'በዚሁ ፊደል የጀመሩት ገጾች:',
'allpages-bad-ns'   => 'በ{{SITENAME}} «$1» የሚባል ክፍለዊኪ የለም።',

# Special:Categories
'categories'         => 'ምድቦች',
'categoriespagetext' => 'በዚሁ ሥራ ዕቅድ ውስጥ የሚከተሉ መደቦች ይኖራሉ።',

# Special:ListUsers
'listusersfrom'      => 'ከዚሁ ፊደል ጀምሮ፦',
'listusers-submit'   => 'ይታይ',
'listusers-noresult' => 'ማንም ተጠቃሚ አልተገኘም።',

# E-mail user
'mailnologin'     => 'ምንም መነሻ አድራሻ የለም',
'mailnologintext' => 'ኢ-ሜል ወደ ሌላ አባል ለመላክ [[Special:UserLogin|መግባት]]ና በ[[Special:Preferences|ምርጫዎችዎ]] ትክክለኛ የኢሜል አድራሻዎ መኖር ያስፈልጋል።',
'emailuser'       => 'ለዚህ/ች ሰው ኢሜል መላክ',
'emailpage'       => 'ወደዚህ/ች አባል ኢ-ሜል ለመላክ',
'emailpagetext'   => 'አባሉ በሳቸው «ምርጫዎች» ክፍል ተግባራዊ ኢ-ሜል አድራሻ ያስገቡ እንደሆነ፣ ከታች ያለው ማመልከቻ አንድን ደብዳቤ በቀጥታ ይልካቸዋል።

ተቀባዩም መልስ በቀጥታ ሊሰጡዎ እንዲችሉ፣ በእርስዎ «ምርጫዎች» ክፍል ያስገቡት ኢ-ሜል አድራሻ በደብዳቤዎ «From:» መስመር ይታይላቸዋል።',
'defemailsubject' => '{{SITENAME}} Email / ኢ-ሜል',
'noemailtitle'    => 'ኢ-ሜል አይቻልም',
'noemailtext'     => 'ለዚህ/ች አባል ኢ-ሜል መላክ አይቻልም። ወይም ተገቢ ኢ-ሜል አድራሻ የለንም፣ ወይም ከሰው ምንም ኢ-ሜል መቀበል አልወደደ/ችም።',
'emailfrom'       => 'ከ',
'emailto'         => 'ለ',
'emailsubject'    => 'ርዕሰ ጉዳይ',
'emailmessage'    => 'መልእክት',
'emailsend'       => 'ይላክ',
'emailccme'       => 'አንድ ቅጂ ደግሞ ለራስዎ ኢ-ሜል ይላክ።',
'emailccsubject'  => 'ወደ $1 የመልዕክትዎ ቅጂ፦ $2',
'emailsent'       => 'ኢ-ሜል ተልኳል።',
'emailsenttext'   => 'ኢ-ሜል መልዕክትዎ ተልኳል።',

# Watchlist
'watchlist'            => 'የምከታተላቸው ገጾች፤',
'mywatchlist'          => 'የምከታተላቸው ገጾች፤',
'watchlistfor'         => "(ለ'''$1''')",
'nowatchlist'          => 'ዝርዝርዎ ባዶ ነው። ምንም ገጽ ገና አልተጨመረም።',
'watchlistanontext'    => 'የሚከታተሉት ገጾች ዝርዝርዎን ለመመልከት ወይም ለማስተካከል እባክዎ $1።',
'watchnologin'         => 'ገና አልገቡም',
'watchnologintext'     => 'የሚከታተሏቸውን ገጾች ዝርዝር ለመቀየር [[Special:UserLogin|መግባት]] ይኖርብዎታል።',
'addedwatch'           => 'ወደሚከታተሉት ገጾች ተጨመረ',
'addedwatchtext'       => "ገጹ «$1» [[Special:Watchlist|ለሚከታተሉት ገጾች]] ተጨምሯል። ወደፊት ይህ ገጽ ወይም የውይይቱ ገጽ ሲቀየር፣ በዚያ ዝርዝር ላይ ይታያል። በተጨማሪም [[Special:RecentChanges|«በቅርብ ጊዜ በተለወጡ» ገጾች]] ዝርዝር፣ በቀላሉ እንዲታይ በ'''ጨለማ ጽህፈት''' ተጽፎ ይገኛል።

በኋላ ጊዜ ገጹን ከሚከታተሉት ገጾች ለማስወግድ የፈለጉ እንደሆነ፣ በጫፉ ዳርቻ «አለመከታተል» የሚለውን ይጫኑ።",
'removedwatch'         => 'ከሚከታተሉት ገጾች ተወገደ',
'removedwatchtext'     => '«<nowiki>$1</nowiki>» የሚለው ከሚከታተሉት ገጾች ዝርዝር ጠፍቷል።',
'watch'                => 'ለመከታተል',
'watchthispage'        => 'ይህንን ገጽ ለመከታተል',
'unwatch'              => 'አለመከታተል',
'unwatchthispage'      => 'መከታተል ይቅር',
'notanarticle'         => 'መጣጥፍ አይደለም',
'notvisiblerev'        => 'ዕትሙ ጠፍቷል',
'watchnochange'        => 'ከተካከሉት ገጾች አንዳችም በተወሰነው ጊዜ ውስጥ አልተለወጠም።',
'watchlist-details'    => 'አሁን በሙሉ {{PLURAL:$1|$1 ገጽ|$1 ገጾች}} እየተከታተሉ ነው።',
'wlheader-enotif'      => '* የ-ኢሜል ማስታወቂያ እንዲሠራ ተደርጓል።',
'wlheader-showupdated' => "* መጨረሻ ከጎበኟቸው ጀምሮ የተቀየሩት ገጾች በ'''ጨለማ ጽሕፈት''' ይታያሉ",
'watchmethod-recent'   => 'የቅርብ ለውጦችን ለሚከታተሉት ገጾች በመፈለግ',
'watchmethod-list'     => 'የሚከታተሉትን ገጾች ለቅርብ ለውጦች በመፈለግ',
'watchlistcontains'    => 'አሁን በሙሉ $1 ገጾች እየተከታተሉ ነው።',
'wlnote'               => 'ባለፉት <b>$2</b> ሰዓቶች የተደረጉት $1 መጨረሻ ለውጦች እታች ይገኛሉ።',
'wlshowlast'           => 'ያለፉት $1 ሰዓት፤ $2 ቀን፤ $3 ይታዩ።',
'watchlist-show-bots'  => 'የቦት (BOT) ለውጦች ይታዩ',
'watchlist-hide-bots'  => 'የቦት (BOT) ለውጦች ይደበቁ',
'watchlist-show-own'   => 'የራሴ ለውጦች ይታዩ',
'watchlist-hide-own'   => 'የራሴ ለውጦች ይደበቁ',
'watchlist-show-minor' => "'ጥ' (ጥቃቅን) ለውጦች ይታዩ",
'watchlist-hide-minor' => "'ጥ' (ጥቃቅን) ለውጦች ይደበቁ",

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'እየተጨመረ ነው...',
'unwatching' => 'እየተወገደ ነው...',

'enotif_mailer'                => 'የ{{SITENAME}} ኢሜል-ማስታወቂያ',
'enotif_reset'                 => 'ገጾች ሁሉ የተጎበኙ ሆነው ለማመልከት',
'enotif_newpagetext'           => 'ይህ አዲስ ገጽ ነው።',
'enotif_impersonal_salutation' => '{{SITENAME}} ተጠቃሚ',
'changed'                      => 'ተለወጠ',
'created'                      => 'ተፈጠረ',
'enotif_subject'               => 'የ{{SITENAME}} ገጽ $PAGETITLE  በ$PAGEEDITOR $CHANGEDORCREATED',
'enotif_lastvisited'           => 'መጨረሻ ከጎበኙ ጀምሮ ለውጦችን ሁሉ ለመመልከት $1 ይዩ።',
'enotif_lastdiff'              => 'ይህን ለውጥ ለማመልከት $1 ይዩ።',
'enotif_anon_editor'           => 'ቁጥር አድራሻ $1',
'enotif_body'                  => 'ለ$WATCHINGUSERNAME ይድረስ፣


የ{{SITENAME}} ገጽ $PAGETITLE በ$PAGEEDITDATE በ$PAGEEDITOR $CHANGEDORCREATED፤ ለአሁኑኑ እትም $PAGETITLE_URL ይዩ።

$NEWPAGE

የአዛጋጁ ማጠቃለያ፦ $PAGESUMMARY $PAGEMINOREDIT

አዛጋጁን ለማገናኘት፦
በኢ-ሜል፦ $PAGEEDITOR_EMAIL
በዊኪ፦ $PAGEEDITOR_WIKI

ገጹን ካልጎበኙ በቀር ምንም ሌላ ኢሜል-ማስታወቂያ አይሰጥም። ደግሞ በተከታተሉት ገጾች ዝርዝር ላለው ገጽ ሁሉ የኢሜል-ማስታወቂያውን ሁኔታ ማስተካከል ይችላሉ።

             ከክብር ጋር፣ የ{{SITENAME}} ኢሜል-ማስታወቂያ መርሃግብር።

--
የሚከታተሉት ገጾች ዝርዝር ለመቀየር፣ {{fullurl:{{ns:special}}:Watchlist/edit}} ይጎበኙ።

በተጨማሪ ለመረዳት፦
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'ገጹ ይጥፋ',
'confirm'                     => 'ማረጋገጫ',
'excontent'                   => 'ይዞታ፦ «$1» አለ።',
'excontentauthor'             => "ይዞታ '$1' አለ (የጻፈበትም '$2' ብቻ ነበር)",
'exbeforeblank'               => 'ባዶ፤ ከተደመሰሰ በፊት ይዞታው «$1» አለ።',
'exblank'                     => 'ገጹ ባዶ ነበረ።',
'delete-confirm'              => '«$1» ለማጥፋት',
'delete-legend'               => 'ለማጥፋት',
'historywarning'              => 'ማስጠንቀቂያ፦ ለዚሁ ገጽ የዕትም ታሪክ ደግሞ ሊጠፋ ነው! :',
'confirmdeletetext'           => 'አንድ ገጽ ወይም ስዕል ከነለውጦቹ በሙሉ ከዚሁ {{SITENAME}} ሊጠፋ ነው! ይህን ማድረግዎ ያሠቡበት መሆኑንና ማጥፋቱ በፖሊሲ ተገቢ እንደሆነ እባክዎ ያረጋግጡ፦',
'actioncomplete'              => 'ተፈጽሟል',
'deletedtext'                 => '«<nowiki>$1</nowiki>» ጠፍቷል።

(የጠፉትን ገጾች ሁሉ ለመመልከት $2 ይዩ።)',
'deletedarticle'              => '«[[$1]]» አጠፋ',
'dellogpage'                  => 'የማጥፋት መዝገብ',
'dellogpagetext'              => 'በቅርቡ የጠፉት ገጾች ከዚህ ታች የዘረዝራሉ።',
'deletionlog'                 => 'የማጥፋት መዝገብ',
'reverted'                    => 'ወደ ቀድመኛ ዕትም ገለበጠው።',
'deletecomment'               => 'የማጥፋቱ ምክንያት፦',
'deleteotherreason'           => 'ሌላ /ተጨማሪ ምክንያት',
'deletereasonotherlist'       => 'ሌላ ምክንያት',
'deletereason-dropdown'       => '*ተራ የማጥፋት ምክንያቶች
** በአቅራቢው ጥያቄ
** ማብዛቱ ያልተፈቀደለት ጽሑፍ
** ተንኮል',
'delete-edit-reasonlist'      => "'ተራ የማጥፋት ምክንያቶች' ለማዘጋጀት",
'rollback'                    => 'ለውጦቹ ይገልበጡ',
'rollback_short'              => 'ይመለስ',
'rollbacklink'                => 'ROLLBACK ይመለስ',
'rollbackfailed'              => 'መገልበጡ አልተከናወነም',
'cantrollback'                => 'ለውጡን መገልበጥ አይቻልም፦ አቅራቢው ብቻ ስላዘጋጁት ነው።',
'alreadyrolled'               => 'የ[[:$1]] መጨረሻ ለውጥ በ[[User:$2|$2]] ([[User talk:$2|ውይይት]]) መገልበት አይቻልም፤ ሌላ ሰው አሁን ገጹን መልሶታል።

መጨረሻው ለውጥ በ[[User:$3|$3]] ([[User talk:$3|ውይይት]]) ነበረ።',
'editcomment'                 => 'ማጠቃለያው፦ «<i>$1</i>» ነበረ።', # only shown if there is an edit comment
'revertpage'                  => 'የ$2ን ለውጦች ወደ $1 እትም መለሰ።', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'የ$1 ለውጦች ተገለበጡ፣ ወደ $2 ዕትም ተመልሷል።',
'protectlogpage'              => 'የማቆለፍ መዝገብ',
'protectlogtext'              => 'ይህ መዝገብ ገጽ ሲቆለፍ ወይም ሲከፈት ይዘረዝራል። ለአሁኑ የተቆለፈውን ለመመልከት፣ [[Special:ProtectedPages|የቆለፉትን ገጾች]] ደግሞ ያዩ።',
'protectedarticle'            => 'ገጹን «[[$1]]» ቆለፈው።',
'modifiedarticleprotection'   => 'የመቆለፍ ደረጃ ለ«[[$1]]» ቀየረ።',
'unprotectedarticle'          => 'ገጹን «[[$1]]» ፈታ።',
'protect-title'               => 'ለ«$1» የመቆለፍ ደረጃ ለማስተካከል',
'protect-legend'              => 'የመቆለፍ ማረጋገጫ',
'protectcomment'              => 'ማጠቃለያ፦',
'protectexpiry'               => 'የሚያልቅበት ግዜ፦',
'protect_expiry_invalid'      => "የተሰጠው 'የሚያልቅበት ጊዜ' ልክ አይደለም።",
'protect_expiry_old'          => "የተሰጠው 'የሚያልቅበት ጊዜ' ባለፈው ግዜ ነበር።",
'protect-unchain'             => 'ገጹን የማዛወር ፈቃዶች ለመፍታት',
'protect-text'                => 'እዚህ ለገጹ «<strong><nowiki>$1</nowiki></strong>» የመቆለፍ ደረጃ መመልከት ወይም መቀይር ይችላሉ።',
'protect-locked-blocked'      => 'ማገጃ እያለብዎት የመቆለፍ ደረጃ ለመቀየር አይችሉም። ለገጹ <strong>$1</strong> የአሁኑኑ ደረጃ እንዲህ ነው፦',
'protect-locked-dblock'       => 'መረጃ-ቤቱ እራሱ አሁን ስለሚቆለፍ፣ የገጽ መቆለፍ ደረጃ ሊቀየር አይችልም። ለገጹ <strong>$1</strong> የአሁኑኑ ደረጃ እንዲህ ነው፦',
'protect-locked-access'       => 'እርስዎ ገጽ የመቆለፍ ወይም የመፍታት ፈቃድ የለዎም።<br />አሁኑ የዚሁ ገጽ መቆለፍ ደረጃ እንዲህ ነው፦ <strong>$1</strong>:',
'protect-cascadeon'           => 'ይህ ገጽ ወደ ተከለከሉት አርእስቶች ተጨምሯል። የመቆለፍ ደረጃ እዚህ መቀየር ቢቻልዎም ገጹ ግን በሚከተለው ድርብ የተቆለፈ ገጽ ውስጥ ይጨመራል።',
'protect-default'             => '(እንደ ወትሮ)',
'protect-fallback'            => 'የ$1 ፈቃደ ለማስፈልግ',
'protect-level-autoconfirmed' => 'ባልገቡትና በአዲስ አባላት ብቻ',
'protect-level-sysop'         => 'መጋቢዎች ብቻ',
'protect-summary-cascade'     => 'በውስጡም ያለውን የሚያቆልፍ አይነት',
'protect-expiring'            => 'በ$1 (UTC) ያልቃል',
'protect-cascade'             => 'በዚህ ገጽ ውስጥ የተካተተው ገጽ ሁሉ ደግሞ ይቆለፍ?',
'protect-cantedit'            => 'ይህንን ገጽ የማዘጋጀት ፈቃድ ስለሌለልዎ መቆለፍ አይቻሎትም።',
'restriction-type'            => 'ፈቃድ፦',
'restriction-level'           => 'የመቆለፍ ደረጃ፦',
'minimum-size'                => 'ቢያንስ',
'maximum-size'                => 'ቢበዛ፦',
'pagesize'                    => 'byte መጠን ያለው ሁሉ',

# Restrictions (nouns)
'restriction-edit' => 'እንዲዘጋጅ፦',
'restriction-move' => 'እንዲዛወር፦',

# Restriction levels
'restriction-level-sysop'         => 'በሙሉ ተቆልፎ',
'restriction-level-autoconfirmed' => 'በከፊል ተቆልፎ',
'restriction-level-all'           => 'ማንኛውም ደረጃ',

# Undelete
'undelete'                   => 'የተደለዘ ገጽ ለመመለስ',
'undeletepage'               => 'የተደለዘ ገጽ ለመመለስ',
'viewdeletedpage'            => 'የተደለዙ ገጾች ለማየት',
'undeletepagetext'           => 'እነዚህ ገጾች ተደለዙ፣ እስካሁን ግን በመዝገቡ ውስጥ ይገኛሉና ሊመለሱ ይቻላል። ሆኖም መዝገቡ አንዳንዴ ሊደመስስ ይቻላል።',
'undeleteextrahelp'          => "እትሞቹን በሙሉ ለመመልስ፣ ሳጥኖቹ ሁሉ ባዶ ሆነው ይቆዩና 'ይመለስ' የሚለውን ይጫኑ። <br />አንዳንድ እትም ብቻ ለመመልስ፣ የተፈለገውን እትሞች በየሳጥኖቹ አመልክተው 'ይመለስ' ይጫኑ። <br />'ባዶ ይደረግ' ቢጫን፣ ማጠቃልያውና ሳጥኖቹ ሁሉ እንደገና ባዶ ይሆናሉ።",
'undeleterevisions'          => 'በመዝገቡ $1 {{PLURAL:$1|ዕትም አለ|ዕትሞች አሉ}}',
'undeletehistory'            => 'የተደለዘ ገጽ ሲመለስ፣ የተመለከቱት ዕትሞች ሁሉ ወደ ዕትሞች ታሪክ ደግሞ ይመልሳሉ። ገጹ ከጠፋ በኋላ በዚያው አርዕሥት ሌላ ገጽ ቢኖር፣ የተመለሱት ዕትሞች ወደ ዕትሞች ታሪክ አንድላይ ይጨመራሉ።',
'undeletehistorynoadmin'     => 'ይህ ገጽ ጠፍቷል። የመጥፋቱ ምክንያት ከዚህ በታች ይታያል። ደግሞ ከጠፋ በፊት ያዘጋጁት ተጠቃሚዎች ይዘረዘራሉ። የተደለዙት ዕትሞች ጽሕፈት ለመጋቢዎች ብቻ ሊታይ ይችላል።',
'undelete-revision'          => 'የ$1 የተደለዘ ዕትም በ$2 በ$3፦',
'undelete-nodiff'            => 'ቀድመኛ ዕትም አልተገኘም።',
'undeletebtn'                => 'ይመለስ',
'undeletelink'               => 'ይመለስ',
'undeletereset'              => 'ባዶ ይደረግ',
'undeletecomment'            => 'ማጠቃልያ፦',
'undeletedarticle'           => '«[[$1]]»ን መለሰ',
'undeletedrevisions'         => '{{PLURAL:$1|1 ዕትም|$1 ዕትሞች}} መለሰ',
'undeletedrevisions-files'   => '{{PLURAL:$1|1 ዕትም|$1 ዕትሞች}} እና {{PLURAL:$2|1 ፋይል|$2 ፋይሎች}} መለሰ',
'undeletedfiles'             => '{{PLURAL:$1|1 ፋይል|$1 ፋይሎች}} መለሰ',
'cannotundelete'             => 'መመለሱ አልተከናወነም፤ ምናልባት ሌላ ሰው ገጹን አስቀድሞ መልሶታል።',
'undeletedpage'              => "<big>'''$1 ተመልሷል'''</big>

በቅርብ የጠፉና የተመለሱ ገጾች ለማመልከት [[Special:Log/delete|የማጥፋቱን መዝገብ]] ይዩ።",
'undelete-header'            => 'በቅርብ ግዜ የተደለዙትን ገጾች ለማመልከት [[Special:Log/delete|የማጥፋቱን መዝገብ]] ይዩ።',
'undelete-search-box'        => 'የተደለዙትን ገጾች ለመፈልግ',
'undelete-search-prefix'     => 'ከዚሁ ፊደል ጀምሮ፦',
'undelete-search-submit'     => 'ይታይ',
'undelete-no-results'        => 'በመዝገቡ ምንም ተመሳሳይ ገጽ አልተገኘም።',
'undelete-filename-mismatch' => 'በጊዜ ማህተም $1 ያለው እትም መመልስ አልተቻለም፤ የፋይል ስም አለመስማማት',
'undelete-error-short'       => 'ፋይል የመመለስ ስኅተት፦ $1',
'undelete-error-long'        => 'ፋይሉ በመመለስ ስኅተቶች ተነሡ፦

$1',

# Namespace form on various pages
'namespace'      => 'ዓይነት፦',
'invert'         => '(ምርጫውን ለመገልበጥ)',
'blanknamespace' => 'መጣጥፎች',

# Contributions
'contributions' => 'ያባል አስተዋጽኦች',
'mycontris'     => 'የኔ አስተዋጽኦች፤',
'contribsub2'   => 'ለ $1 ($2)',
'nocontribs'    => 'ምንም አልተገኘም።',
'uctop'         => '(ላይኛ)',
'month'         => 'እስከዚህ ወር ድረስ፦',
'year'          => 'እስከዚህ አመት (እ.ኤ.አ.) ድረስ፡-',

'sp-contributions-newbies'     => 'የአዳዲስ ተጠቃሚዎች አስተዋጽዖ ብቻ እዚህ ይታይ',
'sp-contributions-newbies-sub' => '(ለአዳዲስ ተጠቃሚዎች)',
'sp-contributions-blocklog'    => 'የማገጃ መዝገብ',
'sp-contributions-search'      => 'የሰውን አስተዋጽኦች ለመፈለግ፦',
'sp-contributions-username'    => 'ብዕር ስም ወይም የቁ. አድራሻ፦',
'sp-contributions-submit'      => 'ፍለጋ',

# What links here
'whatlinkshere'       => 'ወዲህ የሚያያዝ',
'whatlinkshere-title' => 'ወደ «$1» የሚያያዙት ገጾች',
'whatlinkshere-page'  => 'ለገጽ (አርዕስት)፦',
'linklistsub'         => '(ወዲህ የሚያያዝ)',
'linkshere'           => "የሚከተሉት ገጾች ወደ '''[[:$1]]''' ተያይዘዋል።",
'nolinkshere'         => "ወደ '''[[:$1]]''' የተያያዘ ገጽ የለም።",
'nolinkshere-ns'      => "ባመለከቱት ክፍለ-ዊኪ ወደ '''[[:$1]]''' የተያያዘ ገጽ የለም።",
'isredirect'          => 'መምሪያ መንገድ',
'istemplate'          => 'የተሰካ',
'whatlinkshere-prev'  => 'ፊተኛ $1',
'whatlinkshere-next'  => 'ቀጥሎ $1',
'whatlinkshere-links' => '← ወዲህም የሚያያዝ',

# Block/unblock
'blockip'                     => 'ተጠቃሚውን ለማገድ',
'blockip-legend'              => 'ተጠቃሚ ለማገድ',
'blockiptext'                 => 'ከዚህ ታች ያለው ማመልከቻ በአንድ ቁጥር አድርሻ ወይም ብዕር ስም ላይ ማገጃ (ማዕቀብ) ለመጣል ይጠቀማል።  ይህ በ[[{{MediaWiki:Policy-url}}|መርመርያዎቻችን]] መሠረት ተንኮል ወይም ጉዳት ለመከልከል ብቻ እንዲደረግ ይገባል። ከዚህ ታች የተለየ ምክንያት (ለምሣሌ የተጎዳው ገጽ በማጠቆም) ይጻፉ።',
'ipaddress'                   => 'የቁ. አድራሻ፦',
'ipadressorusername'          => 'የቁ. አድራሻ ወይም የብዕር ስም፦',
'ipbexpiry'                   => 'የሚያልቅበት፦',
'ipbreason'                   => 'ምክንያቱ፦',
'ipbreasonotherlist'          => 'ሌላ ምክንያት',
'ipbreason-dropdown'          => "*ተራ የማገጃ ምክንያቶች
** የሀሠት መረጃ መጨምር
** ከገጾች ይዞታውን መደምሰስ
** የ'ስፓም' ማያያዣ ማብዛት
** እንቶ ፈንቶ መጨምር
** ዛቻ ማብዛት
** በአድራሻዎች ብዛት መተንኮል
** የማይገባ ብዕር ስም",
'ipbanononly'                 => 'በቁ.# የሚታወቅ ተጠቃሚ ብቻ ለመከልከል',
'ipbcreateaccount'            => 'ብዕር ስም እንዳያውጣ ለመከልከል',
'ipbemailban'                 => 'ተጠቃሚው ኢ-ሜል ከመላክ ይከለከል',
'ipbenableautoblock'          => 'በተጠቃሚው መጨረሻ ቁ.# እና ካሁን ወዲያ በሚጠቀመው አድራሻ ላይ ማገጃ ይጣል።',
'ipbsubmit'                   => 'ማገጃ ለመጣል',
'ipbother'                    => 'ሌላ የተወሰነ ግዜ፦',
'ipboptions'                  => '2 ሰዓቶች:2 hours,1 ቀን:1 day,3 ቀን:3 days,1 ሳምንት:1 week,2 ሳምንት:2 weeks,1 ወር:1 month,3 ወር:3 months,6 ወር:6 months,1 አመት:1 year,ዘላለም:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'ሌላ',
'ipbotherreason'              => 'ሌላ/ተጨማሪ ምክንያት፦',
'badipaddress'                => 'የማይሆን የቁ. አድራሻ',
'blockipsuccesssub'           => 'ማገጃ ተከናወነ',
'blockipsuccesstext'          => '[[Special:Contributions/$1|$1]] ታግዷል።<br />
ማገጃዎች ለማመልከት [[Special:IPBlockList|የማገጃ ዝርዝሩን]] ይዩ።',
'ipb-edit-dropdown'           => "'ተራ የማገጃ ምክንያቶች' ለማስተካከል",
'ipb-unblock-addr'            => 'ከ$1 መገጃ ለማንሣት',
'ipb-unblock'                 => 'ከብዕር ስም ወይም ከቁ. አድራሻ ማገጃ ለማንሣት',
'ipb-blocklist-addr'          => 'በ$1 ላይ አሁን ያለውን ማገጃ ለመመልከት',
'ipb-blocklist'               => 'አሁን ያሉትን ማገጃዎች ለመመልከት',
'unblockip'                   => 'ከተጠቃሚ ማገጃ ለማንሣት',
'unblockiptext'               => 'በዚህ ማመልከቻ ከታገደ ተጠቃሚ ማገጃውን ለማንሣት ይቻላል።',
'ipusubmit'                   => 'ማገጃውን ለማንሣት',
'unblocked'                   => 'ማገጃ ከ[[User:$1|$1]] ተነሣ',
'unblocked-id'                => 'ማገጃ $1 ተነሣ',
'ipblocklist'                 => 'የአሁኑ ማገጃዎች ዝርዝር',
'ipblocklist-legend'          => 'አንድ የታገደውን ተጠቃሚ ለመፈለግ፦',
'ipblocklist-username'        => 'ይህ ብዕር ስም ወይም የቁጥር አድራሻ #፡',
'ipblocklist-submit'          => 'ይፈለግ',
'blocklistline'               => '$1 (እ.ኤ.አ.)፦ $2 በ$3 ላይ ማገጃ ጣለ ($4)',
'infiniteblock'               => 'መቸም ይማያልቅ',
'expiringblock'               => 'በ$1 እ.ኤ.አ. ያልቃል',
'anononlyblock'               => 'ያልገቡት የቁ.# ብቻ',
'noautoblockblock'            => 'የቀጥታ ማገጃ እንዳይሠራ ተደረገ',
'createaccountblock'          => 'ስም ከማውጣት ተከለከለ',
'emailblock'                  => 'ኢ-ሜል ታገደ',
'ipblocklist-empty'           => 'የማገጃ ዝርዝር ባዶ ነው።',
'ipblocklist-no-results'      => 'የተጠየቀው ተጠቃሚ አሁን የታገደ አይደለም።',
'blocklink'                   => 'ማገጃ',
'unblocklink'                 => 'ማገጃ ለማንሣት',
'contribslink'                => 'አስተዋጽኦች',
'blocklogpage'                => 'የማገጃ መዝገብ',
'blocklogentry'               => 'እስከ $2 ድረስ [[$1]] አገዳ $3',
'blocklogtext'                => 'ይህ መዝገብ ተጠቃሚዎች መቸም ሲታገዱ ወይም ማገጃ ሲነሣ የሚዘረዝር ነው። ለአሁኑ የታገዱት ሰዎች [[Special:IPBlockList|በአሁኑ ማገጃዎች ዝርዝር]] ይታያሉ።',
'unblocklogentry'             => 'የ$1 ማገጃ አነሣ',
'block-log-flags-anononly'    => 'ያልገቡት የቁ. አድራሻዎች ብቻ',
'block-log-flags-nocreate'    => 'አዲስ ብዕር ስም ከማውጣት ተከለከለ',
'block-log-flags-noautoblock' => 'የቀጥታ ማገጃ እንዳይሠራ ተደረገ',
'block-log-flags-noemail'     => 'ኢ-ሜል ታገደ',
'ipb_expiry_invalid'          => 'የሚያልቅበት ግዜ አይሆንም።',
'ipb_already_blocked'         => '«$1» ገና ከዚህ በፊት ታግዶ ነው።',
'proxyblocker-disabled'       => 'ይህ ተግባር እንደማይሠራ ተደርጓል።',
'proxyblocksuccess'           => 'ተደርጓል።',

# Developer tools
'lockdb'              => 'መረጃ-ቤት ለመቆለፍ',
'unlockdb'            => 'መረጃ-ቤት ለመፍታት',
'lockconfirm'         => 'አዎ፣ መረጃ-ቤቱን ለማቆለፍ በውኑ እፈልጋለሁ።',
'unlockconfirm'       => 'አዎ፣ መረጃ-ቤቱን ለመፍታት በውኑ እፈልጋለሁ።',
'lockbtn'             => 'መረጃ-ቤቱ ይቆለፍ',
'unlockbtn'           => 'መረጃ-ቤቱ ይፈታ',
'locknoconfirm'       => 'በማረጋገጫ ሳትኑ ውስጥ ምልክት አላደረጉም።',
'lockdbsuccesssub'    => 'የመረጃ-ቤት መቆለፍ ተከናወነ',
'unlockdbsuccesssub'  => 'የመረጃ-ቤት መቆለፍ ተጨረሰ',
'lockdbsuccesstext'   => 'መረጃ-ቤቱ ተቆልፏል።<br />
ሥራዎን እንደጨረሱ [[Special:UnlockDB|መቆለፉን ለመፍታት]] እንዳይረሱ።',
'unlockdbsuccesstext' => 'መረጃ-ቤቱ ተፈታ።',
'databasenotlocked'   => 'መረጃ-ቤቱ የተቆለፈ አይደለም።',

# Move page
'move-page'               => '«$1»ን ለማዛወር',
'move-page-legend'        => 'የሚዛወር ገጽ',
'movepagetext'            => "ከታች የሚገኘው ማመልከቻ ለገጹ ይዞታ አዲስ አርእስት ያወጣል።
ከይዞታው ጋራ የእትሞች ታሪክ ደግሞ ወደ አዲሱ ገጽ ይዛወራል።
የቆየው አርእስት እንደ መምሪያ መንገድ ለአዲሱ ገጽ ይሆናል።
ይህ ማለት ወደዚያ የሚያያዝ መያያዣ ሁሉ በቀጥታ ወደ አዲሱ ሥፍራ ይወስዳል።
ነገር ግን ገጹን እርስዎ ካዛወሩ፣ መያያዣዎቹ ድርብ ወይም ሰባራ እንዳይሆኑ ለማረጋገጥ ኃላፊነትዎ ነው።

ባዲሱ አርእስት ሥፍራ ሌላ ገጽ ቀድሞ ካለ፤ ሌላው ገጽ ታሪክ የሌለው፣ ባዶ ወይም መምሪያ መንገድ ካልሆነ በስተቀር፣
ይህ ገጽ ወደዚያ ለማዛወር '''የማይቻል''' ነው።  ስለዚህ ስሕተት ካደረጉ ወደ ቆየው አርእስት ገጹን መመለስ ይችላሉ፤ የኖረውን ገጽ በስሕተት ለመደምሰስ አይቻልም ማለት ነው።

'''ማስጠንቀቂያ፦'''
በጣም ለተወደደ ወይም ብዙ ጊዜ ለሚነበብ ገጽ፣ እንዲህ ያለ ለውጥ በፍጹም ያልተጠበቀ ወይም ከባድ ውጤት ያለው ሊሆን ይችላል።  ስለዚህ እባክዎ የሚገባ መደምደሚያ መሆኑን አስቀድመው ያረጋግጡ።",
'movepagetalktext'        => "አብዛኛው ጊዜ፣ ከዚሁ ገጽ ጋራ የሚገናኘው የውይይት ገጽ አንድላይ ይዛወራል፤ '''ነገር ግን፦'''

* ገጹን ወደማይመሳስል ክፍለ-ዊኪ (ለምሳሌ Mediawiki:) ቢያዛውሩት፤
* ባዶ ያልሆነ ውይይት ገጽ ቅድሞ ቢገኝ፤ ወይም
* እታች ከሚገኘውን ሳጥን ምልክቱን ካጠፉ፤
:
:ከነውይይቱ ገጽ አንድላይ አይዛወሩም። የዚያን ጊዜ የውይይቱን ገጽ ለማዛወር ከወደዱ በእጅ ማድረግ ያስፈልግዎታል።",
'movearticle'             => 'የቆየ አርእስት፡',
'movenotallowed'          => 'በ{{SITENAME}} ላይ ገጾችን ለማዛወር ፈቃድ የለዎም።',
'newtitle'                => 'አዲሱ አርእስት',
'move-watch'              => 'ይህ ገጽ በተከታተሉት ገጾች ይጨመር',
'movepagebtn'             => 'ገጹ ይዛወር',
'pagemovedsub'            => 'መዛወሩ ተከናወነ',
'movepage-moved'          => "<big>'''«$1» ወደ «$2» ተዛውሯል'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'በዚያ አርዕሥት ሌላ ገጽ አሁን አለ። አለበለዚያ የመረጡት ስም ልክ አይደለም - ሌላ አርእስት ይምረጡ።',
'cantmove-titleprotected' => 'አዲሱ አርዕስት ከመፈጠር ስለተጠበቀ፣ ገጽ ወደዚያው ሥፍራ ለማዛወር አይችሉም።',
'talkexists'              => "'''ገጹ ወደ አዲሱ አርዕስት ተዛወረ፤ እንጂ በአዲሱ አርዕስት የቆየ ውይይት ገጽ አስቀድሞ ስለ ኖረ የዚህ ውይይት ገጽ ሊዛወር አልተቻለም። እባክዎ፣ በእጅ ያጋጥሙአቸው።'''",
'movedto'                 => 'የተዛወረ ወደ',
'movetalk'                => 'ከተቻለ፣ ከነውይይቱ ገጽ ጋራ ይዛወር',
'1movedto2'               => '«$1» ወደ «[[$2]]» አዛወረ',
'1movedto2_redir'         => '«$1» ወደ «[[$2]]» አዛወረ -- በመምሪያ መንገድ ፈንታ',
'movelogpage'             => 'የማዛወር መዝገብ',
'movelogpagetext'         => 'ይህ መዝገብ ገጽ ሲዛወር ይመዝገባል። <ይመለስ> ቢጫኑ ኖሮ መዛወሩን ይገለብጣል!',
'movereason'              => 'ምክንያት',
'revertmove'              => 'ይመለስ',
'delete_and_move'         => 'ማጥፋትና ማዛወር',
'delete_and_move_text'    => '==ማጥፋት ያስፈልጋል==

መድረሻው ገጽ ሥፍራ «[[:$1]]» የሚለው ገጽ አሁን ይኖራል። ሌላው ገጽ ወደዚያ እንዲዛወር እሱን ለማጥፋት ይወድዳሉ?',
'delete_and_move_confirm' => 'አዎን፣ ገጹ ይጥፋ',
'delete_and_move_reason'  => 'ለመዛወሩ ሥፍራ እንዲገኝ ጠፋ',
'selfmove'                => 'የመነሻ እና የመድረሻ አርዕስቶች አንድ ናቸው፤ ገጽ ወደ ራሱ ለማዛወር አይቻልም።',
'immobile_namespace'      => 'የመነሻ ወይም የመድረሻ አርእስት ልዩ አይነት ነው፤ ከዚያው ወይም ወደዚያው ክፍለ-ዊኪ ገጽ ማዛወር አይቻልም።',

# Export
'export'            => 'ገጾች ወደ ሌላ ዊኪ ለመላክ',
'exportcuronly'     => 'ሙሉ ታሪክ ሳይሆን ያሁኑኑን ዕትም ብቻ ይከተት',
'export-submit'     => 'ለመላክ',
'export-addcattext' => 'ከዚሁ መደብ ገጾች ይጨመሩ፦',
'export-addcat'     => 'ለመጨምር',
'export-download'   => 'እንደ ፋይል ለመቆጠብ',
'export-templates'  => 'ከነመልጠፊያዎቹ',

# Namespace 8 related
'allmessages'               => 'የድረገጽ መልክ መልእክቶች',
'allmessagesname'           => 'የመልእክት ስም',
'allmessagesdefault'        => 'የቆየው ጽሕፈት',
'allmessagescurrent'        => 'ያሁኑ ጽሕፈት',
'allmessagestext'           => 'በ«MediaWiki» ክፍለ-ዊኪ ያሉት የድረገጽ መልክ መልእክቶች ሙሉ ዝርዝር ይህ ነው።
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "'''\$wgUseDatabaseMessages''' ስለ ተዘጋ '''{{ns:special}}:Allmessages''' ሊጠቀም አይችልም።",
'allmessagesfilter'         => 'የመልዕክት ስም ማጣሪያ፦',
'allmessagesmodified'       => 'የተቀየሩ ብቻ ይታዩ',

# Thumbnails
'thumbnail-more'           => 'አጎላ',
'filemissing'              => 'ፋይሉ አልተገኘም',
'thumbnail_error'          => 'ናሙና በመፍጠር ችግር አጋጠመ፦ $1',
'thumbnail_invalid_params' => 'ትክክለኛ ያልሆነ የናሙና ግቤት',

# Special:Import
'import'                   => 'ገጾች ከሌላ ዊኪ ለማስገባት',
'importinterwiki'          => 'ከሌላ ዊኪ ማስገባት',
'import-interwiki-history' => 'ለዚህ ገጽ የታሪክ ዕትሞች ሁሉ ለመቅዳት',
'import-interwiki-submit'  => 'ለማስገባት',
'import-revision-count'    => '$1 {{PLURAL:$1|ዕትም|ዕትሞች}}',
'importnopages'            => 'ለማስገባት ምንም ገጽ የለም።',
'importfailed'             => 'ማስገባቱ አልተከናወነም፦ <nowiki>$1</nowiki>',
'importunknownsource'      => 'ያልታወቀ የማስገባት መነሻ አይነት',
'importcantopen'           => 'የማስገባት ፋይል መክፈት አልተቻለም',
'importnotext'             => 'ባዶ ወይም ጽሕፈት የለም',
'importsuccess'            => 'ማስገባቱ ጨረሰ!',
'import-noarticle'         => 'ለማስገባት ምንም ገጽ የለም!',
'import-nonewrevisions'    => 'ዕትሞቹ ሁሉ ከዚህ በፊት ገብተዋል',

# Import log
'importlogpage'                    => 'የገጽ ማስገባት መዝገብ',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|ዕትም|ዕትሞች}}',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|ዕትም|ዕትሞች}} ከ$2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'የርስዎ መኖርያ ገጽ',
'tooltip-pt-anonuserpage'         => 'ለቁ. አድራሻዎ የመኖርያ ገጽ',
'tooltip-pt-mytalk'               => 'የርስዎ መወያያ ገጽ',
'tooltip-pt-anontalk'             => 'ለቁ. አድራሻዎ የውይይት ገጽ',
'tooltip-pt-preferences'          => 'የድረግጹን መልክ ለመምረጥ',
'tooltip-pt-watchlist'            => 'እርስዎ ስለ ለውጦች የሚከታተሏቸው ገጾች',
'tooltip-pt-mycontris'            => 'እርስዎ ያደረጓቸው ለውጦች በሙሉ',
'tooltip-pt-login'                => 'በብዕር ስም መግባትዎ ጠቃሚ ቢሆንም አስፈላጊነት አይደለም',
'tooltip-pt-anonlogin'            => 'በብዕር ስም መግባትዎ ጠቃሚ ቢሆንም አስፈላጊነት አይደለም',
'tooltip-pt-logout'               => 'ከብዕር ስምዎ ለመውጣት',
'tooltip-ca-talk'                 => 'ስለ ገጹ ለመወያየት',
'tooltip-ca-edit'                 => 'ይህን ገጽ ለማዘጋጀት ይችላሉ!',
'tooltip-ca-addsection'           => 'ለዚሁ ውይይት ገጽ አዲስ አርዕስት ለመጨምር',
'tooltip-ca-viewsource'           => 'ይህ ገጽ ተቆልፏል ~ ጥሬ ምንጩን መመልከት ይችላሉ...',
'tooltip-ca-history'              => 'ለዚሁ ገጽ ያለፉትን እትሞች ለማየት',
'tooltip-ca-protect'              => 'ይህንን ገጽ ለመቆለፍ',
'tooltip-ca-delete'               => 'ይህንን ገጽ ለማጥፋት',
'tooltip-ca-undelete'             => 'በዚህ ገጽ ላይ ሳይጠፋ የተደረጉትን ዕትሞች ለመመልስ',
'tooltip-ca-move'                 => 'ይህ ገጽ ወደ ሌላ አርእስት ለማዋወር',
'tooltip-ca-watch'                => 'ይህንን ገጽ ወደ ተከታተሉት ገጾች ዝርዝር ለመጨምር',
'tooltip-ca-unwatch'              => 'ይህንን ገጽ ከተከታተሉት ገጾች ዝርዝር ለማስወግድ',
'tooltip-search'                  => 'ቃል ወይም አርዕስት በ{{SITENAME}} ለመፈለግ',
'tooltip-search-go'               => 'ከተገኘ በዚሁ አርዕስት ወዳለው ገጽ ለመሄድ',
'tooltip-search-fulltext'         => 'ይህ ጽሕፈት የሚገኝባቸውን ገጾች ለመፈልግ',
'tooltip-p-logo'                  => 'ዋና ገጽ',
'tooltip-n-mainpage'              => 'ወደ ዋናው ገጽ ለመሔድ',
'tooltip-n-portal'                => 'ስለ መርሃገብሩ አጠቃቀም አለመረዳት',
'tooltip-n-currentevents'         => 'ስለ ወቅታዊ ጉዳዮች / ዜና መረጃ ለማግኘት',
'tooltip-n-recentchanges'         => 'በዚሁ ዊኪ ላይ በቅርቡ የተደረጉ ለውጦች',
'tooltip-n-randompage'            => 'ወደ ማንኛውም ገጽ በነሲብ ለመሔድ',
'tooltip-n-help'                  => 'ረድኤት ለማግኘት',
'tooltip-t-whatlinkshere'         => 'ወደዚሁ ገጽ የሚያያዙት ገጾች ዝርዝር በሙሉ',
'tooltip-t-recentchangeslinked'   => 'ከዚሁ ገጽ በተያያዙ ገጾች ላይ የቅርብ ግዜ ለውጦች',
'tooltip-feed-rss'                => 'የRSS ማጉረስ ለዚሁ ገጽ',
'tooltip-feed-atom'               => 'የAtom ማጉረስ ለዚሁ ገጽ',
'tooltip-t-contributions'         => 'የዚሁ አባል ለውጦች ሁሉ ለመመልከት',
'tooltip-t-emailuser'             => 'ወደዚሁ አባል ኢ-ሜል ለመላክ',
'tooltip-t-upload'                => 'ፋይል ወይም ሥዕልን ወደ {{SITENAME}} ለመላክ',
'tooltip-t-specialpages'          => 'የልዩ ገጾች ዝርዝር በሙሉ',
'tooltip-t-print'                 => 'ይህ ገጽ ለህትመት እንዲስማማ',
'tooltip-t-permalink'             => 'ለዚሁ ዕትም ቋሚ መያያዣ',
'tooltip-ca-nstab-main'           => 'መጣጥፉን ለማየት',
'tooltip-ca-nstab-user'           => 'የአባል መኖሪያ ገጽ ለማየት',
'tooltip-ca-nstab-media'          => 'የፋይሉን ገጽ ለማየት',
'tooltip-ca-nstab-special'        => 'ይህ ልዩ ገጽ ነው - ሊያዘጋጁት አይችሉም',
'tooltip-ca-nstab-project'        => 'ግብራዊ ገጹን ለማየት',
'tooltip-ca-nstab-image'          => 'የፋይሉን ገጽ ለማየት',
'tooltip-ca-nstab-mediawiki'      => 'መልእክቱን ለማየት',
'tooltip-ca-nstab-template'       => 'የመልጠፊያውን ገጽ ለመመልከት',
'tooltip-ca-nstab-help'           => 'የእርዳታ ገጽ ለማየት',
'tooltip-ca-nstab-category'       => 'የመደቡን ገጽ ለማየት',
'tooltip-minoredit'               => 'እንደ ጥቃቅን ለውጥ (ጥ) ለማመልከት',
'tooltip-save'                    => 'የለወጡትን ዕትም ወደ {{SITENAME}} ለመላክ',
'tooltip-preview'                 => 'ለውጦችዎ ሳይያቀርቡዋቸው እስቲ ይመለከቷቸው!',
'tooltip-diff'                    => 'እርስዎ የሚያደርጉት ለውጦች ከአሁኑ ዕትም ጋር ለማነጻጸር',
'tooltip-compareselectedversions' => 'ካመለከቱት ዕትሞች መካከል ያለውን ልዩነት ለማነጻጸር',
'tooltip-watch'                   => 'ይህንን ገጽ ወደተከታተሉት ገጾች ዝርዝር ለመጨምር',
'tooltip-recreate'                => 'ገጹ የጠፋ ሆኖም እንደገና ለመፍጠር',
'tooltip-upload'                  => 'ለመጀመር ይጫኑ',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadata ለዚህ ሰርቨር እንደማይሠራ ተደርጓል።',
'nocreativecommons' => 'Creative Commons RDF metadata ለዚህ ሰርቨር እንደማይሠራ ተደርጓል።',

# Attribution
'anonymous'        => 'የ{{SITENAME}} ቁ. አድራሻ ተጠቃሚ(ዎች)',
'siteuser'         => '{{SITENAME}} ተጠቃሚ $1',
'lastmodifiedatby' => 'ይህ ገጽ መጨረሻ የተቀየረው $2፣ $1 በ$3 ነበር።', # $1 date, $2 time, $3 user
'others'           => 'ሌሎች',
'siteusers'        => '{{SITENAME}} ተጠቃሚ(ዎች) $1',

# Spam protection
'spamprotectiontitle' => 'የስፓም መከላከል ማጣሪያ',
'spambot_username'    => 'MediaWiki የስፓም ማፅዳት',
'spam_reverting'      => 'ወደ $1 የሚወስድ መያያዣ ወደሌለበት መጨረሻ ዕትም መለሰው',

# Info page
'infosubtitle'   => 'መረጃ ለገጹ',
'numedits'       => 'የእትሞች ቁጥር (ገጽ)፦ $1',
'numtalkedits'   => 'የእትሞች ቁጥር (የውይይት ገጽ)፦ $1',
'numwatchers'    => 'የሚከታተሉት ተጠቃሚዎች ቁጥር፦ $1',
'numauthors'     => 'የተለዩ አቅራቢዎች ቁጥር (ገጽ)፦ $1',
'numtalkauthors' => 'የተለዩ አቅራቢዎች ቁጥር (የውይይት ገጽ)፦ $1',

# Math options
'mw_math_png'    => 'ሁልጊዜ እንደ PNG',
'mw_math_simple' => 'HTML ቀላል ከሆነ አለዚያ PNG',
'mw_math_html'   => 'HTML ከተቻለ አለዚያ PNG',
'mw_math_modern' => 'ለዘመናዊ ብራውዘር የተሻለ',
'mw_math_mathml' => 'MathML ከተቻለ (የሙከራ)',

# Patrolling
'markaspatrolleddiff'                 => 'የተሳለፈ ሆኖ ማመልከት',
'markaspatrolledtext'                 => 'ይህን ገጽ የተመለከተ ሆኖ ለማሳለፍ',
'markedaspatrolled'                   => 'የተመለከተ ሆኖ ተሳለፈ',
'markedaspatrolledtext'               => 'የተመረጠው ዕትም የተመለከተ ሆኖ ተሳለፈ።',
'rcpatroldisabled'                    => 'የቅርብ ለውጦች ማሳለፊያ አይኖርም',
'rcpatroldisabledtext'                => 'የቅርብ ለውጦች ማሳለፊያ ተግባር አሁን አይሠራም።',
'markedaspatrollederror'              => 'የተመለከተ ሆኖ ለማሳለፍ አይቻልም',
'markedaspatrollederrortext'          => 'የተመለከተ ሆኖ ለማሳለፍ አንድን ዕትም መወሰን አለብዎት።',
'markedaspatrollederror-noautopatrol' => 'የራስዎን ለውጥ የተመለከተ ሆኖ ለማሳለፍ አይችሉም።',

# Patrol log
'patrol-log-page' => 'የማሳለፊያ መዝገብ',
'patrol-log-line' => 'እትም $1 ከ$2 የተመለከተ ሆኖ አሳለፈ $3',
'patrol-log-auto' => '(በቀጥታ)',

# Image deletion
'deletedrevision'                 => 'የቆየው ዕትም $1 አጠፋ',
'filedeleteerror-short'           => 'የፋይል ማጥፋት ስኅተት፦ $1',
'filedeleteerror-long'            => 'ፋይሉን በማጥፋት ስህተቶች ተነስተዋል፦

$1',
'filedelete-missing'              => 'ፋይሉ «$1» ሰለማይኖር ሊጠፋ አይችልም።',
'filedelete-old-unregistered'     => 'የተወሰነው ፋይል ዕትም «$1» በመረጃ-ቤቱ የለም።',
'filedelete-current-unregistered' => 'የተወሰነው ፋይል «$1» በመረጃ-ቤቱ የለም።',

# Browsing diffs
'previousdiff' => '← የፊተኛው ለውጥ',
'nextdiff'     => 'የሚከተለው ለውጥ →',

# Media information
'imagemaxsize'         => 'በፋይል መግለጫ ገጽ ላይ የስዕል መጠን ወሰን ቢበዛ፦',
'thumbsize'            => 'የናሙና መጠን፦',
'widthheightpage'      => '$1 በ$2፣ $3 ገጾች',
'file-info'            => '(የፋይል መጠን፦ $1፣ የMIME አይነት፦ $2)',
'file-info-size'       => '($1 × $2 ፒክስል፤ መጠን፦ $3፤ የMIME ዓይነት፦ $4)',
'file-nohires'         => '<small>ከዚህ በላይ ማጉላት አይቻልም።</small>',
'svg-long-desc'        => '(የSVG ፋይል፡ በተግባር $1 × $2 ፒክስል፤ መጠን፦ $3)',
'show-big-image'       => 'በሙሉ ጒልህነት ለመመልከት',
'show-big-image-thumb' => '<small>የዚህ ናሙና ቅጂ ክልል፦ $1 × $2 ፒክሰል</small>',

# Special:NewImages
'newimages'             => 'የአዳዲስ ሥዕሎች ማሳያ አዳራሽ',
'imagelisttext'         => '$1 የተጨመሩ ሥእሎች ወይም ፋይሎች ከታች ይዘረዝራሉ ($2)።',
'showhidebots'          => '(«bots» $1)',
'noimages'              => 'ምንም የለም!',
'ilsubmit'              => 'ፍለጋ',
'bydate'                => 'በተጨመሩበት ወቅት',
'sp-newimages-showfrom' => 'ከ$2፣ $1 እ.ኤ.አ. ጀምሮ አዲስ ይታዩ',

# Bad image list
'bad_image_list' => 'ሥርዓቱ እንዲህ ነው፦

በ* የሚጀምሩ መስመሮች ብቻ ይቆጠራል። በመስመሩ መጀመርያው መያያዣ የመጥፎ ስዕል መያያዣ መሆን አለበት።  ከዚያ ቀጥሎ በዚያው በመስመር መያያዣ ቢገኝ ግን ስዕሉ እንደ ተፈቀደበት ገጽ ይቆጠራል።',

# Metadata
'metadata'          => 'ተጨማሪ መረጃ',
'metadata-help'     => 'ይህ ፋይል በውስጡ ተጨማሪ መረጃ ይይዛል። መረጃውም በዲጂታል ካሜራ ወይም በኮምፒውተር ስካነር የተጨመረ ይሆናል። ይህ ከኦሪጂናሉ ቅጅ የተለወጠ ከሆነ፣ ምናልባት የመረጃው ዝርዝር ለውጦቹን የማያንጸባረቅ ይሆናል።',
'metadata-expand'   => 'ተጨማሪ መረጃ ይታይ',
'metadata-collapse' => 'ተጨማሪ መረጃ ይደበቅ',
'metadata-fields'   => "በዚህ የሚዘረዘሩ EXIF መረጃ አይነቶች በፋይል ገጽ ላይ በቀጥታ ይታያሉ። ሌሎቹ 'ተጨማሪ መረጃ ይታይ' ካልተጫነ በቀር ይደበቃሉ።
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'ስፋት',
'exif-imagelength'                 => 'ቁመት',
'exif-compression'                 => 'የመጨመቅ ዘዴ',
'exif-photometricinterpretation'   => 'የPixel አሠራር',
'exif-orientation'                 => 'አቀማመጥ',
'exif-samplesperpixel'             => 'የክፍለ ነገሮች ቁጥር',
'exif-planarconfiguration'         => 'የመረጃ አስተዳደር',
'exif-ycbcrpositioning'            => 'የY ና C አቀማመጥ',
'exif-xresolution'                 => 'አድማሳዊ ማጉላት',
'exif-yresolution'                 => 'ቁም ማጉላት',
'exif-resolutionunit'              => 'የX ና Y ማጉላት መስፈርያ',
'exif-stripoffsets'                => 'የስዕል መረጃ ሥፍራ',
'exif-rowsperstrip'                => 'የተርታዎች ቁጥር በየቁራጩ',
'exif-stripbytecounts'             => 'byte በየተጨመቀ ቁራጩ',
'exif-jpeginterchangeformatlength' => 'የJPEG መረጃ byte',
'exif-transferfunction'            => 'የማሻገር ተግባር',
'exif-datetime'                    => 'ፋይሉ የተቀየረበት ቀንና ሰዓት',
'exif-imagedescription'            => 'የስዕል አርዕስት',
'exif-make'                        => 'የካሜራው ሠሪ ድርጅት',
'exif-model'                       => 'የካሜራው ዝርያ',
'exif-software'                    => 'የተጠቀመው ሶፍትዌር',
'exif-artist'                      => 'ደራሲ',
'exif-copyright'                   => 'ባለ መብቱ',
'exif-exifversion'                 => 'የExif ዝርያ',
'exif-flashpixversion'             => 'የተደገፈ Flashpix ዝርያ',
'exif-componentsconfiguration'     => 'የየክፍለ ነገሩ ትርጉም',
'exif-compressedbitsperpixel'      => 'የስዕል መጨመቅ ዘዴ',
'exif-pixelydimension'             => 'እውነተኛ የስዕል ስፋት',
'exif-pixelxdimension'             => 'እውነተኛ የስዕል ቁመት',
'exif-makernote'                   => 'የሠሪው ማሳሰቢያዎች',
'exif-usercomment'                 => 'የተጠቃሚው ማጠቃለያ',
'exif-relatedsoundfile'            => 'የተዛመደ የድምጽ ፋይል',
'exif-datetimeoriginal'            => 'መረጃው የተፈጠረበት ቀንና ሰዓት',
'exif-datetimedigitized'           => 'ዲጂታል የተደረገበት ቀንና ሰዓት',
'exif-exposuretime'                => 'ማንሣት የሚፈጅበት ግዜ',
'exif-exposuretime-format'         => '$1 ሴኮንድ ($2)',
'exif-fnumber'                     => 'የF ቁጥር',
'exif-exposureprogram'             => 'የማንሣት ፕሮግራም',
'exif-shutterspeedvalue'           => 'የመዝጊያ ፍጥነት',
'exif-aperturevalue'               => 'ቀዳዳ',
'exif-brightnessvalue'             => 'ብሩህነት',
'exif-exposurebiasvalue'           => 'የማንሣት ዝንባሌ',
'exif-maxaperturevalue'            => 'የየብስ ቀዳዳ ወሰን ቢበዛ',
'exif-subjectdistance'             => 'የጉዳዩ ርቀት',
'exif-meteringmode'                => 'የመመተር ዘዴ',
'exif-lightsource'                 => 'የብርሃን ምንጭ',
'exif-flash'                       => 'ብልጭታ',
'exif-focallength'                 => 'የመስተዋት ትኩረት እርዝማኔ',
'exif-subjectarea'                 => 'የጉዳዩ ክልል',
'exif-flashenergy'                 => 'የብልጭታ ኅይል',
'exif-subjectlocation'             => 'የጉዳዩ ሥፍራ',
'exif-exposureindex'               => 'የማንሣት ማውጫ',
'exif-sensingmethod'               => 'የመሰማት ዘዴ',
'exif-filesource'                  => 'የፋይል ምንጭ',
'exif-scenetype'                   => 'የትርኢት አይነት',
'exif-customrendered'              => 'ልዩ የስዕል አገባብ',
'exif-exposuremode'                => 'የማንሣት ዘዴ',
'exif-whitebalance'                => 'የነጭ ዝንባሌ',
'exif-digitalzoomratio'            => 'ቁጥራዊ ማጉላት ውድር',
'exif-focallengthin35mmfilm'       => 'በ35 mm ፊልም የትኩረት እርዝማኔ',
'exif-scenecapturetype'            => 'የትርኢት መማረክ አይነት',
'exif-gaincontrol'                 => 'የትርኢት ማሠልጠን',
'exif-contrast'                    => 'የድምቀት አነጻጸር',
'exif-sharpness'                   => 'ስለት',
'exif-subjectdistancerange'        => 'የጉዳዩ ርቀት',
'exif-imageuniqueid'               => 'የስዕሉ መታወቂያ ቁጥር',
'exif-gpsversionid'                => 'የGPS ምልክት ዝርያ',
'exif-gpslatituderef'              => 'ስሜን ወይም ደቡብ ኬክሮስ',
'exif-gpslatitude'                 => 'ኬክሮስ',
'exif-gpslongituderef'             => 'ምስራቅ ወይም ምዕራብ ኬንትሮስ',
'exif-gpslongitude'                => 'ኬንትሮስ',
'exif-gpsaltituderef'              => 'የከፍታ መሰረት',
'exif-gpsaltitude'                 => 'ከፍታ',
'exif-gpstimestamp'                => 'GPS ሰዓት (አቶማዊ ሰዓት)',
'exif-gpssatellites'               => 'ለመስፈር የተጠቀሙ ሰው ሰራሽ መንኮራኩር',
'exif-gpsstatus'                   => 'የተቀባይ ሁኔታ',
'exif-gpsmeasuremode'              => 'የመስፈር ዘዴ',
'exif-gpsdop'                      => 'የመስፈር ልክነት',
'exif-gpsspeedref'                 => 'የፍጥነት መስፈርያ',
'exif-gpsspeed'                    => 'የGPS ተቀባይ ፍጥነት',
'exif-gpstrackref'                 => 'የስዕል እንቅስቃሴ መሰረት',
'exif-gpstrack'                    => 'የእንቅስቃሴ አቅጣጫ',
'exif-gpsimgdirectionref'          => 'የስዕል አቅጣጫ መሠረት',
'exif-gpsimgdirection'             => 'የስዕል አቅጣጫ',
'exif-gpsdestlatituderef'          => 'የመድረሻ ኬክሮስ መሠረት',
'exif-gpsdestlatitude'             => 'የመድረሻ ኬክሮስ',
'exif-gpsdestlongituderef'         => 'የመድረሻ ኬንትሮስ መሠረት',
'exif-gpsdestlongitude'            => 'የመድረሻ ኬንትሮስ',
'exif-gpsdestdistanceref'          => 'የመድረሻ ርቀት መሠረት',
'exif-gpsdestdistance'             => 'ርቀት ከመድረሻ',
'exif-gpsprocessingmethod'         => 'የGPS አግባብ ዘዴ ስም',
'exif-gpsareainformation'          => 'የGPS ክልል ስም',
'exif-gpsdatestamp'                => 'የGPS ቀን',
'exif-gpsdifferential'             => 'GPS ልዩነት ማስተካከል',

# EXIF attributes
'exif-compression-1' => 'ያልተጨመቀ',

'exif-unknowndate' => 'ያልታወቀ ቀን',

'exif-orientation-1' => 'የተለመደ', # 0th row: top; 0th column: left
'exif-orientation-2' => 'በአድማሱ ላይ ተገለበጠ', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° የዞረ', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'በዋልታው ላይ ተገለበጠ', # 0th row: bottom; 0th column: left

'exif-componentsconfiguration-0' => 'አይኖርም',

'exif-exposureprogram-0' => 'አልተወሰነም',
'exif-exposureprogram-1' => 'በዕጅ',
'exif-exposureprogram-2' => 'የተለመደ ፕሮግራም',
'exif-exposureprogram-3' => 'የቀዳዳ ቀዳሚነት',
'exif-exposureprogram-4' => 'የመዝጊያ ቀዳሚነት',
'exif-exposureprogram-6' => 'የድርጊት ፕሮግራም (ለፈጣን መዝጊያ ፍጥነት የዘነበለ)',

'exif-subjectdistance-value' => '$1 ሜትር',

'exif-meteringmode-0'   => 'አይታወቅም',
'exif-meteringmode-1'   => 'አማካኝ',
'exif-meteringmode-3'   => 'ነጥብ',
'exif-meteringmode-6'   => 'በከፊል',
'exif-meteringmode-255' => 'ሌላ',

'exif-lightsource-0'   => 'አይታወቅም',
'exif-lightsource-1'   => 'መዓልት',
'exif-lightsource-3'   => 'Tungsten (ቦግ ያለ መብራት)',
'exif-lightsource-4'   => 'ብልጭታ',
'exif-lightsource-9'   => 'መልካም አየር',
'exif-lightsource-10'  => 'ደመናም አየር',
'exif-lightsource-11'  => 'ጥላ',
'exif-lightsource-17'  => 'የተለመደ ብርሃን A',
'exif-lightsource-18'  => 'የተለመደ ብርሃን B',
'exif-lightsource-19'  => 'የተለመደ ብርሃን C',
'exif-lightsource-255' => 'ሌላ የብርሃን ምንጭ',

'exif-focalplaneresolutionunit-2' => 'inches (ኢንች)',

'exif-sensingmethod-1' => 'ያልተወሰነ',
'exif-sensingmethod-2' => 'የ1-ኤሌክትሮ-ገል ቀለም ክልል ሰሚ',
'exif-sensingmethod-3' => 'የ2-ኤሌክትሮ-ገል ቀለም ክልል ሰሚ',
'exif-sensingmethod-4' => 'የ3-ኤሌክትሮ-ገል ቀለም ክልል ሰሚ',
'exif-sensingmethod-5' => 'ቀለም ተከታታይ ክልል ሰሚ',
'exif-sensingmethod-7' => 'ሦስት መስመር ያለው ሰሚ',
'exif-sensingmethod-8' => 'ቀለም ተከታታይ መስመር ሰሚ',

'exif-scenetype-1' => 'በቀጥታ የተነሣ የፎቶ ስዕል',

'exif-customrendered-0' => 'የተለመደ ሂደት',
'exif-customrendered-1' => 'ልዩ ሂደት',

'exif-exposuremode-0' => 'የቀጥታ ማንሣት',
'exif-exposuremode-1' => 'በዕጅ ማንሣት',

'exif-whitebalance-0' => 'የቀጥታ ነጭ ዝንባሌ',
'exif-whitebalance-1' => 'በእጅ የተደረገ ነጭ ዝንባሌ',

'exif-scenecapturetype-0' => 'የተለመደ',
'exif-scenecapturetype-1' => 'አግድም',
'exif-scenecapturetype-2' => 'ቁም',
'exif-scenecapturetype-3' => 'የሌሊት ትርኢት',

'exif-gaincontrol-0' => 'የለም',

'exif-contrast-0' => 'የተለመደ',
'exif-contrast-1' => 'ለስላሳ',
'exif-contrast-2' => 'ጽኑዕ',

'exif-saturation-0' => 'የተለመደ',

'exif-sharpness-0' => 'የተለመደ',
'exif-sharpness-1' => 'ለስላሳ',
'exif-sharpness-2' => 'ጽኑዕ',

'exif-subjectdistancerange-0' => 'አይታወቅም',
'exif-subjectdistancerange-2' => 'ከቅርብ አስተያየት',
'exif-subjectdistancerange-3' => 'ከሩቅ አስተያየት',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'ስሜን ኬክሮስ',
'exif-gpslatitude-s' => 'ደቡብ ኬክሮስ',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'ምሥራቅ ኬንትሮስ',
'exif-gpslongitude-w' => 'ምዕራብ ኬንትሮስ',

'exif-gpsmeasuremode-2' => '2 አቅጣቻ ያለው መለኪያ',
'exif-gpsmeasuremode-3' => '3 አቅጣቻ ያለው መለኪያ',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'ኪሎሜትር በየሰዓቱ',
'exif-gpsspeed-m' => 'ማይል (mile) በየሰዓቱ',
'exif-gpsspeed-n' => 'Knot (የመርከብ ፍጥነት መለኪያ)',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'ዕውነተኛ አቅጣጫ',
'exif-gpsdirection-m' => 'መግነጢሳዊ አቅጣጫ',

# External editor support
'edit-externally'      => 'ይህንን ፋይል በአፍአዊ ሶፍትዌር ለማዘጋጀት',
'edit-externally-help' => 'ስለ አፍአዊ የስዕል ማዘጋጀት ሶፍትዌር በተጨማሪ ለመረዳት [http://www.mediawiki.org/wiki/Manual:External_editors የመመስረት ትዕዛዝ] ያንብቡ።',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'ሁሉ',
'imagelistall'     => 'ሁሉ',
'watchlistall2'    => 'ሁሉ',
'namespacesall'    => 'ሁሉ (all)',
'monthsall'        => 'ሁሉ',

# E-mail address confirmation
'confirmemail'            => 'ኢ-ሜልዎን ለማረጋገጥ',
'confirmemail_noemail'    => 'በ[[Special:Preferences|ምርጫዎችዎ]] ትክክለኛ ኢሜል አድራሻ አልሰጡም።',
'confirmemail_text'       => 'አሁን በ{{SITENAME}} በኩል «ኢ-ሜል» ለመላክም ሆነ ለመቀበል አድራሻዎን ማረጋገጥ ግዴታ ሆኗል። እታች ያለውን በተጫኑ ጊዜ አንድ የማረጋገጫ መልእክት ቀድሞ ወደ ሰጡት ኢሜል አድራሻ በቀጥታ ይላካል። በዚህ መልእክት ልዩ ኮድ ያለበት መያያዣ ይገኝበታል፣ ይህንን መያያዣ ከዚያ ቢጎብኙ ኢ-ሜል አድራሻዎ የዛኔ ይረጋግጣል።',
'confirmemail_pending'    => '<div class="error">ማረጋገጫ ኮድ ከዚህ በፊት ገና ተልኮልዎታል። ብዕር ስምዎን ያወጡ በቅርብ ጊዜ ከሆነ፣ አዲስ ኮድን ከመጠይቅ በፊት ምናልባት የተላከው እስከሚደርስ ድረስ ጥቂት ደቂቃ መቆየት ይሻላል።</div>',
'confirmemail_send'       => 'የማረጋገጫ ኮድ ወደኔ ኢ-ሜል ይላክልኝ',
'confirmemail_sent'       => 'የማረጋገጫ ኢ-ሜል ቅድም ወደ ሰጡት አድራሻ አሁን ተልኳል!',
'confirmemail_oncreate'   => 'ማረጋገጫ ኮድ ወደ ኢ-ሜል አድራሻዎ ተልኳል። ይኸው ኮድ ለመግባት አያስፈልግም፤ ነገር ግን የዊኪው ኢ-ሜል ተግባር እንዲሠራ ለማድረግ ያስፈልጋል።',
'confirmemail_sendfailed' => 'ወደሰጡት ኢሜል አድራሻ መላክ አልተቻለም። እባክዎ፣ ወደ [[Special:Preferences|«ምርጫዎች»]] ተመልሰው የጻፉትን አድራሻ ደንበኛነት ይመለከቱ።',
'confirmemail_invalid'    => 'ይህ ኮድ አልተከናወነም። (ምናልባት ጊዜው አልፏል።) እንደገና ይሞክሩ!',
'confirmemail_needlogin'  => 'ኢሜል አድራሻዎን ለማረጋገጥ $1 ያስፈልግዎታል።',
'confirmemail_success'    => 'እ-ሜል አድራሻዎ ተረጋግጧል። አሁን ገብተው ዊኪውን መጠቀም ይችላሉ።',
'confirmemail_loggedin'   => 'የርስዎ ኢ-ሜል አድራሻ ተረጋግጧል። አሁን ኢ-ሜል በ{{SITENAME}} በኩል ለመላክ ወይም ለመቀበል ይችላሉ።',
'confirmemail_error'      => 'ማረጋገጫዎን በመቆጠብ አንድ ችግር ተነሣ።',
'confirmemail_subject'    => '{{SITENAME}} email address confirmation / እ-ሜል አድራሻ ማረጋገጫ',
'confirmemail_body'       => 'ጤና ይስጥልኝ

የርስዎ ኢ-ሜል አድራሻ በ$1 ለ{{SITENAME}} ብዕር ስም «$2» ቀርቧል። 

ይህ እርስዎ እንደ ሆኑ ለማረጋገጥና የ{{SITENAME}} ኢ-ሜል ጥቅም ለማግኘት፣ እባክዎን የሚከተለውን መያያዣ ይጎበኙ።

$3

ይህ ምናልባት እርስዎ ካልሆኑ፣ መያያዣውን አይከተሉ። 

የዚህ መያያዣው ኮድ እስከ $4 ድረስ ይሠራል።',

# Scary transclusion
'scarytranscludetoolong' => '[ይቅርታ፤ URL ከመጠን በላይ የረዘመ ነው]',

# Trackbacks
'trackbackremove' => ' ([$1 ማጥፋት])',

# Delete conflict
'deletedwhileediting' => 'ማስጠንቀቂያ፦ መዘጋጀት ከጀመሩ በኋላ ገጹ ጠፍቷል!',
'confirmrecreate'     => "መዘጋጀት ከጀመሩ በኋላ፣ ተጠቃሚው [[User:$1|$1]] ([[User talk:$1|ውይይት]]) ገጹን አጠፍተው ይህን ምክንያት አቀረቡ፦
: ''$2''
እባክዎ ገጹን እንደገና ለመፍጠር በውኑ እንደ ፈለጉ ያረጋግጡ።",
'recreate'            => 'እንደገና ይፈጠር',

# HTML dump
'redirectingto' => 'ወደ [[:$1]] መምሪያ መንገድ ማድረግ...',

# action=purge
'confirm_purge'        => 'የዚሁ ገጽ ካሽ (cache) ይጠረግ?

$1',
'confirm_purge_button' => 'እሺ',

# AJAX search
'searchcontaining' => "''$1'' ላለባቸው ገጾች ለመፈልግ።",
'searchnamed'      => "''$1'' ለተባሉት ገጾች ለመፈልግ።",
'articletitles'    => "በ''$1'' የሚጀመሩ ገጾች፦",
'hideresults'      => 'ውጤቶች ለመደብቅ',
'useajaxsearch'    => 'የAJAX ፍለጋ ይጠቀም',

# Multipage image navigation
'imgmultipageprev' => '← ፊተኛው ገጽ',
'imgmultipagenext' => 'የሚቀጥለው ገጽ →',
'imgmultigo'       => 'ሂድ!',

# Table pager
'table_pager_next'         => 'ቀጥሎ ገጽ',
'table_pager_prev'         => 'ፊተኛው ገጽ',
'table_pager_first'        => 'መጀመርያው ግጽ',
'table_pager_last'         => 'መጨረሻው ገጽ',
'table_pager_limit'        => 'በየገጹ $1 መስመሮች',
'table_pager_limit_submit' => 'ይታዩ',
'table_pager_empty'        => 'ምንም ውጤት የለም',

# Auto-summaries
'autosumm-blank'   => 'ጽሑፉን በሙሉ ደመሰሰ።',
'autosumm-replace' => 'ጽሑፉ በ«$1» ተተካ።',
'autoredircomment' => 'ወደ [[$1]] መምሪያ መንገድ ፈጠረ',
'autosumm-new'     => 'አዲስ ገጽ ፈጠረ፦ «$1»',

# Live preview
'livepreview-loading' => 'በመጫን ላይ ነው...',
'livepreview-ready'   => 'በመጫን ላይ ነው... ዝግጁ!',
'livepreview-failed'  => 'የቀጥታ ቅድመ-ዕይታ አልተከናወነም! የተለመደ ቅድመ-ዕይታ ይሞክሩ።',
'livepreview-error'   => 'መገናኘት አልተከናወነም፦$1 «$2»። የተለመደ ቅድመ-ዕይታ ይሞክሩ።',

# Friendlier slave lag warnings
'lag-warn-normal' => 'ከ$1 ሴኮንድ በፊት ጀምሮ የቀረቡ ለውጦች ምናልባት በዚህ ዝርዝር አይታዩም።',
'lag-warn-high'   => 'የመረጃ-ቤት ሰርቨር በጣም ስለሚዘገይ፣ ከ$1 ሴኮንድ በፊት ጀምሮ የቀረቡ ለውጦች ምናልባት በዚህ ዝርዝር አይታዩም።',

# Watchlist editor
'watchlistedit-numitems'       => 'አሁን በሙሉ {{PLURAL:$1|$1 ገጽ|$1 ገጾች}} እየተከታተሉ ነው።',
'watchlistedit-noitems'        => 'ዝርዝርዎ ባዶ ነው።',
'watchlistedit-normal-title'   => 'ዝርዝሩን ለማስተካከል',
'watchlistedit-normal-legend'  => 'አርእስቶችን ከተካከሉት ገጾች ዝርዝር ለማስወግድ...',
'watchlistedit-normal-explain' => 'ከዚህ ታች፣ የሚከታተሉት ገጾች ሁሉ በሙሉ ተዘርዝረው ይገኛሉ።

አንዳንድ ገጽ ከዚህ ዝርዝር ለማስወግድ ያሠቡ እንደሆነ፣ በሳጥኑ ውስጥ ምልክት አድርገው በስተግርጌ በሚገኘው «ማስወግጃ» የሚለውን ተጭነው ከዚህ ዝርዝር ሊያስወግዷቸው ይቻላል። (ይህን በማድረግዎ ከገጹ ጋር የሚገናኘው ውይይት ገጽ ድግሞ ከዝርዝርዎ ይጠፋል።)

ከዚህ ዘዴ ሌላ [[Special:Watchlist/raw|ጥሬውን ኮድ መቅዳት ወይም ማዘጋጀት]] ይቻላል።',
'watchlistedit-normal-submit'  => 'ማስወገጃ',
'watchlistedit-normal-done'    => 'ከዝርዝርዎ {{PLURAL:$1|1 አርዕስት ተወግዷል|$1 አርእስቶች ተወግደዋል}}፦',
'watchlistedit-raw-title'      => 'የዝርዝሩ ጥሬ ኮድ',
'watchlistedit-raw-legend'     => 'የዝርዝሩን ጥሬ ኮድ ለማዘጋጀት...',
'watchlistedit-raw-explain'    => 'በተከታተሉት ገጾች ዝርዝር ላይ ያሉት አርእስቶች ሁሉ ከዚህ ታች ይታያሉ። በየመስመሩ አንድ አርእስት እንደሚኖር፣ ይህን ዝርዝር ለማዘጋጀት ይችላሉ። አዘጋጅተውት ከጨረሱ በኋላ በስተግርጌ «ዝርዝሩን ለማሳደስ» የሚለውን ይጫኑ። አለበለዚያ ቢሻልዎት፣ የተለመደውን ዘዴ ([[Special:Watchlist/edit|«ዝርዝሩን ለማስተካከል»]]) ይጠቀሙ።',
'watchlistedit-raw-titles'     => 'የተከታተሉት አርእስቶች፦',
'watchlistedit-raw-submit'     => 'ዝርዝሩን ለማሳደስ',
'watchlistedit-raw-done'       => 'ዝርዝርዎ ታድሷል።',
'watchlistedit-raw-added'      => '$1 አርዕስት {{PLURAL:$1|ተጨመረ|ተጨመሩ}}፦',
'watchlistedit-raw-removed'    => '$1 አርዕስት {{PLURAL:$1|ተወገደ|ተወገዱ}}፦',

# Watchlist editing tools
'watchlisttools-view' => 'የምከታተላቸው ለውጦች',
'watchlisttools-edit' => 'ዝርዝሩን ለማስተካከል',
'watchlisttools-raw'  => 'የዝርዝሩ ጥሬ ኮድ',

# Core parser functions
'unknown_extension_tag' => 'ያልታወቀ የቅጥያ ምልክት «$1»',

# Special:Version
'version'                          => 'ዝርያ', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'የተሳኩ ቅጥያዎች',
'version-specialpages'             => 'ልዩ ገጾች',
'version-parserhooks'              => 'የዘርዛሪ ሜንጦዎች',
'version-variables'                => 'ተለዋጮች',
'version-other'                    => 'ሌላ',
'version-hooks'                    => 'ሜንጦዎች',
'version-extension-functions'      => 'የቅጥያ ሥራዎች',
'version-parser-extensiontags'     => 'የዝርዛሪ ቅጥያ ምልክቶች',
'version-parser-function-hooks'    => 'የዘርዛሪ ተግባር ሜጦዎች',
'version-skin-extension-functions' => 'የመልክ ቅጥያ ተግባሮች',
'version-hook-name'                => 'የሜንጦ ስም',
'version-hook-subscribedby'        => 'የተጨመረበት',
'version-version'                  => 'ዝርያ',
'version-license'                  => 'ፈቃድ',
'version-software'                 => 'የተሳካ ሶፍትዌር',
'version-software-product'         => 'ሶፍትዌር',
'version-software-version'         => 'ዝርያ',

# Special:FilePath
'filepath'         => 'የፋይል መንገድ',
'filepath-page'    => 'ፋይሉ፦',
'filepath-submit'  => 'መንገድ',
'filepath-summary' => 'ይህ ልዩ ገጽ ለ1 ፋይል ሙሉ መንገድ ይሰጣል።<br />
ስዕል በሙሉ ማጉላት ይታያል፤ ሌላ አይነት ፋይል በሚገባው ፕሮግራም በቀጥታ ይጀመራል።

የፋይሉ ስም («{{ns:image}}:» የሚለው ባዕድ መነሻ ሳይኖር) ከዚህ ታች ይግባ፦',

# Special:SpecialPages
'specialpages' => 'ልዩ ገጾች',

);
