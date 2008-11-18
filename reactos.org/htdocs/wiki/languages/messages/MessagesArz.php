<?php
/** Egyptian Spoken Arabic (مصرى)
 *
 * @ingroup Language
 * @file
 *
 * @author Alnokta
 * @author Ghaly
 * @author Meno25
 * @author Ramsis II
 * @author Ramsis1978
 */

$fallback = 'ar';

$namespaceNames = array(
	NS_MEDIA          => 'ميديا',
	NS_SPECIAL        => 'خاص',
	NS_TALK           => 'نقاش',
	NS_USER           => 'مستخدم',
	NS_USER_TALK      => 'نقاش_المستخدم',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => 'نقاش_$1',
	NS_IMAGE          => 'صورة',
	NS_IMAGE_TALK     => 'نقاش_الصورة',
	NS_MEDIAWIKI      => 'ميدياويكى',
	NS_MEDIAWIKI_TALK => 'نقاش_ميدياويكى',
	NS_TEMPLATE       => 'قالب',
	NS_TEMPLATE_TALK  => 'نقاش_القالب',
	NS_HELP           => 'مساعدة',
	NS_HELP_TALK      => 'نقاش_المساعدة',
	NS_CATEGORY       => 'تصنيف',
	NS_CATEGORY_TALK  => 'نقاش_التصنيف',
);

$namespaceAliases = array(
	'ملف'             => NS_MEDIA,
);

