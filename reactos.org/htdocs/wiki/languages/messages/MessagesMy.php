<?php
/** Burmese (Myanmasa)
 *
 * @ingroup Language
 * @file
 *
 * @author Hakka
 * @author Hintha
 */

$digitTransformTable = array(
	'0' => '၀',
	'1' => '၁',
	'2' => '၂',
	'3' => '၃',
	'4' => '၄',
	'5' => '၅',
	'6' => '၆',
	'7' => '၇',
	'8' => '၈',
	'9' => '၉',
);

$datePreferences = array(
	'default',
	'my normal',
	'my long',
	'ISO 8601',
);
 
$defaultDateFormat = 'my normal';
 
$dateFormats = array(
	'my normal time' => 'H:i',
	'my normal date' => 'j F Y',
	'my normal both' => ' H:i"၊" j F Y',
 
	'my long time' => 'H:i',
	'my long date' => 'Y "ဇန်နဝါရီ" F"လ" j "ရက်"',
	'my long both' => 'H:i"၊" Y "ဇန်နဝါရီ" F"လ" j "ရက်"',
);

$messages = array(
# User preference toggles
'tog-watchcreations' => 'ကျွန်တော်ထွင်သည့်စာမျက်နှာများကို စောင့်​ကြည့်​စာ​ရင်း​ထဲ ပေါင်းထည့်ပါ',

'underline-always' => 'အမြဲ',

'skinpreview' => '(နမူနာ)',

# Dates
'sunday'        => 'တ​နင်္ဂ​နွေ​',
'monday'        => 'တ​နင်္လာ​',
'tuesday'       => 'အင်္ဂါ​',
'wednesday'     => 'ဗုဒ္ဒ​ဟူး​',
'thursday'      => 'ကြာ​သာ​ပ​တေး​',
'friday'        => 'သော​ကြာ​',
'saturday'      => 'စ​နေ​',
'sun'           => 'တနင်္ဂနွေ',
'mon'           => 'တနင်္ဂလာ',
'tue'           => 'အင်္ဂါ',
'wed'           => 'ဗုဒ္ဓဟူး',
'thu'           => 'ကြာသပတေး',
'fri'           => 'သောကြာ',
'sat'           => 'စနေ',
'january'       => 'ဇန်​န​ဝါ​ရီ​',
'february'      => 'ဖေ​ဖော်​ဝါ​ရီ​',
'march'         => 'မတ်​',
'april'         => 'ဧ​ပြီ​',
'may_long'      => 'မေ​',
'june'          => 'ဇွန်​',
'july'          => 'ဇူ​လိုင်​',
'august'        => 'ဩ​ဂုတ်​',
'september'     => 'စက်​တင်​ဘာ​',
'october'       => 'အောက်​တို​ဘာ​',
'november'      => 'နို​ဝင်​ဘာ​',
'december'      => 'ဒီ​ဇင်​ဘာ​',
'january-gen'   => 'ဇန်​န​ဝါ​ရီ​',
'february-gen'  => 'ဖေ​ဖော်​ဝါ​ရီ​',
'march-gen'     => 'မတ်​',
'april-gen'     => 'ဧ​ပြီ​',
'may-gen'       => 'မေ​',
'june-gen'      => 'ဇွန်​',
'july-gen'      => 'ဇူ​လိုင်​',
'august-gen'    => 'ဩ​ဂုတ်​',
'september-gen' => 'စက်​တင်​ဘာ​',
'october-gen'   => 'အောက်​တို​ဘာ​',
'november-gen'  => 'နို​ဝင်​ဘာ​',
'december-gen'  => 'ဒီ​ဇင်​ဘာ​',
'may'           => 'မေ​',

'about'          => 'အကြောင်း',
'cancel'         => 'မ​လုပ်​တော့​ပါ​',
'qbfind'         => 'ရှာပါ',
'qbedit'         => 'ပြင်​ဆင်​ရန်​',
'qbspecialpages' => 'အ​ထူး​စာ​မျက်​နှာ​',
'mytalk'         => 'ကျွန်​တော့​ပြော​ရေး​ဆို​ရာ​',
'navigation'     => 'အ​ညွှန်း​',

'help'             => 'အ​ကူ​အ​ညီ​',
'search'           => 'ရှာ​ဖွေ​ရန်​',
'searchbutton'     => 'ရှာ​ဖွေ​ရန်​',
'go'               => 'သွား​ပါ​',
'searcharticle'    => 'သွား​ပါ​',
'history_short'    => 'မှတ်​တမ်း​',
'printableversion' => 'ပ​ရင်​တာ​ထုတ်​ရန်​',
'permalink'        => 'ပုံ​သေ​လိပ်​စာ​',
'print'            => 'ပရင့်',
'edit'             => 'ပြင်​ဆင်​ရန်​',
'delete'           => 'ဖျက်​ပါ​',
'protect'          => 'ထိမ်း​သိမ်း​ပါ​',
'newpage'          => 'စာမျက်နှာအသစ်',
'talk'             => 'ပြော​ရေး​ဆို​ရာ​',
'toolbox'          => 'တန်​ဆာ​ပ​လာ​',
'otherlanguages'   => 'အ​ခြား​ဘာ​သာ​ဖြင့်​',
'jumptonavigation' => 'အ​ညွှန်း​',
'jumptosearch'     => 'ရှာ​ဖွေ​ရန်​',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}}အကြောင်း',
'currentevents'        => 'လက်​ရှိ​လုပ်​ငန်း​များ​',
'currentevents-url'    => 'Project:လက်​ရှိ​လုပ်​ငန်း​များ​',
'disclaimers'          => 'သ​တိ​ပေး​ချက်​များ​',
'edithelp'             => 'ပြင်​ဆင်​ခြင်း​အ​ကူ​အ​ညီ​',
'mainpage'             => 'ဗ​ဟို​စာ​မျက်​နှာ​',
'mainpage-description' => 'ဗ​ဟို​စာ​မျက်​နှာ​',
'portal'               => 'ပြော​ရေး​ဆို​ရာ​',

