<?php
/** Russian (Русский)
 *
 * @ingroup Language
 * @file
 *
 * @author Ahonc
 * @author Aleksandrit
 * @author Alessandro
 * @author AlexSm
 * @author Alexander Sigachov (alexander.sigachov@gmail.com)
 * @author EugeneZelenko
 * @author Flrn
 * @author HalanTul
 * @author Illusion
 * @author Innv
 * @author Kaganer
 * @author Kalan
 * @author MaxSem
 * @author Putnik
 * @author VasilievVV
 * @author Александр Сигачёв
 * @author לערי ריינהארט
 */

/*
 * Изменения сделанные в этом файле будут потеряны при обновлении MediaWiki.
 *
 * Если необходимо внести изменения в перевод отдельных строк интерфейса,
 * сделайте это посредством редактирования страниц вида «MediaWiki:*».
 * Их список можно найти на странице «Special:Allmessages».
 */

$separatorTransformTable = array(
	',' => "\xc2\xa0", # nbsp
	'.' => ','
);

$fallback8bitEncoding = 'windows-1251';
$linkPrefixExtension = false;

$namespaceNames = array(
	NS_MEDIA            => 'Медиа',
	NS_SPECIAL          => 'Служебная',
	NS_MAIN             => '',
	NS_TALK             => 'Обсуждение',
	NS_USER             => 'Участник',
	NS_USER_TALK        => 'Обсуждение_участника',
	#NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Обсуждение_{{grammar:genitive|$1}}',
	NS_IMAGE            => 'Изображение',
	NS_IMAGE_TALK       => 'Обсуждение_изображения',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Обсуждение_MediaWiki',
	NS_TEMPLATE         => 'Шаблон',
	NS_TEMPLATE_TALK    => 'Обсуждение_шаблона',
	NS_HELP             => 'Справка',
	NS_HELP_TALK        => 'Обсуждение_справки',
	NS_CATEGORY         => 'Категория',
	NS_CATEGORY_TALK    => 'Обсуждение_категории',
);

$namespaceAliases = array(
	'Участница'            => NS_USER,
	'Обсуждение участницы' => NS_USER_TALK,
);

$skinNames = array(
	'standard'    => 'Классическое',
	'nostalgia'   => 'Ностальгия',
	'cologneblue' => 'Кёльнская тоска',
	'myskin'      => 'Своё',
	'chick'       => 'Цыпа',
	'simple'      => 'Простое',
	'modern'      => 'Современное',
);

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'xg j, Y',
	'mdy both' => 'H:i, xg j, Y',

	'dmy time' => 'H:i',
	'dmy date' => 'j xg Y',
	'dmy both' => 'H:i, j xg Y',

	'ymd time' => 'H:i',
	'ymd date' => 'Y xg j',
	'ymd both' => 'H:i, Y xg j',

	'ISO 8601 time' => 'xnH:xni:xns',
	'ISO 8601 date' => 'xnY-xnm-xnd',
	'ISO 8601 both' => 'xnY-xnm-xnd"T"xnH:xni:xns',

);

$bookstoreList = array(
	'Поиск по библиотекам «Сигла»' => 'http://www.sigla.ru/results.jsp?f=7&t=3&v0=5030030980&f=1003&t=1&v1=&f=4&t=2&v2=&f=21&t=3&v3=&f=1016&t=3&v4=&f=1016&t=3&v5=&bf=4&b=&d=0&ys=&ye=&lng=&ft=&mt=&dt=&vol=&pt=&iss=&ps=&pe=&tr=&tro=&cc=&i=1&v=tagged&s=0&ss=0&st=0&i18n=ru&rlf=&psz=20&bs=20&ce=hJfuypee8JzzufeGmImYYIpZKRJeeOeeWGJIZRrRRrdmtdeee88NJJJJpeeefTJ3peKJJ3UWWPtzzzzzzzzzzzzzzzzzbzzvzzpy5zzjzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzztzzzzzzzbzzzzzzzzzzzzzzzzzzzzzzzzzzzvzzzzzzyeyTjkDnyHzTuueKZePz9decyzzLzzzL*.c8.NzrGJJvufeeeeeJheeyzjeeeeJh*peeeeKJJJJJJJJJJmjHvOJJJJJJJJJfeeeieeeeSJJJJJSJJJ3TeIJJJJ3..E.UEAcyhxD.eeeeeuzzzLJJJJ5.e8JJJheeeeeeeeeeeeyeeK3JJJJJJJJ*s7defeeeeeeeeeeeeeeeeeeeeeeeeeSJJJJJJJJZIJJzzz1..6LJJJJJJtJJZ4....EK*&debug=false',
	'Findbook.ru' => 'http://findbook.ru/search/d0?ptype=4&pvalue=$1',
	'Яндекс.Маркет' => 'http://market.yandex.ru/search.xml?text=$1',
	'ОЗОН' => 'http://www.ozon.ru/?context=advsearch_book&isbn=$1',
	'Books.Ru' => 'http://www.books.ru/shop/search/advanced?as%5Btype%5D=books&as%5Bname%5D=&as%5Bisbn%5D=$1&as%5Bauthor%5D=&as%5Bmaker%5D=&as%5Bcontents%5D=&as%5Binfo%5D=&as%5Bdate_after%5D=&as%5Bdate_before%5D=&as%5Bprice_less%5D=&as%5Bprice_more%5D=&as%5Bstrict%5D=%E4%E0&as%5Bsub%5D=%E8%F1%EA%E0%F2%FC&x=22&y=8',
	'Amazon.com' => 'http://www.amazon.com/exec/obidos/ISBN=$1'
);

$magicWords = array(
	'redirect'            => array( '0', '#REDIRECT', '#ПЕРЕНАПРАВЛЕНИЕ', '#ПЕРЕНАПР' ),
	'notoc'               => array( '0', '__NOTOC__', '__БЕЗ_ОГЛ__' ),
	'nogallery'           => array( '0', '__NOGALLERY__', '__БЕЗ_ГАЛЕРЕИ__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__ОБЯЗ_ОГЛ__' ),
	'toc'                 => array( '0', '__TOC__', '__ОГЛ__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__БЕЗ_РЕДАКТИРОВАНИЯ_РАЗДЕЛА__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'ТЕКУЩИЙ_МЕСЯЦ' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'НАЗВАНИЕ_ТЕКУЩЕГО_МЕСЯЦА' ),
	'currentmonthnamegen' => array( '1', 'CURRENTMONTHNAMEGEN', 'НАЗВАНИЕ_ТЕКУЩЕГО_МЕСЯЦА_РОД' ),
	'currentmonthabbrev'  => array( '1', 'CURRENTMONTHABBREV', 'НАЗВАНИЕ_ТЕКУЩЕГО_МЕСЯЦА_АБР' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'ТЕКУЩИЙ_ДЕНЬ' ),
	'currentday2'         => array( '1', 'CURRENTDAY2', 'ТЕКУЩИЙ_ДЕНЬ_2' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'НАЗВАНИЕ_ТЕКУЩЕГО_ДНЯ' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'ТЕКУЩИЙ_ГОД' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'ТЕКУЩЕЕ_ВРЕМЯ' ),
	'currenthour'         => array( '1', 'CURRENTHOUR', 'ТЕКУЩИЙ_ЧАС' ),
	'localmonth'          => array( '1', 'LOCALMONTH', 'МЕСТНЫЙ_МЕСЯЦ' ),
	'localmonthname'      => array( '1', 'LOCALMONTHNAME', 'НАЗВАНИЕ_МЕСТНОГО_МЕСЯЦА' ),
	'localmonthnamegen'   => array( '1', 'LOCALMONTHNAMEGEN', 'НАЗВАНИЕ_МЕСТНОГО_МЕСЯЦА_РОД' ),
	'localmonthabbrev'    => array( '1', 'LOCALMONTHABBREV', 'НАЗВАНИЕ_МЕСТНОГО_МЕСЯЦА_АБР' ),
	'localday'            => array( '1', 'LOCALDAY', 'МЕСТНЫЙ_ДЕНЬ' ),
	'localday2'           => array( '1', 'LOCALDAY2', 'МЕСТНЫЙ_ДЕНЬ_2' ),
	'localdayname'        => array( '1', 'LOCALDAYNAME', 'НАЗВАНИЕ_МЕСТНОГО_ДНЯ' ),
	'localyear'           => array( '1', 'LOCALYEAR', 'МЕСТНЫЙ_ГОД' ),
	'localtime'           => array( '1', 'LOCALTIME', 'МЕСТНОЕ_ВРЕМЯ' ),
	'localhour'           => array( '1', 'LOCALHOUR', 'МЕСТНЫЙ_ЧАС' ),
	'numberofpages'       => array( '1', 'NUMBEROFPAGES', 'КОЛИЧЕСТВО_СТРАНИЦ' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'КОЛИЧЕСТВО_СТАТЕЙ' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'КОЛИЧЕСТВО_ФАЙЛОВ' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'КОЛИЧЕСТВО_УЧАСТНИКОВ' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'КОЛИЧЕСТВО_ПРАВОК' ),
	'pagename'            => array( '1', 'PAGENAME', 'НАЗВАНИЕ_СТРАНИЦЫ' ),
	'pagenamee'           => array( '1', 'PAGENAMEE', 'НАЗВАНИЕ_СТРАНИЦЫ_2' ),
	'namespace'           => array( '1', 'NAMESPACE', 'ПРОСТРАНСТВО_ИМЁН' ),
	'namespacee'          => array( '1', 'NAMESPACEE', 'ПРОСТРАНСТВО_ИМЁН_2' ),
	'talkspace'           => array( '1', 'TALKSPACE', 'ПРОСТРАНСТВО_ОБСУЖДЕНИЙ' ),
	'talkspacee'          => array( '1', 'TALKSPACEE', 'ПРОСТРАНСТВО_ОБСУЖДЕНИЙ_2' ),
	'subjectspace'        => array( '1', 'SUBJECTSPACE', 'ARTICLESPACE', 'ПРОСТРАНСТВО_СТАТЕЙ' ),
	'subjectspacee'       => array( '1', 'SUBJECTSPACEE', 'ARTICLESPACEE', 'ПРОСТРАНСТВО_СТАТЕЙ_2' ),
	'fullpagename'        => array( '1', 'FULLPAGENAME', 'ПОЛНОЕ_НАЗВАНИЕ_СТРАНИЦЫ' ),
	'fullpagenamee'       => array( '1', 'FULLPAGENAMEE', 'ПОЛНОЕ_НАЗВАНИЕ_СТРАНИЦЫ_2' ),
	'subpagename'         => array( '1', 'SUBPAGENAME', 'НАЗВАНИЕ_ПОДСТРАНИЦЫ' ),
	'subpagenamee'        => array( '1', 'SUBPAGENAMEE', 'НАЗВАНИЕ_ПОДСТРАНИЦЫ_2' ),
	'basepagename'        => array( '1', 'BASEPAGENAME', 'ОСНОВА_НАЗВАНИЯ_СТРАНИЦЫ' ),
	'basepagenamee'       => array( '1', 'BASEPAGENAMEE', 'ОСНОВА_НАЗВАНИЯ_СТРАНИЦЫ_2' ),
	'talkpagename'        => array( '1', 'TALKPAGENAME', 'НАЗВАНИЕ_СТРАНИЦЫ_ОБСУЖДЕНИЯ' ),
	'talkpagenamee'       => array( '1', 'TALKPAGENAMEE', 'НАЗВАНИЕ_СТРАНИЦЫ_ОБСУЖДЕНИЯ_2' ),
	'subjectpagename'     => array( '1', 'SUBJECTPAGENAME', 'ARTICLEPAGENAME', 'НАЗВАНИЕ_СТРАНИЦЫ_СТАТЬИ' ),
	'subjectpagenamee'    => array( '1', 'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE', 'НАЗВАНИЕ_СТРАНИЦЫ_СТАТЬИ_2' ),
	'msg'                 => array( '0', 'MSG:', 'СООБЩ:' ),
	'subst'               => array( '0', 'SUBST:', 'ПОДСТ:' ),
	'msgnw'               => array( '0', 'MSGNW:', 'СООБЩ_БЕЗ_ВИКИ:' ),
	'img_thumbnail'       => array( '1', 'thumbnail', 'thumb', 'мини' ),
	'img_manualthumb'     => array( '1', 'thumbnail=$1', 'thumb=$1', 'мини=$1' ),
	'img_right'           => array( '1', 'right', 'справа' ),
	'img_left'            => array( '1', 'left', 'слева' ),
	'img_none'            => array( '1', 'none', 'без' ),
	'img_width'           => array( '1', '$1px', '$1пкс' ),
	'img_center'          => array( '1', 'center', 'centre', 'центр' ),
	'img_framed'          => array( '1', 'framed', 'enframed', 'frame', 'обрамить' ),
	'img_frameless'       => array( '1', 'frameless', 'безрамки' ),
	'img_page'            => array( '1', 'page=$1', 'page $1', 'страница=$1', 'страница $1' ),
	'img_upright'         => array( '1', 'upright', 'upright=$1', 'upright $1', 'сверхусправа', 'сверхусправа=$1', 'сверхусправа $1' ),
	'img_border'          => array( '1', 'border', 'граница' ),
	'img_baseline'        => array( '1', 'baseline', 'основание' ),
	'img_sub'             => array( '1', 'sub', 'под' ),
	'img_super'           => array( '1', 'super', 'sup', 'над' ),
	'img_top'             => array( '1', 'top', 'сверху' ),
	'img_text_top'        => array( '1', 'text-top', 'текст-сверху' ),
	'img_middle'          => array( '1', 'middle', 'посередине' ),
	'img_bottom'          => array( '1', 'bottom', 'снизу' ),
	'img_text_bottom'     => array( '1', 'text-bottom', 'текст-снизу' ),
	'int'                 => array( '0', 'INT:', 'ВНУТР:' ),
	'sitename'            => array( '1', 'SITENAME', 'НАЗВАНИЕ_САЙТА' ),
	'ns'                  => array( '0', 'NS:', 'ПИ:' ),
	'localurl'            => array( '0', 'LOCALURL:', 'ЛОКАЛЬНЫЙ_АДРЕС:' ),
	'localurle'           => array( '0', 'LOCALURLE:', 'ЛОКАЛЬНЫЙ_АДРЕС_2:' ),
	'server'              => array( '0', 'SERVER', 'СЕРВЕР' ),
	'servername'          => array( '0', 'SERVERNAME', 'НАЗВАНИЕ_СЕРВЕРА' ),
	'scriptpath'          => array( '0', 'SCRIPTPATH', 'ПУТЬ_К_СКРИПТУ' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'ПАДЕЖ:' ),
	'notitleconvert'      => array( '0', '__NOTITLECONVERT__', '__NOTC__', '__БЕЗ_ПРЕОБРАЗОВАНИЯ_ЗАГОЛОВКА__' ),
	'nocontentconvert'    => array( '0', '__NOCONTENTCONVERT__', '__NOCC__', '__БЕЗ_ПРЕОБРАЗОВАНИЯ_ТЕКСТА__' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'ТЕКУЩАЯ_НЕДЕЛЯ' ),
	'currentdow'          => array( '1', 'CURRENTDOW', 'ТЕКУЩИЙ_ДЕНЬ_НЕДЕЛИ' ),
	'localweek'           => array( '1', 'LOCALWEEK', 'МЕСТНАЯ_НЕДЕЛЯ' ),
	'localdow'            => array( '1', 'LOCALDOW', 'МЕСТНЫЙ_ДЕНЬ_НЕДЕЛИ' ),
	'revisionid'          => array( '1', 'REVISIONID', 'ИД_ВЕРСИИ' ),
	'revisionday'         => array( '1', 'REVISIONDAY', 'ДЕНЬ_ВЕРСИИ' ),
	'revisionday2'        => array( '1', 'REVISIONDAY2', 'ДЕНЬ_ВЕРСИИ_2' ),
	'revisionmonth'       => array( '1', 'REVISIONMONTH', 'МЕСЯЦ_ВЕРСИИ' ),
	'revisionyear'        => array( '1', 'REVISIONYEAR', 'ГОД_ВЕРСИИ' ),
	'revisiontimestamp'   => array( '1', 'REVISIONTIMESTAMP', 'ОТМЕТКА_ВРЕМЕНИ_ВЕРСИИ' ),
	'plural'              => array( '0', 'PLURAL:', 'МНОЖЕСТВЕННОЕ_ЧИСЛО:' ),
	'fullurl'             => array( '0', 'FULLURL:', 'ПОЛНЫЙ_АДРЕС:' ),
	'fullurle'            => array( '0', 'FULLURLE:', 'ПОЛНЫЙ_АДРЕС_2:' ),
	'lcfirst'             => array( '0', 'LCFIRST:', 'ПЕРВАЯ_БУКВА_МАЛЕНЬКАЯ:' ),
	'ucfirst'             => array( '0', 'UCFIRST:', 'ПЕРВАЯ_БУКВА_БОЛЬШАЯ:' ),
	'lc'                  => array( '0', 'LC:', 'МАЛЕНЬКИМИ_БУКВАМИ:' ),
	'uc'                  => array( '0', 'UC:', 'БОЛЬШИМИ_БУКВАМИ:' ),
	'raw'                 => array( '0', 'RAW:', 'НЕОБРАБ:' ),
	'displaytitle'        => array( '1', 'DISPLAYTITLE', 'ПОКАЗАТЬ_ЗАГОЛОВОК' ),
	'rawsuffix'           => array( '1', 'R', 'Н' ),
	'newsectionlink'      => array( '1', '__NEWSECTIONLINK__', '__ССЫЛКА_НА_НОВЫЙ_РАЗДЕЛ__' ),
	'currentversion'      => array( '1', 'CURRENTVERSION', 'ТЕКУЩАЯ_ВЕРСИЯ' ),
	'urlencode'           => array( '0', 'URLENCODE:', 'ЗАКОДИРОВАННЫЙ_АДРЕС:' ),
	'anchorencode'        => array( '0', 'ANCHORENCODE', 'КОДИРОВАТЬ_МЕТКУ' ),
	'currenttimestamp'    => array( '1', 'CURRENTTIMESTAMP', 'ОТМЕТКА_ТЕКУЩЕГО_ВРЕМЕНИ' ),
	'localtimestamp'      => array( '1', 'LOCALTIMESTAMP', 'ОТМЕТКА_МЕСТНОГО_ВРЕМЕНИ' ),
	'directionmark'       => array( '1', 'DIRECTIONMARK', 'DIRMARK', 'НАПРАВЛЕНИЕ_ПИСЬМА' ),
	'language'            => array( '0', '#LANGUAGE:', '#ЯЗЫК:' ),
	'contentlanguage'     => array( '1', 'CONTENTLANGUAGE', 'CONTENTLANG', 'ЯЗЫК_СОДЕРЖАНИЯ' ),
	'pagesinnamespace'    => array( '1', 'PAGESINNAMESPACE:', 'PAGESINNS:', 'СТРАНИЦ_В_ПРОСТРАНСТВЕ_ИМЁН:' ),
	'numberofadmins'      => array( '1', 'NUMBEROFADMINS', 'КОЛИЧЕСТВО_АДМИНИСТРАТОРОВ' ),
	'formatnum'           => array( '0', 'FORMATNUM', 'ФОРМАТИРОВАТЬ_ЧИСЛО' ),
	'padleft'             => array( '0', 'PADLEFT', 'ЗАПОЛНИТЬ_СЛЕВА' ),
	'padright'            => array( '0', 'PADRIGHT', 'ЗАПОЛНИТЬ_СПРАВА' ),
	'special'             => array( '0', 'special', 'служебная' ),
	'defaultsort'         => array( '1', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:', 'СОРТИРОВКА_ПО_УМОЛЧАНИЮ', 'КЛЮЧ_СОРТИРОВКИ' ),
	'filepath'            => array( '0', 'FILEPATH:', 'ПУТЬ_К_ФАЙЛУ:' ),
	'tag'                 => array( '0', 'tag', 'тег' ),
	'hiddencat'           => array( '1', '__HIDDENCAT__', '__СКРЫТАЯ_КАТЕГОРИЯ__' ),
);

$imageFiles = array(
	'button-bold'   => 'cyrl/button_bold.png',
	'button-italic' => 'cyrl/button_italic.png',
	'button-link'   => 'cyrl/button_link.png',
);

$linkTrail = '/^([a-zабвгдеёжзийклмнопрстуфхцчшщъыьэюя]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Подчёркивать ссылки:',
'tog-highlightbroken'         => 'Показывать несуществующие ссылки <a href="" class="new">вот так</a> (иначе вот так<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Выравнивать текст по ширине страницы',
'tog-hideminor'               => 'Скрывать малозначимые правки в списке свежих изменений',
'tog-extendwatchlist'         => 'Расширенный список наблюдения, включающий все изменения',
'tog-usenewrc'                => 'Улучшенный список свежих изменений (JavaScript)',
'tog-numberheadings'          => 'Автоматически нумеровать заголовки',
'tog-showtoolbar'             => 'Показывать верхнюю панель инструментов при редактировании (JavaScript)',
'tog-editondblclick'          => 'Править страницы по двойному щелчку (JavaScript)',
'tog-editsection'             => 'Показывать ссылку «править» для каждой секции',
'tog-editsectiononrightclick' => 'Править секции при правом щелчке мышью на заголовке (JavaScript)',
'tog-showtoc'                 => 'Показывать оглавление (для страниц более чем с 3 заголовками)',
'tog-rememberpassword'        => 'Помнить мою учётную запись на этом компьютере',
'tog-editwidth'               => 'Поле редактирования во всю ширину окна браузера',
'tog-watchcreations'          => 'Добавлять созданные мной страницы в список наблюдения',
'tog-watchdefault'            => 'Добавлять изменённые мной страницы в список наблюдения',
'tog-watchmoves'              => 'Добавлять переименованные мной страницы в список наблюдения',
'tog-watchdeletion'           => 'Добавлять удалённые мной страницы в список наблюдения',
'tog-minordefault'            => 'По умолчанию помечать изменения как малозначимые',
'tog-previewontop'            => 'Показывать предпросмотр статьи до окна редактирования',
'tog-previewonfirst'          => 'Предварительный просмотр по первому изменению',
'tog-nocache'                 => 'Запретить кеширование страниц',
'tog-enotifwatchlistpages'    => 'Уведомлять по эл. почте об изменениях страниц из списка наблюдения',
'tog-enotifusertalkpages'     => 'Уведомлять по эл. почте об изменении персональной страницы обсуждения',
'tog-enotifminoredits'        => 'Уведомлять по эл. почте даже при малозначительных изменениях',
'tog-enotifrevealaddr'        => 'Показывать мой почтовый адрес в сообщениях оповещения',
'tog-shownumberswatching'     => 'Показывать число участников, включивших страницу в свой список наблюдения',
'tog-fancysig'                => 'Собственная вики-разметка подписи',
'tog-externaleditor'          => 'Использовать по умолчанию внешний редактор',
'tog-externaldiff'            => 'Использовать по умолчанию внешнюю программу сравнения версий',
'tog-showjumplinks'           => 'Включить вспомогательные ссылки «перейти к»',
'tog-uselivepreview'          => 'Использовать быстрый предварительный просмотр (JavaScript, экспериментально)',
'tog-forceeditsummary'        => 'Предупреждать, когда не указано краткое описание изменений',
'tog-watchlisthideown'        => 'Скрывать мои правки из списка наблюдения',
'tog-watchlisthidebots'       => 'Скрывать правки ботов из списка наблюдения',
'tog-watchlisthideminor'      => 'Скрывать малые правки из списка наблюдения',
'tog-nolangconversion'        => 'Отключить преобразование систем письма',
'tog-ccmeonemails'            => 'Отправлять мне копии писем, которые я посылаю другим участникам.',
'tog-diffonly'                => 'Не показывать содержание страницы под сравнением двух версий',
'tog-showhiddencats'          => 'Показывать скрытые категории',

'underline-always'  => 'Всегда',
'underline-never'   => 'Никогда',
'underline-default' => 'Использовать настройки браузера',

'skinpreview' => '(Предпросмотр)',

# Dates
'sunday'        => 'воскресенье',
'monday'        => 'понедельник',
'tuesday'       => 'вторник',
'wednesday'     => 'среда',
'thursday'      => 'четверг',
'friday'        => 'пятница',
'saturday'      => 'суббота',
'sun'           => 'Вс',
'mon'           => 'Пн',
'tue'           => 'Вт',
'wed'           => 'Ср',
'thu'           => 'Чт',
'fri'           => 'Пт',
'sat'           => 'Сб',
'january'       => 'январь',
'february'      => 'февраль',
'march'         => 'март',
'april'         => 'апрель',
'may_long'      => 'май',
'june'          => 'июнь',
'july'          => 'июль',
'august'        => 'август',
'september'     => 'сентябрь',
'october'       => 'октябрь',
'november'      => 'ноябрь',
'december'      => 'декабрь',
'january-gen'   => 'января',
'february-gen'  => 'февраля',
'march-gen'     => 'марта',
'april-gen'     => 'апреля',
'may-gen'       => 'мая',
'june-gen'      => 'июня',
'july-gen'      => 'июля',
'august-gen'    => 'августа',
'september-gen' => 'сентября',
'october-gen'   => 'октября',
'november-gen'  => 'ноября',
'december-gen'  => 'декабря',
'jan'           => 'янв',
'feb'           => 'фев',
'mar'           => 'мар',
'apr'           => 'апр',
'may'           => 'мая',
'jun'           => 'июн',
'jul'           => 'июл',
'aug'           => 'авг',
'sep'           => 'сен',
'oct'           => 'окт',
'nov'           => 'ноя',
'dec'           => 'дек',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Категория|Категории}}',
'category_header'                => 'Статьи в категории «$1»',
'subcategories'                  => 'Подкатегории',
'category-media-header'          => 'Файлы в категории «$1»',
'category-empty'                 => "''Эта категория в данный момент пуста.''",
'hidden-categories'              => '{{PLURAL:$1|Скрытая категория|Скрытые категории}}',
'hidden-category-category'       => 'Скрытые категории', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Данная категория содержит только следующую подкатегорию.|{{PLURAL:$1|Показана $1 подкатегория|Показано $1 подкатегории|Показано $1 подкатегорий}} из $2.}}',
'category-subcat-count-limited'  => 'В этой категории {{PLURAL:$1|$1 подкатегория|$1 подкатегории|$1 подкатегорий}}.',
'category-article-count'         => '{{PLURAL:$2|Эта категория содержит только одну страницу.|{{PLURAL:$1|Показана $1 страница|Показано $1 страницы|Показано $1 страниц}} этой категории из $2.}}',
'category-article-count-limited' => 'В этой категории {{PLURAL:$1|$1 страница|$1 страницы|$1 страниц}}.',
'category-file-count'            => '{{PLURAL:$2|Эта категория содержит только один файл.|{{PLURAL:$1|Показан $1 файл|Показано $1 файла|Показано $1 файлов}} этой категории  из $2.}}',
'category-file-count-limited'    => 'В этой категории {{PLURAL:$1|$1 файл|$1 файла|$1 файлов}}.',
'listingcontinuesabbrev'         => '(продолжение)',

