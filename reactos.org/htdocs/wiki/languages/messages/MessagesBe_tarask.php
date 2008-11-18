<?php
/** Belarusian (Taraškievica orthography) (Беларуская (тарашкевіца))
 *
 * @ingroup Language
 * @file
 *
 * @author Cesco
 * @author EugeneZelenko
 * @author Jim-by
 * @author Red Winged Duck
 */

$skinNames = array(
	'standard'    => 'Клясычнае',
	'nostalgia'   => 'Настальгія',
	'cologneblue' => 'Кёльнскі смутак',
	'monobook'    => 'Монакніга',
	'myskin'      => 'MySkin',
	'chick'       => 'Цыпа',
	'simple'      => 'Простае',
	'modern'      => 'Сучаснае',
);

$bookstoreList = array(
	'OZ.by' => 'http://oz.by/search.phtml?what=books&isbn=$1',
	'Amazon.com' => 'http://www.amazon.com/exec/obidos/ISBN=$1'
);

$datePreferences = array(
	'default',
	'dmy',
	'ISO 8601',
);

$defaultDateFormat = 'dmy';

$dateFormats = array(
	'dmy time' => 'H:i',
	'dmy date' => 'j xg Y',
	'dmy both' => 'H:i, j xg Y',
);

$magicWords = array(
	'redirect'               => array( 0,    '#перанакіраваньне', '#redirect' ),
	'notoc'                  => array( 0,    '__NOTOC__', '__БЯЗЬ_ЗЬМЕСТУ__' ),
	'nogallery'              => array( 0,    '__NOGALLERY__', '__БЕЗ_ГАЛЕРЭІ__' ),
	'forcetoc'               => array( 0,    '__FORCETOC__', '__ЗЬМЕСТ_ПРЫМУСАМ__' ),
	'toc'                    => array( 0,    '__TOC__', '__ЗЬМЕСТ__' ),
	'noeditsection'          => array( 0,    '__NOEDITSECTION__', '__БЕЗ_РЭДАГАВАНЬНЯ_СЭКЦЫІ__' ),
	'currentmonth'           => array( 1,    'CURRENTMONTH', 'БЯГУЧЫ_МЕСЯЦ' ),
	'currentmonthname'       => array( 1,    'CURRENTMONTHNAME', 'НАЗВА_БЯГУЧАГА_МЕСЯЦА' ),
	'currentmonthnamegen'    => array( 1,    'CURRENTMONTHNAMEGEN', 'НАЗВА_БЯГУЧАГА_МЕСЯЦА_Ў_РОДНЫМ_СКЛОНЕ' ),
	'currentmonthabbrev'     => array( 1,    'CURRENTMONTHABBREV', 'СКАРОЧАНАЯ_НАЗВА_БЯГУЧАГА_МЕСЯЦА' ),
	'currentday'             => array( 1,    'CURRENTDAY', 'БЯГУЧЫ_ДЗЕНЬ' ),
	'currentday2'            => array( 1,    'CURRENTDAY2', 'БЯГУЧЫ_ДЗЕНЬ_2' ),
	'currentdayname'         => array( 1,    'CURRENTDAYNAME', 'НАЗВА_БЯГУЧАГА_ДНЯ' ),
	'currentyear'            => array( 1,    'CURRENTYEAR', 'БЯГУЧЫ_ГОД' ),
	'currenttime'            => array( 1,    'CURRENTTIME', 'БЯГУЧЫ_ЧАС' ),
	'currenthour'            => array( 1,    'CURRENTHOUR', 'БЯГУЧАЯ_ГАДЗІНА' ),
	'localmonth'             => array( 1,    'LOCALMONTH', 'ЛЯКАЛЬНЫ_МЕСЯЦ' ),
	'localmonthname'         => array( 1,    'LOCALMONTHNAME', 'НАЗВА_ЛЯКАЛЬНАГА_МЕСЯЦА' ),
	'localmonthnamegen'      => array( 1,    'LOCALMONTHNAMEGEN', 'НАЗВА_ЛЯКАЛЬНАГА_МЕСЯЦА_Ў_РОДНЫМ_СКЛОНЕ' ),
	'localmonthabbrev'       => array( 1,    'LOCALMONTHABBREV', 'СКАРОЧАНАЯ_НАЗВА_ЛЯКАЛЬНАГА_МЕСЯЦА' ),
	'localday'               => array( 1,    'LOCALDAY', 'ЛЯКАЛЬНЫ_ДЗЕНЬ' ),
	'localday2'              => array( 1,    'LOCALDAY2', 'ЛЯКАЛЬНЫ_ДЗЕНЬ_2' ),
	'localdayname'           => array( 1,    'LOCALDAYNAME', 'НАЗВА_ЛЯКАЛЬНАГА_ДНЯ' ),
	'localyear'              => array( 1,    'LOCALYEAR', 'ЛЯКАЛЬНЫ_ГОД' ),
	'localtime'              => array( 1,    'LOCALTIME', 'ЛЯКАЛЬНЫ_ЧАС' ),
	'localhour'              => array( 1,    'LOCALHOUR', 'ЛЯКАЛЬНАЯ_ГАДЗІНА' ),
	'numberofpages'          => array( 1,    'NUMBEROFPAGES', 'КОЛЬКАСЬЦЬ_СТАРОНАК' ),
	'numberofarticles'       => array( 1,    'NUMBEROFARTICLES', 'КОЛЬКАСЬЦЬ_АРТЫКУЛАЎ' ),
	'numberoffiles'          => array( 1,    'NUMBEROFFILES', 'КОЛЬКАСЬЦЬ_ФАЙЛАЎ' ),
	'numberofusers'          => array( 1,    'NUMBEROFUSERS', 'КОЛЬКАСЬЦЬ_УДЗЕЛЬНІКАЎ' ),
	'numberofedits'          => array( 1,    'NUMBEROFEDITS', 'КОЛЬКАСЬЦЬ_РЭДАГАВАНЬНЯЎ' ),
	'pagename'               => array( 1,    'PAGENAME', 'НАЗВА_СТАРОНКІ' ),
	'pagenamee'              => array( 1,    'PAGENAMEE', 'НАЗВА_СТАРОНКІ_2' ),
	'namespace'              => array( 1,    'NAMESPACE', 'ПРАСТОРА_НАЗВАЎ' ),
	'namespacee'             => array( 1,    'NAMESPACEE', 'ПРАСТОРА_НАЗВАЎ_2' ),
	'talkspace'              => array( 1,    'TALKSPACE', 'ПРАСТОРА_НАЗВАЎ_АБМЕРКАВАНЬНЯ' ),
	'talkspacee'             => array( 1,    'TALKSPACEE', 'ПРАСТОРА_НАЗВАЎ_АБМЕРКАВАНЬНЯ_2' ),
	'subjectspace'           => array( 1,    'SUBJECTSPACE', 'ARTICLESPACE', 'ПРАСТОРА_НАЗВАЎ_ПРАДМЕТУ', 'ПРАСТОРА_НАЗВАЎ_АРТЫКУЛА' ),
	'subjectspacee'          => array( 1,    'SUBJECTSPACEE', 'ARTICLESPACEE', 'ПРАСТОРА_НАЗВАЎ_ПРАДМЕТУ_2', 'ПРАСТОРА_НАЗВАЎ_АРТЫКУЛА_2' ),
	'fullpagename'           => array( 1,    'FULLPAGENAME', 'ПОЎНАЯ_НАЗВА_СТАРОНКІ' ),
	'fullpagenamee'          => array( 1,    'FULLPAGENAMEE', 'ПОЎНАЯ_НАЗВА_СТАРОНКІ_2' ),
	'subpagename'            => array( 1,    'SUBPAGENAME', 'НАЗВА_ПАДСТАРОНКІ' ),
	'subpagenamee'           => array( 1,    'SUBPAGENAMEE', 'НАЗВА_ПАДСТАРОНКІ_2' ),
	'basepagename'           => array( 1,    'BASEPAGENAME', 'НАЗВА_БАЗАВАЙ_СТАРОНКІ' ),
	'basepagenamee'          => array( 1,    'BASEPAGENAMEE', 'НАЗВА_БАЗАВАЙ_СТАРОНКІ_2' ),
	'talkpagename'           => array( 1,    'TALKPAGENAME', 'НАЗВА_СТАРОНКІ_АБМЕРКАВАНЬНЯ' ),
	'talkpagenamee'          => array( 1,    'TALKPAGENAMEE', 'НАЗВА_СТАРОНКІ_АБМЕРКАВАНЬНЯ_2' ),
	'subjectpagename'        => array( 1,    'SUBJECTPAGENAME', 'ARTICLEPAGENAME', 'НАЗВА_СТАРОНКІ_ПРАДМЕТУ', 'НАЗВА_СТАРОНКІ_АРТЫКУЛА' ),
	'subjectpagenamee'       => array( 1,    'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE', 'НАЗВА_СТАРОНКІ_ПРАДМЕТУ_2', 'НАЗВА_СТАРОНКІ_АРТЫКУЛА_2' ),
	'msg'                    => array( 0,    'MSG:', 'ПАВЕДАМЛЕНЬНЕ:' ),
	'subst'                  => array( 0,    'SUBST:', 'ПАДСТАНОЎКА:' ),
	'msgnw'                  => array( 0,    'MSGNW:', 'ПАВЕДАМЛЕНЬНЕ_БЯЗЬ_ВІКІ:' ),
	'img_thumbnail'          => array( 1,    'thumbnail', 'thumb', 'значак', 'міні' ),
	'img_manualthumb'        => array( 1,    'thumbnail=$1', 'thumb=$1', 'значак=$1', 'міні=$1' ),
	'img_right'              => array( 1,    'right', 'справа' ),
	'img_left'               => array( 1,    'left', 'зьлева' ),
	'img_none'               => array( 1,    'none', 'няма' ),
	'img_width'              => array( 1,    '$1px', '$1пкс' ),
	'img_center'             => array( 1,    'center', 'centre', 'цэнтар' ),
	'img_framed'             => array( 1,    'framed', 'enframed', 'frame', 'рамка' ),
	'img_page'               => array( 1,    'page=$1', 'page $1', 'старонка=$1', 'старонка $1' ),
	'img_top'                => array( 1,    'top', 'зьверху' ),
	'img_middle'             => array( 1,    'middle', 'пасярэдзіне' ),
	'img_bottom'             => array( 1,    'bottom', 'зьнізу' ),
	'sitename'               => array( 1,    'SITENAME', 'НАЗВА_САЙТУ' ),
	'ns'                     => array( 0,    'NS:', 'ПН:' ),
	'localurl'               => array( 0,    'LOCALURL:', 'ЛЯКАЛЬНЫ_АДРАС:' ),
	'localurle'              => array( 0,    'LOCALURLE:', 'ЛЯКАЛЬНЫ_АДРАС_2:' ),
	'server'                 => array( 0,    'SERVER', 'СЭРВЭР' ),
	'servername'             => array( 0,    'SERVERNAME', 'НАЗВА_СЭРВЭРА' ),
	'scriptpath'             => array( 0,    'SCRIPTPATH', 'ШЛЯХ_ДА_СКРЫПТА' ),
	'grammar'                => array( 0,    'GRAMMAR:', 'ГРАМАТЫКА:' ),
	'notitleconvert'         => array( 0,    '__NOTITLECONVERT__', '__NOTC__', '__НЕ_КАНВЭРТАВАЦЬ_НАЗВУ__' ),
	'nocontentconvert'       => array( 0,    '__NOCONTENTCONVERT__', '__NOCC__', '__НЕ_КАНВЭРТАВАЦЬ_ТЭКСТ__' ),
	'currentweek'            => array( 1,    'CURRENTWEEK', 'БЯГУЧЫ_ТЫДЗЕНЬ' ),
	'currentdow'             => array( 1,    'CURRENTDOW', 'БЯГУЧЫ_ДЗЕНЬ_ТЫДНЯ' ),
	'localweek'              => array( 1,    'LOCALWEEK', 'ЛЯКАЛЬНЫ_ТЫДЗЕНЬ' ),
	'localdow'               => array( 1,    'LOCALDOW', 'ЛЯКАЛЬНЫ_ДЗЕНЬ_ТЫДНЯ' ),
	'revisionid'             => array( 1,    'REVISIONID', 'ID_ВЭРСІІ' ),
	'revisionday'            => array( 1,    'REVISIONDAY', 'ДЗЕНЬ_ВЭРСІІ' ),
	'revisionday2'           => array( 1,    'REVISIONDAY2', 'ДЗЕНЬ_ВЭРСІІ_2' ),
	'revisionmonth'          => array( 1,    'REVISIONMONTH', 'МЕСЯЦ_ВЭРСІІ' ),
	'revisionyear'           => array( 1,    'REVISIONYEAR', 'ГОД_ВЭРСІІ' ),
	'revisiontimestamp'      => array( 1,    'REVISIONTIMESTAMP', 'МОМАНТ_ЧАСУ_ВЭРСІІ' ),
	'plural'                 => array( 0,    'PLURAL:', 'МНОЖНЫ_ЛІК:'),
	'fullurl'                => array( 0,    'FULLURL:', 'ПОЎНЫ_АДРАС:' ),
	'fullurle'               => array( 0,    'FULLURLE:', 'ПОЎНЫ_АДРАС_2:' ),
	'lcfirst'                => array( 0,    'LCFIRST:', 'ПЕРШАЯ_ЛІТАРА_МАЛАЯ:' ),
	'ucfirst'                => array( 0,    'UCFIRST:', 'ПЕРШАЯ_ЛІТАРА_ВЯЛІКАЯ:' ),
	'lc'                     => array( 0,    'LC:', 'МАЛЫМІ_ЛІТАРАМІ:' ),
	'uc'                     => array( 0,    'UC:', 'ВЯЛІКІМІ_ЛІТАРАМІ:' ),
	'raw'                    => array( 0,    'RAW:', 'НЕАПРАЦАВАНЫ:' ),
	'displaytitle'           => array( 1,    'DISPLAYTITLE', 'ПАКАЗВАЦЬ_НАЗВУ' ),
	'rawsuffix'              => array( 1,    'R', 'Н' ),
	'newsectionlink'         => array( 1,    '__NEWSECTIONLINK__', '__СПАСЫЛКА_НА_НОВУЮ_СЭКЦЫЮ__' ),
	'currentversion'         => array( 1,    'CURRENTVERSION', 'БЯГУЧАЯ_ВЭРСІЯ' ),
	'currenttimestamp'       => array( 1,    'CURRENTTIMESTAMP', 'МОМАНТ_ЧАСУ' ),
	'localtimestamp'         => array( 1,    'LOCALTIMESTAMP', 'ЛЯКАЛЬНЫ_МОМАНТ_ЧАСУ' ),
	'directionmark'          => array( 1,    'DIRECTIONMARK', 'DIRMARK', 'СЫМБАЛЬ_НАПРАМКУ_ПІСЬМА' ),
	'language'               => array( 0,    '#LANGUAGE:', '#МОВА:' ),
	'contentlanguage'        => array( 1,    'CONTENTLANGUAGE', 'CONTENTLANG', 'МОВА_ЗЬМЕСТУ' ),
	'pagesinnamespace'       => array( 1,    'PAGESINNAMESPACE:', 'PAGESINNS:', 'КОЛЬКАСЬЦЬ_СТАРОНАК_У_ПРАСТОРЫ_НАЗВАЎ:' ),
	'numberofadmins'         => array( 1,    'NUMBEROFADMINS', 'КОЛЬКАСЬЦЬ_АДМІНІСТРАТАРАЎ' ),
	'formatnum'              => array( 0,    'FORMATNUM', 'ФАРМАТАВАЦЬ_ЛІК' ),
	'defaultsort'            => array( 1,    'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:',
	                                         'САРТЫРОЎКА_ПА_ЗМОЎЧВАНЬНІ:', 'КЛЮЧ_САРТЫРОЎКІ_ПА_ЗМОЎЧВАНЬНІ:',
	                                         'САРТЫРОЎКА_Ў_КАТЭГОРЫІ_ПА_ЗМОЎЧВАНЬНІ:' ),
	'filepath'               => array( 0,    'FILEPATH:', 'ШЛЯХ_ДА_ФАЙЛА:' ),
	'tag'                    => array( 0,    'tag', 'тэг' ),
);

$namespaceNames = array(
	NS_MEDIA          => 'Мэдыя',
	NS_SPECIAL        => 'Спэцыяльныя',
	NS_MAIN           => '',
	NS_TALK           => 'Абмеркаваньне',
	NS_USER           => 'Удзельнік',
	NS_USER_TALK      => 'Гутаркі_ўдзельніка',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Абмеркаваньне_$1',
	NS_IMAGE          => 'Выява',
	NS_IMAGE_TALK     => 'Абмеркаваньне_выявы',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Абмеркаваньне_MediaWiki',
	NS_TEMPLATE       => 'Шаблён',
	NS_TEMPLATE_TALK  => 'Абмеркаваньне_шаблёну',
	NS_HELP           => 'Дапамога',
	NS_HELP_TALK      => 'Абмеркаваньне_дапамогі',
	NS_CATEGORY       => 'Катэгорыя',
	NS_CATEGORY_TALK  => 'Абмеркаваньне_катэгорыі'
);

$namespaceAliases = array(
	'Удзельніца'          => NS_USER,
	'Гутаркі_ўдзельніцы'  => NS_USER_TALK,
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Двайныя_перанакіраваньні' ),
	'BrokenRedirects'           => array( 'Некарэктныя_перанакіраваньні' ),
	'Disambiguations'           => array( 'Неадназначнасьці' ),
	'CreateAccount'             => array( 'Стварыць_рахунак' ),
	'Watchlist'                 => array( 'Сьпіс_назіраньня' ),
	'Recentchanges'             => array( 'Апошнія_зьмены' ),
	'Upload'                    => array( 'Загрузка' ),
	'Imagelist'                 => array( 'Сьпіс_файлаў' ),
	'Newimages'                 => array( 'Новыя_файлы' ),
	'Listusers'                 => array( 'Сьпіс_удзельнікаў' ),
	'Statistics'                => array( 'Статыстыка' ),
	'Randompage'                => array( 'Выпадковая_старонка' ),
	'Lonelypages'               => array( 'Старонкі-сіраціны' ),
	'Uncategorizedpages'        => array( 'Некатэгарызаваныя_старонкі' ),
	'Uncategorizedcategories'   => array( 'Некатэгарызаваныя_катэгорыі' ),
	'Uncategorizedimages'       => array( 'Некатэгарызаваныя_файлы' ),
	'Uncategorizedtemplates'    => array( 'Некатэгарызаваныя_шаблёны' ),
	'Wantedpages'               => array( 'Запатрабаваныя_старонкі', 'Некарэктныя_спасылкі' ),
	'Wantedcategories'          => array( 'Запатрабаваныя_катэгорыі' ),
	'Shortpages'                => array( 'Кароткія_старонкі' ),
	'Longpages'                 => array( 'Доўгія_старонкі' ),
	'Newpages'                  => array( 'Новыя_старонкі' ),
	'Ancientpages'              => array( 'Найстарэйшыя_старонкі' ),
	'Deadendpages'              => array( 'Тупіковыя_старонкі' ),
	'Protectedpages'            => array( 'Абароненыя_старонкі' ),
	'Protectedtitles'           => array( 'Забароненыя_старонкі' ),
	'Allpages'                  => array( 'Усе_старонкі' ),
	'Specialpages'              => array( 'Спэцыяльныя_старонкі' ),
	'Contributions'             => array( 'Унёсак' ),
	'Emailuser'                 => array( 'Даслаць_ліст' ),
	'Whatlinkshere'             => array( 'Спасылкі_на_старонку' ),
	'Categories'                => array( 'Катэгорыі' ),
	'Export'                    => array( 'Экспарт' ),
	'Version'                   => array( 'Вэрсія' ),
	'Allmessages'               => array( 'Сыстэмныя_паведамленьні' ),
	'Log'                       => array( 'Журналы_падзей' ),
	'Blockip'                   => array( 'Блякаваньне' ),
	'Import'                    => array( 'Імпарт' ),
	'Listredirects'             => array( 'Сьпіс_перанакіраваньняў' ),
	'Randomredirect'            => array( 'Выпадковае_перанакіраваньне' ),
	'Mypage'                    => array( 'Мая_старонка' ),
	'Mytalk'                    => array( 'Мае_размовы' ),
	'Mycontributions'           => array( 'Мой_унёсак' ),
	'Listadmins'                => array( 'Сьпіс_адміністратараў' ),
	'Listbots'                  => array( 'Сьпіс_робатаў' ),
	'Popularpages'              => array( 'Папулярныя_старонкі' ),
	'Search'                    => array( 'Пошук' ),
	'MergeHistory'              => array( 'Гісторыя_аб\'яднаньняў' ),
	'Filepath'                  => array( 'Шлях_да_файла' ),
);

$separatorTransformTable = array(',' => '.', '.' => ',' );

$linkTrail = '/^([абвгґджзеёжзійклмнопрстуўфхцчшыьэюяćčłńśšŭźža-z]+)(.*)$/sDu';

$imageFiles = array(
	'button-bold'     => 'be-tarask/button_bold.png',
	'button-italic'   => 'be-tarask/button_italic.png',
	'button-link'     => 'be-tarask/button_link.png',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Падкрэсьліваць спасылкі:',
'tog-highlightbroken'         => 'Фарматаваць няісныя спасылкі <a href="" class="new">такім чынам</a> (інакш: такім чынам<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Выраўноўваць тэкст па шырыні старонкі',
'tog-hideminor'               => 'Хаваць дробныя зьмены ў сьпісе апошніх зьменаў',
'tog-extendwatchlist'         => 'Пашырыць сьпіс назіраньня, каб ён паказваў усе адпаведныя зьмены',
'tog-usenewrc'                => 'Удасканалены сьпіс апошніх зьменаў (патрабуе JavaScript)',
'tog-numberheadings'          => 'Аўтаматычная нумарацыя загалоўкаў',
'tog-showtoolbar'             => 'Паказваць панэль інструмэнтаў рэдагаваньня (патрабуе JavaScript)',
'tog-editondblclick'          => 'Рэдагаваць старонкі па падвойным шчоўканьні (JavaScript)',
'tog-editsection'             => 'Дазволіць рэдагаваньне асобных сэкцыяў па спасылках [рэдагаваць]',
'tog-editsectiononrightclick' => 'Рэдагаваць сэкцыі па правым шчоўканьні мышкай на загалоўку (JavaScript)',
'tog-showtoc'                 => 'Паказваць зьмест (для старонак з колькасьцю сэкцый болей за 3)',
'tog-rememberpassword'        => 'Запомніць мяне на гэтым кампутары',
'tog-editwidth'               => 'Поле рэдагаваньня ў поўную шырыню',
'tog-watchcreations'          => 'Дадаваць у мой сьпіс назіраньня старонкі, якія я буду ствараць',
'tog-watchdefault'            => 'Дадаваць у мой сьпіс назіраньня старонкі, якія я буду рэдагаваць',
'tog-watchmoves'              => 'Дадаваць у мой сьпіс назіраньня старонкі, якія я буду пераносіць',
'tog-watchdeletion'           => 'Дадаваць у мой сьпіс назіраньня старонкі, якія я буду выдаляць',
'tog-minordefault'            => 'Па змоўчаньні пазначаць усе зьмены дробнымі',
'tog-previewontop'            => 'Паказваць папярэдні прагляд артыкула над полем рэдагаваньня',
'tog-previewonfirst'          => 'Папярэдні прагляд пры першым рэдагаваньні',
'tog-nocache'                 => 'Адключыць кэшаваньне старонак',
'tog-enotifwatchlistpages'    => 'Паведамляць праз электронную пошту пра зьмены старонак ў маім сьпісе назіраньня',
'tog-enotifusertalkpages'     => 'Паведамляць праз электронную пошту пра зьмены маёй старонкі размоваў',
'tog-enotifminoredits'        => 'Паведамляць праз электронную пошту таксама пра дробныя зьмены старонак',
'tog-enotifrevealaddr'        => 'Не хаваць мой адрас электроннай пошты ў паведамленьнях',
'tog-shownumberswatching'     => 'Паказваць колькасьць назіральнікаў',
'tog-fancysig'                => 'Просты подпіс (без аўтаматычнай спасылкі)',
'tog-externaleditor'          => 'Па змоўчаньні выкарыстоўваць вонкавы рэдактар (толькі для адмыслоўцаў, патрэбуе спэцыяльных наладак на вашым кампутары)',
'tog-externaldiff'            => 'Выкарыстоўваць па змоўчаньні вонкавую праграму параўнаньня вэрсіяў (толькі для адмыслоўцаў, патрэбуе спэцыяльных наладак на вашым кампутары)',
'tog-showjumplinks'           => 'Актываваць дапаможныя спасылкі «перайсьці да»',
'tog-uselivepreview'          => 'Выкарыстоўваць хуткі папярэдні прагляд (JavaScript, экспэрымэнтальны)',
'tog-forceeditsummary'        => 'Папярэджваць пра адсутнасьць кароткага апісаньня зьменаў',
'tog-watchlisthideown'        => 'Хаваць мае праўкі ў сьпісе назіраньня',
'tog-watchlisthidebots'       => 'Хаваць праўкі робатаў у сьпісе назіраньня',
'tog-watchlisthideminor'      => 'Хаваць дробныя праўкі ў сьпісе назіраньня',
'tog-ccmeonemails'            => 'Дасылаць мне копіі лістоў, якія я дасылаю іншым удзельнікам і ўдзельніцам',
'tog-diffonly'                => 'Не паказваць зьмест старонкі пад параўнаньнем зьменаў',
'tog-showhiddencats'          => 'Паказваць схаваныя катэгорыі',

'underline-always'  => 'Заўсёды',
'underline-never'   => 'Ніколі',
'underline-default' => 'Як у браўзэры',

'skinpreview' => '(Прагляд)',

# Dates
'sunday'        => 'нядзеля',
'monday'        => 'панядзелак',
'tuesday'       => 'аўторак',
'wednesday'     => 'серада',
'thursday'      => 'чацьвер',
'friday'        => 'пятніца',
'saturday'      => 'субота',
'sun'           => 'Нд',
'mon'           => 'Пн',
'tue'           => 'Аў',
'wed'           => 'Ср',
'thu'           => 'Чц',
'fri'           => 'Пт',
'sat'           => 'Сб',
'january'       => 'студзень',
'february'      => 'люты',
'march'         => 'сакавік',
'april'         => 'красавік',
'may_long'      => 'травень',
'june'          => 'чэрвень',
'july'          => 'ліпень',
'august'        => 'жнівень',
'september'     => 'верасень',
'october'       => 'кастрычнік',
'november'      => 'лістапад',
'december'      => 'сьнежань',
'january-gen'   => 'студзеня',
'february-gen'  => 'лютага',
'march-gen'     => 'сакавіка',
'april-gen'     => 'красавіка',
'may-gen'       => 'траўня',
'june-gen'      => 'чэрвеня',
'july-gen'      => 'ліпеня',
'august-gen'    => 'жніўня',
'september-gen' => 'верасьня',
'october-gen'   => 'кастрычніка',
'november-gen'  => 'лістапада',
'december-gen'  => 'сьнежня',
'jan'           => '01',
'feb'           => '02',
'mar'           => '03',
'apr'           => '04',
'may'           => '05',
'jun'           => '06',
'jul'           => '07',
'aug'           => '08',
'sep'           => '09',
'oct'           => '10',
'nov'           => '11',
'dec'           => '12',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Катэгорыя|Катэгорыі|Катэгорыі}}',
'category_header'                => 'Старонкі ў катэгорыі «$1»',
'subcategories'                  => 'Падкатэгорыі',
'category-media-header'          => 'Файлы ў катэгорыі «$1»',
'category-empty'                 => "''Гэтая катэгорыя ня ўтрымлівае ні старонак, ні файлаў.''",
'hidden-categories'              => '{{PLURAL:$1|Схаваная катэгорыя|Схаваныя катэгорыі}}',
'hidden-category-category'       => 'Схаваныя катэгорыі', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Гэтая катэгорыя зьмяшчае наступную падкатэгорыю.|Гэтая катэгорыя зьмяшчае {{PLURAL:$1|наступную $1 падкатэгорыю|наступныя $1 падкатэгорыі|наступныя $1 падкатэгорыяў}}, з агульнай колькасьці $2.}}',
'category-subcat-count-limited'  => 'У гэтай катэгорыі $1 {{PLURAL:$1|падкатэгорыя|падкатэгорыі|падкатэгорыяў}}.',
'category-article-count'         => '{{PLURAL:$2|Гэтая катэгорыя ўтрымлівае толькі адну старонку.|{{PLURAL:$1|Паказана $1 старонка|Паказаныя $1 старонкі|Паказаныя $1 старонак}} гэтай катэгорыі з $2.}}',
'category-article-count-limited' => 'У гэтай катэгорыі $1 {{PLURAL:$1|старонка|старонкі|старонак}}.',
'category-file-count'            => '{{PLURAL:$2|Гэтая катэгорыя ўтрымлівае толькі адзін файл.|{{PLURAL:$1|Паказаны $1 файл|Паказаныя $1 файлы|Паказаныя $1 файлаў}} гэтай катэгорыі з $2.}}',
'category-file-count-limited'    => 'У гэтай катэгорыі — $1 {{PLURAL:$1|файл|файлы|файлаў}}.',
'listingcontinuesabbrev'         => ' (працяг)',