'newmessageslink'         => 'သ​တင်း​အ​သစ်​',
'youhavenewmessagesmulti' => 'သင့်​အ​တွက်​သီ​တင်း​အ​သစ်​ $1 တွင်​ရှိ​သည်​',
'editsection'             => 'ပြင်​ဆင်​ရန်​',
'editold'                 => 'ပြင်​ဆင်​ရန်​',
'showtoc'                 => 'ပြ',
'hidetoc'                 => 'ဝှက်',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'  => 'စာ​မျက်​နှာ​',
'nstab-user'  => 'မှတ်​ပုံ​တင်​အ​သုံး​ပြု​သူ​၏​စာ​မျက်​နှာ​',
'nstab-image' => 'ဖိုင်',

# General errors
'viewsource' => 'ဆို့​ကို​ပြ​ပါ​',

# Login and logout pages
'welcomecreation'         => 'မင်္ဂ​လာ​ပါ​ $1။ သင့်​အား​မှတ်​ပုံ​တင်​ပြီး​ပါ​ပြီ။​ ဝီ​ကီ​အ​တွက်​သင့်​စိတ်​ကြိုက်​များ​ကို​ရွေး​ချယ်​နိုင်​ပါ​သည်။​',
'yourname'                => 'မှတ်​ပုံ​တင်​အ​မည်:',
'yourpassword'            => 'လှို့​ဝှက်​စ​ကား​လုံး:',
'yourpasswordagain'       => 'ပြန်​ရိုက်​ပါ:',
'remembermypassword'      => 'ဤ​ကွန်​ပျူ​တာ​တွင်​ကျွန်​တော့​ကို​မှတ်​ထား​ပါ​',
'login'                   => 'မှတ်​ပုံ​တင်​ဖြင့်​ဝင်​ပါ​',
'nav-login-createaccount' => 'မှတ်​ပုံ​တင်​ဖြင့်​ဝင်​ပါ​ / မှတ်​ပုံ​တင်​ပြု​လုပ်​ပါ​',
'userlogin'               => 'မှတ်​ပုံ​တင်​ဖြင့်​ဝင်​ပါ​ / မှတ်​ပုံ​တင်​ပြု​လုပ်​ပါ​',
'logout'                  => 'ထွက်​ပါ​',
'userlogout'              => 'ထွက်​ပါ​',
'notloggedin'             => 'မှတ်​ပုံ​တင်​ဖြင့်​မ​ဝင်​ရ​သေး​ပါ​',
'nologinlink'             => 'မှတ်​ပုံ​တင်​ပြု​လုပ်​ပါ​',
'createaccount'           => 'မှတ်​ပုံ​တင်​ပြု​လုပ်​ပါ​',
'gotaccountlink'          => 'မှတ်​ပုံ​တင်​ဖြင့်​ဝင်​ပါ​',
'youremail'               => 'အီ​မေး:',
'username'                => 'မှတ်​ပုံ​တင်​အ​မည်:',
'uid'                     => 'မှတ်​ပုံ​တင်​ID:',
'yourrealname'            => 'နာမည်ရင်း:',
'yourlanguage'            => 'ဘာသာ:',
'yournick'                => 'ဆိုင်း:',
'email'                   => 'အီ​မေး​',
'loginsuccesstitle'       => 'မှတ်​ပုံ​တင်​ဖြင့်​ဝင်​ခြင်းအောင်မြင်သည်။',
'loginlanguagelabel'      => 'ဘာသာ: $1',

