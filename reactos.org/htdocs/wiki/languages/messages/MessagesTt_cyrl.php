<?php
/** Tatar (Cyrillic) (Tatarça/Татарча (Cyrillic))
 *
 * @ingroup Language
 * @file
 *
 * @author Himiq Dzyu
 * @author Ерней
 */

$fallback = 'ru';

$messages = array(
# User preference toggles
'tog-underline'               => 'Сылтамаларны астына сызу:',
'tog-highlightbroken'         => 'Төзелмәгән сылтамаларны <a href="" class="new">шулай</a> (юкса алай <a href="" class="internal">?</a>) күрсәтү.',
'tog-justify'                 => 'Текстны киңлек буенча тигезләү',
'tog-hideminor'               => 'Соңгы үзгәртмәләрдә әһәмиятсезләрне яшерү',
'tog-extendwatchlist'         => 'Күзәтү зурайган исемлеге, бөтен үзгәртмәләр белән',
'tog-usenewrc'                => 'Баягы үзгәртмәләр яхшыртырган исемлеге (JavaScript)',
'tog-numberheadings'          => 'Башисемләрне автономерлау',
'tog-showtoolbar'             => 'Үзгәртү кораллыгы өстендә күрсәтү (JavaScript)',
'tog-editondblclick'          => 'Ике чирттермә белән битләрне үзгәртү (JavaScript)',
'tog-editsection'             => 'Бүлекнең [үзгәртү] сылтамасын күрсәтү',
'tog-editsectiononrightclick' => 'Бүлекне башисемендә уң чирттермә белән үзгәртү (JavaScript)',
'tog-showtoc'                 => 'Эчтәлекне күрсәтү (3-тән күбрәк башисемле битләрдә)',
'tog-rememberpassword'        => 'Хисап язмамны бу компьютердә хәтерләү',
'tog-editwidth'               => 'Үзгәртү урыны күзәтүче тәрәзәсенең тулы киңлеккә зураю',
'tog-watchcreations'          => 'Төзгән битләремне күзәтү исемлегемгә өстәү',
'tog-watchdefault'            => 'Үзгәртергән битләремне күзәтү исемлегемгә өстәү',
'tog-watchmoves'              => 'Күчерергән битләремне күзәтү исемлегемгә өстәү',
'tog-watchdeletion'           => 'Бетерергән битләремне күзәтү исемлегемгә өстәү',
'tog-minordefault'            => 'Барлык үзгәртүләрне килешү буенча әһәмиятсез дип билгеләү',
'tog-previewontop'            => 'Мәкаләнең алдан карауны үзгәртү тәрәзәсенән өскәрәк күрсәтү',
'tog-previewonfirst'          => 'Беренче үзгәртү буенча алдан карау',
'tog-nocache'                 => 'Битләр кешләүне туктату',
'tog-enotifwatchlistpages'    => 'Күзәтелгән битем үзгәртсә электрон почта адресымга хәбәр җибәрү',
'tog-enotifusertalkpages'     => 'Фикер алышу битем үзгәртсә электрон почта аша хәбәр җибәрү',
'tog-enotifminoredits'        => "Битләрдә үзгәртүләр әһәмиятсез булса да, e-mail'ыма хәбәр җибәрү",
'tog-enotifrevealaddr'        => 'Хәбәрләрдә минем e-mail адресым күрсәтү',
'tog-shownumberswatching'     => 'Күзәтче кулланучылар исәбене күрсәтү',
'tog-fancysig'                => 'Имза үз вики-билгеләмәсе (автоматик сылтамасыз)',
'tog-externaleditor'          => 'Гадәттә тышкы үзгәртүче куллану',
'tog-externaldiff'            => 'Гадәттә тышкы версия чагыштыру программасы куллану',
'tog-showjumplinks'           => '«таба йөрергә» сылтамалары эшләтә башлау',
'tog-uselivepreview'          => 'Тиз карап алу куллану (JavaScript, эксперименталь)',
'tog-forceeditsummary'        => 'Үзгәртүләр тасвиры юк булсада кисәтү',
'tog-watchlisthideown'        => 'Күзәтү исемлегедә үзгәртүләремне яшерү',
'tog-watchlisthidebots'       => 'Күзәтү исемлегедә бот үзгәртүләрене яшерергә',
'tog-watchlisthideminor'      => 'Күзәтү исемлегедә әһәмиятсез үзгәртүләрене яшерергә',
'tog-nolangconversion'        => 'Язу системаларының үзгәртүен сүндерү',
'tog-ccmeonemails'            => 'Мин башка кулланучыларга күндерә торган хатлар копияләре миңа да күндерергә.',
'tog-diffonly'                => 'Версия чагыштыру астында бит текстын күрсәтмәскә',
'tog-showhiddencats'          => 'Яшерен төркемнәрне күрсәтергә',

'underline-always'  => 'Һәрвакыт',
'underline-never'   => 'Һичкайчан',
'underline-default' => 'Күзәтүче көйләнмәләрне кулланырга',

'skinpreview' => '(Алдан карау)',

# Dates
'sunday'        => 'якшәмбе',
'monday'        => 'дүшәмбе',
'tuesday'       => 'сишәмбе',
'wednesday'     => 'чәршәмбе',
'thursday'      => 'пәнҗешәмбе',
'friday'        => 'җомга',
'saturday'      => 'шимбә',
'sun'           => 'якш',
'mon'           => 'дүш',
'tue'           => 'сиш',
'wed'           => 'чәр',
'thu'           => 'пән',
'fri'           => 'җом',
'sat'           => 'шим',
'january'       => 'гыйнвар',
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
'january-gen'   => 'гыйнвар',
'february-gen'  => 'февраль',
'march-gen'     => 'март',
'april-gen'     => 'апрель',
'may-gen'       => 'май',
'june-gen'      => 'июнь',
'july-gen'      => 'июль',
'august-gen'    => 'август',
'september-gen' => 'сентябрь',
'october-gen'   => 'октябрь',
'november-gen'  => 'ноябрь',
'december-gen'  => 'декабрь',
'jan'           => 'гый',
'feb'           => 'фев',
'mar'           => 'мар',
'apr'           => 'апр',
'may'           => 'май',
'jun'           => 'июн',
'jul'           => 'июл',
'aug'           => 'авг',
'sep'           => 'сен',
'oct'           => 'окт',
'nov'           => 'ноя',
'dec'           => 'дек',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Төркем|Төркемнәр}}',
'category_header'                => '«$1» төркемендәге битләр',
'subcategories'                  => 'Төркемчәләр',
'category-media-header'          => '«$1» төркемендәге файллар',
'category-empty'                 => "''Бу төркем әле буш.''",
'hidden-categories'              => '{{PLURAL:$1|Яшерен төркем|Яшерен төркемнәр}}',
'hidden-category-category'       => 'Яшерен төркемнәр', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Бу төркемдә киләчәк төркемчә генә бар|$2 төркемчәдән {{PLURAL:$1|$1 төркемчә күрсәтелгән}}.}}',
'category-subcat-count-limited'  => 'Бу төркемдә {{PLURAL:$1|$1 төркемчә}} бар.',
'category-article-count'         => '{{PLURAL:$2|Бу төркемдә бер бит кенә бар.|Бу төркемнең $2 төркеменнән {{PLURAL:$1|$1 бите күрсәтелгән}}.}}',
'category-article-count-limited' => 'Бу төркемдә {{PLURAL:$1|$1 бит}} бар.',
'category-file-count'            => '{{PLURAL:$2|Бу төркемдә бер файл гына бар.|Бу төркемнең $2 файлыннан {{PLURAL:$1|$1 файлы күрсәтелгән}}.}}',
'category-file-count-limited'    => 'Агымдагы төркемдә {{PLURAL:$1|$1 файл}} бар.',
'listingcontinuesabbrev'         => 'дәвам',

'mainpagetext'      => '<big>«MediaWiki» уңышлы куелган.</big>',
'mainpagedocfooter' => "Бу вики турында [http://meta.wikimedia.org/wiki/Ярдәм:Эчтәлек кулланмада] табып була.

