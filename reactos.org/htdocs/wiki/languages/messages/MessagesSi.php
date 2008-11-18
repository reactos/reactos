<?php
/** Sinhala (සිංහල)
 *
 * @ingroup Language
 * @file
 *
 * @author Asiri wiki
 * @author Chandana
 * @author නන්දිමිතුරු
 */

$namespaceNames = array(
	NS_MEDIA          => 'මාධ්‍යය',
	NS_SPECIAL        => 'විශේෂ',
	NS_TALK           => 'සාකච්ඡාව',
	NS_USER           => 'පරිශීලක',
	NS_USER_TALK      => 'පරිශීලක_සාකච්ඡාව',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => '$1_සාකච්ඡාව',
	NS_IMAGE          => 'රූපය',
	NS_IMAGE_TALK     => 'රූපය_සාකච්ඡාව',
	NS_MEDIAWIKI      => 'විකිමාධ්‍ය',
	NS_MEDIAWIKI_TALK => 'විකිමාධ්‍ය_සාකච්ඡාව',
	NS_TEMPLATE       => 'සැකිල්ල',
	NS_TEMPLATE_TALK  => 'සැකිල_සාකච්ඡාව',
	NS_HELP           => 'උදවු',
	NS_HELP_TALK      => 'උදව_සාකච්ඡාව',
	NS_CATEGORY       => 'ප්‍රවර්ගය',
	NS_CATEGORY_TALK  => 'ප්‍රවර්ග_සාකච්ඡාව',
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'ආපසු_හරවා_යවනවා' ),
	'BrokenRedirects'           => array( 'වැරදුනු_යොමුකිරිමක්' ),
	'Userlogin'                 => array( 'ඇතුලුවිම' ),
	'Userlogout'                => array( 'ඉවත්විම' ),
	'CreateAccount'             => array( 'සාමාජිකත්වය_ලැබිමට' ),
	'Preferences'               => array( 'මනාපය' ),
	'Watchlist'                 => array( 'මුරකරනවා' ),
	'Recentchanges'             => array( 'නව_වෙනස්විමි' ),
	'Upload'                    => array( 'ගොනුවක්_ඇතූලත්_කිරිම' ),
	'Imagelist'                 => array( 'පින්තූර_ලැයිස්තුව' ),
	'Newimages'                 => array( 'අලුත්_පින්තූර' ),
	'Listusers'                 => array( 'සාමාජික_ලැයිස්තුව' ),
	'Statistics'                => array( 'සංඛ්‍යා_ලේඛනය' ),
	'Randompage'                => array( 'අහඹු_ලෙස', 'අහඹු_පිටුව' ),
	'Lonelypages'               => array( 'හුදකලා_පිටුව' ),
	'Uncategorizedpages'        => array( 'වර්ගනොකල_පිටුව' ),
	'Uncategorizedcategories'   => array( 'වර්ගනොකල_කොටස්' ),
	'Uncategorizedimages'       => array( 'වර්ගනොකල_පින්තූර' ),
	'Uncategorizedtemplates'    => array( 'වර්ගනොකල_අචිචු' ),
	'Unusedcategories'          => array( 'හාවිතා_නොවන_කොටස්' ),
	'Unusedimages'              => array( 'හාවිතා_නොවන_පින්තූර' ),
	'Wantedpages'               => array( 'අවශය_පිටු' ),
	'Wantedcategories'          => array( 'අවශය_කොටස' ),
	'Mostlinked'                => array( 'ජනපිය_සම්බන්ධකය' ),
	'Mostlinkedcategories'      => array( 'වැඬ්පුර_භව්තාවු_කොටස' ),
	'Mostlinkedtemplates'       => array( 'වැඬ්පුර_භව්තාවු_අච්චු' ),
	'Mostcategories'            => array( 'ජනපිය_කොටස' ),
	'Mostimages'                => array( 'අතිශය_පින්තූර' ),
	'Mostrevisions'             => array( 'අතිශය_පරිශෝධනය' ),
	'Shortpages'                => array( 'කෙට_පිටුව' ),
	'Longpages'                 => array( 'දිග_පිටුව' ),
	'Newpages'                  => array( 'නව_පිටුව' ),
	'Ancientpages'              => array( 'අතීත_පිටුව' ),
	'Deadendpages'              => array( 'අඩු_කරන_පිටුව' ),
	'Protectedpages'            => array( 'ආරක්ෂිත_පිටුව' ),
	'Protectedtitles'           => array( 'ආරක්ෂිත__හිමිකම' ),
	'Allpages'                  => array( 'සියල_පිටුව' ),
	'Prefixindex'               => array( 'උපසර්ගය' ),
	'Specialpages'              => array( 'විශෝෂ_පිටුව' ),
	'Contributions'             => array( 'දායකත්වය' ),
	'Emailuser'                 => array( 'පරිශීලකට_ඉ-ලිපිය_යැවිම' ),
	'Confirmemail'              => array( 'ඉ-ලිපිය_තහවුරු_කරනවා' ),
	'Recentchangeslinked'       => array( 'නුතන_වෙනස්_වීම' ),
	'Movepage'                  => array( 'පිටුව_ගෙන_යනවා' ),
	'Blockme'                   => array( 'මමම_අවහිර_කරනවා' ),
	'Booksources'               => array( 'පුස්තක' ),
	'Categories'                => array( 'වර්ගකරිම' ),
	'Export'                    => array( 'අපනයනය' ),
	'Version'                   => array( 'අනුවාදය' ),
	'Allmessages'               => array( 'සියලු_පණිිවිඩ' ),
	'Log'                       => array( 'කඳ' ),
	'Import'                    => array( 'ආයාත' ),
	'Randomredirect'            => array( 'අහඹු_ලෙස_යොමුකිරිම' ),
	'Mypage'                    => array( 'මගේ__පිටුව' ),
	'Mytalk'                    => array( 'මගේ__කතාබහ' ),
	'Mycontributions'           => array( 'මගේ_දායකත්වය' ),
	'Popularpages'              => array( 'ජනප්‍රිය_පිටුව' ),
	'Search'                    => array( 'සෙවුම' ),
	'Resetpass'                 => array( 'මුර_පදය_යළි_පිහිටුවනවා' ),
	'Withoutinterwiki'          => array( 'පටන_අන්තර්_විකි' ),
	'MergeHistory'              => array( 'ඉතිහාසය_සංයුක්ත_කිරිම' ),
	'Filepath'                  => array( 'ගොනු_පථය' ),
);

$messages = array(
# User preference toggles
'tog-underline'       => 'පුරුක යටින් ඉරි අඳිනවා',
'tog-highlightbroken' => ' කැඩුණු සන්ධිය ආකෘතිය <a href="" වර්ගය="අලුත">මේ සමාන ලෙස </a> (විකල්ප: මේ සමාන ලෙස<a href="" වර්ගය="අභ්‍යනතර">?</a>).',
'tog-justify'         => 'ඡේදය පේළි ගසන්න',
'tog-hideminor'       => 'අලුත් වෙනසහි සුළු සංස්කරණය හැංගිම',
'tog-editsection'     => '[සංස්කරණය] බැඳියාවන් මගින් ඡේද සංස්කරණයට ඉඩ සැලසීම',

'skinpreview' => '(පෙරදසුන)',

# Dates
'sunday'        => 'ඉරිදා',
'monday'        => 'සඳුදා',
'tuesday'       => 'අඟහරුවාදා',
'wednesday'     => 'බදාදා',
'thursday'      => 'බ්‍රහස්පතින්දා',
'friday'        => 'සිකුරාදා',
'saturday'      => 'සෙනසුරාදා',
'sun'           => 'ඉරිදා',
'mon'           => 'සඳු',
'tue'           => 'අඟ',
'wed'           => 'බදා',
'thu'           => 'බ්‍රහස්',
'fri'           => 'සිකු',
'sat'           => 'සෙන',
'january'       => 'ජනවාරි',
'february'      => 'පෙබරවාරි',
'march'         => 'මාර්තු',
'april'         => 'අප්‍රේල්',
'may_long'      => 'මැයි',
'june'          => 'ජූනි',
'july'          => 'ජූලි',
'august'        => 'අගෝස්තු',
'september'     => 'සැප්තැම්බර්',
'october'       => 'ඔක්තෝබර්',
'november'      => 'නොවැම්බර්',
'december'      => 'දෙසැම්බර්',
'january-gen'   => 'ජනවාරි',
'february-gen'  => 'පෙබරවාරි',
'march-gen'     => 'මාර්තු',
'april-gen'     => 'අප්‍රේල්',
'may-gen'       => 'මැයි',
'june-gen'      => 'ජූනි',
'july-gen'      => 'ජූලි',
'august-gen'    => 'අගෝස්තු',
'september-gen' => 'සැප්තැම්බර්',
'october-gen'   => 'ඔක්තෝබර්',
'november-gen'  => 'නොවැම්බර්',
'december-gen'  => 'දෙසැම්බර්',
'jan'           => 'ජන',
'feb'           => 'පෙබ',
'mar'           => 'මාර්',
'apr'           => 'අප්‍රේ',
'may'           => 'මැයි',
'jun'           => 'ජූනි',
'jul'           => 'ජූලි',
'aug'           => 'අගෝ',
'sep'           => 'සැප්',
'oct'           => 'ඔක්',
'nov'           => 'නොවැ',
'dec'           => 'දෙසැ',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|ප්‍රවර්ගය|ප්‍රවර්ග}}',
'category_header'                => '"$1" ප්‍රවර්ගයට අයත් පිටු',
'subcategories'                  => 'උපප්‍රවර්ග',
'category-media-header'          => '"$1" ප්‍රවර්ගයට අයත් මාධ්‍ය',
'category-empty'                 => "''දැනට මෙම ප්‍රවර්ගය පිටු හෝ මාධ්‍ය හෝ නොදරයි.''",
'hidden-categories'              => '{{PLURAL:$1|සැඟවුනු ප්‍රවර්ගය|සැඟවුනු ප්‍රවර්ග}}',
'hidden-category-category'       => 'සැඟවුනු ප්‍රවර්ග', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|මෙම ප්‍රවර්ගය සතු වන්නේ පහත දැක්වෙන උපප්‍රවර්ගය පමණි.|මෙම ප්‍රවර්ගය සතු මුළු $2 උපප්‍රවර්ග ගණන අතර, පහත දැක්වෙන {{PLURAL:$1|උපප්‍රවර්ගය|උපප්‍රවර්ග $1 }} වේ.}}',
'category-subcat-count-limited'  => 'මෙම ප්‍රවර්ගයට පහත දැක්වෙන {{PLURAL:$1|උපප්‍රවර්ගය| උපප්‍රවර්ග $1 ගණන}} අඩංගු වේ.',
'category-article-count'         => '{{PLURAL:$2|මෙම ප්‍රවර්ගය සතු වන්නේ මෙහි පහත දැක්වෙන පිටුව පමණි.|සමස්ත $2 පිටු ගණන අතුරින්, {{PLURAL:$1|පිටුව|පිටු $1 ගණනක්}} මෙම ප්‍රවර්ගය සතුවේ.}}',
'category-article-count-limited' => 'මෙහි පහත දෑක්වෙන {{PLURAL:$1|පිටුව|පිටු $1 ගණන}} අයත් වනුයේ වත්මන් ප්‍රවර්ගයටය.',
'category-file-count'            => '{{PLURAL:$2|මෙම ප්‍රවර්ගයට අයත් වන්නේ පහත දැක්වෙන ගොනුව පමණි.|සමස්ත $2 ගොනු ගණන අතුරින්, මෙහි පහත දැක්වෙන {{PLURAL:$1|ගොනුව|ගොනු $1 ගණන}} මෙම ප්‍රවර්ගය සතු වේ.}}',
'category-file-count-limited'    => 'මෙහි පහත දැක්වෙන {{PLURAL:$1|ගොනුව|ගොනු $1 ගණන}} අයත් වන්නේ වත්මන් ප්‍රවර්ගයටය.',
'listingcontinuesabbrev'         => 'ඉතිරිය.',

