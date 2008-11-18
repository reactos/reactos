<?php
/** Southern Balochi (بلوچی مکرانی)
 *
 * @ingroup Language
 * @file
 *
 * @author Mostafadaneshvar
 */

$fallback = 'fa';

$skinNames = array(
	'standard'    => 'کلاسیک',
	'nostalgia'   => 'نوستالجیک',
	'cologneblue' => 'نیلی کولاجن',
	'monobook'    => 'منوبوک',
	'myskin'      => 'منی جلد',
	'chick'       => 'شیک',
	'simple'      => 'ساده',
	'modern'      => 'مدرن',
);

$specialPageAliases = array(
	'DoubleRedirects'         => array( 'دوبل غیر مستقیم' ),
	'BrokenRedirects'         => array( 'پرشتگین غیرمستقیم' ),
	'Disambiguations'         => array( 'رفع ابهام' ),
	'Userlogin'               => array( 'ورودکاربر' ),
	'Userlogout'              => array( 'دربیگ کاربر' ),
	'CreateAccount'           => array( 'شرکتن حساب' ),
	'Preferences'             => array( 'ترجیحات' ),
	'Watchlist'               => array( 'لیست چارگ' ),
	'Recentchanges'           => array( 'نوکین تغییرات' ),
	'Upload'                  => array( 'آپلود' ),
	'Imagelist'               => array( 'لیست عکس' ),
	'Newimages'               => array( 'نوکین عکسان' ),
	'Listusers'               => array( 'لیست کاربر' ),
	'Listgrouprights'         => array( 'لیست حقوق گروه' ),
	'Statistics'              => array( 'آمار' ),
	'Randompage'              => array( 'صفحه تصادفی' ),
	'Lonelypages'             => array( 'صفحات یتیم' ),
	'Uncategorizedpages'      => array( 'صفحات بی دسته' ),
	'Uncategorizedcategories' => array( 'دستجات بی دسته' ),
	'Uncategorizedimages'     => array( 'عکسان بی دسته' ),
	'Uncategorizedtemplates'  => array( 'تمپلتان بی دسته' ),
	'Unusedcategories'        => array( 'بی استفاده این دسته' ),
	'Unusedimages'            => array( 'بی استفاده این عکس' ),
	'Wantedpages'             => array( 'لوٹتگین صفحات' ),
	'Wantedcategories'        => array( 'لوٹتگین دسته' ),
	'Missingfiles'            => array( 'گارین فایل' ),
	'Mostlinked'              => array( 'گیشتر لینک بوتت' ),
	'Mostlinkedcategories'    => array( 'دستجات گیشتر لینک بوتگین' ),
	'Mostlinkedtemplates'     => array( 'تمپلتان گیشتر لینک بوتگین' ),
	'Mostcategories'          => array( 'گیشترین دستجات' ),
	'Mostimages'              => array( 'گیشترین عکس' ),
	'Mostrevisions'           => array( 'گیشترین بازبینی' ),
	'Fewestrevisions'         => array( 'کمترین بازبینی' ),
	'Shortpages'              => array( 'هوردین صفحات' ),
	'Longpages'               => array( 'مزنین صفحات' ),
	'Newpages'                => array( 'نوکین صفحات' ),
	'Ancientpages'            => array( 'صفحات قدیمی' ),
	'Deadendpages'            => array( 'مرتگین صفحات' ),
	'Protectedpages'          => array( 'صفحات محافظتی' ),
	'Protectedtitles'         => array( 'عناوین محافظتی' ),
	'Allpages'                => array( 'کل صفحات' ),
	'Prefixindex'             => array( 'ایندکس پیشوند' ),
	'Ipblocklist'             => array( 'لیست محدوددیت آی پی' ),
	'Specialpages'            => array( 'حاصین صفحات' ),
	'Contributions'           => array( 'مشارکتان' ),
	'Emailuser'               => array( 'ایمیل کاربر' ),
	'Confirmemail'            => array( 'تایید ایمیل' ),
	'Whatlinkshere'           => array( 'ای لینکی ادان هست' ),
	'Recentchangeslinked'     => array( 'نوکین تغییرات لینک' ),
	'Movepage'                => array( 'جاه په جاهی صفحه' ),
	'Blockme'                 => array( 'محدودیت من' ),
	'Booksources'             => array( 'منابع کتاب' ),
	'Categories'              => array( 'دستجات' ),
	'Export'                  => array( 'درگیزگ' ),
	'Version'                 => array( 'نسخه' ),
	'Allmessages'             => array( 'کل کوله یان' ),
	'Log'                     => array( 'ورودان' ),
	'Blockip'                 => array( 'محدود آی پی' ),
	'Undelete'                => array( 'حذف نکتن' ),
	'Import'                  => array( 'وارد' ),
	'Lockdb'                  => array( 'کبلدب' ),
	'Unlockdb'                => array( 'کلب نه کتن دب' ),
	'Userrights'              => array( 'حقوق کاربر' ),
	'MIMEsearch'              => array( 'گردگ میام' ),
	'FileDuplicateSearch'     => array( 'گردگ کپی فایل' ),
	'Unwatchedpages'          => array( 'نه چارتگین صفحه' ),
	'Listredirects'           => array( 'لیست غیر مستقیمان' ),
	'Revisiondelete'          => array( 'حذف بازبینی' ),
	'Unusedtemplates'         => array( 'تمپلتان بی استفاده' ),
	'Randomredirect'          => array( 'غیرمستقیم تصادفی' ),
	'Mypage'                  => array( 'منی صفحه' ),
	'Mytalk'                  => array( 'منی گپ' ),
	'Mycontributions'         => array( 'منی مشارکت' ),
	'Listadmins'              => array( 'لیست مدیران' ),
	'Listbots'                => array( 'لیست روباتان' ),
	'Popularpages'            => array( 'مردمی صفحات' ),
	'Search'                  => array( 'گردگ' ),
	'Resetpass'               => array( 'تریتگ رمز' ),
	'Withoutinterwiki'        => array( 'بی بین ویکی' ),
	'MergeHistory'            => array( 'چندوبند تاریح' ),
	'Filepath'                => array( 'مسیر فایل' ),
	'Invalidateemail'         => array( 'نامعتبرین ایمیل' ),
	'Blankpage'               => array( 'صفحه هالیک' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => ':لینکانآ خط کش',
'tog-highlightbroken'         => 'پرشتگین لینکانآ فرمت کن <a href="" class="new">په داب شی</a> (یا: پی ای داب<a href="" class="internal">?</a>).',
'tog-justify'                 => 'پاراگرافنآ همتراز کن',
'tog-hideminor'               => 'هوردین تغییراتآ ته نوکین تغییرات پناه کن',
'tog-extendwatchlist'         => 'لیست چارگ مزن کن دان کل تغییرات قابل قبول پیش دراگ بیت',
'tog-usenewrc'                => 'تغییرات نوکین بهتر بوتگین(جاوا اسکریپت)',
'tog-numberheadings'          => 'اتوماتیک شماره کتن عناوین',
'tog-showtoolbar'             => 'میله ابزار اصلاح پیش درا(جاوا)',
'tog-editondblclick'          => 'صفحات گون دو کلیک اصلاح کن(جاوا)',
'tog-editsection'             => 'فعال کتن کسمت اصلاح از طریق لینکان  [edit]',
'tog-editsectiononrightclick' => 'فعال کتن اصلاح کسمت گون کلیک راست اور کسمت عناوین(جاوا)',
'tog-showtoc'                 => 'جدول محتوای‌ء پیش دار( په صفحیانی که گیش چه 3 عنوانش هست)',
'tog-rememberpassword'        => 'منی وارد بیگ ته ای کامپیوتر هیال کن',
'tog-editwidth'               => 'جعبه اصلاح کل پهنات هست',
'tog-watchcreations'          => 'هور کن منی صفحاتی که من ته لیست چارگ شرکتت',
'tog-watchdefault'            => 'هورکن صفحاتی که من اصلاح کتن ته منی لیست چارگ',
'tog-watchmoves'              => 'هور کن صفحاتی که من جاه په جاه کت ته منی لیست چارگ',
'tog-watchdeletion'           => 'هور کن صفحاتی که من ته لیست چارگ که من حذف کتن',
'tog-minordefault'            => 'په طور پیش فرض کل اصلاحات آ په داب جزی مشخص کن',
'tog-previewontop'            => 'بازبین پیش دار پیش چه جعبه اصلاح',
'tog-previewonfirst'          => 'ته اولین اصلاح بازبینی پیش دار',
'tog-nocache'                 => 'ذخیره کتن صفحه یا غیر فعال کن',
'tog-enotifwatchlistpages'    => 'منی ایمیل جن وهدی که یک صفحه ای ته منی لیست چارگ عوص بیت',
'tog-enotifusertalkpages'     => 'منآ ایمیل جن وهدی که صفحه ی گپ کاربر من عوض بیت',
'tog-enotifminoredits'        => 'من ایمیل جن همی داب په هوردین اصلاحات صفحات',
'tog-enotifrevealaddr'        => 'منی ایمیل پیش دار ته ایمیل أن هوژاری',
'tog-shownumberswatching'     => 'پیش دار تعداد کاربرانی که چارگتن',
'tog-fancysig'                => 'حامین امضا يان(بی اتوماتیکی لینک)',
'tog-externaleditor'          => 'به طور پیش فرض اصلاح کنوک حارجی استفاده کن',
'tog-externaldiff'            => 'به طور پیش فرض چه حارجی تمایز استفاده کن',
'tog-showjumplinks'           => 'فعال کن "jump to" لینکان دست رسی آ',
'tog-uselivepreview'          => 'چه زنده این بازبین استفاده کن(جاوا)(تجربی)',
'tog-forceeditsummary'        => 'من آ هال دی وهدی وارد کتن یک هالیکین خلاصه ی اصلاح',
'tog-watchlisthideown'        => 'منی اصلاحات آ چه لیست چارگ پناه کن',
'tog-watchlisthidebots'       => 'اصلاحات بوت چه لیست چارگ پناه کن',
'tog-watchlisthideminor'      => 'هوردین اصلاحات چه لیست چارگ پناه کن',
'tog-nolangconversion'        => 'غیر فعال کتن بدل کتن مغایرت آن',
'tog-ccmeonemails'            => 'په من یک کپی چه ایمیل آنی که من په دگه کاربران راه داته دیم دی',
'tog-diffonly'                => 'چیر تفاوت محتوای صفحه ی پیش مدار',
'tog-showhiddencats'          => 'پناه ین دسته یان پیش دار',

'underline-always'  => 'یکسره',
'underline-never'   => 'هچ وهد',
'underline-default' => 'پیشفرضین بروزر',

'skinpreview' => '(بازبینی)',

# Dates
'sunday'        => 'یک شنبه',
'monday'        => 'دوشنبه',
'tuesday'       => 'سی شنبه',
'wednesday'     => 'چارشنبه',
'thursday'      => 'پنچ شنبه',
'friday'        => 'آدینگ',
'saturday'      => 'شنبه',
'sun'           => 'ی.شنبه',
'mon'           => 'د.شنبه',
'tue'           => 'س.شنبه',
'wed'           => 'چ.شنبه',
'thu'           => 'پ.شنبه',
'fri'           => 'آدینگ',
'sat'           => 'شنبه',
'january'       => 'ژانویه',
'february'      => 'فوریه',
'march'         => 'مارس',
'april'         => 'آپریل',
'may_long'      => 'می',
'june'          => 'جون',
'july'          => 'جولای',
'august'        => 'آگوست',
'september'     => 'سپتامبر',
'october'       => 'اکتبر',
'november'      => 'نومبر',
'december'      => 'دسمبر',
'january-gen'   => 'ژانویه',
'february-gen'  => 'فوریه',
'march-gen'     => 'مارس',
'april-gen'     => 'آپریل',
'may-gen'       => 'می',
'june-gen'      => 'جون',
'july-gen'      => 'جولای',
'august-gen'    => 'آگوست',
'september-gen' => 'سپتمبر',
'october-gen'   => 'اکتبر',
'november-gen'  => 'نومبر',
'december-gen'  => 'دسمبر',
'jan'           => 'جن',
'feb'           => 'فب',
'mar'           => 'ما',
'apr'           => 'آپر',
'may'           => 'می',
'jun'           => 'جون',
'jul'           => 'جول',
'aug'           => 'آگ',
'sep'           => 'سپت',
'oct'           => 'اکت',
'nov'           => 'نو',
'dec'           => 'دس',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|دسته|دسته جات}}',
'category_header'                => 'صفحات ته دسته "$1"',
'subcategories'                  => 'زیردسته جات',
'category-media-header'          => 'مدیا ته دسته "$1"',
'category-empty'                 => "''ای دسته ی هچ صفحه یا مدیا نیست''",
'hidden-categories'              => '{{PLURAL:$1|پناهین دسته|پناهین دسته جات}}',
'hidden-category-category'       => 'پناهین دسته جات', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|ای دسته فقط جهلیگین زیر دسته ای هست..|ای دسته  جهلیگین {{PLURAL:$1|subcategory|$1 زیردسته}}, چه $2 کل.}}',
'category-subcat-count-limited'  => 'ای دسته جهلیگی  {{PLURAL:$1|زیردسته|$1 زیر دسته جات}}.',
'category-article-count'         => '{{PLURAL:$2|ای دسته فقط شامل جهلیگین صفحه انت.|جهلیگین {{PLURAL:$1|صفحه است|$1 صفحات انت}}ته ای دسته , چه $2 کل.}}',
'category-article-count-limited' => 'جهلیگین  {{PLURAL:$1|صفحه هست|$1 صفحات هستن}} تی هنوکین دسته.',
'category-file-count'            => '{{PLURAL:$2|ای دسته فقط شامل جهلیگین فایل انت.|جهلیگین {{PLURAL:$1|افایل انت|$1 فایلان انت}}ته ای دسته, چه $2کلl.}}',
'category-file-count-limited'    => 'جهلیگین {{PLURAL:$1|فایل|$1 فایلان}} ته هنوکین دسته اینت',
'listingcontinuesabbrev'         => 'ادامه.',

'mainpagetext'      => "<big>'''مدیا وی کی گون موفقیت نصب بوت.'''</big>",
'mainpagedocfooter' => "مشورت کنیت گون  [http://meta.wikimedia.org/wiki/Help:Contents User's Guide] په گشیترین اطلاعات په استفاده چه برنامه ویکی.

== شروع بیت ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Configuration settings list]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mailing list]",

'about'          => 'باره',
'article'        => 'محتوا صفحه',
'newwindow'      => '(ته نوکین پنچره ی پچ کن)',
'cancel'         => 'کنسل',
'qbfind'         => 'درگیزگ',
'qbbrowse'       => 'بروز',
'qbedit'         => 'اصلاح',
'qbpageoptions'  => 'صفحه',
'qbpageinfo'     => 'متن',
'qbmyoptions'    => 'منی صفحات',
'qbspecialpages' => 'حاصین صفحات',
'moredotdotdot'  => 'گیشتر...',
'mypage'         => 'می صفحه',
'mytalk'         => 'منی گپ',
'anontalk'       => 'گپ کن گون ای آی پی',
'navigation'     => 'گردگ',
'and'            => 'و',

# Metadata in edit box
'metadata_help' => 'متادیتا',

'errorpagetitle'    => 'حطا',
'returnto'          => 'تررگ به $1.',
'tagline'           => 'چه {{SITENAME}}',
'help'              => 'کمک',
'search'            => 'گردگ',
'searchbutton'      => 'گردگ',
'go'                => 'برو',
'searcharticle'     => 'برو',
'history'           => 'تاریح صفحه',
'history_short'     => 'تاریح',
'updatedmarker'     => 'په روچ بیتگین چه منی اهری  اهری  چارگ',
'info_short'        => 'اطلاعات',
'printableversion'  => 'نسخه چهاپی',
'permalink'         => 'دایمی لینک',
'print'             => 'چهاپ',
'edit'              => 'اصلاح',
'create'            => 'شرکتن',
'editthispage'      => 'ای صفحه اصلاح کن',
'create-this-page'  => 'ای صفحه شرکتن کن',
'delete'            => 'حذف',
'deletethispage'    => 'ای صفحه حذف کن',
'undelete_short'    => 'حذف مکن {{PLURAL:$1|one edit|$1 edits}}',
'protect'           => 'حفاظت',
'protect_change'    => 'عوض کن',
'protectthispage'   => 'ای صفحه حفاظت کن',
'unprotect'         => 'محافظت مکن',
'unprotectthispage' => 'ای صفحه محافظت مکن',
'newpage'           => 'نوکین صفحه',
'talkpage'          => 'ای صفحه بحث کن',
'talkpagelinktext'  => 'گپ کن',
'specialpage'       => 'حاصین صفحه',
'personaltools'     => 'شخصی وسایل',
'postcomment'       => 'یک نظر دیم دی',
'articlepage'       => 'محتوا صفحه به گند',
'talk'              => 'بحث',
'views'             => 'چارگان',
'toolbox'           => 'جعبه ابزار',
'userpage'          => 'به گند صفحه کاربر',
'projectpage'       => 'به گند صفحه',
'imagepage'         => 'به گند صفحه',
'mediawikipage'     => 'به گند صفحه کوله',
'templatepage'      => 'به گند صفحه تمپلت آ',
'viewhelppage'      => 'به گند صفحه کمک آ',
'categorypage'      => 'به گند صفحه دسته آ',
'viewtalkpage'      => 'به گند بحث آ',
'otherlanguages'    => 'ته دگر زبان',
'redirectedfrom'    => '(غیر مستقیم بوتگ چه $1)',
'redirectpagesub'   => 'صفحه غیر مستقیم',
'lastmodifiedat'    => '  $2, $1.ای صفحه اهری تغییر دهگ بیته', # $1 date, $2 time
'viewcount'         => 'ای صفحه دسترسی بیتگ {{PLURAL:$1|بار|$1رند}}.',
'protectedpage'     => 'صفحه محافظتی',
'jumpto'            => 'کپ به:',
'jumptonavigation'  => 'گردگ',
'jumptosearch'      => 'گردگ',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'باره {{SITENAME}}',
'aboutpage'            => 'Project:باره',
'bugreports'           => 'گزارشات باگ',
'bugreportspage'       => 'Project:گزارشات باگ',
'copyright'            => 'محتوا موجودانت تحت $1.',
'copyrightpagename'    => 'حق کپی{{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:حق کپی',
'currentevents'        => 'هنوکین رویداد',
'currentevents-url'    => 'Project:هنوکین رویداد',
'disclaimers'          => 'بی میاری گیان',
'disclaimerpage'       => 'Project:عمومی بی میاریگان',
'edithelp'             => 'کمک اصلاح',
'edithelppage'         => 'Help:اصلاح',
'faq'                  => 'ب.ج.س',
'faqpage'              => 'Project:ب.ج.س',
'helppage'             => 'Help:محتوا',
'mainpage'             => 'صفحه اصلی',
'mainpage-description' => 'صفحه اصلی',
'policy-url'           => 'Project:سیاست',
'portal'               => 'پرتال انجمن',
'portal-url'           => 'Project:پرتال انجمن',
'privacy'              => 'سیاست حفظ اسرار',
'privacypage'          => 'Project:سیاست حفظ اسرار',

'badaccess'        => 'حطا اجازت',
'badaccess-group0' => 'شما مجاز نهیت عملی که درخواست کت اجرا کنیت',
'badaccess-group1' => 'عملی که ما درخواست کتت مربوط به گروه کابران $1.',
'badaccess-group2' => 'کاری که شما درخواست کت محدود په کاربران ته یکی چه گروهان $1.',
'badaccess-groups' => 'کاری که شما درخواست کت محدود په کابران ته یکی چه گروهان $1.',

'versionrequired'     => 'نسخه $1. مدیا وی کی نیازنت',
'versionrequiredtext' => 'نسخه $1 چه مدیا وی کی نیازنت په استفاده ای صفحه. بچار [[Special:Version|version page]].',

'ok'                      => 'هوبنت',
'retrievedfrom'           => 'درگیجگ بیت چه  "$1"',
'youhavenewmessages'      => 'شما هست  $1 ($2).',
'newmessageslink'         => 'نوکین کوله یان',
'newmessagesdifflink'     => 'اهری تغییر',
'youhavenewmessagesmulti' => 'شما را نوکین کوله یان هست ته   $1',
'editsection'             => 'اصلاح',
'editold'                 => 'اصلاح',
'viewsourceold'           => 'به گند منبع ا',
'editsectionhint'         => ': $1اصلاح انتخاب',
'toc'                     => 'محتوا',
'showtoc'                 => 'پیش دار',
'hidetoc'                 => 'پناه کن',
'thisisdeleted'           => 'به گند یا پچ ترین $1?',
'viewdeleted'             => 'به گند $1?',
'restorelink'             => '{{PLURAL:$1|یک حذف اصلاح|$1 حذف اصلاح}}',
'feedlinks'               => 'منبع',
'feed-invalid'            => 'نامعتبرنوع  اشتراک منبع',
'feed-unavailable'        => 'سندیکا منبع موجود نهت',
'site-rss-feed'           => 'منبع $1 RSS',
'site-atom-feed'          => 'منبع $1 Atom',
'page-rss-feed'           => 'منبع "$1" RSS',
'page-atom-feed'          => 'منبع "$1" Atom',
'feed-atom'               => 'اتم',
'feed-rss'                => 'ار اس اس',
'red-link-title'          => '$1(هنگت نویسگ نه بیته)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'صفحه',
'nstab-user'      => 'صفحه کاربر',
'nstab-media'     => 'صفحه مدیا',
'nstab-special'   => 'حاصین',
'nstab-project'   => 'صفحه پروژه',
'nstab-image'     => 'فایل',
'nstab-mediawiki' => 'کوله',
'nstab-template'  => 'تمپلت',
'nstab-help'      => 'صفحه کمک',
'nstab-category'  => 'دسته',

# Main script and global functions
'nosuchaction'      => 'نی چشین عمل',
'nosuchactiontext'  => 'کاری که گون URL مشخص بیته گون وی کی پچاه آرگ نبیت',
'nosuchspecialpage' => 'نی چشین حاصین صفحه',
'nospecialpagetext' => "<big>'''شما یک نامعتبرین صفحه حاصین درخواست کت.'''</big>

