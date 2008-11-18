<?php
/** Bulgarian (Български)
 *
 * @ingroup Language
 * @file
 *
 * @author BloodIce
 * @author Borislav
 * @author DCLXVI
 * @author Daggerstab
 * @author Spiritia
 * @author Петър Петров
 * @author לערי ריינהארט
 */

$fallback8bitEncoding = 'windows-1251';

$namespaceNames = array(
	NS_MEDIA          => 'Медия',
	NS_SPECIAL        => 'Специални',
	NS_MAIN           => '',
	NS_TALK           => 'Беседа',
	NS_USER           => 'Потребител',
	NS_USER_TALK      => 'Потребител_беседа',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => '$1_беседа',
	NS_IMAGE          => 'Картинка',
	NS_IMAGE_TALK     => 'Картинка_беседа',
	NS_MEDIAWIKI      => 'МедияУики',
	NS_MEDIAWIKI_TALK => 'МедияУики_беседа',
	NS_TEMPLATE       => 'Шаблон',
	NS_TEMPLATE_TALK  => 'Шаблон_беседа',
	NS_HELP           => 'Помощ',
	NS_HELP_TALK      => 'Помощ_беседа',
	NS_CATEGORY       => 'Категория',
	NS_CATEGORY_TALK  => 'Категория_беседа',
);

$skinNames = array(
	'standard'    => 'Класика',
	'nostalgia'   => 'Носталгия',
	'cologneblue' => 'Кьолнско синьо',
	'monobook'    => 'Монобук',
	'myskin'      => 'Моят облик',
	'chick'       => 'Пиленце',
	'simple'      => 'Семпъл',
	'modern'      => 'Модерен',
);

$datePreferences = false;

$bookstoreList = array(
	'books.bg'   => 'http://www.books.bg/ISBN/$1',
	'Пингвините' => 'http://www.pe-bg.com/?cid=3&search_q=$1&where=ISBN&x=0&y=0**',
	'Бард'       => 'http://www.bard.bg/search/?q=$1'
);