'mainpagetext'      => "<big>'''මාධ්‍යවිකි සාර්ථක ලෙස ස්ථාපනය කරන ලදි.'''</big>",
'mainpagedocfooter' => 'විකි මෘදුකාංග භාවිතා කිරීම පිළිබඳ තොරතුරු සඳහා  [http://meta.wikimedia.org/wiki/Help:Contents පරිශීලකයන් සඳහා නියමුව] හදාරන්න.

== ඇරඹුම ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings වින්‍යාස සැකසුම් ලැයිස්තුව]
* [http://www.mediawiki.org/wiki/Manual:FAQ මාධ්‍යවිකි නිතර-අසන-පැන]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce මාධ්‍යවිකි නිකුතුව තැපැල් ලැයිස්තුව]',

'about'          => 'පිළිබඳ',
'article'        => 'පටුන',
'newwindow'      => '(නව කවුළුවක විවෘත වේ)',
'cancel'         => 'අත් හරින්න',
'qbfind'         => 'සොයන්න',
'qbbrowse'       => 'පිරික්සන්න',
'qbedit'         => 'සංස්කරණය',
'qbpageoptions'  => 'මෙම පිටුව',
'qbpageinfo'     => 'සන්දර්භය',
'qbmyoptions'    => 'මගේ පිටු',
'qbspecialpages' => 'විශේෂ පිටු',
'moredotdotdot'  => 'තවත්...',
'mypage'         => 'මගේ පිටුව',
'mytalk'         => 'මගේ සාකච්ඡා',
'anontalk'       => 'මෙම අන්තර්ජාල ලිපිනය සඳහා සාකච්ඡාව',
'navigation'     => 'හසුරවන්න',
'and'            => 'සහ',

# Metadata in edit box
'metadata_help' => 'පාරදත්ත:',

'errorpagetitle'    => 'දෝෂය',
'returnto'          => '$1 ට නැවත යන්න.',
'tagline'           => '{{SITENAME}} වෙතින්',
'help'              => 'උදවු',
'search'            => 'ගවේෂණය',
'searchbutton'      => 'ගවේෂණය',
'go'                => 'යන්න',
'searcharticle'     => 'යන්න',
'history'           => 'පිටුවේ ඉතිහාසය',
'history_short'     => 'ඉතිහාසය',
'updatedmarker'     => 'මාගේ අවසාන මුණගැසීමෙන් පසුව යාවත්කාල කර ඇත',
'info_short'        => 'තොරතුරු',
'printableversion'  => 'මුද්‍රණ ආකෘතිය',
'permalink'         => 'ස්ථාවර සබැඳුම',
'print'             => 'මුද්‍රණය කරන්න',
'edit'              => 'සංස්කරණය',
'create'            => 'නිමවන්න',
'editthispage'      => 'මෙම පිටුව සංස්කරණය කරන්න',
'create-this-page'  => 'මෙම පිටුව නිර්මාණය කරන්න',
'delete'            => 'මකන්න',
'deletethispage'    => 'මෙම පිටුව මකන්න',
'undelete_short'    => '{{PLURAL:$1|එක් සංස්කරණයක|සංස්කරණ $1 ගණනක}} මකා දැමීම අවලංගු කරන්න',
'protect'           => 'ආරක්‍ෂණය කරන්න',
'protect_change'    => 'වෙනස් කරන්න',
'protectthispage'   => 'මෙම පිටුව ආරක්ෂා කරන්න',
'unprotect'         => 'ආරක්ෂා කිරීමෙන් ඉවත් වන්න',
'unprotectthispage' => 'මෙම පිටුව ආරක්ෂා කිරීමෙන් ඉවත් වන්න',
'newpage'           => 'නව පිටුව',
'talkpage'          => 'මෙම පිටුව පිළිබඳ සංවාදයකට එළඹෙන්න',
'talkpagelinktext'  => 'සාකච්ඡාව',
'specialpage'       => 'විශේෂ පිටුව',
'personaltools'     => 'පුද්ගලික මෙවලම්',
'postcomment'       => 'පරිකථනයක් ස්ථාපනය කරන්න',
'articlepage'       => 'අන්තර්ගත පිටුව නරඹන්න',
'talk'              => 'සංවාදය',
'views'             => 'නැරඹුම්',
'toolbox'           => 'මෙවලම් ගොන්න',
'userpage'          => 'පරිශීලක පිටුව නරඹන්න',
'projectpage'       => 'ව්‍යාපෘති පිටුව නරඹන්න',
'imagepage'         => 'මාධ්‍ය පිටුව නරඹන්න',
'mediawikipage'     => 'පණිවුඩ පිටුව නරඹන්න',
'templatepage'      => 'සැකිලි පිටුව නරඹන්න',
'viewhelppage'      => 'උදවු පිටුව නරඹන්න',
'categorypage'      => 'ප්‍රවර්ග පිටුව නරඹන්න',
'viewtalkpage'      => 'සංවාදය නරඹන්න',
'otherlanguages'    => 'වෙනත් භාෂා වලින්',
'redirectedfrom'    => '($1 වෙතින් යලි-යොමු කරන ලදි)',
'redirectpagesub'   => 'පිටුව යළි-යොමු කරන්න',
'lastmodifiedat'    => 'මෙම පිටුව අවසන් වරට වෙනස් කරන ලද්දේ $1 දිනදී, $2 වේලාවෙහිදීය.', # $1 date, $2 time
'viewcount'         => 'මෙම පිටුවට  {{PLURAL:$1|එක් වරක්|වාර $1 ගණනක්}} පිවිස ඇත.',
'protectedpage'     => 'ආරක්ෂිත පිටුව',
'jumpto'            => 'වෙත පනින්න:',
'jumptonavigation'  => 'හැසිරවීම',
'jumptosearch'      => 'ගවේෂණය',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} පිළිබඳ
<!--{{SITENAME}}About-->',
'aboutpage'            => 'Project:පිළිබඳ',
'bugreports'           => 'දෝෂ වාර්තා',
'bugreportspage'       => 'Project:දෝෂ වාර්තා',
'copyright'            => ' $1 යටතේ අන්තර්ගතය දැක ගත හැක.',
'copyrightpagename'    => '{{SITENAME}} කර්තෘ-හිමිකම්',
'copyrightpage'        => '{{ns:project}}:කර්තෘ-හිමිකම්',
'currentevents'        => 'කාලීන සිදුවීම්',
'currentevents-url'    => 'Project:කාලීන සිදුවීම්',
'disclaimers'          => 'වියාචනයන්',
'disclaimerpage'       => 'Project:පොදු වියාචන',
'edithelp'             => 'සංස්කරණ උදවු',
'edithelppage'         => 'Help:සංස්කරණ',
'faq'                  => 'නිතර-අසන-පැන',
'faqpage'              => 'Project:නිතර-අසන-පැන',
'helppage'             => 'Help:පටුන',
'mainpage'             => 'මුල් පිටුව',
'mainpage-description' => 'මුල් පිටුව',
'policy-url'           => 'Project:ප්‍රතිපත්තිය',
'portal'               => 'ප්‍රජා ද්වාරය',
'portal-url'           => 'Project:ප්‍රජා ද්වාරය',
'privacy'              => 'රහස්‍යභාවය ප්‍රතිපත්තිය',
'privacypage'          => 'Project: රහස්‍යභාවය ප්‍රතිපත්තිය',

'badaccess'        => 'අවසර දෝෂය',
'badaccess-group0' => 'ඔබ විසින් අයැදුම් කර සිටි කාර්යය ක්‍රියාත්මක කිරීමට ඔබ හට ඉඩ ලබා දෙනු නොලැබේ.',

'versionrequired'     => 'මාධ්‍යවිකි $1 අනුවාදය අවශ්‍ය වේ',
'versionrequiredtext' => 'මෙම පිටුව භාවිතා කිරීමට, මාධ්‍යවිකි හි $1 අනුවාදය අවශ්‍ය වේ.
[[Special:Version|අනුවාද පිටුව]] බලන්න.',

'ok'                      => 'හරි',
'pagetitle'               => '$1 - {{SITENAME}}',
'retrievedfrom'           => '"$1" වෙතින් නැවත ලබාගන්නා ලදි',
'youhavenewmessages'      => 'ඔබ හට $1 ($2) ඇත.',
'newmessageslink'         => 'නව පණිවුඩ',
'newmessagesdifflink'     => 'අවසාන වෙනස',
'youhavenewmessagesmulti' => 'ඔබ හට $1 හි නව පණිවුඩ ඇත',
'editsection'             => 'සංස්කරණය',
'editsection-brackets'    => '[$1]',
'editold'                 => 'සංස්කරණය',
'viewsourceold'           => 'මූලාශ්‍රය නරඹන්න',
'editsectionhint'         => 'ඡේද සංස්කරණය: $1',
'toc'                     => 'පටුන',
'showtoc'                 => 'පෙන්වන්න',
'hidetoc'                 => 'සඟවන්න',
'thisisdeleted'           => 'අවශ්‍යතාවය $1 නැරඹීමද නැතහොත් ප්‍රතිෂ්ඨාපනයද?',
'viewdeleted'             => '$1 නැරඹීම අවශ්‍යයද?',
'restorelink'             => '{{PLURAL:$1|මකා දමනු ලැබූ එක් සංස්කරණයක්| මකා දමනු ලැබූ සංස්කරණ $1  ගණනක්}}',
'feedlinks'               => 'පෝෂකය:',
'site-rss-feed'           => '$1 RSS පෝෂකය',
'site-atom-feed'          => '$1 Atom පෝෂකය',
'page-rss-feed'           => '"$1" RSS පෝෂකය',
'red-link-title'          => '$1 (තවමත් ලියා නොමැත)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'පිටුව',
'nstab-user'      => 'පරිශීලක පිටුව',
'nstab-media'     => 'මාධ්‍ය පිටුව',
'nstab-special'   => 'විශේෂ',
'nstab-project'   => 'ව්‍යාපෘති පිටුව',
'nstab-image'     => 'ගොනුව',
'nstab-mediawiki' => 'පණිවුඩය',
'nstab-template'  => 'සැකිල්ල',
'nstab-help'      => 'උදවු පිටුව',
'nstab-category'  => 'ප්‍රවර්ගය',

# Main script and global functions
'nosuchaction'      => 'මෙම නමැති කාර්යයක් නොමැත',
'nosuchspecialpage' => 'මෙම නමැති විශේෂ පිටුවක් නොමැත',
'nospecialpagetext' => "<big>'''ඔබ අයැද ඇත්තේ අවලංගු විශේෂ පිටුවකි.'''</big>

වලංගු විශේෂ පිටු දැක්වෙන ලැයිස්තුවක් [[Special:SpecialPages|{{int:specialpages}}]]හිදී ඔබහට සම්භ වනු ඇත.",