$magicWords = array(
	'redirect'            => array( '0', '#تحويل', '#REDIRECT' ),
	'notoc'               => array( '0', '__لافهرس__', '__NOTOC__' ),
	'nogallery'           => array( '0', '__لامعرض__', '__NOGALLERY__' ),
	'forcetoc'            => array( '0', '__لصق_فهرس__', '__FORCETOC__' ),
	'toc'                 => array( '0', '__فهرس__', '__TOC__' ),
	'noeditsection'       => array( '0', '__لاتحريرقسم__', '__NOEDITSECTION__' ),
	'currentmonth'        => array( '1', 'شهر_حالى', 'شهر', 'CURRENTMONTH' ),
	'currentmonthname'    => array( '1', 'اسم_الشهر_الحالى', 'اسم_شهر_حالى', 'اسم_شهر', 'CURRENTMONTHNAME' ),
	'currentmonthnamegen' => array( '1', 'اسم_الشهر_الحالى_المولد', 'اسم_شهر_حالى_مولد', 'CURRENTMONTHNAMEGEN' ),
	'currentmonthabbrev'  => array( '1', 'اختصار_الشهر_الحالى', 'اختصار_شهر_حالى', 'CURRENTMONTHABBREV' ),
	'currentday'          => array( '1', 'يوم_حالى', 'يوم', 'CURRENTDAY' ),
	'currentday2'         => array( '1', 'يوم_حالى2', 'يوم2', 'CURRENTDAY2' ),
	'currentdayname'      => array( '1', 'اسم_اليوم_الحالى', 'اسم_يوم_حالى', 'اسم_يوم', 'CURRENTDAYNAME' ),
	'currentyear'         => array( '1', 'عام_حالى', 'عام', 'CURRENTYEAR' ),
	'currenttime'         => array( '1', 'وقت_حالى', 'وقت', 'CURRENTTIME' ),
	'currenthour'         => array( '1', 'ساعة_حالية', 'ساعة', 'CURRENTHOUR' ),
	'localmonth'          => array( '1', 'شهر_محلى', 'LOCALMONTH' ),
	'localmonthname'      => array( '1', 'اسم_الشهر_المحلى', 'اسم_شهر_محلى', 'LOCALMONTHNAME' ),
	'localmonthnamegen'   => array( '1', 'اسم_الشهر_المحلى_المولد', 'اسم_شهر_محلى_مولد', 'LOCALMONTHNAMEGEN' ),
	'localmonthabbrev'    => array( '1', 'اختصار_الشهر_المحلى', 'اختصار_شهر_محلى', 'LOCALMONTHABBREV' ),
	'localday'            => array( '1', 'يوم_محلى', 'LOCALDAY' ),
	'localday2'           => array( '1', 'يوم_محلى2', 'LOCALDAY2' ),
	'localdayname'        => array( '1', 'اسم_اليوم_المحلى', 'اسم_يوم_محلى', 'LOCALDAYNAME' ),
	'localyear'           => array( '1', 'عام_محلى', 'LOCALYEAR' ),
	'localtime'           => array( '1', 'وقت_محلى', 'LOCALTIME' ),
	'localhour'           => array( '1', 'ساعة_محلية', 'LOCALHOUR' ),
	'numberofpages'       => array( '1', 'عدد_الصفحات', 'عدد_صفحات', 'NUMBEROFPAGES' ),
	'numberofarticles'    => array( '1', 'عدد_المقالات', 'عدد_مقالات', 'NUMBEROFARTICLES' ),
	'numberoffiles'       => array( '1', 'عدد_الملفات', 'عدد_ملفات', 'NUMBEROFFILES' ),
	'numberofusers'       => array( '1', 'عدد_المستخدمين', 'عدد_مستخدمين', 'NUMBEROFUSERS' ),
	'numberofedits'       => array( '1', 'عدد_التعديلات', 'عدد_تعديلات', 'NUMBEROFEDITS' ),
	'pagename'            => array( '1', 'اسم_الصفحة', 'اسم_صفحة', 'PAGENAME' ),
	'pagenamee'           => array( '1', 'عنوان_الصفحة', 'عنوان_صفحة', 'PAGENAMEE' ),
	'namespace'           => array( '1', 'نطاق', 'NAMESPACE' ),
	'namespacee'          => array( '1', 'عنوان_نطاق', 'NAMESPACEE' ),
	'talkspace'           => array( '1', 'نطاق_النقاش', 'نطاق_نقاش', 'TALKSPACE' ),
	'talkspacee'          => array( '1', 'عنوان_النقاش', 'عنوان_نقاش', 'TALKSPACEE' ),
	'subjectspace'        => array( '1', 'نطاق_الموضوع', 'نطاق_المقالة', 'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'       => array( '1', 'عنوان_نطاق_الموضوع', 'عنوان_نطاق_المقالة SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'        => array( '1', 'اسم_الصفحة_الكامل', 'اسم_صفحة_كامل', 'اسم_كامل', 'FULLPAGENAME' ),
	'fullpagenamee'       => array( '1', 'عنوان_الصفحة_الكامل', 'عنوان_صفحة_كامل', 'عنوان_كامل', 'FULLPAGENAMEE' ),
	'subpagename'         => array( '1', 'اسم_الصفحة_الفرعي', 'اسم_صفحة_فرعي', 'SUBPAGENAME' ),
	'subpagenamee'        => array( '1', 'عنوان_الصفحة_الفرعى', 'عنوان_صفحة_فرعى', 'SUBPAGENAMEE' ),
	'basepagename'        => array( '1', 'اسم_الصفحة_الأساسى', 'اسم_صفحة_أساسى', 'BASEPAGENAME' ),
	'basepagenamee'       => array( '1', 'عنوان_الصفحة_الأساسى', 'عنوان_صفحة_أساسى', 'BASEPAGENAMEE' ),
	'talkpagename'        => array( '1', 'اسم_صفحة_النقاش', 'اسم_صفحة_نقاش', 'TALKPAGENAME' ),
	'talkpagenamee'       => array( '1', 'عنوان_صفحة_النقاش', 'عنوان_صفحة_نقاش', 'TALKPAGENAMEE' ),
	'subjectpagename'     => array( '1', 'اسم_صفحة_الموضوع', 'اسم_صفحة_المقالة', 'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'    => array( '1', 'عنوان_صفحة_الموضوع', 'عنوان_صفحة_المقالة SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                 => array( '0', 'رسالة:', 'MSG:' ),
	'subst'               => array( '0', 'نسخ:', 'إحلال:', 'SUBST:' ),
	'msgnw'               => array( '0', 'مصدر:', 'مصدر_قالب:', 'MSGNW:' ),
	'img_thumbnail'       => array( '1', 'تصغير', 'thumbnail', 'thumb' ),
	'img_manualthumb'     => array( '1', 'تصغير=$1', 'thumbnail=$1', 'thumb=$1' ),
	'img_right'           => array( '1', 'يمين', 'right' ),
	'img_left'            => array( '1', 'يسار', 'left' ),
	'img_none'            => array( '1', 'بدون', 'بلا', 'none' ),
	'img_width'           => array( '1', '$1بك', '$1px' ),
	'img_center'          => array( '1', 'مركز', 'center', 'centre' ),
	'img_framed'          => array( '1', 'إطار', 'framed', 'enframed', 'frame' ),
	'img_frameless'       => array( '1', 'لاإطار', 'frameless' ),
	'img_page'            => array( '1', 'صفحة=$1', 'صفحة $1', 'page=$1', 'page $1' ),
	'img_upright'         => array( '1', 'معدول', 'معدول=$1', 'معدول $1', 'upright', 'upright=$1', 'upright $1' ),
	'img_border'          => array( '1', 'حد', 'حدود', 'border' ),
	'img_baseline'        => array( '1', 'خط_أساسى', 'baseline' ),
	'img_sub'             => array( '1', 'فرعى', 'sub' ),
	'img_super'           => array( '1', 'سوبر', 'سب', 'super', 'sup' ),
	'img_top'             => array( '1', 'أعلى', 'top' ),
	'img_text_top'        => array( '1', 'نص_أعلى', 'text-top' ),
	'img_middle'          => array( '1', 'وسط', 'middle' ),
	'img_bottom'          => array( '1', 'أسفل', 'bottom' ),
	'img_text_bottom'     => array( '1', 'نص_أسفل', 'text-bottom' ),
	'int'                 => array( '0', 'محتوى:', 'INT:' ),
	'sitename'            => array( '1', 'اسم_الموقع', 'اسم_موقع', 'SITENAME' ),
	'ns'                  => array( '0', 'نط:', 'NS:' ),
	'localurl'            => array( '0', 'مسار_محلى:', 'LOCALURL:' ),
	'localurle'           => array( '0', 'عنوان_المسار_المحلى:', 'عنوان_مسار_محلى:', 'LOCALURLE:' ),
	'server'              => array( '0', 'خادم', 'SERVER' ),
	'servername'          => array( '0', 'اسم_الخادم', 'اسم_خادم', 'SERVERNAME' ),
	'scriptpath'          => array( '0', 'مسار_السكريبت', 'مسار_سكريبت', 'SCRIPTPATH' ),
	'grammar'             => array( '0', 'قواعد_اللغة:', 'قواعد_لغة: GRAMMAR:' ),
	'notitleconvert'      => array( '0', '__لاتحويل_عنوان__', '__لاتع__', '__NOTITLECONVERT__', '__NOTC__' ),
	'nocontentconvert'    => array( '0', '__لاتحويل_محتوى__', '__لاتم__', '__NOCONTENTCONVERT__', '__NOCC__' ),
	'currentweek'         => array( '1', 'أسبوع_حالى', 'أسبوع', 'CURRENTWEEK' ),
	'currentdow'          => array( '1', 'يوم_حالي_مأ', 'CURRENTDOW' ),
	'localweek'           => array( '1', 'أسبوع_محلى', 'LOCALWEEK' ),
	'localdow'            => array( '1', 'يوم_محلى_مأ', 'LOCALDOW' ),
	'revisionid'          => array( '1', 'رقم_النسخة', 'رقم_نسخة', 'REVISIONID' ),
	'revisionday'         => array( '1', 'يوم_النسخة', 'يوم_نسخة', 'REVISIONDAY' ),
	'revisionday2'        => array( '1', 'يوم_النسخة2', 'يوم_نسخة2', 'REVISIONDAY2' ),
	'revisionmonth'       => array( '1', 'شهر_النسخة', 'شهر_نسخة', 'REVISIONMONTH' ),
	'revisionyear'        => array( '1', 'عام_النسخة', 'عام_نسخة', 'REVISIONYEAR' ),
	'revisiontimestamp'   => array( '1', 'طابع_وقت_النسخة', 'طابع_وقت_نسخة', 'REVISIONTIMESTAMP' ),
	'plural'              => array( '0', 'جمع:', 'PLURAL:' ),
	'fullurl'             => array( '0', 'عنوان_كامل:', 'FULLURL:' ),
	'fullurle'            => array( '0', 'مسار_كامل:', 'FULLURLE:' ),
	'lcfirst'             => array( '0', 'عنوان_كبير:', 'LCFIRST:' ),
	'ucfirst'             => array( '0', 'عنوان_صغير:', 'UCFIRST:' ),
	'lc'                  => array( '0', 'صغير:', 'LC:' ),
	'uc'                  => array( '0', 'كبير:', 'UC:' ),
	'raw'                 => array( '0', 'خام:', 'RAW:' ),
	'displaytitle'        => array( '1', 'عرض_العنوان', 'عرض_عنوان', 'DISPLAYTITLE' ),
	'rawsuffix'           => array( '1', 'أر', 'آر', 'R' ),
	'newsectionlink'      => array( '1', '__وصلة_قسم_جديد__', '__NEWSECTIONLINK__' ),
	'currentversion'      => array( '1', 'نسخة_حالية', 'CURRENTVERSION' ),
	'urlencode'           => array( '0', 'كود_المسار:', 'كود_مسار:', 'URLENCODE:' ),
	'anchorencode'        => array( '0', 'كود_الأنكور', 'كود_أنكور', 'ANCHORENCODE' ),
	'currenttimestamp'    => array( '1', 'طابع_الوقت_الحالي', 'طابع_وقت_حالي', 'CURRENTTIMESTAMP' ),
	'localtimestamp'      => array( '1', 'طابع_الوقت_المحلى', 'طابع_وقت_محلى', 'LOCALTIMESTAMP' ),
	'directionmark'       => array( '1', 'علامة_الاتجاه', 'علامة_اتجاه', 'DIRECTIONMARK', 'DIRMARK' ),
	'language'            => array( '0', '#لغة:', '#LANGUAGE:' ),
	'contentlanguage'     => array( '1', 'لغة_المحتوى', 'لغة_محتوى', 'CONTENTLANGUAGE', 'CONTENTLANG' ),
	'pagesinnamespace'    => array( '1', 'صفحات_في_نطاق:', 'صفحات_في_نط:', 'PAGESINNAMESPACE:', 'PAGESINNS:' ),
	'numberofadmins'      => array( '1', 'عدد_الإداريين', 'عدد_إداريين', 'NUMBEROFADMINS' ),
	'formatnum'           => array( '0', 'صيغة_رقم', 'FORMATNUM' ),
	'padleft'             => array( '0', 'باد_يسار', 'PADLEFT' ),
	'padright'            => array( '0', 'باد_يمين', 'PADRIGHT' ),
	'special'             => array( '0', 'خاص', 'special' ),
	'defaultsort'         => array( '1', 'ترتيب_قياسى:', 'ترتيب_افتراضى:', 'مفتاح_ترتيب_قياسى:', 'مفتاح_ترتيب_افتراضى:', 'ترتيب_تصنيف_قياسى:', 'ترتيب_تصنيف_افتراضى:', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:' ),
	'filepath'            => array( '0', 'مسار_الملف:', 'مسار_ملف:', 'FILEPATH:' ),
	'tag'                 => array( '0', 'وسم', 'tag' ),
	'hiddencat'           => array( '1', '__تصنيف_مخفي__', '__HIDDENCAT__' ),
	'pagesincategory'     => array( '1', 'صفحات_في_التصنيف', 'PAGESINCATEGORY', 'PAGESINCAT' ),
	'pagesize'            => array( '1', 'حجم_الصفحة', 'حجم_صفحة', 'PAGESIZE' ),
	'staticredirect'      => array( '1', '__تحويلة_إستاتيكية__', '__تحويلة_ساكنة__', '__STATICREDIRECT__' ),
);

$skinNames = array(
	'standard'    => 'كلاسيك',
	'nostalgia'   => 'نوستالجيا',
	'cologneblue' => 'كولون بلو',
	'monobook'    => 'مونوبوك',
	'myskin'      => 'ماى سكين',
	'chick'       => 'تشيك',
	'simple'      => 'سيمبل',
	'modern'      => 'مودرن',
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'تحويلات_مزدوجة' ),
	'BrokenRedirects'           => array( 'تحويلات_مكسورة' ),
	'Disambiguations'           => array( 'توضيحات' ),
	'Userlogin'                 => array( 'دخول_المستخدم' ),
	'Userlogout'                => array( 'خروج_المستخدم' ),
	'CreateAccount'             => array( 'إنشاء_حساب' ),
	'Preferences'               => array( 'تفضيلات' ),
	'Watchlist'                 => array( 'قايمة_المراقبة' ),
	'Recentchanges'             => array( 'أحدث_التغييرات' ),
	'Upload'                    => array( 'رفع' ),
	'Imagelist'                 => array( 'قايمة_الصور' ),
	'Newimages'                 => array( 'صور_جديدة' ),
	'Listusers'                 => array( 'عرض_المستخدمين', 'قايمة_المستخدمين' ),
	'Listgrouprights'           => array( 'عرض_صلاحيات_المجموعات' ),
	'Statistics'                => array( 'إحصائيات' ),
	'Randompage'                => array( 'عشوائي', 'صفحة_عشوائية' ),
	'Lonelypages'               => array( 'صفحات_وحيدة', 'صفحات_يتيمة' ),
	'Uncategorizedpages'        => array( 'صفحات_غير_مصنفة' ),
	'Uncategorizedcategories'   => array( 'تصنيفات_غير_مصنفة' ),
	'Uncategorizedimages'       => array( 'صور_غير_مصنفة' ),
	'Uncategorizedtemplates'    => array( 'قوالب_غير_مصنفة' ),
	'Unusedcategories'          => array( 'تصنيفات_غير_مستخدمة' ),
	'Unusedimages'              => array( 'صور_غير_مستخدمة' ),
	'Wantedpages'               => array( 'صفحات_مطلوبة', 'وصلات_مكسورة' ),
	'Wantedcategories'          => array( 'تصنيفات_مطلوبة' ),
	'Missingfiles'              => array( 'ملفات_مفقودة', 'صور_مفقودة' ),
	'Mostlinked'                => array( 'الأكثر_وصلا' ),
	'Mostlinkedcategories'      => array( 'أكثر_التصنيفات_وصلا', 'أكثر_التصنيفات_استخداما' ),
	'Mostlinkedtemplates'       => array( 'أكثر_القوالب_وصلا', 'أكثر_القوالب_استخداما' ),
	'Mostcategories'            => array( 'أكثر_التصنيفات' ),
	'Mostimages'                => array( 'أكثر_الصور' ),
	'Mostrevisions'             => array( 'أكثر_المراجعات' ),
	'Fewestrevisions'           => array( 'أقل_المراجعات' ),
	'Shortpages'                => array( 'صفحات_قصيرة' ),
	'Longpages'                 => array( 'صفحات_طويلة' ),
	'Newpages'                  => array( 'صفحات_جديدة' ),
	'Ancientpages'              => array( 'صفحات_قديمة' ),
	'Deadendpages'              => array( 'صفحات_نهاية_مسدودة' ),
	'Protectedpages'            => array( 'صفحات_محمية' ),
	'Protectedtitles'           => array( 'عناوين_محمية' ),
	'Allpages'                  => array( 'كل_الصفحات' ),
	'Prefixindex'               => array( 'فهرس_بادئة' ),
	'Ipblocklist'               => array( 'قائمة_منع_أيبى' ),
	'Specialpages'              => array( 'صفحات_خاصة' ),
	'Contributions'             => array( 'مساهمات' ),
	'Emailuser'                 => array( 'مراسلة_المستخدم' ),
	'Confirmemail'              => array( 'تأكيد_البريد' ),
	'Whatlinkshere'             => array( 'ماذا_يصل_هنا' ),
	'Recentchangeslinked'       => array( 'أحدث_التغييرات_الموصولة', 'تغييرات_مرتبطة' ),
	'Movepage'                  => array( 'نقل_صفحة' ),
	'Blockme'                   => array( 'منعى' ),
	'Booksources'               => array( 'مصادر_كتاب' ),
	'Categories'                => array( 'تصنيفات' ),
	'Export'                    => array( 'تصدير' ),
	'Version'                   => array( 'إصدار' ),
	'Allmessages'               => array( 'كل_الرسايل' ),
	'Log'                       => array( 'سجل', 'سجلات' ),
	'Blockip'                   => array( 'منع_أيبى' ),
	'Undelete'                  => array( 'استرجاع' ),
	'Import'                    => array( 'استيراد' ),
	'Lockdb'                    => array( 'قفل_قب' ),
	'Unlockdb'                  => array( 'فتح_قب' ),
	'Userrights'                => array( 'صلاحيات_المستخدم' ),
	'MIMEsearch'                => array( 'بحث_ميم' ),
	'FileDuplicateSearch'       => array( 'بحث_ملف_مكرر' ),
	'Unwatchedpages'            => array( 'صفحات_غير_مراقبة' ),
	'Listredirects'             => array( 'عرض_التحويلات' ),
	'Revisiondelete'            => array( 'حذف_نسخة' ),
	'Unusedtemplates'           => array( 'قوالب_غير_مستخدمة' ),
	'Randomredirect'            => array( 'تحويلة_عشوائية' ),
	'Mypage'                    => array( 'صفحتى' ),
	'Mytalk'                    => array( 'نقاشى' ),
	'Mycontributions'           => array( 'مساهماتى' ),
	'Listadmins'                => array( 'عرض_الإداريين' ),
	'Listbots'                  => array( 'عرض_البوتات' ),
	'Popularpages'              => array( 'صفحات_مشهورة' ),
	'Search'                    => array( 'بحث' ),
	'Resetpass'                 => array( 'ضبط_كلمة_السر' ),
	'Withoutinterwiki'          => array( 'بدون_إنترويكى' ),
	'MergeHistory'              => array( 'دمج_التاريخ' ),
	'Filepath'                  => array( 'مسار_ملف' ),
	'Invalidateemail'           => array( 'تعطيل_البريد_الإلكترونى' ),
	'Blankpage'                 => array( 'صفحة_فارغة' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'حط خط تحت الوصلات:',
'tog-highlightbroken'         => 'أ بين اللينكات البايظة <a href="" class="new">كدا</a> (البديل: زى دا<a href="" class="internal">؟</a>).',
'tog-justify'                 => 'ساوى الفقرات',
'tog-hideminor'               => 'خبى التعديلات الصغيرة فى اجدد التغييرات',
'tog-extendwatchlist'         => 'وسع لستة المراقبة علشان تبين كل التغييرات اللى ممكن تتطبق',
'tog-usenewrc'                => 'شكل قوى من أجدد التغييرات (جافا سكريبت)',
'tog-numberheadings'          => 'رقم العناوين تلقائيا',
'tog-showtoolbar'             => 'بين شريط التحرير (جافا سكريبت)',
'tog-editondblclick'          => 'عدل الصفحات عند الدبل كليك (جافا سكريبت)',
'tog-editsection'             => 'اسمح ب تعديل الأقسام عن طريق وصلات [تعديل]',
'tog-editsectiononrightclick' => 'اسمح ب تعديل الأقسام لما ندوس رايت كليك على الماوس على عناوين الأقسام (جافاسكريبت)',
'tog-showtoc'                 => 'بين جدول المحتويات (ل الصفحات االلى فيها أكتر من 3 عناوين)',
'tog-rememberpassword'        => 'خليك فاكر دخولى على الكمبيوتر دا',
'tog-editwidth'               => 'صندوق التعديل  واخد العرض كله',
'tog-watchcreations'          => 'ضيف الصفحات اللى أنشأتها للستة الصفحات اللى باراقبها',
'tog-watchdefault'            => 'ضيف الصفحات اللى بأعدلها للستة الصفحات اللى باراقبها',
'tog-watchmoves'              => 'ضيف الصفحات اللى بأنقلها للستة الصفحات اللى باراقبها',
'tog-watchdeletion'           => 'ضيف الصفحات اللى بأمسحها للستة الصفحات اللى باراقبها',
'tog-minordefault'            => 'علم  على كل التعديلات كأنها صغيرة افتراضيا',
'tog-previewontop'            => 'بين البروفة قبل صندوق التعديل',
'tog-previewonfirst'          => 'بين البروفة عند أول تعديل',
'tog-nocache'                 => 'عطل تخبية الصفحه',
'tog-enotifwatchlistpages'    => 'ابعت لى ايميل لما تتغير صفحه فى لستة الصفحات اللى باراقبها',
'tog-enotifusertalkpages'     => 'ابعتلى ايميل لما صفحة مناقشتى تتغيير',
'tog-enotifminoredits'        => 'ابعتلى ايميل للتعديلات الصغيره للصفحات',
'tog-enotifrevealaddr'        => 'بين الايميل بتاعى فى ايميلات الاعلام',
'tog-shownumberswatching'     => 'بين عدد اليوزرز المراقبين',
'tog-fancysig'                => 'امضاء خام (من غير لينك أوتوماتيكي)',
'tog-externaleditor'          => 'استعمل محرر خارجى افتراضيا',
'tog-externaldiff'            => 'استعمل فرق خارجى افتراضيا',
'tog-showjumplinks'           => 'خلى وصلات "روح لـ" تكون شغالة.',
'tog-uselivepreview'          => 'استخدم البروفة السريعة (جافاسكريبت) (تجريبي)',
'tog-forceeditsummary'        => 'نبهنى عند تدخيل ملخص للتعديل  فاضي',
'tog-watchlisthideown'        => 'خبى التعديلات بتاعتى من لستة المراقبة',
'tog-watchlisthidebots'       => 'خبى التعديلات بتاعة البوت من لستة المراقبة',
'tog-watchlisthideminor'      => 'خبى التعديلات البسيطة من لستة المراقبة',
'tog-nolangconversion'        => 'عطل تحويل اللهجات',
'tog-ccmeonemails'            => 'ابعتلى  نسخ من رسايل الايميل اللى بابعتها لليوزرز التانيين',
'tog-diffonly'                => 'ما تبين ش محتوى الصفحة تحت الفروقات',
'tog-showhiddencats'          => 'بين التّصنيفات المستخبية',

'underline-always'  => 'دايما',
'underline-never'   => 'ابدا',
'underline-default' => 'على حسب إعدادات المتصفح',

'skinpreview' => '(عرض)',

# Dates
'sunday'        => 'الحد',
'monday'        => 'الاتنين',
'tuesday'       => 'التلات',
'wednesday'     => 'الاربع',
'thursday'      => 'الخميس',
'friday'        => 'الجمعه',
'saturday'      => 'السبت',
'sun'           => 'الحد',
'mon'           => 'الاتنين',
'tue'           => 'التلات',
'wed'           => 'الاربع',
'thu'           => 'الخميس',
'fri'           => 'الجمعه',
'sat'           => 'السبت',
'january'       => 'يناير',
'february'      => 'فبراير',
'march'         => 'مارس',
'april'         => 'ابريل',
'may_long'      => 'مايو',
'june'          => 'يونيه',
'july'          => 'يوليه',
'august'        => 'اغسطس',
'september'     => 'سبتمبر',
'october'       => 'اكتوبر',
'november'      => 'نوفمبر',
'december'      => 'ديسمبر',
'january-gen'   => 'يناير',
'february-gen'  => 'فبراير',
'march-gen'     => 'مارس',
'april-gen'     => 'ابريل',
'may-gen'       => 'مايو',
'june-gen'      => 'يونيه',
'july-gen'      => 'يوليه',
'august-gen'    => 'اغسطس',
'september-gen' => 'سبتمبر',
'october-gen'   => 'اكتوبر',
'november-gen'  => 'نوفمبر',
'december-gen'  => 'ديسمبر',
'jan'           => 'يناير',
'feb'           => 'فبراير',
'mar'           => 'مارس',
'apr'           => 'ابريل',
'may'           => 'مايو',
'jun'           => 'يونيه',
'jul'           => 'يوليه',
'aug'           => 'اغسطس',
'sep'           => 'سبتمبر',
'oct'           => 'اكتوبر',
'nov'           => 'نوفمبر',
'dec'           => 'ديسمبر',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|تصنيف|تصانيف}}',
'category_header'                => 'الصفحات فى التصنيف "$1"',
'subcategories'                  => 'التصنيفات الفرعيه',
'category-media-header'          => 'ملفات الميديا فى التصنيف "$1"',
'category-empty'                 => "''التصنيف ده مافيهوش حاليا مقالات او ملفات ميديا.''",
'hidden-categories'              => '{{PLURAL:$1|تصنيف مستخبي|تصنيفات مستخبية}}',
'hidden-category-category'       => 'تصنيفات مستخبية', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2| التصنيف دا فيه  التصنيف الفرعى الجاى بس.|التصنيف دا فيه {{PLURAL:$1|تصنيف فرعى|$1 تصنيف فرعى}}، من إجمالى $2.}}',
'category-subcat-count-limited'  => ' التصنيف دا فيه {{PLURAL:$1|تصنيف فرعي|$1 تصنيف فرعي}} كدا.',
'category-article-count'         => '{{PLURAL:$2| التصنيف دا فيه  الصفحة دى بس.|تحت {{PLURAL:$1|ملف|$1 ملف}} فى  التصنيف دا ، من إجمالى $2.}}',
'category-article-count-limited' => 'تحت {{PLURAL:$1|صفحة|$1 صفحة}} فى التصنيف الحالى.',
'category-file-count'            => '{{PLURAL:$2| التصنيف دا  فيه الملف الجاى دا بس.|تحت {{PLURAL:$1|ملف|$1 ملف}} فى  التصنيف دا، من إجمالى $2.}}',
'category-file-count-limited'    => 'تحت {{PLURAL:$1|ملف|$1 ملف}} فى التصنيف الحالى.',
'listingcontinuesabbrev'         => 'متابعه',

'mainpagetext'      => "<big>''' ميدياويكى اتنزلت بنجاح.'''</big>",
'mainpagedocfooter' => 'اسال [http://meta.wikimedia.org/wiki/Help:Contents دليل اليوزر] للمعلومات حوالين استخدام برنامج الويكى.

== البداية ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings لستة اعدادات الضبط]
* [http://www.mediawiki.org/wiki/Manual:FAQ أسئلة بتكرر حوالين الميدياويكى]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce لستة الايميلات بتاعة اعلانات الميدياويكى]',

'about'          => 'عن',
'article'        => 'صفحة محتوى',
'newwindow'      => '(بتفتح ويندو جديده)',
'cancel'         => 'كانسل',
'qbfind'         => 'تدوير',
'qbbrowse'       => 'تصفح',
'qbedit'         => 'عدل',
'qbpageoptions'  => ' الصفحه دى',
'qbpageinfo'     => 'السياق',
'qbmyoptions'    => 'صفحاتى',
'qbspecialpages' => 'الصفحات الخاصة',
'moredotdotdot'  => 'اكتر...',
'mypage'         => 'صفحتى',
'mytalk'         => 'مناقشاتى',
'anontalk'       => 'المناقشة مع عنوان الأيبى دا',
'navigation'     => 'ابحار',
'and'            => 'و',

# Metadata in edit box
'metadata_help' => 'ميتا داتا:',

'errorpagetitle'    => 'غلطه',
'returnto'          => 'ارجع ل $1.',
'tagline'           => 'من {{SITENAME}}',
'help'              => 'مساعده',
'search'            => 'تدوير',
'searchbutton'      => 'تدوير',
'go'                => 'روح',
'searcharticle'     => 'روح',
'history'           => 'تاريخ الصفحه',
'history_short'     => 'تاريخ',
'updatedmarker'     => 'اتحدثت بعد زيارتى الأخيرة',
'info_short'        => 'معلومات',
'printableversion'  => 'نسخه للطبع',
'permalink'         => 'وصله مستديمه',
'print'             => 'اطبع',
'edit'              => 'تعديل',
'create'            => 'أنشيء',
'editthispage'      => 'عدل الصفحه دى',
'create-this-page'  => 'أنشيء الصفحه دى',
'delete'            => 'مسح',
'deletethispage'    => 'امسح الصفحه دى',
'undelete_short'    => 'استرجاع {{PLURAL:$1|تعديل واحد|تعديلان|$1 تعديلات|$1 تعديل|$1 تعديلا}}',
'protect'           => 'حمايه',
'protect_change'    => 'غير',
'protectthispage'   => 'احمى الصفحه دى',
'unprotect'         => 'الغى الحماية',
'unprotectthispage' => 'شيل حماية الصفحه دى',
'newpage'           => 'صفحه جديده',
'talkpage'          => 'ناقش الصفحه دى',
'talkpagelinktext'  => 'مناقشه',
'specialpage'       => 'صفحة مخصوصة',
'personaltools'     => 'ادوات شخصيه',
'postcomment'       => 'ابعت تعليق',
'articlepage'       => 'بين صفحة المحتوى',
'talk'              => 'مناقشه',
'views'             => 'مشاهده',
'toolbox'           => 'علبة العده',
'userpage'          => 'عرض صفحة اليوزر',
'projectpage'       => 'عرض صفحة المشروع',
'imagepage'         => 'عرض صفحة الميديا',
'mediawikipage'     => 'عرض صفحة الرسالة',
'templatepage'      => 'عرض صفحة القالب',
'viewhelppage'      => 'بين صفحة المساعدة',
'categorypage'      => 'عرض صفحة التصنيف',
'viewtalkpage'      => 'بين المناقشة',
'otherlanguages'    => 'بلغات تانيه',
'redirectedfrom'    => '(تحويل من $1)',
'redirectpagesub'   => 'صفحة تحويل',
'lastmodifiedat'    => 'الصفحة دى اتعدلت اخر مرة فى $2، $1.', # $1 date, $2 time
'viewcount'         => 'الصفحة دى اتدخل عليها{{PLURAL:$1|مرة واحدة|مرتين|$1 مرات|$1 مرة}}.',
'protectedpage'     => 'صفحه محميه',
'jumpto'            => 'روح على:',
'jumptonavigation'  => 'ناڤيجيشن',
'jumptosearch'      => 'تدوير',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'عن {{SITENAME}}',
'aboutpage'            => 'Project:معلومات عن',
'bugreports'           => 'بلاغ الاخطاء',
'bugreportspage'       => 'Project:تبليغ اخطاء',
'copyright'            => 'المحتوى موجود تحت $1.',
'copyrightpagename'    => 'حقوق النسخ فى {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:حقوق النسخ',
'currentevents'        => 'الاحداث الحاليه',
'currentevents-url'    => 'Project:الاحداث الحاليه',
'disclaimers'          => 'اخلاء مسؤوليه',
'disclaimerpage'       => 'Project:اخلاء مسؤوليه عمومى',
'edithelp'             => 'مساعده فى التعديل',
'edithelppage'         => 'Help:تعديل',
'faq'                  => 'اسئله بتتسئل كتير',
'faqpage'              => 'Project:اسئله بتتسئل كتير',
'helppage'             => 'Help:محتويات',
'mainpage'             => 'الصفحه الرئيسيه',
'mainpage-description' => 'الصفحه الرئيسيه',
'policy-url'           => 'Project:سياسة',
'portal'               => 'بوابة المجتمع',
'portal-url'           => 'Project:بوابة المجتمع',
'privacy'              => 'خصوصيه',
'privacypage'          => 'Project:سياسة الخصوصيه',

'badaccess'        => 'غلطه فى السماح',
'badaccess-group0' => 'انت مش مسموح لك تنفذ الطلب بتاعك',
'badaccess-group1' => 'الفعل االلى طلبته مسموح بس لليوزرز فى المجموعة $1.',
'badaccess-group2' => 'الفعل اللى طلبته مسموح بس لليوزرز فى واحدة من المجموعات $1.',
'badaccess-groups' => 'الفعل الذى طلبته مسموح بيه بس لليوزرز  اللى فى واحدة من المجموعات دى  $1.',

'versionrequired'     => 'لازم نسخة $1 من ميدياويكي',
'versionrequiredtext' => 'النسخة $1 من ميدياويكى لازم علشان تستعمل الصفحة دى.
شوف [[Special:Version|صفحة النسخة]]',

'ok'                      => 'موافئ',
'retrievedfrom'           => 'اتجابت من "$1"',
'youhavenewmessages'      => 'عندك $1 ($2).',
'newmessageslink'         => 'رسايل جديده',
'newmessagesdifflink'     => 'اخر تعديل',
'youhavenewmessagesmulti' => 'عندك ميسيدج جديدة فى $1',
'editsection'             => 'تعديل',
'editold'                 => 'تعديل',
'viewsourceold'           => 'عرض المصدر',
'editsectionhint'         => 'تعديل جزء : $1',
'toc'                     => 'المحتويات',
'showtoc'                 => 'عرض',
'hidetoc'                 => 'تخبيه',
'thisisdeleted'           => 'عرض او استرجاع $1؟',
'viewdeleted'             => 'عرض $1؟',
'restorelink'             => '{{PLURAL:$1|تعديل واحد ملغي|تعديلين ملغيين|$1 تعديلات ملغية|$1 تعديل ملغي|$1 تعديل ملغي}}',
'feedlinks'               => 'تلقيم:',
'feed-invalid'            => 'نوع اشتراك التغذية مش صح.',
'feed-unavailable'        => 'التغذية مش متوفرة',
'site-rss-feed'           => '$1   ار‌ اس‌ اس فييد',
'site-atom-feed'          => '$1 اتوم فييد',
'page-rss-feed'           => '"$1" ار‌ اس‌ اس فييد',
'page-atom-feed'          => '"$1" فييد أتوم',
'red-link-title'          => '$1 (لسة ما اتكتبت ش )',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'صفحة',
'nstab-user'      => 'صفحة يوزر',
'nstab-media'     => 'صفحة ميديا',
'nstab-special'   => 'مخصوص',
'nstab-project'   => 'صفحة مشروع',
'nstab-image'     => 'فايل',
'nstab-mediawiki' => 'رساله',
'nstab-template'  => 'قالب',
'nstab-help'      => 'صفحة مساعدة',
'nstab-category'  => 'تصنيف',

# Main script and global functions
'nosuchaction'      => 'مافيش فعل زى كده',
'nosuchactiontext'  => 'الويكى ما تعرفتش على الامر فى ال URL',
'nosuchspecialpage' => 'مافيش صفحة خاصة بالاسم ده',
'nospecialpagetext' => "<big>'''انت طلبت صفحة مخصوصة مش صحيحة.'''</big>

لستة الصفحات المخصوصة الصحيحة ممكن تلاقيها فى [[Special:SpecialPages]].",

# General errors
'error'                => 'غلطة',
'databaseerror'        => 'غلط فى قاعدة البيانات',
'dberrortext'          => 'حصل غلط فى صيغة الاستعلام.
ممكن يكون فى عيب فى البرنامج.
آخر استعلام اتطلب من قاعدة البيانات كان:
<blockquote><tt>$1</tt></blockquote>
من جوا الدالة "<tt>$2</tt>".
MySQL  رجعت الغلط "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'حصل غلط فى صيغة الاستعلام.
آخر استعلام اتطلب من قاعدة البيانات كان:
"$1"
من جوا الدالة "$2".
MySQL رجعت الغلط "$3: $4"',
'noconnect'            => 'الويكى عندها شوية مشاكل فنية و مش قادرة توصل بسيرفر الداتابيز. <br />
$1',
'nodb'                 => 'ماقدرناش نختار قاعدة البيانات $1',
'cachederror'          => 'دى نسخة متخبية من الصفحة اللى طلبتها، و ممكن ما تكونش متحدثة.',
'laggedslavemode'      => 'تحذير: الصفحة يمكن ما فيهاش اخر التحديثات.',
'readonly'             => 'قاعدة البيانات مقفولة',
'enterlockreason'      => 'اكتب سبب القفل، وقول امتى تقريبا ح يتلغى القفل',
'readonlytext'         => 'قاعدة البيانات مقفولة دلوقتى قدام المدخلات الجديدة والتعديلات االتانية، يمكن تكون الصيانة الدورية هى السبب ،و بعديها  قاعدة البيانات ح ترجع للوضع الطبيعى.

الإدارى اللى قفل قاعدة البيانات هو اللى كتب التفسير دا:
$1',
'missing-article'      => 'قاعدة البيانات ما لقتش النص الخاص بتاع صفحة كان لازم تلاقيها و اسمها "$1" $2.

عادة دا بيحصل لما تدوس على لينكات قديمة، فرق التعديل أو التاريخ، اللى بتوصلك ل صفحة ملغية.

اذا ما كانش هو دا السبب ،ممكن عندك غلط فى البرامج .
لو سمحت تبلغ واحد من الاداريين و  تديله ال لينك بتاعة الصفحة .',
'missingarticle-rev'   => '(رقم المراجعة: $1)',
'missingarticle-diff'  => '(فرق: $1، $2)',
'readonly_lag'         => 'قاعدة البيانات  اتقفلت اوتوماتيكى علشان تقدر السيرفرات الفرعية تلحق السيرفر الرئيسي',
'internalerror'        => 'غلط داخلي',
'internalerror_info'   => 'غلط داخلي: $1',
'filecopyerror'        => 'ما قدرنا ش  ننسخ الملف "$1" لـ "$2".',
'filerenameerror'      => 'ما قدر نا ش نغير اسم الملف "$1" لـ "$2".',
'filedeleteerror'      => 'ما قدرنا ش نمسح الملف "$1".',
'directorycreateerror' => 'ما قدرناش نعمل المجلد "$1".',
'filenotfound'         => 'مش قادرين نلاقى الملف "$1".',
'fileexistserror'      => 'ما قدرناش نكتب فى الملف "$1": الملف موجود',
'unexpected'           => 'قيمة مش متوقعة: "$1"="$2".',
'formerror'            => 'غلط: مش ممكن تقديم الاستمارة',
'badarticleerror'      => 'مش ممكن ننفذ العملية دى على الصفحة دي',
'cannotdelete'         => 'ما قدرناش نمسح الصفحة أو الملف المطلوب. (ممكن يكون حد تانى مسحه. )',
'badtitle'             => 'عنوان غلط',
'badtitletext'         => 'عنوان الصفحه المطلوب اما مش صحيح او فاضي، و ربما الوصلة بين اللغات أو بين المشاريع غلط. وممكن وجود رموز ماتصلحش للاستخدام فى العناوين.',
'perfdisabled'         => 'متأسفين!  الخاصية دى اتعطلت بشكل مؤقت لأنها بتبطئ قاعدة البيانات لدرجة ان مافيش حد ممكن يستخدم الويكي.',
'perfcached'           => 'البيانات دى متخبية و ممكن ما تكونش متحدثة.',
'perfcachedts'         => 'البيانات دى متخبية، آخر تحديث ليها كان فى $1.',
'querypage-no-updates' => 'التحديثات بتاعةالصفحة دى متعطلة دلوقتي. البيانات اللى هنا مش ح تتحدث فى الوقت الحاضر.',
'wrong_wfQuery_params' => 'محددات غلط فى wfQuery()<br />
الدالة: $1<br />
الاستعلام: $2',
'viewsource'           => 'عرض المصدر',
'viewsourcefor'        => 'ل $1',
'actionthrottled'      => 'الامر دا  اتخنق',
'actionthrottledtext'  => 'علشان نمنع ال سبام ،أنت ممنوع تعمل  الفعل دا عدد كبير من المرات فى فترة زمنية قصيرة، و انت ا تجاوزت  الحد دا . لو سمحت تحاول مرة ثانية بعد دقائق.',
'protectedpagetext'    => 'الصفحة دى اتقفلت فى وش التعديل.',
'viewsourcetext'       => 'ممكن تشوف وتنسخ مصدر  الصفحه دى:',
'protectedinterface'   => 'الصفحة دى هى اللى بتوفر نص الواجهة بتاعة البرنامج،وهى مقفولة لمنع التخريب.',
'editinginterface'     => "'''تحذير''': أنت بتعدل صفحة بتستخدم فى الواجهة النصية  بتاعة البرنامج. التغييرات فى الصفحة دى ح تأثر على مظهر واجهة المستخدم للمستخدمين االتانيين. للترجمات، لو سمحت استخدم [http://translatewiki.net/wiki/Main_Page?setlang=ar بيتاويكي]، مشروع ترجمة الميدياويكي.",
'sqlhidden'            => '(استعلام إس‌كيو‌إل متخبي)',
'cascadeprotected'     => 'الصفحة دى محمية من التعديل، بسبب انها مدمجة فى {{PLURAL:$1|الصفحة|الصفحات}} دي، اللى مستعمل فيها خاصية "حماية الصفحات المدمجة" :
$2',
'namespaceprotected'   => "ما عندكش صلاحية تعديل الصفحات  اللى فى نطاق '''$1'''.",
'customcssjsprotected' => 'ماعندكش صلاحية تعديل  الصفحة دي، علشان فيها الإعدادات الشخصية بتاعة يوزر تاني.',
'ns-specialprotected'  => 'الصفحات المخصوصة مش ممكن تعديلها.',
'titleprotected'       => "العنوان دا محمى من الإنشاء بـ[[User:$1|$1]]. السبب هو ''$2''.",

# Virus scanner
'virus-badscanner'     => 'غلطه : ماسح فيروسات مش معروف: <i>$1</i>',
'virus-scanfailed'     => 'المسح فشل(كود $1)',
'virus-unknownscanner' => 'انتى فيروس مش معروف:',

# Login and logout pages
'logouttitle'                => 'خروج اليوزر',
'logouttext'                 => '<strong>أنت دلوقتى مش مسجل دخولك.</strong><br />
تقدر تكمل استعمال {{SITENAME}} على انك مجهول، أو الدخول مرة تانية بنفس الاسم أو باسم تاني. ممكن تشوف بعض الصفحات  كأنك متسجل ، و دا علشان استعمال الصفحات المتخبية فى المتصفح بتاعك.',
'welcomecreation'            => '== اهلاً و سهلاً يا $1! ==
اتفتحلك حساب.
ما تنساش تغير تفضيلاتك فى {{SITENAME}}.',
'loginpagetitle'             => 'دخول اليوزر',
'yourname'                   => 'اليوزرنيم:',
'yourpassword'               => 'الباسوورد:',
'yourpasswordagain'          => 'اكتب الباسورد تاني:',
'remembermypassword'         => 'افتكر بيانات دخولى على  الكمبيوتر ده',
'yourdomainname'             => 'النطاق بتاعك:',
'externaldberror'            => 'يا إما فى حاجة غلط فى الدخول على قاعدة البيانات الخارجية أو انت مش مسموح لك تعمل تحديث لحسابك الخارجي.',
'loginproblem'               => '<b>حصلت مشكلة وانت بتسجل دخولك.</b><br />لو سمحت تحاول مرة تانية!',
'login'                      => 'دخول',
'nav-login-createaccount'    => 'دخول / فتح حساب',
'loginprompt'                => 'لازم تكون الكوكيز عندك مفعله علشان تقدر تدخل ل {{SITENAME}}.',
'userlogin'                  => 'دخول / فتح حساب',
'logout'                     => 'خروج',
'userlogout'                 => 'خروج',
'notloggedin'                => 'انت مش مسجل دخولك',
'nologin'                    => 'معندكش حساب؟ $1.',
'nologinlink'                => 'افتح حساب',
'createaccount'              => 'افتح حساب',
'gotaccount'                 => 'عندك حساب؟ $1.',
'gotaccountlink'             => 'دخول',
'createaccountmail'          => 'بـ الايميل',
'badretype'                  => 'كلمتين السر اللى  كتبتهم مش  زى بعضهم',
'userexists'                 => 'اسم اليوزر اللى دخلته بيستعمله يوزر غيرك.
دخل اسم تانى.',
'youremail'                  => 'الايميل:',
'username'                   => 'اسم اليوزر:',
'uid'                        => 'رقم اليوزر:',
'prefs-memberingroups'       => 'عضو فى {{PLURAL:$1|مجموعة|مجموعة}}:',
'yourrealname'               => 'الاسم الحقيقى:',
'yourlanguage'               => 'اللغة:',
'yourvariant'                => 'اللهجة:',
'yournick'                   => 'الإمضا:',
'badsig'                     => 'الامضا الخام بتاعتك مش صح.
اتإكد من التاجز بتاعة الHTML.',
'badsiglength'               => 'الإمضا بتاعتك طويلة جدا.
لازم تكون اقل من $1 {{PLURAL:$1|حرف|حروف}}.',
'email'                      => 'الإيميل',
'prefs-help-realname'        => 'الاسم الحقيقى اختيارى.
ولو اخترت تعرض اسمك هنا هايستخدم فى الإشارة لمساهماتك.',
'loginerror'                 => 'غلط فى الدخول',
'prefs-help-email'           => 'عنوان البريد الإلكترونى اختيارى، لكن يسمح بأن يتم إرسال كلمة السر الخاصة بك إليك لو نسيتها.
ممكن كمان تخلى اليوزرز التانيين يقدروا يتصلوا بيك  فى صفحتك او صفة المناقشة بتاعتك من غير ما يعرفو  انت مين.',
'prefs-help-email-required'  => 'عنوان الإيميل مطلوب.',
'nocookiesnew'               => 'اليوزر خلاص اتفتح له حساب، بس انت لسة ما سجلتش دخولك.
بيستخدم {{SITENAME}} كوكيز عشان يسجل الدخول.
الكوكيز عندك متعطلة.
لو سمحت  تخليها تشتغل، بعدين أدخل ب اسم الحساب و الباسورد الجداد.',
'nocookieslogin'             => '{{SITENAME}} بيستخدم الكوكيز  علشان تسجيل الدخول.
الكوكيز عندك متعطلة.
لو سمحت تخليها تشتغل و بعدين حاول مرة تانية.',
'noname'                     => 'انت ما حددتش اسم يوزر صحيح.',
'loginsuccesstitle'          => 'تم الدخول بشكل صحيح',
'loginsuccess'               => "'''تم تسجيل دخولك{{SITENAME}} باسم \"\$1\".'''",
'nosuchuser'                 => 'مافيش يوزر باسم "$1".
اتاكد من تهجية الاسم، او [[Special:Userlogin/signup|افتح حساب جديد]].',
'nosuchusershort'            => 'مافيش يوزر باسم <nowiki>$1</nowiki>".
اتاكد من تهجية الاسم.',
'nouserspecified'            => 'لازم تحدد اسم يوزر.',
'wrongpassword'              => 'كلمة السر اللى كتبتها مش صحيحه. من فضلك حاول تانى.',
'wrongpasswordempty'         => 'كلمة السر المدخله كانت فاضيه.
من فضلك حاول تانى.',
'passwordtooshort'           => 'كلمة السر اللى اخترتها مش صحيحه أو قصيره قوي.
لازم مايقلش طول الكلمه عن {{PLURAL:$1|1 حرف|$1 حرف}} وتكون مختلفه عن اسم اليوزر بتاعك.',
'mailmypassword'             => 'ابعتلى كلمة سر جديدة',
'passwordremindertitle'      => 'كلمة سر مؤقته جديده ل {{SITENAME}}',
'passwordremindertext'       => 'فيه شخص ما (غالبا انت، من عنوان الاى بى $1)
طلب كلمة سر جديده لـ{{SITENAME}} ($4).
كلمة سر مؤقتة لليوزر "$2" تم إنشاؤها و ضبطها إلى "$3".
لو إن ده هوه اللى إنت عايزه، عليك انك تسجل الدخول و تختار كلمة سر جديدة دلوقتى.

لو إن حد تانى هوه اللى عمل الطلب ده، أو انك افتكرت كلمة السر بتاعتك، ومش عايز تغيرها، ممكن تتجاهل الرساله دى وتستمر فى استخدام كلمة السر القديمة بتاعتك.',
'noemail'                    => 'مافيش ايميل متسجل  لليوزر  "$1".',
'passwordsent'               => 'تم إرسال كلمة سر جديدة لعنوان الايميل المتسجل لليوزر "$1".
من فضلك حاول تسجيل الدخول مره تانيه بعد استلامها.',
'blocked-mailpassword'       => 'عنوان الايبى بتاعك ممنوع من التحرير، و كمان مش ممكن تسعمل خاصية ترجيع الباسورد علشان نمنع التخريب.',
'eauthentsent'               => 'فيه ايميل تأكيد اتبعت  للعنوان اللى كتبته.
علشان تبعت اى ايميل تانى للحساب ده لازم تتبع التعليمات اللى فى الايميل اللى اتبعتلك  علشان تأكد ان  الحساب ده بتاعك .',
'throttled-mailpassword'     => 'بعتنالك علشان تفتكر الباسورد بتاعتك، فى خلال الـ{{PLURAL:$1|ساعة|$1 ساعة}} اللى فاتت.
علشان منع التخريب، ح نفكرك مرة و احدة بس كل
{{PLURAL:$1|ساعة|$1 ساعة}}.',
'mailerror'                  => 'غلط فى بعتان الايميل : $1',
'acct_creation_throttle_hit' => 'متأسفين، انت عندك $1 حساب. مش ممكن نفتح واحد تاني.',
'emailauthenticated'         => 'اتأكدنا من الايميل بتاعك  فى $1.',
'emailnotauthenticated'      => 'لسة ما اتكدناش من الايميل بتاعك.
مش ح يتبعتلك اى  ايميلات بخصوص الميزات دي.',
'noemailprefs'               => 'علشان الخصايص دى تشتغل لازم تحددلك عنوان ايميل.',
'emailconfirmlink'           => 'أكد عنوان الإيميل بتاعك',
'invalidemailaddress'        => 'مش ممكن نقبل عنوان الايميل لانه مش مظبوط.
دخل ايميل مظبوط او امسحه من الخانة.',
'accountcreated'             => 'الحساب اتفتح',
'accountcreatedtext'         => 'اتفتح حساب لليوزر ب$1.',
'createaccount-title'        => 'فتح حساب فى {{SITENAME}}',
'createaccount-text'         => 'فى واحد فتح حساب باسم الايمل بتاعك على {{SITENAME}} ($4) بالاسم "$2"، وبباسورد "$3". لازم تسجل دخولك دلوقتى و تغير الباسورد بتاعتك.

لو سمحت تتجاهل الرسالة دى اذا الحساب دا اتفتحلك بالغلط.',
'loginlanguagelabel'         => 'اللغة: $1',

# Password reset dialog
'resetpass'               => 'غير الباسورد بتاعة الحساب',
'resetpass_announce'      => 'اتسجل دخولك دلوقتى بالكود اللى اتبعتلك فى الايميل. علشان تخلص عملية الدخول ،لازم تعملك باسورد جديدة هنا:',
'resetpass_text'          => '<!-- أضف نصا هنا -->',
'resetpass_header'        => 'غير الباسورد',
'resetpass_submit'        => 'اظبط الباسورد و ادخل',
'resetpass_success'       => 'الباسورد بتاعتك اتغيرت بنجاح! دلوقتى  بنسجل دخولك...',
'resetpass_bad_temporary' => 'الباسورد المؤقتة دى غلط. يمكن الباسورد الاصلية تكون اتغيرت بنحاح أو يمكن انت كنت طلبت باسورد مؤقتة جديدة.',
'resetpass_forbidden'     => 'مش ممكن تغيير الباسورد',
'resetpass_missing'       => 'مافيش اى بيانات.',

# Edit page toolbar
'bold_sample'     => 'حروف عريضه',
'bold_tip'        => 'حروف عريضه',
'italic_sample'   => 'كلام مايل',
'italic_tip'      => 'كلام مايل',
'link_sample'     => 'عنوان وصله',
'link_tip'        => 'وصله داخليه',
'extlink_sample'  => 'http://www.example.com عنوان الوصله',
'extlink_tip'     => 'وصله خارجيه (افتكر تحط http:// قبل عنوان الوصله)',
'headline_sample' => 'راس الموضوع',
'headline_tip'    => 'عنوان فرعى من المستوى التانى',
'math_sample'     => 'اكتب المعادله هنا',
'math_tip'        => 'معادله رياضيه (لا تكس )',
'nowiki_sample'   => 'حط  الكلام اللى مش متنسق هنا',
'nowiki_tip'      => 'ما تستعملش فورمات الويكى',
'image_tip'       => 'ملف مغروس',
'media_tip'       => 'وصلة ملف',
'sig_tip'         => 'امضتك مع الساعه والتاريخ',
'hr_tip'          => 'خط افقى (ما تستعملهموش كتير)',

# Edit pages
'summary'                          => 'ملخص',
'subject'                          => 'راس الموضوع/موضوع',
'minoredit'                        => 'التعديل ده تعديل صغير',
'watchthis'                        => 'راقب الصفحه دى',
'savearticle'                      => 'سييف الصفحه',
'preview'                          => 'بروفه',
'showpreview'                      => 'عرض البروفه',
'showlivepreview'                  => 'بروفه حيه',
'showdiff'                         => 'بيين التعديلات',
'anoneditwarning'                  => "'''تحذير:''' انت ما عملتش لوجين.
عنوان الاى  بى  بتاعك هايتسجل فى تاريخ الصفحه .",
'missingsummary'                   => "'''خد بالك:''' انت ما كتبتش ملخص للتعديل.
لو دوست على حفظ الصفحة مرة تانية التعديل بتاعك ح يتحفظ من غير ملخص.",
'missingcommenttext'               => 'لو سمحت اكتب تعليق تحت.',
'missingcommentheader'             => "'''خد بالك:''' انت ما كتبتش عنوان\\موضوع للتعليق داز
لو دوست على حفظ الصفحة مرة تانية، تعليقك ح يتحفظ من غير عنوان.",
'summary-preview'                  => 'بروفه للملخص',
'subject-preview'                  => 'بروفة للعنوان\\الموضوع',
'blockedtitle'                     => 'اليوزر ممنوع',
'blockedtext'                      => "<big>'''تم منع اسم اليوزر أو عنوان الااى بى بتاعك .'''</big>

سبب المنع هو: ''$2''. وقام بالمنع $1.

* بداية المنع: $8
* انتهاء المنع: $6
* الممنوع المقصود: $7

ممكن التواصل مع $1 لمناقشة المنع، أو مع واحد من [[{{MediaWiki:Grouppage-sysop}}|الاداريين]] عن المنع>
افتكر انه مش ممكن تبعت ايميل  لليوزرز الا اذا كنت سجلت عنوان ايميل صحيح فى صفحة [[Special:Preferences|التفضيلات]] بتاعتك.
عنوان الااى بى بتاعك حاليا هو $3 وكود المنع هو #$5.من فضلك ضيف اى واحد منهم أو كلاهما فى اى رسالة للتساؤل عن المنع.",
'autoblockedtext'                  => 'عنوان الأيبى بتاعك اتمنع اتوماتيكى  علشان فى يوزر تانى استخدمه واللى هو كمان ممنوع بــ $1.
السبب هو:

:\'\'$2\'\'

* بداية المنع: $8
* انهاية المنع: $6
* الممنوع المقصود: $7

ممكن تتصل  ب $1 أو واحد من 
[[{{MediaWiki:Grouppage-sysop}}|الإداريين]] االتانيين لمناقشة المنع.

لاحظ أنه مش ممكن استخدام خاصية "ابعت رسالة لليوزر دا" إلا اذا كان عندك ايميل صحيح متسجل فى [[Special:Preferences|تفضيلاتك]].

عنوان الأيبى الحالى الخاص بك هو $3، رقم المنع هو $5. لو سمحت تذكر الرقم دا فى اى استفسار.',
'blockednoreason'                  => 'ما فيش سبب',
'blockedoriginalsource'            => "المصدر بتاع '''$1''' معروض تحت:",
'blockededitsource'                => "نص '''تعديلاتك''' فى '''$1''' معروض هنا:",
'whitelistedittitle'               => 'لازم تسجل دخولك علشان تقدر تعدل',
'whitelistedittext'                => 'لازم $1 علشان تقدر تعدل الصفحات.',
'confirmedittitle'                 => 'علشان تبتدى تعدل، لازم نتاكد من الايميل بتاعك',
'confirmedittext'                  => 'قبل ما تبتدى تعدل لازم نتأكد من الايميل بتاعك. لو سمحت تكتب وتأكد الايميل بتاعك  في[[Special:Preferences|تفضيلاتك]]',
'nosuchsectiontitle'               => 'مافيش قسم بالاسم ده',
'nosuchsectiontext'                => 'انت حاولت تعمل تعديل على قسم مش موجود.
و علشان القسم $1 مش موجود اصلاً، فمش ممكن نحفظ التعديلات بتاعتك.',
'loginreqtitle'                    => 'لازم تسجل دخولك',
'loginreqlink'                     => 'ادخل',
'loginreqpagetext'                 => 'لازم تكون $1 علشان تشوف صفحات تانية.',
'accmailtitle'                     => ' كلمة السر اتبعتت .',
'accmailtext'                      => "الباسورد بتاعة '$1' اتبعتت لـ $2.",
'newarticle'                       => '(جديد)',
'newarticletext'                   => "انت وصلت لصفحه مابتدتش لسه.
علشان  تبتدى الصفحة ابتدى الكتابه فى الصندوق اللى تحت.
(بص على [[{{MediaWiki:Helppage}}|صفحة المساعده]] علشان معلومات اكتر)
لو كانت زيارتك للصفحه دى بالخطأ، اضغط على زر ''رجوع'' فى متصفح الإنترنت عندك.",
'anontalkpagetext'                 => "----'' صفحة النقاش دى بتاعة يوزر مجهول لسة ما فتحش لنفسه حساب أو عنده واحد بس ما بيستعملوش. علشان كدا لازم تستعمل رقم الأيبى علشان تتعرف عليه/عليها. العنوان دا ممكن اكتر من واحد يكونو بيستعملوه. لو انت يوزر مجهول و حاسس  ان فى تعليقات بتتوجهلك مع انك مالكش دعوة بيها ،من فضلك [[Special:UserLogin|افتحلك حساب أو سجل الدخول]] علشان تتجنب اللخبطة اللى ممكن تحصل فى المستقبل مع يوزرز مجهولين تانيين.''",
'noarticletext'                    => 'مافيش  دلوقتى اى نص فى  الصفحه دى ، ممكن [[Special:Search/{{PAGENAME}}|تدور على عنوان الصفحه]] فى الصفحات التانيه او [{{fullurl:{{FULLPAGENAME}}|action=edit}} تعدل الصفحه دى].',
'userpage-userdoesnotexist'        => 'حساب اليوزر "$1" مش متسجل. لو سمحت تشوف لو عايز تبتدي/تعدل الصفحة دي.',
'clearyourcache'                   => "'''ملاحظة - بعد التسييف,  يمكن لازم تفرغ كاش متصفحك لرؤية التغييرات.''' '''موزيللا / فايرفوكس / سافارى:''' دوس على ''Shift'' أثناء ضغط ''Reload,'' أو دوس على أيا من ''Ctrl-F5'' أو ''Ctrl-R'' (''Command-R'' على ماكنتوش); '''كونكرر: '''دوس على ''Reload'' أو دوس على ''F5;'' '''أوبرا:''' فرغ الكاش فى ''Tools → Preferences;'' '''إنترنت إكسبلورر:''' دوس على ''Ctrl'' أثناء ضغط ''Refresh,'' أو دوس على ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => "<strong>ملاحظة:</strong> استعمل زرار ' عرض بروفة' علشان تجرب النمط (CSS) أو الجافا سكريبت الجديد قبل حفظ الصفحة.",
'usercsspreview'                   => "'''افتكرأنك  بتعرض  (CSS) بتاع اليوزر بس.
هى لسة ماتحفظتش!'''",
'userjspreview'                    => "'''أفتكر أنك بس بتجرب/بتعرض الجافا سكريبت بتاع اليوزر بتاعك، و انها لسة ماتحفظتش!'''",
'userinvalidcssjstitle'            => "'''تحذير:'''مافيش واجهة  \"\$1\".
افتكر أن ملفات ال.css و ال.js بتستخدم حروف صغيرة فى العنوان ، مثلا {{ns:user}}:Foo/monobook.css و مش {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(متحدثة)',
'note'                             => '<strong>ملحوظه:</strong>',
'previewnote'                      => '<strong> دى بروفه للصفحه بس،
ولسه ما تسييفتش!</strong>',
'previewconflict'                  => 'البروفة دى بتبينلك فوق إزاى ح يكون شكل النص لو انت دوست على حفظ',
'session_fail_preview'             => '<strong>ما قدرناش  نحفظ التعديلات اللى قمت بيها نتيجة لضياع بيانات  الجلسه.
الرجاء المحاولة مرة تانيه.
فى حال استمرار المشكلة حاول  [[Special:UserLogou|تخرج]] وتدخل مرة تانيه .</strong>',
'session_fail_preview_html'        => "<strong>ماقدرناش نعالج تعديلك بسبب ضياع بيانات الجلسة.</strong>

''لأن {{SITENAME}} بها HTML هل الخام شغاله، البروفه مخفيه كاحتياط ضد هجمات الجافا سكريبت.''

<strong>إذا كانت دى محاولة تعديل صادقه، من فضلك حاول مرة تانيه. إذا كانت لسه مش شغاله، حاول [[Special:UserLogout|تسجيل الخروج]] و تسجيل الدخول من جديد.</strong>",
'token_suffix_mismatch'            => '<strong>تعديلك اترفض لأن عميلك غلط فى علامات الترقيم 
فى نص التعديل. التعديل اترفض علشان ما يبوظش نص المقالة.
دا ساعات بيحصل لما تستعمل خدمة بروكسى مجهولة بايظة أساسها الويب.</strong>',
'editing'                          => 'تعديل $1',
'editingsection'                   => 'تعديل $1 (جزء)',
'editingcomment'                   => 'تعديل $1 (تعليق)',
'editconflict'                     => 'تضارب فى التحرير: $1',
'explainconflict'                  => "فى واحد تانى عدل الصفحة دى  بعد ما انت ابتديت بتحريرها.
صندوق النصوص الفوقانى فيه النص الموجود دلوقتى فى الصفحة.
والتغييرات انت عملتها موجودة فى الصندوق التحتانى فى الصفحة.
لازم تدمج تغييراتك فى النص الموجود دلوقتي.
'''بس''' اللى موجود فى الصندوق الفوقانى هو اللى ح يتحفظ لما تدوس على زرار \"حفظ الصفحة\".",
'yourtext'                         => 'النص بتاعك',
'storedversion'                    => 'النسخة المخزنة',
'nonunicodebrowser'                => '<strong>تحذير: البراوزر بتاعك مش متوافق مع اليونيكود.
اتعالج الموضوع دا علشان تقدر تعدل الصفحة بامان: الحروف اللى مش ASCII ح تظهر فى صندوق التحرير كأكواد سداسية عشرية.</strong>',
'editingold'                       => '<strong>   تحذير: أنت تقدلوقتى بتحرر نسخة قديمة من  الصفحة دي.
 ولو حفظتها، ح تضيع كل التغييرات اللى حصلت بعد  النسخة دي. </strong>',
'yourdiff'                         => 'الفروق',
'copyrightwarning'                 => 'من فضلك لاحظ ان كل المساهمات فى {{SITENAME}} بتتنشر حسب شروط ترخيص $2 (بص على $1 علشان تعرف  تفاصيل اكتر)
لو مش عايز كتابتك تتعدل او تتوزع من غير مقابل و بدون اذنك ، ما تحطهاش هنا<br />. كمان انت  بتتعهد بانك كتبت كلام تعديلك بنفسك، او نسخته من مصدر يعتبر ضمن الملكيه العامه، أو مصدر حر تان.

<strong>ما تحطش اى عمل له حقوق محفوظه بدون اذن صاحب الحق</strong>.',
'copyrightwarning2'                => 'لو سمحت تعمل حسابك ان كل مشاركاتك فى {{SITENAME}} ممكن المشاركين التانيين يعدلوها،يغيروها، او يمسحوها خالص. لو مانتش حابب ان كتاباتك تتعدل و تتغير بالشكل دا، فياريت ما تنشرهاش هنا.<br />.
و كمان انت بتدينا كلمة شرف  انك صاحب الكتابات دي، او انك نقلتها من مكان مش خاضع لحقوق النشر .(شوف التفاصيل فى $1 ).
<strong>لو سمحت ما تحطش هنا اى نص خاضع لحقوق النشر من غير تصريح!</strong>.',
'longpagewarning'                  => '<strong>تحذير: الصفحه دى حجمها $1 كيلوبايت،
بعض المتصفحات (براوزرز) ممكن تواجه مشاكل لما تحاول تعديل صفحات يزيد حجمها عن 32 كيلوبايت.
من فضلك ,لو امكن قسم الصفحة لصفحات اصغر فى الحجم.</strong>',
'longpageerror'                    => '<strong>غلط: النص اللى دخلته حجمه $1 كيلوبايت، ودا أكبر من الحد الأقصى و اللى هو $2 كيلوبايت.
مش ممكن يتحفظ.</strong>',
'readonlywarning'                  => '<strong>تحذير: قاعدة البيانات اتقفلت للصيانة، و علشان كدا انت مش ح تقدر تحفظ التعديلات اللى عملتها دلوقاي.
لو حبيت ممكن  تنسخ النص وتحفظه فى ملف نصى علشان تستعمله بعدين.</strong>',
'protectedpagewarning'             => '<strong>تحذير:الصفحة دا اتقفلت بطريقة تخلى اليوزرز السيسوبات هم بس اللى يقدرو يعدلوها.</strong>',
'semiprotectedpagewarning'         => "'''ملاحظة:''' الصفحة  دى اتقفلت بطريقة تخلى اليوزرز المتسجلين بس هم اللى قدرو يعدلوها.",
'cascadeprotectedwarning'          => '<strong>تحذير: الصفحة دى اتقفلت بطريقة تخلى اليوزرز السيوبات بس هم اللى يقدرو يعدلوها، ودا علشان هى مدموجة فى {{PLURAL:$1|الصفحة|الصفحات}} التالية واللى اتعملها حمتية بخاصية "حماية الصفحات المدموجة":</strong>',
'titleprotectedwarning'            => '<strong>تحذير: الصفحة اتقفلت علشان مش كل اليوزرز يقدرو ينشأوها .</strong>',
'templatesused'                    => 'القوالب المستعمله فى الصفحه دى:',
'templatesusedpreview'             => 'القوالب المستعمله فى البروفه دى:',
'templatesusedsection'             => 'القوالب اللى بتستخدم فى القسم دا:',
'template-protected'               => '(حمايه كامله)',
'template-semiprotected'           => '(حمايه جزئيه )',
'hiddencategories'                 => 'الصفحه دى موجوده فى {{PLURAL:$1|تصنيف مخفى واحد|$1 تصنيف مخفى}}:',
'edittools'                        => '<!-- النص هنا سيظهر تحت صندوق التحرير و استمارة رفع الصور. -->',
'nocreatetitle'                    => 'إنشاء الصفحات اتحدد',
'nocreatetext'                     => '{{SITENAME}} حدد القدره على انشاء صفحات جديده.
ممكن ترجع وتحرر صفحه موجوده بالفعل، او [[Special:UserLogin|الدخول / فتح حساب]].',
'nocreate-loggedin'                => 'انت ما عندك ش صلاحية تعمل صفحات جديدة.',
'permissionserrors'                => 'غلطات فى السماح',
'permissionserrorstext'            => 'ما عندك ش صلاحية تعمل كدا،{{PLURAL:$1|علشان|علشان}}:',
'permissionserrorstext-withaction' => 'أنت ما عندكش الصلاحيات ل$2، لل{{PLURAL:$1|سبب|أسباب}} دي:',
'recreate-deleted-warn'            => "'''تحذير: انت بتعيد انشاء صفحه اتمسحت قبل كده.'''
لازم تتأكد من ان الاستمرار فى تحرير الصفحه دى ملائم.
سجل الحذف للصفحه دى معروض هنا:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'تحذير: الصفحه دى فيهااستدعاءات دالة محلل كثيرة مكلفة.

لازم تكون أقل من $2، فيها دلوقتى $1.',
'expensive-parserfunction-category'       => 'صفحات فيها استدعاءات دوال محلل كثيرة ومكلفة',
'post-expand-template-inclusion-warning'  => 'تحذير: حجم تضمين القالب كبير قوي.
بعض القوالب مش ح تتضمن.',
'post-expand-template-inclusion-category' => 'الصفحات اللى تم تجاوز حجم تضمين القالب فيها',
'post-expand-template-argument-warning'   => 'تحذير: الصفحة  دى فيها عامل قالب واحد على الأقل ليه حجم تمدد كبير قوي.
العوامل دى اتمسحت.',
'post-expand-template-argument-category'  => 'صفحات فيها مناقشات القالب المحذوفة',

# "Undo" feature
'undo-success' => 'ممكن ترجع فى التعديل.
لو سمحت تشوف المقارنة اللى تحت علشان تتأكد من إن هو دا اللى إنت عايز تعمله ،وبعدين احفظ التغييرات اللى تحت علشان ترجع فى التعديل.',
'undo-failure' => 'الرجوع فى التعديل ما نفعش علشان فى تعديلات متعاكسة حصلت فى الصفحة.',
'undo-norev'   => 'الرجوع فى التعديل ما نفعش علشان هو يا إما مش موجود أو انه إتمسح.',
'undo-summary' => 'الرجوع فى التعديل $1 بتاع [[Special:Contributions/$2|$2]] ([[User talk:$2|نقاش]])',

# Account creation failure
'cantcreateaccounttitle' => 'مش ممكن فتح حساب',
'cantcreateaccount-text' => "فتح الحسابات من عنوان الأيبى دا ('''$1''') منعه [[User:$3|$3]].

السبب إللى إداه $3 هو ''$2''",

# History pages
'viewpagelogs'        => 'عرض السجلات للصفحه دى',
'nohistory'           => 'الصفحة دى ما لهاش تاريخ تعديل.',
'revnotfound'         => 'النسخة مش موجودة',
'revnotfoundtext'     => 'ما لقيناش النسخة القديمة من الصفحة اللى طلبتها. لو سمحت تتأكد من اليوأرإل اللى دخلت بيه للصفحة دي.',
'currentrev'          => 'النسخه دلوقتى',
'revisionasof'        => 'تعديلات من $1',
'revision-info'       => 'نسخه $1 بواسطة $2',
'previousrevision'    => '←نسخه اقدم',
'nextrevision'        => 'نسخه احدث→',
'currentrevisionlink' => 'النسخه دلوقتى',
'cur'                 => 'دلوقتى',
'next'                => 'اللى بعد كده',
'last'                => 'قبل كده',
'page_first'          => 'الأولانية',
'page_last'           => 'الأخرانية',
'histlegend'          => 'اختيار الفرق: علم على صناديق النسخ للمقارنه و اضغط قارن بين النسخ المختاره او الزرار اللى تحت.<br />
مفتاح: (دلوقتى) = الفرق مع النسخة دلوقتى
(اللى قبل كده) = الفرق مع النسخة اللى قبل كده، ص = تعديل صغير',
'deletedrev'          => '[ممسوحة]',
'histfirst'           => 'اول',
'histlast'            => 'آخر',
'historysize'         => '({{PLURAL:$1|1 بايت|$1 بايت}})',
'historyempty'        => '(فاضى)',

# Revision feed
'history-feed-title'          => 'تاريخ المراجعة',
'history-feed-description'    => 'تاريخ التعديل بتاع الصفحة دى على الويكي',
'history-feed-item-nocomment' => '$1 فى $2', # user at time
'history-feed-empty'          => 'الصفحة المطلوبة مش موجودة.
من المحتمل تكون الصفحة أتمسحت أو أتنقلت.
حاول [[Special:Search|التدوير فى الويكى]] عن صفحات جديدة ليها صلة.',

# Revision deletion
'rev-deleted-comment'         => '(التعليق اتشال)',
'rev-deleted-user'            => '(اسم اليوزر اتشال)',
'rev-deleted-event'           => '(السجل إتشال)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
مراجعة الصفحة دى إتشالت من الأرشيفات العامة.
ممكن تكون فى تفاصيل فى [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} سجل المسح].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
التعديل دا اتمسح من الأرشيف العام.
ممكن تشوف التعديل دا علشان إنت إدارى فى {{SITENAME}} .
ممكن يكون فى تفاصيل بخصوص دا فى [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} سجل المسح].
</div>',
'rev-delundel'                => 'عرض/إخفاء',
'revisiondelete'              => 'امسح/الغى المسح بتاع المراجعات',
'revdelete-nooldid-title'     => 'مراجعة هدف مش صح',
'revdelete-nooldid-text'      => 'أنت ياإما ما حددتش مراجعة (مراجعات) معينة كهدف للفعل دا، المراجعة المحددة مش موجودة، أو أنك بتحاول تخبى المراجعة الحالية.',
'revdelete-selected'          => '{{PLURAL:$2|المراجعة المختارة|المراجعات المختارة}} ل[[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|حدث السجل المختار|أحداث السجل المختارة}}:',
'revdelete-text'              => 'المراجعات والأحداث الممسوحة هاتفضل تظهر فى تاريخ الصفحة والسجلات،
لكن أجزاء من محتواهم مش هايكون مسموح للعامة يشوفوها.

الإداريون التانين فى {{SITENAME}}هايفضل بإمكانهم رؤية المحتوى المخفى وممكن يسترجعوه من جديد من خلال الواجهة نفسها، الا اذا تم وضع قيود إضافية.',
'revdelete-legend'            => 'وضع حدود رؤية',
'revdelete-hide-text'         => 'إخفاء نص النسخة',
'revdelete-hide-name'         => 'تخبية الإجراء والهدف منه',
'revdelete-hide-comment'      => 'خبى تعليق التعديل',
'revdelete-hide-user'         => 'خبى اسم/عنوان الاى بى بتاع اليوزر',
'revdelete-hide-restricted'   => 'طبق الضوابط  دى على السيسوبات واقفل الواجهة دي',
'revdelete-suppress'          => 'تخبية البيانات عن السيسوبات و اليوزرز التانيين',
'revdelete-hide-image'        => 'خبى المحتويات بتاعة الملف',
'revdelete-unsuppress'        => 'إزالة الضوابط من المراجعات المسترجعة',
'revdelete-log'               => 'تعليق  على السجل:',
'revdelete-submit'            => 'طبق على النسخه المختاره',
'revdelete-logentry'          => 'غير رؤية المراجعة ل[[$1]]',
'logdelete-logentry'          => 'غير رؤية الحدث ل[[$1]]',
'revdelete-success'           => "''' رؤية المراجعات اتظبطت بنجاح.'''",
'logdelete-success'           => "'''رؤية السجلات اتظبطت بنجاح.'''",
'revdel-restore'              => 'تغيير الرؤية',
'pagehist'                    => 'تاريخ الصفحة',
'deletedhist'                 => 'التاريخ الممسوح',
'revdelete-content'           => 'محتويات',
'revdelete-summary'           => 'ملخص التعديل',
'revdelete-uname'             => 'اسم اليوزر',
'revdelete-restricted'        => 'طبق التعليمات على السيسوبات',
'revdelete-unrestricted'      => 'شيل الضوابط من على السيسوبات',
'revdelete-hid'               => 'أخفى $1',
'revdelete-unhid'             => 'أظهر $1',
'revdelete-log-message'       => '$1 ل$2 {{PLURAL:$2|مراجعة|مراجعة}}',
'logdelete-log-message'       => '$1 ل$2 {{PLURAL:$2|حدث|حدث}}',

# Suppression log
'suppressionlog'     => 'سجل الإخفاء',
'suppressionlogtext' => 'تحت فى لستة بعمليات المسح والمنع اللى فيها محتوى مستخبى على الإداريين.
شوف [[Special:IPBlockList|للستة المنع]] علشان تشوف عمليات المنع الشغالة دلوقتى .',

# History merging
'mergehistory'                     => 'دمج تواريخ الصفحة',
'mergehistory-header'              => ' الصفحةدى  بتسمح لك بدمج نسخ تاريخ صفحة  فى صفحة تانية.
اتأكد من أن التغيير دا ح يحافظ على استمرارية تاريخ الصفحة.',
'mergehistory-box'                 => 'دمج تعديلات صفحتين:',
'mergehistory-from'                => 'الصفحه المصدر:',
'mergehistory-into'                => 'الصفحه الهدف:',
'mergehistory-list'                => 'تاريخ التعديل اللى ممكن يتدمج',
'mergehistory-merge'               => 'المراجعات دى من [[:$1|$1]] ممكن دمجها مع[[:$2|$2]].
استخدم عامود الصناديق لدمج المراجعات التى اتنشأت فى وقبل الوقت المحدد.
خد بالك من إن استخدام وصلات التصفح ح يعيد ضبط  العامود دا.',
'mergehistory-go'                  => 'عرض التعديلات اللى ممكن تتدمج',
'mergehistory-submit'              => 'دمج النسخ',
'mergehistory-empty'               => 'مافيش مراجعات ممكن دمجها.',
'mergehistory-success'             => '$3 {{PLURAL:$3|مراجعة|مراجعة}} من [[:$1]] تم دمجها بنجاح فى [[:$2]].',
'mergehistory-fail'                => 'مش قادر يعمل دمج للتاريخ، لو سمحت تتأكد تانى من محددات الصفحة والزمن.',
'mergehistory-no-source'           => 'الصفحة المصدر $1  مش موجودة.',
'mergehistory-no-destination'      => 'الصفحه الهدف $1 مش موجوده.',
'mergehistory-invalid-source'      => 'الصفحه المصدر لازم تكون عنوان صحيح.',
'mergehistory-invalid-destination' => 'الصفحة الهدف لازم تكون عنوانها صحيح.',
'mergehistory-autocomment'         => 'دمج [[:$1]] فى [[:$2]]',
'mergehistory-comment'             => 'دمج [[:$1]] فى [[:$2]]: $3',

# Merge log
'mergelog'           => 'سجل الدمج',
'pagemerge-logentry' => 'دمج [[$1]] لـ [[$2]] (النسخ حتى $3)',
'revertmerge'        => 'استرجاع الدمج',
'mergelogpagetext'   => 'فى تحت لستة بأحدث عمليات الدمج لتاريخ صفحة فى التانية.',

# Diffs
'history-title'           => 'تاريخ تعديل "$1"',
'difference'              => '(الفرق بين النسخ)',
'lineno'                  => 'سطر $1:',
'compareselectedversions' => 'قارن بين النسختين المختارتين',
'editundo'                => 'استرجاع',
'diff-multi'              => '({{PLURAL:$1|نسخه واحده متوسطه|$1 نسخه متوسطه}} مش معروضه.)',

# Search results
'searchresults'             => 'نتايج التدوير',
'searchresulttext'          => 'لو عايز تعرف اكتر عن التدوير فى {{SITENAME}}، شوف [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => "انت كنت بتدور على '''[[:$1]]'''",
'searchsubtitleinvalid'     => "انت دورت على '''$1'''",
'noexactmatch'              => "'''مافيش  صفحه بالاسم \"\$1\"'''.
ممكن [[:\$1| تبتدى الصفحه دى]].",
'noexactmatch-nocreate'     => "'''مافيش صفحة بالاسم \"\$1\".'''",
'toomanymatches'            => 'لقينا حاجات كتيرة متطابقة، لو سمحت تجرب استعلام مختلف',
'titlematches'              => 'عنوان الصفحة زى',
'notitlematches'            => 'مالقيناش ولا عنوان صفحة مطابق:',
'textmatches'               => 'نص الصفحة بيطابق',
'notextmatches'             => 'ما لقيناش أى نص مطابق',
'prevn'                     => '$1 اللى قبل كده',
'nextn'                     => '$1 اللى بعد كده',
'viewprevnext'              => 'بص ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 كلمة|$2 كلمة}})',
'search-result-score'       => 'الارتباط: $1%',
'search-redirect'           => '(تحويله $1)',
'search-section'            => '(قسم $1)',
'search-suggest'            => 'قصدك: $1',
'search-interwiki-caption'  => 'المشاريع الشقيقة',
'search-interwiki-default'  => '$1 نتيجة:',
'search-interwiki-more'     => '(اأكتر)',
'search-mwsuggest-enabled'  => 'مع اقتراحات',
'search-mwsuggest-disabled' => 'مافيش اقتراحات',
'search-relatedarticle'     => 'مرتبطه',
'mwsuggest-disable'         => 'تعطيل اقتراحات أجاكس',
'searchrelated'             => 'مرتبطه',
'searchall'                 => 'الكل',
'showingresults'            => "القائمة دى بتعرض {{PLURAL:$1|'''1''' نتيجة|'''$1''' نتيجة}} من أول  رقم '''$2'''.",
'showingresultsnum'         => "معروض تحت {{PLURAL:$3|'''نتيجة واحدة'''|'''$3''' نتيجة}} من أول من رقم'''$2'''.",
'showingresultstotal'       => "معروض تحت {{PLURAL:$3|النتيجة '''$1''' من '''$3'''|النتايج '''$1 - $2''' من '''$3'''}}",
'nonefound'                 => "'''ملاحظة''':  بعض النطاقات بيتدور فيها اوتوماتيكي.
حاول تبتدى تدويرك ب ''all:'' علشان تدور فى المحتوى كله (مع صفحات النقاش، القوالب، إلخ)، أو استخدم النطاق المطلوب كبريفيكس.",
'powersearch'               => 'تدوير متفصل',
'powersearch-legend'        => 'تدوير متقدم',
'powersearch-ns'            => 'تدوير فى النطاقات:',
'powersearch-redir'         => 'لستة التحويلات',
'powersearch-field'         => 'تدوير على',
'search-external'           => 'تدوير بره',
'searchdisabled'            => 'التدوير فى {{SITENAME}} متعطل.
ممكن تدور فى جوجل دلوقتي.
لاحظ أن فهارسه لمحتوى {{SITENAME}} يمكن تكون مش متحدثة.',

# Preferences page
'preferences'              => 'تفضيلات',
'mypreferences'            => 'تفضيلاتى',
'prefs-edits'              => 'عدد التعديلات:',
'prefsnologin'             => 'مش متسجل',
'prefsnologintext'         => 'لازم تكون <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} مسجل الدخول]</span> علشان تقدر تعدل تفضيلاتك.',
'prefsreset'               => 'التفضيلات اترجعت تانى زى الأول',
'qbsettings'               => 'البار السريع',
'qbsettings-none'          => 'ما فى ش',
'qbsettings-fixedleft'     => 'متثبت فى الشمال',
'qbsettings-fixedright'    => 'متثبت فى اليمين',
'qbsettings-floatingleft'  => 'عايم على الشمال',
'qbsettings-floatingright' => 'عايم على اليمين',
'changepassword'           => 'غير الباسورد',
'skin'                     => 'الوش',
'math'                     => 'رياضة',
'dateformat'               => 'طريقة كتابة التاريخ',
'datedefault'              => 'مافبش تفضيل',
'datetime'                 => 'وقت وتاريخ',
'math_failure'             => 'الاعراب فشل',
'math_unknown_error'       => 'غلط مش معروف',
'math_unknown_function'    => 'وظيفة مش معروفة',
'math_lexing_error'        => 'غلط فى الكلمة',
'math_syntax_error'        => 'غلط فى تركيب الجملة',
'math_image_error'         => 'فشل التحويل لـ PNG ؛ 
اتاكد من التثبيت المضبوط لـ :Latex و dvips و gs و convert.',
'math_bad_tmpdir'          => 'مش ممكن الكتابة أو انشاء مجلد الرياضة الموؤقت',
'math_bad_output'          => 'مش ممكن الكتابة لـ أو إنشاء مجلد الخرج للرياضيات',
'math_notexvc'             => 'ضايعtexvc executable ؛ لو سمحت شوفmath/README للضبط.',
'prefs-personal'           => 'البروفيل بتاع اليوزر',
'prefs-rc'                 => 'اخر التغييرات',
'prefs-watchlist'          => 'لستة المراقبة',
'prefs-watchlist-days'     => 'عدد الأيام للعرض فى لستة المراقبة:',
'prefs-watchlist-edits'    => 'عدد التعديلات اللى بتتعرض فى لستةالمراقبة المتوسعة:',
'prefs-misc'               => 'متفرقات',
'saveprefs'                => 'حفظ',
'resetprefs'               => 'امسح التغييرات اللى مش المحفوظة',
'oldpassword'              => 'الباسورد القديمة:',
'newpassword'              => 'الباسورد جديدة:',
'retypenew'                => 'اكتب الباسورد الجديده تانى:',
'textboxsize'              => 'تعديل',
'rows'                     => 'صفوف:',
'columns'                  => 'عمدان:',
'searchresultshead'        => 'تدوير',
'resultsperpage'           => 'عدد النتايج فى الصفحة:',
'contextlines'             => 'عدد  السطور فى كل نتيجة:',
'contextchars'             => 'عدد  الحروف فى كل سطر',
'stub-threshold'           => 'الحد لتنسيق <a href="#" class="stub">لينك البذرة</a>:',
'recentchangesdays'        => 'عدد الأيام المعروضة فى اخرالتغييرات:',
'recentchangescount'       => 'عدد التعديلات للعرض فى اخر التغييرات، صفحات التواريخ والسجلات:',
'savedprefs'               => 'التفضيلات بتاعتك اتحفظت.',
'timezonelegend'           => 'منطقة التوقيت',
'timezonetext'             => '¹الفرق فى الساعات بين توقيتك المحلى و توقيت السيرفر (UTC).',
'localtime'                => 'التوقيت المحلي',
'timezoneoffset'           => 'الفرق¹',
'servertime'               => 'توقيت السيرفر',
'guesstimezone'            => 'دخل التوقيت من البراوزر',
'allowemail'               => 'السماح لليوزرز التانيين يبعتولى ايميل',
'prefs-searchoptions'      => 'اختيارات التدوير',
'prefs-namespaces'         => 'أسماء النطاقات',
'defaultns'                => 'دور فى النطاقات دى اوتوماتيكي:',
'default'                  => 'اوتوماتيكي',
'files'                    => 'ملفات',

# User rights
'userrights'                  => 'إدارة الحقوق بتاعة اليوزر', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'إدارة مجموعات اليوزر',
'userrights-user-editname'    => 'دخل اسم يوزر:',
'editusergroup'               => 'تعديل مجموعات اليوزر',
'editinguser'                 => "تغيير حقوق االيوزر بتاعة اليوزر'''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'تعديل مجموعات اليوزر',
'saveusergroups'              => 'حفظ مجموعات اليوزر',
'userrights-groupsmember'     => 'عضو في:',
'userrights-groups-help'      => 'إنت ممكن تغير المجموعات اللى اليوزر دا عضو فيها .
* صندوق متعلم يعنى اليوزر دا عضو فى المجموعة دي.
* صندوق مش متعلم يعنى  اليوزر دا مش عضو فى المجموعة دي.
* علامة * يعنى انك مش ممكن تشيل المجموعات بعد ما تضيفها و العكس بالعكس.',
'userrights-reason'           => 'سبب التغيير:',
'userrights-no-interwiki'     => 'أنت  مش من حقك تعدل صلاحيات اليوزرز على الويكيات التانية.',
'userrights-nodatabase'       => 'قاعدة البيانات $1  مش موجودة أو مش محلية.',
'userrights-nologin'          => 'انت لازم [[Special:UserLogin|تسجيل الدخول]] بحساب  مدير لتعديل حقوق اليوزر.',
'userrights-notallowed'       => 'حسابك  ماعندوش  إذن لتعديل حقوق اليوزر.',
'userrights-changeable-col'   => 'المجموعات اللى تقدر تغييرها',
'userrights-unchangeable-col' => 'المجموعات اللى مش ممكن انك تغيرها',

# Groups
'group'               => 'المجموعة:',
'group-user'          => 'يوزرز',
'group-autoconfirmed' => 'يوزرز متأكدين أوتوماتيكي',
'group-bot'           => 'بوتات',
'group-sysop'         => 'سيسوبات',
'group-bureaucrat'    => 'بيروقراطيين',
'group-suppress'      => 'أوفرسايت',
'group-all'           => '(الكل)',

'group-user-member'          => 'يوزر',
'group-autoconfirmed-member' => 'يوزر متأكد أوتوماتيكي',
'group-bot-member'           => 'بوت',
'group-sysop-member'         => 'سيسوب',
'group-bureaucrat-member'    => 'بيروقراطي',
'group-suppress-member'      => 'أوفرسايت',

'grouppage-user'          => '{{ns:project}}:يوزرز',
'grouppage-autoconfirmed' => '{{ns:project}}:يوزرز متأكدين أوتوماتيكي',
'grouppage-bot'           => '{{ns:project}}:بوتات',
'grouppage-sysop'         => '{{ns:project}}:اداريين',
'grouppage-bureaucrat'    => '{{ns:project}}:بيروقراطيين',
'grouppage-suppress'      => '{{ns:project}}:أوفرسايت',

# Rights
'right-read'                 => 'قراية الصفحات',
'right-edit'                 => 'تعديل الصفحات',
'right-createpage'           => 'إبتدى الصفحات (اللى مالهاش صفحات نقاش)',
'right-createtalk'           => 'إبتدى صفحات النقاش',
'right-createaccount'        => 'افتح حسابات يوزر جديده',
'right-minoredit'            => 'التعليم على التعديلات كطفيفة',
'right-move'                 => 'انقل الصفحات',
'right-move-subpages'        => 'انقل الصفحات مع صفحاتها الفرعيه',
'right-suppressredirect'     => 'ما تعملش تحويلة من الاسم القديم عند نقل صفحة',
'right-upload'               => 'حمل الملفات',
'right-reupload'             => 'الكتابة على ملف موجود',
'right-reupload-own'         => 'الكتابة على ملف موجود اتحمل ب اليوزر نفسه',
'right-reupload-shared'      => 'التحميل على الملفات فى مخزن الملفات المشترك  فى المكان دا بس',
'right-upload_by_url'        => 'تحميل ملف من عنوان مسار',
'right-purge'                => 'تحديث كاش الموقع لصفحة من غير تأكيد',
'right-autoconfirmed'        => 'تعديل الصفحات  النص محميه',
'right-bot'                  => 'بتتعامل كأنها عملية أوتوماتيكية',
'right-nominornewtalk'       => 'ماتخليش التعديلات الطفيفة لصفحات النقاش تتطلع برواز الرسايل الجديدة',
'right-apihighlimits'        => 'استخدام حدود أعلى فى استعلامات API',
'right-writeapi'             => 'استخدام API الكتابة',
'right-delete'               => 'مسح الصفحات',
'right-bigdelete'            => 'مسح الصفحات اللى ليها تواريخ كبيرة',
'right-deleterevision'       => 'مسح وترجيع مراجعات معينة من الصفحات',
'right-deletedhistory'       => 'شوف مدخلات التاريخ الممسوحة، من غير النصوص اللى معاها',
'right-browsearchive'        => 'التدوير فى الصفحات الممسوحة',
'right-undelete'             => 'استرجاع صفحة',
'right-suppressrevision'     => 'مراجعة واسترجاع المراجعات المستخبية عن الإداريين',
'right-suppressionlog'       => 'شوف السجلات الخاصة',
'right-block'                => 'امنع اليوزرز التانيين من التعديل',
'right-blockemail'           => 'منع يوزر من إنه يبعت إيميل',
'right-hideuser'             => 'منع اسم يوزر، و خبيه عن الناس',
'right-ipblock-exempt'       => 'إتفادى عمليات منع الأيبي، المنع الأوتوماتيكى ومنع النطاق.',
'right-proxyunbannable'      => 'إتفادى عمليات المنع الأوتوماتيكية للبروكسيهات',
'right-protect'              => 'تغيير مستويات الحماية وتعديل الصفحات المحمية',
'right-editprotected'        => 'تعديل الصفحات المحمية (من غير الحماية المتضمنة)',
'right-editinterface'        => 'تعديل الواجهة بتاعة اليوزر',
'right-editusercssjs'        => 'تعديل ملفات CSS و JS لليوزرز التانيين',
'right-rollback'             => 'رجع بسرعه التعديلات بتاعة آخر يوزر عدل صفحة معينة',
'right-markbotedits'         => 'التعليم على التعديلات المترجعة كتعديلات بوت',
'right-noratelimit'          => 'مش متأثر بحدود المعدل',
'right-import'               => 'استيراد الصفحات من ويكيات تانيه',
'right-importupload'         => 'استيراد الصفحات من فايل متحمل',
'right-patrol'               => 'علم على تعديلات اليوزرز التانيين على انها متراجعة.',
'right-autopatrol'           => 'خلى التعديلات  بتاعتى متعلم عليها كأنها متراجعة أوتوماتيكي',
'right-patrolmarks'          => 'عرض علامات المراجعة فى أحدث التغييرات',
'right-unwatchedpages'       => 'بين لستة الصفحات اللى مش متراقبة',
'right-trackback'            => 'تنفيذ تراكباك',
'right-mergehistory'         => 'ادمج تاريخ الصفحات',
'right-userrights'           => 'تعديل كل الحقوق بتاعة اليوزر',
'right-userrights-interwiki' => 'تعديل صلاحيات اليوزر لليوزرز فى مواقع الويكى التانيه',
'right-siteadmin'            => 'قفل وفتح قاعدة البيانات',

# User rights log
'rightslog'      => 'سجل صلاحيات اليوزرز',
'rightslogtext'  => 'ده سجل بالتغييرات ف صلاحيات اليوزرز .',
'rightslogentry' => 'غير صلاحيات $1 من $2 ل $3',
'rightsnone'     => '(فاضى)',

# Recent changes
'nchanges'                          => '{{PLURAL:$1|تعديل|تعديلين|$1 تعديلات|$1 تعديل|$1 تعديل}}',
'recentchanges'                     => 'اخر التعديلات',
'recentchangestext'                 => 'تابع آخر التغييرات فى الويكى على الصفحة دي.',
'recentchanges-feed-description'    => 'تابع احدث التعديلات للويكى ده عن طريق الفييد ده .',
'rcnote'                            => "فيه تحت {{PLURAL:$1|'''1''' تغيير|آخر '''$1''' تغيير}} فى آخر {{PLURAL:$2|يوم|'''$2''' يوم}}، بدءا من $5، $4.",
'rcnotefrom'                        => "دى التعديلات من '''$2''' (ل '''$1''' معروضه).",
'rclistfrom'                        => 'اظهر التعديلات بدايه من $1',
'rcshowhideminor'                   => '$1 تعديلات صغيره',
'rcshowhidebots'                    => '$1 البوتات',
'rcshowhideliu'                     => '$1 اليوزرز المتسجلين',
'rcshowhideanons'                   => '$1 اليوزرز المجهولين',
'rcshowhidepatr'                    => '$1 التعديلات المتراجعه',
'rcshowhidemine'                    => '$1 تعديلاتى',
'rclinks'                           => 'بيين اخر $1 تعديل فى اخر $2 يوم، $3',
'diff'                              => 'التغيير',
'hist'                              => 'تاريخ',
'hide'                              => 'تخبيه',
'show'                              => 'عرض',
'minoreditletter'                   => 'ص',
'newpageletter'                     => 'ج',
'boteditletter'                     => 'ب',
'sectionlink'                       => '←',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1| يوزر مراقب|يوزر مراقب}}]',
'rc_categories'                     => 'حصر لتصنيفات (مفصولة برمز "|")',
'rc_categories_any'                 => 'أى',
'newsectionsummary'                 => '/* $1 */ قسم جديد',