'mainpagetext'      => "<big>'''MediaWiki пасьпяхова ўсталяваная.'''</big>",
'mainpagedocfooter' => 'Глядзіце [http://meta.wikimedia.org/wiki/Help:Contents дапаможнік карыстальніка] для атрыманьня інфармацыі па карыстаньні вікі-праграмамі.

== З чаго пачаць ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Сьпіс парамэтраў канфігурацыі]
* [http://www.mediawiki.org/wiki/Manual:FAQ Частыя пытаньні MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo//mediawiki-announce Рассылка паведамленьняў пра зьяўленьне новых вэрсіяў MediaWiki]',

'about'          => 'Пра',
'article'        => 'Артыкул',
'newwindow'      => '(адчыняецца ў новым акне)',
'cancel'         => 'Адмяніць',
'qbfind'         => 'Знайсьці',
'qbbrowse'       => 'Праглядзець',
'qbedit'         => 'Рэдагаваць',
'qbpageoptions'  => 'Гэтая старонка',
'qbpageinfo'     => 'Інфармацыя пра старонку',
'qbmyoptions'    => 'Мае старонкі',
'qbspecialpages' => 'Спэцыяльныя старонкі',
'moredotdotdot'  => 'Далей…',
'mypage'         => 'Мая старонка',
'mytalk'         => 'Мае размовы',
'anontalk'       => 'Гутаркі для гэтага IP-адрасу',
'navigation'     => 'Навігацыя',
'and'            => 'і',

# Metadata in edit box
'metadata_help' => 'Мэтазьвесткі:',

'errorpagetitle'    => 'Памылка',
'returnto'          => 'Вярнуцца да $1.',
'tagline'           => 'Зьвесткі зь {{GRAMMAR:родны|{{SITENAME}}}}',
'help'              => 'Дапамога',
'search'            => 'Пошук',
'searchbutton'      => 'Пошук',
'go'                => 'Старонка',
'searcharticle'     => 'Старонка',
'history'           => 'Гісторыя старонкі',
'history_short'     => 'Гісторыя',
'updatedmarker'     => 'абноўлена з часу майго апошняга наведваньня',
'info_short'        => 'Інфармацыя',
'printableversion'  => 'Вэрсія для друку',
'permalink'         => 'Сталая спасылка',
'print'             => 'Друкаваць',
'edit'              => 'Рэдагаваць',
'create'            => 'Стварыць',
'editthispage'      => 'Рэдагаваць гэтую старонку',
'create-this-page'  => 'Стварыць гэтую старонку',
'delete'            => 'Выдаліць',
'deletethispage'    => 'Выдаліць гэтую старонку',
'undelete_short'    => 'Аднавіць $1 {{PLURAL:$1|рэдагаваньне|рэдагаваньні|рэдагаваньняў}}',
'protect'           => 'Абараніць',
'protect_change'    => 'зьмяніць',
'protectthispage'   => 'Абараніць гэтую старонку',
'unprotect'         => 'Зьняць абарону',
'unprotectthispage' => 'Зьняць абарону з гэтай старонкі',
'newpage'           => 'Новая старонка',
'talkpage'          => 'Абмеркаваць гэтую старонку',
'talkpagelinktext'  => 'гутаркі',
'specialpage'       => 'Спэцыяльная старонка',
'personaltools'     => 'Асабістыя прылады',
'postcomment'       => 'Пракамэнтаваць',
'articlepage'       => 'Паказаць артыкул',
'talk'              => 'Гутаркі',
'views'             => 'Прагляды',
'toolbox'           => 'Інструмэнты',
'userpage'          => 'Паказаць старонку ўдзельніка',
'projectpage'       => 'Паказаць старонку праекту',
'imagepage'         => 'Паказаць старонку мэдыя-файла',
'mediawikipage'     => 'Паказаць старонку паведамленьня',
'templatepage'      => 'Паказаць старонку шаблёну',
'viewhelppage'      => 'Паказаць старонку дапамогі',
'categorypage'      => 'Паказаць старонку катэгорыі',
'viewtalkpage'      => 'Паказаць абмеркаваньне',
'otherlanguages'    => 'На іншых мовах',
'redirectedfrom'    => '(Перанакіраваная з $1)',
'redirectpagesub'   => 'Старонка-перанакіраваньне',
'lastmodifiedat'    => 'Гэтая старонка апошні раз рэдагавалася $2, $1.', # $1 date, $2 time
'viewcount'         => 'Гэтую старонку праглядалі $1 {{PLURAL:$1|раз|разы|разоў}}.',
'protectedpage'     => 'Абароненая старонка',
'jumpto'            => 'Перайсьці да:',
'jumptonavigation'  => 'навігацыя',
'jumptosearch'      => 'пошук',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Пра {{GRAMMAR:вінавальны|{{SITENAME}}}}',
'aboutpage'            => 'Project:Пра {{GRAMMAR:вінавальны|{{SITENAME}}}}',
'bugreports'           => 'Паведамленьні пра памылкі',
'bugreportspage'       => 'Project:Паведамленьні пра памылкі',
'copyright'            => 'Зьмест старонкі падпадае пад ліцэнзію $1.',
'copyrightpagename'    => 'Аўтарскія правы {{GRAMMAR:родны|{{SITENAME}}}}',
'copyrightpage'        => '{{ns:project}}:Аўтарскія правы',
'currentevents'        => 'Актуальныя падзеі',
'currentevents-url'    => 'Project:Актуальныя падзеі',
'disclaimers'          => 'Адмова ад адказнасьці',
'disclaimerpage'       => 'Project:Адмова ад адказнасьці',
'edithelp'             => 'Дапамога ў рэдагаваньні',
'edithelppage'         => 'Help:Рэдагаваньне',
'faq'                  => 'Частыя пытаньні',
'faqpage'              => 'Project:Частыя пытаньні',
'helppage'             => 'Help:Зьмест',
'mainpage'             => 'Галоўная старонка',
'mainpage-description' => 'Галоўная старонка',
'policy-url'           => 'Project:Правілы',
'portal'               => 'Суполка',
'portal-url'           => 'Project:Суполка',
'privacy'              => 'Правілы адносна прыватнасьці',
'privacypage'          => 'Project:Правілы адносна прыватнасьці',

'badaccess'        => 'Памылка: не адпаведны ўзровень правоў',
'badaccess-group0' => 'Вам не дазволена выканаць запытанае Вамі дзеяньне.',
'badaccess-group1' => 'Запытанае Вамі дзеяньне дазволенае толькі ўдзельнікам з групы $1.',
'badaccess-group2' => 'Запытанае Вамі дзеяньне дазволенае толькі ўдзельнікам адной з групаў $1.',
'badaccess-groups' => 'Запытанае Вамі дзеяньне дазволенае толькі ўдзельнікам адной з групаў $1.',

'versionrequired'     => 'Патрабуецца MediaWiki вэрсіі $1',
'versionrequiredtext' => 'Для карыстаньня гэтай старонкай патрабуецца MediaWiki вэрсіі $1. Глядзіце [[Special:Version|інфармацыю пра вэрсію]].',

'ok'                      => 'Добра',
'pagetitle'               => '$1 — {{SITENAME}}',
'retrievedfrom'           => 'Атрымана з «$1»',
'youhavenewmessages'      => 'Вы маеце $1 ($2).',
'newmessageslink'         => 'новыя паведамленьні',
'newmessagesdifflink'     => 'апошняя зьмена',
'youhavenewmessagesmulti' => 'Вы атрымалі новыя паведамленьні на $1',
'editsection'             => 'рэдагаваць',
'editold'                 => 'рэдагаваць',
'viewsourceold'           => 'паказаць крыніцу',
'editsectionhint'         => 'Рэдагаваць сэкцыю «$1»',
'toc'                     => 'Зьмест',
'showtoc'                 => 'паказаць',
'hidetoc'                 => 'схаваць',
'thisisdeleted'           => 'Праглядзець ці аднавіць $1?',
'viewdeleted'             => 'Паказаць $1?',
'restorelink'             => '$1 {{PLURAL:$1|выдаленую зьмену|выдаленыя зьмены|выдаленых зьменаў}}',
'feedlinks'               => 'Стужка:',
'feed-invalid'            => 'Памылковы тып стужкі.',
'feed-unavailable'        => 'Стужкі не працуюць',
'site-rss-feed'           => '$1 — RSS-стужка',
'site-atom-feed'          => '$1 — Atom-стужка',
'page-rss-feed'           => '«$1» — RSS-стужка',
'page-atom-feed'          => '«$1» — Atom-стужка',
'red-link-title'          => '$1 (яшчэ не напісаны)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Старонка',
'nstab-user'      => 'Старонка ўдзельніка/ўдзельніцы',
'nstab-media'     => 'Мэдыя',
'nstab-special'   => 'Спэцыяльная',
'nstab-project'   => 'Старонка праекту',
'nstab-image'     => 'Файл',
'nstab-mediawiki' => 'Паведамленьне',
'nstab-template'  => 'Шаблён',
'nstab-help'      => 'Старонка дапамогі',
'nstab-category'  => 'Катэгорыя',

# Main script and global functions
'nosuchaction'      => 'Няма такога дзеяньня',
'nosuchactiontext'  => 'Дзеяньне, пазначанае ў радку URL, не распазнанае праграмным забесьпячэньнем {{GRAMMAR:родны|{{SITENAME}}}}',
'nosuchspecialpage' => 'Такой спэцыяльнай старонкі не існуе',
'nospecialpagetext' => "<big>'''Вы зьвярнуліся на няслушную спэцыяльную старонку.'''</big>

Сьпіс дзейных спэцыяльных старонак ёсьць на [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Памылка',
'databaseerror'        => 'Памылка базы зьвестак',
'dberrortext'          => 'Выяўлена памылка сынтаксісу ў звароце да базы зьвестак.
Магчыма гэта памылка праграмнага забесьпячэньня.
Апошні запыт да базы:
<blockquote><tt>$1</tt></blockquote>
адбыўся з функцыі «<tt>$2</tt>».
MySQL вярнуў памылку «<tt>$3: $4</tt>».',
'dberrortextcl'        => 'Выяўлена памылка сынтаксісу ў звароце да базы зьвестак.
Апошні запыт да базы:
«$1»
адбыўся з функцыі «$2».
MySQL вярнуў памылку «$3: $4»',
'noconnect'            => '{{SITENAME}} мае тэхнічныя цяжкасьці і ня можа далучыцца да сэрвэра базы зьвестак.<br />
$1',
'nodb'                 => 'Немагчыма выбраць базу зьвестак $1',
'cachederror'          => 'Наступнае — кэшаваная копія запытанай старонкі; магчыма, яна ўжо не адпавядае рэчаіснасьці.',
'laggedslavemode'      => 'Увага: старонка можа ня ўтрымліваць апошніх зьменаў.',
'readonly'             => 'База зьвестак заблякаваная',
'enterlockreason'      => 'Пазначце прычыну блякаваньня і вызначаны час разблякаваньня',
'readonlytext'         => 'База зьвестак заблякаваная для дадаваньня новых старонак і іншых зьменаў, верагодна з прычыны тэхнічнага абслугоўваньня, пасьля якога будзе адноўлена звычайная праца.

Адміністратар, які заблякаваў базу зьвестак, пакінуў наступнае тлумачэньне: $1',
'missing-article'      => 'У базе зьвестак ня знойдзены тэкст старонкі «$1» $2.

Звычайна гэта адбываецца ў выпадку пераходу па састарэлай розьніцы альбо спасылцы гісторыі старонкі, якая была выдаленая.

Калі справа ня ў гэтым, верагодна Вы знайшлі памылку ў праграмным забесьпячэньні.
Калі ласка, паведаміце пра гэта [[Special:ListUsers/sysop|адміністратару]], пазначыўшы URL.',
'missingarticle-rev'   => '(вэрсія № $1)',
'missingarticle-diff'  => '(Розьніца: $1, $2)',
'readonly_lag'         => 'База зьвестак была аўтаматычна заблякаваная да выкананьня сынхранізацыі паміж першасным і другаснымі базамі зьвестак',
'internalerror'        => 'Унутраная памылка',
'internalerror_info'   => 'Унутраная памылка: $1',
'filecopyerror'        => 'Немагчыма cкапіяваць файл «$1» у «$2».',
'filerenameerror'      => 'Немагчыма перайменаваць файл «$1» у «$2».',
'filedeleteerror'      => 'Немагчыма выдаліць файл «$1».',
'directorycreateerror' => 'Немагчыма стварыць дырэкторыю «$1».',
'filenotfound'         => 'Немагчыма знайсьці файл «$1».',
'fileexistserror'      => 'Немагчыма запісаць у файл «$1»: файл існуе',
'unexpected'           => 'Нечаканае значэньне: «$1»=«$2».',
'formerror'            => 'Памылка: не атрымалася падаць форму',
'badarticleerror'      => 'Гэтае дзеяньне немагчыма выканаць на гэтай старонцы.',
'cannotdelete'         => 'Немагчыма выдаліць указаную старонку альбо выяву. (Магчыма, яна ўжо выдаленая кімсьці іншым.)',
'badtitle'             => 'Некарэктная назва',
'badtitletext'         => 'Запытаная назва старонкі няслушная ці пустая, альбо няслушна ўказаная міжмоўная ці міжвікі назва. Яна можа ўтрымліваць сымбалі, якія нельга ўжываць у назвах.',
'perfdisabled'         => 'Прабачце! Гэтая магчымасьць часова заблякаваная, у сувязі зь перагружанасьцю сэрвэра {{GRAMMAR:родны|{{SITENAME}}}}.',
'perfcached'           => 'Наступныя зьвесткі кэшаваныя і могуць быць састарэлымі.',
'perfcachedts'         => 'Наступныя зьвесткі кэшаваныя і апошні раз былі абноўленыя $1.',
'querypage-no-updates' => 'Абнаўленьні гэтай старонкі цяпер адключаныя. Зьвесткі ня будуць абнаўляцца.',
'wrong_wfQuery_params' => 'Няслушныя парамэтры для wfQuery()<br />
Функцыя: $1<br />
Запыт: $2',
'viewsource'           => 'Паказаць крыніцу',
'viewsourcefor'        => 'для $1',
'actionthrottled'      => 'Дзеяньне прытрыманае',
'actionthrottledtext'  => 'У межах барацьбы са спамам, Вы абмежаваныя ў выкананьні гэтага дзеяньня надта часта ў кароткі прамежак часу, і Вы перавысілі гэты ліміт. Калі ласка, паспрабуйце яшчэ раз празь некалькі хвілінаў.',
'protectedpagetext'    => 'Рэдагаваньне гэтай старонкі забароненае.',
'viewsourcetext'       => 'Вы можаце праглядаць і капіяваць пачатковы тэкст гэтай старонкі:',
'protectedinterface'   => 'Гэтая старонка ўтрымлівае інтэрфэйснае паведамленьне праграмнага забесьпячэньня, і яе зьмена забаронена.',
'editinginterface'     => "'''Увага:''' Вы рэдагуеце старонку, якая ўтрымлівае сыстэмнае паведамленьне MediaWiki.
Яе зьмена паўплывае на вонкавы выгляд інтэрфэйсу іншых удзельнікаў.
Калі ласка, выкарыстоўвайце [http://translatewiki.net/wiki/Main_Page?setlang=be-tarask Betawiki] — праект для лякалізацыі MediaWiki.",
'sqlhidden'            => '(SQL-запыт схаваны)',
'cascadeprotected'     => 'Гэтая старонка абароненая ад рэдагаваньня, таму што яна ўключаная ў {{PLURAL:$1|наступную $1 старонку, якая была абароненая|наступныя $1 старонкі, якія былі абароненыя|наступныя $1 старонкі, якія былі абароненыя}} з актывізаванай «каскаднай» опцыяй:
$2',
'namespaceprotected'   => "Вы ня маеце правоў на рэдагаваньне старонак у прасторы назваў '''$1'''.",
'customcssjsprotected' => 'Вам забаронена рэдагаваць гэтую старонку, таму што яна ўтрымлівае пэрсанальныя ўстаноўкі іншага ўдзельніка.',
'ns-specialprotected'  => 'Немагчыма рэдагаваць спэцыяльныя старонкі.',
'titleprotected'       => "Стварэньне старонкі з такой назвай было забароненае ўдзельнікам [[User:$1|$1]].
Прычына забароны: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Няслушная канфігурацыя: невядомы антывірусны сканэр: <i>$1</i>',
'virus-scanfailed'     => 'памылка сканаваньня (код $1)',
'virus-unknownscanner' => 'невядомы антывірус:',

# Login and logout pages
'logouttitle'                => 'Выхад з сыстэмы',
'logouttext'                 => '<strong>Вы выйшлі з сыстэмы.</strong>

Вы можаце працягваць працу ў {{GRAMMAR:месны|{{SITENAME}}}} ананімна, альбо можаце [[Special:UserLogin|ўвайсьці ў сыстэму]] як той жа альбо іншы ўдзельнік.
Але некаторыя старонкі могуць адлюстроўвацца як быццам Вы ўсё яшчэ ў сыстэме. Каб гэтага пазьбегнуць трэба ачысьціць кэш браўзэра.',
'welcomecreation'            => '== Вітаем, $1! ==
Ваш рахунак быў створаны.
Не забудзьце зьмяніць Вашыя [[Special:Preferences|ўстаноўкі ў {{GRAMMAR:месны|{{SITENAME}}}}]].',
'loginpagetitle'             => 'Уваход у сыстэму',
'yourname'                   => 'Імя ўдзельніка/ўдзельніцы:',
'yourpassword'               => 'Пароль:',
'yourpasswordagain'          => 'Паўтарыце пароль:',
'remembermypassword'         => 'Запомніць мяне на гэтым кампутары',
'yourdomainname'             => 'Ваш дамэн:',
'externaldberror'            => 'Магчыма, адбылася памылка аўтэнтыфікацыі з дапамогай вонкавай базы зьвестак, ці Вам недазволена абнаўляць свой рахунак.',
'loginproblem'               => '<b>Узьнікла праблема пры ўваходзе ў сыстэму.</b><br />Паспрабуйце яшчэ раз!',
'login'                      => 'Увайсьці',
'nav-login-createaccount'    => 'Уваход / стварэньне рахунку',
'loginprompt'                => 'Вы павінны дазволіць cookie для ўваходу ў {{GRAMMAR:вінавальны|{{SITENAME}}}}.',
'userlogin'                  => 'Стварэньне рахунку ці ўваход',
'logout'                     => 'Выйсьці',
'userlogout'                 => 'Выйсьці',
'notloggedin'                => 'Вы не ўвайшлі ў сыстэму',
'nologin'                    => 'Ня маеце рахунку? $1.',
'nologinlink'                => 'Стварыце рахунак',
'createaccount'              => 'Стварыць новы рахунак',
'gotaccount'                 => 'Ужо маеце рахунак? $1.',
'gotaccountlink'             => 'Увайдзіце',
'createaccountmail'          => 'па электроннай пошце',
'badretype'                  => 'Уведзеныя Вамі паролі не супадаюць.',
'userexists'                 => 'Выбранае Вамі імя ўдзельніка/ўдзельніцы ўжо выкарыстоўваецца кімсьці іншым. Выберыце іншае імя.',
'youremail'                  => 'Адрас электроннай пошты:',
'username'                   => 'Імя ўдзельніка/ўдзельніцы:',
'uid'                        => 'ID удзельніка/удзельніцы:',
'prefs-memberingroups'       => 'Удзельнік {{PLURAL:$1|групы|групаў}}:',
'yourrealname'               => 'Сапраўднае імя:',
'yourlanguage'               => 'Мова інтэрфэйсу:',
'yournick'                   => 'Подпіс:',
'badsig'                     => 'Няслушны неапрацаваны подпіс; праверце HTML-тэгі.',
'badsiglength'               => 'Подпіс занадта доўгі.
Мусіць быць не даўжэй за $1 {{PLURAL:$1|сымбаль|сымбалі|сымбаляў}}.',
'email'                      => 'Электронная пошта',
'prefs-help-realname'        => 'Сапраўднае імя паведамляць неабавязкова.
Калі Вы яго пазначыце, яно будзе выкарыстоўвацца для пазначэньня Вашай працы.',
'loginerror'                 => 'Памылка ўваходу',
'prefs-help-email'           => 'Адрас электроннай пошты — неабавязковы, але дае магчымасьць даслаць Вам пароль, калі Вы забылі яго. Таксама Адрас электроннай пошты дазваляе іншым зьвязвацца з Вамі праз старонкі ўдзельніка безь неабходнасьці раскрыцьця Вашых асабістых зьвестак.',
'prefs-help-email-required'  => 'Патрабуецца адрас электроннай пошты.',
'nocookiesnew'               => 'Рахунак быў створаны, але ў сыстэму Вы не ўвайшлі.
{{SITENAME}} выкарыстоўвае cookie для ўваходу ў сыстэму.
У Вашым браўзэры cookies адключаныя.
Калі ласка, дазвольце іх, а потым ўвайдзіце з новым іменем удзельніка і паролем.',
'nocookieslogin'             => '{{SITENAME}} выкарыстоўвае cookie для ўваходу ў сыстэму.
У Вашым браўзэры cookie адключаныя.
Калі ласка, дазвольце іх і паспрабуйце яшчэ раз.',
'noname'                     => 'Вы пазначылі няслушнае імя ўдзельніка.',
'loginsuccesstitle'          => 'Пасьпяховы ўваход у сыстэму',
'loginsuccess'               => "'''Цяпер Вы ўвайшлі ў {{GRAMMAR:вінавальны|{{SITENAME}}}} як «$1».'''",
'nosuchuser'                 => 'Не існуе ўдзельніка ці ўдзельніцы «$1».
Праверце напісаньне альбо [[Special:Userlogin/signup|стварыце новы рахунак]].',
'nosuchusershort'            => 'Не існуе ўдзельніка ці ўдзельніцы «<nowiki>$1</nowiki>». Праверце напісаньне.',
'nouserspecified'            => 'Вы мусіце пазначыць імя ўдзельніка.',
'wrongpassword'              => 'Уведзены няслушны пароль. Калі ласка, паспрабуйце яшчэ раз.',
'wrongpasswordempty'         => 'Быў уведзены пусты пароль. Калі ласка, паспрабуйце яшчэ раз.',
'passwordtooshort'           => 'Ваш пароль няслушны альбо занадта кароткі.
Ён павінен утрымліваць ня меней за $1 {{PLURAL:$1|сымбаль|сымбалі|сымбаляў}} і адрозьнівацца ад імя ўдзельніка.',
'mailmypassword'             => 'Даслаць новы пароль па электроннай пошце',
'passwordremindertitle'      => 'Новы часовы пароль для {{GRAMMAR:родны|{{SITENAME}}}}',
'passwordremindertext'       => 'Нехта (магчыма Вы, з IP-адрасу $1) запытаў нас даслаць новы пароль для {{GRAMMAR:родны|{{SITENAME}}}} ($4). Для ўдзельніка «$2» быў створаны часовы пароль і ён цяпер «$3». Калі гэта была Вашая ініцыятыва, Вам трэба ўвайсьці ў сыстэму і адразу зьмяніць пароль.

Калі гэты запыт адправіў нехта іншы, альбо Вы ўспомнілі свой пароль і ўжо не жадаеце яго зьмяніць, Вы можаце праігнараваць гэты ліст і працягваць карыстацца старым паролем.',
'noemail'                    => 'Удзельнік «$1» не пазначыў ніякага адрасу электроннай пошты.',
'passwordsent'               => 'Новы пароль быў дасланы на адрас электроннай пошты ўдзельніка «$1».
Калі ласка, увайдзіце ў сыстэму пасьля яго атрыманьня.',
'blocked-mailpassword'       => 'З Вашага IP-адрасу забароненыя рэдагаваньні, а таму таксама не даступная функцыя аднаўленьня паролю дзеля прадухіленьня шкоды.',
'eauthentsent'               => 'Пацьверджаньне было дасланае на пазначаны адрас электроннай пошты.
У лісьце ўтрымліваюцца інструкцыі, па выкананьні якіх, Вы зможаце пацьвердзіць, што адрас сапраўды належыць Вам, і на гэты адрас будзе дасылацца пошта адсюль.',
'throttled-mailpassword'     => 'Напамін паролю ўжо быў дасланы на працягу {{PLURAL:$1|апошняй $1 гадзіны|апошніх $1 гадзінаў|апошніх $1 гадзінаў}}.
Для прадухіленьня злоўжываньняў, напамін будзе дасылацца не часьцей аднаго разу за $1 {{PLURAL:$1|гадзіну|гадзіны|гадзінаў}}.',
'mailerror'                  => 'Памылка пры адпраўцы электроннай пошты: $1',
'acct_creation_throttle_hit' => 'На жаль, Вы ўжо стварылі $1 {{PLURAL:$1|рахунак|рахункі|рахункаў}}. Болей нельга.',
'emailauthenticated'         => 'Ваш адрас электроннай пошты быў пацьверджаны $1.',
'emailnotauthenticated'      => 'Ваш адрас электроннай пошты яшчэ не пацьверджаны.
Электронная пошта для наступных магчымасьцяў дасылацца не будзе.',
'noemailprefs'               => 'Вы павінны пазначыць адрас электроннай пошты, каб актывізаваць гэтыя магчымасьці.',
'emailconfirmlink'           => 'Пацьвердзіць Ваш адрас электроннай пошты',
'invalidemailaddress'        => 'Уведзены адрас электроннай пошты не адпавядае фармату адрасоў электроннай пошты.
Увядзіце слушны адрас электроннай пошты ці ачысьціце гэтае поле.',
'accountcreated'             => 'Рахунак створаны',
'accountcreatedtext'         => 'Рахунак удзельніка для $1 быў створаны.',
'createaccount-title'        => 'Стварэньне рахунку ў {{GRAMMAR:месны|{{SITENAME}}}}',
'createaccount-text'         => 'Нехта стварыў рахунак «$2» у {{GRAMMAR:месны|{{SITENAME}}}} ($4) для Вашага адрасу электроннай пошты. Пароль для гэтага рахунку — «$3». Вам трэба ўвайсьці і зьмяніць Ваш пароль зараз.

Вы можаце праігнараваць гэты ліст, калі гэты рахунак быў створаны памылкова.',
'loginlanguagelabel'         => 'Мова: $1',

# Password reset dialog
'resetpass'               => 'Выдаліць пароль рахунку',
'resetpass_announce'      => 'Вы ўвайшлі з дапамогай часовага паролю, дасланага праз электронную пошту.
Для завяршэньня ўваходу ў сыстэму Вы мусіце ўвесьці тут новы пароль:',
'resetpass_header'        => 'Сьцерці пароль',
'resetpass_submit'        => 'Наставіць пароль і увайсьці',
'resetpass_success'       => 'Ваш пароль быў пасьпяхова зьменены! Выконваецца ўваход у сыстэму…',
'resetpass_bad_temporary' => 'Няслушны часовы пароль.
Магчыма Вы ўжо зьмянілі пароль альбо запыталі новы часовы пароль.',
'resetpass_forbidden'     => 'Пароль ня можа быць зьменены',
'resetpass_missing'       => 'Няма зьвестак ў форме.',

# Edit page toolbar
'bold_sample'     => 'Тлусты тэкст',
'bold_tip'        => 'Тлусты тэкст',
'italic_sample'   => 'Курсіўны тэкст',
'italic_tip'      => 'Курсіўны тэкст',
'link_sample'     => 'Загаловак спасылкі',
'link_tip'        => 'Унутраная спасылка',
'extlink_sample'  => 'http://www.example.com загаловак спасылкі',
'extlink_tip'     => 'Вонкавая спасылка (не забывайцеся пачынаць з http:// )',
'headline_sample' => 'Тэкст загалоўку',
'headline_tip'    => 'Загаловак 2-га ўзроўню',
'math_sample'     => 'Зьмясьціце тут формулу',
'math_tip'        => 'Матэматычная формула (LaTeX)',
'nowiki_sample'   => 'Пішыце сюды нефарматаваны тэкст',
'nowiki_tip'      => 'Ігнараваць вікі-фарматаваньне',
'image_sample'    => 'Прыклад.jpg',
'image_tip'       => 'Укладзены файл',
'media_sample'    => 'Прыклад.ogg',
'media_tip'       => 'Спасылка на файл',
'sig_tip'         => 'Ваш подпіс і момант часу',
'hr_tip'          => 'Гарызантальная лінія (не выкарыстоўвайце часта)',

# Edit pages
'summary'                          => 'Кароткае апісаньне зьменаў',
'subject'                          => 'Тэма/назва',
'minoredit'                        => 'Гэта дробная праўка',
'watchthis'                        => 'Назіраць за гэтай старонкай',
'savearticle'                      => 'Захаваць старонку',
'preview'                          => 'Прагляд',
'showpreview'                      => 'Праглядзець',
'showlivepreview'                  => 'Хуткі папярэдні прагляд',
'showdiff'                         => 'Паказаць зьмены',
'anoneditwarning'                  => "'''Папярэджаньне:''' Вы не ўвайшлі. Ваш IP-адрас будзе запісаны ў гісторыі гэтай старонкі.",
'missingsummary'                   => "'''Напамін:''' Вы не пазначылі кароткае апісаньне зьменаў.
Калі Вы націсьніце кнопку «Запісаць» яшчэ раз, Вашае рэдагаваньне будзе запісанае без апісаньня.",
'missingcommenttext'               => 'Калі ласка, увядзіце камэнтар ніжэй.',
'missingcommentheader'             => "'''Напамін:''' Вы не пазначылі тэмы гэтага камэнтара.
Калі Вы націсьніце кнопку «Захаваць» яшчэ раз, Ваш камэнтар захаваецца бяз тэмы.",
'summary-preview'                  => 'Папярэдні прагляд апісаньня',
'subject-preview'                  => 'Папярэдні прагляд загалоўку',
'blockedtitle'                     => 'Удзельнік заблякаваны',
'blockedtext'                      => "<big>'''Ваш рахунак ўдзельніка ці IP-адрас быў заблякаваны.'''</big>

Блякаваньне выканаў $1.
Прычына гэтага: ''$2''.

* Пачатак блякаваньня: $8
* Сканчэньне блякаваньня: $6
* Быў заблякаваны: $7

Вы можаце скантактавацца з $1 ці адным зь іншых [[{{MediaWiki:Grouppage-sysop}}|адміністратараў]], каб абмеркаваць блякаваньне. Заўважце, што Вы ня зможаце ўжыць мажлівасьць «даслаць ліст па электроннай пошце», пакуль не пазначыце сапраўдны адрас электроннай пошты ў Вашых [[Special:Preferences|устаноўках]], і калі гэта Вам не было забаронена.
Ваш IP-адрас — $3, ідэнтыфікатар блякаваньня — #$5.
Калі ласка, улучайце ўсю вышэйпададзеную інфармацыю ва ўсе запыты, што Вы будзеце рабіць.",
'autoblockedtext'                  => "Ваш IP-адрас быў аўтаматычна заблякаваны, таму што ён ужываўся іншым удзельнікам, які быў заблякаваны $1.
Прычына гэтага:

:''$2''

* Блякаваньне пачалося: $8
* Блякаваньне скончыцца: $6
* Быў заблякаваны: $7

Вы можаце скантактавацца з $1 ці з адным зь іншых [[{{MediaWiki:Grouppage-sysop}}|адміністратараў]], каб абмеркаваць блякаваньне.

Заўважце, што Вы ня зможаце ужываць мажлівасьць «даслаць ліст праз электронную пошту», пакуль ня будзе пазначаны дзейны адрас электроннай пошты ў Вашых [[Special:Preferences|устаноўках удзельніка]], і калі гэта Вам не было забаронена.

Ваш цяперашні IP-адрас — $3, ідэнтыфікатар блякаваньня — #$5.
Калі ласка, улучайце ўсю вышэйпададзеную інфармацыю ва ўсе запыты, што Вы будзеце рабіць.",
'blockednoreason'                  => 'прычына не пададзена',
'blockedoriginalsource'            => "Крыніца '''$1''' паказана ніжэй:",
'blockededitsource'                => "Тэкст '''Вашых зьменаў''' у '''$1''' паказаны ніжэй:",
'whitelistedittitle'               => 'Для рэдагаваньня патрабуецца ўваход у сыстэму',
'whitelistedittext'                => 'Вам трэба $1, каб рэдагаваць старонкі.',
'confirmedittitle'                 => 'Для рэдагаваньня патрэбнае пацьверджаньне адрасу электроннай пошты',
'confirmedittext'                  => 'Вы мусіце пацьвердзіць Ваш адрас электроннай пошты перад рэдагаваньнем старонак. Калі ласка, пазначце і пацьвердзіце адрас электроннай пошты праз Вашы [[Special:Preferences|ўстаноўкі]].',
'nosuchsectiontitle'               => 'Такой сэкцыі не існуе',
'nosuchsectiontext'                => 'Вы спрабуеце рэдагаваць сэкцыю, якой не існуе.
Паколькі сэкцыі $1 не існуе, няма куды захоўваць Вашыя рэдагаваньні.',
'loginreqtitle'                    => 'Патрабуецца ўваход у сыстэму',
'loginreqlink'                     => 'ўвайсьці',
'loginreqpagetext'                 => 'Вы мусіце $1, каб праглядаць іншыя старонкі.',
'accmailtitle'                     => 'Пароль адасланы.',
'accmailtext'                      => 'Пароль для «$1» быў адасланы на адрас $2.',
'newarticle'                       => '(Новы)',
'newarticletext'                   => "Вы прыйшлі па спасылцы на старонку, якая яшчэ не існуе.
Каб стварыць яе, напішыце тэкст у полі ніжэй (глядзіце [[{{MediaWiki:Helppage}}|старонку дапамогі]] дзеля дадатковай інфармацыі).
Калі Вы трапілі сюды памылкова, націсьніце '''назад''' у вашым браўзэры.",
'anontalkpagetext'                 => "----''Гэта старонка гутарак ананімнага ўдзельніка, які яшчэ не стварыў сабе рахунак ці не ўжывае яго. Таму мы вымушаныя ўжываць лічбавы IP-адрас дзеля ягонай ідэнтыфікацыі. Адзін IP-адрас можа выкарыстоўвацца некалькімі ўдзельнікамі. Калі Вы — ананімны ўдзельнік і лічыце, што атрымалі не прызначаныя Вам камэнтары, калі ласка, [[Special:UserLogin|зарэгіструйцеся ці ўвайдзіце ў сыстэму]], каб у будучыні пазьбегнуць магчымай блытаніны зь іншымі ананімнымі ўдзельнікамі.''

''This is the discussion page for an anonymous user who has not created an account yet or who does not use it. We therefore have to use the numerical IP address to identify him/her. Such an IP address can be shared by several users. If you are an anonymous user and feel that irrelevant comments have been directed at you, please [[Special:UserLogin|create an account or log in]] to avoid future confusion with other anonymous users.''",
'noarticletext'                    => 'Зараз тэкст на гэтай старонцы адсутнічае. Вы можаце [[Special:Search/{{PAGENAME}}|пашукаць гэтую назву]] ў іншых старонках альбо [{{fullurl:{{NAMESPACE}}:{{PAGENAME}}|action=edit}} рэдагаваць гэтую старонку].',
'userpage-userdoesnotexist'        => 'Рахунак удзельніка «$1» не зарэгістраваны. Калі ласка, удакладніце, ці жадаеце Вы стварыць/рэдагаваць гэтую старонку.',
'clearyourcache'                   => "'''Заўвага:''' Каб пабачыць зьмены пасьля захаваньня, Вам можа спатрэбіцца ацысьціць кэш Вашага браўзэра. '''Mozilla / Firefox / Safari:''' трымайце ''Shift'' і націсьніце ''Reload'', ці націсьніце ''Ctrl-Shift-R'' (''Cmd-Shift-R'' на Apple Mac); '''IE:''' трымайце ''Ctrl'' і націсьніце ''Refresh'', ці націсьніце ''Ctrl-F5''; '''Konqueror:''': проста націсьніце кнопку ''Reload'', ці націсьніце ''F5''; карыстальнікам '''Opera''' можа спатрэбіцца цалкам ачысьціць кэш праз ''Tools→Preferences''.",
'usercssjsyoucanpreview'           => '<strong>Падказка:</strong> выкарыстоўвайце кнопку «Папярэдні прагляд», каб паспрабаваць новы код CSS/JS перад тым як яго запісаць.',
'usercsspreview'                   => "'''Памятайце, што гэта толькі папярэдні прагляд Вашага CSS. Ён яшчэ не запісаны!'''",
'userjspreview'                    => "'''Памятайце, што гэта толькі папярэдні прагляд Вашага JavaScript, ён яшчэ не запісаны!'''",
'userinvalidcssjstitle'            => "'''Увага:''' няма тэмы афармленьня «$1».
Памятайце, што ўласныя старонкі .css і .js павінны мець назву, якая складаецца з малых літараў, напрыклад, {{ns:user}}:Хтосьці/monobook.css, а не {{ns:user}}:Хтосьці/Monobook.css.",
'updated'                          => '(Абноўлена)',
'note'                             => '<strong>Заўвага: </strong>',
'previewnote'                      => '<strong>Гэта толькі папярэдні прагляд і зьмены яшчэ не былі захаваныя!</strong>',
'previewconflict'                  => 'Гэта папярэдні прагляд тэксту зь верхняга вакна рэдагаваньня, так ён будзе выглядаць, калі Вы вырашыце яго захаваць.',
'session_fail_preview'             => '<strong>Не атрымалася захаваць Вашую праўку праз тое, што былі страчаныя зьвесткі пра сэсію.
Калі ласка, паспрабуйце яшчэ раз. Калі памылка ня зьнікне, паспрабуйце [[Special:UserLogout|выйсьці з сыстэмы]] і ўвайсьці ізноў.</strong>',
'session_fail_preview_html'        => "<strong>Не атрымалася захаваць Вашую праўку праз тое, што былі страчаныя зьвесткі пра сэсію.</strong>

''Таму што ў {{GRAMMAR:месны|{{SITENAME}}}} дазволена выкарыстоўваць чысты HTML, папярэдні прагляд быў адключаны для засьцярогі ад атакаў праз JavaScript.''

<strong>Калі гэта сапраўдная спроба рэдагаваньня, калі ласка, паспрабуйце яшчэ раз. Калі гэта не дапамагае, паспрабуйце [[Special:UserLogout|выйсьці з сыстэмы]] і ўвайсьці ізноў.</strong>",
'token_suffix_mismatch'            => '<strong>Вашае рэдагаваньне было адхіленае, таму што Ваш кліент ня можа апрацоўваць знакі пунктуацыі ў акне рэдагаваньня.
Рэдагаваньне было скасаванае для таго, каб пазьбегнуць зьнішчэньня тэксту старонкі.
Такія памылкі здараюцца, калі Вы выкарыстоўваеце ананімны проксі-сэрвэр, які ўтрымлівае памылкі.</strong>',
'editing'                          => 'Рэдагаваньне: $1',
'editingsection'                   => 'Рэдагаваньне: $1 (сэкцыя)',
'editingcomment'                   => 'Рэдагаваньне: $1 (камэнтар)',
'editconflict'                     => 'Канфлікт рэдагаваньня: $1',
'explainconflict'                  => "Нехта зьмяніў старонку падчас Вашага рэдагаваньня.
У верхнім тэкставым акне знаходзіцца цяперашні зьмест старонкі.
Вашыя зьмены паказаныя ў ніжнім акне.
Вам трэба перанесьці Вашыя зьмены ў цяперашні тэкст.
Калі Вы націсьніце «Захаваць зьмены», будзе захаваны '''толькі''' тэкст верхняга вакна.",
'yourtext'                         => 'Ваш тэкст',
'storedversion'                    => 'Захаваная вэрсія',
'nonunicodebrowser'                => '<strong>ПАПЯРЭДЖАНЬНЕ: Ваш браўзэр не працуе з кадаваньнем UTF-8 (Unicode).
У выніку гэтага ўсе сымбалі ня ўключаныя ў ASCII будуць замененыя на іх шаснаццаткавыя коды.</strong>',
'editingold'                       => '<strong>ПАПЯРЭДЖАНЬНЕ: Вы рэдагуеце састарэлую вэрсію гэтай старонкі.
Калі Вы паспрабуеце захаваць яе, любыя зьмены, зробленыя пасьля гэтай вэрсіі, будуць страчаныя.</strong>',
'yourdiff'                         => 'Адрозьненьні',
'copyrightwarning'                 => 'Калі ласка, зьвярніце ўвагу на тое, што ўсе дадаткі і зьмены ў {{GRAMMAR:месны|{{SITENAME}}}} разглядаюцца як выдадзеныя ў адпаведнасьці з умовамі ліцэнзіі $2 (глядзіце падрабязнасьці на $1). Калі Вы супраць таго, каб Вашыя матэрыялы неабмежавана рэдагавалася і распаўсюджвалася, не дадавайце іх.<br />
Вы таксама абавязуецеся, што Ваш матэрыял напісаны асабіста Вамі або ўзяты з грамадзкай уласнасьці альбо падобных вольных крыніцаў.
<strong>НЕЛЬГА БЕЗ ДАЗВОЛУ ДАДАВАЦЬ МАТЭРЫЯЛЫ, АБАРОНЕНЫЯ АЎТАРСКІМ ПРАВАМ!</strong>',
'copyrightwarning2'                => 'Калі ласка, заўважце, што ўвесь унёсак ў {{GRAMMAR:вінавальны|{{SITENAME}}}} можа рэдагавацца, зьмяняцца і выдаляцца іншымі ўдзельнікамі.
Калі Вы з гэтым ня згодныя, калі ласка, не зьмяшчайце сюды Вашыя тэксты.<br />
Разьмяшчэньнем тут тэкстаў, Вы дэкляруеце, што Вы зьяўляецеся іх аўтарам, ці Вы скапіявалі іх з крыніцы, якая дазваляе вольнае выкарыстаньне сваіх тэкстаў (дзеля падрабязнасьцяў глядзіце $1).

<strong>КАЛІ ЛАСКА, НЕ ЗЬМЯШЧАЙЦЕ ТУТ БЕЗ ДАЗВОЛУ МАТЭРЫЯЛЫ, ЯКІЯ АХОЎВАЮЦЦА АЎТАРСКІМ ПРАВАМ!</strong>',
'longpagewarning'                  => "<strong>ПАПЯРЭДЖАНЬНЕ: Гэтая старонка ўтрымлівае $1 {{PLURAL:$1|кілябайт|кілябайты|кілябайтаў}}; некаторыя браўзэры могуць мець праблемы пры адлюстраваньні старонак аб'ёмам болей за 32кб.
Калі гэта магчыма, паспрабуйце падзяліць зьмест старонкі на больш дробныя часткі.</strong>",
'longpageerror'                    => "<strong>ПАМЫЛКА: Аб'ём тэксту, які Вы спрабуеце запісаць складае $1 {{PLURAL:$1|кілябайт|кілябайты|кілябайтаў}}. Аб'ём старонкі ня можа перавышаць $2 {{PLURAL:$2|кілябайт|кілябайты|кілябайтаў}}. Старонка ня можа быць захаваная.</strong>",
'readonlywarning'                  => '<strong>ПАПЯРЭДЖАНЬНЕ: База зьвестак была заблякаваная для тэхнічнага абслугоўваньня, таму немагчыма зараз захаваць Вашыя зьмены.

Паспрабуйце захаваць тэкст на Вашым кампутары, а потым захаваць сюды.</strong>',
'protectedpagewarning'             => '<strong>ПАПЯРЭДЖАНЬНЕ: Гэтая старонка была абароненая, таму толькі адміністратары могуць рэдагаваць яе.</strong>',
'semiprotectedpagewarning'         => "'''Заўвага:''' Гэтая старонка была абароненая і рэдагаваць яе могуць толькі зарэгістраваныя ўдзельнікі.",
'cascadeprotectedwarning'          => "'''Папярэджаньне:''' Гэтая старонка абароненая, толькі ўдзельнікі з правамі адміністратараў могуць рэдагаваць яе, таму што яна ўключаная ў {{PLURAL:$1|наступную каскадна-абароненую старонку|наступныя каскадна-абароненыя старонкі}}:",
'titleprotectedwarning'            => '<strong>ПАПЯРЭДЖАНЬНЕ: Гэтая старонка была абароненая і стварыць яе могуць толькі зарэгістраваныя ўдзельнікі.</strong>',
'templatesused'                    => 'На гэтай старонцы выкарыстаныя наступныя шаблёны:',
'templatesusedpreview'             => 'У гэтым праглядзе выкарыстаныя наступныя шаблёны:',
'templatesusedsection'             => 'У гэтай сэкцыі выкарыстаныя наступныя шаблёны:',
'template-protected'               => '(абаронены)',
'template-semiprotected'           => '(часткова абароненая)',
'hiddencategories'                 => 'Гэтая старонка належыць $1 {{PLURAL:$1|схаванай катэгорыі|схаваным катэгорыям|схаваным катэгорыям}}:',
'nocreatetitle'                    => 'Стварэньне старонак абмежаванае',
'nocreatetext'                     => 'У {{GRAMMAR:месны|{{SITENAME}}}} абмежаванае стварэньне новых старонак.
Вы можаце вярнуцца і рэдагаваць існуючую старонку, альбо [[Special:UserLogin|ўвайсьці ў сыстэму ці стварыць рахунак]].',
'nocreate-loggedin'                => 'Вы ня маеце дазволу на стварэньне новых старонак.',
'permissionserrors'                => 'Памылкі дазволаў',
'permissionserrorstext'            => 'Вы ня маеце дазволу на гэтае дзеяньне па {{PLURAL:$1|наступнай прычыне|наступных прычынах}}:',
'permissionserrorstext-withaction' => 'Вы ня маеце дазволу на $2 па {{PLURAL:$1|наступнай прычыне|наступных прычынах}}:',
'recreate-deleted-warn'            => "'''ПАПЯРЭДЖАНЬНЕ: Вы ствараеце старонку, якая раней была выдаленая.'''

Упэўніцеся, што стварэньне гэтай старонкі неабходнае.
Ніжэй знаходзіцца журнал выдаленьняў гэтай старонкі:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Папярэджаньне: Гэтая старонка ўтрымлівае зашмат працаёмістых зваротаў да функцыяў парсэра.

Іх павінна быць ня болей за $2, а цяпер ўтрымліваецца $1.',
'expensive-parserfunction-category'       => 'Старонкі, якія ўтрымліваюць зашмат працаёмістых зваротаў да функцыяў парсэра',
'post-expand-template-inclusion-warning'  => 'Папярэджаньне: Памер уключаемых шаблёнаў занадта вялікі.
Некаторыя шаблёны ня будуць уключаныя.',
'post-expand-template-inclusion-category' => 'Старонкі, у якіх перавышаны дапушчальны памер уключаных шаблёнаў',
'post-expand-template-argument-warning'   => 'Увага: Гэтая старонка ўтрымлівае прынамсі адзін парамэтар шаблёна, які мае занадта вялікі выгляд у разгорнутым выглядзе.
Гэтыя парамэтры былі прапушчаныя.',
'post-expand-template-argument-category'  => 'Старонкі, у якіх прапушчаныя парамэтры шаблёнаў',

# "Undo" feature
'undo-success' => 'Рэдагаваньне можа быць адмененае. Калі ласка, параўнайце адрозьненьні паміж вэрсіямі, каб упэўніцца, што гэта адпаведныя зьмены, а потым запішыце зьмены для сканчэньня рэдагаваньня.',
'undo-failure' => 'Рэдагаваньне ня можа быць скасаванае праз канфлікт паміж папярэднімі рэдагаваньнямі.',
'undo-norev'   => 'Рэдагаваньне ня можа быць адмененае, таму што яно не існуе альбо было выдаленае.',
'undo-summary' => 'Скасаваньне праўкі $1 удзельніка [[Special:Contributions/$2|$2]] ([[User talk:$2|гутаркі]])',

# Account creation failure
'cantcreateaccounttitle' => 'Немагчыма стварыць рахунак',
'cantcreateaccount-text' => "Стварэньне рахункаў з гэтага IP-адрасу ('''$1''') было заблякаванае [[User:$3|$3]].

Прычына блякаваньня пададзеная $3: ''$2''",

# History pages
'viewpagelogs'        => 'Паказаць журналы падзей для гэтай старонкі',
'nohistory'           => 'Гісторыя зьменаў для гэтай старонкі адсутнічае.',
'revnotfound'         => 'Вэрсія ня знойдзеная',
'revnotfoundtext'     => 'Ранейшая вэрсія гэтай старонкі ня знойдзеная. Праверце спасылку, празь якую Вы спрабавалі перайсьці на гэтую старонку.',
'currentrev'          => 'Цяперашняя вэрсія',
'revisionasof'        => 'Вэрсія ад $1',
'revision-info'       => 'Вэрсія ад $1, аўтар $2',
'previousrevision'    => '←Папярэдняя вэрсія',
'nextrevision'        => 'Наступная вэрсія→',
'currentrevisionlink' => 'Цяперашняя вэрсія',
'cur'                 => 'цяп',
'next'                => 'наступная',
'last'                => 'папярэдняя',
'page_first'          => 'першая',
'page_last'           => 'апошняя',
'histlegend'          => 'Параўнаньне: пазначце кропкамі дзьве вэрсіі для параўнаньня і націсьніце enter альбо кнопку ўнізе.<br />
Тлумачэньне: (цяп) = адрозьненьні ад цяперашняй вэрсіі, (папярэдняя) = адрозьненьні ад папярэдняй вэрсіі, д = дробная праўка.',
'deletedrev'          => '[выдаленая]',
'histfirst'           => 'найстарэйшыя',
'histlast'            => 'найнавейшыя',
'historysize'         => '($1 {{PLURAL:$1|байт|байты|байтаў}})',
'historyempty'        => '(пуста)',

# Revision feed
'history-feed-title'          => 'Гісторыя зьменаў',
'history-feed-description'    => 'Гісторыя зьменаў гэтай старонкі',
'history-feed-item-nocomment' => '$1 у $2', # user at time
'history-feed-empty'          => 'Запатрабаванай старонкі не існуе.
Магчыма, яна была выдаленая альбо яе перанесьлі.
Паспрабуйце [[Special:Search|пашукаць]] падобныя старонкі.',

# Revision deletion
'rev-deleted-comment'         => '(камэнтар выдалены)',
'rev-deleted-user'            => '(імя ўдзельніка выдаленае)',
'rev-deleted-event'           => '(запіс з журнала падзеяў выдалены)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Гэтая вэрсія старонкі была выдаленая з агульнадаступных архіваў.
Магчыма, падрабязнасьці могуць быць знойдзеныя ў [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} журнале выдаленьняў].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Гэтая вэрсія старонкі была выдаленая з агульнадаступных архіваў.
Вы маеце магчымасьць яе прагляду, таму што зьяўляецеся адміністратарам {{GRAMMAR:родны|{{SITENAME}}}}.
Магчыма, падрабязнасьці могуць быць знойдзеныя ў [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} журнале выдаленьняў].</div>',
'rev-delundel'                => 'паказаць/схаваць',
'revisiondelete'              => 'Выдаліць/аднавіць вэрсіі',
'revdelete-nooldid-title'     => 'Ня выбрана мэтавая вэрсія',
'revdelete-nooldid-text'      => 'Вы ня выбралі мэтавую вэрсію (ці вэрсіі) для выкананьня гэтай функцыі, выбранай вэрсіі не існуе альбо Вы спрабуеце схаваць цяперашнюю вэрсію.',
'revdelete-selected'          => '{{PLURAL:$2|Выбраная вэрсія|Выбраныя вэрсіі}} старонкі [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Выбраны запіс|Выбраныя запісы}} журнала падзеяў:',
'revdelete-text'              => 'Выдаленыя вэрсіі і падзеі будуць адлюстроўвацца ў гісторыі старонкі і журналах, але частка іх зьместу ня будзе даступная для звычайных удзельнікаў.

Іншыя адміністратары {{GRAMMAR:родны|{{SITENAME}}}} будуць мець магчымасьць прагляду зьместу і аднаўленьня старонкі праз гэты інтэрфэйс, калі ня будуць ўведзеныя дадатковыя абмежаваньні.',
'revdelete-legend'            => 'Усталяваць абмежаваньні бачнасьці',
'revdelete-hide-text'         => 'Схаваць тэкст вэрсіі',
'revdelete-hide-name'         => 'Схаваць дзеяньне і мэту',
'revdelete-hide-comment'      => 'Схаваць кароткае апісаньне зьменаў',
'revdelete-hide-user'         => 'Схаваць імя/IP-адрас удзельніка',
'revdelete-hide-restricted'   => 'Увесьці гэтыя абмежаваньні для адміністратараў і заблякаваць гэты інтэрфэйс',
'revdelete-suppress'          => 'Схаваць зьвесткі ад адміністратараў як і ад іншых удзельнікаў',
'revdelete-hide-image'        => 'Схаваць зьмест файла',
'revdelete-unsuppress'        => 'Зьняць абмежаваньні з адноўленых вэрсіяў',
'revdelete-log'               => 'Камэнтар:',
'revdelete-submit'            => 'Ужыць для выбранай вэрсіі',
'revdelete-logentry'          => 'зьмененая бачнасьць вэрсіі старонкі [[$1]]',
'logdelete-logentry'          => 'зьмененая бачнасьць падзеі старонкі [[$1]]',
'revdelete-success'           => "'''Бачнасьць вэрсіі пасьпяхова зьмененая.'''",
'logdelete-success'           => "'''Бачнасьць падзеі пасьпяхова зьмененая.'''",
'revdel-restore'              => 'Зьмяніць бачнасьць',
'pagehist'                    => 'Гісторыя старонкі',
'deletedhist'                 => 'Выдаленая гісторыя',
'revdelete-content'           => 'зьмест',
'revdelete-summary'           => 'кароткае апісаньне зьменаў',
'revdelete-uname'             => 'імя ўдзельніка',
'revdelete-restricted'        => 'ужыць абмежаваньні для адміністратараў',
'revdelete-unrestricted'      => 'зьнятыя абмежаваньні для адміністратараў',
'revdelete-hid'               => 'схаваць $1',
'revdelete-unhid'             => 'не хаваць $1',
'revdelete-log-message'       => '$1 для $2 {{PLURAL:$2|вэрсіі|вэрсій|вэрсій}}',
'logdelete-log-message'       => '$1 для $2 {{PLURAL:$2|падзеі|падзеяў|падзеяў}}',

# Suppression log
'suppressionlog'     => 'Журнал схаваньняў',
'suppressionlogtext' => 'Ніжэй знаходзіцца сьпіс выдаленьняў і блякаваньняў, схаваных нават ад адміністратараў.
Глядзіце [[Special:IPBlockList|сьпіс блякаваньняў IP]], каб паглядзець сьпіс цяперашніх блякаваньняў.',

# History merging
'mergehistory'                     => "Аб'яднаць гісторыі старонак",
'mergehistory-header'              => "Гэтая старонка дазваляе аб'яднаць гісторыю рэдагаваньняў адной старонкі з гісторыяй іншай, новай старонкі.
Упэўніцеся, што гэтыя зьмены не парушаць пасьлядоўнасьць гісторыі старонкі.",
'mergehistory-box'                 => "Аб'яднаць гісторыі рэдагаваньняў дзьвюх старонак:",
'mergehistory-from'                => 'Крынічная старонка:',
'mergehistory-into'                => 'Мэтавая старонка:',
'mergehistory-list'                => "Гісторыя рэдагаваньняў, якую магчыма аб'яднаць",
'mergehistory-merge'               => "Наступныя вэрсіі [[:$1]] можна аб'яднаць у [[:$2]].
Карыстайцеся кнопкамі для аб'яднаньня толькі выбранага дыяпазону рэдагаваньняў.
Заўважце, што выкарыстаньне навігацыйных спасылак выдаліць зьвесткі ў гэтым слупку.",
'mergehistory-go'                  => "Паказаць вэрсіі, якія магчыма аб'яднаць",
'mergehistory-submit'              => "Аб'яднаць гісторыі рэдагаваньняў",
'mergehistory-empty'               => "Няма гісторыі рэдагаваньняў, якую магчыма аб'яднаць.",
'mergehistory-success'             => "$3 {{PLURAL:$3|вэрсія|вэрсіі|вэрсій}} з [[:$1]] пасьпяхова аб'яднаныя ў [[:$2]].",
'mergehistory-fail'                => "Не атрымалася аб'яднаць гісторыі старонак. Калі ласка, праверце парамэтры старонкі і часу.",
'mergehistory-no-source'           => 'Не існуе крынічнай старонкі $1.',
'mergehistory-no-destination'      => 'Не існуе мэтавай старонкі $1.',
'mergehistory-invalid-source'      => 'Крынічная старонка мусіць мець карэктную назву.',
'mergehistory-invalid-destination' => 'Мэтавая старонка мусіць мець карэктную назву.',
'mergehistory-autocomment'         => "Гісторыя [[:$1]] аб'яднаная ў [[:$2]]",
'mergehistory-comment'             => "Гісторыя [[:$1]] аб'яднаная ў [[:$2]]: $3",

# Merge log
'mergelog'           => "Журнал аб'яднаньняў",
'pagemerge-logentry' => "аб'яднаная [[$1]] ў [[$2]] (гісторыя рэдагаваньняў да $3)",
'revertmerge'        => 'Разьяднаць',
'mergelogpagetext'   => "Ніжэй знаходзіцца сьпіс апошніх аб'яднаньняў гісторыяў старонак.",

# Diffs
'history-title'           => 'Гісторыя зьменаў старонкі «$1»',
'difference'              => '(Адрозьненьні паміж вэрсіямі)',
'lineno'                  => 'Радок $1:',
'compareselectedversions' => 'Параўнаць выбраныя вэрсіі',
'editundo'                => 'адмяніць',
'diff-multi'              => '($1 {{PLURAL:$1|прамежная вэрсія не паказаная|прамежныя вэрсіі не паказаныя|прамежных вэрсіяў не паказаныя}}.)',

# Search results
'searchresults'             => 'Вынікі пошуку',
'searchresulttext'          => 'Для атрыманьня больш падрабязнай інфармацыі пра пошук у {{GRAMMAR:месны|{{SITENAME}}}}, глядзіце [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => "Па запыце '''[[:$1]]''' ([[Special:Prefixindex/$1|усе старонкі, якія пачынаюцца з «$1»]] | [[Special:WhatLinksHere/$1|усе старонкі, якія спасылаюцца на «$1»]])",
'searchsubtitleinvalid'     => "Па запыце '''$1'''",
'noexactmatch'              => "'''Старонкі з гэткай назвай не існуе.''' Вы можаце '''[[:$1|стварыць гэтую старонку]]'''.",
'noexactmatch-nocreate'     => "'''Не існуе старонкі з назвай «$1».'''",
'toomanymatches'            => 'Знойдзена зашмат супадзеньняў з запытам, калі ласка, паспрабуйце зьмяніць запыт',
'titlematches'              => 'Супадзеньні ў назвах старонак',
'notitlematches'            => 'Супадзеньні ў назвах старонак ня знойдзеныя',
'textmatches'               => 'Супадзеньні ў тэкстах старонак',
'notextmatches'             => 'Супадзеньні ў тэкстах старонак ня знойдзеныя',
'prevn'                     => 'папярэднія $1',
'nextn'                     => 'наступныя $1',
'viewprevnext'              => 'Паказаць ($1) ($2) ($3)',
'search-result-size'        => '$1 ($2 {{PLURAL:$2|слова|словы|слоў}})',
'search-result-score'       => 'Адпаведнасьць: $1%',
'search-redirect'           => '(перанакіраваньне $1)',
'search-section'            => '(сэкцыя $1)',
'search-suggest'            => 'Вы мелі на ўвазе: $1',
'search-interwiki-caption'  => 'Сумежныя праекты',
'search-interwiki-default'  => 'вынікі з $1:',
'search-interwiki-more'     => '(яшчэ)',
'search-mwsuggest-enabled'  => 'з парадамі',
'search-mwsuggest-disabled' => 'без парад',
'search-relatedarticle'     => 'Зьвязаны',
'mwsuggest-disable'         => 'Адключыць AJAX-падказкі',
'searchrelated'             => 'зьвязаны',
'searchall'                 => 'усе',
'showingresults'            => "Ніжэй {{PLURAL:$1|паданы|паданыя|паданыя}} да '''$1''' {{PLURAL:$1|выніку|вынікаў|вынікаў}}, пачынаючы з #<b>$2</b>.",
'showingresultsnum'         => "Ніжэй паказаны сьпіс з '''$3''' {{PLURAL:$3|выніка|вынікаў|вынікаў}}, пачынаючы ад № '''$2'''.",
'showingresultstotal'       => "Ніжэй {{PLURAL:$3|паказаны '''$1''' вынік з '''$3'''|паказаныя вынікі '''$1 — $2''' з '''$3'''}}",
'nonefound'                 => "'''Увага''': Па змоўчаньні пошук вядзецца не ва ўсіх прасторах назваў. Трэба дапісаць да запыту ''усе:'' каб шукаць па ўсім зьмесьце (уключаючы абмеркаваньні, шаблёны і г.д.) альбо дадаць да запыту неабходную прастору назваў.",
'powersearch'               => 'Пошук',
'powersearch-legend'        => 'Удасканалены пошук',
'powersearch-ns'            => 'Шукаць у прасторах назваў:',
'powersearch-redir'         => 'Паказваць перанакіраваньні',
'powersearch-field'         => 'Шукаць',
'search-external'           => 'Вонкавы пошук',
'searchdisabled'            => 'Функцыя пошуку ў {{GRAMMAR:месны|{{SITENAME}}}} адключаная.
Вы можаце пашукаць з дапамогай Google, але заўважце, што там інфармацыя пра старонкі {{GRAMMAR:родны|{{SITENAME}}}} можа быць састарэлай.',

# Preferences page
'preferences'              => 'Устаноўкі',
'mypreferences'            => 'Мае ўстаноўкі',
'prefs-edits'              => 'Колькасьць рэдагаваньняў:',
'prefsnologin'             => 'Вы не ўвайшлі ў сыстэму',
'prefsnologintext'         => 'Вам трэба <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} ўвайсьці ў сыстэму]</span>, каб зьмяняць свае ўстаноўкі.',
'prefsreset'               => 'Адноўленыя ўстаноўкі па змоўчаньні.',
'qbsettings'               => 'Панэль хуткага доступу',
'qbsettings-none'          => 'Не паказваць',
'qbsettings-fixedleft'     => 'Замацаваная зьлева',
'qbsettings-fixedright'    => 'Замацаваная справа',
'qbsettings-floatingleft'  => 'Рухомая зьлева',
'qbsettings-floatingright' => 'Плавае справа',
'changepassword'           => 'Зьмяніць пароль',
'skin'                     => 'Афармленьне',
'math'                     => 'Матэматыка',
'dateformat'               => 'Фармат даты',
'datedefault'              => 'Па змоўчаньні',
'datetime'                 => 'Дата і час',
'math_failure'             => 'Не атрымалася разабраць',
'math_unknown_error'       => 'невядомая памылка',
'math_unknown_function'    => 'невядомая функцыя',
'math_lexing_error'        => 'лексычная памылка',
'math_syntax_error'        => 'сынтаксычная памылка',
'math_image_error'         => 'Не атрымалася пераўтварыць у PNG;
спраўдзіце дакладнасьць усталяваньня пакетаў latex, dvips, gs, і convert',
'math_bad_tmpdir'          => 'Немагчыма запісаць ці стварыць часовую дырэкторыю для матэматыкі',
'math_bad_output'          => 'Немагчыма запісаць ці стварыць выходную матэматычную дырэкторыю',
'math_notexvc'             => 'Выканаўчы модуль texvc ня знойдзены.
Калі ласка, прачытайце math/README пра яго канфігурацыю.',
'prefs-personal'           => 'Асабістыя зьвесткі',
'prefs-rc'                 => 'Апошнія зьмены',
'prefs-watchlist'          => 'Сьпіс назіраньня',
'prefs-watchlist-days'     => 'Колькасьць дзён для паказу ў сьпісе назіраньня:',
'prefs-watchlist-edits'    => 'Колькасьць рэдагаваньняў для паказу ў пашыраным сьпісе назіраньня:',
'prefs-misc'               => 'Рознае',
'saveprefs'                => 'Захаваць',
'resetprefs'               => 'Скінуць',
'oldpassword'              => 'Стары пароль:',
'newpassword'              => 'Новы пароль:',
'retypenew'                => 'Паўтарыце новы пароль:',
'textboxsize'              => 'Рэдагаваньне',
'rows'                     => 'Радкоў:',
'columns'                  => 'Слупкоў:',
'searchresultshead'        => 'Пошук',
'resultsperpage'           => 'Колькасьць вынікаў на старонцы:',
'contextlines'             => 'Колькасьць радкоў для паказу:',
'contextchars'             => 'Колькасьць сымбаляў на радок:',
'stub-threshold'           => 'Максымальны памер старонкі для паказу спасылак <a href="#" class="stub">як на накід</a> (у байтах):',
'recentchangesdays'        => 'Колькасьць дзён для паказу ў апошніх зьменах:',
'recentchangescount'       => 'Колькасьць рэдагаваньняў для паказу ў апошніх зьменах і гісторыі, колькасьць запісаў у журналах падзей:',
'savedprefs'               => 'Вашыя ўстаноўкі былі захаваныя.',
'timezonelegend'           => 'Часавы пояс',
'timezonetext'             => '¹На колькі гадзінаў Ваш мясцовы час адрозьніваецца ад сэрвэрнага (UTC).',
'localtime'                => 'Мясцовы час',
'timezoneoffset'           => 'Розьніца¹',
'servertime'               => 'Цяперашні час на сэрвэры',
'guesstimezone'            => 'Запоўніць з браўзэра',
'allowemail'               => 'Дазволіць атрыманьне лістоў ад іншых удзельнікаў і ўдзельніц',
'prefs-searchoptions'      => 'Магчымасьці пошуку',
'prefs-namespaces'         => 'Прасторы назваў',
'defaultns'                => 'Па змоўчаньні, шукаць у наступных прасторах назваў:',
'default'                  => 'па змоўчаньні',
'files'                    => 'Файлы',

# User rights
'userrights'                  => 'Кіраваньне правамі ўдзельнікаў і ўдзельніц', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Кіраваньне групамі ўдзельнікаў і ўдзельніц',
'userrights-user-editname'    => 'Увядзіце імя ўдзельніка:',
'editusergroup'               => 'Рэдагаваць групы ўдзельнікаў і ўдзельніц',
'editinguser'                 => "Зьмена правоў удзельніка '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Рэдагаваць групы ўдзельнікаў і ўдзельніц',
'saveusergroups'              => 'Захаваць групы ўдзельнікаў і ўдзельніц',
'userrights-groupsmember'     => 'Уваходзіць у:',
'userrights-groups-help'      => 'Тут можна мяняць групы да якіх належыць гэты ўдзельнік:
* Вызначанае поле азначае прыналежнасьць удзельніка да гэтай групы.
* Ня вызначанае поле азначае, што ўдзельнік не належыць гэтай групе.
* Знак * азначае што Вы ня зможаце выдаліць удзельніка з групы, калі дададзіце яго да яе і наадварот.',
'userrights-reason'           => 'Прычына зьмены:',
'userrights-no-interwiki'     => 'Вы ня маеце дазволу зьмяняць правы ўдзельнікаў іншых вікаў.',
'userrights-nodatabase'       => 'Базы зьвестак $1 не існуе альбо яна не зьяўляецца лякальнай.',
'userrights-nologin'          => 'Вы павінны [[Special:UserLogin|ўвайсьці ў сыстэму]] як адміністратар, каб прызначаць правы ўдзельнікам.',
'userrights-notallowed'       => 'Вы ня можаце прызначаць правы ўдзельнікам.',
'userrights-changeable-col'   => 'Групы, якія Вы можаце мяняць',
'userrights-unchangeable-col' => 'Групы, якія Вы ня можаце мяняць',

# Groups
'group'               => 'Група:',
'group-user'          => 'Удзельнікі',
'group-autoconfirmed' => 'Аўтаматычна пацьверджаныя ўдзельнікі',
'group-bot'           => 'Робаты',
'group-sysop'         => 'Адміністрацыя',
'group-bureaucrat'    => 'Бюракраты',
'group-suppress'      => 'Рэвізоры',
'group-all'           => '(усе)',

'group-user-member'          => 'удзельнік',
'group-autoconfirmed-member' => 'аўтаматычна пацьверджаны ўдзельнік',
'group-bot-member'           => 'робат',
'group-sysop-member'         => 'адміністратар',
'group-bureaucrat-member'    => 'бюракрат',
'group-suppress-member'      => 'рэвізор',

'grouppage-user'          => '{{ns:project}}:Удзельнікі',
'grouppage-autoconfirmed' => '{{ns:project}}:Аўтаматычна пацьверджаныя ўдзельнікі',
'grouppage-bot'           => '{{ns:project}}:Робаты',
'grouppage-sysop'         => '{{ns:project}}:Адміністрацыя',
'grouppage-bureaucrat'    => '{{ns:project}}:Бюракраты',
'grouppage-suppress'      => '{{ns:project}}:Рэвізоры',

# Rights
'right-read'                 => 'прагляд старонак',
'right-edit'                 => 'рэдагаваньне старонак',
'right-createpage'           => 'стварэньне старонак (акрамя старонак абмеркаваньняў)',
'right-createtalk'           => 'стварэньне старонак абмеркаваньня',
'right-createaccount'        => 'стварэньне новых рахункаў удзельніка',
'right-minoredit'            => 'пазначэньне рэдагаваньняў як дробныя',
'right-move'                 => 'перанос старонак',
'right-move-subpages'        => 'перанос старонак разам зь іх падстаронкамі',
'right-suppressredirect'     => 'не ствараць перанакіраваньне са старой назвы пасьля пераносу старонкі',
'right-upload'               => 'загрузка файлаў',
'right-reupload'             => 'перазапіс існуючых файлаў',
'right-reupload-own'         => 'перазапіс уласных існуючых файлаў',
'right-reupload-shared'      => 'перазапіс файлаў з агульнага сховішча лякальнымі',
'right-upload_by_url'        => 'загрузка файлаў з URL-адрасу',
'right-purge'                => 'ачыстка кэшу старонак бяз запыту пацьверджаньня',
'right-autoconfirmed'        => 'рэдагаваньне часткова абароненых старонак',
'right-bot'                  => 'лічыцца аўтаматычным працэсам',
'right-nominornewtalk'       => 'не паведамляць удзельнікам пра новыя паведамленьні на іх старонках размоваў, калі зробленыя там праўкі былі дробнымі',
'right-apihighlimits'        => 'Менш абмежаваньняў на выкананьне API-запытаў',
'right-writeapi'             => 'выкарыстаньне API для запісаў',
'right-delete'               => 'выдаленьне старонак',
'right-bigdelete'            => 'Выдаленьне старонак зь вялікімі гісторыямі',
'right-deleterevision'       => 'выдаленьне і аднаўленьне асобных вэрсій старонак',
'right-deletedhistory'       => 'прагляд выдаленай гісторыі старонак без доступу да выдаленага тэксту',
'right-browsearchive'        => 'пошук выдаленых старонак',
'right-undelete'             => 'аднаўленьне старонак',
'right-suppressrevision'     => 'прагляд і аднаўленьне вэрсій схаваных ад адміністратараў',
'right-suppressionlog'       => 'прагляд прыватных журналаў',
'right-block'                => 'блякаваньне іншых удзельнікаў ад рэдагаваньняў',
'right-blockemail'           => 'блякаваньне іншых ўдзельнікаў ад дасылкі электроннай пошты',
'right-hideuser'             => 'блякаваньне рахунку ўдзельніка і яго хаваньне',
'right-ipblock-exempt'       => 'абход блякаваньняў IP-адрасоў, аўта-блякаваньняў і блякаваньняў дыяпазонаў',
'right-proxyunbannable'      => 'абход аўтаматычных блякаваньняў проксі',
'right-protect'              => 'зьмена ўзроўню абароны старонак і рэдагаваньне абароненых старонак',
'right-editprotected'        => 'рэдагаваньне абароненых старонак (без каскаднай абароны)',
'right-editinterface'        => 'рэдагаваньне інтэрфэйса карыстальніка',
'right-editusercssjs'        => 'рэдагаваньне CSS і JS файлаў іншых удзельнікаў',
'right-rollback'             => 'хуткі адкат правак апошняга ўдзельніка, які рэдагаваў старонку',
'right-markbotedits'         => 'пазначэньне адкатаў як рэдагаваньне робатам',
'right-noratelimit'          => 'няма абмежаваньняў па хуткасьці',
'right-import'               => 'імпарт старонак зь іншых вікі',
'right-importupload'         => 'імпарт старонак праз загрузку файлаў',
'right-patrol'               => 'пазначэньне рэдагаваньняў як «патруляваныя»',
'right-autopatrol'           => 'аўтаматычнае пазначэньне рэдагаваньняў як «патруляваных»',
'right-patrolmarks'          => 'прагляд пазначэньняў пра патруляваньне ў апошніх зьменах',
'right-unwatchedpages'       => 'прагляд сьпісу старонак, за якімі ніхто не назірае',
'right-trackback'            => 'адпраўка trackback',
'right-mergehistory'         => "аб'яднаньне гісторыі старонак",
'right-userrights'           => 'рэдагаваньне правоў усіх удзельнікаў',
'right-userrights-interwiki' => 'рэдагаваньне правоў удзельнікаў у іншых вікі',
'right-siteadmin'            => 'блякаваньне і разблякаваньне базы зьвестак',

# User rights log
'rightslog'      => 'Журнал правоў удзельнікаў',
'rightslogtext'  => 'Гэта журнал зьменаў правоў удзельнікаў.',
'rightslogentry' => 'зьменена прыналежнасьць $1 з групы $2 да $3',
'rightsnone'     => '(няма)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|зьмена|зьмены|зьменаў}}',
'recentchanges'                     => 'Апошнія зьмены',
'recentchangestext'                 => 'Сачыце за апошнімі зьменамі ў {{GRAMMAR:месны|{{SITENAME}}}} на гэтай старонцы.',
'recentchanges-feed-description'    => 'Сачыць за апошнімі зьменамі ў {{GRAMMAR:месны|{{SITENAME}}}} праз гэтую стужку.',
'rcnote'                            => "Ніжэй {{PLURAL:$1|пададзена '''$1''' апошняя зьмена|пададзеныя '''$1''' апошнія зьмены|пададзеныя '''$1''' апошніх зьменаў}} за {{PLURAL:$2|апошні '''$2''' дзень|апошнія '''$2''' дні|апошнія '''$2''' дзён}}, па стане на $5, $4.",
'rcnotefrom'                        => "Ніжэй знаходзяцца зьмены з '''$2''' (да '''$1''' на старонку).",
'rclistfrom'                        => 'Паказаць зьмены з $1',
'rcshowhideminor'                   => '$1 дробныя праўкі',
'rcshowhidebots'                    => '$1 робатаў',
'rcshowhideliu'                     => '$1 зарэгістраваных',
'rcshowhideanons'                   => '$1 ананімаў',
'rcshowhidepatr'                    => '$1 патруляваныя праўкі',
'rcshowhidemine'                    => '$1 мае праўкі',
'rclinks'                           => 'Паказаць апошнія $1 зьменаў за мінулыя $2 дзён<br />$3',
'diff'                              => 'розьн',
'hist'                              => 'гіст',
'hide'                              => 'схаваць',
'show'                              => 'паказаць',
'minoreditletter'                   => 'д',
'newpageletter'                     => 'Н',
'boteditletter'                     => 'р',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|назіральнік|назіральнікі|назіральнікаў}}]',
'rc_categories'                     => 'Абмежаваць катэгорыямі (разьдзяляйце знакам «|»)',
'rc_categories_any'                 => 'Усе',
'newsectionsummary'                 => '/* $1 */ новая сэкцыя',

# Recent changes linked
'recentchangeslinked'          => 'Зьвязаныя праўкі',
'recentchangeslinked-title'    => 'Зьвязаныя праўкі для «$1»',
'recentchangeslinked-noresult' => 'Не было ніякіх зьменаў на зьвязаных старонках за вызначаны пэрыяд.',
'recentchangeslinked-summary'  => "Гэта сьпіс апошніх зьменаў старонак, на якія спасылаецца азначаная старонка (ці ўсіх старонак, якія належаць азначанай катэгорыі).
Старонкі з [[Special:Watchlist|Вашага сьпісу назіраньня]] пазначаныя '''тлустым шрыфтам'''.",
'recentchangeslinked-page'     => 'Назва старонкі:',
'recentchangeslinked-to'       => 'Замест гэтага паказваць зьмены на старонках, што спасылаюцца на гэтую старонку',

# Upload
'upload'                      => 'Загрузіць файл',
'uploadbtn'                   => 'Загрузіць файл',
'reupload'                    => 'Загрузіць зноў',
'reuploaddesc'                => 'Адмяніць загрузку і вярнуцца да формы загрузкі',
'uploadnologin'               => 'Вы не ўвайшлі ў сыстэму',
'uploadnologintext'           => 'Вам трэба [[Special:UserLogin|ўвайсьці ў сыстэму]] для загрузкі файлаў.',
'upload_directory_missing'    => 'Загрузачная дырэкторыя ($1) адсутнічае і ня можа быць створаная сэрвэрам.',
'upload_directory_read_only'  => 'Сэрвэр ня мае правоў на запіс у дырэкторыю загружаных файлаў ($1).',
'uploaderror'                 => 'Памылка загрузкі',
'uploadtext'                  => "Ужывайце форму ніжэй для загрузкі файлаў.
Каб паглядзець ці адшукаць раней загружаныя файлы, глядзіце [[Special:ImageList|сьпіс загружаных файлаў]], загрузкі таксама запісваюцца ў [[Special:Log/upload|журнал загрузак]], а выдаленьні — у [[Special:Log/delete|журнал выдаленьняў]].

Каб улучыць файл у старонку, ужывайце адзін з наступных варыянтаў:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' для поўнай вэрсіі файла
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|Подпіс да выявы]]</nowiki></tt>''' для выявы шырынёй 200 піксэляў у рамцы і тэкстам «Подпіс да выявы» ў якасьці подпісу
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' для простай спасылкі на файл безь яго адлюстраваньня.",
'upload-permitted'            => 'Дазволеныя тыпы файлаў: $1.',
'upload-preferred'            => 'Пажаданыя тыпы файлаў: $1.',
'upload-prohibited'           => 'Забароненыя тыпы файлаў: $1.',
'uploadlog'                   => 'журнал загрузак',
'uploadlogpage'               => 'Журнал загрузак',
'uploadlogpagetext'           => 'Сьпіс апошніх загружаных файлаў.',
'filename'                    => 'Назва файла',
'filedesc'                    => 'Апісаньне',
'fileuploadsummary'           => 'Апісаньне:',
'filestatus'                  => 'Умовы распаўсюджаньня і выкарыстаньня:',
'filesource'                  => 'Крыніца:',
'uploadedfiles'               => 'Загружаныя файлы',
'ignorewarning'               => 'Праігнараваць папярэджаньне і захаваць файл',
'ignorewarnings'              => 'Ігнараваць усе папярэджаньні',
'minlength1'                  => 'Назва файла павінна ўтрымліваць хаця б адзін сымбаль.',
'illegalfilename'             => 'Назва файла «$1» зьмяшчае сымбалі, якія нельга выкарыстоўваць у назвах старонак. Калі ласка, зьмяніце назву файла і паспрабуйце загрузіць яго зноў.',
'badfilename'                 => 'Назва файла была зьмененая на «$1».',
'filetype-badmime'            => 'Загрузка файлаў тыпу MIME «$1» забароненая.',
'filetype-unwanted-type'      => "'''«.$1»''' — непажаданы тып файла.
{{PLURAL:$3|Пажаданым тыпам файла зьяўляецца|Пажаданымі тыпамі файлаў зьяўляюцца}} $2.",
'filetype-banned-type'        => "'''«.$1»''' — забаронены тып файлаў. {{PLURAL:$3|Дазволеным тыпам файла зьяўляецца|Дазволенымі тыпамі файлаў зьяўляюцца}} $2.",
'filetype-missing'            => 'Файл ня мае пашырэньня (напрыклад, «.jpg»).',
'large-file'                  => 'Рэкамэндаваны памер файлаў — ня болей $1;
памер гэтага файла — $2.',
'largefileserver'             => 'Памер гэтага файла перавышае максымальна дазволены.',
'emptyfile'                   => 'Загружаны файл, здаецца, пусты. Магчыма гэты адбылося з-за памылкі ў назьве файла.
Удакладніце, ці Вы сапраўды жадаеце загрузіць гэты файл.',
'fileexists'                  => 'Файл з такой назвай ужо існуе. Калі ласка, праверце <strong><tt>$1</tt></strong>, калі Вы ня ўпэўненыя, што хаціце яго замяніць.',
'filepageexists'              => 'Старонка апісаньня для гэтага файла ўжо існуе як <strong><tt>$1</tt></strong>, але файла з такой назвай няма.
Апісаньне якое Вы дадалі ня зьявіцца на старонцы апісаньня.
Каб яно там зьявілася, Вам трэба рэдагаваць яе самастойна',
'fileexists-extension'        => 'Файл з падобнай назвай ужо існуе:<br />
Назва загружанага файла: <strong><tt>$1</tt></strong><br />
Назва існуючага файла: <strong><tt>$2</tt></strong><br />
Калі ласка, выбярыце іншую назву.',
'fileexists-thumb'            => "<center>'''Існуючы файл'''</center>",
'fileexists-thumbnail-yes'    => 'Верагодна файл зьяўляецца паменшанай копіяй <i>(мініятурай)</i>.
Калі ласка, праверце файл <strong><tt>$1</tt></strong>.<br />
Калі правераны файл зьяўляецца той жа выявай, то загрузка мініятуры ня мае сэнсу.',
'file-thumbnail-no'           => 'Назва файла пачынаецца з <strong><tt>$1</tt></strong>.
Верагодна гэта паменшаная копія выявы <i>(мініятура)</i>.
Калі Вы маеце гэтую выяву ў поўным памеры, загрузіце яе, альбо зьмяніце назву файла.',
'fileexists-forbidden'        => 'Файл з такой назвай ужо існуе.
Калі ласка, вярніцеся назад і загрузіце гэты файл з новай назвай. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Файл з такой назвай ужо існуе ў агульным сховішчы файлаў.
Калі Вы жадаеце загрузіць Ваш файл, вярніцеся назад і загрузіце гэты файл з новай назвай. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Гэты файл дублюе {{PLURAL:$1|наступны файл|наступныя файлы}}:',
'successfulupload'            => 'Загрузка пасьпяхова скончылася',
'uploadwarning'               => 'Папярэджаньне',
'savefile'                    => 'Захаваць файл',
'uploadedimage'               => 'загружаная «[[$1]]»',
'overwroteimage'              => 'загружаная новая вэрсія «[[$1]]»',
'uploaddisabled'              => 'Загрузка файлаў забароненая',
'uploaddisabledtext'          => 'Загрузка файлаў забароненая.',
'uploadscripted'              => 'Гэты файл утрымлівае HTML-код альбо скрыпт, які можа памылкова апрацоўвацца браўзэрам.',
'uploadcorrupt'               => 'Файл пашкоджаны альбо мае неадпаведнае пашырэньне.
Калі ласка, праверце файл і загрузіце яго зноў.',
'uploadvirus'                 => 'Файл утрымлівае вірус! Падрабязнасьці: $1',
'sourcefilename'              => 'Пачатковая назва файла:',
'destfilename'                => 'Канчатковая назва файла:',
'upload-maxfilesize'          => 'Максымальны памер файла: $1',
'watchthisupload'             => 'Назіраць за гэтай старонкай',
'filewasdeleted'              => 'Файл з такой назвай загружаўся, але быў выдалены.
Вам трэба праверыць $1 перад новай загрузкай.',
'upload-wasdeleted'           => "'''Папярэджаньне: Вы спрабуеце загрузіць файл, які ўжо выдаляўся.'''

Упэўніцеся ў неабходнасьці загрузкі гэтага файла.
Журнал выдаленьняў для гэтага файла знаходзіцца ніжэй:",
'filename-bad-prefix'         => 'Назва файла, які Вы загружаеце, пачынаецца з <strong>«$1»</strong>. Падобныя бессэнсоўныя назвы звычайна ствараюцца аўтаматычна лічбавымі фотаапаратамі. Калі ласка, абярыце больш зразумелую назву для Вашага файла.',

'upload-proto-error'      => 'Няслушны пратакол',
'upload-proto-error-text' => 'Аддаленая загрузка файлаў патрабуе URL-адрас, які пачынаецца з <code>http://</code> альбо <code>ftp://</code>.',
'upload-file-error'       => 'Унутраная памылка',
'upload-file-error-text'  => 'Адбылася ўнутраная памылка пад час спробы стварыць часовы файл на сэрвэры.
Калі ласка, зьвярніцеся да [[Special:ListUsers/sysop|сыстэмнага адміністратара]].',
'upload-misc-error'       => 'Невядомая памылка загрузкі',
'upload-misc-error-text'  => 'Адбылася невядомая памылка пад час загрузкі.
Калі ласка, ўпэўніцеся, што URL-адрас слушны, і паспрабуйце ізноў.
Калі памылка паўтарыцца, зьвярніцеся да [[Special:ListUsers/sysop|сыстэмнага адміністратара]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Немагчыма дасягнуць URL-адрас',
'upload-curl-error6-text'  => 'Немагчыма адкрыць пазначаны URL-адрас.
Калі ласка, упэўніцеся, што адрас слушны і сайт працуе.',
'upload-curl-error28'      => 'Ліміт часу загрузкі скончыўся',
'upload-curl-error28-text' => 'Сайт доўга не адказвае.
Калі ласка, упэўніцеся што сайт працуе, і празь некаторы час паспрабуйце яшчэ раз.
Вы можаце паспрабаваць пад час меншай загрузкі сайта.',

'license'            => 'Ліцэнзія:',
'nolicense'          => 'Не выбраная',
'license-nopreview'  => '(Прагляд недаступны)',
'upload_source_url'  => ' (слушны, агульнадаступны URL-адрас)',
'upload_source_file' => ' (файл на Вашым кампутары)',

# Special:ImageList
'imagelist-summary'     => 'На гэтай спэцыяльнай старонцы паказаныя ўсе загружаныя файлы.
Па змоўчваньні апошнія загружаныя файлы адлюстраваныя зьверху сьпісу.
Націсьніце на назву слупка, каб зьмяніць парадак сартаваньня.',
'imagelist_search_for'  => 'Пошук па назьве файла:',
'imgfile'               => 'файл',
'imagelist'             => 'Сьпіс файлаў',
'imagelist_date'        => 'Дата',
'imagelist_name'        => 'Назва',
'imagelist_user'        => 'Удзельнік',
'imagelist_size'        => 'Памер',
'imagelist_description' => 'Апісаньне',

# Image description page
'filehist'                       => 'Гісторыя файла',
'filehist-help'                  => 'Націсьніце на дату/час каб паглядзець, як тады выглядаў файл.',
'filehist-deleteall'             => 'выдаліць усе',
'filehist-deleteone'             => 'выдаліць',
'filehist-revert'                => 'вярнуць',
'filehist-current'               => 'цяперашняя',
'filehist-datetime'              => 'Дата і час',
'filehist-user'                  => 'Удзельнік',
'filehist-dimensions'            => 'Памеры',
'filehist-filesize'              => 'Памер файла',
'filehist-comment'               => 'Камэнтар',
'imagelinks'                     => 'Спасылкі',
'linkstoimage'                   => '{{PLURAL:$1|Наступная старонка спасылаецца|Наступныя старонкі спасылаюцца}} на гэты файл:',
'nolinkstoimage'                 => 'Ніводная старонка не спасылаецца на гэты файл.',
'morelinkstoimage'               => 'Паказаць [[Special:WhatLinksHere/$1|болей спасылак]] на гэты файл.',
'redirectstofile'                => '{{PLURAL:$1|Наступны файл перанакіроўвае|Наступныя файлы перанакіроўваюць}} на гэты файл:',
'duplicatesoffile'               => '{{PLURAL:$1|Наступны файл дублюе|Наступныя файлы дублююць}} гэты файл:',
'sharedupload'                   => 'Гэты файл зьяўляецца агульным і можа выкарыстоўвацца іншымі праектамі.',
'shareduploadwiki'               => 'Глядзіце дадатковую інфармацыю на $1.',
'shareduploadwiki-desc'          => 'Апісаньне $1 з агульнага сховішча паказанае ніжэй.',
'shareduploadwiki-linktext'      => 'старонка апісаньня файла',
'shareduploadduplicate'          => 'Гэты файл дублюе $1 з агульнага сховішча.',
'shareduploadduplicate-linktext' => 'іншага файла',
'shareduploadconflict'           => 'Гэты файл мае такую ж назву як і $1 з агульнага сховішча.',
'shareduploadconflict-linktext'  => 'іншы файл',
'noimage'                        => 'Няма файла з такой назвай, але Вы можаце $1.',
'noimage-linktext'               => 'загрузіць яго',
'uploadnewversion-linktext'      => 'Загрузіць новую вэрсію гэтага файла',
'imagepage-searchdupe'           => 'Пошук дублікатаў файлаў',

# File reversion
'filerevert'                => 'Вярнуць $1',
'filerevert-legend'         => 'Вярнуць папярэднюю вэрсію файла',
'filerevert-intro'          => "Вы вяртаеце '''[[Media:$1|$1]]''' да [вэрсіі $4 ад $3, $2].",
'filerevert-comment'        => 'Камэнтар:',
'filerevert-defaultcomment' => 'Вернутая вэрсія ад $2, $1',
'filerevert-submit'         => 'Вярнуць',
'filerevert-success'        => "'''[[Media:$1|$1]]''' быў вернуты да [вэрсіі $4 ад $3, $2].",
'filerevert-badversion'     => 'Не існуе папярэдняй лякальнай вэрсіі гэтага файла з пазначанай датай.',

# File deletion
'filedelete'                  => 'Выдаліць $1',
'filedelete-legend'           => 'Выдаліць файл',
'filedelete-intro'            => "Вы выдаляеце '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Вы выдаляеце вэрсію '''[[Media:$1|$1]]''' ад [$4 $3, $2].",
'filedelete-comment'          => 'Прычына выдаленьня:',
'filedelete-submit'           => 'Выдаліць',
'filedelete-success'          => "'''$1''' выдалены.",
'filedelete-success-old'      => "Вэрсія '''[[Media:$1|$1]]''' ад $3, $2 была выдаленая.",
'filedelete-nofile'           => "Файл '''$1''' не існуе.",
'filedelete-nofile-old'       => "Не існуе архіўнай вэрсіі '''$1''' з пазначанымі атрыбутамі.",
'filedelete-iscurrent'        => 'Вы спрабуеце выдаліць апошнюю вэрсію гэтага файла.
Спачатку трэба вярнуць папярэднюю вэрсію.',
'filedelete-otherreason'      => 'Іншая/дадатковая прычына:',
'filedelete-reason-otherlist' => 'Іншая прычына',
'filedelete-reason-dropdown'  => '* Звычайныя прычыны выдаленьня
** парушэньне аўтарскіх правоў
** файл-дублікат',
'filedelete-edit-reasonlist'  => 'Рэдагаваць прычыны выдаленьня',

# MIME search
'mimesearch'         => 'Пошук па MIME',
'mimesearch-summary' => 'Гэта старонка дазваляе адбор файлаў па іх MIME-тыпе.
Фармат уводу: тып_зьместу/падтып, напрыклад, <tt>image/jpeg</tt>.',
'mimetype'           => 'Тып MIME:',
'download'           => 'загрузіць',

# Unwatched pages
'unwatchedpages' => 'Старонкі, за якімі ніхто не назірае',

# List redirects
'listredirects' => 'Сьпіс перанакіраваньняў',

# Unused templates
'unusedtemplates'     => 'Шаблёны, якія не выкарыстоўваюцца',
'unusedtemplatestext' => 'На гэтай старонцы зьмешчаны сьпіс усіх старонак прасторы назваў «Шаблёны», якія ня ўключаныя ў іншыя старонкі.
Не забывайцеся праверыць іншыя спасылкі на шаблёны перад іх выдаленьнем.',
'unusedtemplateswlh'  => 'іншыя спасылкі',

# Random page
'randompage'         => 'Выпадковая старонка',
'randompage-nopages' => 'У гэтай прасторы назваў няма старонак.',

# Random redirect
'randomredirect'         => 'Выпадковае перанакіраваньне',
'randomredirect-nopages' => 'У гэтай прасторы назваў няма перанакіраваньняў.',

# Statistics
'statistics'             => 'Статыстыка',
'sitestats'              => 'Статыстыка {{GRAMMAR:родны|{{SITENAME}}}}',
'userstats'              => 'Статыстыка ўдзелу',
'sitestatstext'          => "У базе зьвестак утрымліваецца '''$1''' {{PLURAL:$1|старонка|старонкі|старонак}}.
Сюды ўваходзяць старонкі пра {{GRAMMAR:вінавальны|{{SITENAME}}}}, старонкі абмеркаваньняў, накіды, перанакіраваньні і іншыя старонкі якія ня ўлічваюцца пры падліку артыкулаў.
Акрамя іх ёсьць '''$2''' {{PLURAL:$2|старонка|старонкі|старонак}}, якія лічацца паўнавартаснымі.

{{PLURAL:$8|Быў загружаны '''$8''' файл|Было загружана '''$8''' файлы|Было загружана '''$8''' файлаў}}.

Усяго было '''$3''' {{PLURAL:$3|прагляд|прагляды|праглядаў}}, і '''$4''' {{PLURAL:$4|рэдагаваньне|рэдагаваньні|рэдагаваньняў}} з пачатку працы {{GRAMMAR:родны|{{SITENAME}}}}.
Такім чынам, у сярэднім прыходзіцца '''$5''' рэдагаваньне на адну старонку, і '''$6''' праглядаў на адно рэдагаваньне.

Даўжыня [http://www.mediawiki.org/wiki/Manual:Job_queue чаргі заданьняў] складае '''$7'''.",
'userstatstext'          => "Тут {{PLURAL:$1|зарэгістраваны|зарэгістраваныя|зарэгістраваныя}} '''$1''' [[Special:ListUsers|{{PLURAL:$1|удзельнік|удзельнікі|удзельнікаў}}]], зь якіх '''$2''' (ці '''$4%''') {{PLURAL:$2|мае|маюць|маюць}} правы $5.",
'statistics-mostpopular' => 'Найпапулярнейшыя старонкі',

'disambiguations'      => 'Старонкі-неадназначнасьці',
'disambiguationspage'  => 'Шаблён:Неадназначнасьць',
'disambiguations-text' => "Наступныя старонкі спасылаюцца на '''старонкі-неадназначнасьці'''.
Замест гэтага, яны павінны спасылацца на пэўныя старонкі.<br />
Старонка лічыцца шматзначнай, калі яна ўтрымлівае шаблён назва якога знаходзіцца на старонцы [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Двайныя перанакіраваньні',
'doubleredirectstext'        => 'На гэтай старонцы знаходзіцца сьпіс перанакіраваньняў на іншыя перанакіраваньні. Кожны радок утрымлівае спасылкі на першае і другое перанакіраваньне, а таксама першы радок старонкі другога перанакіраваньня, у якім звычайна знаходзіцца назва старонкі, куды павіннае спасылацца першае перанакіраваньне.',
'double-redirect-fixed-move' => '[[$1]] была перанесеная, яна цяпер перанакіроўвае на [[$2]]',
'double-redirect-fixer'      => 'Выпраўленьне перанакіраваньняў',

'brokenredirects'        => 'Некарэктныя перанакіраваньні',
'brokenredirectstext'    => 'Наступныя перанакіраваньні спасылаюцца на неіснуючыя старонкі:',
'brokenredirects-edit'   => '(рэдагаваць)',
'brokenredirects-delete' => '(выдаліць)',

'withoutinterwiki'         => 'Старонкі без спасылак на іншыя моўныя вэрсіі',
'withoutinterwiki-summary' => 'Наступныя старонкі ня ўтрымліваюць спасылак на іншамоўныя вэрсіі:',
'withoutinterwiki-legend'  => 'Прэфікс',
'withoutinterwiki-submit'  => 'Паказаць',

'fewestrevisions' => 'Старонкі з найменшай колькасьцю рэдагаваньняў',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|байт|байты|байтаў}}',
'ncategories'             => '$1 {{PLURAL:$1|катэгорыя|катэгорыі|катэгорый}}',
'nlinks'                  => '$1 {{PLURAL:$1|спасылка|спасылкі|спасылак}}',
'nmembers'                => '$1 {{PLURAL:$1|элемэнт|элемэнты|элемэнтаў}}',
'nrevisions'              => '$1 {{PLURAL:$1|вэрсія|вэрсіі|вэрсій}}',
'nviews'                  => '$1 {{PLURAL:$1|прагляд|прагляды|праглядаў}}',
'specialpage-empty'       => 'Гэтая старонка — пустая.',
'lonelypages'             => 'Старонкі-сіраціны',
'lonelypagestext'         => 'На наступныя старонкі няма спасылак зь іншых старонак {{GRAMMAR:родны|{{SITENAME}}}}.',
'uncategorizedpages'      => 'Некатэгарызаваныя старонкі',
'uncategorizedcategories' => 'Некатэгарызаваныя катэгорыі',
'uncategorizedimages'     => 'Некатэгарызаваныя файлы',
'uncategorizedtemplates'  => 'Некатэгарызаваныя шаблёны',
'unusedcategories'        => 'Катэгорыі, якія не выкарыстоўваюцца',
'unusedimages'            => 'Файлы, якія не выкарыстоўваюцца',
'popularpages'            => 'Папулярныя старонкі',
'wantedcategories'        => 'Запатрабаваныя катэгорыі',
'wantedpages'             => 'Запатрабаваныя старонкі',
'missingfiles'            => 'Адсутныя файлы',
'mostlinked'              => 'Старонкі, на якія найчасьцей спасылаюцца',
'mostlinkedcategories'    => 'Катэгорыі з найбольшай колькасьцю артыкулаў',
'mostlinkedtemplates'     => 'Шаблёны, якія найчасьцей выкарыстоўваюцца',
'mostcategories'          => 'Старонкі з найбольшай колькасьцю катэгорый',
'mostimages'              => 'Файлы, на якія найчасьцей спасылаюцца',
'mostrevisions'           => 'Старонкі з найбольшай колькасьцю рэдагаваньняў',
'prefixindex'             => 'Пошук старонак па пачатку назвы',
'shortpages'              => 'Кароткія старонкі',
'longpages'               => 'Доўгія старонкі',
'deadendpages'            => 'Тупіковыя старонкі',
'deadendpagestext'        => 'Наступныя старонкі не спасылаюцца на іншыя старонкі {{GRAMMAR:родны|{{SITENAME}}}}.',
'protectedpages'          => 'Абароненыя старонкі',
'protectedpages-indef'    => 'Толькі бестэрміновыя абароны',
'protectedpagestext'      => 'Наступныя старонкі абароненыя ад пераносаў і рэдагаваньняў',
'protectedpagesempty'     => 'Цяпер няма абароненых старонак з пазначанымі парамэтрамі.',
'protectedtitles'         => 'Забароненыя старонкі',
'protectedtitlestext'     => 'Стварэньне наступных старонак забароненае',
'protectedtitlesempty'    => 'Цяпер няма абароненых назваў з пазначанымі парамэтрамі.',
'listusers'               => 'Сьпіс удзельнікаў і ўдзельніц',
'newpages'                => 'Новыя старонкі',
'newpages-username'       => 'Імя ўдзельніка/ўдзельніцы:',
'ancientpages'            => 'Найстарэйшыя старонкі',
'move'                    => 'Перанесьці',
'movethispage'            => 'Перанесьці гэтую старонку',
'unusedimagestext'        => 'Калі ласка, заўважце, што іншыя сайты могуць спасылацца на гэты файл празь непасрэдную спасылку (URL), і ён можа актыўна выкарыстоўвацца нягледзячы на знаходжаньне ў гэтым сьпісе.',
'unusedcategoriestext'    => 'Існуюць наступныя катэгорыі, якія ня ўтрымліваюць артыкулаў ці іншых катэгорыяў.',
'notargettitle'           => 'Не пазначаная мэта',
'notargettext'            => 'Вы не пазначылі мэтавую старонку альбо ўдзельніка для гэтага дзеяньня.',
'nopagetitle'             => 'Мэтавая старонка не існуе',
'nopagetext'              => 'Пазначанай мэтавай старонкі не існуе.',
'pager-newer-n'           => '$1 {{PLURAL:$1|навейшая|навейшыя|навейшых}}',
'pager-older-n'           => '$1 {{PLURAL:$1|старэйшая|старэйшых|старэйшых}}',
'suppress'                => 'Рэвізаваць',

# Book sources
'booksources'               => 'Пошук кніг',
'booksources-search-legend' => 'Пошук кніг',
'booksources-go'            => 'Паказаць',
'booksources-text'          => 'Ніжэй знаходзіцца сьпіс спасылак на іншыя сайты, якія прадаюць новыя і патрыманыя кнігі, і могуць таксама мець інфармацыю пра кнігі, якія Вы шукаеце:',

# Special:Log
'specialloguserlabel'  => 'Удзельнік/удзельніца:',
'speciallogtitlelabel' => 'Назва:',
'log'                  => 'Журналы падзей',
'all-logs-page'        => 'Усе журналы падзей',
'log-search-legend'    => 'Пошук у журналах',
'log-search-submit'    => 'Шукаць',
'alllogstext'          => 'Сумесны паказ усіх журналаў падзей {{GRAMMAR:родны|{{SITENAME}}}}.
Вы можаце адфільтраваць вынікі па тыпе журналу, удзельніку ці старонцы.',
'logempty'             => 'Падобных запісаў у журнале няма.',
'log-title-wildcard'   => 'Шукаць назвы, якія пачынаюцца з гэтага тэксту',

# Special:AllPages
'allpages'          => 'Усе старонкі',
'alphaindexline'    => 'ад $1 да $2',
'nextpage'          => 'Наступная старонка ($1)',
'prevpage'          => 'Папярэдняя старонка ($1)',
'allpagesfrom'      => 'Паказаць старонкі, пачынаючы з:',
'allarticles'       => 'Усе старонкі',
'allinnamespace'    => 'Усе старонкі (прастора назваў: $1)',
'allnotinnamespace' => 'Усе старонкі (не ў прасторы назваў $1)',
'allpagesprev'      => 'Папярэднія',
'allpagesnext'      => 'Наступныя',
'allpagessubmit'    => 'Паказаць',
'allpagesprefix'    => 'Паказаць старонкі, назвы якіх пачынаюцца з:',
'allpagesbadtitle'  => 'Пададзеная назва старонкі была няслушная ці пачыналася зь міжмоўнай ці міжвікі спасылкі. Яна яшчэ можа ўтрымліваць сымбалі, якія ня могуць ужывацца ў назвах.',
'allpages-bad-ns'   => '{{SITENAME}} ня мае прасторы назваў «$1».',

# Special:Categories
'categories'                    => 'Катэгорыі',
'categoriespagetext'            => 'Наступныя катэгорыі зьмяшчаюць старонкі і/альбо мэдыя-файлы.
Тут не паказаныя [[Special:UnusedCategories|катэгорыі, якія не выкарыстоўваюцца]].
Глядзіце таксама [[Special:WantedCategories|сьпіс запатрабаваных катэгорый]].',
'categoriesfrom'                => 'Паказаць катэгорыі, пачынаючы з:',
'special-categories-sort-count' => 'сартаваць паводле колькасьці',
'special-categories-sort-abc'   => 'сартаваць паводле альфабэту',

# Special:ListUsers
'listusersfrom'      => 'Паказаць удзельнікаў, пачынаючы з:',
'listusers-submit'   => 'Паказаць',
'listusers-noresult' => 'Удзельнікі ня знойдзеныя.',

# Special:ListGroupRights
'listgrouprights'          => 'Правы групаў удзельнікаў',
'listgrouprights-summary'  => 'Ніжэй пададзены сьпіс групаў удзельнікаў {{GRAMMAR:родны|{{SITENAME}}}}, разам зь іх правамі.
Таксама можна паглядзець [[{{MediaWiki:Listgrouprights-helppage}}|дадатковую інфармацыю]] пра асабістыя правы.',
'listgrouprights-group'    => 'Група',
'listgrouprights-rights'   => 'Правы',
'listgrouprights-helppage' => 'Help:Правы групаў удзельнікаў',
'listgrouprights-members'  => '(сьпіс удзельнікаў групы)',

# E-mail user
'mailnologin'     => 'Няма адрасу атрымальніка',
'mailnologintext' => 'Вам неабходна [[Special:UserLogin|ўвайсьці ў сыстэму]] і мець пацьверджаны адрас электроннай пошты ў Вашых [[Special:Preferences|устаноўках]], каб дасылаць лісты іншым удзельнікам.',
'emailuser'       => 'Даслаць ліст',
'emailpage'       => 'Даслаць ліст ўдзельніку ці ўдзельніцы па электроннай пошце',
'emailpagetext'   => 'Калі гэты ўдзельнік пазначыў слушны адрас электроннай пошты ў сваіх устаноўках, то з дапамогай формы ніжэй можна даслаць яму ліст.
Адрас электроннай пошты, які Вы пазначалі ў [[Special:Preferences|сваіх устаноўках]], будзе пазначаны ў полі ліста «Ад», і ўдзельнік зможа даслаць на гэты адрас адказ.',
'usermailererror' => 'Пры адсыланьні пошты адбылася памылка:',
'defemailsubject' => 'Электронная пошта {{GRAMMAR:родны|{{SITENAME}}}}',
'noemailtitle'    => 'Адрас электроннай пошты адсутнічае',
'noemailtext'     => 'Гэты удзельнік не пазначыў слушны адрас электроннай пошты альбо пазначыў, што не жадае атрымліваць лісты ад іншых удзельнікаў.',
'emailfrom'       => 'Ад:',
'emailto'         => 'Каму:',
'emailsubject'    => 'Тэма:',
'emailmessage'    => 'Паведамленьне:',
'emailsend'       => 'Даслаць',
'emailccme'       => 'Даслаць мне копію ліста.',
'emailccsubject'  => 'Копія Вашага ліста да $1: $2',
'emailsent'       => 'Ліст адасланы',
'emailsenttext'   => 'Ваш ліст быў адасланы.',
'emailuserfooter' => 'Гэты ліст быў дасланы ўдзельнікам $1 да ўдзельніка $2 з дапамогай функцыі «Даслаць ліст» праекту {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Мой сьпіс назіраньня',
'mywatchlist'          => 'Мой сьпіс назіраньня',
'watchlistfor'         => "(для '''$1''')",
'nowatchlist'          => 'Ваш сьпіс назіраньня — пусты.',
'watchlistanontext'    => 'Калі ласка, $1, каб праглядаць альбо рэдагаваць сьпіс назіраньня.',
'watchnologin'         => 'Вы не ўвайшлі ў сыстэму',
'watchnologintext'     => 'Вам неабходна [[Special:UserLogin|ўвайсьці ў сыстэму]], каб зьмяніць Ваш сьпіс назіраньня.',
'addedwatch'           => 'Дададзеная ў сьпіс назіраньня',
'addedwatchtext'       => "Артыкул «$1» быў дададзены да Вашага [[Special:Watchlist|сьпісу назіраньня]].
Наступныя зьмены ў гэтым артыкуле і зьвязанай зь ім старонцы абмеркаваньняў будуць бачныя там, і старонка будзе выглядаць '''тлустай''' на [[Special:RecentChanges|старонцы са сьпісам апошніх зьменаў]], каб зьмены было лягчэй заўважыць.

Калі Вы захочаце выдаліць артыкул са сьпісу назіраньня, націсьніце «не назіраць» у спэцыяльным радку зьверху артыкула.",
'removedwatch'         => 'Выдаленая са сьпісу назіраньня',
'removedwatchtext'     => 'Старонка «[[:$1]]» была выдаленая з Вашага сьпісу назіраньня.',
'watch'                => 'Назіраць',
'watchthispage'        => 'Назіраць за гэтай старонкай',
'unwatch'              => 'Не назіраць',
'unwatchthispage'      => 'Перастаць назіраць',
'notanarticle'         => 'Не артыкул',
'notvisiblerev'        => 'Вэрсія была выдаленая',
'watchnochange'        => 'Нічога з Вашага сьпісу назіраньня не зьмянілася за паказаны пэрыяд.',
'watchlist-details'    => 'У Вашым сьпісе назіраньня $1 {{PLURAL:$1|старонка|старонкі|старонак}} за выключэньнем старонак абмеркаваньня.',
'wlheader-enotif'      => '* Дасылка паведамленьняў па электроннай пошце ўключаная.',
'wlheader-showupdated' => "* Старонкі, якія былі зьмененыя пасьля Вашага апошняга візыту, вылучаныя '''тлустым''' шрыфтам",
'watchmethod-recent'   => 'прагляд апошніх зьменаў у старонках са сьпісу назіраньня',
'watchmethod-list'     => 'прагляд старонак са сьпісу назіраньня дзеля апошніх зьменах',
'watchlistcontains'    => 'Ваш сьпіс назіраньня зьмяшчае $1 {{PLURAL:$1|старонка|старонкі|старонак}}.',
'iteminvalidname'      => 'Праблема з элемэнтам «$1», няслушная назва…',
'wlnote'               => "Ніжэй {{PLURAL:$1|паказаная '''$1''' апошняя зьмена|паказаныя '''$1''' апошнія зьмены|паказаныя '''$1''' апошніх зьменаў}} за '''$2''' {{PLURAL:$2|гадзіну|гадзіны|гадзінаў}}.",
'wlshowlast'           => 'Паказваць за апошнія $1 гадзінаў $2 дзён $3',
'watchlist-show-bots'  => 'паказаць робатаў',
'watchlist-hide-bots'  => 'схаваць робатаў',
'watchlist-show-own'   => 'паказаць мае праўкі',
'watchlist-hide-own'   => 'схаваць мае праўкі',
'watchlist-show-minor' => 'паказаць дробныя праўкі',
'watchlist-hide-minor' => 'схаваць дробныя праўкі',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Дадаецца ў сьпіс назіраньня…',
'unwatching' => 'Выдаляецца са сьпісу назіраньня…',

'enotif_mailer'                => 'Служба паштовага апавяшчэньня {{GRAMMAR:родны|{{SITENAME}}}}',
'enotif_reset'                 => 'Пазначыць усе старонкі як прагледжаныя',
'enotif_newpagetext'           => 'Гэта новая старонка.',
'enotif_impersonal_salutation' => 'Удзельнік {{GRAMMAR:родны|{{SITENAME}}}}',
'changed'                      => 'зьмененая',
'created'                      => 'створаная',
'enotif_subject'               => 'Старонка {{GRAMMAR:родны|{{SITENAME}}}} $PAGETITLE была $CHANGEDORCREATED удзельнікам $PAGEEDITOR',
'enotif_lastvisited'           => 'Глядзіце на $1 усе апошнія зьмены, якія адбыліся пасьля Вашага апошняга наведваньня.',
'enotif_lastdiff'              => 'Глядзіце $1, каб пабачыць гэтую зьмену.',
'enotif_anon_editor'           => 'ананімны ўдзельнік $1',
'enotif_body'                  => '$WATCHINGUSERNAME,


Старонка $PAGETITLE {{GRAMMAR:родны|{{SITENAME}}}} была $CHANGEDORCREATED $PAGEEDITDATE удзельнікам $PAGEEDITOR, цяперашнюю вэрсію глядзіце на $PAGETITLE_URL.

$NEWPAGE

Кароткае апісаньне зьменаў: $PAGESUMMARY $PAGEMINOREDIT

Зьвязацца з аўтарам:
электронная пошта: $PAGEEDITOR_EMAIL
вікі-старонка: $PAGEEDITOR_WIKI

Паведамленьні ня будуць дасылацца ў выпадку паўторных рэдагаваньняў, пакуль Вы не наведаеце гэтую старонку.
Вы можаце пазначыць сьцяжкі дасылкі паведамленьняў ва ўсіх старонках назіраньня Вашага сьпісу назіраньня.

             Сыстэма паведамленьняў {{GRAMMAR:родны|{{SITENAME}}}} 

--
Каб зьмяніць устаноўкі сьпісу назіраньня, наведайце:
{{fullurl:{{ns:special}}:Watchlist/edit}}

Зваротная сувязь і дапамога:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Выдаліць старонку',
'confirm'                     => 'Пацьверджаньне',
'excontent'                   => 'колішні зьмест: «$1»',
'excontentauthor'             => "зьмест быў: «$1» (і адзіным аўтарам быў '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => 'зьмест да ачысткі: «$1»',
'exblank'                     => 'старонка была пустая',
'delete-confirm'              => 'Выдаліць «$1»',
'delete-legend'               => 'Выдаліць',
'historywarning'              => 'Папярэджаньне: у старонкі, якую Вы зьбіраецеся выдаліць, ёсьць гісторыя:',
'confirmdeletetext'           => 'Зараз Вы выдаліце старонку разам з усёй гісторыяй зьменаў.
Калі ласка, пацьвердзіце, што Вы зьбіраецеся гэта зрабіць і што Вы разумееце ўсе наступствы, а таксама робіце гэта ў адпаведнасьці з [[{{MediaWiki:Policy-url}}|правіламі]].',
'actioncomplete'              => 'Дзеяньне завершанае',
'deletedtext'                 => '«<nowiki>$1</nowiki>» была выдаленая.
Глядзіце журнал выдаленьняў у $2.',
'deletedarticle'              => 'выдаленая «[[$1]]»',
'suppressedarticle'           => 'схаваў «[[$1]]»',
'dellogpage'                  => 'Журнал выдаленьняў',
'dellogpagetext'              => 'Сьпіс апошніх выдаленьняў.',
'deletionlog'                 => 'журнал выдаленьняў',
'reverted'                    => 'Вернутая да папярэдняй вэрсіі',
'deletecomment'               => 'Прычына выдаленьня:',
'deleteotherreason'           => 'Іншая/дадатковая прычына:',
'deletereasonotherlist'       => 'Іншая прычына',
'deletereason-dropdown'       => '* Агульныя прычыны выдаленьня
** Запыт аўтара/аўтаркі
** Парушэньне аўтарскіх правоў
** Вандалізм',
'delete-edit-reasonlist'      => 'Рэдагаваць прычыны выдаленьня',
'delete-toobig'               => 'Гэтая старонка мае доўгую гісторыю рэдагаваньняў, болей за $1 {{PLURAL:$1|вэрсію|вэрсіі|вэрсій}}.
Выдаленьне такіх старонак было забароненае, каб пазьбегнуць праблемаў у працы {{GRAMMAR:родны|{{SITENAME}}}}.',
'delete-warning-toobig'       => 'Гэтая старонка мае доўгую гісторыю рэдагаваньняў, больш за $1 {{PLURAL:$1|вэрсію|вэрсіі|вэрсій}}.
Яе выдаленьне можа выклікаць праблемы ў працы базы зьвестак {{GRAMMAR:родны|{{SITENAME}}}}; будзьце асьцярожны.',
'rollback'                    => 'Адмяніць рэдагаваньні',
'rollback_short'              => 'Адкат',
'rollbacklink'                => 'адкат',
'rollbackfailed'              => 'Памылка адкату',
'cantrollback'                => 'Немагчыма скасаваць зьмену; апошні рэдактар — адзіны аўтар гэтай старонкі.',
'alreadyrolled'               => 'Немагчыма скасаваць апошнюю зьмену [[:$1]], якую зрабіў [[User:$2|$2]] ([[User talk:$2|гутаркі]]); нехта іншы ўжо зьмяніў артыкул ці скасаваў зьмены.

Апошнія зьмены зробленыя [[User:$3|$3]] ([[User talk:$3|гутаркі]]).',
'editcomment'                 => 'Камэнтар рэдагаваньня быў: «<i>$1</i>».', # only shown if there is an edit comment
'revertpage'                  => 'Адмененыя рэдагаваньні [[Special:Contributions/$2|$2]] ([[User talk:$2|Talk]]) да папярэдняй вэрсіі [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Адмененыя рэдагаваньні $1;
вернутая папярэдняя вэрсія $2.',
'sessionfailure'              => 'Магчыма ўзьніклі праблемы ў Вашым цяперашнім сэансе працы;
гэта дзеяньне было скасавана для прадухіленьня перахвату сэанса.
Калі ласка, націсьніце «назад» і перазагрузіце старонку зь якой Вы прыйшлі і паспрабуйце ізноў.',
'protectlogpage'              => 'Журнал абаронаў',
'protectlogtext'              => 'Ніжэй знаходзіцца сьпіс абаронаў і зьняцьцяў абаронаў.
Глядзіце [[Special:ProtectedPages|сьпіс абароненых старонак на цяперашні момант]].',
'protectedarticle'            => 'абароненая «[[$1]]»',
'modifiedarticleprotection'   => 'зьменены ўзровень абароны старонкі «[[$1]]»',
'unprotectedarticle'          => 'зьнятая абарона са старонкі «[[$1]]»',
'protect-title'               => 'Абарона «$1»',
'protect-legend'              => 'Пацьверджаньне абароны',
'protectcomment'              => 'Камэнтар:',
'protectexpiry'               => 'Сканчаецца:',
'protect_expiry_invalid'      => 'Няслушны час сканчэньня абароны.',
'protect_expiry_old'          => 'Тэрмін абароны прайшоў.',
'protect-unchain'             => 'Дазволіць пераносы',
'protect-text'                => 'Тут можна паглядзець і зьмяніць узровень абароны старонкі <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Вы ня можаце зьмяняць узровень абароны, пакуль Вы заблякаваны.
Цяперашнія ўстаноўкі для старонкі <strong>$1</strong>:',
'protect-locked-dblock'       => 'Узровень абароны ня можа быць зьменены, таму што база зьвестак заблякаваная.
Цяперашнія ўстаноўкі для старонкі <strong>$1</strong>:',
'protect-locked-access'       => 'Вы ня маеце правоў для зьмены ўзроўню абароны старонкі.
Цяперашнія наладкі старонкі <strong>$1</strong>:',
'protect-cascadeon'           => 'Гэтая старонка часова абароненая, таму што яна ўключаная ў {{PLURAL:$1|наступную старонку, якая абароненая|наступныя старонкі, якія абароненыя|наступныя старонкі, якія абароненыя}} каскаднай абаронай.
Вы можаце зьмяніць узровень абароны, але гэта не паўплывае на каскадную абарону.',
'protect-default'             => '(па змоўчаньні)',
'protect-fallback'            => 'Патрэбны дазвол «$1»',
'protect-level-autoconfirmed' => 'Абараніць ад ананімных і новых удзельнікаў',
'protect-level-sysop'         => 'Толькі адміністратары',
'protect-summary-cascade'     => 'каскадная',
'protect-expiring'            => 'сканчаецца $1 (UTC)',
'protect-cascade'             => 'Абараняць старонкі, якія ўключаныя ў гэтую старонку (каскадная абарона)',
'protect-cantedit'            => 'Вы ня можаце зьмяніць узровень абароны гэтай старонкі, таму што Вы ня маеце дазволу на яе рэдагаваньне.',
'restriction-type'            => 'Дазвол:',
'restriction-level'           => 'Узровень абмежаваньня:',
'minimum-size'                => 'Мінімальны памер',
'maximum-size'                => 'Максымальны памер:',
'pagesize'                    => '(у байтах)',

# Restrictions (nouns)
'restriction-edit'   => 'Рэдагаваньне',
'restriction-move'   => 'Перанос',
'restriction-create' => 'Стварэньне',
'restriction-upload' => 'Загрузка',

# Restriction levels
'restriction-level-sysop'         => 'поўнасьцю абароненая',
'restriction-level-autoconfirmed' => 'часткова абароненая',
'restriction-level-all'           => 'усе ўзроўні',

# Undelete
'undelete'                     => 'Паказаць выдаленыя старонкі',
'undeletepage'                 => 'Прагляд і аднаўленьне выдаленых старонак',
'undeletepagetitle'            => "'''Ніжэй паданыя выдаленыя вэрсіі [[:$1|$1]]'''.",
'viewdeletedpage'              => 'Паказаць выдаленыя старонкі',
'undeletepagetext'             => 'Наступныя старонкі былі выдаленыя, але яны знаходзяцца ў архіве і могуць быць адноўленыя.
Архіў пэрыядычна чысьціцца.',
'undelete-fieldset-title'      => 'Аднавіць вэрсіі',
'undeleteextrahelp'            => "Для поўнага аднаўленьня старонкі, пакіньце ўсе адзнакі нявызначнымі і націсьніце '''''Аднавіць'''''.
Для частковага аднаўленьня, пазначце тыя вэрсіі старонкі якія трэба аднавіць і націсьніце '''''Аднавіць'''''. Націсьніце '''''Ачысьціць''''' для таго, каб зьняць усе адзнакі і ачысьціць поле камэнтара.",
'undeleterevisions'            => 'У архіве $1 {{PLURAL:$1|вэрсія|вэрсіі|вэрсій}}',
'undeletehistory'              => 'Калі Вы адновіце старонку, будуць адноўленыя ўсе яе вэрсіі разам з журналам падзеяў.
Калі пасьля выдаленьня была створаная новая старонка з такой жа назвай, адноўленыя вэрсіі зьявяцца ў гісторыі перад новымі вэрсіямі.',
'undeleterevdel'               => 'Аднаўленьня не адбудзецца, калі ў яго выніку будзе выдаленая апошняя вэрсія старонкі ці файла.
У гэтым выпадку, Вам трэба зьняць адзнакі, ці паказаць апошнія выдаленыя вэрсіі.',
'undeletehistorynoadmin'       => 'Гэтая старонка была выдаленая.
Прычына выдаленьня пададзена ніжэй, разам са зьвесткамі ўдзельніка, які рэдагаваў старонку перад выдаленьнем.
Тэкст выдаленай старонкі могуць глядзець толькі адміністратары.',
'undelete-revision'            => 'Выдаленая вэрсія $1 (ад $2) ўдзельніка $3:',
'undeleterevision-missing'     => 'Некарэктная ці неіснуючая вэрсія.
Верагодна Вы карысталіся няслушнай спасылкай, альбо, магчыма, вэрсія была выдаленая з архіву.',
'undelete-nodiff'              => 'Папярэдняя вэрсія ня знойдзеная.',
'undeletebtn'                  => 'Аднавіць',
'undeletelink'                 => 'аднавіць',
'undeletereset'                => 'Ачысьціць',
'undeletecomment'              => 'Камэнтар:',
'undeletedarticle'             => 'адноўленая «[[$1]]»',
'undeletedrevisions'           => '{{PLURAL:$1|адноўленая $1 вэрсія|адноўленыя $1 вэрсіі|адноўленыя $1 вэрсій}}',
'undeletedrevisions-files'     => 'адноўненыя $1 {{PLURAL:$1|вэрсія|вэрсіі|вэрсій}} і $2 {{PLURAL:$2|файл|файлы|файлаў}}',
'undeletedfiles'               => '{{PLURAL:$1|адноўлены $1 файл|адноўленыя $1 файлы|адноўленыя $1 файлаў}}',
'cannotundelete'               => 'Аднаўленьне не адбылося; нехта іншы мог пасьпець аднавіць старонку раней.',
'undeletedpage'                => "<big>'''Старонка $1 была адноўленая'''</big>

Глядзіце [[Special:Log/delete|журнал выдаленьняў]] для прагляду апошніх выдаленьняў і аднаўненьняў.",
'undelete-header'              => 'Глядзіце [[Special:Log/delete|журнал выдаленьняў]] для прагляду апошніх выдаленьняў.',
'undelete-search-box'          => 'Пошук выдаленых старонак',
'undelete-search-prefix'       => 'Паказаць старонкі, назвы якіх пачынаюцца з:',
'undelete-search-submit'       => 'Паказаць',
'undelete-no-results'          => 'У архіве выдаленьняў ня знойдзеныя адпаведныя старонкі.',
'undelete-filename-mismatch'   => 'Немагчыма аднавіць вэрсію файла з датай $1: несупадзеньне назвы файла',
'undelete-bad-store-key'       => 'Немагчыма аднавіць вэрсію файла з датай $1: файл адсутнічаў да выдаленьня.',
'undelete-cleanup-error'       => 'Памылка выдаленьня архіўнага файла «$1», які не выкарыстоўваўся.',
'undelete-missing-filearchive' => 'Немагчыма аднавіць файл з ідэнтыфікатарам $1, таму што ён адсутнічае ў базе зьвестак.
Магчыма ён быў ужо адноўлены.',
'undelete-error-short'         => 'Памылка аднаўленьня файла: $1',
'undelete-error-long'          => 'Пад час аднаўленьня файла адбыліся памылкі:

$1',

# Namespace form on various pages
'namespace'      => 'Прастора назваў:',
'invert'         => 'Адваротны выбар',
'blanknamespace' => 'Артыкул',

# Contributions
'contributions' => 'Унёсак',
'mycontris'     => 'Мой унёсак',
'contribsub2'   => 'Для $1 ($2)',
'nocontribs'    => 'Ня знойдзена зьменаў, якія адпавядаюць гэтым крытэрыям.',
'uctop'         => ' (апошняя)',
'month'         => 'Ад месяца (і раней):',
'year'          => 'Ад году (і раней):',

'sp-contributions-newbies'     => 'Паказаць унёсак толькі з новых рахункаў',
'sp-contributions-newbies-sub' => 'Унёсак пачынаючых',
'sp-contributions-blocklog'    => 'журнал блякаваньняў',
'sp-contributions-search'      => 'Пошук унёску',
'sp-contributions-username'    => 'IP-адрас альбо імя ўдзельніка/ўдзельніцы:',
'sp-contributions-submit'      => 'Шукаць',

# What links here
'whatlinkshere'            => 'Спасылкі на старонку',
'whatlinkshere-title'      => 'Старонкі, якія спасылаюцца на $1',
'whatlinkshere-page'       => 'Старонка:',
'linklistsub'              => '(Сьпіс спасылак)',
'linkshere'                => "Наступныя старонкі спасылаюцца на '''[[:$1]]''':",
'nolinkshere'              => "Ніводная старонка не спасылаецца на '''[[:$1]]'''.",
'nolinkshere-ns'           => "Ніводная старонка не спасылаецца на '''[[:$1]]''' з выбранай прасторы назваў.",
'isredirect'               => 'старонка-перанакіраваньне',
'istemplate'               => 'уключэньне',
'isimage'                  => 'спасылка з выяваў',
'whatlinkshere-prev'       => '{{PLURAL:$1|папярэдняя|папярэднія|папярэднія}} $1',
'whatlinkshere-next'       => '{{PLURAL:$1|наступная|наступныя|наступныя}} $1',
'whatlinkshere-links'      => '← спасылкі',
'whatlinkshere-hideredirs' => '$1 перанакіраваньні',
'whatlinkshere-hidetrans'  => '$1 уключэньні',
'whatlinkshere-hidelinks'  => '$1 спасылкі',
'whatlinkshere-hideimages' => '$1 спасылкі на выявы',
'whatlinkshere-filters'    => 'Фільтры',

# Block/unblock
'blockip'                         => 'Блякаваньне ўдзельніка ці ўдзельніцы',
'blockip-legend'                  => 'Заблякаваць удзельніка',
'blockiptext'                     => 'Ужывайце форму ніжэй, каб заблякаваць доступ для запісу з пэўнага IP-адрасу ці імя ўдзельніка. Гэта трэба рабіць толькі прадухіленьня вандалізму і згодна з [[{{MediaWiki:Policy-url}}|правіламі]]. Запоўніце ніжэй пэўную прычыну (напрыклад, пералічыце асобныя старонкі, на якіх былі парушэньні).',
'ipaddress'                       => 'IP-адрас:',
'ipadressorusername'              => 'IP-адрас альбо імя ўдзельніка/ўдзельніцы:',
'ipbexpiry'                       => 'Тэрмін:',
'ipbreason'                       => 'Прычына:',
'ipbreasonotherlist'              => 'Іншая прычына',
'ipbreason-dropdown'              => '* Агульныя прычыны блякаваньняў
** Даданьне ілжывай інфармацыі
** Выдаленьне зьместу старонак
** Спам
** Даданьне бязглузьдзіцы
** Абразы/пагрозы
** Злоўжываньне некалькімі рахункамі
** Недапушчальнае імя ўдзельніка',
'ipbanononly'                     => 'Блякаваць толькі ананімаў',
'ipbcreateaccount'                => 'Забараніць стварэньне рахункаў',
'ipbemailban'                     => 'Забараніць удзельніку дасылаць лісты па электроннай пошце',
'ipbenableautoblock'              => 'Аўтаматычна блякаваць апошні IP-адрас гэтага ўдзельніка, і ўсіх наступных IP-адрасоў зь якіх ён будзе спрабаваць рэдагаваць',
'ipbsubmit'                       => 'Заблякаваць удзельніка',
'ipbother'                        => 'Іншы тэрмін:',
'ipboptions'                      => '2 гадзіны:2 hours,1 дзень:1 day,3 дні:3 days,1 тыдзень:1 week,2 тыдні:2 weeks,1 месяц:1 month,3 месяцы:3 months,6 месяцаў:6 months,1 год:1 year,назаўсёды:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'іншы',
'ipbotherreason'                  => 'Іншая/дадатковая прычына:',
'ipbhidename'                     => 'Схаваць імя ўдзельніка ў журнале блякаваньняў, сьпісе заблякаваных і сьпісе ўдзельнікаў',
'ipbwatchuser'                    => 'Назіраць за старонкай удзельніка і яго размовамі',
'badipaddress'                    => 'Некарэктны IP-адрас',
'blockipsuccesssub'               => 'Блякаваньне пасьпяховае',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] быў заблякаваны/была заблякаваная.
<br />Глядзіце [[Special:IPBlockList|сьпіс заблякаваных IP-адрасоў]] дзеля перагляду блякаваньняў.',
'ipb-edit-dropdown'               => 'Рэдагаваць прычыны блякіровак',
'ipb-unblock-addr'                => 'Разблякаваць $1',
'ipb-unblock'                     => 'Разблякаваць рахунак ўдзельніка ці IP-адрас',
'ipb-blocklist-addr'              => 'Паказаць існуючыя блякаваньні для $1',
'ipb-blocklist'                   => 'Паказаць існуючыя блякаваньні',
'unblockip'                       => 'Разблякаваць удзельніка',
'unblockiptext'                   => 'Карыстайцеся формай паданай ніжэй для аднаўленьня магчымасьці рэдагаваньня для раней заблякаванага IP-адрасу альбо рахунку ўдзельніка.',
'ipusubmit'                       => 'Разблякаваць гэты IP-адрас',
'unblocked'                       => '[[User:$1|$1]] быў разблякаваны.',
'unblocked-id'                    => 'Блякаваньне $1 зьнятае',
'ipblocklist'                     => 'Заблякаваныя IP-адрасы і імёны удзельнікаў',
'ipblocklist-legend'              => 'Пошук заблякаванага ўдзельніка',
'ipblocklist-username'            => 'Імя ўдзельніка/ўдзельніцы альбо IP-адрас:',
'ipblocklist-submit'              => 'Шукаць',
'blocklistline'                   => '$1, $2 заблякаваў $3 ($4)',
'infiniteblock'                   => 'назаўсёды',
'expiringblock'                   => 'канчаецца $1',
'anononlyblock'                   => 'толькі ананімаў',
'noautoblockblock'                => 'аўтаматычнае блякаваньне адключанае',
'createaccountblock'              => 'стварэньне рахунку заблякаванае',
'emailblock'                      => 'дасылка лістоў забароненая',
'ipblocklist-empty'               => 'Сьпіс блякаваньняў пусты.',
'ipblocklist-no-results'          => 'Паданы IP-адрас альбо імя ўдзельніка не заблякаваныя.',
'blocklink'                       => 'заблякаваць',
'unblocklink'                     => 'разблякаваць',
'contribslink'                    => 'унёсак',
'autoblocker'                     => "Вы аўтаматычна заблякаваныя, таму што Ваш IP-адрас нядаўна ўжываўся «[[User:$1|$1]]». Прычына блякаваньня $1 наступная: «'''$2'''»",
'blocklogpage'                    => 'Журнал блякаваньняў',
'blocklogentry'                   => 'заблякаваны [[$1]] на тэрмін: $2 $3',
'blocklogtext'                    => 'Гэта журнал уліку блякаваньняў і разблякаваньняў удзельнікаў. Аўтаматычна заблякаваныя IP-адрасы не пазначаныя. Глядзіце [[Special:IPBlockList|сьпіс заблякаваных IP-адрасоў]], каб пабачыць дзейныя ў гэты момант блякаваньні.',
'unblocklogentry'                 => 'разблякаваны $1',
'block-log-flags-anononly'        => 'толькі ананімныя ўдзельнікі',
'block-log-flags-nocreate'        => 'стварэньне рахункаў забароненае',
'block-log-flags-noautoblock'     => 'аўтаматычнае блякаваньне адключанае',
'block-log-flags-noemail'         => 'электронная пошта заблякаваная',
'block-log-flags-angry-autoblock' => 'уключанае палепшанае аўтаблякаваньне',
'range_block_disabled'            => 'Адміністратарам забаронена блякаваць дыяпазоны.',
'ipb_expiry_invalid'              => 'Няслушны тэрмін блякаваньня.',
'ipb_expiry_temp'                 => 'Блякаваньні са схаваньнем імя ўдзельніка павінны быць бестэрміновымі.',
'ipb_already_blocked'             => '«$1» ужо заблякаваны',
'ipb_cant_unblock'                => 'Памылка: ID блякаваньня $1 ня знойдзена. Верагодна яно было ўжо зьнятае.',
'ipb_blocked_as_range'            => 'Памылка: IP-адрас $1 быў заблякаваны наўпрост і ня можа быць разблякаваны.
Тым ня менш, ён належыць да дыяпазону $2, які можа быць разблякаваны.',
'ip_range_invalid'                => 'Некарэктны дыяпазон IP-адрасоў.',
'blockme'                         => 'Заблякуйце мяне',
'proxyblocker'                    => 'Блякаваньне проксі',
'proxyblocker-disabled'           => 'Гэта функцыя выключаная.',
'proxyblockreason'                => "Ваш IP-адрас быў заблякаваны таму што ён належыць адкрытаму проксі.
Калі ласка, зьвяжыцеся з Вашым Інтэрнэт-правайдарам альбо са службай тэхнічнай падтрымкі і паведаміце ім пра гэтую сур'ёзную праблему бясьпекі.",
'proxyblocksuccess'               => 'Зроблена.',
'sorbsreason'                     => 'Ваш IP-адрас знаходзіцца ў сьпісе адкрытых проксі ў DNSBL, якім карыстаецца {{SITENAME}}.',
'sorbs_create_account_reason'     => 'Ваш IP-адрас знаходзіцца ў сьпісе адкрытых проксі ў DNSBL, якім карыстаецца {{SITENAME}}.
Вы ня зможаце стварыць рахунак',

# Developer tools
'lockdb'              => 'Заблякаваць базу зьвестак',
'unlockdb'            => 'Разблякаваць базу зьвестак',
'lockdbtext'          => 'Блякаваньне базы зьвестак прыпыніць для ўсіх удзельнікаў магчымасьць рэдагаваць старонкі, зьмяняць устаноўкі, рэдагаваць сьпісы назіраньня, і іншыя дзеяньні, якія патрабуюць доступ да базы зьвестак.
Калі ласка, пацьвердзіце, што гэта тое, што Вы зьбіраецеся зрабіць, і што Вы здыміце блякаваньне базы зьвестак, адразу пасьля сканчэньня абслугоўваньня.',
'unlockdbtext'        => 'Разблякаваньне базы зьвестак адновіць для ўсіх удзельнікаў магчымасьць рэдагаваць старонкі, зьмяняць устаноўкі, рэдагаваць сьпісы назіраньня, і іншыя дзеяньні, якія патрабуюць доступ да базы зьвестак. 
Калі ласка, пацьвердзіце, што гэта тое, што Вы зьбіраецеся зрабіць.',
'lockconfirm'         => 'Так, я сапраўды жадаю заблякаваць базу зьвестак.',
'unlockconfirm'       => 'Так, я сапраўды жадаю разблякаваць базу зьвестак.',
'lockbtn'             => 'Заблякаваць базу зьвестак',
'unlockbtn'           => 'Разблякаваць базу зьвестак',
'locknoconfirm'       => 'Вы не пазначылі пацьверджаньне.',
'lockdbsuccesssub'    => 'База зьвестак заблякаваная',
'unlockdbsuccesssub'  => 'База зьвестак разблякаваная',
'lockdbsuccesstext'   => 'База зьвестак была заблякаваная.<br />
Не забудзьцеся [[Special:UnlockDB|зьняць блякаваньне]] пасьля сканчэньня абслугоўваньня.',
'unlockdbsuccesstext' => 'База зьвестак была разблякаваная.',
'lockfilenotwritable' => 'Немагчыма запісаць у файл блякаваньняў базы зьвестак.
Блякаваньне ці разблякаваньне базы зьвестак патрабуе, каб вэб-сэрвэр меў дазвол на запіс у гэты файл.',
'databasenotlocked'   => 'База зьвестак не заблякаваная.',

# Move page
'move-page'               => 'Перанесьці $1',
'move-page-legend'        => 'Перанесьці старонку',
'movepagetext'            => "З дапамогай гэтай формы Вы можаце зьмяніць назву і гісторыю старонкі.
Старая назва будзе перанакіроўваць на новую.
Вы можаце аўтаматычна абнавіць перанакіраваньні на першапачатковую назву.
Калі вы адмовіцеся, упэўніцеся ў адсутнасьці [[Special:DoubleRedirects|падвойных]] ці [[Special:BrokenRedirects|няслушных перанакіраваньняў]].
Адказнасьць за дакладнасьць спасылак ляжыць на тым, хто перанёс старонку.

Заўважце, што старонка '''ня будзе''' перанесеная, калі пад новай назвай ужо існуе іншая старонка, за выключэньнем выпадкаў, калі яна пустая альбо зьяўляецца перанакіраваньнем і ня мае гісторыі рэдагаваньняў. Гэта азначае, што існуе магчымасьць адмяніць зьмену назвы, калі Вы памыліліся, але не магчыма выдаліць існую старонку.

'''ПАПЯРЭДЖАНЬНЕ!'''
Зьмена назвы можа прывесьці да нечаканых зьменаў папулярных старонак;
калі ласка, упэўніцеся, што Вы разумееце наступствы такіх зьменаў.",
'movepagetalktext'        => "Старонка абмеркаваньня будзе перанесеная разам з асноўнай старонкай, '''за выключэньнем:'''
* Не пустая старонка абмеркаваньня ўжо існуе пад новай назвай, альбо
* Вы не паставілі адзнаку ў полі ніжэй.

У такіх выпадках, Вы можаце перанесьці ці аб'яднаць старонку абмеркаваньня самастойна.",
'movearticle'             => 'Перанесьці старонку:',
'movenotallowed'          => 'Вы ня маеце дазволу на перанос старонак.',
'newtitle'                => 'Новая назва:',
'move-watch'              => 'Назіраць за гэтай старонкай',
'movepagebtn'             => 'Перанесьці старонку',
'pagemovedsub'            => 'Пасьпяховы перанос',
'movepage-moved'          => "<big>'''Старонка «$1» была перанесеная ў «$2»'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Старонка з такой назвай ужо існуе, альбо абраная Вамі назва недапушчальная. Калі ласка, абярыце іншую назву.',
'cantmove-titleprotected' => 'Немагчыма перанесьці старонку, таму што новая назва знаходзіцца ў сьпісе забароненых',
'talkexists'              => "'''Старонка пасьпяхова перанесеная, але не атрымалася перанесьці старонку абмеркаваньня, таму што старонка з такой назвай ужо існуе. Калі ласка, аб'яднайце тэксты самастойна.'''",
'movedto'                 => 'перанесеная ў',
'movetalk'                => 'Перанесьці таксама старонку абмеркаваньня.',
'move-subpages'           => 'Перанесьці ўсё падстаронкі, калі гэта магчыма',
'move-talk-subpages'      => 'Перанесьці ўсё падстаронкі старонкі абмеркаваньня, калі гэта магчыма',
'movepage-page-exists'    => 'Старонка $1 ужо існуе і ня можа быць перапісана аўтаматычна.',
'movepage-page-moved'     => 'Старонка $1 перанесеная ў $2.',
'movepage-page-unmoved'   => 'Старонка $1 ня можа быць перанесеная ў $2.',
'movepage-max-pages'      => '$1 {{PLURAL:$1|старонка была перанесеная|старонкі былі перанесеныя|старонак былі перанесеныя}} — гэта дазволены максымум, больш аўтаматычных пераносаў ня будзе.',
'1movedto2'               => '[[$1]] перанесеная ў [[$2]]',
'1movedto2_redir'         => '[[$1]] перанесеная ў [[$2]] з выдаленьнем перанакіраваньня',
'movelogpage'             => 'Журнал пераносаў',
'movelogpagetext'         => 'Ніжэй пададзены сьпіс перанесеных старонак.',
'movereason'              => 'Прычына:',
'revertmove'              => 'адкат',
'delete_and_move'         => 'Выдаліць і перанесьці',
'delete_and_move_text'    => '==Патрабуецца выдаленьне==
Мэтавая старонка «[[:$1]]» ужо існуе.
Ці жадаеце Вы яе выдаліць, каб вызваліць месца для пераносу?',
'delete_and_move_confirm' => 'Так, выдаліць старонку',
'delete_and_move_reason'  => 'Выдаленая, каб вызваліць месца для пераносу',
'selfmove'                => 'Крынічная і мэтавая назвы супадаюць;
немагчыма перанесьці старонку саму на сябе.',
'immobile_namespace'      => 'Крынічная ці мэтавая назва належыць да спэцыяльнага тыпу;
немагчыма перанесьці з гэтай ці ў гэтую прастору назваў.',
'imagenocrossnamespace'   => 'Нельга перанесьці файл у ня-файлавую прастору назваў',
'imagetypemismatch'       => 'Новае пашырэньне файла не адпавядае яго тыпу',
'imageinvalidfilename'    => 'Няслушная назва мэтавага файла',
'fix-double-redirects'    => 'Абнавіць усе перанакіраваньні на першапачатковую назву',

# Export
'export'            => 'Экспартаваць старонкі',
'exporttext'        => 'Вы можаце экспартаваць тэкст і гісторыю рэдагаваньняў асобнай старонкі ці набору старонак у XML-фармаце.
Гэта потым можа быць імпартаванае ў іншую вікі, якая выкарыстоўвае MediaWiki праз [[Special:Import|старонку імпартаваньня]].

Каб экспартаваць старонкі, увядзіце назвы старонак у тэкставае поле ніжэй, адну назву ў адзін радок, і абярыце жадаеце Вы цяперашнюю вэрсію разам з усімі папярэднімі вэрсіямі і гісторыяй рэдагаваньняў, ці толькі цяперашнюю вэрсію з інфармацыяй пра апошняе рэдагаваньне.

У апошнім выпадку Вы можаце таксама скарыстацца спасылкай, напрыклад, [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] для «[[{{MediaWiki:Mainpage}}]]».',
'exportcuronly'     => 'Экспартаваць толькі цяперашнюю вэрсію, бяз поўнай гісторыі',
'exportnohistory'   => "----
'''Заўвага:''' экспартаваньне поўнай гісторыі старонак з дапамогай гэтай формы было адключанае праз прычыны прадукцыйнасьці сыстэмы.",
'export-submit'     => 'Экспартаваць',
'export-addcattext' => 'Дадаць старонкі з катэгорыі:',
'export-addcat'     => 'Дадаць',
'export-download'   => 'Захаваць як файл',
'export-templates'  => 'Разам з шаблёнамі',

# Namespace 8 related
'allmessages'               => 'Сыстэмныя паведамленьні',
'allmessagesname'           => 'Назва',
'allmessagesdefault'        => 'Тэкст па змоўчаньні',
'allmessagescurrent'        => 'Цяперашні тэкст',
'allmessagestext'           => 'Сьпіс усіх сыстэмных паведамленьняў, якія існуюць у прасторы назваў MediaWiki.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:AllMessages''' не падтрымліваецца, таму што адключанае '''\$wgUseDatabaseMessages'''.",
'allmessagesfilter'         => 'Фільтар рэгулярных выразаў:',
'allmessagesmodified'       => 'Паказаць толькі зьмененыя',

# Thumbnails
'thumbnail-more'           => 'Павялічыць',
'filemissing'              => 'Файл адсутны',
'thumbnail_error'          => 'Памылка стварэньня мініятуры: $1',
'djvu_page_error'          => 'Старонка DjVu па-за прамежкам',
'djvu_no_xml'              => 'Немагчыма атрымаць XML для DjVu-файла',
'thumbnail_invalid_params' => 'Няслушныя парамэтры мініятуры',
'thumbnail_dest_directory' => 'Немагчыма стварыць мэтавую дырэкторыю',

# Special:Import
'import'                     => 'Імпартаваць старонкі',
'importinterwiki'            => 'Імпартаваньне зь іншых вікі',
'import-interwiki-text'      => 'Абярыце вікі і назву старонкі для імпартаваньня.
Даты зьменаў і імёны аўтараў будуць захаваныя.
Усе дзеяньні імпартаваньня паміж вікі запісваюцца ў [[Special:Log/import|журнал імпартаваньняў]].',
'import-interwiki-history'   => 'Капіяваць гісторыю старонкі цалкам',
'import-interwiki-submit'    => 'Імпартаваць',
'import-interwiki-namespace' => 'Перанесьці старонкі ў прастору назваў:',
'importtext'                 => 'Калі ласка, экспартуйце файл з крынічнай вікі з дапамогай [[Special:Export|прылады экспарту]].
Захавайце яго на свой дыск, а потым загрузіце сюды.',
'importstart'                => 'Імпартаваньне старонак…',
'import-revision-count'      => '$1 {{PLURAL:$1|вэрсія|вэрсіі|вэрсій}}',
'importnopages'              => 'Няма старонак для імпартаваньня.',
'importfailed'               => 'Немагчыма імпартаваць: $1',
'importunknownsource'        => 'Невядомы тып крыніцы імпарту',
'importcantopen'             => 'Немагчыма адкрыць файл імпарту',
'importbadinterwiki'         => 'Няслушная спасылка на іншую моўную вэрсію',
'importnotext'               => 'Тэкст адсутнічае',
'importsuccess'              => 'Імпартаваньне скончанае!',
'importhistoryconflict'      => 'Канфлікт вэрсіяў у гісторыі рэдагаваньняў (магчыма, гэтую старонку імпартавалі раней)',
'importnosources'            => 'Крыніцы імпарту паміж вікі не былі вызначаныя і наўпроставая загрузка гісторыі адключаная.',
'importnofile'               => 'Файл для імпартаваньня ня быў загружаны.',
'importuploaderrorsize'      => 'Не атрымалася загрузіць файл імпартаваньня.
Памер файла болей за дазволены для загрузкі.',
'importuploaderrorpartial'   => 'Не атрымалася загрузіць файл імпартаваньня.
Ён быў загружаны толькі часткова.',
'importuploaderrortemp'      => 'Не атрымалася загрузіць файл імпартаваньня.
Не стае часовай дырэкторыі.',
'import-parse-failure'       => 'Памылка разбору XML пры імпартаваньні',
'import-noarticle'           => 'Няма старонкі для імпартаваньня!',
'import-nonewrevisions'      => 'Усе вэрсіі былі імпартаваныя раней.',
'xml-error-string'           => '$1 у радку $2, пазыцыі $3 (байт $4): $5',
'import-upload'              => 'Загрузіць XML-зьвесткі',

# Import log
'importlogpage'                    => 'Журнал імпартаваньняў',
'importlogpagetext'                => 'Імпартаваньне адміністратарамі старонак з гісторыяй зьменаў зь іншых вікі.',
'import-logentry-upload'           => 'імпартавана [[$1]] праз загрузку файла',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|вэрсія|вэрсіі|вэрсій}}',
'import-logentry-interwiki'        => 'імпартавана зь іншай вікі $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|вэрсія|вэрсіі|вэрсій}} з $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Мая старонка ўдзельніка',
'tooltip-pt-anonuserpage'         => 'Старонка ўдзельніка для IP-адрасу, зь якога Вы рэдагуеце',
'tooltip-pt-mytalk'               => 'Мая старонка размоваў',
'tooltip-pt-anontalk'             => 'Старонка гутарак пра рэдагаваньні, зробленыя з гэтага IP-адрасу',
'tooltip-pt-preferences'          => 'Мае ўстаноўкі',
'tooltip-pt-watchlist'            => 'Сьпіс старонак, за зьменамі якіх Вы назіраеце',
'tooltip-pt-mycontris'            => 'Мой унёсак',
'tooltip-pt-login'                => 'Вас запрашаюць увайсьці, хаця гэта і неабавязкова.',
'tooltip-pt-anonlogin'            => 'Уваход у сыстэму вітаецца, хаця ён не абавязковы.',
'tooltip-pt-logout'               => 'Выйсьці',
'tooltip-ca-talk'                 => 'Абмеркаваньне зьместу старонкі',
'tooltip-ca-edit'                 => 'Вы можаце рэдагаваць гэтую старонку. Калі ласка, ужывайце кнопку прагляду перад захаваньнем.',
'tooltip-ca-addsection'           => 'Дадаць камэнтар да гэтага абмеркаваньня.',
'tooltip-ca-viewsource'           => 'Гэтая старонка абароненая. Але можна паглядзець яе крынічны тэкст.',
'tooltip-ca-history'              => 'Мінулыя вэрсіі гэтай старонкі.',
'tooltip-ca-protect'              => 'Абараніць гэтую старонку',
'tooltip-ca-delete'               => 'Выдаліць гэтую старонку',
'tooltip-ca-undelete'             => 'Аднавіць рэдагаваньні, зробленыя да выдаленьня гэтай старонкі',
'tooltip-ca-move'                 => 'Перанесьці гэтую старонку',
'tooltip-ca-watch'                => 'Дадаць гэтую старонку ў Ваш сьпіс назіраньня',
'tooltip-ca-unwatch'              => 'Выдаліць гэтую старонку з Вашага сьпісу назіраньня',
'tooltip-search'                  => 'Шукаць у {{GRAMMAR:месны|{{SITENAME}}}}',
'tooltip-search-go'               => 'Перайсьці да старонкі з гэтай назвай, калі старонка існуе',
'tooltip-search-fulltext'         => 'Шукаць гэты тэкст на старонках',
'tooltip-p-logo'                  => 'Галоўная старонка',
'tooltip-n-mainpage'              => 'Наведаць галоўную старонку',
'tooltip-n-portal'                => 'Пра праект, што Вы можаце зрабіць, дзе што знайсьці',
'tooltip-n-currentevents'         => 'Атрымаць інфармацыю пра актуальныя падзеі',
'tooltip-n-recentchanges'         => 'Сьпіс апошніх зьменаў у {{GRAMMAR:месны|{{SITENAME}}}}.',
'tooltip-n-randompage'            => 'Паказаць выпадковую старонку',
'tooltip-n-help'                  => 'Месца, каб пра ўсё даведацца.',
'tooltip-t-whatlinkshere'         => 'Сьпіс усіх старонак, якія спасылаюцца на гэтую',
'tooltip-t-recentchangeslinked'   => 'Апошнія зьмены ў старонках, на якія спасылаецца гэтая старонка',
'tooltip-feed-rss'                => 'RSS-стужка для гэтай старонкі',
'tooltip-feed-atom'               => 'Atom-стужка для гэтай старонкі',
'tooltip-t-contributions'         => 'Паказаць унёсак гэтага удзельніка/гэтай удзельніцы',
'tooltip-t-emailuser'             => 'Даслаць ліст гэтаму ўдзельніку/гэтай удзельніцы па электроннай пошце',
'tooltip-t-upload'                => 'Загрузіць файл',
'tooltip-t-specialpages'          => 'Сьпіс усіх спэцыяльных старонак',
'tooltip-t-print'                 => 'Вэрсія гэтай старонкі для друку',
'tooltip-t-permalink'             => 'Сталая спасылка на гэтую вэрсію старонкі',
'tooltip-ca-nstab-main'           => 'Паказаць зьмест старонкі',
'tooltip-ca-nstab-user'           => 'Паказаць старонку ўдзельніка',
'tooltip-ca-nstab-media'          => 'Паказаць старонку мэдыя-файла',
'tooltip-ca-nstab-special'        => 'Гэта спэцыяльная старонка, і Вы ня можаце яе рэдагаваць',
'tooltip-ca-nstab-project'        => 'Паказаць старонку праекту',
'tooltip-ca-nstab-image'          => 'Паказаць старонку файла',
'tooltip-ca-nstab-mediawiki'      => 'Паказаць сыстэмнае паведамленьне',
'tooltip-ca-nstab-template'       => 'Паказаць шаблён',
'tooltip-ca-nstab-help'           => 'Паказаць старонку дапамогі',
'tooltip-ca-nstab-category'       => 'Паказаць старонку катэгорыі',
'tooltip-minoredit'               => 'Пазначыць гэтую зьмену як дробную',
'tooltip-save'                    => 'Захаваць Вашы зьмены',
'tooltip-preview'                 => 'Праглядзець Вашы зьмены. Калі ласка, выкарыстоўвайце гэтую магчымасьць перад тым, як захаваць старонку!',
'tooltip-diff'                    => 'Паказаць зробленыя Вамі зьмены ў тэксьце.',
'tooltip-compareselectedversions' => 'Пабачыць розьніцу паміж дзьвюма абранымі вэрсіямі гэтай старонкі.',
'tooltip-watch'                   => 'Дадаць гэтую старонку ў Ваш сьпіс назіраньня',
'tooltip-recreate'                => 'Аднавіць старонку, ня гледзячы на тое, што яна была выдаленая',
'tooltip-upload'                  => 'Пачаць загрузку',