یک لیستی چه معتبرین صفحات حاص در کپیت ته [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'حطا',
'databaseerror'        => 'حطا دیتابیس',
'dberrortext'          => 'یک اشتباه ته درخواست دیتابیس پیش آتک.
شی شاید یک باگی ته نرم افزار پیش داریت.
آهرین تلاش درخواست دیتابیس بوته:
<blockquote><tt>$1</tt></blockquote>
"<tt>$2</tt>".
ته ای عملگر ما اس کیو ال ای حطا پیش داشتت "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'یک اشتباه ته درخواست دیتابیس پیش آتک.
آهری تلاش درخواست دیتابیس بوتت:
"$1"
چه ای عملگر"$2".
مای اس کیو ال ای حطا پیش داشتت  "$3: $4"',
'noconnect'            => 'شرمنده! وی کی تکنیکی مشکلاتی هستن و نه تونیت گون دیتابیس اتصال گریت.<br />
$1',
'nodb'                 => 'نه تونیت دیتابیس انتخاب کن $1',
'cachederror'          => 'جهلیگین یک کپی ذخیره ای چه صفحه درخواستین و ممکننت نوک مبیت.',
'laggedslavemode'      => 'هوژاری: صفحه شاید نوکین په روچ بییگان داشته می بیت',
'readonly'             => 'دیتابیس کبلنت',
'enterlockreason'      => 'یک دلیلی په کبل وارد کنیت، شامل یک برآوردی چه وهد کبل ویل بیت',
'readonlytext'         => 'دیتابیس هنو کبلنت اور نوکین ورودیان و تغییرات شابد په داشتن روتین دیتابیس، بعد آی په حالت نرمال تریت.

مدیری که آیآ کبل کتت ای توضیحات داتت: $1',
'missing-article'      => 'دیتابیس نه تونیت متن یک صفحه ی که پیداگ بوتت در گیزیت په نام"$1" $2.

شی معمولای به وسیله ی یک تاریح گوشتگین تاریح یا لینک تاریح به یک صفحه که حذف بوتت پیش کیت.

اگه شی دلیل نهنت، شما شاید یک باگی ته نرم افزار در گهتگ. لطفا په مدیر گزارش دهیت،[[Special:ListUsers/sysop|administrator]]، ای URL یادداشت کینت.',
'missingarticle-rev'   => '(بازبینی#: $1)',
'missingarticle-diff'  => '(تفاوت: $1, $2)',
'readonly_lag'         => 'دیتابیس اتوماتیک کبل بیت وهدی که سرورآن دیتابیس برده مستر بیت.',
'internalerror'        => 'حطادرونی',
'internalerror_info'   => 'حطا درونی: $1',
'filecopyerror'        => ' "$1" به "$2".نه تونیت فایل کپی کنت',
'filerenameerror'      => 'نه تونیت فایل نام عوض کنت  "$1" به "$2".',
'filedeleteerror'      => 'نه تونیت فایل حذف کنت  "$1".',
'directorycreateerror' => 'نه تونیت مسیر شرکتن  "$1".',
'filenotfound'         => 'نه تونیت فایل درگیزگ "$1".',
'fileexistserror'      => 'نه تونیت فایل بنویسیت به  "$1": فایل هستنت',
'unexpected'           => 'ارزش نه لوٹتیگن : "$1"="$2".',
'formerror'            => 'حطا: نه تونیت فرم دیم دنت',
'badarticleerror'      => 'ای کار ته ای صفحه اجرای نه بیت',
'cannotdelete'         => 'نه نونیت فایل یا صفحه مشخص بیتگین آ حذف کن.
شاید گون یکی دگه  حذف بوتت',
'badtitle'             => 'عنوان بد',
'badtitletext'         => 'لوٹتگین عنوان صفحه نامعتبر ،هالیک یا یک عنوان هرابین لینک بین زبانی یا بین وی کی انت.
آی شاید شامل یک یا گیشترین کاراکترانت که ته عناوین استفاده نه بنت.',
'perfdisabled'         => 'شرمنده! ای ویژگی الان غیر فعالنت شاید شی پیش داریت که دیتابیس هاموشنت که هچ کس نه تونیت وی کی استفاده کنت.',
'perfcached'           => 'جهلیگین دیتا ذخیره بیتگنت و شاید نوک می بنت.',
'perfcachedts'         => 'جهلیگین دیتا ذخیره بیتگنت و اهرین په روچ بیگ $1.',
'querypage-no-updates' => 'په روچ بیگان په ای صفحه الان غیر فعالنت. دیتا ادان الان نوکین نهنت.',
'wrong_wfQuery_params' => 'اشتباهین پارامتر به wfQuery()<br />
Function: $1<br />
Query: $2',
'viewsource'           => 'به گند منبع آ',
'viewsourcefor'        => 'په $1',
'actionthrottled'      => 'کار گیر نت',
'actionthrottledtext'  => 'په خاطر یک معیار ضد اسپم شما چه انجام ای کار ته یک کمی زمان محدود بیتگیت، و شما چه ای محدودیت رد بیتگیت.
لطفا چند دقیقه بعد کوشست کن',
'protectedpagetext'    => 'ای صفحه کبل بوتت په حاطر اصلاح بیگ',
'viewsourcetext'       => 'شما تونیت به گند و کپی کنیت منبع ای صفحه آ',
'protectedinterface'   => 'ای صفحه فراهم آریت مداخله ی متنی په برنامه و کبل بیتت په جلوگیری چه سو استفاده.',
'editinginterface'     => "'''هوژاری:''' شما یک صفحه ای اصلاح کنیت که به عنوان مداخله گر متنی برنامه استفاده بیت.
تغییرات ای صفحه کاربرد مداخله گر په دگه کابران تاثیر هلیت.
  [http://translatewiki.net/wiki/Main_Page?setlang=en Betawiki],  په ترجمه یان لطفا توجه کنیت په استفاده پروژه ملکی کتن مدیا وی کی",
'sqlhidden'            => '(SQL درخواست پناهین)',
'cascadeprotected'     => 'ای صفحه محافظت بیت چه اصلاح چرا که آیی شامل جهلیگین {{PLURAL:$1|صفحه, که|صفحات, که}} محافظتی گون the "cascading" option turned on:
$2',
'namespaceprotected'   => "شما اجازت په اصلاح صفحات ته  '''$1'' نام فضا نیست",
'customcssjsprotected' => 'شما را اجازت په اصلاح ای صفحه نیستپه چی که آی شامل شامل  تنظیمات شخصی دگه کاربر اینت',
'ns-specialprotected'  => 'حاصین صفحات اصلاح نه بنت',
'titleprotected'       => "ای عنوان محافظت بوتت چه سربیگ به وسیله  [[User:$1|$1]].
ای دلیل دییگ بیتت ''$2''.",

# Virus scanner
'virus-badscanner'     => 'تنظیم بد: ناشناسین اسکنر ویروس: <i>$1</i>',
'virus-scanfailed'     => 'اسکن پروش وارت(کد $1)',
'virus-unknownscanner' => 'ناشناسین آنتی ویروس:',

# Login and logout pages
'logouttitle'                => 'دربیگ کاربر',
'logouttext'                 => '<strong> شما الان در بوتت.</strong>

شما تونیت چه {{SITENAME}} ناشناس استفاده کنیت یا شما تونیت دگه وراد بیت گون دگه یا هما کاربر.
توجه بیت که لهتی صفحات شاید په داب هما وهدی که شما وراد بوتتیت پیش درگ بند تا وهدی که ذخیره بروزر وتی پاک کنیت.',
'welcomecreation'            => '== وش آتکی،$1! ==
شمی حساب شر بیت.
 مه شموشیت وتی [[Special:Preferences|{{SITENAME}} ترجیحات]] ترجیحات عوض کنیت',
'loginpagetitle'             => 'ورود کاربر',
'yourname'                   => 'نام کاربری',
'yourpassword'               => 'کلمه رمز',
'yourpasswordagain'          => 'کلمه رمز دگه نویس',
'remembermypassword'         => 'می ورود ته ای کامپیوتر په حاطر بدار',
'yourdomainname'             => 'شمی دامین',
'externaldberror'            => 'یک حطا دیتابیس تصدیق هویت دراییگی هست یا شما را اجازت نیست وتی حساب درایی په روچ کنیت.',
'loginproblem'               => '<b>یک مشکلی گون شمی ورود هستت.</b><br /> دگه جهد کن!',
'login'                      => 'ورود',
'nav-login-createaccount'    => 'ورود/شرکتن حساب',
'loginprompt'                => 'شما بایدن په وارد بیگ ته {{SITENAME}} کوکی فعال کنیت',
'userlogin'                  => 'ورود/شرکتن حساب',
'logout'                     => 'در بیگ',
'userlogout'                 => 'در بیگ',
'notloggedin'                => 'وارد نهت',
'nologin'                    => 'حسابء  نیستن؟ $1.',
'nologinlink'                => 'شرکتن یک حساب',
'createaccount'              => 'حساب شرکن',
'gotaccount'                 => 'یک حساب الان هست؟$1.',
'gotaccountlink'             => 'ورود',
'createaccountmail'          => 'گون ایمیل',
'badretype'                  => 'کلماتی رمزی که شما وارد کتگیت یک نهنت.',
'userexists'                 => 'وارد بیتگیت نام کاربری الان استفاده بیت.
لطفا دگه دابین نامی بزوریت.',
'youremail'                  => 'ایمیل:',
'username'                   => 'نام کاربری:',
'uid'                        => 'کاربر شناسگ:',
'prefs-memberingroups'       => 'عضو گروه {{PLURAL:$1|group|groups}}:',
'yourrealname'               => 'راستین  نام:',
'yourlanguage'               => 'زبان:',
'yourvariant'                => 'مغایر:',
'yournick'                   => 'امضا:',
'badsig'                     => 'نامعتبرین حامین امضا تگان HTML چک کن',
'badsiglength'               => 'امضا باز مزنتت.
آی بایدن چوشین  $1 {{PLURAL:$1|character|کاراکتران}}.',
'email'                      => 'ایمیل',
'prefs-help-realname'        => 'راستین  نام اهتیاریتن. اگه شما یکی انتخاب کنیت شی په شمی کارء نشان هلگ په روت.',
'loginerror'                 => 'حطا ورود',
'prefs-help-email'           => 'آدرس ایمیل اختیاری انت، بله اجازت دن که یک نوکین کلمه ی رمزی په شما دیم دهگ بیت وهدی که شما وتی رمزء شموشیت.
شما هنچوش تونیت دگرانء اجازت بدهیت چه طریق شمی بحث_کاربر صفخه بی شی که وتی شناسگ پیش داریت تماس بگرنت.',
'prefs-help-email-required'  => 'آدرس ایمیل نیازنت.',
'nocookiesnew'               => 'حساب کاربر شر بوت بله شما وارد نه بیتگیت ته.
{{SITENAME}} چه کوکی په ورود کابران استفاده کنت.
شما کوکی غیر فعال کتت.
لطفا آییآ فعال کنیت رندا گون وتی نوکین نام کاربری و کلمه رمز وارد بیت.',
'nocookieslogin'             => '{{SITENAME}} په ورود کابران چه کوکی استفاده کنت.
شمی کوکی غیر فعالنت.
لطفا آییا فعال کنیت و دگه  سعی کنیت.',
'noname'                     => 'شما یک معتبرین نام کاربر مشخص نه کتت.',
'loginsuccesstitle'          => 'ورود موفقیت آمیز',
'loginsuccess'               => "''''شما الان وارد {{SITENAME}} په عنوان \"\$1\".'''",
'nosuchuser'                 => 'هچ کاربری گون نام "$1".
وتی املايا چک کنیت یا [[Special:Userlogin/signup|نوکین حسابی شرکنیت]]',
'nosuchusershort'            => 'هچ کاربری گون نام  "<nowiki>$1</nowiki>"نیستن.
وتی املايا کنترل کنیت',
'nouserspecified'            => 'شما باید یک نام کاربری مشخص کنیت.',
'wrongpassword'              => 'اشتباهین کلمه رمز وارد بوت. دگه سعی کن.',
'wrongpasswordempty'         => 'کلمه رمز وارد بیتگین هالیکنت. دگه سعی کن',
'passwordtooshort'           => 'شمی کلمه رمز نامعتبر یا باز هوردنت.
آیی بایدن حداقل {{PLURAL:$1|1 کاراکتر|$1 کاراکتر}} کاراکتر بیت و چه نام کاربری متفاوت بیت.',
'mailmypassword'             => 'نوکین کلمه رمزء ایمیل کن',
'passwordremindertitle'      => 'نوکین هنگین کلمه رمز په {{SITENAME}}',
'passwordremindertext'       => 'یک نفری(شاید شما، چه آی پی $1)
لوٹتگی که ما شما را یک نوکین کلمه رمز دیم دهین په {{SITENAME}} ($4).
کلمه رمز په کاربر "$2" الان شینت"$3".
شما بایدن وارد بیت و وتی کلمه رمزآ بدل کنیت انو.

اگه دگه کسی په شما ای درخواست دیم داته و یا شما وتی کلمه رمزآ خاطر داریت و نه لوٹتیت آیآ عوض کنیت، شما تونیت این کوله یا شموشیت و گون هما قدیمی کلمه رمز ادامه دهیت',
'noemail'                    => 'هچ آدرس ایمیلی په کاربر "$1" ثبت نه بیتت.',
'passwordsent'               => 'یک نوکین کلمه رمزی په آدرس ایمیل ثبت بوتگین په "$1" دیم دهگ بیت.
لطفا بعد چه دریافت وارد بیت',
'blocked-mailpassword'       => 'شمی آدرس آی پی چه اصلاح کتن محدود بوتت و اجازت نداریت په خاطر جلوگیری چه سو استفاده چه عملگر کلمه رمز استفاده بکنت.',
'eauthentsent'               => 'یک ایمیل تاییدی په نامتگین آدرس ایمیل دیم دهگ بوت.
پیش چه هردابین ایمیلی په حساب دیم دیگ بین، شما بایدن چه دستور العملی که ته ایمیل آتکه پیروی کنیت په شی که شمی حساب که شمی گنت تایید بیت.',
'throttled-mailpassword'     => 'یک کلمه رمز یاد آوری پیش تر دیم دهگ بوتت ته  {{PLURAL:$1|ساعت|$1 ساعت}}  ساعت پیش.
په جلوگرگ چه سو استفاده فقط یک کلمه رمز یاد آوری هر$1  ساعت دیم دهگ بیت.',
'mailerror'                  => 'حطا دیم دهگ ایمیل:$1',
'acct_creation_throttle_hit' => 'شرمنده! شما پیشتر $1  حسابی شر کتت.
شما نه تونیت گیشتر شرکنیت.',
'emailauthenticated'         => 'شمی آدرس ایمیل ته $1  تصدیق بوت.',
'emailnotauthenticated'      => 'په آدرس ایمیل هنگت تصدیق نه بوتت.
هچ ایمیلی په جهلیگین ویژگی دیم دهگ نه بیت.',
'noemailprefs'               => 'یک آدرس ایمیل په کار کتن ای ویژگیان مشخص کنیت.',
'emailconfirmlink'           => 'وتی آدرس ایمیل تایید کن',
'invalidemailaddress'        => 'آدرس ایمیل قبول نه بیت چوش که جاه کیت یک فرمت نامعتبری هست.
لطفا یک آدرس ایمیل هو-فرمتی وارد کنیت یا ای فیلد هالیک بلیت.',
'accountcreated'             => 'حساب شر بوت',
'accountcreatedtext'         => 'حساب کاربر په $1 شر بوت.',
'createaccount-title'        => 'شرکتن حساب په {{SITENAME}}',
'createaccount-text'         => 'یکی یک حساب په شمی آدرس ایمیل ته  {{SITENAME}} گون نام ($4)  "$2"، گون کلمه رمز "$3" شرکتت.
شما بایدن وارد بیت و وتی کلمه رمز الان عوض کنیت.

شما شاید ای پیام شموشیت اگه ای ای حساب گون حطا شر بوتت.',
'loginlanguagelabel'         => 'زبان: $1',

# Password reset dialog
'resetpass'               => 'عوض کتن کلمه رمز حساب',
'resetpass_announce'      => 'شما گون یک هنوکین کد ایمیل بوتگین وارد بوتءیت.
په تمام کتن ورود، شما باید یک نوکین کلمه رمز اداں شرکنیت',
'resetpass_text'          => '<!-- متن دان هورکن -->',
'resetpass_header'        => 'عوض کتن کلمه رمز',
'resetpass_submit'        => 'تنظیم کلمه رمز و ورود',
'resetpass_success'       => 'شمی کلمه رمز گون موفقیت عوض بون! هنو شما وارد بیگیت...',
'resetpass_bad_temporary' => 'نامعتبر هنوکین کلمه رمز.
شما شاید پیشتر وتی کلمه رمز آ عوض کتت یا یک نوکین هنوکین کلمه رمز لوٹتگیت.',
'resetpass_forbidden'     => 'کلمات رمز نه توننت عوض بنت.',
'resetpass_missing'       => 'هچ فرم دیتا',

# Edit page toolbar
'bold_sample'     => 'پررنگین متن',
'bold_tip'        => 'پررنگین متن',
'italic_sample'   => 'ایتالیکی متن',
'italic_tip'      => 'ایتالیکی متن',
'link_sample'     => 'عنوان لینک',
'link_tip'        => 'لینک درونی',
'extlink_sample'  => 'http://www.example.com عنوان لینک',
'extlink_tip'     => 'حارجی لینک(مشموش  http:// پیشوند)',
'headline_sample' => 'متن سرخط',
'headline_tip'    => 'سطح ۲ سرخط',
'math_sample'     => 'فرمول اداں وارد کن',
'math_tip'        => 'فرمول ریاضی  (LaTeX)',
'nowiki_sample'   => 'متن بی فرمتءا ادان وارد کن',
'nowiki_tip'      => 'فرمت وی کی شموش',
'image_tip'       => 'فایل هورگین',
'media_tip'       => 'لینک فایل',
'sig_tip'         => 'شمی امضا گون مهر زمان',
'hr_tip'          => 'خط افقی',

# Edit pages
'summary'                          => 'خلاصه',
'subject'                          => 'موضوع/سرخط',
'minoredit'                        => 'ای شی یک هوردین اصلاحیت',
'watchthis'                        => 'ای صفحه بچار',
'savearticle'                      => 'صفحه ذخیره کن',
'preview'                          => 'بازبین',
'showpreview'                      => 'بازبین پیش دار',
'showlivepreview'                  => 'بازبین زنده',
'showdiff'                         => 'تغییرات پیش دار',
'anoneditwarning'                  => "'''هوژاری:''' شما وارد نه بیتگیت.
شمی آی پی ته تاریح اصلاح ای صفحه ثبت بیت.",
'missingsummary'                   => "'''یادآوری:''' شما یک خلاصه چه اصلاح وارد نه کرت.
اگر دگه کلیک کنیت ذخیره آ، شمی اصلاح به بی آی ذخیره بنت.",
'missingcommenttext'               => 'لطفا یک نظری وارد کنیت جهل آ',
'missingcommentheader'             => "'''یاداوری:'' شما یک موضوع/سرخط په ای نظر وارد نکتت.
اگر شما دگه ذخیره کلیک کنیت، شمی اصلاح بی آی ذخیره بنت.",
'summary-preview'                  => 'خلاصه بازبینی',
'subject-preview'                  => 'بازبین موضوع/سرخط',
'blockedtitle'                     => 'کاربر محدود بوتت',
'blockedtext'                      => "<big>'''شمی نام کاربری یا آی پی محدود بیتت.''''</big>

محدودیت توسط $1 شر بوتت. دلیل داتت ''$2''.

*شروع محدودیت: $8*
*هلاسی محدودیت:$6
* لوٹتگین محدود بی یوک: $7

شما تونیت گون $1 یا دگه [[{{MediaWiki:Grouppage-sysop}}|مدیر]] په بحث محدودیت باره تماس گیرت.
شما نه تونیت چه ویژگی'ایمیل ای کاربر' استفاده کنیت مگر شی که یم معتبرین آدرس ایمیلی مشخص بیت ته شمی  [[Special:Preferences|ترجیحات حساب]] و شما چه استفاده ی آیی محدود نه بیت.
  $3 شمی هنوکین آی پی شی ان و شماره محدودیت شی ینت #$5. لطفا هر دو تا یا یکی په وهد سوال کتن هور کنیت.",
'autoblockedtext'                  => 'شمی آی پی اتوماتیکی محدود بوت په چی که آی گون دگر کاربری استفاده بیگت که آیی محدود بوتت گون $1.
دلیل شی انت:

:\'\'$2\'\'

* شروع محدودیت:  $8
* هلگ محدودیت: $6
* لوتوکی محدوی بووک: $7

شما شاید تماس گریت گون $1 یا یکی دگه چه [[{{MediaWiki:Grouppage-sysop}}|مدیران ]] په بحث درباره محدودیت.

توجه بیت شما شاید چه ویژگی "ای کاربر ایمیل دیم دی" مه تونیت استفاده کینت مگر شی که یک معتبرین آدرس ایمیل ته وتی [[Special:Preferences|ترجیحات کاربر ]] ثبت کنیت و شما چه استفاده چه آیی محدود نه بیت.

 مشی هنوکی ان آی پی $3 شمی شماره محدودیت $5
لطفا ای شماره ته هر جوست و پرسی هور کنیت.',
'blockednoreason'                  => 'هچ دلیلی دهگ نه بیته',
'blockedoriginalsource'            => "منبع '''$1''' جهلآ پیش دراگ بیت:",
'blockededitsource'                => "متن '''your edits'' به '''$1''' جهلآ پیش دارگ بیت:",
'whitelistedittitle'               => 'په اصلاح کتن بایدن وارد سیستم بیت',
'whitelistedittext'                => 'شما باید $1به اصلاح کتن صفحات.',
'confirmedittitle'                 => 'به اصلاح کتن تایید ایمیل نیازنت',
'confirmedittext'                  => 'شما بایدن وتی آدرس ایمیل آ پیش چه اصلاح کتن صفحات تایید کنیت.
لطفا وتی آدرس ایمیل آی چه طریق [[Special:Preferences|ترجحات کاربر]] تنظیم و معتبر کنیت.',
'nosuchsectiontitle'               => 'هچ چوشن بخش',
'nosuchsectiontext'                => 'شما سعی کت یک بخشی اصلاح کنیت که نیستن.
چوش که هچ بخشی $1 نیست، هچ جاهی په ذخیره کتن شمی اصلاح نیست.',
'loginreqtitle'                    => 'ورود نیازنت',
'loginreqlink'                     => 'ورود',
'loginreqpagetext'                 => 'شما باید $1 په گندگ دگه صفحات.',
'accmailtitle'                     => 'کلمه رمز دیم دات',
'accmailtext'                      => 'کلمه رمز په "$1"  دیم دهگ بوت په $2.',
'newarticle'                       => '(نوکین)',
'newarticletext'                   => "شما رند چه یک لینکی په یک صفحه ی که هنو نیستند اتکگیت.
په شر کتن صفحه، شروع کن نوشتن ته جعبه جهلی(بچار  [[{{MediaWiki:Helppage}}|صفحه کمک]]  په گیشترین اطلاعات).
اگر شما اشتباهی ادانیت ته وتی بروزر دکمه ''Back'' بجن.",
'anontalkpagetext'                 => "----'' ای صفحه بحث انت په یک ناشناس کاربری که هنگت یک حسابی شر نه کتت یا آی ا ستفاده نه کتت. اچه ما بایدن آدرس آی پی عددی په پچاه آرگ آیی استفاده کنین.
چوشن آدرس آی پی گون چندین کاربر استفاده بیت.
اگه شما یک کاربر ناشناس ایت وی حس کنیت بی ربطین نظر مربوط شمی هست، لطفا [[Special:UserLogin|وارد بیت ]] یا [[Special:UserLogin/signup|حسابی شرکن]] دان چه هور بییگ گون ناسناسین کاربران پرهیز بیت.''",
'noarticletext'                    => 'هنو هچ متنی ته ای صفحه نیست، شما تونیت  [[Special:Search/{{PAGENAME}}|گردگ په عنوان صفحه]]  ته دگه صفحات یا [{{fullurl:{{FULLPAGENAME}}|action=edit}} ای صفحه اصلاح کن].',
'userpage-userdoesnotexist'        => 'حساب کاربر "$1" ثبت نهنت. لطفا کنترل کنیت اگه شما لوٹیت ای صفحه یا شر/اصلاح کنیت.',
'clearyourcache'                   => "'''توجه:''' بعد چه ذخیره کتن، شما شاید مجبور بیت چه وتی ذخیره ی بروزر رد بیت تا تغییرات بگندیت. '''Mozilla / Firefox / Safari:'' ''Shift'' جهل داریت همی وهدی که کلیک کنیت ''Reload'' یا بداریت ''Ctrl-Shift-R'' (''Cmd-Shift-R'' on Apple Mac);'''IE:''' ''Ctrl''  بداری وهدی که کلیک ''Refresh' یا 'Ctrl-F5''; '''Konqueror:''':  راحت کلیک کن دکمه ''Reload'' یا بدار ''F5''; '''Opera''' کاربر بایدن ته ''Tools→Preferences'' ذخیره پاک کنت.",
'usercssjsyoucanpreview'           => "<strong>نکته:</strong> چه دکمه 'Show preview' په آزمایش کتن  CSS/JS پیش چه ذخیره کتن استفاده کن",
'usercsspreview'                   => "''''بزان که شما فقط وتی CSS کاربری بازبینی کنین. هنگنت آیی ذخیره نه بوتت!''''",
'userjspreview'                    => "''''په یاد دار که شما فقط وتی کاربری  JavaScript بازبینی/آزمایش کنگیت، هنگت ذخیره نه بوتت!''''",
'userinvalidcssjstitle'            => "'''هوژاری:''هچ جلدی نیست\"\$1\".
بزان که صفحات .css و .js چه عناوین گون هوردین حرف استفاده کننت، مثلا {{ns:user}}:Foo/monobook.css بدل به په {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(په روچ بیتگین)',
'note'                             => '<strong>یادداشت:</strong>',
'previewnote'                      => '<strong>شی فقط یک بازبینی انت;
تغییرات هنگت ذخیره نهنت. </strong>',
'previewconflict'                  => 'ای بازبین متنء پیش داریت ته منطفه بالدی اصلاحی هنچوش که پیش دارگ بیت اگه شما انتخاب کنیت ذخیره',
'session_fail_preview'             => '<strong>شرمنده! ما نه تونست شمی اصلاحء په خاطر گار کتن دیتا دیوان پردازش کنین.
طلف دگه سعی کنیت. اگر هنگت کار نکنت یک بری [[Special:UserLogout|دربیت]] و پیدا وارد بیت.</strong>',
'session_fail_preview_html'        => "<strong>شرمنده! ما نه تونست شمی اصلاحء په خاطر گار کتن دیتا دیوان پردازش کنین.</strong>

''په چی که {{SITENAME}} HTML هام فعالنت، بازبین په خاطر حملات JavaScript پناهنت.''

<strong> اگر شی یک قانونی تلاش اصلاحنت، دگه کوشش کنیت. اگر هنگت کار نکنت یک بری [[Special:UserLogout|دربیت]] و دگه وارد بیت.</strong>",
'token_suffix_mismatch'            => '<strong> شمی اصلاح رد بوت په چی که شمی کلاینت نویسگ کاراکترانی په هم جتت.
اصلاح رد بوت داں چه هراب بیگ متن صفحه جلوگیری بیت.
شی لهتی وهد پیش کت که شما چه یک هرابین سرویس پروکسی وبی استفاده کنیت.</strong>',
'editing'                          => 'اصلاح $1',
'editingsection'                   => 'اصلاح $1(بخش)',
'editingcomment'                   => 'اصلاح $1 (نظر)',
'editconflict'                     => 'جنگ ورگ اصلاح: $1',
'explainconflict'                  => "کسی دگه ای صفحه یا عوض کتت چه وهدی که شما اصلاح آیء شروع کتء.
بالادی ناحیه متن شامل متن صفحه همی داب که هنگت هست.
شمی تغییرات ته جهلیگین ناحیه متن جاه کیت.
شما بایدن وتی تغییرات آن گون هنوکین متن چن و بند کنیت.
'''فقط''' ناحیه بالادی متن وهدی که شما دکمه  \"Save page\" ذخیره بنت.",
'yourtext'                         => 'شمی متن',
'storedversion'                    => 'نسخه ی ذخیره ای',
'nonunicodebrowser'                => '<strong>هوژاری: شمی بروزر گون یونی کد تنظیم کار نکنت. یک اطراف-کار جاهینن که شما را اجازه دنت صفحات راحت اصلاح کنیت: non-ASCII کاراتران ته جعبه اصلاح په داب کدان hexadecimal جاه کاینت.',
'editingold'                       => '<strong>هوژاری: شما په اصلاح کتن یک قدیمی بازبینی چه ای صفحه ایت.
اگر شما ایء ذخیره کتت، هر تغییری که دهگ بیتء چه ای بازبینی گار بنت.</strong>',
'yourdiff'                         => 'تفاوتان',
'copyrightwarning'                 => 'لطفا توجه بیت که کل نوشته یات ته {{SITENAME}}  تحت $2 نشر بنت.(بچار په جزیات$1).
اگه شما لوٹیت شمی نوشتانک اصلاح و دگه چهاپ مبنت، اچه آیانا ادان مهلیت.<b/>
شما ما را قول دهیت که وتی چیزا بنویسیت یا چه یک دامین عمومی کپی کتگیت.
<strong> نوشتانکی که کپی رایت دارند بی اجازه ادا هور مکنیت</strong>',
'copyrightwarning2'                => 'لطفا توجه کنیت که کل مشارکاتن ته {{SITENAME}} شاید اصلاح, عوض و یا توسط دگه شرکت کننده آن حذف بنت.
اگر شما نه لوٹیت شمی نوشتاک گون بی رحمی اصلاح مه بنت، اچه شما آیء ادان دیم مه دهیت.<br />
شما هنچوش ما را قول دهیت که شما شی وت نوشتت یا ایء چه یک دامین عمومی یا هنچوشین آزاتین منبع کپی کتیت.(بچار $1 په جزییات).
<strong> نوشتاکی که حق کپی دارنت بی اجازت دیم مه دهیت!</strong>',
'longpagewarning'                  => '<strong>هوژاری. ای صفحه $1 کیلوبایت نت;
لهتی چه بروزران شاید مشکلاتی چه دست رسی و اصلاح صفحات گیش چه 32ک.ب داشته بنت.
لطفا توجه کنیت په هورد کتن صفحه په هوردترین چنٹ. </strong>',
'longpageerror'                    => '<strong>حطا: متنی که شما دیم داتت $1 کیلو بایتت، که چه گیشترین حد $2 کیلوبایت مزن
آی نه تونیت ذخیره بوت.</strong>',
'readonlywarning'                  => '<strong>هوژاری: دیتابیس به تعمیرات کبلنت، اچه شما نه تونیت وتی اصلاحات هنو ذخیره کنیت.
شما شاید بلوٹیت متنء تع یم فایل متنی کپی و پیست کنیت و آیء ذخیره کنیت.</strong>',
'protectedpagewarning'             => '<strong>هوژاری: ای صفحه په کبلنت چی که فقط کابران گون اجازت مدیر سیستم توننت آیء اصلاح کننت.</strong>',
'semiprotectedpagewarning'         => "''''توجه:'''' ای صفحه کبلنت چوش که فقط ثبت نامی کابران توننت آیء اصلاح کننت.",
'cascadeprotectedwarning'          => "''هوژاری''ای صفحه کبلنت چوش که فقط کابران گون دسترسی مدیر سیستم توننت آییء اصلاح کننت،په چی که آیی ته چهلین حمایت آبشاری {{PLURAL:$1|صفحات|صفحه}}:",
'titleprotectedwarning'            => '<strong>هوژاری: ای صفحه کبلنت چوش که فقز لهتی کاربر تواننت آیء شر کننت.</strong>',
'templatesused'                    => 'تمپلتانی که ته ای صفحه استفاده بیت:',
'templatesusedpreview'             => 'تلمپلت آنی که ته ای بازبینی استفاده بیت',
'templatesusedsection'             => 'تمپلتانی که ته ای بخش به کار رونت',
'template-protected'               => '(محافظتین)',
'template-semiprotected'           => '(نیم محافظتی)',
'hiddencategories'                 => 'ای صفحه عضوی چه {{PLURAL:$1|1 hidden category|$1 پناهین دسته جات}}:',
'edittools'                        => '<strong>په کپی و پست کتن چه CTRL+V , CTRL+C استفاده کنیت.</strong>',
'nocreatetitle'                    => 'شرکتن صفحه محدودنت',
'nocreatetext'                     => '{{SITENAME}} شما را چه شرکتن نوکین صفحه منه کته.
شما تونیت برگردیت و یک پیشگین صفحه ای اصلاح کنیت، یا [[Special:UserLogin|وارد بیت یان یک حسابی شرکنیت]].',
'nocreate-loggedin'                => 'شما را اجازت په شرکتن نوکین صفحات نیست.',
'permissionserrors'                => 'حطای اجازت',
'permissionserrorstext'            => 'شما را اجازت په انجام آی نیست، په جهلیگین دلیل {{PLURAL:$1|دلیل|دلایل}}:',
'permissionserrorstext-withaction' => 'شما را اجازت په $2, په خاطر جهلیگین {{PLURAL:$1|دلیل|دلایل}}:',
'recreate-deleted-warn'            => "'''هوژاری: شما یک صفحه ای دگه شرکنگیت که پیشتر حذف بوتت.'''

شما بایدن توجه کنیت که ادامه اصلاح ای صفحه درستنت.
آمار حذف ای صفحه په شمی حاطرء ادان هستن:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'هوژاری: ای صفحه شامل بازگین توار عملگر تجریه کنوک سنگیننت.
آیی بایدن کمتر چه  $2, داشته بیت ادان هنو  $1 هست.',
'expensive-parserfunction-category'       => ' صفحات گونبازگین توار عملگر تجریه کنوک',
'post-expand-template-inclusion-warning'  => 'هوژاری: اندازه شامل تمپلت باز مزننت.
لهتی تمپلتان هور نه بینت.',
'post-expand-template-inclusion-category' => 'صفحات که اندازه تمپلت شامل گیشنت',
'post-expand-template-argument-warning'   => 'هوژاری: ای صفحه شامل یک آرگومان تمپلت انت که اندازه ی بازنت.
ای آرگومان آن بایدن حذف بوتنت',
'post-expand-template-argument-category'  => 'صفحات شامل حذفی آرگومان آن تمپلت انت',

# "Undo" feature
'undo-success' => 'اصلاح برگشت نه بیت. لطفا مقایسه جهلگینء کنترل کنیت په تایید شی که شی هما انت که شما لوٹیت، و بعدا تغغیرات جهلی په تمام کتن بر نگردگ اصلاح ذخیره کنیت.',
'undo-failure' => 'اصلاح بر نرگردیت په خاطر تضاد میان اصلاحاتی',
'undo-norev'   => 'اصلاح نه تونیت برگردیت په چی که آی وجود نهنت یا حذف بوتت.',
'undo-summary' => 'بازبینی برگردین $1 گون [[Special:مشارکتان/$2|$2]] ([[User talk:$2|گپ]] | [[Special:Contributions/$2|{{MediaWiki:Contribslink}}]])',

# Account creation failure
'cantcreateaccounttitle' => 'نه نونیت حساب شرکنت',
'cantcreateaccount-text' => "شرکتن حساب چی ای آدرس آی پی ('''$1''') محدود بوتت توسط [[User:$3|$3]].

دلیلی داتگین توسط $3  شی انت ''$2''",

# History pages
'viewpagelogs'        => 'آمار ای صفحه بچار',
'nohistory'           => 'په ای صفحه تاریح اصلاح نیست.',
'revnotfound'         => 'بازبینی در گیزگ نه بوت',
'revnotfoundtext'     => 'کدیمی بازبینی چه ای صفحه که شما لوٹیت ودیگ نه بوت. لطفا URL که شما په رستن په ای صفحه استفاده کنیت کنترلی کنیت.',
'currentrev'          => 'هنوکین بازبینی',
'revisionasof'        => 'بازبینی په عنوان $1',
'revision-info'       => 'بازبینی په داب $1 توسط $2',
'previousrevision'    => '←پیش ترین نسخه',
'nextrevision'        => 'نوکین بازبینی→',
'currentrevisionlink' => 'هنوکین بازبینی',
'cur'                 => 'هنو',
'next'                => 'بعدی',
'last'                => 'اهری',
'page_first'          => 'اولین',
'page_last'           => 'اهرین',
'histlegend'          => 'بخش تفاوت: په مقایسه کتن نسخه یان گزینه انتخاب کنیت اینتر یا دکمه بجن.<br />
Legend: (cur) = تفاوتان گون هنوکین نسخه,
(last) = تفاوت گون بعدی نسخه, M = هوردین  اصلاح.',
'deletedrev'          => '[حذف]',
'histfirst'           => 'اولین',
'histlast'            => 'اهرین',
'historysize'         => '({{PLURAL:$1|1 بایت|$1 بایت}})',
'historyempty'        => '(هالیک)',

# Revision feed
'history-feed-title'          => 'تاریح بازبینی',
'history-feed-description'    => 'تاریح بازبینی په ای صفحه ته ویکی',
'history-feed-item-nocomment' => '$1 ته $2', # user at time
'history-feed-empty'          => 'لوٹتگین صفحه موجود نهنت.
شاید آی چه ویکی حذف بوتت یا نامی بدل بوتت.
آزمایش کن[[Special:Search|گردگ ته ویکی]] په مربطین نوکین صفحات.',

# Revision deletion
'rev-deleted-comment'         => '(نظر زورگ بیتت)',
'rev-deleted-user'            => '(نام کاربری زورگ بیتت)',
'rev-deleted-event'           => '(کار آمار زورگ بیتت)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
ای بازبینی صفحه چه آرشیو عمومی زورگ بیتت.
شاید جزییاتی ته [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} حذف آمار]. بیت</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
ای بازبینی صفحه چه آرشیو عمومی زورگ بیتت.
په عنوان مدیر ته {{SITENAME}}  شما تونیت آیء بگنیت;
شاید جزییاتی ته ببیت [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} آمار حذف].</div>',
'rev-delundel'                => 'پیش دار/پناه کن',
'revisiondelete'              => 'حذف/حذف مکن بازبینیء',
'revdelete-nooldid-title'     => 'نامعتبر بازبینی هدف',
'revdelete-nooldid-text'      => 'شما یا یک بازبینی هدفی په اجرای ای عمل مشخص نه کتت
،بازبینی مشخص موجود نهنت، یا شما تلاش کنیت هنوکین بازبینی پناه کنیت.',
'revdelete-selected'          => '{{PLURAL:$2|بازبینی انتخابی|بازبینی ان انتخابی}} چه [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|رویداد آمار انتخابی|رویداد ان آمار انتخابی}}:',
'revdelete-text'              => 'حذفین بازبینی آن و رویداد ان هنگت ته تاریح و آمار صفحه جاه کاینت، بله لهتی چه محتوا آيان په عام قابل دسترسی نه بنت.

دگه مدیران ته {{SITENAME}} هنگت نوننت په پناهین محتوا دسترسیش بیت و توننت آیء چه طریق همی دستبری آی> تریننت، مگر شی که گیشین محدودیت بلیت.',
'revdelete-legend'            => 'تنظیم محدودیت آن دیستن',
'revdelete-hide-text'         => 'پناه کن متن بازبینیء',
'revdelete-hide-name'         => 'پناه کن کار  و هدفء',
'revdelete-hide-comment'      => 'پناه کن اصلاح نظرء',
'revdelete-hide-user'         => 'پناه کن اصلاح کنوکء نام کاربری/آی پی',
'revdelete-hide-restricted'   => 'ای محدودیت آنء په مدیران سیستم بل و ای دستبری کبل کن',
'revdelete-suppress'          => 'توقیف کن دیتاء چه مدیران سیستم و دگران',
'revdelete-hide-image'        => 'پناه کن فایل محتواء',
'revdelete-unsuppress'        => 'بزور محدودیت آنء جه ترینتگین بازبینی آن',
'revdelete-log'               => 'آمار نظر:',
'revdelete-submit'            => 'بلی اور انتخابی بازبینی',
'revdelete-logentry'          => 'عوض بوت ظاهر بیگ بازبینی  [[$1]]',
'logdelete-logentry'          => 'عوض بیت ظاهر بوتن رویداد چه  [[$1]]',
'revdelete-success'           => "'''ظاهر بازبینی گون موفقیت تنظیم بوت.'''",
'logdelete-success'           => "''''آمار ظاهر بیگ گون موفقیت تنظیم بوت.''''",
'revdel-restore'              => 'عوض کن ظاهر بیگء',
'pagehist'                    => 'تاریح صفحه',
'deletedhist'                 => 'تاریح حذف بوت',
'revdelete-content'           => 'محتوا',
'revdelete-summary'           => 'خلاصه اصلاح',
'revdelete-uname'             => 'نام کاربری',
'revdelete-restricted'        => 'محدودیت آن په مدیران سیستم بوت',
'revdelete-unrestricted'      => 'به زور چه مدیران سیستم محدودیتان',
'revdelete-hid'               => 'پناه $1',
'revdelete-unhid'             => ' $1پنهاه مکن',
'revdelete-log-message'       => '$1 په $2 {{PLURAL:$2|بازبینی|بازبینی ان}}',
'logdelete-log-message'       => '$1 په $2 {{PLURAL:$2|رویداد|رویدادان}}',

# Suppression log
'suppressionlog'     => 'آمار توقیف',
'suppressionlogtext' => 'جهلء یک لیست چه حذفیات و محدودیات من جمله پناهین محتوا چه مدیران سیستم هست.
به چار [[Special:IPBlockList|IP block list]] په لیست هنوکین عملی محدویت آن',

# History merging
'mergehistory'                     => 'چن و بند کن تاریح آن صفحهء',
'mergehistory-header'              => 'ای صفحه شما را اجارت دن بازبینی ان تاریح یکی چه منابه صفحه ته یک نوکین صفحه چن وبند کنت.
مطمین بت که ای تغییر ادامه تاریحی صفحه داریت.',
'mergehistory-box'                 => 'چن وبند بازبینی آن دو صفحه:',
'mergehistory-from'                => 'منبع صفحه:',
'mergehistory-into'                => 'صفحه مقصد:',
'mergehistory-list'                => 'تاریح اصلاح قابل چن و بند',
'mergehistory-merge'               => 'جهلیگین بازبینی آن  [[:$1]] نوننت چن و بند بنت ته [[:$2]].
چه ستون دکمه رادیوی په چن و بند کتن استفاده کتن فقط ته بازبینی آن شربوتگین ته یا پیش چه زمان مشخضین.
توجه بیت استفاده چه لینکان گردگ این ستون بدل کنت.',
'mergehistory-go'                  => 'پیش دار اصلاحات قابل چن وبند',
'mergehistory-submit'              => 'چن وبند کن بازبینی آنء',
'mergehistory-empty'               => 'هچ بازبینی چن و بند نه توننت بنت',
'mergehistory-success'             => '$3 {{PLURAL:$3|بازبینی|بازبینی ان}} ء [[:$1]] گون موفقیت چن و بند بوت ته [[:$2]].',
'mergehistory-fail'                => 'نه تونیت چن وبند تاریح اجرا کنت، لطفا دگه چک کنیت صفحه و وهد پارامترانء.',
'mergehistory-no-source'           => 'منبع صفحه  $1 موجود نهنت.',
'mergehistory-no-destination'      => 'صفحه مقصد  $1 موجود نهنت.',
'mergehistory-invalid-source'      => 'منبع صفحه بایدن یک معتبرین عنوانی بیت.',
'mergehistory-invalid-destination' => 'صفحه مقصد باید یک معتبرین عنوانی بیت.',
'mergehistory-autocomment'         => 'چن و بند بوت  [[:$1]] په [[:$2]]',
'mergehistory-comment'             => 'چن و بند بوت [[:$1]] په[[:$2]]: $3',

# Merge log
'mergelog'           => 'آمار چن وبند',
'pagemerge-logentry' => 'چن و بند بوت [[$1]] په  [[$2]] (بازبینی ان تا$3)',
'revertmerge'        => 'بی چن وبند',
'mergelogpagetext'   => 'جهلء یک لیست چه نوکترین چن وبندان یکی تاریح صفحه په دگری هست.',

# Diffs
'history-title'           => 'تاریح بازبینی "$1"',
'difference'              => '(تفاوتان بین نسخه یان)',
'lineno'                  => 'خط$1:',
'compareselectedversions' => 'مقایسه انتخاب بوتگین نسخه یان',
'editundo'                => 'خنثی کتن',
'diff-multi'              => '({{PLURAL:$1|یک متوسطین بازبینیان میانی}} پیش دارگ نه بیت .)',

# Search results
'searchresults'             => 'نتایج گردگ',
'searchresulttext'          => 'په گیشترین اطلاعات گردگ باره {{SITENAME}}، بچار [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'شما گردگیت په \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|کل صفحات شروع بنت گون "$1"]] | [[Special:WhatLinksHere/$1|کل صفحات که لینک انت په "$1"]])',
'searchsubtitleinvalid'     => "شما گردگیت په '''$1'''",
'noexactmatch'              => "'''صفحه ی گون عنوان نیست\"\$1\".'''
شما تونیت [[:\$1|ای صفحه ی شرکنیت]].",
'noexactmatch-nocreate'     => "'''.هچ صفحه ای تحت عنوان \"\$1\" نیست'''",
'toomanymatches'            => 'بازگین هم دپ درگیزگ بوت، لطفا یک متفاوتین درخواست آزمایش کنیت',
'titlematches'              => 'عنوان صفحه هم دپ نت',
'notitlematches'            => 'هچ عنوان صفحه هم دپ نهنت',
'textmatches'               => 'متن صفحه هم دپ بنت',
'notextmatches'             => 'هچ متن صفحه هم دپ نهنت',
'prevn'                     => 'پیشگین $1',
'nextn'                     => 'بعدی $1',
'viewprevnext'              => '($1) ($2) ($3) دیدگ',
'search-result-size'        => '$1 ({{PLURAL:$2|1کلمه|$2 کلمات}})',
'search-result-score'       => 'ربط: $1%',
'search-redirect'           => '(غیر مستقیم $1 )',
'search-section'            => '(بخش $1 )',
'search-suggest'            => 'شما را منظور ات: $1',
'search-interwiki-caption'  => 'پروژه آن گوهار',
'search-interwiki-default'  => '$1 نتایج:',
'search-interwiki-more'     => '(گیشتر)',
'search-mwsuggest-enabled'  => 'گون پیشنهاد',
'search-mwsuggest-disabled' => 'هچ پیشنهاد',
'search-relatedarticle'     => 'مربوطین',
'mwsuggest-disable'         => 'پیشنهادات آژاکسیء غیر فعال کن',
'searchrelated'             => 'مربوط',
'searchall'                 => 'کل',
'showingresults'            => "جهل پیش دارگنت تا  {{PLURAL:$1|'''1'''نتیجه|'''$1''' نتایج}} شروع بنت گون #'''$2'''.",
'showingresultsnum'         => "جهل پیش داریت  {{PLURAL:$3|'''1''' نتیجه|'''$3''' نتایج}} شروع بیت گون #'''$2'''.",
'showingresultstotal'       => "جهل پیش داریت  {{PLURAL:$3|نتیجه '''$1''' of '''$3'''|نتایج '''$1 - $2''' چه '''$3'''}}",
'nonefound'                 => "'''توجه''':  فقط لهتی نام فضا په طور پیش فرض گردگ بیتت. سعی کنیت وتی جوستء هور کنیت گون ''کل:'' په گردگ په کل محتوا (شامل صفحات گپ، تمپلتان ودگر)، یا استفاده کنیت لوٹیگن نام فضا په داب پیش وند.",
'powersearch'               => 'پیشرپتگی گردگ',
'powersearch-legend'        => 'گردگ پیشرفته',
'powersearch-ns'            => 'گردگ ته نام فضا آن',
'powersearch-redir'         => 'لیست عیرمستقیم آن',
'powersearch-field'         => 'گردگ په',
'search-external'           => 'حارجی گردگ',
'searchdisabled'            => '{{SITENAME}} گردگ غیر فعالنت.
شما نونیت بگردیت چه طرق گوگل هم زمان.
توجه که اندیکس آن {{SITENAME}} محتوا شاید تاریح گوستگین بنت.',

