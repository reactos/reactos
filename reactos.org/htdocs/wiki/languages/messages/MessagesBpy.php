<?php
/** Bishnupria Manipuri (ইমার ঠার/বিষ্ণুপ্রিয়া মণিপুরী)
 *
 * @ingroup Language
 * @file
 *
 * @author Usingha
 * @author Uttam Singha, Dec 2006
 */

$digitTransformTable = array(
	'0' => '০',
	'1' => '১',
	'2' => '২',
	'3' => '৩',
	'4' => '৪',
	'5' => '৫',
	'6' => '৬',
	'7' => '৭',
	'8' => '৮',
	'9' => '৯'
);

$namespaceNames = array(
	NS_MEDIA          => 'মিডিয়া',
	NS_SPECIAL        => 'বিশেষ',
	NS_MAIN           => '',
	NS_TALK           => 'য়্যারী',
	NS_USER           => 'আতাকুরা',
	NS_USER_TALK      => 'আতাকুরার_য়্যারী',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1_য়্যারী',
	NS_IMAGE          => 'ছবি',
	NS_IMAGE_TALK     => 'ছবি_য়্যারী',
	NS_MEDIAWIKI      => 'মিডিয়াউইকি',
	NS_MEDIAWIKI_TALK => 'মিডিয়াউইকির_য়্যারী',
	NS_TEMPLATE       => 'মডেল',
	NS_TEMPLATE_TALK  => 'মডেলর_য়্যারী',
	NS_HELP           => 'পাংলাক',
	NS_HELP_TALK      => 'পাংলাকর_য়্যারী',
	NS_CATEGORY       => 'থাক',
	NS_CATEGORY_TALK  => 'থাকর_য়্যারী',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'লিঙ্কর তলে দুরগ দিক:',
'tog-highlightbroken'         => 'বাগা লিঙ্ক অতারে<a href="" class="new">এসারে</a> দেখাদে (নাইলে: এসারে<a href="" class="internal">?</a>).',
'tog-justify'                 => 'অনুচ্ছেদহানির দুরগি দ্বিয়পারাদেত্ত মান্নাকরিক',
'tog-hideminor'               => 'হুরু পতানি গুর',
'tog-extendwatchlist'         => 'পতাসি অতা দেখা দেনারকা আহিরফঙে থসি তালিকাহান সালকরানি অক',
'tog-usenewrc'                => 'পতাসি অতারমা হাব্বিত্ত ঙালসেতা (জাভাস্ক্রিপ্ট)',
'tog-numberheadings'          => 'নিজেলত্ত পাজালার চিঙনাঙ',
'tog-showtoolbar'             => 'পতানির আতিয়ার দেহাদে (জাভাস্ক্রিপ্ট)',
'tog-editondblclick'          => 'দ্বিমাউ যাতিয়া পতাহান পতিক (জাভাস্ক্রিপ্ট)',
'tog-editsection'             => '[পতিক] লিঙ্ক এহান্ন পরিচ্ছদ পতানি অক',
'tog-editsectiononrightclick' => 'পরিচ্ছদ পতানির য়্যাথাঙহান বাতেদের গোথামগ <br /> পরিচ্ছদর চিঙনাঙর গজে যাতিলে দে (জাভাস্ক্রিপ্ট)',
'tog-showtoc'                 => 'বিষয়র মাঠেলহানি দেহাদে (যে পাতারতা ৩হানর গজে চিঙনাঙ আসে)',
'tog-rememberpassword'        => 'কম্পিউটার এহাত মর লগইন নিঙশিঙে থ',
'tog-editwidth'               => 'পতিক উপুগর দীঘালাহান পুরা ইসে',
'tog-watchcreations'          => 'যে পতাহানি মি ইকরিসু অতা মর তালাবির তালিকাত থ',
'tog-watchdefault'            => 'যে পতাহানি মি পতাসু অতা মর তালাবির তালিকাত থ',
'tog-watchmoves'              => 'যে পতাহানি মি থেইকরিসু অতা মর তালাবির তালিকাত থ',
'tog-watchdeletion'           => 'যে পতাহানি মি পুসিসু অতা মর তালাবির তালিকাত থ',
'tog-minordefault'            => 'অকরাতই হাব্বি পতা ফাঙনেই বুলিয়া দেহাদে',
'tog-previewontop'            => 'পতা উপুগর গজে লেহার মিল্লেখ দেহাদে',
'tog-previewonfirst'          => 'পয়লা পতানিহাত মিল্লেখ দেহাদে',
'tog-nocache'                 => 'পাতা য়মকরানি থা নাদি',
'tog-enotifwatchlistpages'    => 'মরে ইমেইল কর যদি মর মিল্লেঙে থসু অতা পতিলে',
'tog-enotifusertalkpages'     => 'মরে ইমেইল কর যদি মর য়্যারির পাতা পতিলে',
'tog-enotifminoredits'        => 'মরে ইমেইল কর পাতা আহানর পতানিহান হুরু ইলেউ',
'tog-enotifrevealaddr'        => 'জানানি মেইল অতাত মর ইমেইলর ঠিকানাহান ফঙকর',
'tog-shownumberswatching'     => 'চাকুরার সংখ্যাহান দেহাদে',
'tog-fancysig'                => 'দস্তখত তিলকরানি (নিজেত্ত লিঙ্ক নেইকরিয়া)',
'tog-externaleditor'          => 'পয়লাকাত্তই বারেদের পতানির আতিয়ার আতা',
'tog-externaldiff'            => 'পয়লাকাত্ত বারেদের ফারাকহান আতা',
'tog-showjumplinks'           => '"চঙদে" বুলতারা মিলাপর য়্যাথাঙদে',
'tog-uselivepreview'          => 'লগে লগে মিল্লেঙ আহান দেহাদে (জাভাস্ক্রিপ্ট) (লইনাসে)',
'tog-forceeditsummary'        => 'খালি পতা সারমর্ম হমিলে মরে হারপুৱাদে',
'tog-watchlisthideown'        => 'মি পতাসু অতা গুর মর তালাবিত্ত',
'tog-watchlisthidebots'       => 'বটল পতাসি অতা গুর মর তালাবিত্ত',
'tog-watchlisthideminor'      => 'হুরু করে পতাসি অতা গুর মর তালাবিত্ত',
'tog-nolangconversion'        => 'সারুকর সিলপা থেপকর',
'tog-ccmeonemails'            => 'আরতারে দিয়াপেঠাউরি ইমেইল মরাঙউ কপি আহান যাকগা',
'tog-diffonly'                => 'ফারাকর তলে পাতাহানর বিষয়বস্তু নাদেখাদি',
'tog-showhiddencats'          => 'আরুমে আসে থাকহানি ফংকর',

'underline-always'  => 'হারি সময়',
'underline-never'   => 'সুপৌনা',
'underline-default' => 'বাউজারগত যেসারে আসিল',

'skinpreview' => '(মিল্লেখ)',

# Dates
'sunday'        => 'লামুইসিং',
'monday'        => 'নিংথৌকাপা',
'tuesday'       => 'লেইপাকপা',
'wednesday'     => 'ইনসাইনসা',
'thursday'      => 'সাকলসেন',
'friday'        => 'ইরেই',
'saturday'      => 'থাংচা',
'sun'           => 'লামু',
'mon'           => 'নিং',
'tue'           => 'লেই',
'wed'           => 'ইন',
'thu'           => 'সাকল',
'fri'           => 'ইরে',
'sat'           => 'থাং',
'january'       => 'জানুয়ারী',
'february'      => 'ফেব্রুয়ারী',
'march'         => 'মার্চ',
'april'         => 'এপ্রিল',
'may_long'      => 'মে',
'june'          => 'জুন',
'july'          => 'জুলাই',
'august'        => 'আগস্ট',
'september'     => 'সেপ্টেম্বর',
'october'       => 'অক্টোবর',
'november'      => 'নভেম্বর',
'december'      => 'ডিসেম্বর',
'january-gen'   => 'জানুয়ারী',
'february-gen'  => 'ফেব্রুয়ারী',
'march-gen'     => 'মার্চ',
'april-gen'     => 'এপ্রিল',
'may-gen'       => 'মে',
'june-gen'      => 'জুন',
'july-gen'      => 'জুলাই',
'august-gen'    => 'আগষ্ট',
'september-gen' => 'সেপ্টেম্বর',
'october-gen'   => 'অক্টোবর',
'november-gen'  => 'নভেম্বর',
'december-gen'  => 'ডিসেম্বর',
'jan'           => 'জানু',
'feb'           => 'ফেব্রু',
'mar'           => 'মার্চ',
'apr'           => 'এপ্রিল',
'may'           => 'মে',
'jun'           => 'জুন',
'jul'           => 'জুলাই',
'aug'           => 'আগস্ট',
'sep'           => 'সেপ্টে',
'oct'           => 'অক্টো',
'nov'           => 'নভে',
'dec'           => 'ডিসে',

# Categories related messages
'pagecategories'           => '{{PLURAL:$1|থাক|থাকহানি}}',
'category_header'          => '"$1" বিষয়রথাকে আসে নিবন্ধহানি',
'subcategories'            => 'উপথাক',
'category-media-header'    => '"$1" থাকর মিডিয়া',
'category-empty'           => "''এরে থাক এহাত এবাকা কোন পাতা বা মিডিয়া নেই''",
'hidden-categories'        => '{{PLURAL:$1|গুরিসি থাকহান|গুরিসি থাকহানি}}',
'hidden-category-category' => 'আরুম করিসি থাকহানি', # Name of the category where hidden categories will be listed
'listingcontinuesabbrev'   => 'চলতই',

'mainpagetext'      => "<big>'''মিডিয়াউইকি হবাবালা ইয়া ইন্সটল ইল'''</big>",
'mainpagedocfooter' => 'উইকি সফটৱ্যার এহান আতানির বারে দরকার ইলে [http://meta.wikimedia.org/wiki/Help:Contents আতাকুরার গাইড]হানর পাঙলাক নেগা।

== অকরানিহান ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings কনফিগারেশন সেটিংর তালিকাহান]
* [http://www.mediawiki.org/wiki/Manual:FAQ মিডিয়া উইকি আঙলাক]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce মিডিয়া উইকির ফঙপার বারে মেইলর তালিকাহান]',

'about'          => 'বারে',
'article'        => 'মেথেলর পাতা',
'newwindow'      => '(নুৱা উইন্ডত নিকুলতই)',
'cancel'         => 'বাতিল করেদে',
'qbfind'         => 'বিসারিয়া চা',
'qbbrowse'       => 'বুলিয়া চা',
'qbedit'         => 'পতানি',
'qbpageoptions'  => 'পাতা এহানর সারুক',
'qbpageinfo'     => 'পাতা এহানর পৌ',
'qbmyoptions'    => 'মর পছন',
'qbspecialpages' => 'বিশেষ পাতাহানি',
'moredotdotdot'  => 'আরাকউ...',
'mypage'         => 'মর পাতাহান',
'mytalk'         => 'মর য়্যারি-পরি',
'anontalk'       => 'অচিনা এগর য়্যারির পাতা',
'navigation'     => 'দিশা-ধরুনী',
'and'            => 'বারো',

# Metadata in edit box
'metadata_help' => 'মেটাডাটা:',

'errorpagetitle'    => 'লাল',
'returnto'          => '$1-ত আলথকে যাগা।',
'tagline'           => 'মুক্ত বিশ্বকোষ উইকিপিডিয়াত্ত',
'help'              => 'পাংলাক',
'search'            => 'বিসারিয়া চা',
'searchbutton'      => 'বিসারানি',
'go'                => 'হাত',
'searcharticle'     => 'হাত',
'history'           => 'পতাহানর ইতিহাসহান',
'history_short'     => 'ইতিহাসহান',
'updatedmarker'     => 'লমিলগা চানাহাত্ত বদলিসেতা',
'info_short'        => 'পৌ',
'printableversion'  => 'ছাপানি একরব সংস্করণ',
'permalink'         => 'আকুবালা মিলাপ',
'print'             => 'ছাপা',
'edit'              => 'পতানি',
'create'            => 'হঙকর',
'editthispage'      => 'পাতা এহান পতিক',
'create-this-page'  => 'পাতা এহান হঙকর',
'delete'            => 'পুসানি',
'deletethispage'    => 'পাতা এহান পুসে বেলিক',
'undelete_short'    => 'পুসানিহান আলকর {{PLURAL:$1|পতাহান|$1 পতাহানি}}',
'protect'           => 'লুকর',
'protect_change'    => 'লুকরানিহান সিলকর',
'protectthispage'   => 'পাতা এহান লু কর',
'unprotect'         => 'লু নাকরি',
'unprotectthispage' => 'পাতা এহানর লুপাহান এরাদিক',
'newpage'           => 'নুৱা পাতা',
'talkpage'          => 'পাতা এহান্ন য়্যারি দিক',
'talkpagelinktext'  => 'য়্যারি',
'specialpage'       => 'বিশেষ পাতাহান',
'personaltools'     => 'নিজস্ব আতিয়ার',
'postcomment'       => 'নিজর মতহান থ',
'articlepage'       => 'নিবন্ধ চেইক',
'talk'              => 'য়্যারী',
'views'             => 'চা',
'toolbox'           => 'আতিয়ার',
'userpage'          => 'আতাকুরার পাতাহান চেইক',
'projectpage'       => 'প্রকল্পর পাতাহান',
'imagepage'         => 'ছবির পাতাহান চেইক',
'mediawikipage'     => 'পৌর পাতাহান চা',
'templatepage'      => 'মডেলর পাতাহান চা',
'viewhelppage'      => 'পাঙলাকর পাতাহান চা',
'categorypage'      => 'বিষয়থাকর পাতাহানি চা',
'viewtalkpage'      => 'য়্যারীর পাতাহান চেইক',
'otherlanguages'    => 'আরআর ঠারে',
'redirectedfrom'    => '($1 -ত্ত পাকদিয়া আহিল)',
'redirectpagesub'   => 'কুইপা পাতা',
'lastmodifiedat'    => 'পাতা এহানর লমিলগা পতানিহান $2, $1.', # $1 date, $2 time
'viewcount'         => 'পাতা এহান $1 মাউ চানা ইল।',
'protectedpage'     => 'লুকরা পাতা',
'jumpto'            => 'চঙদে:',
'jumptonavigation'  => 'দিশা ধরানি',
'jumptosearch'      => 'বিসারা',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}}র বারে',
'aboutpage'            => 'Project:বারে',
'bugreports'           => 'লাল বিবরণী',
'bugreportspage'       => 'Project:লাল_বিবরণী',
'copyright'            => '$1-র মাতুঙে এহানর মেথেলহানি পানা একরের।',
'copyrightpagename'    => '{{SITENAME}} স্বত্তাধিকারহান',
'copyrightpage'        => '{{ns:project}}:স্বত্তাধিকারহানি',
'currentevents'        => 'হাদি এহানর ঘটনা',
'currentevents-url'    => 'Project:হাদি এহানর ঘটনাহানি',
'disclaimers'          => 'দাবি বেলানি',
'disclaimerpage'       => 'Project:ইজ্জু দাবি বেলানি',
'edithelp'             => 'পতানি পাংলাক',
'edithelppage'         => 'Help:কিসাদে_পাতা_আহান_পতানি',
'faq'                  => 'আঙলাক',
'faqpage'              => 'Project:আঙলাক',
'helppage'             => 'Help:পাংলাক',
'mainpage'             => 'পয়লা পাতা',
'mainpage-description' => 'পয়লা পাতা',
'policy-url'           => 'Project:নীতিহান',
'portal'               => 'শিংলুপ',
'portal-url'           => 'Project:শিংলুপ',
'privacy'              => 'লুকরানির নীতিহান',
'privacypage'          => 'Project:লুকরানির নীতিহান',