'mainpagetext'      => '<big>Вики-движок «MediaWiki» успешно установлен.</big>',
'mainpagedocfooter' => 'Информацию по работе с этой вики можно найти в [http://meta.wikimedia.org/wiki/%D0%9F%D0%BE%D0%BC%D0%BE%D1%89%D1%8C:%D0%A1%D0%BE%D0%B4%D0%B5%D1%80%D0%B6%D0%B0%D0%BD%D0%B8%D0%B5 руководстве пользователя].

== Некоторые полезные ресурсы ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Список возможных настроек];
* [http://www.mediawiki.org/wiki/Manual:FAQ Часто задаваемые вопросы и ответы по MediaWiki];
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Рассылка уведомлений о выходе новых версий MediaWiki].',

'about'          => 'Описание',
'article'        => 'Статья',
'newwindow'      => '(в новом окне)',
'cancel'         => 'Отменить',
'qbfind'         => 'Поиск',
'qbbrowse'       => 'Просмотреть',
'qbedit'         => 'Править',
'qbpageoptions'  => 'Настройки страницы',
'qbpageinfo'     => 'Сведения о странице',
'qbmyoptions'    => 'Ваши настройки',
'qbspecialpages' => 'Специальные страницы',
'moredotdotdot'  => 'Далее…',
'mypage'         => 'Личная страница',
'mytalk'         => 'Моя страница обсуждения',
'anontalk'       => 'Обсуждение для этого IP-адреса',
'navigation'     => 'Навигация',
'and'            => 'и',

# Metadata in edit box
'metadata_help' => 'Метаданные:',

'errorpagetitle'    => 'Ошибка',
'returnto'          => 'Возврат к странице $1.',
'tagline'           => 'Материал из {{grammar:genitive|{{SITENAME}}}}',
'help'              => 'Справка',
'search'            => 'Поиск',
'searchbutton'      => 'Найти',
'go'                => 'Перейти',
'searcharticle'     => 'Перейти',
'history'           => 'История',
'history_short'     => 'История',
'updatedmarker'     => 'обновлено после моего последнего посещения',
'info_short'        => 'Информация',
'printableversion'  => 'Версия для печати',
'permalink'         => 'Постоянная ссылка',
'print'             => 'Печать',
'edit'              => 'Править',
'create'            => 'Создать',
'editthispage'      => 'Править эту страницу',
'create-this-page'  => 'Создать эту страницу',
'delete'            => 'Удалить',
'deletethispage'    => 'Удалить эту страницу',
'undelete_short'    => 'Восстановить $1 {{PLURAL:$1|правку|правки|правок}}',
'protect'           => 'Защитить',
'protect_change'    => 'изменить',
'protectthispage'   => 'Защитить эту страницу',
'unprotect'         => 'Снять защиту',
'unprotectthispage' => 'Снять защиту',
'newpage'           => 'Новая страница',
'talkpage'          => 'Обсудить эту страницу',
'talkpagelinktext'  => 'Обсуждение',
'specialpage'       => 'Служебная страница',
'personaltools'     => 'Личные инструменты',
'postcomment'       => 'Прокомментировать',
'articlepage'       => 'Просмотреть статью',
'talk'              => 'Обсуждение',
'views'             => 'Просмотры',
'toolbox'           => 'Инструменты',
'userpage'          => 'Просмотреть страницу участника',
'projectpage'       => 'Просмотреть страницу проекта',
'imagepage'         => 'Просмотреть страницу изображения',
'mediawikipage'     => 'Показать страницу сообщения',
'templatepage'      => 'Просмотреть страницу шаблона',
'viewhelppage'      => 'Получить справку',
'categorypage'      => 'Просмотреть страницу категории',
'viewtalkpage'      => 'Просмотреть обсуждение',
'otherlanguages'    => 'На других языках',
'redirectedfrom'    => '(Перенаправлено с $1)',
'redirectpagesub'   => 'Страница-перенаправление',
'lastmodifiedat'    => 'Последнее изменение этой страницы: $2, $1.', # $1 date, $2 time
'viewcount'         => 'К этой странице обращались $1 {{PLURAL:$1|раз|раза|раз}}.',
'protectedpage'     => 'Защищённая страница',
'jumpto'            => 'Перейти к:',
'jumptonavigation'  => 'навигация',
'jumptosearch'      => 'поиск',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Описание {{grammar:genitive|{{SITENAME}}}}',
'aboutpage'            => 'Project:Описание',
'bugreports'           => 'Отчёт об ошибке',
'bugreportspage'       => 'Project:Отчёты об ошибке',
'copyright'            => 'Содержимое доступно в соответствии с $1.',
'copyrightpagename'    => 'Авторские права проекта {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Авторское право',
'currentevents'        => 'Текущие события',
'currentevents-url'    => 'Project:Текущие события',
'disclaimers'          => 'Отказ от ответственности',
'disclaimerpage'       => 'Project:Отказ от ответственности',
'edithelp'             => 'Справка по редактированию',
'edithelppage'         => 'Help:Справка по редактированию',
'faq'                  => 'ЧаВО',
'faqpage'              => 'Project:ЧаВО',
'helppage'             => 'Help:Справка',
'mainpage'             => 'Заглавная страница',
'mainpage-description' => 'Заглавная страница',
'policy-url'           => 'Project:Правила',
'portal'               => 'Сообщество',
'portal-url'           => 'Project:Портал сообщества',
'privacy'              => 'Политика конфиденциальности',
'privacypage'          => 'Project:Политика конфиденциальности',

'badaccess'        => 'Ошибка доступа',
'badaccess-group0' => 'Вы не можете выполнять запрошенное действие.',
'badaccess-group1' => 'Запрошенное действие могут выполнять только участники из группы $1.',
'badaccess-group2' => 'Запрошенное действие могут выполнять только участники из групп $1.',
'badaccess-groups' => 'Запрошенное действие могут выполнять только участники из групп $1.',

'versionrequired'     => 'Требуется MediaWiki версии $1',
'versionrequiredtext' => 'Для работы с этой страницей требуется MediaWiki версии $1. См. [[Special:Version|информацию о версиях используемого ПО]].',

'ok'                      => 'OK',
'pagetitle'               => '$1 — {{SITENAME}}',
'retrievedfrom'           => 'Источник — «$1»',
'youhavenewmessages'      => 'Вы получили $1 ($2).',
'newmessageslink'         => 'новые сообщения',
'newmessagesdifflink'     => 'последнее изменение',
'youhavenewmessagesmulti' => 'Вы получили новые сообщения на $1',
'editsection'             => 'править',
'editold'                 => 'править',
'viewsourceold'           => 'просмотреть исходный код',
'editsectionhint'         => 'Править секцию: $1',
'toc'                     => 'Содержание',
'showtoc'                 => 'показать',
'hidetoc'                 => 'убрать',
'thisisdeleted'           => 'Просмотреть или восстановить $1?',
'viewdeleted'             => 'Просмотреть $1?',
'restorelink'             => '{{PLURAL:$1|$1 удалённую правку|$1 удалённые правки|$1 удалённых правок}}',
'feedlinks'               => 'В виде:',
'feed-invalid'            => 'Неправильный тип канала для подписки.',
'feed-unavailable'        => 'Ленты синдикации недоступны',
'site-rss-feed'           => '$1 — RSS-лента',
'site-atom-feed'          => '$1 — Atom-лента',
'page-rss-feed'           => '«$1» — RSS-лента',
'page-atom-feed'          => '«$1» — Atom-лента',
'red-link-title'          => '$1 (ещё не написано)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Статья',
'nstab-user'      => 'Участник',
'nstab-media'     => 'Мультимедиа',
'nstab-special'   => 'Служебная страница',
'nstab-project'   => 'О проекте',
'nstab-image'     => 'Файл',
'nstab-mediawiki' => 'Сообщение',
'nstab-template'  => 'Шаблон',
'nstab-help'      => 'Справка',
'nstab-category'  => 'Категория',

# Main script and global functions
'nosuchaction'      => 'Такого действия нет',
'nosuchactiontext'  => 'Действие, указанное в URL, не распознаётся программным обеспечением вики',
'nosuchspecialpage' => 'Такой специальной страницы нет',
'nospecialpagetext' => "<big>'''Запрошенной вами служебной страницы не существует.'''</big>

См. [[Special:SpecialPages|список служебных страниц]].",

# General errors
'error'                => 'Ошибка',
'databaseerror'        => 'Ошибка базы данных',
'dberrortext'          => 'Обнаружена ошибка синтаксиса запроса к базе данных.
Последний запрос к базе данных:
<blockquote><tt>$1</tt></blockquote>
произошёл из функции <tt>«$2»</tt>.
MySQL возвратил ошибку <tt>«$3: $4»</tt>.',
'dberrortextcl'        => 'Обнаружена ошибка синтаксиса запроса к базе данных.
Последний запрос к базе данных:
«$1»
произошёл из функции «$2».
MySQL возвратил ошибку «$3: $4».',
'noconnect'            => 'Извините, технические проблемы в работе вики-движка, невозможно связаться с сервером базы данных.<br />
$1',
'nodb'                 => 'Невозможно выбрать базу данных $1',
'cachederror'          => 'Ниже представлена кешированная копия запрошенной страницы; возможно, она устарела.',
'laggedslavemode'      => 'Внимание: страница может не содержать последних обновлений.',
'readonly'             => 'Запись в базу данных заблокирована',
'enterlockreason'      => 'Укажите причину и намеченный срок блокировки.',
'readonlytext'         => 'Добавление новых статей и другие изменения базы данных сейчас заблокированы: вероятно, в связи с плановым обслуживанием.
Заблокировавший оператор оставил следующее разъяснение:
$1',
'missing-article'      => 'В базе данных не найдено запрашиваемого текста страницы, которая должна была быть найдена, «$1» $2.

Подобная ситуация обычно возникает при попытке перехода по устаревшей ссылке на историю изменения страницы, которая была удалена.