# Preferences page
'preferences'              => 'ترجیحات',
'mypreferences'            => 'منی ترجیحات',
'prefs-edits'              => 'تعداد اصلاحات:',
'prefsnologin'             => 'وارد نهیت',
'prefsnologintext'         => 'شما بایدن  <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} وارد بیت]</span> په تنظیم کتن ترجیحات.',
'prefsreset'               => 'ترجیحات چه ذخیره ترینگ بوتنت.',
'qbsettings'               => 'میله سریع',
'qbsettings-none'          => 'هچ یک',
'qbsettings-fixedleft'     => 'چپ ثابت',
'qbsettings-fixedright'    => 'راست ثابت',
'qbsettings-floatingleft'  => 'چپ شناور',
'qbsettings-floatingright' => 'راست شناور',
'changepassword'           => 'کلمه رمز عوض کن',
'skin'                     => 'پوست',
'math'                     => 'ریاضی',
'dateformat'               => 'فرم تاریح',
'datedefault'              => 'هچ ترجیح',
'datetime'                 => 'تاریح و وهد',
'math_failure'             => 'تجزیه پروش وارت',
'math_unknown_error'       => 'ناشناسین حطا',
'math_unknown_function'    => 'ناشناس عملگر',
'math_lexing_error'        => 'حطا نوشتاری',
'math_syntax_error'        => 'حطا ساختار',
'math_image_error'         => 'بدل کتن PNGپروش وارت;
کنترل کنیت په نصب latex, dvips, gs, و convert',
'math_bad_tmpdir'          => 'نه نونیت بنویسیت یا مسیر غیر دایمی ریاضی شرکنت',
'math_bad_output'          => 'نه تونیت بنویسیت یا مشیر خروجی ریاضی شرکنت.',
'math_notexvc'             => 'ترکیب کتن texvc  قابل اجرا;
لطفا بچار math/README په تنظیم کتن.',
'prefs-personal'           => 'نمایه کاربر',
'prefs-rc'                 => 'نوکین تغییرات',
'prefs-watchlist'          => 'لیست چارگ',
'prefs-watchlist-days'     => 'روچان په پیش دارگ ته لیست چارگ',
'prefs-watchlist-edits'    => 'گشیترین تعداد تغییرات په پیشدارگ ته پچین لیست چارگ:',
'prefs-misc'               => 'هردابین',
'saveprefs'                => 'ذخیره',
'resetprefs'               => 'پاکن تغییرات ذخیره نه بوتگین',
'oldpassword'              => 'کلمه رمز کهنگین:',
'newpassword'              => 'نوکین کلمه رمز:',
'retypenew'                => 'کلمه رمز دگه بنویس',
'textboxsize'              => 'اصلاح',
'rows'                     => 'ردیفآن«',
'columns'                  => 'ستون‌ان:',
'searchresultshead'        => 'گردگ',
'resultsperpage'           => 'کلیک ته هر صفحه:',
'contextlines'             => 'خطوط در کلیک:',
'contextchars'             => 'متن در خط:',
'stub-threshold'           => 'سرحد په  <a href="#" class="stub">چنڈ لینک</a> فرمت (بایت):',
'recentchangesdays'        => 'روچ ان به پیش دارگ ته نوکیت تغییرات:',
'recentchangescount'       => 'تعداد اصلاحات به پیش دارگ ته نوکین تغییرات',
'savedprefs'               => 'شمی ترجیحات ذخیره بوتن',
'timezonelegend'           => 'وهد ملک',
'timezonetext'             => '¹تعداد ساعاتی که شمی ملکی وهد چه زمان سرور فرق کنت (UTC).',
'localtime'                => 'ملکی وهد',
'timezoneoffset'           => 'اختلاف¹',
'servertime'               => 'وهد سرور',
'guesstimezone'            => 'پرکن چه بروزر',
'allowemail'               => 'فعال کن ایمیل چه دگه کابران',
'prefs-searchoptions'      => 'گردگ انتخابان',
'prefs-namespaces'         => 'نام فصا',
'defaultns'                => 'گردگ ته ای نام فضا آن په طور پیش فرض:',
'default'                  => 'پیش فرض',
'files'                    => 'فایلان',

# User rights
'userrights'                  => 'مدیریت حقوق کاربر', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'مدیریت گروه کاربر',
'userrights-user-editname'    => 'یک نام کاربری وارد کن',
'editusergroup'               => 'اصلاح گروه کاربر',
'editinguser'                 => "عوض کنت حقوق کاربر  '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'اصلاح گروه کاربر',
'saveusergroups'              => 'ذخیره گروه کاربر',
'userrights-groupsmember'     => 'عضو انت:',
'userrights-groups-help'      => 'شما شاید گروه ای کاربرء هست ته:
* یک جعبه علامتی یعنی شی که کاربر تا آ گروه انت.
* یک بی علامتین جعبه یعنی شی که کاربر ته آ گروه نهنت.
* A * پیش داریت که شما نه تونیت بزور گروهی که یک بری که آیء اضافه کت یا برعکس.',
'userrights-reason'           => 'دلیل په عوض کتن:',
'userrights-no-interwiki'     => 'شما را اجازت نیست دان حقوق کاربر ته دگ ویکی آن اصلاح کنیت.',
'userrights-nodatabase'       => 'دیتابیس $1  موجود نهنت یا محلی نهنت.',
'userrights-nologin'          => 'شما باید [[Special:UserLogin|وارد بیت]]  گون حساب مدیریتی په مشخص کتن حقوق کاربر.',
'userrights-notallowed'       => 'شمی حساب اجازت په مشخص کتن حقوق کاربر نیست.',
'userrights-changeable-col'   => 'گروهانی که شما تونیت عوض کنیت',
'userrights-unchangeable-col' => 'گروهانی که شما نه نونیت عوض کنیت',