'badaccess'        => 'য়্যাথাঙে লালসে',
'badaccess-group0' => 'তি যে কামহানর হেইচা করিসত, তরতা অহান করানির য়্যাথাং নেই।',
'badaccess-group1' => 'তি যে কামহানর হেইচা করিসত, অহান করানির য়্যাথাং হুদ্দা $1 গ্রুপরতা আসে।',
'badaccess-group2' => 'তি যে কামহানর হেইচা করিসত, অহান করানির য়্যাথাং হুদ্দা $1 গ্রুপর আতাকুরারতা আসে।',
'badaccess-groups' => 'তি যে কামহানর হেইচা করিসত, অহান করানির য়্যাথাং হুদ্দা $1 গ্রুপরতা আসে।',

'ok'                  => 'চুমিসে',
'retrievedfrom'       => "'$1' -ত্ত আনানি অসে",
'youhavenewmessages'  => 'তরতা $1 ($2) আসে।',
'newmessageslink'     => 'নুৱা পৌ',
'newmessagesdifflink' => 'গেলগা সিলপা',
'editsection'         => 'পতিক',
'editold'             => 'পতিক',
'viewsourceold'       => 'উৎস চা',
'editsectionhint'     => 'সেকসনহান পতা: $1',
'toc'                 => 'মেথেল',
'showtoc'             => 'ফংকর',
'hidetoc'             => 'মেথেল আরুম কর',
'site-rss-feed'       => '$1 আরএসএস ফিড',
'site-atom-feed'      => '$1 এটম ফিড',
'page-rss-feed'       => '"$1" আরএসএস ফিড',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'নিবন্ধ',
'nstab-user'      => 'আতাকুরার পাতা',
'nstab-special'   => 'বিশেষ',
'nstab-project'   => 'প্রকল্প পাতা',
'nstab-image'     => 'ফাইল',
'nstab-mediawiki' => 'পৌ',
'nstab-template'  => 'মডেল',
'nstab-help'      => 'পাঙলাকর পাতা',
'nstab-category'  => 'থাক',