== Файдалы ресурслар ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Көйләнмәләр исемлеге];
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki турында еш бирелгән сораулар һәм җаваплар];
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki'нең яңа версияләре турында хәбәрләр җибәреп чыгу].",

'about'          => 'Тасвир',
'article'        => 'Эчтәлек бите',
'newwindow'      => '(яңа тәрәзәдә ачыла)',
'cancel'         => 'Үткәрмәү',
'qbfind'         => 'Эзләү',
'qbbrowse'       => 'Күзәтү',
'qbedit'         => 'Үзгәртү',
'qbpageoptions'  => 'Бу бит',
'qbpageinfo'     => 'Бит турындагы мәгълүматлар',
'qbmyoptions'    => 'Битләрем',
'qbspecialpages' => 'Махсус битләр',
'moredotdotdot'  => 'Дәвам…',
'mypage'         => 'Минем битем',
'mytalk'         => 'Фикер алышу битем',
'anontalk'       => 'Бу IP-адрес өчен фикер алышу',
'navigation'     => 'Навигация',
'and'            => 'һәм',

# Metadata in edit box
'metadata_help' => 'Мета-мәгълүматлар:',

'errorpagetitle'    => 'Хата',
'returnto'          => '$1 битенә кайту.',
'tagline'           => "{{SITENAME}}'дән",
'help'              => 'Ярдәм',
'search'            => 'Эзләү',
'searchbutton'      => 'Эзләү',
'go'                => 'Күчү',
'searcharticle'     => 'Күчү',
'history'           => 'Битнең тарихы',
'history_short'     => 'Тарихы',
'updatedmarker'     => 'соңгы керүемнән соң яңартырган',
'info_short'        => 'Мәгълүмат',
'printableversion'  => 'Бастыру версиясе',
'permalink'         => 'Даими сылтама',
'print'             => 'Бастыру',
'edit'              => 'Үзгәртү',
'create'            => 'Төзү',
'editthispage'      => 'Бу битне үзгәртү',
'create-this-page'  => 'Бу битне төзү',
'delete'            => 'Бетерү',
'deletethispage'    => 'Бу битне бетерү',
'undelete_short'    => '$1 үзгәртмә торгызу',
'protect'           => 'Яклау',
'protect_change'    => 'яклауны үзгәртү',
'protectthispage'   => 'Бу битне яклау',
'unprotect'         => 'Яклауны бетерү',
'unprotectthispage' => 'Бу битнең яклауын бетерү',
'newpage'           => 'Яңа бит',
'talkpage'          => 'Бу битне фикер алышу',
'talkpagelinktext'  => 'Фикер алышу',
'specialpage'       => 'Махсус бит',
'personaltools'     => 'Шәхси кораллар',
'postcomment'       => 'Шәрехләү',
'articlepage'       => 'Битне карарга',
'talk'              => 'Фикер алышу',
'views'             => 'Караулар',
'toolbox'           => 'Кораллар җыелмасы',
'userpage'          => 'Кулланучы битен карарга',
'projectpage'       => 'Проект битен карарга',
'imagepage'         => 'Рәсем битен карарга',
'mediawikipage'     => 'Хәбәр битен карарга',
'templatepage'      => 'Үрнәкнең битен карарга',
'viewhelppage'      => 'Ярдәм битен карарга',
'categorypage'      => 'Төркем битен карарга',
'viewtalkpage'      => 'Фикер алышуны карарга',
'otherlanguages'    => 'Башка телләрендә',
'redirectedfrom'    => '($1 битенән юнәлтелгән)',
'redirectpagesub'   => 'Юнәлтү бите',
'lastmodifiedat'    => 'Бу битне соңгы үзгәртмә: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Бу биткә $1 {{PLURAL:$1|тапкыр}} мөрәҗәгать иттеләр.',
'protectedpage'     => 'Якланган бит',
'jumpto'            => 'Күчү:',
'jumptonavigation'  => 'навигация',
'jumptosearch'      => 'эзләү',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{GRAMMAR:genitive|{{SITENAME}}}} турында',
'aboutpage'            => 'Project:Тасвир',
'bugreports'           => 'Хата хисаплары',
'bugreportspage'       => 'Project:Хата хисаплары',
'copyright'            => 'Мәглүмат үтәрлек моның буенча: $1.',
'copyrightpagename'    => '{{SITENAME}} проектының авторлык хокукы',
'copyrightpage'        => '{{ns:project}}:Авторлык хокуклары',
'currentevents'        => 'Хәзерге вакыйгалар',
'currentevents-url'    => 'Project:Хәзерге вакыйгалар',
'disclaimers'          => 'Җаваплылыктан баш тарту',
'disclaimerpage'       => 'Project:Җаваплылыктан баш тарту',
'edithelp'             => 'Үзгәртү ярдәме',
'edithelppage'         => 'Help:Үзгәртү',
'faq'                  => 'ЕБС (FAQ)',
'faqpage'              => 'Project:ЕБС',
'helppage'             => 'Help:Эчтәлек',
'mainpage'             => 'Төп бит',
'mainpage-description' => 'Төп бит',
'policy-url'           => 'Project:Сәясәт',
'portal'               => 'Җәмгыять үзәге',
'portal-url'           => 'Project:Җәмгыять үзәге',
'privacy'              => 'Яшеренлек сәясәте',
'privacypage'          => 'Project:Яшеренлек сәясәте',

'badaccess'        => 'Рөхсәт хатасы',
'badaccess-group0' => 'Сез сораган гамәлне башкара алмыйсыз.',
'badaccess-group1' => 'Соралган гамәл $1 төркеменең кулланучылары гына өчен.',
'badaccess-group2' => 'Соралган гамәл $1 төркемләренең кулланучылары гына өчен.',
'badaccess-groups' => 'Соралган гамәл $1 төркемләренең кулланучылары гына өчен.',

'versionrequired'     => 'MediaWiki версия $1 кирәк',
'versionrequiredtext' => "Бу бит белән эшләү өчен MediaWiki'нең $1 версиясе кирәк. [[Special:Version|Версия битен]] кара.",

'ok'                      => 'Ярар',
'pagetitle'               => '$1 — {{SITENAME}}',
'retrievedfrom'           => 'Чыганагы — «$1»',
'youhavenewmessages'      => 'Сездә $1 ($2) бар.',
'newmessageslink'         => 'яңа хәбәрләр',
'newmessagesdifflink'     => 'соңгы үзгәртмә',
'youhavenewmessagesmulti' => 'Сезнең $1 да яңа хәбәрләр бар',
'editsection'             => 'үзгәртү',
'editold'                 => 'үзгәртү',
'viewsourceold'           => 'баштагы текст карау',
'editsectionhint'         => '$1 бүлеген үзгәртү',
'toc'                     => 'Эчтәлек',
'showtoc'                 => 'күрсәтү',
'hidetoc'                 => 'яшерү',
'thisisdeleted'           => '$1 карыйсыгызмы яки торгызасыгызмы килә?',
'viewdeleted'             => '$1 карыйсыгызмы килә?',
'restorelink'             => '{{PLURAL:$1|$1 бетерелгән төзәтмә}}',
'feedlinks'               => 'Төр буенча:',
'site-rss-feed'           => '$1 — RSS тасмасы',
'site-atom-feed'          => '$1 — Atom тасмасы',
'page-rss-feed'           => '«$1» — RSS тасмасы',
'page-atom-feed'          => '«$1» — Atom тасмасы',
'red-link-title'          => '$1 (әле язылмаган)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Бит',
'nstab-user'      => 'Кулланучы бите',
'nstab-media'     => 'Мультимедиа',
'nstab-special'   => 'Махсус',
'nstab-project'   => 'Проект бите',
'nstab-image'     => 'Файл',
'nstab-mediawiki' => 'Хәбәр',
'nstab-template'  => 'Өлге',
'nstab-help'      => 'Белешмә бите',
'nstab-category'  => 'Төркем',

