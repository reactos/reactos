<?php
/** Punjabi (ਪੰਜਾਬੀ)
 *
 * @ingroup Language
 * @file
 *
 * @author AS Alam
 * @author Gman124
 * @author Sukh
 * @author Ævar Arnfjörð Bjarmason
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard'    => 'ਕਲਾਸਿਕ',
	'monobook'    => 'ਮੋਨੋਬੁੱਕ',
	'myskin'      => 'ਮੇਰੀਸਕਿਨ',
	'chick'       => 'ਚੀਚਕ',
	'simple'      => 'ਸੈਂਪਲ'
);

$namespaceNames = array(
	NS_MEDIA          => 'ਮੀਡੀਆ',
	NS_SPECIAL        => 'ਖਾਸ',
	NS_MAIN           => '',
	NS_TALK           => 'ਚਰਚਾ',
	NS_USER           => 'ਮੈਂਬਰ',
	NS_USER_TALK      => 'ਮੈਂਬਰ_ਚਰਚਾ',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1_ਚਰਚਾ',
	NS_IMAGE          => 'ਤਸਵੀਰ',
	NS_IMAGE_TALK     => 'ਤਸਵੀਰ_ਚਰਚਾ',
	NS_MEDIAWIKI      => 'ਮੀਡੀਆਵਿਕਿ',
	NS_MEDIAWIKI_TALK => 'ਮੀਡੀਆਵਿਕਿ_ਚਰਚਾ',
	NS_TEMPLATE       => 'ਨਮੂਨਾ',
	NS_TEMPLATE_TALK  => 'ਨਮੂਨਾ_ਚਰਚਾ',
	NS_HELP           => 'ਮਦਦ',
	NS_HELP_TALK      => 'ਮਦਦ_ਚਰਚਾ',
	NS_CATEGORY       => 'ਸ਼੍ਰੇਣੀ',
	NS_CATEGORY_TALK  => 'ਸ਼੍ਰੇਣੀ_ਚਰਚਾ'
);

$digitTransformTable = array(
	'0' => '੦', # &#x0a66;
	'1' => '੧', # &#x0a67;
	'2' => '੨', # &#x0a68;
	'3' => '੩', # &#x0a69;
	'4' => '੪', # &#x0a6a;
	'5' => '੫', # &#x0a6b;
	'6' => '੬', # &#x0a6c;
	'7' => '੭', # &#x0a6d;
	'8' => '੮', # &#x0a6e;
	'9' => '੯', # &#x0a6f;
);
$linkTrail = '/^([ਁਂਃਅਆਇਈਉਊਏਐਓਔਕਖਗਘਙਚਛਜਝਞਟਠਡਢਣਤਥਦਧਨਪਫਬਭਮਯਰਲਲ਼ਵਸ਼ਸਹ਼ਾਿੀੁੂੇੈੋੌ੍ਖ਼ਗ਼ਜ਼ੜਫ਼ੰੱੲੳa-z]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'          => 'ਅੰਡਰ-ਲਾਈਨ ਲਿੰਕ:',
'tog-numberheadings'     => 'ਆਟੋ-ਨੰਬਰ ਹੈਡਿੰਗ',
'tog-showtoolbar'        => 'ਐਡਿਟ ਟੂਲਬਾਰ ਵੇਖੋ (JavaScript)',
'tog-showtoc'            => 'ਟੇਬਲ ਆਫ਼ ਕੰਨਟੈੱਟ ਵੇਖਾਓ (for pages with more than 3 headings)',
'tog-rememberpassword'   => 'ਇਹ ਕੰਪਿਊਟਰ ਲਈ ਮੇਰਾ ਲਾਗਇਨ ਯਾਦ ਰੱਖੋ',
'tog-editwidth'          => 'ਐਡਿਟ ਬਾਕਸ ਪੇਜ ਦੀ ਪੂਰੀ ਚੌੜਾਈ ਵਿੱਚ ਕਰੋ',
'tog-previewontop'       => 'ਐਡਿਟ ਬਕਸੇ ਤੋਂ ਪਹਿਲਾਂ ਝਲਕ ਵੇਖਾਓ',
'tog-previewonfirst'     => 'ਪਹਿਲੇ ਐਡਿਟ ਉੱਤੇ ਝਲਕ ਵੇਖਾਓ',
'tog-watchlisthideminor' => 'ਛੋਟੇ ਸੋਧ ਵਾਚ-ਲਿਸਟ ਤੋਂ ਓਹਲੇ ਰੱਖੋ',

'underline-always'  => 'ਹਮੇਸ਼ਾਂ',
'underline-never'   => 'ਕਦੇ ਨਹੀਂ',
'underline-default' => 'ਬਰਾਊਜ਼ਰ ਡਿਫਾਲਟ',

'skinpreview' => '(ਝਲਕ)',

# Dates
'sunday'        => 'ਐਤਵਾਰ',
'monday'        => 'ਸੋਮਵਾਰ',
'tuesday'       => 'ਮੰਗਲਵਾਰ',
'wednesday'     => 'ਬੁੱਧਵਾਰ',
'thursday'      => 'ਵੀਰਵਾਰ',
'friday'        => 'ਸ਼ੁੱਕਰਵਾਰ',
'saturday'      => 'ਸ਼ਨਿੱਚਰਵਾਰ',
'sun'           => 'ਐਤ',
'mon'           => 'ਸੋਮ',
'tue'           => 'ਮੰਗਲ',
'wed'           => 'ਬੁੱਧ',
'thu'           => 'ਵੀਰ',
'fri'           => 'ਸ਼ੁੱਕਰ',
'sat'           => 'ਸ਼ਨਿੱਚਰ',
'january'       => 'ਜਨਵਰੀ',
'february'      => 'ਫ਼ਰਵਰੀ',
'march'         => 'ਮਾਰਚ',
'april'         => 'ਅਪ੍ਰੈਲ',
'may_long'      => 'ਮਈ',
'june'          => 'ਜੂਨ',
'july'          => 'ਜੁਲਾਈ',
'august'        => 'ਅਗਸਤ',
'september'     => 'ਸਤੰਬਰ',
'october'       => 'ਅਕਤੂਬਰ',
'november'      => 'ਨਵੰਬਰ',
'december'      => 'ਦਿਸੰਬਰ',
'january-gen'   => 'ਜਨਵਰੀ',
'february-gen'  => 'ਫ਼ਰਵਰੀ',
'march-gen'     => 'ਮਾਰਚ',
'april-gen'     => 'ਅਪ੍ਰੈਲ',
'may-gen'       => 'ਮਈ',
'june-gen'      => 'ਜੂਨ',
'july-gen'      => 'ਜੁਲਾਈ',
'august-gen'    => 'ਅਗਸਤ',
'september-gen' => 'ਸਤੰਬਰ',
'october-gen'   => 'ਅਕਤੂਬਰ',
'november-gen'  => 'ਨਵੰਬਰ',
'december-gen'  => 'ਦਿਸੰਬਰ',
'jan'           => 'ਜਨਵਰੀ',
'feb'           => 'ਫ਼ਰਵਰੀ',
'mar'           => 'ਮਾਰਚ',
'apr'           => 'ਅਪ੍ਰੈਲ',
'may'           => 'ਮਈ',
'jun'           => 'ਜੂਨ',
'jul'           => 'ਜੁਲਾਈ',
'aug'           => 'ਅਗਸਤ',
'sep'           => 'ਸਤੰਬਰ',
'oct'           => 'ਅਕਤੂਬਰ',
'nov'           => 'ਨਵੰਬਰ',
'dec'           => 'ਦਿਸੰਬਰ',

# Categories related messages
'pagecategories'        => '{{PLURAL:$1|ਕੈਟਾਗਰੀ|ਕੈਟਾਗਰੀਆਂ}}',
'category_header'       => 'ਕੈਟਾਗਰੀ "$1" ਵਿੱਚ ਲੇਖ',
'subcategories'         => 'ਸਬ-ਕੈਟਾਗਰੀਆਂ',
'category-media-header' => 'ਕੈਟਾਗਰੀ "$1" ਵਿੱਚ ਮੀਡਿਆ',
'category-empty'        => "''ਇਹ ਕੈਟਾਗਰੀ ਵਿੱਚ ਇਸ ਵੇਲੇ ਕੋਈ ਲੇਖ (ਆਰਟੀਕਲ) ਜਾਂ ਮੀਡਿਆ ਨਹੀਂ ਹੈ।''",

'mainpagetext' => "<big>'''ਮੀਡਿਆਵਿਕਿ ਠੀਕ ਤਰ੍ਹਾਂ ਇੰਸਟਾਲ ਹੋ ਗਿਆ ਹੈ।'''</big>",

'about'          => 'ਇਸ ਬਾਰੇ',
'article'        => 'ਸਮੱਗਰੀ ਪੇਜ',
'newwindow'      => '(ਨਵੀਂ ਵਿੰਡੋ ਵਿੱਚ ਖੋਲ੍ਹੋ)',
'cancel'         => 'ਰੱਦ ਕਰੋ',
'qbfind'         => 'ਖੋਜ',
'qbbrowse'       => 'ਬਰਾਊਜ਼',
'qbedit'         => 'ਸੋਧ',
'qbpageoptions'  => 'ਇਹ ਪੇਜ',
'qbpageinfo'     => 'ਭਾਗ',
'qbmyoptions'    => 'ਮੇਰੇ ਪੇਜ',
'qbspecialpages' => 'ਖਾਸ ਪੇਜ',
'moredotdotdot'  => 'ਹੋਰ...',
'mypage'         => 'ਮੇਰਾ ਪੇਜ',
'mytalk'         => 'ਮੇਰੀ ਗੱਲਬਾਤ',
'anontalk'       => 'ਇਹ IP ਲਈ ਗੱਲਬਾਤ',
'navigation'     => 'ਨੇਵੀਗੇਸ਼ਨ',
'and'            => 'ਅਤੇ',

'errorpagetitle'    => 'ਗਲਤੀ',
'returnto'          => '$1 ਤੇ ਵਾਪਸ ਜਾਓ',
'tagline'           => '{{SITENAME}} ਤੋਂ',
'help'              => 'ਮੱਦਦ',
'search'            => 'ਖੋਜ',
'searchbutton'      => 'ਖੋਜ',
'go'                => 'ਜਾਓ',
'searcharticle'     => 'ਜਾਓ',
'history'           => 'ਪੇਜ ਅਤੀਤ',
'history_short'     => 'ਅਤੀਤ',
'updatedmarker'     => 'ਮੇਰੇ ਆਖਰੀ ਖੋਲ੍ਹਣ ਬਾਦ ਅੱਪਡੇਟ',
'info_short'        => 'ਜਾਣਕਾਰੀ',
'printableversion'  => 'ਪਰਿੰਟਯੋਗ ਵਰਜਨ',
'permalink'         => 'ਪੱਕਾ ਲਿੰਕ',
'print'             => 'ਪਰਿੰਟ ਕਰੋ',
'edit'              => 'ਬਦਲੋ',
'editthispage'      => 'ਇਹ ਪੇਜ ਸੋਧੋ',
'delete'            => 'ਹਟਾਓ',
'deletethispage'    => 'ਇਹ ਪੇਜ ਹਟਾਓ',
'undelete_short'    => 'ਅਣ-ਹਟਾਓ {{PLURAL:$1|one edit|$1 edits}}',
'protect'           => 'ਸੁਰੱਖਿਆ',
'protect_change'    => 'ਸੁਰੱਖਿਆ ਬਦਲੋ',
'protectthispage'   => 'ਇਹ ਪੇਜ ਸੁਰੱਖਿਅਤ ਕਰੋ',
'unprotect'         => 'ਅਣ-ਸੁਰੱਖਿਅਤ',
'unprotectthispage' => 'ਇਹ ਪੇਜ ਅਣ-ਸੁਰੱਖਿਅਤ ਬਣਾਓ',
'newpage'           => 'ਨਵਾਂ ਪੇਜ',
'talkpage'          => 'ਇਸ ਪੇਜ ਬਾਰੇ ਚਰਚਾ',
'talkpagelinktext'  => 'ਗੱਲਬਾਤ',
'specialpage'       => 'ਖਾਸ ਪੇਜ',
'personaltools'     => 'ਨਿੱਜੀ ਟੂਲ',
'postcomment'       => 'ਇੱਕ ਟਿੱਪਣੀ ਦਿਓ',
'articlepage'       => 'ਸਮੱਗਰੀ ਪੇਜ ਵੇਖੋ',
'talk'              => 'ਚਰਚਾ',
'views'             => 'ਵੇਖੋ',
'toolbox'           => 'ਟੂਲਬਾਕਸ',
'userpage'          => 'ਯੂਜ਼ਰ ਪੇਜ ਵੇਖੋ',
'projectpage'       => 'ਪਰੋਜੈਕਟ ਪੇਜ ਵੇਖੋ',
'imagepage'         => 'ਚਿੱਤਰ ਪੇਜ ਵੇਖੋ',
'mediawikipage'     => 'ਸੁਨੇਹਾ ਪੇਜ ਵੇਖੋ',
'templatepage'      => 'ਟੈਪਲੇਟ ਪੇਜ ਵੇਖੋ',
'viewhelppage'      => 'ਮੱਦਦ ਪੇਜ ਵੇਖੋ',
'categorypage'      => 'ਕੈਟਾਗਰੀ ਪੇਜ ਵੇਖੋ',
'viewtalkpage'      => 'ਚਰਚਾ ਵੇਖੋ',
'otherlanguages'    => 'ਹੋਰ ਭਾਸ਼ਾਵਾਂ ਵਿੱਚ',
'redirectedfrom'    => '($1 ਤੋਂ ਰੀ-ਡਿਰੈਕਟ)',
'redirectpagesub'   => 'ਰੀ-ਡਿਰੈਕਟ ਪੇਜ',
'lastmodifiedat'    => 'ਇਹ ਪੇਜ ਆਖਰੀ ਵਾਰ $2, $1 ਨੂੰ ਸੋਧਿਆ ਗਿਆ ਸੀ।', # $1 date, $2 time
'viewcount'         => 'ਇਹ ਪੇਜ ਅਸੈੱਸ ਕੀਤਾ ਗਿਆ {{PLURAL:$1|ਇੱਕਵਾਰ|$1 ਵਾਰ}}.',
'protectedpage'     => 'ਸੁਰੱਖਿਅਤ ਪੇਜ',
'jumpto'            => 'ਜੰਪ ਕਰੋ:',
'jumptonavigation'  => 'ਨੇਵੀਗੇਸ਼ਨ',
'jumptosearch'      => 'ਖੋਜ',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} ਬਾਰੇ',
'aboutpage'            => 'Project:ਬਾਰੇ',
'bugreports'           => 'ਬੱਗ ਰਿਪੋਰਟਾਂ',
'bugreportspage'       => 'Project:ਬੱਗ ਰਿਪੋਰਟ',
'copyright'            => 'ਸਮੱਗਰੀ $1 ਹੇਠ ਉਪਲੱਬਧ ਹੈ।',
'copyrightpagename'    => '{{SITENAME}} ਕਾਪੀਰਾਈਟ',
'copyrightpage'        => '{{ns:project}}:ਕਾਪੀਰਾਈਟ',
'currentevents'        => 'ਮੌਜੂਦਾ ਇਵੈਂਟ',
'currentevents-url'    => 'Project:ਮੌਜੂਦਾ ਈਵੈਂਟ',
'disclaimers'          => 'ਦਾਆਵਾ',
'disclaimerpage'       => 'Project:ਆਮ ਡਿਕਲੇਅਮਰ',
'edithelp'             => 'ਮੱਦਦ ਐਡੀਟਿੰਗ',
'edithelppage'         => 'Help:ਐਡਟਿੰਗ',
'faq'                  => 'ਸਵਾਲ-ਜਵਾਬ',
'faqpage'              => 'Project:ਸਵਾਲ-ਜਵਾਬ',
'helppage'             => 'Help:ਸਮੱਗਰੀ',
'mainpage'             => 'ਮੁੱਖ ਪੇਜ',
'mainpage-description' => 'ਮੁੱਖ ਪੇਜ',
'policy-url'           => 'Project:ਪਾਲਸੀ',
'portal'               => 'ਕਮਿਊਨਟੀ ਪੋਰਟਲ',
'portal-url'           => 'Project:ਕਮਿਊਨਟੀ ਪੋਰਟਲ',
'privacy'              => 'ਪਰਾਈਵੇਸੀ ਪਾਲਸੀ',
'privacypage'          => 'Project:ਪਰਾਈਵੇਸ ਪੇਜ',

'badaccess'        => 'ਅਧਿਕਾਰ ਗਲਤੀ',
'badaccess-group0' => 'ਤੁਹਾਨੂੰ ਉਹ ਐਕਸ਼ਨ ਕਰਨ ਦੀ ਮਨਜ਼ੂਰੀ ਨਹੀਂ, ਜਿਸ ਦੀ ਤੁਸੀਂ ਮੰਗ ਕੀਤੀ ਹੈ।',

'ok'                      => 'ਠੀਕ ਹੈ',
'retrievedfrom'           => '"$1" ਤੋਂ ਲਿਆ',
'youhavenewmessages'      => 'ਤੁਹਾਨੂੰ $1 ($2).',
'newmessageslink'         => 'ਨਵੇਂ ਸੁਨੇਹੇ',
'newmessagesdifflink'     => 'ਆਖਰੀ ਬਦਲਾਅ',
'youhavenewmessagesmulti' => 'ਤੁਹਾਨੂੰ ਨਵੇਂ ਸੁਨੇਹੇ $1 ਉੱਤੇ ਹਨ',
'editsection'             => 'ਸੋਧ',
'editold'                 => 'ਸੋਧ',
'editsectionhint'         => 'ਸ਼ੈਕਸ਼ਨ ਸੋਧ: $1',
'toc'                     => 'ਸਮਗੱਰੀ',
'showtoc'                 => 'ਵੇਖਾਓ',
'hidetoc'                 => 'ਓਹਲੇ',
'thisisdeleted'           => 'ਵੇਖੋ ਜਾਂ $1 ਰੀਸਟੋਰ?',
'viewdeleted'             => '$1 ਵੇਖਣਾ?',
'feedlinks'               => 'ਫੀਡ:',
'red-link-title'          => '$1 (ਹੁਣ ਤਕ ਨਹੀਂ ਲਿਖਿਆ ਗਿਆ)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'ਲੇਖ',
'nstab-user'      => 'ਯੂਜ਼ਰ ਪੇਜ',
'nstab-media'     => 'ਮੀਡਿਆ ਪੇਜ',
'nstab-special'   => 'ਖਾਸ',
'nstab-project'   => 'ਪਰੋਜੈਕਟ ਪੇਜ',
'nstab-image'     => 'ਫਾਇਲ',
'nstab-mediawiki' => 'ਸੁਨੇਹਾ',
'nstab-template'  => 'ਟੈਪਲੇਟ',
'nstab-help'      => 'ਮੱਦਦ ਪੇਜ',
'nstab-category'  => 'ਕੈਟਾਗਰੀ',

# Main script and global functions
'nosuchaction'      => 'ਕੋਈ ਇੰਝ ਦਾ ਐਕਸ਼ਨ ਨਹੀਂ',
'nosuchspecialpage' => 'ਕੋਈ ਇੰਝ ਦਾ ਖਾਸ ਪੇਜ ਨਹੀਂ',
'nospecialpagetext' => "<big>'''ਤੁਸੀਂ ਇੱਕ ਅਵੈਧ ਖਾਸ ਪੇਜ ਦੀ ਮੰਗ ਕੀਤੀ ਹੈ।'''</big>

A list of valid special pages can be found at [[Special:SpecialPages]].",

# General errors
'error'              => 'ਗਲਤੀ',
'databaseerror'      => 'ਡਾਟਾਬੇਸ ਗਲਤੀ',
'nodb'               => 'ਡਾਟਾਬੇਸ $1 ਚੁਣਿਆ ਨਹੀਂ ਜਾ ਸਕਦਾ',
'readonly'           => 'ਡਾਟਾਬੇਸ ਲਾਕ ਹੈ',
'internalerror'      => 'ਅੰਦਰੂਨੀ ਗਲਤੀ',
'internalerror_info' => 'ਅੰਦਰੂਨੀ ਗਲਤੀ: $1',
'badtitle'           => 'ਗਲਤ ਟਾਇਟਲ',
'viewsource'         => 'ਸਰੋਤ ਵੇਖੋ',
'viewsourcefor'      => '$1 ਲਈ',

# Login and logout pages
'logouttitle'                => 'ਯੂਜ਼ਰ ਲਾਗਆਉਟ',
'logouttext'                 => '<strong>ਹੁਣ ਤੁਸੀਂ ਲਾਗਆਉਟ ਹੋ ਗਏ ਹੋ।</strong><br />
You can continue to use {{SITENAME}} anonymously, or you can log in
again as the same or as a different user. Note that some pages may
continue to be displayed as if you were still logged in, until you clear
your browser cache.',
'welcomecreation'            => "== ਜੀ ਆਇਆਂ ਨੂੰ, $1! ==

Your account has been created. Don't forget to change your {{SITENAME}} preferences.",
'loginpagetitle'             => 'ਯੂਜ਼ਰ ਲਾਗਇਨ',
'yourname'                   => 'ਯੂਜ਼ਰ ਨਾਂ:',
'yourpassword'               => 'ਪਾਸਵਰਡ:',
'yourpasswordagain'          => 'ਪਾਸਵਰਡ ਮੁੜ-ਲਿਖੋ:',
'remembermypassword'         => 'ਇਹ ਕੰਪਿਊਟਰ ਲਈ ਆਪਣਾ ਲਾਗਇਨ ਯਾਦ ਰੱਖੋ',
'yourdomainname'             => 'ਤੁਹਾਡੀ ਡੋਮੇਨ:',
'loginproblem'               => '<b>ਤੁਹਾਡੇ ਲਾਗਇਨ ਨਾਲ ਇੱਕ ਸਮੱਸਿਆ ਹੈ।</b><br />ਮੁੜ ਕੋਸ਼ਿਸ਼ ਕਰੋ!',
'login'                      => 'ਲਾਗ ਇਨ',
'nav-login-createaccount'    => 'ਲਾਗ ਇਨ / ਅਕਾਊਂਟ ਬਣਾਓ',
'loginprompt'                => 'ਤੁਹਾਨੂੰ {{SITENAME}} ਉੱਤੇ ਲਾਗਇਨ ਕਰਨ ਲਈ ਕੂਕੀਜ਼ ਯੋਗ ਕਰਨੇ ਜ਼ਰੂਰੀ ਹਨ।',
'userlogin'                  => 'ਲਾਗ ਇਨ / ਅਕਾਊਂਟ ਬਣਾਓ',
'logout'                     => 'ਲਾਗ ਆਉਟ',
'userlogout'                 => 'ਲਾਗ ਆਉਟ',
'notloggedin'                => 'ਲਾਗਇਨ ਨਹੀਂ',
'nologin'                    => 'ਲਾਗਇਨ ਨਹੀਂ ਹੈ? $1.',
'nologinlink'                => 'ਇੱਕ ਅਕਾਊਂਟ ਬਣਾਓ',
'createaccount'              => 'ਅਕਾਊਂਟ ਬਣਾਓ',
'gotaccount'                 => 'ਪਹਿਲਾਂ ਹੀ ਇੱਕ ਅਕਾਊਂਟ ਹੈ? $1.',
'gotaccountlink'             => 'ਲਾਗਇਨ',
'createaccountmail'          => 'ਈਮੇਲ ਨਾਲ',
'badretype'                  => 'ਤੁਹਾਡੇ ਵਲੋਂ ਦਿੱਤੇ ਪਾਸਵਰਡ ਮਿਲਦੇ ਨਹੀਂ ਹਨ।',
'userexists'                 => 'ਯੂਜ਼ਰ, ਜੋ ਦਿੱਤਾ ਹੈ, ਪਹਿਲਾਂ ਹੀ ਵਰਤਿਆ ਜਾ ਰਿਹਾ ਹੈ। ਇੱਕ ਵੱਖਰਾ ਨਾਂ ਚੁਣੋ ਜੀ।',
'youremail'                  => 'ਈਮੇਲ:',
'username'                   => 'ਯੂਜ਼ਰ ਨਾਂ:',
'uid'                        => 'ਯੂਜ਼ਰ ID:',
'yourrealname'               => 'ਅਸਲੀ ਨਾਂ:',
'yourlanguage'               => 'ਭਾਸ਼ਾ:',
'yournick'                   => 'ਛੋਟਾ ਨਾਂ:',
'badsiglength'               => 'ਛੋਟਾ ਨਾਂ (Nickname) ਬਹੁਤ ਲੰਮਾ ਹੋ ਗਿਆ ਹੈ, ਇਹ $1 ਅੱਖਰਾਂ ਤੋਂ ਘੱਟ ਚਾਹੀਦਾ ਹੈ।',
'email'                      => 'ਈਮੇਲ',
'prefs-help-realname'        => 'ਅਸਲੀ ਨਾਂ ਚੋਣਵਾਂ ਹੈ, ਅਤੇ ਜੇ ਤੁਸੀਂ ਇਹ ਦਿੱਤਾ ਹੈ ਤਾਂ ਤੁਹਾਡੇ ਕੰਮ ਵਾਸਤੇ ਗੁਣ ਦੇ ਤੌਰ ਉੱਤੇ ਵਰਤਿਆ ਜਾਵੇਗਾ।',
'loginerror'                 => 'ਲਾਗਇਨ ਗਲਤੀ',
'prefs-help-email'           => 'ਈਮੇਲ ਐਡਰੈੱਸ ਚੋਣਵਾਂ ਹੈ, ਪਰ ਇਹ ਤੁਹਾਨੂੰ ਹੋਰਾਂ ਵਲੋਂ ਤੁਹਾਡੇ ਨਾਲ ਤੁਹਾਡੇ ਯੂਜ਼ਰ ਜਾਂ ਯੂਜ਼ਰ_ਗੱਲਬਾਤ ਰਾਹੀਂ ਬਿਨਾਂ ਤੁਹਾਡੇ ਪਛਾਣ ਦੇ ਸੰਪਰਕ ਲਈ ਮੱਦਦ ਦਿੰਦਾ ਹੈ।',
'nocookiesnew'               => 'ਯੂਜ਼ਰ ਅਕਾਊਂਟ ਬਣਾਇਆ ਗਿਆ ਹੈ, ਪਰ ਤੁਸੀਂ ਲਾਗਇਨ ਨਹੀਂ ਕੀਤਾ ਹੈ।{{SITENAME}} uses cookies to log in users. You have cookies disabled. Please enable them, then log in with your new username and password.',
'nocookieslogin'             => '{{SITENAME}} ਯੂਜ਼ਰਾਂ ਨੂੰ ਲਾਗਇਨ ਕਰਨ ਲਈ ਕੂਕੀਜ਼ ਵਰਤਦੀ ਹੈ। ਤੁਹਾਡੇ ਕੂਕੀਜ਼ ਆਯੋਗ ਕੀਤੇ ਹੋਏ ਹਨ। ਉਨ੍ਹਾਂ ਨੂੰ ਯੋਗ ਕਰਕੇ ਮੁੜ ਟਰਾਈ ਕਰੋ।',
'noname'                     => 'ਤੁਸੀਂ ਇੱਕ ਵੈਧ ਯੂਜ਼ਰ ਨਾਂ ਨਹੀਂ ਦਿੱਤਾ ਹੈ।',
'loginsuccesstitle'          => 'ਲਾਗਇਨ ਸਫ਼ਲ ਰਿਹਾ',
'loginsuccess'               => "'''ਤੁਸੀਂ {{SITENAME}} ਉੱਤੇ \"\$1\" ਵਾਂਗ ਲਾਗਇਨ ਕਰ ਚੁੱਕੇ ਹੋ।'''",
'nosuchuser'                 => '"$1" ਨਾਂ ਨਾਲ ਕੋਈ ਯੂਜ਼ਰ ਨਹੀਂ ਹੈ। ਆਪਣੇ ਸ਼ਬਦ ਧਿਆਨ ਨਾਲ ਚੈੱਕ ਕਰੋ ਜਾਂ ਨਵਾਂ ਅਕਾਊਂਟ ਬਣਾਓ।',
'nosuchusershort'            => '"<nowiki>$1</nowiki>" ਨਾਂ ਨਾਲ ਕੋਈ ਵੀ ਯੂਜ਼ਰ ਨਹੀਂ ਹੈ। ਆਪਣੇ ਸ਼ਬਦ ਧਿਆਨ ਨਾਲ ਚੈੱਕ ਕਰੋ।',
'nouserspecified'            => 'ਤੁਹਾਨੂੰ ਇੱਕ ਯੂਜ਼ਰ-ਨਾਂ ਦੇਣਾ ਪਵੇਗਾ।',
'wrongpassword'              => 'ਗਲਤ ਪਾਸਵਰਡ ਦਿੱਤਾ ਹੈ। ਮੁੜ-ਟਰਾਈ ਕਰੋ ਜੀ।',
'wrongpasswordempty'         => 'ਖਾਲੀ ਪਾਸਵਰਡ ਦਿੱਤਾ ਹੈ। ਮੁੜ-ਟਰਾਈ ਕਰੋ ਜੀ।',
'passwordtooshort'           => 'ਤੁਹਾਡਾ ਪਾਸਵਰਡ ਗਲਤ ਹੈ ਜਾਂ ਬਹੁਤ ਛੋਟਾ ਹੈ। ਇਸ ਵਿੱਚ ਘੱਟੋ-ਘੱਟ$1 ਅੱਖਰ ਹੋਣੇ ਚਾਹੀਦੇ ਹਨ ਅਤੇ ਤੁਹਾਡੇ ਯੂਜ਼ਰ ਨਾਂ ਤੋਂ ਵੱਖਰਾ ਹੋਣਾ ਚਾਹੀਦਾ ਹੈ।',
'mailmypassword'             => 'ਪਾਸਵਰਡ ਈਮੇਲ ਕਰੋ',
'passwordremindertitle'      => '{{SITENAME}} ਲਈ ਪਾਸਵਰਡ ਯਾਦ ਰੱਖੋ',
'passwordremindertext'       => 'ਕਿਸੇ ਨੇ (ਸ਼ਾਇਦ ਤੁਸੀਂ, IP ਐਡਰੈੱਸ $1 ਤੋਂ)
ਮੰਗ ਕੀਤੀ ਸੀ ਕਿ ਅਸੀਂ ਤੁਹਾਨੂੰ {{SITENAME}} ($4) ਲਈ ਪਾਸਵਰਡ ਭੇਜੀਏ।
ਯੂਜ਼ਰ "$2" ਲਈ ਹੁਣ ਪਾਸਵਰਡ "$3" ਹੈ।
ਤੁਹਾਨੂੰ ਹੁਣ ਲਾਗਇਨ ਕਰਕੇ ਆਪਣਾ ਪਾਸਵਰਡ ਹੁਣੇ ਬਦਲਣਾ ਚਾਹੀਦਾ ਹੈ।

If someone else made this request or if you have remembered your password and
you no longer wish to change it, you may ignore this message and continue using
your old password.',
'noemail'                    => 'ਯੂਜ਼ਰ "$1" ਲਈ ਰਿਕਾਰਡ ਵਿੱਚ ਕੋਈ ਈਮੇਲ ਐਡਰੈੱਸ ਨਹੀਂ ਹੈ।',
'passwordsent'               => '"$1" ਨਾਲ ਰਜਿਸਟਰ ਕੀਤੇ ਈਮੇਲ ਐਡਰੈੱਸ ਉੱਤੇ ਈਮੇਲ ਭੇਜੀ ਗਈ ਹੈ।
ਇਹ ਮਿਲ ਦੇ ਬਾਅਦ ਮੁੜ ਲਾਗਇਨ ਕਰੋ ਜੀ।',
'throttled-mailpassword'     => 'ਇੱਕ ਪਾਸਵਰਡ ਰੀਮਾਈਡਰ ਪਹਿਲਾਂ ਹੀ ਭੇਜਿਆ ਗਿਆ ਹੈ, ਆਖਰੀ
$1 ਘੰਟੇ ਵਿੱਚ। ਨੁਕਸਾਨ ਤੋਂ ਬਚਣ ਲਈ, $1 ਘੰਟਿਆਂ ਵਿੱਚ ਇੱਕ ਹੀ ਪਾਸਵਰਡ ਰੀਮਾਈਡਰ ਭੇਜਿਆ ਜਾਂਦਾ ਹੈ।',
'mailerror'                  => 'ਈਮੇਲ ਭੇਜਣ ਦੌਰਾਨ ਗਲਤੀ: $1',
'acct_creation_throttle_hit' => 'ਅਫਸੋਸ ਹੈ, ਪਰ ਤੁਸੀਂ ਪਹਿਲਾਂ ਹੀ $1 ਅਕਾਊਂਟ ਬਣਾ ਚੁੱਕੇ ਹੋ। ਤੁਸੀਂ ਹੋਰ ਨਹੀਂ ਬਣਾ ਸਕਦੇ।',
'emailauthenticated'         => 'ਤੁਹਾਡਾ ਈਮੇਲ ਐਡਰੈੱਸ $1 ਉੱਤੇ ਪਰਮਾਣਿਤ ਕੀਤਾ ਗਿਆ ਹੈ।',
'emailnotauthenticated'      => 'ਤੁਹਾਡਾ ਈਮੇਲ ਐਡਰੈੱਸ ਹਾਲੇ ਪਰਮਾਣਿਤ ਨਹੀਂ ਹੈ। ਹੇਠ ਦਿੱਤੇ ਫੀਚਰਾਂ ਲਈ ਕੋਈ ਵੀ ਈਮੇਲ ਨਹੀਂ ਭੇਜੀ ਜਾਵੇਗੀ।',
'noemailprefs'               => 'ਇਹ ਫੀਚਰ ਵਰਤਣ ਲਈ ਇੱਕ ਈਮੇਲ ਐਡਰੈੱਸ ਦਿਓ।।',
'emailconfirmlink'           => 'ਆਪਣਾ ਈ-ਮੇਲ ਐਡਰੈੱਸ ਕਨਫਰਮ ਕਰੋ।',
'invalidemailaddress'        => 'ਈਮੇਲ ਐਡਰੈੱਸ ਮਨਜ਼ੂਰ ਨਹੀਂ ਕੀਤਾ ਜਾ ਸਕਦਾ ਹੈ ਕਿਉਂਕਿ ਇਹ ਠੀਕ ਫਾਰਮੈਟ ਨਹੀਂ ਜਾਪਦਾ ਹੈ। ਇੱਕ ਠੀਕ ਫਾਰਮੈਟ ਵਿੱਚ ਦਿਓ ਜਾਂ ਇਹ ਖੇਤਰ ਖਾਲੀ ਛੱਡ ਦਿਓ।',
'accountcreated'             => 'ਅਕਾਊਂਟ ਬਣਾਇਆ',
'accountcreatedtext'         => '$1 ਲਈ ਯੂਜ਼ਰ ਅਕਾਊਂਟ ਬਣਾਇਆ ਗਿਆ।',
'loginlanguagelabel'         => 'ਭਾਸ਼ਾ: $1',

# Password reset dialog
'resetpass'               => 'ਅਕਾਊਂਟ ਪਾਸਵਰਡ ਰੀ-ਸੈੱਟ ਕਰੋ',
'resetpass_announce'      => 'ਤੁਸੀਂ ਇੱਕ ਆਰਜ਼ੀ ਈ-ਮੇਲ ਕੀਤੇ ਕੋਡ ਨਾਲ ਲਾਗਇਨ ਕੀਤਾ ਹੈ। ਲਾਗਇਨ ਪੂਰਾ ਕਰਨ ਲਈ, ਤੁਹਾਨੂੰ ਇੱਥੇ ਨਵਾਂ ਪਾਸਵਰਡ ਦੇਣਾ ਪਵੇਗਾ:',
'resetpass_header'        => 'ਪਾਸਵਰਡ ਰੀ-ਸੈੱਟ ਕਰੋ',
'resetpass_submit'        => 'ਪਾਸਵਰਡ ਸੈੱਟ ਕਰੋ ਅਤੇ ਲਾਗਇਨ ਕਰੋ',
'resetpass_success'       => 'ਤੁਹਾਡਾ ਪਾਸਵਰਡ ਠੀਕ ਤਰਾਂ ਬਦਲਿਆ ਗਿਆ ਹੈ! ਹੁਣ ਤੁਸੀਂ ਲਾਗਇਨ ਕਰ ਸਕਦੇ ਹੋ...',
'resetpass_bad_temporary' => 'ਗਲਤ ਆਰਜ਼ੀ ਪਾਸਵਰਡ ਹੈ। ਤੁਸੀਂ ਸ਼ਾਇਦ ਪਹਿਲਾਂ ਹੀ ਆਪਣਾ ਪਾਸਵਰਡ ਬਦਲ ਚੁੱਕੇ ਹੋ ਜਾਂ ਇੱਕ ਨਵੇਂ ਆਰਜ਼ੀ ਪਾਸਵਰਡ ਦੀ ਮੰਗ ਭੇਜੀ ਹੈ।',
'resetpass_forbidden'     => 'ਇਹ ਵਿਕਿ ਲਈ ਪਾਸਵਰਡ ਬਦਲਿਆ ਨਹੀਂ ਜਾ ਸਕਦਾ।',
'resetpass_missing'       => 'ਕੋਈ ਫਾਰਮ ਡਾਟਾ ਨਹੀਂ।',

# Edit page toolbar
'bold_sample'     => 'ਬੋਲਡ ਟੈਕਸਟ',
'bold_tip'        => 'ਬੋਲਡ ਟੈਕਸਟ',
'italic_sample'   => 'ਤਿਰਛਾ ਟੈਕਸਟ',
'italic_tip'      => 'ਤਿਰਛਾ ਟੈਕਸਟ',
'link_sample'     => 'ਲਿੰਕ ਟਾਇਟਲ',
'link_tip'        => 'ਅੰਦਰੂਨੀ ਲਿੰਕ',
'headline_sample' => 'ਹੈੱਡਲਾਈਣ ਟੈਕਸਟ',
'math_tip'        => 'ਗਣਿਤ ਫਾਰਮੂਲਾ (LaTeX)',
'image_tip'       => 'ਇੰਬੈੱਡ ਚਿੱਤਰ',
'media_tip'       => 'ਮੀਡਿਆ ਫਾਇਲ ਲਿੰਕ',
'sig_tip'         => 'ਟਾਈਮ-ਸਟੈਂਪ ਨਾਲ ਤੁਹਾਡੇ ਦਸਤਖਤ',
'hr_tip'          => 'ਹਾਰੀਜ਼ਟਲ ਲਾਈਨ (use sparingly)',

# Edit pages
'summary'                => 'ਸੰਖੇਪ',
'subject'                => 'ਵਿਸ਼ਾ/ਹੈੱਡਲਾਈਨ',
'minoredit'              => 'ਇਹ ਛੋਟੀ ਸੋਧ ਹੈ',
'watchthis'              => 'ਇਹ ਪੇਜ ਵਾਚ ਕਰੋ',
'savearticle'            => 'ਪੇਜ ਸੰਭਾਲੋ',
'preview'                => 'ਝਲਕ',
'showpreview'            => 'ਝਲਕ ਵੇਖੋ',
'showlivepreview'        => 'ਲਾਈਵ ਝਲਕ',
'showdiff'               => 'ਬਦਲਾਅ ਵੇਖਾਓ',
'anoneditwarning'        => "'''ਚੇਤਾਵਨੀ:''' ਤੁਸੀਂ ਲਾਗਇਨ ਨਹੀਂ ਕੀਤਾ ਹੈ। ਤੁਹਾਡਾ IP ਐਡਰੈੱਸ ਇਸ ਪੇਜ ਦੇ ਐਡਿਟ ਅਤੀਤ ਵਿੱਚ ਰਿਕਾਰਡ ਕੀਤਾ ਜਾਵੇਗਾ।",
'missingcommenttext'     => 'ਹੇਠਾਂ ਇੱਕ ਟਿੱਪਣੀ ਦਿਓ।',
'summary-preview'        => 'ਸੰਖੇਪ ਝਲਕ',
'subject-preview'        => 'ਵਿਸ਼ਾ/ਹੈੱਡਲਾਈਨ ਝਲਕ',
'blockedtitle'           => 'ਯੂਜ਼ਰ ਬਲਾਕ ਕੀਤਾ ਗਿਆ',
'whitelistedittitle'     => 'ਸੋਧਣ ਲਈ ਲਾਗਇਨ ਕਰਨਾ ਪਵੇਗਾ',
'whitelistedittext'      => 'ਪੇਜ ਸੋਧਣ ਲਈ ਤੁਹਾਨੂੰ $1 ਕਰਨਾ ਪਵੇਗਾ।',
'confirmedittitle'       => 'ਐਡੀਟ ਕਰਨ ਲਈ ਈਮੇਲ ਕਨਫਰਮੇਸ਼ਨ ਦੀ ਲੋੜ ਹੈ',
'nosuchsectiontitle'     => 'ਇੰਝ ਦਾ ਕੋਈ ਸ਼ੈਕਸ਼ਨ ਨਹੀਂ ਹੈ।',
'loginreqtitle'          => 'ਲਾਗਇਨ ਚਾਹੀਦਾ ਹੈ',
'loginreqlink'           => 'ਲਾਗਇਨ',
'loginreqpagetext'       => 'ਹੋਰ ਪੇਜ ਵੇਖਣ ਲਈ ਤੁਹਾਨੂੰ $1 ਕਰਨਾ ਪਵੇਗਾ।',
'accmailtitle'           => 'ਪਾਸਵਰਡ ਭੇਜਿਆ।',
'accmailtext'            => '"$1" ਲਈ ਪਾਸਵਰਡ $2 ਨੂੰ ਭੇਜਿਆ ਗਿਆ।',
'newarticle'             => '(ਨਵਾਂ)',
'updated'                => '(ਅੱਪਡੇਟ)',
'note'                   => '<strong>ਨੋਟ:</strong>',
'previewnote'            => '<strong>ਇਹ ਸਿਰਫ਼ ਇੱਕ ਝਲਕ ਹੈ; ਬਦਲਾਅ ਹਾਲੇ ਸੰਭਾਲੇ ਨਹੀਂ ਗਏ ਹਨ!</strong>',
'editing'                => '$1 ਸੋਧਿਆ ਜਾ ਰਿਹਾ ਹੈ',
'editingsection'         => '$1 (ਸ਼ੈਕਸ਼ਨ) ਸੋਧ',
'editingcomment'         => '$1 (ਟਿੱਪਣੀ) ਸੋਧ',
'editconflict'           => 'ਅਪਵਾਦ ਟਿੱਪਣੀ: $1',
'yourtext'               => 'ਤੁਹਾਡਾ ਟੈਕਸਟ',
'storedversion'          => 'ਸੰਭਾਲਿਆ ਵਰਜਨ',
'yourdiff'               => 'ਅੰਤਰ',
'templatesused'          => 'ਇਸ ਪੇਜ ਉੱਤੇ ਟੈਪਲੇਟ ਵਰਤਿਆ ਜਾਂਦਾ ਹੈ:',
'templatesusedpreview'   => 'ਇਹ ਝਲਕ ਵਿੱਚ ਟੈਪਲੇਟ ਵਰਤਿਆ ਜਾਂਦਾ ਹੈ:',
'templatesusedsection'   => 'ਇਹ ਸ਼ੈਕਸ਼ਨ ਵਿੱਚ ਟੈਪਲੇਟ ਵਰਤਿਆ ਜਾਂਦਾ ਹੈ:',
'template-protected'     => '(ਸੁਰੱਖਿਅਤ)',
'template-semiprotected' => '(ਸੈਮੀ-ਸੁਰਖਿਅਤ)',
'permissionserrors'      => 'ਅਧਿਕਾਰ ਗਲਤੀਆਂ',
'permissionserrorstext'  => 'ਤੁਹਾਨੂੰ ਇੰਝ ਕਰਨ ਦੇ ਅਧਿਕਾਰ ਨਹੀਂ ਹਨ। ਹੇਠ ਦਿੱਤੇ {{PLURAL:$1|ਕਾਰਨ|ਕਾਰਨ}} ਨੇ:',

# Account creation failure
'cantcreateaccounttitle' => 'ਅਕਾਊਂਟ ਬਣਾਇਆ ਨਹੀਂ ਜਾ ਸਕਦਾ',

# History pages
'viewpagelogs'        => 'ਇਸ ਪੇਜ ਦੇ ਲਈ ਲਾਗ ਵੇਖੋ',
'revnotfound'         => 'ਰੀਵਿਜ਼ਨ ਨਹੀਂ ਲੱਭਿਆ',
'currentrev'          => 'ਮੌਜੂਦਾ ਰੀਵਿਜ਼ਨ',
'revisionasof'        => '$1 ਦੇ ਰੀਵਿਜ਼ਨ ਵਾਂਗ',
'previousrevision'    => '←ਪੁਰਾਣਾ ਰੀਵਿਜ਼ਨ',
'nextrevision'        => 'ਨਵਾਂ ਰੀਵਿਜ਼ਨ→',
'currentrevisionlink' => 'ਮੌਜੂਦਾ ਰੀਵਿਜ਼ਨ',
'cur'                 => 'ਮੌਜੂਦਾ',
'next'                => 'ਅੱਗੇ',
'last'                => 'ਆਖਰੀ',
'page_first'          => 'ਪਹਿਲਾਂ',
'page_last'           => 'ਆਖਰੀ',
'deletedrev'          => '[ਹਟਾਇਆ]',
'histfirst'           => 'ਸਭ ਤੋਂ ਪਹਿਲਾਂ',
'histlast'            => 'ਸਭ ਤੋਂ ਨਵਾਂ',
'historysize'         => '($1 ਬਾਈਟ)',
'historyempty'        => '(ਖਾਲੀ)',

# Revision feed
'history-feed-title' => 'ਰੀਵਿਜ਼ਨ ਅਤੀਤ',

# Revision deletion
'rev-deleted-comment'     => '(ਟਿੱਪਣੀ ਹਟਾਈ)',
'rev-deleted-user'        => '(ਯੂਜ਼ਰ ਨਾਂ ਹਟਾਇਆ)',
'rev-deleted-event'       => '(ਐਂਟਰੀ ਹਟਾਈ)',
'rev-delundel'            => 'ਵੇਖਾਓ/ਓਹਲੇ',
'revdelete-nooldid-title' => 'ਕੋਈ ਟਾਰਗੇਟ ਰੀਵਿਜ਼ਨ ਨਹੀਂ',
'revdelete-legend'        => 'ਪਾਬੰਦੀਆਂ ਸੈੱਟ ਕਰੋ:',
'revdelete-hide-text'     => 'ਰੀਵਿਜ਼ਨ ਟੈਕਸਟ ਓਹਲੇ',
'revdelete-hide-name'     => 'ਐਕਸ਼ਨ ਅਤੇ ਟਾਰਗੇਟ ਓਹਲੇ',
'revdelete-hide-image'    => 'ਫਾਇਲ ਸਮੱਗਰੀ ਓਹਲੇ',
'revdelete-log'           => 'ਲਾਗ ਟਿੱਪਣੀ:',
'revdelete-submit'        => 'ਚੁਣੇ ਰੀਵਿਜ਼ਨ ਉੱਤੇ ਲਾਗੂ ਕਰੋ',
'pagehist'                => 'ਪੇਜ ਦਾ ਅਤੀਤ',
'deletedhist'             => 'ਹਟਾਇਆ ਗਿਆ ਅਤੀਤ',

# Diffs
'difference'              => '(ਰੀਵਿਜ਼ਨ ਵਿੱਚ ਅੰਤਰ)',
'lineno'                  => 'ਲਾਈਨ $1:',
'compareselectedversions' => 'ਚੁਣੇ ਵਰਜਨਾਂ ਦੀ ਤੁਲਨਾ',
'editundo'                => 'ਵਾਪਸ(undo)',

# Search results
'searchresults'         => 'ਖੋਜ ਨਤੀਜੇ',
'searchresulttext'      => '{{SITENAME}} ਖੋਜ ਬਾਰੇ ਹੋਰ ਜਾਣਕਾਰੀ ਲਵੋ, ਵੇਖੋ [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => "ਤੁਸੀਂ '''[[:$1]]''' ਲਈ ਖੋਜ ਕੀਤੀ।",
'searchsubtitleinvalid' => "ਤੁਸੀਂ'''$1''' ਲਈ ਖੋਜ ਕੀਤੀ।",
'noexactmatch'          => "'''\"\$1\"''' ਟਾਇਟਲ ਨਾਲ ਦਾ ਕੋਈ ਪੇਜ ਨਹੀਂ ਹੈ। ਤੁਸੀਂ [[:\$1|ਇਹ ਪੇਜ]] ਬਣਾ ਸਕਦੇ ਹੋ।",
'titlematches'          => 'ਆਰਟੀਕਲ ਟੈਕਸਟ ਮਿਲਦਾ',
'notitlematches'        => 'ਕੋਈ ਪੇਜ ਟਾਇਟਲ ਨਹੀਂ ਮਿਲਦਾ',
'textmatches'           => 'ਪੇਜ ਟੈਕਸਟ ਮਿਲਦਾ',
'notextmatches'         => 'ਕੋਈ ਪੇਜ ਟੈਕਸਟ ਨਹੀਂ ਮਿਲਦਾ',
'prevn'                 => 'ਪਿੱਛੇ $1',
'nextn'                 => 'ਅੱਗੇ $1',
'viewprevnext'          => 'ਵੇਖੋ ($1) ($2) ($3)',
'searchall'             => 'ਸਭ',
'powersearch'           => 'ਖੋਜ',

# Preferences page
'preferences'           => 'ਮੇਰੀ ਪਸੰਦ',
'mypreferences'         => 'ਮੇਰੀ ਪਸੰਦ',
'prefs-edits'           => 'ਸੋਧਾਂ ਦੀ ਗਿਣਤੀ:',
'prefsnologin'          => 'ਲਾਗਇਨ ਨਹੀਂ',
'prefsnologintext'      => 'ਯੂਜ਼ਰ ਪਸੰਦ ਸੈੱਟ ਕਰਨ ਲਈ ਤੁਹਾਨੂੰ [[Special:UserLogin|logged in]] ਕਰਨਾ ਪਵੇਗਾ।',
'prefsreset'            => 'ਪਸੰਦੀ ਸਟੋਰੇਜ਼ ਤੋਂ ਮੁੜ-ਸੈੱਟ ਕੀਤੀ ਗਈ ਹੈ।',
'qbsettings'            => 'ਤੁਰੰਤ ਬਾਰ',
'qbsettings-none'       => 'ਕੋਈ ਨਹੀਂ',
'changepassword'        => 'ਪਾਸਵਰਡ ਬਦਲੋ',
'skin'                  => 'ਸਕਿਨ',
'math'                  => 'ਗਣਿਤ',
'dateformat'            => 'ਮਿਤੀ ਫਾਰਮੈਟ',
'datedefault'           => 'ਕੋਈ ਪਸੰਦ ਨਹੀਂ',
'datetime'              => 'ਮਿਤੀ ਅਤੇ ਸਮਾਂ',
'math_failure'          => 'ਪਾਰਸ ਕਰਨ ਲਈ ਫੇਲ੍ਹ',
'math_unknown_error'    => 'ਅਣਜਾਣ ਗਲਤੀ',
'math_unknown_function' => 'ਅਣਜਾਣ ਫੰਕਸ਼ਨ',
'math_lexing_error'     => 'lexing ਗਲਤੀ',
'math_syntax_error'     => 'ਸੰਟੈਕਸ ਗਲਤੀ',
'prefs-personal'        => 'ਯੂਜ਼ਰ ਪਰੋਫਾਇਲ',
'prefs-rc'              => 'ਤਾਜ਼ਾ ਬਦਲਾਅ',
'prefs-watchlist'       => 'ਵਾਚ-ਲਿਸਟ',
'prefs-misc'            => 'ਫੁਟਕਲ',
'saveprefs'             => 'ਸੰਭਾਲੋ',
'resetprefs'            => 'ਰੀ-ਸੈੱਟ',
'oldpassword'           => 'ਪੁਰਾਣਾ ਪਾਸਵਰਡ:',
'newpassword'           => 'ਨਵਾਂ ਪਾਸਵਰਡ:',
'retypenew'             => 'ਨਵਾਂ ਪਾਸਵਰਡ ਮੁੜ-ਲਿਖੋ:',
'textboxsize'           => 'ਸੰਪਾਦਨ',
'rows'                  => 'ਕਤਾਰਾਂ:',
'columns'               => 'ਕਾਲਮ:',
'searchresultshead'     => 'ਖੋਜ',
'resultsperpage'        => 'ਪ੍ਰਤੀ ਪੇਜ ਹਿੱਟ:',
'savedprefs'            => 'ਤੁਹਾਡੀ ਪਸੰਦ ਸੰਭਾਲੀ ਗਈ ਹੈ।',
'timezonelegend'        => 'ਟਾਈਮ ਜ਼ੋਨ',
'localtime'             => 'ਲੋਕਲ ਟਾਈਮ',
'servertime'            => 'ਸਰਵਰ ਟਾਈਮ',
'guesstimezone'         => 'ਬਰਾਊਜ਼ਰ ਤੋਂ ਭਰੋ',
'allowemail'            => 'ਹੋਰ ਯੂਜ਼ਰਾਂ ਤੋਂ ਈਮੇਲ ਯੋਗ ਕਰੋ',
'default'               => 'ਡਿਫਾਲਟ',
'files'                 => 'ਫਾਇਲਾਂ',

# User rights
'userrights-lookup-user'   => 'ਯੂਜ਼ਰ ਗਰੁੱਪ ਦੇਖਭਾਲ',
'userrights-user-editname' => 'ਇੱਕ ਯੂਜ਼ਰ ਨਾਂ ਦਿਓ:',
'editusergroup'            => 'ਯੂਜ਼ਰ ਗਰੁੱਪ ਸੋਧ',
'editinguser'              => '<b>$1</b> ਯੂਜ਼ਰ ਸੋਧਿਆ ਜਾ ਰਿਹਾ ਹੈ ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])',
'userrights-editusergroup' => 'ਯੂਜ਼ਰ ਗਰੁੱਪ ਸੋਧ',
'saveusergroups'           => 'ਯੂਜ਼ਰ ਗਰੁੱਪ ਸੰਭਾਲੋ',
'userrights-groupsmember'  => 'ਇਸ ਦਾ ਮੈਂਬਰ:',
'userrights-reason'        => 'ਬਦਲਣ ਦੇ ਕਾਰਨ:',

# Groups
'group'      => 'ਗਰੁੱਪ:',
'group-user' => 'ਮੈਂਬਰ',
'group-all'  => '(ਸਭ)',

'group-user-member' => 'ਮੈਂਬਰ',

# User rights log
'rightsnone' => '(ਕੋਈ ਨਹੀਂ)',

# Recent changes
'recentchanges'     => 'ਤਾਜ਼ਾ ਬਦਲਾਅ',
'rcshowhidemine'    => '$1 ਮੇਰਾ ਐਡਿਟ',
'hide'              => 'ਓਹਲੇ',
'show'              => 'ਵੇਖਾਓ',
'minoreditletter'   => 'ਛ',
'newpageletter'     => 'ਨ',
'boteditletter'     => 'ਬ',
'rc_categories_any' => 'ਕੋਈ ਵੀ',

# Recent changes linked
'recentchangeslinked' => 'ਸਬੰਧਿਤ ਬਦਲਾਅ',

# Upload
'upload'               => 'ਫਾਇਲ ਅੱਪਲੋਡ ਕਰੋ',
'uploadbtn'            => 'ਫਾਇਲ ਅੱਪਲੋਡ ਕਰੋ',
'reupload'             => 'ਰੀ-ਅੱਪਲੋਡ',
'reuploaddesc'         => 'ਅੱਪਲੋਡ ਫਾਰਮ ਉੱਤੇ ਜਾਓ।',
'uploadnologin'        => 'ਲਾਗਇਨ ਨਹੀਂ ਹੋ',
'uploadnologintext'    => 'ਤੁਹਾਨੂੰ[[Special:UserLogin|logged in] ਕਰਨਾ ਪਵੇਗਾ]
to upload files.',
'uploaderror'          => 'ਅੱਪਲੋਡ ਗਲਤੀ',
'uploadlog'            => 'ਅੱਪਲੋਡ ਲਾਗ',
'uploadlogpage'        => 'ਅੱਪਲੋਡ ਲਾਗ',
'filename'             => 'ਫਾਇਲ ਨਾਂ',
'filedesc'             => 'ਸੰਖੇਪ',
'fileuploadsummary'    => 'ਸੰਖੇਪ:',
'filestatus'           => 'ਕਾਪੀਰਾਈਟ ਹਾਲਤ:',
'filesource'           => 'ਸੋਰਸ:',
'uploadedfiles'        => 'ਅੱਪਲੋਡ ਕੀਤੀਆਂ ਫਾਇਲਾਂ',
'ignorewarning'        => 'ਚੇਤਾਵਨੀ ਅਣਡਿੱਠੀ ਕਰਕੇ ਕਿਵੇਂ ਵੀ ਫਾਇਲ ਸੰਭਾਲੋ।',
'minlength1'           => 'ਫਾਇਲ ਨਾਂ ਵਿੱਚ ਘੱਟੋ-ਘੱਟ ਇੱਕ ਅੱਖਰ ਹੋਣਾ ਚਾਹੀਦਾ ਹੈ।',
'badfilename'          => 'ਫਾਇਲ ਨਾਂ "$1" ਬਦਲਿਆ ਗਿਆ ਹੈ।',
'filetype-missing'     => 'ਫਾਇਲ ਦੀ ਕੋਈ ਐਕਸ਼ਟੇਸ਼ਨ ਨਹੀਂ ਹੈ (ਜਿਵੇਂ ".jpg").',
'fileexists'           => 'ਇਹ ਫਾਇਲ ਨਾਂ ਪਹਿਲਾਂ ਹੀ ਮੌਜੂਦ ਹੈ। ਜੇ ਤੁਸੀਂ ਇਹ ਬਦਲਣ ਬਾਰੇ ਜਾਣਦੇ ਨਹੀਂ ਹੋ ਤਾਂ  <strong><tt>$1</tt></strong> ਵੇਖੋ ਜੀ।',
'fileexists-extension' => 'ਇਸ ਨਾਂ ਨਾਲ ਰਲਦੀ ਫਾਇਲ ਮੌਜੂਦ ਹੈ:<br />
ਅੱਪਲੋਡ ਕੀਤੀ ਫਾਇਲ ਦਾ ਨਾਂ: <strong><tt>$1</tt></strong><br />
ਮੌਜੂਦ ਫਾਇਲ ਦਾ ਨਾਂ: <strong><tt>$2</tt></strong><br />
ਇੱਕ ਵੱਖਰਾ ਨਾਂ ਚੁਣੋ ਜੀ',
'fileexists-thumb'     => "<center>'''ਮੌਜੂਦ ਚਿੱਤਰ'''</center>",
'successfulupload'     => 'ਠੀਕ ਤਰ੍ਹਾਂ ਅੱਪਲੋਡ',
'uploadwarning'        => 'ਅੱਪਲੋਡ ਚੇਤਾਵਨੀ',
'savefile'             => 'ਫਾਇਲ ਸੰਭਾਲੋ',
'uploadedimage'        => '"[[$1]]" ਅੱਪਲੋਡ',
'uploaddisabled'       => 'ਅੱਪਲੋਡ ਆਯੋਗ ਹੈ',
'uploadvirus'          => 'ਇਹ ਫਾਇਲ ਵਿੱਚ ਵਾਇਰਸ ਹੈ! ਵੇਰਵੇ ਲਈ ਵੇਖੋ: $1',
'sourcefilename'       => 'ਸੋਰਸ ਫਾਇਲ ਨਾਂ:',
'watchthisupload'      => 'ਇਹ ਪੇਜ ਵਾਚ ਕਰੋ',

'upload-file-error' => 'ਅੰਦਰੂਨੀ ਗਲਤੀ',
'upload-misc-error' => 'ਅਣਜਾਣ ਅੱਪਲੋਡ ਗਲਤੀ',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error28' => 'ਅੱਪਲੋਡ ਟਾਈਮ-ਆਉਟ',

'license'            => 'ਲਾਈਸੈਂਸਿੰਗ:',
'nolicense'          => 'ਕੁਝ ਵੀ ਚੁਣਿਆ',
'license-nopreview'  => '(ਝਲਕ ਉਪਲੱਬਧ ਨਹੀਂ)',
'upload_source_file' => ' (ਤੁਹਾਡੇ ਕੰਪਿਊਟਰ ਉੱਤੇ ਇੱਕ ਫਾਇਲ)',

# Special:ImageList
'imgfile'               => 'ਫਾਇਲ',
'imagelist'             => 'ਫਾਇਲ ਲਿਸਟ',
'imagelist_date'        => 'ਮਿਤੀ',
'imagelist_name'        => 'ਨਾਂ',
'imagelist_user'        => 'ਯੂਜ਼ਰ',
'imagelist_size'        => 'ਆਕਾਰ',
'imagelist_description' => 'ਵੇਰਵਾ',

# Image description page
'filehist'                  => 'ਫਾਇਲ ਅਤੀਤ',
'filehist-deleteall'        => 'ਸਭ ਹਟਾਓ',
'filehist-deleteone'        => 'ਇਹ ਹਟਾਓ',
'filehist-revert'           => 'ਰੀਵਰਟ',
'filehist-current'          => 'ਮੌਜੂਦਾ',
'filehist-datetime'         => 'ਮਿਤੀ/ਸਮਾਂ',
'filehist-user'             => 'ਯੂਜ਼ਰ',
'filehist-dimensions'       => 'ਮਾਪ',
'filehist-filesize'         => 'ਫਾਇਲ ਆਕਾਰ',
'filehist-comment'          => 'ਟਿੱਪਣੀ',
'imagelinks'                => 'ਲਿੰਕ',
'noimage-linktext'          => 'ਇਹ ਅੱਪਲੋਡ',
'uploadnewversion-linktext' => 'ਇਸ ਫਾਇਲ ਦਾ ਇੱਕ ਨਵਾਂ ਵਰਜਨ ਅੱਪਲੋਡ ਕਰੋ',

# File reversion
'filerevert'         => '$1 ਰੀਵਰਟ',
'filerevert-legend'  => 'ਫਾਇਲ ਰੀਵਰਟ',
'filerevert-comment' => 'ਟਿੱਪਣੀ:',
'filerevert-submit'  => 'ਰੀਵਰਟ',

# File deletion
'filedelete'         => '$1 ਹਟਾਓ',
'filedelete-legend'  => 'ਫਾਇਲ ਹਟਾਓ',
'filedelete-comment' => 'ਟਿੱਪਣੀ:',
'filedelete-submit'  => 'ਹਟਾਓ',
'filedelete-success' => "'''$1''' ਨੂੰ ਹਟਾਇਆ ਗਿਆ।",

# MIME search
'mimesearch' => 'MIME ਖੋਜ',
'mimetype'   => 'MIME ਕਿਸਮ:',
'download'   => 'ਡਾਊਨਲੋਡ',

# Statistics
'statistics'             => 'ਅੰਕੜੇ',
'sitestats'              => '{{SITENAME}} ਅੰਕੜੇ',
'userstats'              => 'ਯੂਜ਼ਰ ਅੰਕੜੇ',
'statistics-mostpopular' => 'ਸਭ ਤੋਂ ਵੱਧ ਵੇਖੇ ਪੇਜ',

'brokenredirects-edit'   => '(ਸੋਧ)',
'brokenredirects-delete' => '(ਹਟਾਓ)',

# Miscellaneous special pages
'unusedcategories'  => 'ਅਣਵਰਤੀਆਂ ਕੈਟਾਗਰੀਆਂ',
'unusedimages'      => 'ਅਣਵਰਤੀਆਂ ਫਾਇਲਾਂ',
'popularpages'      => 'ਪਾਪੂਲਰ ਪੇਜ',
'shortpages'        => 'ਛੋਟੇ ਪੇਜ',
'listusers'         => 'ਯੂਜ਼ਰ ਲਿਸਟ',
'newpages'          => 'ਨਵੇਂ ਪੇਜ',
'newpages-username' => 'ਯੂਜ਼ਰ ਨਾਂ:',
'ancientpages'      => 'ਸਭ ਤੋਂ ਪੁਰਾਣੇ ਪੇਜ',
'move'              => 'ਭੇਜੋ',
'movethispage'      => 'ਇਹ ਪੇਜ ਭੇਜੋ',
'notargettitle'     => 'ਟਾਰਗੇਟ ਨਹੀਂ',

# Book sources
'booksources'    => 'ਕਿਤਾਬ ਸਰੋਤ',
'booksources-go' => 'ਜਾਓ',

# Special:Log
'specialloguserlabel'  => 'ਯੂਜ਼ਰ:',
'speciallogtitlelabel' => 'ਟਾਇਟਲ:',
'log'                  => 'ਲਾਗ',
'all-logs-page'        => 'ਸਭ ਲਾਗ',
'log-search-submit'    => 'ਜਾਓ',

# Special:AllPages
'allpages'          => 'ਸਭ ਪੇਜ',
'alphaindexline'    => '$1 ਤੋਂ $2',
'nextpage'          => 'ਅੱਗੇ ਪੇਜ ($1)',
'prevpage'          => 'ਪਿੱਛੇ ਪੇਜ ($1)',
'allarticles'       => 'ਸਭ ਲੇਖ',
'allinnamespace'    => 'ਸਭ ਪੇਜ ($1 ਨੇਮਸਪੇਸ)',
'allnotinnamespace' => 'ਸਭ ਪੇਜ ($1 ਨੇਮਸਪੇਸ ਵਿੱਚ ਨਹੀਂ)',
'allpagesprev'      => 'ਪਿੱਛੇ',
'allpagesnext'      => 'ਅੱਗੇ',
'allpagessubmit'    => 'ਜਾਓ',

# Special:Categories
'categories' => 'ਕੈਟਾਗਰੀਆਂ',

# Special:ListUsers
'listusers-submit'   => 'ਵੇਖੋ',
'listusers-noresult' => 'ਕੋਈ ਯੂਜ਼ਰ ਨਹੀਂ ਲੱਭਿਆ।',

# E-mail user
'mailnologin'     => 'ਕੋਈ ਭੇਜਣ ਐਡਰੈੱਸ ਨਹੀਂ',
'emailuser'       => 'ਇਹ ਯੂਜ਼ਰ ਨੂੰ ਈਮੇਲ ਕਰੋ',
'emailpage'       => 'ਯੂਜ਼ਰ ਨੂੰ ਈਮੇਲ ਕਰੋ',
'defemailsubject' => '{{SITENAME}} ਈਮੇਲ',
'noemailtitle'    => 'ਕੋਈ ਈਮੇਲ ਐਡਰੈੱਸ ਨਹੀਂ',
'emailfrom'       => 'ਵਲੋਂ',
'emailto'         => 'ਵੱਲ',
'emailsubject'    => 'ਵਿਸ਼ਾ',
'emailmessage'    => 'ਸੁਨੇਹਾ',
'emailsend'       => 'ਭੇਜੋ',
'emailccme'       => 'ਸੁਨੇਹੇ ਦੀ ਇੱਕ ਕਾਪੀ ਮੈਨੂੰ ਵੀ ਭੇਜੋ।',
'emailsent'       => 'ਈਮੇਲ ਭੇਜੀ ਗਈ',
'emailsenttext'   => 'ਤੁਹਾਡੀ ਈਮੇਲ ਭੇਜੀ ਗਈ ਹੈ।',

# Watchlist
'watchlist'          => 'ਮੇਰੀ ਵਾਚ-ਲਿਸਟ',
'mywatchlist'        => 'ਮੇਰੀ ਵਾਚ-ਲਿਸਟ',
'watchlistfor'       => "('''$1''' ਲਈ)",
'watchnologin'       => 'ਲਾਗਇਨ ਨਹੀਂ',
'addedwatch'         => 'ਵਾਚ-ਲਿਸਟ ਵਿੱਚ ਸ਼ਾਮਲ',
'watch'              => 'ਵਾਚ',
'watchthispage'      => 'ਇਹ ਪੇਜ ਵਾਚ ਕਰੋ',
'unwatch'            => 'ਅਣ-ਵਾਚ',
'wlshowlast'         => 'ਆਖਰੀ $1 ਦਿਨ $2 ਘੰਟੇ $3 ਵੇਖੋ',
'watchlist-show-own' => 'ਮੇਰੀ ਸੋਧ ਵੇਖਾਓ',
'watchlist-hide-own' => 'ਮੇਰੀ ਸੋਧ ਓਹਲੇ',

'enotif_newpagetext'           => 'ਇਹ ਨਵਾਂ ਪੇਜ ਹੈ।',
'enotif_impersonal_salutation' => '{{SITENAME}} ਯੂਜ਼ਰ',
'changed'                      => 'ਬਦਲਿਆ',
'created'                      => 'ਬਣਾਇਆ',
'enotif_anon_editor'           => 'ਅਗਿਆਤ ਯੂਜ਼ਰ $1',

# Delete/protect/revert
'deletepage'       => 'ਪੇਜ ਹਟਾਓ',
'confirm'          => 'ਪੁਸ਼ਟੀ',
'excontent'        => "ਸਮੱਗਰੀ ਸੀ: '$1'",
'exblank'          => 'ਪੇਜ ਖਾਲੀ ਹੈ',
'actioncomplete'   => 'ਐਕਸ਼ਨ ਪੂਰਾ ਹੋਇਆ',
'deletedarticle'   => '"[[$1]]" ਹਟਾਇਆ',
'rollback_short'   => 'ਰੋਲਬੈਕ',
'rollbacklink'     => 'ਰੋਲਬੈਕ',
'rollbackfailed'   => 'ਰੋਲਬੈਕ ਫੇਲ੍ਹ',
'protectlogpage'   => 'ਸੁਰੱਖਿਆ ਲਾਗ',
'protect-legend'   => 'ਸੁਰੱਖਿਆ ਕਨਫਰਮ',
'protectcomment'   => 'ਟਿੱਪਣੀ:',
'protectexpiry'    => 'ਮਿਆਦ:',
'protect-default'  => '(ਡਿਫਾਲਟ)',
'restriction-type' => 'ਅਧਿਕਾਰ:',
'minimum-size'     => 'ਘੱਟੋ-ਘੱਟ ਆਕਾਰ',
'maximum-size'     => 'ਵੱਧੋ-ਵੱਧ ਆਕਾਰ',
'pagesize'         => '(ਬਾਈਟ)',

# Restrictions (nouns)
'restriction-edit'   => 'ਸੋਧ',
'restriction-move'   => 'ਭੇਜੋ',
'restriction-upload' => 'ਅੱਪਲੋਡ',

# Restriction levels
'restriction-level-sysop'         => 'ਪੂਰਾ ਸੁਰੱਖਿਅਤ',
'restriction-level-autoconfirmed' => 'ਅਰਧ-ਸੁਰੱਖਿਅਤ',
'restriction-level-all'           => 'ਕੋਈ ਲੈਵਲ',

# Undelete
'undeletebtn'     => 'ਰੀਸਟੋਰ',
'undeletereset'   => 'ਰੀ-ਸੈੱਟ',
'undeletecomment' => 'ਟਿੱਪਣੀ:',

# Namespace form on various pages
'invert'         => 'ਉਲਟ ਚੋਣ',
'blanknamespace' => '(ਮੁੱਖ)',

# Contributions
'contributions' => 'ਯੂਜ਼ਰ ਯੋਗਦਾਨ',
'mycontris'     => 'ਮੇਰਾ ਯੋਗਦਾਨ',
'contribsub2'   => '$1 ($2) ਲਈ',

'sp-contributions-newbies-sub' => 'ਨਵੇਂ ਅਕਾਊਂਟਾਂ ਲਈ',
'sp-contributions-username'    => 'IP ਐਡਰੈੱਸ ਜਾਂ ਯੂਜ਼ਰ ਨਾਂ:',
'sp-contributions-submit'      => 'ਖੋਜ',

# What links here
'linklistsub'         => '(ਲਿੰਕਾਂ ਦੀ ਲਿਸਟ)',
'whatlinkshere-links' => '← ਲਿੰਕ',

# Block/unblock
'blockip'              => 'ਯੂਜ਼ਰ ਬਲਾਕ ਕਰੋ',
'ipaddress'            => 'IP ਐਡਰੈੱਸ:',
'ipadressorusername'   => 'IP ਐਡਰੈਸ ਜਾਂ ਯੂਜ਼ਰ ਨਾਂ:',
'ipbexpiry'            => 'ਮਿਆਦ:',
'ipbreason'            => 'ਕਾਰਨ:',
'ipbreasonotherlist'   => 'ਹੋਰ ਕਾਰਨ',
'ipbanononly'          => 'ਕੇਵਲ ਅਗਿਆਤ(anonymous) ਯੂਜ਼ਰਾਂ ਲਈ ਪਾਬੰਦੀ',
'ipbsubmit'            => 'ਇਹ ਯੂਜ਼ਰ ਲਈ ਪਾਬੰਦੀ',
'ipbother'             => 'ਹੋਰ ਟਾਈਮ:',
'ipbotheroption'       => 'ਹੋਰ',
'ipbotherreason'       => 'ਹੋਰ/ਆਮ ਕਾਰਨ:',
'badipaddress'         => 'ਗਲਤ IP ਐਡਰੈੱਸ',
'ipb-unblock-addr'     => '$1 ਅਣ-ਬਲਾਕ',
'ipb-unblock'          => 'ਇੱਕ ਯੂਜ਼ਰ ਨਾਂ ਜਾਂ IP ਐਡਰੈੱਸ ਅਣ-ਬਲਾਕ ਕਰੋ',
'unblockip'            => 'ਯੂਜ਼ਰ ਅਣ-ਬਲਾਕ ਕਰੋ',
'ipblocklist-username' => 'ਯੂਜ਼ਰ ਨਾਂ ਜਾਂ IP ਐਡਰੈੱਸ:',
'ipblocklist-submit'   => 'ਖੋਜ',
'blocklistline'        => '$1, $2 ਬਲਾਕ $3 ($4)',
'infiniteblock'        => 'ਬੇਅੰਤ',
'expiringblock'        => '$1 ਮਿਆਦ ਖਤਮ',
'anononlyblock'        => 'anon. ਹੀ',
'emailblock'           => 'ਈਮੇਲ ਬਲਾਕ ਹੈ',
'blocklink'            => 'ਬਲਾਕ',
'unblocklink'          => 'ਅਣ-ਬਲਾਕ',
'proxyblocksuccess'    => 'ਪੂਰਾ ਹੋਇਆ',

# Developer tools
'lockdb' => 'ਡਾਟਾਬੇਸ ਲਾਕ',

# Move page
'move-page-legend' => 'ਪੇਜ ਮੂਵ ਕਰੋ',
'movearticle'      => 'ਪੇਜ ਮੂਵ ਕਰੋ:',
'newtitle'         => 'ਨਵੇਂ ਟਾਇਟਲ ਲਈ:',
'move-watch'       => 'ਇਹ ਪੇਜ ਵਾਚ ਕਰੋ',
'movepagebtn'      => 'ਪੇਜ ਮੂਵ ਕਰੋ',
'pagemovedsub'     => 'ਮੂਵ ਸਫ਼ਲ ਰਿਹਾ',
'movedto'          => 'ਮੂਵ ਕੀਤਾ',
'movelogpage'      => 'ਮੂਵ ਲਾਗ',
'movereason'       => 'ਕਾਰਨ:',
'revertmove'       => 'ਰੀਵਰਟ',
'delete_and_move'  => 'ਹਟਾਓ ਅਤੇ ਮੂਵ ਕਰੋ',

# Export
'export'        => 'ਸਫ਼ੇ ਐਕਸਪੋਰਟ ਕਰੋ',
'export-submit' => 'ਐਕਸਪੋਰਟ',
'export-addcat' => 'ਸ਼ਾਮਲ',

# Namespace 8 related
'allmessages'        => 'ਸਿਸਟਮ ਸੁਨੇਹੇ',
'allmessagesname'    => 'ਨਾਂ',
'allmessagesdefault' => 'ਡਿਫਾਲਟ ਟੈਕਸਟ',
'allmessagescurrent' => 'ਮੌਜੂਦਾ ਟੈਕਸਟ',

# Thumbnails
'filemissing' => 'ਫਾਇਲ ਗੁੰਮ ਹੈ',

# Special:Import
'import'                  => 'ਪੇਜ ਇੰਪੋਰਟ ਕਰੋ',
'import-interwiki-submit' => 'ਇੰਪੋਰਟ',
'importstart'             => 'ਪੇਜ ਇੰਪੋਰਟ ਕੀਤੇ ਜਾ ਰਹੇ ਹਨ...',
'importfailed'            => 'ਇੰਪੋਰਟ ਫੇਲ੍ਹ: $1',
'importnotext'            => 'ਖਾਲੀ ਜਾਂ ਕੋਈ ਟੈਕਸਟ ਨਹੀਂ',
'importsuccess'           => 'ਇੰਪੋਰਟ ਸਫ਼ਲ!',
'importnofile'            => 'ਕੋਈ ਇੰਪੋਰਟ ਫਾਇਲ ਅੱਪਲੋਡ ਨਹੀਂ ਕੀਤੀ।',

# Import log
'importlogpage'                 => 'ਇੰਪੋਰਟ ਲਾਗ',
'import-logentry-upload-detail' => '$1 ਰੀਵਿਜ਼ਨ',

# Tooltip help for the actions
'tooltip-pt-userpage'        => 'ਮੇਰਾ ਯੂਜ਼ਰ ਪੇਜ',
'tooltip-pt-mytalk'          => 'ਮੇਰਾ ਗੱਲਬਾਤ ਪੇਜ',
'tooltip-pt-preferences'     => 'ਮੇਰੀ ਪਸੰਧ',
'tooltip-pt-mycontris'       => 'ਮੇਰੇ ਯੋਗਦਾਨ ਦੀ ਲਿਸਟ',
'tooltip-pt-logout'          => 'ਲਾਗ ਆਉਟ',
'tooltip-ca-protect'         => 'ਇਹ ਪੇਜ ਸੁਰੱਖਿਅਤ ਬਣਾਓ',
'tooltip-ca-delete'          => 'ਇਹ ਪੇਜ ਹਟਾਓ',
'tooltip-ca-move'            => 'ਇਹ ਪੇਜ ਭੇਜੋ',
'tooltip-search'             => 'ਖੋਜ {{SITENAME}}',
'tooltip-p-logo'             => 'ਮੁੱਖ ਪੇਜ',
'tooltip-n-mainpage'         => 'ਮੁੱਖ ਪੇਜ ਖੋਲ੍ਹੋ',
'tooltip-n-randompage'       => 'ਇੱਕ ਰਲਵਾਂ ਪੇਜ ਲੋਡ ਕਰੋ',
'tooltip-n-help'             => 'ਖੋਜਣ ਲਈ ਥਾਂ',
'tooltip-t-emailuser'        => 'ਇਹ ਯੂਜ਼ਰ ਨੂੰ ਮੇਲ ਭੇਜੋ',
'tooltip-t-upload'           => 'ਚਿੱਤਰ ਜਾਂ ਮੀਡਿਆ ਫਾਇਲਾਂ ਅੱਪਲੋਡ ਕਰੋ',
'tooltip-ca-nstab-main'      => 'ਸਮਗੱਰੀ ਪੇਜ ਵੇਖੋ',
'tooltip-ca-nstab-user'      => 'ਯੂਜ਼ਰ ਪੇਜ ਵੇਖੋ',
'tooltip-ca-nstab-media'     => 'ਮੀਡਿਆ ਪੇਜ ਵੇਖੋ',
'tooltip-ca-nstab-project'   => 'ਪਰੋਜੈਕਟ ਪੇਜ ਵੇਖੋ',
'tooltip-ca-nstab-image'     => 'ਚਿੱਤਰ ਪੇਜ ਵੇਖੋ',
'tooltip-ca-nstab-mediawiki' => 'ਸਿਸਟਮ ਸੁਨੇਹੇ ਵੇਖੋ',
'tooltip-ca-nstab-template'  => 'ਟੈਪਲੇਟ ਵੇਖੋ',
'tooltip-ca-nstab-help'      => 'ਮੱਦਦ ਪੇਜ ਵੇਖੋ',
'tooltip-ca-nstab-category'  => 'ਕੈਟਾਗਰੀ ਪੇਜ ਵੇਖੋ',
'tooltip-save'               => 'ਆਪਣੇ ਬਦਲਾਅ ਸੰਭਾਲੋ',
'tooltip-upload'             => 'ਅੱਪਲੋਡ ਸਟਾਰਟ ਕਰੋ',

# Attribution
'others'      => 'ਹੋਰ',
'siteusers'   => '{{SITENAME}} ਯੂਜ਼ਰ $1',
'creditspage' => 'ਪੇਜ ਮਾਣ',

# Spam protection
'spamprotectiontitle' => 'Spam ਸੁਰੱਖਿਆ ਫਿਲਟਰ',

# Info page
'infosubtitle' => 'ਸਫ਼ੇ ਦੀ ਜਾਣਕਾਰੀ',

# Patrol log
'patrol-log-auto' => '(ਆਟੋਮੈਟਿਕ)',

# Browsing diffs
'previousdiff' => '← ਅੰਤਰ ਪਿੱਛੇ',
'nextdiff'     => 'ਅੰਤਰ ਅੱਗੇ →',

# Media information
'thumbsize'            => 'ਥੰਮਨੇਲ ਆਕਾਰ:',
'widthheightpage'      => '$1×$2, $3 ਪੇਜ਼',
'file-info'            => '(ਫਾਇਲ ਆਕਾਰ: $1, MIME ਕਿਸਮ: $2)',
'file-info-size'       => '($1 × $2 ਪਿਕਸਲ, ਫਾਇਲ ਆਕਾਰ: $3, MIME ਕਿਸਮ: $4)',
'svg-long-desc'        => '(SVG ਫਾਇਲ, nominally $1 × $2 pixels, file size: $3)',
'show-big-image'       => 'ਪੂਰਾ ਰੈਜ਼ੋਲੇਸ਼ਨ',
'show-big-image-thumb' => '<small>ਇਹ ਝਲਕ ਦਾ ਆਕਾਰ: $1 × $2 ਪਿਕਸਲ</small>',

# Special:NewImages
'newimages' => 'ਨਵੀਆਂ ਫਾਇਲਾਂ ਦੀ ਗੈਲਰੀ',
'noimages'  => 'ਵੇਖਣ ਲਈ ਕੁਝ ਨਹੀਂ',
'ilsubmit'  => 'ਖੋਜ',
'bydate'    => 'ਮਿਤੀ ਨਾਲ',

# EXIF tags
'exif-imagewidth'       => 'ਚੌੜਾਈ',
'exif-imagelength'      => 'ਉਚਾਈ',
'exif-samplesperpixel'  => 'ਭਾਗਾਂ ਦੀ ਗਿਣਤੀ',
'exif-transferfunction' => 'ਟਰਾਂਸਫਰ ਫੰਕਸ਼ਨ',
'exif-imagedescription' => 'ਚਿੱਤਰ ਟਾਇਟਲ',
'exif-make'             => 'ਕੈਮਰਾ ਨਿਰਮਾਤਾ',
'exif-model'            => 'ਕੈਮਰਾ ਮਾਡਲ',
'exif-software'         => 'ਵਰਤਿਆ ਸਾਫਟਵੇਅਰ',
'exif-artist'           => 'ਲੇਖਕ',
'exif-copyright'        => 'ਕਾਪੀਰਾਈਟ ਟਾਇਟਲ',
'exif-subjectarea'      => 'ਵਿਸ਼ਾ ਖੇਤਰ',
'exif-gpsdatestamp'     => 'GPS ਮਿਤੀ',

'exif-unknowndate' => 'ਅਣਜਾਣ ਮਿਤੀ',

'exif-exposureprogram-2' => 'ਸਧਾਰਨ ਪਰੋਗਰਾਮ',

'exif-meteringmode-0'   => 'ਅਣਜਾਣ',
'exif-meteringmode-1'   => 'ਔਸਤ',
'exif-meteringmode-5'   => 'ਪੈਟਰਨ',
'exif-meteringmode-255' => 'ਹੋਰ',

'exif-lightsource-0'  => 'ਅਣਜਾਣ',
'exif-lightsource-9'  => 'ਵਧੀਆ ਮੌਸਮ',
'exif-lightsource-10' => 'ਬੱਦਲ ਵਾਲਾ ਮੌਸਮ',

'exif-focalplaneresolutionunit-2' => 'ਇੰਚ',

'exif-scenecapturetype-0' => 'ਸਟੈਂਡਰਡ',
'exif-scenecapturetype-1' => 'ਲੈਂਡਸਕੇਪ',
'exif-scenecapturetype-2' => 'ਪੋਰਟਰੇਟ',

'exif-subjectdistancerange-0' => 'ਅਣਜਾਣ',
'exif-subjectdistancerange-1' => 'ਮਾਈਕਰੋ',
'exif-subjectdistancerange-2' => 'ਝਲਕ ਬੰਦ ਕਰੋ',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'ਕਿਲੋਮੀਟਰ ਪ੍ਰਤੀ ਘੰਟਾ',
'exif-gpsspeed-m' => 'ਮੀਲ ਪ੍ਰਤੀ ਘੰਟਾ',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'ਸਭ',
'imagelistall'     => 'ਸਭ',
'watchlistall2'    => 'ਸਭ',
'namespacesall'    => 'ਸਭ',
'monthsall'        => 'ਸਭ',

# E-mail address confirmation
'confirmemail'          => 'ਈਮੇਲ ਐਡਰੈੱਸ ਪੁਸ਼ਟੀ',
'confirmemail_send'     => 'ਇੱਕ ਪੁਸ਼ਟੀ ਕੋਡ ਭੇਜੋ',
'confirmemail_sent'     => 'ਪੁਸ਼ਟੀ ਈਮੇਲ ਭੇਜੀ ਗਈ।',
'confirmemail_invalid'  => 'ਗਲਤ ਪੁਸ਼ਟੀ ਕੋਡ ਹੈ। ਕੋਡ ਦੀ ਮਿਆਦ ਪੁੱਗੀ ਹੋ ਸਕਦੀ ਹੈ।',
'confirmemail_loggedin' => 'ਹੁਣ ਤੁਹਾਡਾ ਈਮੇਲ ਐਡਰੈੱਸ ਚੈੱਕ (confirmed) ਹੋ ਗਿਆ ਹੈ।',
'confirmemail_subject'  => '{{SITENAME}} ਈਮੇਲ ਐਡਰੈੱਸ ਪੁਸ਼ਟੀ',

# Scary transclusion
'scarytranscludetoolong' => '[URL ਬਹੁਤ ਲੰਮਾ ਹੈ; ਅਫਸੋਸ ਹੈ]',

# Trackbacks
'trackbackremove' => ' ([$1 ਹਟਾਓ])',
'trackbacklink'   => 'ਟਰੈਕਬੈਕ',

# Delete conflict
'recreate' => 'ਮੁੜ-ਬਣਾਓ',

# HTML dump
'redirectingto' => '[[:$1]] ਲਈ ਰੀ-ਡਿਰੈਕਟ ਕੀਤਾ ਜਾ ਰਿਹਾ ਹੈ...',

# action=purge
'confirm_purge_button' => 'ਠੀਕ ਹੈ',

# AJAX search
'hideresults' => 'ਨਤੀਜੇ ਓਹਲੇ',

# Multipage image navigation
'imgmultipageprev' => '← ਪਿਛਲਾ ਪੇਜ',
'imgmultipagenext' => 'ਅਗਲਾ ਪੇਜ →',
'imgmultigo'       => 'ਜਾਓ!',

# Table pager
'table_pager_next'         => 'ਅਗਲਾ ਪੇਜ',
'table_pager_prev'         => 'ਪਿਛਲਾ ਪੇਜ',
'table_pager_first'        => 'ਪਹਿਲਾ ਪੇਜ',
'table_pager_last'         => 'ਆਖਰੀ ਪੇਜ',
'table_pager_limit'        => 'ਹਰੇਕ ਪੇਜ ਲਈ $1 ਆਈਟਮਾਂ',
'table_pager_limit_submit' => 'ਜਾਓ',
'table_pager_empty'        => 'ਕੋਈ ਨਤੀਜਾ ਨਹੀਂ',

# Auto-summaries
'autosumm-blank' => 'ਪੇਜ ਤੋਂ ਸਭ ਸਮੱਗਰੀ ਹਟਾਓ',
'autosumm-new'   => 'ਨਵਾਂ ਪੇਜ: $1',

# Live preview
'livepreview-loading' => 'ਲੋਡ ਕੀਤਾ ਜਾ ਰਿਹਾ ਹੈ…',
'livepreview-ready'   => 'ਲੋਡ ਕੀਤਾ ਜਾ ਰਿਹਾ ਹੈ...ਤਿਆਰ!',

# Watchlist editor
'watchlistedit-raw-titles'  => 'ਟਾਇਟਲ:',
'watchlistedit-raw-added'   => '{{PLURAL:$1|1 title was|$1 titles were}} ਸ਼ਾਮਲ:',
'watchlistedit-raw-removed' => '{{PLURAL:$1|1 title was|$1 titles were}} ਹਟਾਓ:',

# Special:Version
'version' => 'ਵਰਜਨ', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages'             => 'ਖਾਸ ਪੇਜ',
'specialpages-group-login' => 'ਲਾਗ ਇਨ / ਅਕਾਊਂਟ ਬਣਾਓ',

);