# General errors
'error'              => 'লালুইসে',
'cachederror'        => 'এরে পাতা এহান বা লাতলগ পুছানি নাকরল। (নিঙকরুরিতাঃ আগেদে কুঙগ আগই পুছে বেলাসিসাত)',
'badarticleerror'    => 'এরে পাতা এহান কাম এহান করানি সম্ভব নেই।',
'badtitle'           => 'চিঙনাঙহান চুমনাইসে নাইসে।',
'badtitletext'       => 'হেইচা করিসত পাতাহানর চিঙনাঙহান চুম নাইসে, খালি বা আর ঠার বা আন্তঃউইকি চিঙনাঙ মিলাপ অসিল। হয়ত এহানত আক বারো গজে কোন আখর মিহিসে, যেতা চিঙনাঙে বরানি লালুইসে।',
'viewsource'         => 'উৎসহান চা',
'viewsourcefor'      => '$1-র কা',
'viewsourcetext'     => 'পাতা এহানর উত্স চা বারো কপি করে পারর:',
'protectedinterface' => 'পাতা এহানর মেথেল উইকি সফটওয়্যারর ইন্টারফেসর পৌহান দের, অহানে এহানরে ইতু করিয়া থনা অসে এবিউসেত্ত ঙাক্করানির কাজে।',

# Login and logout pages
'logouttitle'                => 'আতাকুরার নিকুলানি',
'welcomecreation'            => '== সম্ভাষা, $1! ==

তর একাউন্টহান মুকিল। তর {{SITENAME}} পছনহান পতানি না পাহুরিস।',
'loginpagetitle'             => 'আতাকুরার হমানি',
'yourname'                   => 'আতাকুরার নাংহান (Username)',
'yourpassword'               => 'খন্তাচাবিগ (password)',
'yourpasswordagain'          => 'খন্তাচাবিগ (password) আরাকমু ইকর',
'remembermypassword'         => 'এরে কম্পিউটার এহাত্ত সাইট এহাত মর হমানিহান মনে থ',
'yourdomainname'             => 'তর ডোমেইনগ',
'login'                      => 'হমানি',
'nav-login-createaccount'    => 'লগইন / একাউন্ট খুল',
'loginprompt'                => 'তি যেসাদেউ [[Special:UserLogin|হমাসি {{SITENAME}}]] পাতা এহানর কুকিসর য়্যাথাঙ দে।',
'userlogin'                  => 'হমানি / নৱা একাউন্ট খুলানি',
'logout'                     => 'নিকুলানি',
'userlogout'                 => 'নিকুলানি',
'nologin'                    => 'লগইন নেই? $1.',
'nologinlink'                => 'একাউন্ট আহান খুল',
'createaccount'              => 'একাউন্ট খুল',
'gotaccount'                 => 'মান্নাপা একাউন্ট আহান আগেত্তর আসে? $1।',
'gotaccountlink'             => 'লগইন',
'badretype'                  => 'খন্তাচাবি (password) দ্বিয়গি না মিলের।',
'youremail'                  => 'ই-মেইল *:',
'yourrealname'               => 'আৱৈপা নাংহান *:',
'yourlanguage'               => 'ঠারহান:',
'yournick'                   => 'দাহানির নাংহান:',
'email'                      => 'ইমেইল',
'prefs-help-realname'        => 'আয়ৌপা নাংহান নাদলেউ চলের।
যদি তি দের অতাইলে তর কামর থাকাত দেনাত সুবিধা অইতই।',
'loginerror'                 => 'লগইনে লালুইসে',
'loginsuccesstitle'          => 'লগইনহান চুমিল',
'loginsuccess'               => "'''এরে {{SITENAME}}ত তি \"\$1\" হিসাবে না হমাসত।'''",
'nosuchuser'                 => 'এরে "$1" নাঙর কোন আতাকুরা নেই।
তর বানানহান খিয়াল কর, নাইলে আরাক আহান হঙকর।',
'nosuchusershort'            => 'এরে "<nowiki>$1</nowiki>" নাঙর কোন আতাকুরা নেই।
তর বানানহান খিয়াল কর।',
'nouserspecified'            => 'তি আতাকুরার নাঙ আহান থনা লাগতই।',
'wrongpassword'              => 'খন্তাচাবি চুম নাইসে।
আলথকে হতনা কর।',
'wrongpasswordempty'         => 'খন্তা চাবি খালি ইসে।
বারো হতনা কর।',
'passwordtooshort'           => 'খন্তাচাবি লালুইসে নাইলে বাট্টি ইসে।
খন্তাচাবি যেসারেউ {{PLURAL:$1|মেয়েক আকগর|$1 মেয়েকগির}} বারো আতাকুরার নাঙেত্ত তঙাল অনা লাগতই।',
'mailmypassword'             => 'খন্তাচাবি ইমেইল করেদে',
'passwordremindertitle'      => 'নুয়া খন্তাচাবি {{SITENAME}}র কাজে',
'passwordremindertext'       => 'কুঙগ আগই (মনে অর তি, $1 আইপি ঠিকানা এহাত্ত) হেইচা করিসত যে আমি তরে {{SITENAME}}-র কা আরাক নুৱা খন্তাচাবি দিয়া পেঠাদেনা ($4)।
"$2" নাঙর আতাকুরার এপাগার খন্তাচাবি "$3"।
তি একাউন্টহান হমিয়াই খন্তাচাবি বদালানি থকিতই।

তি নায়া আরাক আগই হেইচা করিয়া থাইতারা, নাইলে তরতা পুরানা খন্তাচাবিগ নিঙশিঙ ইয়া থার অতাইলে বারো অগ সিলকরানির খৌরাঙ না থার অতা ইলে এরে পৌ এহান বেলিয়া পুরানা খন্তাচাবিগই আতা পারতেই।',
'noemail'                    => 'এহানাত আতাকুরা "$1"র কুন ইমেল ঠিকানা নেয়সে।',
'passwordsent'               => 'নুৱা খন্তাচাবি দিয়াপেঠানি ইল আতাকুরা "$1"র ইমেইল ঠিকানাত।
কৃপা করিয়া লগইন কর পানার লগে লগে।',
'eauthentsent'               => 'লেপ্পা ইমেইল আহান তি দিয়াসিলে ইমেইল ঠিকানাহাত দিয়া পেঠানি ইল।
আর কোন ইমেইল দিয়া এপঠানির আগেই তি পাসত ইমেইল অহানর নিদর্শনা ইলয়া যাগা, এহান করানি অরতা তর ইমেইল ঠিকানাহান চুমিসেতানা কিতা লেপকরানির কা।',
'acct_creation_throttle_hit' => 'ঙাক্করেদিবাং, তি এবাকাপেয়া $1হান অ্যাকাউন্ট হংকরেবেলাসত৷ অতাত্ত বপ হঙকরানির য়্যাথাং নেই।৷',
'accountcreated'             => 'একাউন্টহান হঙকরানি ইল',
'accountcreatedtext'         => 'আতাকুরা $1 -র কা একাউন্টহান হঙকরানি ইল।',

# Edit page toolbar
'bold_sample'     => 'গাঢ়পা ৱাহি',
'bold_tip'        => 'গাঢ়পা ৱাহি',
'italic_sample'   => 'ইটালিক মেয়েক',
'italic_tip'      => 'ইটালিক মেয়েক',
'link_sample'     => 'চিঙনাঙ মিলাপ',
'link_tip'        => 'ভিতরর মিলাপ',
'extlink_sample'  => 'http://www.example.com চিঙনাঙ মিলাপ',
'extlink_tip'     => 'বারেদের মিলাপ (মুঙে http:// বারনি না পাহুরিস)',
'headline_sample' => 'চিঙনাঙর খন্তাহানি',
'headline_tip'    => 'থাক ২র চিঙনাঙ',
'math_sample'     => 'এহাত সুত্র বরা',
'math_tip'        => 'অংকর সুত্র (LaTeX)',
'nowiki_sample'   => 'ফরমেট নাকরিসি মেয়েক বরা',
'nowiki_tip'      => 'উইকির পাজালানিহান লালুয়া যাগা',
'image_tip'       => 'তিলকরিসি ফাইলগ',
'media_tip'       => 'ফাইল মিলাপ',
'sig_tip'         => 'তর স্বাক্ষরহান লগে খেন্তাম বরিয়া',
'hr_tip'          => 'পাথারি খাস (খানি করা ইয়া আতা)',