# Groups
'group'               => 'گروه:',
'group-user'          => 'کابران',
'group-autoconfirmed' => 'کابران تایید اتوماتیکی',
'group-bot'           => 'روباتان',
'group-sysop'         => 'مدیران سیستم',
'group-bureaucrat'    => 'دیوان داران',
'group-suppress'      => 'رویت کنوکان',
'group-all'           => '(کل)',

'group-user-member'          => 'کاربر',
'group-autoconfirmed-member' => 'کاربر تایید اتوماتیکی',
'group-bot-member'           => 'روبات',
'group-sysop-member'         => 'مدیر سیستم',
'group-bureaucrat-member'    => 'دیوان دار',
'group-suppress-member'      => 'رویت کنوک',

'grouppage-user'          => '{{ns:project}}:کابران',
'grouppage-autoconfirmed' => '{{ns:project}}:کابران تایید اتوماتیکی',
'grouppage-bot'           => '{{ns:project}}:روباتان',
'grouppage-sysop'         => '{{ns:project}}:مدیران',
'grouppage-bureaucrat'    => '{{ns:project}}:دیواندارآن',
'grouppage-suppress'      => '{{ns:project}}:رویت',

# Rights
'right-read'                 => 'بوان صفحاتء',
'right-edit'                 => 'اصلاح کن صفحاتء',
'right-createpage'           => 'شرکن صفحاتء(که صفحات بحث نهنت)',
'right-createtalk'           => 'شرکتن صفحات بحث',
'right-createaccount'        => 'شرکتن نوکین حسابان کاربری',
'right-minoredit'            => 'نشان کن اصلاحات په داب هوردین',
'right-move'                 => 'جاه په جاه کن صفحات',
'right-move-subpages'        => 'جاه په جاه کن صفحات گون زیرصفحاتش',
'right-suppressredirect'     => 'شر نه کتن یک غیر مستقیم چه کهنگین نام وهدی که یک صفحه ای جاه په جاه بیت',
'right-upload'               => 'آپلود فایلان',
'right-reupload'             => 'هستین فایلی اوری بنویس',
'right-reupload-own'         => 'بنویس هستین فایلی که یک نفری آپلود بوتگین',
'right-reupload-shared'      => 'بنویس فایلانی که ته انبار میدیا شریکی ملکی انت',
'right-upload_by_url'        => 'فایل چه آدرس URL  آپلود کن',
'right-purge'                => 'پاک کتن ذخیره سایت په یک صفحه ای بی تایید',
'right-autoconfirmed'        => 'اصلاح کن صفحات نیم محافظتی آ',
'right-bot'                  => 'په داب یک پروسه اتوماتیکی زانگ بین',
'right-nominornewtalk'       => 'نداشتن هوردین اصلاح ته صفحات بحث یک نوکین کوله یانی پیش داریت',
'right-apihighlimits'        => 'استفاده کن چه بالاترین محدویتان ته جوستان API',
'right-writeapi'             => 'استفاده چه نوشتن API',
'right-delete'               => 'حذف صفحات',
'right-bigdelete'            => 'حذف صفحات گون درازین تاریح',
'right-deleterevision'       => 'حذف و حذف نه کتن مخصوصین بازبینی آن صفحات',
'right-deletedhistory'       => 'مداخل تاریح حذف بوتگین به گند، بی همراهی متن آیان',
'right-browsearchive'        => 'گردگ صفحات حذفی',
'right-undelete'             => 'حذف مکن یک صفحه ایء',
'right-suppressrevision'     => 'بازبینی و ترینگ بازبینی آن پناهین چه مدیران سیستم',
'right-suppressionlog'       => 'به گند خصوصی آماران',
'right-block'                => 'دگ کابران چه اصلاح محدود کن',
'right-blockemail'           => 'یک کاربری چه ایمیل دیم دهگ منع کن',
'right-hideuser'             => 'یک نام کاربری منع کن، آیی چه عام پناه کنگنت',
'right-ipblock-exempt'       => 'منع جنبی آی پی، منع اتوماتیکی و منع بردی',
'right-proxyunbannable'      => 'جنبی اتوماتیکی منع پروکسی',
'right-protect'              => 'سطوح محافظت عوض کن و اصلاح کن محافظتی صفحاتء',
'right-editprotected'        => 'اصلاح کن محافظتی صفحات (بی حفاظت آبشاری)',
'right-editinterface'        => 'دستبر کاربر اصلاح کن',
'right-editusercssjs'        => 'دگر کابرانی فایلان  CSS  و JS اصلاح کن',
'right-rollback'             => 'سریع برگردین اصلاحات آهری کاربر که یک بخصوصین صفحه ای اصلاح کتت.',
'right-markbotedits'         => 'نشان کن اصلاحات برگشتی په داب اصلاحات روباتی',
'right-noratelimit'          => 'تاثیر نهلیت گون محدودیاتان میزان',
'right-import'               => 'صفحات چه دگ ویکی ان وارد کن',
'right-importupload'         => 'صفحات چه یک آپلود فایل وارد کن',
'right-patrol'               => 'نشان کن اصلاحات دگرانء په دابی نظارت بوتگین',
'right-autopatrol'           => 'اتوماتیکی اصلاحات یکیء چه وتی نشان کن په داب نظارت بوتگین',
'right-patrolmarks'          => 'به گند نوکین تغییرات نشان نظارتی',
'right-unwatchedpages'       => 'به گند په داب یکیک لیست نچارتگین صفحات',
'right-trackback'            => 'یک رندگری دیم دی',
'right-mergehistory'         => 'چن وبند کن تاریح صفحاتء',
'right-userrights'           => 'اصلاح کل حقوق کاربری',
'right-userrights-interwiki' => 'اصلاح حقوق کابرانی کابران دگه ویکی انء',
'right-siteadmin'            => 'کبل و پچ دیتابیس',

# User rights log
'rightslog'      => 'ورودان حقوق کاربر',
'rightslogtext'  => 'شی یک آماری چه تغییرات په حقوق کاربری انت.',
'rightslogentry' => 'عوض بوت عضویت گروهی په $1  چه $2 په $3',
'rightsnone'     => '(هچ یک)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|تغییر|تغییرات}}',
'recentchanges'                     => 'نوکین تغییرات',
'recentchangestext'                 => 'رندگر نوکترین تغییرات ته ویکی تی ای صفحه.',
'recentchanges-feed-description'    => 'آهرین تغییرات ته وی کی چه ای فید رند گر',
'rcnote'                            => "جهلء{{PLURAL:$1|هست '''1''' تغییری|هستن آهری '''$1''' تغییرات}} ته آهرین {{PLURAL:$2|روچ|'''$2''' روچان}}, چه$5, $4.",
'rcnotefrom'                        => "جهلا تغییرات چه '''$2''' (تا  '''$1''' پیش دارگنت). هست",
'rclistfrom'                        => 'پیش دار نوکین تغییراتآ چه $1',
'rcshowhideminor'                   => '$1 هوردین تغییرات',
'rcshowhidebots'                    => '$1 روبوت',
'rcshowhideliu'                     => '$1 کاربران وارد بوتگین',
'rcshowhideanons'                   => '$1 نا شناسین کاربران',
'rcshowhidepatr'                    => '$1 اصلاحات کنترل بیتگین',
'rcshowhidemine'                    => '$1 اصلاحات من',
'rclinks'                           => 'پیش دار آهرین$1 تغییرات ته آهرین $2 روچان<br />$3',
'diff'                              => 'تفاوت',
'hist'                              => 'تاریخ',
'hide'                              => 'پناه',
'show'                              => 'پیش دراگ',
'minoreditletter'                   => 'م',
'newpageletter'                     => 'ن',
'boteditletter'                     => 'ب',
'number_of_watching_users_pageview' => '[$1 چارگنت {{PLURAL:$1|کاربر|کابران}}]',
'rc_categories'                     => 'محدودیت په دسته جات(دورش گون"|")',
'rc_categories_any'                 => 'هرچی',
'newsectionsummary'                 => '/* $1 */ نوکین بخش',

# Recent changes linked
'recentchangeslinked'          => 'مربوطین تغییرات',
'recentchangeslinked-title'    => 'تغییراتی مربوط په "$1"',
'recentchangeslinked-noresult' => 'هچ تغییری ته صفحات لینک بوتگین ته داتگین دوره نیست',
'recentchangeslinked-summary'  => "شی یک لیستی چه تغییراتی هستنت که نوکی اعمال بوتگنت په صفحاتی که چه یک صفحه خاصی لینک بوته( یا په اعضای یک خاصین دسته).
صفحات ته [[Special:Watchlist| شمی لیست چارگ]] '''' پررنگنت''''",
'recentchangeslinked-page'     => 'صفحه نام:',
'recentchangeslinked-to'       => 'پیش دار تغییرات په صفحاتی که لینک بوتگنت به جاه داتگین صفحه',

# Upload
'upload'                      => 'آپلود کتن فایل',
'uploadbtn'                   => 'آپلود فایل',
'reupload'                    => 'دگه آپلود',
'reuploaddesc'                => 'کنسل آپلودء و ترر په فرم آپلود',
'uploadnologin'               => 'وارد نهیت',
'uploadnologintext'           => 'شما بایدن [[Special:UserLogin|واردبیت]] په آپلود کتن فایل.',
'upload_directory_missing'    => 'مسیر آپلود ($1)  گارنت و گون وب سرور شر گنگ نه بیت.',
'upload_directory_read_only'  => 'مسیر آپلود ($1)  قابل نوشتن گون وب سرور نهنت.',
'uploaderror'                 => 'حطا آپلود',
'uploadtext'                  => "چه جهلگین فرم په آپلود فایلان استفاده کنت.
په دیستن یا گشتن پیشگین آپلودی فایلان برو  [[Special:ImageList|لیست فایلان آپلودی]], آپلودان و حذفیات هنچو هستن ته [[Special:Log/upload|آمار آپلود]].

په وارد کتن فایل ته یک صفحه ای، چه لینک ته فرم استفاده کن
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|alt text]]</nowiki></tt>''' په استفاده چه نسخه کامل فایل
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|alt text]]</nowiki></tt>''' په استفاده چه ۲۰۰ پیکسل پهنات ته یک جعبه ته چپ
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>'''  په مسیری لینک دهگ په فایل بی پیش دارگ فایل",
'upload-permitted'            => 'مجازین نوع فایل:  $1.',
'upload-preferred'            => 'ترجیحی نوع فایل:  $1.',
'upload-prohibited'           => 'غیرمجازین نوع فایل:  $1.',
'uploadlog'                   => 'آپلود آمار',
'uploadlogpage'               => 'آپلود ورودان',
'uploadlogpagetext'           => 'جهلء یک لیست چه نوکترین آپلودان قایل هست.
[[Special:NewImages|گالری نوکین فایلان]]',
'filename'                    => 'نام فایل',
'filedesc'                    => 'خلاصه',
'fileuploadsummary'           => 'خلاصه:',
'filestatus'                  => 'وضعیت حق کپی:',
'filesource'                  => 'منبع:',
'uploadedfiles'               => 'آپلود بوتگین فایلان',
'ignorewarning'               => 'هوژاری شموش و هرداب منته فایل ذخیره کن',
'ignorewarnings'              => 'هردابین هوژاری شموش',
'minlength1'                  => 'نام فایل باید یک حرفی حداقل بیت',
'illegalfilename'             => 'نام فایل  "$1"  شامل کاراکترانی انت که مجاز نهنت ته ای عناوین صفحه.
لطفا نام فایل بدل کنیت و دگه آپلود آزمایش کنیت.',
'badfilename'                 => 'نام فایل عوض بوتت په "$1".',
'filetype-badmime'            => 'فایلان نوع مایم  "$1" مجاز په آپلود بیگ نهنت.',
'filetype-unwanted-type'      => '\'\'\'".$1"\' یک نه لوٹتگین نوع فایل انت. انواع فایل ترجیحی  $2 انت.
نوع ترجیحی {{PLURAL:$3|نوع فایلنت|انواع فایلان انت}} $2.',
'filetype-banned-type'        => "'''\".\$1\"''' یک نوع مجازی ان فایلی نهنت. مجازین {{PLURAL:\$3|نوع فایل|انواع فایلان}}  \$2.",
'filetype-missing'            => ' (په داب ".jpg").فایل هچ بندی نیست',
'large-file'                  => 'توصیه بیت که فایلان مزنتر چه  $1 مبنت;
ای فایل $2 انت.',
'largefileserver'             => 'ای فایل مزنتر چه حدی انت که سرور تنظیم بوتت په اجازه دهگ.',
'emptyfile'                   => 'فایلی که شما آپلود کتت هالیک انت. شاید شی په خاطر اشتباه نه نام فایل بیت.
لطفا کتنرل کنیت که آیا واقعا شما لوٹیت ای فایلء آپلود کنیت.',
'fileexists'                  => 'یک فایل گون ای نام هستنت،لطفا کنترل کن <strong><tt>$1</tt></strong> اگه شما مطمین نهیت اگه لوٹیت نامی آیء عوض کنیت.',
'filepageexists'              => 'صفحه توضیح په ای فایل پیشتر شر بوتت ته <strong><tt>$1</tt></strong>, بله هچ فایلی گون ای نام هنو نیست.
خلاصه ای که شما وارد کت ته صفحه توضیح ظاهر نه بیت.
په ظاهر کتن خلاصه ادان شما لازمنت آیء دستی اصلاح کنیت.',
'fileexists-extension'        => 'یک فایلی گون یک دابی نام هستن:<br />
نام فایلی که آپلود بیت: <strong><tt>$1</tt></strong><br />
نام هستین فایل:<strong><tt>$2</tt></strong><br />
لطفا دگه نامی بزوریت.',
'fileexists-thumb'            => "<center>'''هستین فایل'''</center>",
'fileexists-thumbnail-yes'    => 'فایل به نظر رسیت که یک عکس هورد بوتگین اندازه انت. <i>(پنچی انگشت)</i>.
لطفا فایل کنترل کن  <strong><tt>$1</tt></strong>.<br />
اگر فایل کنترلی هما عکسنت گون اصلی اندازه لازم نهنت یک پنچ انگشتی گیشین آپلود کنیت.',
'file-thumbnail-no'           => 'نام فایل شروع بیت گون <strong><tt>$1</tt></strong>.
جاه کیت که یک هور بوتگین اندازه عکس ایت.<i>(پینچ انگشت)</i>.
اگر شما را ای عکس ته وضوح کامل هست ایء آپلود کنیت یا که نام فایل عوض کنیت لطفا',
'fileexists-forbidden'        => 'فایل گو ای نام الان هستنت؛
لطفا برگردیت و ای فایل گون یک نوکین نامی آپلود کنیت.[[Image:$1|انگشتی|مرکز|$1]]',
'fileexists-shared-forbidden' => 'یک فایلی گون ای نام الان ته منبع مشترک فایل هستن.
لطفا برگردیت و ای فایل گون نوکین نامی آپلود کنیت.[[Image:$1|انگشتی|مرکز|$1]]',
'file-exists-duplicate'       => 'ای فایل کپیء چه جهلیگین  {{PLURAL:$1|فایل|فایلان}}:',
'successfulupload'            => 'آپلود موفق',
'uploadwarning'               => 'هوژاری آپلود',
'savefile'                    => 'ذخیره فایل',
'uploadedimage'               => 'اپلود بوت "[[$1]]"',
'overwroteimage'              => 'یک نوکین نسخه چه "[[$1]]" آپلود بیتت',
'uploaddisabled'              => 'آپ.د غبر فعال انت',
'uploaddisabledtext'          => 'آپلود فایل غیر فعال انت.',
'uploadscripted'              => 'ای فایل شامل کد HTML یا اسکریپت انت که شاید گون وب بروزر اشتباهی وانگ بیت.',
'uploadcorrupt'               => 'ای فایل حرابنت یا اشتباهین بندی هست.
لطفا فایل کتنرل کنیت و دگه آپلود کنیت.',
'uploadvirus'                 => 'فایل یک ویروسی داریتن! جزییات: $1',
'sourcefilename'              => 'منبع نام فایل:',
'destfilename'                => 'مقصد نام فایل',
'upload-maxfilesize'          => 'آهرین هد اندازه فایل : $1',
'watchthisupload'             => 'ای صفحه بچار',
'filewasdeleted'              => 'یک فایلی گو ای نام پیشتر آپلود بوتت و رندا حذف بوت.
شما بایدن کنترل کنیت  $1 پیش چه شی که دگه آپلود کنیت.',
'upload-wasdeleted'           => "'''هوژاری: شما یک فایلی آپلود کنگیت که پیشتر حذف بوتت.'''

شما بایدن توجه کنیت که آیا ادامه دهگ آپلود کتن فایل مناسبنت.
آمار حذف فایل په ای فایل ادان په شمی حاطرء هست:",
'filename-bad-prefix'         => 'نام  فایلی که آپلود بیت شروع بیت گون <strong>"$1"</strong>, که یک نام بی توضیحی هنچکا اتوماتیکی گون دوربین دیجیتال دهگ بوتت.
لطفا یک تشریحی ترین نامی په وتی فایل بزرویت.',
'filename-prefix-blacklist'   => '#<!-- leave this line exactly as it is --> <pre>
# Syntax is as follows:
#   * Everything from a "#" character to the end of the line is a comment
#   * Every non-blank line is a prefix for typical file names assigned automatically by digital cameras
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # some mobil phones
IMG # generic
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- leave this line exactly as it is -->',

'upload-proto-error'      => 'اشتباه پروتوکل',
'upload-proto-error-text' => 'آپلود دراین نیاز په URL آنی داریت که شروع بیت گون  <code>http://</code> یا <code>ftp://</code>.',
'upload-file-error'       => 'حطا درونی',
'upload-file-error-text'  => 'یک حطای درونی پیش اتک وهد شرکتن فایل موقت ته سرور.
لطفا گون یک [[Special:ListUsers/sysop|مدیر]].تماس گریت.',
'upload-misc-error'       => 'ناشناس حطا آپلود',
'upload-misc-error-text'  => 'یک ناشناسین حطا وهد آپلود کتن پیش آتک.
لطفا تایید کنیت که URL معتبرانت و دسترسی بیت و دگه سعی کنیت.
اگر مشکل ادامه داشت، گون [[Special:ListUsers/sysop|مدیر]]ء تماس گریت.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'نه تونیت په URL برسیت',
'upload-curl-error6-text'  => 'داتگین URL دسترسی نه بیت.
لطفا دگه کنترل کنیت که URL درستنت و سایت په راه انت.',
'upload-curl-error28'      => 'وهد آپلود هلت',
'upload-curl-error28-text' => 'سایت باز وهدی بورت په جواب دهگء.
لطفا کنترل کنیت که سایت روشننت، کمی صبر کنیت و دگه سعی کنیت.
شاید شما یک وهد حلوت تری کوشش کنیت.',

'license'            => 'لیسانس کنت:',
'nolicense'          => 'هچ یک انتخاب نه بوتت',
'license-nopreview'  => '(بازبین موجود نهنت)',
'upload_source_url'  => '(یک متعبرین، عمومی دسترسی انت URL)',
'upload_source_file' => '(یک فایل ته شمی کامپیوتر)',

# Special:ImageList
'imagelist-summary'     => 'ای حاصین صفحه کل آپلودی فایلانء پیش داریت.
به طور پیش فرض اهری فایل آپلودی ته بالاد لیست پیش دارگ بیت.
یک کلیکی بالاد ستون ترتیب عوض کنت.',
'imagelist_search_for'  => 'گردگ په  مدیا:',
'imgfile'               => 'فایل',
'imagelist'             => 'لیست فایل',
'imagelist_date'        => 'تاریح',
'imagelist_name'        => 'نام',
'imagelist_user'        => 'کاربر',
'imagelist_size'        => 'اندازه',
'imagelist_description' => 'توضیح',

# Image description page
'filehist'                       => 'تاریح فایل',
'filehist-help'                  => 'اور تاریح/زمان کلیک کنیت دان فایلا په داب هما تاریح بگندیت',
'filehist-deleteall'             => 'کل حذف',
'filehist-deleteone'             => 'حذف',
'filehist-revert'                => 'واتر',
'filehist-current'               => 'هنو',
'filehist-datetime'              => 'تاریح/زمان',
'filehist-user'                  => 'کاربر',
'filehist-dimensions'            => 'جنبه یان',
'filehist-filesize'              => 'اندازه فایل',
'filehist-comment'               => 'نظر',
'imagelinks'                     => 'لینکان',
'linkstoimage'                   => 'جهلیگین {{PLURAL:$1|صفحه |$1 صفحات لینک}} پی ای فایل',
'nolinkstoimage'                 => 'هچ صفحه ای نیست که به ای فایل لینک بوت.',
'morelinkstoimage'               => 'View [[Special:WhatLinksHere/$1|گیشتر لینکان]]به ای فایل',
'redirectstofile'                => 'جهلیگین {{PLURAL:$1|فایل غیر مستقیم بنت|$1 فایلان غیر مستقیم بنت.}} به ای فایل',
'duplicatesoffile'               => 'جهلیگین {{PLURAL:$1|فایل یک کپی انت|$1 فایلان کپی انت}} چه هی فایل:',
'sharedupload'                   => 'ای فایل یک مشترکین آپلودی فایلیت و شاید گون دگه پروژه یان استفاده بیت.',
'shareduploadwiki'               => 'لطفا بجار  $1 په گیشترین اطلاعات',
'shareduploadwiki-desc'          => 'توضیح  $1 ایء ته منبع شریکی چهلء پیش دارگ بیت.',
'shareduploadwiki-linktext'      => 'صفحه توضیح فایل',
'shareduploadduplicate'          => 'ای فایل کپی  جه  $1 چه منیع شریکی  انت.',
'shareduploadduplicate-linktext' => 'دگه فایلی',
'shareduploadconflict'           => 'فایل یک دابی نامی چوش  $1 هست چه منبع شریکی.',
'shareduploadconflict-linktext'  => 'دگه فایلی',
'noimage'                        => 'چوشین فایل گون ای نام نیست، بله شما تونیت $1',
'noimage-linktext'               => 'یکیء آپلود کن',
'uploadnewversion-linktext'      => 'یک نوکین نسخه ای چه ای فایل آپلود کن',
'imagepage-searchdupe'           => 'گردگ په کپی  فایلان',

# File reversion
'filerevert'                => 'ترین $1',
'filerevert-legend'         => 'ترینگ فایل',
'filerevert-intro'          => " شما په ترینگء '''[[Media:$1|$1]]''' په  [$4 نسخه ای په داب چه $3, $2].",
'filerevert-comment'        => 'نظر:',
'filerevert-defaultcomment' => 'تررت په نسخه په داب $2, $1',
'filerevert-submit'         => 'تررگ',
'filerevert-success'        => "''[[Media:$1|$1]]'''  بدل بوتت په [$4 نسخه په داب چه $3, $2].",
'filerevert-badversion'     => 'چه ای فایل پیشگین نسخه مکلی گون داتگین وهد نیست.',

# File deletion
'filedelete'                  => 'حذف $1',
'filedelete-legend'           => 'حذف فایل',
'filedelete-intro'            => "شما حذف کنگت ''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => " شما په حذف کتن نسخه ای چه '''[[Media:$1|$1]]''' په داب چه [$4 $3, $2].",
'filedelete-comment'          => 'دلیل په حذف:',
'filedelete-submit'           => 'حذف',
'filedelete-success'          => "'''$1''' حذف بوت.",
'filedelete-success-old'      => '<span class="plainlinks">نسخه چه \'\'\'[[Media:$1|$1]]\'\'\'  په داب چه $3, $2 حذف بوتت.</span>',
'filedelete-nofile'           => "'''$1'''  موجود نهنت.",
'filedelete-nofile-old'       => "هچ نسخه آرشیوی چه'''$1'''  گون مشخصین نشان نیست.",
'filedelete-iscurrent'        => 'شما لوٹیت نوکترین نسخه ای فایلء حذف کنیت.
لطفا اول په یک پیشرتین نسخه بدل کنیت.',
'filedelete-otherreason'      => 'دگر/گیشترین دلیل:',
'filedelete-reason-otherlist' => 'دگ دلیل',
'filedelete-reason-dropdown'  => '*متداول این دلایل حذف
** نقص حق کپی
** فایل کپی',
'filedelete-edit-reasonlist'  => 'اصلاح دلایل حذف',

# MIME search
'mimesearch'         => 'گردگ په مایم',
'mimesearch-summary' => 'ای صفحه فیلتر کتن فایلان په اساس نوع مایم اش فعال کنت.
ورودی:متحوانوع/زیرنوع،مثل<tt>image/jpeg</tt>.',
'mimetype'           => 'نوع مایم:',
'download'           => 'آیرگیزگ',

# Unwatched pages
'unwatchedpages' => 'نه چارتگین صفحات',

# List redirects
'listredirects' => 'لیست غیر مستقیمان',

# Unused templates
'unusedtemplates'     => 'تمپلتان بی استفاده',
'unusedtemplatestext' => 'ای صفحه لیست کن کل صفحات ته تمپلت نام فضا که ته دگه صفحه نهنت.
مه شموش تا کنترل کنیت په دگه لینکان ته تمپلتان پیش چه حذف کتن آیان.',
'unusedtemplateswlh'  => 'دگر لینکان',