# General errors
'error'                => 'දෝෂය',
'databaseerror'        => 'පරිගණක දත්ත-ගබඩා දෝෂය',
'nodb'                 => 'පරිගණක දත්ත-ගබඩාව $1 තෝරාගත නොහැකි විය',
'laggedslavemode'      => 'අවවාදයයි: මෙම පිටුවෙහි මෑතදී සිදු කල  යාවත්නාල කිරීම් අඩංගු නොවිය හැක.',
'missingarticle-rev'   => '(සංශෝධනය#: $1)',
'missingarticle-diff'  => '(වෙනස: $1, $2)',
'internalerror'        => 'අභ්‍යන්තර දෝෂය',
'internalerror_info'   => 'අභ්‍යන්තර දෝෂය: $1',
'filecopyerror'        => '"$1" ගොනුව "$2" වෙත පිටපත් කිරීමට නොහැකි විය.',
'filerenameerror'      => '"$1" ගොනුව "$2" බවට යළි-නම්-කිරීම සිදු කල නොහැකි විය.',
'filedeleteerror'      => '"$1" ගොනුව මකා-දැමිය නොහැකි විය.',
'directorycreateerror' => '"$1" නාමාවලිය තැනීම කල නොහැකි විය.',
'filenotfound'         => '"$1" ගොනුව සොයා ගත නොහැකි විය.',
'fileexistserror'      => '"$1" ගොනුව වෙත ලිවීම කල නොහැකි විය: ගොනුව පවතියි',
'unexpected'           => 'අනපේක්‍ෂිත අගය: "$1"="$2".',
'formerror'            => 'දෝෂය: ආකෘති-පත්‍රය ඉදිරිපත් කල නොහැකි විය',
'badarticleerror'      => 'මෙම පිටුව විෂයයෙහි මෙම කාර්යය ඉටු නල නොහැකි විය.',
'cannotdelete'         => 'නිරූපිත පිටුව හෝ ගොනුව හෝ මකා දැමිය නොහැකි විය.
අනෙකෙකු විසින් දැනටමත් ‍මකා දැමීම සිදු කර ඇතිවා විය හැක.',
'badtitle'             => 'නුසුදුසු මාතෘකාවක්',
'badtitletext'         => 'අයැද ඇති පිටු මාතෘකාව එක්කෝ අවලංගු, හිස් නැත්නම් සාවද්‍ය ලෙස සබැඳි අන්තර්-භාෂා හෝ අන්තර්-විකී මාතෘකාවකි.
මාතෘකාවන්හි භාවිතා කල නොහැකි අක්ෂර එකක් හෝ කිහිපයක් හෝ එහි අඩංගු වීමට ඉඩ ඇත.',
'viewsource'           => 'මූලාශ්‍රය නරඹන්න',
'viewsourcefor'        => '$1 සඳහා',
'actionthrottled'      => 'ක්‍රියාව අවකරණය කරන ලදි',
'viewsourcetext'       => 'මෙම පිටුවෙහි මූලාශ්‍රය නැරඹීමට හා පිටපත් කිරීමට ඔබ හට හැකිය:',
'namespaceprotected'   => "'''$1''' නාමඅවකාශයෙහි පිටු සංස්කරණය කිරීමට ඔබහට අවසර නොමැත.",
'customcssjsprotected' => 'තවත් පරිශීලකයෙකුගේ පෞද්ගලික පරිස්ථිතිය අඩංගු වන බැවින්, මෙම පිටුව සංස්කරණය කිරීමට ඔබ හට අවසර නොමැත.',
'ns-specialprotected'  => 'විශේෂ පිටු සංස්කරණය කිරීම සිදු කල නොහැක.',

# Login and logout pages
'welcomecreation'            => '== ආයුබෝවන්, $1! ==

ඔබ‍ගේ ගිණුම තැනී ඇත.
ඔබ‍ගේ [[Special:Preferences|{{SITENAME}} අභිරුචි ]] වෙනස් කර ගන්න අමතක කරන්න එපා.',
'loginpagetitle'             => 'පරිශීලක ප්‍රවිෂ්ටය',
'yourname'                   => 'පරිශීලක නාමය:',
'yourpassword'               => 'මුරපදය:',
'yourpasswordagain'          => 'මුරපදය නැවත ලියන්න:',
'remembermypassword'         => 'මාගේ ප්‍රවිෂ්ටය පිළිබඳ විස්තර මෙම පරිගණක මතකයෙහි රඳවා තබා ගන්න',
'yourdomainname'             => 'ඔබගේ වසම:',
'loginproblem'               => '<b>ඔබගේ ප්‍රවිෂ්ටය පිළිබඳ ගැටළුවක් පැන නැගී ඇත.</b><br />නැවත උත්සාහ කරන්න!',
'login'                      => 'ප්‍රවිෂ්ටය',
'nav-login-createaccount'    => 'ප්‍රවිෂ්ට වන්න / ගිණුමක් තනන්න',
'loginprompt'                => '{{SITENAME}} වෙත ප්‍රවිෂ්ට වීම සඳහා ඔබ විසින් කුකීස් සක්‍රීය කොට තිබිය යුතුය.',
'userlogin'                  => 'ප්‍රවිෂ්ට වන්න / ගිණුමක් තනන්න',
'logout'                     => 'නිෂ්ක්‍රමණය',
'userlogout'                 => 'නිෂ්ක්‍රමණය',
'notloggedin'                => 'ප්‍රවිෂ්ට වී නොමැත',
'nologin'                    => 'ඔබ හට ගිණුමක් නොමැතිද? $1.',
'nologinlink'                => 'ගිණුමක් තනන්න',
'createaccount'              => 'ගිණුමක් තනන්න',
'gotaccount'                 => 'දැනටමත් ගිණුමක් තිබේද? $1.',
'gotaccountlink'             => 'ප්‍රවිෂ්ට වන්න',
'createaccountmail'          => 'විද්‍යුත් තැපෑල මගින්',
'badretype'                  => 'ඔබ ඇතුළු කල මුරපද නොගැලපෙති.',
'userexists'                 => 'ඔබ ඇතුළු කල පරිශීලක නාමය දැනටමත් භාවිතයෙහි ඇත.
කරුණාකර වෙනස් නමක් තෝරා ගන්න.',
'youremail'                  => 'විද්‍යුත් තැපෑල:',
'username'                   => 'පරිශීලක නාමය:',
'uid'                        => 'පරිශීලක අනන්‍යතාව:',
'prefs-memberingroups'       => 'ඉදිරියේ දැක්වෙන {{PLURAL:$1|කණ්ඩායමෙහි|කණ්ඩායම් වල}} සාමාජිකයෙකි:',
'yourrealname'               => 'සැබෑ නාමය:',
'yourlanguage'               => 'භාෂාව:',
'yourvariant'                => 'විචල්‍යය:',
'yournick'                   => 'විද්‍යුත් අත්සන:',
'badsiglength'               => 'විද්‍යුත් අත්සන පමණට වඩා දිගු වැඩිය.
එය $1 {{PLURAL:$1|අක්ෂරයට|අක්ෂරයන්ට}} වඩා කෙටි විය යුතුය.',
'email'                      => 'විද්‍යුත් තැපෑල',
'prefs-help-realname'        => 'සැබෑ නාමය හෙළි කිරීම වෛකල්පිකයි.
ඔබ විසින් එය හෙළි කල හොත්, ඔබගේ කෘතීන් සඳහා ඔබහට කතෘ-බුහුමන් පිරිනැමීමට එය භාවිතා කරනු ඇතk.',
'loginerror'                 => 'ප්‍රවිෂ්ට වීමේ දෝෂයකි',
'prefs-help-email'           => 'විද්‍යුත්-තැපෑල ලිපිනය සැපයීම වෛකල්පිකයි, එනමුදු ඔබගේ මුර-පදය ඔබහට අමතක වූ විටෙක නව මුර-පදයක් ඔබහට විද්‍යුත්-තැපැල්ගත කිරීමට එය ප්‍රයෝජනවත් විය හැක.
අනෙක් අතට, ඔබගේ අනන්‍යතාවය හෙළි නොකරමින්, අනෙකුන් හට ඔබ හා සම්බන්ධ වීමට අවස්ථාව සැලසීමට, ඔබගේ පරිශීලක පිටුව හෝ පරිශීලක_සාකච්ඡා පිටුව භාවිතා කිරීමට ඔබහට හැකිය.',
'prefs-help-email-required'  => 'විද්‍යුත් තැපෑල් ලිපිනය අවශ්‍යයි.',
'nocookiesnew'               => 'පරිශීලක ගිණුම තනා ඇති නමුදු, ඔබ ප්‍රවිෂ්ට වී නොමැත.
පරිශීලකයන් ප්‍රවිෂ්ට කර ගැනීම සඳහා, {{SITENAME}} විසින් කුකී භාවිතා කරයි.
ඔබ විසින් කුකී අක්‍රීය කර ඇත.
කරුණාකර ඒවා සක්‍රීය කොට, ඔබගේ නව පරිශීලක-නාමය හා මුර-පදය සමගින් ප්‍රවිෂ්ට වන්න.',
'nocookieslogin'             => 'පරිශීලකයන් ප්‍රවිෂ්ට කර ගැනීම සඳහා, {{SITENAME}} විසින් කුකී භාවිතා කරනු ලැබේ.
ඔබ විසින් කුකී අක්‍රීය නොට ඇත.
කරුණාකර, ඒවා සක්‍රීය කොට, නැවත උත්සාහ ‍කරන්න.',
'noname'                     => 'වලංගු පරිශීලක-නාමයක් සඳහන් කිරීමට ඔබ අසමත් වී ඇත.',
'loginsuccesstitle'          => 'ප්‍රවිෂ්ට වීම සාර්ථකයි',
'loginsuccess'               => "'''ඔබ දැන්, \"\$1\" ලෙස, {{SITENAME}} යට ප්‍රවිෂ්ට විමට සමත් වී ඇත.'''",
'nosuchuser'                 => '"$1" යන නමැති පරිශීලකයෙකු නොමැත.
ඔබගේ අක්ෂර-වින්‍යාසය පිරික්සා බැලීම හෝ, [[Special:Userlogin/signup|නව ගිණුමක් තැනීම]] හෝ සිදුකරන්න.',
'nosuchusershort'            => '"<nowiki>$1</nowiki>" යන නමැති පරිශීලකයෙකු නොමැත.
ඔබගේ අක්ෂර-වින්‍යාසය පිරික්සා බලන්න.',
'nouserspecified'            => 'ඔබ විසින් පරිශීලක-නාමයක් සඳහන් කල යුතු වේ.',
'wrongpassword'              => 'සාවද්‍ය මුර-පදයක් ඇතුළත් කෙරිණි.
නැවත උත්සාහ කොට බලන්න.',
'wrongpasswordempty'         => 'හිස් මුර-පදයක් ඇතුළත් කෙරිණි.
නැවත උත්සාහ කොට බලන්න.',
'passwordtooshort'           => 'ඔබගේ මුර-පදය එක්කෝ අවලංගු එකකි නැතිනම් පමණට වඩා කෙටි එකකි.
එහි අවම වශයෙන්,  {{PLURAL:$1|එක් අක්ෂරයක්|අක්ෂර $1 සංඛ්‍යාවක්}} අඩංගු විය යුතු අතර, ඔබගේ පරිශීලක-නාමයෙන් වෙනස් පදයක් විය යුතුය.',
'mailmypassword'             => 'නව මුරපදය විද්‍යුත් තැපෑල‍ට යවන්න',
'passwordremindertitle'      => '{{SITENAME}} සඳහා නව තාවකාලික මුර-පදය',
'passwordremindertext'       => 'යම් අයෙකු  ($1 විද්‍යුත් තැපැල් ලිපිනය තුලින් සමහර විට ඔබ) විසින්  {{SITENAME}} ($4) 
සඳහා නව මුර-පදයක් ඉල්ලා සිට ඇත. පරිශීලක "$2"  වෙනුවෙන් තාවකාලික 
 මුර-පදයක් තනා "$3" බවට නියම කර ඇත. මෙය ඔබගේ අභිලාශය වූයේ නම් 
ඔබ විසින් ළහිළහියේ ප්‍රවිෂ්ට වී, නව මුර-පදයක් තෝරා ගත යුතුව ඇත.