# Edit pages
'summary'                => 'সারমর্ম',
'subject'                => 'বিষয়/চিঙনাঙ',
'minoredit'              => 'এহান হুরু-মুরু সম্পাদনাহানহে।',
'watchthis'              => 'পাতাএহান খিয়ালে থ',
'savearticle'            => 'পাতাহান ইতুকর',
'preview'                => 'আগচা',
'showpreview'            => 'আগচা',
'showdiff'               => 'পতাসিতা দেহাদে',
'anoneditwarning'        => "'''সিঙুইসঃ''' তি লগইন নাকরিসত। পতানির ইতিহাসহাত তর IP addressহান সিজিল ইতই।",
'summary-preview'        => 'সারমর্মর আগচা',
'blockedtitle'           => 'আতাকুরাগরে থেপ করানি অসে',
'blockedtext'            => "<big>'''তর আতাকুরা নাঙহান নাইলেউ আইপি ঠিকানাহানরে থেপকরানি অসে।'''</big>

থেপকরিসেতাই: $1
এহানর কারণহান অসেতাইঃ: ''$2''

* থেপকরানি অকরিসিতা: $8
* থেপকরানিহান লমিতইতা: $6
* থেপকরানি মনাসিলাতা: $7

তি $1 নাইলেউ [[{{MediaWiki:Grouppage-sysop}}|প্রশাসকর]] মা যে কোন আগর লগে বিষয় এহান্ন য়্যারি পরি দে পারর। বিশেষ মাতিলতাঃ তর ই-মেইল ঠিকানাহান যদি [[Special:Preferences|তর পছন তালিকাত]] বরিয়া নাথার, অতা ইলে তি উইকিপিডিয়াত হের আতাকুরারে ই-মেইল করানি নুৱারবে। তর আইপি ঠিকানাহান ইলতাই $3 বারো থেপকরিসি আইপিগ ইলতাই #$5। 
কৃপা করিয়া যে কোন যোগাযোগর সময়ত এরে আইপি ঠিকানাহানি যেসাদেউ বরিস।",
'confirmedittitle'       => 'সম্পাদনা করানির কা ই-মেইল লেপকানি থকিতই',
'confirmedittext'        => 'যেহানউ সম্পাদনা করানির আগে তর ই-মেইল ঠিকানাহন যেসাদেউ লেপকরানি লাগতই। কৃপাকরিয়া তর ই-মেইল ঠিকানাহান [[Special:Preferences|আতাকুরার পছনতালিকা]]ত চুমকরে বরা।',
'loginreqtitle'          => 'লগইন দরকার ইসে',
'accmailtitle'           => 'খন্তাচাবি(password) দিয়াপেঠৱা দিলাং।',
'accmailtext'            => '"$1"-র খন্তাচাবি(password) $2-রাঙ দিয়াপেঠৱাদেনা ইল।',
'newarticle'             => '(নুৱা)',
'newarticletext'         => 'এর নিবন্ধ এহান এপাগাউ {{SITENAME}}-ত না তিলসে। তি চেইলে তলর বক্সগত বিষয়হানর বারে খানি ইকরিয়া ইতুকরে পারর বারো নিবন্ধহান অকরে পারর। যদি হারনাপেয়া এহাত আহিয়া থার অতা ইলে ব্রাউজারর ব্যাক গুতমগত ক্লিক করিয়া আগর পাতাত আল পারর।',
'anontalkpagetext'       => "''এহান অচিনা অতার য়্যারির পাতাহান। এরে আইপি ঠিকানা (IP Address) এহানাত্ত লগ-ইন নাকরিয়া পতানিত মেইক্ষু অসিল। আক্কুস ক্ষেন্তামে আইপি ঠিকানা হামেসা বদল অর, বিশেষ করিয়া ডায়াল-আপ ইন্টারনেট, প্রক্সি সার্ভার মাহি ক্ষেত্র এতা সিলরতা, বারো আগত্ত বপ ব্যবহারকারেকুরার ক্ষেত্রত প্রযোজ্য ইতে পারে। অহানে তি নিশ্চকে এরে আইপি এহাত্ত উইকিপিডিয়াত হমিয়া কোন য়্যারী দেখর, অহান তরে নিঙকরিয়া নাউ ইতে পারে। অহানে হাবিত্ত হবা অর, তি যদি [[Special:UserLogin|লগ-ইন করর, বা নৱা একাউন্ট খুলর]] অহানবুলতেউ লগ-ইন করলে কুঙগউ তর আইপি ঠিকানাহান, বারো অহানর মাতুঙে তর অবস্থানহান সুপকরেউ হার না পেইবা।''",
'noarticletext'          => 'এপাগা এরে পাতাত কোন টেক্সট নেই। তি মনেইলে হের পাতাহান [[Special:Search/{{PAGENAME}}|এরে চিঙনাঙল বিসারা পারর]] নাইলে [{{fullurl:{{FULLPAGENAME}}|action=edit}} এরে পাতা এহান পতা পারর]।',
'clearyourcache'         => "'''খিয়াল থ:''' তর পছনহানি রক্ষা করানির থাঙনাত পতাহানি চানার কা তর ব্রাউজারর ক্যাশ লালুয়া যানা লাগতে পারে। '''মোজিলা/ফায়ারফক্স/সাফারি:''' শিফট কী চিপিয়া থয়া রিলোড-এ ক্লিক কর, নাইলে ''কন্ট্রোল-শিফট-R''(এপল ম্যাক-এ ''কমান্ড-শিফট-R'') আকপাকে চিপা; '''ইন্টারনেট এক্সপ্লোরার:''' ''কন্ট্রোল'' চিপিয়া থয়া রিফ্রেশ-এ ক্লিক কর, নাইলে ''কন্ট্রোল-F5'' চিপা; '''কংকারার:''' হুদ্দা রিলোড ক্লিক করলে বা F5 চিপিলে চলতই; '''অপেরা''' আতাকুরাই ''Tools→Preferences''-এ গিয়া কাশ সম্পূর্ণ ঙক্ষি করানি লাগতে পারে।",
'previewnote'            => '<strong>এহান হুদ্দা আগচাহান;
ফারাকহান এপাগাউ ইতু করানি নাইসে!</strong>',
'editing'                => 'পতানি চলের $1',
'editingsection'         => '$1র পতানি চলের (ডেংগ)',
'yourtext'               => 'তর ইকরা বিষয়হানি',
'yourdiff'               => 'ফারাকহানি',
'copyrightwarning'       => 'দয়া করিয়া খিয়াল কর {{SITENAME}}-ত হারি অবদান $2-র মাতুঙে পাসিতা (আরাকউ হবাকরে $1-ত চা)। তর জমা দিয়াসত লেখা যেগউ বে-রিদয় ইয়া পতিতে পারে বারো যেসারে খুশি অসারে বিলিতে পারে। তি যদি এহানর বারে একমত নার, অতা ইলে তর লেখা এহাত জমা নাদি।<br />
তি আরাকউ ৱাশাক করর যে, এরে লেখা এহান তি নিজে ইকিসতহান, নাইলে  হাব্বির কা উন্মুক্ত কোন উৎস আহাত্ত পাসতহান।
<strong>স্বত্ব সংরক্ষিত অসে অসাদে কোন লেখা স্বত্বাধিকারীর য়্যাথাঙ না লুইয়া এহাত জমা না দিস!</strong>',
'longpagewarning'        => '<strong>সিঙুইস: এরে পাতা এহান $1 কিলোবাইট ডাঙর; ব্রাউজার আকেইগত ৩২ কিলোবাইটর গজে ডাঙর পাতানিত বেরা ইতে পারে।
দয়া করিয়া পাতা এহানরে হুরকা হুরকা কত অংশত খেইকরানির হতনা কর।</strong>',
'templatesused'          => 'পাতাহান মডেল বরাসিতা:',
'templatesusedpreview'   => 'আগচা এহানাত মিহিসে মডেল:',
'template-protected'     => '(লুকরিসি)',
'template-semiprotected' => '(আধা-কাচা লুকরিসি)',
'nocreatetext'           => '{{SITENAME}}-এরে নুৱা পাতা এহানর পতানিহানাত থিতপা আসে।
তি আলথকে গিয়া আসে হের পাতা সিলকরানি পারর, নাইলে [[Special:UserLogin|অ্যাকাউন্টহানাত হমানি বারো অ্যাকাউন্ট খুলে পারর]]।',
'recreate-deleted-warn'  => "'''সিঙুইস: তি যে পতাহান হঙকরলে অহান আগে আরাকমু হঙকরানি অসিল।