# Metadata
'nodublincore'      => 'Мэтазьвесткі Dublin Core RDF забароненыя на гэтым сэрвэры.',
'nocreativecommons' => 'Мэтазьвесткі Creative Commons RDF забароненыя на гэтым сэрвэры.',
'notacceptable'     => 'Вікі-сэрвэр ня можа адлюстраваць зьвесткі ў выглядзе, які здольны прачытаць Ваш браўзэр.',

# Attribution
'anonymous'        => 'Ананімныя ўдзельнікі і ўдзельніцы {{GRAMMAR:родны|{{SITENAME}}}}',
'siteuser'         => 'Удзельнік/удзельніца {{GRAMMAR:родны|{{SITENAME}}}} $1',
'lastmodifiedatby' => 'Гэтую старонку апошні раз рэдагаваў $3 $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'На аснове працы $1.',
'others'           => 'іншыя',
'siteusers'        => 'Удзельнік(і) {{grammar:родны|{{SITENAME}}}} $1',
'creditspage'      => 'Падзякі',
'nocredits'        => 'Няма інфармацыі пра аўтараў гэтай старонкі.',

# Spam protection
'spamprotectiontitle' => 'Фільтар для абароны ад спаму',
'spamprotectiontext'  => 'Старонка, якую Вы жадаеце захаваць, была заблякаваная спам-фільтрам.
Верагодна яна ўтрымлівае спасылку на вонкавы сайт.',
'spamprotectionmatch' => 'Спам-фільтар даслаў наступнае паведамленьне: $1',
'spambot_username'    => 'Чыстка спаму MediaWiki',
'spam_reverting'      => 'Адкат да апошняй вэрсіі без спасылак на $1',
'spam_blanking'       => 'Усе вэрсіі ўтрымліваюць спасылкі на $1, чыстка',