$magicWords = array(
	'redirect'            => array( '0', '#redirect', '#пренасочване', '#виж' ),
	'notoc'               => array( '0', '__NOTOC__', '__БЕЗСЪДЪРЖАНИЕ__' ),
	'nogallery'           => array( '0', '__NOGALLERY__', '__БЕЗГАЛЕРИЯ__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__СЪССЪДЪРЖАНИЕ__' ),
	'toc'                 => array( '0', '__TOC__', '__СЪДЪРЖАНИЕ__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__БЕЗ_РЕДАКТИРАНЕ_НА_РАЗДЕЛИ__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'ТЕКУЩМЕСЕЦ' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'ТЕКУЩМЕСЕЦИМЕ' ),
	'currentmonthnamegen' => array( '1', 'CURRENTMONTHNAMEGEN', 'ТЕКУЩМЕСЕЦИМЕРОД' ),
	'currentmonthabbrev'  => array( '1', 'CURRENTMONTHABBREV', 'ТЕКУЩМЕСЕЦСЪКР' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'ТЕКУЩДЕН' ),
	'currentday2'         => array( '1', 'CURRENTDAY2', 'ТЕКУЩДЕН2' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'ТЕКУЩДЕНИМЕ' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'ТЕКУЩАГОДИНА' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'ТЕКУЩОВРЕМЕ' ),
	'currenthour'         => array( '1', 'CURRENTHOUR', 'ТЕКУЩЧАС' ),
	'numberofpages'       => array( '1', 'NUMBEROFPAGES', 'БРОЙСТРАНИЦИ' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'БРОЙСТАТИИ' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'БРОЙФАЙЛОВЕ' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'БРОЙПОТРЕБИТЕЛИ' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'БРОЙРЕДАКЦИИ' ),
	'pagename'            => array( '1', 'PAGENAME', 'СТРАНИЦА' ),
	'pagenamee'           => array( '1', 'PAGENAMEE', 'СТРАНИЦАИ' ),
	'namespace'           => array( '1', 'NAMESPACE', 'ИМЕННОПРОСТРАНСТВО' ),
	'namespacee'          => array( '1', 'NAMESPACEE', 'ИМЕННОПРОСТРАНСТВОИ' ),
	'subjectspace'        => array( '1', 'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'       => array( '1', 'SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'        => array( '1', 'FULLPAGENAME', 'ПЪЛНОИМЕ_СТРАНИЦА' ),
	'fullpagenamee'       => array( '1', 'FULLPAGENAMEE', 'ПЪЛНОИМЕ_СТРАНИЦАИ' ),
	'subpagename'         => array( '1', 'SUBPAGENAME', 'ИМЕ_ПОДСТРАНИЦА' ),
	'subpagenamee'        => array( '1', 'SUBPAGENAMEE', 'ИМЕ_ПОДСТРАНИЦАИ' ),
	'talkpagename'        => array( '1', 'TALKPAGENAME', 'ИМЕ_БЕСЕДА' ),
	'talkpagenamee'       => array( '1', 'TALKPAGENAMEE', 'ИМЕ_БЕСЕДАИ' ),
	'subjectpagename'     => array( '1', 'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'    => array( '1', 'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                 => array( '0', 'MSG:', 'СЪОБЩ:' ),
	'subst'               => array( '0', 'SUBST:', 'ЗАМЕСТ:' ),
	'msgnw'               => array( '0', 'MSGNW:', 'СЪОБЩБУ:' ),
	'img_thumbnail'       => array( '1', 'thumbnail', 'thumb', 'мини' ),
	'img_manualthumb'     => array( '1', 'thumbnail=$1', 'thumb=$1', 'мини=$1' ),
	'img_right'           => array( '1', 'right', 'вдясно', 'дясно', 'д' ),
	'img_left'            => array( '1', 'left', 'вляво', 'ляво', 'л' ),
	'img_none'            => array( '1', 'none', 'н' ),
	'img_width'           => array( '1', '$1px', '$1пкс', '$1п' ),
	'img_center'          => array( '1', 'center', 'centre', 'център', 'центр', 'ц' ),
	'img_framed'          => array( '1', 'framed', 'enframed', 'frame', 'рамка', 'врамка' ),
	'img_frameless'       => array( '1', 'frameless', 'безрамка' ),
	'img_border'          => array( '1', 'border', 'ръб', 'контур' ),
	'int'                 => array( '0', 'INT:', 'ВЪТР:' ),
	'sitename'            => array( '1', 'SITENAME', 'ИМЕНАСАЙТА' ),
	'ns'                  => array( '0', 'NS:', 'ИП:' ),
	'localurl'            => array( '0', 'LOCALURL:', 'ЛОКАЛЕНАДРЕС:' ),
	'localurle'           => array( '0', 'LOCALURLE:', 'ЛОКАЛЕНАДРЕСИ:' ),
	'server'              => array( '0', 'SERVER', 'СЪРВЪР' ),
	'servername'          => array( '0', 'SERVERNAME', 'ИМЕНАСЪРВЪРА' ),
	'scriptpath'          => array( '0', 'SCRIPTPATH', 'ПЪТДОСКРИПТА' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'ГРАМАТИКА:' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'ТЕКУЩАСЕДМИЦА' ),
	'currentdow'          => array( '1', 'CURRENTDOW', 'ТЕКУЩ_ДЕН_ОТ_СЕДМИЦАТА' ),
	'revisionid'          => array( '1', 'REVISIONID', 'ИД_НА_ВЕРСИЯТА' ),
	'revisionday'         => array( '1', 'REVISIONDAY', 'ДЕН_НА_ВЕРСИЯТА' ),
	'revisionday2'        => array( '1', 'REVISIONDAY2', 'ДЕН_НА_ВЕРСИЯТА2' ),
	'revisionmonth'       => array( '1', 'REVISIONMONTH', 'МЕСЕЦ_НА_ВЕРСИЯТА' ),
	'revisionyear'        => array( '1', 'REVISIONYEAR', 'ГОДИНА_НА_ВЕРСИЯТА' ),
	'plural'              => array( '0', 'PLURAL:', 'МН_ЧИСЛО:' ),
	'fullurl'             => array( '0', 'FULLURL:', 'ПЪЛЕН_АДРЕС:' ),
	'fullurle'            => array( '0', 'FULLURLE:', 'ПЪЛЕН_АДРЕСИ:' ),
	'lcfirst'             => array( '0', 'LCFIRST:', 'МБПЪРВА:' ),
	'ucfirst'             => array( '0', 'UCFIRST:', 'ГБПЪРВА:' ),
	'lc'                  => array( '0', 'LC:', 'МБ:' ),
	'uc'                  => array( '0', 'UC:', 'ГБ:' ),
	'raw'                 => array( '0', 'RAW:', 'НЕОБРАБ:' ),
	'displaytitle'        => array( '1', 'DISPLAYTITLE', 'ПОКАЗВ_ЗАГЛАВИЕ' ),
	'newsectionlink'      => array( '1', '__NEWSECTIONLINK__', '__ВРЪЗКА_ЗА_НОВ_РАЗДЕЛ__' ),
	'language'            => array( '0', '#LANGUAGE:', '#ЕЗИК:' ),
	'numberofadmins'      => array( '1', 'NUMBEROFADMINS', 'БРОЙАДМИНИСТРАТОРИ' ),
	'defaultsort'         => array( '1', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:', 'СОРТКАТ:' ),
	'hiddencat'           => array( '1', '__HIDDENCAT__', '__СКРИТАКАТЕГОРИЯ__' ),
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Двойни_пренасочвания' ),
	'BrokenRedirects'           => array( 'Невалидни_пренасочвания' ),
	'Disambiguations'           => array( 'Пояснителни_страници' ),
	'Userlogin'                 => array( 'Регистриране_или_влизане' ),
	'Userlogout'                => array( 'Излизане' ),
	'CreateAccount'             => array( 'Създаване_на_сметка' ),
	'Preferences'               => array( 'Настройки' ),
	'Watchlist'                 => array( 'Списък_за_наблюдение' ),
	'Recentchanges'             => array( 'Последни_промени' ),
	'Upload'                    => array( 'Качване' ),
	'Imagelist'                 => array( 'Файлове' ),
	'Newimages'                 => array( 'Нови_файлове' ),
	'Listusers'                 => array( 'Потребители' ),
	'Statistics'                => array( 'Статистика' ),
	'Randompage'                => array( 'Случайна_страница' ),
	'Lonelypages'               => array( 'Страници_сираци' ),
	'Uncategorizedpages'        => array( 'Некатегоризирани_страници' ),
	'Uncategorizedcategories'   => array( 'Некатегоризирани_категории' ),
	'Uncategorizedimages'       => array( 'Некатегоризирани_картинки' ),
	'Uncategorizedtemplates'    => array( 'Некатегоризирани_шаблони' ),
	'Unusedcategories'          => array( 'Неизползвани_категории' ),
	'Unusedimages'              => array( 'Неизползвани_картинки' ),
	'Wantedpages'               => array( 'Желани_страници' ),
	'Wantedcategories'          => array( 'Желани_категории' ),
	'Mostlinked'                => array( 'Най-препращани_страници' ),
	'Mostlinkedcategories'      => array( 'Най-препращани_категории' ),
	'Mostlinkedtemplates'       => array( 'Най-препращани_шаблони' ),
	'Mostcategories'            => array( 'Страници_с_най-много_категории' ),
	'Mostimages'                => array( 'Най-препращани_картинки' ),
	'Mostrevisions'             => array( 'Страници_с_най-много_версии' ),
	'Fewestrevisions'           => array( 'Страници_с_най-малко_версии' ),
	'Shortpages'                => array( 'Кратки_страници' ),
	'Longpages'                 => array( 'Дълги_страници' ),
	'Newpages'                  => array( 'Нови_страници' ),
	'Ancientpages'              => array( 'Стари_страници' ),
	'Deadendpages'              => array( 'Задънени_страници' ),
	'Protectedpages'            => array( 'Защитени_страници' ),
	'Protectedtitles'           => array( 'Защитени_заглавия' ),
	'Allpages'                  => array( 'Всички_страници' ),
	'Prefixindex'               => array( 'Всички_страници_(с_представка)' ),
	'Ipblocklist'               => array( 'Блокирани_потребители' ),
	'Specialpages'              => array( 'Специални_страници' ),
	'Contributions'             => array( 'Приноси' ),
	'Emailuser'                 => array( 'Писмо_на_потребител' ),
	'Confirmemail'              => array( 'Потвърждаване_на_е-поща' ),
	'Whatlinkshere'             => array( 'Какво_сочи_насам' ),
	'Recentchangeslinked'       => array( 'Свързани_промени' ),
	'Movepage'                  => array( 'Преместване_на_страница' ),
	'Blockme'                   => array( 'Блокирай_ме' ),
	'Booksources'               => array( 'Източници_на_книги' ),
	'Categories'                => array( 'Категории' ),
	'Export'                    => array( 'Изнасяне' ),
	'Version'                   => array( 'Версия' ),
	'Allmessages'               => array( 'Системни_съобщения' ),
	'Log'                       => array( 'Дневници' ),
	'Blockip'                   => array( 'Блокиране' ),
	'Undelete'                  => array( 'Възстановяване' ),
	'Import'                    => array( 'Внасяне' ),
	'Lockdb'                    => array( 'Заключване_на_БД' ),
	'Unlockdb'                  => array( 'Отключване_на_БД' ),
	'Userrights'                => array( 'Потребителски_права' ),
	'MIMEsearch'                => array( 'MIME-търсене' ),
	'FileDuplicateSearch'       => array( 'Повтарящи_се_файлове' ),
	'Unwatchedpages'            => array( 'Ненаблюдавани_страници' ),
	'Listredirects'             => array( 'Пренасочвания' ),
	'Revisiondelete'            => array( 'Изтриване_на_версии' ),
	'Unusedtemplates'           => array( 'Неизползвани_шаблони' ),
	'Randomredirect'            => array( 'Случайно_пренасочване' ),
	'Mypage'                    => array( 'Моята_страница' ),
	'Mytalk'                    => array( 'Моята_беседа' ),
	'Mycontributions'           => array( 'Моите_приноси' ),
	'Listadmins'                => array( 'Администратори' ),
	'Listbots'                  => array( 'Ботове' ),
	'Popularpages'              => array( 'Най-посещавани_страници' ),
	'Search'                    => array( 'Търсене' ),
	'Resetpass'                 => array( 'Изтриване_на_парола' ),
	'Withoutinterwiki'          => array( 'Без_междууикита' ),
	'MergeHistory'              => array( 'История_на_сливането' ),
	'Filepath'                  => array( 'Път_към_файл' ),
);

$linkTrail = '/^([a-zабвгдежзийклмнопрстуфхцчшщъыьэюя]+)(.*)$/sDu';

$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Подчертаване на препратките:',
'tog-highlightbroken'         => 'Показване на невалидните препратки <a href="#" class="new">така</a> (алтернативно: така<a href="#" class="internal">?</a>)',
'tog-justify'                 => 'Двустранно подравняване на абзаците',
'tog-hideminor'               => 'Скриване на малки редакции в последните промени',
'tog-extendwatchlist'         => 'Разширяване на списъка, така че да показва всички промени',
'tog-usenewrc'                => 'Подобряване на последните промени (Джаваскрипт)',
'tog-numberheadings'          => 'Номериране на заглавията',
'tog-showtoolbar'             => 'Помощна лента за редактиране (Джаваскрипт)',
'tog-editondblclick'          => 'Редактиране при двойно щракване (Джаваскрипт)',
'tog-editsection'             => 'Възможност за редактиране на раздел чрез препратка [редактиране]',
'tog-editsectiononrightclick' => 'Възможност за редактиране на раздел при щракване с десния бутон върху заглавие на раздел (Джаваскрипт)',
'tog-showtoc'                 => 'Показване на съдържание (за страници с повече от три раздела)',
'tog-rememberpassword'        => 'Запомняне между сесиите',
'tog-editwidth'               => 'Максимална ширина на кутията за редактиране',
'tog-watchcreations'          => 'Добавяне на създадените от мен страници към списъка ми за наблюдение',
'tog-watchdefault'            => 'Добавяне на редактираните от мен страници към списъка ми за наблюдение',
'tog-watchmoves'              => 'Добавяне на преместените от мен страници към списъка ми за наблюдение',
'tog-watchdeletion'           => 'Добавяне на изтритите от мен страници към списъка ми за наблюдение',
'tog-minordefault'            => 'Отбелязване на всички промени като малки по подразбиране',
'tog-previewontop'            => 'Показване на предварителния преглед преди текстовата кутия',
'tog-previewonfirst'          => 'Показване на предварителен преглед при първа редакция',
'tog-nocache'                 => 'Без складиране на страниците',
'tog-enotifwatchlistpages'    => 'Уведомяване по е-пощата при промяна на страница от списъка ми за наблюдение',
'tog-enotifusertalkpages'     => 'Уведомяване по е-пощата при промяна на беседата ми',
'tog-enotifminoredits'        => 'Уведомяване по е-пощата даже при малки промени',
'tog-enotifrevealaddr'        => 'Показване на електронния ми адрес в известяващите писма',
'tog-shownumberswatching'     => 'Показване на броя на потребителите, наблюдаващи дадена страница',
'tog-fancysig'                => 'Без превръщане на подписа в препратка към потребителската страница',
'tog-externaleditor'          => 'Използване на външен редактор по подразбиране',
'tog-externaldiff'            => 'Използване на външна програма за разлики по подразбиране',
'tog-showjumplinks'           => 'Показване на препратки за достъпност от типа „Към…“',
'tog-uselivepreview'          => 'Използване на бърз предварителен преглед (Джаваскрипт) (експериментално)',
'tog-forceeditsummary'        => 'Предупреждаване при празно поле за резюме на редакцията',
'tog-watchlisthideown'        => 'Скриване на моите редакции в списъка ми за наблюдение',
'tog-watchlisthidebots'       => 'Скриване на редакциите на ботове в списъка ми за наблюдение',
'tog-watchlisthideminor'      => 'Скриване на малките промени в списъка ми за наблюдение',
'tog-nolangconversion'        => 'Без преобразувания при различни езикови варианти',
'tog-ccmeonemails'            => 'Получаване на копия на писмата, които пращам на другите потребители',
'tog-diffonly'                => 'Без показване на съдържанието на страницата при преглед на разлики',
'tog-showhiddencats'          => 'Показване на скритите категории',

'underline-always'  => 'Винаги',
'underline-never'   => 'Никога',
'underline-default' => 'Според настройките на браузъра',

'skinpreview' => '(Предварителен преглед)',

# Dates
'sunday'        => 'неделя',
'monday'        => 'понеделник',
'tuesday'       => 'вторник',
'wednesday'     => 'сряда',
'thursday'      => 'четвъртък',
'friday'        => 'петък',
'saturday'      => 'събота',
'sun'           => 'нд',
'mon'           => 'пн',
'tue'           => 'вт',
'wed'           => 'ср',
'thu'           => 'чт',
'fri'           => 'пт',
'sat'           => 'сб',
'january'       => 'януари',
'february'      => 'февруари',
'march'         => 'март',
'april'         => 'април',
'may_long'      => 'май',
'june'          => 'юни',
'july'          => 'юли',
'august'        => 'август',
'september'     => 'септември',
'october'       => 'октомври',
'november'      => 'ноември',
'december'      => 'декември',
'january-gen'   => 'януари',
'february-gen'  => 'февруари',
'march-gen'     => 'март',
'april-gen'     => 'април',
'may-gen'       => 'май',
'june-gen'      => 'юни',
'july-gen'      => 'юли',
'august-gen'    => 'август',
'september-gen' => 'септември',
'october-gen'   => 'октомври',
'november-gen'  => 'ноември',
'december-gen'  => 'декември',
'jan'           => 'яну',
'feb'           => 'фев',
'mar'           => 'мар',
'apr'           => 'апр',
'may'           => 'май',
'jun'           => 'юни',
'jul'           => 'юли',
'aug'           => 'авг',
'sep'           => 'сеп',
'oct'           => 'окт',
'nov'           => 'ное',
'dec'           => 'дек',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Категория|Категории}}',
'category_header'                => 'Страници в категория „$1“',
'subcategories'                  => 'Подкатегории',
'category-media-header'          => 'Файлове в категория „$1“',
'category-empty'                 => "''В момента тази категория не съдържа страници или файлове.''",
'hidden-categories'              => '{{PLURAL:$1|Скрита категория|Скрити категории}}',
'hidden-category-category'       => 'Скрити категории', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Тази категория съдържа само една подкатегория.|{{PLURAL:$1|Показана е една|Показани са $1}} от общо $2 подкатегории на тази категория.}}',
'category-subcat-count-limited'  => 'Тази категория включва {{PLURAL:$1|следната една подкатегория|следните $1 подкатегории}}.',
'category-article-count'         => '{{PLURAL:$2|Тази категория съдържа само една страница.|{{PLURAL:$1|Показана е една|Показани са $1}} от общо $2 страници в тази категория.}}',
'category-article-count-limited' => 'Текущата категория съдържа {{PLURAL:$1|следната страница|следните $1 страници}}.',
'category-file-count'            => '{{PLURAL:$2|Тази категория съдържа само един файл.|{{PLURAL:$1|Показан е един|Показани са $1}} от общо $2 файла в тази категория.}}',
'category-file-count-limited'    => 'В текущата категория се {{PLURAL:$1|намира следният файл|намират следните $1 файла}}.',
'listingcontinuesabbrev'         => ' продълж.',

'mainpagetext'      => "<big>'''Уикито беше успешно инсталирано.'''</big>",
'mainpagedocfooter' => 'Разгледайте [http://meta.wikimedia.org/wiki/Help:Contents ръководството] за подробна информация относно използването на софтуера.

== Първи стъпки ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Конфигурационни настройки]
* [http://www.mediawiki.org/wiki/Manual:FAQ ЧЗВ за МедияУики]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Пощенски списък относно нови версии на МедияУики]',

'about'          => 'За {{SITENAME}}',
'article'        => 'Страница',
'newwindow'      => '(отваря се в нов прозорец)',
'cancel'         => 'Отказ',
'qbfind'         => 'Търсене',
'qbbrowse'       => 'Избор',
'qbedit'         => 'Редактиране',
'qbpageoptions'  => 'Тази страница',
'qbpageinfo'     => 'Информация за страницата',
'qbmyoptions'    => 'Моите страници',
'qbspecialpages' => 'Специални страници',
'moredotdotdot'  => 'Още…',
'mypage'         => 'Моята страница',
'mytalk'         => 'Моята беседа',
'anontalk'       => 'Беседа за адреса',
'navigation'     => 'Навигация',
'and'            => 'и',

# Metadata in edit box
'metadata_help' => 'Метаданни:',

'errorpagetitle'    => 'Грешка',
'returnto'          => 'Обратно към $1.',
'tagline'           => 'от {{SITENAME}}',
'help'              => 'Помощ',
'search'            => 'Търсене',
'searchbutton'      => 'Търсене',
'go'                => 'Отваряне',
'searcharticle'     => 'Отваряне',
'history'           => 'История',
'history_short'     => 'История',
'updatedmarker'     => 'има промяна (от последното ви влизане)',
'info_short'        => 'Информация',
'printableversion'  => 'Версия за печат',
'permalink'         => 'Постоянна препратка',
'print'             => 'Печат',
'edit'              => 'Редактиране',
'create'            => 'Създаване',
'editthispage'      => 'Редактиране',
'create-this-page'  => 'Създаване на страницата',
'delete'            => 'Изтриване',
'deletethispage'    => 'Изтриване',
'undelete_short'    => 'Възстановяване на {{PLURAL:$1|една редакция|$1 редакции}}',
'protect'           => 'Защита',
'protect_change'    => 'промяна',
'protectthispage'   => 'Защита',
'unprotect'         => 'Сваляне на защитата',
'unprotectthispage' => 'Сваляне на защитата',
'newpage'           => 'Нова страница',
'talkpage'          => 'Дискусионна страница',
'talkpagelinktext'  => 'Беседа',
'specialpage'       => 'Специална страница',
'personaltools'     => 'Лични инструменти',
'postcomment'       => 'Оставяне на съобщение',
'articlepage'       => 'Преглед на страница',
'talk'              => 'Беседа',
'views'             => 'Прегледи',
'toolbox'           => 'Инструменти',
'userpage'          => 'Потребителска страница',
'projectpage'       => 'Проектна страница',
'imagepage'         => 'Преглед на файла',
'mediawikipage'     => 'Преглед на съобщението',
'templatepage'      => 'Преглед на шаблона',
'viewhelppage'      => 'Преглед на помощната страница',
'categorypage'      => 'Преглед на категорията',
'viewtalkpage'      => 'Преглед на беседата',
'otherlanguages'    => 'На други езици',
'redirectedfrom'    => '(пренасочване от $1)',
'redirectpagesub'   => 'Пренасочваща страница',
'lastmodifiedat'    => 'Последна промяна на страницата: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Страницата е била преглеждана {{PLURAL:$1|един път|$1 пъти}}.',
'protectedpage'     => 'Защитена страница',
'jumpto'            => 'Направо към:',
'jumptonavigation'  => 'навигация',
'jumptosearch'      => 'търсене',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'За {{SITENAME}}',
'aboutpage'            => 'Project:За {{SITENAME}}',
'bugreports'           => 'Съобщения за грешки',
'bugreportspage'       => 'Project:Съобщения за грешки',
'copyright'            => 'Съдържанието е достъпно при условията на $1.',
'copyrightpagename'    => 'Авторски права в {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Авторски права',
'currentevents'        => 'Текущи събития',
'currentevents-url'    => 'Project:Текущи събития',
'disclaimers'          => 'Условия за ползване',
'disclaimerpage'       => 'Project:Условия за ползване',
'edithelp'             => 'Помощ при редактиране',
'edithelppage'         => 'Help:Редактиране',
'faq'                  => 'ЧЗВ',
'faqpage'              => 'Project:ЧЗВ',
'helppage'             => 'Help:Съдържание',
'mainpage'             => 'Начална страница',
'mainpage-description' => 'Начална страница',
'policy-url'           => 'Project:Политика',
'portal'               => 'Портал за общността',
'portal-url'           => 'Project:Портал',
'privacy'              => 'Защита на личните данни',
'privacypage'          => 'Project:Защита на личните данни',

'badaccess'        => 'Грешка при достъп',
'badaccess-group0' => 'Нямате права да извършите исканото действие',
'badaccess-group1' => 'Исканото действие могат да изпълнят само потребители от групата $1.',
'badaccess-group2' => 'Исканото действие могат да изпълнят само потребители от групите $1.',
'badaccess-groups' => 'Исканото действие могат да изпълнят само потребители от групите $1.',

'versionrequired'     => 'Изисква се версия $1 на МедияУики',
'versionrequiredtext' => 'Използването на тази страница изисква версия $1 на софтуера МедияУики. Вижте [[Special:Version|текущата използвана версия]].',

'ok'                      => 'Добре',
'pagetitle'               => '$1 — {{SITENAME}}',
'retrievedfrom'           => 'Взето от „$1“.',
'youhavenewmessages'      => 'Имате $1 ($2).',
'newmessageslink'         => 'нови съобщения',
'newmessagesdifflink'     => 'разлика с предишната версия',
'youhavenewmessagesmulti' => 'Имате нови съобщения в $1',
'editsection'             => 'редактиране',
'editold'                 => 'редактиране',
'viewsourceold'           => 'преглед на кода',
'editsectionhint'         => 'Редактиране на раздел: $1',
'toc'                     => 'Съдържание',
'showtoc'                 => 'показване',
'hidetoc'                 => 'скриване',
'thisisdeleted'           => 'Преглед или възстановяване на $1?',
'viewdeleted'             => 'Преглед на $1?',
'restorelink'             => '{{PLURAL:$1|една изтрита редакция|$1 изтрити редакции}}',
'feedlinks'               => 'Във вида:',
'feed-invalid'            => 'Невалиден формат на информацията',
'feed-unavailable'        => 'За {{SITENAME}} не се предлагат емисии',
'site-rss-feed'           => 'Емисия на RSS за $1',
'site-atom-feed'          => 'Емисия на Atom за $1',
'page-rss-feed'           => 'Емисия на RSS за „$1“',
'page-atom-feed'          => 'Емисия на Atom за „$1“',
'red-link-title'          => '$1 (страницата все още не съществува)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Страница',
'nstab-user'      => 'Потребител',
'nstab-media'     => 'Медия',
'nstab-special'   => 'Специална страница',
'nstab-project'   => 'Проект',
'nstab-image'     => 'Файл',
'nstab-mediawiki' => 'Съобщение',
'nstab-template'  => 'Шаблон',
'nstab-help'      => 'Помощ',
'nstab-category'  => 'Категория',

# Main script and global functions
'nosuchaction'      => 'Няма такова действие',
'nosuchactiontext'  => 'Действието, указано от мрежовия адрес, не се разпознава от системата.',
'nosuchspecialpage' => 'Няма такава специална страница',
'nospecialpagetext' => "<big>'''Отправихте заявка за невалидна специална страница.'''</big>

Списък с валидните специални страници може да бъде видян на [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Грешка',
'databaseerror'        => 'Грешка при работа с базата от данни',
'dberrortext'          => 'Възникна синтактична грешка при заявка към базата от данни.
Последната заявка към базата от данни беше:
<blockquote><code>$1</code></blockquote>
при функцията „<code>$2</code>“.
MySQL дава грешка „<code>$3: $4</code>“.',
'dberrortextcl'        => 'Възникна синтактична грешка при заявка към базата от данни.
Последната заявка беше:
„$1“
при функцията „$2“.
MySQL дава грешка „$3: $4“.',
'noconnect'            => 'В момента са на лице технически затруднения и не може да се осъществи връзка с базата данни.<br />
$1',
'nodb'                 => 'Неуспех при избирането на база от данни $1',
'cachederror'          => 'Показано е складирано копие на желаната страница, което евентуално може да е остаряло.',
'laggedslavemode'      => 'Внимание: Страницата може да не съдържа последните обновявания.',
'readonly'             => 'Базата от данни е затворена за промени',
'enterlockreason'      => 'Посочете причина за затварянето, като дадете и приблизителна оценка кога базата от данни ще бъде отново отворена',
'readonlytext'         => 'Базата от данни е временно затворена за промени — вероятно за рутинна поддръжка, след която ще бъде отново на разположение.
Администраторът, който я е затворил, дава следното обяснение:
$1',
'missingarticle-rev'   => '(версия#: $1)',
'missingarticle-diff'  => '(Разлика: $1, $2)',
'readonly_lag'         => 'Базата от данни беше автоматично заключена, докато подчинените сървъри успеят да се съгласуват с основния сървър.',
'internalerror'        => 'Вътрешна грешка',
'internalerror_info'   => 'Вътрешна грешка: $1',
'filecopyerror'        => 'Файлът „$1“ не можа да бъде копиран като „$2“.',
'filerenameerror'      => 'Файлът „$1“ не можа да бъде преименуван на „$2“.',
'filedeleteerror'      => 'Файлът „$1“ не можа да бъде изтрит.',
'directorycreateerror' => 'Невъзможно е да бъде създадена директория „$1“.',
'filenotfound'         => 'Файлът „$1“ не беше намерен.',
'fileexistserror'      => 'Невъзможност за запис във файл „$1“: файлът съществува',
'unexpected'           => 'Неочаквана стойност: „$1“=„$2“.',
'formerror'            => 'Възникна грешка при изпращане на формуляра',
'badarticleerror'      => 'Действието не може да се изпълни върху страницата.',
'cannotdelete'         => 'Указаната страница или файл не можа да бъде изтрит(а). Възможно е вече да е изтрит(а) от някой друг.',
'badtitle'             => 'Невалидно заглавие',
'badtitletext'         => 'Желаното заглавие на страница е невалидно, празно или неправилна препратка към друго уики. Възможно е да съдържа знаци, които не са позволени в заглавия.',
'perfdisabled'         => 'Съжаляваме! Това свойство е временно изключено, защото забавя базата от данни дотам, че никой не може да използва уикито.',
'perfcached'           => 'Следните данни са извлечени от склада и затова може да не отговарят на текущото състояние:',
'perfcachedts'         => 'Данните са складирани и обновени за последно на $1.',
'querypage-no-updates' => 'Обновяването на тази страница в момента е изключено. Засега данните тук няма да бъдат обновявани.',
'wrong_wfQuery_params' => 'Невалидни аргументи за wfQuery()<br />
Функция: $1<br />
Заявка: $2',
'viewsource'           => 'Преглед на кода',
'viewsourcefor'        => 'за $1',
'actionthrottled'      => 'Ограничение в скоростта',
'actionthrottledtext'  => 'Като част от защитата против спам, многократното повтаряне на това действие за кратък период от време е ограничено и вие вече сте надвишили лимита си. Опитайте отново след няколко минути.',
'protectedpagetext'    => 'Тази страница е заключена за редактиране.',
'viewsourcetext'       => 'Можете да разгледате и да копирате кодa на страницата:',
'protectedinterface'   => 'Тази страница съдържа текст, нужен за работата на системата. Тя е защитена против редактиране, за да се предотвратят възможни злоупотреби.',
'editinginterface'     => "'''Внимание:''' Редактирате страница, която се използва за интерфейса на софтуера. Промяната й ще повлияе на външния вид на уикито.",
'sqlhidden'            => '(Заявка на SQL — скрита)',
'cascadeprotected'     => 'Тази страница е защитена против редактиране, защото е включена в {{PLURAL:$1|следната страница, която от своя страна има|следните страници, които от своя страна имат}} каскадна защита:
$2',
'namespaceprotected'   => "Нямате права за редактиране на страници в именно пространство '''$1'''.",
'customcssjsprotected' => 'Нямате права за редактиране на тази страница, защото тя съдържа чужди потребителски настройки.',
'ns-specialprotected'  => 'Специалните страници не могат да бъдат редактирани.',
'titleprotected'       => "Тази страница е била защитена срещу създаване от [[User:$1|$1]].
Посочената причина е ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Лоша конфигурация: непознат скенер за вируси: <i>$1</i>',
'virus-scanfailed'     => 'сканирането не сполучи (код $1)',
'virus-unknownscanner' => 'непознат антивирус:',

# Login and logout pages
'logouttitle'                => 'Излизане от системата',
'logouttext'                 => '<strong>Излязохте от системата.</strong>

Можете да продължите да използвате {{SITENAME}} анонимно или да влезете отново като друг потребител. Обърнете внимание, че някои страници все още ще се показват така, сякаш сте влезли, докато не изтриете кеш-паметта на браузъра.',
'welcomecreation'            => '== Добре дошли, $1! ==

Вашата сметка беше успешно открита. Сега можете да промените настройките на {{SITENAME}} по ваш вкус.',
'loginpagetitle'             => 'Влизане в системата',
'yourname'                   => 'Потребителско име:',
'yourpassword'               => 'Парола:',
'yourpasswordagain'          => 'Парола (повторно):',
'remembermypassword'         => 'Запомняне на паролата',
'yourdomainname'             => 'Домейн:',
'externaldberror'            => 'Или е станала грешка в базата от данни при външното удостоверяване, или не ви е позволено да обновявате външната си сметка.',
'loginproblem'               => '<strong>Имаше проблем с влизането ви.</strong><br />Опитайте отново!',
'login'                      => 'Влизане',
'nav-login-createaccount'    => 'Регистриране или влизане',
'loginprompt'                => "За влизане в {{SITENAME}} е необходимо да въведете потребителското си име и парола и да натиснете бутона '''Влизане''', като, за да бъде това успешно, бисквитките (cookies) трябва да са разрешени в браузъра ви.

Ако все още не сте се регистрирали (нямате открита сметка), лесно можете да сторите това, като последвате препратката '''Създаване на сметка'''.",
'userlogin'                  => 'Регистриране или влизане',
'logout'                     => 'Излизане',
'userlogout'                 => 'Излизане',
'notloggedin'                => 'Не сте влезли',
'nologin'                    => 'Нямате потребителско име? $1.',
'nologinlink'                => 'Създаване на сметка',
'createaccount'              => 'Регистриране',
'gotaccount'                 => 'Имате ли вече сметка? $1.',
'gotaccountlink'             => 'Влизане',
'createaccountmail'          => 'с писмо по електронната поща',
'badretype'                  => 'Въведените пароли не съвпадат.',
'userexists'                 => 'Въведеното потребителско име вече се използва.
Изберете друго име.',
'youremail'                  => 'Е-поща:',
'username'                   => 'Потребителско име:',
'uid'                        => 'Потребителски номер:',
'prefs-memberingroups'       => 'Член на {{PLURAL:$1|група|групи}}:',
'yourrealname'               => 'Истинско име:',
'yourlanguage'               => 'Език:',
'yourvariant'                => 'Вариант',
'yournick'                   => 'Подпис:',
'badsig'                     => 'Избраният подпис не е валиден. Проверете HTML-етикетите!',
'badsiglength'               => 'Подписът е твърде дълъг.
Максимално допустимата дължина на подпис е $1 {{PLURAL:$1|знак|знака}}.',
'email'                      => 'Е-поща',
'prefs-help-realname'        => '* <strong>Истинско име</strong> <em>(незадължително)</em>: Ако го посочите, на него ще бъдат приписани вашите приноси.',
'loginerror'                 => 'Грешка при влизане',
'prefs-help-email'           => 'Електронната поща е незадължителна, но позволява да ви бъде изпратена нова парола, в случай, че забравите текущата. Можете също да изберете дали другите потребители могат да се свързват с вас без да се налага да им съобщавате адреса си.',
'prefs-help-email-required'  => 'Изисква се адрес за електронна поща.',
'nocookiesnew'               => 'Потребителската сметка беше създадена, но все още не сте влезли. {{SITENAME}} използва бисквитки при влизането на потребителите. Разрешете бисквитките във вашия браузър, тъй като те са забранени, и след това влезте с потребителското си име и парола.',
'nocookieslogin'             => '{{SITENAME}} използва бисквитки (cookies) за запис на влизанията. Разрешете бисквитките във вашия браузър, тъй като те са забранени, и опитайте отново.',
'noname'                     => 'Не указахте валидно потребителско име.',
'loginsuccesstitle'          => 'Успешно влизане',
'loginsuccess'               => "'''Влязохте в {{SITENAME}} като „$1“.'''",
'nosuchuser'                 => 'Не съществува потребител на име „$1“. Проверете изписването или [[Special:Userlogin/signup|създайте нова сметка]].',
'nosuchusershort'            => 'Не съществува потребител с името „<nowiki>$1</nowiki>“. Проверете изписването.',
'nouserspecified'            => 'Необходимо е да се посочи потребителско име.',
'wrongpassword'              => 'Въведената парола е невалидна. Опитайте отново.',
'wrongpasswordempty'         => 'Въведената парола е празна. Опитайте отново.',
'passwordtooshort'           => 'Паролата ви е прекалено къса.
Необходимо е да съдържа поне {{PLURAL:$1|1 знак|$1 знака}} и да е различна от потребителското име.',
'mailmypassword'             => 'Изпращане на нова парола',
'passwordremindertitle'      => 'Напомняне за парола от {{SITENAME}}',
'passwordremindertext'       => 'Някой (най-вероятно вие, от IP-адрес $1) е изискал нова парола за влизане в {{SITENAME}} ($4).
За потребител „$2“ е създадена временната парола „$3“.
Сега би трябвало да влезете в системата и да си изберете нова парола.

Ако заявката е направена от друг или пък сте си спомнили паролата и не искате да я променяте, можете да пренебрегнете това съобщение и да продължите да използвате старата си парола.',
'noemail'                    => 'Няма записана електронна поща за потребителя „$1“.',
'passwordsent'               => 'Нова парола беше изпратена на електронната поща на „$1“.
След като я получите, влезте отново.',
'blocked-mailpassword'       => 'Редактирането от вашия IP-адрес е забранено, затова не ви е позволено да използвате възможността за възстановяване на загубена парола.',
'eauthentsent'               => 'Писмото за потвърждение е изпратено на посочения адрес. В него са описани действията, които трябва да се извършат, за да потвърдите, че този адрес за електронна поща действително е ваш.',
'throttled-mailpassword'     => 'Функцията за напомняне на паролата е използвана през {{PLURAL:$1|последния един час|последните $1 часа}}.
За предотвратяване на злоупотреби е разрешено да се изпраща не повече от едно напомняне в рамките на {{PLURAL:$1|един час|$1 часа}}.',
'mailerror'                  => 'Грешка при изпращане на писмо: $1',
'acct_creation_throttle_hit' => 'Съжаляваме, вече сте създали $1 сметки и нямате право на повече.',
'emailauthenticated'         => 'Адресът на електронната ви поща беше потвърден на $1.',
'emailnotauthenticated'      => 'Адресът на електронната ви поща <strong>не е потвърден</strong>. Няма да получавате писма за никоя от следните възможности.',
'noemailprefs'               => '<strong>Не е указан адрес за електронна поща</strong>, функциите няма да работят.',
'emailconfirmlink'           => 'Потвърждаване на адреса за електронна поща',
'invalidemailaddress'        => 'Въведеният адрес не може да бъде приет, тъй като не съответства на формата на адрес за електронна поща. Въведете коректен адрес или оставете полето празно.',
'accountcreated'             => 'Потребителската сметка беше създадена',
'accountcreatedtext'         => 'Потребителската сметка за $1 беше създадена.',
'createaccount-title'        => 'Създаване на сметка за {{SITENAME}}',
'createaccount-text'         => 'Някой е създал сметка за $2 в {{SITENAME}} ($4) и е посочил този адрес за електронна поща. Паролата за „$2“ е „$3“. Необходимо е да влезете в системата и да смените паролата си.

Можете да пренебрегнете това съобщение, ако сметката е създадена по грешка.',
'loginlanguagelabel'         => 'Език: $1',

# Password reset dialog
'resetpass'               => 'Смяна на паролата',
'resetpass_announce'      => 'Влязохте с временен код, получен по електронната поща. Сега е нужно да си изберете нова парола:',
'resetpass_text'          => '<!-- Тук добавете текст -->',
'resetpass_header'        => 'Смяна на паролата',
'resetpass_submit'        => 'Избиране на парола и влизане',
'resetpass_success'       => 'Паролата ви беше сменена! Сега влизате…',
'resetpass_bad_temporary' => 'Невалидна временна парола. Възможно е вече да сте променили паролата си или пък да сте поискали нова временна парола.',
'resetpass_forbidden'     => 'На това уики не е разрешена смяната на парола',
'resetpass_missing'       => 'Липсват формулярни данни.',

# Edit page toolbar
'bold_sample'     => 'Получер текст',
'bold_tip'        => 'Получер (удебелен) текст',
'italic_sample'   => 'Курсивен текст',
'italic_tip'      => 'Курсивен (наклонен) текст',
'link_sample'     => 'Име на препратка',
'link_tip'        => 'Вътрешна препратка',
'extlink_sample'  => 'http://www.example.com Текст на външната препратка',
'extlink_tip'     => 'Външна препратка (не забравяйте http:// отпред)',
'headline_sample' => 'Заглавие на раздел',
'headline_tip'    => 'Заглавие',
'math_sample'     => 'Тук въведете формулата',
'math_tip'        => 'Математическа формула (LaTeX)',
'nowiki_sample'   => 'Тук въведете текст',
'nowiki_tip'      => 'Пренебрегване на форматиращите команди',
'image_sample'    => 'Пример.jpg',
'image_tip'       => 'Вмъкване на картинка',
'media_sample'    => 'Пример.ogg',
'media_tip'       => 'Препратка към файл',
'sig_tip'         => 'Вашият подпис заедно с времева отметка',
'hr_tip'          => 'Хоризонтална линия (използвайте пестеливо)',

# Edit pages
'summary'                          => 'Резюме',
'subject'                          => 'Тема/заглавие',
'minoredit'                        => 'Това е малка промяна',
'watchthis'                        => 'Наблюдаване на страницата',
'savearticle'                      => 'Съхраняване',
'preview'                          => 'Предварителен преглед',
'showpreview'                      => 'Предварителен преглед',
'showlivepreview'                  => 'Бърз предварителен преглед',
'showdiff'                         => 'Показване на промените',
'anoneditwarning'                  => "'''Внимание:''' Не сте влезли в системата. В историята на страницата ще бъде записан вашият IP-адрес.",
'missingsummary'                   => "'''Напомняне:''' Не е въведено кратко описание на промените. При повторно натискане на бутона „Съхраняване“, редакцията ще бъде съхранена без резюме.",
'missingcommenttext'               => 'По-долу въведете вашето съобщение.',
'missingcommentheader'             => "'''Напомняне:''' Не е въведено заглавие на коментара. При повторно натискане на бутона „Съхраняване“, редакцията ще бъде записана без такова.",
'summary-preview'                  => 'Предварителен преглед на резюмето',
'subject-preview'                  => 'Предварителен преглед на заглавието',
'blockedtitle'                     => 'Потребителят е блокиран',
'blockedtext'                      => "<big>'''Вашето потребителско име (или IP-адрес) беше блокирано.'''</big>

Блокирането е извършено от $1. Посочената причина е: ''$2''

*Начало на блокирането: $8
*Край на блокирането: $6
*Блокирането се отнася за: $7

Можете да се свържете с $1 или с някой от останалите [[{{MediaWiki:Grouppage-sysop}}|администратори]], за да обсъдите блокирането.

Можете да използвате услугата „Пращане писмо на потребител“ само ако не ви е забранена употребата й и ако сте посочили валидна електронна поща в [[Special:Preferences|настройките]] си.

Вашият IP адрес е $3, а номерът на блокирането е $5. Включвайте едно от двете или и двете във всяко запитване, което правите.",
'autoblockedtext'                  => "Вашият IP-адрес беше блокиран автоматично, защото е бил използван от друг потребител, който е бил блокиран от $1.
Посочената причина е:

:''$2''

* Начало на блокирането: $8
* Край на блокирането: $6
* Блокирането се отнася за: $7

Можете да се свържете с $1 или с някой от останалите [[{{MediaWiki:Grouppage-sysop}}|администратори]], за да обсъдите блокирането.

Можете да използвате услугата „Пращане писмо на потребител“ само ако не ви е забранена употребата й и ако сте посочили валидна електронна поща в [[Special:Preferences|настройките]] си.

Текущият ви IP-адрес е $3, а номерът на блокирането ви е $5. Включвайте ги във всяко питане, което правите.",
'blockednoreason'                  => 'не е указана причина',
'blockedoriginalsource'            => "По-долу е показано съдържанието на '''$1''':",
'blockededitsource'                => "По-долу е показан текстът на '''вашите редакции''' на '''$1''':",
'whitelistedittitle'               => 'Необходимо е да влезете, за да може да редактирате',
'whitelistedittext'                => 'Редактирането на страници изисква $1 в системата.',
'confirmedittitle'                 => 'Необходимо е потвърждение на адреса ви за електронна поща',
'confirmedittext'                  => 'Необходимо е да потвърдите електронната си поща, преди да редактирате страници.
Въведете и потвърдете адреса си на [[Special:Preferences|страницата с настройките]].',
'nosuchsectiontitle'               => 'Няма такъв раздел',
'nosuchsectiontext'                => 'Опитахте да редактирате несъществуващия раздел $1. Поради тази причина е невъзможно редакцията ви да бъде съхранена.',
'loginreqtitle'                    => 'Изисква се влизане',
'loginreqlink'                     => 'влизане',
'loginreqpagetext'                 => 'Необходимо е да $1, за да може да разглеждате други страници.',
'accmailtitle'                     => 'Паролата беше изпратена.',
'accmailtext'                      => 'Паролата за „$1“ беше изпратена на $2.',
'newarticle'                       => '(нова)',
'newarticletext'                   => 'Последвахте препратка към страница, която все още не съществува.
За да я създадете, просто започнете да пишете в долната текстова кутия
(вижте [[{{MediaWiki:Helppage}}|помощната страница]] за повече информация).',
'anontalkpagetext'                 => "----''Това е дискусионната страница на анонимен потребител, който все още няма регистрирана сметка или не я използва, затова се налага да използваме IP-адрес, за да го идентифицираме. Такъв адрес може да се споделя от няколко потребители.''

''Ако сте анонимен потребител и мислите, че тези неуместни коментари са отправени към вас, [[Special:UserLogin/signup|регистрирайте се]] или [[Special:UserLogin|влезте в системата]], за да избегнете евентуално бъдещо объркване с други анонимни потребители.''",
'noarticletext'                    => "(Тази страница все още не съществува. Можете да [[Special:Search/{{PAGENAME}}|потърсите за заглавието на страницата]] в други страници или да създадете страницата като щракнете на '''Редактиране'''.)",
'userpage-userdoesnotexist'        => 'Няма регистрирана потребителска сметка за „$1“. Изисква се потвърждение, че желаете да създадете/редактирате тази страница?',
'clearyourcache'                   => "'''Бележка:''' След съхранението е необходимо да изтриете кеша на браузъра, за да видите промените:
'''Mozilla / Firefox / Safari:''' натиснете бутона ''Shift'' и щракнете върху ''Презареждане'' (''Reload''), или изберете клавишната комбинация ''Ctrl-Shift-R'' (''Cmd-Shift-R'' за Apple Mac);
'''IE:''' натиснете ''Ctrl'' и щракнете върху ''Refresh'', или клавишната комбинация ''CTRL-F5'';
'''Konqueror:''' щракнете върху ''Презареждане'' или натиснете ''F5'';
'''Opera:''' вероятно е необходимо да изчистите кеша през менюто ''Tools→Preferences''.",
'usercssjsyoucanpreview'           => '<strong>Съвет:</strong> Използвайте бутона „Предварителен преглед“, за да изпробвате новия код на CSS/Джаваскрипт преди съхранението.',
'usercsspreview'                   => "'''Не забравяйте, че това е само предварителен преглед на кода на CSS. Страницата все още не е съхранена!'''",
'userjspreview'                    => "'''Не забравяйте, че това е само изпробване/предварителен преглед на кода на Джаваскрипт. Страницата все още не е съхранена!'''",
'userinvalidcssjstitle'            => "'''Внимание:''' Не съществува облик „$1“. Необходимо е да се знае, че имената на потребителските ви страници за CSS и Джаваскрипт трябва да се състоят от малки букви, например: „{{ns:user}}:Иван/monobook.css“ (а не „{{ns:user}}:Иван/Monobook.css“).",
'updated'                          => '(обновена)',
'note'                             => '<strong>Забележка:</strong>',
'previewnote'                      => '<strong>Това е само предварителен преглед. Промените все още не са съхранени!</strong>',
'previewconflict'                  => 'Този предварителен преглед отразява текста в горната текстова кутия така, както би се показал, ако съхраните.',
'session_fail_preview'             => '<strong>За съжаление редакцията ви не успя да бъде обработена поради загуба на данните за текущата сесия. Опитайте отново. Ако все още не работи, опитайте да [[Special:UserLogout|излезете]] и да влезете отново.</strong>',
'session_fail_preview_html'        => "<strong>За съжаление редакцията ви не беше записана поради изтичането на сесията ви.</strong>

''Тъй като {{SITENAME}} приема обикновен HTML, предварителният преглед е скрит като предпазна мярка срещу атаки чрез Джаваскрипт.''

<strong>Опитайте отново. Ако все още не сработва, пробвайте да [[Special:UserLogout|излезете]] и влезете отново.</strong>",
'token_suffix_mismatch'            => '<strong>Редакцията ви беше отхвърлена, защото браузърът ви е развалил пунктуационните знаци в редакционната отметка. Евентуалното съхранение би унищожило съдържанието на страницата. Понякога това се случва при използването на грешно работещи анонимни междинни сървъри.</strong>',
'editing'                          => 'Редактиране на „$1“',
'editingsection'                   => 'Редактиране на „$1“ (раздел)',
'editingcomment'                   => 'Редактиране на „$1“ (нов раздел)',
'editconflict'                     => 'Различна редакция: $1',
'explainconflict'                  => "Някой друг вече е променил тази страница, откакто започнахте да я редактирате.
Горната текстова кутия съдържа текущия текст на страницата без вашите промени, които са показани в долната кутия.
За да бъдат и те съхранени, е необходимо ръчно да ги преместите в горното поле, тъй като '''единствено''' текстът в него ще бъде съхранен при натискането на бутона „Съхраняване“.",
'yourtext'                         => 'Вашият текст',
'storedversion'                    => 'Съхранена версия',
'nonunicodebrowser'                => '<strong>ВНИМАНИЕ: Браузърът ви не поддържа Уникод. За да можете спокойно да редактирате страници, всички символи, невключени в ASCII-таблицата, ще бъдат заменени с шестнадесетични кодове.</strong>',
'editingold'                       => '<strong>ВНИМАНИЕ: Редактирате остаряла версия на страницата.
Ако съхраните, всякакви промени, направени след тази версия, ще бъдат изгубени.</strong>',
'yourdiff'                         => 'Разлики',
'copyrightwarning'                 => 'Обърнете внимание, че всички приноси към {{SITENAME}} се публикуват при условията на $2 (за подробности вижте $1).
Ако не сте съгласни вашата писмена работа да бъде променяна и разпространявана без ограничения, не я публикувайте.<br />

Също потвърждавате, че <strong>вие</strong> сте написали материала или сте използвали <strong>свободни ресурси</strong> — <em>обществено достояние</em> или друг свободен източник.
Ако сте ползвали чужди материали, за които имате разрешение, непременно посочете източника.

<div style="font-variant:small-caps"><strong>Не публикувайте произведения с авторски права без разрешение!</strong></div>',
'copyrightwarning2'                => 'Обърнете внимание, че всички приноси към {{SITENAME}} могат да бъдат редактирани, променяни или премахвани от останалите сътрудници.
Ако не сте съгласни вашата писмена работа да бъде променяна без ограничения, не я публикувайте.<br />
Също потвърждавате, че <strong>вие</strong> сте написали материала или сте използвали <strong>свободни ресурси</strong> — <em>обществено достояние</em> или друг свободен източник (за подробности вижте $1).
Ако сте ползвали чужди материали, за които имате разрешение, непременно посочете източника.

<div style="font-variant:small-caps"><strong>Не публикувайте произведения с авторски права без разрешение!</strong></div>',
'longpagewarning'                  => '<strong>ВНИМАНИЕ: Страницата има размер $1 килобайта; някои браузъри могат да имат проблеми при редактиране на страници по-големи от 32 KB.
Обмислете дали страницата не може да се раздели на няколко по-малки части.</strong>',
'longpageerror'                    => '<strong>ГРЕШКА: Текстът, който пращате, е с големина $1 килобайта, което надвишава позволения максимум от $2 килобайта. Заради това не може да бъде съхранен.</strong>',
'readonlywarning'                  => '<strong>ВНИМАНИЕ: Базата от данни беше затворена за поддръжка, затова в момента промените ви не могат да бъдат съхранени. Ако желаете, можете да съхраните страницата като текстов файл и да се опитате да я публикувате по-късно.</strong>',
'protectedpagewarning'             => '<strong>ВНИМАНИЕ: Страницата е защитена и само администратори могат да я редактират.</strong>',
'semiprotectedpagewarning'         => "'''Забележка:''' Страница е защитена, като само регистрирани потребители могат да я редактират.",
'cascadeprotectedwarning'          => "'''Внимание:''' Страницата е защитена, като само потребители с администраторски права могат да я редактират. Тя е включена в {{PLURAL:$1|следната страница|следните страници}} с каскадна защита:",
'titleprotectedwarning'            => '<strong>ВНИМАНИЕ:  Тази страница беше заключена и само някои потребители могат да я създадат.</strong>',
'templatesused'                    => 'Шаблони, използвани на страницата:',
'templatesusedpreview'             => 'Шаблони, използвани в предварителния преглед:',
'templatesusedsection'             => 'Шаблони, използвани в този раздел:',
'template-protected'               => '(защитен)',
'template-semiprotected'           => '(полузащитен)',
'hiddencategories'                 => 'Тази страница е включена в {{PLURAL:$1|Една скрита категория|$1 скрити категории}}:',
'edittools'                        => '<!-- Евентуален текст тук ще бъде показван под формулярите за редактиране и качване. -->',
'nocreatetitle'                    => 'Създаването на страници е ограничено',
'nocreatetext'                     => 'Създаването на нови страници в {{SITENAME}} е ограничено. Можете да се върнете назад и да редактирате някоя от съществуващите страници, [[Special:UserLogin|да се регистрирате или да създадете нова потребителска сметка]].',
'nocreate-loggedin'                => 'Нямате необходимите права да създавате нови страници в {{SITENAME}}.',
'permissionserrors'                => 'Грешки при правата на достъп',
'permissionserrorstext'            => 'Нямате правата да извършите това действие по {{PLURAL:$1|следната причина|следните причини}}:',
'permissionserrorstext-withaction' => 'Нямате разрешение за $2 поради {{PLURAL:$1|следната причина|следните причини}}:',
'recreate-deleted-warn'            => "'''Внимание: Създавате страница, която по-рано вече е била изтрита.'''

Обмислете добре дали е уместно повторното създаване на страницата.
За ваша информация по-долу е посочена причината за предишното изтриване на страницата:",

# Parser/template warnings
'expensive-parserfunction-warning'       => 'Внимание: Тази страница прекалено много пъти използва ресурсоемки парсерни функции.

В момента има $1, трябва да са по-малко от $2.',
'expensive-parserfunction-category'      => 'Страници, които прекалено много пъти използват ресурсоемки парсерни функции',
'post-expand-template-inclusion-warning' => 'Внимание: Размерът за включване на този шаблон е твърде голям.
Някои шаблони няма да бъдат включени.',
'post-expand-template-argument-category' => 'Страници, съдържащи шаблони с пропуснати параметри',

# "Undo" feature
'undo-success' => 'Редакцията може да бъде върната. Прегледайте долното сравнение и се уверете, че наистина искате да го направите. След това съхранете страницата, за да извършите връщането.',
'undo-failure' => 'Редакцията не може да бъде върната поради конфликтни междинни редакции.',
'undo-norev'   => 'Редакцията не може да бъде върната тъй като не съществува или е била изтрита.',
'undo-summary' => 'Премахната редакция $1 на [[Special:Contributions/$2|$2]] ([[User talk:$2|беседа]])',

# Account creation failure
'cantcreateaccounttitle' => 'Невъзможно е да бъде създадена потребителска сметка.',
'cantcreateaccount-text' => "[[User:$3|Потребител:$3]] е блокирал(а) създаването на сметки от този IP-адрес ('''$1''').

Причината, изложена от $3, е ''$2''",

# History pages
'viewpagelogs'        => 'Преглед на извършените административни действия по страницата',
'nohistory'           => 'Няма редакционна история за тази страница.',
'revnotfound'         => 'Версията не е открита',
'revnotfoundtext'     => 'Желаната стара версия на страницата не беше открита. Проверете адреса, който използвахте за достъп до страницата.',
'currentrev'          => 'Текуща версия',
'revisionasof'        => 'Версия от $1',
'revision-info'       => 'Версия от $1 на $2',
'previousrevision'    => '←По-стара версия',
'nextrevision'        => 'По-нова версия→',
'currentrevisionlink' => 'преглед на текущата версия',
'cur'                 => 'тек',
'next'                => 'след',
'last'                => 'пред',
'page_first'          => 'първа',
'page_last'           => 'последна',
'histlegend'          => '<em>Разлики:</em> Изберете версиите, които желаете да сравните, чрез превключвателите срещу тях и натиснете &lt;Enter&gt; или бутона за сравнение.<br />
<em>Легенда:</em> (<strong>тек</strong>) = разлика с текущата версия, (<strong>пред</strong>) = разлика с предишната версия, <strong>м</strong>&nbsp;=&nbsp;малка промяна',
'deletedrev'          => '[изтрита]',
'histfirst'           => 'Първи',
'histlast'            => 'Последни',
'historysize'         => '({{PLURAL:$1|1 байт|$1 байта}})',
'historyempty'        => '(празна)',

# Revision feed
'history-feed-title'          => 'Редакционна история',
'history-feed-description'    => 'Редакционна история на страницата в {{SITENAME}}',
'history-feed-item-nocomment' => '$1 в $2', # user at time
'history-feed-empty'          => 'Исканата страница не съществува — може да е била изтрита или преименувана. Опитайте да [[Special:Search|потърсите]] нови страници, които биха могли да са ви полезни.',

# Revision deletion
'rev-deleted-comment'         => '(коментарът е изтрит)',
'rev-deleted-user'            => '(името на автора е изтрито)',
'rev-deleted-event'           => '(записът е изтрит)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Тази версия на страницата беше премахната от общодостъпния архив.
Възможно да има повече подробности в [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} дневника на изтриванията].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Тази версия на страницата е изтрита от общодостъпния архив.
Като администратор на този сайт, вие можете да я видите;
Възможно е обяснения да има в [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} дневника на изтриванията].
</div>',
'rev-delundel'                => 'показване/скриване',
'revisiondelete'              => 'Изтриване/възстановяване на версии',
'revdelete-nooldid-title'     => 'Не е зададена версия',
'revdelete-nooldid-text'      => 'Не сте задали версия или версии за изпълнението на тази функция.',
'revdelete-selected'          => "{{PLURAL:$2|Избрана версия|Избрани версии}} от '''$1:'''",
'logdelete-selected'          => '{{PLURAL:$1|Избрано събитие|Избрани събития}}:',
'revdelete-text'              => 'Изтритите версии ше се показват в историята на страницата, но тяхното съдържание ще бъде недостъпно за обикновенните потребители.

Администраторите на това уики имат достъп до скритото съдържание и могат да го възстановят, с изключение на случаите, когато има наложено допълнително ограничение.',
'revdelete-legend'            => 'Задаване на ограничения:',
'revdelete-hide-text'         => 'Скриване на текста на версията',
'revdelete-hide-name'         => 'Скриване на действието и целта',
'revdelete-hide-comment'      => 'Скриване на коментара',
'revdelete-hide-user'         => 'Скриване на името/IP-адреса на автора',
'revdelete-hide-restricted'   => 'Прилагане на тези ограничения и към администраторите',
'revdelete-suppress'          => 'Скриване на причината за изтриването и от администраторите',
'revdelete-hide-image'        => 'Скриване на файловото съдържание',
'revdelete-unsuppress'        => 'Премахване на ограниченията за възстановените версии',
'revdelete-log'               => 'Коментар:',
'revdelete-submit'            => 'Прилагане към избраната версия',
'revdelete-logentry'          => 'промени видимостта на версия на [[$1]]',
'logdelete-logentry'          => 'промени видимостта на събитие за [[$1]]',
'revdelete-success'           => 'Видимостта на версията беше променена.',
'logdelete-success'           => 'Видимостта на събитието беше променена.',
'revdel-restore'              => 'Промяна на видимостта',
'pagehist'                    => 'История на страницата',
'deletedhist'                 => 'Изтрита история',
'revdelete-content'           => 'съдържание',
'revdelete-summary'           => 'резюме',
'revdelete-uname'             => 'потребителско име',
'revdelete-restricted'        => 'добавени ограничения за администраторите',
'revdelete-unrestricted'      => 'премахнати ограничения за администраторите',
'revdelete-hid'               => 'скри $1',
'revdelete-unhid'             => 'разкри $1',
'revdelete-log-message'       => '$1 за $2 {{PLURAL:$2|версия|версии}}',
'logdelete-log-message'       => '$1 за $2 {{PLURAL:$2|събитие|събития}}',

# Suppression log
'suppressionlog'     => 'Дневник на премахванията',
'suppressionlogtext' => 'По-долу е посочен списък на изтривания и блокирания, свързан със съдържание, скрито от администраторите.
За текущите блокирания и забрани, вижте [[Special:IPBlockList|списъка с блокираните IP адреси]].',

# History merging
'mergehistory'                     => 'Сливане на редакционни истории',
'mergehistory-header'              => 'Тази страница ви позволява да сливате историята на редакциите на дадена изходна страница с историята на нова страница. Уверете се, че тази промяна ще запази целостта на историята.',
'mergehistory-box'                 => 'Сливане на редакционните истории на две страници:',
'mergehistory-from'                => 'Изходна страница:',
'mergehistory-into'                => 'Целева страница:',
'mergehistory-list'                => 'Сливаема редакционна история',
'mergehistory-merge'               => 'Следните версии на [[:$1]] могат да бъдат прехвърлени към [[:$2]]. Използвайте колонката с превключвателите, за да прехвърлите само тези версии, създадени преди или през избрания времеви период. Обърнете внимание, че ползването на навигационните препратки, ще премахне избраното в тази колонка.',
'mergehistory-go'                  => 'Показване на редакциите, които могат да се слеят',
'mergehistory-submit'              => 'Сливане на редакции',
'mergehistory-empty'               => 'Няма редакции, които могат да бъдат слети.',
'mergehistory-success'             => '$3 {{PLURAL:$3|версия|версии}} от [[:$1]] бяха успешно слети с редакционната история на [[:$2]].',
'mergehistory-fail'                => 'Невъзможно е да се извърши сливане на редакционните истории; проверете страницата и времевите параметри.',
'mergehistory-no-source'           => 'Изходната страница $1 не съществува.',
'mergehistory-no-destination'      => 'Целевата страница $1 не съществува.',
'mergehistory-invalid-source'      => 'Изходната страница трябва да притежава коректно име.',
'mergehistory-invalid-destination' => 'Целевата страница трябва да притежава коректно име.',
'mergehistory-autocomment'         => 'Слята [[:$1]] в [[:$2]]',
'mergehistory-comment'             => 'Слята [[:$1]] в [[:$2]]: $3',

# Merge log
'mergelog'           => 'Дневник на сливанията',
'pagemerge-logentry' => 'обедини [[$1]] с [[$2]] (до редакция $3)',
'revertmerge'        => 'Разделяне',
'mergelogpagetext'   => 'Страницата съдържа списък с последните сливания на редакционни истории.',

# Diffs
'history-title'           => 'Преглед на историята на „$1“',
'difference'              => '(Разлики между версиите)',
'lineno'                  => 'Ред $1:',
'compareselectedversions' => 'Сравнение на избраните версии',
'editundo'                => 'връщане',
'diff-multi'              => '({{PLURAL:$1|Една междинна версия не е показана|$1 междинни версии не са показани}}.)',

# Search results
'searchresults'             => 'Резултати от търсенето',
'searchresulttext'          => 'За повече информация относно търсенето в {{SITENAME}}, вижте [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'За заявка „[[:$1]]“',
'searchsubtitleinvalid'     => 'За заявка „$1“',
'noexactmatch'              => "В {{SITENAME}} не съществува страница с това заглавие. Можете да я '''[[:$1|създадете]]'''.",
'noexactmatch-nocreate'     => "'''Не съществува страница „$1“.'''",
'toomanymatches'            => 'Бяха открити твърде много съвпадения, опитайте с различна заявка',
'titlematches'              => 'Съответствия в заглавията на страници',
'notitlematches'            => 'Няма съответствия в заглавията на страници',
'textmatches'               => 'Съответствия в текста на страници',
'notextmatches'             => 'Няма съответствия в текста на страници',
'prevn'                     => 'предишни $1',
'nextn'                     => 'следващи $1',
'viewprevnext'              => 'Преглед ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|една дума|$2 думи}})',
'search-result-score'       => 'Релевантност: $1%',
'search-redirect'           => '(пренасочване $1)',
'search-section'            => '(раздел $1)',
'search-suggest'            => 'Вероятно имахте предвид: $1',
'search-interwiki-caption'  => 'Сродни проекти',
'search-interwiki-default'  => '$1 резултата:',
'search-interwiki-more'     => '(още)',
'search-mwsuggest-enabled'  => 'с предположения',
'search-mwsuggest-disabled' => 'без предположения',
'search-relatedarticle'     => 'Свързани',
'mwsuggest-disable'         => 'Изключване на AJAX предположенията',
'searchrelated'             => 'свързани',
'searchall'                 => 'всички',
'showingresults'            => "Показване на до {{PLURAL:$1|'''1''' резултат|'''$1''' резултата}}, като се започва от номер '''$2'''.",
'showingresultsnum'         => "Показване на {{PLURAL:$3|'''1''' резултат|'''$3''' резултата}}, като се започва от номер '''$2'''.",
'showingresultstotal'       => "По-долу {{PLURAL:$3|е показан резултат '''$1''' от '''$3'''|са показани резултати от '''$1''' до '''$2''' от общо '''$3'''}}",
'nonefound'                 => "'''Забележка''': Безрезултатните търсения често са причинени от това, че се търсят основни думи като „има“ или „от“, които не се индексират, или от това, че се търсят повече от една думи, тъй като се показват само страници, съдържащи всички зададени понятия.",
'powersearch'               => 'Търсене',
'powersearch-legend'        => 'Разширено търсене',
'powersearch-ns'            => 'Търсене в именни пространства:',
'powersearch-redir'         => 'Списък на пренасочванията',
'powersearch-field'         => 'Търсене на',
'search-external'           => 'Външно търсене',
'searchdisabled'            => 'Търсенето в {{SITENAME}} е временно изключено. Междувременно можете да търсите чрез Google. Обърнете внимание, че съхранените при тях страници най-вероятно са остарели.',

# Preferences page
'preferences'              => 'Настройки',
'mypreferences'            => 'Моите настройки',
'prefs-edits'              => 'Брой редакции:',
'prefsnologin'             => 'Не сте влезли',
'prefsnologintext'         => 'Необходимо е да <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} влезете]</span>, за да може да променяте потребителските си настройки.',
'prefsreset'               => 'Текущите промени бяха отменени.',
'qbsettings'               => 'Лента за бърз избор',
'qbsettings-none'          => 'Без меню',
'qbsettings-fixedleft'     => 'Неподвижно вляво',
'qbsettings-fixedright'    => 'Неподвижно вдясно',
'qbsettings-floatingleft'  => 'Плаващо вляво',
'qbsettings-floatingright' => 'Плаващо вдясно',
'changepassword'           => 'Смяна на парола',
'skin'                     => 'Облик',
'math'                     => 'Математически формули',
'dateformat'               => 'Формат на датата',
'datedefault'              => 'Без предпочитание',
'datetime'                 => 'Дата и час',
'math_failure'             => 'Неуспех при разбора',
'math_unknown_error'       => 'непозната грешка',
'math_unknown_function'    => 'непозната функция',
'math_lexing_error'        => 'лексикална грешка',
'math_syntax_error'        => 'синтактична грешка',
'math_image_error'         => 'Превръщането към PNG не сполучи. Проверете дали latex, dvips, gs и convert са правилно инсталирани.',
'math_bad_tmpdir'          => 'Невъзможно е писането във или създаването на временна директория за математическите операции',
'math_bad_output'          => 'Невъзможно е писането във или създаването на изходяща директория за математическите операции',
'math_notexvc'             => 'Липсва изпълнимият файл на texvc. Прегледайте math/README за информация относно конфигурирането.',
'prefs-personal'           => 'Потребителски данни',
'prefs-rc'                 => 'Последни промени',
'prefs-watchlist'          => 'Списък за наблюдение',
'prefs-watchlist-days'     => 'Брой дни, които да се показват в списъка за наблюдение:',
'prefs-watchlist-edits'    => 'Брой редакции, които се показват в разширения списък за наблюдение:',
'prefs-misc'               => 'Други настройки',
'saveprefs'                => 'Съхраняване',
'resetprefs'               => 'Отмяна на текущите промени',
'oldpassword'              => 'Стара парола:',
'newpassword'              => 'Нова парола:',
'retypenew'                => 'Нова парола повторно:',
'textboxsize'              => 'Редактиране',
'rows'                     => 'Редове:',
'columns'                  => 'Колони:',
'searchresultshead'        => 'Търсене',
'resultsperpage'           => 'Резултати на страница:',
'contextlines'             => 'Редове за резултат:',
'contextchars'             => 'Знаци от контекста на ред:',
'stub-threshold'           => 'Праг за форматиране на <a href="#" class="stub">препратки към мъничета</a>:',
'recentchangesdays'        => 'Брой дни в последни промени:',
'recentchangescount'       => 'Брой редакции в последни промени:',
'savedprefs'               => 'Вашите настройки бяха съхранени.',
'timezonelegend'           => 'Часова зона',
'timezonetext'             => 'Броят часове, с които вашето местно време се различава от това на сървъра (UTC).',
'localtime'                => 'Местно време',
'timezoneoffset'           => 'Отместване¹',
'servertime'               => 'Време на сървъра',
'guesstimezone'            => 'Попълване чрез браузъра',
'allowemail'               => 'Възможност за получаване на писма от други потребители',
'prefs-searchoptions'      => 'Настройки за търсене',
'prefs-namespaces'         => 'Именни пространства',
'defaultns'                => 'Търсене в тези именни пространства по подразбиране:',
'default'                  => 'по подразбиране',
'files'                    => 'Файлове',

# User rights
'userrights'                  => 'Управление на потребителските права', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Управляване на потребителските групи',
'userrights-user-editname'    => 'Потребителско име:',
'editusergroup'               => 'Редактиране на потребителските групи',
'editinguser'                 => "Промяна на потребителските права на потребител '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Редактиране на потребителските групи',
'saveusergroups'              => 'Съхраняване на потребителските групи',
'userrights-groupsmember'     => 'Член на:',
'userrights-groups-help'      => 'Можете да променяте групите, в които е потребителят:
* Поставена отметка означава, че потребителят е член на групата.
* Поле без отметка означава, че потребителят не е член на групата.
* Символът * показва, че не можете да премахнете групата, след като е вече добавена (или обратно).',
'userrights-reason'           => 'Причина за промяната:',
'userrights-no-interwiki'     => 'Нямате права да редактирате потребителските групи на други уикита.',
'userrights-nodatabase'       => 'Базата данни $1 не съществува или не е на локалния сървър.',
'userrights-nologin'          => 'За управление на потребителските права е необходимо [[Special:UserLogin|влизане]] с администраторска сметка.',
'userrights-notallowed'       => 'Не ви е позволено да променяте потребителски права.',
'userrights-changeable-col'   => 'Групи, които можете да променяте',
'userrights-unchangeable-col' => 'Групи, които не можете да променяте',

# Groups
'group'               => 'Потребителска група:',
'group-user'          => 'Потребители',
'group-autoconfirmed' => 'Автоматично одобрени потребители',
'group-bot'           => 'Ботове',
'group-sysop'         => 'Администратори',
'group-bureaucrat'    => 'Бюрократи',
'group-suppress'      => 'Ревизори',
'group-all'           => '(всички)',

'group-user-member'          => 'Потребител',
'group-autoconfirmed-member' => 'Автоматично одобрен потребител',
'group-bot-member'           => 'Бот',
'group-sysop-member'         => 'Администратор',
'group-bureaucrat-member'    => 'Бюрократ',
'group-suppress-member'      => 'Ревизор',

'grouppage-user'          => '{{ns:project}}:Потребители',
'grouppage-autoconfirmed' => '{{ns:project}}:Автоматично одобрени потребители',
'grouppage-bot'           => '{{ns:project}}:Ботове',
'grouppage-sysop'         => '{{ns:project}}:Администратори',
'grouppage-bureaucrat'    => '{{ns:project}}:Бюрократи',
'grouppage-suppress'      => '{{ns:project}}:Ревизори',

# Rights
'right-read'                 => 'четене на страници',
'right-edit'                 => 'редактиране на страници',
'right-createpage'           => 'създаване на страници (които не са беседи)',
'right-createtalk'           => 'създаване на дискусионни страници',
'right-createaccount'        => 'създаване на нови потребителски сметки',
'right-minoredit'            => 'отбелязване като малка промяна',
'right-move'                 => 'преместване на страници',
'right-move-subpages'        => 'преместване на страници и техните подстраници',
'right-suppressredirect'     => 'без създаване на пренасочване от старото име при преместване на страница',
'right-upload'               => 'качване на файлове',
'right-reupload'             => 'презаписване на съществуващ файл',
'right-reupload-own'         => 'Презаписване на съществуващ файл, качен от същия потребител',
'right-upload_by_url'        => 'качване на файл от URL адрес',
'right-purge'                => 'изчистване на складираното съдържание на страниците без показване на страница за потвърждение',
'right-autoconfirmed'        => 'редактиране на полузащитени страници',
'right-bot'                  => 'третиране като авоматизиран процес',
'right-apihighlimits'        => 'използване на крайните предели в API заявките',
'right-writeapi'             => 'Употреба на API за писане',
'right-delete'               => 'изтриване на страници',
'right-bigdelete'            => 'изтриване на страници с големи редакционни истории',
'right-deleterevision'       => 'изтриване и възстановяване на отделни версии на страниците',
'right-deletedhistory'       => 'преглеждане на записи от изтрити редакционни истории без асоциирания към тях текст',
'right-browsearchive'        => 'търсене на изтрити страници',
'right-undelete'             => 'възстановяване на страници',
'right-suppressrevision'     => 'преглед и възстановяване на версии, скрити от администраторите',
'right-suppressionlog'       => 'преглеждане на тайните дневници',
'right-block'                => 'спиране на достъпа до редактиране',
'right-blockemail'           => 'блокиране на потребители да изпращат писма по е-поща',
'right-hideuser'             => 'блокиране и скриване на потребителско име',
'right-ipblock-exempt'       => 'пренебрегване на блокирания по IP blocks, автоматични блокирания и блокирани IP интервали',
'right-proxyunbannable'      => 'пренебрегване на автоматичното блокиране на проксита',
'right-protect'              => 'променяне на нивото на защита и редактиране на защитени страници',
'right-editprotected'        => 'редактиране на защитени страници (без каскадна защита)',
'right-editinterface'        => 'редактиране на интерфейса',
'right-editusercssjs'        => 'редактиране на CSS и JS файловете на други потребители',
'right-rollback'             => 'Бърза отмяна на промените, направени от последния потребител, редактирал дадена страница',
'right-markbotedits'         => 'отбелязване на възвърнатите редакции като редакции на ботове',
'right-noratelimit'          => 'Пренебрегване на всякакви ограничения',
'right-import'               => 'внасяне на страници от други уикита',
'right-importupload'         => 'внасяне на страници от качен файл',
'right-patrol'               => 'отбелязване на редакциите като проверени',
'right-autopatrol'           => 'автоматично отбелязване на редакции като проверени',
'right-unwatchedpages'       => 'преглеждане на списъка с ненаблюдаваните страници',
'right-trackback'            => 'изпращане на обратна следа',
'right-mergehistory'         => 'сливане на редакционни истории на страници',
'right-userrights'           => 'редактиране на потребителските права',
'right-userrights-interwiki' => 'редактиране на потребителски права на потребители в други уикита',
'right-siteadmin'            => 'заключване и отключване на базата от данни',

# User rights log
'rightslog'      => 'Дневник на потребителските права',
'rightslogtext'  => 'Това е дневник на промените на потребителски права.',
'rightslogentry' => 'промени потребителската група на $1 от $2 в $3',
'rightsnone'     => '(никакви)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|промяна|промени}}',
'recentchanges'                     => 'Последни промени',
'recentchangestext'                 => "Проследяване на последните промени в {{SITENAME}}.

Легенда: '''тек''' = разлика на текущата версия,
'''ист''' = история на версиите, '''м'''&nbsp;=&nbsp;малка промяна, <strong class='newpage'>Н</strong>&nbsp;=&nbsp;новосъздадена страница",
'recentchanges-feed-description'    => 'Проследяване на последните промени в {{SITENAME}}.',
'rcnote'                            => "{{PLURAL:$1|Показана е '''1''' промяна|Показани са последните '''$1''' промени}} през {{PLURAL:$2|последния ден|последните '''$2''' дни}}, към $5, $4.",
'rcnotefrom'                        => 'Дадени са промените от <strong>$2</strong> (до <strong>$1</strong> показани).',
'rclistfrom'                        => 'Показване на промени, като се започва от $1.',
'rcshowhideminor'                   => '$1 на малки промени',
'rcshowhidebots'                    => '$1 на ботове',
'rcshowhideliu'                     => '$1 на влезли потребители',
'rcshowhideanons'                   => '$1 на анонимни потребители',
'rcshowhidepatr'                    => '$1 на проверени редакции',
'rcshowhidemine'                    => '$1 на моите приноси',
'rclinks'                           => 'Показване на последните $1 промени за последните $2 дни<br />$3',
'diff'                              => 'разл',
'hist'                              => 'ист',
'hide'                              => 'Скриване',
'show'                              => 'Показване',
'minoreditletter'                   => 'м',
'newpageletter'                     => 'Н',
'boteditletter'                     => 'б',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|наблюдаващ потребител|наблюдаващи потребители}}]',
'rc_categories'                     => 'Само от категории (разделител „|“)',
'rc_categories_any'                 => 'Която и да е',
'newsectionsummary'                 => 'Нова тема /* $1 */',