# Edit page toolbar
'italic_sample' => 'စာသားဆောင်း',
'italic_tip'    => 'စာသားဆောင်း',
'math_sample'   => 'သည်မှာသင်္ချာပုံသေနည်းအားထည့်',
'math_tip'      => 'သင်္ချာပုံသေနည်း (LaTeX)',
'hr_tip'        => 'မျဉ်းလဲ',

# Edit pages
'summary'            => 'အ​ကျဉ်း​ချုပ်​',
'minoredit'          => 'သာ​မန်​ပြင်​ဆင်​မှု​ဖြစ်​ပါ​သည်​',
'watchthis'          => 'ဤ​စာ​မျက်​နှာ​အား​စောင့်​ကြည့်​ပါ​',
'savearticle'        => 'သိမ်း​ပါ​',
'preview'            => 'နမူနာ',
'showpreview'        => 'န​မူ​နာ​ပြ​ပါ​',
'showlivepreview'    => 'နမူနာအရှင်',
'showdiff'           => 'ပြင်​ဆင်​ထား​သည်​များ​ကို​ပြ​ပါ​',
'summary-preview'    => 'အ​ကျဉ်း​ချုပ်​န​မူ​နာ',
'whitelistedittitle' => 'ပြင်​ဆင်​ခြင်း​သည်​မှတ်​ပုံ​တင်​ရန်​လို​သည်​',
'loginreqtitle'      => 'မှတ်​ပုံ​တင်​ဖြင့်​ဝင်​ဖို့လိုပါတယ်',
'loginreqlink'       => 'မှတ်​ပုံ​တင်​ဖြင့်​ဝင်​ပါ​',
'accmailtitle'       => 'ဝှက်​စ​ကား​လုံးကိုပို့ပြီးပြီ',
'newarticle'         => '(အသစ်)',

# History pages
'page_first' => 'ပထမဆုံး',
'page_last'  => 'အနောက်ဆုံး',

# Revision deletion
'rev-delundel' => 'ပြ/ဝှက်',

# Search results
'powersearch' => 'ရှာ​ဖွေ​ရန်​',

# Preferences page
'preferences'       => 'ကျွန်​တော့​ရွေး​ချယ်​စ​ရာ​များ​',
'mypreferences'     => 'ကျွန်​တော့​ရွေး​ချယ်​စ​ရာ​များ​',
'prefsnologin'      => 'မှတ်​ပုံ​တင်​ဖြင့်​မ​ဝင်​ရ​သေး​ပါ​',
'changepassword'    => 'ဝှက်​စ​ကား​လုံးကိုပြောင်းပါ',
'math'              => 'သင်္ချာ',
'datetime'          => 'နေ့စွဲနှင့် အချိန်',
'oldpassword'       => 'ဝှက်​စ​ကား​လုံးအဟောင်း:',
'newpassword'       => 'ဝှက်​စ​ကား​လုံးအသစ်:',
'retypenew'         => 'ဝှက်​စ​ကား​လုံးပအသစ်ကိုထပ်ရိုက်ပါ:',
'searchresultshead' => 'ရှာ​ဖွေ​ရန်​',

# Groups
'group-all' => '(အားလုံး)',

# Recent changes
'recentchanges'   => 'လတ်​တ​လော​အ​ပြောင်း​အ​လဲ​များ​',
'rcshowhideminor' => 'အသေးအမွှာပြင်ဆင်ရန်$1',
'rcshowhidebots'  => 'ဆက်ရုပ်များ$1',
'hide'            => 'ဝှက်',
'show'            => 'ပြ',
'newpageletter'   => 'သစ်',
'boteditletter'   => 'ဆ',

# Recent changes linked
'recentchangeslinked' => 'ဆက်​ဆပ်​သော​အ​ပြောင်း​အ​လဲ​များ​',