# Recent changes linked
'recentchangeslinked'          => 'تعديلات  ليها علاقه',
'recentchangeslinked-title'    => 'التعديلات المرتبطه  ب "$1"',
'recentchangeslinked-noresult' => 'مافيش تعديلات حصلت فى الصفحات اللى ليها وصلات هنا خلال الفترة المحدده.',
'recentchangeslinked-summary'  => "دى صفحة مخصوصه بتعرض اخر التغييرات فى الصفحات الموصوله (أو إلى الأعضاء ضمن تصنيف معين).
الصفحات اللى فى  [[Special:Watchlist|لسته بالصفحات اللى انت بتراقب التعديلات فيها]] معروضه'''بحروف عريضه'''",
'recentchangeslinked-page'     => 'اسم الصفحه :',
'recentchangeslinked-to'       => 'إظهارالتغييرات للصفحات الموصولة للصفحة اللى انت اديتها',

# Upload
'upload'                      => 'حمل',
'uploadbtn'                   => 'حمل الملف',
'reupload'                    => 'حمل مره تانيه',
'reuploaddesc'                => 'إلغى التحميل وارجع لاستمارة التحميل',
'uploadnologin'               => 'ما سجلتش الدخول',
'uploadnologintext'           => 'لازم تكون [[Special:UserLogin|مسجل الدخول]] علشان تقدر تحمل الملفات.',
'upload_directory_missing'    => 'مجلد التحميل($1) ضايع السيرفير وماقدرش يعمل واحد تاني.',
'upload_directory_read_only'  => 'مجلد التحميل ($1) مش ممكن الكتابة عليه بواسطة سيرڨر الويب.',
'uploaderror'                 => 'غلطه فى التحميل',
'uploadtext'                  => "استخدم الاستمارة لتحميل الملفات.
لعرض أو البحث ف الملفات المتحملة سابقا، راجع [[Special:ImageList|قايمة الملفات المتحملة]]، عمليات التحميل والحذف موجودة فى [[Special:Log/upload|سجل التحميل]].