# Main script and global functions
'nosuchaction'      => 'Шулай гамәл юк',
'nosuchactiontext'  => 'Вики программ тәэминаты URL-га күрсәтелгән хәрәкәтне аңламый.',
'nosuchspecialpage' => 'Андый махсус бит юк',
'nospecialpagetext' => "<big>'''Сез сорый торган махсус бит юк.'''</big>

[[Special:SpecialPages|Махсус битләр исемлеге]] кара.",

# General errors
'error'                => 'Хата',
'databaseerror'        => 'Мәгълүматлар базасы хатасы',
'dberrortext'          => 'Мәгълүматлар базасы соравының синтаксик хатасы табылган.
Бәлки, программада бер хата бар.
Мәгълүматлар базасына соңгы сорау:
<blockquote><tt>$1</tt></blockquote>
<tt>«$2»</tt> функциядән.
MySQL <tt>«$3: $4»</tt> хатаны күрсәткән.',
'dberrortextcl'        => 'Мәгълүматлар базасы соравының синтаксик хатасы табылган.
Мәгълүматлар базасына соңгы сорау:
<blockquote><tt>$1</tt></blockquote>
<tt>«$2»</tt> функциядән.
MySQL <tt>«$3: $4»</tt> хатаны күрсәткән.',
'noconnect'            => 'Гафу итегез, хәзер викинең техник авырлыклары бар һәм мәгълүматлар базасының серверы белән тоташып булмый.<br />
$1',
'nodb'                 => '$1 мәгълүмат базасын сайлап булмый.',
'cachederror'          => 'Аста соралган битнең кештә сакланган копиясе күрсәтелгән; бәлки, ул искергән.',
'laggedslavemode'      => 'Игътибар: бәлки, биттә соңгы яңартмалары юк.',
'readonly'             => 'Мәгълүматлар базасына язу йомылган',
'enterlockreason'      => 'Йому сәбәбен һәм мөддәтен күрсәтегез.',
'readonlytext'         => 'Мәгълүмат базасы хәзер яңа бит ясамадан да башка ялмаштырмалардан йомылган. Бәлки, бу нормаль хезмәт күрсәтү өчен ителгән.
Йомучы бу аңлатманы язган:
$1',
'missing-article'      => 'Мәгълүматлар базасында «$1» битенең соралган тексты $2 табылмаган.

Бу гадәттә беткән битнең тарихынагы сылтама ачу белән ясала.

Әгәр хата моның белән түгел ясалса, сез, бәлки, программ хатасы тапкансыз.
Зинһар, URL күрсәтеп, моның турында администраторга хәбәр итегез.',
'missingarticle-rev'   => '(версия № $1)',
'missingarticle-diff'  => '(аерма: $1, $2)',
'readonly_lag'         => 'Мәгълүматлар базасы, МБ өстәмә серверы төп серверы белән синхронизацияләшкәнче, автоматик үзгәрүдән ябык.',
'internalerror'        => 'Эчке хата',
'internalerror_info'   => 'Эчке хата: $1',
'filecopyerror'        => '«$2» файлгагы «$1» файлның копиясен ясап булмый.',
'filerenameerror'      => '«$1» файлга «$2» исемне биреп булмый.',
'filedeleteerror'      => '«$1» файлны бетерә алмаган.',
'directorycreateerror' => '«$1» директорияне тудыра булмый.',
'filenotfound'         => '«$1» файлны таба алмаган.',
'fileexistserror'      => '«$1» файлга язып булмый: файл инде була.',
'unexpected'           => 'Көтелмәгән әһәмият: «$1»=«$2».',
'formerror'            => 'Хата: форма мәгълүматларын тапшырып булмый',
'badarticleerror'      => 'Бу биттә андый гамәл итеп булмый.',
'cannotdelete'         => 'Бу битне яки файлны бетереп булмый. Бәлки, ул инде бетерелгән.',
'badtitle'             => 'Яраксыз башлам',
'badtitletext'         => 'Битнең соралган исеме дөрес түгел я юк, яки телара я интервики исем дөрес түгел күрсәтелгән. Бәлки, исемдә ярамаган символлар куллыйлар.',
'perfdisabled'         => 'Гафу итегез, бу мөмкинлек сервер мәшгульлеге аркасында кулланып булмый.',
'perfcached'           => 'Бу мәгълүматлар кештән бирелгәннәр һәм, бәлки, аларда соңгы үзгәртмәләр юк.',
'perfcachedts'         => 'Бу мәгълүматлар кештән бирелгәннәр, ул соңгы тапкырда $1 яңарды.',
'querypage-no-updates' => 'Хәзер бу битне үзгәртеп булмый. Бу мәгълүматлар хәзер яңармаслар.',
'wrong_wfQuery_params' => 'Ярамаган параметрлар wfQuery() функция өчен<br />
Функция: $1<br />
Сорау: $2',
'viewsource'           => 'Чыганак карарга',
'viewsourcefor'        => 'Бит «$1»',
'actionthrottled'      => 'Гамәл кысылган',
'actionthrottledtext'  => 'Спам белән көрәш өчен аз вакыт эчендә еш бу гамәл куллану кысылган. Зинһар, соңгырак кабатлыйгыз.',
'protectedpagetext'    => 'Бу битне үзгәртеп булмый.',
'viewsourcetext'       => 'Сез бу битнең башлангыч текстны карый һәм күчермә аласыз:',
'protectedinterface'   => 'Бу биттә программа интерфейс хәбәре бар. Вандализмга каршы, бу битне үзгәртеп булмый.',
'editinginterface'     => "'''Игътибар:''' Сез MediaWiki системасы хәбәре беләнге битне үзгәртәсез. Бу башка кулланучылар интерфейсын үзгәртер. Сезнең тәрҗемә итәсегез килсә, зинһар, [http://translatewiki.net/wiki/Main_Page?setlang=tt-cyrl Betawiki] кулланыгыз.",
'sqlhidden'            => '(SQL соравы яшерелгән)',
'cascadeprotected'     => 'Бу бит үзгәртүдән сакланган, чөнки ул андый "каскад" сакланган {{PLURAL:$1|биткә|битләргә}} өстәлгән:
$2',
'namespaceprotected'   => "'''$1''' исем киңлегендәге битләрне үзгәртү өчен сезнең рөхсәтегез юк.",
'customcssjsprotected' => 'Сез бу битне үзгәртә алмыйсыз, чөнки анда башка кулланычының көйләнмәләре бар.',
'ns-specialprotected'  => 'Махсус битләрне үзгәртеп булмый.',
'titleprotected'       => "Бу исем белән битне тудыру [[Кулланучы:$1|$1]] белән тыелган.
Андый сәбәп күрсәтелгән: ''$2''.",

# Login and logout pages
'logouttitle'                => 'Чыгарга',
'logouttext'                 => '<strong>Сез хәзер чыккансыз.</strong>