වෙන යම් අයෙකු විසින් මෙම ආයාචනය සිදු කර ඇත්නම් හෝ ඔබ හට ඔබගේ මුර-පදය නැවත 
සිහිවුනි නම් හා එබැවින් එය වෙනස් කිරීම තවදුරටත් ඔබගේ අභිලාෂය නොවේ නම්
මෙම පණිවුඩය නොසලකාහරිමින් ඔබගේ පැරැණි මුර-පදය දිගටම භාවිතා කරන්න.',
'noemail'                    => 'පරිශීලක  "$1" සඳහා විද්‍යුත්-තැපැල් ලිපිනයක් සටහන් වී නොමැත.',
'passwordsent'               => ' "$1" වෙනුවෙන් ලේඛනගත කර ඇති විද්‍යුත් තැපැල් ලිපිනයට නව මුර පදයක් යවා ඇත.
ඔබට එය ලැබුනු පසු ප්‍රවිෂ්ට වන්න.',
'eauthentsent'               => 'නම් කර ඇති විද්‍යුත්-තැපැල් ලිපිනය වෙත, තහවුරු කිරීම් විද්‍යුත්-තැපෑලක් යැවීම, දැනටමත් සිදු කර ඇත.
වෙන යම් විද්‍යුත්-තැපෑලක් ගිනුම වෙත එවීමට පෙර, ගිණුම සත්‍ය වශයෙන්ම ඔබගේම බව තහවුරු කරනු වස්, විද්‍යුත්-තැපෑලෙහි අඩංගු උපදෙස්  පිළිපැදීමට ඔබ හට සිදු වනු ඇත.',
'acct_creation_throttle_hit' => 'ඔබ දැන‍ටමත් ගිණුම $1 තනා ඇත.ඔබට තවත් ගිණුම් තැනිය නොහැක.',
'emailconfirmlink'           => 'ඔබගේ විද්‍යුත් තැපැල් ලිපිනය තහවුරු කරන්න',
'loginlanguagelabel'         => 'භාෂාව: $1',

# Password reset dialog
'resetpass_header' => 'මුරපදය යළි පිහිටුවන්න',

# Edit page toolbar
'bold_sample'     => 'තදකුරු',
'bold_tip'        => 'තදකුරු',
'italic_sample'   => 'ඇලකුරු',
'italic_tip'      => 'ඇලකුරු',
'link_sample'     => 'සබැඳියෙහි මාතෘකාව',
'link_tip'        => 'අභ්‍යන්තර සබැඳිය',
'extlink_sample'  => 'http://www.example.com සබැඳියෙහි මාතෘකාව',
'extlink_tip'     => 'බාහිර සබැඳිය ( http:// උපසර්ගය සිහි තබාගන්න)',
'headline_sample' => 'සිරස්තල  පෙළ',
'headline_tip'    => '2වන මට්ටමෙහි සිරස්තලය',
'math_sample'     => 'සූත්‍රය මෙහි රුවන්න',
'math_tip'        => 'ගණිත සුත්‍ර(LaTeX)',
'nowiki_sample'   => 'ආකෘතිකරණය-නොකල පෙළ මෙහි රුවන්න',
'nowiki_tip'      => 'විකි ආකෘතිකරණය නොසලකාහරින්න',
'image_tip'       => 'නිවේශිත ගොනුව',
'media_tip'       => 'ගොනු සබැඳිය',
'sig_tip'         => 'වේලා-මුද්‍රාව හා සමග ඔබගේ විද්‍යුත් අත්සන',
'hr_tip'          => 'තිරස් පේළිය (අවම වශයෙන් භාවිතා කරන්න)',

# Edit pages
'summary'                => 'සාරාංශය',
'subject'                => 'මාතෘකාව/සිරස් තලය',
'minoredit'              => 'මෙය සුළු සංස්කරණයකි',
'watchthis'              => 'මෙම පිටුව මුර කරන්න',
'savearticle'            => 'පිටුව සුරකින්න',
'preview'                => 'පෙරදසුන',
'showpreview'            => 'පෙරදසුන පෙන්වන්න',
'showdiff'               => 'වෙනස්වීම් පෙන්වන්න',
'anoneditwarning'        => "'''අවවාදයයි:''' ඔබ පරිශීලකයෙකු වශයෙන් පද්ධතියට ප්‍රවිෂ්ට වී නොමැත.
එමනිසා මෙම පිටුවෙහි සංස්කරණ ඉතිහාසයෙහි, ඔබගේ අන්තර්ජාල ලිපිනය සටහන් කරගැනීමට සිදුවනු ඇත.",
'summary-preview'        => 'සාරාංශ පෙර-දසුන',
'blockedtext'            => "<big>ඔබගේ පරිශීලක නාමය හෝ අන්තර්ජාල ලිපිනය හෝ වාරණය කොට ඇත.'''</big>

මෙම වාරණය සිදුකොට ඇත්තේ  $1 විසිනි.
මේ සඳහා දී ඇති හේතුව ''$2'' වේ.

* වාරණයෙහි ඇරඹුම: $8
*වාරණයයෙහි අවසානය: $6
* අදහස් කරන ලද  වාරණ-ලාභී: $7

වාරණය පිළිබඳ සංවාදයකට එළඹීමෙනු වස්, $1 හෝ  වෙනත් [[{{MediaWiki:Grouppage-sysop}}|පරිශීලකයෙකු]] හෝ සම්බන්ධ කරගැනීමට ඔබ හට හැකිය.
ඔබගේ  [[Special:Preferences|ගිණුම් අභිරුචි]] වල, වලංගු විද්‍යුත්-තැපැල් ලිපිනයක් නිරූපනය කොට  ඇති නම් හා ඔබ විසින් එය භාවිත කිරීම වාරණය කොට නෙමැති නම් මිස,  'මෙම පරිශීලකයාට විද්‍යුත්-තැපෑලක් යවන්න' යන අංගය ඔබ විසින් භාවිතා කල නොහැකිය.
ඔබගේ වත්මන් අන්තර්ජාල ලිපිනය  $3 වන අතර, වාරණ අනන්‍යතාවය #$5 වේ.
ඔබ විසින් සිදු කරන ඕනෑම විමසුමකදී ඉහත සියළු විස්තර අඩංගු කරන්න.",
'newarticle'             => '(නව)',
'newarticletext'         => "බැඳියක් ඔස්සේ පැමිණ ඔබ අවතීර්ණ වී ඇත්තේ දැනට නොපවතින ලිපියකටයි.
මෙම ලිපිය තැනීමට එනම් නිමැවීමට අවශ්‍ය නම්, පහත ඇති කොටුව තුල අකුරු ලිවීම අරඹන්න (වැඩිමනත් තොරතුරු සඳහා [[{{MediaWiki:Helppage}}|උදවු පිටුව]] බලන්න).
ඔබ මෙහි අවතීර්ණ වී ඇත්තේ කිසියම් අත්වැරැද්දකින් බව හැ‍‍ඟෙන්නේ නම්, ඔබගේ සැරිසරයෙහි (බ්‍රවුසරයෙහි) '''පසුපසට''' බොත්තම ක්ලික් කරන්න.",
'noarticletext'          => 'වර්තමානයෙහිදී මෙම පිටුවෙහි කිසිදු පෙළක් නොමැත, ඔබ හට, අනෙකුත් පිටු තුල  [[Special:Search/{{PAGENAME}}|මෙම පිටු -නාමය සඳහා ගවේෂණය කල හැක]] නැතහොත් [{{fullurl:{{FULLPAGENAME}}|action=edit}} මෙම පිටුව සංස්කරණය කල හැක].',
'previewnote'            => '<strong>මෙය පෙරදසුනක් පමණකි;
වෙනස්කම් සුරැකීම තවමත් සිදුකොට නොමැත!</strong>',
'editing'                => '$1 සංස්කරණය කරමින් පවතියි',
'editingsection'         => '$1 (ඡේදය) සංස්කරණය කරමින් පවතියි',
'editingcomment'         => '$1 (පරිකථනය) සංස්කරණය කරමින් පවතියි',
'yourtext'               => 'ඔබගේ පෙළ',
'copyrightwarning'       => '{{SITENAME}} සඳහා ඔබ විසින් දායක වන කෘතීන් පල කොට මුදා හැරීමෙහිදී,  $2 ට යටත් වන බව කරුණාවෙන් සලකන්න (වැඩි විස්තර සඳහා $1 බලන්න). ඔබගේ ලියැවිලි, අනෙකුන් විසින් හිත්පිත් නොමැති තරම් ඉතාමත් රළු අයුරින් සංස්කරණය කිරීම හා ඔවුන්ගේ රිසිය පරිදි  ප්‍රතිසංවිධානය කිරීම,  ඔබ හට දරා ගැනීමට නොහැකි නම්, ඔබගේ කෘති මෙහි පල කිරීමෙන් වලකින්න.<br />
එසේ ම මෙය ඔබ විසින් ම ලියූ බවට හෝ පොදු විෂයපථයකින්, ඊ‍ට ස‍මාන නිදහස් මූලාශ්‍රයකින් උපුටා ගත් බව‍ට හෝ අපහ‍‍ට සහතික විය යුතු ය. (තොරතුරු සඳහා $1 බලන්න).
<strong>හිමිකම් ඇවුරුණු දේ අනවසරයෙන් ප්‍රකාශ කිරිමෙන් වලකින්න!</strong>',
'copyrightwarning2'      => "Please note that all contributions to {{SITENAME}} may be edited, altered, or removed by other contributors. If you don't want your writing to be edited mercilessly, then don't submit it here.<br />
එසේ ම මෙය ඔබ විසින් ම ලියූ බවට හෝ පොදු විෂයපථයකින්, ඊ‍ට ස‍මාන නිදහස් මූලාශ්‍රයකින් උපුටා ගත් බව‍ට හෝ අපහ‍‍ට සහතික විය යුතු ය. (තොරතුරු සඳහා $1 බලන්න).
<strong>හිමිකම් ඇවුරුණු දේ අනවසරයෙන් ප්‍රකාෂ කිරිමෙන් වලකින්න!</strong>",
'longpagewarning'        => '<strong>අවවාදයයි: මෙම පිටුව කිලෝ බයිට්  $1 ගණනක් දිගුය;
 32කි.බ. පමණට කිට්ටු හෝ ඊට වඩා දිගු පිටු සංස්කරණය කිරීම සමහරක් බ්‍රවුසර වලට දුෂ්කර විය හැක.
මෙම  ‍පිටුව කුඩා කොටස් වලට බෙදීම පිළිබඳව කරුණාකර අවධානය යොමු කරන්න.</strong>',
'templatesused'          => 'මෙම පිටුවෙහි භාවිතා කල සැකිලි:',
'templatesusedpreview'   => 'මෙම පෙර-දසුනෙහි භාවිතා වන සැකිලි:',
'template-protected'     => '(රක්ෂිත)',
'template-semiprotected' => '(අර්ධ-රක්ෂිත)',
'hiddencategories'       => 'මෙම පිටුව, {{PLURAL:$1| එක් සැඟවුණු ප්‍රවර්ගයක| සැඟවුණු ප්‍රවර්ගයන් $1 ගණනක}} අවයවයක් වේ:',
'edittools'              => '<!-- Text here will be shown below edit and upload forms. -->',
'nocreatetitle'          => 'පිටු තැනීම සීමා කර ඇත',
'nocreatetext'           => 'නව පිටු තැනීමේ හැකියාව {{SITENAME}} විසින් සීමාකර ඇත.
ඔබ හට පෙරළා ගොස්,  දැනට පවතින පිටුවක් සංස්කරණය කිරීම හෝ,  [[Special:UserLogin|ගිණුමකට ප්‍රවිෂ්ට වීම හෝ  නව ගිණුමක් තැනීම හෝ]] සිදුකල හැක.',
'nocreate-loggedin'      => '{{SITENAME}} හි නව පිටු තැනීමට අවසරයක් ඔබ හට ප්‍රදානය කොට නොමැත.',
'permissionserrors'      => 'අවසරයන් පිළිබඳ දෝෂයන් පවතී',
'permissionserrorstext'  => 'පහත දැක්වෙන {{PLURAL:$1|හේතුව|හේතූන්}} නිසා, ඔබ හට එය සිදුකිරීමට අවසර ලබා දීමට නොහැක:',
'recreate-deleted-warn'  => "'''අවවාදයයි: පෙරදී මකා දැමූ ගොනුවක් ඔබ විසින් යලි-තනමින් පවතියි.'''