Если дело не в этом, то скорее всего, вы обнаружили ошибку в программном обеспечении.
Пожалуйста, сообщите об этом [[Special:ListUsers/sysop|администратору]], указав URL.',
'missingarticle-rev'   => '(версия № $1)',
'missingarticle-diff'  => '(разность: $1, $2)',
'readonly_lag'         => 'База данных автоматически заблокирована от изменений на время, пока вторичный сервер БД не синхронизируется с первичным.',
'internalerror'        => 'Внутренняя ошибка',
'internalerror_info'   => 'Внутренняя ошибка: $1',
'filecopyerror'        => 'Невозможно скопировать файл «$1» в «$2».',
'filerenameerror'      => 'Невозможно переименовать файл «$1» в «$2».',
'filedeleteerror'      => 'Невозможно удалить файл «$1».',
'directorycreateerror' => 'Невозможно создать директорию «$1».',
'filenotfound'         => 'Невозможно найти файл «$1».',
'fileexistserror'      => 'Невозможно записать в файл «$1»: файл существует.',
'unexpected'           => 'Неподходящее значение: «$1»=«$2».',
'formerror'            => 'Ошибка: невозможно передать данные формы',
'badarticleerror'      => 'Это действие не может быть выполнено на данной странице.',
'cannotdelete'         => 'Невозможно удалить указанную страницу или файл. Возможно, его уже удалил кто-то другой.',
'badtitle'             => 'Недопустимое название',
'badtitletext'         => 'Запрашиваемое название статьи неправильно, пусто, либо неправильно указано межъязыковое или интервики название. Возможно, в названии используются недопустимые символы.',
'perfdisabled'         => 'К сожалению, эта возможность временно недоступна в связи с загруженностью сервера.',
'perfcached'           => 'Следующие данные взяты из кеша и могут не учитывать последних изменений.',
'perfcachedts'         => 'Следующие данные взяты из кеша, последний раз он обновлялся в $1.',
'querypage-no-updates' => 'Обновление этой страницы сейчас отключено.
Представленные здесь данные не будут актуализироваться.',
'wrong_wfQuery_params' => 'Недопустимые параметры для функции wfQuery()<br />
Функция: $1<br />
Запрос: $2',
'viewsource'           => 'Просмотр',
'viewsourcefor'        => 'Страница «$1»',
'actionthrottled'      => 'Ограничение по скорости',
'actionthrottledtext'  => 'В качестве меры борьбы со спамом, установлено ограничение на многократное применение данного действия в течение короткого промежутка времени. Пожалуйста, повторите попытку через несколько минут.',
'protectedpagetext'    => 'Эта страница закрыта для редактирования.',
'viewsourcetext'       => 'Вы можете просмотреть и скопировать исходный текст этой страницы:',
'protectedinterface'   => 'Эта страница содержит интерфейсное сообщение программного обеспечения. Во избежание вандализма её изменение запрещено.',
'editinginterface'     => "'''Внимание.''' Вы редактируете страницу, содержащую текст интерфейса программного обеспечения.
Её изменение повлияет на внешний вид интерфейса для других пользователей.
Для переводов лучше использовать [http://translatewiki.net/wiki/Main_Page?setlang=ru Betawiki], проект по локализации MediaWiki.",
'sqlhidden'            => '(SQL запрос скрыт)',
'cascadeprotected'     => 'Страница защищена от изменений, поскольку она включена в {{PLURAL:$1|следующую страницу, для которой|следующие страницы, для которых}} включена каскадная защита:
$2',
'namespaceprotected'   => 'У вас нет разрешения редактировать страницы в пространстве имён «$1».',
'customcssjsprotected' => 'У вас нет разрешения редактировать эту страницу, так как она содержит личные настройки другого участника.',
'ns-specialprotected'  => 'Страницы пространства имён «{{ns:special}}» не могут правиться.',
'titleprotected'       => "Создание страницы с таким заголовком было запрещено участником [[Участник:$1|$1]].
Указана следующая причина: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Ошибка настройки. Неизвестный сканер вирусов: <i>$1</i>',
'virus-scanfailed'     => 'ошибка сканирования (код $1)',
'virus-unknownscanner' => 'неизвестный антивирус:',

# Login and logout pages
'logouttitle'                => 'Стать инкогнито',
'logouttext'                 => 'Вы работаете в том же режиме, который был до вашего представления системе. Вы идентифицируетесь не по имени, а по IP-адресу.
Вы можете продолжить участие в проекте анонимно или начать новый сеанс как тот же самый или другой пользователь. Некоторые страницы могут отображаться, как будто вы ещё представлены системе под именем, для борьбы с этим явлением обновите кеш браузера.',
'welcomecreation'            => '== Добро пожаловать, $1! ==
Ваша учётная запись создана.
Не забудьте провести [[Special:Preferences|персональную настройку]] сайта.',
'loginpagetitle'             => 'Представиться системе',
'yourname'                   => 'Имя участника:',
'yourpassword'               => 'Пароль:',
'yourpasswordagain'          => 'Повторный набор пароля:',
'remembermypassword'         => 'Помнить мою учётную запись на этом компьютере',
'yourdomainname'             => 'Ваш домен:',
'externaldberror'            => 'Произошла ошибка при аутентификации с помощью внешней базы данных, или у вас недостаточно прав для внесения изменений в свою внешнюю учётную запись.',
'loginproblem'               => '<span style="color:red">Участник не опознан.</span>',
'login'                      => 'Представиться системе',
'nav-login-createaccount'    => 'Представиться / зарегистрироваться',
'loginprompt'                => 'Вы должны разрешить «cookies», чтобы представиться {{grammar:genitive|{{SITENAME}}}}.',
'userlogin'                  => 'Представиться или зарегистрироваться',
'logout'                     => 'Завершение сеанса',
'userlogout'                 => 'Завершение сеанса',
'notloggedin'                => 'Вы не представились системе',
'nologin'                    => 'Нет учётной записи? $1.',
'nologinlink'                => 'Создайте учётную запись',
'createaccount'              => 'Зарегистрировать нового участника',
'gotaccount'                 => 'Вы уже зарегистрированы? $1.',
'gotaccountlink'             => 'Представьтесь',
'createaccountmail'          => 'по эл. почте',
'badretype'                  => 'Введённые вами пароли не совпадают.',
'userexists'                 => 'Введённое имя участника уже используется.
Пожалуйста, выберите другое имя.',
'youremail'                  => 'Электронная почта:',
'username'                   => 'Регистрационное имя:',
'uid'                        => 'Идентификатор пользователя:',
'prefs-memberingroups'       => 'Член {{PLURAL:$1|группы|групп}}:',
'yourrealname'               => 'Ваше настоящее имя:',
'yourlanguage'               => 'Язык интерфейса:',
'yourvariant'                => 'Вариант языка',
'yournick'                   => 'Ваш псевдоним (для подписей):',
'badsig'                     => 'Неверная подпись. Проверьте корректность HTML-тегов.',
'badsiglength'               => 'Слишком длинная подпись.
Подпись не должна превышать $1 {{PLURAL:$1|символа|символов|символов}}.',
'email'                      => 'Эл. почта',
'prefs-help-realname'        => 'Настоящее имя (необязательное поле): если вы укажите его, то оно будет использовано для того чтобы показать кем был внесена правка страницы.',
'loginerror'                 => 'Ошибка опознавания участника',
'prefs-help-email'           => 'Электронная почта (необязательное поле). Если адрес электронной почты указан, то вы сможете запросить отправить вам новый пароль, если вдруг забудете действующий.
Также это позволит другим участникам связаться с вами через вашу страницу в вики без необходимости раскрытия адреса вашей электронной почты.',
'prefs-help-email-required'  => 'Необходимо указать адрес электронной почты.',
'nocookiesnew'               => 'Участник зарегистрирован, но не представлен. {{SITENAME}} использует «cookies» для представления участников. У вас «cookies» запрещены. Пожалуйста, разрешите их, а затем представьтесь с вашим новым именем участника и паролем.',
'nocookieslogin'             => '{{SITENAME}} использует «cookies» для представления участников. Вы их отключили. Пожалуйста, включите их и попробуйте снова.',
'noname'                     => 'Вы не указали допустимого имени участника.',
'loginsuccesstitle'          => 'Опознание прошло успешно',
'loginsuccess'               => 'Теперь вы работаете под именем $1.',
'nosuchuser'                 => 'Участника с именем $1 не существует.
Проверьте правильность написания имени или воспользуйтесь формой ниже, чтобы [[Special:Userlogin/signup|зарегистрировать нового участника]].',
'nosuchusershort'            => 'Не существует участника с именем <nowiki>$1</nowiki>. Проверьте написание имени.',
'nouserspecified'            => 'Вы должны указать имя участника.',
'wrongpassword'              => 'Введённый вами пароль неверен. Попробуйте ещё раз.',
'wrongpasswordempty'         => 'Пожалуйста, введите непустой пароль.',
'passwordtooshort'           => 'Введённый пароль недействителен или слишком короткий.
Пароль должен состоять не менее чем из $1 {{PLURAL:$1|символа|символов|символов}} и отличаться от имени участника.',
'mailmypassword'             => 'Выслать новый пароль',
'passwordremindertitle'      => 'Напоминание пароля участника {{grammar:genitive|{{SITENAME}}}}',
'passwordremindertext'       => 'Кто-то (вероятно, вы, с IP-адреса $1) запросил создать
новый пароль для {{grammar:genitive|{{SITENAME}}}} ($4). Для участника $2
создан временный пароль: $3. Если это был ваш запрос,
вам следует представиться системе и выбрать новый пароль.

Если вы не посылали запроса на смену пароля, или если вы уже вспомнили свой пароль,
и не желаете его менять, вы можете проигнорировать данное сообщение и
продолжить использовать свой старый пароль.',
'noemail'                    => 'Для участника с именем $1 электронный адрес указан не был.',
'passwordsent'               => 'Новый пароль был выслан на адрес электронной почты, указанный для участника $1.

Пожалуйста, представьтесь системе заново после получения пароля.',
'blocked-mailpassword'       => 'Редактирование с вашего IP-адреса запрещено, заблокирована и функция восстановления пароля.',
'eauthentsent'               => 'На указанный адрес электронной почты отправлено письмо с запросом на подтверждение изменения адреса. В письме также описаны действия, которые нужно выполнить для подтверждения того, что этот адрес электронной почты действительно принадлежит вам.',
'throttled-mailpassword'     => 'Функция напоминания пароля уже использовалась в течение {{PLURAL:$1|последнего $1 часа|последних $1 часов|последних $1 часов}} .
Для предотвращения злоупотреблений, разрешено запрашивать не более одного напоминания за $1 {{PLURAL:$1|час|часа|часов}}.',
'mailerror'                  => 'Ошибка при отправке почты: $1',
'acct_creation_throttle_hit' => 'К сожалению, вы уже создали $1 учётных записей. Вы не можете создать больше ни одной.',
'emailauthenticated'         => 'Ваш почтовый адрес был подтверждён $1.',
'emailnotauthenticated'      => 'Ваш адрес электронной почты ещё не был подтверждён, функции вики-движка по работе с эл. почтой отключены.',
'noemailprefs'               => 'Адрес электронной почты не был указан, функции вики-движка по работе с эл. почтой отключены.',
'emailconfirmlink'           => 'Подтвердить ваш адрес электронной почты',
'invalidemailaddress'        => 'Адрес электронной почты не может быть принят, так как он не соответствует формату.
Пожалуйста, введите корректный адрес или оставьте поле пустым.',
'accountcreated'             => 'Учётная запись создана',
'accountcreatedtext'         => 'Создана учётная запись участника $1.',
'createaccount-title'        => '{{SITENAME}}: создание учётной записи',
'createaccount-text'         => 'Кто-то создал учётную запись «$2» на сервере проекта {{SITENAME}} ($4) с паролем «$3», указав ваш адрес электронной почты. Вам следует зайти и изменить пароль.

Проигнорируйте данное сообщение, если учётная запись была создана по ошибке.',
'loginlanguagelabel'         => 'Язык: $1',

# Password reset dialog
'resetpass'               => 'Сброс пароля от учётной записи',
'resetpass_announce'      => 'Вы представились с помощью временного пароля, полученного по электронной почте. Для завершения входа в систему, вы должны установить новый пароль.',
'resetpass_text'          => '<!-- Добавьте сюда текст -->',
'resetpass_header'        => 'Сброс пароля',
'resetpass_submit'        => 'Установить пароль и представиться',
'resetpass_success'       => 'Ваш пароль был успешно изменён! Выполняется вход в систему…',
'resetpass_bad_temporary' => 'Недействительный временный пароль. Возможно, вы уже изменили ваш пароль, или попробуйте запросить временный пароль снова.',
'resetpass_forbidden'     => 'Пароль не может быть изменён',
'resetpass_missing'       => 'Форма не содержит данных.',

# Edit page toolbar
'bold_sample'     => 'Полужирное начертание',
'bold_tip'        => 'Полужирное начертание',
'italic_sample'   => 'Курсивное начертание',
'italic_tip'      => 'Курсивное начертание',
'link_sample'     => 'Заголовок ссылки',
'link_tip'        => 'Внутренняя ссылка',
'extlink_sample'  => 'http://www.example.com заголовок ссылки',
'extlink_tip'     => 'Внешняя ссылка (помните о префиксе http:// )',
'headline_sample' => 'Текст заголовка',
'headline_tip'    => 'Заголовок 2-го уровня',
'math_sample'     => 'Вставьте сюда формулу',
'math_tip'        => 'Математическая формула (формат LaTeX)',
'nowiki_sample'   => 'Вставляйте сюда неотформатированный текст.',
'nowiki_tip'      => 'Игнорировать вики-форматирование',
'image_tip'       => 'Встроенное изображение',
'media_tip'       => 'Ссылка на медиа-файл',
'sig_tip'         => 'Ваша подпись и момент времени',
'hr_tip'          => 'Горизонтальная линия (не используйте часто)',

# Edit pages
'summary'                          => 'Описание изменений',
'subject'                          => 'Тема/заголовок',
'minoredit'                        => 'Малое изменение',
'watchthis'                        => 'Включить эту страницу в список наблюдения',
'savearticle'                      => 'Записать страницу',
'preview'                          => 'Предпросмотр',
'showpreview'                      => 'Предварительный просмотр',
'showlivepreview'                  => 'Быстрый предпросмотр',
'showdiff'                         => 'Внесённые изменения',
'anoneditwarning'                  => "'''Внимание''': Вы не представились системе. Ваш IP-адрес будет записан в историю изменений этой страницы.",
'missingsummary'                   => "'''Напоминание.''' Вы не дали краткого описания изменений. При повторном нажатии на кнопку «Записать страницу», ваши изменения будут сохранены без комментария.",
'missingcommenttext'               => 'Пожалуйста, введите ниже ваше сообщение.',
'missingcommentheader'             => "'''Напоминание:''' Вы не указали заголовок комментария.
При повторном нажатии на кнопку сохранения, ваша правка будет записана без заголовка.",
'summary-preview'                  => 'Описание будет',
'subject-preview'                  => 'Заголовок будет',
'blockedtitle'                     => 'Участник заблокирован',
'blockedtext'                      => "<big>'''Ваша учётная запись или IP-адрес заблокированы.'''</big>

Блокировка произведена администратором $1.
Указана следующая причина: ''«$2»''.

* Начало блокировки: $8
* Окончание блокировки: $6
* Был заблокирован: $7

Вы можете отправить письмо участнику $1 или любому другому [[{{MediaWiki:Grouppage-sysop}}|администратору]], чтобы обсудить блокировку.
Обратите внимание, что вы не сможете отправить письмо администратору, если вы не зарегистрированы и не подтвердили свой адрес электронной почты в [[Special:Preferences|личных настройках]], а также если вам было запрещено отправлять письма при блокировке.
Ваш IP-адрес — $3, идентификатор блокировки — #$5.
Пожалуйста, указывайте эти данные в ваших обращениях.",
'autoblockedtext'                  => 'Ваш IP-адрес автоматически заблокирован в связи с тем, что он ранее использовался кем-то из заблокированных участников. Заблокировавший его администратор ($1) указал следующую причину блокировки:

:«$2»

* Начало блокировки: $8
* Окончание блокировки: $6
* Был заблокирован: $7

Вы можете отправить письмо участнику $1 или любому другому [[{{MediaWiki:Grouppage-sysop}}|администратору]], чтобы обсудить блокировку.

Обратите внимание, что вы не сможете отправить письмо администратору, если вы не зарегистрированы в проекте и не подтвердили свой адрес электронной почты в [[Special:Preferences|личных настройках]], а также если вам было запрещено отправлять письма при блокировке.

Ваш IP-адрес — $3, идентификатор блокировки — #$5.
Пожалуйста, указывайте эти данные в ваших обращениях.',
'blockednoreason'                  => 'причина не указана',
'blockedoriginalsource'            => 'Ниже показан текст страницы «$1».',
'blockededitsource'                => "Ниже показан текст '''ваших изменений''' страницы «$1».",
'whitelistedittitle'               => 'Для изменения требуется авторизация',
'whitelistedittext'                => 'Вы должны $1 для изменения страниц.',
'confirmedittitle'                 => 'Требуется подтверждение адреса электронной почты',
'confirmedittext'                  => 'Вы должны подтвердить ваш адрес электронной почты перед правкой страниц.
Пожалуйста, введите и подтвердите ваш адрес электронной почты на [[Special:Preferences|странице настроек]].',
'nosuchsectiontitle'               => 'Нет такой секции',
'nosuchsectiontext'                => 'Вы пытаетесь редактировать подстраницу, которой не существует. Так как не существует подстраницы с названием $1, ваши правки некуда сохранять.',
'loginreqtitle'                    => 'Требуется авторизация',
'loginreqlink'                     => 'представиться',
'loginreqpagetext'                 => 'Вы должны $1, чтобы просмотреть другие страницы.',
'accmailtitle'                     => 'Пароль выслан.',
'accmailtext'                      => 'Пароль для $1 выслан на $2.',
'newarticle'                       => '(Новая)',
'newarticletext'                   => "Вы перешли по ссылке на статью, которая пока не существует.
Чтобы создать новую страницу, наберите текст в окне, расположенном ниже
(см. [[{{MediaWiki:Helppage}}|справочную страницу]], чтобы получить больше информации).
Если вы оказались здесь по ошибке, просто нажмите кнопку '''назад''' вашего браузера.",
'anontalkpagetext'                 => "----''Эта страница обсуждения принадлежит анонимному участнику, который ещё не зарегистрировался или который не представился регистрационным именем.
Для идентификации используется цифровой IP-адрес.
Этот же адрес может соответствовать нескольким другим участникам.
Если вы анонимный участник и полагаете, что получили сообщения, адресованные не вам, пожалуйста, [[Special:UserLogin|представьтесь системе]], чтобы впредь избежать возможной путаницы с другими участниками.''",
'noarticletext'                    => "В настоящий момент текст на данной странице отсутствует. Вы можете [[Special:Search/{{PAGENAME}}|найти упоминание данного названия]] в других статьях или '''[{{fullurl:{{FULLPAGENAME}}|action=edit}} создать страницу с таким названием]'''.",
'userpage-userdoesnotexist'        => 'Учётной записи «$1» не существует. Убедитесь, что вы действительно желаете создать или изменить эту страницу.',
'clearyourcache'                   => "'''Замечание:''' Чтобы после сохранения увидеть сделанные изменения, очистите кеш своего браузера: '''Mozilla / Firefox''': ''Ctrl+Shift+R'', '''IE:''' ''Ctrl+F5'', '''Safari''': ''Cmd+Shift+R'', '''Konqueror''': ''F5'', '''Opera''': через меню ''Tools→Preferences''.",
'usercssjsyoucanpreview'           => '<strong>Подсказка.</strong> Нажмите кнопку «Предварительный просмотр», чтобы проверить ваш новый CSS- или JS-файл перед сохранением.',
'usercsspreview'                   => "'''Помните, что это только предварительный просмотр вашего CSS-файла, он ещё не сохранён!'''",
'userjspreview'                    => "'''Помните, что это только предварительный просмотр вашего javascript-файла, он ещё не сохранён!'''",
'userinvalidcssjstitle'            => "'''Внимание:''' тема оформления «$1» не найдена. Помните, что пользовательские страницы .css и .js должны иметь название, состоящее только из строчных букв, например «{{ns:user}}:Некто/monobook.css», а не «{{ns:user}}:Некто/Monobook.css».",
'updated'                          => '(Обновлена)',
'note'                             => '<strong>Примечание:</strong>',
'previewnote'                      => '<strong>Это только предварительный просмотр, текст ещё не записан!</strong>',
'previewconflict'                  => 'Этот предварительный просмотр отражает текст в верхнем окне редактирования так, как он будет выглядеть, если вы решите записать его.',
'session_fail_preview'             => '<strong>К сожалению, сервер не смог обработать вашу правку из-за потери идентификатора сессии.
Пожалуйста, попробуйте ещё раз.
Если эта ошибка повторится, попробуйте [[Special:UserLogout|завершить сеанс]] и заново представиться системе.</strong>',
'session_fail_preview_html'        => "<strong>К сожалению, сервер не смог обработать вашу правку из-за потери данных сессии.</strong>

''Так как {{SITENAME}} разрешает использовать чистый HTML, предварительный просмотр отключён в качестве меры предотвращения JavaScript-атак.''

<strong>Если это добросовестная попытка редактирования, пожалуйста, попробуйте ещё раз.
Если не получается повторная правка, попробуйте [[Special:UserLogout|завершить сеанс]] работы и заново представиться.</strong>",
'token_suffix_mismatch'            => '<strong>Ваша правка была отклонена, так как ваша программа неправильно обрабатывает знаки пунктуации
в окне редактирования. Правка была отменена для предотвращени искажения текста статьи.
Подобные проблемы могут возникать при использовании анонимизирующих веб-прокси, содержащих ошибки.</strong>',
'editing'                          => 'Редактирование: $1',
'editingsection'                   => 'Редактирование $1 (секция)',
'editingcomment'                   => 'Редактирование $1 (комментарий)',
'editconflict'                     => 'Конфликт редактирования: $1',
'explainconflict'                  => 'Пока вы редактировали эту статью, кто-то внёс в неё изменения. В верхнем окне для редактирования вы видите тот текст статьи, который будет сохранён при нажатии на кнопку «Записать страницу». В нижнем окне для редактирования находится ваш вариант. Чтобы сохранить ваши изменения, перенесите их из нижнего окна для редактирования в верхнее.<br />',
'yourtext'                         => 'Ваш текст',
'storedversion'                    => 'Сохранённая версия',
'nonunicodebrowser'                => '<strong>ПРЕДУПРЕЖДЕНИЕ: Ваш браузер не поддерживает кодировку Юникод. При редактировании статей все не-ASCII символы будут заменены на свои шестнадцатеричные коды.</strong>',
'editingold'                       => '<strong>ПРЕДУПРЕЖДЕНИЕ: Вы редактируете устаревшую версию данной страницы. После сохранения страницы будут потеряны изменения, сделанные в последующих версиях.</strong>',
'yourdiff'                         => 'Различия',
'copyrightwarning'                 => 'Обратите внимание, что все добавления и изменения текста статьи рассматриваются, как выпущенные на условиях лицензии $2 (см. $1).
Если вы не хотите, чтобы ваши тексты свободно распространялись и редактировались любым желающим, не помещайте их сюда.<br />
Вы также подтверждаете, что являетесь автором вносимых дополнений, или скопировали их из
источника, допускающего свободное распространение и изменение своего содержимого.<br />
<strong>НЕ РАЗМЕЩАЙТЕ БЕЗ РАЗРЕШЕНИЯ МАТЕРИАЛЫ, ОХРАНЯЕМЫЕ АВТОРСКИМ ПРАВОМ!</strong>',
'copyrightwarning2'                => 'Пожалуйста, обратите внимание, что все ваши добавления могут быть отредактированы или удалены другими участниками.
Если вы не хотите, чтобы кто-либо изменял ваши тексты, не помещайте их сюда.<br />
Вы также подтверждаете, что являетесь автором вносимых дополнений, или скопировали их из источника, допускающего свободное распространение и изменение своего содержимого (см. $1).
<strong>НЕ РАЗМЕЩАЙТЕ БЕЗ РАЗРЕШЕНИЯ ОХРАНЯЕМЫЕ АВТОРСКИМ ПРАВОМ МАТЕРИАЛЫ!</strong>',
'longpagewarning'                  => '<strong>ПРЕДУПРЕЖДЕНИЕ: Длина этой страницы составляет $1 килобайт. Страницы, размер которых приближается к 32 КБ или превышает это значение, могут неверно отображаться в некоторых браузерах.
Пожалуйста, рассмотрите вариант разбиения страницы на меньшие части.</strong>',
'longpageerror'                    => '<strong>ОШИБКА: записываемый вами текст имеет размер $1 килобайт, что больше, чем установленный предел $2 килобайт. Страница не может быть сохранена.</strong>',
'readonlywarning'                  => '<strong>ПРЕДУПРЕЖДЕНИЕ: база данных заблокирована в связи с процедурами обслуживания,
поэтому вы не можете записать ваши изменения прямо сейчас.
Возможно, вам следует сохранить текст в файл на своём диске и поместить его в данный проект позже.</strong>',
'protectedpagewarning'             => '<strong>ПРЕДУПРЕЖДЕНИЕ: эта страница защищена от изменений, её могут редактировать только администраторы.</strong>',
'semiprotectedpagewarning'         => "'''Замечание:''' эта страница была защищена; редактировать её могут только зарегистрированные участники.",
'cascadeprotectedwarning'          => "'''Предупреждение:''' Данную страницу могут редактировать только участники группы «Администраторы», поскольку она включена {{PLURAL:$1|в следующую страницу, для которой|в следующие страницы, для которых}} включена каскадная защита:",
'titleprotectedwarning'            => '<strong>Предупреждение. Эта страница была защищена, создать её могут только определённые участники.</strong>',
'templatesused'                    => 'Шаблоны, использованные на текущей версии страницы:',
'templatesusedpreview'             => 'Шаблоны, используемые в предпросматриваемой странице:',
'templatesusedsection'             => 'Шаблоны, используемые в этой секции:',
'template-protected'               => '(защищено)',
'template-semiprotected'           => '(частично защищено)',
'hiddencategories'                 => 'Эта страница относится к $1 {{PLURAL:$1|скрытой категории|скрытым категориям|скрытым категориям}}:',
'edittools'                        => '<!-- Расположенный здесь текст будет показываться под формой редактирования и формой загрузки. -->',
'nocreatetitle'                    => 'Создание страниц ограничено',
'nocreatetext'                     => 'На этом сайте ограничена возможность создания новых страниц.
Вы можете вернуться назад и отредактировать существующую страницу, [[Special:UserLogin|представиться системе или создать новую учётную запись]].',
'nocreate-loggedin'                => 'У вас нет разрешения создавать новые страницы.',
'permissionserrors'                => 'Ошибки прав доступа',
'permissionserrorstext'            => 'У вас нет прав на выполнение этой операции по {{PLURAL:$1|следующей причине|следующим причинам}}:',
'permissionserrorstext-withaction' => "У вас нет разрешения на действие «'''$2'''» по {{PLURAL:$1|следующей причине|следующим причинам}}:",
'recreate-deleted-warn'            => "'''Внимание: вы пытаетесь воссоздать страницу, которая ранее удалялась.'''

Проверьте, действительно ли вам нужно воссоздавать эту страницу. Ниже приведён журнал удалений.",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Внимание. Эта страница содержит слишком много вызовов ресурсоёмких функций.

Количество вызовов не должно превышать $2, сейчас же оно равно $1.',
'expensive-parserfunction-category'       => 'Страницы со слишком большим количеством вызовов ресурсоёмких функций',
'post-expand-template-inclusion-warning'  => 'Внимание. Размер включаемых шаблонов слишком велик.
Некоторые шаблоны не будут включены.',
'post-expand-template-inclusion-category' => 'Страницы, для которых превышен допустимый размер включаемых шаблонов',
'post-expand-template-argument-warning'   => 'Внимание. Эта страница содержит по крайней мере один аргумент шаблона, имеющий слишком большой размер для развёртывания.
Подобные аргументы были опущены.',
'post-expand-template-argument-category'  => 'Страницы, содержащие пропущенные аргументы шаблонов',

# "Undo" feature
'undo-success' => 'Правка может быть отменена. Пожалуйста, просмотрите сравнение версий, чтобы убедиться, что это именно те изменения, которые вас интересуют, и нажмите «Записать страницу», чтобы изменения вступили в силу.',
'undo-failure' => 'Правка не может быть отменена из-за несовместимости промежуточных изменений.',
'undo-norev'   => 'Правка не может быть отменена, так как её не существует или она была удалена.',
'undo-summary' => 'Отмена правки $1 участника [[Special:Contributions/$2|$2]] ([[User talk:$2|обсуждение]])',

# Account creation failure
'cantcreateaccounttitle' => 'Невозможно создать учётную запись',
'cantcreateaccount-text' => "Создание учётных записей с этого IP-адреса (<b>$1</b>) было заблокировано [[User:$3|участником $3]].

$3 указал следующую причину: ''$2''",

# History pages
'viewpagelogs'        => 'Показать журналы для этой страницы',
'nohistory'           => 'Для этой страницы журнал изменений отсутствует.',
'revnotfound'         => 'Версия не найдена',
'revnotfoundtext'     => 'Старая версия страницы не найдена. Пожалуйста, проверьте правильность ссылки, которую вы использовали для доступа к этой странице.',
'currentrev'          => 'Текущая версия',
'revisionasof'        => 'Версия $1',
'revision-info'       => 'Версия от $1; $2',
'previousrevision'    => '← Предыдущая',
'nextrevision'        => 'Следующая →',
'currentrevisionlink' => 'Текущая версия',
'cur'                 => 'текущ.',
'next'                => 'след.',
'last'                => 'пред.',
'page_first'          => 'первая',
'page_last'           => 'последняя',
'histlegend'          => "Пояснения: (текущ.) — отличие от текущей версии; (пред.) — отличие от предшествующей версии; '''м''' — малозначимое изменение",
'deletedrev'          => '[удалена]',
'histfirst'           => 'старейшие',
'histlast'            => 'недавние',
'historysize'         => '($1 {{PLURAL:$1|байт|байта|байт}})',
'historyempty'        => '(пусто)',

# Revision feed
'history-feed-title'          => 'История изменений',
'history-feed-description'    => 'История изменений этой страницы в вики',
'history-feed-item-nocomment' => '$1 в $2', # user at time
'history-feed-empty'          => 'Запрашиваемой страницы не существует.
Она могла быть удалена или переименована.
Попробуйте [[Special:Search|найти в вики]] похожие страницы.',

# Revision deletion
'rev-deleted-comment'         => '(комментарий удалён)',
'rev-deleted-user'            => '(имя автора стёрто)',
'rev-deleted-event'           => '(запись удалена)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Эта версия страницы была удалена из общедоступного архива.
Возможно, объяснения даны в [{{fullurl:{{ns:special}}:Log/delete|page={{PAGENAMEE}}}} журнале удалений].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Эта версия страницы была удалена из общедоступного архива.
Вы можете просмотреть её, так как являетесь администратором сайта.
Возможно, объяснения удаления даны в [{{fullurl:{{ns:special}}:Log/delete|page={{PAGENAMEE}}}} журнале удалений].
</div>',
'rev-delundel'                => 'показать/скрыть',
'revisiondelete'              => 'Удалить / восстановить версии страницы',
'revdelete-nooldid-title'     => 'Не задана целевая версия',
'revdelete-nooldid-text'      => 'Вы не задали целевую версию (или версии) для выполнения этой функции.',
'revdelete-selected'          => '{{PLURAL:$2|Выбранная версия|Выбранные версии}} страницы [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Выбранная запись|Выбранные записи}} журнала:',
'revdelete-text'              => 'Удалённые версии будут показываться в истории страницы и журналах,
но часть их содержания будет недоступна обычным посетителям.

Администраторы будут иметь доступ к скрытому содержанию и смогут восстановить его через этот же интерфейс,
за исключением случаев, когда было установлено дополнительное ограничение.',
'revdelete-legend'            => 'Установить ограничения:',
'revdelete-hide-text'         => 'Скрыть текст этой версии страницы',
'revdelete-hide-name'         => 'Скрыть действие и его объект',
'revdelete-hide-comment'      => 'Скрыть комментарий',
'revdelete-hide-user'         => 'Скрыть имя автора',
'revdelete-hide-restricted'   => 'Применить ограничения также и к администраторам',
'revdelete-suppress'          => 'Скрывать данные также и от администраторов',
'revdelete-hide-image'        => 'Скрыть содержимое файла',
'revdelete-unsuppress'        => 'Снять ограничения с восстановленных версий',
'revdelete-log'               => 'Примечание:',
'revdelete-submit'            => 'Применить к выбранной версии',
'revdelete-logentry'          => 'Изменена видимость версии страницы [[$1]]',
'logdelete-logentry'          => 'Изменена видимость события для [[$1]]',
'revdelete-success'           => 'Видимость версии изменена.',
'logdelete-success'           => 'Видимость события изменена.',
'revdel-restore'              => 'Изменить видимость',
'pagehist'                    => 'История страницы',
'deletedhist'                 => 'История удалений',
'revdelete-content'           => 'содержимое',
'revdelete-summary'           => 'описание изменений',
'revdelete-uname'             => 'имя участника',
'revdelete-restricted'        => 'ограничения применяются к администраторам',
'revdelete-unrestricted'      => 'ограничения сняты для администраторов',
'revdelete-hid'               => 'скрыт $1',
'revdelete-unhid'             => 'раскрыт $1',
'revdelete-log-message'       => '$1 для $2 {{PLURAL:$2|версия|версии|версий}}',
'logdelete-log-message'       => '$1 для $2 {{PLURAL:$2|события|событий|событий}}',

# Suppression log
'suppressionlog'     => 'Журнал сокрытий',
'suppressionlogtext' => 'Ниже представлен список недавних удалений и блокировок, включающих скрытые от администраторов материалы.
См. [[Special:IPBlockList|список IP-блокировок]], чтобы просмотреть список текущих блокировок.',

# History merging
'mergehistory'                     => 'Объединение историй правок',
'mergehistory-header'              => 'Эта страница позволяет вам объединить историю правок двух различных страниц.
Убедитесь, что это изменение сохранит целостность истории страницы.',
'mergehistory-box'                 => 'Объединить истории правок двух страниц:',
'mergehistory-from'                => 'Исходная страница:',
'mergehistory-into'                => 'Целевая страница:',
'mergehistory-list'                => 'Объединяемая история правок',
'mergehistory-merge'               => 'Следующие версии [[:$1]] могут быть объединены в [[:$2]]. Используйте переключатели для того, чтобы объединить только выбранный диапазон правок. Учтите, что при использовании навигационных ссылок данные будут потерянны.',
'mergehistory-go'                  => 'Показать объединяемые правки',
'mergehistory-submit'              => 'Объединить правки',
'mergehistory-empty'               => 'Не найдены правки для объединения.',
'mergehistory-success'             => '$3 {{PLURAL:$3|правка|правки|правок}} из [[:$1]] успешно {{PLURAL:$3|перенесена|перенесены|перенесены}} в [[:$2]].',
'mergehistory-fail'                => 'Не удалось произвести объединение историй страниц, пожалуйста проверьте параметры страницы и времени.',
'mergehistory-no-source'           => 'Исходная страница «$1» не существует.',
'mergehistory-no-destination'      => 'Целевая страница «$1» не существует.',
'mergehistory-invalid-source'      => 'Источник должен иметь правильный заголовок.',
'mergehistory-invalid-destination' => 'Целевая страница должна иметь правильный заголовок.',
'mergehistory-autocomment'         => 'Перенос [[:$1]] в [[:$2]]',
'mergehistory-comment'             => 'Перенос [[:$1]] в [[:$2]]: $3',

# Merge log
'mergelog'           => 'Журнал объединений',
'pagemerge-logentry' => 'объединена [[$1]] и [[$2]] (версии вплоть до $3)',
'revertmerge'        => 'Разделить',
'mergelogpagetext'   => 'Ниже приведён список последних объединений историй страниц.',

# Diffs
'history-title'           => '$1 — история изменений',
'difference'              => '(Различия между версиями)',
'lineno'                  => 'Строка $1:',
'compareselectedversions' => 'Сравнить выбранные версии',
'editundo'                => 'отменить',
'diff-multi'              => '({{PLURAL:$1|$1 промежуточная версия не показана|$1 промежуточные версии не показаны|$1 промежуточных версий не показаны.}})',

# Search results
'searchresults'             => 'Результаты поиска',
'searchresulttext'          => 'Для получения более подробной информации о поиске на страницах проекта, см. [[{{MediaWiki:Helppage}}|справочный раздел]].',
'searchsubtitle'            => 'По запросу «[[:$1]]»',
'searchsubtitleinvalid'     => 'По запросу «$1»',
'noexactmatch'              => "'''Страницы с названием «$1» не существует.''' [[:$1|Создать страницу]].",
'noexactmatch-nocreate'     => 'Страницы с названием «$1» не существует.',
'toomanymatches'            => 'Найдено слишком много соответствий, пожалуйста, попробуйте другой запрос',
'titlematches'              => 'Совпадения в названиях страниц',
'notitlematches'            => 'Нет совпадений в названиях страниц',
'textmatches'               => 'Совпадения в текстах страниц',
'notextmatches'             => 'Нет совпадений в текстах страниц',
'prevn'                     => 'предыдущие $1',
'nextn'                     => 'следующие $1',
'viewprevnext'              => 'Просмотреть ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|$2 слово|$2 слова|$2 слов}})',
'search-result-score'       => 'Релевантность: $1 %',
'search-redirect'           => '(перенаправление $1)',
'search-section'            => '(раздел $1)',
'search-suggest'            => 'Возможно, вы имели в виду: $1',
'search-interwiki-caption'  => 'Родственные проекты',
'search-interwiki-default'  => '$1 результ.:',
'search-interwiki-more'     => '(ещё)',
'search-mwsuggest-enabled'  => 'с советами',
'search-mwsuggest-disabled' => 'без советов',
'search-relatedarticle'     => 'Связанный',
'mwsuggest-disable'         => 'Отключить AJAX-подсказки',
'searchrelated'             => 'связанный',
'searchall'                 => 'все',
'showingresults'            => 'Ниже {{PLURAL:$1|показан|показаны|показаны}} <strong>$1</strong> {{PLURAL:$1|результат|результата|результатов}}, начиная с №&nbsp;<strong>$2</strong>.',
'showingresultsnum'         => 'Ниже {{PLURAL:$3|показан|показаны|показаны}} <strong>$3</strong> {{PLURAL:$3|результат|результата|результатов}}, начиная с №&nbsp;<strong>$2</strong>.',
'showingresultstotal'       => "Ниже {{PLURAL:$3|показан результат '''$1''' из '''$3'''|показаны результаты '''$1 — $2''' из '''$3'''}}",
'nonefound'                 => "'''Замечание.''' По умолчанию поиск производится не во всех пространствах имён. Используйте приставку ''all:'', чтобы искать во всех пространствах имён (включая обсуждения участников, шаблоны и пр.), или укажите требуемое пространство имён.",
'powersearch'               => 'Расширенный поиск',
'powersearch-legend'        => 'Расширенный поиск',
'powersearch-ns'            => 'Поиск в пространствах имён:',
'powersearch-redir'         => 'Выводить перенаправления',
'powersearch-field'         => 'Поиск',
'search-external'           => 'Внешний поиск',
'searchdisabled'            => 'Извините, но встроенный полнотекстовый поиск выключен. Вы можете воспользоваться поиском по сайту через поисковые системы общего назначения, однако имейте в виду, что копия сайта в их кеше может быть несколько устаревшей.',

# Preferences page
'preferences'              => 'Настройки',
'mypreferences'            => 'Настройки',
'prefs-edits'              => 'Количество правок:',
'prefsnologin'             => 'Вы не представились системе',
'prefsnologintext'         => 'Вы должны <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} представиться системе]</span>, чтобы изменять настройки участника.',
'prefsreset'               => 'Восстановлены настройки по умолчанию.',
'qbsettings'               => 'Панель навигации',
'qbsettings-none'          => 'Не показывать',
'qbsettings-fixedleft'     => 'Неподвижная слева',
'qbsettings-fixedright'    => 'Неподвижная справа',
'qbsettings-floatingleft'  => 'Плавающая слева',
'qbsettings-floatingright' => 'Плавающая справа',
'changepassword'           => 'Сменить пароль',
'skin'                     => 'Оформление',
'math'                     => 'Отображение формул',
'dateformat'               => 'Формат даты',
'datedefault'              => 'По умолчанию',
'datetime'                 => 'Дата и время',
'math_failure'             => 'Невозможно разобрать выражение',
'math_unknown_error'       => 'неизвестная ошибка',
'math_unknown_function'    => 'неизвестная функция',
'math_lexing_error'        => 'лексическая ошибка',
'math_syntax_error'        => 'синтаксическая ошибка',
'math_image_error'         => 'Преобразование в PNG прошло с ошибкой; проверьте правильность установки latex, dvips, gs и convert',
'math_bad_tmpdir'          => 'Не удаётся создать или записать во временный каталог математики',
'math_bad_output'          => 'Не удаётся создать или записать в выходной каталог математики',
'math_notexvc'             => 'Выполняемый файл texvc не найден; См. math/README — справку по настройке.',
'prefs-personal'           => 'Личные данные',
'prefs-rc'                 => 'Страница свежих правок',
'prefs-watchlist'          => 'Список наблюдения',
'prefs-watchlist-days'     => 'Максимальное число дней, отображаемых в списке наблюдения:',
'prefs-watchlist-edits'    => 'Максимальное количество правок, отображаемых в расширенном списке наблюдения:',
'prefs-misc'               => 'Другие настройки',
'saveprefs'                => 'Записать',
'resetprefs'               => 'Сбросить',
'oldpassword'              => 'Старый пароль:',
'newpassword'              => 'Новый пароль:',
'retypenew'                => 'Повторите ввод нового пароля:',
'textboxsize'              => 'Редактирование',
'rows'                     => 'Строк:',
'columns'                  => 'Столбцов:',
'searchresultshead'        => 'Поиск',
'resultsperpage'           => 'Количество найденных записей на страницу:',
'contextlines'             => 'Количество показываемых строк для каждой найденной:',
'contextchars'             => 'Количество символов контекста на строку:',
'stub-threshold'           => 'Порог для определения оформления <a href="#" class="stub">ссылок на заготовки</a> (в байтах):',
'recentchangesdays'        => 'Количество дней, за которые показывать свежие правки:',
'recentchangescount'       => 'Количество правок, отображаемое в списках и журналах:',
'savedprefs'               => 'Ваши настройки сохранены.',
'timezonelegend'           => 'Часовой пояс',
'timezonetext'             => 'Введите смещение (в часах) вашего местного времени
от времени сервера (UTC — гринвичского).',
'localtime'                => 'Местное время',
'timezoneoffset'           => 'Смещение¹',
'servertime'               => 'Текущее время сервера',
'guesstimezone'            => 'Заполнить из браузера',
'allowemail'               => 'Разрешить приём электронной почты от других участников',
'prefs-searchoptions'      => 'Настройки поиска',
'prefs-namespaces'         => 'Пространства имён',
'defaultns'                => 'По умолчанию искать в следующих пространствах имён:',
'default'                  => 'по умолчанию',
'files'                    => 'Файлы',

# User rights
'userrights'                  => 'Управление правами участников', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Управление группами участников',
'userrights-user-editname'    => 'Введите имя участника:',
'editusergroup'               => 'Изменить группы участника',
'editinguser'                 => "Изменение прав участника '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Изменить группы участника',
'saveusergroups'              => 'Сохранить группы участника',
'userrights-groupsmember'     => 'Член групп:',
'userrights-groups-help'      => 'Вы можете изменить группы, в которые входит этот участник.
* Если около названия группы стоит отметка, значит участник входит в эту группу.
* Если отметка не стоит — участник не относится к соответствующей группе.
* Знак * отмечает, что вы не можете удалить из группы участника, если добавите его в неё или наоборот.',
'userrights-reason'           => 'Причина изменения:',
'userrights-no-interwiki'     => 'У вас нет разрешения изменять права участников на других вики.',
'userrights-nodatabase'       => 'База данных $1 не существует или не является локальной.',
'userrights-nologin'          => 'Вы должны [[Special:UserLogin|представиться системе]] с учётной записи администратора, чтобы назначать права пользователям.',
'userrights-notallowed'       => 'С вашей учётной записи не разрешено назначать права пользователям.',
'userrights-changeable-col'   => 'Группы, которые вы можете изменять',
'userrights-unchangeable-col' => 'Группы, которые вы не можете изменять',

# Groups
'group'               => 'Группа:',
'group-user'          => 'Участники',
'group-autoconfirmed' => 'Автоподтверждённые участники',
'group-bot'           => 'Боты',
'group-sysop'         => 'Администраторы',
'group-bureaucrat'    => 'Бюрократы',
'group-suppress'      => 'Ревизоры',
'group-all'           => '(все)',

'group-user-member'          => 'участник',
'group-autoconfirmed-member' => 'автоподтверждённый участник',
'group-bot-member'           => 'бот',
'group-sysop-member'         => 'администратор',
'group-bureaucrat-member'    => 'бюрократ',
'group-suppress-member'      => 'Ревизор',

'grouppage-user'          => '{{ns:project}}:Участники',
'grouppage-autoconfirmed' => '{{ns:project}}:Автоподтверждённые участники',
'grouppage-bot'           => '{{ns:project}}:Боты',
'grouppage-sysop'         => '{{ns:project}}:Администраторы',
'grouppage-bureaucrat'    => '{{ns:project}}:Бюрократы',
'grouppage-suppress'      => '{{ns:project}}:Ревизоры',

# Rights
'right-read'                 => 'просмотр страниц',
'right-edit'                 => 'правка страниц',
'right-createpage'           => 'создание страниц (не являющихся обсуждениями)',
'right-createtalk'           => 'создавать страницы обсуждений',
'right-createaccount'        => 'создание новых учётных записей участников',
'right-minoredit'            => 'простановка отметки «малое изменение»',
'right-move'                 => 'переименование страниц',
'right-move-subpages'        => 'переименовывать страницы с их подстраницами',
'right-suppressredirect'     => 'не создаётся перенаправление со старого имени при переименовании страницы',
'right-upload'               => 'загрузка файлов',
'right-reupload'             => 'запись файлов поверх существующих',
'right-reupload-own'         => 'перезапись файлов тем же участником',
'right-reupload-shared'      => 'подмена файлов из общих хранилищ локальными',
'right-upload_by_url'        => 'загрузка файлов с адреса URL',
'right-purge'                => 'очистка кэша страниц без страницы подтверждения',
'right-autoconfirmed'        => 'правка частично защищённых страниц',
'right-bot'                  => 'считаться автоматическим процессом',
'right-nominornewtalk'       => 'отсутствие малых правок на страницах обсуждений включает режим новых сообщений',
'right-apihighlimits'        => 'меньше ограничений на выполнение API-запросов',
'right-writeapi'             => 'использование API для записи',
'right-delete'               => 'удаление страниц',
'right-bigdelete'            => 'удаление страниц с длинными историями',
'right-deleterevision'       => 'удаление и восстановление конкретных версий страниц',
'right-deletedhistory'       => 'просмотр истории удалённых страниц без доступа к удалённому тексту',
'right-browsearchive'        => 'поиск удалённых страниц',
'right-undelete'             => 'восстановление страниц',
'right-suppressrevision'     => 'просмотр и восстановление скрытых от администраторов версий страниц',
'right-suppressionlog'       => 'просмотр частных журналов',
'right-block'                => 'установка запрета на редактирование другим участникам',
'right-blockemail'           => 'установка запрета на отправку электронной почты',
'right-hideuser'             => 'запрет имени участника и его сокрытие',
'right-ipblock-exempt'       => 'обход блокировок по IP, автоблокировок и блокировок диапазонов',
'right-proxyunbannable'      => 'обход автоматической блокировки прокси',
'right-protect'              => 'изменение уровня защиты страниц и правка защищённых страниц',
'right-editprotected'        => 'правка защищённых страниц (без каскадной защиты)',
'right-editinterface'        => 'изменение пользовательского интерфейса',
'right-editusercssjs'        => 'правка CSS- и JS-файлов других участников',
'right-rollback'             => 'быстрый откат правок последнего участник на некоторой странице',
'right-markbotedits'         => 'отметка откатываемых правок как правок бота',
'right-noratelimit'          => 'нет ограничений по скорости',
'right-import'               => 'импорт страниц из других вики',
'right-importupload'         => 'импорт страниц через загрузку файлов',
'right-patrol'               => 'отметка правок как отпатрулированных',
'right-autopatrol'           => 'правки автоматически отмечаются как патрулированные',
'right-patrolmarks'          => 'просмотр отметок о патрулировании в свежих правках',
'right-unwatchedpages'       => 'просмотр списка ненаблюдаемых страниц',
'right-trackback'            => 'отправка Trackback',
'right-mergehistory'         => 'объединение историй страниц',
'right-userrights'           => 'изменение прав всех участников',
'right-userrights-interwiki' => 'изменение прав участников на других вики-сайтах',
'right-siteadmin'            => 'блокировка и разблокировка базы данных',

# User rights log
'rightslog'      => 'Журнал прав участника',
'rightslogtext'  => 'Это журнал изменений прав участника.',
'rightslogentry' => 'изменил права доступа для участника $1 с $2 на $3',
'rightsnone'     => '(нет)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|изменение|изменения|изменений}}',
'recentchanges'                     => 'Свежие правки',
'recentchangestext'                 => 'Ниже в хронологическом порядке перечислены последние изменения на страницах {{grammar:genitive|{{SITENAME}}}}.',
'recentchanges-feed-description'    => 'Отслеживать последние изменения в вики в этом потоке.',
'rcnote'                            => "{{PLURAL:$1|Последнее '''$1''' изменение|Последние '''$1''' изменения|Последние '''$1''' изменений}} за '''$2''' {{PLURAL:$2|день|дня|дней}}, на момент времени $5 $4.",
'rcnotefrom'                        => 'Ниже перечислены изменения с <strong>$2</strong> (по <strong>$1</strong>).',
'rclistfrom'                        => 'Показать изменения с $1.',
'rcshowhideminor'                   => '$1 малые правки',
'rcshowhidebots'                    => '$1 ботов',
'rcshowhideliu'                     => '$1 представившихся участников',
'rcshowhideanons'                   => '$1 анонимов',
'rcshowhidepatr'                    => '$1 проверенные правки',
'rcshowhidemine'                    => '$1 свои правки',
'rclinks'                           => 'Показать последние $1 изменений за $2 {{PLURAL:$2|день|дня|дней}};<br />$3.',
'diff'                              => 'разн.',
'hist'                              => 'история',
'hide'                              => 'Скрыть',
'show'                              => 'Показать',
'minoreditletter'                   => 'м',
'newpageletter'                     => 'Н',
'boteditletter'                     => 'б',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|наблюдающий пользователь|наблюдающих пользователя|наблюдающих пользователей}}]',
'rc_categories'                     => 'Только из категорий (разделитель «|»)',
'rc_categories_any'                 => 'Любой',
'newsectionsummary'                 => '/* $1 */ Новая тема',