# Random page
'randompage'         => 'تصادفی صفحه',
'randompage-nopages' => 'هچ صفحه ای ته ای نام فضا نیست.',

# Random redirect
'randomredirect'         => 'تصادفی غیر مستقیم',
'randomredirect-nopages' => 'هچ غیر مستقیمی ته ای نام فضا نیست.',

# Statistics
'statistics'             => 'آمار',
'sitestats'              => '{{SITENAME}} آمار',
'userstats'              => 'آمار کاربر',
'sitestatstext'          => "ادان {{PLURAL:\$1|هست '''1'''صفحه|هستن '''\$1''' کل صفحات}}تی ای دیتابیس.
شی شامل صفحات  \"talk\"، صفحاتی درباره {{SITENAME}}، هوردین صفحات\"stub\"، غیر مستقیمان و دگه چیز که بلکین لایق صفحات محتوا نهنت.
غیر چه آیآن، ادان {{PLURAL:\$2|هست '''1''' صفحه که هست یک|هستن '''\$2'''صفحاتی  که هستن}}شاید محتوای قانونی {{PLURAL:\$2|صفحه|صفحات}}.

'''\$8''' {{PLURAL:\$8فایل |فایلان}} آپلود بوتنت.
ادان کلا '''\$3''' {{PLURAL:\$3|چارگ صفحه|چارگ صفحه}}, و '''\$4''' {{PLURAL:\$4|اصلاح صفحه|اصلاحات صفحه}} چه {{SITENAME}} بنگیج بوتت.
شامل '''\$5''' اصلاح میانگین ته هر صفحه و  '''\$6''' دیستن ته هر اصلاح بیت.

[http://www.mediawiki.org/wiki/Manual:Job_queue job queue] طولی انت '''\$7'''.",
'userstatstext'          => "ادان {{PLURAL:$1هست '''1''' ثبت بوتگین[[Special:ListUsers|کاربر]]|هستن '''$1''' ثبت نامی [[Special:ListUsers|کابران]]}}, که چه آیء '''$2''' (یا'''$4%''') {{PLURAL:$2|داریت|دارنت}} $5 حقوقی.",
'statistics-mostpopular' => 'باز چار تگین صفحات',

'disambiguations'      => 'صفحات رفع ابهام',
'disambiguationspage'  => 'Template:رفع ابهام',
'disambiguations-text' => "جهلیگین صفحه لینک انت په یک '''صفحه رفع ابهام'''.
شما بایدن په جاه آیی په یک مناسبین موضوعی لینک دهیت.<br />
یک صفحه ای که په داب صفحه رفع ابهام چارگ بیت اگر آیء چه یک تمپلتی که لینک بیت چه [[MediaWiki:Disambiguationspage|صفحه رفع ابهام]] استفاده کنت.",

'doubleredirects'            => 'دوبل غیر مستقیم',
'doubleredirectstext'        => 'ای صفحه لیست کنت صفحاتی که غیر مستقیم رونت په دگه صفحات. هر ردیف شامل لینکانی انت به اولی و دومی غیر مستقیم، و هدف دومی غیر مستقیم، که معمولا استفاده بیت "real" صفحه هدف، که بایدن اولی غیر مستقیم پیش داریت.',
'double-redirect-fixed-move' => '[[$1]] انتقال دهگ بوتت، و الان تغییر مسیری په [[$2]] انت',
'double-redirect-fixer'      => 'تعمیرکنوک غیر مستقیم',

'brokenredirects'        => 'پروشتگین غیر مستقیمان',
'brokenredirectstext'    => 'جهلیگین غیر مستقیم لینک بوتگن په صفحات نیستن:',
'brokenredirects-edit'   => '(اصلاح)',
'brokenredirects-delete' => '(حذف)',

'withoutinterwiki'         => 'صفحاتی بی لینکان زبان',
'withoutinterwiki-summary' => 'جهلیگین صفحات په دگه نسخه آن زبان لینک نه بوتت:',
'withoutinterwiki-legend'  => 'پیشوند',
'withoutinterwiki-submit'  => 'پیش دار',

'fewestrevisions' => 'صفحات گون کمترین بازبینی',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|بایت|بایت}}',
'ncategories'             => '$1 {{PLURAL:$1|دسته|دسته جات}}',
'nlinks'                  => '$1 {{PLURAL:$1|link|لینک}}',
'nmembers'                => '$1 {{PLURAL:$1|member|اعضا}}',
'nrevisions'              => '$1 {{PLURAL:$1|بازبینی|بازبینی ان}}',
'nviews'                  => '$1 {{PLURAL:$1|دیستن|دیستن}}',
'specialpage-empty'       => 'په ای گزارش هچ نتیجه ای نیست ان.',
'lonelypages'             => 'صفحات یتیم',
'lonelypagestext'         => 'جهلیگین صفحات چه دگه صفحات لینک نه بوتگنت ته {{SITENAME}}.',
'uncategorizedpages'      => 'صفحات بی دسته',
'uncategorizedcategories' => 'دسته جات دسته بندی نه بوتگین',
'uncategorizedimages'     => 'فایلان بی دسته',
'uncategorizedtemplates'  => 'تمپلتان بی دسته',
'unusedcategories'        => 'بی استفاده این دسته جات',
'unusedimages'            => 'بی استفاده این فایلان',
'popularpages'            => 'مردمی صفحات',
'wantedcategories'        => 'لوٹتگین دسته جات',
'wantedpages'             => 'لوٹتگین صفحات',
'missingfiles'            => 'گارین فایلان',
'mostlinked'              => 'صفحاتی که گیشنر لینک دیگ بیتگنت',
'mostlinkedcategories'    => 'دسته جاتی که گیشتر لینک دیگ بیتگنت',
'mostlinkedtemplates'     => 'تمپلتانی که گیشتر لینک بیتگنت',
'mostcategories'          => 'صفحات گون گیشترین دسته جات',
'mostimages'              => 'فایلان گیشنر لینک بوتیگن',
'mostrevisions'           => 'صفحاتی گون گیشترین بازبینی',
'prefixindex'             => 'اندیکس پیش وند',
'shortpages'              => 'هوردین صفحه',
'longpages'               => 'صفحات مزنین',
'deadendpages'            => 'مرتگین صفحات',
'deadendpagestext'        => 'جهلیگین صفحات په صفحات دگر لینک نهنت ته {{SITENAME}}.',
'protectedpages'          => 'صفحات حفاظت بیتگین',
'protectedpages-indef'    => 'فقط محافظت نامحدود',
'protectedpagestext'      => 'جهلیگین صفحات محافظت بوتگین چه اصلاح و جاه په جاه بیگ',
'protectedpagesempty'     => 'هچ صفحه ای گون ای پارامترآن',
'protectedtitles'         => 'عناوین محافظتی',
'protectedtitlestext'     => 'جهلیگین عناوین چه شر بیگ محافظتن',
'protectedtitlesempty'    => 'هچ عنوانی هنو گو ای پارامتران محافظت نهنت.',
'listusers'               => 'لیست کاربر',
'newpages'                => 'نوکین صفحات',
'newpages-username'       => 'نام کاربری:',
'ancientpages'            => 'صفحات قدیمی',
'move'                    => 'جاه په جاه',
'movethispage'            => 'ای صفحه جاه په جاه کن',
'unusedimagestext'        => 'لطفا توجه کنیت که دگه وب سایتان شاید گون یک مستقیم URL لینک بیت و شاید هنگن ادان لیست بوتت جدا چه شی که ته استفاده فعال انت.',
'unusedcategoriestext'    => 'جهلیگین دسته ی صفحات هستنت بله هچ صفحه یا دسته ای چه آیان استفاده نکت.',
'notargettitle'           => 'هچ هدف',
'notargettext'            => 'شما یک ضفحه یا کاربر مقصد مشخص نه کتت په اجرا کتن ای عمل اور آیی.',
'nopagetitle'             => 'نی چوشین صفحه مقصد',
'nopagetext'              => 'صفحه مقصدی که شما مشخص کتت موجود نهنت.',
'pager-newer-n'           => '{{PLURAL:$1|نوکتر 1|نوکتر $1}}',
'pager-older-n'           => '{{PLURAL:$1|قدیمیتر 1|قدیمیتر $1}}',
'suppress'                => 'رویت',

# Book sources
'booksources'               => 'منابع کتاب',
'booksources-search-legend' => 'گردگ په منابع کتاب',
'booksources-isbn'          => 'شابک:',
'booksources-go'            => 'برو',
'booksources-text'          => 'چهلا یک لیستی چه لینکان په دگه سایتان هست که نوکین  یا مستعمل این کتاب بها کنند و شما شاید گیشترین اطلاعات آی کتابانی باره که پرش گردیت در گیزیت:',

# Special:Log
'specialloguserlabel'  => 'کاربر:',
'speciallogtitlelabel' => 'عنوان:',
'log'                  => 'ورودان',
'all-logs-page'        => 'کل ورودان',
'log-search-legend'    => 'گردگ په آمار',
'log-search-submit'    => 'برو',
'alllogstext'          => 'هور کت پیش دارگ کل موجودین آمار {{SITENAME}}.
شما تونیت گون انتخاب یک نوع آمار،نام کاربر (حساس په هورد-مزنی)، یا متاثرین صفحه (هنچوش حساس په هورد-مزنی) کمتری کنیت.',
'logempty'             => 'هچ آیتم هم دپ ته آمار',
'log-title-wildcard'   => 'بگرد عناوین که گون ای متن شروع بنت',

# Special:AllPages
'allpages'          => 'کل صفحات',
'alphaindexline'    => '$1 په $2',
'nextpage'          => 'صفحه ی بعدی ($1)',
'prevpage'          => ' ($1)پیشگین صفحه',
'allpagesfrom'      => 'پیش در صفحات شروع بنت ته:',
'allarticles'       => 'کل صفحات',
'allinnamespace'    => 'کل صفحات($1 نام فضا)',
'allnotinnamespace' => 'صفحات کل (ته نام فضا $1 نه)',
'allpagesprev'      => 'پیشگین',
'allpagesnext'      => 'بعدی',
'allpagessubmit'    => 'برو',
'allpagesprefix'    => 'صفحات پیش دار گون پیشوند:',
'allpagesbadtitle'  => 'داتگین عنوان صفحه نامعتبر انت یا  یک پیشوند بین ویکی یا یبن زبانی سحتی هستت.
شاید شامل یک یا گیشتر کاراکتر بیت که ته عنوانین استفاده نه بیت.',
'allpages-bad-ns'   => '{{SITENAME}} فضانامی نیست "$1".',

# Special:Categories
'categories'                    => 'دسته یان',
'categoriespagetext'            => 'جهلیگین دسته جات شامل صفحات یا مدیا انت
[[Special:UnusedCategories|دسته جات بی استفاده]] ادان پیشدارگ نه بنت.
 هنچوش بچار[[Special:WantedCategories|wanted categories]].',
'categoriesfrom'                => 'پیشدار دسته جات که شروع بنت گون:',
'special-categories-sort-count' => 'ترتیب په اساس شمار',
'special-categories-sort-abc'   => 'ترتیب الفبی',

# Special:ListUsers
'listusersfrom'      => 'پیشدار کابرانی که شروع بنت گون:',
'listusers-submit'   => 'پیش دار',
'listusers-noresult' => 'هچ کابری در گیزگ نه بوت.',

# Special:ListGroupRights
'listgrouprights'          => 'حقوق گروه کاربر',
'listgrouprights-summary'  => 'جهلیگین یک لیستی چه گروهان کاربری تعریف بوتگین ته ای ویکی انت گون آیانی حق دسترسی آن همراهنت.
 درباره هر حقی ته صفحه [[{{MediaWiki:Listgrouprights-helppage}}|گیشترین اطلاعات]] هستن.',
'listgrouprights-group'    => 'گروه',
'listgrouprights-rights'   => 'حقوق',
'listgrouprights-helppage' => 'Help: حقوق گروه',
'listgrouprights-members'  => '(لیست اعضا)',

# E-mail user
'mailnologin'     => 'هچ آدرس دیم دهگ',
'mailnologintext' => 'شما بایدن [[Special:UserLogin|وارد بیت]] و یک معتبرین آدرس ایمیلی داشته بیت ته وتی [[Special:Preferences|ترجیحات]] په دیم داتن ایمیل په دگه کاربران',
'emailuser'       => 'په ای کابر ایمیل دیم دی',
'emailpage'       => 'ایمیل کاربر',
'emailpagetext'   => 'اگر ای کاربر یک معتبرین آدرس ایمیلی ته وتی ترجیحات کاربری وارد کتت،جهلگین فرم په آیء یک کوله ای دیم دنت.
آدرس ایمیلی که شما وارد کتت ته [[Special:Preferences|وتی ترجیحات]] په داب آدرس  "From" پیش دارگ بیت، اچه گروک ایمیل تونیت پسوء دنت.',
'usermailererror' => 'شی ایمیل حطا پیش داشت',
'defemailsubject' => '{{SITENAME}} ایمیل',
'noemailtitle'    => 'هچ آدرس ایمیل',
'noemailtext'     => 'ای کاربر یک آدرس ایمیل معتبری مشخص نه کتت یا انتخابی کت ای که چه دگه کابران ایمیل مه گریت.',
'emailfrom'       => ':چه',
'emailto'         => 'به:',
'emailsubject'    => 'موضوع:',
'emailmessage'    => 'کوله:',
'emailsend'       => 'دیم دی',
'emailccme'       => 'یک کپی چه منی کوله په من وت ایمیل کن.',
'emailccsubject'  => 'کپی چه شمی کوله په $1: $2',
'emailsent'       => 'ایمیل دیم دهگ بوت',
'emailsenttext'   => 'شمی کوله ایمیل دیم دهگ بوت.',
'emailuserfooter' => 'این نامه الکترونیکی گون استفاده چه ویژگی «پست الکترونیکی به کاربر» {{SITENAME}} گون $1 په $2 دیم دهگ بوتت.',

# Watchlist
'watchlist'            => 'منی لیست چارگ',
'mywatchlist'          => 'منی لیست چارگ',
'watchlistfor'         => "(په '''$1''')",
'nowatchlist'          => 'شما را هچ چیزی ته وتی لیست چارگ نیست.',
'watchlistanontext'    => 'لطفا  $1 په دیستن یا اصلاح ایتیمان ته وتی لیست چارگء',
'watchnologin'         => 'وارد نه بی تگیت',
'watchnologintext'     => 'شما بایدن  [[Special:UserLogin|وارد بیت]] په تغییر داتن وتی لیست چارگء',
'addedwatch'           => 'په لیست چارگ هور بوت',
'addedwatchtext'       => 'صفحه  "[[:$1]]"  په شمی [[Special:Watchlist|watchlist]] هور بیت.
دیمگی تغییرات په ای صفحه و آیاء صفحه گپ ادان لیست بنت، و صفحه پررنگ جاه کیت ته [[Special:RecentChanges|لیست نوکیت تغییرات]] په راحتر کتن شی که آی زورگ بیت.',
'removedwatch'         => 'چه لیست چارگ زورگ بیت',
'removedwatchtext'     => 'صفحه"[[:$1]]"  چه [[Special:Watchlist|شمی لیست چارگ]]. دربیت.',
'watch'                => 'به چار',
'watchthispage'        => 'ای صفحه ی بچار',
'unwatch'              => 'نه چارگ',
'unwatchthispage'      => 'چارگ بند کن',
'notanarticle'         => 'یک صفحه محتوا نهت',
'notvisiblerev'        => 'بازبینی حذف بوتت',
'watchnochange'        => 'هچ یک چه شمی چارتگین آیتم اصلاح نه بوتت ته ای دوره زمانی که پیش دارگ بیت.',
'watchlist-details'    => '{{PLURAL:$1|$1 صفحه|$1 صفحات}} چارتگ بیت صفحات گپ حساب نه بیگن',
'wlheader-enotif'      => '* اخطار ایمیل فعالنت.',
'wlheader-showupdated' => "* صفحات که عوض بوتگنت چه شمی آهری چارتن '''پررنگ''' پیش دراگ بنت.",
'watchmethod-recent'   => 'کنترل نوکین اصلاحات په صفحاتی که چارگ بنت',
'watchmethod-list'     => 'کنترل صفحاتی که چارگ بنت په نوکین اصلاحات',
'watchlistcontains'    => 'شمی لیست چارگ شامل  $1 {{PLURAL:$1|صفحه|صفحات}}.',
'iteminvalidname'      => "مشکل گون آیتم  '$1', نامعتبر  این نام",
'wlnote'               => "جهلء {{PLURAL:$1|آهرین تغییر هست|آهرین هست'''$1''' تغییرات}} ته آهرین {{PLURAL:$2|ساعت|'''$2''' ساعات}}.",
'wlshowlast'           => 'پیش دار آهرین $1  ساعات $2 روچان $3',
'watchlist-show-bots'  => 'پیش دار اصلاحات روباتء',
'watchlist-hide-bots'  => 'اصلاحات بت پناه کن',
'watchlist-show-own'   => 'پیش دار منی اصلاحاتء',
'watchlist-hide-own'   => 'منی اصلاحات آ پناه کن',
'watchlist-show-minor' => 'پیش دار هوردین اصلاحاتء',
'watchlist-hide-minor' => 'هوردین تغییرات پناه کن',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'چارگ بین',
'unwatching' => 'نه چارگ بیت',

'enotif_mailer'                => '{{SITENAME}} ایمیل دیم دهوک اخطاری',
'enotif_reset'                 => 'نشان کن کل صفحات په داب چارتگین',
'enotif_newpagetext'           => 'شی یک نوکین صفحه ایت.',
'enotif_impersonal_salutation' => '{{SITENAME}} کاربر',
'changed'                      => 'عوض بوت.',
'created'                      => 'شربوتت',
'enotif_subject'               => '{{SITENAME}} صفحه $PAGETITLE بوتت $CHANGEDORCREATED گون $PAGEEDITOR',
'enotif_lastvisited'           => 'بچار  $1 په کلین تغییرات چه شمی آهری چارگ.',
'enotif_lastdiff'              => 'بچار $1 په گندگ ای تغییر.',
'enotif_anon_editor'           => 'ناشناس کاربر $1',
'enotif_body'                  => 'آزیزین $WATCHINGUSERNAME،

صفحه {{SITENAME}} $PAGETITLE بوتت $CHANGEDORCREATED ته  $PAGEEDITDATE گون $PAGEEDITOR، بچار $PAGETITLE_URL په هنوکین نسخه.

$NEWPAGE

خلاصهء اصلاح کنوک:$PAGESUMMARY $PAGEMINOREDIT

تماس گر گون اصلاح کنوک:
ایمیل:$PAGEEDITOR_EMAIL
ویکی: $PAGEEDITOR_WIKI

دگه گیشترین اخطار په تغییرات دگه دیم دهگ نه بوت مگر شی که شما ای صفحه بچاریت.
شما تونیت هنچوش نشانآن اخطارء ته وتی لیست چارگ په کلی چارتگین صفحات تنظیم کنیت.

شمی دوستین سیستم اخطار {{SITENAME}}

--
په عوض کتن تنظیمات وتی لیست چارگ،به چار
{{fullurl:{{ns:special}}:Watchlist/edit}}

نظرات و گیشترین کمک:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'حذف صفحه',
'confirm'                     => 'تایید',
'excontent'                   => "محتوا هستنت:  '$1'",
'excontentauthor'             => "محتوا ات: '$1' (و  فقط شرکت کنندگان انت '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "محتوا پیش چه صاف بیگ بوتت : '$1'",
'exblank'                     => 'صفحه هالیک انت',
'delete-confirm'              => 'حذف "$1"',
'delete-legend'               => 'حذف',
'historywarning'              => 'هوژاری: صفحه ای که شما لوٹتیت آیآ حذف کنیت یک تاریحی داریت:',
'confirmdeletetext'           => 'شما لوٹیت یک صفحه ای گون کل تاریحانی حذف کنیت.
لطفا تایید کنیت که شما چوش کنیت که شما زانیت آی ء عاقبتانآ و شی که شما ای کارآ گون [[{{MediaWiki:Policy-url}}|سیاست]] انجام دهیت',
'actioncomplete'              => 'کار انجام بیت',
'deletedtext'                 => '"<nowiki>$1</nowiki>" حذف بیت.
بگندیت $2 په ثبتی که نوکین حذفیات',
'deletedarticle'              => 'حذف بوت "[[$1]]"',
'suppressedarticle'           => 'متوقف بوت "[[$1]]"',
'dellogpage'                  => 'حذف ورودان',
'dellogpagetext'              => 'جهلء یک لیستی چه نوکترین حذفیات هست.',
'deletionlog'                 => 'آمار حذف',
'reverted'                    => 'ترینگ بوت په پیشترین بازبینی',
'deletecomment'               => 'دلیل حذف:',
'deleteotherreason'           => 'دگه/گیشترین دلیل:',
'deletereasonotherlist'       => 'دگه دلیل',
'deletereason-dropdown'       => '*متداولین دلایل حذف
** درخواست نویسوک
** نقض حق کپی
** حرابکاری',
'delete-edit-reasonlist'      => 'اصلاح کن دلایل حذفء',
'delete-toobig'               => 'صفحهء یک مزنین تاریح اصلاحی هست گیشتر چه $1 {{PLURAL:$1|بازبینی|بازبینی}}.
حذف چوشین صفحات په خاظر جلو گر چه ناگهانی اتفاق ته سایت {{SITENAME}} ممنوع بوتت.',
'delete-warning-toobig'       => 'ای صفحه  مزنین تاریح اصلاح هست، گیش چه  $1 {{PLURAL:$1|بازبینی|بازبینی}}.
حذف آی شاید کار دیتابیس  {{SITENAME}} قطع کنت؛
گون اخطار پیش روت.',
'rollback'                    => 'پشت ترگ اصلاحات',
'rollback_short'              => 'پشتررگ',
'rollbacklink'                => 'عقب ترگ',
'rollbackfailed'              => 'پشتررگ پروشت',
'cantrollback'                => 'نه تونیت اصلاح برگردینیت؛
آهری شرکت کننده فقط نویسوک ای صفحه انت.',
'alreadyrolled'               => 'نه تونیت ترینیت اهری اصلاح چه  [[:$1]] گون  [[User:$2|$2]] ([[User talk:$2|گپ]] | [[Special:Contributions/$2|{{int:contribslink}}]]);
یکی دگه پیش تر صفحهء اصلاح کتت یا بری گردینت.