পাতা এহান তি আরাতা হঙকরতেইতানা কিতা খালকরিয়া চা।
তর সুবিধারকা পাতা এহানর পুসিসি লগ এহানাত দেনা ইল:",

# History pages
'viewpagelogs'        => 'পাতাহানর লগ চা',
'currentrev'          => 'হাদিএহানর পতানি',
'revisionasof'        => 'রিভিসনহান $1 পেয়া',
'revision-info'       => '$1 পেয়া  $2-এ পতাসেতা',
'previousrevision'    => '←পুরানা পতানিহান',
'nextrevision'        => 'নুৱা ভার্সনহান→',
'currentrevisionlink' => 'হাদি এহানর পতানি',
'cur'                 => 'এপাগা',
'last'                => 'লাতঙ',
'page_first'          => 'পয়লাকা',
'page_last'           => 'লমনিত',
'histlegend'          => 'ফারাক (Diff) বাছানি: যে সংস্করণহানি তুলনা করানি চার, অহান লেপকরিয়া এন্টার বা তলর খুথামগত যাতা।<br />
নির্দেশিকা: (এব) = এবাকার সংস্করণহানর লগে ফারাক,(আ) =  জানে আগে-আগে গেলগা সংস্করণহানর লগে ফারাক, হ = হুরু-মুরু (নামাতলেউ একরব অসারে) সম্পাদনাহান।',
'histfirst'           => 'হাব্বিত্ত পুরানা',
'histlast'            => 'হাব্বিত্ত নুৱা',

# Revision feed
'history-feed-item-nocomment' => '$1 খেন্তাম $2 ত', # user at time

# Diffs
'history-title'           => '"$1"-র রিভিসন ইতিহাসহান',
'difference'              => '(রিভিসনহানির ফারাকহান)',
'lineno'                  => 'লাইন $1:',
'compareselectedversions' => 'বাসাইল সংস্করণহানি তুলনা কর',
'editundo'                => 'আলকর',
'diff-multi'              => '({{PLURAL:$1|হমবুকর রিভিসন আহান|$1 হমবুকর রিভিসন হানি}} দেহাদেনা এহাত না মিহিসে।)',

# Search results
'noexactmatch' => "'''\"\$1\" চিংনাঙর কোন পাতা নেই।'''
তি [[:\$1|পাতা এহান হঙকরে পারর]]।",
'prevn'        => 'পিসেদে $1',
'nextn'        => 'থাংনাত $1',
'viewprevnext' => 'চা ($1) ($2) ($3)',
'powersearch'  => 'এডভান্স বিসারানি',

# Preferences page
'preferences'    => 'পছনহানি',
'mypreferences'  => 'মর পছন',
'changepassword' => 'খন্তাচাবি(password) পতা',
'saveprefs'      => 'ইতু',
'retypenew'      => 'নুৱা খন্তাচাবি বারো টাইপ কর:',
'columns'        => 'দুরগিঃ',
'allowemail'     => 'আরতা(ব্যবহার করেকুরা)ই ইমেইল করানির য়্যাথাং দে।',

'grouppage-sysop' => '{{ns:project}}:প্রশাসকগি',

# User rights log
'rightslog' => 'আতাকুরার অধিকারর লগ',

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|সিলপা|সিলপাহানি}}',
'recentchanges'                  => 'হাদিএহান পতাসিতা',
'recentchanges-feed-description' => 'ফিড এহানর মা পাতা এহার পতানিহানর গজে মিল্লেং দে।',
'rcnote'                         => 'গেলগা <strong>$2</strong> দিনে পতাসি <strong>$1</strong> হান পরিবর্তন তলে দেখাদেনা ইলতা $5, $4 পেয়া।',
'rcnotefrom'                     => "তলে গেলগা '''$2''' ত্ত পতাসিতা দেনা অইল ('''$1''' পেয়া)।",
'rclistfrom'                     => 'নুৱাতা পতাসিতা $1 পাতাহানাত্ত চিঙকরিয়া',
'rcshowhideminor'                => '$1 হুরু পতানিহান',
'rcshowhidebots'                 => '$1 বটগি',
'rcshowhideliu'                  => '$1 হমাসি আতাকুরা',
'rcshowhideanons'                => '$1 হারানাপাসি আতাকুরা',
'rcshowhidepatr'                 => '$1 পাহারাত আসে পতানি',
'rcshowhidemine'                 => '$1 মর পাতানিহানি',
'rclinks'                        => 'গেলগা $1 হান পতানি দেখাদে $2 দিনরতা <br />$3',
'diff'                           => 'ফারাক',
'hist'                           => 'ইতিহাসহান',
'hide'                           => 'আরুম',
'show'                           => 'দেখাদে',
'minoreditletter'                => 'হ',
'newpageletter'                  => 'নু',
'boteditletter'                  => 'ব',

# Recent changes linked
'recentchangeslinked'          => 'সাকেই আসে পতা',
'recentchangeslinked-title'    => 'পতানিহান "$1"র লগে সর্ম্পক আসে',
'recentchangeslinked-noresult' => 'দেনা অসে খেন্তামর ভিতরে পতাসিতা নেই।',
'recentchangeslinked-summary'  => "লেপকরা পাতা আহান (অথবা লেপকরা বিষয়শ্রেণী)ত্ত তিলসে এরে পাতা এহানর হাদি এহান পতাসি অহানর লাতঙ দেনা অইল। তর [[Special:Watchlist|তর চালাতঙ]]এ থসি পাতাহানি '''গাঢ়''' করিয়া দেহাদেনা অসে।",

# Upload
'upload'          => 'আপলোড ফাইল',
'uploadbtn'       => 'আপলোড',
'uploadlogpage'   => 'আপলোড করিসি লগ',
'badfilename'     => 'ফাইলগর নাঙহান পতিয়া $1" করানি ইল।',
'savefile'        => 'ফাইল ইতু',
'uploadedimage'   => 'আপলোডকরানি অইল "[[$1]]"',
'watchthisupload' => 'পাতাএহান খিয়ালে থ',

# Special:ImageList
'imagelist' => 'ছবির তালিকা',

# Image description page
'filehist'                  => 'ফাইলর ইতিহাস',
'filehist-help'             => 'দিন/সময়-র গজে যাতিলে ঔ খেন্তাম পেয়া হঙিসে ফাইলগ চ পারতেই।',
'filehist-current'          => 'এপাগা',
'filehist-datetime'         => 'দিন/সময়',
'filehist-user'             => 'আতাকুরা',
'filehist-dimensions'       => 'চাঙহান',
'filehist-filesize'         => 'ফাইলর সাইজহান',
'filehist-comment'          => 'মতহান',
'imagelinks'                => 'জুরিসিতা',
'linkstoimage'              => 'এরে ফাইলর লগে {{PLURAL:$1|পাতার মিলাপ|$1 পাতাহানির মিলাপ}} আসে:',
'nolinkstoimage'            => 'ফাইল এগর লগে মিলাপ অসে অসাদে কোন পাতা নেই।',
'sharedupload'              => 'ফাইল এগ শেয়ার আপলোডে আসে, মনে অর আর আর প্রকল্পউ আতিতারা।',
'noimage'                   => 'এরে নাঙর কোন ফাইল নেই, তি পারর $1।',
'noimage-linktext'          => 'আপলোড কর',
'uploadnewversion-linktext' => 'এরে ফাইল এগর নুৱা সংস্করনহান আপলোড কর',

# MIME search
'mimesearch' => 'MIME বিসারানি',

# List redirects
'listredirects' => 'আলথক করেদের পাতার লাতঙগি',

# Unused templates
'unusedtemplates' => 'বপ নাচলের মডেলহানি',

# Random page
'randompage' => 'খাংদা পাতা',

# Random redirect
'randomredirect' => 'চৌরাপ আলথকপা',

# Statistics
'statistics' => 'হিসাবহান',

'disambiguations' => 'সন্দই চুমকরের পাতাহানি',

'doubleredirects' => 'আলথকে যানা দ্বিমাউ মাতের',

'brokenredirects' => 'বারো-নির্দেশ কামনাকরের',

'withoutinterwiki' => 'ঠারর মিলাপ নেয়সে পাতাহানি',

'fewestrevisions' => 'যে পাতাহানির কম রিভিসন অসে',