# Recent changes linked
'recentchangeslinked'          => 'Свързани промени',
'recentchangeslinked-title'    => 'Промени, свързани с „$1“',
'recentchangeslinked-noresult' => 'Няма промени в свързаните страници за дадения период.',
'recentchangeslinked-summary'  => "Тази специална страница показва последните промени в свързаните страници. Страниците от списъка ви за наблюдение се показват в '''получер'''.",
'recentchangeslinked-page'     => 'Име на страницата:',

# Upload
'upload'                      => 'Качване',
'uploadbtn'                   => 'Качване',
'reupload'                    => 'Повторно качване',
'reuploaddesc'                => 'Връщане към формуляра за качване.',
'uploadnologin'               => 'Не сте влезли',
'uploadnologintext'           => 'Необходимо е да [[Special:UserLogin|влезете]], за да може да качвате файлове.',
'upload_directory_missing'    => 'Директорията за качване ($1) липсва и не може да бъде създадена на сървъра.',
'upload_directory_read_only'  => 'Сървърът няма достъп за писане в директорията за качване „$1“.',
'uploaderror'                 => 'Грешка при качване',
'uploadtext'                  => "Формулярът по-долу служи за качване на файлове, които ще могат да се използват в страниците.
За преглеждане и търсене на вече качените файлове може да се използва [[Special:ImageList|списъка с качени файлове]]. Качванията и изтриванията се записват в [[Special:Log/upload|дневника на качванията]].