آهری اصلاح توسط [[User:$3|$3]] ([[User talk:$3|گپ کن]]).',
'editcomment'                 => 'نظر اصلاح ات:"<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'ترینت اصلاحات توسط  [[Special:Contributions/$2|$2]] ([[User talk:$2|گپ کن]])په آهری بازبینی گون [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'اصلاحات ترینگ بوتت گون $1;
په آهری نسخه ترینگ بوتنت گون $2.',
'sessionfailure'              => 'چوش جاه کیت که گون شمی نند  وارد بییگ مشکلی هست;
ای کار په خاطر سوء استفاده چه نندگ کنسل بوت.
لطفا بجنیت "back" و صفحه ای که چه آیء اتکگیت دگه بار کنیت او راندء دگه تلاش کنیت.',
'protectlogpage'              => 'ورودان حفاظت',
'protectlogtext'              => 'جهلء یک لیستی چه هست چه صفحه پچ و بند کبلان.
بچار [[Special:ProtectedPages|لیست صفحات محافظتی]]  په لیستی محافظتات اجرای هنوکین صفحه',
'protectedarticle'            => 'محافظتی "[[$1]]"',
'modifiedarticleprotection'   => 'عوض بوت سطح حفاظت په "[[$1]]"',
'unprotectedarticle'          => 'بی حمایت  "[[$1]]"',
'protect-title'               => 'عوض کن سطح حفاظت په  "$1"',
'protect-legend'              => 'حفاظت تایید کن',
'protectcomment'              => 'نظر:',
'protectexpiry'               => 'منقضی بیت:',
'protect_expiry_invalid'      => 'تاریح انقضای معتبر نهنت.',
'protect_expiry_old'          => 'تاریخ انقصا ته گذشته انت.',
'protect-unchain'             => 'اجازه یان جاه په جاهی پچ کن',
'protect-text'                => 'شما شاید ادان سطح حفاظت بگندیت و تغییر دیهت په صفحه <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'شما نه تونیت سطوح حفاظت وهدی مه محدود انت عوض کنیت.
ادان تنظیمات هنوی په صفحه است<strong>$1</strong>:',
'protect-locked-dblock'       => 'سطوح حفاظتی په خاطر یم فعالین کبل دیتابیس عوض نه  بنت.
ادان تنظیمات هنوی په صفحه است <strong>$1</strong>:',
'protect-locked-access'       => 'شمی حساب اجازه نداریت سطوح حفاظت صفحه ی عوض کنت.
ادان هنوکین تنظیمات هست په صفحه <strong>$1</strong>:',
'protect-cascadeon'           => 'ای صفحه الان محافظت بیت چوش که آی شامل جهلی {{PLURAL:$1|صفحات| درانت  که }} حفاظت آبشار روشن.
شما تونیت ای صفحه ی سطح حفاظت آ عوص کنیت، بله آی ء حفاظت آبشاریء تاثیر نهلیت.',
'protect-default'             => '(پیش فرض)',
'protect-fallback'            => 'اجازه "$1" لازم داریت',
'protect-level-autoconfirmed' => 'کابران ثبت نام نه بوتگینآ محدود کن',
'protect-level-sysop'         => 'فقط کاربران سیستمی',
'protect-summary-cascade'     => 'آبشاری',
'protect-expiring'            => 'منقضی بوت $1 (UTC)',
'protect-cascade'             => 'حفاظت کن صفحاتی په داب ای صفحه (محافظت آبشاری)',
'protect-cantedit'            => 'شما نه تونیت سطح حمایت ای صفحه یا عوض کنیت، چون شما اجازه اصلاح کتن نیست',
'restriction-type'            => 'اجازت',
'restriction-level'           => 'سطح محدود',
'minimum-size'                => 'هوردی اندازه',
'maximum-size'                => 'مزنی اندازه',
'pagesize'                    => '(بایت)',

# Restrictions (nouns)
'restriction-edit'   => 'اصلاح',
'restriction-move'   => 'جاه په جاه کن',
'restriction-create' => 'شرکتن',
'restriction-upload' => 'آپلود',

# Restriction levels
'restriction-level-sysop'         => 'محافظتی کامل',
'restriction-level-autoconfirmed' => 'نیمه محافظتی',
'restriction-level-all'           => 'هر سطحی',

# Undelete
'undelete'                     => 'به گند صفحات حذفی',
'undeletepage'                 => 'به گند و برگردین صفحات حذفیء',
'undeletepagetitle'            => "'''جهلیگین شامل حذف بوتگین بازبینی آننت چه  [[:$1|$1]]'''.",
'viewdeletedpage'              => 'به گند صفحات حذفیء',
'undeletepagetext'             => 'جهلیگین صفحات حذف بوتگنت بله هنگیت ته آرشیو هستن و توننت برگردینگ بنت.
آرشیو شاید هر چند وهد پهک کنگ بیت.',
'undelete-fieldset-title'      => 'ترینگ بازبینی  ان',
'undeleteextrahelp'            => "په ترینگ کل صفحه، کل جعبه انتخاب مه کن و کلیک کن  '''''تررین'''''.
په اجرا کتن تررینگ انتخابی جعبه هانی که مطابق بازبینی آن باید تررینگ بیت نشان بلیت، و کلیک کنیت '''''تررین''''. کلیک کتن '''''دیگه نندینگ''''' فیلد نظرء و کل جعبه نشان پهک کنت.",
'undeleterevisions'            => '$1 {{PLURAL:$1|بازبینی|بازبینی ان}} آرشیو بوتنت',
'undeletehistory'              => 'اگر  صفحه ای تررینیت، کل بازبینی آن ته تاریح دکه ذخیره بنت.
اگر یک نوکین صفحه گون یک دابی نام بعد چه حذف شر بوتت، دگه ذخیره بوتگین بازبینی آن ته تاریح اولتر جاه کآینت.',
'undeleterevdel'               => 'تررینگ حذف انجام نه بیت اگر آی تاثیری ته اصلی صفحه یا فایل بازبینی که جری جذف بوتت.
ته ای موارد شما بایدن چک می کنیت یا پناه مه کنیت نوکترین بازبینی حدفیء.',
'undeletehistorynoadmin'       => 'ای صفحه حذف بوتت.
دلیل حذف ته جهلگی خلاصه پیش دارگ بیت، گون جزییات کابرانی که ایء اصلاحش کتت پیش چه حذف بیگ.
متن واقعی ای بازبینی آن حذف فقط په مدیران موجود انت.',
'undelete-revision'            => 'حذفی بازبینی $1 (چه  $2) گون $3:',
'undeleterevision-missing'     => 'نامعتبرین یا گارین بازبینی.
شما شاید بدین لینکی داشته ایت یا بازبینی حذف یا ترینگ بوتت چه آرشیو.',
'undelete-nodiff'              => 'هچ پیشگین بازبینی درگیزگ نه بوت.',
'undeletebtn'                  => 'باز گردینگ',
'undeletelink'                 => 'واتر',
'undeletereset'                => 'برگردینگ',
'undeletecomment'              => 'نظر:',
'undeletedarticle'             => 'واترینت "[[$1]]"',
'undeletedrevisions'           => '{{PLURAL:$1|1 بازبینی|$1 بازبینی آن}} واترینگ بیت',
'undeletedrevisions-files'     => '{{PLURAL:$1|1 بازبینی|$1بازبینی ان}} و {{PLURAL:$2|1 فایل|$2 فایلان}} برگردینگ بوتن',
'undeletedfiles'               => '{{PLURAL:$1|1 فایل|$1 فایلآن}} واترینگ بین',
'cannotundelete'               => 'حذف نه کتن پروشت؛
یک نفری دگه شاید ای صفحهء  پیشتر حذفی ترینتت.',
'undeletedpage'                => "<big>'''$1 تررینگ بوتت'''</big>

شوهاز کن [[Special:Log/delete|آمار حذف]] په یک ثبتی چه نوکین حذفیات و بازتررینگان.",
'undelete-header'              => 'See [[Special:Log/delete|آمار حذف]] په نوکین حذفی صفحات..',
'undelete-search-box'          => 'بگرد په صفحات خذفی',
'undelete-search-prefix'       => 'پیش دار صفحات شروع بنت گون:',
'undelete-search-submit'       => 'گردگ',
'undelete-no-results'          => 'په صفحه ی هم دپ ته آرشیو حذف در نه بوت.',
'undelete-filename-mismatch'   => 'نه تونیت بازبینی فایل حذفیء ترینیت گون ای وهد$1: نام فایل یک نهنت',
'undelete-bad-store-key'       => 'نه تونیت بازبینی فایل حذفیء ترینیت گون ای وهد$1:فایل پیش چه حذف گار ات.',
'undelete-cleanup-error'       => 'حطا وهد حذف کتن نه دیستگین فایل آرشیو "$1".',
'undelete-missing-filearchive' => 'نه نونیت فایل آرشیو شناسگ  $1 برگردینت په چی که آیء ته دیتابیس نهنت.
شاید الان حذف ترینگ بوتت.',
'undelete-error-short'         => 'حطا ته ترینگ حذف فایل:$1',
'undelete-error-long'          => 'حطایانی پیش آت وهدی که فایل حذف ترینگ بوت:

$1',

# Namespace form on various pages
'namespace'      => 'فاصله نام',
'invert'         => 'برگردینگ انتخاب',
'blanknamespace' => '(اصلی)',

# Contributions
'contributions' => 'مشارکتان کاربر',
'mycontris'     => 'می مشارکتان',
'contribsub2'   => 'په $1 ($2)',
'nocontribs'    => 'هچ تغییر هم دپ گون ای معیار در نه بوت.',
'uctop'         => '(بالا)',
'month'         => 'چه ماه(و پیش تر):',
'year'          => 'چه سال(و پیشتر)',

'sp-contributions-newbies'     => 'پیش دار فقط مشارکتان نوکین حسایانء',
'sp-contributions-newbies-sub' => 'په نوکین حسابان',
'sp-contributions-blocklog'    => 'محدود کتن ورود',
'sp-contributions-search'      => 'گردگ په مشارکتان',
'sp-contributions-username'    => 'آدرس آی پی یا نام کاربری',
'sp-contributions-submit'      => 'گردگ',

# What links here
'whatlinkshere'            => 'ای لینکی که ادا هست',
'whatlinkshere-title'      => 'صفحاتی که لینگ بوتگنت په "$1"',
'whatlinkshere-page'       => 'صفحه:',
'whatlinkshere-barrow'     => '>',
'linklistsub'              => '(لیست کل لینکان)',
'linkshere'                => "جهلیگی صفحات لینک بوت '''[[:$1]]''':",
'nolinkshere'              => "هچ لینک صفحه ای په '''[[:$1]]'''.",
'nolinkshere-ns'           => "هج صفحه ای لینک نهنت په '''[[:$1]]''' ته ای انتخابی نام فضا",
'isredirect'               => 'صفحه غیر مستقیم',
'istemplate'               => 'همراهی',
'isimage'                  => 'لینک عکس',
'whatlinkshere-prev'       => '{{PLURAL:$1|پیشگین|پیشگین $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|بعدی|بعدی $1}}',
'whatlinkshere-links'      => '← لینکان',
'whatlinkshere-hideredirs' => '$1 غیر مستقیم',
'whatlinkshere-hidetrans'  => '$1 بین اضاف',
'whatlinkshere-hidelinks'  => '$1 لینکان',
'whatlinkshere-hideimages' => '$1 لیناکن عکس',
'whatlinkshere-filters'    => 'فیلتران',

# Block/unblock
'blockip'                         => 'محدود کتن کاربر',
'blockip-legend'                  => 'کاربر محدود کن',
'blockiptext'                     => 'چه ای فرم جهلی په نوشتن دسترسی په یک خاصین آدرس آی پی یا نام کاربری استفاده کن.
شی فقط انجام بیت په خاطر جلوگیری چه هرابکاری  په اساس [[{{MediaWiki:Policy-url}}|سیاست]].
یک حاصین دلیلی بنویس جهلء (مثلا، گوشگ صفخات خاصی که هراب بپتگنت).',
'ipaddress'                       => 'آدرس آی پی:',
'ipadressorusername'              => 'آدرس آي پی یا نام کاربری:',
'ipbexpiry'                       => 'وهد هلگ:',
'ipbreason'                       => 'دلیل:',
'ipbreasonotherlist'              => 'دگ دلیل',
'ipbreason-dropdown'              => '* متداولین دلایل محدودیت
** وارد کتن غلطین اطلاحات
** زورگ محتوا چه صفحات
** لینکان اسپمی په دراین سایت
**وارد کتن بی ربطین/نامفومین چیز په صفحات
** ترسناکین رفتار/ آزار
**سوء استفاده چه چنت حساب
** غیر قابل قبولین نام کاربری',
'ipbanononly'                     => 'فقط کابران ناشناس محدود کن',
'ipbcreateaccount'                => 'مهل حساب شرکنت',
'ipbemailban'                     => 'کاربر چه ایمیل دیم دهگ محدود کن',
'ipbenableautoblock'              => 'اتوماتیکی اهری آدرس آی پی که گون ای کاربر استفاده بوتت محدود کن، و هر چی زیر آی پی هست که سعی کننت اصلاح کننت',
'ipbsubmit'                       => 'ای کاربرء محدود کن',
'ipbother'                        => 'دگر وهد:',
'ipboptions'                      => '2 ساعت: 2 ساعت، 1 روچ: 1 روچ، 3 روچ: 3 روچ، 1 هفته: 1 هفته، 2 هفته: 2هفته، 1 ماه: 1 ماه: 2ماه، 3 ماه: 3 ماه، 6 ماه: 6 ماه، 1 سال: 1 سال، بی حد: بی حد', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'دگر',
'ipbotherreason'                  => 'دگر/اضافی ان دلیل:',
'ipbhidename'                     => 'پناه کن نام کاربری چه آمار محدودیت، فعال کن لیست محدودیت و لیست کاربر',
'ipbwatchuser'                    => 'بچار ای کاربرء صفحات گپ و کاربری آ',
'badipaddress'                    => 'نامعتبر آدرس آی پی',
'blockipsuccesssub'               => 'محدودیت موفق بوت',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] محدود بوتت..<br />
بچار [[Special:IPBlockList|لیست آی پی محدود]] په بازبینی محدودیتان.',
'ipb-edit-dropdown'               => 'اصلاح کن دلایل محدودیت',
'ipb-unblock-addr'                => 'رفع محدودیت  $1',
'ipb-unblock'                     => 'نام کاربری یا آدرس آی پی رفع محدودیت کن',
'ipb-blocklist-addr'              => 'به گند هستین محدودیت په $1',
'ipb-blocklist'                   => 'به گند هنوکین محدودیتان',
'unblockip'                       => 'کاربر رفع محدودیت کن',
'unblockiptext'                   => 'چه ای جهلی فرم استفاده کن په ترینگ دسترسی نوشتن په یک پیشگین آدرس آی پی محدود یا نام کاربری.',
'ipusubmit'                       => 'ای آدرسء رفع محدودیت کن',
'unblocked'                       => '[[User:$1|$1]] رفع محدودیت بیت.',
'unblocked-id'                    => 'محدودیت $1  زورگ بیتت',
'ipblocklist'                     => 'لیست محدود بیتگین آی پی و نام کاربران',
'ipblocklist-legend'              => 'درگیزگ یم محدودین کاربری',
'ipblocklist-username'            => 'نام کاربری یا آدرس آی پی:',
'ipblocklist-submit'              => 'گردگ',
'blocklistline'                   => '$1, $2محدود انت $3 ($4)',
'infiniteblock'                   => 'بی حد',
'expiringblock'                   => 'منقضی بوت $1',
'anononlyblock'                   => 'فقط ناش',
'noautoblockblock'                => 'اتوماتیکی محدودی غیر فعال',
'createaccountblock'              => 'شرکتن حساب محدود انت',
'emailblock'                      => 'ایمیل محدودانت',
'ipblocklist-empty'               => 'لیست محدودی هالیک انت.',
'ipblocklist-no-results'          => 'لوٹتگین نام کاربری یا آدرس آی پی محدود نهنت.',
'blocklink'                       => 'محدود',
'unblocklink'                     => 'رفع محدودیت',
'contribslink'                    => 'مشارکتان',
'autoblocker'                     => 'اتوماتیک کبلت په چی که شمی آدرس آی پی نوکی استفاده بوتت گون  "[[User:$1|$1]]".
داتگین دلیل په محدود کتن $1 شی انت: "$2"',
'blocklogpage'                    => 'بلاک ورود',
'blocklogentry'                   => 'محدود بوته [[$1]] گون یک زمان انقاضای $2 $3',
'blocklogtext'                    => 'شی یک آماری چه کاران محدود و رفع محدودیت چه ای کاربر انت.
اتوماتیکی محدود بوتگین آدرس آی پی ادان لیست نهنت.
بچار [[Special:IPBlockList|لیست محدودیت آی پی]] په لیست هنوکین عملی محدودیتان و بند کتان.',
'unblocklogentry'                 => 'محدود نه کتن $1',
'block-log-flags-anononly'        => 'ناشناس کابران فقط',
'block-log-flags-nocreate'        => 'شرکتن حساب غیر فعال',
'block-log-flags-noautoblock'     => 'اتوماتیکی محدوددیت غیر فعال',
'block-log-flags-noemail'         => 'ایمیل محدودانت',
'block-log-flags-angry-autoblock' => 'بند کتن دسترسی خودکار پیشرفته فعال انت',
'range_block_disabled'            => 'توانایی مدیران سیستم په شرکتن محدوده محدودیت غیر فعالنت.',
'ipb_expiry_invalid'              => 'وهد هلگ نامعتبر انت.',
'ipb_expiry_temp'                 => 'پناهین نام کاربری محدودیاتن بایدن دایمی بنت.',
'ipb_already_blocked'             => '"$1" الان محدودنت.',
'ipb_cant_unblock'                => 'حطا: شناسگ محدودیت  $1 در گیزگ نه بوت. شاید هنگیت رفع محدودیت نهنت.',
'ipb_blocked_as_range'            => 'حطا: ای پی  $1 مستقیما محدود نهنت و نه تونیت رفع محدودیت بیت.
بله آی جزی چه محدوده  $2 محدود بوتت که تونیت رفع محدودیت بیت.',
'ip_range_invalid'                => 'نامعتبر محدوده آی پی',
'blockme'                         => 'مناء محدود کن',
'proxyblocker'                    => 'محدود کننده ی پروکسی',
'proxyblocker-disabled'           => 'ای عمگر غیرفعالنت.',
'proxyblockreason'                => 'شمی آدرس آی پی محدود بوتت په چی که ایء یک پچین پروکسی ات.
لطفا گون وتی اینترنتی شرکت تماس گریت یا حمایت تکنیکی و آیانا چی ای مشکل امنیتی شدید سهی کنیت.',
'proxyblocksuccess'               => 'انجام بوت.',
'sorbs'                           => 'دی ان اس بی ال',
'sorbsreason'                     => 'شمی آدرس آی پی لیست بوتت په داب پچین پروکسی ته  DNSBL که استفاده بیت گون {{SITENAME}}.',
'sorbs_create_account_reason'     => 'شمی آدرس آی پی لیست بوتت په داب پچین پروکسی ته  دی ان ای بی ال که استفاده بیت گون {{SITENAME}}.
شما نه تونیت حسابی شرکنیت',

# Developer tools
'lockdb'              => 'دیتابیس کبل کن',
'unlockdb'            => 'دیتابیس پچ کن',
'lockdbtext'          => 'کبل کتن دیتابیس توان کل کابرانء معلق کتن په اصلاح صفحات، عوض کتن ترجیحات، اصلاح آیانی لیست چارگانء، و دگه چیزانی که نیاز په دیتابیس دارنت.
لطفا تایید کنیت که آ چیزی که شما لوٹیت انجام دهیت و هنچوش که تعمییر هلت آیء کبلیء پچ کنیت.',
'unlockdbtext'        => 'پچ کتن کبل دیتابیس توان کل کابران په اصلاحات صفحات، تغییر ترجیحات، اصلاح دیگه چیزانی که نیاز په دیتابیس دارند بر گردینیت.
لطفا تایید کنیت که شی هما چیزی انت که شما لوٹیت. انجام دهیت.',
'lockconfirm'         => 'بله،من واکی لوٹان دیتابیس کبل کنان.',
'unlockconfirm'       => 'بله، من واکی لوٹان دیتابیس پچ کنان',
'lockbtn'             => 'دیتابیس کبل کن',
'unlockbtn'           => 'دیتابیس پچ کن',
'locknoconfirm'       => 'شما جعبه تایید نشان نه کت',
'lockdbsuccesssub'    => 'دیتابیس کبل موفق بوت',
'unlockdbsuccesssub'  => 'کبل دیتابیس زورگ بیت',
'lockdbsuccesstext'   => 'دیتابیس کبلنت.<br />
بزان که  [[Special:UnlockDB|کبل بزور]] بعد چه شی که شمی تعمیر کامل بوت.',
'unlockdbsuccesstext' => 'دیتابیس پچ بوتت/',
'lockfilenotwritable' => 'فایل کبل دیتابیس نویسگی نهنت.
په کبل و پچ کتن دیتابیس، ای قایل لازمت گون وب سرور نوشتن بیت.',
'databasenotlocked'   => 'دیتابیس کبل نهنت.',

# Move page
'move-page'               => 'جاه په جاه کن $1',
'move-page-legend'        => 'صفحه جاه په جاه کن',
'movepagetext'            => "استفاده چه جهلگی فرم یک صفحه ای نامی آ بدل کنت، کل تاریح آیآ په نوکین نام جاه په جاه کنت.
گهنگین عنوان یک صفحه غیر مستقیمی په نوکین عنوان بیت.
لینکان په کهنگین عوض نبنت;
مطمین بیت په خاطر [[Special:DoubleRedirects|دوتایی]] یا [[Special:BrokenRedirects|پرشتگین غیر مستقیم]].
شما مسولیت که مطمین بیت که لینکان ادامه دهنت روگ په جاهی که قرار برونت.

توجه کینت صفحه جاه په جاه نه بیت اگه یک صفحه ای گون نوکین عنوان هست، مگر شی که آی هالیک بیت یا یک غیرمسقیم و پی سرین تاریح اصلاح می بیت. شی په ای معنی اینت که شما تونیت یک صفحه ای آ نامی بدل کینت که  آی نام په خطا عوض بیت و شما نه توینت یک صفحه ی نامی بازنویسی کنیت.