# Miscellaneous special pages
'nbytes'                  => '$1 বাইট',
'ncategories'             => '$1 {{PLURAL:$1|থাক|থাকহানি}}',
'nlinks'                  => '$1 {{PLURAL:$1|মিলাপ|মিলাপহানি}}',
'nmembers'                => '$1 {{PLURAL:$1|আতাকুরা|আতাকুরাগি}}',
'lonelypages'             => 'এতিম পাতাহানি',
'uncategorizedpages'      => 'বিষয়রথাকে নাবেলাসি পাতাহানি',
'uncategorizedcategories' => 'বিষয়থাকে নাবেলাসি থাকহানি',
'uncategorizedimages'     => 'বিষয়থাকে নাবেলাসি ফাইলগি',
'uncategorizedtemplates'  => 'বিষয়থাকে নাবেলাসি থাকহানি',
'unusedcategories'        => 'বিষয়থাক বপ নাচলের',
'unusedimages'            => 'নাচলের ফাইলগি',
'wantedcategories'        => 'চারাঙ বিষয়র থাকহানি',
'wantedpages'             => 'খৌরাঙর পাতাহানি',
'mostlinked'              => 'হাবিত্ত মিলাপ বপিসে পাতাহানি',
'mostlinkedcategories'    => 'বিষয়থাকে য়্যামসে মিলাপ',
'mostlinkedtemplates'     => 'মডেলহানিত য়্যামসে মিলাপ',
'mostcategories'          => 'বপতা বিষয়থাকর পাতাহানি',
'mostimages'              => 'ফাইলগিত য়্যামসে মিলাপ',
'mostrevisions'           => 'রিভিসন বপসে পাতাহানি',
'prefixindex'             => 'প্রিফিক্স সুচি',
'shortpages'              => 'হুরু পাতাহানি',
'longpages'               => 'ডাঙর পাতাহানি',
'deadendpages'            => 'যে পাতাহানিত্ত কোন মিলাপ নেই',
'protectedpages'          => 'লুকরিসি পাতাহানি',
'listusers'               => 'আতাকুরার লাতংগ',
'newpages'                => 'নুৱা পাতাহানি',
'ancientpages'            => 'পুরানা পাতাহানি',
'move'                    => 'থেইকরানি',
'movethispage'            => 'পাতা এহান থেইকর',

# Book sources
'booksources' => 'লেরিকর উৎসহান',

# Special:Log
'specialloguserlabel'  => 'আতাকুরাগ:',
'speciallogtitlelabel' => 'চিঙনাঙ:',
'log'                  => 'লগ',
'all-logs-page'        => 'হাব্বি লগ',

# Special:AllPages
'allpages'       => 'হাবি পাতাহানি',
'alphaindexline' => '$1 ত $2',
'nextpage'       => 'থাঙনার পাতা ($1)',
'prevpage'       => 'আগেকার পাতা ($1)',
'allpagesfrom'   => 'যেহাত্ত অকরিসি অহাত্ত পাতাহানি দেহাদেঃ',
'allarticles'    => 'নিবন্ধহাবি',
'allinnamespace' => 'পাতাহানি হাবি ($1 নাঙরজাগা)',
'allpagesprev'   => 'আলথকে',
'allpagesnext'   => 'থাঙনাত',
'allpagessubmit' => 'হাত',
'allpagesprefix' => 'মেয়েক এগন অকরিসি ৱাহির পাতাহানি দেহাদেঃ',

# Special:Categories
'categories'         => 'বিষয়রথাকহানি',
'categoriespagetext' => 'ইমারঠারর উইকিপিডিয়াত এবাকার বিষয়রথাক:',

# E-mail user
'emailuser' => 'আতাকুরাগরে ইমেইল কর',

# Watchlist
'watchlist'            => 'মর তালাবি',
'mywatchlist'          => 'মর তালাবি',
'watchlistfor'         => "('''$1'''-র কা)",
'addedwatch'           => 'তালাবির তালিকাহাত থনা ইল',
'addedwatchtext'       => "\"<nowiki>\$1</nowiki>\" পাতা এহান তর [[Special:Watchlist|আহির-আরুম তালিকা]]-ত তিলকরানি ইল। পিসেদে এরে পাতা এহান বারো পাতা এহানর লগে সাকেই আসে য়্যারী পাতাত অইতই হারি জাতর পতানি এহানাত তিলকরানি অইতই। অতাবাদেউ [[Special:RecentChanges|হাদি এহানর পতানিহানি]]-ত পাতা এহানরে '''গাঢ়করা''' মেয়েকে দেহা দেনা অইতই যাতে তি নুঙিকরে পাতা এহান চিনে পারবেতা।

পিসেদে তি পাতা এহানরে থেইকরানি মনেইলে \"আহির-আরুমেত্ত থেইকরেদে\" ট্যাবগত ক্লিক করিস৷",
'removedwatch'         => 'তালাবির পাতাত্ত গুসাদে',
'removedwatchtext'     => 'এরে পাতা "[[:$1]]" এহান গুসানি ইলতা [[Special:Watchlist|তর তালাবির]] পাতাত্ত।',
'watch'                => 'তালাবি',
'watchthispage'        => 'পাতাএহান খিয়ালে থ',
'unwatch'              => 'তালাবি নেই',
'unwatchthispage'      => 'তালাবি এরাদেনা',
'watchlist-details'    => '{{PLURAL:$1|$1 পাতা|$1 পাতাহানি}} চানাঅসিল অতার কোন য়্যারির পাতা নেই।',
'wlshowlast'           => 'গেলগা $1 ঘন্টা $2 দিনর $3 দেখাদে',
'watchlist-hide-bots'  => 'বোটর পতানি থেইকর',
'watchlist-hide-own'   => 'মি পতাসুতা গুর',
'watchlist-hide-minor' => 'হুরকা পতানি থেইকর',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'চা...',
'unwatching' => 'নাউচা...',

'changed' => 'পতেসে',

# Delete/protect/revert
'deletepage'                  => 'পাতাহান পুস',
'confirm'                     => 'লেপকরানি',
'historywarning'              => 'সিঙুইস: তি যে পাতাহান পুসানিত লেপুইসত এহানর ইতিহাস আহান আসে:',
'confirmdeletetext'           => 'তি যে পাতাহান পুসানি লেপুইসত অহানর লগে ইতিহাসহানউ পুসতই।
তি লেপকর যে তি এহান করতেই বুলিয়া, বারো তি এহানর পিসহান হারপাসত লগে [[{{MediaWiki:Policy-url}}|পলিসিহান]] ইলয়া তি কামএহান করানিত লেপুইসত।',
'actioncomplete'              => 'কামহান লমিল।',
'deletedtext'                 => '"<nowiki>$1</nowiki>" পুসানি অইল।
চা $2 এহার বারে আগে আসে পুসানির লাতংগ।',
'deletedarticle'              => 'পুসানিইল "[[$1]]"',
'dellogpage'                  => 'পুসিসিতার লাতংগ',
'deletecomment'               => 'পুসানির কারনহান:',
'deleteotherreason'           => 'আরাক/উপরি কারন:',
'deletereasonotherlist'       => 'আর আর কারন',
'rollbacklink'                => 'রোলবেক',
'cantrollback'                => 'আগেকার সঙস্করনহাত আলথকে যানা নুৱারলু, লমিলগা সম্পদনাকরেকুরা অগ পাতা অহানর আকখুলা লেখকগ।',
'protectlogpage'              => 'লুকরানির লগ',
'protectcomment'              => 'মতহান:',
'protectexpiry'               => 'মিয়াদহান লালর:',
'protect_expiry_invalid'      => 'খেন্তাম লিতনাহান লালুইসে।',
'protect_expiry_old'          => 'বাতিলর খেন্তামহান আগেকার তারিখে পরিসে।',
'protect-unchain'             => 'গুসানি পারানির য়্যাথাঙ মুকা',
'protect-text'                => 'তি চেইলে <strong><nowiki>$1</nowiki> পাতাহানর লুকরানির মাত্রাহান চানা বারো সিলকরানি পারর</strong>।',
'protect-locked-access'       => 'তরতা পাতা লুকরে পারানির মত য়্যাথাঙ নেই।
পাতাহান <strong>$1</strong>র এপাগার পাজালানিহান:',
'protect-cascadeon'           => 'এরে পাতাহান এপাগা লুকরানি অসে, কারণ পাতাহানর তলে {{PLURAL:$1|পাতা আহানাত|পাতা হানিত}} অন্তর্ভুক্ত ইসে, যেহানাত আগপাতাকরেকুরাতাত লুকরানিহান আসে। তি চেইলে অহান সিলকরে পারর, তবে এরে আগপাতাকরেকুরাতাত কোন বদালা নাইব।',
'protect-default'             => '(ডিফল্ট)',
'protect-fallback'            => 'য়্যাথাং "$1" দরকার',
'protect-level-autoconfirmed' => 'রেজিষ্টার নাকরিসি আতাকুরারের থেপকর',
'protect-level-sysop'         => 'হুদ্দা ডান্ডিকরেকুরা',
'protect-summary-cascade'     => 'আগপাতাকরেকুরা',
'protect-expiring'            => '$1 (আমাস) খেন্তামে মিয়াদহান লালুইতই',
'protect-cascade'             => 'এরে পাতাত মিহিসে পাতাহানি তালাবি করানি অক (আগপাতাকরেকুরা তালাবি)',
'protect-cantedit'            => 'লুকরিসি পাতাহানরে তি সিলকরে নারবে, কিদিয়া বুল্লে তরতা পতানির য়্যাথাঙ নেই।',
'restriction-type'            => 'য়্যাথাঙ:',
'restriction-level'           => 'লুকরানির থাক:',