Сез проект аноним кебек куллана аласыз, яки Сез шул кулланучы да я башка кулланучы кебек керә аласыз.
Ниндидер битләр һәм хәбәрләр, Сез кермәгәнсез, дип кебек күрсәтеләләр, әгәр сез кешегез бушалып калдырмасагыз.',
'welcomecreation'            => '== Рәхим итегез, $1! ==
Сез теркәлгәнсез.
Сайт персональ көйләргә онытмагыз.',
'loginpagetitle'             => 'Кулланучының теркәү исеме',
'yourname'                   => 'Кулланучы исеме:',
'yourpassword'               => 'Серсүз:',
'yourpasswordagain'          => 'Серсүзне кабат кертү:',
'remembermypassword'         => 'Теркәү исемемне бу компьютердә онытмаска',
'yourdomainname'             => 'Сезнең доменыгыз:',
'loginproblem'               => '<b>Керүегездә авырлык бар.</b><br />Кабатлагызчы!',
'login'                      => 'Керү',
'nav-login-createaccount'    => 'Керү / хисап язмасы төзү',
'userlogin'                  => 'Керү / хисап язмасы төзү',
'logout'                     => 'Чыгу',
'userlogout'                 => 'Чыгу',
'notloggedin'                => 'Кермәгәнсез',
'nologin'                    => 'Теркәмәгәнсез? $1.',
'nologinlink'                => 'Хисап язмасыны төзәргә',
'createaccount'              => 'Хисап язмасыны төзәргә',
'gotaccount'                 => 'Сездә инде хисап язмасы бармы? $1.',
'gotaccountlink'             => 'Керергә',
'createaccountmail'          => 'электрон почта белән',
'youremail'                  => 'Электрон почта:',
'username'                   => 'Теркәү исеме:',
'uid'                        => 'Кулланучының идентификаторы:',
'yourrealname'               => 'Чын исем:',
'yourlanguage'               => 'Тел:',
'yournick'                   => 'Имза өчен тахалус:',
'badsig'                     => 'Имза дөрес түгел. HTML-теглар тикшерегез.',
'badsiglength'               => 'Имза өчен тахалус бигрәк озын.
Ул $1 {{PLURAL:$1|хәрефтән}} күбрәк түгел булырга тиеш.',
'email'                      => 'Электрон почта',
'prefs-help-realname'        => 'Чын исемегез (кирәкми): аны күрсәтсәгез, ул битне үзгәртүче күрсәтү өчен файдалаячак.',
'loginerror'                 => 'Керү хатасы',
'prefs-help-email-required'  => 'Электрон почта адресы кирәк.',
'loginsuccesstitle'          => 'Керү уңышлы үтте',
'loginsuccess'               => "'''Сез {{SITENAME}} проектына $1 исеме белән кергәнсез.'''",
'nosuchuser'                 => '$1 исемле кулланучы барлыкта юк.<br />
Язылышыгызны тикшерегез яки яңа хисап язмасыны төзегез.',
'nosuchusershort'            => "Кулланучы '''<nowiki>$1</nowiki>''' ирешү исеме белән юк. Исем язу тикшерегез.",
'nouserspecified'            => 'Сез теркәү исмегезне күрсәтергә тиешсез.',
'wrongpassword'              => 'Язылган серсүз дөрес түгел. Тагын бер тапкыр сынагыз.',
'wrongpasswordempty'         => 'Буш түгел серсүзне кертегез әле.',
'passwordtooshort'           => 'Язылган серсүз начар яки ифрат кыска. Сезсүз $1 хәрефтән булырга һәм кулланучы исеменнән аерылырга тиеш.',
'mailmypassword'             => 'Серсүзне электрон почтага җибәрергә',
'passwordremindertitle'      => '{{SITENAME}} кулланучысына яңа вакытлы серсүз',
'passwordremindertext'       => 'Кемдер (сез, бәлки) $1 IP-адрестан,
без сезгә {{SITENAME}} ($4) кулланучысы яңа серсүзе күндерербез, дип сораган.
Кулланучы $2 өчен хәзер: <code>$3</code>.
Сез системага керергә һәм серсүзне алмаштырырга тиеш.

Әгәр сез серсүзне алмаштыру сорамаса идегез яки серсүз хәтерләсәгез,
сез бу хәбәр игътибарсыз калдыра һәм иске серсүзне куллану дәвам итә аласыз.',
'noemail'                    => '$1 кулланучы өчен электрон почта адресы язылмаган.',
'passwordsent'               => 'Яңа серсүз электрон почта $1 катнашучы өчен күрсәтелгән адресына җибәрелгән.

Зинһар, серсүзне алгач системага яңадан керегез.',
'blocked-mailpassword'       => 'Сезнең IP-адресыгыз белән үзгәртеп һәм серсүзне яңартып булмый.',
'eauthentsent'               => 'Адресны үзгәртмә дөресләүенең хаты электрон почта күрсәтелгән адресына җибәрелгән.
Бу ардес хуҗалыгының дөресләүе тәэсирләре хатта ясылганнар.',
'throttled-mailpassword'     => 'Серсүзне искәртмә соңгы $1 сәгать дәвамында инде күндерелде. Начар куллану булдырмау өчен $1 сәгать дәвамында бер тапкырдан күбрәк түгел сорап була.',
'mailerror'                  => 'Хат җибәрү хатасы: $1',
'acct_creation_throttle_hit' => 'Гафу итегез, сез $1 хисап язмагызны төзгәнсез инде.
Күбрәк төзи алмыйсыз.',
'emailauthenticated'         => 'Почта адресыгыз дөресләнгән $1.',
'emailnotauthenticated'      => 'Электрон почта адресыгыз әле дөресләнмәгән, викинең электрон почта белән эшләре сүндерелгәннәр.',
'noemailprefs'               => 'Электрон почта адресыгыз күрсәтелмәгән, викинең электрон почта белән эшләре сүндерелгән.',
'emailconfirmlink'           => 'Электрон почта адресыгызны дөресләгез.',
'invalidemailaddress'        => 'Күрсәтелгән адрес дөрес түгел, чөнки бу электрон почта адресы форматы түгел. Зинһар, дөрес адресны күрсәтегез яки адрес урынын буш калдырыгыз.',
'accountcreated'             => 'Хисап язмасы төзелгән',
'accountcreatedtext'         => '$1 кулланучы өчен хисап язмасы төзелгән.',
'createaccount-title'        => '{{SITENAME}}: хисап язмасы төзү',
'createaccount-text'         => 'Кемдер, электрон почта адресыгыз күрсәтеп, {{SITENAME}} ($4) проектының серверында «$3» серсүзе белән хисап язмасы «$2» төзегән. Сез керергә һәм серсүзегезне үзгәртергә тиеш.

Хисап язмасы төзү хата булса, бу хат онытыгыз.',
'loginlanguagelabel'         => 'Тел: $1',

# Password reset dialog
'resetpass'               => 'Хисап язмасының серсүзе бетерү',
'resetpass_announce'      => 'Сез электрон почта белән вакытлы бирелгән серсүз белән кергәнсез. Системага керү төгәлләп, сез яңа серсүз төзергә тиеш.',
'resetpass_text'          => '<!-- Монда текст өстәгез -->',
'resetpass_header'        => 'Серсүзне ташлатырга',
'resetpass_submit'        => 'Серсүз төзү дә керү',
'resetpass_bad_temporary' => 'Вакытлы серсүз дөрес түгел. Бәлки, сез инде серсүзне алмаштырган идегез, яки тагын бер тапкыр вакытлы серсүз сорарга сынагыз.',
'resetpass_forbidden'     => 'Бу вики-системада серсүзләрне алмаштырып булмый.',

# Edit page toolbar
'bold_sample'     => 'Калын язылышы',
'bold_tip'        => 'Калын язылышы',
'italic_sample'   => 'Курсив язылышы',
'italic_tip'      => 'Курсив язылышы',
'link_sample'     => 'Сылтаманың башламы',
'link_tip'        => 'Эчке сылтама',
'extlink_sample'  => 'http://www.example.com сылтаманың башламы',
'extlink_tip'     => 'Тышкы сылтама (http:// алкушымчасы турында онытмагыз)',
'headline_sample' => 'Башисем тексты',
'headline_tip'    => '2-нче дәрәҗәле башисем',
'math_sample'     => 'Формуланы монда өстәгез',
'math_tip'        => 'Математика формуласы (LaTeX)',
'nowiki_sample'   => 'Форматланмаган текстны монда өстәгез',
'nowiki_tip'      => 'Вики-форматлауны исәпкә алмаска',
'image_tip'       => 'Куелган рәсем',
'media_tip'       => 'Медиа-файлга сылтама',
'sig_tip'         => 'Имзагыз да вакыт',
'hr_tip'          => 'Горизонталь сызык (еш кулланмагыз)',