# Recent changes linked
'recentchangeslinked'          => 'Связанные правки',
'recentchangeslinked-title'    => 'Связанные правки для $1',
'recentchangeslinked-noresult' => 'На связанных страницах не было изменений за указанный период.',
'recentchangeslinked-summary'  => "Это список недавних изменений в страницах, на которые ссылается указанная страница (или входящих в указанную категорию).
Страницы, входящие в [[Special:Watchlist|ваш список наблюдения]] '''выделены'''.",
'recentchangeslinked-page'     => 'Название страницы:',
'recentchangeslinked-to'       => 'Наоборот, показать изменения на страницах, которые ссылаются на указанную страницу',

# Upload
'upload'                      => 'Загрузить файл',
'uploadbtn'                   => 'Загрузить файл',
'reupload'                    => 'Изменить загрузку',
'reuploaddesc'                => 'Вернуться к форме загрузки',
'uploadnologin'               => 'Вы не представились системе',
'uploadnologintext'           => 'Вы должны [[Special:UserLogin|представиться системе]],
чтобы загружать файлы на сервер.',
'upload_directory_missing'    => 'Директория для загрузок ($1) отсутствует и не может быть создана веб-сервером.',
'upload_directory_read_only'  => 'Веб-сервер не имеет прав записи в папку ($1), в которой предполагается хранить загружаемые файлы.',
'uploaderror'                 => 'Ошибка загрузки файла',
'uploadtext'                  => "Используя эту форму вы можете загрузить на сервер файлы.
Чтобы просмотреть ранее загруженные файлы, обратитесь к [[Special:ImageList|списку загруженных файлов]]. Загрузка файлов также записывается в [[Special:Log/upload|журнал загрузок]], удаления файлов записываются в [[Special:Log/delete|журнал удалений]].