За включване на файл в страница, може да се използва една от следните препратки: '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:картинка.jpg|алтернативен текст]]</nowiki>''' за изображения или '''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:звук.ogg]]</nowiki>''' за звукови файлове.",
'upload-permitted'            => 'Разрешени файлови формати: $1.',
'upload-preferred'            => 'Предпочитани файлови формати: $1.',
'upload-prohibited'           => 'Непозволени файлови формати: $1.',
'uploadlog'                   => 'дневник на качванията',
'uploadlogpage'               => 'Дневник на качванията',
'uploadlogpagetext'           => 'Списък на последните качвания.',
'filename'                    => 'Име на файл',
'filedesc'                    => 'Описание',
'fileuploadsummary'           => 'Описание:',
'filestatus'                  => 'Авторско право:',
'filesource'                  => 'Изходен код:',
'uploadedfiles'               => 'Качени файлове',
'ignorewarning'               => 'Съхраняване на файла въпреки предупреждението.',
'ignorewarnings'              => 'Пренебрегване на всякакви предупреждения',
'minlength1'                  => 'Имената на файловете трябва да съдържат поне един знак.',
'illegalfilename'             => 'Името на файла „$1“ съдържа знаци, които не са позволени в заглавия на страници. Преименувайте файла и се опитайте да го качите отново.',
'badfilename'                 => 'Файлът беше преименуван на „$1“.',
'filetype-badmime'            => 'Не е разрешено качването на файлове с MIME-тип „$1“.',
'filetype-unwanted-type'      => "'''„.$1“''' е нежелан файлов формат. {{PLURAL:$3|Преопръчителният файлов формат е|Препоръчителните файлови формати са}} $2.",
'filetype-banned-type'        => "'''„.$1“''' не е позволен файлов формат. {{PLURAL:$3|Позволеният файлов формат е|Позволените файлови формати са}} $2.",
'filetype-missing'            => 'Файлът няма разширение (напр. „.jpg“).',
'large-file'                  => 'Не се препоръчва файловете да се по-големи от $1; този файл е $2.',
'largefileserver'             => 'Файлът е по-голям от допустимия от сървъра размер.',
'emptyfile'                   => 'Каченият от вас файл е празен. Това може да е предизвикано от грешка в името на файла. Уверете се дали наистина желаете да го качите.',
'fileexists'                  => 'Вече съществува файл с това име! Прегледайте <strong><tt>$1</tt></strong>, ако не сте сигурни, че желаете да го промените.',
'filepageexists'              => 'Описателната страница за този файл вече е създадена на <strong><tt>$1</tt></strong>, въпреки че файл с това име в момента не съществува. Въведеното резюме няма да бъде добавено и показано на описателната страница. За да бъде показано, страницата трябва да бъде редактирана ръчно.',
'fileexists-extension'        => 'Съществува файл със сходно име:<br />
Име на качвания файл: <strong><tt>$1</tt></strong><br />
Име на съществуващия файл: <strong><tt>$2</tt></strong><br />
Има разлика единствено в разширенията на файловете, изразяваща се в ползване на малки и главни букви. Проверете дали файловете не са еднакви.',
'fileexists-thumb'            => "<center>'''Съществуваща картинка'''</center>",
'fileexists-thumbnail-yes'    => 'Изглежда, че файлът е картинка с намален размер <i>(миникартинка)</i>. Проверете файла <strong><tt>$1</tt></strong>.<br />
Ако съществуващият файл представлява оригиналната версия на картинката, няма нужда да се качва неин умален вариант.',
'file-thumbnail-no'           => 'Файловото име започва с <strong><tt>$1</tt></strong>. Изглежда, че е картинка с намален размер <i>(миникартинка)</i>.
Ако разполагате с версия в пълна разделителна способност, качете нея. В противен случай сменете името на този файл.',
'fileexists-forbidden'        => 'Вече съществува файл с това име!
Върнете се и качете файла с ново име. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'В споделеното хранилище за файлове вече съществува файл с това име.
Ако все още желаете да качите вашия файл, върнете се и качете файла с ново име. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Този файл се повтаря със {{PLURAL:$1|следния файл|следните файлове}}:',
'successfulupload'            => 'Качването беше успешно',
'uploadwarning'               => 'Предупреждение при качване',
'savefile'                    => 'Съхраняване на файл',
'uploadedimage'               => 'качена „[[$1]]“',
'overwroteimage'              => 'качена е нова версия на „[[$1]]“',
'uploaddisabled'              => 'Качванията са забранени.',
'uploaddisabledtext'          => 'В това уики качването на файлове е забранено.',
'uploadscripted'              => 'Файлът съдържа HTML или скриптов код, който може да бъде погрешно  интерпретиран от браузъра.',
'uploadcorrupt'               => 'Файлът е повреден или е с неправилно разширение. Проверете го и го качете отново.',
'uploadvirus'                 => 'Файлът съдържа вирус! Подробности: $1',
'sourcefilename'              => 'Първоначално име:',
'destfilename'                => 'Целево име:',
'upload-maxfilesize'          => 'Максимален допустим размер на файла: $1',
'watchthisupload'             => 'Наблюдаване на страницата',
'filewasdeleted'              => 'Файл в този име е съществувал преди време, но е бил изтрит. Проверете $1 преди да го качите отново.',
'upload-wasdeleted'           => "'''Внимание: Качвате файл, който вече е бил изтрит.'''

Преценете дали е удачно да продължите с качването на файла. За ваше удобство, ето записа за него в дневника на изтриванията:",
'filename-bad-prefix'         => 'Името на файла, който качвате, започва с <strong>„$1“</strong>, което е неописателно име, типично задавано по автоматичен начин от цифровите камери или апарати. Изберете по-описателно име на файла.',