علشان تحط صورة فى صفحة، استخدم الوصلات فى الصيغ التالية:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' لاستخدام النسخة الكاملة لملف
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|نص بديل]]</nowiki></tt>''' لاستخدام صورة عرضها 200 بكسل فى صندوق فى الجانب الأيسر مع 'نص بديل' كوصف
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' للوصل للملف مباشرة بدون عرض الملف.",
'upload-permitted'            => 'أنواع الملفات المسموحة: $1.',
'upload-preferred'            => 'أنواع الملفات المفضلة: $1.',
'upload-prohibited'           => 'أنواع الملفات الممنوعة: $1.',
'uploadlog'                   => 'سجل التحميل',
'uploadlogpage'               => 'سجل التحميل',
'uploadlogpagetext'           => 'تحت فية لستة بأحدث عمليات تحميل الملفات.
انظر [[Special:NewImages|معرض الملفات الجديدة]] لعرض بصرى أكتر',
'filename'                    => 'اسم الملف',
'filedesc'                    => 'الخلاصة',
'fileuploadsummary'           => 'الخلاصة:',
'filestatus'                  => 'حالة حقوق النسخ:',
'filesource'                  => 'مصدر:',
'uploadedfiles'               => 'الملفات المتحملة',
'ignorewarning'               => 'إتجاهل التحذير و احفظ الملف وخلاص',
'ignorewarnings'              => 'اتجاهل اى تحذير',
'minlength1'                  => 'أسامى الملفات لازم تكون متكونة من حرف واحد على الأقل.',
'illegalfilename'             => 'اسم الملف "$1" فيه علامات  مش مسموح بيها فى عناوين الصفحات.
لو سمحت تختار اسم تانى للمف و بعدين تحمله من اول و جديد.',
'badfilename'                 => ' اسم الملف إتغيير ل "$1".',
'filetype-badmime'            => 'مش مسموح تحميل ملفات من نوع "$1".',
'filetype-unwanted-type'      => "'''\".\$1\"''' هو مش نوع ملف مرغوب فيه.
{{PLURAL:\$3|نوع الملف المفضل هو|أنواع الملفات المفضلة هي}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' مش نوع ملف مسموح بيه.
{{PLURAL:\$3|نوع الملف المسموح بيه هو|أنواع الملفات المسموح بيها هي}} \$2.",
'filetype-missing'            => 'الملف مالوش امتدا(مثلا ".jpg").',
'large-file'                  => 'ينصح ان الملفات ماتكونش أكبر من $1؛ الملف ده حجمه $2.',
'largefileserver'             => 'حجم الملف ده أكبر من المسموح بيه على السيرڨر ده .',
'emptyfile'                   => 'الظاهر ان الملف اللى انت حملته طلع فاضي.
يمكن يكون السبب هوه كتابة الاسم غلط.
لو سمحت تتاكد من إنك فعلا عايز تحمل الملف دا..',
'fileexists'                  => 'فيه  ملف موجود بالاسم ده  الرجاء التأكد من الملف ده باتباع الوصلة التالية <strong><tt>$1</tt></strong> قبل ما تغيره.',
'filepageexists'              => 'صفحة الوصف بتاعة المف دا خلاص اتعملها انشاء فى <strong><tt>$1</tt></strong>، بس مافيش ملف بالاسم دا دلوقتى.
الملخص اللى ح تكتبه  مش ح يظهر على صفحة الوصف.
علشان تخلى الملف يظهر هناك، ح تحتاج تعدله يدوي.',
'fileexists-extension'        => 'فى ملف موجود باسم قريب:<br />
اسم الملف اللى انت عايز تحمله: <strong><tt>$1</tt></strong><br />
اسم الملف الموجود: <strong><tt>$2</tt></strong><br />
لو سمحت تختار اسم تاني.',
'fileexists-thumb'            => "<center>'''الملف الموجود'''</center>",
'fileexists-thumbnail-yes'    => 'الظاهر ان الملف دا عبارة عن صورة متصغرة <i>(تصغير)</i>. 
لو سمحت تشيك على الملف <strong><tt>$1</tt></strong>.<br />
لو كان الملف هو نفس الصورة بالحجم الاصلي، ف مافيش داعى تحمله مرة تانية',
'file-thumbnail-no'           => 'يبدأ الملف ب <strong><tt>$1</tt></strong>. 
يبدو أن الملف مصتغر لحجم أعلى <i>(تصغير)</i>.
إذا كان عندك الصورة فى درجة دقة كامله حملها، أو غير اسم الملف من فضلك.',
'fileexists-forbidden'        => 'فى ملف تانى بنفس الاسم موجود
لو سمحت ترجع تانى و تحمل الملف باسم جديد. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'فى ملف بنفس الاسم دا فى مخزن الملفات المشترك.
لو كنت لسه عايز ترفعه، لو سمحت ارجع وحمل الملف دا باسم جديد.
[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'الملف دا تكرار  {{PLURAL:$1|للملف|للملفات}} دي:',
'successfulupload'            => 'التحميل ناجح',
'uploadwarning'               => 'تحذير التحميل',
'savefile'                    => 'حفظ الملف',
'uploadedimage'               => 'اتحمل "[[$1]]"',
'overwroteimage'              => 'اتحملت  نسخة جديدة من "[[$1]]"',
'uploaddisabled'              => 'التحميل متعطل',
'uploaddisabledtext'          => 'تحميل الملفات متعطل.',
'uploadscripted'              => 'الملف دا  فيه كود HTML أو كود تانى يمكن البراوزر يفهمه غلط.',
'uploadcorrupt'               => 'الملف دا بايظ أو ليه امتداد غلط.
لو سمحت ا تأكد من الملف و حمله مرة تانية.',
'uploadvirus'                 => 'الملف فيه فيروس! التفاصيل: $1',
'sourcefilename'              => 'اسم الملف  بتاع المصدر:',
'destfilename'                => 'اسم الملف المستهدف:',
'upload-maxfilesize'          => 'حجم الملف الأقصى: $1',
'watchthisupload'             => 'حط الصفحة دى تحت المراقبة',
'filewasdeleted'              => 'فى ملف بنفس الاسم دا تحمل قبل كدا و بعدين اتمسح.
لو سمحت تراجع $1 قبل ما تحمله كمان مرة.',
'upload-wasdeleted'           => "'''تحذير: انت بتحمل ملف اتمسح قبل كدا.'''

لازم تتاكد من انك عايز تستمر فى تحميل الملف دا.
سجل المسح بتاع الملف دا معروض هنا علشان تبص عليه:",
'filename-bad-prefix'         => 'اسم الملف اللى بتحمله بيبتدى بـ<strong>"$1"</strong>، واللى هو اسم مش وصفى بيتحط غالبا من الكاميرات الديجيتال اوتوماتيكي.
لو سمحت تختار اسم يكون بيوصف الملف بتاعك احسن من كدا.',
'filename-prefix-blacklist'   => ' #<!-- سيب السطر ده زى ما هوه --> <pre>
# الصيغة كدا: 
#   * كل حاجة من أول علامة "#" لحد أخر السطر هى تعليق
#   * كل سطر مش فاضى هو بريفيكس لأسماء الملفات النمطية اللى بتحطها اوتوماتيكى  الكاميرات الديجيتال
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # بعض التليفونات المحمولة
IMG # generic
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- سيب السطر ده زى ما هوه -->',

'upload-proto-error'      => 'بروتوكول مش صحيح',
'upload-proto-error-text' => 'االتحميل عن بعد لازمله يوأرإل بيبتدى بـ <code>http://</code> أو <code>ftp://</code>.',
'upload-file-error'       => 'غلط داخلي',
'upload-file-error-text'  => 'حصل غلط داخلى واحنا بنحاول نعمل ملف مؤقت على السيرفر.
لو سمحت اتصل بإدارى نظام.',
'upload-misc-error'       => 'غلط مش معروف فى التحميل',
'upload-misc-error-text'  => 'حصل غلط مش معروف وإنت بتحمل.
لو سمحت تتاكد أن اليوأرإل صح و ممكن تدخل عليه و بعدين حاول تاني.
إذا المشكلة تنتها موجودة،اتصل بإدارى نظام.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'ما قدرناش نوصل لليو أر إل',
'upload-curl-error6-text'  => 'ما قدرناش نوصل لليوأرإل إللى انت عاوزه.
لو سمحت تشيك تانى إن اليوأرإل صح و إن السايت شغال.',
'upload-curl-error28'      => 'انتهاء مهلة التحميل',
'upload-curl-error28-text' => 'السايت خد وقت كبير علشان يستجيب.
لو سمحت اتأكد أن السايت شغال، واستنا شوية و بعدين حاول تاني.
يمكن تجرب تانى فى وقت مايكونش فيه زحمة.',

'license'            => 'ترخيص:',
'nolicense'          => 'مش متحدد',
'license-nopreview'  => '(البروفه مش متوفره)',
'upload_source_url'  => '  (اليوأرإل صحيح وممكن توصل ليه)',
'upload_source_file' => ' (ملف على الكمبيوتر بتاعك)',

# Special:ImageList
'imagelist-summary'     => 'الصفحة المخصوصة دى بتعرض كل الملفات المتحملة.
اوتوماتيكى اخر الملفات اللى اتحملت ح تظهر فى اللستة من فوق.
لو دوست على راس العمود الترتيب ح يتغير.',
'imagelist_search_for'  => 'دور على اسم الميديا:',
'imgfile'               => 'ملف',
'imagelist'             => 'لستة الملفات',
'imagelist_date'        => 'تاريخ',
'imagelist_name'        => 'اسم',
'imagelist_user'        => 'يوزر',
'imagelist_size'        => 'حجم',
'imagelist_description' => 'وصف',

# Image description page
'filehist'                       => 'تاريخ الملف',
'filehist-help'                  => 'اضغط على الساعه/التاريخ علشان تشوف الفايل زى ما كان فى  الوقت ده.',
'filehist-deleteall'             => 'امسح كله',
'filehist-deleteone'             => 'مسح',
'filehist-revert'                => 'استرجع',
'filehist-current'               => 'دلوقتي',
'filehist-datetime'              => 'الساعه / التاريخ',
'filehist-user'                  => 'يوزر',
'filehist-dimensions'            => 'ابعاد',
'filehist-filesize'              => 'حجم الفايل',
'filehist-comment'               => 'تعليق',
'imagelinks'                     => 'وصلات',
'linkstoimage'                   => '{{PLURAL:$1|الصفحة|ال$1 صفحة}} دى فيها وصله للفايل ده:',
'nolinkstoimage'                 => 'مافيش صفحات بتوصل للفايل ده.',
'morelinkstoimage'               => 'عرض [[Special:WhatLinksHere/$1|لينكات اكتر]] للملف دا.',
'redirectstofile'                => '{{PLURAL:$1| الملف|ال$1 ملف}} اللى جاى  بيحول للملف دا:',
'duplicatesoffile'               => '{{PLURAL:$1| الملف|ال$1 ملف اللى بعده}} مكررات للملف دا:',
'sharedupload'                   => 'الملف ده اتحمل علشان التشارك بين المشاريع وممكن استخدامه فى المشاريع التانيه.',
'shareduploadwiki'               => 'لو سمحت تشوف $1 اللى فيه معلوات زيادة.',
'shareduploadwiki-desc'          => 'الوصف على $1 فى المخزن المشترك معروض تحت.',
'shareduploadwiki-linktext'      => 'صفحة وصف الملف',
'shareduploadduplicate'          => 'الملف دا تكرار ل$1 من المخزن المشترك.',
'shareduploadduplicate-linktext' => 'ملف تاني',
'shareduploadconflict'           => 'الملف دا ليه نفس الاسم زى $1 من المخزن المشترك.',
'shareduploadconflict-linktext'  => 'ملف تانى',
'noimage'                        => 'مافيش  ملف بالاسم ده، لكن ممكن انك  تـ$1.',
'noimage-linktext'               => 'تحميل واحد',
'uploadnewversion-linktext'      => 'حمل نسخه جديده من الملف ده',
'imagepage-searchdupe'           => 'دور على ملفات متكررة',

# File reversion
'filerevert'                => 'استرجع $1',
'filerevert-legend'         => 'استرجع الملف',
'filerevert-intro'          => "أنت بترجع '''[[Media:$1|$1]]''' [$4 للنسخةاللى بتاريخ $2، $3].",
'filerevert-comment'        => 'تعليق:',
'filerevert-defaultcomment' => 'رجع النسخة اللى بتاريخ $2، $1',
'filerevert-submit'         => 'استرجع',
'filerevert-success'        => "'''[[Media:$1|$1]]''' اترجعت [$4 للنسخةاللى بتاريخ $2، $3].",
'filerevert-badversion'     => 'مافيش نسخة محلية قديمة  للملف دا بالتاريخ المتقدم',

# File deletion
'filedelete'                  => 'امسح $1',
'filedelete-legend'           => 'امسح الملف',
'filedelete-intro'            => "أنت بتمسح '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => '<span class="plainlinks">أنت بتمسح نسخة \'\'\'[[Media:$1|$1]]\'\'\'اللى  بتاريخ [$4 $3، $2].</span>',
'filedelete-comment'          => 'سبب المسح:',
'filedelete-submit'           => 'مسح',
'filedelete-success'          => "'''$1''' خلاص اتمسح.",
'filedelete-success-old'      => "نسخة الـ'''[[Media:$1|$1]]''' اللى بتاريخ $3، $2 اتمسحت.",
'filedelete-nofile'           => "'''$1''' مش موجود.",
'filedelete-nofile-old'       => "مافيش نسخة فى الارشيف من '''$1''' بالعناصر المتحددة.",
'filedelete-iscurrent'        => 'أنت بتحاول تمسح أجدد نسخة من الملف دا.
لو سمحت ترجع النسخة القديمة الاول.',
'filedelete-otherreason'      => 'سبب زيادة/تاني:',
'filedelete-reason-otherlist' => 'سبب تانى',
'filedelete-reason-dropdown'  => '*أسباب المسح الشايعة
** الاعتداء على حقوق النشر
** ملف متكرر',
'filedelete-edit-reasonlist'  => 'عدل أسباب المسح',

# MIME search
'mimesearch'         => 'تدوير MIME',
'mimesearch-summary' => 'الصفحة دى مهمتها فلترة الملفات على حسب نوعها.
المدخل: نوع المحتوى/النوع الفرعي، يعنى مثلا
<tt>image/jpeg</tt>.',
'mimetype'           => 'نوع الملف:',
'download'           => 'تنزيل',

# Unwatched pages
'unwatchedpages' => 'صفحات مش متراقبة',

# List redirects
'listredirects' => 'عرض التحويلات',

# Unused templates
'unusedtemplates'     => 'قوالب مش مستعمله',
'unusedtemplatestext' => 'الصفحة دى فيها لستة بالصفحات من نطاق قالب و اللى مش مستعملة فى صفحات تانية
افتكر قبل ما تمسحها تشوف لو فى وصلات تانية للقوالب دي',
'unusedtemplateswlh'  => 'وصلات  تانيه',

# Random page
'randompage'         => 'صفحة عشوائيه',
'randompage-nopages' => 'مافيش صفحات فى النطاق دا.',

# Random redirect
'randomredirect'         => 'تحويله عشوائيه',
'randomredirect-nopages' => 'مافيش تحويلات فى النطاق ده.',

# Statistics
'statistics'             => 'احصائيات',
'sitestats'              => 'إحصائيات {{SITENAME}}',
'userstats'              => 'الاحصاءات بتاعة اليوزر',
'sitestatstext'          => "فيه{{PLURAL:\$1|'''1''' صفحه|'''\$1''' صفحه كليه}} فى قاعدة البيانات.
ده يشمل  صفحات \"المناقشه\"، الصفحات حول {{SITENAME}}، الصغرى \"البذور\"
صفحات، التحويلات، و غيرها اللى غالبا ما تتأهلش كصفحات محتوى.
باستثناء اللى، فيه {{PLURAL:\$2|'''1''' صفحة| '''\$2''' صفحة}} تعتبر على الأرجح
{{PLURAL:\$2|صفحة|صفحة}} محتوى معتبرة.

'''\$8''' {{PLURAL:\$8|ملف|ملف}} تم تحميله.

فيه كإجمالى '''\$3''' {{PLURAL:\$3|عرض صفحة|عرض صفحة}}، و '''\$4''' {{PLURAL:\$4|تعديل صفحة|تعديل صفحة}}
من تنصيب {{SITENAME}}.
ده يعنى '''\$5''' تعديل متوسط لكل صفحة، و '''\$6''' عرض لكل تعديل.

طول [http://www.mediawiki.org/wiki/Manual:Job_queue طابور الشغل] هو '''\$7'''.",
'userstatstext'          => "فى {{PLURAL:$1|[[Special:ListUsers|يوزر]] متسجل '''واحد'''|'''$1''' [[Special:ListUsers|يوزر]] متسجل}}، منهم '''$2''' (أو '''$4%''') {{PLURAL:$2|عنده|عندهم}} صلاحيات $5.",
'statistics-mostpopular' => 'اكتر صفحات اتشافت',

'disambiguations'      => 'صفحات التوضيح',
'disambiguationspage'  => 'Template:توضيح',
'disambiguations-text' => "الصفحات دى بتوصل لـ '''صفحة توضيح'''.
المفروض على العكس انهم يوصلو ل للصفحات المناسبة. <br />
أى صفحة بتتعامل على انها صفحة توضيح إذا كانت بتستعمل قالب موجود فى [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'تحويلات مزدوجه',
'doubleredirectstext'        => 'الصفحة دى فيها لستة الصفحات اللى فيها تحويلة لصفحة تانية فيها تحويلة.
كل سطر فى اللستة دى  فيه لينك للتحويلة الأولانية والتانية و كمان للصفحة بتاعة التحويلة التانية و اللى غالبا هى الصفحة الاصلية اللى المفروض التحويلة الاولانية توصل ليها.',
'double-redirect-fixed-move' => '[[$1]] اتنقلت، هى دلوقتى تحويله ل [[$2]]',
'double-redirect-fixer'      => 'مصلح التحويل',

'brokenredirects'        => 'تحويلات مكسوره',
'brokenredirectstext'    => 'التحويلات دى بتوصل لصفحات مش موجودة:',
'brokenredirects-edit'   => '(تحرير)',
'brokenredirects-delete' => '(مسح)',

'withoutinterwiki'         => 'صفحات بدون وصلات للغات تانيه',
'withoutinterwiki-summary' => 'الصفحات دى  مالهاش لينكات لنسخ بلغات تانية:',
'withoutinterwiki-legend'  => 'بريفيكس',
'withoutinterwiki-submit'  => 'عرض',

'fewestrevisions' => 'اقل المقالات فى عدد التعديلات',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|بايت|بايت}}',
'ncategories'             => '$1 {{PLURAL:$1|تصنيف واحد|تصنيفين|تصنيفات|تصنيف}}',
'nlinks'                  => '{{PLURAL:$1|وصله واحده|وصلتين|$1 وصلات|$1 وصله}}',
'nmembers'                => '$1 {{PLURAL:$1|عضو|اعضاء}}',
'nrevisions'              => '{{PLURAL:$1|تعديل وحيد|تعديلين|$1 تعديلات|$1 تعديل|$1}}',
'nviews'                  => '{{PLURAL:$1|مشاهدة واحدة|مشاهدتين|$1 مشاهدات|$1 مشاهدة}}',
'specialpage-empty'       => 'مافيش نتايج للتقرير دا.',
'lonelypages'             => 'صفحات يتيمه',
'lonelypagestext'         => 'الصفحات دى ماعندهاش لينكات  من الصفحات التانية فى {{SITENAME}}.',
'uncategorizedpages'      => 'صفحات مش متصنفه',
'uncategorizedcategories' => 'تصنيفات مش متصنفه',
'uncategorizedimages'     => 'ملفات مش متصنفه',
'uncategorizedtemplates'  => 'قوالب مش متصنفه',
'unusedcategories'        => 'تصانيف مش  مستعمله',
'unusedimages'            => 'صور مش مستعمله',
'popularpages'            => 'صفحات مشهورة',
'wantedcategories'        => 'تصانيف مطلوبه',
'wantedpages'             => 'صفحات مطلوبه',
'missingfiles'            => 'ملفات ضايعة',
'mostlinked'              => 'اكتر صفحات موصولة بصفحات تانيه',
'mostlinkedcategories'    => 'اكتر التصانيف فى عدد الارتباطات',
'mostlinkedtemplates'     => 'اكتر القوالب فى عدد الوصلات',
'mostcategories'          => 'اكتر الصفحات فى عدد التصانيف',
'mostimages'              => 'اكتر الملفات فى عدد الارتباطات',
'mostrevisions'           => 'اكتر المقالات فى عدد التعديلات',
'prefixindex'             => 'فهرس البريفكسات',
'shortpages'              => 'صفحات قصيره',
'longpages'               => 'صفحات طويله',
'deadendpages'            => 'صفحات ما بتوصلش  لحاجه',
'deadendpagestext'        => 'الصفحات دى مابتوصلش  لصفحات تانية فى {{SITENAME}}.',
'protectedpages'          => 'صفحات محميه',
'protectedpages-indef'    => 'عمليات الحماية اللى مش متحددة بس',
'protectedpagestext'      => 'الصفحات دى محمية من النقل أو التعديل',
'protectedpagesempty'     => 'مافيش  صفحات محمية دلوقتى  على حسب المحددات دي.',
'protectedtitles'         => 'عناوين محمية',
'protectedtitlestext'     => 'العناوين دى محمية ضد الإنشاء',
'protectedtitlesempty'    => 'مافيش عناوين محمية دلوقتى على حسب المحددات دي.',
'listusers'               => 'لستة الأعضاء',
'newpages'                => 'صفحات جديده',
'newpages-username'       => 'اسم اليوزر:',
'ancientpages'            => 'اقدم الصفحات',
'move'                    => 'انقل',
'movethispage'            => 'انقل الصفحه دى',
'unusedimagestext'        => 'لو سمحت تاخد بالك إن المواقع التانية ممكن تكون بتوصل لملف عن طريق يوأرإل مباشر، و علشان كدا ممكن يكون لسة معروض هنا مع إنه بيستعمل.',
'unusedcategoriestext'    => 'التصنيفات دى موجودة مع إنها ما فيهاش اى صفحات او تصنيفات تانية.',
'notargettitle'           => 'مافيش هدف',
'notargettext'            => 'انت ما حددتش الصفحة أو اليوزر المستهدف لعمل العملية دي.',
'nopagetitle'             => 'مافيش صفحة هدف بالاسم ده',
'nopagetext'              => 'صفحة الهدف اللى انت طالبها مش موجودة.',
'pager-newer-n'           => '{{PLURAL:$1|أجدد 1|أجدد $1}}',
'pager-older-n'           => '{{PLURAL:$1|أقدم 1|أقدم $1}}',
'suppress'                => 'أوفرسايت',

# Book sources
'booksources'               => 'مصادر من كتب',
'booksources-search-legend' => 'التدوير على مصادر الكتب',
'booksources-go'            => 'روح',
'booksources-text'          => 'فى تحت لستة بوصلات لمواقع تانية بتبيع الكتب الجديدة والمستعملة، كمان ممكن تلاقى معلومات إضافية عن الكتب اللى يتدور عليها :',

# Special:Log
'specialloguserlabel'  => 'اليوزر:',
'speciallogtitlelabel' => 'العنوان:',
'log'                  => 'سجلات',
'all-logs-page'        => 'كل السجلات',
'log-search-legend'    => 'دور على سجلات',
'log-search-submit'    => 'روح',
'alllogstext'          => 'عرض شامل لكل السجلات الموجودة فى {{SITENAME}}. 
ممكن تخلى اللستة متحددة اكتر لو تختار نوع العملية، أو اسم اليوزر (حساس لحالة الحروف)، أو الصفحة المتأثرة (برضه حساس لحالة الحروف).',
'logempty'             => 'مافيش  سجلات مطابقة فى السجل.',
'log-title-wildcard'   => 'التدوير على عناوين تبتدى بالنص دا',

# Special:AllPages
'allpages'          => 'كل الصفحات',
'alphaindexline'    => '$1 ل $2',
'nextpage'          => 'الصفحه اللى بعد كده ($1)',
'prevpage'          => 'الصفحه اللى قبل كده ($1)',
'allpagesfrom'      => 'عرض الصفحات بدايه من:',
'allarticles'       => 'كل المقالات',
'allinnamespace'    => 'كل الصفحات (فى نطاق $1)',
'allnotinnamespace' => 'كل الصفحات (مش فى نطاق $1)',
'allpagesprev'      => 'اللى فلت',
'allpagesnext'      => 'اللى بعد كده',
'allpagessubmit'    => 'روح',
'allpagesprefix'    => 'عرض الصفحات  اللى تبتدى بـ:',
'allpagesbadtitle'  => 'العنوان االلى اديته للصفحة مش نافع أو فيه لغات تانية أو بريفيكس إنترويكي.
يمكن فيه حروف ماينفعش تنكتب بيها العناوين.',
'allpages-bad-ns'   => '{{SITENAME}} مافيهاش نطاق "$1".',

# Special:Categories
'categories'                    => 'تصانيف',
'categoriespagetext'            => 'التصنيفات دى فيها صفحات أو ميديا
[[Special:UnusedCategories|التصنيفات المش مستعملة]] مش معروضة هنا.
شوف كمان [[Special:WantedCategories|التصنيفات المطلوبة]].',
'categoriesfrom'                => 'اعرض التصانيف من أول:',
'special-categories-sort-count' => 'رتب بالعدد',
'special-categories-sort-abc'   => 'ترتيب ابجدي',

# Special:ListUsers
'listusersfrom'      => 'عرض اليوزرز من أول:',
'listusers-submit'   => 'عرض',
'listusers-noresult' => 'ما فى ش يوزر',

# Special:ListGroupRights
'listgrouprights'          => 'حقوق مجموعات اليوزرز',
'listgrouprights-summary'  => 'دى لستة بمجموعات اليوزرز المتعرفة فى الويكى دا، بالحقوق اللى معاهم.
ممكن تلاقى معلومات زيادة عن الحقوق بتاعة كل واحد  [[{{MediaWiki:Listgrouprights-helppage}}|هنا]].',
'listgrouprights-group'    => 'المجموعة',
'listgrouprights-rights'   => 'الحقوق',
'listgrouprights-helppage' => 'Help: حقوق المجموعات',
'listgrouprights-members'  => '(لستة الأعضاء)',

# E-mail user
'mailnologin'     => 'مافيش عنوان نبعت عليه',
'mailnologintext' => 'لازم تعمل [[Special:UserLogin|تسجيل الدخول]] و تدخل ايميل صحيح فى صفحة [[Special:Preferences|التفضيلات]] علشان تقدر تبعت ايميلات لليوزرز التانيين.',
'emailuser'       => 'ابعت ايميل لليوزر دا',
'emailpage'       => 'ابعت ايميل لليوزر ده',
'emailpagetext'   => 'لو اليوزر دا دخل ايميل صحيح فى التفضيلات بتاعته،
ف حيتبعت له رسالة واحدة بس حسب الاستمارة اللى تحت دي.
عنوان الايميل اللى دخلته فى [[Special:Preferences|التفضيلات بتاعتك]]
ح يظهر فى  على انه عنوان الاستمارة و بكدة اللى حيستقبله ح يقدر يرد على الايميل.',
'usermailererror' => 'البريد رجع غلط:',
'defemailsubject' => 'إيميل من {{SITENAME}}',
'noemailtitle'    => 'مافيش  عنوان ايميل',
'noemailtext'     => 'يا إما اليوزر دا ما كتبش االايميل بتاعه صح ، أو  انه طلب ما يستلمش ايميلات من اليوزرز التانيين.',
'emailfrom'       => 'من:',
'emailto'         => 'لـ:',
'emailsubject'    => 'الموضوع:',
'emailmessage'    => 'الرساله:',
'emailsend'       => 'إبعت',
'emailccme'       => 'ابعتلى نسخة من الايميل اللى بعته.',
'emailccsubject'  => 'نسخة من رسالتك ل $1: $2',
'emailsent'       => 'الإيميل اتبعت',
'emailsenttext'   => 'الايميل بتاعك اتبعت خلاص.',
'emailuserfooter' => 'الايميل دا بعته $1 لـ $2 عن طريق خاصية "مراسلة اليوزر" فى {{SITENAME}}.',

# Watchlist
'watchlist'            => 'لستة الصفحات اللى باراقبها',
'mywatchlist'          => 'لستة  الصفحات اللى باراقبها',
'watchlistfor'         => "(ل '''$1''')",
'nowatchlist'          => 'مافيش حاجة فى لستة مراقبتك.',
'watchlistanontext'    => 'لو سمحت $1 لعرض أو تعديل الصفحات فى لستة مراقبتك.',
'watchnologin'         => 'مش متسجل',
'watchnologintext'     => 'لازم تكون [[Special:UserLogin|مسجل الدخول]] علشان تعدل لستة المراقبة بتاعتك.',
'addedwatch'           => 'تمت الاضافه للستة الصفحات اللى بتراقبها',
'addedwatchtext'       => 'تمت إضافة الصفحه  "$1"  [[Special:Watchlist|للستة الصفحات اللى بتراقبها]].
التعديلات اللى بعد كده ها تتحط على الصفحه دى، وصفحة المناقش الخاصه بها ها تتحط هناك. واسم الصفحة هايظهر  بخط <b>عريض</b> فى صفحة [[Special:RecentChanges|أحدث التعديلات]] لتسهيل تحديدها واكتشافها.',
'removedwatch'         => 'اتشالت  من لستة الصفحات اللى بتراقبها',
'removedwatchtext'     => 'الصفحه دى اتشالت "[[:$1]]" من [[Special:Watchlist|لستة الصفحات اللى بتراقبها]].',
'watch'                => 'راقب',
'watchthispage'        => 'راقب الصفحه دى',
'unwatch'              => 'بطل مراقبه',
'unwatchthispage'      => 'اتوقف عن المراقبة',
'notanarticle'         => 'دى مش صفحة بتاعة محتوى',
'notvisiblerev'        => 'النسحة اتمسحت',
'watchnochange'        => 'مافيش ولا صفحة اتعدلت فى لستة مراقبتك فى الفترة الزمنية اللى حددتها.',
'watchlist-details'    => '{{PLURAL:$1|$1 صفحه|$1 صفحه}} فى قايمه مراقبتك، بدون عد صفحات المناقشه.',
'wlheader-enotif'      => '*خاصية الاعلام بالايميل متفعلة',
'wlheader-showupdated' => "* الصفحات اللى اتغيرت  بعد زيارتك ليها اخر مرة معروضة بالخط '''العريض'''",
'watchmethod-recent'   => 'التشييك على التعديلات الاخيرة للصفحات المتراقبة',
'watchmethod-list'     => 'التشييك فى الصفحات المتراقبة على التعديلات الاخيرة',
'watchlistcontains'    => 'لستة المراقبة بتاعتك فيها $1 {{PLURAL:$1|صفحة|صفحات}}.',
'iteminvalidname'      => "مشكلة فى '$1'، اسم مش صحيح...",
'wlnote'               => "تحت فى {{PLURAL:$1|آخر تغيير|آخر '''$1''' تغيير}} فى آخر {{PLURAL:$2|ساعة|'''$2''' ساعة}}.",
'wlshowlast'           => 'عرض اخر $1 ساعات $2 ايام $3',
'watchlist-show-bots'  => 'عرض تعديلات البوتات',
'watchlist-hide-bots'  => 'تخبية تعديلات البوت',
'watchlist-show-own'   => 'عرض تعديلاتى',
'watchlist-hide-own'   => 'اخفاء تعديلاتى',
'watchlist-show-minor' => 'عرض التعديلات الطفيفة',
'watchlist-hide-minor' => 'خبى التعديلات الصغيره',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'راقب...',
'unwatching' => 'بطل مراقبه...',

'enotif_mailer'                => 'نظام {{SITENAME}} البريدى للإخطارات',
'enotif_reset'                 => 'علم على كل الصفحات كأنك خلاص زرتها',
'enotif_newpagetext'           => 'دى صفحه جديده.',
'enotif_impersonal_salutation' => 'يوزر {{SITENAME}}',
'changed'                      => 'اتغيرت',
'created'                      => 'إتنشأت',
'enotif_subject'               => 'صفحة {{SITENAME}} $PAGETITLE تم $CHANGEDORCREATED بواسطة $PAGEEDITOR',
'enotif_lastvisited'           => 'شوف $1 لمراجعة كل التغييرات اللى حصلت من أخر زيارة ليك.',
'enotif_lastdiff'              => 'شوف $1 علشان تبص على التغيير دا.',
'enotif_anon_editor'           => 'يوزر مش معروف $1',
'enotif_body'                  => 'عزيزى $WATCHINGUSERNAME،

صفحة $PAGETITLE فى {{SITENAME}} إتغيرت ل $CHANGEDORCREATED قبل كدا $PAGEEDITOR،  علشان تشوف النسخة الحالية من الصفحة بص: $PAGETITLE_URL.

$NEWPAGE

ملخص تعديل المحرر: $PAGESUMMARY $PAGEMINOREDIT

اتصل بالمحرر:
إيميل: $PAGEEDITOR_EMAIL
صفحة اليوزر: $PAGEEDITOR_WIKI

مش ح يكون فى اى إخطارات تانية عن أى تغييرات ح تحصل فى المستقبل إلا إذا زرت  الصفحه دى. ممكن أنك تعيد تصفير عداد الإخطارات لكل الصفحات اللى بتراقبها فى لستة مراقبتك.

نظام إخطار {{SITENAME}}

--
لتغيير إعدادات قايمة مراقبتك، روح على:
{{fullurl:{{ns:special}}:Watchlist/edit}}

للاقتراحات والحصول على مساعدة إضافية:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'امسح الصفحه',
'confirm'                     => 'أكد',
'excontent'                   => "المحتوى كان: '$1'",
'excontentauthor'             => "المحتوى كان: '$1' (والمساهم الوحيد كان '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "المحتوى قبل التفضيه كان: '$1'",
'exblank'                     => 'الصفحه كانت فاضيه',
'delete-confirm'              => 'مسح"$1"',
'delete-legend'               => 'مسح',
'historywarning'              => 'تحذير: الصفحه اللى ها  تمسحها ليها تاريخ:',
'confirmdeletetext'           => 'انت على وشك انك تمسح صفحه أو صوره و كل تاريخها.
من فضلك  اتأكد انك عايز المسح وبأنك فاهم نتايج  العمليه  دى. عمليات الحذف لازم تتم بناء على [[{{MediaWiki:Policy-url}}|القواعد المتفق عليها]].',
'actioncomplete'              => 'العمليه خلصت',
'deletedtext'                 => '"<nowiki>$1</nowiki>" اتمسحت.
بص على $2 لسجل آخر عمليات المسح.',
'deletedarticle'              => 'اتمسحت "[[$1]]"',
'suppressedarticle'           => 'خببى "[[$1]]"',
'dellogpage'                  => 'سجل المسح',
'dellogpagetext'              => 'لسته بأحدث عمليات المسح.',
'deletionlog'                 => 'سجل المسح',
'reverted'                    => 'استرجع لنسخة أقدم',
'deletecomment'               => 'سبب المسح:',
'deleteotherreason'           => 'سبب تانى/اضافي:',
'deletereasonotherlist'       => 'سبب تانى',
'deletereason-dropdown'       => '*أسباب المسح المشهورة
** طلب المؤلف
** التعدى على حقوق النشر
** التخريب',
'delete-edit-reasonlist'      => 'عدل اسباب المسح',
'delete-toobig'               => 'الصفحه دى  ليها تاريخ تعديل كبير، أكتر من $1 {{PLURAL:$1|مراجعة|مراجعة}}.
مسح الصفحات اللى زى دى تم تحديده لمنع الاضطراب العرضى فى {{SITENAME}}.',
'delete-warning-toobig'       => 'الصفحة دى ليها تاريخ تعديل كبير، أكتر من $1 {{PLURAL:$1|مراجعة|مراجعة}}.
ممكن مسحها يعمل اضطراب  فى عمليات قاعدة البيانات فى {{SITENAME}}؛
استمر بس خد بالك.',
'rollback'                    => 'إرجع فى التعديلات',
'rollback_short'              => 'إرجع لـ ورا',
'rollbacklink'                => 'استعاده',
'rollbackfailed'              => 'الترجيع ما نفعش',
'cantrollback'                => 'ماقدرناش نرجع فى التعديل؛ آخر مساهم هوه الوحيد اللى ساهم فى الصفحة دي.',
'alreadyrolled'               => 'ماقدرناش نرجع آخر تعديل للصفحة [[$1]] [[User:$2|$2]] ([[User talk:$2|نقاش]]) لأن فى واحد عمل تعديل او ترجيع للصفحة دى قبل كده.

آخر واحد  عدل الصفحة دى هوه [[User:$3|$3]] ([[User talk:$3|نقاش]]).',
'editcomment'                 => 'تعليق التعديل كان: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'استرجع التعديلات بواسطة [[Special:Contributions/$2|$2]] ([[User talk:$2|مناقشة]]) لآخر نسخة بواسطة [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'استرجع تعديلات $1؛
استرجع لآخر نسخة بواسطة $2.',
'sessionfailure'              => 'الظاهر انه فى مشكلة فى جلسة دخولك دى ؛
وعلشان كدا العملية دى اتلغت كإجراء احترازى ضد الاختراق.
لو سمحت دوس على زرار"رجوع" علشان تحمل الصفحة اللى جيت منها مرة تانية، و بعدين حاول تاني.',
'protectlogpage'              => 'سجل الحمايه',
'protectlogtext'              => 'تحت فى لستة بالصفحات اللى اعملها حماية او اتشالت منها الحماية.
شوف [[Special:ProtectedPages|لستة الصفحات المحمية]] لستة بعمليات حماية الصفحات الشغالة دلوقتي.',
'protectedarticle'            => 'حمى "[[$1]]"',
'modifiedarticleprotection'   => 'غير مستوى الحماية ل"[[$1]]"',
'unprotectedarticle'          => 'شال حماية [[$1]]',
'protect-title'               => 'غير مستوى الحماية ل"$1"',
'protect-legend'              => 'تأكيد الحماية',
'protectcomment'              => 'تعليق:',
'protectexpiry'               => 'تنتهى فى:',
'protect_expiry_invalid'      => 'وقت الانتهاء مش صحيح.',
'protect_expiry_old'          => 'وقت انتهاء المنع قديم.',
'protect-unchain'             => 'استعادة سماح النقل',
'protect-text'                => 'ممكن هنا تعرض و تغير مستوى الحمايه للصفحه <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'انت مش ممكن تغير مستويات الحماية وأنت ممنوع.
الإعدادات بتاعة الصفحة <strong>$1</strong> دلوقتى هي:',
'protect-locked-dblock'       => 'ما ينفعش تغير مستويات الحماية بسبب قفل قاعدة البيانات دلوقتي.
الإعدادات بتاعة الصفحة <strong>$1</strong> دلوقتى هي:',
'protect-locked-access'       => 'حسابك ما لوش  صلاحية تغيير مستوى حماية الصفحه.
الاعدادات الحالية للصفحه <strong>$1</strong> هى:',
'protect-cascadeon'           => 'الصفحه دى محميه لكونها متضمنه فى {{PLURAL:$1|الصفحه|الصفحات}} دى، واللى  فيها اختيار حماية الصفحات المتضمنه شغال.
ممكن تغير مستوى حماية الصفحه دى بدون التأثير على حماية الصفحات المتضمنه التانيه.',
'protect-default'             => '(افتراضى)',
'protect-fallback'            => 'محتاج  اذن "$1"',
'protect-level-autoconfirmed' => 'منع الوزرز غير المسجلين',
'protect-level-sysop'         => 'سيسوب بس',
'protect-summary-cascade'     => 'متضمنه',
'protect-expiring'            => 'تنتهى فى $1 (UTC)',
'protect-cascade'             => 'احمى الصفحات المتضمنه فى الصفحه دى (حمايه مضمنه)',
'protect-cantedit'            => 'مش ممكن تغير مستويات الحمايه للصفحه دى، لانك ماعندكش صلاحية تعديلها.',
'restriction-type'            => 'سماح:',
'restriction-level'           => 'مستوى القيود :',
'minimum-size'                => 'أقل حجم',
'maximum-size'                => 'أكبر حجم:',
'pagesize'                    => '(بايت)',

# Restrictions (nouns)
'restriction-edit'   => 'تعديل',
'restriction-move'   => 'انقل',
'restriction-create' => 'اعمل',
'restriction-upload' => 'تحميل',

# Restriction levels
'restriction-level-sysop'         => 'محمية بالكامل',
'restriction-level-autoconfirmed' => 'نص محمية',
'restriction-level-all'           => 'أى مستوى',

# Undelete
'undelete'                     => 'عرض الصفحات الممسوحة',
'undeletepage'                 => 'عرض واسترجاع الصفحات المسوحة',
'undeletepagetitle'            => "'''دا بيتكون من النسخ الممسوحة لـ[[:$1]]'''.",
'viewdeletedpage'              => 'عرض الصفحات الممسوحة',
'undeletepagetext'             => 'الصفحات دى إتمسحت بس لسة موجودة فى الأرشيف و ممكن تترجع.
الأرشيف ممكن يتنضف كل شوية.',
'undelete-fieldset-title'      => 'رجع النسخ',
'undeleteextrahelp'            => "علشان ترجع تاريخ الصفحة كله، سيب كل الصناديق فاضية و دوس '''''ترجيع'''''.
علشان ترجع جزء من الصفحة، حط علامة فى الصناديق أدام التعديلات اللى عايز  ترجعهاو دوس '''''ترجيع'''''.
لو دوست على  '''''إبتدى تاني'''''  التعليق ح يتمسح و كل العلامات  اللى فى الصناديق ح تتحذف.",
'undeleterevisions'            => '$1 {{PLURAL:$1|نسخة|نسخة}} اتحطت فى  الارشيف',
'undeletehistory'              => 'لو رجعت الصفحة، كل المراجعات ح تترجع للتاريخ دا
لو فى صفحة جديدة اتعملت بنفس الاسم بعد المسح، المراجعات المترجعة ح تبان فى التاريخ اللى فات.',
'undeleterevdel'               => 'الترجيع مش ح يحصل لو كان ح يسبب ان المراجعة تيجى فى راس الصفحة أو ان الملف يتمسح حتة منه .
فى الحالات اللى زى كدا، لازم تبين أخر المراجعات الممسوحة.',
'undeletehistorynoadmin'       => 'الصفحة دى اتمسحت.
سبب المسح موجود فى الملخص اللى تحت، كمان فى تفاصيل اليوزرز اللى عملو تعديل على الصفحة دى قبل ما تتمسح.
نص المراجعات الممسوحة دى متوفرة بس للاداريين.',
'undelete-revision'            => 'المراجعة الممسوحة ل$1 (بتاريخ $2) عن طريق $3:',
'undeleterevision-missing'     => 'مراجعة مش صحيحة أو ضايعة.
يمكن اللينك بتاعتك بايظة، أو يمكن المراجعة اترجعت او اتشالت من الارشيف.',
'undelete-nodiff'              => 'ما لقيناش نسخة قديمة.',
'undeletebtn'                  => 'استعاده',
'undeletelink'                 => 'استرجاع',
'undeletereset'                => 'ابتدى من الأول',
'undeletecomment'              => 'تعليق:',
'undeletedarticle'             => 'رجع  "[[$1]]" تاني',
'undeletedrevisions'           => 'رجعنا تانى {{PLURAL:$1|تعديل واحد|تعديلين|$1 تعديلات|$1 تعديل|$1 تعديل}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|1 نسخة|$1 نسخة}} و {{PLURAL:$2|1 ملف|$2 ملف}} رجعو تاني',
'undeletedfiles'               => '{{PLURAL:$1|ملف|ملفات}} $1 رجعو تاني',
'cannotundelete'               => 'الترجيع ما نفعش :ممكن يكون فى حد تانى رجع الصفحة قبل كدا.',
'undeletedpage'                => "<big>'''اترجع $1'''</big>

بص على [[Special:Log/delete|سجل المسح]] علشان تشوف عمليات المسح و الترجيع الاخيرة.",
'undelete-header'              => 'شوف الصفحات الممسوحة قريب فى [[Special:Log/delete|سجل المسح]].',
'undelete-search-box'          => 'دور فى الصفحات الممسوحة',
'undelete-search-prefix'       => 'عرض الصفحات اللى بتبتدى بـ:',
'undelete-search-submit'       => 'دور',
'undelete-no-results'          => 'مالقيناش صفحات مطابقة فى أرشيف المسح.',
'undelete-filename-mismatch'   => 'ماقدرناش نرجع المراجعة بتاعة الملف بتاريخ $1: اسم الملف مش مطابق',
'undelete-bad-store-key'       => 'ما قدرناش نرجع المراجعة بتاعة الملف بتاريخ $1: الملف كان ضايع قبل المسح',
'undelete-cleanup-error'       => 'خطأ مسح ملف أرشيف مش بيستعمل"$1".',
'undelete-missing-filearchive' => 'مش قادرين نرجع ملف الأرشيف رقم $1 لأنه مش موجود فى قاعدة البيانات.
يمكن يكون اترجع قبل كدا.',
'undelete-error-short'         => 'غلطة ترجيع ملف: $1',
'undelete-error-long'          => 'حصلت غلطات و الملف بيترجع:

$1',

# Namespace form on various pages
'namespace'      => 'النيمسبيس:',
'invert'         => 'عكس الاختيار',
'blanknamespace' => '(رئيسى)',

# Contributions
'contributions' => 'مساهمات اليوزر',
'mycontris'     => 'تعديلاتى',
'contribsub2'   => 'لليوزر $1 ($2)',
'nocontribs'    => 'مالقيناش   تغييرات تطابق المحددات دي.',
'uctop'         => '(فوق)',
'month'         => 'من شهر (واللى قبل كده):',
'year'          => 'من سنة (واللى قبل كده):',

'sp-contributions-newbies'     => 'عرض مساهمات الحسابات الجديدة بس',
'sp-contributions-newbies-sub' => 'للحسابات الجديده',
'sp-contributions-blocklog'    => 'سجل المنع',
'sp-contributions-search'      => 'دور على مساهمات',
'sp-contributions-username'    => 'عنوان أيبى أو اسم يوزر:',
'sp-contributions-submit'      => 'تدوير',

# What links here
'whatlinkshere'            => 'ايه بيوصل هنا',
'whatlinkshere-title'      => 'الصفحات اللى بتودى لـ "$1"',
'whatlinkshere-page'       => 'الصفحة:',
'whatlinkshere-barrow'     => '>',
'linklistsub'              => '(لسته بالوصلات)',
'linkshere'                => "الصفحات دى فيها وصله ل '''[[:$1]]''':",
'nolinkshere'              => "مافيش صفحات بتوصل ل '''[[:$1]]'''.",
'nolinkshere-ns'           => "مافيش صفحات بتوصل لـ '''[[:$1]]''' فى النطاق اللى انت اختارته.",
'isredirect'               => 'صفحة تحويل',
'istemplate'               => 'متضمن',
'isimage'                  => 'لينك صورة',
'whatlinkshere-prev'       => '{{PLURAL:$1|اللى قبل كده|الـ $1 اللى قبل كده}}',
'whatlinkshere-next'       => '{{PLURAL:$1|اللى بعد كده|الـ $1 اللى بعد كده}}',
'whatlinkshere-links'      => '← وصلات',
'whatlinkshere-hideredirs' => '$1 التحويلات',
'whatlinkshere-hidetrans'  => '$1 التضمينات',
'whatlinkshere-hidelinks'  => '$1 لينكات',
'whatlinkshere-hideimages' => '$1 وصلة صورة',
'whatlinkshere-filters'    => 'فلاتر',

# Block/unblock
'blockip'                         => 'منع يوزر',
'blockip-legend'                  => 'منع اليوزر',
'blockiptext'                     => 'استخدم الاستمارة اللى تحت لمنع عنوان أيبى أو يوزر معين من الكتابة.
دا لازم يحصل بس علشان تمنع التخريب ،و على حسب 
[[{{MediaWiki:Policy-url}}|السياسة]].
اكتب سبب محدد تحت (يعنى مثلا، اكتب الصفحات المعينة اللى اتخربت بسببه).',
'ipaddress'                       => 'عنوان الأيبي:',
'ipadressorusername'              => 'عنوان الأيبى أو اسم اليوزر:',
'ipbexpiry'                       => 'مدة المنع:',
'ipbreason'                       => 'السبب:',
'ipbreasonotherlist'              => 'سبب تاني',
'ipbreason-dropdown'              => '*أسباب المنع المشهورة
** تدخيل معلومات غلط
** مسح المحتوى من الصفحات
** سبام لينك لمواقع خارجية
** كتابة كلام مالوش معنى فى الصفحات
** سلوك عدواني/تحرش
** إساءة استخدام اكتر من حسابات 
** اسم يوزر مش مقبول',
'ipbanononly'                     => 'امنع اليوزرز المجهولين بس',
'ipbcreateaccount'                => 'امنع فتح الحسابات',
'ipbemailban'                     => 'منع اليوزر ده من بعتان إيميل',
'ipbenableautoblock'              => ' امنع آخر عنوان أيبى استخدمه اليوزر دا اوتوماتيكي، وأى عناوين أيبى تانية يحاول التحرير منها',
'ipbsubmit'                       => 'منع اليوزر دا',
'ipbother'                        => 'وقت تاني:',
'ipboptions'                      => 'ربع ساعة:15 minutes,ساعة واحدة:1 hour,ساعتين:2 hours,يوم:1 day,تلات-ت-أيام:3 days,أسبوع:1 week,أسبوعين:2 weeks,شهر:1 month,تلات شهور:3 months,ست-ت- شهور:6 months,سنة واحدة:1 year,على طول:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'كمان',
'ipbotherreason'                  => 'سبب تاني:',
'ipbhidename'                     => 'خبى اسم اليوزر من سجل المنع، لستة المنع النشطة ولستة اليوزرز',
'ipbwatchuser'                    => 'راقب صفحات اليوزر و النقاش بتوع اليوزر دا',
'badipaddress'                    => 'عنوان أيبى مش صحيح',
'blockipsuccesssub'               => 'المنع حصل بنجاح',
'blockipsuccesstext'              => 'اتمنع [[Special:Contributions/$1|$1]].<br />
شوف [[Special:IPBlockList|لستة منع الأيبي]] علشان تراجع حالات المنع.',
'ipb-edit-dropdown'               => 'عدل أسباب المنع',
'ipb-unblock-addr'                => 'رفع منع $1',
'ipb-unblock'                     => 'رفع المنع عن يوزر أو عنوان أيبي',
'ipb-blocklist-addr'              => 'عرض المنع الموجود ل$1',
'ipb-blocklist'                   => 'عرض حالات المنع الموجودة دلوقتي',
'unblockip'                       => 'رفع منع يوزر',
'unblockiptext'                   => 'استخدم الاستمارة اللى تحت علشان ترجع حق الكتابة بتاعة عنوان أيبى أو يوزر اتسحب منه الحق دا قبل كدا.',
'ipusubmit'                       => 'رفع منع  العنوان ده',
'unblocked'                       => 'المنع اترفع عن [[User:$1|$1]]',
'unblocked-id'                    => 'منع $1 اترفع',
'ipblocklist'                     => 'لستة عناوين الااى بى واسامى اليوزر الممنوعه',
'ipblocklist-legend'              => 'دور على مستخدم ممنوع',
'ipblocklist-username'            => 'اسم اليوزر او عنوان ال اى بي.',
'ipblocklist-submit'              => 'تدوير',
'blocklistline'                   => '$1, $2 منع $3 ($4)',
'infiniteblock'                   => 'دايم',
'expiringblock'                   => 'ينتهى فى $1',
'anononlyblock'                   => 'مجهول بس',
'noautoblockblock'                => 'المنع التلقائى متعطل',
'createaccountblock'              => ' فتح الحسابات ممنوع',
'emailblock'                      => 'الإيميل ممنوع',
'ipblocklist-empty'               => 'لستة المنع فاضية.',
'ipblocklist-no-results'          => 'عنوان الأيبى أو اسم اليوزر المطلوب مش ممنوع.',
'blocklink'                       => 'منع',
'unblocklink'                     => 'رفع المنع',
'contribslink'                    => 'تعديلات',
'autoblocker'                     => 'انت اتمنعت اوتوماتيكى لأن الأيبى بتاعك استعمله "[[User:$1|$1]]" فى الفترة الاخيرة.
السبب اللى خلا $1 يتمنع هو: "$2"',
'blocklogpage'                    => 'سجل المنع',
'blocklogentry'                   => 'منع "[[$1]]" لفترةه زمنيه مدتها $2 $3',
'blocklogtext'                    => 'دا سجل بعمليات المنع ورفع المنع.
عناوين الأيبى اللى اتمنعت اوتوماتيكى مش معروضة.
شوف [[Special:IPBlockList|عناوين الأيبى الممنوعة]] علشان تشوف عمليات المنع الشغالة دلوقتي.',
'unblocklogentry'                 => 'رفع منع $1',
'block-log-flags-anononly'        => 'اليوزرز المجهولين  بس',
'block-log-flags-nocreate'        => ' فتح الحسابات ممنوع',
'block-log-flags-noautoblock'     => 'المنع التلقائى متعطل',
'block-log-flags-noemail'         => 'الإيميل ممنوع',
'block-log-flags-angry-autoblock' => 'المنع الاوتوماتيكى المتقدم متفعل',
'range_block_disabled'            => 'إمكانيةالسيسوب لمنع نطاق متعطلة.',
'ipb_expiry_invalid'              => 'تاريخ الانتهاء مش صحيح.',
'ipb_expiry_temp'                 => 'عمليات منع أسماء اليوزرز المستخبية لازم تكون على طول.',
'ipb_already_blocked'             => '"$1" ممنوع فعلا',
'ipb_cant_unblock'                => 'غلطه: عنوان الااى بى الممنوع  مش موجود  $1.
يمكن اترفع منعه فعلا.',
'ipb_blocked_as_range'            => 'غلط: الأيبى $1 مش ممنوع مباشرةو مش ممكن رفع المنع عنه.
بس هو، على الرغم من كدا،ممنوع لانه جزء من النطاق $2، و اللى ممكن رفع المنع عنه.',
'ip_range_invalid'                => 'نطاق عناوين الأيبى مش صحيح.',
'blockme'                         => 'امنعنى',
'proxyblocker'                    => 'مانع البروكسي',
'proxyblocker-disabled'           => 'الخاصية دى متعطلة.',
'proxyblockreason'                => 'عنوان الأيبى بتاعك اتمنع لانه بروكسى مفتوح.
لو سمحت تتصل بمزود خدمة الإنترنت بتاعك أو الدعم الفنى و قولهم على المشكلة الامنية الخطيرة دي.',
'proxyblocksuccess'               => 'خلاص.',
'sorbs'                           => 'دى إن إس بى إل',
'sorbsreason'                     => 'عنوان الأيبى بتاعك موجود كبروكسى مفتوح فى DNSBL اللى بيستعمله{{SITENAME}}.',
'sorbs_create_account_reason'     => 'عنوان الأيبى بتاعك موجود كبروكسى مفتوح فى ال DNSBL اللى بيستعمله {{SITENAME}}. 
ما ينفعش تفتح حساب.',

# Developer tools
'lockdb'              => 'اقفل قاعدة البيانات',
'unlockdb'            => 'افتح قاعدة البيانات',
'lockdbtext'          => 'قفل قاعدة البيانات ح يمنع كل اليوزرز من تحرير الصفحات وتغيير التفضيلات بتاعتهم وتعديل لستة المراقبة حاجات تانية بتحتاج تغيير قاعدة البيانات.
لو سمحت تتأكد من  ان هو دا اللى انت عايز تعمله فعلا، ومن إنك ح تشيل القفل بعد ما تخلص الصيانة.',
'unlockdbtext'        => 'فتح قاعدة البيانات ح يخلى كل اليوزرز يقدرو يحررو الصفحات، يغيرو  تفضيلاتهم،يعدلو لستة المراقبة  بتاعتهم، و حاجات تانية محتاجين يغيروها فى قاعدة البانات.
. لو سمحت تتاكد ان هو دا اللى انت عايز تعمله',
'lockconfirm'         => 'أيوه، أنا فعلا عايز اقفل قاعدة البيانات.',
'unlockconfirm'       => 'أيوه، أنا فعلا عايز افتح قاعدة البيانات.',
'lockbtn'             => 'قفل قاعدة البيانات',
'unlockbtn'           => 'افتح قاعدة البيانات',
'locknoconfirm'       => 'انت ما علمتش على صندوق التأكيد.',
'lockdbsuccesssub'    => 'نجح قفل قاعدة البيانات',
'unlockdbsuccesssub'  => 'قفل قاعدة البيانات إتشال.',
'lockdbsuccesstext'   => 'قاعدة البانات اتقفلت خلاص.<br />
ماتنساش [[Special:UnlockDB|تشيل القفل]] بعد أعمال الصيانة ما تخلص .',
'unlockdbsuccesstext' => 'قاعدة البيانات إتفتحت تانى',
'lockfilenotwritable' => 'ملف قفل قاعدة البيانات مش ممكن يتكتب عليه.
علشان تقفل قاعدة البيانات أو تشيل القفل لازم سيرفر الويب يسمح بالكتابة على الملف دا .',
'databasenotlocked'   => 'قاعدة البيانات بتاعتك مش  مقفولة.',

# Move page
'move-page'               => 'انقل $1',
'move-page-legend'        => 'انقل الصفحة',
'movepagetext'            => "لو استعملت النموذج ده ممكن تغير اسم الصفحه، و تنقل تاريخها للاسم الجديد.
هاتبتدى تحويله من العنوان القديم للصفحه بالعنوان الجديد.
لكن،  الوصلات فى الصفحات اللى بتتوصل بالصفحه دى مش ها تتغير؛ اتأكد من ان مافيش  [[Special:BrokenRedirects|وصلات مقطوعه]] ، أو [[Special:DoubleRedirects|وصلات متتاليه]] ، للتأكد من أن المقالات تتصل مع بعضها بشكل مناسب.

لاحظ ان الصفحه مش هاتتنقل لو كان فيه صفحه بالاسم الجديد، إلا إذا كانت صفحة فاضيه، أو صفحة تحويل، ومالهاش تاريخ. و ده معناه أنك مش ها تقدر تحط صفحه مكان صفحه، كمان ممكن ارجاع الصفحه لمكانها فى حال تم النقل بشكل غلط.

'''تحذير!'''
نقل الصفحه ممكن يكون له اثار كبيرة، وتغييرات مش متوقعه بالنسبة للصفحات المشهوره. من فضلك  اتأكد من فهم عواقب نقل الصفحات قبل ما تقوم بنقل الصفحه.",
'movepagetalktext'        => "صفحة المناقشه بتاعة المقاله هاتتنقل برضه، لو كانت موجوده. لكن صفحة المناقشه '''مش''' هاتتنقل فى الحالات دى:
* نقل الصفحة عبر نطاقات  مختلفه.
*فيه  صفحة مناقشه موجوده تحت العنوان الجديد للمقاله.
* لو انت شلت اختيار نقل صفحة المناقشه .

وفى الحالات  دى، لو عايز  تنقل صفحة المناقشه  لازم تنقل أو تدمج محتوياتها  يدويا.",
'movearticle'             => 'انقل الصفحه:',
'movenotallowed'          => 'ماعندكش الصلاحية لنقل الصفحات.',
'newtitle'                => 'للعنوان الجديد:',
'move-watch'              => 'راقب الصفحه دى',
'movepagebtn'             => 'نقل الصفحه',
'pagemovedsub'            => 'تم  النقل بنجاح',
'movepage-moved'          => '<big>\'\'\'"$1" اتنقلت ل"$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'يا اما فيه صفحه  بالاسم ده، او ان الاسم اللى  تم اختياره مش صالح.
لو سمحت اختار اسم تانى.',
'cantmove-titleprotected' => 'ما ينفعش تنقل صفحة للمكان دا،لأن العنوان الجديد محمى ضد الانشاء',
'talkexists'              => "'''الصفحه دى اتنقلت لصفحة بنجاح، ولكن صفحة المناقشه بتاعتها ما اتنقلتش  علشان فيه صفحة مناقشه تحت العنوان الجديد.
من فضلك انقل محتويات صفحة المناقشه يدويا، وادمجها مع المحتويات اللى قبل كده.'''",
'movedto'                 => 'اتنقلت ل',
'movetalk'                => 'انقل صفحة المناقشه.',
'move-subpages'           => 'انقل كل الصفحات الفرعية، إن امكن',
'move-talk-subpages'      => 'انقل كل الصفحات الفرعية لصفحة النقاش، لو ممكن',
'movepage-page-exists'    => 'الصفحة $1 موجودة فعلا ومش ممكن الكتابة عليها اوتوماتيكي..',
'movepage-page-moved'     => 'الصفحة $1 اتنقلت لـ $2.',
'movepage-page-unmoved'   => 'ماقدرناش ننقل الصفحة $1 لـ $2.',
'movepage-max-pages'      => 'الحد الأقصى $1 {{PLURAL:$1|صفحة|صفحة}} اتنقل. و مافيش حاجة تانى ح تتنقل اوتوماتيكي.',
'1movedto2'               => '[[$1]] اتنقلت ل [[$2]]',
'1movedto2_redir'         => '[[$1]] انقلت لـ[[$2]] فوق التحويلة',
'movelogpage'             => 'سجل النقل',
'movelogpagetext'         => 'تحت فى لستة الصفحات اللى اتنقلت.',
'movereason'              => 'السبب:',
'revertmove'              => 'استعاده',
'delete_and_move'         => 'مسح ونقل',
'delete_and_move_text'    => '==المسح مطلوب==
الصفحة الهدف "[[:$1]]" موجودة فعلا.
انت عايز تمسحها علشان تقدر تنقلها؟',
'delete_and_move_confirm' => 'ايوة، امسح الصفحة',
'delete_and_move_reason'  => 'اتمسحت علشان تسمح للنقل',
'selfmove'                => 'عنوان المصدر والهدف هو نفسه؛
مش ممكن نقل الصفحة على نفسها.',
'immobile_namespace'      => 'عنوان المصدر أو الهدف ليه طبيعة خاصة؛
مش ممكن تنقل الصفحات من و للنطاق دا.',
'imagenocrossnamespace'   => 'مش ممكن تنقل الملف لنطاق غير نطاق الملفات',
'imagetypemismatch'       => 'امتداد الملف الجديد مش ماشى مع نوعه',
'imageinvalidfilename'    => 'اسم الملف الهدف مش صحيح',
'fix-double-redirects'    => 'اعمل تحديث لاى تحويلات بتشاور على العنوان الاصلي',

# Export
'export'            => 'تصدير صفحات',
'exporttext'        => 'انت ممكن تصدر النص وتاريخ تعديلات صفحة معينة أو مجموعة صفحات فى صيغة إكس إم إل. لو قصدكو بكدا ممكن استيرادها فى ويكى تانى بيستعمل ميدياويكى عن طريق الصفحة [[Special:Import|صفحة الاستيراد]].

علشان تصدر الصفحات، اكتب العناوين فى الصندوق اللى تحت، عنوان واحد فى كل السطر، و اختار اذا كنت عايز  النسخة الحالية بالإضافة  للنسخ القديمة كاملة أو مع معلومات تاريخ الصفحة عنها ولا بس النسخة الحالية مع معلومات عن التعديل الأخير.

فى الحالة التانية ممكن تستخدم لينك مباشرة، مثلا [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] للصفحة [[{{MediaWiki:Mainpage}}]].',
'exportcuronly'     => 'ضمن المراجعة دى بس، ومش التاريخ الكامل',
'exportnohistory'   => "----
ملاحظة:''' التصدير الكامل لتاريخ الصفحة  بالطريقة دى مش شغال بسبب الاداء'''",
'export-submit'     => 'تصدير',
'export-addcattext' => 'ضيف صفحات من تصنيف:',
'export-addcat'     => 'زيادة',
'export-download'   => 'احفظ كملف',
'export-templates'  => 'دخل القوالب',

# Namespace 8 related
'allmessages'               => 'رسايل النظام',
'allmessagesname'           => 'اسم',
'allmessagesdefault'        => 'النص الاوتوماتيكي',
'allmessagescurrent'        => 'النص دلوقتى',
'allmessagestext'           => 'دى لستة برسايل النظام المتوفرة فى نطاق ميدياويكي.
لو سمحت تزور[http://www.mediawiki.org/wiki/Localisation ترجمة ميدياويكي] و [http://translatewiki.net بيتاويكي] لو كنت عايز تساهم فى ترجمة ميدياويكى الاصلية.',
'allmessagesnotsupportedDB' => "الصفحة دى مش يمكن حد يستعملها علشان'''\$wgUseDatabaseMessages''' متعطل.",
'allmessagesfilter'         => 'فلتر اسم الرسالة:',
'allmessagesmodified'       => 'اعرض اللى اتعدل بس',

# Thumbnails
'thumbnail-more'           => 'كبر',
'filemissing'              => 'الملف ضايع',
'thumbnail_error'          => 'غلطه فى انشاء صوره مصغره: $1',
'djvu_page_error'          => 'صفحة DjVu بره النطاق',
'djvu_no_xml'              => 'مش ممكن تجيب XML لملف DjVu',
'thumbnail_invalid_params' => 'محددات التصغير مش صحيحة',
'thumbnail_dest_directory' => 'مش قادر ينشئ المجلد الهدف',

# Special:Import
'import'                     => 'استيراد صفحات',
'importinterwiki'            => 'استيراد ترانسويكي',
'import-interwiki-text'      => 'اختار الويكى و عنوان الصفحة اللى عاوز تستوردها.
تواريخ التعديلات و اسامى المحررين  ح يتحافظ عليها.
كل عمليات الاستيراد للترانسويكى بتتسجل فى [[Special:Log/import|سجل الاستيراد]].',
'import-interwiki-history'   => 'انسخ كل نسخ التاريخ للصفحة دي',
'import-interwiki-submit'    => 'استيراد',
'import-interwiki-namespace' => 'انقل الصفحات للنطاق:',
'importtext'                 => 'لو سمحت تصدّر الملف من الويكى المصدر عن طريق [[Special:Export|خاصية التصدير]].
احفظه على جهازك و بعدين حمله هنا.',
'importstart'                => 'استيراد صفحات...',
'import-revision-count'      => '{{PLURAL:$1|نسخة واحدة|نسخة}} $1',
'importnopages'              => 'مافيش صفحات للاستيراد',
'importfailed'               => 'فشل استيراد: $1',
'importunknownsource'        => 'نوع مصدر الاستيراد مش معروف',
'importcantopen'             => 'ماقدرناش نفتح ملف الاستيراد',
'importbadinterwiki'         => 'اللينك بتاعة الانترويكى دى غلط',
'importnotext'               => 'فاضى او مافيش نص',
'importsuccess'              => 'الاستيراد خلص!',
'importhistoryconflict'      => 'فى تاريخ تعديلات متعارض مع بعضه(يمكن الصفحة دى تكون استوردت قبل كدا)',
'importnosources'            => ' مصادر استيراد الترانسويكى ما تحددتش  و الاستيراد المباشر عن طريق التحميل مش شغال.',
'importnofile'               => 'ملف الاستيراد ما تحملش.',
'importuploaderrorsize'      => 'تحميل ملف الاستيراد فشل.
الملف أكبر من حجم التحميل المسموح.',
'importuploaderrorpartial'   => 'تحميل ملف الاستيراد فشل.
جزء من الملف بس اتحمل.',
'importuploaderrortemp'      => 'تحميل ملف الاستيراد فشل.
فى مجلد مؤقت ضايع.',
'import-parse-failure'       => 'فشل بارس استيراد XML',
'import-noarticle'           => 'مافيش صفحة للاستيراد!',
'import-nonewrevisions'      => 'كل النسخ استوردت قبل كدا.',
'xml-error-string'           => '$1 عند السطر $2، العمود $3 (بايت $4): $5',
'import-upload'              => 'حمل بيانات إكس إم إل',

# Import log
'importlogpage'                    => 'سجل الاستيراد',
'importlogpagetext'                => 'استيرادات إدارية لصفحات ليها تاريخ تعديل من مواقع ويكى تانية.',
'import-logentry-upload'           => 'استورد [[$1]] بواسطة تحميل ملف',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|تعديل واحد|تعديل}}',
'import-logentry-interwiki'        => 'استيراد ويكى $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|تعديل واحد|تعديل}} من $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'صفحتى الخاصه',
'tooltip-pt-anonuserpage'         => 'صفحة اليوزر للأيبى اللى انت بتعمل منه تحرير',
'tooltip-pt-mytalk'               => 'صفحة مناقشاتى',
'tooltip-pt-anontalk'             => 'نقاش حوالين التعديلات من عنوان الأيبى دا',
'tooltip-pt-preferences'          => 'تفضيلاتى',
'tooltip-pt-watchlist'            => 'لسته بالصفحات اللى انت بتراقب التعديلات فيها',
'tooltip-pt-mycontris'            => 'لسته بتعديلاتى',
'tooltip-pt-login'                => 'من الافضل انك تسجل دخولك، لكن ده مش شرط',
'tooltip-pt-anonlogin'            => 'من الأفضل انك تسجل دخولك، لكن ده مش إجبارى.',
'tooltip-pt-logout'               => 'خروج',
'tooltip-ca-talk'                 => 'مناقشة صفحة الموضوع',
'tooltip-ca-edit'                 => 'ممكن تعدل  الصفحه دى.
بس لو سمحت استعمل زرار البروفه قبل ما تسييفها.',
'tooltip-ca-addsection'           => 'ضيف تعليق للمناقشه دى.',
'tooltip-ca-viewsource'           => 'الصفحه دى محميه.
ممكن تشوف مصدرها.',
'tooltip-ca-history'              => 'النسخ القديمة من الصفحة دي',
'tooltip-ca-protect'              => 'احمى الصفحه دى',
'tooltip-ca-delete'               => 'امسح الصفحه دى',
'tooltip-ca-undelete'             => 'رجع التعديلات اللى حصلت على الصفحة دى قبل ما تتمسح',
'tooltip-ca-move'                 => 'انقل الصفحه دى',
'tooltip-ca-watch'                => 'حط الصفحة دى فى لسته الصفحات اللى باراقب التعديلات فيها',
'tooltip-ca-unwatch'              => 'شيل الصفحه دى من لستة الصفحات اللى بتراقبها',
'tooltip-search'                  => 'دور فى {{SITENAME}}',
'tooltip-search-go'               => 'روح  لصفحة بنفس الاسم دا لو موجودة',
'tooltip-search-fulltext'         => 'دور فى الصفحات  على النص دا',
'tooltip-p-logo'                  => 'الصفحه الرئيسيه',
'tooltip-n-mainpage'              => 'زور الصفحه الرئيسيه',
'tooltip-n-portal'                => 'عن المشروع، ممكن تعمل ايه، و فين تلاقى اللى بتدور عليه',
'tooltip-n-currentevents'         => 'مطالعه سريعه لاهم الاحداث دلوقتى',
'tooltip-n-recentchanges'         => 'لسته بالتعديلات الجديده فى الويكى',
'tooltip-n-randompage'            => 'حمل صفحة عشوائيه',
'tooltip-n-help'                  => 'لو محتاج مساعده بص هنا',
'tooltip-t-whatlinkshere'         => 'صفحات الويكى اللى بتوصل هنا',
'tooltip-t-recentchangeslinked'   => 'اخر التغييرات فى الصفحات الموصولة من الصفحة دي',
'tooltip-feed-rss'                => 'تلقيم أر إس إس للصفحة دي',
'tooltip-feed-atom'               => 'تلقيم أتوم للصفحة دي',
'tooltip-t-contributions'         => 'عرض مساهمات اليوزر ده',
'tooltip-t-emailuser'             => 'ابعت ايميل لليوزر ده',
'tooltip-t-upload'                => 'حمل ملفات',
'tooltip-t-specialpages'          => 'لسته بكل الصفحات المخصوصه',
'tooltip-t-print'                 => 'نسخة للطباعة لصفحة دي',
'tooltip-t-permalink'             => 'لينك دايمة للنسخة دى من الصفحة',
'tooltip-ca-nstab-main'           => 'اعرض صفحة المحتوى',
'tooltip-ca-nstab-user'           => 'اعرض صفحة اليوزر',
'tooltip-ca-nstab-media'          => 'اعرض صفحة الميديا',
'tooltip-ca-nstab-special'        => 'الصفحة دى صفحة مخصوصة ، مش ممكن تعدل الصفحة نفسها',
'tooltip-ca-nstab-project'        => 'اعرض صفحة المشروع',
'tooltip-ca-nstab-image'          => 'اعرض صفحة الفايل',
'tooltip-ca-nstab-mediawiki'      => 'اعرض رسالة النظام',
'tooltip-ca-nstab-template'       => 'اعرض القالب',
'tooltip-ca-nstab-help'           => 'اعرض صفحة المساعده',
'tooltip-ca-nstab-category'       => 'اعرض صفحة التصنيف',
'tooltip-minoredit'               => 'علم على ده كتعديل صغير',
'tooltip-save'                    => ' سييف تعديلاتك',
'tooltip-preview'                 => 'اعرض بروفه لتعديلاتك، من فضلك شوف البروفه قبل ما تسييف!',
'tooltip-diff'                    => 'اعرض التعديلات اللى انت عملتها على النص.',
'tooltip-compareselectedversions' => 'شوف الفروق بين النسختين المختارتين للصفحه دى.',
'tooltip-watch'                   => 'ضم الصفحه دى للستة الصفحات اللى بتراقبها',
'tooltip-recreate'                => 'إنشيء الصفحة تانى مع انها اتمسحت قبل كدا',
'tooltip-upload'                  => 'ابتدى التحميل',

# Stylesheets
'common.css'      => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على كل الواجهات */',
'standard.css'    => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة ستاندرد */',
'nostalgia.css'   => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة نوستالشيا */',
'cologneblue.css' => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة كولون بلو */',
'monobook.css'    => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة مونوبوك */',
'myskin.css'      => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة ماى سكين */',
'chick.css'       => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة تشيك */',
'simple.css'      => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة سيمبل */',
'modern.css'      => '/* الأنماط المتراصة CSS المعروضة هنا ستؤثر على مستخدمى واجهة مودرن */',

# Scripts
'common.js'      => '/*  أى جافاسكريبت  هناح يتحمل لكل اليوزرز مع كل تحميل للصفحة. */',
'standard.js'    => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة ستاندرد */',
'nostalgia.js'   => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة نوستالجيا */',
'cologneblue.js' => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة كولون بلو */',
'monobook.js'    => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة مونوبوك */',
'myskin.js'      => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة ماى سكين */',
'chick.js'       => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة تشيك */',
'simple.js'      => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة سيمبل */',
'modern.js'      => '/* أى جافاسكريبت هنا ح تتحمل لليوزرز اللى بيستعملو واجهة مودرن */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadata متعطلة للسيرفر دا.',
'nocreativecommons' => 'Creative Commons RDF metadata متعطلة  للسيرفر دا.',
'notacceptable'     => 'السيرفر بتاع الويكى مش ممكن يديلك بيانات بصيغة ممكن عميلك يقراها.',

# Attribution
'anonymous'        => 'يوزر مجهول ل {{SITENAME}}',
'siteuser'         => 'يوزر {{SITENAME}} $1',
'lastmodifiedatby' => 'آخر تعديل  للصفحة دى كان فى $2، $1 عن طريق $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'بناء على عمل $1.',
'others'           => 'تانيين',
'siteusers'        => 'يوزر(ز) {{SITENAME}} $1',
'creditspage'      => 'حقوق الصفحة',
'nocredits'        => 'مافيش معلومات حقوق متوفرة للصفحة دي.',

# Spam protection
'spamprotectiontitle' => 'فلتر الحمايه من السبام',
'spamprotectiontext'  => 'السبام فيلتر منعك من إنك تحفظ الصفحة دى.
السبب يمكن علشان فى لينك لسايت خارجى فى القايمة السودة.',
'spamprotectionmatch' => 'النص دا هو اللى نشط السبام فيلتر بتاعنا: $1',
'spambot_username'    => 'تنظيف سبام ميدياويكى',
'spam_reverting'      => 'ترجيع آخر نسخة مافيهاش لينكات لـ $1',
'spam_blanking'       => 'كل النسخ فيها لينكات ل $1، فضيها',

# Info page
'infosubtitle'   => 'معلومات للصفحه',
'numedits'       => 'عدد التعديلات (صفحة): $1',
'numtalkedits'   => 'عدد التعديلات (صفحة نقاش): $1',
'numwatchers'    => 'عدد المراقبين: $1',
'numauthors'     => 'عدد المؤلفين المميزين (صفحة): $1',
'numtalkauthors' => 'عدد المؤلفين المميزين (صحفة نقاش): $1',

# Math options
'mw_math_png'    => 'دايما اعرض PNG',
'mw_math_simple' => 'يا إما HTML لو بسيطة قوى أو PNG',
'mw_math_html'   => 'ياإما HTML لو ممكن أو PNG',
'mw_math_source' => 'اعرض على هيئة TeX (للبراوزرات النصية)',
'mw_math_modern' => 'أحسن للبراوزرات الحديثة',
'mw_math_mathml' => 'اعرض بصيغة MathML لو ممكن (تحت التجريب)',

# Patrolling
'markaspatrolleddiff'                 => 'علم عليها انها متراجعة',
'markaspatrolledtext'                 => 'علم على المقاله دى إنها متراجعة',
'markedaspatrolled'                   => 'اتعلم عليها متراجعة',
'markedaspatrolledtext'               => 'النسخة المختارة اتعلم عيها انها متراجعة',
'rcpatroldisabled'                    => 'مراجعة أخر التغييرات متعطلة',
'rcpatroldisabledtext'                => 'خاصية مراجعة أحدث التغييرات متعطلة  دلوقتي',
'markedaspatrollederror'              => 'مش ممكن تعلم علها إنها متراجعة',
'markedaspatrollederrortext'          => 'لازم تختار النسخة اللى عاوز تعلم عليها إنها متراجعة',
'markedaspatrollederror-noautopatrol' => 'مش مسموح ليك تعلم على تغييراتك الشخصية كأنها متراجعة.',

# Patrol log
'patrol-log-page'   => 'سجل المراجعة',
'patrol-log-header' => 'دا سجل بالنسخ المتراجعة',
'patrol-log-line'   => 'علم على $1 من $2 كأنها متراجعة $3',
'patrol-log-auto'   => '(اوتوماتيكي)',
'patrol-log-diff'   => 'ن$1',

# Image deletion
'deletedrevision'                 => 'مسح النسخة القديمة $1',
'filedeleteerror-short'           => 'غلط مسح الملف: $1',
'filedeleteerror-long'            => 'حصلت غلطات و الملف دا بيتمسح :

$1',
'filedelete-missing'              => 'الملف "$1" ما ينفعش يتمسح لأنه مش موجود.',
'filedelete-old-unregistered'     => 'نسخة الملف المحددة "$1" مش فى قاعدة البيانات.',
'filedelete-current-unregistered' => 'الملف المحدد "$1" مش فى قاعدة البيانات.',
'filedelete-archive-read-only'    => 'مش ممكن تكتب على مجلد الأرشيف "$1" بالويب سيرفر',

# Browsing diffs
'previousdiff' => '→ التعديل اللى قبل كده',
'nextdiff'     => 'التعديل اللى بعد كده ←',

# Media information
'mediawarning'         => "'''تحذير''': الملف دا فيه كود خبيث، يمكن عند تشغيله يبوظ الكمبيوتر بتاعك.<hr />",
'imagemaxsize'         => 'حدد الصور فى صفحات وصف الملفات لـ:',
'thumbsize'            => 'حجم العرض المتصغر:',
'widthheightpage'      => '$1×$2، $3 {{PLURAL:$3|صفحة|صفحة}}',
'file-info'            => '(حجم الملف: $1، نوع MIME: $2)',
'file-info-size'       => '($1 × $2 بكسل حجم الفايل: $3، نوع MIME: $4)',
'file-nohires'         => '<small>مافيش  ريزوليوشن اعلى متوفر.</small>',
'svg-long-desc'        => '(ملف SVG، اساسا $1 × $2 بكسل، حجم الملف: $3)',
'show-big-image'       => 'الصورة بدقه كامله',
'show-big-image-thumb' => '<small>حجم البروفه دى: $1 × $2 بكسل</small>',

# Special:NewImages
'newimages'             => 'جاليرى الصور الجديده',
'imagelisttext'         => 'دى لستة بـ$1 {{PLURAL:$1|ملف|ملفات}} مترتبة $2.',
'newimages-summary'     => 'الصفحةالمخصوصة دى بتعرض آخر الملفات المتحملة',
'showhidebots'          => '($1 بوتات)',
'noimages'              => 'مافيش حاجة للعرض.',
'ilsubmit'              => 'تدوير',
'bydate'                => 'على حسب التاريخ',
'sp-newimages-showfrom' => 'بين الملفات الجديدة  من أول $2، $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'video-dims'     => '$1، $2×$3',
'seconds-abbrev' => 'ث',
'minutes-abbrev' => 'ق',
'hours-abbrev'   => 'س',

# Bad image list
'bad_image_list' => 'الصيغه بالشكل ده:

عناصر اللسته  بس (السطور اللى تبتدى ب *) ها تتاخد فى الاعتبار.
أول وصلة فى السطر لازم تكون وصله لملف سيىء.
أى وصلات بعد كده فى نفس السطر هاتعتبر استثناءات، بمعنى تانى  مقالات ممكن الملف يكون موجود فيها.',

# Metadata
'metadata'          => 'بيانات ميتا',
'metadata-help'     => 'الملف ده فيه معلومات إضافيه، غالبا ما تكون أضيفت من الديجيتال كاميرا أو السكانر ح الضوئى المستخدم فى نقل الملف للكومبيوتر.
إذا كان الملف اتعدل عن حالته الأصلية، فبعض التفاصيل مش ها تعبر عن الملف المعدل.',
'metadata-expand'   => 'عرض التفاصيل الاضافيه',
'metadata-collapse' => 'تخبية التفاصيل الاضافيه',
'metadata-fields'   => 'حقول معطيات الميتا EXIF الموجوده فى الرساله دى هاتتعرض فى صفحة الصوره لما يكون جدول معطيات الميتا مضغوط. الحقول التانيه هاتكون مخفيه افتراضيا.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'العرض',
'exif-imagelength'                 => 'الطول',
'exif-bitspersample'               => 'بتس لكل مكون',
'exif-compression'                 => 'نظام الضغط',
'exif-photometricinterpretation'   => 'تركيب البكسل',
'exif-orientation'                 => 'التوجيه',
'exif-samplesperpixel'             => 'عدد المكونات',
'exif-planarconfiguration'         => 'ترتيب البيانات',
'exif-ycbcrsubsampling'            => 'نسبة العينة الفرعية بتاعة Y لـ C',
'exif-ycbcrpositioning'            => 'وضع Y و C',
'exif-xresolution'                 => 'الدقة الأفقية',
'exif-yresolution'                 => 'الدقة الرأسية',
'exif-resolutionunit'              => 'وحدة تحليل X و Y',
'exif-stripoffsets'                => 'موقع بيانات الصورة',
'exif-rowsperstrip'                => 'عدد الصفوف لكل ستريب',
'exif-stripbytecounts'             => 'بايت لكل ستريب مضغوط',
'exif-jpeginterchangeformat'       => 'الحد ل JPEG SOI',
'exif-jpeginterchangeformatlength' => 'بايت من بيانات JPEG',
'exif-transferfunction'            => 'وظيفة النقل',
'exif-whitepoint'                  => 'ألوان النقطة البيضا',
'exif-primarychromaticities'       => 'ألوان الأساسيات',
'exif-ycbcrcoefficients'           => 'معاملات مصفوفة تحويل فراغ اللون',
'exif-referenceblackwhite'         => 'جوز من قيم المرجع السودا والبيضا',
'exif-datetime'                    => 'تاريخ و وقت تغيير الملف',
'exif-imagedescription'            => 'عنوان الصورة',
'exif-make'                        => 'منتج الكاميرا',
'exif-model'                       => 'موديل الكاميرا',
'exif-software'                    => 'البرمجيات المستخدمة',
'exif-artist'                      => 'المؤلف',
'exif-copyright'                   => 'صاحب الحقوق الممحفوظة',
'exif-exifversion'                 => 'نسخة Exif',
'exif-flashpixversion'             => 'نسخة فلاش بكس المدعومة',
'exif-colorspace'                  => 'فرق اللون',
'exif-componentsconfiguration'     => 'معنى كل مكون',
'exif-compressedbitsperpixel'      => 'طور ضغط الصورة',
'exif-pixelydimension'             => 'عرض صورة صحيح',
'exif-pixelxdimension'             => 'ارتفاع صورة صحيح',
'exif-makernote'                   => 'ملاحظات الصانع',
'exif-usercomment'                 => 'تعليقات اليوزر',
'exif-relatedsoundfile'            => 'ملف صوت مرتبط',
'exif-datetimeoriginal'            => 'تاريخ و وقت الإنتاج',
'exif-datetimedigitized'           => 'تاريخ و وقت التحويل الرقمى',
'exif-subsectime'                  => 'وقت تاريخ ثوانى فرعية',
'exif-subsectimeoriginal'          => 'وقت تاريخ أصلى ثوانى فرعية',
'exif-subsectimedigitized'         => 'وقت تاريخ رقمى ثوانى فرعية',
'exif-exposuretime'                => 'مدة التعرض',
'exif-exposuretime-format'         => '$1 ثانية ($2)',
'exif-fnumber'                     => 'العدد البؤرى',
'exif-fnumber-format'              => 'البعد البؤرى/$1',
'exif-exposureprogram'             => 'برنامج التعرض',
'exif-spectralsensitivity'         => 'الحساسية الطيفية',
'exif-isospeedratings'             => 'تقييم سرعة أيزو',
'exif-oecf'                        => 'عامل التحويل الكهروضوئى',
'exif-shutterspeedvalue'           => 'سرعة القافل',
'exif-aperturevalue'               => 'فتحة القافل',
'exif-brightnessvalue'             => 'الضي',
'exif-exposurebiasvalue'           => 'تعويض التعرض',
'exif-maxaperturevalue'            => 'أقصى قافل أرضى',
'exif-subjectdistance'             => 'مسافة من الجسم',
'exif-meteringmode'                => 'طور القياس بالمتر',
'exif-lightsource'                 => 'مصدر النور',
'exif-flash'                       => 'فلاش',
'exif-focallength'                 => 'البعد البؤرى  للعدسة',
'exif-focallength-format'          => '$1 ملم',
'exif-subjectarea'                 => 'منطقة الجسم',
'exif-flashenergy'                 => 'طاقة الفلاش',
'exif-spatialfrequencyresponse'    => 'استجابة التردد الفراغي',
'exif-focalplanexresolution'       => 'تحليل المستوى البؤرى X',
'exif-focalplaneyresolution'       => 'تحليل المستوى البؤرى Y',
'exif-focalplaneresolutionunit'    => 'وحدة تحليل المستوى البؤرى',
'exif-subjectlocation'             => 'مكان الجسم',
'exif-exposureindex'               => 'فهرس التعرض',
'exif-sensingmethod'               => 'وسيلة الاستشعار',
'exif-filesource'                  => 'مصدر الملف',
'exif-scenetype'                   => 'نوع المشهد',
'exif-cfapattern'                  => 'نمط سى إف إيه',
'exif-customrendered'              => 'تظبيط الصورة حسب الطلب',
'exif-exposuremode'                => 'طريقة التعرض',
'exif-whitebalance'                => 'توازن الأبيض',
'exif-digitalzoomratio'            => 'نسبة الزوم الرقمية',
'exif-focallengthin35mmfilm'       => 'البعد البؤرى فى فيلم 35 مم',
'exif-scenecapturetype'            => 'نوع تصوير المشهد',
'exif-gaincontrol'                 => 'التحكم فى المشهد',
'exif-contrast'                    => 'التعارض',
'exif-saturation'                  => 'التشبع',
'exif-sharpness'                   => 'الحده',
'exif-devicesettingdescription'    => 'وصف ظبط الأداة',
'exif-subjectdistancerange'        => 'المسافه اللى بين  الهدف و الكاميرا',
'exif-imageuniqueid'               => 'رقم الصورة الفريد',
'exif-gpsversionid'                => 'نسخة علامة ال چى بى إس',
'exif-gpslatituderef'              => 'شمال أو جنوب خطوط العرض',
'exif-gpslatitude'                 => 'خط العرض',
'exif-gpslongituderef'             => 'خط الطول الشرقى أو الغربى',
'exif-gpslongitude'                => 'خط الطول',
'exif-gpsaltituderef'              => 'مرجع الارتفاع',
'exif-gpsaltitude'                 => 'الارتفاع',
'exif-gpstimestamp'                => 'وقت ال چى پى إس (ساعه ذريه)',
'exif-gpssatellites'               => 'الأقمار الصناعيه المستخدمه للقياس',
'exif-gpsstatus'                   => 'حالة جهاز الاستقبال',
'exif-gpsmeasuremode'              => 'طريقة القياس',
'exif-gpsdop'                      => 'دقة القياس',
'exif-gpsspeedref'                 => 'وحدة السرعة',
'exif-gpsspeed'                    => 'سرعة مستقبل ال چى بى إس',
'exif-gpstrackref'                 => 'المرجع لاتجاه الحركة',
'exif-gpstrack'                    => 'اتجاه الحركه',
'exif-gpsimgdirectionref'          => 'المرجع لاتجاه الصوره',
'exif-gpsimgdirection'             => 'اتجاه الصوره',
'exif-gpsmapdatum'                 => 'بيانات استطلاع الجيوديسيك المستخدمة',
'exif-gpsdestlatituderef'          => 'المرجع لخط عرض الهدف',
'exif-gpsdestlatitude'             => 'خط عرض الهدف',
'exif-gpsdestlongituderef'         => 'المرجع لخط طول الهدف',
'exif-gpsdestlongitude'            => 'خط طول الهدف',
'exif-gpsdestbearingref'           => 'المرجع لتحمل الهدف',
'exif-gpsdestbearing'              => 'تحمل الهدف',
'exif-gpsdestdistanceref'          => 'المرجع للمسافه للهدف',
'exif-gpsdestdistance'             => 'المسافه للهدف',
'exif-gpsprocessingmethod'         => 'اسم طريقة معالجة جى بى إس',
'exif-gpsareainformation'          => 'اسم لمنطقة ال چى پى إس',
'exif-gpsdatestamp'                => 'تاريخ GPS',
'exif-gpsdifferential'             => 'تصحيح GPS التفاضلي',

# EXIF attributes
'exif-compression-1' => 'مش مضغوط',
'exif-compression-6' => 'جيه بى إى جي',

'exif-photometricinterpretation-2' => 'آر جى بى',
'exif-photometricinterpretation-6' => 'واى سب سر',

'exif-unknowndate' => 'تاريخ مش معروف',

'exif-orientation-1' => 'عادي', # 0th row: top; 0th column: left
'exif-orientation-2' => 'دار بالعرض', # 0th row: top; 0th column: right
'exif-orientation-3' => 'دار 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'دار بالطول', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'اتلفت 90° CW 90° CW و اتقلب على جنبه', # 0th row: left; 0th column: top
'exif-orientation-6' => 'اتلفت 90° CW', # 0th row: right; 0th column: top
'exif-orientation-7' => 'اتلفت 90° CW 90° CW و اتقلب على راسه', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'اتلفت 90° عكس عقارب الساعة', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'صيغه تخينه',
'exif-planarconfiguration-2' => 'الصيغه المستويه',

'exif-componentsconfiguration-0' => 'مش موجود',

'exif-exposureprogram-0' => 'مش معروف',
'exif-exposureprogram-1' => 'يدوي',
'exif-exposureprogram-2' => 'برنامج عادى',
'exif-exposureprogram-3' => 'أولوية القافل',
'exif-exposureprogram-4' => 'أولوية القفل',
'exif-exposureprogram-5' => 'برنامج صنع (بيميل ناحية عمق الفيلد)',
'exif-exposureprogram-6' => 'برنامج الفعل (بيميل ناحية سرعة القفل)',
'exif-exposureprogram-7' => 'وضع البورتريه (لصور القفل مع الخلفية بعيدة عن البؤرة)',
'exif-exposureprogram-8' => 'وضع الأرضية (لصور الأرضية مع الخلفية فى البؤرة)',

'exif-subjectdistance-value' => '$1 متر',

'exif-meteringmode-0'   => 'مش معروف',
'exif-meteringmode-1'   => 'متوسط',
'exif-meteringmode-2'   => 'متوسط موزون بالمركز',
'exif-meteringmode-3'   => 'بقعة',
'exif-meteringmode-4'   => 'مالتى سبوت',
'exif-meteringmode-5'   => 'نمط',
'exif-meteringmode-6'   => 'جزئي',
'exif-meteringmode-255' => 'تاني',

'exif-lightsource-0'   => 'مش معروف',
'exif-lightsource-1'   => 'نورالنهار',
'exif-lightsource-2'   => 'فلورسنت',
'exif-lightsource-3'   => 'تنجستين (ضوء مشع)',
'exif-lightsource-4'   => 'فلاش',
'exif-lightsource-9'   => 'جو صحو',
'exif-lightsource-10'  => 'جو مغيم',
'exif-lightsource-11'  => 'ضل',
'exif-lightsource-12'  => 'فلورسنت نور النهار (D 5700 – 7100K)',
'exif-lightsource-13'  => 'فلورسنت نهار أبيض (N 4600 – 5400K)',
'exif-lightsource-14'  => 'فلورسنت أبيض كوول(W 3900 – 4500K)',
'exif-lightsource-15'  => 'فلورسنت أبيض (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'ضوء قياسى  A',
'exif-lightsource-18'  => 'ضوء قياسى B',
'exif-lightsource-19'  => 'ضوء قياسى C',
'exif-lightsource-20'  => 'دي55',
'exif-lightsource-21'  => 'دي65',
'exif-lightsource-22'  => 'دي75',
'exif-lightsource-23'  => 'دي50',
'exif-lightsource-24'  => 'تنجستين ستوديو أيزو',
'exif-lightsource-255' => 'مصدر  نور تانى',

'exif-focalplaneresolutionunit-2' => 'بوصة',

'exif-sensingmethod-1' => 'مش متعرف',
'exif-sensingmethod-2' => 'حساس لون المساحة من راق واحد',
'exif-sensingmethod-3' => 'حساس لون المساحة من راقين',
'exif-sensingmethod-4' => 'حساس لون المساحة من تلات راقات',
'exif-sensingmethod-5' => 'حساس لون مساحة متتابع',
'exif-sensingmethod-7' => 'حساس بتلات خطوط',
'exif-sensingmethod-8' => 'حساس لون خطى متتابع',

'exif-filesource-3' => 'دى إس سي',

'exif-scenetype-1' => 'صورة متاخدة على طول',

'exif-customrendered-0' => 'عملية عادية',
'exif-customrendered-1' => 'عملية حسب الطلب',

'exif-exposuremode-0' => 'تعرض أوتوماتيكي',
'exif-exposuremode-1' => 'تعرض باللإيد',
'exif-exposuremode-2' => 'اقواس أوتوماتيكي',

'exif-whitebalance-0' => 'توازن الأبيض اوتوماتيكي',
'exif-whitebalance-1' => 'توازن الأبيض بالإيد',

'exif-scenecapturetype-0' => 'مظبوط',
'exif-scenecapturetype-1' => 'أرضية',
'exif-scenecapturetype-2' => 'بورتوريه',
'exif-scenecapturetype-3' => 'منظر بالليل',

'exif-gaincontrol-0' => 'مافيش',
'exif-gaincontrol-1' => 'تحكم لفوق واطي',
'exif-gaincontrol-2' => 'تحكم لفوق عالي',
'exif-gaincontrol-3' => 'تحكم تحت واطي',
'exif-gaincontrol-4' => 'تحكم تحت  عالي',

'exif-contrast-0' => 'طبيعي',
'exif-contrast-1' => 'ناعم',
'exif-contrast-2' => 'ناشف',

'exif-saturation-0' => 'عادي',
'exif-saturation-1' => 'تشبع واطي',
'exif-saturation-2' => 'تشبع عالي',

'exif-sharpness-0' => 'عادي',
'exif-sharpness-1' => 'ناعم',
'exif-sharpness-2' => 'ناشف',

'exif-subjectdistancerange-0' => 'مش معروف',
'exif-subjectdistancerange-1' => 'ماكرو',
'exif-subjectdistancerange-2' => 'منظر من قريب',
'exif-subjectdistancerange-3' => 'منظر من بعيد',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'دايرة العرض الشمالية',
'exif-gpslatitude-s' => 'دايرة العرض الجنوبية',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'خط الطول الشرقي',
'exif-gpslongitude-w' => 'خط الطول الغربي',

'exif-gpsstatus-a' => 'القياس شغال',
'exif-gpsstatus-v' => 'شمول القياس',

'exif-gpsmeasuremode-2' => 'قياس ببعدين',
'exif-gpsmeasuremode-3' => 'قياس  تلاتى الابعاد',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'كيلومتر فى الساعة',
'exif-gpsspeed-m' => 'ميل فى الساعة',
'exif-gpsspeed-n' => 'عقد',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'الاتجاه الحقيقي',
'exif-gpsdirection-m' => 'الاتجاه المغناطيسي',

# External editor support
'edit-externally'      => 'استعمل تطبيق من بره علشان تعدل الملف دا',
'edit-externally-help' => 'بص على [http://www.mediawiki.org/wiki/Manual:External_editors  تعليمات الاعداد] علشان معلومات اكتر.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'الكل',
'imagelistall'     => 'الكل',
'watchlistall2'    => 'الكل',
'namespacesall'    => 'الكل',
'monthsall'        => 'الكل',

# E-mail address confirmation
'confirmemail'             => 'اعمل تأكيد للأيميل بتاعك',
'confirmemail_noemail'     => 'إنت ما عندكش ايميل صحيح متسجل فى [[Special:Preferences|تفضيلاتك]].',
'confirmemail_text'        => '{{SITENAME}} بيطلب انك تعمل تأكيد للأيميل قبل ما تستعمل الخصايص المرتبطة بالايميل.
دوس على زرار التفعيل اللى تحت علشان يتبعتلك ايميل التأكيد.
الايميل ح يكون فيه لينك فيها كود تفعيل؛
دوس على اللينك  علشان نتأكد إن ايميلك صحيح.',
'confirmemail_pending'     => '<div class="error">كود التأكيد خلاص اتبعت للأيميل بتاعك;
لو كنت لسة فاتح حسابك من شوية صغيرة، لو سمحت تستنى دقيقتين تلاتة قبل ما تطلب كود تاني.</div>',
'confirmemail_send'        => 'ابعت كود التأكيد',
'confirmemail_sent'        => 'إيميل التأكيد خلاص اتبعت.',
'confirmemail_oncreate'    => 'كود التأكيد اتبعت للأيميل بتاعك.
مش لازم تستعمل الكود دا علشان تسجل دخولك ،بس ح تحتاج تدخله بعدين قبل ما تقدر  تستفيد من أى خاصية مربوطة بالايميل فى الويكي.',
'confirmemail_sendfailed'  => '{{SITENAME}} ماقدرش يبعت ايميل التأكيد.
لو سمحت تتأكد من الايميل بتاعك.

الغلط اللى حصل: $1',
'confirmemail_invalid'     => 'كود تفعيل غلط.
يمكن صلاحيته تكون انتهت.',
'confirmemail_needlogin'   => 'لازم $1 علشان تأكد الايميل بتاعك.',
'confirmemail_success'     => 'الايميل بتاعك اتأكد خلاص.
ممكن دلوقتى تسجل دخولك و تستمتع بالويكي.',
'confirmemail_loggedin'    => 'الايميل بتاعك اتأكد خلاص.',
'confirmemail_error'       => 'حصلت حاجة غلط و احنا بنحفظ التأكيد بتاعك.',
'confirmemail_subject'     => 'تأكيد الايميل من {{SITENAME}}',
'confirmemail_body'        => 'فى واحد، ممكن يكون إنتا، من عنوان الأيبى $1،
فتح حساب "$2" بعنوان الايميل دا فى {{SITENAME}}.

علشان نتأكد أن  الحساب دا بتاعك فعلا و علشان كمان تفعيل خواص الايميل فى {{SITENAME}}، افتح اللينك دى فى البراوزر بتاعك :

$3

إذا *ماكنتش* إنتا اللى فتحت الحساب ، دوس على اللينك دى علشان تلغى تأكيد الايميل
:

$5

كود التفعيل دا ح ينتهى $4.',
'confirmemail_invalidated' => 'تأكيد عنوان الايميل اتلغى',
'invalidateemail'          => 'إلغى تأكيد الايميل',

# Scary transclusion
'scarytranscludedisabled' => '[التضمين  فى الإنترويكى متعطل]',
'scarytranscludefailed'   => '[التدوير على القالب فشل ل$1]',
'scarytranscludetoolong'  => '[عنوان طويل جدا]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">التراكباك بتاع الصفحة دي:<br />
$1
</div>',
'trackbackremove'   => '([$1 امسح])',
'trackbacklink'     => 'تراكباك',
'trackbackdeleteok' => 'التراكباك اتمسح بنجاح.',

# Delete conflict
'deletedwhileediting' => "'''تحذير''':  الصفحة دى اتمسحت بعد ما بدأت أنت  فى تحريرها!",
'confirmrecreate'     => "اليوزر [[User:$1|$1]] ([[User talk:$1|مناقشة]]) مسح المقالة دى بعد ما انت بدأت فى تحريرها علشان:
:''$2''
لو سمحت تتأكد من أنك عايز تبتدى المقالة دى تاني.",
'recreate'            => 'ابتدى تاني',

'unit-pixel' => 'بيكس',

# HTML dump
'redirectingto' => 'بتتحول لـ [[:$1]]...',

# action=purge
'confirm_purge'        => 'امسح الكاش بتاع الصفحة دي؟

$1',
'confirm_purge_button' => 'طيب',

# AJAX search
'searchcontaining' => "دور على الصفحات اللى فيها ''$1''.",
'searchnamed'      => "دور على الصفحات اللى اسمها ''$1''.",
'articletitles'    => "الصفحات اللى بتبتدى بـ''$1''",
'hideresults'      => 'خبى النتايج',
'useajaxsearch'    => 'دور بـ أجاكس',

# Separators for various lists, etc.
'semicolon-separator' => '؛',
'comma-separator'     => '،&#32;',
'autocomment-prefix'  => '-',

# Multipage image navigation
'imgmultipageprev' => '← الصفحة اللى فاتت',
'imgmultipagenext' => 'الصفحه اللى بعد كده →',
'imgmultigo'       => 'روح!',
'imgmultigoto'     => 'روح لصفحة $1',

# Table pager
'ascending_abbrev'         => 'طالع',
'descending_abbrev'        => 'نازل',
'table_pager_next'         => 'الصفحه اللى بعد كده',
'table_pager_prev'         => 'الصفحة اللى فاتت',
'table_pager_first'        => 'أول صفحة',
'table_pager_last'         => 'آخر صفحة',
'table_pager_limit'        => 'اعرض $1 عنصر فى الصفحة',
'table_pager_limit_submit' => 'روح',
'table_pager_empty'        => 'ما فى ش نتايج',

# Auto-summaries
'autosumm-blank'   => 'مسح كل اللى فى الصفحة',
'autosumm-replace' => "تبديل الصفحة ب'$1'",
'autoredircomment' => 'تحويل لـ [[$1]]',
'autosumm-new'     => 'صفحه جديده: $1',

# Size units
'size-bytes'     => '$1 بايت',
'size-kilobytes' => '$1 كيلوبايت',
'size-megabytes' => '$1 ميجابايت',
'size-gigabytes' => '$1 جيجابايت',

# Live preview
'livepreview-loading' => 'تحميل…',
'livepreview-ready'   => 'تحميل… جاهز!',
'livepreview-failed'  => 'البروفة الحية مانفعتش!
جرب البروفة العادية.',
'livepreview-error'   => 'الاتصال مانفعش: $1 "$2"
جرب البروفة العادية.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'التغييرات الأحدث من $1 {{PLURAL:$1|ثانية|ثانية}} ثانية ممكن ما تظهرش فى اللستة دي.',
'lag-warn-high'   => 'علشان فى تأخير كبير فى تحديث قاعدة البيانات بتاعة السيرفر، التغييرات أحدث من $1 {{PLURAL:$1|ثانية|ثانية}}
ممكن ما تظهرش فى اللستة دي.',

# Watchlist editor
'watchlistedit-numitems'       => 'لستة المراقبة بتاعتك  فيها{{PLURAL:$1|عنوان واحد|$1 عنوان}}، من غير صفحات المناقشة.',
'watchlistedit-noitems'        => 'لستة الرقابة بتاعتك  مافيهاش ولا عنوان.',
'watchlistedit-normal-title'   => 'تعديل لستة المراقبة',
'watchlistedit-normal-legend'  => 'شيل العناوين من لستة المراقبة',
'watchlistedit-normal-explain' => 'العناوين فى لستة المراقبة بتاعتك معروضة تحت.
علشان تشيل عنوان، دوس على الصندوق اللى جنبه، ودوس على شيل العناوين.
ممكن كمان [[Special:Watchlist/raw|تعديل اللستة الخام]].',
'watchlistedit-normal-submit'  => 'شيل العناوين',
'watchlistedit-normal-done'    => '{{PLURAL:$1|عنوان واحد|$1 عنوان}} اتشال من لستة المراقبة بتاعتك:',
'watchlistedit-raw-title'      => 'تعديل لستة المراقبة الخام',
'watchlistedit-raw-legend'     => 'تعديل لستة المراقبة الخام',
'watchlistedit-raw-explain'    => 'العناوين فى لستة مراقبتك معروضة تحت، وممكن تعدلها لما تزود او تشيل من اللستة؛
عنوان واحد فى السطر.
لما تخلص، دوس تحديث لستة المراقبة.
ممكن كمان [[Special:Watchlist/edit|تستعمل المحرر القياسي]].',
'watchlistedit-raw-titles'     => 'العناوين:',
'watchlistedit-raw-submit'     => 'تحديث لستة المراقبة',
'watchlistedit-raw-done'       => 'لستة المراقبة بتاعتك اتحدثت خلاص.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|عنوان واحد|$1 عنوان}} اتزود:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|عنوان واحد|$1 عنوان}} اتشال:',

# Watchlist editing tools
'watchlisttools-view' => 'عرض التعديلات المرتبطه',
'watchlisttools-edit' => 'عرض وتعديل لستة الصفحات اللى باراقبها',
'watchlisttools-raw'  => 'عدل لستة المراقبه الخام',

# Iranian month names
'iranian-calendar-m1'  => 'فروردین',
'iranian-calendar-m2'  => 'أردیبهشت',
'iranian-calendar-m3'  => 'خرداد',
'iranian-calendar-m4'  => 'تیر',
'iranian-calendar-m5'  => 'امرداد',
'iranian-calendar-m6'  => 'شهریور',
'iranian-calendar-m7'  => 'مهر',
'iranian-calendar-m8'  => 'آبان',
'iranian-calendar-m9'  => 'آذر',
'iranian-calendar-m10' => 'دی',
'iranian-calendar-m11' => 'بهمن',
'iranian-calendar-m12' => 'إسفند',

# Hijri month names
'hijri-calendar-m1'  => 'محرم',
'hijri-calendar-m2'  => 'صفر',
'hijri-calendar-m3'  => 'ربيع الأول',
'hijri-calendar-m4'  => 'ربيع الثانى',
'hijri-calendar-m5'  => 'جمادى الأول',
'hijri-calendar-m6'  => 'جمادى الثانى',
'hijri-calendar-m7'  => 'رجب',
'hijri-calendar-m8'  => 'شعبان',
'hijri-calendar-m9'  => 'رمضان',
'hijri-calendar-m10' => 'شوال',
'hijri-calendar-m11' => 'ذو القعدة',
'hijri-calendar-m12' => 'ذو الحجة',

# Hebrew month names
'hebrew-calendar-m1'      => 'تيشرى',
'hebrew-calendar-m2'      => 'تيشفان',
'hebrew-calendar-m3'      => 'كيسليف',
'hebrew-calendar-m4'      => 'تيفيت',
'hebrew-calendar-m5'      => 'شيفات',
'hebrew-calendar-m6'      => 'أدار',
'hebrew-calendar-m6a'     => 'أدار الأول',
'hebrew-calendar-m6b'     => 'أدار الثانى',
'hebrew-calendar-m7'      => 'نيزان',
'hebrew-calendar-m8'      => 'أيار',
'hebrew-calendar-m9'      => 'سيفان',
'hebrew-calendar-m10'     => 'تموز',
'hebrew-calendar-m11'     => 'آف',
'hebrew-calendar-m12'     => 'أيلول',
'hebrew-calendar-m1-gen'  => 'تيشرى',
'hebrew-calendar-m2-gen'  => 'تيشفان',
'hebrew-calendar-m3-gen'  => 'كيسليف',
'hebrew-calendar-m4-gen'  => 'تيفيت',
'hebrew-calendar-m5-gen'  => 'شيفات',
'hebrew-calendar-m6-gen'  => 'أدار',
'hebrew-calendar-m6a-gen' => 'أدار الأول',
'hebrew-calendar-m6b-gen' => 'أدار الثانى',
'hebrew-calendar-m7-gen'  => 'نيزان',
'hebrew-calendar-m8-gen'  => 'أيار',
'hebrew-calendar-m9-gen'  => 'سيفان',
'hebrew-calendar-m10-gen' => 'تموز',
'hebrew-calendar-m11-gen' => 'آب',
'hebrew-calendar-m12-gen' => 'أيلول',

# Signatures
'timezone-utc' => 'يو تى سى',

# Core parser functions
'unknown_extension_tag' => 'تاج بتاع امتداد مش معروف "$1"',

# Special:Version
'version'                          => 'نسخه', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'الامتدادات المتثبتة',
'version-specialpages'             => 'صفحات مخصوصة',
'version-parserhooks'              => 'خطاطيف البريزر',
'version-variables'                => 'المتغيرات',
'version-other'                    => 'تانية',
'version-mediahandlers'            => 'متحكمات الميديا',
'version-hooks'                    => 'الخطاطيف',
'version-extension-functions'      => 'وظايف الامتداد',
'version-parser-extensiontags'     => 'التاجز بتوع امتداد البريزر',
'version-parser-function-hooks'    => 'خطاطيف دالة المحلل',
'version-skin-extension-functions' => 'الوظايف بتاعة امتداد الواجهة',
'version-hook-name'                => 'اسم الخطاف',
'version-hook-subscribedby'        => 'اشتراك باسم',
'version-version'                  => 'نسخه',
'version-license'                  => 'الترخيص',
'version-software'                 => 'السوفتوير المتستاب',
'version-software-product'         => 'المنتج',
'version-software-version'         => 'النسخه',

# Special:FilePath
'filepath'         => 'مسار ملف',
'filepath-page'    => 'الملف:',
'filepath-submit'  => 'المسار',
'filepath-summary' => 'الصفحة المخصوصة دى بتعرض المسار الكامل  بتاع ملف.
الصور بتتعرض  بدقة كاملة، أنواع الملفات التانية ح تشتغل فى البرنامج بتاعهم مباشرة.
دخل اسم الملف  من غير البريفيكس "{{ns:image}}:"',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'دور على الملفات المتكررة',
'fileduplicatesearch-summary'  => 'دور على الملفات المتكررة على اساس قيمة الهاش بتاعتها.

دخل اسم الملف من غير البريفكس "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'تدوير على متكرر',
'fileduplicatesearch-filename' => 'اسم الملف:',
'fileduplicatesearch-submit'   => 'تدوير',
'fileduplicatesearch-info'     => '$1 × $2 بكسل<br />حجم الملف: $3<br />نوع MIME: $4',
'fileduplicatesearch-result-1' => 'الملف "$1" ما لهو ش تكرار متطابق.',
'fileduplicatesearch-result-n' => 'الملف "$1" فيه {{PLURAL:$2|1 تكرار متطابق|$2 تكرار متطابق}}.',

# Special:SpecialPages
'specialpages'                   => 'صفحات مخصوصه',
'specialpages-note'              => '----
* صفحات خاصة عادية.
* <span class="mw-specialpagerestricted">صفحات خاصة للناس اللى مسموح لهم.</span>',
'specialpages-group-maintenance' => 'تقارير الصيانة',
'specialpages-group-other'       => 'صفحات خاصه تا نيه',
'specialpages-group-login'       => 'ادخل / سجل',
'specialpages-group-changes'     => 'السجلات واحدث التغييرات',
'specialpages-group-media'       => 'تقارير الميديا وعمليات التحميل',
'specialpages-group-users'       => 'اليوزرز و الحقوق',
'specialpages-group-highuse'     => 'صفحات بتستخدم كتير',
'specialpages-group-pages'       => 'لستات الصفحة',
'specialpages-group-pagetools'   => 'أدوات الصفحات',
'specialpages-group-wiki'        => 'بيانات وأدوات الويكى',
'specialpages-group-redirects'   => 'صفحات  التحويل الخاصه',
'specialpages-group-spam'        => 'أدوات السبام',

# Special:BlankPage
'blankpage'              => 'صفحة فاضية',
'intentionallyblankpage' => 'الصفحة دى متسابة فاضية بالقصد',

);