# Edit pages
'summary'                => 'Үзгәртүләр тасвиры',
'subject'                => 'Тема/башисем',
'minoredit'              => 'Бу әһәмиятсез үзгәртү',
'watchthis'              => 'Бу битне күзәтергә',
'savearticle'            => 'Битне саклау',
'preview'                => 'Алдан карау',
'showpreview'            => 'Алдан карау',
'showlivepreview'        => 'Тиз алдан карау',
'showdiff'               => 'Үзгәртүләрне күрсәтү',
'anoneditwarning'        => "'''Игътибар''': Сез системага кермәгәнсез. IP-адресыгыз бу битнең тарихына язылыр.",
'missingsummary'         => "'''Искәртү.''' Сез үзгәртмә кыска язу бирмәгәнсез. Сез «Битне саклау» кнопкасына тагын бер тапкыр бассагыз, үзгәртмәгез комментсыз сакланыр.",
'missingcommenttext'     => 'Зинһар, аска комментыгыз языгыз.',
'missingcommentheader'   => "'''Искәртү:''' Сез комментыгызның башын күрсәтмәгәнсез.
Сез «Битне саклау» кнопкасына бассагыз, үзгәртмәгез башсыз язылыр.",
'summary-preview'        => 'Җыелма нәтиҗәне алдан карау',
'subject-preview'        => 'Башисемне алдан карау',
'blockedtitle'           => 'Кулланучы кыстырган',
'blockednoreason'        => 'сәбәп күрсәтмәгән',
'blockedoriginalsource'  => 'Бит «$1» тексты аска күрсәткән.',
'blockededitsource'      => "Бит «$1» '''үзгәртүегез''' тексты аска күрсәткән.",
'whitelistedittitle'     => 'Үзгәртү өчен керү кирәк',
'whitelistedittext'      => 'Сез битләрне үзгәртү өчен $1 тиеш.',
'nosuchsectiontitle'     => 'Андый секция юк',
'loginreqtitle'          => 'Керү кирәк',
'loginreqlink'           => 'керү',
'loginreqpagetext'       => 'Сез башка битләр карау өчен $1 тиеш',
'accmailtitle'           => 'Серсүз җибәрелгән.',
'accmailtext'            => '$1 өченге серсүз $2 кулланучыга күндерелгән.',
'newarticle'             => '(Яңа)',
'newarticletext'         => "Сез әле язылмаган биткәге сылтама куллангансыз.
Яңа бит ясау өчен аскагы тәрәзәдә мәкалә тексты языгыз
([[{{MediaWiki:Helppage}}|ярдәм бите]] к. күбрәк информация алу өчен).
Әгәр сез бу бит ялгышлык белән ачса идегез, гади браузерыгызның '''артка''' кнопкасына басыгыз.",
'anontalkpagetext'       => "----''Бу хисапланмаган да хисапланган исем белән кергән кулланучы фикер алышу бите.
Аны билгеләү өчен IP-адрес файдалый.
Әгәр сез аноним кулланучы һәм сез, сезгә күндерелмәгән хәбәрләр алдыгыз, дип саныйсыз (бер IP-адрес күп кулланучы өчен була ала), зинһар, [[Special:UserLogin|системага керегез]], киләчәктә аңлашмау теләмәсәгез.''",
'noarticletext'          => "Хәзер бу биттә текст юк. Сез [[Special:Search/{{PAGENAME}}|аның башы башка мәкаләләрдә таба]] яки '''[{{fullurl:{{FULLPAGENAME}}|action=edit}} андый баш белән бит ясый]''' аласыз.",
'clearyourcache'         => "'''Искәрмә:''' Битне саклаудан соң төзәтмәләр күрү өчен күзәтүчегезнең кэшын буш итегез.
'''Mozilla / Firefox''': ''Ctrl+Shift+R'', '''Safari''': ''Cmd+Shift+R'', '''IE:''' ''Ctrl+F5'', '''Konqueror''': ''F5'', '''Opera''': ''Tools→Preferences'' сайлагында.",
'previewnote'            => '<strong>Бу фәкать алдан карау, төзәтмәләр әле сакланмаган!</strong>',
'editing'                => 'Төзәтү: $1',
'editingsection'         => '$1 үзгәртүе (бүлек)',
'yourtext'               => 'Сезнең текст',
'storedversion'          => 'Сакланган юрама',
'yourdiff'               => 'Аермалыклар',
'copyrightwarning'       => 'Бөтен өстәмә һәм үзгәртмә $2 (к. $1) лицензиянең шартлары буенча уйлана, дип игътибар итегезче.
Если вы не хотите, чтобы һәркем аларны ирекле үзгәртә вә тарата, дип теләмәсәгез, анда алар язмагыз.<br />
Сез дә, бу өстәлмә үзем яздыгыз яке ирекле үзгәрүче чыганакның копиясе иттегез, дип сүз берәсез.<br />
<strong>РӨХСӘТСЕЗ АВТОР ХОКУКЫ САКЛАНУЧЫ МАТЕРИАЛЛАРНЫ ЯЗМАГЫЗ!</strong>',
'longpageerror'          => '<strong>ХАТА: языла торган текстта $1 килобайт бар, бу $2 килобайт чигеннән күбрәк. Бит саклана алмый.</strong>',
'templatesused'          => 'Бу биттә кулланган өлгеләр:',
'templatesusedpreview'   => 'Бу алдан карау биттә кулланган өлгеләр:',
'templatesusedsection'   => 'Бу бүлектә кулланган өлгеләр:',
'template-protected'     => '(якланган)',
'template-semiprotected' => '(өлешчә якланган)',
'nocreatetitle'          => 'Битләр төзүе чикләнгән',

# Account creation failure
'cantcreateaccounttitle' => 'Хисап язмасыны төзергә мөмкинлек юк',

# History pages
'viewpagelogs'        => 'Бу бит өчен журналларны карарга',
'currentrev'          => 'Агымдагы юрама',
'revisionasof'        => 'Юрама $1',
'revision-info'       => 'Юрама: $1; $2',
'previousrevision'    => '← Алдагы төзәтмәләр',
'nextrevision'        => 'Чираттагы төзәтмәләр →',
'currentrevisionlink' => 'Агымдагы юрама',
'cur'                 => 'агым.',
'last'                => 'бая.',
'page_first'          => 'беренче',
'page_last'           => 'соңгы',
'histfirst'           => 'Баштагы',
'histlast'            => 'Баягы',

# Revision deletion
'rev-deleted-comment' => '(искәрмә бетергән)',
'rev-deleted-user'    => '(авторның исеме бетерергән)',
'rev-deleted-event'   => '(язма бетерергән)',
'rev-delundel'        => 'күрсәтергә/яшерергә',

# Diffs
'history-title'           => '$1 — төзәтү тарихы',
'difference'              => '(Төзәтмәләр арасында аермалар)',
'lineno'                  => '$1 юл:',
'compareselectedversions' => 'Сайланган юрамаларны чагыштырырга',
'editundo'                => 'үткәрмәү',

# Search results
'searchresults'       => 'Эзләү нәтиҗәләре',
'noexactmatch'        => "'''«$1» атлы битне әле юк.'''
Аны [[:$1|төзергә]] мөмкин.",
'prevn'               => 'алдагы $1',
'nextn'               => 'чираттагы $1',
'viewprevnext'        => '($1) ($2) ($3) карарга',
'search-result-size'  => '$1 ({{PLURAL:$2|$2 сүз}})',
'search-redirect'     => '($1 җибәрүлеге)',
'search-section'      => '($1 бүлеге)',
'searchall'           => 'барлык',
'showingresultstotal' => "Астарак '''$3''' данәдән '''$1—$2''' нәтиҗәләре күрсәтелгән",
'powersearch'         => 'Өстәмә эзләү',
'powersearch-legend'  => 'Өстәмә эзләү',