# Upload
'upload'            => 'ဖိုင်​တင်​ရန်​',
'uploadbtn'         => 'ဖိုင်​တင်​ရန်​',
'uploadnologin'     => 'မှတ်​ပုံ​တင်​ဖြင့်​မ​ဝင်​ရ​သေး​ပါ​',
'filename'          => 'ဖိုင်အမည်',
'filedesc'          => 'အ​ကျဉ်း​ချုပ်​',
'fileuploadsummary' => 'အ​ကျဉ်း​ချုပ်:',
'watchthisupload'   => 'ဤ​စာ​မျက်​နှာ​အား​စောင့်​ကြည့်​ပါ​',

# Special:ImageList
'imgfile'        => 'ဖိုင်',
'imagelist'      => 'ဖိုင်စာရင်း',
'imagelist_date' => 'နေ့စွဲ',

# Image description page
'filehist'           => 'ဖိုင်မှတ်တမ်း',
'filehist-deleteall' => 'အားလုံးဖျက်',
'filehist-deleteone' => 'ဖျက်',
'filehist-current'   => 'ကာလပေါ်',
'filehist-datetime'  => 'နေ့စွဲ/အချိန်',
'filehist-filesize'  => 'ဖိုင်ဆိုက်',

# File deletion
'filedelete'        => '$1 ကိုဖျက်ပါ',
'filedelete-legend' => 'ဖိုင်ကိုဖျက်ပါ',
'filedelete-submit' => 'ဖျက်',

# Unused templates
'unusedtemplateswlh' => 'အခြားလိပ်စာများ',

# Random page
'randompage' => 'ကျ​ပန်း​စာ​မျက်​နှာ​',

# Statistics
'statistics' => 'စာရင်းအင်း',
'sitestats'  => '{{SITENAME}} စာရင်းအင်းများ',

'brokenredirects-edit'   => '(ပြင်​ဆင်​ရန်)',
'brokenredirects-delete' => '(ဖျက်​ပါ)',

'withoutinterwiki-submit' => 'ပြ',

# Miscellaneous special pages
'shortpages'        => 'စာမျက်နှာတို',
'newpages'          => 'စာမျက်နှာအသစ်',
'newpages-username' => 'မှတ်​ပုံ​တင်​အ​မည်:',
'ancientpages'      => 'အဟောင်းဆုံးစာမျက်နှာ',
'move'              => 'ရွေ့​ပြောင်း​ပါ​',
'movethispage'      => 'ဤစာမျက်နှာအားရွှေ့ပြောင်းပါ',

# Book sources
'booksources-go' => 'သွား​ပါ​',

# Special:Log
'log-search-submit' => 'သွား​ပါ​',

# Special:AllPages
'allpages'       => 'စာမျက်နှာအားလုံး',
'allarticles'    => 'စာမျက်နှာအားလုံး',
'allpagessubmit' => 'သွား​ပါ​',

# Special:ListUsers
'listusers-submit' => 'ပြ',

# E-mail user
'emailsend' => 'ပို့',

# Watchlist
'watchlist'     => 'စောင့်​ကြည့်​စာ​ရင်း​',
'mywatchlist'   => 'စောင့်​ကြည့်​စာ​ရင်း​',
'watchlistfor'  => "('''$1'''အတွက်)",
'watch'         => 'စောင့်​ကြည့်​ပါ​',
'watchthispage' => 'ဤ​စာ​မျက်​နှာ​အား​စောင့်​ကြည့်​ပါ​',

# Delete/protect/revert
'deletepage'       => 'စာမျက်နှာကိုဖျက်ပါ',
'confirm'          => 'အတည်ပြု',
'delete-confirm'   => '"$1"ကို ဖျက်ပါ',
'delete-legend'    => 'ဖျက်',
'restriction-type' => 'အခွင့်:',

# Restrictions (nouns)
'restriction-edit'   => 'ပြင်​ဆင်​ရန်​',
'restriction-move'   => 'ရွေ့​ပြောင်း​ပါ​',
'restriction-create' => 'ထွင်',

# Undelete
'undelete-search-submit' => 'ရှာ​ဖွေ​ရန်​',

# Contributions
'contributions' => 'မှတ်​ပုံ​တင်​အ​သုံး​ပြု​သူ​:ပံ​ပိုး​မှု​များ​',
'mycontris'     => 'ကျွန်​တော်​ပေး​သော​ပံ​ပိုး​မှု​များ​',
'contribsub2'   => '$1အတွက် ($2)',
'uctop'         => '(အထိပ်)',