Для включения файла в статью вы можете использовать строки вида:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' для вставки полной версии файла;
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|описание]]</nowiki></tt>''' для вставки уменьшенной до 200 пикселей по ширине версии файла слева от текста с выводом под ним указанного описания;
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' для вставки ссылки на файл, без отображения его содержимого на странице.",
'upload-permitted'            => 'Разрешённые типы файлов: $1.',
'upload-preferred'            => 'Предпочтительные типы файлов: $1.',
'upload-prohibited'           => 'Запрещённые типы файлов: $1.',
'uploadlog'                   => 'журнал загрузок',
'uploadlogpage'               => 'Журнал загрузок',
'uploadlogpagetext'           => 'Ниже представлен список последних загрузок файлов.
См. также [[Special:NewImages|галерею новых файлов]], где сведения о новых загрузках представлены в более наглядном виде.',
'filename'                    => 'Имя файла',
'filedesc'                    => 'Краткое описание',
'fileuploadsummary'           => 'Краткое описание:',
'filestatus'                  => 'Условия распространения:',
'filesource'                  => 'Источник:',
'uploadedfiles'               => 'Загруженные файлы',
'ignorewarning'               => 'Игнорировать предупреждения и сохранить файл',
'ignorewarnings'              => 'Игнорировать предупреждения',
'minlength1'                  => 'Название файла должно содержать хотя бы одну букву.',
'illegalfilename'             => 'Имя файла «$1» содержит символы, которые не разрешается использовать в заголовках. Пожалуйста, переименуйте файл и попытайтесь загрузить его снова.',
'badfilename'                 => 'Название файла было изменено на $1.',
'filetype-badmime'            => 'Файлы, имеющие MIME-тип "$1", не могут быть загружены.',
'filetype-unwanted-type'      => "'''\".\$1\"''' — нежелательный тип файла.
{{PLURAL:\$3|Предпочтительным типом файла является|Предпочтительные типы файлов:}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' — запрещённый тип файла.
{{PLURAL:\$3|Разрешённым типом файла является|Разрешённые типы файлов:}} \$2.",
'filetype-missing'            => 'Отсутствует расширение у файла (например, «.jpg»).',
'large-file'                  => 'Рекомендуется использовать изображения, размер которых не превышает $1 байт (размер загруженного файла составляет $2 байт).',
'largefileserver'             => 'Размер файла превышает максимально разрешённый.',
'emptyfile'                   => 'Загруженный вами файл вероятно пустой. Возможно, это произошло из-за ошибки при наборе имени файла. Пожалуйста, проверьте, действительно ли вы хотите загрузить этот файл.',
'fileexists'                  => 'Файл с этим именем уже существует, пожалуйста, проверьте <strong><tt>$1</tt></strong>, если вы не уверены, что хотите заменить его.',
'filepageexists'              => 'Страница описания для этого файла уже создана как <strong><tt>$1</tt></strong>, но файла с таким именем нет. Введённое описание не появится на странице описания изображения. Чтобы добавить новое описание, вам придётся изменить его вручную.',
'fileexists-extension'        => 'Существует файл с похожим именем:<br />
Имя загруженного файла: <strong><tt>$1</tt></strong><br />
Имя существующего файла: <strong><tt>$2</tt></strong><br />
Пожалуйста, выберите другое имя.',
'fileexists-thumb'            => "<center>'''Существующее изображение'''</center>",
'fileexists-thumbnail-yes'    => 'Файл, вероятно, является уменьшенной копией (миниатюрой). Пожалуйста, проверьте файл <strong><tt>$1</tt></strong>.<br />
Если указанный файл является тем же изображением, не стоит загружать отдельно его уменьшенную копию.',
'file-thumbnail-no'           => 'Название файла начинается с <strong><tt>$1</tt></strong>.
Вероятно, это уменьшенная копия изображения <i>(миниатюра)</i>.
Если у вас есть данное изображение в полном размере, пожалуйста, загрузите его или измените имя файла.',
'fileexists-forbidden'        => 'Файл с этим именем уже существует; пожалуйста, вернитесь назад и загрузите файл под другим именем. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Файл с этим именем уже существует в общем хранилище файлов.
Если вы всё-таки хотите загрузить этот файл, пожалуйста, вернитесь назад и измените имя файла. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Этот файл является дубликатом {{PLURAL:$1|следующего файла|следующих файлов}}:',
'successfulupload'            => 'Загрузка успешно завершена',
'uploadwarning'               => 'Предупреждение',
'savefile'                    => 'Записать файл',
'uploadedimage'               => 'загружено «[[$1]]»',
'overwroteimage'              => 'загружена новая версия «[[$1]]»',
'uploaddisabled'              => 'Загрузка запрещена',
'uploaddisabledtext'          => 'Загрузка файлов отключена.',
'uploadscripted'              => 'Файл содержит HTML-код или скрипт, который может быть ошибочно обработан браузером.',
'uploadcorrupt'               => 'Файл либо повреждён, либо имеет неверное расширение. Пожалуйста, проверьте файл и попробуйте загрузить его ещё раз.',
'uploadvirus'                 => 'Файл содержит вирус! См. $1',
'sourcefilename'              => 'Исходное имя файла:',
'destfilename'                => 'Целевое имя файла:',
'upload-maxfilesize'          => 'Максимальный размер файла: $1',
'watchthisupload'             => 'Включить этот файл в список наблюдения',
'filewasdeleted'              => 'Файл с таким именем уже существовал ранее, но был удалён. Пожалуйста, проверьте $1 перед повторной загрузкой.',
'upload-wasdeleted'           => "'''Внимание: вы пытаетесь загрузить файл, который ранее удалялся.'''

Проверьте, действительно ли вам нужно загружать этот файл.
Ниже приведён журнал удалений:",
'filename-bad-prefix'         => 'Имя загружаемого файла начинается с <strong>«$1»</strong> и вероятно является шаблонным именем, которое цифровая фотокамера даёт снимкам. Пожалуйста, выберите имя лучше описывающее содержание файла.',
'filename-prefix-blacklist'   => ' #<!-- оставьте эту строчку как есть --> <pre>
# Синтаксис следующий:
#   * Всё, что начинается с символа «#» считается комментарием (до конца строки)
#   * Каждая непустая строка — префикс стандартного названия файла, которое обычно даёт цифровая камера
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # некоторые мобильные телефоны
IMG # общее
JD # Jenoptik
MGP # Pentax
PICT # различные
 #</pre> <!-- оставьте эту строчку как есть -->',

'upload-proto-error'      => 'Неправильный протокол',
'upload-proto-error-text' => 'Для удалённой загрузки требуется адрес, начинающийся с <code>http://</code> или <code>ftp://</code>.',
'upload-file-error'       => 'Внутренняя ошибка',
'upload-file-error-text'  => 'Внутренняя ошибка при попытке создать временный файл на сервере. Пожалуйста, обратитесь к системному администратору.',
'upload-misc-error'       => 'Неизвестная ошибка загрузки',
'upload-misc-error-text'  => 'Неизвестная ошибка загрузки. Пожалуйста, проверьте, что адрес верен, и повторите попытку. Если проблема остаётся, обратитесь к системному администратору.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Невозможно обратить по указанному адресу.',
'upload-curl-error6-text'  => 'Невозможно обратить по указанному адресу. Пожалуйста, проверьте, что адрес верен, а сайт доступен.',
'upload-curl-error28'      => 'Время, отведённое на загрузку, истекло',
'upload-curl-error28-text' => 'Сайт слишком долго не отвечает. Пожалуйста, проверьте что сайт работоспособен и после небольшого перерыва попробуйте ещё раз. Возможно, операцию следует провести в другое время, когда сайт менее нагружен.',

'license'            => 'Лицензирование:',
'nolicense'          => 'Отсутствует',
'license-nopreview'  => '(Предпросмотр недоступен)',
'upload_source_url'  => ' (правильный, публично доступный интернет-адрес)',
'upload_source_file' => ' (файл на вашем компьютере)',

# Special:ImageList
'imagelist-summary'     => 'Эта служебная страница показывает все загруженные файлы.
Недавно загруженные файлы по умолчанию показываются в верху списка.
Щелчок на заголовке колонки изменяет порядок сортировки.',
'imagelist_search_for'  => 'Поиск по имени изображения:',
'imgfile'               => 'файл',
'imagelist'             => 'Список файлов',
'imagelist_date'        => 'Дата',
'imagelist_name'        => 'Имя файла',
'imagelist_user'        => 'Участник',
'imagelist_size'        => 'Размер',
'imagelist_description' => 'Описание',

# Image description page
'filehist'                       => 'История файла',
'filehist-help'                  => 'Нажмите на дату/время, чтобы просмотреть, как тогда выглядел файл.',
'filehist-deleteall'             => 'удалить все',
'filehist-deleteone'             => 'удалить',
'filehist-revert'                => 'вернуть',
'filehist-current'               => 'текущий',
'filehist-datetime'              => 'Дата/время',
'filehist-user'                  => 'Участник',
'filehist-dimensions'            => 'Размер объекта',
'filehist-filesize'              => 'Размер файла',
'filehist-comment'               => 'Примечание',
'imagelinks'                     => 'Ссылки',
'linkstoimage'                   => '{{PLURAL:$1|Следующая $1 страница ссылается|Следующие $1 страницы ссылаются|Следующие $1 страниц ссылаются}} на данный файл:',
'nolinkstoimage'                 => 'Нет страниц, ссылающихся на данный файл.',
'morelinkstoimage'               => 'Просмотреть [[Special:WhatLinksHere/$1|остальные ссылки]] на этот файл.',
'redirectstofile'                => 'Со {{PLURAL:$1|следующего $1 файла установлено перенаправление|следующих $1 файлов установлены перенаправления|следующих $1 файлов установлены перенаправления}} на этот файл:',
'duplicatesoffile'               => '{{PLURAL:$1|Следующий $1 файл является дубликатом|Следующие $1 файла являются дубликатами|Следующие $1 файлов являются дубликатами}} этого файла:',
'sharedupload'                   => 'Этот файл загружен в общее для нескольких проектов хранилище.',
'shareduploadwiki'               => 'Дополнительную информацию можно найти на $1.',
'shareduploadwiki-desc'          => 'Содержимое его $1 из общего хранилища показано ниже.',
'shareduploadwiki-linktext'      => 'страницы описания',
'shareduploadduplicate'          => 'Этот файл является дубликатом $1 из общего хранилища.',
'shareduploadduplicate-linktext' => 'другого файла',
'shareduploadconflict'           => 'Этот файл имеет такое же имя как и $1 из общего хранилища.',
'shareduploadconflict-linktext'  => 'другой файл',
'noimage'                        => 'Файла с таким именем не существует, но вы можете $1.',
'noimage-linktext'               => 'загрузить его',
'uploadnewversion-linktext'      => 'Загрузить новую версию этого файла',
'imagepage-searchdupe'           => 'Поиск одинаковых файлов',

# File reversion
'filerevert'                => 'Возврат к старой версии $1',
'filerevert-legend'         => 'Возвратить версию файла',
'filerevert-intro'          => '<span class="plainlinks">Вы возвращаете \'\'\'[[Media:$1|$1]]\'\'\' к [$4 версии от $3, $2].</span>',
'filerevert-comment'        => 'Примечание:',
'filerevert-defaultcomment' => 'Возврат к версии от $2, $1',
'filerevert-submit'         => 'Возвратить',
'filerevert-success'        => "'''[[Media:$1|$1]]''' был возвращён к [$4 версии от $3, $2].",
'filerevert-badversion'     => 'Не существует предыдущей локальной версии этого файла с указанной отметкой даты и времени.',

# File deletion
'filedelete'                  => '$1 — удаление',
'filedelete-legend'           => 'Удалить файл',
'filedelete-intro'            => "Вы удаляете '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => '<span class="plainlinks">Вы удаляете версию \'\'\'[[Media:$1|$1]]\'\'\' от [$4 $3, $2].</span>',
'filedelete-comment'          => 'Причина удаления:',
'filedelete-submit'           => 'Удалить',
'filedelete-success'          => "'''$1''' был удалён.",
'filedelete-success-old'      => "Версия '''[[Media:$1|$1]]''' от $3 $2 была удалена.",
'filedelete-nofile'           => "'''$1''' не существует.",
'filedelete-nofile-old'       => "Не существует архивной версии '''$1''' с указанными атрибутами.",
'filedelete-iscurrent'        => 'Вы пытаетесь удалить последнюю версию этого файла. Пожалуйста, верните сначала файл к одной из старых версий.',
'filedelete-otherreason'      => 'Другая причина:',
'filedelete-reason-otherlist' => 'Другая причина',
'filedelete-reason-dropdown'  => '* Распространенные причины удаления
** нарушение авторских прав
** файл-дубликат',
'filedelete-edit-reasonlist'  => 'Править список причин',

# MIME search
'mimesearch'         => 'Поиск по MIME',
'mimesearch-summary' => 'Эта страница позволяет отбирать файлы по их MIME-типу. Формат ввода: типсодержимого/подтип, например <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-тип:',
'download'           => 'загрузить',

# Unwatched pages
'unwatchedpages' => 'Страницы, за которыми никто не следит',

# List redirects
'listredirects' => 'Список перенаправлений',

# Unused templates
'unusedtemplates'     => 'Неиспользуемые шаблоны',
'unusedtemplatestext' => 'На этой странице перечислены все страницы пространства имён «Шаблоны», которые не включены в другие страницы. Не забывайте проверить отсутствие других ссылок на шаблон, перед его удалением.',
'unusedtemplateswlh'  => 'другие ссылки',

# Random page
'randompage'         => 'Случайная статья',
'randompage-nopages' => 'В данном пространстве имён отсутствуют страницы.',

# Random redirect
'randomredirect'         => 'Случайное перенаправление',
'randomredirect-nopages' => 'Это пространство имён не содержит перенаправлений.',

# Statistics
'statistics'             => 'Статистика',
'sitestats'              => 'Статистика сайта',
'userstats'              => 'Статистика участников',
'sitestatstext'          => "Всего в базе данных содержится '''$1''' {{PLURAL:$1|страница|страницы|страниц}}.
Это число включает в себя страницы о проекте, страницы обсуждений, незаконченные статьи, перенаправления и другие страницы, которые, не учитываются при подсчёте количества статей.
За исключением них, есть '''$2''' {{PLURAL:$2|страница|страницы|страниц}}, которые считаются полноценными статьями.

{{PLURAL:$8|Был загружен|Было загружено|Было загружено}} '''$8''' {{PLURAL:$8|файл|файла|файлов}}.

Всего с момента установки вики {{PLURAL:$3|был произведён '''$3''' просмотр|было произведено '''$3''' просмотра|было произведено '''$3''' просмотров}} страниц и '''$4''' {{PLURAL:$4|изменение|изменения|изменений}} страниц. Таким образом, в среднем приходится '''$5''' {{PLURAL:$5|изменение|изменения|изменений}} на одну страницу, и '''$6''' просмотров на одно изменение.

Величина [http://www.mediawiki.org/wiki/Manual:Job_queue очереди заданий] составляет '''$7'''.",
'userstatstext'          => "{{PLURAL:$1|Зарегистрировался|Зарегистрировались|Зарегистрировались}} '''$1''' {{PLURAL:$1|участник|участника|участников}}, из которых '''$2''' ($4 %) имеют права «$5».",
'statistics-mostpopular' => 'Наиболее часто просматриваемые страницы',

'disambiguations'      => 'Страницы, описывающие многозначные термины',
'disambiguationspage'  => 'Template:Неоднозначность',
'disambiguations-text' => "Следующие страницы ссылаются на '''многозначные страницы'''.
Вместо этого они, вероятно, должны указывать на соответствующую конкретную статью.<br />
Страница считается многозначной, если на ней размещён шаблон, имя которого указано на странице [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Двойные перенаправления',
'doubleredirectstext'        => 'На этой странице представлен список перенаправлений на другие перенаправления. Каждая строка содержит ссылки на первое и второе перенаправления, а также первую строчку страницы второго перенаправления, в которой обычно указывается название страницы, куда должно ссылаться первое перенаправление.',
'double-redirect-fixed-move' => 'Страница [[$1]] была переименована, сейчас она перенаправляет на [[$2]]',
'double-redirect-fixer'      => 'Исправитель перенаправлений',

'brokenredirects'        => 'Разорванные перенаправления',
'brokenredirectstext'    => 'Следующие перенаправления указывают на несуществующие страницы.',
'brokenredirects-edit'   => '(править)',
'brokenredirects-delete' => '(удалить)',

'withoutinterwiki'         => 'Страницы без межъязыковых ссылок',
'withoutinterwiki-summary' => 'Следующие страницы не имеют интервики-ссылок:',
'withoutinterwiki-legend'  => 'Приставка',
'withoutinterwiki-submit'  => 'Показать',

'fewestrevisions' => 'Страницы с наименьшим количеством изменений',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|байт|байта|байт}}',
'ncategories'             => '$1 {{PLURAL:$1|категория|категории|категорий}}',
'nlinks'                  => '$1 {{PLURAL:$1|ссылка|ссылки|ссылок}}',
'nmembers'                => '$1 {{PLURAL:$1|объект|объекта|объектов}}',
'nrevisions'              => '$1 {{PLURAL:$1|версия|версии|версий}}',
'nviews'                  => '$1 {{PLURAL:$1|просмотр|просмотра|просмотров}}',
'specialpage-empty'       => 'Запрос не дал результатов.',
'lonelypages'             => 'Страницы-сироты',
'lonelypagestext'         => 'На следующие страницы нет ссылок с других страниц данной вики.',
'uncategorizedpages'      => 'Некатегоризованные страницы',
'uncategorizedcategories' => 'Некатегоризованные категории',
'uncategorizedimages'     => 'Некатегоризованные изображения',
'uncategorizedtemplates'  => 'Некатегоризованные шаблоны',
'unusedcategories'        => 'Неиспользуемые категории',
'unusedimages'            => 'Неиспользуемые файлы',
'popularpages'            => 'Популярные страницы',
'wantedcategories'        => 'Требуемые категории',
'wantedpages'             => 'Требуемые страницы',
'missingfiles'            => 'Отсутствующие файлы',
'mostlinked'              => 'Страницы, на которые больше всего ссылок',
'mostlinkedcategories'    => 'Категории, на которые больше всего ссылок',
'mostlinkedtemplates'     => 'Самые используемые шаблоны',
'mostcategories'          => 'Страницы, включённые в большое количество категорий',
'mostimages'              => 'Самые используемые изображения',
'mostrevisions'           => 'Наиболее часто редактировавшиеся страницы',
'prefixindex'             => 'Указатель по началу слов',
'shortpages'              => 'Короткие статьи',
'longpages'               => 'Длинные страницы',
'deadendpages'            => 'Тупиковые страницы',
'deadendpagestext'        => 'Следующие страницы не содержат ссылок на другие страницы в этой вики.',
'protectedpages'          => 'Защищённые страницы',
'protectedpages-indef'    => 'Только бессрочные защиты',
'protectedpagestext'      => 'Следующие страницы защищены от переименования или изменения.',
'protectedpagesempty'     => 'В настоящий момент нет защищённых страниц с указанными параметрами',
'protectedtitles'         => 'Запрещённые названия',
'protectedtitlestext'     => 'Следующие названия не разрешается использовать',
'protectedtitlesempty'    => 'В настоящий момент нет запрещённых названий с указанными параметрами.',
'listusers'               => 'Список участников',
'newpages'                => 'Новые страницы',
'newpages-username'       => 'Участник:',
'ancientpages'            => 'Статьи по дате последнего редактирования',
'move'                    => 'Переименовать',
'movethispage'            => 'Переименовать эту страницу',
'unusedimagestext'        => 'Пожалуйста, учтите, что другие веб-сайты могут использовать прямую ссылку (URL) на это изображение, и поэтому изображение может активно использоваться несмотря на его вхождение в этот список.',
'unusedcategoriestext'    => 'Существуют следующие страницы категорий, не содержащие статей или других категорий.',
'notargettitle'           => 'Не указана цель',
'notargettext'            => 'Вы не указали целевую страницу или участника для этого действия.',
'nopagetitle'             => 'Нет такой целевой страницы',
'nopagetext'              => 'Указанной целевой страницы не существует.',
'pager-newer-n'           => '{{PLURAL:$1|более новая|более новые|более новых}} $1',
'pager-older-n'           => '{{PLURAL:$1|более старая|более старые|более старых}} $1',
'suppress'                => 'Сокрытие',

# Book sources
'booksources'               => 'Источники книг',
'booksources-search-legend' => 'Поиск информации о книге',
'booksources-go'            => 'Найти',
'booksources-text'          => 'На этой странице приведён список ссылок на сайты, где вы, возможно, найдёте дополнительную информацию о книге. Это интернет-магазины и системы поиска в библиотечных каталогах.',

# Special:Log
'specialloguserlabel'  => 'Участник:',
'speciallogtitlelabel' => 'Заголовок:',
'log'                  => 'Журналы',
'all-logs-page'        => 'Все журналы',
'log-search-legend'    => 'Поиск журналов',
'log-search-submit'    => 'Найти',
'alllogstext'          => 'Общий список журналов сайта {{SITENAME}}.
Вы можете отфильтровать результаты по типу журнала, имени участника (учитывается регистр) или затронутой странице (также учитывается регистр).',
'logempty'             => 'Подходящие записи в журнале отсутствуют.',
'log-title-wildcard'   => 'Найти заголовки, начинающиеся на с данных символов',

# Special:AllPages
'allpages'          => 'Все страницы',
'alphaindexline'    => 'от $1 до $2',
'nextpage'          => 'Следующая страница ($1)',
'prevpage'          => 'Предыдущая страница ($1)',
'allpagesfrom'      => 'Вывести страницы, начинающиеся на:',
'allarticles'       => 'Все страницы',
'allinnamespace'    => 'Все страницы в пространстве имён «$1»',
'allnotinnamespace' => 'Все страницы (кроме пространства имён «$1»)',
'allpagesprev'      => 'Предыдущие',
'allpagesnext'      => 'Следующие',
'allpagessubmit'    => 'Выполнить',
'allpagesprefix'    => 'Найти страницы, начинающиеся с:',
'allpagesbadtitle'  => 'Недопустимое название страницы. Заголовок содержит интервики, межъязыковой префикс или запрещённые в заголовках символы.',
'allpages-bad-ns'   => '{{SITENAME}} не содержит пространства имён «$1».',

# Special:Categories
'categories'                    => 'Категории',
'categoriespagetext'            => 'Следующие категории содержат страницы или медиа-файлы.
Здесь не показаны [[Special:UnusedCategories|Неиспользуемые категории]].
См. также [[Special:WantedCategories|список требуемых категорий]].',
'categoriesfrom'                => 'Показать категории, начинающиеся с:',
'special-categories-sort-count' => 'упорядочить по количеству',
'special-categories-sort-abc'   => 'упорядочить по алфавиту',

# Special:ListUsers
'listusersfrom'      => 'Показать участников, начиная с:',
'listusers-submit'   => 'Показать',
'listusers-noresult' => 'Не найдено участников.',

# Special:ListGroupRights
'listgrouprights'          => 'Права групп участников',
'listgrouprights-summary'  => 'Ниже представлен список определённых в этой вики групп участников, указаны соответствующие им права доступа.
Возможно, существует [[{{MediaWiki:Listgrouprights-helppage}}|дополнительная информация]] об индивидуальных правах.',
'listgrouprights-group'    => 'Группа',
'listgrouprights-rights'   => 'Права',
'listgrouprights-helppage' => 'Help:Права групп',
'listgrouprights-members'  => '(список группы)',

# E-mail user
'mailnologin'     => 'Адрес для отправки отсутствует',
'mailnologintext' => 'Вы должны [[Special:UserLogin|представиться системе]] и иметь действительный адрес электронной почты в ваших [[Special:Preferences|настройках]], чтобы иметь возможность отправлять электронную почту другим участникам.',
'emailuser'       => 'Письмо участнику',
'emailpage'       => 'Письмо участнику',
'emailpagetext'   => 'Если этот участник указал действительный адрес электронной почты в своих настройках, то, заполнив форму ниже, можно отправить ему сообщение.
Электронный адрес, который вы указали в [[Special:Preferences|своих настройках]], будет указан в поле письма «От кого», поэтому получатель будет иметь возможность ответить непосредственно вам.',
'usermailererror' => 'При отправке сообщения электронной почты произошла ошибка:',
'defemailsubject' => 'Письмо из {{grammar:genitive|{{SITENAME}}}}',
'noemailtitle'    => 'Адрес электронной почты отсутствует',
'noemailtext'     => 'Этот участник не указал действительный адрес электронной почты или указал, что не желает получать письма от других участников.',
'emailfrom'       => 'От кого:',
'emailto'         => 'Кому:',
'emailsubject'    => 'Тема:',
'emailmessage'    => 'Сообщение:',
'emailsend'       => 'Отправить',
'emailccme'       => 'Отправить мне копию письма.',
'emailccsubject'  => 'Копия вашего сообщения для $1: $2',
'emailsent'       => 'Письмо отправлено',
'emailsenttext'   => 'Ваше электронное сообщение отправлено.',
'emailuserfooter' => 'Это письмо было отправлено участнику $2 от участника $1 с помощью функции «Отправить письмо» проекта {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Список наблюдения',
'mywatchlist'          => 'Cписок наблюдения',
'watchlistfor'         => "(участника '''$1''')",
'nowatchlist'          => 'Ваш список наблюдения пуст.',
'watchlistanontext'    => 'Вы должны $1, чтобы просмотреть или отредактировать список наблюдения.',
'watchnologin'         => 'Нужно представиться системе',
'watchnologintext'     => 'Вы должны [[Special:UserLogin|представиться системе]], чтобы иметь возможность изменять свой список наблюдения',
'addedwatch'           => 'Добавлена в список наблюдения',
'addedwatchtext'       => 'Страница «[[:$1]]» была добавлена в ваш [[Special:Watchlist|список наблюдения]].
Последующие изменения этой страницы и связанной с ней страницы обсуждения будут отмечаться в этом списке, а также будут выделены жирным шрифтом на странице со [[Special:RecentChanges|списком свежих изменений]], чтобы их было легче заметить.',
'removedwatch'         => 'Удалена из списка наблюдения',
'removedwatchtext'     => 'Страница «[[:$1]]» была удалена из вашего списка наблюдения.',
'watch'                => 'Следить',
'watchthispage'        => 'Наблюдать за этой страницей',
'unwatch'              => 'Не следить',
'unwatchthispage'      => 'Прекратить наблюдение',
'notanarticle'         => 'Не статья',
'notvisiblerev'        => 'Версия была удалена',
'watchnochange'        => 'Ничто из списка наблюдения не изменялось в рассматриваемый период.',
'watchlist-details'    => 'В вашем списке наблюдения $1 {{PLURAL:$1|страница|страницы|страниц}}, не считая страниц обсуждения.',
'wlheader-enotif'      => '* Уведомление по эл. почте включено.',
'wlheader-showupdated' => "* Страницы, изменившиеся с вашего последнего их посещения, выделены '''жирным''' шрифтом.",
'watchmethod-recent'   => 'просмотр последних изменений для наблюдаемых страниц',
'watchmethod-list'     => 'просмотр наблюдаемых страниц для последних изменений',
'watchlistcontains'    => 'Ваш список наблюдения содержит $1 {{PLURAL:$1|страница|страницы|страниц}}.',
'iteminvalidname'      => 'Проблема с элементом «$1», недопустимое название…',
'wlnote'               => 'Ниже {{PLURAL:$1|следует последнее $1 изменение|следуют последние $1 изменения|следуют последние $1 изменений}} за {{PLURAL:$2|последний|последние|последние}} <strong>$2</strong> {{plural:$2|час|часа|часов}}.',
'wlshowlast'           => 'Показать за последние $1 часов $2 дней $3',
'watchlist-show-bots'  => 'Показать правки ботов',
'watchlist-hide-bots'  => 'Скрыть правки ботов',
'watchlist-show-own'   => 'Показать мои правки',
'watchlist-hide-own'   => 'Скрыть мои правки',
'watchlist-show-minor' => 'Показать малые правки',
'watchlist-hide-minor' => 'Скрыть малые правки',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Добавление в список наблюдения…',
'unwatching' => 'Удаление из списка наблюдения…',

'enotif_mailer'                => '{{SITENAME}} Служба извещений по почте',
'enotif_reset'                 => 'Отметить все страницы как просмотренные',
'enotif_newpagetext'           => 'Это новая страница.',
'enotif_impersonal_salutation' => 'Участник {{grammar:genitive|{{SITENAME}}}}',
'changed'                      => 'изменена',
'created'                      => 'создана',
'enotif_subject'               => 'Страница проекта «{{SITENAME}}» $PAGETITLE была $CHANGEDORCREATED участником $PAGEEDITOR',
'enotif_lastvisited'           => 'См. $1 для просмотра всех изменений, произошедших с вашего последнего посещения.',
'enotif_lastdiff'              => 'См. $1 для ознакомления с изменением.',
'enotif_anon_editor'           => 'анонимный участник $1',
'enotif_body'                  => '$WATCHINGUSERNAME,

$PAGEEDITDATE страница проекта «{{SITENAME}}» $PAGETITLE была $CHANGEDORCREATED участником $PAGEEDITOR, см. $PAGETITLE_URL для просмотра текущей версии.

$NEWPAGE

Краткое описание изменения: $PAGESUMMARY $PAGEMINOREDIT

Обратиться к изменившему:
эл. почта $PAGEEDITOR_EMAIL
вики $PAGEEDITOR_WIKI

Не будет никаких других уведомлений в случае дальнейших изменений, если Вы не посещаете эту страницу. Вы могли также повторно установить флаги уведомления для всех ваших наблюдаемых страниц в вашем списке наблюдения.

             Система оповещения {{grammar:genitive|{{SITENAME}}}}

--
Чтобы изменить настройки вашего списка наблюдения обратитесь к
{{fullurl:{{ns:special}}:Watchlist/edit}}

Обратная связь и помощь:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Удалить страницу',
'confirm'                     => 'Подтвердить',
'excontent'                   => 'содержимое: «$1»',
'excontentauthor'             => 'содержимое: «$1» (единственным автором был [[Special:Contributions/$2|$2]])',
'exbeforeblank'               => 'содержимое до очистки: «$1»',
'exblank'                     => 'страница была пуста',
'delete-confirm'              => '$1 — удаление',
'delete-legend'               => 'Удаление',
'historywarning'              => 'Предупреждение: у страницы, которую вы собираетесь удалить, есть история изменений:',
'confirmdeletetext'           => 'Вы запросили полное удаление страницы (или изображения) и всей её истории изменений из базы данных.
Пожалуйста, подтвердите, что вы действительно желаете это сделать, понимаете последствия своих действий,
и делаете это в соответствии с правилами, изложенными в разделе [[{{MediaWiki:Policy-url}}]].',
'actioncomplete'              => 'Действие выполнено',
'deletedtext'                 => '«<nowiki>$1</nowiki>» была удалена.
См. $2 для просмотра списка последних удалений.',
'deletedarticle'              => 'удалил «[[$1]]»',
'suppressedarticle'           => 'скрыл «[[$1]]»',
'dellogpage'                  => 'Журнал удалений',
'dellogpagetext'              => 'Ниже приведён журнал последних удалений.',
'deletionlog'                 => 'журнал удалений',
'reverted'                    => 'Откачено к ранней версии',
'deletecomment'               => 'Причина удаления:',
'deleteotherreason'           => 'Другая причина/дополнение:',
'deletereasonotherlist'       => 'Другая причина',
'deletereason-dropdown'       => '* Типовые причины удаления
** вандализм
** по запросу автора
** нарушение авторских прав',
'delete-edit-reasonlist'      => 'Править список причин',
'delete-toobig'               => 'У этой страницы очень длинная история изменений, более $1 {{PLURAL:$1|версии|версий|версий}}.
Удаление таких страниц было запрещено во избежание нарушений в работе сайта {{SITENAME}}.',
'delete-warning-toobig'       => 'У этой страницы очень длинная история изменений, более $1 {{PLURAL:$1|версии|версий|версий}}.
Её удаление может привести к нарушению нормальной работы базы данных сайта {{SITENAME}};
действуйте с осторожностью.',
'rollback'                    => 'Откатить изменения',
'rollback_short'              => 'Откат',
'rollbacklink'                => 'откатить',
'rollbackfailed'              => 'Ошибка при совершении отката',
'cantrollback'                => 'Невозможно откатить изменения; последний, кто вносил изменения, является единственным автором этой статьи.',
'alreadyrolled'               => 'Невозможно откатить последние изменения [[:$1]], сделанные [[User:$2|$2]] ([[User talk:$2|Обсуждение]] | [[Special:Contributions/$2|{{int:contribslink}}]]);
кто-то другой уже отредактировал или откатил эту страницу.

Последние изменения внёс [[User:$3|$3]] ([[User talk:$3|Обсуждение]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Изменение было пояснено так: <i>«$1»</i>.', # only shown if there is an edit comment
'revertpage'                  => 'Правки [[Special:Contributions/$2|$2]] ([[User talk:$2|обсуждение]]) откачены к версии [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Откачены правки $1; возврат к версии $2.',
'sessionfailure'              => 'Похоже, возникли проблемы с текущим сеансом работы;
это действие было отменено в целях предотвращения «захвата сеанса».
Пожалуйста, нажмите кнопку «Назад» и перезагрузите страницу, с которой вы пришли.',
'protectlogpage'              => 'Журнал защиты',
'protectlogtext'              => 'Ниже приведён журнал установок и снятий защиты со статей. Вы можете также просмотреть [[Special:ProtectedPages|список страниц, которые в данный момент защищены]].',
'protectedarticle'            => 'защищена страница «[[$1]]»',
'modifiedarticleprotection'   => 'изменён уровень защиты страницы «[[$1]]»',
'unprotectedarticle'          => 'снята защита со страницы «[[$1]]»',
'protect-title'               => 'Установка уровня защиты для «$1»',
'protect-legend'              => 'Подтвердите установку защиты страницы',
'protectcomment'              => 'Причина установки защиты:',
'protectexpiry'               => 'Истекает:',
'protect_expiry_invalid'      => 'Неправильное время окончания защиты.',
'protect_expiry_old'          => 'Время окончания — в прошлом.',
'protect-unchain'             => 'Разблокировать переименование страницы',
'protect-text'                => 'Здесь вы можете просмотреть и изменить уровень защиты для страницы <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Вы не можете изменять уровень защиты страницы, пока ваша учётная запись заблокирована.
Текущие установки для страницы <strong>$1</strong>:',
'protect-locked-dblock'       => 'Уровень защиты не может быть изменён, так как основная база данных временно заблокирована.
Текущие установки для страницы <strong>$1</strong>:',
'protect-locked-access'       => 'У вашей учётной записи недостаточно прав для изменения уровня защиты страницы. Текущие установки для страницы <strong>$1</strong>:',
'protect-cascadeon'           => 'Эта страница защищена в связи с тем, что она включена {{PLURAL:$1|в указанную ниже страницу, на которую|в нижеследующие страницы, на которые}} установлена каскадная защита. Вы можете изменить уровень защиты этой страницы, но это не повлияет на каскадную защиту.',
'protect-default'             => '(по умолчанию)',
'protect-fallback'            => 'Требуется разрешение «$1»',
'protect-level-autoconfirmed' => 'Защитить от незарегистрированных и новых участников',
'protect-level-sysop'         => 'Только администраторы',
'protect-summary-cascade'     => 'каскадная',
'protect-expiring'            => 'истекает $1 (UTC)',
'protect-cascade'             => 'Защищать страницы, включённые в эту страницу (каскадная защита)',
'protect-cantedit'            => 'Вы не можете изменить уровень защиты этой страницы, потому что у вас нет прав для её редактирования.',
'restriction-type'            => 'Права:',
'restriction-level'           => 'Уровень доступа:',
'minimum-size'                => 'Минимальный размер',
'maximum-size'                => 'Максимальный размер:',
'pagesize'                    => '(байт)',

# Restrictions (nouns)
'restriction-edit'   => 'Редактирование',
'restriction-move'   => 'Переименование',
'restriction-create' => 'Создание',
'restriction-upload' => 'Загрузка',

# Restriction levels
'restriction-level-sysop'         => 'полная защита',
'restriction-level-autoconfirmed' => 'частичная защита',
'restriction-level-all'           => 'все уровни',

# Undelete
'undelete'                     => 'Просмотреть удалённые страницы',
'undeletepage'                 => 'Просмотр и восстановление удалённых страниц',
'undeletepagetitle'            => "'''Ниже перечислены удалённые версии страницы [[:$1]]'''.",
'viewdeletedpage'              => 'Просмотреть удалённые страницы',
'undeletepagetext'             => 'Следующие страницы были удалены, однако они всё ещё находятся в архиве, и поэтому могут быть восстановлены. Архив периодически очищается.',
'undelete-fieldset-title'      => 'Восстановить версии',
'undeleteextrahelp'            => "Для полного восстановления истории страницы оставьте все отметки пустыми и нажмите '''«Восстановить»'''.
Для частичного восстановления отметьте те версии страницы, которые нужно восстановить, и нажмите '''«Восстановить»'''.
Нажмите '''«Очистить»''', чтобы снять все отметки и очистить поле примечания.",
'undeleterevisions'            => 'в архиве $1 {{PLURAL:$1|версия|версии|версий}}',
'undeletehistory'              => 'При восстановлении страницы восстанавливается и её история правок.
Если после удаления была создана новая страница с тем же названием, то восстановленные версии появятся в истории правок перед новыми версиями.',
'undeleterevdel'               => 'Восстановление не будет произведено, если оно приведёт к частичному удалению последней версии страницы или файла.
В подобном случае вы должны снять отметку или показать последние удалённые версии.',
'undeletehistorynoadmin'       => 'Статья была удалена. Причина удаления и список участников, редактировавших статью до её удаления, показаны ниже. Текст удалённой статьи могут просмотреть только администраторы.',
'undelete-revision'            => 'Удалённая версия $1 (от $2) участника $3:',
'undeleterevision-missing'     => 'Неверная или отсутствующая версия. Возможно, вы перешли по неправильной ссылке, либо версия могла быть удалена из архива.',
'undelete-nodiff'              => 'Не найдено предыдущей версии.',
'undeletebtn'                  => 'Восстановить',
'undeletelink'                 => 'восстановить',
'undeletereset'                => 'Очистить',
'undeletecomment'              => 'Комментарий:',
'undeletedarticle'             => 'восстановил «[[$1]]»',
'undeletedrevisions'           => '$1 {{PLURAL:$1|изменение|изменения|изменений}} восстановлено',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|версия|версии|версий}} и $2 {{PLURAL:$2|файл|файла|файлов}} восстановлено',
'undeletedfiles'               => '$1 {{PLURAL:$1|файл восстановлен|файла восстановлено|файлов восстановлено}}',
'cannotundelete'               => 'Ошибка восстановления. Возможно, кто-то другой уже восстановил страницу.',
'undeletedpage'                => "<big>'''Страница «$1» была восстановлена.'''</big>

Для просмотра списка последних удалений и восстановлений см. [[Special:Log/delete|журнал удалений]].",
'undelete-header'              => 'Список недавно удалённых страниц можно посмотреть в [[Special:Log/delete|журнале удалений]].',
'undelete-search-box'          => 'Поиск удалённых страниц',
'undelete-search-prefix'       => 'Показать страницы, начинающиеся с:',
'undelete-search-submit'       => 'Найти',
'undelete-no-results'          => 'Не найдено соответствующих страниц в архиве удалений.',
'undelete-filename-mismatch'   => 'Невозможно восстановить версию файла с отметкой времени $1: несоответствие имени файла',
'undelete-bad-store-key'       => 'Невозможно восстановить версию файла с отметкой времени $1: файл отсутствовал до удаления.',
'undelete-cleanup-error'       => 'Ошибка удаления неиспользуемого архивного файла «$1».',
'undelete-missing-filearchive' => 'Невозможно восстановить файл с архивным идентификатором $1, так как он отсутствует в базе данных. Возможно, файл уже был восстановлен.',
'undelete-error-short'         => 'Ошибка восстановления файла: $1',
'undelete-error-long'          => 'Во время восстановления файла возникли ошибки:

$1',

# Namespace form on various pages
'namespace'      => 'Пространство имён:',
'invert'         => 'Обратить выделенное',
'blanknamespace' => '(Основное)',

# Contributions
'contributions' => 'Вклад участника',
'mycontris'     => 'Мой вклад',
'contribsub2'   => 'Вклад $1 ($2)',
'nocontribs'    => 'Изменений, соответствующих заданным условиям, найдено не было.',
'uctop'         => ' (последняя)',
'month'         => 'С месяца (и ранее):',
'year'          => 'С года (и ранее):',

'sp-contributions-newbies'     => 'Показать только вклад, сделанный с новых учётных записей',
'sp-contributions-newbies-sub' => 'С новых учётных записей',
'sp-contributions-blocklog'    => 'Журнал блокировок',
'sp-contributions-search'      => 'Поиск вклада',
'sp-contributions-username'    => 'IP-адрес или имя участника:',
'sp-contributions-submit'      => 'Найти',

# What links here
'whatlinkshere'            => 'Ссылки сюда',
'whatlinkshere-title'      => 'Страницы, ссылающиеся на «$1»',
'whatlinkshere-page'       => 'Страница:',
'linklistsub'              => '(Список ссылок)',
'linkshere'                => "Следующие страницы ссылаются на '''[[:$1]]''':",
'nolinkshere'              => "На страницу '''[[:$1]]''' отсутствуют ссылки с других страниц.",
'nolinkshere-ns'           => "В выбранном пространстве имён нет страниц, ссылающихся на '''[[:$1]]'''.",
'isredirect'               => 'страница-перенаправление',
'istemplate'               => 'включение',
'isimage'                  => 'ссылка с изображения',
'whatlinkshere-prev'       => '{{PLURAL:$1|предыдущая|предыдущие|предыдущие}} $1',
'whatlinkshere-next'       => '{{PLURAL:$1|следующая|следующие|следующие}} $1',
'whatlinkshere-links'      => '← ссылки',
'whatlinkshere-hideredirs' => '$1 перенаправления',
'whatlinkshere-hidetrans'  => '$1 включения',
'whatlinkshere-hidelinks'  => '$1 ссылки',
'whatlinkshere-hideimages' => '$1 ссылки с изображений',
'whatlinkshere-filters'    => 'Фильтры',

# Block/unblock
'blockip'                         => 'Заблокировать',
'blockip-legend'                  => 'Блокировка участника',
'blockiptext'                     => 'Используйте форму ниже, чтобы заблокировать возможность записи с определённого IP-адреса.
Это может быть сделано только для предотвращения вандализма и только в соответствии с [[{{MediaWiki:Policy-url}}|правилами]].
Ниже укажите конкретную причину (к примеру, процитируйте некоторые страницы с признаками вандализма).',
'ipaddress'                       => 'IP-адрес:',
'ipadressorusername'              => 'IP-адрес или имя участника:',
'ipbexpiry'                       => 'Закончится через:',
'ipbreason'                       => 'Причина:',
'ipbreasonotherlist'              => 'Другая причина',
'ipbreason-dropdown'              => '
* Стандартные причины блокировок
** Вставка ложной информации
** Удаление содержимого страниц
** Спам-ссылки на внешние сайты
** Добавление бессмысленного текста/мусора
** Угрозы, преследование участников
** Злоупотребление несколькими учётными записями
** Неприемлемое имя участника',
'ipbanononly'                     => 'Блокировать только анонимных участников',
'ipbcreateaccount'                => 'Запретить создание новых учётных записей',
'ipbemailban'                     => 'Запретить участнику отправлять письма по электронной почте',
'ipbenableautoblock'              => 'Автоматически блокировать используемые участником IP-адреса',
'ipbsubmit'                       => 'Заблокировать этот адрес/участника',
'ipbother'                        => 'Другое время:',
'ipboptions'                      => '15 минут:15 minutes,2 часа:2 hours,6 часов:6 hours,12 часов:12 hours,1 день:1 day,3 дня:3 days,1 неделю:1 week,2 недели:2 weeks,1 месяц:1 month,3 месяца:3 months,6 месяцев:6 months,1 год:1 year,бессрочно:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'другое',
'ipbotherreason'                  => 'Другая причина/дополнение:',
'ipbhidename'                     => 'Скрыть имя участника или IP-адрес из журнала блокировок, списка заблокированных и общего списка участников.',
'ipbwatchuser'                    => 'Добавить в список наблюдения личную страницу участника и его страницу обсуждения',
'badipaddress'                    => 'IP-адрес записан в неправильном формате, или участника с таким именем не существует.',
'blockipsuccesssub'               => 'Блокировка произведена',
'blockipsuccesstext'              => '[[Special:Contributions/$1|«$1»]] заблокирован.<br />
См. [[Special:IPBlockList|список заблокированных IP-адресов]].',
'ipb-edit-dropdown'               => 'Править список причин',
'ipb-unblock-addr'                => 'Разблокировать $1',
'ipb-unblock'                     => 'Разблокировать участника или IP-адрес',
'ipb-blocklist-addr'              => 'Показать действующие блокировки для $1',
'ipb-blocklist'                   => 'Показать действующие блокировки',
'unblockip'                       => 'Разблокировать IP-адрес',
'unblockiptext'                   => 'Используйте форму ниже, чтобы восстановить возможность записи с ранее заблокированного IP-адреса или учётной записи.',
'ipusubmit'                       => 'Разблокировать этот адрес',
'unblocked'                       => '[[User:$1|$1]] разблокирован.',
'unblocked-id'                    => 'Блокировка $1 была снята',
'ipblocklist'                     => 'Заблокированные IP-адреса и учётные записи',
'ipblocklist-legend'              => 'Поиск заблокированного участника',
'ipblocklist-username'            => 'Имя участника или IP-адрес:',
'ipblocklist-submit'              => 'Найти',
'blocklistline'                   => '$1, $2 заблокировал $3 ($4)',
'infiniteblock'                   => 'бессрочная блокировка',
'expiringblock'                   => 'блокировка завершится $1',
'anononlyblock'                   => 'только анонимов',
'noautoblockblock'                => 'автоблокировка отключена',
'createaccountblock'              => 'создание учётных записей заблокировано',
'emailblock'                      => 'отправка писем запрещена',
'ipblocklist-empty'               => 'Список блокировок пуст.',
'ipblocklist-no-results'          => 'Заданный IP-адрес или имя участника не заблокированы.',
'blocklink'                       => 'заблокировать',
'unblocklink'                     => 'разблокировать',
'contribslink'                    => 'вклад',
'autoblocker'                     => 'Автоблокировка из-за совпадения вашего IP-адреса с $1. Причина блокировки адреса — «$2».',
'blocklogpage'                    => 'Журнал блокировок',
'blocklogentry'                   => 'заблокировал [[$1]] на период $2 $3',
'blocklogtext'                    => 'Журнал блокирования и разблокирования участников.
Автоматически блокируемые IP-адреса здесь не указываются.
См. [[Special:IPBlockList|Список текущих запретов и блокировок]].',
'unblocklogentry'                 => 'разблокировал $1',
'block-log-flags-anononly'        => 'только анонимные пользователи',
'block-log-flags-nocreate'        => 'запрещена регистрация учётных записей',
'block-log-flags-noautoblock'     => 'автоблокировка отключена',
'block-log-flags-noemail'         => 'отправка писем запрещена',
'block-log-flags-angry-autoblock' => 'включён расширенный автоблок',
'range_block_disabled'            => 'Администраторам запрещено блокировать диапазоны.',
'ipb_expiry_invalid'              => 'Недопустимый период действия.',
'ipb_expiry_temp'                 => 'Блокировки с сокрытием имени участника должны быть бессрочными.',
'ipb_already_blocked'             => '«$1» уже заблокирован.',
'ipb_cant_unblock'                => 'Ошибка. Не найдена блокировка с ID $1. Возможно, она уже была снята.',
'ipb_blocked_as_range'            => 'Ошибка: IP-адрес $1 был заблокирован не напрямую и не может быть разблокирован. Однако, он принадлежит к заблокированному диапазону $2, который можно разблокировать.',
'ip_range_invalid'                => 'Недопустимый диапазон IP-адресов.',
'blockme'                         => 'Заблокируй меня',
'proxyblocker'                    => 'Блокировка прокси',
'proxyblocker-disabled'           => 'Функция отключена.',
'proxyblockreason'                => 'Ваш IP-адрес заблокирован потому что это открытый прокси. Пожалуйста, свяжитесь с вашим интернет-провайдером  или службой поддержки и сообщите им об этой серьёзной проблеме безопасности.',
'proxyblocksuccess'               => 'Выполнено.',
'sorbsreason'                     => 'Ваш IP-адрес числится как открытый прокси в DNSBL.',
'sorbs_create_account_reason'     => 'Ваш IP-адрес числится как открытый прокси в DNSBL. Вы не можете создать учётную запись.',

# Developer tools
'lockdb'              => 'Сделать базу данных доступной только для чтения',
'unlockdb'            => 'Восстановить возможность записи в базу данных',
'lockdbtext'          => 'Блокировка базы данных приостановит для всех участников возможность редактировать страницы, изменять настройки,
изменять списки наблюдения и производить другие действия, требующие доступа к базе данных.
Пожалуйста, подтвердите, что это — именно то, что вы хотите сделать, и что вы снимете блокировку как только закончите
процедуру обслуживания базы данных.',
'unlockdbtext'        => 'Разблокирование базы данных восстановит для всех участников
возможность редактировать страницы, изменять настройки, изменять списки наблюдения и производить
другие действия, требующие доступа к базе данных.
Пожалуйста, подтвердите, что вы намерены это сделать.',
'lockconfirm'         => 'Да, я действительно хочу заблокировать базу данных на запись.',
'unlockconfirm'       => 'Да, я действительно хочу снять блокировку базы данных.',
'lockbtn'             => 'Сделать базу данных доступной только для чтения',
'unlockbtn'           => 'Восстановить возможность записи в базу данных',
'locknoconfirm'       => 'Вы не поставили галочку в поле подтверждения.',
'lockdbsuccesssub'    => 'База данных заблокирована',
'unlockdbsuccesssub'  => 'База данных разблокирована',
'lockdbsuccesstext'   => 'База данных проекта была заблокирована.<br />
Не забудьте [[Special:UnlockDB|убрать блокировку]] после завершения процедуры обслуживания.',
'unlockdbsuccesstext' => 'База данных проекта была разблокирована.',
'lockfilenotwritable' => 'Нет права на запись в файл блокировки базы данных. Чтобы заблокировать или разблокировать БД, веб-сервер должен иметь разрешение на запись в этот файл.',
'databasenotlocked'   => 'База данных не была заблокирована.',

# Move page
'move-page'               => '$1 — переименование',
'move-page-legend'        => 'Переименование страницы',
'movepagetext'            => "Воспользовавшись формой ниже, вы переименуете страницу, одновременно переместив на новое место её журнал изменений.
Старое название станет перенаправлением на новое название.
Вы можете автоматически обновить перенаправления, которые вели на старое название.
Если вы этого не сделаете, пожалуйста, проверьте наличие [[Special:DoubleRedirects|двойных]] и [[Special:BrokenRedirects|разорванных перенаправлений]].
Вы отвечаете за то, что бы ссылки продолжали и далее указывают туда, куда предполагалось.

Обратите внимание, что страница '''не будет''' переименована, если страница с новым названием уже существует, кроме случаев, если она является перенаправлением или пуста и не имеет истории правок.
Это означает, что вы можете переименовать страницу обратно в то название, которое у него только что было, если вы переименовали по ошибке, но вы не можете случайно затереть существующую страницу.

'''ПРЕДУПРЕЖДЕНИЕ!'''
Переименование может привести к масштабным и неожиданным изменениям для ''популярных'' страниц.
Пожалуйста, прежде чем вы продолжите, убедитесь, что вы понимаете все возможные последствия.",
'movepagetalktext'        => "Присоединённая страница обсуждения будет также автоматически переименована, '''кроме случаев, когда:'''

*Не пустая страница обсуждения уже существует под таким же именем или
*Вы не поставили галочку в поле ниже.

В этих случаях, вы будете вынуждены переместить или объединить страницы вручную, если это нужно.",
'movearticle'             => 'Переименовать страницу',
'movenotallowed'          => 'У вас нет разрешения переименовывать страницы.',
'newtitle'                => 'Новое название',
'move-watch'              => 'Включить эту страницу в список наблюдения',
'movepagebtn'             => 'Переименовать страницу',
'pagemovedsub'            => 'Страница переименована',
'movepage-moved'          => "<big>'''Страница «$1» переименована в «$2»'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Страница с таким именем уже существует или указанное вами название недопустимо.
Пожалуйста, выберите другое название.',
'cantmove-titleprotected' => 'Невозможно переименовать страницу, так как новое название входит в список запрещённых.',
'talkexists'              => "'''Страница была переименована, но страница обсуждения не может быть переименована, потому что страница с таким названием уже существует. Пожалуйста, объедините их вручную.'''",
'movedto'                 => 'переименована в',
'movetalk'                => 'Переименовать соответствующую страницу обсуждения',
'move-subpages'           => 'Переименовать все подстраницы, если возможно',
'move-talk-subpages'      => 'Переименовать все подстраницы страницы обсуждения, если это возможно',
'movepage-page-exists'    => 'Страница $1 уже существует и не может быть автоматически перезаписана.',
'movepage-page-moved'     => 'Страница $1 была переименована в $2.',
'movepage-page-unmoved'   => 'Страница $1 не может быть переименована в $2.',
'movepage-max-pages'      => '$1 {{PLURAL:$1|страница была переименована|страницы было переименовано|страниц было переименовано}} — это максимум, больше страниц нельзя переименовать автоматически.',
'1movedto2'               => '«[[$1]]» переименована в «[[$2]]»',
'1movedto2_redir'         => '«[[$1]]» переименована в «[[$2]]» поверх перенаправления',
'movelogpage'             => 'Журнал переименований',
'movelogpagetext'         => 'Ниже представлен список переименованных страниц.',
'movereason'              => 'Причина',
'revertmove'              => 'откат',
'delete_and_move'         => 'Удалить и переименовать',
'delete_and_move_text'    => '==Требуется удаление==

Страница с именем [[:$1|«$1»]] уже существует. Вы хотите её удалить, чтобы сделать возможным переименование?',
'delete_and_move_confirm' => 'Да, удалить эту страницу',
'delete_and_move_reason'  => 'Удалено для возможности переименования',
'selfmove'                => 'Невозможно переименовать страницу: исходное и новое имя страницы совпадают.',
'immobile_namespace'      => 'Невозможно переименовать страницу: новое или старое имя содержит зарезервированное служебное слово.',
'imagenocrossnamespace'   => 'Невозможно дать изображению имя из другого пространства имён',
'imagetypemismatch'       => 'Новое расширение файла не соответствует его типу',
'imageinvalidfilename'    => 'Целевое имя файла ошибочно',
'fix-double-redirects'    => 'Автоматически исправить перенаправления, указывающие на прежнее название',

# Export
'export'            => 'Экспортирование статей',
'exporttext'        => 'Вы можете экспортировать текст и журнал изменений конкретной страницы или набора страниц в XML, который потом может быть [[Special:Import|импортирован]] в другой вики-проект, работающий на программном обеспечении MediaWiki.

Чтобы экспортировать статьи, введите их наименования в поле редактирования, одно название на строку, и выберите хотите ли вы экспортировать всю историю изменений статей или только последние версии статей.

Вы также можете использовать специальный адрес для экспорта только последней версии. Например для страницы [[{{MediaWiki:Mainpage}}]] это будет адрес [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]].',
'exportcuronly'     => 'Включать только текущую версию, без полной предыстории',
'exportnohistory'   => "----
'''Замечание:''' экспорт полной истории изменений страниц отключен из-за проблем с производительностью.",
'export-submit'     => 'Экспортировать',
'export-addcattext' => 'Добавить страницы из категории:',
'export-addcat'     => 'Добавить',
'export-download'   => 'Предложить сохранить как файл',
'export-templates'  => 'Включить шаблоны',