# Preferences page
'preferences'           => 'Көйләнмәләр',
'mypreferences'         => 'Көйләнмәләрем',
'prefs-edits'           => 'Үзгәртүләр исәбе:',
'prefsnologin'          => 'Кермәгәнсез',
'prefsnologintext'      => 'Кулланучы көйләнмәләрене үзгәртү өчен, сез [[Special:UserLogin|керергә]] тиешсез.',
'qbsettings'            => 'Күчешләр аслыгы',
'qbsettings-none'       => 'Күрсәтмәскә',
'changepassword'        => 'Серсүзне алыштырырга',
'skin'                  => 'Күренеш',
'math'                  => 'Формулалар',
'dateformat'            => 'Датаның форматы',
'datetime'              => 'Дата һәм вакыт',
'prefs-personal'        => 'Шәхси мәгълүматлар',
'prefs-rc'              => 'Баягы төзәтмәләр',
'prefs-watchlist'       => 'Күзәтү исемлеге',
'prefs-watchlist-days'  => 'Күзәтү исемлегендә ничә көн буена үзгәртүләрне күрсәтергә:',
'prefs-watchlist-edits' => 'Яхшыртырган исемлегендә төзәтмәләрнең иң югары исәбе:',
'prefs-misc'            => 'Башка көйләнмәләр',
'saveprefs'             => 'Саклау',
'resetprefs'            => 'Сакланмаган төзәтмәләрне бетерү',
'oldpassword'           => 'Иске серсүз:',
'newpassword'           => 'Яңа серсүз:',
'retypenew'             => 'Яңа серсүзне кабатлагыз:',
'textboxsize'           => 'Үзгәртү',
'rows'                  => 'Юллар:',
'columns'               => 'Баганалар:',
'servertime'            => 'Серверның вакыты',
'default'               => 'килешү буенча',
'files'                 => 'Файллар',

# User rights
'userrights-groupsmember' => 'Әгъза:',

# Groups
'group-bot'        => 'Ботлар',
'group-sysop'      => 'Идарәчеләр',
'group-bureaucrat' => 'Бюрократлар',
'group-suppress'   => 'Тикшерүчеләр',
'group-all'        => '(барлык)',

'group-autoconfirmed-member' => 'Авторасланган кулланучы',
'group-bot-member'           => 'Бот',
'group-sysop-member'         => 'Идарәче',
'group-bureaucrat-member'    => 'Бюрократ',
'group-suppress-member'      => 'Тикшерүче',

'grouppage-autoconfirmed' => '{{ns:project}}:Авторасланган кулланучылар',
'grouppage-bot'           => '{{ns:project}}:Ботлар',
'grouppage-sysop'         => '{{ns:project}}:Идарәчеләр',
'grouppage-bureaucrat'    => '{{ns:project}}:Бюрократлар',
'grouppage-suppress'      => '{{ns:project}}:Тикшерүчеләр',

# User rights log
'rightslog'  => 'Кулланучының хокуклары журналы',
'rightsnone' => '(юк)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|төзәтмә|төзәтмә}}',
'recentchanges'                     => 'Баягы төзәтмәләр',
'rcnote'                            => "Соңгы '''$1''' үзгәртмә '''$2''' көндә, сәгатьтә $5 $4.",
'rcnotefrom'                        => "Астарак '''$2''' башлап ('''$1''' кадәр) төзәтмәләр күрсәтелгән.",
'rclistfrom'                        => '$1 башлап яңа төзәтмәләрне күрсәтергә',
'rcshowhideminor'                   => '$1 әһәмиятсез үзгәртүләр',
'rcshowhidebots'                    => '$1 бот',
'rcshowhideliu'                     => '$1 кергән кулланучы',
'rcshowhideanons'                   => '$1 кермәгән кулланучы',
'rcshowhidepatr'                    => '$1 тикшерергән үзгәртү',
'rcshowhidemine'                    => '$1 минем үзгәртү',
'rclinks'                           => 'Соңгы $2 көн эчендә соңгы $1 төзәтмәне күрсәтергә<br />$3',
'diff'                              => 'аерма.',
'hist'                              => 'тарих',
'hide'                              => 'Яшерергә',
'show'                              => 'Күрсәтергә',
'minoreditletter'                   => 'ә',
'newpageletter'                     => 'Я',
'boteditletter'                     => 'б',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|күзәтеп тора кулланучы}}]',
'rc_categories_any'                 => 'Һәрбер',

# Recent changes linked
'recentchangeslinked'          => 'Бәйләнешле төзәтмәләр',
'recentchangeslinked-title'    => '"$1" битенә бәйләнешле төзәтмәләр',
'recentchangeslinked-noresult' => 'Күрсәтелгән вакытта сылташкан битләрнең үзгәртелмәләре юк иде.',
'recentchangeslinked-summary'  => "Бу күрсәтелгән бит белән сылталган (йә күрсәтелгән төркемгә керткән) битләрнең үзгәртелмәләре исемлеге.
[[Special:Watchlist|Күзәтү исемлегегезгә]] керә торган битләр '''калын'''.",

# Upload
'upload'        => 'Файлны йөкләргә',
'uploadbtn'     => 'Файлны йөкләргә',
'uploadlogpage' => 'Йөкләү журналы',
'uploadedimage' => '«[[$1]]» йөкләнгән',

# Special:ImageList
'imagelist'             => 'Сүрәтләр исемлеге',
'imagelist_date'        => 'Вакыт',
'imagelist_name'        => 'Ат',
'imagelist_user'        => 'Кулланучы',
'imagelist_size'        => 'Үлчәм',
'imagelist_description' => 'Тасвир',

# Image description page
'filehist'                  => 'Файлның тарихы',
'filehist-help'             => 'Датага/сәгатькә басыгыз, шул вакытта бит нинди булды дип карау өчен.',
'filehist-current'          => 'агымдагы',
'filehist-datetime'         => 'Дата/вакыт',
'filehist-user'             => 'Кулланучы',
'filehist-dimensions'       => 'Зурлык',
'filehist-filesize'         => 'Файлның зурлыгы',
'filehist-comment'          => 'Искәрмә',
'imagelinks'                => 'Сылтамалар',
'linkstoimage'              => 'Бу файлга киләчәк {{PLURAL:$1|бит|$1 бит}} сылтый:',
'nolinkstoimage'            => 'Бу файлга сылтаган битләр юк.',
'noimage'                   => 'Бу атлы файлны барлыкта юк, $1 сез булдырасыз.',
'noimage-linktext'          => 'аны йөкләргә',
'uploadnewversion-linktext' => 'Бу файлның яңа юрамасыны йөкләргә',

# MIME search
'mimesearch' => 'MIME эзләү',

# List redirects
'listredirects' => 'Җибәрүлек исемлеге',

# Unused templates
'unusedtemplates' => 'Кулланмаган өлгеләр',

# Random page
'randompage' => 'Очраклы бит',

# Random redirect
'randomredirect' => 'Очраклы биткә күчү',

# Statistics
'statistics' => 'Статистика',

'disambiguations' => 'Күп мәгънәле сүзләр турында битләр',

'doubleredirects' => 'Икеләтә җибәрүлекләр',

'brokenredirects' => 'Бәйләнешсез җибәрүлек',

'withoutinterwiki'        => 'Телләрара сылтамасыз битләр',
'withoutinterwiki-submit' => 'Күрсәтергә',

'fewestrevisions' => 'Аз үзгәртүләр белән битләр',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|байт}}',
'nlinks'                  => '$1 {{PLURAL:$1|сылтама}}',
'nmembers'                => '$1 {{PLURAL:$1|әгъза}}',
'lonelypages'             => 'Үксез битләр',
'uncategorizedpages'      => 'Төркемләнмәгән битләр',
'uncategorizedcategories' => 'Төркемләнмәгән төркемнәр',
'uncategorizedimages'     => 'Төркемләнмәгән сүрәтләр',
'uncategorizedtemplates'  => 'Төркемләнмәгән өлгеләр',
'unusedcategories'        => 'Кулланмаган төркемнәр',
'unusedimages'            => 'Кулланмаган сүрәтләр',
'wantedcategories'        => 'Зарур төркемнәр',
'wantedpages'             => 'Зарур битләр',
'mostlinked'              => 'Күп үзенә сылтамалы битләр',
'mostlinkedcategories'    => 'Күп үзенә сылтамалы төркемнәр',
'mostlinkedtemplates'     => 'Иң кулланган өлгеләр',
'mostcategories'          => 'Күп төркемләргә кертелгән битләр',
'mostimages'              => 'Иң кулланган сүрәтләр',
'mostrevisions'           => 'Күп үзгәртүләр белән битләр',
'prefixindex'             => 'Алкушымча буенча күрсәткеч',
'shortpages'              => 'Кыска мәкаләләр',
'longpages'               => 'Озын битләр',
'deadendpages'            => 'Тупик битләре',
'protectedpages'          => 'Якланган битләр',
'listusers'               => 'Кулланучылар исемлеге',
'newpages'                => 'Яңа битләр',
'ancientpages'            => 'Баягы төзәтмәләр белән битләр',
'move'                    => 'Күчерергә',
'movethispage'            => 'Бу битне күчерергә',