# Info page
'infosubtitle'   => 'Інфармацыя пра старонку',
'numedits'       => 'Колькасьць зьменаў (старонкі): $1',
'numtalkedits'   => 'Колькасьць зьменаў (старонкі абмеркаваньня): $1',
'numwatchers'    => 'Колькасьць назіральнікаў і назіральніц: $1',
'numauthors'     => 'Колькасьць розных аўтараў і аўтарак (старонкі): $1',
'numtalkauthors' => 'Колькасьць розных аўтараў і аўтарак (старонкі абмеркаваньня): $1',

# Math options
'mw_math_png'    => 'Заўсёды паказваць як PNG',
'mw_math_simple' => 'HTML у простых выпадках, інакш PNG',
'mw_math_html'   => 'HTML калі магчыма, інакш PNG',
'mw_math_source' => 'Пакідаць у выглядзе TeX (для тэкставых браўзэраў)',
'mw_math_modern' => 'Рэкамэндуецца для сучасных браўзэраў',
'mw_math_mathml' => 'MathML калі магчыма (экспэрымэнтальна)',

# Patrolling
'markaspatrolleddiff'                 => 'Пазначыць як «патруляваную»',
'markaspatrolledtext'                 => 'Пазначыць гэтую старонку як «патруляваную»',
'markedaspatrolled'                   => 'Пазначаная як «патруляваная»',
'markedaspatrolledtext'               => 'Абраная вэрсія пазначаная як «патруляваная»',
'rcpatroldisabled'                    => 'Патруляваньне апошніх зьменаў адключанае',
'rcpatroldisabledtext'                => 'Магчымасьць патруляваньня апошніх зьменаў цяпер адключаная.',
'markedaspatrollederror'              => 'Немагчыма пазначыць як «патруляваную»',
'markedaspatrollederrortext'          => 'Вы мусіце абраць вэрсію, каб пазначыць яе «патруляванай».',
'markedaspatrollederror-noautopatrol' => 'Вам не дазволена пазначаць Вашыя ўласныя зьмены як «патруляваныя».',