මෙම පිටුව සංස්කරණය කිරීම තවදුරටත් සිදුකරගෙන යාම සුදුසුද යන වග ඔබ විසින් සලකා බැලිය යුතුය.
මෙම පිටුවට අදාල මකා දැමීම් පිළිබඳ විස්තර දැක්වෙන මකා-දැමීම්-ලඝු-සටහන ඔබගේ පහසුව තකා මෙහි දක්වා ඇත:",

# Parser/template warnings
'post-expand-template-inclusion-warning'  => 'අවවාදයයි: සැකිලි අඩංගු කිරීමේ ප්‍රමාණය අවසර ලබා දී ඇති පමණට වඩා විශාලයි.
සමහරක් සැකිලි අඩංගු නොකරනු ඇත.',
'post-expand-template-inclusion-category' => 'මෙම පිටු තුල, සැකිලි අඩංගු කිරීමේ පුමාණය, අවසර දී ඇති සීමා ඉක්මවා ගොස් ඇත',
'post-expand-template-argument-warning'   => 'අවවාදයයි: ව්‍යාප්ති ප්‍රමාණය ඇවැසි තරමට වඩා විශාල ලෙස දක්වා ඇති සැකිලි විචල්‍යයන් අඩුම වශයෙන් එකක් හෝ  මෙම පිටුව තුල අන්තර්ගතය.
එම විචල්‍යයන් නොසලකා හැර ඇත.',
'post-expand-template-argument-category'  => 'මෙම පිටුවල, සැකිලි විචල්‍යයන් හරියාකාර දැක්වීම පැහැර හැරීම පිළිබඳ ගැටළු පවතී',

# Account creation failure
'cantcreateaccounttitle' => 'ගිණුම තැනිය නොහැක',
'cantcreateaccount-text' => "මෙම අන්තර්ජාල ලිපිනය ('''$1''') මගින් ගිණුම් තැනීම [[User:$3|$3]] විසින් වාරණය කොට ඇත.

$3 විසින් සපයා ඇති හේතුව ''$2'' වේ",

# History pages
'viewpagelogs'        => 'මෙම පිටුව සඳහා ලඝු-සටහන් නරඹන්න',
'nohistory'           => 'මෙම පිටුව සඳහා සංස්කරණ ඉතිහාසයක් නොමැත.',
'currentrev'          => 'වත්මන් සංශෝධනය',
'revisionasof'        => '$1 තෙක් සංශෝධනය',
'revision-info'       => '$1 වන විට  $2 විසින් සිදු කර ඇති සංශෝධන',
'previousrevision'    => '← පුරාණ සංශෝධනය',
'nextrevision'        => 'නවීන සංශෝධනය →',
'currentrevisionlink' => 'වත්මන් සංශෝධනය',
'cur'                 => 'වත්මන්',
'last'                => 'අවසන්',
'page_first'          => 'පළමු',
'page_last'           => 'අවසන්',
'histlegend'          => 'වෙනස තේරීම: සැසඳිය යුතු අනුවාදයන්හි  රේඩියෝ බොක්ස් සලකුණු කොට ඉන්පසු එන්ටර් බොත්තම එබීම හෝ පහළින්ම ඇති බොත්තම එබීම කරන්න.<br />
ආඛ්‍යායිකාව: (වත්මන්) = වත්මන් අනුවාදය හා සමග වෙනස,
(අවසන්) = පෙර අනුවාදය හා සමග වෙනස, සුළු = සුළු සංස්කරණය.',
'histfirst'           => 'පැරණිතම',
'histlast'            => 'නවීනතම',
'historyempty'        => '(හිස්)',

# Revision feed
'history-feed-title'          => 'සංශෝධන ඉතිහාසය',
'history-feed-item-nocomment' => '$1 විසින්  $2 හිදී', # user at time

# Revision deletion
'rev-delundel' => 'පෙන්වන්න/සඟවන්න',

# Diffs
'history-title'           => '"$1" හි සංශෝධන ඉතිහාසය',
'difference'              => '(අනුවාද අතර වෙනස්කම්)',
'lineno'                  => 'පේළිය $1:',
'compareselectedversions' => 'තෝරාගෙන ඇති අනුවාද සසඳන්න',
'editundo'                => 'ආපසු',
'diff-multi'              => '({{PLURAL:$1|එක් අතරමැදි සංශෝධනයක්| අතරමැදි සංශෝධන $1 ගණනාවක්}} පෙන්නුම් කර නොමැත.)',

# Search results
'noexactmatch' => "''' \"\$1\" යන නාමය හිමි  පිටුවක් නොමැත.'''
ඔබ හට [[:\$1|මෙම පිටුව තැනිය හැක]].",
'prevn'        => 'පූර්ව  $1',
'nextn'        => 'ඊලඟ  $1',
'viewprevnext' => '($1) ($2) ($3) නරඹන්න',
'powersearch'  => 'ගැඹුරින් ගවේෂණය කරන්න',

# Preferences page
'preferences'    => 'අභීරුචි',
'mypreferences'  => 'මගේ අභිරුචි',
'changepassword' => 'මුරපදය වෙනස් කරන්න',
'datetime'       => 'දිනය සහ වේලාව',
'prefs-personal' => 'පරිශීලක පැතිකඩ',
'prefs-rc'       => 'නව වෙනස්වීම්',
'prefs-misc'     => 'විවිධ',
'saveprefs'      => 'Save',
'resetprefs'     => 'යළි පිහිටුවන්න',
'retypenew'      => 'නව මුර-පදය නැවත ටයිප් කරන්න:',
'files'          => 'ගොනු',

'grouppage-sysop' => '{{ns:project}}:පරිපාලකවරු',

# User rights log
'rightslog' => 'පරිශීලක හිමිකම් ලඝු-සටහන',

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|වෙනස්කම|වෙනස්කම්}}',
'recentchanges'                  => 'මෑතදී සිදුවූ වෙනස්වීම්',
'recentchanges-feed-description' => 'මෙම පෝෂකයෙහි විකියට බොහෝ මෑතදී සිදුකල වෙනස්කම් හෙළිකරන්න.',
'rcnote'                         => "$5, $4 වන තෙක් සැලකිල්ලට ගත් කල, අවසන් {{PLURAL:$2|දිනදී|දින '''$2''' ගණන තුලදී}} සිදුවී ඇති, {{PLURAL:$1| '''1''' වෙනස|අවසන් '''$1''' වෙනස්කම් ගණන}} මෙහි පහත දැක්වේ.",
'rcnotefrom'                     => "'''$2''' න් පසු සිදුවී ඇති වෙනස්කම් මෙහි පහත දැක්වේ ('''$1''' ක ප්‍රමාණයක උපරිමයක් පෙන්වා ඇත).",
'rclistfrom'                     => '$1 සිට බලපැවැත්වෙන නව වෙනස්වීම් පෙන්වන්න',
'rcshowhideminor'                => 'සුළු සංස්කරණ $1 ගණනක්',
'rcshowhidebots'                 => 'රොබෝ $1 දෙනෙක්',
'rcshowhideliu'                  => 'ප්‍රවිෂ්ට වූ පරිශීලකයන් $1 දෙනෙකි',
'rcshowhideanons'                => 'නිර්නාමික පරිශීලකයෝ $1 ගණනක්',
'rcshowhidepatr'                 => 'පරික්‍ෂා කර බැලූ සංස්කරණ $1 ගණනකි',
'rcshowhidemine'                 => 'මගේ සංස්කරණ $1 ගණනක්',
'rclinks'                        => 'අවසන් දින $2 ගණන තුලදී සිදුවී ඇති අවසන් වෙනස්කම් $1 ගණන පෙන්නුම් කරන්න<br />$3',
'diff'                           => 'වෙනස',
'hist'                           => 'විත්ති',
'hide'                           => 'සඟවන්න',
'show'                           => 'පෙන්වන්න',
'minoreditletter'                => 'සුළු',
'newpageletter'                  => 'නව',
'boteditletter'                  => 'රොබෝ',
'sectionlink'                    => '→',

# Recent changes linked
'recentchangeslinked'          => 'සබැඳි වෙනස්වීම්',
'recentchangeslinked-title'    => '"$1" ට සම්බන්ධී වෙනස්කම්',
'recentchangeslinked-noresult' => 'සලකා බැලූ කාලසීමාවෙහිදී, සබැඳි පිටු වල කිසිදු වෙනසක් සිදුවී නොමැත.',
'recentchangeslinked-summary'  => "විශේෂී ලෙස නිරූපිත පිටුවකට (හෝ විශේෂි ලෙස නිරූපිත ප්‍රවර්ගයක සාමාජීකයන්ට) සබැඳි පිටුවල  මෑතදී සිදුවූ වෙනස්කම් දැක්වෙන ලැයිස්තුවක් මෙහි දැක්වේ.
[[Special:Watchlist|ඔබගේ  මුර-ලැයිස්තුවෙහි]] පිටු  '''තදකුරු''' වලින් දක්වා ඇත.",

# Upload
'upload'        => 'ගොනුවක් උඩුගත කිරීම',
'uploadbtn'     => 'ගොනුව උඩුගත කරන්න',
'uploadlogpage' => 'ලඝු-සටහන උඩුගත කරන්න',
'uploadedimage' => '"[[$1]]" උඩුගත කරන ලදි',

# Special:ImageList
'imagelist' => 'ගොනු ලැයිස්තුව',

# Image description page
'filehist'                  => 'ගොනු ඉතිහාසය',
'filehist-help'             => 'එම අවස්ථාවෙහිදී  ගොනුව පැවැති ආකාරය නැරඹීම ඔබ හට රිසි නම්  දිනය/වේලාව මත ක්ලික් කරන්න.',
'filehist-current'          => 'වත්මන්',
'filehist-datetime'         => 'දිනය/කාලය',
'filehist-user'             => 'පරිශීලක',
'filehist-dimensions'       => 'මාන',
'filehist-filesize'         => 'ගොනුවේ විශාලත්වය',
'filehist-comment'          => 'පරිකථනය',
'imagelinks'                => 'සබැඳි',
'linkstoimage'              => 'මෙම ගොනුවට  {{PLURAL:$1|ලිපිය බැ‍ඳෙයි|ලිපි $1 ගණනක් බැඳෙති}}:',
'nolinkstoimage'            => 'මෙම ගොනුවට සබැඳෙන පිටු කිසිවක් නොමැත.',
'sharedupload'              => 'මෙම ගොනුව හවුල් උඩුගත කිරීමක් වන අතර අනෙකුත් ව්‍යාපෘති සඳහාද භාවිතා කල හැකි වෙයි.',
'noimage'                   => 'මෙම නම සහිත ගොනුවක් නොපවතින නමුදු, ඔබ හට $1 සිදු කල හැක.',
'noimage-linktext'          => 'එකක් උඩුගත කරන්න',
'uploadnewversion-linktext' => 'මෙම ගොනුවෙහි නව අනුවාදයක් උඩුගත කරන්න',

# MIME search
'mimesearch' => 'MIME ගවේෂණය',

# List redirects
'listredirects' => 'යළි-යොමුවීම් ලැයිස්තුව',

# Unused templates
'unusedtemplates' => 'භාවිතා නොවූ සැකිලි',

# Random page
'randompage' => 'අහඹු පිටුව',

# Random redirect
'randomredirect' => 'අහුඹු යළි-ෙයාමුකිරීම',

# Statistics
'statistics' => 'සංඛ්‍යාන දත්ත',

'disambiguations' => 'වක්‍රෝත්තිහරණ පිටු',

'doubleredirects' => 'ද්විත්ව යළි-යොමුකිරීම්',

'brokenredirects' => 'භින්න යළි-යොමුවීම්',

'withoutinterwiki' => 'භාෂා සබැඳි විරහිත පිටු',