'sp-contributions-submit' => 'ရှာ​ဖွေ​ရန်​',

# What links here
'whatlinkshere' => 'မည်​သည့်​စာ​မျက်​နှာ​များ​မှ​ညွန်း​ထား​သည်​',

# Block/unblock
'ipbreason'          => 'အ​ကြောင်း​ပြ​ချက်:',
'ipbother'           => 'အခြားအချိန်:',
'ipboptions'         => '၂ နာရီ:2 hours,၁ နေ့:1 day,၃ နေ့:3 days,၁ ပတ်:1 week,၂ ပတ်:2 weeks,၁ လ:1 month,၃ လ:3 months,၆ လ:6 months,၁ နှစ်:1 year,အနန္တ:infinite', # display1:time1,display2:time2,...
'ipbotheroption'     => 'အခြား',
'ipblocklist-submit' => 'ရှာ​ဖွေ​ရန်​',
'expiringblock'      => '$1 ဆုံးမည်',

# Move page
'move-page-legend' => 'စာ​မျက်​နှာ​အား​ရွေ့​ပြောင်း​ပါ​',
'movearticle'      => 'စာ​မျက်​နှာ​အား​ရွေ့​ပြောင်း​ပါ​',
'movepagebtn'      => 'စာ​မျက်​နှာ​အား​ရွေ့​ပြောင်း​ပါ​',
'pagemovedsub'     => 'ပြောင်းရွှေ့ခြင်းအောင်မြင်သည်',
'movedto'          => 'ရွေ့​ပြောင်း​ရန်​နေ​ရာ​',
'1movedto2'        => '[[$1]]  မှ​ [[$2]] သို့​',
'movereason'       => 'အ​ကြောင်း​ပြ​ချက်​',

# Namespace 8 related
'allmessages'     => 'စ​နစ်​၏​သ​တင်း​များ​',
'allmessagesname' => 'အမည်',

# Thumbnails
'thumbnail-more' => 'ချဲ့',

# Tooltip help for the actions
'tooltip-pt-userpage'    => 'ကျွန်တော့စာမျက်နှာ',
'tooltip-pt-mytalk'      => 'ကျွန်​တော့​ပြော​ရေး​ဆို​ရာ​',
'tooltip-pt-preferences' => 'ကျွန်​တော့​ရွေး​ချယ်​စ​ရာ​များ​',
'tooltip-pt-logout'      => 'ထွက်​ပါ​',
'tooltip-ca-protect'     => 'ဤစာမျက်နှာကို ထိန်းသိမ်းပါ',
'tooltip-ca-delete'      => 'ဤစာမျက်နှာဖျက်ပါ',
'tooltip-ca-move'        => 'ဤ​စာ​မျက်​နှာ​အား​ရွေ့​ပြောင်း​ပါ​',
'tooltip-t-upload'       => 'ဖိုင်တင်ပါ',
'tooltip-save'           => 'ပြင်ဆင်ရန်သိမ်းပါ',

# Special:NewImages
'ilsubmit' => 'ရှာ​ဖွေ​ရန်​',

# EXIF tags
'exif-exposuretime-format' => '$1 စက္ကန့် ($2)',
'exif-gpsaltitude'         => 'အမြင့်',

'exif-meteringmode-255' => 'အခြား',

'exif-lightsource-1' => 'နေ့အလင်း',

'exif-focalplaneresolutionunit-2' => 'လက်မှတ်',

'exif-scenecapturetype-3' => 'ညနေပုံ',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'အားလုံး',
'imagelistall'     => 'အားလုံး',
'watchlistall2'    => 'အားလုံး',
'namespacesall'    => 'အားလုံး',
'monthsall'        => 'အားလုံး',

# E-mail address confirmation
'confirmemail' => 'အီးမေးကိုအတည်ပြုပါ',

# Multipage image navigation
'imgmultigo' => 'သွား​ပါ!',

# Table pager
'table_pager_limit_submit' => 'သွား​ပါ​',

# Auto-summaries
'autosumm-new' => 'စာမျက်နှာအသစ်: $1',

# Special:Version
'version-other' => 'အခြား',

# Special:FilePath
'filepath-page' => 'ဖိုင်:',

# Special:SpecialPages
'specialpages' => 'အ​ထူး​စာ​မျက်​နှာ​',

);