# Patrol log
'patrol-log-page'   => 'Журнал патруляваньняў',
'patrol-log-header' => 'Гэта журнал патруляваных вэрсіяў.',
'patrol-log-line'   => 'пазначана $1 з $2 патруляваных $3',
'patrol-log-auto'   => '(аўтаматычна)',

# Image deletion
'deletedrevision'                 => 'Выдаленая старая вэрсія $1',
'filedeleteerror-short'           => 'Памылка выдаленьня файла: $1',
'filedeleteerror-long'            => 'У часе выдаленьня файла ўзьніклі наступныя памылкі:

$1',
'filedelete-missing'              => 'Немагчыма выдаліць файл «$1», таму што ён не існуе.',
'filedelete-old-unregistered'     => 'Пазначанай вэрсіі файла «$1» няма ў базе зьвестак.',
'filedelete-current-unregistered' => 'Пазначанага файла «$1» няма ў базе зьвестак.',
'filedelete-archive-read-only'    => 'Архіўная дырэкторыя «$1» не даступная для запісу вэб-сэрвэра.',

# Browsing diffs
'previousdiff' => '← Папярэдняя зьмена',
'nextdiff'     => 'Наступная зьмена →',

# Media information
'mediawarning'         => "'''Папярэджаньне''': гэты файл можа ўтрымліваць зламысны код, выкананьне якога можа нашкодзіць вашай сыстэме.<hr />",
'imagemaxsize'         => 'Абмяжоўваць памер выяваў на старонках апісаньня да:',
'thumbsize'            => 'Памер паменшанай вэрсіі выявы:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|старонка|старонкі|старонак}}',
'file-info'            => '(памер файла: $1, тып MIME: $2)',
'file-info-size'       => '($1 × $2 {{PLURAL:$2|піксэль|піксэлі|піксэляў}}, памер файла: $3, тып MIME: $4)',
'file-nohires'         => '<small>Няма вэрсіі зь лепшым разрозьненьнем.</small>',
'svg-long-desc'        => '(SVG-файл, намінальна $1 × $2 {{PLURAL:$2|піксэль|піксэлі|піксэляў}}, памер файла: $3).',
'show-big-image'       => 'Найлепшае разрозьненьне',
'show-big-image-thumb' => '<small>Памер пры папярэднім праглядзе: $1 × $2 {{PLURAL:$2|піксэль|піксэлі|піксэляў}}</small>',