# Namespace 8 related
'allmessages'               => 'Системные сообщения',
'allmessagesname'           => 'Сообщение',
'allmessagesdefault'        => 'Текст по умолчанию',
'allmessagescurrent'        => 'Текущий текст',
'allmessagestext'           => 'Это список системных сообщений, доступных в пространстве имён «MediaWiki».
Пожалуйста, посетите на страницу [http://www.mediawiki.org/wiki/Localisation описания локализации] и проект [http://translatewiki.net Betawiki], если вы хотите внести вклад в общую локализацию MediaWiki.',
'allmessagesnotsupportedDB' => "Эта страница недоступна, так как отключена опция '''\$wgUseDatabaseMessages'''.",
'allmessagesfilter'         => 'Фильтр в формате регулярного выражения:',
'allmessagesmodified'       => 'Показать только изменённые',

# Thumbnails
'thumbnail-more'           => 'Увеличить',
'filemissing'              => 'Файл не найден',
'thumbnail_error'          => 'Ошибка создания миниатюры: $1',
'djvu_page_error'          => 'Номер страницы DjVu вне досягаемости',
'djvu_no_xml'              => 'Невозможно получить XML для DjVu',
'thumbnail_invalid_params' => 'Ошибочный параметр миниатюры',
'thumbnail_dest_directory' => 'Невозможно создать целевую директорию',

# Special:Import
'import'                     => 'Импортирование страниц',
'importinterwiki'            => 'Межвики импорт',
'import-interwiki-text'      => 'Укажите вики и название импортируемой страницы.
Даты изменений и имена авторов будут сохранены.
Все операции межвики импорта регистрируются в [[Special:Log/import|соответствующем журнале]].',
'import-interwiki-history'   => 'Копировать всю историю изменений этой страницы',
'import-interwiki-submit'    => 'Импортировать',
'import-interwiki-namespace' => 'Помещать страницы в пространство имён:',
'importtext'                 => 'Пожалуйста, экспортируйте страницу из исходной вики, используя [[Special:Export|соответствующий инструмент]]. Сохраните файл на диск, а затем загрузите его сюда.',
'importstart'                => 'Импортирование страниц…',
'import-revision-count'      => '$1 {{PLURAL:$1|версия|версии|версий}}',
'importnopages'              => 'Нет страниц для импортирования.',
'importfailed'               => 'Не удалось импортировать: $1',
'importunknownsource'        => 'Неизвестный тип импортируемой страницы',
'importcantopen'             => 'Невозможно открыть импортируемый файл',
'importbadinterwiki'         => 'Неправильная интервики-ссылка',
'importnotext'               => 'Текст отсутствует',
'importsuccess'              => 'Импортировано выполнено!',
'importhistoryconflict'      => 'Конфликт существующих версий (возможно, эта страница уже была импортирована)',
'importnosources'            => 'Не был выбран источник межвики-импорта, прямая загрузка истории изменений отключена.',
'importnofile'               => 'Файл для импорта не был загружен.',
'importuploaderrorsize'      => 'Не удалось загрузить или импортировать файл. Размер файла превышает установленный предел.',
'importuploaderrorpartial'   => 'Не удалось загрузить или импортировать файл. Он был загружен лишь частично.',
'importuploaderrortemp'      => 'Не удалось загрузить или импортировать файл. Временная папка отсутствует.',
'import-parse-failure'       => 'Ошибка разбора XML при импорте',
'import-noarticle'           => 'Нет страницы для импортирования!',
'import-nonewrevisions'      => 'Все редакции были ранее импортированы.',
'xml-error-string'           => '$1 в строке $2, позиции $3 (байт $4): $5',
'import-upload'              => 'Загрузить XML-данные',

# Import log
'importlogpage'                    => 'Журнал импорта',
'importlogpagetext'                => 'Импортирование администраторами страниц с историей изменений из других вики.',
'import-logentry-upload'           => '«[[$1]]» — импорт из файла',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|версия|версии|версий}}',
'import-logentry-interwiki'        => '«$1» — межвики импорт',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|версия|версии|версий}} из $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Моя страница участника',
'tooltip-pt-anonuserpage'         => 'Страница участника для моего IP',
'tooltip-pt-mytalk'               => 'Моя страница обсуждений',
'tooltip-pt-anontalk'             => 'Страница обсуждений для моего IP',
'tooltip-pt-preferences'          => 'Мои настройки',
'tooltip-pt-watchlist'            => 'Список страниц моего наблюдения',
'tooltip-pt-mycontris'            => 'Мой вклад',
'tooltip-pt-login'                => 'Здесь можно зарегистрироваться в системе, но это необязательно.',
'tooltip-pt-anonlogin'            => 'Здесь можно зарегистрироваться в системе, но это необязательно.',
'tooltip-pt-logout'               => 'Завершить зарегистрированный сеанс',
'tooltip-ca-talk'                 => 'Обсуждение содержания страницы',
'tooltip-ca-edit'                 => 'Эту страницу можно изменять. Используйте, пожалуйста, предварительный просмотр перед сохранением',
'tooltip-ca-addsection'           => 'Добавить комментарий к обсуждению',
'tooltip-ca-viewsource'           => 'Эта страница защищена от изменений, но вы можете посмотреть и скопировать её исходный текст',
'tooltip-ca-history'              => 'Журнал изменений страницы',
'tooltip-ca-protect'              => 'Защитить страницу от изменений',
'tooltip-ca-delete'               => 'Удалить эту страницу',
'tooltip-ca-undelete'             => 'Восстановить исправления страницы, сделанные до того, как она была удалена',
'tooltip-ca-move'                 => 'Переименовать страницу',
'tooltip-ca-watch'                => 'Добавить эту страницу в ваш список наблюдения',
'tooltip-ca-unwatch'              => 'Удалить эту страницу из вашего списка наблюдения',
'tooltip-search'                  => 'Искать это слово',
'tooltip-search-go'               => 'Перейти к странице, имеющей в точности такое название',
'tooltip-search-fulltext'         => 'Найти страницы, содержащие указанный текст',
'tooltip-p-logo'                  => 'Заглавная страница',
'tooltip-n-mainpage'              => 'Перейти на заглавную страницу',
'tooltip-n-portal'                => 'О проекте, о том, что вы можете сделать, где что находится',
'tooltip-n-currentevents'         => 'Список текущих событий',
'tooltip-n-recentchanges'         => 'Список последних изменений',
'tooltip-n-randompage'            => 'Посмотреть случайную страницу',
'tooltip-n-help'                  => 'Справочник по проекту «{{SITENAME}}»',
'tooltip-t-whatlinkshere'         => 'Список всех страниц, которые ссылаются на эту страницу',
'tooltip-t-recentchangeslinked'   => 'Последние изменения в страницах, на которые ссылается эта страница',
'tooltip-feed-rss'                => 'Трансляция в RSS для этой страницы',
'tooltip-feed-atom'               => 'Трансляция в Atom для этой страницы',
'tooltip-t-contributions'         => 'Список страниц, которые изменял этот участник',
'tooltip-t-emailuser'             => 'Отправить письмо этому участнику',
'tooltip-t-upload'                => 'Загрузить изображения или мультимедиа-файлы',
'tooltip-t-specialpages'          => 'Список служебных страниц',
'tooltip-t-print'                 => 'Версия для печати этой страницы',
'tooltip-t-permalink'             => 'Постоянная ссылка на эту версию страницы',
'tooltip-ca-nstab-main'           => 'Содержание статьи',
'tooltip-ca-nstab-user'           => 'Персональная страница участника',
'tooltip-ca-nstab-media'          => 'Медиа-файл',
'tooltip-ca-nstab-special'        => 'Это служебная страница, она недоступна для редактирования',
'tooltip-ca-nstab-project'        => 'Страница проекта',
'tooltip-ca-nstab-image'          => 'Страница изображения',
'tooltip-ca-nstab-mediawiki'      => 'Страница сообщения MediaWiki',
'tooltip-ca-nstab-template'       => 'Страница шаблона',
'tooltip-ca-nstab-help'           => 'Страница справки',
'tooltip-ca-nstab-category'       => 'Страница категории',
'tooltip-minoredit'               => 'Отметить это изменение как незначительное',
'tooltip-save'                    => 'Сохранить ваши изменения',
'tooltip-preview'                 => 'Предварительный просмотр страницы, пожалуйста, используйте перед сохранением!',
'tooltip-diff'                    => 'Показать изменения, сделанные по отношению к исходному тексту.',
'tooltip-compareselectedversions' => 'Посмотреть разницу между двумя выбранными версиями этой страницы.',
'tooltip-watch'                   => 'Добавить текущую страницу в список наблюдения',
'tooltip-recreate'                => 'Восстановить страницу несмотря на то, что она была удалена',
'tooltip-upload'                  => 'Начать загрузку',