# Restrictions (nouns)
'restriction-edit' => 'পতানিহান_চিয়ৌকর',

# Undelete
'undeletebtn' => 'বারোইতুকর',

# Namespace form on various pages
'namespace'      => 'নাঙরথাক:',
'invert'         => 'বাসিসি এহান আলকর',
'blanknamespace' => '(গুরি)',

# Contributions
'contributions' => 'আতাকুরার অবদান',
'mycontris'     => 'মর অবদান',
'contribsub2'   => '$1 ($2)-র কা',
'uctop'         => '(গজ)',
'month'         => 'মাহাহানাত্ত (বারো অতার আগেত্ত):',
'year'          => 'বসরেত্ত (বারো অতার আগেত্ত):',

'sp-contributions-newbies-sub' => 'নুৱা একাউন্টর কা',
'sp-contributions-blocklog'    => 'থেপকরিসি লগ',

# What links here
'whatlinkshere'       => 'যে পাতাহানিত্ত এহানাত মিলাপ আসে',
'whatlinkshere-title' => 'পাতাহানি $1 -ত মিলাপ আসে',
'linklistsub'         => '(মিলাপর লাতংগ)',
'linkshere'           => "থাঙনার পাতাহানি '''[[:$1]]'''র লগে মিলাপ আসে:",
'nolinkshere'         => "পাতা '''[[:$1]]'''হানাত কোন মিলাপ নেই।",
'isredirect'          => 'বুলনদের পাতা',
'istemplate'          => 'বরানি',
'whatlinkshere-prev'  => '{{PLURAL:$1|পিসেদে|পিসেদে $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|থাংনা|থাংনা $1}}',
'whatlinkshere-links' => '← মিলাপহানি',

# Block/unblock
'blockip'            => 'আতাকুরাগরে থেপকর',
'ipboptions'         => '২ ঘন্টা:2 hours,১ দিন:1 day,৩ দিন:3 days,হাপ্তা আহান:1 week,হাপ্তা দুহান:2 weeks,মাহা আহান:1 month,৩ মাহা:3 months,৬ মাহা:6 months,বসর আহান:1 year,লম নেই সময়:infinite', # display1:time1,display2:time2,...
'badipaddress'       => 'আইপি ঠিকানাহান গ্রহনযোগ্যনাইসে',
'blockipsuccesssub'  => 'থেপকরানিহান চুমিল',
'blockipsuccesstext' => '[[Special:Contributions/$1|$1]] রে থেপকরিয়া থসি <br />থেপকরানিহান খাল করানি থকিলে,[[Special:IPBlockList| থেপকরিয়া থসি আইপি ঠিকানার তালিকাহান]] চা।',
'ipblocklist'        => 'থেপকরিয়া থসি আইপি ঠিকানা বারো আতাকুরার লাতঙগি',
'blocklistline'      => '$1 তারিখে $2, $3 ($4) রে থেপকরানি অসে।',
'blocklink'          => 'থেপ কর',
'unblocklink'        => 'ব্লকনাকরি',
'contribslink'       => 'অবদান',
'blocklogpage'       => 'থেপকরানির log',
'blocklogentry'      => '"[[$1]]"-রে $2 মেয়াদর কা থেপকরানি অসে। $3',

# Move page
'movepagetext'            => "তলর ফর্মহান ব্যবহার করিয়া পাতা আহানর চিঙনাঙ সিলকরানি একরতই, বারো লগে অহানর নুৱি চিঙনাঙ বারো ইতিহাসহান থেইকরানি একরতই।
পুরনা চিঙনাঙ অহান নুৱা চিঙনাঙে যানার পথগ বাগেইতই।
পুরনা চিঙনাঙর প্রতি মিলাপ অতাত কোন পতানি নাইব; অহান থকিয়া দ্বিমাউকার আলথকে যানার পাতা নাচলের আলথকে দিয়াপেঠার মিলাপ পরীক্ষা করানিত নাপাহুরিস।
মিলাপ অহানি আয়ৌপা যাগাত থুঙকগা, অহান লেপকরানির দায়িত্বহান তরহান।

খিয়াল কর যে যদি নুৱা চিঙনাঙ অহান্ন আগেত্তর পাতা আসেতানা কিতা, থা থাইলে  নুৱা পাতা এহান অহানাত '''না'''যিবগা, যদি না পাতা অহান খালি থার বা আলথকর নিদের্শনা আসে বারো আগেকার পতাসি ইতিহাস না থার। অর্থাৎ তি হারনাপেয়া নাঙ সিলকরিয়া থার সহজেই পুরানা নাঙহাত আলুইয়া যানা পারতেই, কিন্তু আগেত্তর আসে পাতার গজে ইকরানি নুৱারতেই।

'''সিঙুইস!'''
মানুর প্রিয় পাতার বারে এসাদে সিলনা খাঙদা ইতে পারে; মুঙেদে আগ বারানির আগে কামহার ফলহান কিহান ইতই, অহান লেপুইয়া করানি থক।",
'movepagetalktext'        => "পাতাহান গুসানির লগে লগে অহানর য়্যারির পাতাহানউ আপ্পানে যিতইগা '''যদি না:'''
*খালি নাইসে এসাদে য়্যারির পাতা নুৱা চিঙনাঙর তলে আগত্তর থা থাইলে, নাইলে
*তি তলর বাক্সগত্ত টিক চিনৎহান থেইকরে পারর।