'upload-proto-error'      => 'Неправилен протокол',
'upload-proto-error-text' => 'Изисква се адрес започващ с <code>http://</code> или <code>ftp://</code>.',
'upload-file-error'       => 'Вътрешна грешка',
'upload-file-error-text'  => 'Вътрешна грешка при опит да се създаде временен файл на сървъра. Обърнете се към [[Special:ListUsers/sysop|администратор]].',
'upload-misc-error'       => 'Неизвестна грешка при качване',
'upload-misc-error-text'  => 'Неизвестна грешка при качване. Убедете се, че адресът е верен и опитайте отново. Ако отново имате проблем, обърнете се към [[Special:ListUsers/sysop|администратор]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Не е възможно достигането на указания URL адрес',
'upload-curl-error6-text'  => 'Търсеният адрес не може да бъде достигнат. Проверете дали е написан вярно.',
'upload-curl-error28'      => 'Времето за качване изтече',
'upload-curl-error28-text' => 'Сайтът не отговаря твърде дълго. Убедете се, че сайтът работи, изчакайте малко и опитайте отново. В краен случай опитайте през по-ненатоварено време.',

'license'            => 'Лицензиране:',
'nolicense'          => 'Нищо не е избрано',
'license-nopreview'  => '(Не е наличен предварителен преглед)',
'upload_source_url'  => ' (правилен, публично достъпен интернет-адрес)',
'upload_source_file' => ' (файл на вашия компютър)',

# Special:ImageList
'imagelist-summary'     => 'Тази специална страница показва всички качени файлове.
По подразбиране последно качените файлове се показват най-високо в списъка.
Сортирането се променя с щракване в заглавна клетка на колоната.',
'imagelist_search_for'  => 'Търсене по име на файла:',
'imgfile'               => 'файл',
'imagelist'             => 'Списък на файловете',
'imagelist_date'        => 'Дата',
'imagelist_name'        => 'Име на файла',
'imagelist_user'        => 'Потребител',
'imagelist_size'        => 'Размер',
'imagelist_description' => 'Описание',

# Image description page
'filehist'                       => 'История на файла',
'filehist-help'                  => 'Избирането на дата/час ще покаже как е изглеждал файлът към онзи момент.',
'filehist-deleteall'             => 'изтриване на всички',
'filehist-deleteone'             => 'изтриване',
'filehist-revert'                => 'връщане',
'filehist-current'               => 'текуща',
'filehist-datetime'              => 'Дата/Час',
'filehist-user'                  => 'Потребител',
'filehist-dimensions'            => 'Размер',
'filehist-filesize'              => 'Размер на файла',
'filehist-comment'               => 'Коментар',
'imagelinks'                     => 'Препратки към файла',
'linkstoimage'                   => '{{PLURAL:$1|Следната страница сочи|Следните $1 страници сочат}} към файла:',
'nolinkstoimage'                 => 'Няма страници, сочещи към файла.',
'morelinkstoimage'               => 'Можете да видите [[Special:WhatLinksHere/$1|още препратки]] към този файл.',
'redirectstofile'                => '{{PLURAL:$1|Следният файл пренасочва|Следните $1 файла пренасочват}} към този файл:',
'duplicatesoffile'               => '{{PLURAL:$1|Следният файл се повтаря|Следните $1 файла се повтарят}} с този файл:',
'sharedupload'                   => 'Този файл е споделен и може да бъде използван от други проекти.',
'shareduploadwiki'               => 'Разгледайте $1 за повече информация.',
'shareduploadwiki-desc'          => 'Следва описанието от $1 от споделеното хранилище.',
'shareduploadwiki-linktext'      => 'описателната страница на файла',
'shareduploadduplicate'          => 'Този файл се повтаря с $1 от споделеното мултимедийно хранилище.',
'shareduploadduplicate-linktext' => 'друг файл',
'shareduploadconflict'           => 'Този файл има същото име като $1 от споделеното мултимедийно хранилище.',
'shareduploadconflict-linktext'  => 'друг файл',
'noimage'                        => 'Не съществува файл с това име, можете $1.',
'noimage-linktext'               => 'да го качите',
'uploadnewversion-linktext'      => 'Качване на нова версия на файла',
'imagepage-searchdupe'           => 'Търсене на повтарящи се файлове',

# File reversion
'filerevert'                => 'Възвръщане на $1',
'filerevert-legend'         => 'Възвръщане на файла',
'filerevert-intro'          => "Възвръщане на '''[[Media:$1|$1]]''' към [$4 версията от $3, $2].",
'filerevert-comment'        => 'Коментар:',
'filerevert-defaultcomment' => 'Възвръщане към версия от $2, $1',
'filerevert-submit'         => 'Възвръщане',
'filerevert-success'        => "Файлът '''[[Media:$1|$1]]''' беше възвърнат към [$4 версия от $3, $2].",
'filerevert-badversion'     => 'Не съществува предишна локална версия на файла със зададения времеви отпечатък.',

# File deletion
'filedelete'                  => 'Изтриване на $1',
'filedelete-legend'           => 'Изтриване на файл',
'filedelete-intro'            => "На път сте да изтриете '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Изтривате версията на '''[[Media:$1|$1]]''' към [$4 $3, $2].",
'filedelete-comment'          => 'Коментар:',
'filedelete-submit'           => 'Изтриване',
'filedelete-success'          => "Файлът '''$1''' беше изтрит.",
'filedelete-success-old'      => "Версията на '''[[Media:$1|$1]]''' към $3, $2 е била изтрита.",
'filedelete-nofile'           => "Файлът '''$1''' не съществува в {{SITENAME}}.",
'filedelete-nofile-old'       => "Не съществува архивна версия на '''$1''' с указаните параметри.",
'filedelete-iscurrent'        => 'Опитвате се да изтриете последната версия на този файл. Първо направете възвръщане към по-стара версия.',
'filedelete-otherreason'      => 'Друга/допълнителна причина:',
'filedelete-reason-otherlist' => 'Друга причина',
'filedelete-reason-dropdown'  => '*Общи причини за изтриване
** Нарушение на авторските права
** Файлът се повтаря',
'filedelete-edit-reasonlist'  => 'Редактиране на причините за изтриване',

# MIME search
'mimesearch'         => 'MIME-търсене',
'mimesearch-summary' => 'На тази страница можете да филтрирате файловете по техния MIME-тип. Заявката трябва да се състои от медиен тип и подтип, разделени с наклонена черта (слеш), напр. <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-тип:',
'download'           => 'сваляне',

# Unwatched pages
'unwatchedpages' => 'Ненаблюдавани страници',

# List redirects
'listredirects' => 'Списък на пренасочванията',

# Unused templates
'unusedtemplates'     => 'Неизползвани шаблони',
'unusedtemplatestext' => 'Тази страница съдържа списък на шаблоните, които не са включени в друга страница. Проверявайте за препратки към отделните шаблони преди да ги изтриете или предложите за изтриване.',
'unusedtemplateswlh'  => 'други препратки',

# Random page
'randompage'         => 'Случайна страница',
'randompage-nopages' => 'В това именно пространство няма страници.',

# Random redirect
'randomredirect'         => 'Случайно пренасочване',
'randomredirect-nopages' => 'В това именно пространство няма пренасочвания.',

# Statistics
'statistics'             => 'Статистика',
'sitestats'              => 'Статистика на {{SITENAME}}',
'userstats'              => 'Потребители',
'sitestatstext'          => "Базата от данни съдържа {{PLURAL:$1|'''една''' страница|'''$1''' страници}}.
Това включва всички страници от всички именни пространства в {{SITENAME}} (''Основно'', Беседа, Потребител, Категория и т.н.). Измежду тях {{PLURAL:$2|'''една''' страница се смята за действителна|'''$2''' страници се смятат за действителни}} (броят се само страниците от основното именно пространство, като се изключват пренасочвания и страници, несъдържащи препратки).

{{PLURAL:$8|Бил е качен '''един''' файл|Били са качени '''$8''' файла}}.

Имало е {{PLURAL:$3|'''един''' преглед на страница|'''$3''' прегледа на страници}} и {{PLURAL:$4|'''една''' редакция|'''$4''' редакции}} от пускането на {{SITENAME}}.
Това прави средно по '''$5''' редакции на страница и по '''$6''' прегледа на редакция.

Дължината на [http://www.mediawiki.org/wiki/Manual:Job_queue работната опашка] е '''$7'''.",
'userstatstext'          => "Има {{PLURAL:$1|'''1''' [[Special:ListUsers|регистриран потребител]]|'''$1''' [[Special:ListUsers|регистрирани потребители]]}} и '''$2''' {{PLURAL:$2|потребител|потребители}} (или '''$4%''') с права на $5.",
'statistics-mostpopular' => 'Най-преглеждани страници',

'disambiguations'      => 'Пояснителни страници',
'disambiguationspage'  => 'Template:Пояснение',
'disambiguations-text' => "Следните страници сочат към '''пояснителна страница''', вместо към истинската тематична страница.<br />Една страница се смята за пояснителна, ако ползва шаблон, към който се препраща от [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Двойни пренасочвания',
'doubleredirectstext'        => 'Всеки ред съдържа препратки към първото и второто пренасочване, както и целта на второто пренасочване, която обикновено е „истинската“ страница, към която първото пренасочване би трябвало да сочи.',
'double-redirect-fixed-move' => 'Оправяне на двойно пренасочване след преместването на [[$1]] като [[$2]]',
'double-redirect-fixer'      => 'Redirect fixer',

'brokenredirects'        => 'Невалидни пренасочвания',
'brokenredirectstext'    => 'Следните пренасочващи страници сочат към несъществуващи страници:',
'brokenredirects-edit'   => '(редактиране)',
'brokenredirects-delete' => '(изтриване)',

'withoutinterwiki'         => 'Страници без междуезикови препратки',
'withoutinterwiki-summary' => 'Следните страници не препращат към версии на други езици:',
'withoutinterwiki-legend'  => 'Представка',
'withoutinterwiki-submit'  => 'Показване',

'fewestrevisions' => 'Страници с най-малко версии',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|байт|байта}}',
'ncategories'             => '$1 {{PLURAL:$1|категория|категории}}',
'nlinks'                  => '$1 {{PLURAL:$1|препратка|препратки}}',
'nmembers'                => '$1 {{PLURAL:$1|член|члена}}',
'nrevisions'              => '$1 {{PLURAL:$1|версия|версии}}',
'nviews'                  => '$1 {{PLURAL:$1|преглед|прегледа}}',
'specialpage-empty'       => 'Страницата е празна.',
'lonelypages'             => 'Страници сираци',
'lonelypagestext'         => 'Към следващите страници няма препратки от други страници на тази енциклопедия.',
'uncategorizedpages'      => 'Некатегоризирани страници',
'uncategorizedcategories' => 'Некатегоризирани категории',
'uncategorizedimages'     => 'Некатегоризирани картинки',
'uncategorizedtemplates'  => 'Некатегоризирани шаблони',
'unusedcategories'        => 'Неизползвани категории',
'unusedimages'            => 'Неизползвани файлове',
'popularpages'            => 'Най-посещавани страници',
'wantedcategories'        => 'Желани категории',
'wantedpages'             => 'Желани страници',
'missingfiles'            => 'Липсващи файлове',
'mostlinked'              => 'Най-препращани страници',
'mostlinkedcategories'    => 'Най-препращани категории',
'mostlinkedtemplates'     => 'Най-препращани шаблони',
'mostcategories'          => 'Страници с най-много категории',
'mostimages'              => 'Най-препращани картинки',
'mostrevisions'           => 'Страници с най-много версии',
'prefixindex'             => 'Всички страници (с представка)',
'shortpages'              => 'Кратки страници',
'longpages'               => 'Дълги страници',
'deadendpages'            => 'Задънени страници',
'deadendpagestext'        => 'Следните страници нямат препратки към други страници от {{SITENAME}}.',
'protectedpages'          => 'Защитени страници',
'protectedpages-indef'    => 'Само безсрочни защити',
'protectedpagestext'      => 'Следните страници са защитени против редактиране или преместване',
'protectedpagesempty'     => 'В момента няма защитени страници с тези параметри.',
'protectedtitles'         => 'Защитени заглавия',
'protectedtitlestext'     => 'Следните заглавия са защитени срещу създаване',
'protectedtitlesempty'    => 'В момента няма заглавия, защитени с тези параметри.',
'listusers'               => 'Списък на потребителите',
'newpages'                => 'Нови страници',
'newpages-username'       => 'Потребител:',
'ancientpages'            => 'Стари страници',
'move'                    => 'Преместване',
'movethispage'            => 'Преместване на страницата',
'unusedimagestext'        => 'Обърнете внимание, че други сайтове могат да сочат към файла чрез пряк адрес и въпреки това тя може да се намира в списъка.',
'unusedcategoriestext'    => 'Следните категории съществуват, но никоя страница или категория не ги използва.',
'notargettitle'           => 'Няма цел',
'notargettext'            => 'Не указахте целева страница или потребител, върху която/който да се изпълни действието.',
'nopagetitle'             => 'Няма такава целева страница',
'nopagetext'              => 'Посочената целева страница не съществува.',
'pager-newer-n'           => '{{PLURAL:$1|по-нова 1|по-нови $1}}',
'pager-older-n'           => '{{PLURAL:$1|по-стара 1|по-стари $1}}',
'suppress'                => 'Премахване от публичния архив',

# Book sources
'booksources'               => 'Източници на книги',
'booksources-search-legend' => 'Търсене на информация за книга',
'booksources-go'            => 'Търсене',
'booksources-text'          => 'По-долу е списъкът от връзки към други сайтове, продаващи нови и използвани книги или имащи повече информация за книгите, които търсите:',

# Special:Log
'specialloguserlabel'  => 'Потребител:',
'speciallogtitlelabel' => 'Заглавие:',
'log'                  => 'Дневници',
'all-logs-page'        => 'Всички дневници',
'log-search-legend'    => 'Претърсване на дневниците',
'log-search-submit'    => 'Отиване',
'alllogstext'          => 'Смесено показване на записи от всички налични дневници в {{SITENAME}}.
Можете да ограничите прегледа, като изберете вид на дневника, потребителско име или определена страница.',
'logempty'             => 'Дневникът не съдържа записи, отговарящи на избрания критерий.',
'log-title-wildcard'   => 'Търсене на заглавия, започващи със',

# Special:AllPages
'allpages'          => 'Всички страници',
'alphaindexline'    => 'от $1 до $2',
'nextpage'          => 'Следваща страница ($1)',
'prevpage'          => 'Предходна страница ($1)',
'allpagesfrom'      => 'Показване на страниците, като се започва от:',
'allarticles'       => 'Всички страници',
'allinnamespace'    => 'Всички страници (именно пространство $1)',
'allnotinnamespace' => 'Всички страници (без именно пространство $1)',
'allpagesprev'      => 'Предишна',
'allpagesnext'      => 'Следваща',
'allpagessubmit'    => 'Отиване',
'allpagesprefix'    => 'Показване на страници, започващи със:',
'allpagesbadtitle'  => 'Зададеното име е невалидно. Възможно е да съдържа междуезикова или междупроектна представка или пък знаци, които не могат да се използват в заглавия.',
'allpages-bad-ns'   => 'В {{SITENAME}} не съществува именно пространство „$1“.',

# Special:Categories
'categories'                    => 'Категории',
'categoriespagetext'            => 'Следните категории съдържат страници или медийни файлове.
[[Special:UnusedCategories|Неизползваните категории]] не са показани тук.
Вижте също списъка с [[Special:WantedCategories|желани категории]].',
'categoriesfrom'                => 'Показване на категориите, като се започне от:',
'special-categories-sort-count' => 'сортиране по брой',
'special-categories-sort-abc'   => 'сортиране по азбучен ред',

# Special:ListUsers
'listusersfrom'      => 'Показване на потребителите, започвайки от:',
'listusers-submit'   => 'Показване',
'listusers-noresult' => 'Няма намерени потребители.',

# Special:ListGroupRights
'listgrouprights'          => 'Права по потребителски групи',
'listgrouprights-summary'  => 'По-долу на тази страница е показан списък на групите потребители в това уики с асоциираните им права за достъп. Допълнителна информация за отделните права може да бъде намерена [[{{MediaWiki:Listgrouprights-helppage}}|тук]].',
'listgrouprights-group'    => 'Група',
'listgrouprights-rights'   => 'Права',
'listgrouprights-helppage' => 'Help:Права на групите',
'listgrouprights-members'  => '(списък на членовете)',

# E-mail user
'mailnologin'     => 'Няма електронна поща',
'mailnologintext' => 'Необходимо е да [[Special:UserLogin|влезете]] и да посочите валидна електронна поща в [[Special:Preferences|настройките]] си, за да може да пращате писма на други потребители.',
'emailuser'       => 'Писмо до потребителя',
'emailpage'       => 'Пращане писмо на потребител',
'emailpagetext'   => 'Ако този потребител е посочил валидна електронна поща в настройките си, чрез долния формуляр можете да му изпратите съобщение. Адресът, записан в [[Special:Preferences|настройките ви]], ще се появи в полето „От“ на изпратеното писмо, така че получателят ще е в състояние да ви отговори.',
'usermailererror' => 'Пощенският обект даде грешка:',
'defemailsubject' => 'Писмо от {{SITENAME}}',
'noemailtitle'    => 'Няма електронна поща',
'noemailtext'     => 'Потребителят не е посочил валидна електронна поща или е избрал да не получава писма от други потребители.',
'emailfrom'       => 'От:',
'emailto'         => 'До:',
'emailsubject'    => 'Относно:',
'emailmessage'    => 'Съобщение:',
'emailsend'       => 'Изпращане',
'emailccme'       => 'Изпращане на копие на писмото до автора.',
'emailccsubject'  => 'Копие на писмото ви до $1: $2',
'emailsent'       => 'Писмото е изпратено',
'emailsenttext'   => 'Писмото ви беше изпратено.',
'emailuserfooter' => 'Това писмо беше изпратено от $1 на $2 чрез функцията „Изпращане на писмо до потребителя“ на {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Моят списък за наблюдение',
'mywatchlist'          => 'Моят списък за наблюдение',
'watchlistfor'         => "(за '''$1''')",
'nowatchlist'          => 'Списъкът ви за наблюдение е празен.',
'watchlistanontext'    => 'За преглеждане и редактиране на списъка за наблюдение се изисква $1 в системата.',
'watchnologin'         => 'Не сте влезли',
'watchnologintext'     => 'Необходимо е да [[Special:UserLogin|влезете]], за да може да променяте списъка си за наблюдение.',
'addedwatch'           => 'Добавено в списъка за наблюдение',
'addedwatchtext'       => "Страницата „'''[[:$1]]'''“ беше добавена към [[Special:Watchlist|списъка ви за наблюдение]].
Нейните бъдещи промени, както и на съответната й дискусионна страница, ще се описват там, а тя ще се появява в '''получер''' в [[Special:RecentChanges|списъка на последните промени]], което ще направи по-лесно избирането й.",
'removedwatch'         => 'Премахнато от списъка за наблюдение',
'removedwatchtext'     => 'Страницата „<nowiki>$1</nowiki>“ беше премахната от списъка ви за наблюдение.',
'watch'                => 'Наблюдение',
'watchthispage'        => 'Наблюдаване на страницата',
'unwatch'              => 'Спиране на наблюдение',
'unwatchthispage'      => 'Спиране на наблюдение',
'notanarticle'         => 'Не е страница',
'notvisiblerev'        => 'Версията беше изтрита',
'watchnochange'        => 'Никоя от наблюдаваните страници не е била редактирана в показаното време.',
'watchlist-details'    => '{{PLURAL:$1|Една наблюдавана страница|$1 наблюдавани страници}} от списъка ви за наблюдение (без беседи).',
'wlheader-enotif'      => '* Известяването по електронна поща е включено.',
'wlheader-showupdated' => "* Страниците, които са били променени след последния път, когато сте ги посетили, са показани с '''получер''' шрифт.",
'watchmethod-recent'   => 'проверка на последните редакции за наблюдавани страници',
'watchmethod-list'     => 'проверка на наблюдаваните страници за скорошни редакции',
'watchlistcontains'    => 'Списъкът ви за наблюдение съдържа {{PLURAL:$1|една страница|$1 страници}}.',
'iteminvalidname'      => 'Проблем с „$1“, грешно име…',
'wlnote'               => "{{PLURAL:$1|Показана е последната промяна|Показани са последните '''$1''' промени}} през {{PLURAL:$2|последния час|последните '''$2''' часа}}.",
'wlshowlast'           => 'Показване на последните $1 часа $2 дни $3',
'watchlist-show-bots'  => 'Показване на ботове',
'watchlist-hide-bots'  => 'Скриване на ботове',
'watchlist-show-own'   => 'Показване на моите приноси',
'watchlist-hide-own'   => 'Скриване на моите приноси',
'watchlist-show-minor' => 'Показване на малки промени',
'watchlist-hide-minor' => 'Скриване на малки промени',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Наблюдение…',
'unwatching' => 'Спиране на наблюдение…',

'enotif_mailer'                => 'Известяване по пощата на {{SITENAME}}',
'enotif_reset'                 => 'Отбелязване на всички страници като посетени',
'enotif_newpagetext'           => 'Това е нова страница.',
'enotif_impersonal_salutation' => 'Потребител на {{SITENAME}}',
'changed'                      => 'променена',
'created'                      => 'създадена',
'enotif_subject'               => 'Страницата $PAGETITLE в {{SITENAME}} е била $CHANGEDORCREATED от $PAGEEDITOR',
'enotif_lastvisited'           => 'Преглед на всички промени след последното ви посещение: $1.',
'enotif_lastdiff'              => 'Преглед на тази промяна: $1.',
'enotif_anon_editor'           => 'анонимен потребител $1',
'enotif_body'                  => 'Уважаеми(а) $WATCHINGUSERNAME,