# Special:NewImages
'newimages'             => 'Галерэя новых файлаў',
'imagelisttext'         => "Ніжэй пададзены сьпіс з '''$1''' {{PLURAL:$1|файла|файлаў|файлаў}}, адсартаваных $2.",
'newimages-summary'     => 'Гэтая спэцыяльная старонка паказвае нядаўна загружаныя файлы.',
'showhidebots'          => '($1 робатаў)',
'noimages'              => 'Выявы адсутнічаюць.',
'ilsubmit'              => 'Шукаць',
'bydate'                => 'па даце',
'sp-newimages-showfrom' => 'Паказаць новыя файлы, загружаныя пазьней за $2, $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'seconds-abbrev' => 'с',
'minutes-abbrev' => 'хв',
'hours-abbrev'   => 'г',

# Bad image list
'bad_image_list' => 'Фармат наступны:

Разглядаюцца толькі элемэнты сьпісу (радкі, якія пачынаюцца з *). Першая спасылка ў радку мусіць быць спасылкай на кепскую выяву. Усе наступныя спасылкі ў тым жа радку будуць разглядацца як выключэньні, напрыклад, старонкі, дзе можа зьяўляцца выява.',

# Metadata
'metadata'          => 'Мэтазьвесткі',
'metadata-help'     => 'Гэты файл утрымлівае дадатковую інфармацыю, хутчэй за ўсё дададзеную зь лічбавай фотакамэры ці сканэра.
Калі файл рэдагаваўся пасьля стварэньня, то частка зьвестак можа не адпавядаць зьмененаму файлу.',
'metadata-expand'   => 'Паказаць падрабязнасьці',
'metadata-collapse' => 'Схаваць падрабязнасьці',
'metadata-fields'   => 'Палі мэтазьвестак EXIF гэтага сьпісу будуць адлюстраваныя на старонцы выявы, астатнія будуць схаваныя.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Шырыня',
'exif-imagelength'                 => 'Вышыня',
'exif-bitspersample'               => 'Глыбіня колеру',
'exif-compression'                 => 'Мэтад сьцісканьня',
'exif-photometricinterpretation'   => 'Колеравая мадэль',
'exif-orientation'                 => 'Арыентацыя кадра',
'exif-samplesperpixel'             => 'Колькасьць колеравых кампанэнтаў',
'exif-planarconfiguration'         => 'Прынцып арганізацыі зьвестак',
'exif-ycbcrsubsampling'            => 'Суадносіны памераў кампанэнтаў Y і C',
'exif-ycbcrpositioning'            => 'Парадак разьмяшчэньня кампанэнтаў Y і C',
'exif-xresolution'                 => 'Гарызантальнае разрозьненьне',
'exif-yresolution'                 => 'Вэртыкальнае разрозьненьне',
'exif-resolutionunit'              => 'Адзінка вымярэньня разрозьненьняў X і Y',
'exif-stripoffsets'                => 'Разьмяшчэньне блёку зьвестак',
'exif-rowsperstrip'                => 'Колькасьць радкоў у блёку',
'exif-stripbytecounts'             => 'Памер сьціснутага блёку',
'exif-jpeginterchangeformat'       => 'Разьмяшчэньне пачатку блёку JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Байты JPEG-зьвестак',
'exif-transferfunction'            => 'Функцыя пераўтварэньня колеравай прасторы',
'exif-whitepoint'                  => 'Колернасьць белага пункту',
'exif-primarychromaticities'       => 'Колернасьць асноўных колераў',
'exif-ycbcrcoefficients'           => 'Каэфіцыенты пераўтварэньня колеравай прасторы',
'exif-referenceblackwhite'         => 'Становішча белага і чорнага пунктаў',
'exif-datetime'                    => 'Дата і час зьмены файла',
'exif-imagedescription'            => 'Назва выявы',
'exif-make'                        => 'Вытворца фотаапарата',
'exif-model'                       => 'Мадэль фотаапарата',
'exif-software'                    => 'Праграмнае забесьпячэньне',
'exif-artist'                      => 'Аўтар',
'exif-copyright'                   => 'Уласьнік аўтарскіх правоў',
'exif-exifversion'                 => 'Вэрсія Exif',
'exif-flashpixversion'             => 'Вэрсія Flashpix, якая падтрымліваецца',
'exif-colorspace'                  => 'Колеравая прастора',
'exif-componentsconfiguration'     => 'Канфігурацыя колеравых кампанэнтаў',
'exif-compressedbitsperpixel'      => 'Глыбіня колеру пасьля сьцісканьня',
'exif-pixelydimension'             => 'Дапушчальная шырыня выявы',
'exif-pixelxdimension'             => 'Дапушчальная вышыня выявы',
'exif-makernote'                   => 'Заўвагі вытворцы',
'exif-usercomment'                 => 'Камэнтары карыстальніка',
'exif-relatedsoundfile'            => 'Датычны аўдыё-файл',
'exif-datetimeoriginal'            => 'Дата і час утварэньня зьвестак',
'exif-datetimedigitized'           => 'Дата і час лічбаваньня',
'exif-subsectime'                  => 'Доля сэкундаў часу зьмяненьня файла',
'exif-subsectimeoriginal'          => 'Долі сэкундаў арыгінальнага часу',
'exif-subsectimedigitized'         => 'Долі сэкундаў часу лічбаваньня',
'exif-exposuretime'                => 'Час насьвятленьня',
'exif-exposuretime-format'         => '$1 сэк ($2)',
'exif-fnumber'                     => 'Лік дыяфрагмы',
'exif-exposureprogram'             => 'Праграма насьвятленьня',
'exif-spectralsensitivity'         => 'Спэктральная адчувальнасьць',
'exif-isospeedratings'             => 'Сьветаадчувальнасьць ISO',
'exif-oecf'                        => 'Каэфіцыент оптаэлектроннага ператварэньня',
'exif-shutterspeedvalue'           => 'Вытрымка',
'exif-aperturevalue'               => 'Дыяфрагма',
'exif-brightnessvalue'             => 'Яркасьць',
'exif-exposurebiasvalue'           => 'Кампэнсацыя насьвятленьня',
'exif-maxaperturevalue'            => 'Мінімальны лік дыяфрагмы',
'exif-subjectdistance'             => "Адлегласьць да аб'екту",
'exif-meteringmode'                => 'Лад вымярэньня насьвятленьня',
'exif-lightsource'                 => 'Крыніца сьвятла',
'exif-flash'                       => 'Успышка',
'exif-focallength'                 => "Фокусная адлегласьць аб'ектыва",
'exif-focallength-format'          => '$1 мм',
'exif-subjectarea'                 => "Плошча аб'екту",
'exif-flashenergy'                 => 'Энэргія бліску',
'exif-spatialfrequencyresponse'    => 'Прасторавая частотная характарыстыка',
'exif-focalplanexresolution'       => 'Разрозьненьне ў фокуснай плоскасьці па восі X',
'exif-focalplaneyresolution'       => 'Разрозьненьне ў фокуснай плоскасьці па восі Y',
'exif-focalplaneresolutionunit'    => 'Адзінка разрозьненьня фокуснай плоскасьці',
'exif-subjectlocation'             => "Месцазнаходжаньне аб'екту",
'exif-exposureindex'               => 'Індэкс насьвятленьня',
'exif-sensingmethod'               => 'Тып сэнсару',
'exif-filesource'                  => 'Крыніца файла',
'exif-scenetype'                   => 'Тып сцэны',
'exif-cfapattern'                  => 'Тып колеравага фільтру',
'exif-customrendered'              => 'Дадатковая апрацоўка',
'exif-exposuremode'                => 'Лад насьвятленьня',
'exif-whitebalance'                => 'Балянс белага',
'exif-digitalzoomratio'            => 'Каэфіцыент лічбавага павелічэньня',
'exif-focallengthin35mmfilm'       => 'Эквівалентная фокусная адлегласьць (для 35 мм стужкі)',
'exif-scenecapturetype'            => 'Тып сцэны пры здымцы',
'exif-gaincontrol'                 => 'Падвышэньне яркасьці',
'exif-contrast'                    => 'Кантраст',
'exif-saturation'                  => 'Насычанасьць',
'exif-sharpness'                   => 'Рэзкасьць',
'exif-devicesettingdescription'    => 'Апісаньне ўстановак прыстасаваньня',
'exif-subjectdistancerange'        => "Адлегласьць да аб'екту",
'exif-imageuniqueid'               => 'Унікальны ідэнтыфікатар выявы',
'exif-gpsversionid'                => 'Вэрсія блёку GPS-інфармацыі',
'exif-gpslatituderef'              => 'Паўночная ці паўднёвая шырата',
'exif-gpslatitude'                 => 'Шырата',
'exif-gpslongituderef'             => 'Усходняя ці заходняя даўгата',
'exif-gpslongitude'                => 'Даўгата',
'exif-gpsaltituderef'              => 'Пункт адліку вышыні',
'exif-gpsaltitude'                 => 'Вышыня',
'exif-gpstimestamp'                => 'Час GPS (атамны гадзіньнік)',
'exif-gpssatellites'               => 'Спадарожнікі, ужытыя для вымярэньня',
'exif-gpsstatus'                   => 'Статус прымача',
'exif-gpsmeasuremode'              => 'Лад вымярэньня',
'exif-gpsdop'                      => 'Дакладнасьць вымярэньня',
'exif-gpsspeedref'                 => 'Адзінка вымярэньня хуткасьці',
'exif-gpsspeed'                    => 'Хуткасьць руху GPS-прымача',
'exif-gpstrackref'                 => 'Тып азімуту GPS-прымача',
'exif-gpstrack'                    => 'Кірунак руху',
'exif-gpsimgdirectionref'          => 'Тып азімуту выявы',
'exif-gpsimgdirection'             => 'Кірунак выявы',
'exif-gpsmapdatum'                 => 'Скарыстаная геадэзічная сыстэма каардынатаў',
'exif-gpsdestlatituderef'          => "Пункт адліку шыраты аб'екту",
'exif-gpsdestlatitude'             => "Шырата аб'екту",
'exif-gpsdestlongituderef'         => "Пункт адліку даўгаты аб'екту",
'exif-gpsdestlongitude'            => "Даўгата аб'екту",
'exif-gpsdestbearingref'           => "Тып пэленгу аб'екту",
'exif-gpsdestbearing'              => "Пэленг аб'екту",
'exif-gpsdestdistanceref'          => "Пункт адліку адлегласьці да аб'екту",
'exif-gpsdestdistance'             => "Адлегласьць да аб'екту",
'exif-gpsprocessingmethod'         => 'Назва мэтаду вылічэньня GPS',
'exif-gpsareainformation'          => 'Назва GPS-зоны',
'exif-gpsdatestamp'                => 'Дата GPS',
'exif-gpsdifferential'             => 'Дыфэрэнцыяльная папраўка GPS',

