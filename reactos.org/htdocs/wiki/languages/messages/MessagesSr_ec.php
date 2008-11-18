<?php
/** Serbian Cyrillic ekavian (ћирилица)
 *
 * @ingroup Language
 * @file
 *
 * @author Kale
 * @author Millosh
 * @author Red Baron
 * @author Sasa Stefanovic
 * @author Јованвб
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA            => "Медија",
	NS_SPECIAL          => "Посебно",
	NS_MAIN             => "",
	NS_TALK             => "Разговор",
	NS_USER             => "Корисник",
	NS_USER_TALK        => "Разговор_са_корисником",
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => "Разговор_о_$1",
	NS_IMAGE            => "Слика",
	NS_IMAGE_TALK       => "Разговор_о_слици",
	NS_MEDIAWIKI        => "МедијаВики",
	NS_MEDIAWIKI_TALK   => "Разговор_о_МедијаВикију",
	NS_TEMPLATE         => 'Шаблон',
	NS_TEMPLATE_TALK    => 'Разговор_о_шаблону',
	NS_HELP             => 'Помоћ',
	NS_HELP_TALK        => 'Разговор_о_помоћи',
	NS_CATEGORY         => 'Категорија',
	NS_CATEGORY_TALK    => 'Разговор_о_категорији',
);

# Aliases to latin namespaces
$namespaceAliases = array(
	"Medija"                  => NS_MEDIA,
	"Posebno"                 => NS_SPECIAL,
	"Razgovor"                => NS_TALK,
	"Korisnik"                => NS_USER,
	"Razgovor_sa_korisnikom"  => NS_USER_TALK,
	"Razgovor_o_$1"           => NS_PROJECT_TALK,
	"Slika"                   => NS_IMAGE,
	"Razgovor_o_slici"        => NS_IMAGE_TALK,
	"MedijaViki"              => NS_MEDIAWIKI,
	"Razgovor_o_MedijaVikiju" => NS_MEDIAWIKI_TALK,
	'Šablon'                  => NS_TEMPLATE,
	'Razgovor_o_šablonu'      => NS_TEMPLATE_TALK,
	'Pomoć'                   => NS_HELP,
	'Razgovor_o_pomoći'      => NS_HELP_TALK,
	'Kategorija'              => NS_CATEGORY,
	'Razgovor_o_kategoriji'   => NS_CATEGORY_TALK,
);

$skinNames = array(
 "Обична", "Носталгија", "Келнско плаво"
);

$extraUserToggles = array(
	'nolangconversion',
);

$datePreferenceMigrationMap = array(
	'default',
	'hh:mm d. month y.',
	'hh:mm d month y',
	'hh:mm dd.mm.yyyy',
	'hh:mm d.m.yyyy',
	'hh:mm d. mon y.',
	'hh:mm d mon y',
	'h:mm d. month y.',
	'h:mm d month y',
	'h:mm dd.mm.yyyy',
	'h:mm d.m.yyyy',
	'h:mm d. mon y.',
	'h:mm d mon y',
);

$datePreferences = array(
	'default',
	'hh:mm d. month y.',
	'hh:mm d month y',
	'hh:mm dd.mm.yyyy',
	'hh:mm d.m.yyyy',
	'hh:mm d. mon y.',
	'hh:mm d mon y',
	'h:mm d. month y.',
	'h:mm d month y',
	'h:mm dd.mm.yyyy',
	'h:mm d.m.yyyy',
	'h:mm d. mon y.',
	'h:mm d mon y',
);

$defaultDateFormat = 'hh:mm d. month y.';

$dateFormats = array(
	/*
	'Није битно',
	'06:12, 5. јануар 2001.',
	'06:12, 5 јануар 2001',
	'06:12, 05.01.2001.',
	'06:12, 5.1.2001.',
	'06:12, 5. јан 2001.',
	'06:12, 5 јан 2001',
	'6:12, 5. јануар 2001.',
	'6:12, 5 јануар 2001',
	'6:12, 05.01.2001.',
	'6:12, 5.1.2001.',
	'6:12, 5. јан 2001.',
	'6:12, 5 јан 2001',
	 */

	'hh:mm d. month y. time'    => 'H:i',
	'hh:mm d month y time'      => 'H:i',
	'hh:mm dd.mm.yyyy time'     => 'H:i',
	'hh:mm d.m.yyyy time'       => 'H:i',
	'hh:mm d. mon y. time'      => 'H:i',
	'hh:mm d mon y time'        => 'H:i',
	'h:mm d. month y. time'     => 'G:i',
	'h:mm d month y time'       => 'G:i',
	'h:mm dd.mm.yyyy time'      => 'G:i',
	'h:mm d.m.yyyy time'        => 'G:i',
	'h:mm d. mon y. time'       => 'G:i',
	'h:mm d mon y time'         => 'G:i',

	'hh:mm d. month y. date'    => 'j. F Y.',
	'hh:mm d month y date'      => 'j F Y',
	'hh:mm dd.mm.yyyy date'     => 'd.m.Y',
	'hh:mm d.m.yyyy date'       => 'j.n.Y',
	'hh:mm d. mon y. date'      => 'j. M Y.',
	'hh:mm d mon y date'        => 'j M Y',
	'h:mm d. month y. date'     => 'j. F Y.',
	'h:mm d month y date'       => 'j F Y',
	'h:mm dd.mm.yyyy date'      => 'd.m.Y',
	'h:mm d.m.yyyy date'        => 'j.n.Y',
	'h:mm d. mon y. date'       => 'j. M Y.',
	'h:mm d mon y date'         => 'j M Y',

	'hh:mm d. month y. both'    =>'H:i, j. F Y.',
	'hh:mm d month y both'      =>'H:i, j F Y',
	'hh:mm dd.mm.yyyy both'     =>'H:i, d.m.Y',
	'hh:mm d.m.yyyy both'       =>'H:i, j.n.Y',
	'hh:mm d. mon y. both'      =>'H:i, j. M Y.',
	'hh:mm d mon y both'        =>'H:i, j M Y',
	'h:mm d. month y. both'     =>'G:i, j. F Y.',
	'h:mm d month y both'       =>'G:i, j F Y',
	'h:mm dd.mm.yyyy both'      =>'G:i, d.m.Y',
	'h:mm d.m.yyyy both'        =>'G:i, j.n.Y',
	'h:mm d. mon y. both'       =>'G:i, j. M Y.',
	'h:mm d mon y both'         =>'G:i, j M Y',
);

/* NOT USED IN STABLE VERSION */
$magicWords = array(
#	ID                                CASE SYNONYMS
	'redirect'               => array( 0, '#Преусмери', '#redirect', '#преусмери', '#ПРЕУСМЕРИ' ),
	'notoc'                  => array( 0, '__NOTOC__', '__БЕЗСАДРЖАЈА__' ),
	'forcetoc'               => array( 0, '__FORCETOC__', '__ФОРСИРАНИСАДРЖАЈ__' ),
	'toc'                    => array( 0, '__TOC__', '__САДРЖАЈ__' ),
	'noeditsection'          => array( 0, '__NOEDITSECTION__', '__БЕЗ_ИЗМЕНА__', '__БЕЗИЗМЕНА__' ),
	'currentmonth'           => array( 1, 'CURRENTMONTH', 'ТРЕНУТНИМЕСЕЦ' ),
	'currentmonthname'       => array( 1, 'CURRENTMONTHNAME', 'ТРЕНУТНИМЕСЕЦИМЕ' ),
	'currentmonthnamegen'    => array( 1, 'CURRENTMONTHNAMEGEN', 'ТРЕНУТНИМЕСЕЦГЕН' ),
	'currentmonthabbrev'     => array( 1, 'CURRENTMONTHABBREV', 'ТРЕНУТНИМЕСЕЦСКР' ),
	'currentday'             => array( 1, 'CURRENTDAY', 'ТРЕНУТНИДАН' ),
	'currentdayname'         => array( 1, 'CURRENTDAYNAME', 'ТРЕНУТНИДАНИМЕ' ),
	'currentyear'            => array( 1, 'CURRENTYEAR', 'ТРЕНУТНАГОДИНА' ),
	'currenttime'            => array( 1, 'CURRENTTIME', 'ТРЕНУТНОВРЕМЕ' ),
	'numberofarticles'       => array( 1, 'NUMBEROFARTICLES', 'БРОЈЧЛАНАКА' ),
	'numberoffiles'          => array( 1, 'NUMBEROFFILES', 'БРОЈДАТОТЕКА', 'БРОЈФАЈЛОВА' ),
	'pagename'               => array( 1, 'PAGENAME', 'СТРАНИЦА' ),
	'pagenamee'              => array( 1, 'PAGENAMEE', 'СТРАНИЦЕ' ),
	'namespace'              => array( 1, 'NAMESPACE', 'ИМЕНСКИПРОСТОР' ),
	'namespacee'             => array( 1, 'NAMESPACEE', 'ИМЕНСКИПРОСТОРИ' ),
	'fullpagename'           => array( 1, 'FULLPAGENAME', 'ПУНОИМЕСТРАНЕ' ),
	'fullpagenamee'          => array( 1, 'FULLPAGENAMEE', 'ПУНОИМЕСТРАНЕЕ' ),
	'msg'                    => array( 0, 'MSG:', 'ПОР:' ),
	'subst'                  => array( 0, 'SUBST:', 'ЗАМЕНИ:' ),
	'msgnw'                  => array( 0, 'MSGNW:', 'НВПОР:' ),
	'img_thumbnail'          => array( 1, 'thumbnail', 'thumb', 'мини' ),
	'img_manualthumb'        => array( 1, 'thumbnail=$1', 'thumb=$1', 'мини=$1' ),
	'img_right'              => array( 1, 'right', 'десно', 'д' ),
	'img_left'               => array( 1, 'left', 'лево', 'л' ),
	'img_none'               => array( 1, 'none', 'н', 'без' ),
	'img_width'              => array( 1, '$1px', '$1пискел' , '$1п' ),
	'img_center'             => array( 1, 'center', 'centre', 'центар', 'ц' ),
	'img_framed'             => array( 1, 'framed', 'enframed', 'frame', 'оквир', 'рам' ),
	'int'                    => array( 0, 'INT:', 'ИНТ:' ),
	'sitename'               => array( 1, 'SITENAME', 'ИМЕСАЈТА' ),
	'ns'                     => array( 0, 'NS:', 'ИП:' ),
	'localurl'               => array( 0, 'LOCALURL:', 'ЛОКАЛНААДРЕСА:' ),
	'localurle'              => array( 0, 'LOCALURLE:', 'ЛОКАЛНЕАДРЕСЕ:' ),
	'server'                 => array( 0, 'SERVER', 'СЕРВЕР' ),
	'servername'             => array( 0, 'SERVERNAME', 'ИМЕСЕРВЕРА' ),
	'scriptpath'             => array( 0, 'SCRIPTPATH', 'СКРИПТА' ),
	'grammar'                => array( 0, 'GRAMMAR:', 'ГРАМАТИКА:' ),
	'notitleconvert'         => array( 0, '__NOTITLECONVERT__', '__NOTC__', '__БЕЗКН__', '__BEZKN__' ),
	'nocontentconvert'       => array( 0, '__NOCONTENTCONVERT__', '__NOCC__', '__БЕЗЦЦ__' ),
	'currentweek'            => array( 1, 'CURRENTWEEK', 'ТРЕНУТНАНЕДЕЉА' ),
	'currentdow'             => array( 1, 'CURRENTDOW', 'ТРЕНУТНИДОВ' ),
	'revisionid'             => array( 1, 'REVISIONID', 'ИДРЕВИЗИЈЕ' ),
	'plural'                 => array( 0, 'PLURAL:', 'МНОЖИНА:' ),
	'fullurl'                => array( 0, 'FULLURL:', 'ПУНУРЛ:' ),
	'fullurle'               => array( 0, 'FULLURLE:', 'ПУНУРЛЕ:' ),
	'lcfirst'                => array( 0, 'LCFIRST:', 'ЛЦПРВИ:' ),
	'ucfirst'                => array( 0, 'UCFIRST:', 'УЦПРВИ:' ),
	'lc'                     => array( 0, 'LC:', 'ЛЦ:' ),
	'uc'                     => array( 0, 'UC:', 'УЦ:' ),
);
$separatorTransformTable = array(',' => '.', '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Подвуци везе:',
'tog-highlightbroken'         => 'Форматирај покварене везе <a href="" class="new">овако</a> (алтернатива: овако<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Уравнај пасусе',
'tog-hideminor'               => 'Сакриј мале измене у списку скорашњих измена',
'tog-extendwatchlist'         => 'Побољшан списак надгледања',
'tog-usenewrc'                => 'Побољшан списак скорашњих измена (захтева JavaScript)',
'tog-numberheadings'          => 'Аутоматски нумериши поднаслове',
'tog-showtoolbar'             => 'Прикажи дугмиће за измене (захтева JavaScript)',
'tog-editondblclick'          => 'Мењај странице двоструким кликом (захтева JavaScript)',
'tog-editsection'             => 'Омогући измену делова [уреди] везама',
'tog-editsectiononrightclick' => 'Омогући измену делова десним кликом<br />на њихове наслове (захтева JavaScript)',
'tog-showtoc'                 => 'Прикажи садржај (у чланцима са више од 3 поднаслова)',
'tog-rememberpassword'        => 'Памти лозинку кроз више сеанси',
'tog-editwidth'               => 'Поље за измене има пуну ширину',
'tog-watchcreations'          => 'Додај странице које правим у мој списак надгледања',
'tog-watchdefault'            => 'Додај странице које мењам у мој списак надгледања',
'tog-watchmoves'              => 'Додај странице које премештам у мој списак надгледања',
'tog-watchdeletion'           => 'Додај странице које бришем у мој списак надгледања',
'tog-minordefault'            => 'Означи све измене малим испрва',
'tog-previewontop'            => 'Прикажи претпреглед пре поља за измену',
'tog-previewonfirst'          => 'Прикажи претпреглед при првој измени',
'tog-nocache'                 => 'Онемогући кеширање страница',
'tog-enotifwatchlistpages'    => 'Пошаљи ми е-пошту када се промени страна коју надгледам',
'tog-enotifusertalkpages'     => 'Пошаљи ми е-пошту када се промени моја корисничка страна за разговор',
'tog-enotifminoredits'        => 'Пошаљи ми е-пошту такође за мале измене страна',
'tog-enotifrevealaddr'        => 'Откриј адресу моје е-поште у пошти обавештења',
'tog-shownumberswatching'     => 'Прикажи број корисника који надгледају',
'tog-fancysig'                => 'Чист потпис (без аутоматских веза)',
'tog-externaleditor'          => 'Користи спољашњи уређивач по подразумеваним подешавањима (само за експерте)',
'tog-externaldiff'            => 'Користи спољашњи програм за приказ разлика (само за експерте)',
'tog-showjumplinks'           => 'Омогући "скочи на" повезнице',
'tog-uselivepreview'          => 'Користи живи претпреглед (JavaScript) (експериментално)',
'tog-forceeditsummary'        => 'Упозори ме кад не унесем опис измене',
'tog-watchlisthideown'        => 'Сакриј моје измене са списка надгледања',
'tog-watchlisthidebots'       => 'Сакриј измене ботова са списка надгледања',
'tog-watchlisthideminor'      => 'Сакриј мале измене са списка надгледања',
'tog-nolangconversion'        => 'Искључи конверзију варијанти',
'tog-ccmeonemails'            => 'Пошаљи ми копије порука које шаљем другим корисницима путем е-поште',
'tog-diffonly'                => 'Не приказуј садржај странице испод разлике странице',
'tog-showhiddencats'          => 'Прикажи скривене категорије',

'underline-always'  => 'Увек',
'underline-never'   => 'Никад',
'underline-default' => 'По подешавањима браузера',

'skinpreview' => '(Преглед)',

# Dates
'sunday'        => 'недеља',
'monday'        => 'понедељак',
'tuesday'       => 'уторак',
'wednesday'     => 'среда',
'thursday'      => 'четвртак',
'friday'        => 'петак',
'saturday'      => 'субота',
'sun'           => 'нед',
'mon'           => 'пон',
'tue'           => 'уто',
'wed'           => 'сре',
'thu'           => 'чет',
'fri'           => 'пет',
'sat'           => 'суб',
'january'       => 'јануар',
'february'      => 'фебруар',
'march'         => 'март',
'april'         => 'април',
'may_long'      => 'мај',
'june'          => 'јун',
'july'          => 'јул',
'august'        => 'август',
'september'     => 'септембар',
'october'       => 'октобар',
'november'      => 'новембар',
'december'      => 'децембар',
'january-gen'   => 'јануара',
'february-gen'  => 'фебруара',
'march-gen'     => 'марта',
'april-gen'     => 'априла',
'may-gen'       => 'маја',
'june-gen'      => 'јуна',
'july-gen'      => 'јула',
'august-gen'    => 'августа',
'september-gen' => 'септембра',
'october-gen'   => 'октобра',
'november-gen'  => 'новембра',
'december-gen'  => 'децембра',
'jan'           => 'јан',
'feb'           => 'феб',
'mar'           => 'мар',
'apr'           => 'апр',
'may'           => 'мај',
'jun'           => 'јун',
'jul'           => 'јул',
'aug'           => 'авг',
'sep'           => 'сеп',
'oct'           => 'окт',
'nov'           => 'нов',
'dec'           => 'дец',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Категорија|Категорије|Категорије}} страница',
'category_header'                => 'Чланака у категорији "$1"',
'subcategories'                  => 'Поткатегорије',
'category-media-header'          => 'Мултимедијалних фајлова у категорији "$1"',
'category-empty'                 => "''Ова категорија тренутно не садржи чланке нити медије.''",
'hidden-categories'              => '{{PLURAL:$1|Скривена категорија|Скривене категорије|Скривених категорија}}',
'hidden-category-category'       => 'Скривене категорије', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Ова категорија има само следећу поткатегорију.|Ова категорија има {{PLURAL:$1|следећу поткатегорију|$1 следеће поткатегорије|$1 следећих поткатегорија}}, од укупно $2.}}',
'category-subcat-count-limited'  => 'Ова категорија садржи {{PLURAL:$1|следећу поткатегорију|$1 следеће поткатегорије}}.',
'category-article-count'         => '{{PLURAL:$2|Ова категорија садржи само следећу страну.|{{PLURAL:$1|страна је|$1 стране је|$1 страна је}} у овој категорији од укупно $2.}}',
'category-article-count-limited' => '{{PLURAL:$1|Следећа страна је|$1 следеће стране су|$1 следећих страна је}} у овој категорији.',
'category-file-count'            => '{{PLURAL:$2|Ова категорија садржи само следећи фајл.|{{PLURAL:$1|Следећи фајл је|$1 следећа фајла су|$1 следећих фајлова су}} у овој категорији, од укупно $2.}}',
'category-file-count-limited'    => 'Следећи {{PLURAL:$1|фајл је|$1 фајлови су}} у овој категорији.',
'listingcontinuesabbrev'         => 'наст.',

'mainpagetext'      => "<big>'''МедијаВики је успешно инсталиран.'''</big>",
'mainpagedocfooter' => 'Молимо видите [http://meta.wikimedia.org/wiki/Help:Contents кориснички водич] за информације о употреби вики софтвера.