'fewestrevisions' => 'ස්වල්පතම සංශෝධන සහිත පිටු',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|බයිටය|බයිට්}}',
'ncategories'             => '$1 {{PLURAL:$1|ප්‍රවර්ගය|ප්‍රවර්ගයන්}}',
'nlinks'                  => '$1 {{PLURAL:$1|සබැඳිය|සබැඳියන්}}',
'nmembers'                => '$1 {{PLURAL:$1|සාමාජීකයා|සාමාජීකයන්}}',
'lonelypages'             => 'හුදෙකලා පිටු',
'uncategorizedpages'      => 'ප්‍රවර්ගීකරණය නොවූ පිටු',
'uncategorizedcategories' => 'ප්‍රවර්ගීකරණය නොවූ ප්‍රවර්ග',
'uncategorizedimages'     => 'ප්‍රවර්ගීකරණය නොවූ ගොනු',
'uncategorizedtemplates'  => 'ප්‍රවර්ගීකරණය නොවූ සැකිලි',
'unusedcategories'        => 'භාවිතා නොවූ ප්‍රවර්ග',
'unusedimages'            => 'භාවිතා නොවූ ගොනු',
'wantedcategories'        => 'අවශ්‍ය ප්‍රවර්ග',
'wantedpages'             => 'අවශ්‍ය පිටු',
'mostlinked'              => 'පිටු වලට බෙහෙවින්ම සබැඳි',
'mostlinkedcategories'    => 'ප්‍රවර්ගයන්ට බෙහෙවින්ම සබැඳි',
'mostlinkedtemplates'     => 'සැකිලි වලට බෙහෙවින්ම සබැඳි',
'mostcategories'          => 'ප්‍රවර්ගයන් බොහෝමයක් සහිත පිටු',
'mostimages'              => 'ගොනු වලට බෙහෙවින්ම සබැඳි',
'mostrevisions'           => 'වඩාත්ම සංශෝධන සහිත පිටු',
'prefixindex'             => 'උපසර්ග සූචිය',
'shortpages'              => 'කෙටි පිටු',
'longpages'               => 'දිගු පිටු',
'deadendpages'            => 'පියැවි-අගැති පිටු',
'protectedpages'          => 'ආරක්ෂිත පිටු',
'listusers'               => 'පරිශීලක ලැයිස්තුව',
'newpages'                => 'අළුත් පිටු',
'ancientpages'            => 'පුරාණතම පිටු',
'move'                    => 'ගෙනයන්න',
'movethispage'            => 'මෙම පිටුව ගෙන යන්න',

# Book sources
'booksources'    => 'ග්‍රන්ථ මූලාශ්‍ර',
'booksources-go' => 'යන්න',

# Special:Log
'specialloguserlabel'  => 'පරිශීලකයා:',
'speciallogtitlelabel' => 'මාතෘකාව:',
'log'                  => 'ලඝු-සටහන්',
'all-logs-page'        => 'සියළු ලඝු-සටහන්',
'log-search-submit'    => 'යන්න',

# Special:AllPages
'allpages'       => 'සියළු පිටු',
'alphaindexline' => '$1 සි‍ට $2 වෙත',
'nextpage'       => 'ඊළඟ පිටුව ($1)',
'prevpage'       => 'පූර්ව පිටුව ($1)',
'allpagesfrom'   => 'මෙහිදී ඇරඹෙන පිටු පෙන්වන්න:',
'allarticles'    => 'සියළු පිටු',
'allpagessubmit' => 'යන්න',
'allpagesprefix' => 'මෙම උපසර්ගය සහිත පිටු පෙන්වන්න:',

# Special:Categories
'categories' => 'ප්‍රවර්ග',

# Special:ListUsers
'listusers-submit' => 'පෙන්වන්න',

# E-mail user
'emailuser' => 'මෙම පරිශීලකයාහට විද්‍යුත්-තැපෑලක් යවන්න',

# Watchlist
'watchlist'            => 'මගේ මුර-ලැයිස්තුව',
'mywatchlist'          => 'මගේ මුර ලැයිස්තුව',
'watchlistfor'         => "('''$1''' සඳහා)",
'addedwatch'           => 'මුර-ලැයිස්තුවට එක් කරන ලදි',
'addedwatchtext'       => "\"[[:\$1]]\" පිටුව ඔබගේ [[Special:Watchlist|මුර-ලැයිස්තුවට]] එක් කොට ඇත.
මෙම පිටුවට සහ එයට අදාළ සාකච්ඡා පිටුවට ඉදිරියෙහිදී සිදු කෙරෙන වෙනස් කම් මෙහි ලේඛනගත වන අතර, ප්‍රභේදනය කර ගැනීමෙහි පහසුව තකා,  [[Special:RecentChanges|මෑත වෙනස්කම් ලැයිස්තුව]]  තුල මෙම පිටුව  '''තදකුරු''' වලින් දක්වනු ඇත.",
'removedwatch'         => 'මුර-ලැයිස්තුවෙන් ඉවත් කරන ලදි',
'removedwatchtext'     => 'මෙම "[[:$1]]"  පිටුව  [[Special:Watchlist|ඔබගේ  මුර-ලැයිස්තුවෙන්]] ඉවත් කරන ලදි.',
'watch'                => 'මුර කරන්න',
'watchthispage'        => 'මෙම පිටුව මුර කරන්න',
'unwatch'              => 'මුර නොකරන්න',
'watchlist-details'    => 'සාකච්ඡා පිටු ‍නොගිණිය කල, ඔබගේ මුර-ලැයිස්තුවෙහි {{PLURAL:$1|$1 පිටුවක්|පිටු $1 ගණනක්}} ඇත.',
'wlshowlast'           => 'අවසන් පැය  $1 දින  $2  $3 පෙන්වන්න',
'watchlist-hide-bots'  => 'රොබෝ සංස්කරණ සඟවන්න',
'watchlist-hide-own'   => 'මාගේ සංස්කරණ සඟවන්න',
'watchlist-hide-minor' => 'සුළු සංස්කරණ සඟවන්න',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'මුර කරමින්...',
'unwatching' => 'මුර නොකරමින්...',

'enotif_mailer' => '{{SITENAME}}හි නිවේදන යවන්නා',

# Delete/protect/revert
'deletepage'                  => 'පිටුව මකා දමන්න',
'historywarning'              => 'අවවාදයයි: ඔබ විසින් මකා දැමීමට සූදානම් වන පිටුවට ඉතිහාසයක් ඇත:',
'confirmdeletetext'           => 'එහි සමස්ත ඉතිහාසය හා සමගින් පිටුවක් මකා දැමීමට ඔබ සැරසෙයි.
ඔබගේ අභිමතාර්ථය මෙයමදැයි අවලෝකනය කරමින්, මෙහි ප්‍රතිවිපාක මුළුමනින් ඔබ විසින් අවබෝධ කරගෙන ඇති බවට සෑහීමට පත් වෙමින් හා, ඔබ මෙය සිදුකරන්නේ  [[{{MediaWiki:Policy-url}}|ප්‍රතිපත්තියට]] අනුකූලවදැයි විමසා බලන්න.',
'actioncomplete'              => 'ක්‍රියාව සමාප්තයි',
'deletedtext'                 => '"<nowiki>$1</nowiki>" මකා දමා ඇත.
මෑත මකාදැමීම් පිළිබඳ වාර්තාවක් සඳහා $2 බලන්න.',
'deletedarticle'              => '"[[$1]]" මකා දමන ලදි',
'dellogpage'                  => 'මකා-දැමීම් ලඝු සටහන',
'deletecomment'               => 'මකා දැමීමට හේතුව:',
'deleteotherreason'           => 'අනෙකුත්/අමතර හේතුව:',
'deletereasonotherlist'       => 'අනෙකුත් හේතුව',
'rollbacklink'                => 'පෙරළායෑම',
'protectlogpage'              => 'ආරක්ෂණය කිරීම් දැක්වෙන ලඝු-සටහන',
'protectcomment'              => 'පරිකථනය:',
'protectexpiry'               => 'ඉකුත් වීම:',
'protect_expiry_invalid'      => 'අවලංගු ඉකුත් වීමේ කාලයකි.',
'protect_expiry_old'          => 'ඉකුත් වීමේ කාලය දැනටමත් ඉක්ම ගොස් ඇත.',
'protect-unchain'             => 'ගෙන යාම පිළිබඳ දැනට පනවා ඇති වාරණය ඉවත් කරන්න',
'protect-text'                => '<strong><nowiki>$1</nowiki></strong> පිටුව සඳහා ආරක්ෂණ මට්ටම නැරඹීම හා වෙනස් කිරීම මෙහිදී ඔබ විසින් සිදු කල හැක.',
'protect-locked-access'       => 'පිටුවෙහි ආරක්ෂණ මට්ටම් වෙනස් කිරීම සඳහා ඔබගේ ගිණුමට අවසර නැත.
පිටුවෙහි වත්මන් සැකසුම් මෙහි දැක්වේ <strong>$1</strong>:',
'protect-cascadeon'           => 'තීරු දර්ශන ආරක්ෂණය බල ගන්වා ඇති පහත  {{PLURAL:$1|පිටුව|පිටු}} අන්තර්ගත වීම හේතුවෙන් මෙම පිටුව දැනට ආරක්ෂණයට ලක්ව ඇත.
පිටුවෙහි ආරක්ෂණ මට්ටම ඔබ විසින් වෙනස් කල හැකි නමුදු, එම ක්‍රියාව තීරු දර්ශන ආරක්ෂණය කෙරෙහි බලපෑම් ඇති නොකරනු ඇත.',
'protect-default'             => '(පෙරනිමි)',
'protect-fallback'            => '"$1" අවසරය අවශ්‍ය වේ',
'protect-level-autoconfirmed' => 'ලියාපදිංචි වී නොමැති පරිශීලකයන් වාරණය කරන්න',
'protect-level-sysop'         => 'පරිපාලකවරුන්ට පමණයි',
'protect-summary-cascade'     => 'තීරු දර්ශනය',
'protect-expiring'            => 'ඉකුත් වේ  $1 (යූටීසි)',
'protect-cascade'             => 'මෙම පිටුවෙහි ඇතුළත් කර ඇති පිටු ආරක්ෂණය කරන්න (තීරු දර්ශන ආරක්ෂණය)',
'protect-cantedit'            => 'ඔබ හට එය සංස්කරණය කිරීමට අවසර නොමැති බැවින්, ඔබ හට මෙම පිටුවෙහි ආරක්ෂණ මට්ටම වෙනස් කල නොහැක.',
'restriction-type'            => 'අවසරය:',
'restriction-level'           => 'පරිසීමා මට්ටම:',

# Undelete
'undeletebtn'            => 'ප්‍රතිෂ්ඨාපනය',
'undelete-search-submit' => 'සොයන්න',

# Namespace form on various pages
'namespace'      => 'නාමඅවකාශය:',
'invert'         => 'තෝරාගැනුම උඩු-යටිකුරු කරන්න',
'blanknamespace' => '(ප්‍රධාන)',

# Contributions
'contributions' => 'මේ පරිශීලකයාගේ දායකත්වය',
'mycontris'     => 'මගේ දායකත්ව',
'contribsub2'   => '$1 ($2) සඳහා',
'uctop'         => '(පෙරටු)',
'month'         => 'මෙම මස (හා ඉන් පෙර) සිට:',
'year'          => 'මෙම වසර (හා ඉන් පෙරාතුව) සිට:',

'sp-contributions-newbies'     => 'නව ගිණුම් වලට පමණක් අදාල දායකත්ව පෙන්වන්න',
'sp-contributions-newbies-sub' => 'නව ගිණුම් වලට අදාල',
'sp-contributions-blocklog'    => 'වාරණ ලඝු-සටහන',
'sp-contributions-search'      => 'දායකත්ව පිළිබඳ ගවේෂණය කරන්න',
'sp-contributions-username'    => 'පරිශීලක නාමය හෝ අන්තර්ජාල ලිපිනය:',
'sp-contributions-submit'      => 'ගවේෂණය කරන්න',