# Book sources
'booksources'               => 'Китап чыганаклары',
'booksources-search-legend' => 'Китап чыганакларыны эзләү',
'booksources-go'            => 'Башкару',

# Special:Log
'specialloguserlabel'  => 'Кулланучы:',
'speciallogtitlelabel' => 'Башлам:',
'log'                  => 'Журналлар',
'all-logs-page'        => 'Барлык журналлар',
'log-search-legend'    => 'Журналларны эзләү',
'log-search-submit'    => 'Башкару',

# Special:AllPages
'allpages'       => 'Барлык битләр',
'alphaindexline' => '$1 дан $2 гача',
'nextpage'       => 'Чираттагы бит ($1)',
'prevpage'       => 'Алдагы бит ($1)',
'allarticles'    => 'Барлык мәкаләләр',
'allpagesprev'   => 'Элекке',
'allpagesnext'   => 'Киләсе',
'allpagessubmit' => 'Башкару',
'allpagesprefix' => 'Алкушымчалы битләрне күрсәтергә:',

# Special:Categories
'categories'                    => 'Төркемнәр',
'categoriespagetext'            => 'Викидә бу категорияләре бар.',
'special-categories-sort-count' => 'исәп буенча тәртипләү',
'special-categories-sort-abc'   => 'әлифба буенча тәртипләү',

# Special:ListUsers
'listusers-submit'   => 'Күрсәтергә',
'listusers-noresult' => 'Кулланучыларны табылмады.',

# E-mail user
'emailuser'       => 'Бу кулланучыга хат',
'emailpage'       => 'Кулланучыга хат җибәрергә',
'defemailsubject' => '{{SITENAME}}: хат',
'noemailtitle'    => 'Электрон почта адресы юк',
'emailfrom'       => 'Кемдән',
'emailto'         => 'Кемгә',
'emailsubject'    => 'Тема',
'emailmessage'    => 'Хәбәр',
'emailsend'       => 'Җибәрергә',
'emailccme'       => 'Миңа хәбәрнең күчермәсене җибәрергә.',
'emailccsubject'  => '$1 өчен хәбәрегезнең күчермәсе: $2',
'emailsent'       => 'Хат җибәрелгән',

# Watchlist
'watchlist'            => 'Күзәтү исемлегем',
'mywatchlist'          => 'Күзәтү исемлегем',
'watchlistfor'         => "('''$1''' кулланучы өчен)",
'addedwatch'           => 'Күзәтү исемлегенә өстәгән',
'removedwatch'         => 'Күзәтү исемлегенән бетерергән',
'removedwatchtext'     => '«[[:$1]]» бите сезнең күзәтү исемлегездә бетерергән',
'watch'                => 'Күзәтергә',
'watchthispage'        => 'Бу битне күзәтергә',
'unwatch'              => 'Күзәтмәскә',
'wlshowlast'           => 'Баягы $1 сәгать $2 көн эчендә яки $3ны күрсәтергә',
'watchlist-show-bots'  => 'Ботларның үзгәртүләрене күрсәтергә',
'watchlist-hide-bots'  => 'Ботлар төзәтмәләрне яшерергә',
'watchlist-show-own'   => 'Минем үзгәртүләремне күрсәтергә',
'watchlist-hide-own'   => 'Төзәтмәләремне яшерергә',
'watchlist-show-minor' => 'Әһәмиятсез үзгәртүләрне күрсәтергә',
'watchlist-hide-minor' => 'Әһәмиятсез үзгәртүләрне яшерергә',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Күзәтү исемлегемә өстәүе…',
'unwatching' => 'Күзәтү исемлегемнән чыгаруы…',

'enotif_newpagetext'           => 'Бу яңа бит.',
'enotif_impersonal_salutation' => '{{SITENAME}} кулланучы',
'changed'                      => 'үзгәртергән',
'created'                      => 'төзергән',

# Delete/protect/revert
'deletepage'                  => 'Битне бетерергә',
'confirm'                     => 'Расларга',
'excontent'                   => 'эчтәлек: «$1»',
'exblank'                     => 'бит буш иде',
'delete-confirm'              => '«$1» бетерүе',
'delete-legend'               => 'Бетерү',
'historywarning'              => 'Кисәтү: сез бетерергә теләгән биттә үзгәртү тарихы бар:',
'actioncomplete'              => 'Гамәл башкарган',
'deletedtext'                 => '«<nowiki>$1</nowiki>» бетерергән инде.<br />
Соңгы бетерүләр карау өчен, $2 кара.',
'deletedarticle'              => '«[[$1]]» бетерергән',
'dellogpage'                  => 'Бетерү исемлеге',
'deletionlog'                 => 'бетерү журналы',
'deletecomment'               => 'Бетерү сәбәбе:',
'deleteotherreason'           => 'Башка/өстәмә сәбәп:',
'deletereasonotherlist'       => 'Башка сәбәп',
'rollbacklink'                => 'кире кайтару',
'protectlogpage'              => 'Яклану журналы',
'protectedarticle'            => '«[[$1]]» якланган',
'unprotectedarticle'          => '«[[$1]]» инде якланмаган',
'protectcomment'              => 'Искәрмә:',
'protect-unchain'             => 'Битнең күчерү рөхсәте ачарга',
'protect-text'                => 'Биредә сез <strong><nowiki>$1</nowiki></strong> бите өчен яклау дәрәҗәсене карый һәм үзгәрә аласыз.',
'protect-default'             => '(килешү буенча)',
'protect-fallback'            => '«$1»нең рөхсәте кирәк',
'protect-level-autoconfirmed' => 'Теркәлмәгән кулланучыларны кысарга',
'protect-level-sysop'         => 'Идарәчеләр генә',
'protect-summary-cascade'     => 'каскадлы',
'protect-expiring'            => '$1 үтә (UTC)',
'protect-cascade'             => 'Бу биткә кергән битләрне якларга (каскадлы яклау)',
'protect-cantedit'            => 'Сез бу битнең яклау дәрәҗәсене үзгәрә алмыйсыз, чөнки сездә аны үзгәртергә рөхсәтегез юк.',
'restriction-type'            => 'Рөхсәт:',
'restriction-level'           => 'Мөмкинлек дәрәҗәсе:',
'minimum-size'                => 'Иң кечкенә зурлык',
'maximum-size'                => 'Иң югары зурлык:',
'pagesize'                    => '(байт)',

# Restrictions (nouns)
'restriction-edit'   => 'Үзгәртү',
'restriction-move'   => 'Күчерү',
'restriction-create' => 'Төзү',

# Restriction levels
'restriction-level-sysop'         => 'тулы яклау',
'restriction-level-autoconfirmed' => 'өлешчә яклау',
'restriction-level-all'           => 'барлык дәрәҗәләр',

# Undelete
'undelete'               => 'Бетерергән битләрне карарга',
'undeletepage'           => 'Бетерергән битләрне карау һәм торгызу',
'viewdeletedpage'        => 'Бетерергән битләрне карарга',
'undeletebtn'            => 'Торгызырга',
'undeletelink'           => 'торгызырга',
'undeletereset'          => 'Ташлатырга',
'undeletecomment'        => 'Искәрмә:',
'undeletedarticle'       => '«[[$1]]» торгызырган',
'undelete-search-submit' => 'Эзләргә',

# Namespace form on various pages
'namespace'      => 'Исемнәр мәйданы:',
'invert'         => 'Сайланганны әйләнергә',
'blanknamespace' => '(Төп)',

# Contributions
'contributions' => 'Кулланучының кертеме',
'mycontris'     => 'Кертемем',
'uctop'         => '(ахыргы)',
'month'         => 'Айдан башлап (һәм элегрәк):',
'year'          => 'Елдан башлап (һәм элегрәк):',