# EXIF attributes
'exif-compression-1' => 'Нясьціснуты',

'exif-unknowndate' => 'Невядомая дата',

'exif-orientation-1' => 'Звычайная', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Адлюстраваная па гарызанталі', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Павернутая на 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Адлюстраваная па вэртыкалі', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Павернутая на 90° супраць руху гадзіньнікавай стрэлкі і адлюстраваная вэртыкальна', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Павернутая на 90° па руху гадзіньнікавай стрэлкі', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Павернутая на 90° па руху гадзіньнікавай стрэлкі і адлюстраваная вэртыкальна', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Павернутая на 90° супраць руху гадзіньнікавай стрэлкі', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'фармат «chunky»',
'exif-planarconfiguration-2' => 'фармат «planar»',

'exif-componentsconfiguration-0' => 'не існуе',

'exif-exposureprogram-0' => 'Ня вызначана',
'exif-exposureprogram-1' => 'Ручны лад',
'exif-exposureprogram-2' => 'Праграмны лад',
'exif-exposureprogram-3' => 'Прыярытэт дыяфрагмы',
'exif-exposureprogram-4' => 'Прыярытэт затвора',
'exif-exposureprogram-5' => 'Творчы лад (перавага глыбіні рэзкасьці)',
'exif-exposureprogram-6' => 'Лад руху (перавага хуткасьці затвора)',
'exif-exposureprogram-7' => 'Партрэтны лад (здымкі ў набліжэньня, з фонам ня ў фокусе)',
'exif-exposureprogram-8' => 'Пэйзажны лад (для здымкаў краявідаў, з фонам у фокусе)',