# What links here
'whatlinkshere'            => 'මෙයට සබැ‍ඳෙන පිටු',
'whatlinkshere-title'      => '"$1" වෙත සබැ‍ඳෙන පිටු',
'whatlinkshere-page'       => 'පිටුව:',
'linkshere'                => "ඉදිරියෙහි දැක්වෙන පිටු, '''[[:$1]]''' වෙත සබැඳෙයි:",
'nolinkshere'              => "'''[[:$1]]''' වෙත කිසිදු පිටුවක් සබැඳී නොමැත.",
'nolinkshere-ns'           => "තෝරාගෙන ඇති නාම-අවකාශය තුලදී, කිසිදු පිටුවක්, '''[[:$1]]''' වෙත නොබැඳෙයි.",
'isredirect'               => 'පිටුව යළි-යොමුකරන්න',
'istemplate'               => 'අන්තහ්කරණය',
'isimage'                  => 'ප්‍රතිමූර්ති-සබැඳිය',
'whatlinkshere-prev'       => '{{PLURAL:$1|පූර්ව|පූර්ව $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|ඉදිරි|ඉදිරි $1}}',
'whatlinkshere-links'      => '← සබැඳි',
'whatlinkshere-hideredirs' => '$1 යළි-යොමුකරයි',
'whatlinkshere-hidetrans'  => '$1 අන්තඃගතයන්',

# Block/unblock
'blockip'                         => 'පරිශීලකයා වාරණය කරන්න',
'ipaddress'                       => 'IP යොමුව:',
'ipboptions'                      => 'පැය 2:2 hours,දින 1:1 day,දින 3:3 days,සති 1:1 week,සති 2:2 weeks,මාස 1:1 month,මාස 3:3 months,මාස 6:6 months,වසර 1:1 year,අනන්තය:infinite', # display1:time1,display2:time2,...
'ipblocklist'                     => 'වාරණයට ලක්වූ අන්තර්ජාල ලිපිනයන් හා පරිශීලක නාම',
'blocklink'                       => 'වාරණය',
'unblocklink'                     => 'වාරණයෙන් ඉවත්වන්න',
'contribslink'                    => 'දායකත්ව',
'autoblocker'                     => 'ඔබගේ අන්තර්ජාල ලිපිනය "[[පරිශීලක:$1|$1]]" විසින් මෑතකදී භාවිතා කර ඇති බැවින් ඔබ ස්වයංක්‍රීය-වාරණයකට ලක් කර ඇත.
$1 ගේ වාරණයට හේතුව මෙය වේ: "$2"',
'blocklogpage'                    => 'වාරණ ලඝු සටහන',
'blocklogentry'                   => '$2 $3 වෙතින් දැක්වෙන ඉකුත් වීමේ කාලයකට යටත් කොට [[$1]] වාරණයට ලක් කර ඇත',
'blocklogtext'                    => 'පරිශීලකයන් වාරණය කිරීමේ හා වාරණයන් අත්හිටුවීමේ කාර්යයන් දැක්වෙන ලඝු සටහන මෙහි දැක්වේ.
ස්වයංක්‍රීයව වාරණය කල අන්තර්ජාල ලිපිනයන් ලැයිස්තුගත කොට නොමැත.
වර්තමානයෙහි ක්‍රියාත්මක වන තහනම් හා වාරණ සඳහා [[Special:IPBlockList|අන්තර්ජාල ලිපිනයන් වාරණ ලැයිස්තුව]] බලන්න.',
'unblocklogentry'                 => '$1 හි වාරණය අත්හිටුවන ලදි',
'block-log-flags-anononly'        => 'නිර්නාමික පරිශීලකයන් පමණි',
'block-log-flags-nocreate'        => 'ගිණුම් තැනීම අක්‍රීය කොට ඇත',
'block-log-flags-noautoblock'     => 'ස්වයංක්‍රීය වාරණය අක්‍රීය කොට ඇත',
'block-log-flags-noemail'         => 'වි-තැපෑල වාරණය කොට ඇත',
'block-log-flags-angry-autoblock' => 'ප්‍රබලකල (ඉවැඩි) ස්වයංක්‍රීය වාරණය සක්‍රීය කරන ලදි',
'range_block_disabled'            => 'පරාස වාරණයන් සිදුකිරීමට පරිපාලක වරුන්ට ඇති හැකියාව අක්‍රීය කරන ලදි.',
'ipb_expiry_invalid'              => 'ඉකුත්වීමේ කාලය අවලංගුය.',
'ipb_expiry_temp'                 => 'සැඟවුනු පරිශීලක-නාම වාරණයන් ස්ථීර ඒවා විය යුතුය.',
'ipb_already_blocked'             => '"$1" දැනටමත් වාරණයට ලක් කර ඇත',
'ipb_cant_unblock'                => 'දෝෂය: වාරණ අනන්‍යතාවය $1 සොයා ගත නොහැකි විය.
මෙය දැනටමත් වාරණ අත්හිටුවීමකට භාජනය වී ඇතිවා විය හැක.',
'ipb_blocked_as_range'            => 'දෝෂය: $1 අන්තර්ජාල ලිපිනය සෘජුව වාරණය කොට නොමැති අතර එහි වාරණ‍ය අත්හිටුවිය නොහැක.
එනමුදු, එය, $2 පරාසයෙහි කොටසක් ලෙස වාරණයට ලක් කොට ඇති අතර, එහි වාරණය අත්හිටුවිය හැක.',
'ip_range_invalid'                => 'අවලංගු අන්තර්ජාල ලිපින පරාසයකි.',
'blockme'                         => 'මා වාරණය කරන්න',
'proxyblocker'                    => 'ප්‍රතියුක්ත (ප්‍රොක්සි) වාරණකරු',
'proxyblocker-disabled'           => 'මෙම කෘත්‍යය අක්‍රීය කොට ඇත.',
'proxyblockreason'                => 'ඔබගේ අන්තර්ජාල ලිපිනය විවෘත ප්‍රතියුක්තයක් (ප්‍රොක්සි) බැවින් එය වාරණය කොට ඇත.
ඔබගේ අන්තර්ජාල සේවා ප්‍රතිපාදකයා හෝ තාක්ෂණික අනුග්‍රාහකයා හෝ අමතා මෙම බරපතළ ආරක්ෂණ ගැටළුව ඔවුනට නිරාවරණය කරන්න.',
'proxyblocksuccess'               => 'සිදුකලා.',

# Move page
'movepagetext'     => "පහත ආකෘතිය භාවිතා කිරීමෙන්, එහි සියළු ඉතිහාසය නව නාමයට අනුයුක්ත කරමින්,  පිටුවක නම-වෙනස් කිරීම සිදුවේ.
නව නාමය වෙත යළි-යොමු වන්නාවූ පිටුවක් බවට පැරැණි නාමය පත් වෙයි.
ආදිමය නාමය වෙත ස්වයංක්‍රීයව එල්ල වන යළි-යොමු වීම් යාවත්කාලීන කිරීම් ඔබ විසින් සිදු කල හැක.
එසේ සිදු කිරීමට ඔබ නොරිසි නම්, [[Special:DoubleRedirects|ද්විත්ව]] හෝ [[Special:BrokenRedirects|භින්න යළි-යොමු වීම්]] පරික්ෂා කර බැලීමට යුහුසුළු වන්න.
නියමිත යොමු කරා සබැඳියන්  දිගටම එල්ල වන බව සහතික කිරීම ඔබගේ වගකීමකි.