== За почетак ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Помоћ у вези са подешавањима]
* [http://www.mediawiki.org/wiki/Manual:FAQ Најчешће постављена питања]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Мејлинг листа о издањима МедијаВикија]',

'about'          => 'О...',
'article'        => 'Чланак',
'newwindow'      => '(нови прозор)',
'cancel'         => 'Поништи',
'qbfind'         => 'Пронађи',
'qbbrowse'       => 'Прелиставај',
'qbedit'         => 'Уреди',
'qbpageoptions'  => 'Опције странице',
'qbpageinfo'     => 'Информације о страници',
'qbmyoptions'    => 'Моје опције',
'qbspecialpages' => 'Посебне странице',
'moredotdotdot'  => 'Још...',
'mypage'         => 'Моја страница',
'mytalk'         => 'Мој разговор',
'anontalk'       => 'Разговор за ову ИП адресу',
'navigation'     => 'Навигација',
'and'            => 'и',

# Metadata in edit box
'metadata_help' => 'Метаподаци:',

'errorpagetitle'    => 'Грешка',
'returnto'          => 'Повратак на $1.',
'tagline'           => 'Из {{SITENAME}}',
'help'              => 'Помоћ',
'search'            => 'претрага',
'searchbutton'      => 'Претрага',
'go'                => 'Иди',
'searcharticle'     => 'Иди',
'history'           => 'Историја странице',
'history_short'     => 'Историја',
'updatedmarker'     => 'ажурирано од моје последње посете',
'info_short'        => 'Информације',
'printableversion'  => 'Верзија за штампу',
'permalink'         => 'Пермалинк',
'print'             => 'Штампа',
'edit'              => 'Уреди',
'create'            => 'Креирај',
'editthispage'      => 'Уреди ову страницу',
'create-this-page'  => 'Креирај ову страну',
'delete'            => 'обриши',
'deletethispage'    => 'Обриши ову страницу',
'undelete_short'    => 'врати {{PLURAL:$1|једну обрисану измену|$1 обрисане измене|$1 обрисаних измена}}',
'protect'           => 'заштити',
'protect_change'    => 'промени',
'protectthispage'   => 'Заштити ову страницу',
'unprotect'         => 'Склони заштиту',
'unprotectthispage' => 'Склони заштиту са ове странице',
'newpage'           => 'Нова страница',
'talkpage'          => 'Разговор о овој страници',
'talkpagelinktext'  => 'Разговор',
'specialpage'       => 'Посебна страница',
'personaltools'     => 'Лични алати',
'postcomment'       => 'Пошаљи коментар',
'articlepage'       => 'Погледај чланак',
'talk'              => 'Разговор',
'views'             => 'Прегледи',
'toolbox'           => 'алати',
'userpage'          => 'Погледај корисничку страну',
'projectpage'       => 'Погледај страну пројекта',
'imagepage'         => 'Погледај страну слике',
'mediawikipage'     => 'Види страницу поруке',
'templatepage'      => 'Види страницу шаблона',
'viewhelppage'      => 'Види страницу помоћи',
'categorypage'      => 'Види страницу категорије',
'viewtalkpage'      => 'Погледај разговор',
'otherlanguages'    => 'Остали језици',
'redirectedfrom'    => '(Преусмерено са $1)',
'redirectpagesub'   => 'Страна преусмерења',
'lastmodifiedat'    => 'Ова страница је последњи пут измењена $2, $1.', # $1 date, $2 time
'viewcount'         => 'Овој страници је приступљено {{PLURAL:$1|једном|$1 пута|$1 пута}}.',
'protectedpage'     => 'Заштићена страница',
'jumpto'            => 'Скочи на:',
'jumptonavigation'  => 'навигација',
'jumptosearch'      => 'претрага',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'О пројекту {{SITENAME}}',
'aboutpage'            => 'Project:О',
'bugreports'           => 'Пријаве грешака',
'bugreportspage'       => 'Project:Пријаве грешака',
'copyright'            => 'Садржај је објављен под $1.',
'copyrightpagename'    => 'Ауторска права пројекта {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Ауторска права',
'currentevents'        => 'Тренутни догађаји',
'currentevents-url'    => 'Project:Тренутни догађаји',
'disclaimers'          => 'Одрицање одговорности',
'disclaimerpage'       => 'Project:Услови коришћења, правне напомене и одрицање одговорности',
'edithelp'             => 'Помоћ око уређивања',
'edithelppage'         => 'Help:Како се мења страна',
'faq'                  => 'НПП',
'faqpage'              => 'Project:НПП',
'helppage'             => 'Help:Садржај',
'mainpage'             => 'Главна страна',
'mainpage-description' => 'Главна страна',
'policy-url'           => 'Project:Политика приватности',
'portal'               => 'Радионица',
'portal-url'           => 'Project:Радионица',
'privacy'              => 'Политика приватности',
'privacypage'          => 'Project:Политика_приватности',

'badaccess'        => 'Грешка у дозволама',
'badaccess-group0' => 'Није вам дозвољено да извршите акцију коју сте покренули.',
'badaccess-group1' => 'Акција коју сте покренули је резеревисана за кориснике у групи $1.',
'badaccess-group2' => 'Акција коју сте покренули је резервисана за кориснике из једне од група $1.',
'badaccess-groups' => 'Акција коју сте покренули је резервисана за кориснике из једне од група $1.',

'versionrequired'     => 'Верзија $1 МедијаВикија је потребна',
'versionrequiredtext' => 'Верзија $1 МедијаВикија је потребна да би се користила ова страна. Погледајте [[Special:Version|верзију]]',

'ok'                      => 'да',
'retrievedfrom'           => 'Добављено из "$1"',
'youhavenewmessages'      => 'Имате $1 ($2).',
'newmessageslink'         => 'нових порука',
'newmessagesdifflink'     => 'најсвежије измене',
'youhavenewmessagesmulti' => 'Имате нових порука на $1',
'editsection'             => 'уреди',
'editold'                 => 'уреди',
'viewsourceold'           => 'погледај код',
'editsectionhint'         => 'Уреди део: $1',
'toc'                     => 'Садржај',
'showtoc'                 => 'прикажи',
'hidetoc'                 => 'сакриј',
'thisisdeleted'           => 'Погледај или врати $1?',
'viewdeleted'             => 'Погледај $1?',
'restorelink'             => '{{PLURAL:$1|једна обрисана измена|$1 обрисане измене|$1 обрисаних измена}}',
'feedlinks'               => 'Фид:',
'feed-invalid'            => 'Лош тип фида пријаве.',
'feed-unavailable'        => 'Фидови нису доступни на {{SITENAME}}',
'site-rss-feed'           => '$1 RSS фид',
'site-atom-feed'          => '$1 Atom фид',
'page-rss-feed'           => '"$1" RSS фид',
'page-atom-feed'          => '"$1" Atom фид',
'feed-atom'               => 'Атом',
'red-link-title'          => '$1 (није још написан)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Чланак',
'nstab-user'      => 'Корисничка страна',
'nstab-media'     => 'Медија',
'nstab-special'   => 'Посебна',
'nstab-project'   => 'Страна пројекта',
'nstab-image'     => 'Слика',
'nstab-mediawiki' => 'Порука',
'nstab-template'  => 'Шаблон',
'nstab-help'      => 'Помоћ',
'nstab-category'  => 'Категорија',

# Main script and global functions
'nosuchaction'      => 'Нема такве акције',
'nosuchactiontext'  => 'Акцију наведену у УРЛ-у вики софтвер
није препознао.',
'nosuchspecialpage' => 'Нема такве посебне странице',
'nospecialpagetext' => "<big>'''Тражили сте непостојећу посебну страницу.'''</big>

Списак свих посебних страница се може наћи на [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Грешка',
'databaseerror'        => 'Грешка у бази',
'dberrortext'          => 'Десила се синтаксна грешка упита базе.
Ово можда указује на грешке у софтверу.
Последњи покушани упит је био:
<blockquote><tt>$1</tt></blockquote>
из функције "<tt>$2</tt>".
MySQL је вратио грешку "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Десила се синтаксна грешка упита базе.
Последњи покушани упит је био:
"$1"
из функције "$2".
MySQL је вратио грешку "$3: $4".',
'noconnect'            => 'Жалимо! Вики има неке техничке потешкоће, и не може да се повеже се сервером базе.<br /> 
$1',
'nodb'                 => 'Не могу да изаберем базу $1',
'cachederror'          => 'Ово је кеширана копија захтеване странице, и можда није ажурирана.',
'laggedslavemode'      => 'Упозорење: могуће је да страна није скоро ажурирана.',
'readonly'             => 'База је закључана',
'enterlockreason'      => 'Унесите разлог за закључавање, укључујући процену
времена откључавања',
'readonlytext'         => 'База података је тренутно закључана за нове
уносе и остале измене, вероватно због рутинског одржавања,
после чега ће бити враћена у уобичајено стање.

Администратор који ју је закључао понудио је ово објашњење: $1',
'missing-article'      => 'Текст стране под именом "$1" ($2) није пронађен.

Узрок за ову грешку је обично застарели diff или веза ка обрисаној верзији чланка.

Ако то није случај, можда сте пронашли баг у софтверу. 
У том случају, пријавите грешку [[Special:ListUsers/sysop|администратору]] уз одговарајући линк.',
'missingarticle-rev'   => '(ревизија#: $1)',
'missingarticle-diff'  => '(Раз: $1, $2)',
'readonly_lag'         => 'База података је аутоматски закључана док слејв сервери не сустигну мастер',
'internalerror'        => 'Интерна грешка',
'internalerror_info'   => 'Интерна грешка: $1',
'filecopyerror'        => 'Не могу да ископирам фајл "$1" на "$2".',
'filerenameerror'      => 'Не могу да преименујем фајл "$1" у "$2".',
'filedeleteerror'      => 'Не могу да обришем фајл "$1".',
'directorycreateerror' => 'Не могу да направим директоријум "$1".',
'filenotfound'         => 'Не могу да нађем фајл "$1".',
'fileexistserror'      => 'Не могу да пишем по фајлу "$1": фајл постоји',
'unexpected'           => 'Неочекивана вредност: "$1"="$2".',
'formerror'            => 'Грешка: не могу да пошаљем упитник',
'badarticleerror'      => 'Ова акција не може бити извршена на овој страници.',
'cannotdelete'         => 'Не могу да обришем наведену страницу или фајл. (Могуће је да је неко други већ обрисао.)',
'badtitle'             => 'Лош наслов',
'badtitletext'         => 'Захтевани наслов странице је био неисправан, празан или
неисправно повезан међујезички или интервики наслов. Можда садржи један или више карактера који не могу да се употребљавају у насловима.',
'perfdisabled'         => 'Жао нам је! Ова могућност је привремено онемогућена јер успорава базу до те мере да више нико не може да користи вики.',
'perfcached'           => 'Следећи подаци су кеширани и не морају бити у потпуности ажурирани.',
'perfcachedts'         => 'Следећи подаци су кеширани и последњи пут су ажурирани $1.',
'querypage-no-updates' => 'Ажурирање ове странице је тренутно онемогућено. Подаци овде неће бити освежени одмах.',
'wrong_wfQuery_params' => 'Нетачни параметри за wfQuery()<br />
Функција: $1<br />
Претрага: $2',
'viewsource'           => 'погледај код',
'viewsourcefor'        => 'за $1',
'actionthrottled'      => 'Акцији је смањена брзина.',
'actionthrottledtext'  => 'У циљу борбе против спама, нисте у могућности да учините то више пута у кратком времену, а управо сте прешли тај лимит. Покушајте поново за пар минута.',
'protectedpagetext'    => 'Ова страница је закључана како се не би вршиле измене на њој.',
'viewsourcetext'       => 'Можете да прегледате и копирате садржај ове стране:',
'protectedinterface'   => 'Ова страна пружа текст интерфејса за софтвер и закључана је како би се спречила злоупотреба.',
'editinginterface'     => "'''Упозорење:''' Уређујете страну која се користи да пружи текст за интерфејс овог софтвера. 
Измене на овој страни ће утицати на изглед корисничког интерфејса за остале кориснике.
За преводе, посетите [http://translatewiki.net/wiki/Main_Page?setlang=sr_ec Betawiki], пројекат локализације МедијаВики софтвера.",
'sqlhidden'            => '(SQL претрага сакривена)',
'cascadeprotected'     => 'Ова страница је закључана и њено уређивање је онемогућено јер је укључена у садржај {{PLURAL:$1|следеће стране|следећих страна}}, који је заштићен са опцијом "преносиве" заштите:
$2',
'namespaceprotected'   => "Немате овлашћења да уређујете странице у '''$1''' именском простору.",
'customcssjsprotected' => 'Немате овлашћења да уређујете ову страницу јер садржи лична подешавања другог корисника.',
'ns-specialprotected'  => 'Странице у {{ns:special}} именском простору не могу се уређивати.',
'titleprotected'       => "Овај наслов је блокиран за прављење.
Блокирао га је [[User:$1|$1]] а дати разлог је ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Лоша конфигурација због неодговарајућег скенера за вирус: <i>$1</i>',
'virus-scanfailed'     => 'скенирање пропало (код $1)',
'virus-unknownscanner' => 'непознати антивирус:',

# Login and logout pages
'logouttitle'                => 'Одјави се',
'logouttext'                 => '<strong>Сада сте одјављени.</strong><br />
Можете да наставите да користите пројекат {{SITENAME}} анонимно, или се поново пријавити као други корисник. Обратите пажњу да неке странице могу наставити да се приказују као да сте још увек пријављени, док не очистите кеш свог браузера.',
'welcomecreation'            => '== Добродошли, $1! ==

Ваш налог је креиран.
Не заборавите да прилагодите себи своја [[Special:Preferences|{{SITENAME}} подешавања]].',
'loginpagetitle'             => 'Пријављивање',
'yourname'                   => 'Корисничко име',
'yourpassword'               => 'Ваша лозинка',
'yourpasswordagain'          => 'Поновите лозинку',
'remembermypassword'         => 'Запамти ме',
'yourdomainname'             => 'Ваш домен',
'externaldberror'            => 'Дошло је или до грешке при спољашњој аутентификацији базе података или вам није дозвољено да ажурирате свој спољашњи налог.',
'loginproblem'               => '<b>Било је проблема са вашим пријављивањем.</b><br />Покушајте поново!',
'login'                      => 'Пријави се',
'nav-login-createaccount'    => 'Региструј се / Пријави се',
'loginprompt'                => "Морате да имате омогућене колачиће (''cookies'') да бисте се пријавили на {{SITENAME}}.",
'userlogin'                  => 'Региструј се / Пријави се',
'logout'                     => 'Одјави се',
'userlogout'                 => 'Одјави се',
'notloggedin'                => 'Нисте пријављени',
'nologin'                    => 'Немате налог? $1.',
'nologinlink'                => 'Направите налог',
'createaccount'              => 'Направи налог',
'gotaccount'                 => 'Имате налог? $1.',
'gotaccountlink'             => 'Пријавите се',
'createaccountmail'          => 'е-поштом',
'badretype'                  => 'Лозинке које сте унели се не поклапају.',
'userexists'                 => 'Корисничко име које сте унели већ је у употреби.
Молимо изаберите друго име.',
'youremail'                  => 'Адреса ваше е-поште *',
'username'                   => 'Корисничко име:',
'uid'                        => 'Кориснички ИД:',
'prefs-memberingroups'       => 'Члан {{PLURAL:$1|групе|група}}:',
'yourrealname'               => 'Ваше право име *',
'yourlanguage'               => 'Језик:',
'yourvariant'                => 'Варијанта:',
'yournick'                   => 'Надимак:',
'badsig'                     => 'Грешка у потпису, проверите HTML тагове.',
'badsiglength'               => 'Потпис је предугачак. 
Мора бити испод $1 {{PLURAL:$1|карактер|карактера}}.',
'email'                      => 'Е-пошта',
'prefs-help-realname'        => '* Право име (опционо): ако изаберете да дате име, ово ће бити коришћено за приписивање за ваш рад.',
'loginerror'                 => 'Грешка при пријављивању',
'prefs-help-email'           => 'Е-пошта је опциона, али омогућује осталима да вас контактирају преко ваше корисничке стране или стране разговора са корисником без потребе да одајете свој идентитет.',
'prefs-help-email-required'  => 'Адреса е-поште је потребна.',
'nocookiesnew'               => "Кориснички налог је направљен, али нисте пријављени. {{SITENAME}} користи колачиће (''cookies'') да би се корисници пријавили. Ви сте онемогућили колачиће на свом рачунару. Молимо омогућите их, а онда се пријавите са својим новим корисничким именом и лозинком.",
'nocookieslogin'             => "{{SITENAME}} користи колачиће (''cookies'') да би се корисници пријавили. Ви сте онемогућили колачиће на свом рачунару. Молимо омогућите их и покушајте поново са пријавом.",
'noname'                     => 'Нисте изабрали исправно корисничко име.',
'loginsuccesstitle'          => 'Пријављивање успешно',
'loginsuccess'               => "'''Сада сте пријављени на {{SITENAME}} као \"\$1\".'''",
'nosuchuser'                 => 'Не постоји корисник са именом "$1".
Проверите да ли сте добро написали или [[Special:Userlogin/signup|направите нови кориснички налог]].',
'nosuchusershort'            => 'Не постоји корисник са именом "<nowiki>$1</nowiki>". Проверите да ли сте добро написали.',
'nouserspecified'            => 'Морате да назначите корисничко име.',
'wrongpassword'              => 'Лозинка коју сте унели је неисправна. Молимо покушајте поново.',
'wrongpasswordempty'         => 'Лозинка коју сте унели је празна. Молимо покушајте поново.',
'passwordtooshort'           => 'Ваша шифра је неисправна или превише кратка. 
Мора да има бар {{PLURAL:$1|1 карактер|$1 карактера}} и да буде различита од вашег корисничког имена.',
'mailmypassword'             => 'Пошаљи ми нову лозинку',
'passwordremindertitle'      => '{{SITENAME}} подсетник за шифру',
'passwordremindertext'       => 'Неко (вероватно ви, са ИП адресе $1) је захтевао да вам пошаљемо нову
шифру за пријављивање на {{SITENAME}} ($4). 
Привремена шифра за корисника „$2“ је сада „$3“. Уколико је ово
Ваш захтев, сада се пријавите и изаберите нову шифу.

Уколико је неко други захтевао промену шифре, или сте ви заборавили вашу 
шифру и више не желите да је мењате, можете игнорисати ову поруку и
наставити користити вашу стару.',
'noemail'                    => 'Не постоји адреса е-поште за корисника "$1".',
'passwordsent'               => 'Нова шифра је послата на адресу е-поште корисника "$1".
Молимо пријавите се пошто је примите.',
'blocked-mailpassword'       => 'Вашој ИП адреси је блокиран приступ уређивању, из ког разлога није могуће користити функцију подсећања лозинке, ради превенције извршења недозвољене акције.',
'eauthentsent'               => 'Е-пошта за потврду је послата на назначену адресу е-поште. Пре него што се било која друга е-пошта пошаље на налог, мораћете да пратите упутства у е-пошти, да бисте потврдили да је налог заиста ваш.',
'throttled-mailpassword'     => 'Подсетник лозинке вам је већ послао једну поруку у {{PLURAL:$1|протеклом сату|последњих $1 сата|последњих $1 сати}}. 
Ради превенције извршења недозвољене акције, подсетник шаље само једну поруку у року од {{PLURAL:$1|једног сата|$1 сата|$1 сати}}.',
'mailerror'                  => 'Грешка при слању е-поште: $1',
'acct_creation_throttle_hit' => 'Жао нам је, већ сте направили $1 корисничка имена. Више није дозвољено.',
'emailauthenticated'         => 'Ваша адреса е-поште је потврђена: $1.',
'emailnotauthenticated'      => 'Ваша адреса е-поште још увек није потврђена. Е-пошта неће бити послата ни за једну од следећих могућности.',
'noemailprefs'               => 'Назначите адресу е-поште како би ове могућности радиле.',
'emailconfirmlink'           => 'Потврдите вашу адресу е-поште',
'invalidemailaddress'        => 'Адреса е-поште не може бити примљена јер изгледа није правилног формата. 
Молимо унесите добро-форматирану адресу или испразните то поље.',
'accountcreated'             => 'Налог је направљен',
'accountcreatedtext'         => 'Кориснички налог за $1 је направљен.',
'createaccount-title'        => 'Прављење корисничког налога за {{SITENAME}}',
'createaccount-text'         => 'Неко је направио налог са вашом адресом е-поште на {{SITENAME}} ($4) под именом „$2”, са лозинком „$3”.
Пријавите се и промените вашу лозинку.

Можете игронисати ову поруку, уколико је налог направљен грешком.',
'loginlanguagelabel'         => 'Језик: $1',

# Password reset dialog
'resetpass'               => 'Ресетујте корисничку лозинку',
'resetpass_announce'      => 'Пријавили сте се са привременом лозинком послатом електронском поштом. Да бисте завршили са пријавом, морате подесити нову лозинку овде:',
'resetpass_header'        => 'Ресетовање лозинке',
'resetpass_submit'        => 'Подеси лозинку и пријави се',
'resetpass_success'       => 'Ваша лозинка је успешно промењена! Пријављивање у току...',
'resetpass_bad_temporary' => 'Привремена лозинка не одговара. Могуће је да сте већ успешно променили лозинку или да сте затражили да вам се пошаље нова привремена лозинка.',
'resetpass_forbidden'     => 'Лозинке не могу бити промењене на {{SITENAME}}',
'resetpass_missing'       => 'Недостају подаци формулара.',

# Edit page toolbar
'bold_sample'     => 'подебљан текст',
'bold_tip'        => 'подебљан текст',
'italic_sample'   => 'курзиван текст',
'italic_tip'      => 'курзиван текст',
'link_sample'     => 'наслов везе',
'link_tip'        => 'унутрашња веза',
'extlink_sample'  => 'http://www.example.com опис адресе',
'extlink_tip'     => 'спољашња веза (не заборавите префикс http://)',
'headline_sample' => 'Наслов',
'headline_tip'    => 'Наслов другог нивоа',
'math_sample'     => 'Овде унесите формулу',
'math_tip'        => 'Математичка формула (LaTeX)',
'nowiki_sample'   => 'Додај неформатирани текст овде',
'nowiki_tip'      => 'Игнориши вики форматирање',
'image_sample'    => 'Пример.jpg',
'image_tip'       => 'Уклопљена слика',
'media_sample'    => 'име_медија_фајла.mp3',
'media_tip'       => 'Путања ка мултимедијалном фајлу',
'sig_tip'         => 'Ваш потпис са тренутним временом',
'hr_tip'          => 'Хоризонтална линија',

# Edit pages
'summary'                          => 'Опис измене',
'subject'                          => 'Тема/Наслов',
'minoredit'                        => 'Ово је мала измена',
'watchthis'                        => 'Надгледај овај чланак',
'savearticle'                      => 'Сними страницу',
'preview'                          => 'Претпреглед',
'showpreview'                      => 'Прикажи претпреглед',
'showlivepreview'                  => 'Живи претпреглед',
'showdiff'                         => 'Прикажи промене',
'anoneditwarning'                  => "'''Пажња:''' Нисте пријављени. Ваша ИП адреса ће бити забележена у историји измена ове стране.",
'missingsummary'                   => "'''Опомена:''' Нисте унели опис измене. Уколико кликнете Сними страницу поново, ваше измене ће бити снимљене без описа.",
'missingcommenttext'               => 'Унестите коментар доле.',
'missingcommentheader'             => "'''Подсетник:''' Нисте навели наслов овог коментара. Уколико кликнете ''Сними поново'', ваш коментар ће бити снимљен без наслова.",
'summary-preview'                  => 'Претпреглед описа измене',
'subject-preview'                  => 'Претпреглед предмета/одељка',
'blockedtitle'                     => 'Корисник је блокиран',
'blockedtext'                      => "<big>'''Ваше корисничко име или ИП адреса је блокирана.'''</big>

Блокирање је извршеио $1. 
Дати разлог је следећи: ''$2''.

* Почетак блока: $8
* Истек блока: $6
* Блокирани: $7

Можете контактирати $1 или другог [[{{MediaWiki:Grouppage-sysop}}|администратора]] да бисте разговарали о блокади.
Не можете користити „Пошаљи е-пошту овом кориснику“ функцију уколико нисте регистровали важећу адресу за е-пошту у вашим [[Special:Preferences|подешавањима]].
Ваша ИП адреса је $3, и ИД број блока је #$5. 
При сваком захтеву наведите оба, или само један податак.",
'autoblockedtext'                  => 'Ваша IP адреса је аутоматски блокирана јер ју је употребљавао други корисник, кога је блокирао $1.
Дат разлог је:

:\'\'$2\'\'

* Почетак блокаде: $8
* Блокада истиче: $6
* Блокирани: $7

Можете контактирати $1 или неког другог
[[{{MediaWiki:Grouppage-sysop}}|администратора]] да бисте разјаснили ову блокаду.

Имајте у виду да не можете да користите опцију "пошаљи е-пошту овом кориснику" уколико нисте регистровали валидну адресу електронске поште
у вашим [[Special:Preferences|корисничким подешавањима]] и уколико вам блокадом није онемогућена употреба ове опције.

ИП адреса која је блокирана је $3, а ID ваше блокаде је $5. 
Молимо вас наведите овај ID број приликом прављења било каквих упита.',
'blockednoreason'                  => 'није дат разлог',
'blockedoriginalsource'            => "Извор '''$1''' је приказан испод:",
'blockededitsource'                => "Текст '''ваших измена''' за '''$1''' је приказан испод:",
'whitelistedittitle'               => 'Обавезно је пријављивање за мењање',
'whitelistedittext'                => 'Морате да се $1 да бисте мењали странице.',
'confirmedittitle'                 => 'Потребна је потврда адресе е-поштe за уређивање',
'confirmedittext'                  => 'Морате потврдити вашу адресу е-поште пре уређивања страна. Молимо поставите и потврдите адресу ваше е-поште преко ваших [[Special:Preferences|корисничких подешавања]].',
'nosuchsectiontitle'               => 'Не постоји такав одељак',
'nosuchsectiontext'                => 'Покушали сте да уредите одељак који не постоји. Како не постоји одељак $1, нема ни места за чување ваше измене.',
'loginreqtitle'                    => 'Потребно пријављивање',
'loginreqlink'                     => 'пријава',
'loginreqpagetext'                 => 'Морате $1 да бисте видели остале стране.',
'accmailtitle'                     => 'Лозинка је послата.',
'accmailtext'                      => 'Лозинка за налог "$1" је послата на адресу $2.',
'newarticle'                       => '(Нови)',
'newarticletext'                   => "Пратили сте везу ка страници која још не постоји.
Да бисте је направили, почните да куцате у пољу испод
(погледајте [[{{ns:help}}:Садржај|помоћ]] за више информација).
Ако сте дошли овде грешком, само кликните дугме '''back''' дугме вашег браузера.",
'anontalkpagetext'                 => '---- Ово је страница за разговор за анонимног корисника који још није направио налог, или га не користи. 
Због тога морамо да користимо бројчану ИП адресу како бисмо идентификовали њега или њу. 
Такву адресу може делити више корисника. 
Ако сте анонимни корисник и мислите да су вам упућене небитне примедбе, молимо вас да [[Special:UserLogin/signup|направите налог]] или [[Special:UserLogin|се пријавите]] да бисте избегли будућу забуну са осталим анонимним корисницима.',
'noarticletext'                    => 'Тренутно не постоји чланак под тим именом, можете [[Special:Search/{{PAGENAME}}|тражити ову страницу]] у другим чланцима или је [{{fullurl:{{FULLPAGENAME}}|action=edit}} уредити].',
'userpage-userdoesnotexist'        => 'Налог "$1" није регистрован. Проверите да ли желите да правите/уређујете ову страницу.',
'clearyourcache'                   => "'''Запамтите:''' Након снимања, можда морате очистити кеш вашег браузера да бисте видели промене. '''Mozilla / Firefox / Safari:''' држите ''Shift'' док кликћете ''Reload'' или притисните  ''Shift+Ctrl+R'' (''Cmd-Shift-R'' на ''Apple Mac'' машини); '''IE:''' држите ''Ctrl'' док кликћете ''Refresh'' или притисните ''Ctrl-F5''; '''Konqueror:''': само кликните ''Reload'' дугме или притисните ''F5''; корисници '''Оpera''' браузера можда морају да у потпуности очисте свој кеш преко ''Tools→Preferences''.",
'usercssjsyoucanpreview'           => "<strong>Савет:</strong> Кориситите 'Прикажи претпреглед' дугме да тестирате свој нови CSS/JS пре снимања.",
'usercsspreview'                   => "'''Запамтите ово је само претпреглед вашег CSS.'''
'''Још увек није снимљен!'''",
'userjspreview'                    => "'''Запамтите ово је само претпреглед ваше JavaScript-е и да још увек није снимљен!'''",
'userinvalidcssjstitle'            => "'''Пажња:''' Не постоји кожа \"\$1\". Запамтите да личне .css и .js странице користе мала почетна слова, нпр. {{ns:user}}:Петар/monobook.css а не {{ns:user}}:Петар/Monobook.css.",
'updated'                          => '(Ажурирано)',
'note'                             => '<strong>Напомена:</strong>',
'previewnote'                      => '<strong>Ово само претпреглед; измене још нису сачуване!</strong>',
'previewconflict'                  => 'Овај претпреглед осликава како ће текст у
текстуалном пољу изгледати ако се одлучите да га снимите.',
'session_fail_preview'             => '<strong>Жао нам је! Нисмо могли да обрадимо вашу измену због губитка података сеансе. Молимо покушајте касније. Ако и даље не ради, покушајте да се одјавите и поново пријавите.</strong>',
'session_fail_preview_html'        => "<strong>Жао нам је! Нисмо могли да обрадимо вашу измену због губитка података сесије.</strong>

''Због тога што {{SITENAME}} има омогућен сиров HTML, претпреглед је сакривен као предострожност против JavaScript напада.''

<strong>Ако сте покушали да направите праву измену, молимо покушајте поново. 
Ако и даље не ради, покушајте да се [[Special:UserLogout|одјавите]] и поново пријавите.</strong>",
'token_suffix_mismatch'            => '<strong>Ваша измена је одбијена зато што је ваш клијент окрњио интерпункцијске знаке на крају токена. Ова измена је одбијена због заштите конзистентности текста стране. Понекад се ово догађа кад се користи баговит прокси сервис.</strong>',
'editing'                          => 'Уређујете $1',
'editingsection'                   => 'Уређујете $1 (део)',
'editingcomment'                   => 'Уређујете $1 (коментар)',
'editconflict'                     => 'Сукобљене измене: $1',
'explainconflict'                  => 'Неко други је променио ову страницу откад сте ви почели да је мењате.
Горње текстуално поље садржи текст странице какав тренутно постоји.
Ваше измене су приказане у доњем тексту.
Мораћете да унесете своје промене у постојећи текст.
<b>Само</b> текст у горњем текстуалном пољу ће бити снимљен када
притиснете "Сними страницу".<br />',
'yourtext'                         => 'Ваш текст',
'storedversion'                    => 'Ускладиштена верзија',
'nonunicodebrowser'                => '<strong>УПОЗОРЕЊЕ: Ваш браузер не подржава уникод. Молимо промените га пре него што почнете са уређивањем чланка.</strong>',
'editingold'                       => '<strong>ПАЖЊА: Ви мењате старију ревизију ове странице.
Ако је снимите, све промене учињене од ове ревизије биће изгубљене.</strong>',
'yourdiff'                         => 'Разлике',
'copyrightwarning'                 => 'Молимо вас да обратите пажњу да се за сваки допринос {{SITENAME}} сматра да је објављен под $2 лиценцом (погледајте $1 за детаље). Ако не желите да се ваше писање мења и редистрибуира без ограничења, онда га немојте слати овде.<br />
Такође нам обећавате да сте га сами написали, или прекопирали из извора који је у јавном власништву или сличног слободног извора.
<strong>НЕ ШАЉИТЕ РАДОВЕ ЗАШТИЋЕНЕ АУТОРСКИМ ПРАВИМА БЕЗ ДОЗВОЛЕ!</strong>',
'copyrightwarning2'                => 'Напомена: Сви доприноси {{SITENAME}} могу да се мењају или уклоне од стране других корисника. Ако не желите да се ваши доприноси немилосрдно мењају, не шаљите их овде.<br />
Такође нам обећавате да сте ово сами написали или прекопирали из извора у јавном власништву или сличног слободног извора (видите $1 за детаље).
<strong>НЕ ШАЉИТЕ РАДОВЕ ЗАШТИЋЕНЕ АУТОРСКИМ ПРАВИМА БЕЗ ДОЗВОЛЕ!</strong>',
'longpagewarning'                  => '<strong>ПАЖЊА: Ова страница има $1 килобајта; неки браузери имају проблема са уређивањем страна које имају близу или више од 32 килобајта. Молимо вас да размотрите разбијање странице на мање делове.</strong>',
'longpageerror'                    => '<strong>ГРЕШКА: Текст који снимате је велик $1 килобајта, што је веће од максимално дозвољене величине која износи $2 килобајта. Немогуће је снимити страницу.</strong>',
'readonlywarning'                  => '<strong>ПАЖЊА: База је управо закључана због одржавања,
тако да сада нећете моћи да снимите своје измене. Можда би било добро да ископирате текст у неки едитор текста и снимите га за касније.</strong>',
'protectedpagewarning'             => "<strong>'''ПАЖЊА:''' Ова страница је закључана тако да само корисници са администраторским привилегијама могу да је мењају.</strong>",
'semiprotectedpagewarning'         => "'''Напомена:''' Ова страна је закључана тако да је само регистровани корисници могу уређивати.",
'cascadeprotectedwarning'          => "'''Упозорење:''' Ова страница је заштићена тако да је могу уређивати само корисници са администраторским привилегијама јер је укључена у преносиву заштиту {{PLURAL:$1|следеће стране|следећих страна}}:",
'titleprotectedwarning'            => '<strong>ПРАЖЊА: Ова страница је закључана тако да само неки корисници могу да је направе.</strong>',
'templatesused'                    => 'Шаблони који се користе на овој страници:',
'templatesusedpreview'             => 'Шаблони који се користе у овом претпрегледу:',
'templatesusedsection'             => 'Шаблони који се користе у овом одељку:',
'template-protected'               => '(заштићено)',
'template-semiprotected'           => '(полузаштићено)',
'hiddencategories'                 => 'Ова страна је члан {{PLURAL:$1|1 скривене категорије|$1 скривене категорије|$1 скривених категорија}}:',
'edittools'                        => '<!-- Текст одавде ће бити показан испод формулара за уређивање и слање слика. -->',
'nocreatetitle'                    => 'Прављење странице лимитирано',
'nocreatetext'                     => 'На {{SITENAME}} је забрањено прављење нових чланака.
Можете се вратити назад и уређивати постојећи чланак, или [[Special:UserLogin|се пријавите или направите налог]].',
'nocreate-loggedin'                => 'Немате овлашћења да правите нове стране на {{SITENAME}}.',
'permissionserrors'                => 'Грешке у овлашћењима',
'permissionserrorstext'            => 'Немате овлашћење да урадите то из {{PLURAL:$1|следећег|следећих}} разлога:',
'permissionserrorstext-withaction' => 'Немате дозволу да $2, због следећег: {{PLURAL:$1|разлога|разлога}}:',
'recreate-deleted-warn'            => "'''Упозорење: Поново правите страницу која је претходно обрисана.'''

Требало би да размотрите да ли је прикладно да наставите са уређивањем ове странице.
Дневник брисања ове стране је приказан овде:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Упозорење: Ова страна садржи превише скупих позива функције парсирања.

Требало би да има мање од $2, а сада је $1.',
'expensive-parserfunction-category'       => 'Стране са превише скупих позива функција парсирања.',
'post-expand-template-inclusion-warning'  => 'Упозорење: Величина укљученог шаблона је превелика. Неки шаблони неће бити укључени.',
'post-expand-template-inclusion-category' => 'Стране на којима је прекорачена величина укључивања шаблона.',
'post-expand-template-argument-warning'   => 'Упозорење: Ова страна садржи бар један превелики аргумент шаблона, који ће бити изостављени.',
'post-expand-template-argument-category'  => 'Стране са изостављеним аргументима шаблона.',

# "Undo" feature
'undo-success' => 'Ова измена може да се врати. Проверите разлике испод како би проверили да је ово то што желите да урадите, тада снимите измене како би завршили враћање измене.',
'undo-failure' => 'Измена не може бити опорављена услед сукобљених међуизмена.',
'undo-norev'   => 'Измена не може бити опорављена зато што не постоји или је обрисана.',
'undo-summary' => 'Вратите ревизију $1 корисника [[Special:Contributions/$2|$2]] ([[User talk:$2|разговор]])',

# Account creation failure
'cantcreateaccounttitle' => 'Не може да се направи налог',
'cantcreateaccount-text' => "Прављење налога са ове ИП адресе ('''$1''') је блокирао [[User:$3|$3]].

Разлог који је дао $3 је ''$2''",

# History pages
'viewpagelogs'        => 'Протоколи за ову страну',
'nohistory'           => 'Не постоји историја измена за ову страницу.',
'revnotfound'         => 'Ревизија није пронађена',
'revnotfoundtext'     => 'Старија ревизија ове странице коју сте затражили није нађена.
Молимо вас да проверите УРЛ који сте употребили да бисте приступили овој страници.',
'currentrev'          => 'Тренутна ревизија',
'revisionasof'        => 'Ревизија од $1',
'revision-info'       => 'Ревизија од $1; $2',
'previousrevision'    => '← Претходна ревизија',
'nextrevision'        => 'Следећа ревизија →',
'currentrevisionlink' => 'Тренутна ревизија',
'cur'                 => 'трен',
'next'                => 'след',
'last'                => 'посл',
'page_first'          => 'прво',
'page_last'           => 'последње',
'histlegend'          => 'Одабирање разлика: одаберите кутијице ревизија за упоређивање и притисните ентер или дугме на дну.<br />
Објашњење: (трен) = разлика са тренутном верзијом,
(посл) = разлика са претходном верзијом, М = мала измена',
'deletedrev'          => '[обрисан]',
'histfirst'           => 'Најраније',
'histlast'            => 'Последње',
'historysize'         => '({{PLURAL:$1|1 бајт|$1 бајта|$1 бајтова}})',
'historyempty'        => '(празно)',

# Revision feed
'history-feed-title'          => 'Контролна историја',
'history-feed-description'    => 'Историја ревизија за ову страну на викију',
'history-feed-item-nocomment' => '$1, $2', # user at time
'history-feed-empty'          => 'Тражена страна не постоји.
Могуће да је обрисана из викија или преименована.
Покушајте [[Special:Search|да претражите вики]] за релевантне нове стране.',

# Revision deletion
'rev-deleted-comment'         => '(коментар уклоњен)',
'rev-deleted-user'            => '(корисничко име уклоњено)',
'rev-deleted-event'           => '(историја уклоњена)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Ревизија ове странице је уклоњена за јавне архиве.
Могуће да има више детаља у [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} историји брисања].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Ревизија ове странице је уклоњена за јавне архиве.
Као администратор, можете да је поглеате;
Могуће да има више детаља у [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} историји брисања].</div>',
'rev-delundel'                => 'покажи/сакриј',
'revisiondelete'              => 'Обриши/врати ревизије',
'revdelete-nooldid-title'     => 'Нема одабране ревизије',
'revdelete-nooldid-text'      => 'Нисте одабрали жељену ревизију или ревизије како бисте укључили ове функције.',
'revdelete-selected'          => "{{PLURAL:$2|Одабрана ревизија|Одабране ревизије}} за '''[[:$1]]'''",
'logdelete-selected'          => '{{PLURAL:$1|Изабрани догађај из историје|Изабрани догађаји из историје}}:',
'revdelete-text'              => 'Обрисане ревизије ће се и даље појављивати на историји странице, али ће њихов садржај бити скривен у јавности.

Остали администратори на {{SITENAME}} ће и даље имати могућност да виде скривени садржај и моћи ће да га врате поново путем ове исте команде, све уколико нису примењене додатне рестрикције оператора сајта.',
'revdelete-legend'            => 'Постави видне рестрикције',
'revdelete-hide-text'         => 'Сакриј текст ревизије',
'revdelete-hide-name'         => 'Сакриј акцију и циљ.',
'revdelete-hide-comment'      => 'Сакриј опис измене',
'revdelete-hide-user'         => 'Сакриј корисничко име/ИП адресу корисника који је уређивао страницу',
'revdelete-hide-restricted'   => 'Примени ове рестрикције за администраторе и закључај.',
'revdelete-suppress'          => 'Сакриј податке од сисопа и осталих.',
'revdelete-hide-image'        => 'Сакриј садржај фајла',
'revdelete-unsuppress'        => 'Уклони забране над опорављеним верзијама.',
'revdelete-log'               => 'Коментар лога:',
'revdelete-submit'            => 'Примени на изабране ревизије',
'revdelete-logentry'          => 'промењен приказ ревизије за [[$1]]',
'logdelete-logentry'          => 'промењена видност догађаја за страну [[$1]]',
'revdelete-success'           => "'''Видност измене је успешно подешена.'''",
'logdelete-success'           => "'''Видност лога је успешно подешена.'''",
'revdel-restore'              => 'Промена видности',
'pagehist'                    => 'Историја стране',
'deletedhist'                 => 'Обрисана историја',
'revdelete-content'           => 'садржај',
'revdelete-summary'           => 'опис измене',
'revdelete-uname'             => 'корисничко име',
'revdelete-restricted'        => 'ограничења за сисопе су примењена',
'revdelete-unrestricted'      => 'ограничења за сисопе су уклоњена',
'revdelete-hid'               => 'сакривено: $1',
'revdelete-unhid'             => 'откривено: $1',
'revdelete-log-message'       => '$1 за $2 {{PLURAL:$2|ревизију|ревизије|ревизија}}',
'logdelete-log-message'       => '$1 за $2 {{PLURAL:$2|догађај|догађаја}}',

# Suppression log
'suppressionlog'     => 'Лог сакривања',
'suppressionlogtext' => 'Испод се налази списак блокова и обрисаних страна који су сакривени од сисопа. Погледај [[Special:IPBlockList|списак блокираних ИП адреса]] за списак тренутно важећих банова и блокова.',

# History merging
'mergehistory'                     => 'Уједини историје страна.',
'mergehistory-header'              => 'Ова страна омогућава спајање верзија једне стране у другу. Уверите се претходно да ће ова измена одржати континуитет историје стране.',
'mergehistory-box'                 => 'Спој верзије две стране:',
'mergehistory-from'                => 'Изворна страница:',
'mergehistory-into'                => 'Жељена страница:',
'mergehistory-list'                => 'Историја измена која се може спојити.',
'mergehistory-merge'               => 'Следеће верзије стране [[:$1]] могу се спојити са [[:$2]]. Користи колону с "радио дугмићима" за спајање само оних верзија које су направљене пре датог времена. Коришћење навигационих линкова ће поништити ову колону.',
'mergehistory-go'                  => 'Прикажи измене које се могу спојити.',
'mergehistory-submit'              => 'Спој измене.',
'mergehistory-empty'               => 'Нема измена које се могу спојити.',
'mergehistory-success'             => '$3 {{PLURAL:$3|ревизија|ревизије|ревизија}} стране [[:$1]] успешно спојено у [[:$2]].',
'mergehistory-fail'                => 'Није могуће спојити верзије; провери параметре стране и времена.',
'mergehistory-no-source'           => 'Изворна страница $1 не постоји.',
'mergehistory-no-destination'      => 'Жељена страница $1 не постоји.',
'mergehistory-invalid-source'      => 'Име изворне странице мора бити исправно.',
'mergehistory-invalid-destination' => 'Име жељене странице мора бити исправно.',
'mergehistory-autocomment'         => 'Спојена страна [[:$1]] у страну [[:$2]].',
'mergehistory-comment'             => 'Спојена страна [[:$1]] у страну [[:$2]]: $3',

# Merge log
'mergelog'           => 'Лог спајања',
'pagemerge-logentry' => 'спојена страна [[$1]] у страну [[$2]] (број верзија до: $3)',
'revertmerge'        => 'растављање',
'mergelogpagetext'   => 'Испод се налази листа скорашњих спајања верзија једне стране у другу.',

# Diffs
'history-title'           => 'Историја верзија за "$1"',
'difference'              => '(Разлика између ревизија)',
'lineno'                  => 'Линија $1:',
'compareselectedversions' => 'Упореди означене верзије',
'editundo'                => 'врати',
'diff-multi'              => '({{PLURAL:$1|Једна ревизија није приказана|$1 ревизије нису приказане|$1 ревизија није приказано}}.)',

# Search results
'searchresults'             => 'Резултати претраге',
'searchresulttext'          => 'За више информација о претраживању {{SITENAME}}, погледајте [[{{MediaWiki:Helppage}}|Претраживање {{SITENAME}}]].',
'searchsubtitle'            => 'Тражили сте \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|све странице које почињу са "$1"]] | [[Special:WhatLinksHere/$1|све странице које повезују на "$1"]])',
'searchsubtitleinvalid'     => "Тражили сте '''$1'''",
'noexactmatch'              => "'''Не постоји страница са насловом \"\$1\".''' Можете [[:\$1|написати ту страницу]].",
'noexactmatch-nocreate'     => "'''Не постоји страница са насловом \"\$1\".'''",
'toomanymatches'            => 'Превише погодака је врећно. Измените упит.',
'titlematches'              => 'Наслов странице одговара',
'notitlematches'            => 'Ниједан наслов странице не одговара',
'textmatches'               => 'Текст странице одговара',
'notextmatches'             => 'Ниједан текст странице не одговара',
'prevn'                     => 'претходних $1',
'nextn'                     => 'следећих $1',
'viewprevnext'              => 'Погледај ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 реч|$2 речи}})',
'search-result-score'       => 'Релевантност: $1%',
'search-redirect'           => '(преусмерење $1)',
'search-section'            => '(наслов $1)',
'search-suggest'            => 'Да ли сте мислили: $1',
'search-interwiki-caption'  => 'Братски пројекти',
'search-interwiki-default'  => '$1 резултати:',
'search-interwiki-more'     => '(више)',
'search-mwsuggest-enabled'  => 'са сугестијама',
'search-mwsuggest-disabled' => 'без сугестија',
'search-relatedarticle'     => 'Сродно',
'mwsuggest-disable'         => 'Искључи АЈАКС сугестије',
'searchrelated'             => 'сродно',
'searchall'                 => 'све',
'showingresults'            => "Приказујем испод до {{PLURAL:$1|'''1''' резултат|'''$1''' резултата}} почев од #'''$2'''.",
'showingresultsnum'         => "Приказујем испод до {{PLURAL:$3|'''1''' резултат|'''$3''' резултата}} почев од #'''$2'''.",
'nonefound'                 => "'''Напомена''': неуспешне претраге су
често изазване тражењем честих речи као \"је\" или \"од\",
које нису индексиране, или навођењем више од једног израза за тражење (само странице
које садрже све изразе који се траже ће се појавити у резултату).",
'powersearch'               => 'Тражи',
'powersearch-legend'        => 'Напредна претрага',
'powersearch-ns'            => 'Тражи у именским просторима:',
'powersearch-redir'         => 'Списак преусмерења',
'powersearch-field'         => 'Претражи за',
'search-external'           => 'Спољашња претрага',
'searchdisabled'            => '<p>Извињавамо се! Пуна претрага текста је привремено онемогућена, због бржег рада {{SITENAME}}. У међувремену, можете користити Гугл претрагу испод, која може бити застарела.</p>',

# Preferences page
'preferences'              => 'Подешавања',
'mypreferences'            => 'Моја подешавања',
'prefs-edits'              => 'Број измена:',
'prefsnologin'             => 'Нисте пријављени',
'prefsnologintext'         => 'Морате бити <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} пријављени]</span> да бисте подешавали корисничка подешавања.',
'prefsreset'               => 'Враћена су ускладиштена подешавања.',
'qbsettings'               => 'Брза палета',
'qbsettings-none'          => 'Никаква',
'qbsettings-fixedleft'     => 'Причвршћена лево',
'qbsettings-fixedright'    => 'Причвршћена десно',
'qbsettings-floatingleft'  => 'Плутајућа лево',
'qbsettings-floatingright' => 'Плутајућа десно',
'changepassword'           => 'Промени лозинку',
'skin'                     => 'Кожа',
'math'                     => 'Математике',
'dateformat'               => 'Формат датума',
'datedefault'              => 'Није битно',
'datetime'                 => 'Датум и време',
'math_failure'             => 'Неуспех при парсирању',
'math_unknown_error'       => 'непозната грешка',
'math_unknown_function'    => 'непозната функција',
'math_lexing_error'        => 'речничка грешка',
'math_syntax_error'        => 'синтаксна грешка',
'math_image_error'         => 'PNG конверзија неуспешна; проверите тачну инсталацију latex-а, dvips-а, gs-а и convert-а',
'math_bad_tmpdir'          => 'Не могу да напишем или направим привремени math директоријум',
'math_bad_output'          => 'Не могу да напишем или направим директоријум за math излаз.',
'math_notexvc'             => 'Недостаје извршно texvc; молимо погледајте math/README да бисте подесили.',
'prefs-personal'           => 'Корисничка подешавања',
'prefs-rc'                 => 'Скорашње измене',
'prefs-watchlist'          => 'Списак надгледања',
'prefs-watchlist-days'     => 'Број дана који треба да се види на списку надгледања:',
'prefs-watchlist-edits'    => 'Број измена који треба да се види на проширеном списку надгледања:',
'prefs-misc'               => 'Разно',
'saveprefs'                => 'Сачувај',
'resetprefs'               => 'Очисти измене',
'oldpassword'              => 'Стара лозинка:',
'newpassword'              => 'Нова лозинка:',
'retypenew'                => 'Поново откуцајте нову лозинку:',
'textboxsize'              => 'Величине текстуалног поља',
'rows'                     => 'Редова',
'columns'                  => 'Колона',
'searchresultshead'        => 'Претрага',
'resultsperpage'           => 'Погодака по страници:',
'contextlines'             => 'Линија по поготку:',
'contextchars'             => 'Карактера контекста по линији:',
'stub-threshold'           => 'Праг за форматирање <a href="#" class="stub">линка као клице</a> (у бајтовима):',
'recentchangesdays'        => 'Број дана за приказ у скорашњим изменема:',
'recentchangescount'       => 'Број наслова у скорашњим изменама:',
'savedprefs'               => 'Ваша подешавања су сачувана.',
'timezonelegend'           => 'Временска зона',
'timezonetext'             => 'Број сати за који се ваше локално време разликује од серверског времена (UTC).',
'localtime'                => 'Локално време',
'timezoneoffset'           => 'Одступање¹',
'servertime'               => 'Време на серверу',
'guesstimezone'            => 'Попуни из браузера',
'allowemail'               => 'Омогући е-пошту од других корисника',
'prefs-searchoptions'      => 'Опције претраге',
'prefs-namespaces'         => 'Именски простори',
'defaultns'                => 'По стандарду тражи у овим именским просторима:',
'default'                  => 'стандард',
'files'                    => 'Фајлови',

# User rights
'userrights'                  => 'Управљање корисничким правима', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Управљај корисничким групама',
'userrights-user-editname'    => 'Унесите корисничко име:',
'editusergroup'               => 'Мењај групе корисника',
'editinguser'                 => "Мењате корисничка права корисника '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Промени корисничке групе',
'saveusergroups'              => 'Сачувај корисничке групе',
'userrights-groupsmember'     => 'Члан:',
'userrights-groups-help'      => 'Можете контролисати групе у којима се овај корисник налази.
* Штиклирани квадратић означава да се корисник налази у групи.
* Квадратић који није штиклиран означава да се корисник не налази у групи.
* Звездица (*) означава да ви не можете уклонити групу уколико сте је додали, или обратно.',
'userrights-reason'           => 'Разлог измене:',
'userrights-no-interwiki'     => 'Немате овлашћења да мењате корисничка права на осталим викијима.',
'userrights-nodatabase'       => 'База података $1 не постоји или је локална.',
'userrights-nologin'          => 'Морате се [[Special:UserLogin|пријавити]] са администраторским налогом да додате корисничка права.',
'userrights-notallowed'       => 'Ваш налог нема овлашћења да додаје корисника права.',
'userrights-changeable-col'   => 'Групе које можете мењати',
'userrights-unchangeable-col' => 'Групе које не можете мењати',

# Groups
'group'               => 'Група:',
'group-user'          => 'Корисници',
'group-autoconfirmed' => 'аутоматски потврђени сарадници',
'group-bot'           => 'Ботови',
'group-sysop'         => 'Администратори',
'group-bureaucrat'    => 'Бирократе',
'group-suppress'      => 'оверсајти',
'group-all'           => '(сви)',

'group-user-member'          => 'Корисник',
'group-autoconfirmed-member' => 'аутоматски потврђен сарадник',
'group-bot-member'           => 'Бот',
'group-sysop-member'         => 'Администратор',
'group-bureaucrat-member'    => 'Бирократа',
'group-suppress-member'      => 'оверсајт',

'grouppage-user'          => '{{ns:project}}:Корисници',
'grouppage-autoconfirmed' => '{{ns:project}}:Аутоматски потврђени сарадници',
'grouppage-bot'           => '{{ns:project}}:Ботови',
'grouppage-sysop'         => '{{ns:project}}:Администратори',
'grouppage-bureaucrat'    => '{{ns:project}}:Бирократе',
'grouppage-suppress'      => '{{ns:project}}:Оверсајт',

# Rights
'right-read'                 => 'Прегледање страница',
'right-edit'                 => 'Уређивање страница',
'right-createpage'           => 'Прављење страница (које нису странице за разговор)',
'right-createtalk'           => 'Прављење страница за разговор',
'right-createaccount'        => 'Прављење нових корисничких налога',
'right-minoredit'            => 'Означавање измена малом',
'right-move'                 => 'Премештање страница',
'right-move-subpages'        => 'Премештање страница са њиховим подстраницама',
'right-suppressredirect'     => 'Нестварање преусмерења од старог имена по преименовању стране.',
'right-upload'               => 'Слање фајлова',
'right-reupload'             => 'Преснимавање постојећег фајла',
'right-reupload-own'         => 'Преснимавање сопственог постојећег фајла',
'right-reupload-shared'      => 'локално преписивање фајлова на дељеном складишту медија',
'right-upload_by_url'        => 'слање фајла са URL адресе',
'right-purge'                => 'чишчење кеша сајта за страну без потврде',
'right-autoconfirmed'        => 'мењање полузаштићених страна',
'right-bot'                  => 'сарадник је, заправо, аутоматски процес (бот)',
'right-nominornewtalk'       => 'непоседовање малих измена на странама за разговор окида промпт за нову поруку',
'right-apihighlimits'        => 'коришћење виших лимита за упите из API-ја',
'right-writeapi'             => 'писање API-ја',
'right-delete'               => 'Брисање страница',
'right-bigdelete'            => 'Брисање страница са великом историјом',
'right-deleterevision'       => 'брисање и враћање посебних верзија страна',
'right-deletedhistory'       => 'гледање обрисаних верзија страна без текста који је везан за њих',
'right-browsearchive'        => 'Претраживање обрисаних страница',
'right-undelete'             => 'Враћање обрисане странице',
'right-suppressrevision'     => 'прегледање и враћање верзија сакривених за сисопе',
'right-suppressionlog'       => 'преглед приватних логова',
'right-block'                => 'забрана мењења страна другим сарадницима',
'right-blockemail'           => 'забрана слања имејла сарадницима',
'right-hideuser'             => 'забрана сарадничког имена скривањем од јавности',
'right-ipblock-exempt'       => 'пролазак ИП блокова, аутоматских блокова и блокова опсега',
'right-proxyunbannable'      => 'пролазак аутоматских блокова проксија',
'right-protect'              => 'промена степена заштите и измена заштићених страна',
'right-editprotected'        => 'измена заштићених страна (без могућности измене степена заштите)',
'right-editinterface'        => 'Уреди кориснички интерфејс',
'right-editusercssjs'        => 'мењање туђих CSS и JS фајлова',
'right-rollback'             => 'брзо враћање измена последњег сарадника који је мењао конкретну страну',
'right-markbotedits'         => 'означавање враћених страна као измена које је направио бот',
'right-noratelimit'          => 'не бити погођен лимитима',
'right-import'               => 'увожење страна с других викија',
'right-importupload'         => 'увожење страна из послатог фајла',
'right-patrol'               => 'маркирање туђих измена као патролираних',
'right-autopatrol'           => 'аутоматско маркирање својих измена као патролираних',
'right-patrolmarks'          => 'виђење ознака за патролирање унутар скорашњих измена',
'right-unwatchedpages'       => 'виђење списка ненадгледаних страна',
'right-trackback'            => 'пошаљи извештај',
'right-mergehistory'         => 'спајање историја страна',
'right-userrights'           => 'измена свих права сарадника',
'right-userrights-interwiki' => 'измена права сарадника на другим викијима',
'right-siteadmin'            => 'закључавање и откључавање базе података',

# User rights log
'rightslog'      => 'Историја корисничких права',
'rightslogtext'  => 'Ово је историја измена корисничких права.',
'rightslogentry' => 'је променио права за $1 од $2 на $3',
'rightsnone'     => '(нема)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|измена|измене|измена}}',
'recentchanges'                     => 'Скорашње измене',
'recentchangestext'                 => 'Пратите најскорије измене на викију на овој страници.',
'recentchanges-feed-description'    => 'Пратите скорашње измене уз помоћ овог фида.',
'rcnote'                            => "Испод {{PLURAL:$1|је '''1''' промена|су последње '''$1''' промене|су последњих '''$1''' промена}} у {{PLURAL:$2|последњем дану|последњa '''$2''' дана|последњих '''$2''' дана}}, од $5, $4.",
'rcnotefrom'                        => 'Испод су промене од <b>$2</b> (до <b>$1</b> приказано).',
'rclistfrom'                        => 'Покажи нове промене почев од $1',
'rcshowhideminor'                   => '$1 мале измене',
'rcshowhidebots'                    => '$1 ботове',
'rcshowhideliu'                     => '$1 пријављене кориснике',
'rcshowhideanons'                   => '$1 анонимне кориснике',
'rcshowhidepatr'                    => '$1 означене измене',
'rcshowhidemine'                    => '$1 сопствене измене',
'rclinks'                           => 'Покажи последњих $1 промена у последњих $2 дана<br />$3',
'diff'                              => 'разл',
'hist'                              => 'ист',
'hide'                              => 'сакриј',
'show'                              => 'покажи',
'minoreditletter'                   => 'м',
'newpageletter'                     => 'Н',
'boteditletter'                     => 'б',
'number_of_watching_users_pageview' => '[$1 надгледа {{PLURAL:$1|корисник|корисника}}]',
'rc_categories'                     => 'Ограничи на категорије (раздвоји са "|")',
'rc_categories_any'                 => 'Било који',
'newsectionsummary'                 => '/* $1 */ нова секција',

# Recent changes linked
'recentchangeslinked'          => 'Сродне промене',
'recentchangeslinked-title'    => 'Сродне промене за "$1"',
'recentchangeslinked-noresult' => 'Нема измена на повезаним страницама за одабрани период.',
'recentchangeslinked-summary'  => "Ова посебна страница показује списак поселењих промена на страницама које су повезане (или чланови одређене категорије). 
Странице са [[Special:Watchlist|вашег списка надгледања]] су '''подебљане'''.",
'recentchangeslinked-page'     => 'Име странице:',
'recentchangeslinked-to'       => 'приказивање измена према странама повезаних са датом страном',

# Upload
'upload'                      => 'Пошаљи фајл',
'uploadbtn'                   => 'Пошаљи фајл',
'reupload'                    => 'Поново пошаљи',
'reuploaddesc'                => 'Врати се на упитник за слање.',
'uploadnologin'               => 'Нисте пријављени',
'uploadnologintext'           => 'Морате бити [[Special:UserLogin|пријављени]]
да бисте слали фајлове.',
'upload_directory_missing'    => 'Директоријум за прихват фајлова ($1) недостаје, а веб сервер га не може направити.',
'upload_directory_read_only'  => 'На директоријум за слање ($1) сервер не може да пише.',
'uploaderror'                 => 'Грешка при слању',
'uploadtext'                  => "Користите формулар доле да бисте послали фајлове.
Да бисте видели или тражили претходно послате фајлове идите на [[Special:ImageList|списак послатих фајлова]], поновна слања су записани у [[Special:Log/upload|историји слања]], а брисања у [[Special:Log/delete|историји брисања]].

Слику додајете у погодне чланке користећи синтаксу:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Слика.jpg]]</nowiki></tt>''' да бисте користили пуну верзију фајла
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Слика.png|200п|мини|лево|опис]]</nowiki></tt>''' да висте користили 200 пиксела широку уоквирену слику са леве стране и са \"опис\" као описом слике.
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Фајл.ogg]]</nowiki></tt>''' да директно повежете ка фајлу без приказивања истог",
'upload-permitted'            => 'Дозвољени типови фајлова су: $1.',
'upload-preferred'            => 'Пожељни типови фајлова су: $1.',
'upload-prohibited'           => 'Забрањени типови фајлова су: $1.',
'uploadlog'                   => 'историја слања',
'uploadlogpage'               => 'историја слања',
'uploadlogpagetext'           => 'Испод је списак најскоријих слања.',
'filename'                    => 'Име фајла',
'filedesc'                    => 'Опис',
'fileuploadsummary'           => 'Опис:',
'filestatus'                  => 'Статус ауторског права:',
'filesource'                  => 'Извор:',
'uploadedfiles'               => 'Послати фајлови',
'ignorewarning'               => 'Игнориши упозорење и сними датотеку',
'ignorewarnings'              => 'Игнориши сва упозорења',
'minlength1'                  => 'Имена фајлова морају имати најмање један карактер.',
'illegalfilename'             => 'Фајл "$1" садржи карактере који нису дозвољени у називима страница. Молимо Вас промените име фајла и поново га пошаљите.',
'badfilename'                 => 'Име слике је промењено у "$1".',
'filetype-badmime'            => 'Није дозвољено слати фајлове MIME типа "$1".',
'filetype-unwanted-type'      => "'''\".\$1\"''' није пожељан тип фајла. 
Пожељни {{PLURAL:\$3|тип фајла је|типови фајлова су}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' је забрањен тип фајла. 
Пожељни {{PLURAL:\$3|тип фајла је|типови фајлова су}} \$2.",
'filetype-missing'            => 'Овај фајл нема екстензију (нпр ".jpg").',
'large-file'                  => 'Препоручљиво је да фајлови не буду већи од $1; овај фајл је $2.',
'largefileserver'             => 'Овај фајл је већи него што је подешено да сервер дозволи.',
'emptyfile'                   => 'Фајл који сте послали делује да је празан. Ово је могуће због грешке у имену фајла. Молимо проверите да ли стварно желите да пошаљете овај фајл.',
'fileexists'                  => 'Фајл са овим именом већ постоји. Молимо проверите <strong><tt>$1</tt></strong> ако нисте сигурни да ли желите да га промените.',
'filepageexists'              => 'Страна за опис овог фајла је већ направљена у време <strong><tt>$1</tt></strong>, али не постоји фајл с тим именом. Опис који унесеш се неће појавити на страни за опис. Да би се видео, мораћеш да измениш страну ручно.',
'fileexists-extension'        => 'Фајл са сличним именом већ постоји:<br />
Име фајла који шаљете: <strong><tt>$1</tt></strong><br />
Име постојећег фајла: <strong><tt>$2</tt></strong><br />
Молимо изаберите друго име.',
'fileexists-thumb'            => "<center>'''Постојећи фајл'''</center>",
'fileexists-thumbnail-yes'    => 'Овај фајл је највероватније умањена верзија слике. Молимо вас проверите фајл <strong><tt>$1</tt></strong>.<br />
Уколико је дати фајл иста слика или оригинална слика, није потребно да шаљете додатно умањену верзију исте.',
'file-thumbnail-no'           => 'Фајл почиње са <strong><tt>$1</tt></strong>. 
Претпоставља се да је ово умањена верзија слике.
Уколико имате ову слику у пуној резолицуји, пошаљите је, а уколико немате, промените име фајла.',
'fileexists-forbidden'        => 'Фајл са овим именом већ постоји; молимо вратите се и пошаљите овај фајл под новим именом. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Фајл са овим именом већ постоји у заједничкој остави. 
Молимо вратите се и пошаљите овај фајл под новим именом. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Овај фајл је дупликат {{PLURAL:$1|следећег фајла|следеђих фајлова}}:',
'successfulupload'            => 'Успешно слање',
'uploadwarning'               => 'Упозорење при слању',
'savefile'                    => 'Сними фајл',
'uploadedimage'               => 'послао "[[$1]]"',
'overwroteimage'              => 'послата нова верзија "[[$1]]"',
'uploaddisabled'              => 'Слање фајлова је искључено.',
'uploaddisabledtext'          => 'Слања фајлова су онемогућена на {{SITENAME}}.',
'uploadscripted'              => 'Овај фајл садржи ХТМЛ или код скрипте које интернет браузер може погрешно да интерпретира.',
'uploadcorrupt'               => 'Фајл је неисправан или има нетачну екстензију. Молимо проверите фајл и пошаљите га поново.',
'uploadvirus'                 => 'Фајл садржи вирус! Детаљи: $1',
'sourcefilename'              => 'Име фајла извора:',
'destfilename'                => 'Циљано име фајла:',
'upload-maxfilesize'          => 'Максимална величина фајла: $1',
'watchthisupload'             => 'Надгледај страницу',
'filewasdeleted'              => 'Фајл са овим именом је раније послат, а касније обрисан. Требало би да проверите $1 пре него што наставите са поновним слањем.',
'upload-wasdeleted'           => "'''Паћња: Шаљете фајл који је претходно обрисан.'''

Проверите да ли сте сигурно да желите послати овај фајл.
Разлог брисања овог фајла раније је:",
'filename-bad-prefix'         => 'Име овог фајла почиње са <strong>"$1"</strong>, што није описно име, најчешће је назван аутоматски са дигиталним фотоапаратом. Молимо изаберите описније име за ваш фајл.',

'upload-proto-error'      => 'Некоректни протокол',
'upload-proto-error-text' => 'Слање екстерних фајлова захтева УРЛове који почињу са <code>http://</code> или <code>ftp://</code>.',
'upload-file-error'       => 'Интерна грешка',
'upload-file-error-text'  => 'Десила се интерна грешка при покушају прављења привременог фајла на серверу. Контактирајте систем администратора.',
'upload-misc-error'       => 'Непозната грешка при слању фајла',
'upload-misc-error-text'  => 'Непозната грешка при слању фајла. Проверите да ли је УРЛ исправан и покушајте поново. Ако проблем остане, контактирајте систем администратора.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'УРЛ није доступан',
'upload-curl-error6-text'  => 'УРЛ који сте унели није доступан. Урадите дупли клик на УРЛ да проверите да ли је адреса доступна.',
'upload-curl-error28'      => 'Тајмаут грешка',
'upload-curl-error28-text' => 'Сајту је требало превише времена да одговори. Проверите да ли сајт ради, или сачекајте мало и покушајте поново.',

'license'            => 'Лиценца:',
'nolicense'          => 'Нема',
'license-nopreview'  => '(приказ није доступан)',
'upload_source_url'  => ' (валидан, јавно доступан УРЛ)',
'upload_source_file' => ' (фајл на вашем рачунару)',

# Special:ImageList
'imagelist-summary'     => 'Ова посебна страна приказује све послате фајлове. Подразумева се да је последњи послат фајл приказан на врху списка. Кликом на заглавље колоне мења се принцип сортирања.',
'imagelist_search_for'  => 'Тражи име слике:',
'imgfile'               => 'фајл',
'imagelist'             => 'Списак слика',
'imagelist_date'        => 'Датум',
'imagelist_name'        => 'Име',
'imagelist_user'        => 'Корисник',
'imagelist_size'        => 'Величина (бајтови)',
'imagelist_description' => 'Опис слике',

# Image description page
'filehist'                       => 'Историја фајла',
'filehist-help'                  => 'Кликните на датум/време да видите верзију фајла из тог времена.',
'filehist-deleteall'             => 'обриши све',
'filehist-deleteone'             => 'обриши',
'filehist-revert'                => 'врати',
'filehist-current'               => 'тренутно',
'filehist-datetime'              => 'Датум/Време',
'filehist-user'                  => 'Корисник',
'filehist-dimensions'            => 'Димензије',
'filehist-filesize'              => 'Величина фајла',
'filehist-comment'               => 'Коментар',
'imagelinks'                     => 'Употреба слике',
'linkstoimage'                   => '{{PLURAL:$1|Следећа страница користи|$1 Следеће странице користе}} овај фајл:',
'nolinkstoimage'                 => 'Нема страница које користе овај фајл.',
'morelinkstoimage'               => 'Види [[Special:WhatLinksHere/$1|више веза]] према овом фајлу.',
'redirectstofile'                => 'Следећи {{PLURAL:$1|фајл се преусмерава|$1 фајла се преусмеравају|$1 фајлова се преусмерава}} на овај фајл:',
'duplicatesoffile'               => 'Следећи {{PLURAL:$1|фајл је дупликат|$1 фајла су дупликати|$1 фајлова су дупликати}} овог фајла:',
'sharedupload'                   => 'Ова слика је са заједничке оставе и можда је користе остали пројекти.',
'shareduploadwiki'               => 'Молимо погледајте $1 за даље информације.',
'shareduploadwiki-desc'          => 'Опис за $1 на дељеном складишту налази се испод.',
'shareduploadwiki-linktext'      => 'страна за опис фајла',
'shareduploadduplicate'          => 'Овај фајл је дупликат фајла $1 на дељеном складишту.',
'shareduploadduplicate-linktext' => 'други фајл',
'shareduploadconflict'           => 'Овај фајл има исто име као фајл $1 са дељеног складишта.',
'shareduploadconflict-linktext'  => 'други фајл',
'noimage'                        => 'Не постоји фајл са овим именом, можете $1',
'noimage-linktext'               => 'послати један',
'uploadnewversion-linktext'      => 'Пошаљите новију верзију ове датотеке',
'imagepage-searchdupe'           => 'Претражи дупликате фајлова',

# File reversion
'filerevert'                => 'Врати $1',
'filerevert-legend'         => 'Врати фајл',
'filerevert-intro'          => "Враћате '''[[Media:$1|$1]]''' на [$4 верзију од $3, $2].",
'filerevert-comment'        => 'Коментар:',
'filerevert-defaultcomment' => 'Враћено на верзију од $2, $1',
'filerevert-submit'         => 'Врати',
'filerevert-success'        => "'''[[Media:$1|$1]]''' је враћен на [$4 верзију од $3, $2].",
'filerevert-badversion'     => 'Не постоји претходна локална верзија фајла са унесеним временом.',

# File deletion
'filedelete'                  => 'Обриши $1',
'filedelete-legend'           => 'Обриши фајл',
'filedelete-intro'            => "Бришете '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Бришете верзију фајла '''[[Media:$1|$1]]''' од [$4 $3, $2].",
'filedelete-comment'          => 'Коментар:',
'filedelete-submit'           => 'Обриши',
'filedelete-success'          => "'''$1''' је обрисан.",
'filedelete-success-old'      => "Верзија фајла '''[[Media:$1|$1]]''' од $3, $2 је обрисана.",
'filedelete-nofile'           => "'''$1''' не постоји.",
'filedelete-nofile-old'       => "Не постоји складиштена верзија фајла '''$1''' са датим особинама.",
'filedelete-iscurrent'        => 'Покушаваш да обришеш последњу измену фајла. Неопходно је да претходно вратиш фајл на претходну измену.',
'filedelete-otherreason'      => 'Други/додатни разлог:',
'filedelete-reason-otherlist' => 'Други разлог',
'filedelete-reason-dropdown'  => '*Најчешћи разлози брисања
** Кршење ауторских права
** Дупликат',
'filedelete-edit-reasonlist'  => 'Уреди разлоге за брисање',

# MIME search
'mimesearch'         => 'МИМЕ претрага',
'mimesearch-summary' => 'Ова страна омогућава филтерисање фајлова за свој MIME-тип. Улаз: contenttype/subtype, тј. <tt>image/jpeg</tt>.',
'mimetype'           => 'МИМЕ тип:',
'download'           => 'Преузми',

# Unwatched pages
'unwatchedpages' => 'Ненадгледане стране',

# List redirects
'listredirects' => 'Списак преусмерења',

# Unused templates
'unusedtemplates'     => 'Неискоришћени шаблони',
'unusedtemplatestext' => 'Ова страна наводи све странице у именском простору шаблона које нису укључене ни на једној другој страни. Не заборавите да проверите остале повезнице ка шаблонима пре него што их обришете.',
'unusedtemplateswlh'  => 'остале повезнице',

# Random page
'randompage'         => 'Случајна страница',
'randompage-nopages' => 'Нема страница у овом именском простору.',

# Random redirect
'randomredirect'         => 'Случајно преусмерење',
'randomredirect-nopages' => 'Нема преусмерења у овом именском простору.',

# Statistics
'statistics'             => 'Статистике',
'sitestats'              => 'Статистике сајта',
'userstats'              => 'Статистике корисника',
'sitestatstext'          => "База података тренутно садржи {{PLURAL:\$1|'''1''' страницу|'''\$1''' странице|'''\$1''' страница}}.
Овај број укључује странице за разговор, странице о {{SITENAME}}, минималне \"клице\", редиректе, и друге неквалифициране странице.
Искључујући ове, имамо {{PLURAL:\$2|чланак|чланка|чланака}}.

'''\$8''' {{PLURAL:\$8|фајл је послат|фајла је послато|фајлова је послато}}.

Постоји укупно '''\$3''' {{PLURAL:\$3|прелед|прегледа}} страница и '''\$4''' {{PLURAL:\$4|измена странице|измене странице|измена страница}} од када вики постоји.
У просеку корисници направе '''\$5''' измена по страници и претпрегледаје страницу '''\$6''' пута током једне измене.

[http://www.mediawiki.org/wiki/Manual:Job_queue Џоб кју] дужина је '''\$7'''.",
'userstatstext'          => "{{PLURAL:$1|Постоји '''1''' регистровани [[Special:ListUsers|корисник]]|Постоје '''$1''' регистрована [[Special:ListUsers|корисника]]|Постоји '''$1''' регистрованих [[Special:ListUsers|корисника]]}}, од којих '''$2''' (или '''$4%''') {{PLURAL:$2|има|имају|имају}} $5 права.",
'statistics-mostpopular' => 'Најпосећеније странице',

'disambiguations'      => 'Странице за вишезначне одреднице',
'disambiguationspage'  => 'Template:Вишезначна одредница',
'disambiguations-text' => "Следеће стране имају везе ка '''вишезначним одредницама'''. Потребно је да упућују на одговарајући чланак.

Страна се сматра вишезначном одредницом ако користи шаблон који је упућен са стране [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Двострука преусмерења',
'doubleredirectstext'        => 'Сваки ред садржи везе на прво и друго преусмерење, као и на прву линију текста другог преусмерења, што обично даје "прави" циљни чланак, на који би прво преусмерење и требало да показује.',
'double-redirect-fixed-move' => '[[$1]] је премештен, сада је преусмерење на [[$2]]',
'double-redirect-fixer'      => 'Поправљач преусмерења',

'brokenredirects'        => 'Покварена преусмерења',
'brokenredirectstext'    => 'Следећа преусмерења су повезана на непостојећи чланак.',
'brokenredirects-edit'   => '(уреди)',
'brokenredirects-delete' => '(обриши)',

'withoutinterwiki'         => 'Странице без интервики веза',
'withoutinterwiki-summary' => 'Следеће странице не вежу ка другим језицима (међувики):',
'withoutinterwiki-legend'  => 'префикс',
'withoutinterwiki-submit'  => 'Прикажи',

'fewestrevisions' => 'Странице са најмање ревизија',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|бајт|бајтова}}',
'ncategories'             => '$1 {{PLURAL:$1|категорија|категорије}}',
'nlinks'                  => '$1 {{PLURAL:$1|веза|везе}}',
'nmembers'                => '$1 {{PLURAL:$1|члан|члана|чланова}}',
'nrevisions'              => '$1 {{PLURAL:$1|ревизија|ревизије}}',
'nviews'                  => '$1 {{PLURAL:$1|преглед|прегледа}}',
'specialpage-empty'       => 'Нема резултата за овај извештај.',
'lonelypages'             => 'Сирочићи',
'lonelypagestext'         => 'Следеће странице нису повезане са других страница на овом викију.',
'uncategorizedpages'      => 'Странице без категорије',
'uncategorizedcategories' => 'Категорије без категорија',
'uncategorizedimages'     => 'Фајлови без категорија',
'uncategorizedtemplates'  => 'Шаблони без категорија',
'unusedcategories'        => 'Неискоришћене категорије',
'unusedimages'            => 'Неискоришћени фајлови',
'popularpages'            => 'Популарне странице',
'wantedcategories'        => 'Тражене категорије',
'wantedpages'             => 'Тражене странице',
'missingfiles'            => 'недостајући фајлови',
'mostlinked'              => 'Највише повезане стране',
'mostlinkedcategories'    => 'Чланци са највише категорија',
'mostlinkedtemplates'     => 'Најповезанији шаблони',
'mostcategories'          => 'Чланци са највише категорија',
'mostimages'              => 'Највише повезани фајлови',
'mostrevisions'           => 'Чланци са највише ревизија',
'prefixindex'             => 'Списак префикса',
'shortpages'              => 'Кратке странице',
'longpages'               => 'Дугачке странице',
'deadendpages'            => 'Странице без интерних веза',
'deadendpagestext'        => 'Следеће странице не вежу на друге странице на овом викију.',
'protectedpages'          => 'Заштићене странице',
'protectedpages-indef'    => 'само неограничене заштите',
'protectedpagestext'      => 'Следеће странице су заштићене од премештања или уређивања',
'protectedpagesempty'     => 'Нема заштићених страница са овим параметрима.',
'protectedtitles'         => 'Заштићени наслови',
'protectedtitlestext'     => 'Следећи наслови су заштићени од стварања:',
'protectedtitlesempty'    => 'Нема наслова који су тренутно заштићени помоћу ових параметара.',
'listusers'               => 'Списак корисника',
'newpages'                => 'Нове странице',
'newpages-username'       => 'Корисничко име:',
'ancientpages'            => 'Најстарији чланци',
'move'                    => 'премести',
'movethispage'            => 'премести ову страницу',
'unusedimagestext'        => 'Обратите пажњу да се други веб сајтови могу повезивати на слику директним УРЛ-ом, и тако могу још увек бити приказани овде упркос активној употреби.',
'unusedcategoriestext'    => 'Наредне стране категорија постоје иако их ни један други чланак или категорија не користе.',
'notargettitle'           => 'Нема циља',
'notargettext'            => 'Нисте навели циљну страницу или корисника
на коме би се извела ова функција.',
'nopagetitle'             => 'Не постоји таква страна',
'nopagetext'              => 'Циљана страна не постоји.',
'pager-newer-n'           => '{{PLURAL:$1|новији 1|новија $1|новијих $1}}',
'pager-older-n'           => '{{PLURAL:$1|старији 1|старија $1|старијих $1}}',
'suppress'                => 'оверсајт',

# Book sources
'booksources'               => 'Штампани извори',
'booksources-search-legend' => 'Претражите изворе књига',
'booksources-go'            => 'Иди',

# Special:Log
'specialloguserlabel'  => 'Корисник:',
'speciallogtitlelabel' => 'Наслов:',
'log'                  => 'Протоколи',
'all-logs-page'        => 'Све историје',
'log-search-legend'    => 'Претражи записе',
'log-search-submit'    => 'Иди',
'alllogstext'          => 'Комбиновани приказ свих доступних историја за {{SITENAME}}.
Можете сузити преглед одабиром типа историје, корисничког имена или тражене странице.',
'logempty'             => 'Протокол је празан.',
'log-title-wildcard'   => 'Тражи наслове који почињу са овим текстом',

# Special:AllPages
'allpages'          => 'Све странице',
'alphaindexline'    => '$1 у $2',
'nextpage'          => 'Следећа страна ($1)',
'prevpage'          => 'Претходна страна ($1)',
'allpagesfrom'      => 'Прикажи странице почетно са:',
'allarticles'       => 'Сви чланци',
'allinnamespace'    => 'Све странице ($1 именски простор)',
'allnotinnamespace' => 'Све странице (које нису у $1 именском простору)',
'allpagesprev'      => 'Претходна',
'allpagesnext'      => 'Следећа',
'allpagessubmit'    => 'Иди',
'allpagesprefix'    => 'Прикажи стране са префиксом:',
'allpagesbadtitle'  => 'Дати назив странице није добар или садржи међујезички или интервики префикс. Могуће је да садржи карактере који не могу да се користе у називима.',
'allpages-bad-ns'   => '{{SITENAME}} нема именски простор "$1".',

# Special:Categories
'categories'                    => 'Категоријe',
'categoriespagetext'            => 'Следеће категорије садрже странице или мултимедијалне фајлове.
[[Special:UnusedCategories|Неупотребљене категорије]]  се не приказују овде.
Такође погледате [[Special:WantedCategories|тражене категорије]].',
'categoriesfrom'                => 'Прикажи категорије на:',
'special-categories-sort-count' => 'сортирај по броју',
'special-categories-sort-abc'   => 'сортирај азбучно',

# Special:ListUsers
'listusersfrom'      => 'Прикажи кориснике почевши од:',
'listusers-submit'   => 'Прикажи',
'listusers-noresult' => 'Није пронађен корисник.',

# Special:ListGroupRights
'listgrouprights'          => 'права сарадничких група',
'listgrouprights-group'    => 'Група',
'listgrouprights-rights'   => 'Права',
'listgrouprights-helppage' => 'Help:Права',
'listgrouprights-members'  => '(списак чланова)',

# E-mail user
'mailnologin'     => 'Нема адресе за слање',
'mailnologintext' => 'Морате бити [[Special:UserLogin|пријављени]] и имати исправну адресу е-поште у вашим [[Special:Preferences|подешавањима]]
да бисте слали е-пошту другим корисницима.',
'emailuser'       => 'Пошаљи е-пошту овом кориснику',
'emailpage'       => 'Пошаљи е-писмо кориснику',
'emailpagetext'   => 'Ако је овај корисник унео исправну адресу е-поште у своја корисничка подешавања, упитник испод ће послати једну поруку.
Адреса е-поште коју сте ви унели у својим [[Special:Preferences|корисничким подешавањима]] ће се појавити као "From" адреса поруке, тако да ће прималац моћи директно да одговори.',
'usermailererror' => 'Објекат поште је вратио грешку:',
'defemailsubject' => '{{SITENAME}} е-пошта',
'noemailtitle'    => 'Нема адресе е-поште',
'noemailtext'     => 'Овај корисник није навео исправну адресу е-поште,
или је изабрао да не прима е-пошту од других корисника.',
'emailfrom'       => 'Од:',
'emailto'         => 'За:',
'emailsubject'    => 'Наслов:',
'emailmessage'    => 'Порука:',
'emailsend'       => 'Пошаљи',
'emailccme'       => 'Пошаљи ми копију моје поруке у моје сандуче е-поште.',
'emailccsubject'  => 'Копија ваше поруке на $1: $2',
'emailsent'       => 'Порука послата',
'emailsenttext'   => 'Ваша порука је послата електронском поштом.',
'emailuserfooter' => 'Овај имејл посла $1 сараднику $2 помоћу "Пошаљи имејл" функције на сајту "{{SITENAME}}".',

# Watchlist
'watchlist'            => 'Мој списак надгледања',
'mywatchlist'          => 'Мој списак надгледања',
'watchlistfor'         => "(за '''$1''')",
'nowatchlist'          => 'Немате ништа на свом списку надгледања.',
'watchlistanontext'    => 'Молимо $1 да бисте гледали или мењали ставке на вашем списку надгледања.',
'watchnologin'         => 'Нисте пријављени',
'watchnologintext'     => 'Морате бити [[Special:UserLogin|пријављени]] да бисте мењали списак надгледања.',
'addedwatch'           => 'Додато списку надгледања',
'addedwatchtext'       => 'Страница "[[:$1]]" је додата вашем [[Special:Watchlist|списку надгледања]].
Будуће промене ове странице и њој придружене странице за разговор ће бити наведене овде, и страница ће бити <b>подебљана</b> у [[Special:RecentChanges|списку]] скорашњих измена да би се лакше уочила.

Ако касније желите да уклоните страницу са вашег списка надгледања, кликните на "прекини надгледање" на бочној палети.',
'removedwatch'         => 'Уклоњено са списка надгледања',
'removedwatchtext'     => 'Страница "[[:$1]]" је уклоњена са вашег списка надгледања.',
'watch'                => 'надгледај',
'watchthispage'        => 'Надгледај ову страницу',
'unwatch'              => 'Прекини надгледање',
'unwatchthispage'      => 'Прекини надгледање',
'notanarticle'         => 'Није чланак',
'notvisiblerev'        => 'Ревизија је обрисана',
'watchnochange'        => 'Ништа што надгледате није промењено у приказаном времену.',
'watchlist-details'    => '{{PLURAL:$1|$1 страна|$1 стране|$1 страна}} на вашем списку надгледања, не рачунајући странице за разговор.',
'wlheader-enotif'      => '* Обавештавање е-поштом је омогућено.',
'wlheader-showupdated' => "* Странице које су измењене од када сте их последњи пут посетили су приказане '''подебљано'''",
'watchmethod-recent'   => 'проверавам има ли надгледаних страница у скорашњим изменама',
'watchmethod-list'     => 'проверавам има ли скорашњих измена у надгледаним страницама',
'watchlistcontains'    => 'Ваш списак надгледања садржи $1 {{PLURAL:$1|страну|стране|страна}}.',
'iteminvalidname'      => "Проблем са ставком '$1', неисправно име...",
'wlnote'               => "Испод {{PLURAL:$1|је последња измена|су последње '''$1''' измене|последњих '''$1''' измена}} у {{PLURAL:$2|последњем сату|последња '''$2''' сата|последњих '''$2''' сати}}.",
'wlshowlast'           => 'Прикажи последњих $1 сати $2 дана $3',
'watchlist-show-bots'  => 'Прикажи измене ботова',
'watchlist-hide-bots'  => 'Сакриј измене ботова',
'watchlist-show-own'   => 'Прикажи моје измене',
'watchlist-hide-own'   => 'Сакриј моје измене',
'watchlist-show-minor' => 'Прикажи мале измене',
'watchlist-hide-minor' => 'Сакриј мале измене',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Надгледам...',
'unwatching' => 'Уклањање надгледања...',

'enotif_mailer'                => '{{SITENAME}} пошта обавештења',
'enotif_reset'                 => 'Означи све стране као посећене',
'enotif_newpagetext'           => 'Ово је нови чланак.',
'enotif_impersonal_salutation' => '{{SITENAME}} корисник',
'changed'                      => 'промењена',
'created'                      => 'направљена',
'enotif_subject'               => '{{SITENAME}} страница $PAGETITLE је била $CHANGEDORCREATED од стране $PAGEEDITOR',
'enotif_lastvisited'           => 'Погледајте $1 за све промене од ваше последње посете.',
'enotif_lastdiff'              => 'Погледајте $1 да видите ову измену.',
'enotif_anon_editor'           => 'анонимни корисник $1',
'enotif_body'                  => 'Драги $WATCHINGUSERNAME,

{{SITENAME}} страна $PAGETITLE је била $CHANGEDORCREATED $PAGEEDITDATE од стране $PAGEEDITOR,
погледајте {{fullurl:$PAGETITLE}} за тренутну верзију.

$NEWPAGE

Резиме едитора: $PAGESUMMARY $PAGEMINOREDIT

Контактирајте едитора:
пошта {{fullurl:Special:Emailuser|target=$PAGEEDITOR}}
вики {{fullurl:User:$PAGEEDITOR}}

Неће бити других обавештења у случају даљих промена уколико не посетите ову страну.
Такође можете да ресетујете заставице за обавештења за све ваше надгледане стране на вашем списку надгледања.

             Ваш пријатељски {{SITENAME}} систем обавештавања

--
Да промените подешавања везана за списак надгледања посетите
{{fullurl:Special:Watchlist|edit=yes}}

Фидбек и даља помоћ:
{{fullurl:Help:Садржај}}',

# Delete/protect/revert
'deletepage'                  => 'Обриши страницу',
'confirm'                     => 'Потврди',
'excontent'                   => "садржај је био: '$1'",
'excontentauthor'             => "садржај је био: '$1' (а једину измену је направио '$2')",
'exbeforeblank'               => "садржај пре брисања је био: '$1'",
'exblank'                     => 'страница је била празна',
'delete-confirm'              => 'Обриши „$1“',
'delete-legend'               => 'Обриши',
'historywarning'              => 'Пажња: страница коју желите да обришете има историју:',
'confirmdeletetext'           => 'На путу сте да трајно обришете страницу
или слику заједно са њеном историјом из базе података.
Молимо вас потврдите да намеравате да урадите ово, да разумете
последице, и да ово радите у складу са
[[{{MediaWiki:Policy-url}}|правилима]] {{SITENAME}}.',
'actioncomplete'              => 'Акција завршена',
'deletedtext'                 => 'Чланак "<nowiki>$1</nowiki>" је обрисан.
Погледајте $2 за запис о скорашњим брисањима.',
'deletedarticle'              => 'обрисан "[[$1]]"',
'suppressedarticle'           => 'сактивено: "[[$1]]"',
'dellogpage'                  => 'историја брисања',
'dellogpagetext'              => 'Испод је списак најскоријих брисања.',
'deletionlog'                 => 'историја брисања',
'reverted'                    => 'Враћено на ранију ревизију',
'deletecomment'               => 'Разлог за брисање',
'deleteotherreason'           => 'Други/додатни разлог:',
'deletereasonotherlist'       => 'Други разлог',
'deletereason-dropdown'       => '*Најчешћи разлози брисања
** Захтев аутора
** Кршење ауторских права
** Вандализам',
'delete-edit-reasonlist'      => 'Уреди разлоге за брисање',
'delete-toobig'               => 'Ова страница има велику историју странице, преко $1 {{PLURAL:$1|ревизије|ревизије|ревизија}}.
Брисање таквих страница је забрањено ради превентиве од случајног оштећења сајта.',
'rollback'                    => 'Врати измене',
'rollback_short'              => 'Врати',
'rollbacklink'                => 'врати',
'rollbackfailed'              => 'Враћање није успело',
'cantrollback'                => 'Не могу да вратим измену; последњи аутор је уједно и једини.',
'alreadyrolled'               => 'Не могу да вратим последњу измену [[:$1]] од корисника [[User:$2|$2]] ([[User talk:$2|разговор]] | [[Special:Contributions/$2|{{int:contribslink}}]]); неко други је већ изменио или вратио чланак.

Последња измена од корисника [[User:$3|$3]] ([[User talk:$3|разговор]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Коментар измене је: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Враћене измене корисника [[Special:Contributions/$2|$2]] ([[User talk:$2|Разговор]]) на последњу измену корисника [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Враћене измене од стране $1; на последњу измену од стране $2.',
'sessionfailure'              => 'Изгледа да постоји проблем са вашом сеансом пријаве;
ова акција је прекинута као предострожност против преотимања сеанси.
Молимо кликните "back" и поново учитајте страну одакле сте дошли, а онда покушајте поново.',
'protectlogpage'              => 'историја закључавања',
'protectlogtext'              => 'Испод је списак заштићених страница.<br />
Погледајте [[Special:ProtectedPages|правила о заштити страница]] за више информација.',
'protectedarticle'            => 'заштитио $1',
'modifiedarticleprotection'   => 'промењен ниво заштите за „[[$1]]“',
'unprotectedarticle'          => 'скинуо заштиту са $1',
'protect-title'               => 'стављање заштите "$1"',
'protect-legend'              => 'Потврдите заштиту',
'protectcomment'              => 'Коментар:',
'protectexpiry'               => 'Истиче:',
'protect_expiry_invalid'      => 'Време истека није одговарајуће.',
'protect_expiry_old'          => 'Време истека је у прошлости.',
'protect-unchain'             => 'Откључај дозволе премештања',
'protect-text'                => 'Овде можете погледати и мењати ниво заштите за страницу <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Не можете мењати нивое заштите док сте блокирани.
Ово су тренутна подешавања за страницу <strong>$1</strong>:',
'protect-locked-access'       => 'Ваш налог нема дозволе за измену нивоа заштите странице.
Ово су тренутна подешавања за страницу <strong>$1</strong>:',
'protect-cascadeon'           => 'Ова страница је тренутно заштићена јер се налази на {{PLURAL:$1|страници, која је заштићена|стране, које су заштићене|страна, које су заштићене}} са опцијом "преносиво". Можете изменити степен заштите ове странице, али он неће утицати на преносиву заштиту.',
'protect-default'             => '(стандард)',
'protect-fallback'            => 'Захтева "$1" овлашћења',
'protect-level-autoconfirmed' => 'Блокирај нерегистроване кориснике',
'protect-level-sysop'         => 'Само администратори',
'protect-summary-cascade'     => 'преносива заштита',
'protect-expiring'            => 'истиче $1 (UTC)',
'protect-cascade'             => 'Заштићене странице укључене у ову страницу (преносива заштита)
Protect pages included in this page (cascading protection)',
'protect-cantedit'            => 'Не можете мењати нивое заштите за ову страницу, због тога што немате овлашћења да је уређујете.',
'restriction-type'            => 'Овлашћење:',
'restriction-level'           => 'Ниво заштите:',
'minimum-size'                => 'Мин величина',
'maximum-size'                => 'Макс величина:',
'pagesize'                    => '(бајта)',

# Restrictions (nouns)
'restriction-edit'   => 'Уреди',
'restriction-move'   => 'Премештање',
'restriction-create' => 'Направи',
'restriction-upload' => 'Пошаљи фајл',

# Restriction levels
'restriction-level-sysop'         => 'пуна заштита',
'restriction-level-autoconfirmed' => 'полу-заштита',
'restriction-level-all'           => 'било који ниво',

# Undelete
'undelete'                   => 'Погледај обрисане странице',
'undeletepage'               => 'Погледај и врати обрисане странице',
'undeletepagetitle'          => "'''Следеће садржи обрисане измене чланка: [[:$1|$1]]'''.",
'viewdeletedpage'            => 'Погледај обрисане стране',
'undeletepagetext'           => 'Следеће странице су обрисане али су још увек у архиви и
могу бити враћене. Архива може бити периодично чишћена.',
'undelete-fieldset-title'    => 'враћање верзија',
'undeleteextrahelp'          => "Да бисте вратили историју целе стране, оставите све кућице неоткаченим и кликните на '''''Врати'''''. 
Да извршите селективно враћање, откачите кућице које одговарају ревизији која треба да се врати и кликните на '''''Врати'''''. 
Кликом на '''''Поништи''''' ћете обрисати поље за коментар и све кућице.",
'undeleterevisions'          => '$1 {{PLURAL:$1|ревизија|ревизије|ревизија}} архивирано',
'undeletehistory'            => 'Ако вратите страницу, све ревизије ће бити враћене њеној историји.
Ако је нова страница истог имена направљена од брисања, враћене ревизије ће се појавити у ранијој историји.',
'undeletehistorynoadmin'     => 'Ова страна је обрисана.
Испод се налази део историје брисања и историја ревизија обрисане стране.
Питајте администратора ако желите да се страница врати.',
'undelete-revision'          => 'Обрисана страна $1 (у време $2) од стране сарадника $3:',
'undeleterevision-missing'   => 'Некоректна или непостојећа ревизија. Можда је ваш линк погрешан, или је ревизија рестаурирана, или обрисана из архиве.',
'undelete-nodiff'            => 'Нема претходних измена.',
'undeletebtn'                => 'Врати',
'undeletelink'               => 'врати',
'undeletereset'              => 'Поништи',
'undeletecomment'            => 'Коментар:',
'undeletedarticle'           => 'вратио "[[$1]]"',
'undeletedrevisions'         => '{{PLURAL:$1|1 ревизија враћена|$1 ревизије врећене|$1 ревизија враћено}}',
'undeletedrevisions-files'   => '$1 {{PLURAL:$1|ревизија|ревизије|ревизија}} и $2 {{PLURAL:$2|фајл|фајла|фајлова}} враћено',
'undeletedfiles'             => '$1 {{PLURAL:$1|фајл|фајла|фајлова}} {{PLURAL:$1|враћен|враћена|враћено}}',
'cannotundelete'             => 'Враћање обрисане верзије није успело; неко други је вратио страницу пре вас.',
'undeletedpage'              => "<big>'''Страна $1 је враћена'''</big>",
'undelete-header'            => 'Види [[Special:Log/delete|лог брисања]] за скоро обрисане стране.',
'undelete-search-box'        => 'Претражи обрисане странице',
'undelete-search-prefix'     => 'Прикажи стране које почињу са:',
'undelete-search-submit'     => 'Претрага',
'undelete-no-results'        => 'Нема таквих страна у складишту обрисаних.',
'undelete-filename-mismatch' => 'Није могуће обрисати верзију фајла од времена $1: име фајла се не поклапа.',
'undelete-bad-store-key'     => 'Није могуће вратити измену верзије фајла времена $1: фајл је недеостајао пре брисања.',
'undelete-cleanup-error'     => 'Грешка приликом брисања некоришћеног фајла из архиве "$1".',
'undelete-error-short'       => 'Грешка при враћању фајла: $1',
'undelete-error-long'        => 'Десила се грешка при враћању фајла:

$1',

# Namespace form on various pages
'namespace'      => 'Именски простор:',
'invert'         => 'Обрни селекцију',
'blanknamespace' => '(Главно)',

# Contributions
'contributions' => 'Прилози корисника',
'mycontris'     => 'Моји прилози',
'contribsub2'   => 'За $1 ($2)',
'nocontribs'    => 'Нису нађене промене које задовољавају ове услове.',
'uctop'         => ' (врх)',
'month'         => 'За месец (и раније):',
'year'          => 'Од године (и раније):',

'sp-contributions-newbies'     => 'Прикажи само прилоге нових налога',
'sp-contributions-newbies-sub' => 'За новајлије',
'sp-contributions-blocklog'    => 'Историја блокирања',
'sp-contributions-search'      => 'Претрага прилога',
'sp-contributions-username'    => 'ИП адреса или корисничко име:',
'sp-contributions-submit'      => 'Претрага',

# What links here
'whatlinkshere'            => 'Шта је повезано овде',
'whatlinkshere-title'      => 'Странице које су повезане на „$1“',
'whatlinkshere-page'       => 'Страна:',
'linklistsub'              => '(списак веза)',
'linkshere'                => "Следеће странице су повезане на '''[[:$1]]''':",
'nolinkshere'              => "Ни једна страница није повезана на: '''[[:$1]]'''.",
'nolinkshere-ns'           => "Ни једна страница у одабраном именском простору се не веже за '''[[:$1]]'''",
'isredirect'               => 'преусмеривач',
'istemplate'               => 'укључивање',
'isimage'                  => 'линк ка слици',
'whatlinkshere-prev'       => '{{PLURAL:$1|претходни|претходних $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|следећи|следећих $1}}',
'whatlinkshere-links'      => '← везе',
'whatlinkshere-hideredirs' => '$1 преусмерења',
'whatlinkshere-hidetrans'  => '$1 укључења',
'whatlinkshere-hidelinks'  => '$1 везе',
'whatlinkshere-hideimages' => 'број веза према сликама: $1',
'whatlinkshere-filters'    => 'Филтери',

# Block/unblock
'blockip'                         => 'Блокирај корисника',
'blockip-legend'                  => 'Блокирај корисника',
'blockiptext'                     => 'Употребите доњи упитник да бисте уклонили право писања
са одређене ИП адресе или корисничког имена.
Ово би требало да буде урађено само да би се спречио вандализам, и у складу
са [[{{MediaWiki:Policy-url}}|смерницама]].
Унесите конкретан разлог испод (на пример, наводећи које
странице су вандализоване).',
'ipaddress'                       => 'ИП адреса',
'ipadressorusername'              => 'ИП адреса или корисничко име',
'ipbexpiry'                       => 'Трајање',
'ipbreason'                       => 'Разлог:',
'ipbreasonotherlist'              => 'Други разлог',
'ipbreason-dropdown'              => '*Најчешћи разлози блокирања
** Уношење лажних информација
** Уклањање садржаја са страница
** Постављање веза ка спољашњим сајтовима
** Унос бесмислица у странице
** Непожељно понашање
** Употреба више налога
** Непожељно корисничко име',
'ipbanononly'                     => 'Блокирај само анонимне кориснике',
'ipbcreateaccount'                => 'Спречи прављење налога',
'ipbemailban'                     => 'Забраните кориснику да шаље е-пошту',
'ipbenableautoblock'              => 'Аутоматски блокирај последњу ИП адресу овог корисника, и сваку следећу адресу са које се покуша уређивање.',
'ipbsubmit'                       => 'Блокирај овог корисника',
'ipbother'                        => 'Остало време',
'ipboptions'                      => '2 сата:2 hours,1 дан:1 day,3 дана:3 days,1 недеља:1 week,2 недеље:2 weeks,1 месец:1 month,3 месеца:3 months,6 месеци:6 months,1 година:1 year,бесконачно:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'остало',
'ipbotherreason'                  => 'Други/додатни разлог:',
'ipbhidename'                     => 'Сакриј сарадничко име од лога блокирања, активног списка блокова и списка сарадника.',
'ipbwatchuser'                    => 'надгледање сарадничке стране и стране за разговор овог сарадника',
'badipaddress'                    => 'Лоша ИП адреса',
'blockipsuccesssub'               => 'Блокирање је успело',
'blockipsuccesstext'              => '"[[Special:Contributions/$1|$1]]" је блокиран.
<br />Погледајте [[Special:IPBlockList|ИП списак блокираних корисника]] за преглед блокирања.',
'ipb-edit-dropdown'               => 'Мењајте разлоге блока',
'ipb-unblock-addr'                => 'Одблокирај $1',
'ipb-unblock'                     => 'Одблокирај корисничко име или ИП адресу',
'ipb-blocklist-addr'              => 'Погледајте посојеће блокове за $1',
'ipb-blocklist'                   => 'Погледајте постојеће блокове',
'unblockip'                       => 'Одблокирај корисника',
'unblockiptext'                   => 'Употребите доњи упитник да бисте вратили право писања
раније блокираној ИП адреси или корисничком имену.',
'ipusubmit'                       => 'Одблокирај ову адресу',
'unblocked'                       => '[[User:$1|$1]] је деблокиран',
'unblocked-id'                    => 'Блок $1 је уклоњен',
'ipblocklist'                     => 'Блокиране ИП адресе и корисничка имена',
'ipblocklist-legend'              => 'Пронађи блокираног корисника',
'ipblocklist-username'            => 'Корисник или ИП адреса:',
'ipblocklist-submit'              => 'Претрага',
'blocklistline'                   => '$1, $2 блокирао корисника $3, (истиче $4)',
'infiniteblock'                   => 'бесконачан',
'expiringblock'                   => 'истиче $1',
'anononlyblock'                   => 'само анонимни',
'noautoblockblock'                => 'Аутоблокирање је онемогућено',
'createaccountblock'              => 'блокирано прављење налога',
'emailblock'                      => 'е-пошта блокираном',
'ipblocklist-empty'               => 'Списак блокова је празан.',
'ipblocklist-no-results'          => 'Унешена ИП адреса или корисничко име није блокирано.',
'blocklink'                       => 'блокирај',
'unblocklink'                     => 'одблокирај',
'contribslink'                    => 'прилози',
'autoblocker'                     => 'Аутоматски сте блокирани јер делите ИП адресу са "[[User:$1|$1]]". Дати разлог за блокирање корисника $1 је: "\'\'\'$2\'\'\'"',
'blocklogpage'                    => 'историја блокирања',
'blocklogentry'                   => 'је блокирао "[[$1]]" са временом истицања блокаде од $2 $3',
'blocklogtext'                    => 'Ово је историја блокирања и деблокирања корисника. Аутоматски
блокиране ИП адресе нису исписане овде. Погледајте [[Special:IPBlockList|блокиране ИП адресе]] за списак тренутних забрана и блокирања.',
'unblocklogentry'                 => 'одблокирао "$1"',
'block-log-flags-anononly'        => 'само анонимни корисници',
'block-log-flags-nocreate'        => 'забрањено прављење налога',
'block-log-flags-noautoblock'     => 'искључено аутоматско блокирање',
'block-log-flags-noemail'         => 'блокирано слање е-поште',
'block-log-flags-angry-autoblock' => 'омогућен је побољшани аутоблок',
'range_block_disabled'            => 'Администраторска могућност да блокира блокове ИП адреса је искључена.',
'ipb_expiry_invalid'              => 'Погрешно време трајања.',
'ipb_expiry_temp'                 => 'Сакривени блокови сарадничких имена морају бити стални.',
'ipb_already_blocked'             => '"$1" је већ блокиран',
'ipb_cant_unblock'                => 'Грешка: ИД блока $1 није нађен. Могуће је да је већ одблокиран.',
'ip_range_invalid'                => 'Нетачан блок ИП адреса.',
'blockme'                         => 'Блокирај ме',
'proxyblocker'                    => 'Блокер проксија',
'proxyblocker-disabled'           => 'Ова фукција је искључена.',
'proxyblockreason'                => 'Ваша ИП адреса је блокирана јер је она отворени прокси. Молимо контактирајте вашег Интернет сервис провајдера или техничку подршку и обавестите их о овом озбиљном сигурносном проблему.',
'proxyblocksuccess'               => 'Урађено.',
'sorbsreason'                     => 'Ваша ИП адреса је на списку као отворен прокси на DNSBL.',
'sorbs_create_account_reason'     => 'Ваша ИП адреса се налази на списку као отворени прокси на DNSBL. Не можете да направите налог',

# Developer tools
'lockdb'              => 'Закључај базу',
'unlockdb'            => 'Откључај базу',
'lockdbtext'          => 'Закључавање базе ће свим корисницима укинути могућност измене страница,
промене корисничких подешавања, измене списка надгледања, и свега осталог
што захтева промене у бази.
Молимо потврдите да је ово заиста оно што намеравате да урадите и да ћете
откључати базу када завршите посао око њеног одржавања.',
'unlockdbtext'        => 'Откључавање базе ће свим корисницима вратити могућност измене страница,
промене корисничких подешавања, измене списка надгледања, и свега осталог
што захтева промене у бази.
Молимо потврдите да је ово заиста оно што намеравате да урадите.',
'lockconfirm'         => 'Да, заиста желим да закључам базу.',
'unlockconfirm'       => 'Да, заиста желим да откључам базу.',
'lockbtn'             => 'Закључај базу',
'unlockbtn'           => 'Откључај базу',
'locknoconfirm'       => 'Нисте потврдили своју намеру.',
'lockdbsuccesssub'    => 'База је закључана',
'unlockdbsuccesssub'  => 'База је откључана',
'lockdbsuccesstext'   => 'База података је закључана.<br />
Сетите се да је [[Special:UnlockDB|откључате]] када завршите са одржавањем.',
'unlockdbsuccesstext' => 'База података је откључана.',
'lockfilenotwritable' => 'По фајлу за закључавање базе података не може да се пише. Да бисте закључали или откључали базу, по овом фајлу мора да буде омогућено писање од стране веб сервера.',
'databasenotlocked'   => 'База података није закључана.',

# Move page
'move-page'               => 'Премести $1',
'move-page-legend'        => 'Премештање странице',
'movepagetext'            => "Доњи упитник ће преименовати страницу, премештајући сву њену историју на ново име.
Стари наслов ће постати преусмерење на нови наслов.
Повезнице према старом наслову неће бити промењене, обавезно потражите [[Special:DoubleRedirects|двострука]] или [[Special:BrokenRedirects|покварена]] преусмерења.
На вама је одговорност да везе и даље иду тамо где би и требало да иду.

Обратите пажњу да страница '''неће''' бити померена ако већ постоји страница са новим насловом, осим ако је она празна или преусмерење и нема историју промена.
Ово значи да не можете преименовати страницу на оно име са кога сте је преименовали ако погрешите, и не можете преписати постојећу страницу.

'''ПАЖЊА!'''
Ово може бити драстична и неочекивана промена за популарну страницу;
молимо да будете сигурни да разумете последице овога пре него што наставите.",
'movepagetalktext'        => "Одговарајућа страница за разговор, ако постоји, биће аутоматски премештена истовремено '''осим ако:'''
*Непразна страница за разговор већ постоји под новим именом, или
*Одбележите доњу кућицу.

У тим случајевима, мораћете ручно да преместите или спојите страницу уколико то желите.",
'movearticle'             => 'Премести страницу',
'movenotallowed'          => 'Немате облашћења за премештање страница.',
'newtitle'                => 'Нови наслов',
'move-watch'              => 'Надгледај ову страницу',
'movepagebtn'             => 'премести страницу',
'pagemovedsub'            => 'Премештање успело',
'movepage-moved'          => '<big>\'\'\'Страна "$1" је преименована у "$2"!\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Страница под тим именом већ постоји, или је
име које сте изабрали неисправно.
Молимо изаберите друго име.',
'cantmove-titleprotected' => 'Не можете преместити страницу на ову локацију, зато што је нови наслов заштићен за прављење',
'talkexists'              => "'''Сама страница је успешно премештена, али
страница за разговор није могла бити премештена јер таква већ постоји на новом наслову. Молимо вас да их спојите ручно.'''",
'movedto'                 => 'премештена на',
'movetalk'                => 'Премести "страницу за разговор" такође, ако је могуће.',
'move-subpages'           => 'Преименуј све подстране ако је могуће.',
'move-talk-subpages'      => 'Преименуј све подстране стране за разговор ако је могуће.',
'movepage-page-exists'    => 'Страна $1 већ постоји не може се аутоматски преписати.',
'movepage-page-moved'     => 'Страна $1 је преименована у $2.',
'movepage-page-unmoved'   => 'Страна $1 не може бити преименована у $2.',
'1movedto2'               => 'је променио име чланку [[$1]] у [[$2]]',
'1movedto2_redir'         => 'је променио име чланку [[$1]] у [[$2]] путем преусмерења',
'movelogpage'             => 'историја премештања',
'movelogpagetext'         => 'Испод је списак премештања чланака.',
'movereason'              => 'Разлог:',
'revertmove'              => 'врати',
'delete_and_move'         => 'Обриши и премести',
'delete_and_move_text'    => '==Потребно брисање==

Циљани чланак "[[:$1]]" већ постоји. Да ли желите да га обришете да бисте направили место за премештање?',
'delete_and_move_confirm' => 'Да, обриши страницу',
'delete_and_move_reason'  => 'Обрисано како би се направило место за премештање',
'selfmove'                => 'Изворни и циљани назив су исти; страна не може да се премести преко саме себе.',
'immobile_namespace'      => 'Циљани назив је посебног типа; не могу да преместе стране у тај именски простор.',
'imagenocrossnamespace'   => 'Фајл се не може преименовати у именски простор који не припада фајловима.',
'imagetypemismatch'       => 'Нови наставак за фајлове се не поклапа са својим типом.',
'imageinvalidfilename'    => 'Циљано име фајла је погрешно.',
'fix-double-redirects'    => 'Освежава било које преусмерење које веже на оригинални наслов',

# Export
'export'            => 'Извези странице',
'exporttext'        => 'Можете извозити текст и историју промена одређене
странице или групе страница у XML формату. Ово онда може бити увезено у други
вики који користи МедијаВики софтвер преко {{ns:special}}:Import странице.

Да бисте извозили странице, унесите називе у текстуалном пољу испод, са једним насловом по реду, и одаберите да ли желите тренутну верзију са свим старим верзијама или само тренутну верзију са информацијама о последњој измени.

У другом случају, можете такође користити везу, нпр. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] за страницу [[{{MediaWiki:Mainpage}}]].',
'exportcuronly'     => 'Укључи само тренутну ревизију, не целу историју',
'exportnohistory'   => "----
'''Напомена:''' извожење пуне историје страна преко овог формулара је онемогућено због серверских разлога.",
'export-submit'     => 'Извоз',
'export-addcattext' => 'Додај странице из категорије:',
'export-addcat'     => 'Додај',
'export-download'   => 'Сачувај као фајл',
'export-templates'  => 'Укључује шаблоне',

# Namespace 8 related
'allmessages'               => 'Системске поруке',
'allmessagesname'           => 'Име',
'allmessagesdefault'        => 'Стандардни текст',
'allmessagescurrent'        => 'Тренутни текст',
'allmessagestext'           => 'Ово је списак системских порука које су у МедијаВики именском простору.
Посетите [http://translatewiki.net Betawiki] уколико желите да помогнете у локализацији.',
'allmessagesnotsupportedDB' => "Ова страница не може бити употребљена зато што је '''\$wgUseDatabaseMessages''' искључен.",
'allmessagesfilter'         => 'Филтер за регуларне изразе:',
'allmessagesmodified'       => 'Прикажи само измењене',

# Thumbnails
'thumbnail-more'           => 'увећај',
'filemissing'              => 'Недостаје фајл',
'thumbnail_error'          => 'Грешка при прављењу умањене слике: $1',
'djvu_page_error'          => 'DjVu страна је ван опсега.',
'djvu_no_xml'              => 'Не могу преузети XML за DjVu фајл.',
'thumbnail_invalid_params' => 'Погрешни параметри за малу слику.',
'thumbnail_dest_directory' => 'Не могу направити одредишни директоријум.',

# Special:Import
'import'                     => 'Увоз страница',
'importinterwiki'            => 'Трансвики увожење',
'import-interwiki-text'      => 'Одаберите вики и назив стране за увоз. Датуми ревизије и имена уредника ће бити сачувани. Сви трансвики увози су забележени у [[Special:Log/import|историји увоза]].',
'import-interwiki-history'   => 'Копирај све ревизије ове стране',
'import-interwiki-submit'    => 'Увези',
'import-interwiki-namespace' => 'Пребаци странице у именски простор:',
'importtext'                 => 'Молимо извезите фајл из изворног викија користећи [[Special:Export|извоз]].
Сачувајте га код себе и пошаљите овде.',
'importstart'                => 'Увожење страна у току...',
'import-revision-count'      => '$1 {{PLURAL:$1|ревизија|ревизије|ревизија}}',
'importnopages'              => 'Нема страна за увоз.',
'importfailed'               => 'Увоз није успео: $1',
'importunknownsource'        => 'Непознати тип извора уноса',
'importcantopen'             => 'Неуспешно отварање фајла за увоз',
'importbadinterwiki'         => 'Лоша интервики веза',
'importnotext'               => 'Страница је празна или без текста.',
'importsuccess'              => 'Успешан увоз!',
'importhistoryconflict'      => 'Постоји конфликтна историја ревизија (можда је ова страница већ увезена раније)',
'importnosources'            => 'Није дефинисан ниједан извор трансвики увожења и директна слања историја су онемогућена.',
'importnofile'               => 'Није послат ниједан увозни фајл.',
'importuploaderrorsize'      => 'Слање и унос фајла нису успели. Фајл је већи него што је дозвољено.',
'importuploaderrorpartial'   => 'Слање фајла за унос података није успело. Фајл је делимично стигао.',
'importuploaderrortemp'      => 'Слање фајла за унос није успело. Привремени директоријум недостаје.',
'import-parse-failure'       => 'Неуспешно парсирање унесеног XML-а.',
'import-noarticle'           => 'Нема страница за увоз!',
'import-nonewrevisions'      => 'Све верзије су претходно унесене.',
'import-upload'              => 'слање XML података',

# Import log
'importlogpage'                    => 'историја увоза',
'importlogpagetext'                => 'Административни увози страна са историјама измена са других викија.',
'import-logentry-upload'           => 'увезен $1 преко слања фајла',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|ревизија|ревизије|ревизија}}',
'import-logentry-interwiki'        => 'премештено са другог викија: $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|ревизија|ревизије|ревизија}} од $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Моја корисничка страница',
'tooltip-pt-anonuserpage'         => 'Корисничка страница ИП адресе са које уређујете',
'tooltip-pt-mytalk'               => 'Моја страница за разговор',
'tooltip-pt-anontalk'             => 'Разговор о прилозима са ове ИП адресе',
'tooltip-pt-preferences'          => 'Моја корисничка подешавања',
'tooltip-pt-watchlist'            => 'Списак чланака које надгледате',
'tooltip-pt-mycontris'            => 'Списак мојих прилога',
'tooltip-pt-login'                => 'Препоручује се да се пријавите, али није обавезно',
'tooltip-pt-anonlogin'            => 'Препоручује се да се пријавите, али није обавезно',
'tooltip-pt-logout'               => 'Одјави се',
'tooltip-ca-talk'                 => 'Разговор о чланку',
'tooltip-ca-edit'                 => 'Можете уредити ову страницу. Молимо користите претпреглед пре сачувавања.',
'tooltip-ca-addsection'           => 'Додајте коментар на ову дискусију',
'tooltip-ca-viewsource'           => 'Ова страница је закључана. Можете видети њен извор',
'tooltip-ca-history'              => 'Претходне верзије ове странице',
'tooltip-ca-protect'              => 'Заштити ову страницу',
'tooltip-ca-delete'               => 'Обриши ову страницу',
'tooltip-ca-undelete'             => 'Враћати измене које су начињене пре брисања странице',
'tooltip-ca-move'                 => 'Премести ову страницу',
'tooltip-ca-watch'                => 'Додајте ову страницу на Ваш списак надгледања',
'tooltip-ca-unwatch'              => 'Уклоните ову страницу са Вашег списка надгледања',
'tooltip-search'                  => 'Претражите вики',
'tooltip-search-go'               => 'Иди на страну с тачним именом ако постоји.',
'tooltip-search-fulltext'         => 'Претражите стране са овим текстом',
'tooltip-p-logo'                  => 'Главна страна',
'tooltip-n-mainpage'              => 'Посетите главну страну',
'tooltip-n-portal'                => 'О пројекту, шта можете да радите и где да пронађете ствари',
'tooltip-n-currentevents'         => 'Сазнајте више о актуелностима',
'tooltip-n-recentchanges'         => 'Списак скорашњих измена на викију',
'tooltip-n-randompage'            => 'Учитавај случајну страницу',
'tooltip-n-help'                  => 'Место где можете да научите нешто',
'tooltip-t-whatlinkshere'         => 'Списак свих страница које везују на ову',
'tooltip-t-recentchangeslinked'   => 'Скорашње измене на чланцима повезаним са ове странице',
'tooltip-feed-rss'                => 'RSS фид за ову страницу',
'tooltip-feed-atom'               => 'Atom фид за ову страницу',
'tooltip-t-contributions'         => 'Погледај списак прилога овог корисника',
'tooltip-t-emailuser'             => 'Пошаљи електронску пошту овом кориснику',
'tooltip-t-upload'                => 'Пошаљи слике и медија фајлове',
'tooltip-t-specialpages'          => 'Списак свих посебних страница',
'tooltip-t-print'                 => 'Верзија за штампање ове стране',
'tooltip-t-permalink'             => 'стални линк ка овој верзији стране',
'tooltip-ca-nstab-main'           => 'Погледајте чланак',
'tooltip-ca-nstab-user'           => 'Погледајте корисничку страницу',
'tooltip-ca-nstab-media'          => 'Погледајте медија страницу',
'tooltip-ca-nstab-special'        => 'Ово је посебна страница, не можете је мењати',
'tooltip-ca-nstab-project'        => 'Преглед странице пројекта',
'tooltip-ca-nstab-image'          => 'Погледајте страницу слике',
'tooltip-ca-nstab-mediawiki'      => 'Погледајте системску поруку',
'tooltip-ca-nstab-template'       => 'Погледајте шаблон',
'tooltip-ca-nstab-help'           => 'Погледајте страницу за помоћ',
'tooltip-ca-nstab-category'       => 'Погледајте страницу категорије',
'tooltip-minoredit'               => 'Назначите да се ради о малој измени',
'tooltip-save'                    => 'Снимите Ваше измене',
'tooltip-preview'                 => 'Претпреглед Ваших измена, молимо користите ово пре снимања!',
'tooltip-diff'                    => 'Прикажи које промене сте направили на тексту.',
'tooltip-compareselectedversions' => 'Погледаj разлике између две одабране верзије ове странице.',
'tooltip-watch'                   => 'Додајте ову страницу на Ваш списак надгледања',
'tooltip-recreate'                => 'Направи поново страницу без обзира да је била обрисана',
'tooltip-upload'                  => 'Почни слање',

# Stylesheets
'common.css'   => '/** CSS стављен овде ће се односити на све коже */',
'monobook.css' => '/* CSS стављен овде ће се односити на кориснике Монобук коже */',

# Metadata
'nodublincore'      => 'Dublin Core RDF метаподаци онемогућени за овај сервер.',
'nocreativecommons' => 'Creative Commons RDF метаподаци онемогућени за овај сервер.',
'notacceptable'     => 'Вики сервер не може да пружи податке у оном формату који ваш клијент може да прочита.',

# Attribution
'anonymous'        => 'Анонимни корисник {{SITENAME}}',
'siteuser'         => '{{SITENAME}} корисник $1',
'lastmodifiedatby' => 'Ову страницу је последњи пут променио $3 у $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Базирано на раду корисника $1.',
'others'           => 'остали',
'siteusers'        => '{{SITENAME}} корисник (корисници) $1',
'creditspage'      => 'Заслуге за страницу',
'nocredits'        => 'Нису доступне информације о заслугама за ову страницу.',

# Spam protection
'spamprotectiontitle' => 'Филтер за заштиту од нежељених порука',
'spamprotectiontext'  => 'Страна коју желите да сачувате је блокирана од стране филтера за нежељене поруке.
Ово је вероватно изазвано блокираном везом ка спољашњем сајту.',
'spamprotectionmatch' => 'Следећи текст је изазвао наш филтер за нежељене поруке: $1',
'spambot_username'    => 'Чишћење нежељених порука у МедијаВикију',
'spam_reverting'      => 'Враћање на стару ревизију која не садржи повезнице ка $1',
'spam_blanking'       => 'Све ревизије су садржале повезнице ка $1, пражњење',

# Info page
'infosubtitle'   => 'Информације за страницу',
'numedits'       => 'Број промена (чланак): $1',
'numtalkedits'   => 'Број промена (страница за разговор): $1',
'numwatchers'    => 'Број корисника који надгледају: $1',
'numauthors'     => 'Број различитих аутора (чланак): $1',
'numtalkauthors' => 'Број различитих аутора (страница за разговор): $1',

# Math options
'mw_math_png'    => 'Увек прикажи PNG',
'mw_math_simple' => 'HTML ако је врло једноставно, иначе PNG',
'mw_math_html'   => 'HTML ако је могуће, иначе PNG',
'mw_math_source' => 'Остави као ТеХ (за текстуалне браузере)',
'mw_math_modern' => 'Препоручено за савремене браузере',
'mw_math_mathml' => 'MathML ако је могуће (експериментално)',

# Patrolling
'markaspatrolleddiff'                 => 'Означи као патролиран',
'markaspatrolledtext'                 => 'Означи овај чланак као патролиран',
'markedaspatrolled'                   => 'Означен као патролиран',
'markedaspatrolledtext'               => 'Изабрана ревизија је означена као патролирана.',
'rcpatroldisabled'                    => 'Патрола скорашњих измена онемогућена',
'rcpatroldisabledtext'                => 'Патрола скорашњих измена је тренутно онемогућена.',
'markedaspatrollederror'              => 'Немогуће означити као патролирано',
'markedaspatrollederrortext'          => 'Морате изабрати ревизију да бисте означили као патролирано.',
'markedaspatrollederror-noautopatrol' => 'Није ти дозвољено да обележиш своје измене патролираним.',

# Patrol log
'patrol-log-page' => 'Историја патролирања',
'patrol-log-line' => 'обележена верзија $1 стране $2 као патролирана ($3)',
'patrol-log-auto' => '(аутоматски)',

# Image deletion
'deletedrevision'                 => 'Обрисана стара ревизија $1.',
'filedeleteerror-short'           => 'Грешка при брисању фајла: $1',
'filedeleteerror-long'            => 'Појавиле су се грешке приликом брисања фајла:

$1',
'filedelete-missing'              => 'Фајл „$1” се не може обрисати, зато што не постоји.',
'filedelete-old-unregistered'     => 'Дата верзија фајла "$1" не постоји у бази.',
'filedelete-current-unregistered' => 'Дати фајл "$1" не постоји у бази.',
'filedelete-archive-read-only'    => 'Веб сервер не може писати по кладишном директоријуму "$1".',

# Browsing diffs
'previousdiff' => '← Старија измена',
'nextdiff'     => 'Новија измена →',

# Media information
'mediawarning'         => "'''Упозорење''': Овај фајл садржи лош код, његовим извршавањем можете да угрозите ваш систем.<hr />",
'imagemaxsize'         => 'Ограничи слике на странама за разговор о сликама на:',
'thumbsize'            => 'Величина умањеног приказа :',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|страна|стране|страна}}',
'file-info'            => '(величина фајла: $1, MIME тип: $2)',
'file-info-size'       => '($1 × $2 пиксела, величина фајла: $3, MIME тип: $4)',
'file-nohires'         => '<small>Није доступна већа резолуција</small>',
'svg-long-desc'        => '(SVG фајл, номинално $1 × $2 пиксела, величина фајла: $3)',
'show-big-image'       => 'Пуна резолуција',
'show-big-image-thumb' => '<small>Величина овог приказа: $1 × $2 пиксела</small>',

# Special:NewImages
'newimages'             => 'Галерија нових слика',
'imagelisttext'         => "Испод је списак од '''$1''' {{PLURAL:$1|фајла|фајла|фајлова}} поређаних $2.",
'newimages-summary'     => 'Ова посебна страна приказује последње послате фајлове.',
'showhidebots'          => '($1 ботове)',
'noimages'              => 'Нема ништа да се види',
'ilsubmit'              => 'Тражи',
'bydate'                => 'по датуму',
'sp-newimages-showfrom' => 'Прикажи нове фајлове почевши од $2, $1',

# Bad image list
'bad_image_list' => 'Формат је следећи:

Разматрају се само ставке у списку (линије које почињу са *). 
Прва веза у линији мора бити веза на високо ризичну слику. 
Све друге везе у истој линији се сматрају изузецима тј. чланци у којима се слика може приказати.',

# Variants for Serbian language
'variantname-sr-ec' => 'ћирилица',
'variantname-sr-el' => 'latinica',
'variantname-sr'    => 'disable',

# Metadata
'metadata'          => 'Метаподаци',
'metadata-help'     => 'Овај фајл садржи додатне информације, које су вероватно додали дигитални фотоапарат или скенер који су коришћени да би се направила слика. 
Ако је првобитно стање фајла промењено, могуће је да неки детаљи не описују у потпуности измењен фајл.',
'metadata-expand'   => 'Покажи детаље',
'metadata-collapse' => 'Сакриј детаље',
'metadata-fields'   => 'Поља EXIF метаподатака наведена у овој поруци ће бити убачена на страну о слици када се рашири табела за метаподатке. Остала ће бити сакривена по подразумеваном.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Ширина',
'exif-imagelength'                 => 'Висина',
'exif-bitspersample'               => 'Битова по компоненти',
'exif-compression'                 => 'Шема компресије',
'exif-photometricinterpretation'   => 'Композиција пиксела',
'exif-orientation'                 => 'Оријентација',
'exif-samplesperpixel'             => 'Број компоненти',
'exif-planarconfiguration'         => 'Принцип распореда података',
'exif-ycbcrsubsampling'            => 'Однос компоненте Y према C',
'exif-ycbcrpositioning'            => 'Размештај компонената Y и C',
'exif-xresolution'                 => 'Хоризонатална резолуција',
'exif-yresolution'                 => 'Вертикална резолуција',
'exif-resolutionunit'              => 'Јединица резолуције',
'exif-stripoffsets'                => 'Положај блока података',
'exif-rowsperstrip'                => 'Број редова у блоку',
'exif-stripbytecounts'             => 'Величина компресованог блока',
'exif-jpeginterchangeformat'       => 'Удаљеност ЈПЕГ прегледа од почетка фајла',
'exif-jpeginterchangeformatlength' => 'Количина бајтова ЈПЕГ прегледа',
'exif-transferfunction'            => 'Функција преобликовања колор простора',
'exif-whitepoint'                  => 'Хромацитет беле тачке',
'exif-primarychromaticities'       => 'Хромацитет примарних боја',
'exif-ycbcrcoefficients'           => 'Матрични коефицијенти трансформације колор простора',
'exif-referenceblackwhite'         => 'Место беле и црне тачке',
'exif-datetime'                    => 'Датум последње промене фајла',
'exif-imagedescription'            => 'Име слике',
'exif-make'                        => 'Произвођач камере',
'exif-model'                       => 'Модел камере',
'exif-software'                    => 'Коришћен софтвер',
'exif-artist'                      => 'Аутор',
'exif-copyright'                   => 'Носилац права',
'exif-exifversion'                 => 'Exif верзија',
'exif-flashpixversion'             => 'Подржана верзија Флешпикса',
'exif-colorspace'                  => 'Простор боје',
'exif-componentsconfiguration'     => 'Значење сваке од компоненти',
'exif-compressedbitsperpixel'      => 'Мод компресије слике',
'exif-pixelydimension'             => 'Пуна висина слике',
'exif-pixelxdimension'             => 'Пуна ширина слике',
'exif-makernote'                   => 'Напомене произвођача',
'exif-usercomment'                 => 'Кориснички коментар',
'exif-relatedsoundfile'            => 'Повезани звучни запис',
'exif-datetimeoriginal'            => 'Датум и време сликања',
'exif-datetimedigitized'           => 'Датум и време дигитализације',
'exif-subsectime'                  => 'Део секунде у којем је сликано',
'exif-subsectimeoriginal'          => 'Део секунде у којем је фотографисано',
'exif-subsectimedigitized'         => 'Део секунде у којем је дигитализовано',
'exif-exposuretime'                => 'Експозиција',
'exif-exposuretime-format'         => '$1 сек ($2)',
'exif-fnumber'                     => 'F број отвора бленде',
'exif-exposureprogram'             => 'Програм експозиције',
'exif-spectralsensitivity'         => 'Спектрална осетљивост',
'exif-isospeedratings'             => 'ИСО вредност',
'exif-oecf'                        => 'Оптоелектронски фактор конверзије',
'exif-shutterspeedvalue'           => 'Брзина затварача',
'exif-aperturevalue'               => 'Отвор бленде',
'exif-brightnessvalue'             => 'Светлост',
'exif-exposurebiasvalue'           => 'Компензација експозиције',
'exif-maxaperturevalue'            => 'Минимални број отвора бленде',
'exif-subjectdistance'             => 'Удаљеност до објекта',
'exif-meteringmode'                => 'Режим мерача времена',
'exif-lightsource'                 => 'Извор светлости',
'exif-flash'                       => 'Блиц',
'exif-focallength'                 => 'Фокусна даљина сочива',
'exif-subjectarea'                 => 'Положај и површина објекта снимка',
'exif-flashenergy'                 => 'Енергија блица',
'exif-spatialfrequencyresponse'    => 'Просторна фреквенцијска карактеристика',
'exif-focalplanexresolution'       => 'Водоравна резолуција фокусне равни',
'exif-focalplaneyresolution'       => 'Хоризонатлна резолуција фокусне равни',
'exif-focalplaneresolutionunit'    => 'Јединица резолуције фокусне равни',
'exif-subjectlocation'             => 'Положај субјекта',
'exif-exposureindex'               => 'Индекс експозиције',
'exif-sensingmethod'               => 'Тип сензора',
'exif-filesource'                  => 'Изворни фајл',
'exif-scenetype'                   => 'Тип сцене',
'exif-cfapattern'                  => 'CFA шаблон',
'exif-customrendered'              => 'Додатна обрада слике',
'exif-exposuremode'                => 'Режим избора експозиције',
'exif-whitebalance'                => 'Баланс беле боје',
'exif-digitalzoomratio'            => 'Однос дигиталног зума',
'exif-focallengthin35mmfilm'       => 'Еквивалент фокусне даљине за 35 mm филм',
'exif-scenecapturetype'            => 'Тип сцене на снимку',
'exif-gaincontrol'                 => 'Контрола осветљености',
'exif-contrast'                    => 'Контраст',
'exif-saturation'                  => 'Сатурација',
'exif-sharpness'                   => 'Оштрина',
'exif-devicesettingdescription'    => 'Опис подешавања уређаја',
'exif-subjectdistancerange'        => 'Распон удаљености субјеката',
'exif-imageuniqueid'               => 'Јединствени идентификатор слике',
'exif-gpsversionid'                => 'Верзија блока ГПС-информације',
'exif-gpslatituderef'              => 'Северна или јужна ширина',
'exif-gpslatitude'                 => 'Ширина',
'exif-gpslongituderef'             => 'Источна или западна дужина',
'exif-gpslongitude'                => 'Дужина',
'exif-gpsaltituderef'              => 'Висина испод или изнад мора',
'exif-gpsaltitude'                 => 'Висина',
'exif-gpstimestamp'                => 'Време по ГПС-у (атомски сат)',
'exif-gpssatellites'               => 'Употребљени сателити',
'exif-gpsstatus'                   => 'Статус пријемника',
'exif-gpsmeasuremode'              => 'Режим мерења',
'exif-gpsdop'                      => 'Прецизност мерења',
'exif-gpsspeedref'                 => 'Јединица брзине',
'exif-gpsspeed'                    => 'Брзина ГПС пријемника',
'exif-gpstrackref'                 => 'Тип азимута пријемника (прави или магнетни)',
'exif-gpstrack'                    => 'Азимут пријемника',
'exif-gpsimgdirectionref'          => 'Тип азимута слике (прави или магнетни)',
'exif-gpsimgdirection'             => 'Азимут слике',
'exif-gpsmapdatum'                 => 'Коришћени геодетски координатни систем',
'exif-gpsdestlatituderef'          => 'Индекс географске ширине објекта',
'exif-gpsdestlatitude'             => 'Географска ширина објекта',
'exif-gpsdestlongituderef'         => 'Индекс географске дужине објекта',
'exif-gpsdestlongitude'            => 'Географска дужина објекта',
'exif-gpsdestbearingref'           => 'Индекс азимута објекта',
'exif-gpsdestbearing'              => 'Азимут објекта',
'exif-gpsdestdistanceref'          => 'Мерне јединице удаљености објекта',
'exif-gpsdestdistance'             => 'Удаљеност објекта',
'exif-gpsprocessingmethod'         => 'Име методе обраде ГПС података',
'exif-gpsareainformation'          => 'Име ГПС подручја',
'exif-gpsdatestamp'                => 'ГПС датум',
'exif-gpsdifferential'             => 'ГПС диференцијална корекција',

# EXIF attributes
'exif-compression-1' => 'Некомпресован',
'exif-compression-6' => 'ЈПЕГ',

'exif-unknowndate' => 'Непознат датум',

'exif-orientation-1' => 'Нормално', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Обрнуто по хоризонтали', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Заокренуто 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Обрнуто по вертикали', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Заокренуто 90° супротно од смера казаљке на сату и обрнуто по вертикали', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Заокренуто 90° у смеру казаљке на сату', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Заокренуто 90° у смеру казаљке на сату и обрнуто по вертикали', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Заокренуто 90° супротно од смера казаљке на сату', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'делимични формат',
'exif-planarconfiguration-2' => 'планарни формат',

'exif-componentsconfiguration-0' => 'не постоји',

'exif-exposureprogram-0' => 'Непознато',
'exif-exposureprogram-1' => 'Ручно',
'exif-exposureprogram-2' => 'Нормални програм',
'exif-exposureprogram-3' => 'Приоритет отвора бленде',
'exif-exposureprogram-4' => 'Приоритет затварача',
'exif-exposureprogram-5' => 'Уметнички програм (на бази нужне дубине поља)',
'exif-exposureprogram-6' => 'Спортски програм (на бази што бржег затварача)',
'exif-exposureprogram-7' => 'Портретни режим (за крупне кадрове са неоштром позадином)',
'exif-exposureprogram-8' => 'Режим пејзажа (за слике пејзажа са оштром позадином)',

'exif-subjectdistance-value' => '$1 метара',

'exif-meteringmode-0'   => 'Непознато',
'exif-meteringmode-1'   => 'Просек',
'exif-meteringmode-2'   => 'Просек са тежиштем на средини',
'exif-meteringmode-3'   => 'Тачка',
'exif-meteringmode-4'   => 'Више тачака',
'exif-meteringmode-5'   => 'Матрични',
'exif-meteringmode-6'   => 'Делимични',
'exif-meteringmode-255' => 'Друго',

'exif-lightsource-0'   => 'Непознато',
'exif-lightsource-1'   => 'Дневна светлост',
'exif-lightsource-2'   => 'Флуоресцентно',
'exif-lightsource-3'   => 'Волфрам (светло)',
'exif-lightsource-4'   => 'Блиц',
'exif-lightsource-9'   => 'Лепо време',
'exif-lightsource-10'  => 'Облачно време',
'exif-lightsource-11'  => 'Сенка',
'exif-lightsource-12'  => 'Флуоресцентна светлост (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Флуоресцентна светлост (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Флуоресцентна светлост (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Бела флуоресценција (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Стандардно светло А',
'exif-lightsource-18'  => 'Стандардно светло Б',
'exif-lightsource-19'  => 'Стандардно светло Ц',
'exif-lightsource-24'  => 'ИСО студијски волфрам',
'exif-lightsource-255' => 'Други извор светла',

'exif-focalplaneresolutionunit-2' => 'инчи',

'exif-sensingmethod-1' => 'Недефинисано',
'exif-sensingmethod-2' => 'Једнокристални матрични сензор',
'exif-sensingmethod-3' => 'Двокристални матрични сензор',
'exif-sensingmethod-4' => 'Трокристални матрични сензор',
'exif-sensingmethod-5' => 'Секвенцијални матрични сензор',
'exif-sensingmethod-7' => 'Тробојни линеарни сензор',
'exif-sensingmethod-8' => 'Секвенцијални линеарни сензор',

'exif-filesource-3' => 'Дигитални фотоапарат',

'exif-scenetype-1' => 'Директно фотографисана слика',

'exif-customrendered-0' => 'Нормални процес',
'exif-customrendered-1' => 'Нестадардни процес',

'exif-exposuremode-0' => 'Аутоматски',
'exif-exposuremode-1' => 'Ручно',
'exif-exposuremode-2' => 'Аутоматски са задатим распоном',

'exif-whitebalance-0' => 'Аутоматски',
'exif-whitebalance-1' => 'Ручно',

'exif-scenecapturetype-0' => 'Стандардно',
'exif-scenecapturetype-1' => 'Пејзаж',
'exif-scenecapturetype-2' => 'Портрет',
'exif-scenecapturetype-3' => 'Ноћно',

'exif-gaincontrol-0' => 'Нема',
'exif-gaincontrol-1' => 'Мало повећање',
'exif-gaincontrol-2' => 'Велико повећање',
'exif-gaincontrol-3' => 'Мало смањење',
'exif-gaincontrol-4' => 'Велико смањење',

'exif-contrast-0' => 'Нормално',
'exif-contrast-1' => 'Меко',
'exif-contrast-2' => 'Тврдо',

'exif-saturation-0' => 'Нормално',
'exif-saturation-1' => 'Ниска сатурација',
'exif-saturation-2' => 'Висока сатурација',

'exif-sharpness-0' => 'Нормално',
'exif-sharpness-1' => 'Меко',
'exif-sharpness-2' => 'Тврдо',

'exif-subjectdistancerange-0' => 'Непознато',
'exif-subjectdistancerange-1' => 'Крупни кадар',
'exif-subjectdistancerange-2' => 'Блиски кадар',
'exif-subjectdistancerange-3' => 'Далеки кадар',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Север',
'exif-gpslatitude-s' => 'Југ',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Исток',
'exif-gpslongitude-w' => 'Запад',

'exif-gpsstatus-a' => 'Мерење у току',
'exif-gpsstatus-v' => 'Спреман за пренос',

'exif-gpsmeasuremode-2' => 'Дводимензионално мерење',
'exif-gpsmeasuremode-3' => 'Тродимензионално мерење',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Километри на час',
'exif-gpsspeed-m' => 'Миље на час',
'exif-gpsspeed-n' => 'Чворови',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Прави правац',
'exif-gpsdirection-m' => 'Магнетни правац',

# External editor support
'edit-externally'      => 'Измените овај фајл користећи спољашњу апликацију',
'edit-externally-help' => 'Погледајте [http://www.mediawiki.org/wiki/Manual:External_editors упутство за подешавање] за више информација.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'све',
'imagelistall'     => 'све',
'watchlistall2'    => 'све',
'namespacesall'    => 'сви',
'monthsall'        => 'све',

# E-mail address confirmation
'confirmemail'             => 'Потврдите адресу е-поште',
'confirmemail_noemail'     => 'Немате потврђену адресу ваше е-поште у вашим [[Special:Preferences|корисничким подешавањима интерфејса]].',
'confirmemail_text'        => 'Ова вики захтева да потврдите адресу ваше е-поште пре него што користите могућности е-поште. Активирајте дугме испод како бисте послали пошту за потврду на вашу адресу. Пошта укључује везу која садржи код; учитајте ту везу у ваш браузер да бисте потврдили да је адреса ваше е-поште валидна.',
'confirmemail_send'        => 'Пошаљи код за потврду',
'confirmemail_sent'        => 'Е-пошта за потврђивање послата.',
'confirmemail_sendfailed'  => '{{SITENAME}} није успела да пошање е-пошту. 
Проверита адресу због неправилних карактера.

Враћено: $1',
'confirmemail_invalid'     => 'Нетачан код за потврду. Могуће је да је код истекао.',
'confirmemail_needlogin'   => 'Морате да $1 да бисте потврдили адресу ваше е-поште.',
'confirmemail_success'     => 'Адреса ваше е-поште је потврђена. Можете сада да се пријавите и уживате у викију.',
'confirmemail_loggedin'    => 'Адреса ваше е-поште је сада потврђена.',
'confirmemail_error'       => 'Нешто је пошло по злу приликом снимања ваше потврде.',
'confirmemail_subject'     => '{{SITENAME}} адреса е-поште за потврђивање',
'confirmemail_body'        => 'Неко, вероватно ви, са ИП адресе $1 
је регистровао налог „$2” са овом адресом е-поште на сајту {{SITENAME}}.

Да потврдите да овај налог стварно припада вама и да активирате 
могућност е-поште на сајту {{SITENAME}}, отворите ову везу у вашем браузеру:

$3

Ако ово *нисте* ви, пратите ову везу како бисте прекинули регистрацију:

$5 

Овај код за потврду ће истећи у $4.',
'confirmemail_invalidated' => 'Овера електронске адресе је поништена.',
'invalidateemail'          => 'поништавање потврде путем имејла',

# Scary transclusion
'scarytranscludedisabled' => '[Интервики укључивање је онемогућено]',
'scarytranscludefailed'   => '[Доношење шаблона за $1 неуспешно]',
'scarytranscludetoolong'  => '[УРЛ је предугачак]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Враћања за овај чланак:<br />
$1
</div>',
'trackbackremove'   => '([$1 Брисање])',
'trackbacklink'     => 'Враћање',
'trackbackdeleteok' => 'Враћање је успешно обрисано.',

# Delete conflict
'deletedwhileediting' => "'''Упозорење''': Ова страна је обрисана пошто сте почели уређивање!",
'confirmrecreate'     => "Корисник [[User:$1|$1]] ([[User talk:$1|разговор]]) је обрисао овај чланак пошто сте почели уређивање са разлогом:
: ''$2''

Молимо потврдите да стварно желите да поново направите овај чланак.",
'recreate'            => 'Поново направи',

# HTML dump
'redirectingto' => 'Преусмеравам на [[:$1]]...',

# action=purge
'confirm_purge'        => 'Да ли желите очистити кеш ове странице?

$1',
'confirm_purge_button' => 'Да',

# AJAX search
'searchcontaining' => "Тражи чланке који садрже ''$1''.",
'searchnamed'      => "Чланци који се зову ''$1''.",
'articletitles'    => "Чланци почећи са ''$1''",
'hideresults'      => 'Сакриј резултате',
'useajaxsearch'    => 'Користи AJAX претрагу',

# Multipage image navigation
'imgmultipageprev' => '&larr; претходна страна',
'imgmultipagenext' => 'следећа страна &rarr;',
'imgmultigo'       => 'Иди!',
'imgmultigoto'     => 'Иди на страну $1',

# Table pager
'ascending_abbrev'         => 'раст',
'descending_abbrev'        => 'опад',
'table_pager_next'         => 'Следећа страна',
'table_pager_prev'         => 'Претходна страна',
'table_pager_first'        => 'Прва страница',
'table_pager_last'         => 'Последња страница',
'table_pager_limit'        => 'Прикажи $1 делова информације по страници',
'table_pager_limit_submit' => 'Иди',
'table_pager_empty'        => 'Без резултата',

# Auto-summaries
'autosumm-blank'   => 'Уклањање целокупног садржаја са стране',
'autosumm-replace' => "Замена странице са '$1'",
'autoredircomment' => 'Преусмерење на [[$1]]',
'autosumm-new'     => 'Нова страница: $1',

# Live preview
'livepreview-loading' => 'Учитавање…',
'livepreview-ready'   => 'Учитавање… Готово!',
'livepreview-failed'  => 'Брзи приказ неуспешан! Покушајте нормални приказ.',
'livepreview-error'   => 'Неуспешна конекција: $1 "$2". Пробајте нормални приказ.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Измене новије од $1 {{PLURAL:$1|секунде|секунде|секунди}} се неће приказати у списку.',
'lag-warn-high'   => 'Због великог лага базе података, измене новије од $1 {{PLURAL:$1|секунде|секунде|секунди}} се неће приказати на списку.',

# Watchlist editor
'watchlistedit-numitems'      => 'Ваш списак надгледања садржи {{PLURAL:$1|1 наслов|$1 наслова}}, искључујући странице за разговор.',
'watchlistedit-noitems'       => 'Нема наслова у вашем списку надгледања.',
'watchlistedit-normal-title'  => 'Уреди списак надгледања',
'watchlistedit-normal-legend' => 'Уклони наслове са списка надгледања',
'watchlistedit-normal-submit' => 'Уклони наслове',
'watchlistedit-raw-title'     => 'мењање сировог списка надгледања',
'watchlistedit-raw-legend'    => 'мењање сировог списка надгледања',
'watchlistedit-raw-titles'    => 'Наслови:',
'watchlistedit-raw-submit'    => 'Освежите списак надгледања',
'watchlistedit-raw-done'      => 'Ваш списак надгледања је освежен.',
'watchlistedit-raw-added'     => '{{PLURAL:$1|1 наслов је додат|$1 наслова су додата|$1 наслова је додато}}:',
'watchlistedit-raw-removed'   => '{{PLURAL:$1|1 наслов је уклоњен|$1 наслова су уклоњена|$1 наслова је уклоњено}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Преглед сродних промена',
'watchlisttools-edit' => 'Преглед и измена списка надгледања',
'watchlisttools-raw'  => 'Измена списка надгледања',

# Core parser functions
'unknown_extension_tag' => 'Непознати таг за екстензију: "$1".',

# Special:Version
'version'                          => 'Верзија', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Инсталисане екстензије',
'version-specialpages'             => 'Посебне странице',
'version-parserhooks'              => 'закачке парсера',
'version-variables'                => 'Варијабле',
'version-other'                    => 'Остало',
'version-mediahandlers'            => 'руковаоци медијима',
'version-hooks'                    => 'закачке',
'version-extension-functions'      => 'Функције додатка',
'version-parser-extensiontags'     => 'тагови екстензије Парсер',
'version-parser-function-hooks'    => 'закачке парсерове функције',
'version-skin-extension-functions' => 'екстензије функције коже',
'version-hook-name'                => 'име закачке',
'version-hook-subscribedby'        => 'пријављени',
'version-version'                  => 'Верзија',
'version-license'                  => 'Лиценца',
'version-software'                 => 'Инсталиран софтвер',
'version-software-product'         => 'Производ',
'version-software-version'         => 'Верзија',

# Special:FilePath
'filepath'        => 'Путања фајла',
'filepath-page'   => 'Фајл:',
'filepath-submit' => 'Путања',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Претражите дупликате фајлова',
'fileduplicatesearch-legend'   => 'Претражите дупликате',
'fileduplicatesearch-filename' => 'Име фајла:',
'fileduplicatesearch-submit'   => 'Претрага',
'fileduplicatesearch-info'     => '$1 × $2 поксел<br />Величина фајла: $3<br />MIME тип: $4',

# Special:SpecialPages
'specialpages'                   => 'Посебне странице',
'specialpages-group-maintenance' => 'Извештаји',
'specialpages-group-other'       => 'Остале посебне странице',
'specialpages-group-login'       => 'Пријави се / региструј се',
'specialpages-group-changes'     => 'Скорашње измене и историје',
'specialpages-group-media'       => 'Мултимедијални извештаји и записи слања',
'specialpages-group-users'       => 'Корисници и корисничка права',
'specialpages-group-highuse'     => 'Највише коришћене стране',
'specialpages-group-pages'       => 'Списак страница',
'specialpages-group-pagetools'   => 'Алатке са странице',
'specialpages-group-wiki'        => 'подаци и оруђа викија',
'specialpages-group-redirects'   => 'преусмерење посебних страна',
'specialpages-group-spam'        => 'оруђа против спама',

# Special:BlankPage
'blankpage'              => 'празна страна',
'intentionallyblankpage' => 'Ова страна је намерно остављена празном.',

);