'exif-subjectdistance-value' => '$1 мэтры',

'exif-meteringmode-0'   => 'Невядома',
'exif-meteringmode-1'   => 'Сярэдні',
'exif-meteringmode-2'   => 'Сярэдне-ўзважаны',
'exif-meteringmode-3'   => 'Кропкавы',
'exif-meteringmode-4'   => 'Шматкропкавы',
'exif-meteringmode-6'   => 'Частковы',
'exif-meteringmode-255' => 'Іншы',

'exif-lightsource-0'   => 'Невядомая',
'exif-lightsource-1'   => 'Дзённае сьвятло',
'exif-lightsource-4'   => 'Успышка',
'exif-lightsource-11'  => 'Цень',
'exif-lightsource-255' => 'Іншая крыніца сьвятла',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'паўночнай шыраты',
'exif-gpslatitude-s' => 'паўднёвай шыраты',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'усходняй даўгаты',
'exif-gpslongitude-w' => 'заходняй даўгаты',

# External editor support
'edit-externally'      => 'Рэдагаваць гэты файл з выкарыстаньнем вонкавай праграмы',
'edit-externally-help' => 'Глядзіце падрабязнасьці ў [http://www.mediawiki.org/wiki/Manual:External_editors інструкцыі па наладцы] (па-ангельску).',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'усе',
'imagelistall'     => 'усе',
'watchlistall2'    => 'усе',
'namespacesall'    => 'усе',
'monthsall'        => 'усе',

# E-mail address confirmation
'confirmemail'            => 'Пацьвердзіць адрас электроннай пошты',
'confirmemail_noemail'    => 'Вы не пазначылі слушны адрас электроннай пошты ў Вашых [[Special:Preferences|устаноўках удзельніка]].',
'confirmemail_text'       => '{{SITENAME}} патрабуе, каб Вы пацьвердзілі Ваш адрас электроннай пошты перад ўжываньнем мажлівасьцей электроннай пошты. Актывізуйце кнопку ніжэй, каб даслаць ліст з пацьверджаньнем на Ваш адрас. Ліст будзе ўтрымліваць спасылку з кодам; загрузіце спасылку ў Вашым браўзэры, каб пацьвердзіць, што Ваш адрас электроннай пошты зьяўляецца слушным.',
'confirmemail_pending'    => '<div class="error">
Код пацьверджаньня ужо быў дасланы Вам праз электронную пошту; калі Вы нядаўна стварылі рахунак, Вам варта пачакаць код некалькі хвілінаў перад спробай запыту новага коду.
</div>',
'confirmemail_send'       => 'Даслаць код пацьверджаньня',
'confirmemail_sent'       => 'Ліст для пацьверджаньня дасланы.',
'confirmemail_oncreate'   => 'Код пацьверджаньня быў адпраўлены на Ваш адрас электроннай пошты. Гэты код не патрэбны, каб увайсьці, але Вам трэба будзе падаць яго перад ужываньнем мажлівасьцей, заснаваных на электроннай пошце, у {{GRAMMAR:месны|{{SITENAME}}}}.',
'confirmemail_sendfailed' => '{{SITENAME}} ня можа даслаць пацьвярджальны ліст. Праверце адрас на наяўнасьць няслушных сымбаляў.

Адказ паштовага сэрвэра: $1',
'confirmemail_invalid'    => 'Няслушны код пацьверджаньня. Магчыма, скончыўся яго тэрмін дзеяньня.',
'confirmemail_needlogin'  => 'Вам трэба $1, каб пацьвердзіць Ваш адрас электроннай пошты.',
'confirmemail_success'    => 'Ваш адрас электроннай пошты быў пацьверджаны. Зараз Вы можаце ўвайсьці.',
'confirmemail_loggedin'   => 'Ваш адрас электроннай пошты быў пацьверджаны.',
'confirmemail_error'      => 'Нешта пайшло ня так у часе захаваньня вашага пацьверджаньня.',
'confirmemail_subject'    => '{{SITENAME}}: Пацьверджаньне адрасу электроннай пошты',
'confirmemail_body'       => 'Нехта, магчыма Вы, з IP-адрасу $1, стварыў рахунак «$2» з гэтым паштовым адрасам у праекце {{SITENAME}}.

Каб пацьвердзіць, што гэты рахунак належыць Вам актывізаваць магчымасьці электроннай пошты ў {{GRAMMAR:месны|{{SITENAME}}}}, адчыніце гэтую спасылку у Вашым браўзэры:

$3

Калі гэта ня Вы, адчыніце наступную спасылку для таго, каб не пацьвердзіць адрас электроннай пошты.

$5

Код дзейнічае да $4.',

# Delete conflict
'confirmrecreate' => "Удзельнік [[User:$1|$1]] ([[User talk:$1|гутаркі]]) выдаліў гэтую старонку, перад тым як Вы пачалі яе рэдагаваць, з прычыны:
: ''$2''
Калі ласка, пацьвердзіце, што Вы сапраўды жадаеце стварыць нанава гэтую старонку.",
'recreate'        => 'Стварыць ізноў',

'unit-pixel' => 'пкс',

# action=purge
'confirm_purge'        => 'Ачысьціць кэш гэтай старонкі?

$1',
'confirm_purge_button' => 'Так',

# AJAX search
'articletitles' => "Старонкі, якія пачынаюцца з ''$1''",
'hideresults'   => 'Схаваць вынікі',

# Multipage image navigation
'imgmultipageprev' => '← папярэдняя старонка',
'imgmultipagenext' => 'наступная старонка →',
'imgmultigoto'     => 'Перайсьці на старонку $1',

# Table pager
'table_pager_next'  => 'Наступная старонка',
'table_pager_prev'  => 'Папярэдняя старонка',
'table_pager_first' => 'Першая старонка',
'table_pager_last'  => 'Апошняя старонка',

# Auto-summaries
'autosumm-blank'   => 'Зьмест старонкі выдалены цалкам',
'autosumm-replace' => "Старонка замененая на '$1'",
'autoredircomment' => 'Перанакіроўвае на [[$1]]',
'autosumm-new'     => 'Новая старонка: $1',

# Size units
'size-bytes'     => '$1 б',
'size-kilobytes' => '$1 кб',
'size-megabytes' => '$1 Мб',
'size-gigabytes' => '$1 Гб',

# Live preview
'livepreview-loading' => 'Загрузка…',
'livepreview-ready'   => 'Загрузка… Гатова!',

# Watchlist editor
'watchlistedit-normal-title' => 'Рэдагаваць сьпіс назіраньня',
'watchlistedit-raw-titles'   => 'Назвы:',

# Watchlist editing tools
'watchlisttools-view' => 'Паказаць зьмены ў старонках зь сьпісу',
'watchlisttools-edit' => 'Праглядзець альбо рэдагаваць сьпіс назіраньня',
'watchlisttools-raw'  => 'Рэдагаваць як тэкст',

# Core parser functions
'unknown_extension_tag' => 'Невядомы тэг пашырэньня «$1»',

# Special:Version
'version'                  => 'Вэрсія', # Not used as normal message but as header for the special page itself
'version-extensions'       => 'Усталяваныя пашырэньні',
'version-specialpages'     => 'Спэцыяльныя старонкі',
'version-other'            => 'Іншыя',
'version-version'          => 'Вэрсія',
'version-license'          => 'Ліцэнзія',
'version-software-version' => 'Вэрсія',

# Special:FilePath
'filepath'        => 'Шлях да файла',
'filepath-page'   => 'Файл:',
'filepath-submit' => 'Шлях',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Пошук дублікатаў файлаў',
'fileduplicatesearch-filename' => 'Назва файла:',
'fileduplicatesearch-submit'   => 'Шукаць',
'fileduplicatesearch-result-1' => 'Файл «$1» ня мае ідэнтычных копій.',
'fileduplicatesearch-result-n' => 'Файл «$1» мае $2 {{PLURAL:$2|ідэнтычную копію|ідэнтычных копіі|ідэнтычных копій}}.',

# Special:SpecialPages
'specialpages'                 => 'Спэцыяльныя старонкі',
'specialpages-group-other'     => 'Іншыя спэцыяльныя старонкі',
'specialpages-group-changes'   => 'Апошнія зьмены і журналы падзей',
'specialpages-group-media'     => 'Справаздачы пра мэдыя-файлы і загрузка',
'specialpages-group-users'     => 'Удзельнікі і правы',
'specialpages-group-pages'     => 'Сьпісы старонак',
'specialpages-group-pagetools' => 'Інструмэнты для старонак',
'specialpages-group-spam'      => 'Інструмэнты для барацьбы са спамам',

# Special:BlankPage
'blankpage'              => 'Пустая старонка',
'intentionallyblankpage' => 'Гэтая старонка наўмысна пакінутая пустой',

);