# Stylesheets
'common.css'   => '/** Размещённый здесь CSS будет применяться ко всем темам оформления */',
'monobook.css' => '/* Размещённый здесь CSS будет применяться к теме оформления Monobook */

/*
Это нужно чтобы в окошке поиска кнопки не разбивались на 2 строки
к сожалению в main.css для кнопки Go прописаны паддинги .5em.
Что хорошо для "Go" плохо для "Перейти" --st0rm
*/

#searchGoButton {
    padding-left: 0em;
    padding-right: 0em;
    font-weight: bold;
}',

# Scripts
'common.js'   => '/* Размещённый здесь код JavaScript будет загружен всем пользователям при обращении к какой-либо странице */',
'monobook.js' => '/* Указанный здесь JavaScript будет загружен всем участникам, использующим тему оформления MonoBook  */',

# Metadata
'nodublincore'      => 'Метаданные Dublin Core RDF запрещены для этого сервера.',
'nocreativecommons' => 'Метаданные Creative Commons RDF запрещены для этого сервера.',
'notacceptable'     => "Вики-сервер не может предоставить данные в формате, который мог бы прочитать ваш браузер.<br />
The wiki server can't provide data in a format your client can read.",

# Attribution
'anonymous'        => 'Анонимные пользователи {{grammar:genitive|{{SITENAME}}}}',
'siteuser'         => 'Участник {{grammar:genitive|{{SITENAME}}}} $1',
'lastmodifiedatby' => 'Эта страница последний раз была изменена $2, $1 участником $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Основано на работе $1.',
'others'           => 'другие',
'siteusers'        => 'Участник(и) {{grammar:genitive|{{SITENAME}}}} $1',
'creditspage'      => 'Благодарности',
'nocredits'        => 'Нет списка участников для этой статьи',

# Spam protection
'spamprotectiontitle' => 'Спам-фильтр',
'spamprotectiontext'  => 'Страница, которую вы пытаетесь сохранить, заблокирована спам-фильтром.
Вероятно, это произошло из-за того, что она содержит ссылку на занесённый в чёрный список внешний сайт.',
'spamprotectionmatch' => 'Следующее сообщение было получено от спам-фильтра: $1.',
'spambot_username'    => 'Чистка спама',
'spam_reverting'      => 'Откат к последней версии, не содержащей ссылки на $1',
'spam_blanking'       => 'Все версии содержат ссылки на $1, очистка',

# Info page
'infosubtitle'   => 'Информация о странице',
'numedits'       => 'Число правок (статья): $1',
'numtalkedits'   => 'Число правок (страница обсуждения): $1',
'numwatchers'    => 'Число наблюдателей: $1',
'numauthors'     => 'Число различных авторов (статья): $1',
'numtalkauthors' => 'Число различных авторов (страница обсуждения): $1',

# Math options
'mw_math_png'    => 'Всегда генерировать PNG',
'mw_math_simple' => 'HTML в простых случаях, иначе PNG',
'mw_math_html'   => 'HTML, если возможно, иначе PNG',
'mw_math_source' => 'Оставить в разметке ТеХ (для текстовых браузеров)',
'mw_math_modern' => 'Как рекомендуется для современных браузеров',
'mw_math_mathml' => 'MathML, если возможно (экспериментальная опция)',

# Patrolling
'markaspatrolleddiff'                 => 'Отметить как проверенную',
'markaspatrolledtext'                 => 'Отметить эту статью как проверенную',
'markedaspatrolled'                   => 'Отмечена как проверенная',
'markedaspatrolledtext'               => 'Выбранная версия отмечена как проверенная.',
'rcpatroldisabled'                    => 'Патрулирование последних изменений запрещено',
'rcpatroldisabledtext'                => 'Возможность патрулирования последних изменений в настоящее время отключена.',
'markedaspatrollederror'              => 'Невозможно отметить как проверенную',
'markedaspatrollederrortext'          => 'Вы должны указать версию, которая будет отмечена как проверенная.',
'markedaspatrollederror-noautopatrol' => 'Вам не разрешено отмечать собственные правки как проверенные.',

# Patrol log
'patrol-log-page'   => 'Журнал патрулирования',
'patrol-log-header' => 'Это журнал патрулированных версий.',
'patrol-log-line'   => 'проверена $1 из $2 $3',
'patrol-log-auto'   => '(автоматически)',

# Image deletion
'deletedrevision'                 => 'Удалена старая версия $1',
'filedeleteerror-short'           => 'Ошибка удаления файла: $1',
'filedeleteerror-long'            => 'Во время удаления файла возникли ошибки:

$1',
'filedelete-missing'              => 'Файл «$1» не может быть удалён, так как его не существует.',
'filedelete-old-unregistered'     => 'Указанной версии файла «$1» не существует в базе данных.',
'filedelete-current-unregistered' => 'Указанного файла «$1» не существует в базе данных.',
'filedelete-archive-read-only'    => 'Архивная директория «$1» не доступна для записи веб-серверу.',

# Browsing diffs
'previousdiff' => '← Предыдущая правка',
'nextdiff'     => 'Следующая правка →',

# Media information
'mediawarning'         => "'''Внимание''': этот файл может содержать вредоносный программный код, выполнение которого способно подвергнуть риску вашу систему. <hr />",
'imagemaxsize'         => 'Ограничивать изображения на странице изображений до:',
'thumbsize'            => 'Размер уменьшенной версии изображения:',
'widthheight'          => '$1 × $2',
'widthheightpage'      => '$1 × $2, $3 {{PLURAL:$3|страница|страницы|страниц}}',
'file-info'            => '(размер файла: $1, MIME-тип: $2)',
'file-info-size'       => '($1 × $2 пикселов, размер файла: $3, MIME-тип: $4)',
'file-nohires'         => '<small>Нет версии с большим разрешением.</small>',
'svg-long-desc'        => '(SVG-файл, номинально $1 × $2 пикселов, размер файла: $3)',
'show-big-image'       => 'Изображение в более высоком разрешении',
'show-big-image-thumb' => '<small>Размер при предпросмотре: $1 × $2 пикселов</small>',

# Special:NewImages
'newimages'             => 'Галерея новых файлов',
'imagelisttext'         => "Ниже представлен список из '''$1''' {{PLURAL:$1|файла|файлов|файлов}}, отсортированных $2.",
'newimages-summary'     => 'Эта служебная страница показывает недавно загруженные файлы.',
'showhidebots'          => '($1 ботов)',
'noimages'              => 'Изображения отсутствуют.',
'ilsubmit'              => 'Найти',
'bydate'                => 'по дате',
'sp-newimages-showfrom' => 'Показать новые изображения, начиная с $2, $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'video-dims'     => '$1, $2 × $3',
'seconds-abbrev' => 'с',
'minutes-abbrev' => 'м',
'hours-abbrev'   => 'ч',

# Bad image list
'bad_image_list' => 'Формат должен быть следующим:

Будут учитываться только элементы списка (строки, начинающиеся на символ *).
Первая ссылка строки должна быть ссылкой на запрещённое для вставки изображение.
Последующие ссылки в той же строке будут рассматриваться как исключения, то есть статьи, куда изображение может быть включено.',

# Metadata
'metadata'          => 'Метаданные',
'metadata-help'     => 'Файл содержит дополнительные данные, обычно добавляемые цифровыми камерами или сканерами. Если файл после создания редактировался, то некоторые параметры могут не соответствовать текущему изображению.',
'metadata-expand'   => 'Показать дополнительные данные',
'metadata-collapse' => 'Скрыть дополнительные данные',
'metadata-fields'   => 'Поля метаданных, перечисленные в этом списке, будут показаны на странице изображения по умолчанию, остальные будут скрыты.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Ширина',
'exif-imagelength'                 => 'Высота',
'exif-bitspersample'               => 'Глубина цвета',
'exif-compression'                 => 'Метод сжатия',
'exif-photometricinterpretation'   => 'Цветовая модель',
'exif-orientation'                 => 'Ориентация кадра',
'exif-samplesperpixel'             => 'Количество цветовых компонентов',
'exif-planarconfiguration'         => 'Принцип организации данных',
'exif-ycbcrsubsampling'            => 'Отношение размеров компонент Y и C',
'exif-ycbcrpositioning'            => 'Порядок размещения компонент Y и C',
'exif-xresolution'                 => 'Горизонтальное разрешение',
'exif-yresolution'                 => 'Вертикальное разрешение',
'exif-resolutionunit'              => 'Единица измерения разрешения',
'exif-stripoffsets'                => 'Положение блока данных',
'exif-rowsperstrip'                => 'Количество строк в 1 блоке',
'exif-stripbytecounts'             => 'Размер сжатого блока',
'exif-jpeginterchangeformat'       => 'Положение начала блока preview',
'exif-jpeginterchangeformatlength' => 'Размер данных блока preview',
'exif-transferfunction'            => 'Функция преобразования цветового пространства',
'exif-whitepoint'                  => 'Цветность белой точки',
'exif-primarychromaticities'       => 'Цветность основных цветов',
'exif-ycbcrcoefficients'           => 'Коэффициенты преобразования цветовой модели',
'exif-referenceblackwhite'         => 'Положение белой и чёрной точек',
'exif-datetime'                    => 'Дата и время изменения файла',
'exif-imagedescription'            => 'Название изображения',
'exif-make'                        => 'Производитель камеры',
'exif-model'                       => 'Модель камеры',
'exif-software'                    => 'Программное обеспечение',
'exif-artist'                      => 'Автор',
'exif-copyright'                   => 'Владелец авторского права',
'exif-exifversion'                 => 'Версия Exif',
'exif-flashpixversion'             => 'Поддерживаемая версия FlashPix',
'exif-colorspace'                  => 'Цветовое пространство',
'exif-componentsconfiguration'     => 'Конфигурация цветовых компонентов',
'exif-compressedbitsperpixel'      => 'Глубина цвета после сжатия',
'exif-pixelydimension'             => 'Полная высота изображения',
'exif-pixelxdimension'             => 'Полная ширина изображения',
'exif-makernote'                   => 'Дополнительные данные производителя',
'exif-usercomment'                 => 'Дополнительный комментарий',
'exif-relatedsoundfile'            => 'Файл звукового комментария',
'exif-datetimeoriginal'            => 'Оригинальные дата и время',
'exif-datetimedigitized'           => 'Дата и время оцифровки',
'exif-subsectime'                  => 'Доли секунд времени изменения файла',
'exif-subsectimeoriginal'          => 'Доли секунд оригинального времени',
'exif-subsectimedigitized'         => 'Доли секунд времени оцифровки',
'exif-exposuretime'                => 'Время экспозиции',
'exif-exposuretime-format'         => '$1 с ($2)',
'exif-fnumber'                     => 'Число диафрагмы',
'exif-exposureprogram'             => 'Программа экспозиции',
'exif-spectralsensitivity'         => 'Спектральная чувствительность',
'exif-isospeedratings'             => 'Светочувствительность ISO',
'exif-oecf'                        => 'OECF (коэффициент оптоэлектрического преобразования)',
'exif-shutterspeedvalue'           => 'Выдержка',
'exif-aperturevalue'               => 'Диафрагма',
'exif-brightnessvalue'             => 'Яркость',
'exif-exposurebiasvalue'           => 'Компенсация экспозиции',
'exif-maxaperturevalue'            => 'Минимальное число диафрагмы',
'exif-subjectdistance'             => 'Расстояние до объекта',
'exif-meteringmode'                => 'Режим замера экспозиции',
'exif-lightsource'                 => 'Источник света',
'exif-flash'                       => 'Статус вспышки',
'exif-focallength'                 => 'Фокусное расстояние',
'exif-focallength-format'          => '$1 мм',
'exif-subjectarea'                 => 'Положение и площадь объекта съёмки',
'exif-flashenergy'                 => 'Энергия вспышки',
'exif-spatialfrequencyresponse'    => 'Пространственная частотная характеристика',
'exif-focalplanexresolution'       => 'Разрешение по X в фокальной плоскости',
'exif-focalplaneyresolution'       => 'Разрешение по Y в фокальной плоскости',
'exif-focalplaneresolutionunit'    => 'Единица измерения разрешения в фокальной плоскости',
'exif-subjectlocation'             => 'Положение объекта относительно левого верхнего угла',
'exif-exposureindex'               => 'Индекс экспозиции',
'exif-sensingmethod'               => 'Тип сенсора',
'exif-filesource'                  => 'Источник файла',
'exif-scenetype'                   => 'Тип сцены',
'exif-cfapattern'                  => 'Тип цветового фильтра',
'exif-customrendered'              => 'Дополнительная обработка',
'exif-exposuremode'                => 'Режим выбора экспозиции',
'exif-whitebalance'                => 'Баланс белого',
'exif-digitalzoomratio'            => 'Коэффициент цифрового увеличения (цифровой зум)',
'exif-focallengthin35mmfilm'       => 'Эквивалентное фокусное расстояние (для 35 мм плёнки)',
'exif-scenecapturetype'            => 'Тип сцены при съёмке',
'exif-gaincontrol'                 => 'Повышение яркости',
'exif-contrast'                    => 'Контрастность',
'exif-saturation'                  => 'Насыщенность',
'exif-sharpness'                   => 'Резкость',
'exif-devicesettingdescription'    => 'Описание предустановок камеры',
'exif-subjectdistancerange'        => 'Расстояние до объекта съёмки',
'exif-imageuniqueid'               => 'Номер изображения (ID)',
'exif-gpsversionid'                => 'Версия блока GPS-информации',
'exif-gpslatituderef'              => 'Индекс широты',
'exif-gpslatitude'                 => 'Широта',
'exif-gpslongituderef'             => 'Индекс долготы',
'exif-gpslongitude'                => 'Долгота',
'exif-gpsaltituderef'              => 'Индекс высоты',
'exif-gpsaltitude'                 => 'Высота',
'exif-gpstimestamp'                => 'Точное время по UTC',
'exif-gpssatellites'               => 'Описание использованных спутников',
'exif-gpsstatus'                   => 'Статус приёмника в момент съёмки',
'exif-gpsmeasuremode'              => 'Метод измерения положения',
'exif-gpsdop'                      => 'Точность измерения',
'exif-gpsspeedref'                 => 'Единицы измерения скорости',
'exif-gpsspeed'                    => 'Скорость движения',
'exif-gpstrackref'                 => 'Тип азимута приёмника GPS (истинный, магнитный)',
'exif-gpstrack'                    => 'Азимут приёмника GPS',
'exif-gpsimgdirectionref'          => 'Тип азимута изображения (истинный, магнитный)',
'exif-gpsimgdirection'             => 'Азимут изображения',
'exif-gpsmapdatum'                 => 'Использованная геодезическая система координат',
'exif-gpsdestlatituderef'          => 'Индекс долготы объекта',
'exif-gpsdestlatitude'             => 'Долгота объекта',
'exif-gpsdestlongituderef'         => 'Индекс широты объекта',
'exif-gpsdestlongitude'            => 'Широта объекта',
'exif-gpsdestbearingref'           => 'Тип пеленга объекта (истинный, магнитный)',
'exif-gpsdestbearing'              => 'Пеленг объекта',
'exif-gpsdestdistanceref'          => 'Единицы измерения расстояния',
'exif-gpsdestdistance'             => 'Расстояние',
'exif-gpsprocessingmethod'         => 'Метод вычисления положения',
'exif-gpsareainformation'          => 'Название области GPS',
'exif-gpsdatestamp'                => 'Дата',
'exif-gpsdifferential'             => 'Дифференциальная поправка',