'sp-contributions-newbies-sub' => 'Яңа хисап язмалары өчен',
'sp-contributions-blocklog'    => 'Кысу журналы',
'sp-contributions-username'    => 'Кулланучының IP адресы яки исеме:',
'sp-contributions-submit'      => 'Эзләргә',

# What links here
'whatlinkshere'       => 'Бирегә нәрсә сылтый',
'whatlinkshere-title' => '$1 битенә сылтый торган битләр',
'whatlinkshere-page'  => 'Бит:',
'linklistsub'         => '(Сылтамалар исемлеге)',
'linkshere'           => "'''[[:$1]]''' биткә чираттагы битләр сылтый:",
'nolinkshere'         => "'''[[:$1]]''' битенә башка битләр сылтамыйлар.",
'isredirect'          => 'җибәрү өчен бит',
'whatlinkshere-prev'  => '{{PLURAL:$1|алдагы|алдагы $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|чираттагы|чираттагы $1}}',
'whatlinkshere-links' => '← сылтамалар',

# Block/unblock
'blockip'      => 'Кулланучыны кысарга',
'ipboptions'   => '15 минут:15 minutes,2 сәгать:2 hours,6 сәгать:6 hours,12 сәгать:12 hours,1 көн:1 day,3 көн:3 days,1 атна:1 week,2 атна:2 weeks,1 ай:1 month,3 ай:3 months,6 ай:6 months,1 ел:1 year,вакытсыз:infinite', # display1:time1,display2:time2,...
'ipblocklist'  => 'Кысылган IP-адреслар һәм кулланучы исемләр исемлеге',
'blocklink'    => 'кысарга',
'unblocklink'  => 'кысмаска',
'contribslink' => 'кертем',
'blocklogpage' => 'Кысу журналы',

# Move page
'movearticle'    => 'Битне күчерергә:',
'newtitle'       => 'Яңа башлам:',
'move-watch'     => 'Бу битне күзәтергә',
'movepagebtn'    => 'Битне күчерергә',
'pagemovedsub'   => 'Бит күчерергән',
'movepage-moved' => "<big>'''«$1» бит «$2» биткә күчкән'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'movedto'        => 'күчерергән:',
'movetalk'       => 'Бәйләнешле фикер алышу битне күчерергә',
'1movedto2'      => '«[[$1]]» бите «[[$2]]» биткә күчерергән',
'movelogpage'    => 'Күчерү журналы',
'movereason'     => 'Сәбәп:',
'revertmove'     => 'кире кайту',

# Export
'export' => 'Битләрне чыгаруы',

# Namespace 8 related
'allmessages' => 'Система хәбәрләре',

# Thumbnails
'thumbnail-more'  => 'Зурайтырга',
'thumbnail_error' => 'Кечкенә сүрәт төзүе хатасы: $1',

# Import log
'importlogpage' => 'Кертү журналы',

# Tooltip help for the actions
'tooltip-pt-userpage'       => 'Минем кулланучы битем',
'tooltip-pt-mytalk'         => 'Фикер алышу битем',
'tooltip-pt-preferences'    => 'Минем көйләнмәләрем',
'tooltip-pt-watchlist'      => 'Сез күзәтелгән төзәтмәле битләр исемлеге',
'tooltip-pt-mycontris'      => 'Минем кертемем исемлеге',
'tooltip-pt-login'          => 'Сез хисап язмасы төзи алыр идегез, әмма бу мәҗбүри түгел.',
'tooltip-pt-logout'         => 'Чыгарга',
'tooltip-ca-talk'           => 'Битнең эчтәлеге турында фикер алышу',
'tooltip-ca-edit'           => 'Сез бу бит үзгәртә аласыз. Зинһар, саклаганчы карап алуны кулланыгыз.',
'tooltip-ca-addsection'     => 'Бу фикер алышуда шәрех калдырырга.',
'tooltip-ca-protect'        => 'Бу битне якларга',
'tooltip-ca-delete'         => 'Бу битне бетерергә',
'tooltip-ca-move'           => 'Бу битне күчерергә',
'tooltip-ca-watch'          => 'Бу битне сезнең күзәтү исемлегезгә өстәргә',
'tooltip-ca-unwatch'        => 'Бу битне сезнең күзәтү исемлегездә бетерергә',
'tooltip-search'            => 'Эзләү {{SITENAME}}',
'tooltip-n-mainpage'        => 'Төп битне кереп чыгарга',
'tooltip-n-portal'          => 'Проект турында, сез нәрсә итә аласыз һәм нәрсә кайда була дип турында.',
'tooltip-n-currentevents'   => 'Агымдагы вакыйгалар турында мәгълүматны табарга',
'tooltip-n-recentchanges'   => 'Баягы төзәтмәләр исемлеге.',
'tooltip-n-randompage'      => 'Очраклы битне карарга',
'tooltip-n-help'            => '«{{SITENAME}}» проектының белешмәлек',
'tooltip-t-whatlinkshere'   => 'Бирегә сылтаган барлык битләрнең исемлеге',
'tooltip-t-contributions'   => 'Кулланучының кертеме исемлегене карарга',
'tooltip-t-emailuser'       => 'Бу кулланучыга хат җибәрергә',
'tooltip-t-upload'          => 'Файлларны йөкләргә',
'tooltip-t-specialpages'    => 'Барлык махсус битләр исемлеге',
'tooltip-ca-nstab-user'     => 'Кулланучының битене карарга',
'tooltip-ca-nstab-project'  => 'Проектның битене карарга',
'tooltip-ca-nstab-image'    => 'Сүрәтнең битене карарга',
'tooltip-ca-nstab-template' => 'Өлгене карарга',
'tooltip-ca-nstab-help'     => 'Белешмәнең битене карарга',
'tooltip-ca-nstab-category' => 'Төркемнең битене карарга',
'tooltip-minoredit'         => 'Бу үзгәртүне әһәмиятсез булып билгеләргә',
'tooltip-save'              => 'Сезнең төзетмәләрегезне сакларга',
'tooltip-preview'           => 'Сезнең төзәтмәләрегезнең алдан карауы, саклаудан кадәр кулланыгыз әле!',
'tooltip-diff'              => 'Сезнең үзгәртмәләрегезне күрсәтү.',
'tooltip-watch'             => 'Бу битне күзәтү исемлегемә өстәргә',

# Browsing diffs
'previousdiff' => '← Алдагы аерма',
'nextdiff'     => 'Чираттагы аерма →',

# Media information
'file-info-size'       => '($1 × $2 нокта, файлның зурлыгы: $3, MIME тибы: $4)',
'file-nohires'         => '<small>Югары ачыклык белән юрама юк.</small>',
'svg-long-desc'        => '(SVG файлы, шартлы $1 × $2 нокта, файлның зурлыгы: $3)',
'show-big-image'       => 'Тулы ачыклык',
'show-big-image-thumb' => '<small>Алдан карау зурлыгы: $1 × $2 нокта</small>',

# Special:NewImages
'newimages' => 'Яңа сүрәтләр җыелмасы',

# Bad image list
'bad_image_list' => 'Киләчәк рәвеш кирәк:

Исемлек кисәкләре генә (* символыннан башланучы юллар) саналырлар.
Юлның беренче сылтамасы куйма өчен тыелган рәсемгә сылтама булырга тиеш.
Шул ук юлның киләчәк сылтамалары чыгармалар, рәсемгә тыелмаган битләре, саналырлар.',

# Metadata
'metadata'          => 'Мета мәгълүматлар',
'metadata-expand'   => 'Өстәмә мәгълүматларны күрсәтергә',
'metadata-collapse' => 'Өстәмә мәгълүматларны яшерергә',

# EXIF tags
'exif-brightnessvalue' => 'Яктылык',

# External editor support
'edit-externally' => 'Бу файлны тышкы кушымтаны кулланып үзгәртергә',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'барлык',
'namespacesall' => 'барлык',
'monthsall'     => 'барлык',

# Watchlist editing tools
'watchlisttools-edit' => 'Күзәтү исемлегене карау һәм үзгәртү',

# Special:Version
'version' => 'Юрама', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Махсус битләр',

);