এতার বারে তি চেইলে নিজর আতহানল পাতা অহান গুসানি বা পুলকরানি পারর।",
'movearticle'             => 'পাতাহান থেইকর:',
'newtitle'                => 'নুৱা চিঙনার কা:',
'move-watch'              => 'পাতা এহান খিয়াল কর',
'movepagebtn'             => 'পাতা থেইকর',
'pagemovedsub'            => 'গুসানিহান হবা বালাই লমিল',
'movepage-moved'          => '<big>\'\'\'"$1" থেইককরানি ইল "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'ইতে পারে এরে শিরোনাঙর নিবন্ধহান হঙপরসেগা, নাইলে তি দিয়াসত শিরোনাং এহান দেনার য়্যাথাং নেই। কৃপা করিয়া আরাক শিরোনাং আহান দেনার হৎনা কর।',
'talkexists'              => "'''পাতাহান হবা বালাই গুসিল কিন্তু অরে নাঙর য়্যারির পাতা আহান আগেত্তর থানাই না গুসিল।
দয়া করিয়া তি নিজর আতহান্ন তিলকরগা।'''",
'movedto'                 => 'থেইকর',
'movetalk'                => 'লগর য়্যারির পাতাহান গুসা',
'1movedto2'               => '[[$1]]-রে [[$2]]-ত গুসানি ইল',
'1movedto2_redir'         => '[[$1]]-রে [[$2]]-ত বারো-র্নির্দেশনার মা থেইকরানি ইল',
'movelogpage'             => 'লগ গুসা',
'movereason'              => 'কারন:',
'revertmove'              => 'রিভার্ট',
'delete_and_move'         => 'পুসানি বারো থেইকরানি',
'delete_and_move_confirm' => 'হায়, পাতা এহান পুস',

# Export
'export' => 'পাতাহান দিয়াপেঠা',

# Namespace 8 related
'allmessages'         => 'সিস্টেমর পৌহানি',
'allmessagesname'     => 'নাং',
'allmessagescurrent'  => 'হাদি এহানর ৱাহি',
'allmessagestext'     => 'তলে মিডিয়াউইকি: নাঙরজাগাত পানা একরের সিস্টেম পৌহানির তালিকাহান দেনা ইল।',
'allmessagesmodified' => 'পতাসি অতা হুদ্দা দেহাদে',

# Thumbnails
'thumbnail-more'  => 'ডাঙরকর',
'thumbnail_error' => 'থাম্বনেইল হংকরানিত লেইলেক অসে: $1',

# Import log
'importlogpage' => 'লগ আন',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'মরকা পাতাহান',
'tooltip-pt-mytalk'               => 'মর য়্যারির পাতা',
'tooltip-pt-preferences'          => 'মর পছন',
'tooltip-pt-watchlist'            => 'পতাসি পাতার হিসাব আসে লাতংগ',
'tooltip-pt-mycontris'            => 'মর তালাবির লাতংগ',
'tooltip-pt-login'                => 'লগ করানির হেইচা কররাঙ, যদিউ অহান বাধ্যতামুলকহান নাবে।',
'tooltip-pt-logout'               => 'নিকুলানি',
'tooltip-ca-talk'                 => 'পাতাহানর বারে য়্যারি দে',
'tooltip-ca-edit'                 => 'তি পাতা এহান পতা পারতেই।
পাতাহান ইতুকরানির আগে আলথকে মিল্লেং আহান দে।',
'tooltip-ca-addsection'           => 'য়্যারী এহাত তর মতহান তিলকর।',
'tooltip-ca-viewsource'           => 'পাতা এহান লুকরানি অসে।
তি হুদ্দা উত্স চা পারতেই।',
'tooltip-ca-protect'              => 'পাতাএহান লুকর',
'tooltip-ca-delete'               => 'পাতা এহান পুস',
'tooltip-ca-move'                 => 'পাতা এহান থেইকর',
'tooltip-ca-watch'                => 'পাতা এহান চাফামে থ',
'tooltip-ca-unwatch'              => 'তর মিল্লেঙর লাতঙেত্ত পাতা এহান গুসা',
'tooltip-search'                  => 'বিসারা {{SITENAME}}',
'tooltip-p-logo'                  => 'পয়লা পাতা',
'tooltip-n-mainpage'              => 'মুল পাতাহান চেইক',
'tooltip-n-portal'                => 'প্রকল্প এহানর বারে, তি কিহান পাংকরে পারতেই, বস্তু কুরাঙত বিসারিয়া পানা',
'tooltip-n-currentevents'         => 'এপাগার ইভেন্টহানরকা পিসেদের ইতাহানহান বিসারা',
'tooltip-n-recentchanges'         => 'উইকি এহাত হাদি এহান পতাসি পাতার লাতংগ।',
'tooltip-n-randompage'            => 'খাঙদা পাতা আহান লোড কর',
'tooltip-n-help'                  => 'বিসরিয়া পানার ফামহান।',
'tooltip-t-whatlinkshere'         => 'উইকির যে পাতাহানি এহানাত মিলাপ অসে অতার লাতংগ',
'tooltip-t-contributions'         => 'পাতাএহান অবদান থসি অতার লাতংগ চা',
'tooltip-t-emailuser'             => 'আতাকুরা এগরে ইমেল আহান কর',
'tooltip-t-upload'                => 'ফাইল আপলোড কর',
'tooltip-t-specialpages'          => 'বিশেষ পাতাহানির লাতংগ',
'tooltip-ca-nstab-user'           => 'আতাকুরার পাতাহান চা',
'tooltip-ca-nstab-project'        => 'প্রকল্প পাতাহান চা',
'tooltip-ca-nstab-image'          => 'ফাইলর পাতাহান চা',
'tooltip-ca-nstab-template'       => 'মডেলহান চা',
'tooltip-ca-nstab-help'           => 'পাঙলাকর পাতা চা',
'tooltip-ca-nstab-category'       => 'থাকর পাতাহানি চা',
'tooltip-minoredit'               => 'এহান হুরু পতানিহান বুলিয়া চিনতদে',
'tooltip-save'                    => 'তর পতানিহান ইতু কর',
'tooltip-preview'                 => 'তি ইতু করানির আগে যেসাদেউ আগচা (প্রিভিউ) দে!',
'tooltip-diff'                    => 'তি কিসারেতা কিসারেতা পতাসত অতা চা।',
'tooltip-compareselectedversions' => 'এরে পাতা এহানর দুহান ভার্সনর তুলনা কর।',
'tooltip-watch'                   => 'পাতা এহান তর মিল্লেঙে থ',

# Attribution
'anonymous' => '{{SITENAME}}র বেনাঙর আতাকুরা(গি)',

# Browsing diffs
'previousdiff' => '← পিসেদের ফারাক',
'nextdiff'     => 'থাংনার ফারাক →',

# Media information
'file-info-size'       => '($1 × $2 পিক্সেল, ফাইলর সাইজহান: $3, এমআইএমই-র অংতা: $4)',
'file-nohires'         => '<small>এহাত্ত গজর রিজরিউশন নেই।</small>',
'svg-long-desc'        => '(SVG ফাইল, সাধারনত $1 × $2 পিক্সেল, ফাইলর সাইজহান: $3)',
'show-big-image'       => 'পুল্লাপ রিজলিউশন',
'show-big-image-thumb' => '<small>আগচা হানর সাইজহান: $1 × $2 পিক্সেলস</small>',

# Special:NewImages
'newimages' => 'নুৱা ফাইলর গ্যালারিগ',
'ilsubmit'  => 'বিসারা',
'bydate'    => 'তারিখর সিজিলন',

# Bad image list
'bad_image_list' => 'ফরমেটহান তলর সাদে:

লিস্টর বস্তু হুদ্দা (লাইনহান অকরতইতাই *) বিবেচিত অইতই।
পইলাকার লাইনহান যেসাদেই হবানেই ফাইলর মিলাপ করতই।
লাইন অহানর ভিতরে আর আর মিলাপ অতা ব্যতিক্রম বুলিয়া ধরানি অইতই, যেসাদে: যেহাত পাতাহানির ফইলগ লাইনহানর মা মাতানি অইতই।',

# Metadata
'metadata'          => 'মেটারপৌ',
'metadata-help'     => 'ফাইল এগত আরাকউ হেলপা পৌ খানি তিলুইসে, মনে অরতা ডিজিটাল ক্যামেরাগত্ত নাইলে স্ক্যানারহাত্ত হমাসে। যদি ফাইল এগ মুল অংতাত্ত পতিয়া থার অতা ইলে খানি মানি পৌ না তিলুতে পারে।',
'metadata-expand'   => 'আরাকউ সালকরিসি পৌ চা',
'metadata-collapse' => 'সালকরিসি পৌ ঝিপা',
'metadata-fields'   => 'এরে পৌ এহান তিলসে EXIF মেটাপৌ অতা ছবির পাতাত দেখাদেনা ইতই, যেপাগা হেলপা উপাত্ত সারণি অতা জিপানি ইতই। হের ক্ষেত্রহানি স্বাভাবিক অবস্থাত জিপিয়া থাইতই।
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# External editor support
'edit-externally'      => 'এর ফাইল এগ পতানির কা বারেদের এপ্লিকেশন আতা',
'edit-externally-help' => 'আরাকউ হারপানির কা [http://www.mediawiki.org/wiki/Manual:External_editors সেটাপর বারে পৌ] হানি চা।',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'হাব্বি',
'namespacesall' => 'হাব্বি',
'monthsall'     => 'হাব্বি',

# E-mail address confirmation
'confirmemail'            => 'ই-মেইল ঠিকানাহান লেপকর',
'confirmemail_send'       => 'লেপকরেকুরা কোডগ দিয়াপেঠাদে',
'confirmemail_sent'       => 'লেপকরেকুরা ই-মেইলহান দিয়াপেঠা দিলাং।',
'confirmemail_sendfailed' => 'লেপকরেকুরা ই-মেইলহান দিয়াপেঠাদে নুৱাররাং। ইমেইল ঠিকানাহান চুমকরে ইকরিসত্তানাকিতা আরাক আকমু খিয়াল করিয়া চা। আলথকে আহিলঃ $1',
'confirmemail_invalid'    => 'লেপকরেকুরা কোডগ চুম নাইসে। সম্ভবতঃ এগ পুরানা ইয়া পরসেগা।',
'confirmemail_success'    => 'তর ই-মেইল ঠিকানাহার লেপ্পাহান চুমিল। তি এবাকা হমানি(log in) পারর।',
'confirmemail_loggedin'   => 'তর ই-মেইল ঠিকানাহার লেপকরানিহান চুমিল।',

# action=purge
'confirm_purge'        => 'পাতা এহানর ক্যাশহান ঙক্ষি করানি মনারতা? 

$1',
'confirm_purge_button' => 'চুমিসে',

# AJAX search
'articletitles' => "যে পাতাহানি ''$1'' ন অকরাগ, অতার তালিকা",

# Auto-summaries
'autoredircomment' => '[[$1]]-ত যানার বারো-র্নিদেশ করানি ইল',

# Watchlist editing tools
'watchlisttools-view' => 'মিল আসে পতা চা',
'watchlisttools-edit' => 'তর তালাবির পাতা চা বারো পতা',
'watchlisttools-raw'  => 'পেরকা তালাবির পাতা পতা',

# Special:Version
'version' => 'সংস্করন', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'বিশেষ পাতাহানি',

);