නව නාමය සහිත පිටුවක් දැනටමත් තිබේ නම්, එය හිස් නම් හෝ යළි-යොමුවක් හා එහි පූර්ව සංස්කරණ ඉතිහාසයක් නොමැති නම් මිස, පිටුව ගෙනයෑම සිදු ''නොකරන''' බව සලකන්න.
මෙහි අරුත වන්නේ, ඔබ විසින් අත්වැරැද්දක් සිදුවුනි නම්, නම වෙනස් කල යම් පිටුවක නම ‍වෙනස් කිරීමට පැවැති පිටුවට ආපසු නම වෙනස් කල හැකි බවත්, එනමුදු දැනට පවතින පිටුවක් උඩින්-ලිවීම සිදු කල නොහැකි බවත්ය.

'''අවවාදයයි!'''
මෙම වෙනස ජනප්‍රිය පිටුවකට විෂයෙහි සිදුවන උග්‍ර හා අනපේක්‍ෂිත වෙනස්කමක් විය හැක;
බිඳක් නැවැතී  මෙහි ප්‍රතිවිපාක පිළිබඳ පරිලෝකනය කිරීමට යුහුසුළු වන්න.",
'movepagetalktext' => "එය සමග ආශ්‍රිත සාකච්ඡා පිටුව ස්වයංක්‍රීය ලෙස ගෙනයාම වළක්වන '''වැළැහීම්:'''
*නව පිටු නාමය යටතේ, හිස්-නොවන සාකච්ඡා පිටුවක් දැනටමත් පැවැතීම, හෝ
*පහත කොටුව ඔබ විසින් නොතේරූ නිසාවෙන්.

මෙවන් අවස්ථා වලදී, අවශ්‍යතාවය පැන නගී නම්, හස්තීය ලෙස ගෙන යාම හෝ සංයුක්ත කිරීම හෝ සිදු කිරීමට ඔබ හට සිදුවේ.",
'movearticle'      => 'පිටුව ගෙනයන්න:',
'newtitle'         => 'නව පිටු නාමය වෙත:',
'move-watch'       => 'මෙම පිටුව මුර කරන්න',
'movepagebtn'      => 'පිටුව ගෙන යන්න',
'pagemovedsub'     => 'ගෙන යාම සාර්ථකයි',
'movepage-moved'   => '<big>\'\'\'"$1" යන පිටුව  "$2"\'\'\' වෙත ගෙන යන ලදි</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'    => 'එක්කෝ මෙම නම ඇති පිටුවක් දැනටමත් පවතී, නැත්නම් ඔබ විසින් තෝරා ගෙන ඇති පිටුව වලංගු එකක් නොවේ.
වෙන යම් නමක් තෝරාගන්න.',
'talkexists'       => "'''මෙම පිටුව සාර්ථක ලෙස ගෙන ගිය නමුදු, සාකච්ඡා පිටුව එසේ ගෙන යාම කල නොහැකි වූයේ නව පිටු නාමයට අදාලව සාකච්ඡා පිටුවක් දැනටමත් පවතින බැවිනි.
කරුණාකර ඒවා හස්තීය ලෙස සංයුක්ත කරන්න.'''",
'movedto'          => 'වෙත ගෙන යන ලදි',
'movetalk'         => 'ආශ්‍රිත සාකච්ඡා පිටුව ගෙන යන්න',
'1movedto2'        => '[[$1]] යන්න [[$2]] වෙත ගෙන යන ලදි',
'movelogpage'      => 'ගෙනයෑම් ලඝු-සටහන',
'movereason'       => 'හේතුව:',
'revertmove'       => 'ප්‍රතිවර්තනය',

# Export
'export' => 'පිටු නිර්යාත කරන්න',

# Namespace 8 related
'allmessages'     => 'පද්ධති පණිවුඩ',
'allmessagesname' => 'නම',

# Thumbnails
'thumbnail-more'  => 'විශාලනය කිරීම',
'thumbnail_error' => 'සංක්‍ෂිප්තයක් තැනීමෙහිදී ඇතිවූ දෝෂය: $1',

# Import log
'importlogpage' => 'ලඝු-සටහන් ආයාත කරන්න',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'මාගේ පරිශීලක පිටුව',
'tooltip-pt-mytalk'               => 'මාගේ සාකච්ඡා පිටුව',
'tooltip-pt-preferences'          => 'මගේ අභිරුචි',
'tooltip-pt-watchlist'            => 'වෙනස්කම් සිදුවී තිබේදැයි යන්න පිලිබඳව ඔබගේ විමසුමට ලක්ව ඇති පිටු ලැයිස්තුව',
'tooltip-pt-mycontris'            => 'මාගේ දායකත්ව ලැයිස්තුව‍',
'tooltip-pt-login'                => 'එය අවශ්‍ය‍යෙන් කල යුත්තක් ‍නොවුනද, ප්‍රවිෂ්ට වීම සඳහා ඔබ ධෛර්යමත් කරනු ලැබේ.',
'tooltip-pt-anonlogin'            => 'එය අවශ්‍ය‍යෙන් කල යුත්තක් ‍නොවුනද, ප්‍රවිෂ්ට වීම සඳහා ඔබ ධෛර්යමත් කරනු ලැබේ.',
'tooltip-pt-logout'               => 'නිෂ්ක්‍රමණය',
'tooltip-ca-talk'                 => 'අන්තර්ගත පිටුව පිළිබඳ සංවාදය',
'tooltip-ca-edit'                 => 'ඔබ‍ට මෙම පිටුව සංස්කරණය කල හැක.
 සුරැකීමට පෙර කරුණාකර පෙරදසුන බොත්තම භාවිතා කරන්න.',
'tooltip-ca-addsection'           => 'මෙම සංවාදයට පරිකථනයක් ඇතුලත් කරන්න.',
'tooltip-ca-viewsource'           => 'මෙම පිටුව ආරක්ෂණය කොට ඇත.
ඔබට එහි මූලාශ්‍රය නැරඹිය හැක.',
'tooltip-ca-history'              => 'මෙම පිටුවේ පැරණි අනුවාදයන්.',
'tooltip-ca-protect'              => 'මෙම පිටුව ආරක්‍ෂණය කරන්න',
'tooltip-ca-delete'               => 'මේ පිටුව මකන්න',
'tooltip-ca-undelete'             => 'මෙම පිටුව මකා දැමීමට පෙර එයට සිදුකල සංස්කරණයන් නැවත ප්‍රතිෂ්ඨාපනය කරන්න',
'tooltip-ca-move'                 => 'මෙම පිටුව ගෙන යන්න',
'tooltip-ca-watch'                => 'මෙම පිටුව ඔබගේ මුර-ලැයිස්තුවට එක් කරන්න',
'tooltip-ca-unwatch'              => 'මෙම පිටුව ඔබගේ මුර-ලැයිස්තුවෙන් ඉවත් කරන්න',
'tooltip-search'                  => '{{SITENAME}} ගවේෂණය',
'tooltip-search-go'               => 'මෙම නාමයට තථ්‍ය ලෙස ගැලපෙන පිටුවක් තිබේ නම් එය වෙත යන්න',
'tooltip-search-fulltext'         => 'මෙම පෙළ අඩංගු පිටු ගවේෂණය කරන්න',
'tooltip-p-logo'                  => 'මුල් පිටුව',
'tooltip-n-mainpage'              => 'මුල් පිටුව‍ට යන්න',
'tooltip-n-portal'                => 'ව්‍යාපෘතියට අදාළව, ඔබට සිදුකල හැකි දෑ, අවශ්‍ය දෑ සොයා ගත හැකි අයුරු, යනාදී වැදගත් තොරතුරු',
'tooltip-n-currentevents'         => 'කාලීන සිදුවීම් පිළිබඳ පසුබිම් තොරතුරු සොයා දැනගන්න',
'tooltip-n-recentchanges'         => 'විකියෙහි මෑත වෙනස්කම් දැක්වෙන ලැයිස්තුවක්.',
'tooltip-n-randompage'            => 'අහුඹු පිටුවක් ප්‍රවේශනය කරන්න (බා ගන්න)',
'tooltip-n-help'                  => 'තොරතුරු නිරාවරණය කර ගත හැකි තැන.',
'tooltip-t-whatlinkshere'         => 'මෙය හා සබැ‍ඳෙන සියළු විකි පිටු ලැයිස්තුව',
'tooltip-t-recentchangeslinked'   => 'මෙම පිටුව හා සබැඳි පිටුවල මෑත වෙනස්කම්',
'tooltip-t-contributions'         => 'මෙම පරිශීලකයාගේ දායකත්ව ලැයිස්තුව නරඹන්න',
'tooltip-t-emailuser'             => 'මෙම පරිශීලකයාට විද්‍යුත්-තැපෑලක් යවන්න',
'tooltip-t-upload'                => 'ගොනු උඩුගත කරන්න',
'tooltip-t-specialpages'          => 'සියලු විශේෂ පිටු ලැයිස්තුව',
'tooltip-t-print'                 => 'මෙම පිටුවෙහි මුද්‍රණය කල හැකි අනුවාදය',
'tooltip-t-permalink'             => 'පිටුවෙහි මෙම අනුවාදයට, ස්ථාවර බැඳිය',
'tooltip-ca-nstab-main'           => 'අන්තර්ගත පිටුව නරඹන්න',
'tooltip-ca-nstab-user'           => 'පරිශීලක පිටුව නරඹන්න',
'tooltip-ca-nstab-media'          => 'මාධ්‍ය පිටුව නරඹන්න',
'tooltip-ca-nstab-special'        => 'මෙය විශේෂ පිටුවකි, එයම සංස්කරණය කිරීමට ඔබට නොහැක',
'tooltip-ca-nstab-project'        => 'ව්‍යාපෘති පිටුව නරඹන්න',
'tooltip-ca-nstab-image'          => 'ගොනු පිටුව නරඹන්න',
'tooltip-ca-nstab-mediawiki'      => 'පද්ධති පණිවුඩය නරඹන්න',
'tooltip-ca-nstab-template'       => 'සැකිල්ල නරඹන්න',
'tooltip-ca-nstab-help'           => 'උදවු පිටුව නරඹන්න',
'tooltip-ca-nstab-category'       => 'ප්‍රවර්ග පිටුව නරඹන්න',
'tooltip-minoredit'               => 'මෙය සුළු සංස්කරණයක් ලෙස සනිටුහන් කරගන්න',
'tooltip-save'                    => 'ඔබ විසින් කල  වෙනස් කිරීම් සුරකින්න',
'tooltip-preview'                 => 'ඔබ විසින් කල  වෙනස් කිරීම් පෙර-දසුන් කර, ඉන් අනතුරුව සුරැකීම සිදුකිරීමට කාරුණික වන්න!',
'tooltip-diff'                    => 'පෙළෙහි ඔබ සිදුකල වෙනස්කම් මොනවාදැයි හුවා දක්වන්න.',
'tooltip-compareselectedversions' => 'මෙම පිටුවෙහි, තෝරාගෙන ඇති අනුවාද දෙක අතර වෙනස්කම් බලන්න.',
'tooltip-watch'                   => 'මෙම පිටුව ඔබගේ මුර-ලැයිස්තුවට එක් කරන්න',
'tooltip-recreate'                => 'පිටුව මකාදමා ඇති වුවද, එය යළි-නිර්මාණය කරන්න',
'tooltip-upload'                  => 'උඩුගත කිරීම අරඹන්න',

# Browsing diffs
'previousdiff' => '← පැරැණි සංස්කරණය',
'nextdiff'     => 'නවීන සංස්කරණය →',

# Media information
'file-info-size'       => '($1 × $2 පික්සල, ගොනු විශාලත්වය: $3, MIME ශෛලිය: $4)',
'file-nohires'         => '<small>උච්චතර විසර්ජනය දක්වා එළඹිය නොහැක.</small>',
'svg-long-desc'        => '(SVG ගොනුව, නාමමාත්‍රිකව $1 × $2 පික්සල්, ගොනු විශාලත්වය: $3)',
'show-big-image'       => 'සම්පූර්ණ විසර්ජනය',
'show-big-image-thumb' => '<small>පෙර නැරඹුමෙහි  විශාලත්වය: $1 × $2 පික්සල</small>',

# Special:NewImages
'newimages' => 'නව ගොනු ගැලරිය',
'ilsubmit'  => 'සොයන්න',

# Bad image list
'bad_image_list' => 'ආකාතිය පහත පෙන්වා ඇති පරිදි වේ:

ලැයිස්තු අයිතම පමණක් (* යන්නෙන් ආරම්භ වන්නාවූ පේළි) සළකා බලනු ලැබේ.
පේළියක පළමු සබැඳිය සදොස් ගොනුවකට යොමු වන සබැඳියක් විය යුතුය.
එම පේළියෙහිම ඉනික්බිති හමුවන ඕනෑම සබැඳියක් සලකනු ලබන්නේ ව්‍යහිවාරයක් ලෙසටය, එනම්, ගොනු එක පේළියට පැවතිය හැකි පිටු.',

# Metadata
'metadata'          => 'පාරදත්ත',
'metadata-help'     => 'සමහරවිට ඩිජිටල් කැමරාවක් හෝ ස්කෑනරයක් හෝ භාවිතයෙන්, නිමැවා හෝ සංඛ්‍යාංකකරණය (ඩිජිටල්කරණය) කොට එක් කල , අමතර තොරතුරු මෙම ගොනුවේ අඩංගුය.
ගොනුව මුලින්ම පැවැති තත්ත්වයෙහි සිට විකරණය කොට තිබේ නම්, සමහරක් තොරතුරු විකරිත ගොනුව පූර්ණ වශයෙන් පිළිඹිමු නොකරනු ඇත.',
'metadata-expand'   => 'විස්තීරණය කරන ලද විස්තර පෙන්වන්න',
'metadata-collapse' => 'විස්තීරණය කරන ලද විස්තර සඟවන්න',
'metadata-fields'   => 'පාරදත්ත වගුව බිඳවැටෙන විට, මෙම පණිවුඩයෙහි ලැයිස්තු ගත කොට ඇති  EXIF පාරදත්ත ක්ෂේත්‍රයන් රූප පිටු ප්‍රදර්ශනයෙහි ඇතුළත් කෙරෙයි.
අනෙක්වා ‍‍ පෙර නිමි අයුරින් සඟවනු ලැබේ.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'   => 'පළල',
'exif-imagelength'  => 'උස',
'exif-artist'       => 'කතෘ',
'exif-gpslatitude'  => 'අක්ෂාංශය',
'exif-gpslongitude' => 'දේශාංශය',

'exif-subjectdistance-value' => 'මීටර $1',

'exif-focalplaneresolutionunit-2' => 'අඟල්',

# External editor support
'edit-externally'      => 'බාහිර  උපයෝගයක් භාවිතා කරමින් මෙම ගොනුව සංස්කරණය කරන්න',
'edit-externally-help' => 'වැඩිමනත් තොරතුරු සඳහා [http://www.mediawiki.org/wiki/Manual:External_editors පිහිටුවීම් උපදෙස්] බලන්න.',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'සියල්ල',
'namespacesall' => 'සියල්ල',
'monthsall'     => 'සියළු',

# action=purge
'confirm_purge_button' => 'හරි',

# AJAX search
'useajaxsearch' => 'AJAX සෙවුම භාවිත කරන්න',

# Multipage image navigation
'imgmultipageprev' => '← පෙර පිටුව',
'imgmultipagenext' => 'ඊළඟ පිටුව →',
'imgmultigo'       => 'යන්න!',

# Table pager
'table_pager_next'         => 'ඊළඟ පිටුව',
'table_pager_prev'         => 'පෙර පිටුව',
'table_pager_first'        => 'පළමු පිටුව',
'table_pager_last'         => 'අවසාන පිටුව',
'table_pager_limit_submit' => 'යන්න',

# Watchlist editing tools
'watchlisttools-view' => 'අදාල වෙනස්කම් නරඹන්න',
'watchlisttools-edit' => 'මුර-ලැයිස්තුව නැරඹීම හා සංස්කරණය සිදු කරන්න',
'watchlisttools-raw'  => 'නොනිමි මුර-ලැයිස්තුව සංස්කරණය කරන්න',

# Special:Version
'version'              => 'අනුවාදය', # Not used as normal message but as header for the special page itself
'version-specialpages' => 'විශේෂ පිටු',
'version-other'        => 'වෙනත්',

# Special:SpecialPages
'specialpages'             => 'විශේෂ පිටු',
'specialpages-group-pages' => 'පිටු ලැයිස්තුව',

);