# EXIF attributes
'exif-compression-1' => 'Несжатый',

'exif-unknowndate' => 'Неизвестная дата',

'exif-orientation-1' => 'Нормальная', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Отражено по горизонтали', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Повёрнуто на 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Отражено по вертикали', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Повёрнуто на 90° против часовой стрелки и отражено по вертикали', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Повёрнуто на 90° по часовой стрелке', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Повёрнуто на 90° по часовой стрелке и отражено по вертикали', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Повёрнуто на 90° против часовой стрелки', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'формат «chunky»',
'exif-planarconfiguration-2' => 'формат «planar»',

'exif-xyresolution-i' => '$1 точек на дюйм',
'exif-xyresolution-c' => '$1 точек на сантиметр',

'exif-componentsconfiguration-0' => 'не существует',

'exif-exposureprogram-0' => 'Неизвестно',
'exif-exposureprogram-1' => 'Ручной режим',
'exif-exposureprogram-2' => 'Программный режим (нормальный)',
'exif-exposureprogram-3' => 'Приоритет диафрагмы',
'exif-exposureprogram-4' => 'Приоритет выдержки',
'exif-exposureprogram-5' => 'Художественная программа (на основе нужной глубины резкости)',
'exif-exposureprogram-6' => 'Спортивный режим (с минимальной выдержкой)',
'exif-exposureprogram-7' => 'Портретный режим (для снимков на близком расстоянии, с фоном не в фокусе)',
'exif-exposureprogram-8' => 'Пейзажный режим (для пейзажных снимков, с фоном в фокусе)',

'exif-subjectdistance-value' => '$1 метров',

'exif-meteringmode-0'   => 'Неизвестно',
'exif-meteringmode-1'   => 'Средний',
'exif-meteringmode-2'   => 'Центровзвешенный',
'exif-meteringmode-3'   => 'Точечный',
'exif-meteringmode-4'   => 'Мультиточечный',
'exif-meteringmode-5'   => 'Матричный',
'exif-meteringmode-6'   => 'Частичный',
'exif-meteringmode-255' => 'Другой',

'exif-lightsource-0'   => 'Неизвестно',
'exif-lightsource-1'   => 'Дневной свет',
'exif-lightsource-2'   => 'Лампа дневного света',
'exif-lightsource-3'   => 'Лампа накаливания',
'exif-lightsource-4'   => 'Вспышка',
'exif-lightsource-9'   => 'Хорошая погода',
'exif-lightsource-10'  => 'Облачно',
'exif-lightsource-11'  => 'Тень',
'exif-lightsource-12'  => 'Лампа дневного света тип D (5700 − 7100K)',
'exif-lightsource-13'  => 'Лампа дневного света тип N (4600 − 5400K)',
'exif-lightsource-14'  => 'Лампа дневного света тип W (3900 − 4500K)',
'exif-lightsource-15'  => 'Лампа дневного света тип WW (3200 − 3700K)',
'exif-lightsource-17'  => 'Стандартный источник света типа A',
'exif-lightsource-18'  => 'Стандартный источник света типа B',
'exif-lightsource-19'  => 'Стандартный источник света типа C',
'exif-lightsource-24'  => 'Студийная лампа стандарта ISO',
'exif-lightsource-255' => 'Другой источник света',

'exif-focalplaneresolutionunit-2' => 'дюймов',

'exif-sensingmethod-1' => 'Неопределённый',
'exif-sensingmethod-2' => 'Однокристальный матричный цветной сенсор',
'exif-sensingmethod-3' => 'Цветной сенсор с двумя матрицами',
'exif-sensingmethod-4' => 'Цветной сенсор с тремя матрицами',
'exif-sensingmethod-5' => 'Матричный сенсор с последовательным измерением цвета',
'exif-sensingmethod-7' => 'Трёхцветный линейный сенсор',
'exif-sensingmethod-8' => 'Линейный сенсор с последовательным измерением цвета',

'exif-filesource-3' => 'Цифровой фотоаппарат',

'exif-scenetype-1' => 'Изображение сфотографировано напрямую',

'exif-customrendered-0' => 'Не производилась',
'exif-customrendered-1' => 'Нестандартная обработка',

'exif-exposuremode-0' => 'Автоматическая экспозиция',
'exif-exposuremode-1' => 'Ручная установка экспозиции',
'exif-exposuremode-2' => 'Брэкетинг',

'exif-whitebalance-0' => 'Автоматический баланс белого',
'exif-whitebalance-1' => 'Ручная установка баланса белого',

'exif-scenecapturetype-0' => 'Стандартный',
'exif-scenecapturetype-1' => 'Ландшафт',
'exif-scenecapturetype-2' => 'Портрет',
'exif-scenecapturetype-3' => 'Ночная съёмка',

'exif-gaincontrol-0' => 'Нет',
'exif-gaincontrol-1' => 'Небольшое увеличение',
'exif-gaincontrol-2' => 'Большое увеличение',
'exif-gaincontrol-3' => 'Небольшое уменьшение',
'exif-gaincontrol-4' => 'Сильное уменьшение',

'exif-contrast-0' => 'Нормальная',
'exif-contrast-1' => 'Мягкое повышение',
'exif-contrast-2' => 'Сильное повышение',

'exif-saturation-0' => 'Нормальная',
'exif-saturation-1' => 'Небольшая насыщенность',
'exif-saturation-2' => 'Большая насыщенность',

'exif-sharpness-0' => 'Нормальная',
'exif-sharpness-1' => 'Мягкое повышение',
'exif-sharpness-2' => 'Сильное повышение',

'exif-subjectdistancerange-0' => 'Неизвестно',
'exif-subjectdistancerange-1' => 'Макросъёмка',
'exif-subjectdistancerange-2' => 'Съёмка с близкого расстояния',
'exif-subjectdistancerange-3' => 'Съёмка издалека',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'северной широты',
'exif-gpslatitude-s' => 'южной широты',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'восточной долготы',
'exif-gpslongitude-w' => 'западной долготы',

'exif-gpsstatus-a' => 'Измерение не закончено',
'exif-gpsstatus-v' => 'Готов к передаче данных',

'exif-gpsmeasuremode-2' => 'Измерение 2-х координат',
'exif-gpsmeasuremode-3' => 'Измерение 3-х координат',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'км/час',
'exif-gpsspeed-m' => 'миль/час',
'exif-gpsspeed-n' => 'узлов',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'истинный',
'exif-gpsdirection-m' => 'магнитный',

# External editor support
'edit-externally'      => 'Редактировать этот файл, используя внешнюю программу',
'edit-externally-help' => 'Подробности см. на странице [http://www.mediawiki.org/wiki/Manual:External_editors Meta:Help:External_editors].',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'все',
'imagelistall'     => 'все',
'watchlistall2'    => 'все',
'namespacesall'    => 'все',
'monthsall'        => 'все',

# E-mail address confirmation
'confirmemail'             => 'Подтверждение адреса электронной почты',
'confirmemail_noemail'     => 'Вы не задали адрес электронной почты в своих [[Special:Preferences|настройках]], либо он некорректен.',
'confirmemail_text'        => 'Вики-движок требует подтверждения адреса электронной почты перед тем, как начать с ним работать.
Нажмите на кнопку, чтобы на указанный адрес было отправлено письмо, содержащее ссылку на специальную страницу, после открытия которой в браузере адрес электронной почты будет считаться подтверждённым.',
'confirmemail_pending'     => '<div class="error">
Письмо с кодом подтверждения уже было отправлено.
Если вы недавно создали учётную запись, то, вероятно,
вам следует подождать несколько минут пока письмо придёт перед тем, как запросить код ещё раз.
</div>',
'confirmemail_send'        => 'Отправить письмо с запросом на подтверждение',
'confirmemail_sent'        => 'Письмо с запросом на подтверждение отправлено.',
'confirmemail_oncreate'    => 'Письмо с кодом подтверждения было отправлено на указанный вами почтовый ящик.
Данный код не требуется для входа в систему, однако вы должны указать его,
прежде чем будет разрешено использование возможностей электронной почты в этом проекте.',
'confirmemail_sendfailed'  => '{{SITENAME}} не может отправить письмо с запросом на подтверждение.
Пожалуйста, проверьте правильность адреса электронной почты.

Ответ сервера: $1',
'confirmemail_invalid'     => 'Неправильный код подтверждения или срок действия кода истёк.',
'confirmemail_needlogin'   => 'Вы должны $1 для подтверждения вашего адреса электронной почты.',
'confirmemail_success'     => 'Ваш адрес электронной почты подтверждён.',
'confirmemail_loggedin'    => 'Ваш адрес электронной почты подтверждён.',
'confirmemail_error'       => 'Во время процедуры подтверждения адреса электронной почты произошла ошибка.',
'confirmemail_subject'     => '{{SITENAME}}:Запрос на подтверждение адреса электронной почты',
'confirmemail_body'        => 'Кто-то (возможно вы) с IP-адресом $1 зарегистрировал
на сервере проекта {{SITENAME}} учётную запись «$2»,
указав этот адрес электронной почты.

Чтобы подтвердить, что вы эта учётная запись действительно
принадлежит вам и включить возможность отправки электронной почты
с сайта {{SITENAME}}, откройте приведённую ниже ссылку в браузере.

$3

Если вы *не* регистрировали подобной учётной записи, то перейдите
по следующей ссылке, чтобы отменить подтверждение адреса

$5

Код подтверждения действителен до $4.',
'confirmemail_invalidated' => 'Подтверждение адреса электронной почты отменено',
'invalidateemail'          => 'Отменить подтверждение адреса эл. почты',

# Scary transclusion
'scarytranscludedisabled' => '[«Interwiki transcluding» отключён]',
'scarytranscludefailed'   => '[Ошибка обращения к шаблону $1]',
'scarytranscludetoolong'  => '[Слишком длинный URL]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Trackback для этой статьи:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 удалить])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Trackback был удалён.',

# Delete conflict
'deletedwhileediting' => "'''Внимание'''. Эта страница была удалена после того, как вы начали её править!",
'confirmrecreate'     => "Участник [[User:$1|$1]] ([[User talk:$1|обсуждение]]) удалил эту страницу после того, как вы начали её редактировать, причина удаления:
: ''$2''
Пожалуйста, подтвердите, что вы хотите восстановить эту страницу.",
'recreate'            => 'Создать заново',

'unit-pixel' => ' пикс.',

# HTML dump
'redirectingto' => 'Перенаправление на страницу [[:$1]]…',

# action=purge
'confirm_purge'        => 'Очистить кеш этой страницы?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => 'Поиск статей, содержащих «$1».',
'searchnamed'      => "Поиск страниц с именем ''$1''.",
'articletitles'    => 'Статьи, начинающиеся с «$1»',
'hideresults'      => 'Скрыть результаты',
'useajaxsearch'    => 'Использовать AJAX-поиск',

# Multipage image navigation
'imgmultipageprev' => '← предыдущая страница',
'imgmultipagenext' => 'следующая страница →',
'imgmultigo'       => 'Перейти!',
'imgmultigoto'     => 'Перейти на страницу $1',

# Table pager
'ascending_abbrev'         => 'возр',
'descending_abbrev'        => 'убыв',
'table_pager_next'         => 'Следующая страница',
'table_pager_prev'         => 'Предыдущая страница',
'table_pager_first'        => 'Первая страница',
'table_pager_last'         => 'Последняя страница',
'table_pager_limit'        => 'Показать $1 элементов на странице',
'table_pager_limit_submit' => 'Выполнить',
'table_pager_empty'        => 'Не найдено',

# Auto-summaries
'autosumm-blank'   => 'Полностью удалено содержимое страницы',
'autosumm-replace' => 'Содержимое страницы заменено на «$1»',
'autoredircomment' => 'Перенаправление на [[$1]]',
'autosumm-new'     => 'Новая: $1',

# Size units
'size-bytes'     => '$1 байт',
'size-kilobytes' => '$1 КБ',
'size-megabytes' => '$1 МБ',
'size-gigabytes' => '$1 ГБ',

# Live preview
'livepreview-loading' => 'Загрузка…',
'livepreview-ready'   => 'Загрузка… Готово!',
'livepreview-failed'  => 'Не удалось использовать быстрый предпросмотр. Попробуйте воспользоваться обычным предпросмотром.',
'livepreview-error'   => 'Не удалось установить соединение: $1 «$2». Попробуйте воспользоваться обычным предпросмотром.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Изменения, сделанные менее чем $1 {{PLURAL:$1|секунду|секунды|секунд}} назад, могут быть не показаны в этом списке.',
'lag-warn-high'   => 'Из-за большого отставания в синхронизации серверов баз данных изменения, сделанные менее чем $1 {{PLURAL:$1|секунду|секунды|секунд}} назад, могут быть не показаны в этом списке.',

# Watchlist editor
'watchlistedit-numitems'       => 'Ваш список наблюдения содержит {{PLURAL:$1|$1 запись|$1 записи|$1 записей}}, исключая страницы обсуждений.',
'watchlistedit-noitems'        => 'Ваш список наблюдения не содержит записей.',
'watchlistedit-normal-title'   => 'Изменение списка наблюдения',
'watchlistedit-normal-legend'  => 'Удаление записей из списка наблюдения',
'watchlistedit-normal-explain' => "Ниже перечислены страницы, находящиеся в вашем списке наблюдения.
Для удаления записей отметьте соответствующие позиции и нажмите кнопку '''«Удалить записи»'''.
Вы также можете [[Special:Watchlist/raw|править список как текст]].",
'watchlistedit-normal-submit'  => 'Удалить записи',
'watchlistedit-normal-done'    => '{{PLURAL:$1|$1 запись была удалена|$1 записи были удалены|$1 записей были удалены}} из вашего списка наблюдения:',
'watchlistedit-raw-title'      => 'Изменение «сырого» списка наблюдения',
'watchlistedit-raw-legend'     => 'Изменение «сырого» списка наблюдения',
'watchlistedit-raw-explain'    => 'Ниже перечислены страницы, находящиеся в вашем списке наблюдения. Вы можете изменять этот список, добавляя и удаляя из него строки с названиями.
После завершения правок нажмите кнопку «Сохранить список».
Вы также можете удалять страницы из списка [[Special:Watchlist/edit|обычным способом]].',
'watchlistedit-raw-titles'     => 'Записи:',
'watchlistedit-raw-submit'     => 'Сохранить список',
'watchlistedit-raw-done'       => 'Ваш список наблюдения сохранён.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|$1 запись была добавлена|$1 записи были добавлены|$1 записей были добавлены}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|$1 запись была удалена|$1 записи были удалены|$1 записей были удалены}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Изменения на страницах из списка',
'watchlisttools-edit' => 'Смотреть/править список',
'watchlisttools-raw'  => 'Править как текст',

# Iranian month names
'iranian-calendar-m1'  => 'Фарвардин',
'iranian-calendar-m2'  => 'Ордибехешт',
'iranian-calendar-m3'  => 'Хордад',
'iranian-calendar-m4'  => 'Тир',
'iranian-calendar-m5'  => 'Мордад',
'iranian-calendar-m6'  => 'Шахривар',
'iranian-calendar-m7'  => 'Мехр',
'iranian-calendar-m8'  => 'Абан',
'iranian-calendar-m9'  => 'Азар',
'iranian-calendar-m10' => 'Дей',
'iranian-calendar-m11' => 'Бахман',
'iranian-calendar-m12' => 'Эсфанд',

# Hebrew month names
'hebrew-calendar-m1'      => 'Тишрей',
'hebrew-calendar-m2'      => 'Хешван',
'hebrew-calendar-m4'      => 'Тевет',
'hebrew-calendar-m5'      => 'Шват',
'hebrew-calendar-m6'      => 'Адар',
'hebrew-calendar-m6a'     => 'Адар I',
'hebrew-calendar-m6b'     => 'Адар II',
'hebrew-calendar-m7'      => 'Нисан',
'hebrew-calendar-m8'      => 'Ияр',
'hebrew-calendar-m9'      => 'Сиван',
'hebrew-calendar-m10'     => 'Таммуз',
'hebrew-calendar-m11'     => 'Ав',
'hebrew-calendar-m12'     => 'Элул',
'hebrew-calendar-m1-gen'  => 'Тишрея',
'hebrew-calendar-m2-gen'  => 'Хешвана',
'hebrew-calendar-m3-gen'  => 'Кислева',
'hebrew-calendar-m4-gen'  => 'Тевета',
'hebrew-calendar-m5-gen'  => 'Швата',
'hebrew-calendar-m6-gen'  => 'Адара',
'hebrew-calendar-m6a-gen' => 'Адара I',
'hebrew-calendar-m6b-gen' => 'Адара II',
'hebrew-calendar-m7-gen'  => 'Нисана',
'hebrew-calendar-m8-gen'  => 'Ияра',
'hebrew-calendar-m9-gen'  => 'Сивана',
'hebrew-calendar-m10-gen' => 'Таммуза',
'hebrew-calendar-m11-gen' => 'Ава',
'hebrew-calendar-m12-gen' => 'Элула',

# Core parser functions
'unknown_extension_tag' => 'Неизвестный тег дополнения «$1»',

# Special:Version
'version'                          => 'Версия MediaWiki', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Установленные расширения',
'version-specialpages'             => 'Служебные страницы',
'version-parserhooks'              => 'Перехватчики синтаксического анализатора',
'version-variables'                => 'Переменные',
'version-other'                    => 'Иное',
'version-mediahandlers'            => 'Обработчики медиа',
'version-hooks'                    => 'Перехватчики',
'version-extension-functions'      => 'Функции расширений',
'version-parser-extensiontags'     => 'Теги расширений синтаксического анализатора',
'version-parser-function-hooks'    => 'Перехватчики функций синтаксического анализатора',
'version-skin-extension-functions' => 'Функции расширений тем оформления',
'version-hook-name'                => 'Имя перехватчика',
'version-hook-subscribedby'        => 'Подписан на',
'version-version'                  => 'Версия',
'version-license'                  => 'Лицензия',
'version-software'                 => 'Установленное программное обеспечение',
'version-software-product'         => 'Продукт',
'version-software-version'         => 'Версия',

# Special:FilePath
'filepath'         => 'Путь к файлу',
'filepath-page'    => 'Файл:',
'filepath-submit'  => 'Путь',
'filepath-summary' => 'Данная служебная страница возвращает полный путь к файлу в том виде, в котором он хранится на диске.

Введите имя файла без префикса <code>{{ns:image}}:</code>.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Поиск одинаковых файлов',
'fileduplicatesearch-summary'  => 'Поиск одинаковых файлов по их хэш-коду.

Введите имя файла без приставки «{{ns:image}}:».',
'fileduplicatesearch-legend'   => 'Поиск дубликатов',
'fileduplicatesearch-filename' => 'Имя файла:',
'fileduplicatesearch-submit'   => 'Найти',
'fileduplicatesearch-info'     => '$1 × $2 пикселов<br />Размер файла: $3<br />MIME-тип: $4',
'fileduplicatesearch-result-1' => 'Файл «$1» не имеет идентичных дубликатов.',
'fileduplicatesearch-result-n' => 'Файл «$1» имеет $2 {{PLURAL:$2|идентичный дубликат|идентичных дубликата|идентичных дубликатов}}.',

# Special:SpecialPages
'specialpages'                   => 'Спецстраницы',
'specialpages-note'              => '----
* Обычные служебные страницы.
* <span class="mw-specialpagerestricted">Служебные страницы с ограниченным доступом.</span>',
'specialpages-group-maintenance' => 'Отчёты технического обслуживания',
'specialpages-group-other'       => 'Другие служебные страницы',
'specialpages-group-login'       => 'Представиться / Зарегистрироваться',
'specialpages-group-changes'     => 'Свежие правки и журналы',
'specialpages-group-media'       => 'Отчёты о медиа-материалах и загрузка',
'specialpages-group-users'       => 'Участники и права',
'specialpages-group-highuse'     => 'Интенсивно используемые страницы',
'specialpages-group-pages'       => 'Списки страниц',
'specialpages-group-pagetools'   => 'Инструменты для страниц',
'specialpages-group-wiki'        => 'Вики-данные и инструменты',
'specialpages-group-redirects'   => 'Перенаправляющие служебные страницы',
'specialpages-group-spam'        => 'Инструменты против спама',

# Special:BlankPage
'blankpage'              => 'Пустая страница',
'intentionallyblankpage' => 'Эта страница намеренно оставлена пустой',

);