''''هوژاری!''''  
شی ممکننت یک تغییر آنی و نه لوٹتگین په یک معروفین صفحه ای بیت;
لصفا مطمین بیت شما عواقب شی زانیت پیش چه دیم روگآ",
'movepagetalktext'        => "همراهی گپان صفحه اتوماتیک گون آی جاه په چاه بنت ''''مگر:''''
یک ناهالیکین صفحه گپی چیر آی ء نوکین نام بیت، یا
شما جهلیگین باکس آ تیک مجنیت.
ته ای موراد شما بایدن صفحه یا دسته جاه په جاه کنی و یا آیآ چن و بند کینت.",
'movearticle'             => 'جاه په چاهی صفحه:',
'movenotallowed'          => 'شما را اجازت به جاه په جاه کتن صفحات نیست.',
'newtitle'                => 'په نوکین عنوان:',
'move-watch'              => 'این صفحه یا بچار',
'movepagebtn'             => 'جاه په جاه کن صفحه',
'pagemovedsub'            => 'جاه په جاهی موفقیت بود',
'movepage-moved'          => '<big>\'\'\'"$1" جاه په اجه بوت په"$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'صفحه گون آن نام پیش تر هستت، یا نامی که شما زورتت نامعتبرنت.
یک دگه نامی بزوریت.',
'cantmove-titleprotected' => 'شما نه تونیت یک صفحه ای به ای جاگاه جاه په جاه کنیت، په چی که نوکین عنوان محافظت بیت چه شربیگ',
'talkexists'              => "''''صفحه وتی گون موفقیت جاه په جاه بوت، بله صفحه گپ نه نویت جاه  په جاه بیت چون که یکی ته نوکین عنوان هست.
لطفا آیآ دستی چند و بند کنیت.''''",
'movedto'                 => 'جاه په جاه بیت په',
'movetalk'                => 'جاه په جاه کتن صفحه کپ همراه',
'move-subpages'           => 'جاه په جاه کن کل زیرصفحاتء، اگر بیت',
'move-talk-subpages'      => 'جاه په جاه کن کل زیرصفحات صفحه گپء، اگه بیت',
'movepage-page-exists'    => 'صفحه  $1 هنو هستن و نه تونیت اتوماتیکی سر بنویسیت.',
'movepage-page-moved'     => 'صفحه  $1 جاه په جاه بیت په  $2',
'movepage-page-unmoved'   => 'صفحه $1نه تونیت جاه په جاه بیت په $2',
'movepage-max-pages'      => 'گیشترین $1 {{PLURAL:$1|صفحه|صفحات}}جاه په جاه بوتگن  ودگه هچی اتوماتیکی جاه په جاه نه بیت.',
'1movedto2'               => '[[$1]] چاه په چاه بوت په [[$2]]',
'1movedto2_redir'         => '[[$1]] جاه په جاه بوت په [[$2]] غیر مستقیم',
'movelogpage'             => 'جاه په جاهی ورود',
'movelogpagetext'         => 'جهلء یک لیستی چه صفحات جاه په جاه بوتگین هست',
'movereason'              => 'دلیل:',
'revertmove'              => 'برگردینگ',
'delete_and_move'         => 'حذف وجاه په جاه کن',
'delete_and_move_text'    => '== حذف نیاز داریت په ==
صفحه مبدا "[[:$1]]"  که هنگت هستن.
آیا شما لوٹیت آیء حذف کنیت دان په حذف‌ آیء راهی شر بیت؟',
'delete_and_move_confirm' => 'بله، صفحه حذف کن',
'delete_and_move_reason'  => 'حذف بوت په شرکتن راه په جاه په جاه کتن',
'selfmove'                => 'منبع و مقصد عناوین یک انت؛
نه تونیت صفحه ای په وتی جاگاهء جاه په جاه کنت',
'immobile_namespace'      => 'منبع یا مقصد عنوان چه نوح حاصین انت؛
نه تونین صفحاتء چه یا په آی نام فضا جاه په جاه کنت.',
'imagenocrossnamespace'   => 'نه تونیت جاه په جاه کنت فایل په یا نام فضای غیر فایلی',
'imagetypemismatch'       => 'نوکین فایل بند گون نوع آی هم دپ نهنت.',
'imageinvalidfilename'    => 'فایل عکس هدف نام معتبر انت',
'fix-double-redirects'    => 'په روچ کتن هر غیر مستقیمی که په مقاله اصلی اشاره کنت',

# Export
'export'            => 'خروج صفحات',
'exporttext'        => 'شما تونیت متن درکینت و تاریح اصلاح یک بخصوص این صفحه ایء یا مجموعه چنت صفحه تلمتلین ته لهتی XML.
شی بوتن که وارد دگه ویکی بیگ گون [[Special:Import|import page]].

په خروج صفحات، عناوین آیء ته جهلگی باکس وارد کن, هر عنوان ته یک حطی، و انتخاب کن که آیا شمل لوٹیت هنوکین نسخه و کل کدیمی نسخ،گون خطوط تاریح صفحه, یا فقط هنوکین نسخه گون اطلاعاتی درباره آهری اصلاح.

په اهری مورد شما تونیت هنچوش چه یک لینکی استفاده کنیت،مثلا [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] په صفحه ی "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'فقط شامل هنوکین بازبینی، نه تاریح کامل',
'exportnohistory'   => "----
'''توجه:''' گردگ تاریح کامل صفحات چه طریق ای فرم په خاطر دلایل اجرایی غیر فعال بوتت.",
'export-submit'     => 'درگیزگ',
'export-addcattext' => 'چه دسته صفحات اضافه کن:',
'export-addcat'     => 'اضافه کن',
'export-download'   => 'ذخیره په داب فایلی',
'export-templates'  => 'شامل تمپلتان',

# Namespace 8 related
'allmessages'               => 'پیامان سیستم',
'allmessagesname'           => 'نام',
'allmessagesdefault'        => 'پیش فرضین متن',
'allmessagescurrent'        => 'هنوکین متن',
'allmessagestext'           => 'شی یک لیستی چه کوله یان موجود ته نام فضای مدیا وی کی انت.
لطفا بچاریت  [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] و [http://translatewiki.net Betawiki] اگر شما لوٹیت ته ملکی کتن مدیا وی کی کمک کنیت.',
'allmessagesnotsupportedDB' => "ای صفحه نه تونیت استفاده بیت په چی که'''\$wgUseDatabaseMessages''' غیر فعالنت.",
'allmessagesfilter'         => 'فیلتر نام کوله:',
'allmessagesmodified'       => 'فقط پیش دار تغییر دهگ بیتیگن',

# Thumbnails
'thumbnail-more'           => 'مزن',
'filemissing'              => 'فایل گارنت',
'thumbnail_error'          => 'خطا ته شرکتن هوردوکین$1',
'djvu_page_error'          => 'صفحه Djvu در چه محدوده انت',
'djvu_no_xml'              => 'نه تونیت XML بیاریت په فایل DjVu',
'thumbnail_invalid_params' => 'نامعتبر پارامتران پنچ انگشتی',
'thumbnail_dest_directory' => 'نه تونیت شرکنت مسیر مقصدء',

# Special:Import
'import'                     => 'وارد کن صفحاتء',
'importinterwiki'            => 'ورود بین ویکی',
'import-interwiki-text'      => 'یک ویکی و  عنوان صفحه انتخاب کن په ورود.
تاریح بازبینی و نامان اصلاح کنوکان دارگ بیت.
کل کاران ورود بین ویکی وارد بیت نه [[Special:Log/import|ورود آمار]].',
'import-interwiki-history'   => 'کپی کن کل بازبینی آن تاریح په ای صفحه',
'import-interwiki-submit'    => 'ورود',
'import-interwiki-namespace' => 'ترانسفر صفحات په فضا نام',
'importtext'                 => 'لطفا فایل چه منبع ویکی درگیز گون حاصین:[[Special:Export|وسیله درگیزگ]], ایء ته وتی دیسک ذخیره کن و ادان آپلود کن.',
'importstart'                => 'وارد کنت صفحات...',
'import-revision-count'      => '$1 {{PLURAL:$1|بازبینی|بازبینی ان}}',
'importnopages'              => 'هچ صفحه ای په ورود.',
'importfailed'               => 'ورود پروشت: <nowiki>$1</nowiki>',
'importunknownsource'        => 'ناشناس نوع منبع ورود',
'importcantopen'             => 'نه تونت فایل ورودء پچ کنت',
'importbadinterwiki'         => 'بدین لینک بین ویکی',
'importnotext'               => 'هالیک یا بی متن',
'importsuccess'              => 'وارد کتن هلت!',
'importhistoryconflict'      => 'متضادین بازبینی تاریح هستن (شایدای صفحهء پیش تر وارد کتت)',
'importnosources'            => 'هچ منابع ورود بین ویکیء تعریف نه بوتت و آپلود مستقیم تاریح غیر فعالنت.',
'importnofile'               => 'هچ فایل ورودی آپلود نه بوت.',
'importuploaderrorsize'      => 'آپلود کتن فایل ورود پروشت. فایل چه آهرین حد مجاز آپلود مزنتر انت.',
'importuploaderrorpartial'   => 'آپلود کتن  فایل ورود پروشت. فایل فقط جزی أپلود بوت.',
'importuploaderrortemp'      => 'آپلود کتن فایل وارد پروشت. یک فودلدر هنوکین کارنت.',
'import-parse-failure'       => 'تجزیه XML وارد کتن پروش واردت',
'import-noarticle'           => 'هچ صفحه په وارد بیگ',
'import-nonewrevisions'      => 'کل بازبینی آن پیش تر وارد بیتگن',
'xml-error-string'           => '$1 ته خط $2, ستون $3 (بایت $4): $5',
'import-upload'              => 'آپلود دیتا XML',

# Import log
'importlogpage'                    => 'ورودان وارد کن',
'importlogpagetext'                => 'ورود مدیریتی صفحات گون تاریح صلاح چه دگه ویکی آن.',
'import-logentry-upload'           => 'وارد بوت [[$1]]  گون فایل آپلود بوتگین',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|بازبینی|بازبینی ان}}',
'import-logentry-interwiki'        => 'بین ویکی بوت $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|بازبینی|بازبینی ان}} چه $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'صفحه کاربری من',
'tooltip-pt-anonuserpage'         => 'صفحه کاربر په آی پی که شما هنو اصلاح کنیت په داب',
'tooltip-pt-mytalk'               => 'صفحه گپ من',
'tooltip-pt-anontalk'             => 'بحث باره ی اصلاحات چه ای آدرس آی پی',
'tooltip-pt-preferences'          => 'منی ترجیحات',
'tooltip-pt-watchlist'            => 'لیست صفحیانی که شما تغییرات آیانا رند گرگیت',
'tooltip-pt-mycontris'            => 'لیست منی مشارکتان',
'tooltip-pt-login'                => 'شر ترنت که وارد بیت، بله شی اجبار نهنت',
'tooltip-pt-anonlogin'            => 'چه شما دست بندی بیت وارد بیت، بله شی هنگت ضرورت نهنت.',
'tooltip-pt-logout'               => 'در بیگ',
'tooltip-ca-talk'                 => 'بحث دباره محتوای صفحه',
'tooltip-ca-edit'                 => 'شما تونیت ای صفحه یا اصلاح کنیت. لطفا چه بازبین دکمه پیش چه ذخیره کتن استفاده کنیت.',
'tooltip-ca-addsection'           => 'په ای بحث یک نظر هور کن',
'tooltip-ca-viewsource'           => 'ای صفحه محافظت بوتت. شما تونیت آیی منبع آ بچاریت',
'tooltip-ca-history'              => 'پیشگین نسخ چه ای صفحه',
'tooltip-ca-protect'              => 'ای صفحه یا حفاظت کن',
'tooltip-ca-delete'               => 'ای صفحه حذف کن',
'tooltip-ca-undelete'             => 'بازترینگ اصلاحات انجان بوت په ای صفحه پیش چه شی که حذف بیت',
'tooltip-ca-move'                 => 'ای صفحه یا جاه په جاه کن',
'tooltip-ca-watch'                => 'ای صفحه یا ته شمی لیست چارگ هور کنت',
'tooltip-ca-unwatch'              => 'ای صفحه یا چه وتی لیست چارگ در کن',
'tooltip-search'                  => 'گردگ {{SITENAME}}',
'tooltip-search-go'               => 'برو به یک صفحه گون همی نامی اگر که هستن',
'tooltip-search-fulltext'         => 'گرد صفحات په ای متن',
'tooltip-p-logo'                  => 'صفحه اصلی',
'tooltip-n-mainpage'              => 'صفحه اصلی بچار',
'tooltip-n-portal'                => 'پروژه ی باره: هرچی که شما تونیت انجام دهیت، جاهی که چیزانا درگیزیت',
'tooltip-n-currentevents'         => 'در گیزگ اطلاعات پیش زمینه ته هنوکین رویدادآن',
'tooltip-n-recentchanges'         => 'لیست نوکین تغییر ته وی کی',
'tooltip-n-randompage'            => 'یک شانسی صفحه پچ کن',
'tooltip-n-help'                  => 'جاهی په زانگ',
'tooltip-t-whatlinkshere'         => 'لیست کل صفحات وی کی که ادان لینک بوتگنت',
'tooltip-t-recentchangeslinked'   => 'نوکین تغییرات ته صفحاتی که چه ای صفحه لینک بوتگنت',
'tooltip-feed-rss'                => 'منبع آر اس اس په ای صفحه',
'tooltip-feed-atom'               => 'منبع اتم په ای صفحه',
'tooltip-t-contributions'         => 'لیست مشارکتان ای کاربر بچار',
'tooltip-t-emailuser'             => 'په ای کاربر یک ایمیل دیم دی',
'tooltip-t-upload'                => 'آپلود فایلان',
'tooltip-t-specialpages'          => 'لیست کل حصاین صفحات',
'tooltip-t-print'                 => 'چهاپی نسخه چه ای صفحه',
'tooltip-t-permalink'             => 'دایمی لینکی په ای نسخه صفحه',
'tooltip-ca-nstab-main'           => 'به گند صفحه محتواء',
'tooltip-ca-nstab-user'           => 'چارگ صفحه کاربر',
'tooltip-ca-nstab-media'          => 'به گند صفحه مدیاء',
'tooltip-ca-nstab-special'        => 'شی یک حاصین صفحه اینت، شما نه تونیت وت صفحه اصلاح کنیت',
'tooltip-ca-nstab-project'        => 'بچار صفحه پروژه یا',
'tooltip-ca-nstab-image'          => 'صفحه فایل بگند',
'tooltip-ca-nstab-mediawiki'      => 'به گند کوله سیستمء',
'tooltip-ca-nstab-template'       => 'چارگ تمپلت',
'tooltip-ca-nstab-help'           => 'صفحه کمک بچار',
'tooltip-ca-nstab-category'       => 'دسته صفحه ی بچار',
'tooltip-minoredit'               => 'شی آ په داب یک اصلاح جزی نشان بل',
'tooltip-save'                    => 'وتی تغییرات ذخیره کن',
'tooltip-preview'                 => 'بازبین کن وتی تغییراتا، لطفا پیش چه ذخیره کتن شیا استفاده کن.',
'tooltip-diff'                    => 'پیش دار تغییراتی که شما په نوشته دات.',
'tooltip-compareselectedversions' => 'بچار تفاوتان بین دو انتخاب بوتگین نسخه یان این صفحه',
'tooltip-watch'                   => 'ای صفحه یانا ته وتی لیست چارگ هور کن',
'tooltip-recreate'                => 'دگه شرکتن صفحه علاوه بر شی که ای حذف بوتت',
'tooltip-upload'                  => 'آپلود بنگیج بوت',

# Stylesheets
'common.css'   => '/* CSS که اداننت په کل پوستان په کار رونت. */',
'monobook.css' => '/* CSS که اداننت کابران پوست مونوبوک تاثیر کننت */',

# Scripts
'common.js'   => '/* هر جاوا اسکریپتی ادان په کل کابران ته هر صفحه ای بار بیت. */',
'monobook.js' => '/* جاوا اسکریپت ادان فقط په کابرانی که چه پوست منوبوک استفاده کننت بار بیت. */',

# Metadata
'nodublincore'      => 'هسته دوبلین RDF متادیتا ته ای سرور غیر فعالنت.',
'nocreativecommons' => 'کریتیو کامان متادیتا RDF ته ای سرور غیر فعال انت.',
'notacceptable'     => 'سروری ویکی نه تونیت دیتای ته فرمتی که شمی کلاینت بتوننت آی بوانند فراهم کنت.',

# Attribution
'anonymous'        => 'ناشناس کاربر(ان)  {{SITENAME}}',
'siteuser'         => '{{SITENAME}} کاربر $1',
'lastmodifiedatby' => 'ای صفحه اهری رندی که تغییر دهگ بیته $2, $1گون $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'براساس کار توسط $1.',
'others'           => 'دگران',
'siteusers'        => '{{SITENAME}} کاربر(آن) $1',
'creditspage'      => 'اعتبارات صفحه',
'nocredits'        => 'په ای صفحه اطلاعات اعتبارات موجود نهنت.',

# Spam protection
'spamprotectiontitle' => 'فیلتر حفاظت اسپم',
'spamprotectiontext'  => 'صفحه ای که شما لوٹتیت آیء ذخیره کنیت گون فیلتر اسپم محدود بوتت.
شی شاید په خاطر یم حارجی سایت لینکی پیش‌آتکگ.',
'spamprotectionmatch' => 'جهلیگین متن چیزی انت که می فیلتر اسپمی آورت بالاد: $1',
'spambot_username'    => 'اسپم پاک کنوک مدیا وی کی',
'spam_reverting'      => 'عوض کتن په آهری نسحه که شامل لینکان می بیت په $1',
'spam_blanking'       => 'کل بازبینی آن شامل لینکان په $1, بوتت  هالیکی',

# Info page
'infosubtitle'   => 'اطلاعات په صفحه',
'numedits'       => 'تعداد اصلاحات (صفحه): $1',
'numtalkedits'   => 'تعداد اصلاحات (صفحه بحث): $1',
'numwatchers'    => 'تعداد چاروکان: $1',
'numauthors'     => 'تعداد دوراین نویسوکان (صفحه): $1',
'numtalkauthors' => 'تعداد مجزاین نویسوکان(صفحه بحث): $1',

# Math options
'mw_math_png'    => 'یکسره PNG تحویل دی',
'mw_math_simple' => 'HTML اگر باز سادگت یا دگه PNG',
'mw_math_html'   => 'HTML اگر ممکنت یا دگه  دگهPNG',
'mw_math_source' => 'آیء په داب TeX بل (په بروززان متنی)',
'mw_math_modern' => 'په مدرنین بروزر آن توصیه بیت',
'mw_math_mathml' => 'MathML اگر ممکن انت (آزمایشی)',

# Patrolling
'markaspatrolleddiff'                 => 'نشان کن په داب نظارت بوتگین',
'markaspatrolledtext'                 => 'ای صفحه نشان کن په داب نظارت بوتگین',
'markedaspatrolled'                   => 'نشاننت په داب نظارتی',
'markedaspatrolledtext'               => 'انتخاب بوتگین بازبینی په داب نظارتی نشان بوتت.',
'rcpatroldisabled'                    => 'نظارت تغییرات نوکیت غیر فعال',
'rcpatroldisabledtext'                => 'وسیله نظارت تغییرات نوکین هنو غیر فعالنت.',
'markedaspatrollederror'              => 'نه تونتی په عنوان نظارت بوتگین نشان کنت',
'markedaspatrollederrortext'          => 'شما بایدن یک بازبینی مشخص کنیت په عنوان نظارت بوتگین.',
'markedaspatrollederror-noautopatrol' => 'شما را اجازت نیست وتی تغییراتء په عنوان نظارت بیتگین نشان کنیت.',

# Patrol log
'patrol-log-page'   => 'آمار نظارت',
'patrol-log-header' => 'شی آماری چه بازبینی آن گشتی انت.',
'patrol-log-line'   => 'نشان هلگ بیتن $1 چه $2 نظارت $3',
'patrol-log-auto'   => '(اتوماتیک)',
'patrol-log-diff'   => 'ر$1',

# Image deletion
'deletedrevision'                 => 'قدیمی بازبینی $1 حذف بوت',
'filedeleteerror-short'           => 'حطا حذف فایل: $1',
'filedeleteerror-long'            => 'حطای پیش آتک وهدی که فایل حذف بیگت:

$1',
'filedelete-missing'              => 'فایل "$1" حذف نه بیت, په چی که آی وجود نداریت',
'filedelete-old-unregistered'     => 'بازبینی فایل مشخص بوتگین "$1" ته دیتابیس نیست.',
'filedelete-current-unregistered' => 'مشخص بوتگین فایل "$1" ته دیتابیس نیست.',
'filedelete-archive-read-only'    => 'مسیر آرشیو "$1" چه طرف وب سرور نویسگ نه بیت.',

# Browsing diffs
'previousdiff' => '← پیشگین اصلاح',
'nextdiff'     => 'نوکترین اصلاح→',

# Media information
'mediawarning'         => "''''هوژاری:'''' ای فایل شاید شامل بد واهین کد بوت،اجرای آیی ته وتی سیستم شاید توافقی بیت.<hr />",
'imagemaxsize'         => 'محدودیت تصاویر ته فایل صفحات توضیح ته:',
'thumbsize'            => 'اندازه پیج انگشتی',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|صفحه|صفحات}}',
'file-info'            => '(اندازه فایل: $1, مایم نوع: $2)',
'file-info-size'       => '($1 × $2 پیکسل, اندازه فایل: $3, مایم نوع: $4)',
'file-nohires'         => '<small>مزنترین رزلوشن نیست.</small>',
'svg-long-desc'        => '(اس وی جی  فایل, معمولا $1 × $2 پیکسل, فایل اندازه: $3)',
'show-big-image'       => 'کل صفحه',
'show-big-image-thumb' => '<small>اندازه ای بازبین:$1× $2 pixels</small>',

# Special:NewImages
'newimages'             => 'گالری نوکین فایلان',
'imagelisttext'         => "جهل یک لیستی چه  '''$1''' {{PLURAL:$1|فایل|فایلان}} هست که ترتیبنت $2.",
'newimages-summary'     => 'ای حاصین صفحه اهرین آپلود بوتگین فایلان پیشداریت',
'showhidebots'          => '(روباتان $1 )',
'noimages'              => 'هیچی په دیستن',
'ilsubmit'              => 'گردگ',
'bydate'                => 'گون تاریح',
'sp-newimages-showfrom' => 'پیش دار نوکین فایلان شروع بینت چه $2, $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'seconds-abbrev' => 'س',
'minutes-abbrev' => 'م',
'hours-abbrev'   => 'ه',

# Bad image list
'bad_image_list' => 'فرمت په داب جهلیگی انت:

فقط ایتمان لیست چارگ بنت(خطانی که گون * شروع بنت).
اولین لینک ته یک خط باید یک لینکی په یک بدین فایلی بیت.
هر لینکی که کیت ته هما خط اسنثتا بینت.',

/*
Short names for language variants used for language conversion links.
To disable showing a particular link, set it to 'disable', e.g.
'variantname-zh-sg' => 'disable',
Variants for Chinese language
*/
'variantname-zh-hans' => 'هانس',
'variantname-zh-hant' => 'هانت',
'variantname-zh-cn'   => 'چن',
'variantname-zh-tw'   => 'تایوان',
'variantname-zh-hk'   => 'هک',
'variantname-zh-sg'   => 'چی=سج',
'variantname-zh'      => 'چین',

# Variants for Serbian language
'variantname-sr-ec' => 'سر-اک',
'variantname-sr-el' => 'سر-ال',
'variantname-sr'    => 'سر',

# Variants for Kazakh language
'variantname-kk-kz'   => 'کک-کز',
'variantname-kk-tr'   => 'کک-تر',
'variantname-kk-cn'   => 'کک-سن',
'variantname-kk-cyrl' => 'کک-سرل',
'variantname-kk-latn' => 'کک-لت',
'variantname-kk-arab' => 'کک-ارب',
'variantname-kk'      => 'کک',

# Variants for Kurdish language
'variantname-ku-arab' => 'کو-ار',
'variantname-ku-latn' => 'کو-لت',
'variantname-ku'      => 'کرد',

# Variants for Tajiki language
'variantname-tg-cyrl' => 'سریل-تج',
'variantname-tg-latn' => 'لاتین-ت.ج',
'variantname-tg'      => 'تج',

# Metadata
'metadata'          => 'متا دیتا',
'metadata-help'     => 'ای فایل شامل مزیدین اطلاعاتنیت، شاید چه یک دوربین یا اسکنر په شرکتن و دیجیتالی کتن هور بیتت.
اگه فایل چه اولیگین حالتی تغییر داته بوته شاید لهتی کل جزییات شر پیش مداریت.',
'metadata-expand'   => 'پیش دار گیشترین جزییات',
'metadata-collapse' => 'پناه کن مزیدین جزییاتا',
'metadata-fields'   => 'EXIF متادیتا فیلدان لسیت بوتگن ته ای کوله شامل بینت تع  عکس  صفحه پیش داریت وهخهدی کهجدول متادیتا is هراب بیت.
دگران پناه بنت په طور پیش فرض.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'پهنات',
'exif-imagelength'                 => 'بلندی',
'exif-bitspersample'               => 'بیت ته جز',
'exif-compression'                 => 'طرح کمپرس',
'exif-photometricinterpretation'   => 'ترکیب پیکسل',
'exif-orientation'                 => 'جهت',
'exif-samplesperpixel'             => 'تعداد اجزاء',
'exif-planarconfiguration'         => 'ترکیب دیتا',
'exif-ycbcrsubsampling'            => 'زیر نمونه نرم چه Yتا C',
'exif-ycbcrpositioning'            => 'جاگاه Y و C',
'exif-xresolution'                 => 'افقی وضوح',
'exif-yresolution'                 => 'وضوح عمودی',
'exif-resolutionunit'              => 'واحد X وY وضوح',
'exif-stripoffsets'                => 'جاگاه دیتای عکس',
'exif-rowsperstrip'                => 'تعداد ردیف آن ته هر نوار',
'exif-stripbytecounts'             => 'بایت ته هر نوار کمپرس بوتگین',
'exif-jpeginterchangeformat'       => 'عوض کتن په JPEG SOI',
'exif-jpeginterchangeformatlength' => 'بیت آن دیتا جیی پی جی',
'exif-transferfunction'            => 'عملگر جاه په جاهی',
'exif-whitepoint'                  => 'رنگ پذیری نکته اسپیت',
'exif-primarychromaticities'       => 'رنگ پذیری اولیگی',
'exif-ycbcrcoefficients'           => ' ضرایب فضا رنگ جاه په جاهی ماتریکس',
'exif-referenceblackwhite'         => 'جفتی چه سیاه و اسپیتین ارزشان مرجع',
'exif-datetime'                    => 'فایل تغییر تاریح  و وهد',
'exif-imagedescription'            => 'عنوان عکس',
'exif-make'                        => 'شرکنوک دوربین',
'exif-model'                       => 'مدل دوربین',
'exif-software'                    => 'برنامه استفاده بوتگ',
'exif-artist'                      => 'نویسنده',
'exif-copyright'                   => 'داروک حق کپی',
'exif-exifversion'                 => 'نسخه Exif',
'exif-flashpixversion'             => 'نسخه فلش پیکس حمایتی',
'exif-colorspace'                  => 'فضا رنگ',
'exif-componentsconfiguration'     => 'معنی هر جز',
'exif-compressedbitsperpixel'      => 'مدل کمپرس کتن عکس',
'exif-pixelydimension'             => 'معتبرین پهنات عکس',
'exif-pixelxdimension'             => 'معتبرین ارتفاع عکس',
'exif-makernote'                   => 'یادداشتان شرکنوک',
'exif-usercomment'                 => 'نظرات کاربر',
'exif-relatedsoundfile'            => 'مربوطین فایل صوتی',
'exif-datetimeoriginal'            => 'تاریح و وهد شرکتن دیتا',
'exif-datetimedigitized'           => 'تاریح و هود دیجیتالی بوگ',
'exif-subsectime'                  => 'تاریح وهد زیر ثانیه',
'exif-subsectimeoriginal'          => 'تاریخ زمان اصلی زیر ثانیه',
'exif-subsectimedigitized'         => 'تاریح زمان دیجتالی بوتگین زیر ثانیه',
'exif-exposuretime'                => 'وهد ته معرض بیگ',
'exif-exposuretime-format'         => '$1 ثانیه ($2)',
'exif-fnumber'                     => 'شماره اف',
'exif-fnumber-format'              => 'ف/$1',
'exif-exposureprogram'             => 'برنامه ته معرض بوتن',
'exif-spectralsensitivity'         => 'حساسیت طیفی',
'exif-isospeedratings'             => 'میزان سرعت ISO',
'exif-oecf'                        => 'فاکتور تبدیل اوپتوالکترونیکی',
'exif-shutterspeedvalue'           => 'سرعت شاتر',
'exif-aperturevalue'               => 'پچ بیگ',
'exif-brightnessvalue'             => 'روشنی',
'exif-exposurebiasvalue'           => 'معرض پیشقدر',
'exif-maxaperturevalue'            => 'آهری حد پیش بیگ سطح',
'exif-subjectdistance'             => 'فاصله شی',
'exif-meteringmode'                => 'مدل متر گنگ',
'exif-lightsource'                 => 'منبع نور',
'exif-flash'                       => 'فلاش',
'exif-focallength'                 => 'طول کانونی لنز',
'exif-focallength-format'          => '$1م.م',
'exif-subjectarea'                 => 'ناحیه شی',
'exif-flashenergy'                 => 'قدرت فلاش',
'exif-spatialfrequencyresponse'    => 'عکس العمل متداول فاصله ای',
'exif-focalplanexresolution'       => 'وضوح X سطح کانونی',
'exif-focalplaneyresolution'       => 'وضوح Y سطح کانونی',
'exif-focalplaneresolutionunit'    => 'واحد وضوح سطح کانونی',
'exif-subjectlocation'             => 'جاگاه شی',
'exif-exposureindex'               => 'ایندکس دته معرض بوگ',
'exif-sensingmethod'               => 'روش حس کتن',
'exif-filesource'                  => 'منبع فایل',
'exif-scenetype'                   => 'نوع صحنه',
'exif-cfapattern'                  => 'الگو سی اف ای',
'exif-customrendered'              => 'پردازش عکس سنت',
'exif-exposuremode'                => 'مدل پچ بوگ دیافراگم',
'exif-whitebalance'                => 'توازن اسپیت',
'exif-digitalzoomratio'            => 'نسبت زوم دیجیتالی',
'exif-focallengthin35mmfilm'       => 'فاصله کانونی ته فیلم 35 م.م',
'exif-scenecapturetype'            => 'نوع گرگ صحنه',
'exif-gaincontrol'                 => 'کنترل صحنه',
'exif-contrast'                    => 'کنتراست',
'exif-saturation'                  => 'اشباع',
'exif-sharpness'                   => 'تیزی',
'exif-devicesettingdescription'    => 'توضیح تنظیمات وسیله',
'exif-subjectdistancerange'        => 'محدوده فاصله شی',
'exif-imageuniqueid'               => 'شناسگ یکی عکس',
'exif-gpsversionid'                => 'برچسپ نسخه جی پی اس',
'exif-gpslatituderef'              => 'عرض جنوبی یا شمالی',
'exif-gpslatitude'                 => 'عرض جغرافیایی',
'exif-gpslongituderef'             => 'طول غربی یا شرقی',
'exif-gpslongitude'                => 'طول جغرافیایی',
'exif-gpsaltituderef'              => 'منبع ارتفاع',
'exif-gpsaltitude'                 => 'ارتفاع',
'exif-gpstimestamp'                => 'وهد جی پی اس(ساع اتمی)',
'exif-gpssatellites'               => 'ماهواره آنی که په اندازه گرگ استفاده بنت',
'exif-gpsstatus'                   => 'وضعیت رسیور',
'exif-gpsmeasuremode'              => 'مدل اندازه گرگ',
'exif-gpsdop'                      => 'صحت اندازه گرگ',
'exif-gpsspeedref'                 => 'واحد سرعت',
'exif-gpsspeed'                    => 'سرعت رسیور جی پی اس',
'exif-gpstrackref'                 => 'منبع په مسیر حرکت',
'exif-gpstrack'                    => 'مسیر حرکت',
'exif-gpsimgdirectionref'          => 'منبع په مسیر عکس',
'exif-gpsimgdirection'             => 'مسیر عکس',
'exif-gpsmapdatum'                 => 'چه روش زمین پیمایی دیتا استفاده بیت',
'exif-gpsdestlatituderef'          => 'مرجع په عرض جغرافیایی مقصد',
'exif-gpsdestlatitude'             => 'عرض جغرافیای مقصد',
'exif-gpsdestlongituderef'         => 'مرجع په طول جغرافیای مقصد',
'exif-gpsdestlongitude'            => 'طول جغرافیای مقصد',
'exif-gpsdestbearingref'           => 'مرجع په تاب مقصد',
'exif-gpsdestbearing'              => 'تاب مقصد',
'exif-gpsdestdistanceref'          => 'مرجع په فاصله دان مقصد',
'exif-gpsdestdistance'             => 'فاصله تا مقصد',
'exif-gpsprocessingmethod'         => 'نام روش پردازش جی پی اس',
'exif-gpsareainformation'          => 'نام منطقه جی پی اس',
'exif-gpsdatestamp'                => 'تاریح جی پی اس',
'exif-gpsdifferential'             => 'اصلاح متفاوت جی پی اس',

# EXIF attributes
'exif-compression-1' => 'کمپرس نه بوتت',
'exif-compression-6' => 'جیی پی ای جی',

'exif-photometricinterpretation-2' => 'آی جی بی',
'exif-photometricinterpretation-6' => 'وای سی بی سی آر',

'exif-unknowndate' => 'ناشناس تاریح',

'exif-orientation-1' => 'نرمال', # 0th row: top; 0th column: left
'exif-orientation-2' => 'چپ بیگ افقی', # 0th row: top; 0th column: right
'exif-orientation-3' => 'گردگ 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'چپ بیگ عمودی', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'چرحتن 90° ضد ساعت گرد و چپ بیگ عمودی', # 0th row: left; 0th column: top
'exif-orientation-6' => 'چرحتن 90° ساعت گرد', # 0th row: right; 0th column: top
'exif-orientation-7' => 'چرحتن 90° ساعت گرد و چپ بیگ عمودی', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'چرتن 90°ساعت گرد', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'فرمتی چنکی',
'exif-planarconfiguration-2' => 'فرمت سطحی',

'exif-xyresolution-i' => '$1 دی پی آی',
'exif-xyresolution-c' => '$1 دی پی سی',

'exif-colorspace-1' => 'اس ار جی بی',

'exif-componentsconfiguration-0' => 'موجود نهنت',
'exif-componentsconfiguration-1' => 'وای',
'exif-componentsconfiguration-2' => 'سی بی',
'exif-componentsconfiguration-3' => 'سی آر',
'exif-componentsconfiguration-4' => 'س',
'exif-componentsconfiguration-5' => 'س',
'exif-componentsconfiguration-6' => 'ن',

'exif-exposureprogram-0' => 'تعریف نه بیتت',
'exif-exposureprogram-1' => 'دستی',
'exif-exposureprogram-2' => 'برنامه نرمال',
'exif-exposureprogram-3' => 'ترجیح سولاح',
'exif-exposureprogram-4' => 'ترجیح شاتر',
'exif-exposureprogram-5' => 'برنامه شرکنوک ( متمایل په عمق زمینه)',
'exif-exposureprogram-6' => 'برنامه کار (تمایل په سرعت سریع شاتر)',
'exif-exposureprogram-7' => 'حالت پرورتره(په نزیکین عکسان در چه تمرکز به پیش سر )',
'exif-exposureprogram-8' => 'حالت منظره (په تصاویر منظره ای گون تمرکز ته پیش صحنه)',

'exif-subjectdistance-value' => '$1 متر',

'exif-meteringmode-0'   => 'ناشناس',
'exif-meteringmode-1'   => 'میانگین',
'exif-meteringmode-2'   => 'میانگین وسط وزن',
'exif-meteringmode-3'   => 'نکته',
'exif-meteringmode-4'   => 'چندنکته ای',
'exif-meteringmode-5'   => 'الگو',
'exif-meteringmode-6'   => 'جزی',
'exif-meteringmode-255' => 'دگر',

'exif-lightsource-0'   => 'ناشناس',
'exif-lightsource-1'   => 'نور روچ',
'exif-lightsource-2'   => 'فلورسنت',
'exif-lightsource-3'   => 'تنگستن(نور اسپیت)',
'exif-lightsource-4'   => 'فلاش',
'exif-lightsource-9'   => 'وشین آپ و هوا',
'exif-lightsource-10'  => 'هوری آپ و هوا',
'exif-lightsource-11'  => 'ساهیل',
'exif-lightsource-12'  => 'فلورسنت نور روچ (D 5700 – 7100K)',
'exif-lightsource-13'  => 'فلورسنت اسپیت روچ (N 4600 – 5400K)',
'exif-lightsource-14'  => 'فلورسنت اسپیتء وشین (W 3900 – 4500K)',
'exif-lightsource-15'  => 'فلورسنت اسپیت(WW 3200 – 3700K)',
'exif-lightsource-17'  => 'نور استاندارد آ',
'exif-lightsource-18'  => 'نور استاندارد بی',
'exif-lightsource-19'  => 'نور استاندارد سی',
'exif-lightsource-20'  => 'د55',
'exif-lightsource-21'  => 'د56',
'exif-lightsource-22'  => 'ی57',
'exif-lightsource-23'  => 'د50',
'exif-lightsource-24'  => 'ایزو استدیو تنگستن',
'exif-lightsource-255' => 'دگ منبع نور',

'exif-focalplaneresolutionunit-2' => 'اینچ',

'exif-sensingmethod-1' => 'تعریف نه بوتگین',
'exif-sensingmethod-2' => 'سنسور ناحیه رنگ یک چیپ',
'exif-sensingmethod-3' => 'سنسور ناحیه رنگ دو چیپ',
'exif-sensingmethod-4' => 'سنسور ناحیه رنگ سه چیپ',
'exif-sensingmethod-5' => 'سنسور ناحیه ترتیبی رنگ',
'exif-sensingmethod-7' => 'سنسور سه خطی',
'exif-sensingmethod-8' => 'سنسور خطی ترکیبی رنگ',

'exif-filesource-3' => 'دی اس سی',

'exif-scenetype-1' => 'یک عکس مستقیمی گپتگین',

'exif-customrendered-0' => 'پردازش نرمال',
'exif-customrendered-1' => 'پردازش سنتی',

'exif-exposuremode-0' => 'مدت پچ بیگ دیافراگم دوربین',
'exif-exposuremode-1' => 'دستی پچ بیگ دیافراگ دوربین',
'exif-exposuremode-2' => 'اتوماتیکی پرانتز',

'exif-whitebalance-0' => 'اتوماتیکی توازن اسپیت',
'exif-whitebalance-1' => 'دستی توازن اسپیت',

'exif-scenecapturetype-0' => 'استاندارد',
'exif-scenecapturetype-1' => 'منظره',
'exif-scenecapturetype-2' => 'پرورتره',
'exif-scenecapturetype-3' => 'شپی صحنه',

'exif-gaincontrol-0' => 'هچ یک',
'exif-gaincontrol-1' => 'پایین گرگ برز',
'exif-gaincontrol-2' => 'بالا گرگ برز',
'exif-gaincontrol-3' => 'پایین گرگ جهل',
'exif-gaincontrol-4' => 'بالا گرگ بلند',

'exif-contrast-0' => 'نرمال',
'exif-contrast-1' => 'نرم',
'exif-contrast-2' => 'ترند',

'exif-saturation-0' => 'نرمال',
'exif-saturation-1' => 'اشباع کم',
'exif-saturation-2' => 'اشباع بالا',

'exif-sharpness-0' => 'نرمال',
'exif-sharpness-1' => 'نرم',
'exif-sharpness-2' => 'ترند',

'exif-subjectdistancerange-0' => 'ناشناس',
'exif-subjectdistancerange-1' => 'مزن',
'exif-subjectdistancerange-2' => 'نزیک گندگ',
'exif-subjectdistancerange-3' => 'دورین گندگ',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'عرض شمالی',
'exif-gpslatitude-s' => 'عرض جنوبی',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'طول شرقی',
'exif-gpslongitude-w' => 'طول غربی',

'exif-gpsstatus-a' => 'اندازه گرگ ته جریاننت',
'exif-gpsstatus-v' => 'اندازه گرگ بین عملی',

'exif-gpsmeasuremode-2' => 'اندازه گرگ 2-بعدی',
'exif-gpsmeasuremode-3' => 'اندازه گرگ 3-بعدی',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'کیلومتر ته ساعت',
'exif-gpsspeed-m' => 'مایل ته ساعت',
'exif-gpsspeed-n' => 'گرهنان',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'جهت درست',
'exif-gpsdirection-m' => 'مسیر آهن ربایی',

# External editor support
'edit-externally'      => 'ای صفحه یا اصلاح کن گون یک درآین برنامه ای',
'edit-externally-help' => 'په گیشترین اطلاعات بچار[http://www.mediawiki.org/wiki/Manual:External_editors setup instructions]',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'کل',
'imagelistall'     => 'کل',
'watchlistall2'    => 'کل',
'namespacesall'    => 'کل',
'monthsall'        => 'کل',

# E-mail address confirmation
'confirmemail'             => 'آدرس ایمیل تایید کن',
'confirmemail_noemail'     => 'شما یک معتبرین آدرس ایمیل تنظیم نه کتت نه وتی [[Special:Preferences|ترجیحات کاربر]].',
'confirmemail_text'        => '{{SITENAME}} لوٹیت که شما وتی آدرس ایمیلء تایید کنید پیش چه شی که سرویسان ایمیلی استفاده کنیت.
ای جهلی دکمه فعال کن تا یک ایمیل تایید په شمی آدرس دیم دنت.
ای ایمیل شامل یک لینکیت که کد همراه داریت;
ته وتی بروزر لینک پچ کن تا شمی آدرس ایمیل تایید بیت',
'confirmemail_pending'     => '<div class="error">یک کد تایید پیش تر په شما ایمیل بوتت;
اگر شما نوکی وتی حسابء شرکتت، شما بلکین چند دقیقه صبر کنیت تا آی برسیت پیش چه شی که یک نوکین درخواست په نوکین کتن کنیت.</div>',
'confirmemail_send'        => 'یک کد تایید ایمیل کن',
'confirmemail_sent'        => 'ایمیل تاییدی دیم دات',
'confirmemail_oncreate'    => 'یک کد تایید په شمی آدرس ایمیل دیم دهگ بوت.
ای کد په وارد بوتن نیاز نهنت، بله شما پیش چه فعال کتن هر دابین سرویس ایمیل ته ای ویکی بایدن آیی بیارت.',
'confirmemail_sendfailed'  => '{{SITENAME}}نه تونیت ایمیل تایید دیم دنت.
آدرس په خاطر کاراکتران نامعتبر کنترل کنیت.

ایمیل کنوک ترینت: $1',
'confirmemail_invalid'     => 'نامعتبر کد تایید.
شمی کد ممکننت تاریحی گوستت.',
'confirmemail_needlogin'   => 'شما را $1 نیازنت په تایید کتن وتی آدرس ایمیل',
'confirmemail_success'     => 'شمی آدرس ایمیل تایید بوتت.
شما الان تونت وارد بیت و چه ویکی سوب برت',
'confirmemail_loggedin'    => 'شمی آدرس ایمیل هنو تایید بوتت',
'confirmemail_error'       => 'لهتی چیز اشتباهت وهد ذخیره کتن شمی تایید.',
'confirmemail_subject'     => '{{SITENAME}} تایید آدرس ایمیل',
'confirmemail_body'        => 'یک نفر،بلکه شما، چه آی پی آدرس $1,
یک حسابی ثبت کتت "$2"  گون ای آدرس ایمیل ته {{SITENAME}}.

په تایید شی که واقعا ای حساب جه شماینت و فعال کتن ایمیل
مشحصات ته {{SITENAME}}، ته وتی بروزرء ای لینکء پچ کن:

$3

اگه شما ثبت نام *نه* کتت ای حسابء، رند چه ای لینک بروت
په کنسل کتن تایید آدرس ایمیل:

$5

ای کد تایید تا $4 وهدی هلیت.',
'confirmemail_invalidated' => 'تایید آدرس ایمیل کنسل بوت.',
'invalidateemail'          => 'کنسل کن تایید ایمیلء',

# Scary transclusion
'scarytranscludedisabled' => '[جاه په جاهی بین ویکی غیر فعالنت]',
'scarytranscludefailed'   => '[تمپلت آرگ پروش وارت په $1]',
'scarytranscludetoolong'  => '[URL باز مزننت]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">گرند گروگان ای صفحه:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 حذف])',
'trackbacklink'     => 'رند گر',
'trackbackdeleteok' => 'رند گر گون موفقیت حذف بوت.',

# Delete conflict
'deletedwhileediting' => "'''هوژاری''': ای صفحه حذف بوتت رند چه شمی اصلاح کتن شروه بیگ!",
'confirmrecreate'     => "کاربر [[User:$1|$1]] ([[User talk:$1|گپ]]) ای صفحهء حذف کتت بعد چه شی که شما اصلاح شروع کتت گون ای دلیل:
: ''$2''
لطفا تایید کنیت که واقعا شما لوٹیت ای صفحه دگه شرکنیت.",
'recreate'            => 'دگ شرکن',

'unit-pixel' => 'پیکس',

# HTML dump
'redirectingto' => 'دگه شرگنگنت په [[:$1]]...',

# action=purge
'confirm_purge'        => 'ذخیره ای صفحه پهک کنت؟

$1',
'confirm_purge_button' => 'هوبنت',

# AJAX search
'searchcontaining' => "گردگ په صفحات شامل ''$1''.",
'searchnamed'      => "گردگ په صفحات په نام ''$1''.",
'articletitles'    => "صفحات شروع بیت گون ''$1''",
'hideresults'      => 'پناه کن نتایجء',
'useajaxsearch'    => 'چه گردگ آژاکسی استفاده کن',

# Separators for various lists, etc.
'colon-separator'    => ':&#32;',
'autocomment-prefix' => '-',

# Multipage image navigation
'imgmultipageprev' => '← پیشگین صفحه',
'imgmultipagenext' => 'صفحه بعدی →',
'imgmultigo'       => 'برو!',
'imgmultigoto'     => 'برو به صفحه  $1',

# Table pager
'ascending_abbrev'         => 'بالادی',
'descending_abbrev'        => 'جهلادی',
'table_pager_next'         => 'صفحه بعدی',
'table_pager_prev'         => 'پیشگین صفحه',
'table_pager_first'        => 'اولی صفحه',
'table_pager_last'         => 'اهری صفحه',
'table_pager_limit'        => 'پیش دار  $1  ایتم ته هر صفحه',
'table_pager_limit_submit' => 'برو',
'table_pager_empty'        => 'بی نتیجه',

# Auto-summaries
'autosumm-blank'   => 'محتوا چه کل صفحه دور کنگنت',
'autosumm-replace' => "جاه په جاه کتن صفحه گون '$1'",
'autoredircomment' => 'غیر مستقیم روگنت په [[$1]]',
'autosumm-new'     => 'نوکین صفحه: $1',

# Size units
'size-bytes'     => '$1 ب',
'size-kilobytes' => '$1 ک.ب',
'size-megabytes' => '$1 م.ب',
'size-gigabytes' => '$1 گ.ب',

# Live preview
'livepreview-loading' => '...بار بیت',
'livepreview-ready'   => 'باربیت... حاضر!',
'livepreview-failed'  => 'زنده بازبینی پروش وارت. نرمال بازبینی سعی کن.',
'livepreview-error'   => 'پروش ته وصل بیگ :$1 "$2".  نرمال بازبینی سعی کن.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'تغییرات نوکتر چه {{PLURAL:$1|ثانیه|ثانیه}} ثانیه انت شاید ته ای لیست پجاه می کاینت.',
'lag-warn-high'   => 'خاطر بازگین تاخیر سرور دیتابیس، تغییرات نوکتر چه  {{PLURAL:$1|ثانیه|ثانیه}} شایدن ته ای لیست پیش دارگمه بنت.',

# Watchlist editor
'watchlistedit-numitems'       => 'شمی لیست چارگ شامل  {{PLURAL:$1|1 عنوان|$1 عناوین}}, بجز صفحات گپ.',
'watchlistedit-noitems'        => 'شمی لیست چارگ هچ عنوانی نداریت.',
'watchlistedit-normal-title'   => 'اصلاح لیست چارگ',
'watchlistedit-normal-legend'  => 'بزور عناوینء چه لیست چارگ',
'watchlistedit-normal-explain' => 'عناوین ته شمی لیست چارگ جهلء پیشدارگ بنت.
په زورتن یک عنوانی، جعبه کش آییء تیک زن، و کلیک کن زوگ عناوینء.
شما تونیت هنچوش [[Special:Watchlist/raw|لیست هام اصلاح کنیت]].',
'watchlistedit-normal-submit'  => 'بزور عناوینء',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 |$1 عنوانی ات}} چه شمی لیست چارگ حذف بوت:',
'watchlistedit-raw-title'      => 'اصلاح لیست چارگ هام',
'watchlistedit-raw-legend'     => 'اصلاح لیست چارگ هام',
'watchlistedit-raw-explain'    => 'عناوین ته شمی لیست چارگ جهلء پیش دارگ بنت،و توننت اصلاح بنت گون اضافه و زورگ چه لیست;
هر عنوانی ته یک خط.
وهدی که هلت، په روچ کتن لیست چارگ کلیک کن.
شما هنچو تونیت[[Special:Watchlist/edit|استفاده کنیت چه اصلاحگر استانداردء]].',
'watchlistedit-raw-titles'     => ':عناوین',
'watchlistedit-raw-submit'     => 'په روچ کن لیست چارگء',
'watchlistedit-raw-done'       => 'شمی لیست چارگ په روچ بیتگت',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 عنوان انت|$1 عناوین ات}} اضافه بوت:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 عنوان|$1 عناوین}} دور بوت:',

# Watchlist editing tools
'watchlisttools-view' => 'مربوطین تغییرات بچار',
'watchlisttools-edit' => 'به چار و اصلاح کن لیست چارگ آ',
'watchlisttools-raw'  => 'هامین لیست چارگ آ اصلاح کن',

# Iranian month names
'iranian-calendar-m1'  => 'فروردین',
'iranian-calendar-m2'  => 'اردیبهشت',
'iranian-calendar-m3'  => 'خرداد',
'iranian-calendar-m4'  => 'تیر',
'iranian-calendar-m5'  => 'آمرداد',
'iranian-calendar-m6'  => 'شهریور',
'iranian-calendar-m7'  => 'مهر',
'iranian-calendar-m8'  => 'آبان',
'iranian-calendar-m9'  => 'آذر',
'iranian-calendar-m10' => 'دی',
'iranian-calendar-m11' => 'بهمن',
'iranian-calendar-m12' => 'اسفند',

# Hebrew month names
'hebrew-calendar-m1'      => 'تشری',
'hebrew-calendar-m2'      => 'چشوان',
'hebrew-calendar-m3'      => 'کسلو',
'hebrew-calendar-m4'      => 'تووت',
'hebrew-calendar-m5'      => 'شووت',
'hebrew-calendar-m6'      => 'ادر',
'hebrew-calendar-m6a'     => 'ادر 1',
'hebrew-calendar-m6b'     => 'ادر 2',
'hebrew-calendar-m7'      => 'نیسان',
'hebrew-calendar-m8'      => 'لیار',
'hebrew-calendar-m9'      => 'سیوان',
'hebrew-calendar-m10'     => 'تموز',
'hebrew-calendar-m11'     => 'آو',
'hebrew-calendar-m12'     => 'الول',
'hebrew-calendar-m1-gen'  => 'تیشری',
'hebrew-calendar-m2-gen'  => 'چشوان',
'hebrew-calendar-m3-gen'  => 'کسلو',
'hebrew-calendar-m4-gen'  => 'تووت',
'hebrew-calendar-m5-gen'  => 'شووات',
'hebrew-calendar-m6-gen'  => 'ادر',
'hebrew-calendar-m6a-gen' => 'ادر1',
'hebrew-calendar-m6b-gen' => 'ادر 2',
'hebrew-calendar-m7-gen'  => 'نیسان',
'hebrew-calendar-m8-gen'  => 'لیار',
'hebrew-calendar-m9-gen'  => 'سیوان',
'hebrew-calendar-m10-gen' => 'تموز',
'hebrew-calendar-m11-gen' => 'آو',
'hebrew-calendar-m12-gen' => 'الول',

# Core parser functions
'unknown_extension_tag' => 'ناشناس برجسب الحاق  "$1"',

# Special:Version
'version'                          => 'نسخه', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'نصب بوتگیت الحاق آن',
'version-specialpages'             => 'حاصین صفحات',
'version-parserhooks'              => 'تجزیه کنوک گیر کت',
'version-variables'                => 'متغییران',
'version-other'                    => 'دگر',
'version-mediahandlers'            => 'دست گروک مدیا',
'version-hooks'                    => 'گیر کنت',
'version-extension-functions'      => 'عملگران الحاقی',
'version-parser-extensiontags'     => 'برچسپان الحاقی تجزیه گر',
'version-parser-function-hooks'    => 'عمل گر تجزیه کنوک گیر کت',
'version-skin-extension-functions' => 'عملگران الحاقی پوستک',
'version-hook-name'                => 'نام گیر',
'version-hook-subscribedby'        => 'اشتراک بیت گون',
'version-version'                  => 'نسخه',
'version-license'                  => 'لیسانس',
'version-software'                 => 'نصبین برنامه',
'version-software-product'         => 'محصول',
'version-software-version'         => 'نسخه',

# Special:FilePath
'filepath'         => 'مسیر فایل',
'filepath-page'    => 'فایل:',
'filepath-submit'  => 'مسیر',
'filepath-summary' => 'ای حاصین صفحه مسیر کامل په یک فایل پیش داریت.
تصاویر گون وضوح کامل پیش دارگ بنت و دگه نوع فایلان گون وتی برنامه یانش مستقیما پچ بنت.

نام فایل بی پسوند "{{ns:image}}:" وارد کن',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'گردگ په کپی  فایلان',
'fileduplicatesearch-summary'  => 'گردگ په کپی فایلان په اساس درهمین ارزش.

نا فایل بی پیش وند "{{ns:image}}:" وارد کنیت',
'fileduplicatesearch-legend'   => 'گردگ په  کپی',
'fileduplicatesearch-filename' => ':نام فایل',
'fileduplicatesearch-submit'   => 'گردگ',
'fileduplicatesearch-info'     => '$1 × $2 پیکسل<br />اندازه فایل: $3<br />نوع مایم: $4',
'fileduplicatesearch-result-1' => ' "$1" فایل هچ مشحصین دوبلی نیست.',
'fileduplicatesearch-result-n' => 'فایل "$1" has {{PLURAL:$2|1 identical duplication|$2 مشخصین کپی بوتن}}.',

# Special:SpecialPages
'specialpages'                   => 'حاصین صفحات',
'specialpages-note'              => '----
* نرمال صفحات حاص.
*  <span class="mw-specialpagerestricted">محدودین صفحات حاص.</span>',
'specialpages-group-maintenance' => 'گزارشات دارگ',
'specialpages-group-other'       => 'دگر حاصین صفحات',
'specialpages-group-login'       => 'ورود/ثبت نام',
'specialpages-group-changes'     => 'نوکین تغییرات و آمار',
'specialpages-group-media'       => 'گزارشات مدیا و آپلودان',
'specialpages-group-users'       => 'کابران و حقوق',
'specialpages-group-highuse'     => 'کاربرد بالای صفحات',
'specialpages-group-pages'       => 'لیست صفحات',
'specialpages-group-pagetools'   => 'وسایل صفحه',
'specialpages-group-wiki'        => 'وسایل و دیتا وی کی',
'specialpages-group-redirects'   => 'غیر مستقیم بیگنت صفحات حاصین',
'specialpages-group-spam'        => 'وسایل اسپم',

# Special:BlankPage
'blankpage'              => 'هالیکین صفحه',
'intentionallyblankpage' => 'ای صفحه عمدا هالیک هلگ بوتت و په محک زتن ویا دگه چیز.',

);