на $PAGEEDITDATE страницата „$PAGETITLE“ в {{SITENAME}} е била $CHANGEDORCREATED от $PAGEEDITOR.

Текуща версия: $PAGETITLE_URL

$NEWPAGE

Кратко описание на измененията: $PAGESUMMARY $PAGEMINOREDIT

Връзка с редактора:
* електронна поща: $PAGEEDITOR_EMAIL
* уики: $PAGEEDITOR_WIKI

Няма да ви се пращат други известявания за последващи изменения, докато не посетите страницата. На страницата със списъка ви за наблюдение можете да включите известяванията наведнъж за всички страници.

             Системата за известяване на {{SITENAME}}

--
За да промените настройките на списъка си за наблюдение, посетете {{fullurl:{{ns:special}}:Watchlist/edit}}

Обратна връзка и помощ: {{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Изтриване',
'confirm'                     => 'Потвърждаване',
'excontent'                   => 'съдържанието беше: „$1“',
'excontentauthor'             => 'съдържанието беше: „$1“ (като единственият автор беше [[Special:Contributions/$2|$2]])',
'exbeforeblank'               => 'премахнато преди това съдържание: „$1“',
'exblank'                     => 'страницата беше празна',
'delete-confirm'              => 'Изтриване на „$1“',
'delete-legend'               => 'Изтриване',
'historywarning'              => 'Внимание: Страницата, която ще изтриете, има история:',
'confirmdeletetext'           => 'На път сте безвъзвратно да изтриете страница или файл, заедно с цялата прилежаща редакционна история, от базата от данни.
Потвърдете, че искате това, разбирате последствията и правите това в съответствие с [[{{MediaWiki:Policy-url}}|линията на поведение]].',
'actioncomplete'              => 'Действието беше изпълнено',
'deletedtext'                 => 'Страницата „<nowiki>$1</nowiki>“ беше изтрита. Вижте $2 за запис на последните изтривания.',
'deletedarticle'              => 'изтри „[[$1]]“',
'suppressedarticle'           => 'премахна "[[$1]]"',
'dellogpage'                  => 'Дневник на изтриванията',
'dellogpagetext'              => 'Списък на последните изтривания.',
'deletionlog'                 => 'дневник на изтриванията',
'reverted'                    => 'Възвръщане към предишна версия',
'deletecomment'               => 'Причина за изтриването',
'deleteotherreason'           => 'Друга/допълнителна причина:',
'deletereasonotherlist'       => 'Друга причина',
'deletereason-dropdown'       => '*Стандартни причини за изтриване
** По молба на автора
** Нарушение на авторски права
** Вандализъм',
'delete-edit-reasonlist'      => 'Редактиране на причините за изтриване',
'delete-toobig'               => 'Тази страница има голяма редакционна история с над $1 {{PLURAL:$1|версия|версии}}. Изтриването на такива страници е ограничено, за да се предотвратят евентуални поражения на {{SITENAME}}.',
'delete-warning-toobig'       => 'Тази страница има голяма редакционна история с над $1 {{PLURAL:$1|версия|версии}}. Възможно е изтриването да наруши някои операции в базата данни на {{SITENAME}}; необходимо е особено внимание при продължаване на действието.',
'rollback'                    => 'Отмяна на промените',
'rollback_short'              => 'Отмяна',
'rollbacklink'                => 'отмяна',
'rollbackfailed'              => 'Отмяната не сполучи',
'cantrollback'                => 'Не може да се извърши отмяна на редакциите. Последният редактор е и единствен автор на страницата.',
'alreadyrolled'               => 'Редакцията на [[:$1]], направена от [[User:$2|$2]] ([[User talk:$2|Беседа]] | [[Special:Contributions/$2|{{int:contribslink}}]]), не може да бъде отменена. Някой друг вече е редактирал страницата или е отменил промените.

Последната редакция е на [[User:$3|$3]] ([[User talk:$3|Беседа]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => "Коментарът на редакцията е бил: „''$1''“.", # only shown if there is an edit comment
'revertpage'                  => 'Премахване на [[Special:Contributions/$2|редакции на $2]] ([[User talk:$2|беседа]]); възвръщане към последната версия на [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Отменени редакции на $1; възвръщане към последната версия на $2.',
'sessionfailure'              => 'Изглежда има проблем със сесията ви; действието беше отказано като предпазна мярка срещу крадене на сесията. Натиснете бутона за връщане на браузъра, презаредете страницата, от която сте дошли, и опитайте отново.',
'protectlogpage'              => 'Дневник на защитата',
'protectlogtext'              => 'Списък на защитите и техните сваляния за страницата.
Можете да прегледате и [[Special:ProtectedPages|списъка на текущо защитените страници]].',
'protectedarticle'            => 'защитаване на „[[$1]]“',
'modifiedarticleprotection'   => 'смяна на нивото на защита на „[[$1]]“',
'unprotectedarticle'          => 'сваляне на защитата на „[[$1]]“',
'protect-title'               => 'Защитаване на „$1“',
'protect-legend'              => 'Потвърждение на защитата',
'protectcomment'              => 'Коментар:',
'protectexpiry'               => 'Изтичане:',
'protect_expiry_invalid'      => 'Времето на изтичане е невалидно.',
'protect_expiry_old'          => 'Времето на изтичане лежи в миналото.',
'protect-unchain'             => 'Позволяване на преместванията',
'protect-text'                => 'Тук можете да прегледате и промените нивото на защита на страницата „[[$1]]“. Желателно е да се придържате към [[{{ns:project}}:Защитена страница|ръководните принципи на проекта]].',
'protect-locked-blocked'      => 'Нямате правото да променяте нивата на защита на страниците, докато сте блокиран(а). Ето текущите настройки за страницата „<strong>$1</strong>“:',
'protect-locked-dblock'       => 'Нивата на защита на страниците не могат да бъдат променяни, защото базата от данни е заключена. Ето текущите настройки за страницата „<strong>$1</strong>“:',
'protect-locked-access'       => 'Нямате правото да променяте нивата на защита на страниците. Ето текущите настройки за страницата „<strong>$1</strong>“:',
'protect-cascadeon'           => 'Тази страница е защитена против редактиране, защото е включена в {{PLURAL:$1|следната страница, която от своя страна има|следните страници, които от своя страна имат}} каскадна защита. Можете да промените нивото на защита на страницата, но това няма да повлияе върху каскадната защита.',
'protect-default'             => '(по подразбиране)',
'protect-fallback'            => 'Необходими са права на „$1“',
'protect-level-autoconfirmed' => 'Блокиране на нерегистрирани потребители',
'protect-level-sysop'         => 'Само за администратори',
'protect-summary-cascade'     => 'каскадно',
'protect-expiring'            => 'изтича на $1 (UTC)',
'protect-cascade'             => 'Каскадна защита — защита на всички страници, включени в настоящата страница.',
'protect-cantedit'            => 'Не можете да промените нивото на защита на тази страницата, защото нямате права да я редактирате.',
'restriction-type'            => 'Състояние на защитата:',
'restriction-level'           => 'Ниво на защитата:',
'minimum-size'                => 'Минимален размер',
'maximum-size'                => 'Максимален размер',
'pagesize'                    => '(байта)',

# Restrictions (nouns)
'restriction-edit'   => 'Редактиране',
'restriction-move'   => 'Преместване',
'restriction-create' => 'Създаване',
'restriction-upload' => 'Качване',

# Restriction levels
'restriction-level-sysop'         => 'пълна защита',
'restriction-level-autoconfirmed' => 'полузащита',
'restriction-level-all'           => 'всички',

# Undelete
'undelete'                     => 'Преглед на изтрити страници',
'undeletepage'                 => 'Преглед и възстановяване на изтрити страници',
'undeletepagetitle'            => "'''По-долу е показан списък на изтритите версии на [[:$1|$1]]'''.",
'viewdeletedpage'              => 'Преглед на изтрити страници',
'undeletepagetext'             => 'Следните страници бяха изтрити, но се намират все още
в архива и могат да бъдат възстановени. Архивът може да се почиства от време на време.',
'undelete-fieldset-title'      => 'Възстановяване на версии',
'undeleteextrahelp'            => "За пълно възстановяване на историята на страницата, не слагайте отметки и натиснете '''''Възстановяване'''''.
За частично възстановяване отметнете тези версии на страницата, които трябва да бъдат въстановени, и натиснете '''''Възстановяване'''''.
Натискането на '''''Изчистване''''' ще премахне всички отметки и ще изчисти полето за коментар.",
'undeleterevisions'            => '{{PLURAL:$1|Една версия беше архивирана|$1 версии бяха архивирани}}',
'undeletehistory'              => 'Ако възстановите страницата, всички версии ще бъдат върнати в историята.
Ако след изтриването е създадена страница със същото име, възстановените версии ще се появят като по-ранна история, а текущата версия на страницата няма да бъде заменена автоматично. Също така обърнете внимание, че ограниченията, приложени върху версиите, ще се загубят след възстановяването.',
'undeleterevdel'               => 'Възстановяването няма да бъде изпълнено, ако би довело до частично изтриване на актуалната версия. В такъв случай актуалната версия не трябва да бъде избирана или пък състоянието й трябва да бъде променено на нормална (нескрита) версия. Версиите на файлове, които нямате право да преглеждате, няма да бъдат възстановени.',
'undeletehistorynoadmin'       => 'Тази страница е била изтрита. В резюмето отдолу е посочена причината за това, заедно с информация за потребителите, редактирали страницата преди изтриването й. Конкретното съдържание на изтритите версии е достъпно само за администратори.',
'undelete-revision'            => 'Изтрита версия на $1 (към $2) от $3:',
'undeleterevision-missing'     => 'Неправилна или липсваща версия. Може да сте последвали грешна препратка или указаната версия да е била възстановена или премахната от архива',
'undelete-nodiff'              => 'Не е открита предишна редакция.',
'undeletebtn'                  => 'Възстановяване',
'undeletelink'                 => 'възстановяване',
'undeletereset'                => 'Изчистване',
'undeletecomment'              => 'Коментар:',
'undeletedarticle'             => '„[[$1]]“ беше възстановена',
'undeletedrevisions'           => '{{PLURAL:$1|Една версия беше възстановена|$1 версии бяха възстановени}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|Една версия|$1 версии}} и {{PLURAL:$1|един файл|$2 файла}} бяха възстановени',
'undeletedfiles'               => '{{PLURAL:$1|Един файл беше възстановен|$1 файла бяха възстановени}}',
'cannotundelete'               => 'Грешка при възстановяването. Възможно е някой друг вече да е възстановил страницата.',
'undeletedpage'                => "<big>'''Страницата „$1“ беше възстановена.'''</big>

Можете да видите последните изтрити и възстановени страници в [[Special:Log/delete|дневника на изтриванията]].",
'undelete-header'              => 'Прегледайте [[Special:Log/delete|дневника на изтриванията]] за текущо изтритите страници.',
'undelete-search-box'          => 'Търсене на изтрити страници',
'undelete-search-prefix'       => 'Показване на страници, започващи със:',
'undelete-search-submit'       => 'Търсене',
'undelete-no-results'          => 'Не са намерени страници, отговарящи на търсения критерий.',
'undelete-filename-mismatch'   => 'Не е възможно възстановяването на файловата версия с времеви отпечатък $1: несъответствие в името на файла',
'undelete-bad-store-key'       => 'Не е възможно възстановяването на файловата версия с времеви отпечатък $1: файлът е липсвал преди изтриването.',
'undelete-cleanup-error'       => 'Грешка при изтриване на неизползвания архивен файл „$1“.',
'undelete-missing-filearchive' => 'Не е възможно възстановяването на файла с ID $1, защото не присъства в базата от данни. Вероятно вече е възстановен.',
'undelete-error-short'         => 'Грешка при възстановяването на изтрития файл: $1',
'undelete-error-long'          => 'Възникнаха грешки при възстановяването на изтрития файл:

$1',

# Namespace form on various pages
'namespace'      => 'Именно пространство:',
'invert'         => 'Обръщане на избора',
'blanknamespace' => '(Основно)',

# Contributions
'contributions' => 'Приноси',
'mycontris'     => 'Моите приноси',
'contribsub2'   => 'За $1 ($2)',
'nocontribs'    => 'Не са намерени промени, отговарящи на критерия.',
'uctop'         => ' (последна)',
'month'         => 'Месец:',
'year'          => 'Година:',

'sp-contributions-newbies'     => 'Показване само на приносите на нови потребители',
'sp-contributions-newbies-sub' => 'на нови потребители',
'sp-contributions-blocklog'    => 'Дневник на блокиранията',
'sp-contributions-search'      => 'Търсене на приноси',
'sp-contributions-username'    => 'IP-адрес или потребителско име:',
'sp-contributions-submit'      => 'Търсене',

# What links here
'whatlinkshere'            => 'Какво сочи насам',
'whatlinkshere-title'      => 'Страници, които сочат към „$1“',
'whatlinkshere-page'       => 'Страница:',
'linklistsub'              => '(Списък с препратки)',
'linkshere'                => "Следните страници сочат към '''[[:$1]]''':",
'nolinkshere'              => "Няма страници, сочещи към '''[[:$1]]'''.",
'nolinkshere-ns'           => "Няма страници, сочещи към '''[[:$1]]''' в избраното именно пространство.",
'isredirect'               => 'пренасочваща страница',
'istemplate'               => 'включване',
'isimage'                  => 'препратка към файла',
'whatlinkshere-prev'       => '{{PLURAL:$1|предишна|предишни $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|следваща|следващи $1}}',
'whatlinkshere-links'      => '← препратки',
'whatlinkshere-hideredirs' => '$1 на пренасочващи страници',
'whatlinkshere-hidetrans'  => '$1 на включени страници',
'whatlinkshere-hidelinks'  => '$1 на препратки',
'whatlinkshere-hideimages' => '$1 препратки към файла',
'whatlinkshere-filters'    => 'Филтри',

# Block/unblock
'blockip'                         => 'Блокиране',
'blockip-legend'                  => 'Блокиране на потребител',
'blockiptext'                     => 'Формулярът по-долу се използва за да се забрани правото на писане
на определен IP-адрес или потребител.
Това трябва да се направи само за да се предотвратят прояви на вандализъм
и в съответствие с [[{{MediaWiki:Policy-url}}|политиката за поведение]] в {{SITENAME}}.
Необходимо е да се посочи и причина за блокирането (например заглавия на страници, станали обект на вандализъм).',
'ipaddress'                       => 'IP-адрес:',
'ipadressorusername'              => 'IP-адрес или потребител:',
'ipbexpiry'                       => 'Срок:',
'ipbreason'                       => 'Причина:',
'ipbreasonotherlist'              => 'Друга причина',
'ipbreason-dropdown'              => '* Общи причини за блокиране
** Въвеждане на невярна информация
** Премахване на съдържание от страниците
** Добавяне на спам/нежелани външни препратки
** Въвеждане на безсмислици в страниците
** Заплашително поведение/тормоз
** Злупотреба с няколко потребителски сметки
** Неприемливо потребителско име',
'ipbanononly'                     => 'Блокиране само на анонимни потребители',
'ipbcreateaccount'                => 'Забрана за създаване на потребителски сметки',
'ipbemailban'                     => 'Забрана на потребителя да праща е-поща',
'ipbenableautoblock'              => 'Автоматично блокиране на последния IP-адрес, използван от потребителя, както и на всички останали адреси, от които се опита да редактира',
'ipbsubmit'                       => 'Блокиране на потребителя',
'ipbother'                        => 'Друг срок:',
'ipboptions'                      => 'два часа:2 hours,един ден:1 day,три дни:3 days,една седмица:1 week,две седмици:2 weeks,един месец:1 month,три месеца:3 months,шест месеца:6 months,една година:1 year,безсрочно:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'друг',
'ipbotherreason'                  => 'Друга/допълнителна причина:',
'ipbhidename'                     => 'Скриване на потребителското име/IP-адрес в дневника на блокиранията, в списъка с текущите блокирания и в списъка на потребителите',
'ipbwatchuser'                    => 'Наблюдаване на потребителската страница и беседата на този потребител',
'badipaddress'                    => 'Невалиден IP-адрес',
'blockipsuccesssub'               => 'Блокирането беше успешно',
'blockipsuccesstext'              => '„[[Special:Contributions/$1|$1]]“ беше блокиран.<br />
Вижте [[Special:IPBlockList|списъка на блокираните потребители]], за да прегледате всички блокирания.',
'ipb-edit-dropdown'               => 'Причини за блокиране',
'ipb-unblock-addr'                => 'Отблокиране на $1',
'ipb-unblock'                     => 'Отблокиране на потребителско име IP-адрес',
'ipb-blocklist-addr'              => 'Преглед на текущите блокирания на $1',
'ipb-blocklist'                   => 'Преглед на текущите блокирания',
'unblockip'                       => 'Отблокиране на потребител',
'unblockiptext'                   => 'Използвайте долния формуляр, за да възстановите правото на писане на по-рано блокиран IP-адрес или потребител.',
'ipusubmit'                       => 'Отблокиране на адреса',
'unblocked'                       => '[[User:$1|$1]] беше отблокиран.',
'unblocked-id'                    => 'Блок № $1 беше премахнат',
'ipblocklist'                     => 'Списък на блокирани IP-адреси и потребители',
'ipblocklist-legend'              => 'Откриване на блокиран потребител',
'ipblocklist-username'            => 'Потребителско име или IP адрес:',
'ipblocklist-submit'              => 'Търсене',
'blocklistline'                   => '$1, $2 е блокирал $3 ($4)',
'infiniteblock'                   => 'неограничено',
'expiringblock'                   => 'изтича на $1',
'anononlyblock'                   => 'само анон.',
'noautoblockblock'                => 'автоблокировката е изключена',
'createaccountblock'              => 'създаването на сметки е блокирано',
'emailblock'                      => 'е-пощенската услуга е блокирана',
'ipblocklist-empty'               => 'Списъкът на блокиранията е празен.',
'ipblocklist-no-results'          => 'Указаният IP-адрес или потребител не е блокиран.',
'blocklink'                       => 'блокиране',
'unblocklink'                     => 'отблокиране',
'contribslink'                    => 'приноси',
'autoblocker'                     => 'Бяхте блокиран автоматично, тъй като неотдавна IP-адресът ви е бил ползван от блокирания в момента потребител [[User:$1|$1]]. Причината за неговото блокиране е: „$2“.',
'blocklogpage'                    => 'Дневник на блокиранията',
'blocklogentry'                   => 'блокиране на „[[$1]]“ със срок на изтичане $2 $3',
'blocklogtext'                    => 'Тази страница съдържа дневник на блокиранията и отблокиранията, извършени от този потребител.
Автоматично блокираните IP-адреси не са показани.
Вижте [[Special:IPBlockList|списъка на блокираните IP-адреси]] за текущото състояние на блокиранията.',
'unblocklogentry'                 => 'отблокиране на „$1“',
'block-log-flags-anononly'        => 'само анонимни потребители',
'block-log-flags-nocreate'        => 'създаването на сметки е изключено',
'block-log-flags-noautoblock'     => 'автоблокировката е изключена',
'block-log-flags-noemail'         => 'е-пощенската услуга е блокирана',
'block-log-flags-angry-autoblock' => 'разширената автоблокировка е включена',
'range_block_disabled'            => 'Възможността на администраторите да задават интервали при IP-адресите е изключена.',
'ipb_expiry_invalid'              => 'Невалиден срок на изтичане.',
'ipb_already_blocked'             => '„$1“ е вече блокиран',
'ipb_cant_unblock'                => 'Грешка: Не е намерен блок с номер $1. Вероятно потребителят е вече отблокиран.',
'ipb_blocked_as_range'            => 'Грешка: IP-адресът $1 не може да бъде разблокиран, тъй като е част от блокирания регистър $2. Можете да разблокирате адреса, като разблокирате целия регистър.',
'ip_range_invalid'                => 'Невалиден интервал за IP-адреси.',
'blockme'                         => 'Самоблокиране',
'proxyblocker'                    => 'Блокировач на проксита',
'proxyblocker-disabled'           => 'Тази функция е деактивирана.',
'proxyblockreason'                => 'Вашият IP адрес беше блокиран, тъй като е анонимно достъпен междинен сървър. Свържете се с доставчика ви на интернет и го информирайте за този сериозен проблем в сигурността.',
'proxyblocksuccess'               => 'Готово.',
'sorbsreason'                     => 'Вашият IP-адрес е записан като анонимно достъпен междинен сървър в DNSBL на {{SITENAME}}.',
'sorbs_create_account_reason'     => 'Вашият IP-адрес е записан като анонимно достъпен междинен сървър в DNSBL на {{SITENAME}}. Не можете да създадете сметка.',

# Developer tools
'lockdb'              => 'Заключване на базата от данни',
'unlockdb'            => 'Отключване на базата от данни',
'lockdbtext'          => 'Заключването на базата от данни ще попречи на всички потребители да редактират страници, да сменят своите настройки, да редактират своите списъци за наблюдение и на всички други техни действия, изискващи промени в базата данни.
Потвърдете, че искате точно това и ще отключите базата от данни, когато привършите с работата по подръжката.',
'unlockdbtext'        => 'Отключването на базата от данни ще възстанови способността на потребителите да редактират страници, да сменят своите настройки, да редактират своите списъци за наблюдение и изпълнението на всички други действия, изискващи промени в базата от данни.
Потвърдете, че искате точно това.',
'lockconfirm'         => 'Да, наистина искам да заключа базата от данни.',
'unlockconfirm'       => 'Да, наистина искам да отключа базата от данни.',
'lockbtn'             => 'Заключване на базата от данни',
'unlockbtn'           => 'Отключване на базата от данни',
'locknoconfirm'       => 'Не сте отметнали кутийката за потвърждение.',
'lockdbsuccesssub'    => 'Заключването на базата от данни беше успешно',
'unlockdbsuccesssub'  => 'Отключването на базата от данни беше успешно',
'lockdbsuccesstext'   => 'Базата данни на {{SITENAME}} беше заключена.
<br />Не забравяйте да я [[Special:UnlockDB|отключите]] когато привършите с работата по поддръжката.',
'unlockdbsuccesstext' => 'Базата от данни на {{SITENAME}} беше отключена.',
'lockfilenotwritable' => 'Няма права за писане върху файла за заключване на базата данни. За да заключи или отключи базата данни, уеб-сървърът трябва да има тези права.',
'databasenotlocked'   => 'Базата от данни не е заключена.',

# Move page
'move-page'               => 'Преместване на $1',
'move-page-legend'        => 'Преместване на страница',
'movepagetext'            => "Посредством долния формуляр можете да преименувате страница, премествайки цялата й история на новото име. Старото заглавие ще се превърне в пренасочваща страница.
Препратките към старата страница няма да бъдат променени; затова проверете за двойни или невалидни пренасочвания.
Вие сами би трябвало да се убедите в това, дали препратките продължават да сочат там, където се предполага.

Страницата '''няма''' да бъде преместена, ако вече съществува страница с новото име, освен ако е празна или пренасочване и няма редакционна история.

'''ВНИМАНИЕ!'''
Това може да е голяма и неочаквана промяна за известна страница. Уверете се, че разбирате последствията, преди да продължите.",
'movepagetalktext'        => "Ако съществува, съответната дискусионна страница ще бъде преместена автоматично заедно с нея, '''освен ако:'''
* не местите страницата от едно именно пространство в друго,
* вече съществува непразна дискусионна страница с това име или
* не сте отметнали долната кутийка.

В тези случаи, ако желаете, ще е необходимо да преместите страницата ръчно.",
'movearticle'             => 'Преместване на страница:',
'movenotallowed'          => 'Нямате права за преместване на страници.',
'newtitle'                => 'Към ново заглавие:',
'move-watch'              => 'Наблюдаване на страницата',
'movepagebtn'             => 'Преместване',
'pagemovedsub'            => 'Преместването беше успешно',
'movepage-moved'          => "<big>'''Страницата „$1“ беше преместена под името „$2“.'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Вече съществува страница с това име или името, което сте избрали, е невалидно. Изберете друго име.',
'cantmove-titleprotected' => 'Страницата не може да бъде преместена под новото заглавие, тъй като то е защитено от създаване',
'talkexists'              => "'''Страницата беше успешно преместена, но без съответната дискусионна страница, защото под новото име има една съществуваща. Обединете ги ръчно.'''",
'movedto'                 => 'преместена като',
'movetalk'                => 'Преместване и на дискусионната страница, ако е приложимо.',
'move-subpages'           => 'Преместване на всички страници ако е приложимо',
'move-talk-subpages'      => 'Преместване на всички подстраници на беседата ако е приложимо',
'movepage-page-exists'    => 'Страницата $1 вече съществува и няма да бъде автоматично презаписана.',
'movepage-page-moved'     => 'Страницата $1 беше преместена като $2.',
'movepage-page-unmoved'   => 'Страницата $1 не може да бъде преместена като $2.',
'movepage-max-pages'      => 'Преместен беше максималният брой от $1 {{PLURAL:$1|страница|страници}} и повече страници няма да бъдат премествани автоматично.',
'1movedto2'               => '„[[$1]]“ преместена като „[[$2]]“',
'1movedto2_redir'         => '„[[$1]]“ преместена като „[[$2]]“ (върху пренасочване)',
'movelogpage'             => 'Дневник на преместванията',
'movelogpagetext'         => 'По-долу е показан списък на преместванията.',
'movereason'              => 'Причина:',
'revertmove'              => 'връщане',
'delete_and_move'         => 'Изтриване и преместване',
'delete_and_move_text'    => '== Наложително изтриване ==

Целевата страница „[[:$1]]“ вече съществува. Искате ли да я изтриете, за да освободите място за преместването?',
'delete_and_move_confirm' => 'Да, искам да изтрия тази страница.',
'delete_and_move_reason'  => 'Изтрита, за да се освободи място за преместване',
'selfmove'                => 'Страницата не може да бъде преместена, тъй като целевото име съвпада с първоначалното й заглавие.',
'immobile_namespace'      => 'Целевото заглавие е от специален тип. Не е възможно местенето на страници в това именно пространство.',
'imagenocrossnamespace'   => 'Невъзможно е да се преместват картинки извън това именно пространство',
'imagetypemismatch'       => 'Новото разширение на файла не съвпада с типа му',
'imageinvalidfilename'    => 'Целевото име на файл е невалидно',
'fix-double-redirects'    => 'Обновяване на всички двойни пренасочвания, които сочат към оригиналното заглавие',

# Export
'export'            => 'Изнасяне на страници',
'exporttext'        => "Тук можете да изнесете като XML текста и историята на една или повече страници. Получените данни можете да вмъкнете в друг сайт, използващ софтуера МедияУики, чрез [[Special:Import|неговата страница за внaсяне]].

За да изнесете няколко страници, въвеждайте всяко ново заглавие на '''нов ред'''. След това изберете дали искате само текущата версия (заедно с информация за последната редакция) или всички версии (заедно с текущата) на страницата.

Ако желаете само текущата версия, бихте могли да използвате препратка от вида [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] за страницата [[{{MediaWiki:Mainpage}}]].",
'exportcuronly'     => 'Включване само на текущата версия, а не на цялата история',
'exportnohistory'   => "----
'''Важно:''' Изнасянето на пълната история на страниците е забранено, защото много забавя уикито.",
'export-submit'     => 'Изнасяне',
'export-addcattext' => 'Добавяне на страници от категория:',
'export-addcat'     => 'Добавяне',
'export-download'   => 'Съхраняване като файл',
'export-templates'  => 'Включване на шаблоните',

# Namespace 8 related
'allmessages'               => 'Системни съобщения',
'allmessagesname'           => 'Име',
'allmessagesdefault'        => 'Текст по подразбиране',
'allmessagescurrent'        => 'Текущ текст',
'allmessagestext'           => 'Тази страница съдържа списък на системните съобщения от именното пространство „МедияУики“.
Посетете [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] и [http://translatewiki.net Betawiki], ако желаете да допринесете за общата локализация на софтуера МедияУики.',
'allmessagesnotsupportedDB' => "Тази страница не може да бъде използвана, тъй като е изключена възможността '''\$wgUseDatabaseMessages'''.",
'allmessagesfilter'         => 'Филтриране на съобщенията по име:',
'allmessagesmodified'       => 'Показване само на променените',

# Thumbnails
'thumbnail-more'           => 'Увеличаване',
'filemissing'              => 'Липсващ файл',
'thumbnail_error'          => 'Грешка при създаване на миникартинка: $1',
'djvu_page_error'          => 'Номерът на DjVu-страницата е извън обхвата',
'djvu_no_xml'              => 'Не е възможно вземането на XML за DjVu-файла',
'thumbnail_invalid_params' => 'Параметрите за миникартинка са невалидни',
'thumbnail_dest_directory' => 'Целевата директория не може да бъде създадена',

# Special:Import
'import'                     => 'Внасяне на страници',
'importinterwiki'            => 'Внасяне чрез Трансуики',
'import-interwiki-text'      => 'Изберете уики и име на страницата.
Датите на редакциите и имената на авторите ще бъдат запазени.
Всички операции при внасянето от друго уики се записват в [[Special:Log/import|дневника на внасянията]].',
'import-interwiki-history'   => 'Копиране на всички версии на страницата',
'import-interwiki-submit'    => 'Внасяне',
'import-interwiki-namespace' => 'Прехвърляне на страници към именно пространство:',
'importtext'                 => 'Изнесете файла от изходното уики чрез инструмента „[[Special:Export]]“, съхранете го на диска си и го качете тук.',
'importstart'                => 'Внасяне на страници…',
'import-revision-count'      => '$1 {{PLURAL:$1|версия|версии}}',
'importnopages'              => 'Няма страници за внасяне.',
'importfailed'               => 'Внасянето беше неуспешно: $1',
'importunknownsource'        => 'Непознат тип файл',
'importcantopen'             => 'Не е възможно да се отвори файла за внасяне',
'importbadinterwiki'         => 'Невалидна уики препратка',
'importnotext'               => 'Празно',
'importsuccess'              => 'Внасянето беше успешно!',
'importhistoryconflict'      => 'Съществува версия от историята, която си противоречи с тази (възможно е страницата да е била вече внесена)',
'importnosources'            => 'Не са посочени източници за внасяне чрез Трансуики. Прякото качване на версионни истории не е позволено.',
'importnofile'               => 'Файлът за внасяне не беше качен.',
'importuploaderrorsize'      => 'Качването на файла за внасяне беше неуспешно. Файлът е по-голям от максималната допустима за качване големина.',
'importuploaderrorpartial'   => 'Качването на файла за внасяне беше неуспешно. Файлът беше качен частично.',
'importuploaderrortemp'      => 'Качването на файла за внасяне беше неуспешно. Временната директория липсва.',
'import-parse-failure'       => 'Грешка в разбора при внасяне на XML',
'import-noarticle'           => 'Няма страници, които да бъдат внесени!',
'import-nonewrevisions'      => 'Всички версии са били внесени преди.',
'xml-error-string'           => '$1 на ред $2, колона $3 (байт $4): $5',
'import-upload'              => 'Качване на XML данни',

# Import log
'importlogpage'                    => 'Дневник на внасянията',
'importlogpagetext'                => 'Административни внасяния на страници с редакционна история от други уикита.',
'import-logentry-upload'           => '[[$1]] беше внесена от файл',
'import-logentry-upload-detail'    => '{{PLURAL:$1|една версия|$1 версии}}',
'import-logentry-interwiki'        => '$1 беше внесена от друго уики',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|една версия|$1 версии}} на $2 бяха внесени',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Вашата потребителска страница',
'tooltip-pt-anonuserpage'         => 'Потребителската страница за адреса, от който редактирате',
'tooltip-pt-mytalk'               => 'Вашата дискусионна страница',
'tooltip-pt-anontalk'             => 'Дискусия относно редакциите от този адрес',
'tooltip-pt-preferences'          => 'Вашите настройки',
'tooltip-pt-watchlist'            => 'Списък на страници, чиито промени сте избрали да наблюдавате',
'tooltip-pt-mycontris'            => 'Списък на вашите приноси',
'tooltip-pt-login'                => 'Насърчаваме ви да влезете, въпреки че не е задължително.',
'tooltip-pt-anonlogin'            => 'Насърчаваме ви да влезете, въпреки че не е задължително.',
'tooltip-pt-logout'               => 'Излизане от {{SITENAME}}',
'tooltip-ca-talk'                 => 'Беседа относно страницата',
'tooltip-ca-edit'                 => 'Можете да редактирате страницата. Използвайте бутона за предварителен преглед преди да съхраните.',
'tooltip-ca-addsection'           => 'Добавяне на коментар към страницата',
'tooltip-ca-viewsource'           => 'Страницата е защитена. Можете да разгледате изходния й код.',
'tooltip-ca-history'              => 'Предишни версии на страницата',
'tooltip-ca-protect'              => 'Защитаване на страницата',
'tooltip-ca-delete'               => 'Изтриване на страницата',
'tooltip-ca-undelete'             => 'Възстановяване на изтрити редакции на страницата',
'tooltip-ca-move'                 => 'Преместване на страницата',
'tooltip-ca-watch'                => 'Добавяне на страницата към списъка ви за наблюдение',
'tooltip-ca-unwatch'              => 'Премахване на страницата от списъка ви за наблюдение',
'tooltip-search'                  => 'Претърсване на {{SITENAME}}',
'tooltip-search-go'               => 'Отиване на страницата ако тя съществува с точно това име',
'tooltip-search-fulltext'         => 'Търсене в страниците за този текст',
'tooltip-p-logo'                  => 'Началната страница',
'tooltip-n-mainpage'              => 'Началната страница',
'tooltip-n-portal'                => 'Информация за проекта — какво, къде, как',
'tooltip-n-currentevents'         => 'Информация за текущите събития по света',
'tooltip-n-recentchanges'         => 'Списък на последните промени в {{SITENAME}}',
'tooltip-n-randompage'            => 'Зареждане на случайна страница',
'tooltip-n-help'                  => 'Помощната страница',
'tooltip-t-whatlinkshere'         => 'Списък на всички страници, сочещи насам',
'tooltip-t-recentchangeslinked'   => 'Последните промени на страници, сочени от тази страница',
'tooltip-feed-rss'                => 'RSS feed за страницата',
'tooltip-feed-atom'               => 'Atom feed за страницата',
'tooltip-t-contributions'         => 'Показване на приносите на потребителя',
'tooltip-t-emailuser'             => 'Изпращане на писмо до потребителя',
'tooltip-t-upload'                => 'Качване на файлове',
'tooltip-t-specialpages'          => 'Списък на всички специални страници',
'tooltip-t-print'                 => 'Версия за печат на страницата',
'tooltip-t-permalink'             => 'Постоянна препратка към тази версия на страницата',
'tooltip-ca-nstab-main'           => 'Преглед на основната страница',
'tooltip-ca-nstab-user'           => 'Преглед на потребителската страница',
'tooltip-ca-nstab-media'          => 'Преглед на медийната страница',
'tooltip-ca-nstab-special'        => 'Това е специална страница, която не може да се редактира.',
'tooltip-ca-nstab-project'        => 'Преглед на проектната страница',
'tooltip-ca-nstab-image'          => 'Преглед на страницата на файла',
'tooltip-ca-nstab-mediawiki'      => 'Преглед на системното съобщение',
'tooltip-ca-nstab-template'       => 'Преглед на шаблона',
'tooltip-ca-nstab-help'           => 'Преглед на помощната страница',
'tooltip-ca-nstab-category'       => 'Преглед на категорията',
'tooltip-minoredit'               => 'Отбелязване на промяната като малка',
'tooltip-save'                    => 'Съхраняване на промените',
'tooltip-preview'                 => 'Предварителен преглед, използвайте го преди да съхраните!',
'tooltip-diff'                    => 'Показване на направените от вас промени по текста',
'tooltip-compareselectedversions' => 'Показване на разликите между двете избрани версии на страницата',
'tooltip-watch'                   => 'Добавяне на страницата към списъка ви за наблюдение',
'tooltip-recreate'                => 'Възстановяване на страницата независимо, че е била изтрита',
'tooltip-upload'                  => 'Започване на качването',

# Stylesheets
'common.css'   => '/* Чрез редактиране на този файл ще промените всички облици */',
'monobook.css' => '/* Чрез редактиране на този файл можете да промените облика Монобук */',

# Scripts
'common.js'   => '/* Този файл съдържа код на Джаваскрипт и се зарежда при всички потребители. */',
'monobook.js' => '/* Остаряла страница; използвайте [[MediaWiki:Common.js]] */',

# Metadata
'nodublincore'      => 'Метаданните Dublin Core RDF са изключени за този сървър.',
'nocreativecommons' => 'Метаданните Creative Commons RDF са изключени за този сървър.',
'notacceptable'     => 'Сървърът не може да предостави данни във формат, който да се разпознава от клиента ви.',

# Attribution
'anonymous'        => 'Анонимен потребител(и) на {{SITENAME}}',
'siteuser'         => 'потребител на {{SITENAME}} $1',
'lastmodifiedatby' => 'Последната промяна на страницата е извършена от $3 на $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Основаващо се върху работа на $1.',
'others'           => 'други',
'siteusers'        => 'потребителите на {{SITENAME}} $1',
'creditspage'      => 'Библиография и източници',
'nocredits'        => 'Няма въведени източници или библиография.',

# Spam protection
'spamprotectiontitle' => 'Филтър за защита от спам',
'spamprotectiontext'  => 'Страницата, която искахте да съхраните, беше блокирана от филтъра против спам. Това обикновено е причинено от препратка към външен сайт.',
'spamprotectionmatch' => 'Следният текст предизвика включването на филтъра: $1',
'spambot_username'    => 'Спамочистач',
'spam_reverting'      => 'Връщане на последната версия, несъдържаща препратки към $1',
'spam_blanking'       => 'Всички версии, съдържащи препратки към $1, изчистване',

# Info page
'infosubtitle'   => 'Информация за страницата',
'numedits'       => 'Брой редакции (страница): $1',
'numtalkedits'   => 'Брой редакции (дискусионна страница): $1',
'numwatchers'    => 'Брой наблюдатели: $1',
'numauthors'     => 'Брой различни автори (страница): $1',
'numtalkauthors' => 'Брой различни автори (дискусионна страница): $1',

# Math options
'mw_math_png'    => 'Използване винаги на PNG',
'mw_math_simple' => 'HTML при опростен TeX, иначе PNG',
'mw_math_html'   => 'HTML по възможност, иначе PNG',
'mw_math_source' => 'Оставяне като TeX (за текстови браузъри)',
'mw_math_modern' => 'Препоръчително за нови браузъри',
'mw_math_mathml' => 'MathML по възможност (експериментално)',

# Patrolling
'markaspatrolleddiff'                 => 'Отбелязване като проверена версия',
'markaspatrolledtext'                 => 'Отбелязване на версията като проверена',
'markedaspatrolled'                   => 'Проверена версия',
'markedaspatrolledtext'               => 'Избраната версия беше отбелязана като проверена.',
'rcpatroldisabled'                    => 'Патрулът е деактивиран',
'rcpatroldisabledtext'                => 'Патрулът на последните промени е деактивиран',
'markedaspatrollederror'              => 'Не е възможно да се отбележи като проверена',
'markedaspatrollederrortext'          => 'Необходимо е да се посочи редакция, която да бъде отбелязана като проверена.',
'markedaspatrollederror-noautopatrol' => 'Не е разрешено да маркирате своите редакции като проверени.',

# Patrol log
'patrol-log-page'   => 'Дневник на патрула',
'patrol-log-header' => 'Тази страница съдържа дневник на проверените версии.',
'patrol-log-line'   => 'отбеляза $1 от $2 като проверена $3',
'patrol-log-auto'   => '(автоматично)',
'patrol-log-diff'   => 'версия $1',

# Image deletion
'deletedrevision'                 => 'Изтрита стара версия $1',
'filedeleteerror-short'           => 'Грешка при изтриване на файл: $1',
'filedeleteerror-long'            => 'Възникнаха грешки при изтриването на файла:

$1',
'filedelete-missing'              => 'Файлът „$1“ не съществува и затова не може да бъде изтрит.',
'filedelete-old-unregistered'     => 'Посочената версия на файла „$1“ не беше открита в базата от данни.',
'filedelete-current-unregistered' => 'Указаният файл „$1“ не е в базата данни.',
'filedelete-archive-read-only'    => 'Сървърът няма права за писане в архивната директория „$1“.',

# Browsing diffs
'previousdiff' => '← По-стара редакция',
'nextdiff'     => 'По-нова редакция →',

# Media information
'mediawarning'         => "'''Внимание''': Възможно е файлът да съдържа злонамерен програмен код, чието изпълнение да доведе до повреди в системата ви.
<hr />",
'imagemaxsize'         => 'Ограничаване на картинките на описателните им страници до:',
'thumbsize'            => 'Размери на миникартинките:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|страница|страници}}',
'file-info'            => '(големина на файла: $1, MIME-тип: $2)',
'file-info-size'       => '($1 × $2 пиксела, големина на файла: $3, MIME-тип: $4)',
'file-nohires'         => '<small>Не е налична версия с по-висока разделителна способност.</small>',
'svg-long-desc'        => '(Файл във формат SVG, основен размер: $1 × $2 пиксела, големина на файла: $3)',
'show-big-image'       => 'Пълна разделителна способност',
'show-big-image-thumb' => '<small>Размер на предварителния преглед: $1 × $2 пиксела</small>',

# Special:NewImages
'newimages'             => 'Галерия на новите файлове',
'imagelisttext'         => "Списък от {{PLURAL:$1|един файл|'''$1''' файла, сортирани $2}}.",
'newimages-summary'     => 'Тази специална страница показва последно качените файлове.',
'showhidebots'          => '($1 на ботове)',
'noimages'              => 'Няма нищо.',
'ilsubmit'              => 'Търсене',
'bydate'                => 'по дата',
'sp-newimages-showfrom' => 'Показване на новите файлове, като се започва от $2, $1',

# Bad image list
'bad_image_list' => 'Спазва се следният формат:

Отчитат се само записите в списъчен вид (редове, започващи със *). Първата препратка в реда трябва да сочи към неприемлив файл. Всички последващи препратки на същия ред се считат за изключения, т.е. страници, в които този файл може да се визуализира.',

# Metadata
'metadata'          => 'Метаданни',
'metadata-help'     => 'Файлът съдържа допълнителни данни, обикновено добавяни от цифровите апарати или скенери. Ако файлът е редактиран след създаването си, то някои параметри може да не съответстват на текущото изображение.',
'metadata-expand'   => 'Показване на допълнителните данни',
'metadata-collapse' => 'Скриване на допълнителните данни',
'metadata-fields'   => 'EXIF данните, показани в това съобщение, ще бъдат включени на медийната страница, когато информационната таблица е сгъната. Останалите данни ще са скрити по подразбиране.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                => 'Ширина',
'exif-imagelength'               => 'Височина',
'exif-bitspersample'             => 'Дълбочина на цвета (битове)',
'exif-compression'               => 'Вид компресия',
'exif-photometricinterpretation' => 'Състав на пиксела',
'exif-orientation'               => 'Ориентация',
'exif-samplesperpixel'           => 'Редица от компоненти',
'exif-planarconfiguration'       => 'Принцип на организация на данните',
'exif-ycbcrpositioning'          => 'Y и C позициониране',
'exif-xresolution'               => 'Хоризонтална разделителна способност',
'exif-yresolution'               => 'Вертикална разделителна способност',
'exif-rowsperstrip'              => 'Брой редове на ивица',
'exif-stripbytecounts'           => 'Байтове на компресирана ивица',
'exif-transferfunction'          => 'Функция по пренос',
'exif-whitepoint'                => 'Хроматичност на бялото',
'exif-ycbcrcoefficients'         => 'Коефициенти в матрицата на трансформацията на цветовото пространство',
'exif-datetime'                  => 'Дата и час на изменението на файла',
'exif-imagedescription'          => 'Название на изображението',
'exif-make'                      => 'Производител',
'exif-model'                     => 'Модел на фотоапарата',
'exif-software'                  => 'Използван софтуер',
'exif-artist'                    => 'Автор',
'exif-copyright'                 => 'Притежател на авторското право',
'exif-exifversion'               => 'Exif версия',
'exif-flashpixversion'           => 'Поддържана версия Flashpix',
'exif-colorspace'                => 'Цветово пространство',
'exif-componentsconfiguration'   => 'Значение на всеки компонент',
'exif-compressedbitsperpixel'    => 'Режим на компресия на образа',
'exif-pixelydimension'           => 'Пълна ширина на изображението',
'exif-pixelxdimension'           => 'Пълна височина на изображението',
'exif-makernote'                 => 'Допълнителни данни на производителя',
'exif-usercomment'               => 'Допълнителни коментари',
'exif-relatedsoundfile'          => 'Свързан звуков файл',
'exif-datetimeoriginal'          => 'Дата и час на създаване',
'exif-datetimedigitized'         => 'Дата и час на записа',
'exif-exposuretime'              => 'Време на експонация',
'exif-exposuretime-format'       => '$1 сек ($2)',
'exif-fnumber'                   => 'F (бленда)',
'exif-exposureprogram'           => 'Програма на експонацията',
'exif-spectralsensitivity'       => 'Спектрална чувствителност',
'exif-isospeedratings'           => 'Светлочувствителност ISO',
'exif-shutterspeedvalue'         => 'Скорост на затвора',
'exif-aperturevalue'             => 'Диаметър на обектива',
'exif-brightnessvalue'           => 'Светлосила',
'exif-exposurebiasvalue'         => 'Отклонение от експонацията',
'exif-subjectdistance'           => 'Разстояние до обекта',
'exif-meteringmode'              => 'Режим на измерване',
'exif-lightsource'               => 'Източник на светлина',
'exif-flash'                     => 'Светкавица',
'exif-focallength'               => 'Фокусно разстояние',
'exif-subjectarea'               => 'Зона на обекта',
'exif-flashenergy'               => 'Мощност на светкавицата',
'exif-spatialfrequencyresponse'  => 'Пространствен честотен отклик',
'exif-focalplanexresolution'     => 'Фокусна равнина X резолюция',
'exif-focalplaneyresolution'     => 'Фокусна равнина Y резолюция',
'exif-focalplaneresolutionunit'  => 'Единица за разделителна способност на фокалната равнина',
'exif-subjectlocation'           => 'Местоположение на обекта',
'exif-exposureindex'             => 'Индекс на експонацията',
'exif-sensingmethod'             => 'Метод на засичане',
'exif-filesource'                => 'Файлов източник',
'exif-scenetype'                 => 'Вид сцена',
'exif-cfapattern'                => 'Стандартен цветови стил',
'exif-customrendered'            => 'Допълнителна обработка на изображението',
'exif-exposuremode'              => 'Режим на експонация',
'exif-whitebalance'              => 'Баланс на бялото',
'exif-digitalzoomratio'          => 'Съотношение на цифровото увеличение',
'exif-focallengthin35mmfilm'     => 'Фокусно разстояние в 35 mm филм',
'exif-gaincontrol'               => 'Увеличение на яркостта',
'exif-contrast'                  => 'Контраст',
'exif-saturation'                => 'Наситеност',
'exif-sharpness'                 => 'Острота',
'exif-devicesettingdescription'  => 'Описание на настройките на апарата',
'exif-imageuniqueid'             => 'Уникален идентификатор на изображението',
'exif-gpslatituderef'            => 'Северна или южна ширина',
'exif-gpslatitude'               => 'Географска ширина',
'exif-gpslongituderef'           => 'Източна или западна дължина',
'exif-gpslongitude'              => 'Географска дължина',
'exif-gpsaltituderef'            => 'Отправна височина',
'exif-gpsaltitude'               => 'Надморска височина',
'exif-gpstimestamp'              => 'GPS време (атомен часвник)',
'exif-gpssatellites'             => 'Използвани за измерването сателити',
'exif-gpsstatus'                 => 'Състояние на получателя',
'exif-gpsmeasuremode'            => 'Метод за измерване',
'exif-gpsdop'                    => 'Прецизност',
'exif-gpsspeedref'               => 'Единица за скорост',
'exif-gpsspeed'                  => 'Скорост на GPS приемник',
'exif-gpstrack'                  => 'Посока на движение',
'exif-gpsimgdirection'           => 'Направление на изображението',
'exif-gpsdestlatitude'           => 'Географска ширина на целта',
'exif-gpsdestlongitude'          => 'Географска дължина на целта',
'exif-gpsdestbearing'            => 'Местоположение на целта',
'exif-gpsdestdistance'           => 'Разстояние до целта',
'exif-gpsareainformation'        => 'Име на GPS зоната',
'exif-gpsdatestamp'              => 'GPS дата',
'exif-gpsdifferential'           => 'Диференциална корекция на GPS',

# EXIF attributes
'exif-compression-1' => 'Некомпресиран',

'exif-unknowndate' => 'Неизвестна дата',

'exif-orientation-1' => 'Нормално', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Отражение по хоризонталата', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Обърнато на 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Отражение по вертикалата', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Обърнато на 90° срещу часовниковата стрелка и отразено по вертикалата', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Обърнато на 90° по часовниковата стрелка', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Обърнато на 90° по часовниковата стрелка и отразено по вертикалата', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Обърнато на 90° срещу часовниковата стрелка', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'формат „chunky“',
'exif-planarconfiguration-2' => 'формат „planar“',

'exif-componentsconfiguration-0' => 'не съществува',

'exif-exposureprogram-0' => 'Не е определено',
'exif-exposureprogram-1' => 'Ръчна настройка',
'exif-exposureprogram-2' => 'Нормална програма',
'exif-exposureprogram-3' => 'Приоритет на блендата',
'exif-exposureprogram-4' => 'Приоритет на скоростта',
'exif-exposureprogram-5' => 'Приоритет на дълбочината на фокуса',
'exif-exposureprogram-6' => 'Приоритет на скоростта на затвора',
'exif-exposureprogram-7' => 'Режим „Портрет“ (за снимки в едър план, фонът не е на фокус)',
'exif-exposureprogram-8' => 'Режим „Пейзаж“ (за пейзажни снимки, в които фонът е на фокус)',

'exif-subjectdistance-value' => '$1 метра',

'exif-meteringmode-0'   => 'Неизвестно',
'exif-meteringmode-1'   => 'Средно',
'exif-meteringmode-2'   => 'Централно измерване на светлината',
'exif-meteringmode-3'   => 'Точково измерване',
'exif-meteringmode-4'   => 'Многоточково измерване',
'exif-meteringmode-5'   => 'Образец',
'exif-meteringmode-6'   => 'Частично измерване',
'exif-meteringmode-255' => 'Друго',

'exif-lightsource-0'   => 'неизвестно',
'exif-lightsource-1'   => 'дневна светлина',
'exif-lightsource-2'   => 'Флуоресцентно осветление',
'exif-lightsource-3'   => 'Волфрамово осветление',
'exif-lightsource-4'   => 'Светкавица',
'exif-lightsource-9'   => 'хубаво време',
'exif-lightsource-10'  => 'облачно',
'exif-lightsource-11'  => 'Сянка',
'exif-lightsource-17'  => 'Стандартна светлина тип A',
'exif-lightsource-18'  => 'Стандартна светлина тип B',
'exif-lightsource-19'  => 'Стандартна светлина тип C',
'exif-lightsource-24'  => 'Студийна лампа стандарт ISO',
'exif-lightsource-255' => 'друг източник на светлина',

'exif-focalplaneresolutionunit-2' => 'инчове',

'exif-sensingmethod-1' => 'Неопределено',
'exif-sensingmethod-2' => 'Едночипов цветови пространствен сензор',
'exif-sensingmethod-3' => 'Двучипов цветови пространствен сензор',
'exif-sensingmethod-4' => 'Тричипов цветови пространствен сензор',
'exif-sensingmethod-5' => 'Цветови последователен пространствен сензор',
'exif-sensingmethod-7' => 'Трилинеен сензор',
'exif-sensingmethod-8' => 'Цветови последователен линеен сензор',

'exif-filesource-3' => 'цифров фотоапарат',

'exif-scenetype-1' => 'Пряко заснето изображение',

'exif-customrendered-0' => 'нормален процес',
'exif-customrendered-1' => 'нестандартна обработка',

'exif-exposuremode-0' => 'автоматична експонация',
'exif-exposuremode-1' => 'ръчна експонация',
'exif-exposuremode-2' => 'Автоматичен клин',

'exif-whitebalance-0' => 'Автоматичен баланс на бялото',
'exif-whitebalance-1' => 'Ръчно определяне на баланса на бялото',

'exif-scenecapturetype-0' => 'Стандартен',
'exif-scenecapturetype-1' => 'Ландшафт',
'exif-scenecapturetype-2' => 'Портрет',
'exif-scenecapturetype-3' => 'Нощна сцена',

'exif-gaincontrol-0' => 'Нищо',
'exif-gaincontrol-1' => 'Неголямо увеличение',
'exif-gaincontrol-2' => 'Голямо увеличение',
'exif-gaincontrol-3' => 'Неголямо намаление',
'exif-gaincontrol-4' => 'Силно намаление',

'exif-contrast-0' => 'Нормален',
'exif-contrast-1' => 'Слабо повишение',
'exif-contrast-2' => 'Силно повишение',

'exif-saturation-0' => 'Нормална',
'exif-saturation-1' => 'Неголяма наситеност',
'exif-saturation-2' => 'Голяма наситеност',

'exif-sharpness-0' => 'Нормална',
'exif-sharpness-1' => 'по-меко',
'exif-sharpness-2' => 'по-остро',

'exif-subjectdistancerange-0' => 'Неизвестен',
'exif-subjectdistancerange-1' => 'Макро',
'exif-subjectdistancerange-2' => 'Близко',
'exif-subjectdistancerange-3' => 'Далечно',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'северна ширина',
'exif-gpslatitude-s' => 'южна ширина',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'източна дължина',
'exif-gpslongitude-w' => 'западна дължина',

'exif-gpsstatus-a' => 'Измерване в ход',
'exif-gpsstatus-v' => 'Оперативна съвместимост на измерването',

'exif-gpsmeasuremode-2' => 'Двуизмерно измерване',
'exif-gpsmeasuremode-3' => 'Триизмерно измерване',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'км/час',
'exif-gpsspeed-m' => 'мили/час',
'exif-gpsspeed-n' => 'възли',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'истинска',
'exif-gpsdirection-m' => 'магнитна',

# External editor support
'edit-externally'      => 'Редактиране на файла чрез външно приложение',
'edit-externally-help' => 'За повече информация прегледайте [http://www.mediawiki.org/wiki/Manual:External_editors указанията за настройките].',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'всички',
'imagelistall'     => 'всички',
'watchlistall2'    => 'всички',
'namespacesall'    => 'Всички',
'monthsall'        => 'всички',

# E-mail address confirmation
'confirmemail'             => 'Потвърждаване на адрес за електронна поща',
'confirmemail_noemail'     => 'Не сте посочили валиден адрес за електронна поща в [[Special:Preferences|настройки си]].',
'confirmemail_text'        => '{{SITENAME}} изисква да потвърдите адреса си за електронна поща преди да използвате възможностите за е-поща. Натиснете долния бутон, за да ви бъде изпратено писмо, съдържащо специално генерирана препратка, чрез която ще можете да потвърдите валидността на адреса си.',
'confirmemail_pending'     => '<div class="error">
Кодът за потвърждение вече е изпратен. Ако току-що сте се регистрирали, изчакайте няколко минути да пристигне писмото, преди да поискате нов код.
</div>',
'confirmemail_send'        => 'Изпращане на код за потвърждение',
'confirmemail_sent'        => 'Кодът за потвърждение беше изпратен.',
'confirmemail_oncreate'    => 'Код за потвърждение беше изпратен на електронната ви поща.
Този код не е необходим за влизане, но ще ви трябва при активирането на функциите в {{SITENAME}}, изискващи валидна електронна поща.',
'confirmemail_sendfailed'  => '{{SITENAME}} не можа да изпрати писмо с код за потвърждение. Проверете адреса си за недопустими знаци.
Изпращачът на е-поща отвърна: $1',
'confirmemail_invalid'     => 'Грешен код за потвърждение. Възможно е кодът да е остарял.',
'confirmemail_needlogin'   => 'Необходимо е да $1, за потвърждаване на адреса за електронна поща.',
'confirmemail_success'     => 'Адресът ви за електронна поща беше потвърден. Вече можете да влезете и да се наслаждавате на уикито.',
'confirmemail_loggedin'    => 'Адресът ви за електронна поща беше потвърден.',
'confirmemail_error'       => 'Станала е грешка при потвърждаването на адреса ви.',
'confirmemail_subject'     => '{{SITENAME}} — Потвърждаване на адрес за е-поща',
'confirmemail_body'        => 'Някой, вероятно вие, от IP-адрес $1, е регистрирал потребител „$2“ в {{SITENAME}}, като е посочил този адрес за електронна поща.

За да потвърдите, че сметката в {{SITENAME}} и настоящият пощенски адрес са ваши, заредете долната препратка в браузъра си:

$3

Ако някой друг е направил регистрацията в {{SITENAME}} и не желаете да я потвърждавате, последвайте препратката по-долу:

$5

Кодът за потвърждение ще загуби валидност след $4.',
'confirmemail_invalidated' => 'Отменено потвърждение за електронна поща',
'invalidateemail'          => 'Отмяна на потвърждението за електронна поща',

# Scary transclusion
'scarytranscludedisabled' => '[Включването между уикита е деактивирано]',
'scarytranscludefailed'   => '[Зареждането на шаблона за $1 не сполучи]',
'scarytranscludetoolong'  => '[Адресът е твърде дълъг]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Обратни следи за статията:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Изтриване])',
'trackbacklink'     => 'Обратна следа',
'trackbackdeleteok' => 'Обратната следа беше изтрита.',

# Delete conflict
'deletedwhileediting' => "'''Внимание''': Страницата е била изтрита, след като сте започнали да я редактирате!",
'confirmrecreate'     => "Потребителят [[User:$1|$1]] ([[User talk:$1|беседа]]) е изтрил страницата, откакто сте започнали да я редактирате, като е посочил следното обяснение:
: ''$2''
Потвърдете, че наистина желаете да създадете страницата отново.",
'recreate'            => 'Ново създаване',

# HTML dump
'redirectingto' => 'Пренасочване към [[:$1]]…',

# action=purge
'confirm_purge'        => 'Изчистване на складираното копие на страницата?

$1',
'confirm_purge_button' => 'Добре',

# AJAX search
'searchcontaining' => "Търсене на статии, съдържащи ''$1''.",
'searchnamed'      => "Търсене на статии, чиито имена съдържат ''$1''.",
'articletitles'    => "Страници, започващи с ''$1''",
'hideresults'      => 'Скриване на резултатите',
'useajaxsearch'    => 'Използване на AJAX-търсене',

# Multipage image navigation
'imgmultipageprev' => '← предишна страница',
'imgmultipagenext' => 'следваща страница →',
'imgmultigo'       => 'Отиване',
'imgmultigoto'     => 'Отиване на страница $1',

# Table pager
'ascending_abbrev'         => 'възх',
'descending_abbrev'        => 'низх',
'table_pager_next'         => 'Следваща страница',
'table_pager_prev'         => 'Предишна страница',
'table_pager_first'        => 'Първа страница',
'table_pager_last'         => 'Последна страница',
'table_pager_limit'        => 'Показване на $1 записа на страница',
'table_pager_limit_submit' => 'Отиване',
'table_pager_empty'        => 'Няма резултати',

# Auto-summaries
'autosumm-blank'   => 'Премахване на цялото съдържание на страницата',
'autosumm-replace' => 'Заместване на съдържанието на страницата с „$1“',
'autoredircomment' => 'Пренасочване към [[$1]]',
'autosumm-new'     => 'Нова страница: $1',

# Live preview
'livepreview-loading' => 'Зарежда се…',
'livepreview-ready'   => 'Зарежда се… Готово!',
'livepreview-failed'  => 'Бързият предварителен преглед не е възможен! Опитайте нормален предварителен преглед.',
'livepreview-error'   => 'Връзката не сполучи: $1 „$2“ Опитайте нормален предварителен преглед.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Промените от {{PLURAL:$1|последната $1 секунда|последните $1 секунди}} вероятно не са показани в списъка.',
'lag-warn-high'   => 'Поради голямото изоставане в сървърната синхронизация, промените от {{PLURAL:$1|последната $1 секунда|последните $1 секунди}} вероятно не са показани в списъка.',

# Watchlist editor
'watchlistedit-numitems'       => 'Списъкът ви за наблюдение съдържа {{PLURAL:$1|1 страница |$1 страници}} (без беседите).',
'watchlistedit-noitems'        => 'Списъкът ви за наблюдение е празен.',
'watchlistedit-normal-title'   => 'Редактиране на списъка за наблюдение',
'watchlistedit-normal-legend'  => 'Премахване на записи от списъка за наблюдение',
'watchlistedit-normal-explain' => 'По-долу са показани заглавията на страниците от списъка ви за наблюдение. За да премахнете страница, отбележете полето пред нея и щракнете на бутона „Премахване“. Можете също да редактирате [[Special:Watchlist/raw|необработения списък за наблюдение]].',
'watchlistedit-normal-submit'  => 'Премахване',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 страница беше премахната|$1 страници бяха премахнати}} от вашия списък за наблюдение:',
'watchlistedit-raw-title'      => 'Редактиране на необработения списък за наблюдение',
'watchlistedit-raw-legend'     => 'Редактиране на необработения списък за наблюдение',
'watchlistedit-raw-explain'    => 'По-долу са показани заглавията на страниците във вашия списък за наблюдение, които можете да редактирате, като добавяте или премахвате по едно заглавие на ред. Когато приключите, щракнете бутона „Обновяване на списъка за наблюдение“.
	Можете да използвате и [[Special:Watchlist/edit|стандартния редактор]].',
'watchlistedit-raw-titles'     => 'Страници:',
'watchlistedit-raw-submit'     => 'Обновяване на списъка за наблюдение',
'watchlistedit-raw-done'       => 'Вашият списък за наблюдение е обновен.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 страница беше добавена|$1 страници бяха добавени}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Една страница беше премахната|$1 страници бяха премахнати}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Преглед на списъка за наблюдение',
'watchlisttools-edit' => 'Преглед и редактиране на списъка за наблюдение',
'watchlisttools-raw'  => 'Редактиране на необработения списък за наблюдение',

# Core parser functions
'unknown_extension_tag' => 'Непознат етикет на разширение „$1“',

# Special:Version
'version'                          => 'Версия', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Инсталирани разширения',
'version-specialpages'             => 'Специални страници',
'version-parserhooks'              => 'Куки в парсера',
'version-variables'                => 'Променливи',
'version-other'                    => 'Други',
'version-mediahandlers'            => 'Обработчици на медия',
'version-hooks'                    => 'Куки',
'version-extension-functions'      => 'Допълнителни функции',
'version-parser-extensiontags'     => 'Етикети от парсерни разширения',
'version-parser-function-hooks'    => 'Куки в парсерни функции',
'version-skin-extension-functions' => 'Функции на разширения за облици',
'version-hook-name'                => 'Име на куката',
'version-hook-subscribedby'        => 'Ползвана от',
'version-version'                  => 'Версия',
'version-license'                  => 'Лиценз',
'version-software'                 => 'Инсталиран софтуер',
'version-software-product'         => 'Продукт',
'version-software-version'         => 'Версия',

# Special:FilePath
'filepath'         => 'Път към файл',
'filepath-page'    => 'Файл:',
'filepath-submit'  => 'Път',
'filepath-summary' => 'Тази специална страница връща пълния път до даден файл. Изображенията се показват в пълната им разделителна способност, а други типове файлове се отварят направо с приложенията, с които са асоциирани.

Името на файла се изписва без представката „{{ns:image}}:“',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Търсене на повтарящи се файлове',
'fileduplicatesearch-summary'  => 'Въведете име на файл (без представката „{{ns:image}}:“), за да потърсите повтарящи го файлове въз основа на неговата хеш стойност.',
'fileduplicatesearch-legend'   => 'Търсене на повтарящ се файл',
'fileduplicatesearch-filename' => 'Име на файл:',
'fileduplicatesearch-submit'   => 'Търсене',
'fileduplicatesearch-info'     => '$1 × $2 пиксела<br />Размер на файла: $3<br />MIME тип: $4',
'fileduplicatesearch-result-1' => 'Файлът "$1" няма идентично копие.',
'fileduplicatesearch-result-n' => 'Файлът "$1" има {{PLURAL:$2|едно идентично копие|$2 идентични копия}}.',

# Special:SpecialPages
'specialpages'                   => 'Специални страници',
'specialpages-note'              => '----
* Обикновени специални страници.
* <span class="mw-specialpagerestricted">Специални страници с ограничения.</span>',
'specialpages-group-maintenance' => 'Доклади по поддръжката',
'specialpages-group-other'       => 'Други специални страници',
'specialpages-group-login'       => 'Влизане / регистриране',
'specialpages-group-changes'     => 'Последни промени и дневници',
'specialpages-group-media'       => 'Доклади за файловете и качванията',
'specialpages-group-users'       => 'Потребители и права',
'specialpages-group-highuse'     => 'Широко използвани страници',
'specialpages-group-pages'       => 'Списък на страниците',
'specialpages-group-pagetools'   => 'Инструменти за страниците',
'specialpages-group-wiki'        => 'Уики данни и инструменти',
'specialpages-group-redirects'   => 'Пренасочващи специални страници',
'specialpages-group-spam'        => 'Инструменти против спам',

# Special:BlankPage
'blankpage'              => 'Празна страница',
'intentionallyblankpage' => 'Тази страница умишлено е оставена празна',

);
