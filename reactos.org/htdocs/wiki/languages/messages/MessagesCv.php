<?php
/** Chuvash (Чăвашла)
 *
 * @ingroup Language
 * @file
 *
 * @author Chavash
 * @author PCode
 */

$fallback = 'ru';

$linkPrefixExtension = true;

$namespaceNames = array(
	NS_MEDIA            => 'Медиа',
	NS_SPECIAL          => 'Ятарлă',
	NS_MAIN             => '',
	NS_TALK             => 'Сӳтсе явасси',
	NS_USER             => 'Хутшăнакан',
	NS_USER_TALK        => 'Хутшăнаканăн_канашлу_страници',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_сӳтсе_явмалли',
	NS_IMAGE            => 'Ӳкерчĕк',
	NS_IMAGE_TALK       => 'Ӳкерчĕке_сӳтсе_явмалли',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_сӳтсе_явмалли',
	NS_TEMPLATE         => 'Шаблон',
	NS_TEMPLATE_TALK    => 'Шаблона_сӳтсе_явмалли',
	NS_HELP             => 'Пулăшу',
	NS_HELP_TALK        => 'Пулăшăва_сӳтсе_явмалли',
	NS_CATEGORY         => 'Категори',
	NS_CATEGORY_TALK    => 'Категорине_сӳтсе_явмалли',
);

$linkTrail = '/^([a-zа-яĕçăӳ"»]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Ссылкăсене аялтан туртса палармалла:',
'tog-highlightbroken'         => 'Халлĕхе çук ссылкăсене <a href="" class="new">çапла</a> (урăхла ак çапла<a href="" class="internal">?</a> курăнĕ) кăтартмалла.',
'tog-justify'                 => 'Страницăри текста сарлакăшĕпе вырнаçтармалла',
'tog-hideminor'               => 'Юлашки улшăнусене кăтарнă страницăра пĕчĕк тӳрлетӳ çеç кĕртнисене кăтартмалла мар',
'tog-extendwatchlist'         => 'Пĕтĕм улшăнусене кăтартакан анлă сăнав списокĕ',
'tog-usenewrc'                => 'Юлашки улшăнусен лайăхлатнă списокĕ (JavaScript)',
'tog-numberheadings'          => 'Заголовоксене хăй тĕллĕн номерлесе пымалла.',
'tog-showtoolbar'             => 'Тӳрлетнĕ чухне çӳлти хатĕрсен хăмине кăтартмалла (JavaScript)',
'tog-editondblclick'          => 'Иккĕ пуссан статьяна тӳрлетмелле (JavaScript)',
'tog-editsection'             => '«Тӳрлет» ссылкăна кашни пайра кăтартмалла',
'tog-editsectiononrightclick' => 'Шăшин сылтăм пускăчне пуссан статья пайне тӳрлетмелле (JavaScript)',
'tog-showtoc'                 => 'Тупмаллине кăтартмалла (3, е ытларах заголовок пулсан)',
'tog-rememberpassword'        => 'Мана ку компьютер çинче астуса хăвармалла',
'tog-editwidth'               => 'Тӳрлетмелли чӳречене браузерăн пĕтĕм сарлакăшĕпе кăтартмалла',
'tog-watchcreations'          => 'Эпĕ тунă страницăсене сăнав списокне кĕртмелле',
'tog-watchdefault'            => 'Тӳрлетнĕ страницăсене сăнав списокне кĕртмелле',
'tog-watchmoves'              => 'Страница ятне эпĕ улаштарсан вĕсене сăнав списокне кĕртмелле',
'tog-watchdeletion'           => 'Эпĕ кăларса пăрахнă страницăсене сăнав списокне кĕртмелле',
'tog-minordefault'            => 'Палăртман чухне улшăнусене пĕчĕк тӳрлетӳсем пек палăртмалла',
'tog-previewontop'            => 'Тӳрлетӳ чӳречине кăтартас умĕн малтанхи курăмне кăтармалла',
'tog-previewonfirst'          => 'Пĕрремĕш хут тӳрлетнĕ чухне малтанхи курăнăва кăтартмалла',
'tog-nocache'                 => 'Страницăсене кэшра сыхласа хăварассине чармалла',
'tog-enotifwatchlistpages'    => 'Сăнав списокĕнчи статяна тӳрлетӳ кĕртсен электронлă почта урлă пĕлтермелле',
'tog-enotifusertalkpages'     => 'Ман канашлу страницинче улшăнусем пулсан мана эл. почта урлă пĕлтермелле',
'tog-enotifminoredits'        => 'Пĕчĕк улшăнусене те эл. почта урлă пĕлтермелле',
'tog-enotifrevealaddr'        => 'Асăрхаттару пĕлтерĕвĕсенче ман эл. адреса кăтартмалла',
'tog-shownumberswatching'     => 'Страницăна миçе хутшăнакан сăнав списокне кĕртнине кăтартмалла',
'tog-fancysig'                => 'Хăвăр тунă алă пуснин вики-паллисем',
'tog-externaleditor'          => 'Палăртман чухне тулашри редактора усă курмалла',
'tog-externaldiff'            => 'Палăртман чухне версисене танлаштарма тулашри программăна усă курмалла',
'tog-showjumplinks'           => '«... патне куç» хушма каçăсене усă курмалла',
'tog-uselivepreview'          => 'Хăвăрт кăтартакан малтанхи курăнăва усă курмалла (JavaScript, экспериментлă)',
'tog-forceeditsummary'        => 'Тӳрлетӳсен кĕске ăнлантарăвне кăтартман чухне асăрхаттармалла',
'tog-watchlisthideown'        => 'Сăнав списокĕнче эпĕ тунă тӳрлетӳсене кăтартмалла мар',
'tog-watchlisthidebots'       => 'Сăнав списокĕнче бот тӳрлетĕвĕсене кăтартмалла мар',
'tog-watchlisthideminor'      => 'Сăнав списокĕнче пĕчĕк улшăнусене кăтартмалла мар',
'tog-ccmeonemails'            => 'Эп ытти хутшăнакансем патне янă çырусен копине мана та ярса памалла.',
'tog-diffonly'                => 'Икĕ версине танлаштарнă чухне страница ăшлăхне кăтартмалла мар',
'tog-showhiddencats'          => 'Пытарнă категорисене кăтартмалла',

'underline-always'  => 'Яланах',
'underline-never'   => 'Нихăçан та',
'underline-default' => 'Браузер ĕнерлевĕсене усă курмалла',

'skinpreview' => '(Малтанхи курăну)',

# Dates
'sunday'        => 'вырсарникун',
'monday'        => 'Тунтикун',
'tuesday'       => 'Ытларикун',
'wednesday'     => 'Юнкун',
'thursday'      => 'Кĕçнерникун',
'friday'        => 'Эрнекун',
'saturday'      => 'шăматкун',
'sun'           => 'Вр',
'mon'           => 'Тн',
'tue'           => 'Ыт',
'wed'           => 'Юнк',
'thu'           => 'Кç',
'fri'           => 'Эрн',
'sat'           => 'Шм',
'january'       => 'Кăрлач',
'february'      => 'нарăс',
'march'         => 'Пуш',
'april'         => 'Ака',
'may_long'      => 'Çу',
'june'          => 'Çěртме',
'july'          => 'Утă',
'august'        => 'Çурла',
'september'     => 'Авăн',
'october'       => 'Юпа',
'november'      => 'Чӳк',
'december'      => 'Раштав',
'january-gen'   => 'кăрлач',
'february-gen'  => 'нарăс',
'march-gen'     => 'пуш',
'april-gen'     => 'ака',
'may-gen'       => 'çу',
'june-gen'      => 'çĕртме',
'july-gen'      => 'утă',
'august-gen'    => 'çурла',
'september-gen' => 'авăн',
'october-gen'   => 'юпа',
'november-gen'  => 'чӳк',
'december-gen'  => 'раштав',
'jan'           => 'Кăр',
'feb'           => 'нар',
'mar'           => 'Пуш',
'apr'           => 'Ака',
'may'           => 'Çу',
'jun'           => 'Çěр',
'jul'           => 'Утă',
'aug'           => 'Çур',
'sep'           => 'Авн',
'oct'           => 'Юпа',
'nov'           => 'Чӳк',
'dec'           => 'Раш',

# Categories related messages
'pagecategories'                 => 'Категорисем',
'category_header'                => '«$1» категорири статьясем',
'subcategories'                  => 'Подкатегорисем',
'category-media-header'          => '«$1» категорири файлсем',
'category-empty'                 => "''Хальхи вăхăтра ку категори пушă.''",
'hidden-categories'              => '{{PLURAL:$1|Пытарнă категори|Пытарнă категорисем}}',
'hidden-category-category'       => 'Пытарнă категорисем', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Ку категоринче çак айри категори пур.|$2-ран(-рен,-тан,-тен) {{PLURAL:$1|$1 айри категорине кăтартнă|$1 айри категорине кăтартнă|$1 айри категорине кăтартнă}}.}}',
'category-subcat-count-limited'  => 'Ку категоринче {{PLURAL:$1|$1 айри категори|$1 айри категори|$1 айри категори}}.',
'category-article-count'         => '{{PLURAL:$2|Ку категоринче пĕр страница кăна.|Ку категорири $2 страницăран $1 кăтартнă.}}',
'category-article-count-limited' => 'Ку категоринче $1 страница.',
'category-file-count'            => '{{PLURAL:$2|Ку категоринче пĕр файл кăна.|Ку категоринчи $2 файлтан $1 кăтартнă.}}',
'category-file-count-limited'    => 'Ку категоринче $1 файл.',
'listingcontinuesabbrev'         => '(малалли)',

'linkprefix'        => '/^(.*?)([a-zA-Z\\x80-\\xff«"]+)$/sD',
'mainpagetext'      => '<big>«MediaWiki» вики-движока лартасси ăнăçлă вĕçленчĕ.</big>',
'mainpagedocfooter' => 'Ку википе ĕçлеме пулăшакан информацине [http://meta.wikimedia.org/wiki/%D0%9F%D0%BE%D0%BC%D0%BE%D1%89%D1%8C:%D0%A1%D0%BE%D0%B4%D0%B5%D1%80%D0%B6%D0%B0%D0%BD%D0%B8%D0%B5 усăç руководствинче] тупма пултаратăр.

== Пулăшма пултарĕç ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Ĕнерлевсен списокĕ];
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki тăрăх час-часах ыйтакан ыйтусемпе хуравсем];
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki çĕнĕ верси тухнине пĕлтерекен рассылка].',

'about'          => 'Ăнлантаркăч',
'article'        => 'Статья',
'newwindow'      => '(çĕнĕ чӳречере)',
'cancel'         => 'Пăрахăçла',
'qbfind'         => 'Шырав',
'qbbrowse'       => 'Курăм',
'qbedit'         => 'Тӳрлет',
'qbpageoptions'  => 'Страница ĕнерлевĕсем',
'qbpageinfo'     => 'Страница çинчен',
'qbmyoptions'    => 'Сирĕн ĕнĕрлевсем',
'qbspecialpages' => 'Ятарлӑ страницӑсем',
'moredotdotdot'  => 'Малалла…',
'mypage'         => 'Ман страница',
'mytalk'         => 'Ман канашлу страници',
'anontalk'       => 'Çак IP-адреса сӳтсе явни',
'navigation'     => 'Меню',
'and'            => 'тата',

# Metadata in edit box
'metadata_help' => 'Метаданнăйсем:',

'errorpagetitle'    => 'Йăнăш',
'returnto'          => '$1 страницăна таврăн.',
'tagline'           => '{{GRAMMAR:genitive|{{SITENAME}}}}ри материал',
'help'              => 'Пулăшу',
'search'            => 'Шырасси',
'searchbutton'      => 'Шырасси',
'go'                => 'Куç',
'searcharticle'     => 'Куç',
'history'           => 'Истори',
'history_short'     => 'Истори',
'updatedmarker'     => 'эпĕ юлашки хут кĕнĕ хыççăн çĕнелнĕ',
'info_short'        => 'Информаци',
'printableversion'  => 'Пичетлемелли верси',
'permalink'         => 'Яланхи вырăн',
'print'             => 'Пичетле',
'edit'              => 'Тӳрлетӳ',
'create'            => 'Çĕннине ту',
'editthispage'      => 'Страницăна тӳрлетесси',
'create-this-page'  => 'Ку страницăна хатĕрле',
'delete'            => 'Кăларса пăрахасси',
'deletethispage'    => 'Хурат ăна',
'undelete_short'    => '$1 тӳрлетӳсене каялла тавăр',
'protect'           => 'хӳтĕле',
'protect_change'    => 'хӳтĕлĕве улăштар',
'protectthispage'   => 'Хӳтĕле',
'unprotect'         => 'Хӳтĕлеве пăрахăçла',
'unprotectthispage' => 'Хӳтĕлеве пăрахăçла',
'newpage'           => 'Çĕнĕ статья',
'talkpage'          => 'Сӳтсе явасси',
'talkpagelinktext'  => 'Сӳтсе яв',
'specialpage'       => 'Ятарлă страницă',
'personaltools'     => 'Ман хатĕрсем',
'postcomment'       => 'Хуравла',
'articlepage'       => 'Статьяна пăх',
'talk'              => 'Сӳтсе явасси',
'views'             => 'Пурĕ пăхнă',
'toolbox'           => 'Ĕç хатĕрĕсем',
'userpage'          => 'Хутшăнакан страницине пăх',
'projectpage'       => 'Проект страницине пăх',
'imagepage'         => 'Ӳкерчĕк страницине пăх',
'mediawikipage'     => 'Пĕлтерӳ страницине кăтарт',
'templatepage'      => 'Шаблонăн страницине пăх',
'viewhelppage'      => 'Справка страницине пăх',
'categorypage'      => 'Категори страницине пăх',
'viewtalkpage'      => 'Сӳтсе явнине тишкер',
'otherlanguages'    => 'Урăх чěлхесем',
'redirectedfrom'    => '($1 çинчен куçарнă)',
'redirectpagesub'   => 'Куçаракан страница',
'lastmodifiedat'    => 'Ку страницăна юлашки улăштарнă вăхăт: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Ку страницăна $1 хут пăхнă.',
'protectedpage'     => 'Хӳтĕленĕ статья',
'jumpto'            => 'Куçас:',
'jumptonavigation'  => 'навигаци',
'jumptosearch'      => 'Шырав',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} çинчен',
'aboutpage'            => 'Project:çинчен',
'bugreports'           => 'Йăнăш çинчен пĕлтерни',
'bugreportspage'       => 'Project:Йăнăшсен пĕтĕмлетĕвĕ',
'copyright'            => 'Ку ăшлăх $1 килĕшӳллĕн сарăлать.',
'copyrightpagename'    => '{{SITENAME}} проектăн автор прависем',
'copyrightpage'        => '{{ns:project}}:Автор прависем',
'currentevents'        => 'Хыпарсем',
'currentevents-url'    => 'Project:Хыпарсем',
'disclaimers'          => 'Яваплăха тивĕçтерменни',
'disclaimerpage'       => 'Project:Яваплăха тивĕçтерменни',
'edithelp'             => 'Улшăнусене кĕртме пулăшакан пулăшу',
'edithelppage'         => 'Help:Улшăнусене кĕртме пулăшакан пулăшу',
'faq'                  => 'ЫйХу',
'faqpage'              => 'Project:ЫйХу',
'helppage'             => 'Help:Пулăшу',
'mainpage'             => 'Тĕп страницă',
'mainpage-description' => 'Тĕп страницă',
'policy-url'           => 'Project:Йĕркесем',
'portal'               => 'Портал',
'portal-url'           => 'Project:Портал',
'privacy'              => 'Конфиденциальность йĕркисем',
'privacypage'          => 'Project:Конфиденциальность йĕркисем',

'badaccess'        => 'Доступ йăнăшĕ',
'badaccess-group0' => 'Эсир ыйтакан ĕçе тăваймастăр',
'badaccess-group1' => 'Ку ĕçе $1 шутне кĕрекен хутшăнакан кăна тума пултарать.',
'badaccess-group2' => 'Ку ĕçе $1 шутне кĕрекен хутшăнакан кăна тума пултарать.',
'badaccess-groups' => 'Ку ĕçе $1 шутне кĕрекен хутшăнакан кăна тума пултарать.',

'versionrequired'     => 'MediaWiki-н $1 версийĕ кирлĕ',
'versionrequiredtext' => 'Ку страницăпа ĕслемешкĕн сире MediaWiki-н $1 версийĕ кирлĕ. [[Special:Version|Усă куракан программăсен версийĕсем çинчен пĕлтерекен информацине]] пăх.',

'ok'                      => 'OK',
'retrievedfrom'           => 'Çăлкуç — «$1»',
'youhavenewmessages'      => 'Сирĕн $1 пур ($2).',
'newmessageslink'         => 'çĕнĕ пĕлтерӳсем',
'newmessagesdifflink'     => 'юлашки улшăну',
'youhavenewmessagesmulti' => '$1-та çĕнĕ пĕлтерӳсем пур.',
'editsection'             => 'тӳрлет',
'editold'                 => 'тӳрлет',
'viewsourceold'           => 'пуçламăш текста пăх',
'editsectionhint'         => '$1 пайне тӳрлет',
'toc'                     => 'Тупмалли',
'showtoc'                 => 'кăтарт',
'hidetoc'                 => 'кӑтартмалла мар',
'thisisdeleted'           => '$1 пăхса каялла тавăрмалла-и?',
'viewdeleted'             => '$1 пăхар-и?',
'restorelink'             => '$1 кăларса пăрахнă тӳрлетĕве',
'feedlinks'               => 'Çапла кур:',
'feed-invalid'            => 'Çырăнмашкăн ку канал тĕсĕ каймасть.',
'feed-unavailable'        => '{{SITENAME}} сайтри синдикаци хăйăвĕсем тупăнмарĕç',
'site-rss-feed'           => '$1 — RSS-хăю',
'site-atom-feed'          => '$1 - Atom хăю',
'page-rss-feed'           => '«$1» - RSS хăю',
'page-atom-feed'          => '«$1» - Atom хăю',
'red-link-title'          => '$1 (хальлĕхе çырман)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Статья',
'nstab-user'      => 'Хутшăнакан страници',
'nstab-media'     => 'Мультимеди',
'nstab-special'   => 'Ятарлă',
'nstab-project'   => 'Проект çинчен',
'nstab-image'     => 'Файл',
'nstab-mediawiki' => 'Пĕлтерӳ',
'nstab-template'  => 'Шаблон',
'nstab-help'      => 'пулăшу',
'nstab-category'  => 'Категори',

# Main script and global functions
'nosuchaction'      => 'Ку ĕçе тăваймастпăр',
'nosuchactiontext'  => 'URLта çырнă хушăва вики скрипчĕ ăнланмасть',
'nosuchspecialpage' => 'Вăл ятлă ятарлă страница çук',
'nospecialpagetext' => 'Эсир ыйтакан ятарлă страница çук. [[Special:SpecialPages|Ятарлă страницăсен списокне]] пăхăр.',

# General errors
'error'                => 'Йăнăш',
'databaseerror'        => 'Пĕлĕм пуххин йăнăшĕ',
'dberrortext'          => 'Пĕлĕм пуххине янă ыйтăвĕнче синтаксис йăнăшĕ пур.
Пĕлĕм пуххине янă юлашки ыйту:
<blockquote><tt>$1</tt></blockquote>
<tt>«$2»</tt> функци ыйтнă.
MySQL çак йăнăша тавăрнă <tt>«$3: $4»</tt>.',
'dberrortextcl'        => 'Пĕлĕм пуххине янă ыйтăвĕнче синтаксис йăнăшĕ пур.
Пĕлĕм пуххине янă юлашки ыйту:
«$1»
«$2» функци ыйтнă.
MySQL çак йăнăша тавăрнă «$3: $4».',
'noconnect'            => 'Каçарăр та, хальхи вăхăтра техника йăнăшне пула пĕлĕм пуххин серверĕпе çыханаймастпăр.<br />
$1',
'nodb'                 => '$1 пĕлĕм пуххине усă курма май çук',
'cachederror'          => 'Аяларах эсир ыйтнă страницăн кэшри копине куратăр; вăл кивелме те пултарнă.',
'laggedslavemode'      => 'Асăрхăр! Страница çинче юлашки улшăнусене кăтартмасăр пултарнă.',
'readonly'             => 'Пĕлĕм пуххине çырассине чарса хунă',
'enterlockreason'      => 'Чарнин сăлтавне тата палăртнă вăхăта кăтартăр.',
'readonlytext'         => 'Çĕнĕ статьясене кĕртессипе улăштарассине чарнă: план тăрăх иртекен ĕçсем пулма пултараççĕ.
Ĕçлеме чарнă опаратор çак ăнлантарăва хăварнă:
$1',
'missing-article'      => 'Пĕлĕм пуххинче эсир ыйтакан текст тупăнмарĕ. «$1» $2 тупăнмаллаччĕ.

Ку йăнăш ытларах тахçанхи каçă урлă каçнă чухне тĕл пулать — вăл вăхăтран ăна кăларса пăрахма та пултарнă.

Енчен те йăнăш урăх сăлтава пула тухрĕ пулсан — [[Special:ListUsers/sysop|администратора]] пĕлтерĕр. Пĕлтернĕ чухне URL пĕлтерме ан манăр.',
'missingarticle-rev'   => '(верси №: $1)',
'missingarticle-diff'  => '(уйрăмлăх: $1, $2)',
'readonly_lag'         => 'Иккĕмĕш шайри пĕлĕм пуххи серверĕ пĕрремĕш шайрипе синхронизацилениччен пĕлĕм пуххине улшăнусем кĕртессине чарса лартнă.',
'internalerror'        => 'Шалти йăнăш',
'internalerror_info'   => 'Шалти йăнăш: $1',
'filecopyerror'        => '«$1» файла «$2» çине копилеймерĕмер.',
'filerenameerror'      => '«$1» файл ятне «$2» çине улăштарма май çук.',
'filedeleteerror'      => '«$1» файла кăларса парахма май çук.',
'directorycreateerror' => '«$1» директорине тума май çук.',
'filenotfound'         => '«$1» файла тупма май çук.',
'fileexistserror'      => '«$1» файла çырма май çук: ку ятлă файл пур.',
'unexpected'           => 'Тĕрĕс мар пĕлтерĕш: «$1»=«$2».',
'formerror'            => 'Йăнăш: формăри даннăйсене леçме май çук',
'badarticleerror'      => 'Çак страницăра эсир ку ĕçĕ тăваймастăр.',
'cannotdelete'         => 'Эсир кăтартнă страницăна е файла кăларса пăрахаймастпăр. Ăна, тен, урăххи кăларса пăрахнă?',
'badtitle'             => 'Ку ят килĕшмест',
'badtitletext'         => 'Эсир кăтартнă статья ячĕ тĕрĕс мар, пушă, е чĕлхе хушшинчипе интервики ячĕ тĕрĕс мар. Ятра усă курма юраман паллăсене çырнинче пулма пултарать.',
'perfdisabled'         => 'Каçарăр та, сервер йывăррăн ĕçленине пула ку ĕçе вăхăтлăха тума май çук.',
'perfcached'           => 'Ку даннăйсене кэшран илнĕ, çавна май унта юлашки улшăнусем палăрмасăр пултараççĕ.',
'perfcachedts'         => 'Ку даннăйсене кэшран илнĕ, юлашки хут вăл $1 вăхăтра çĕнелнĕ.',
'querypage-no-updates' => 'Ку страницăна хальхи вăхăтра улăштарма чарнă. Ку даннăйсене хальхи вăхăтра çĕнетме май çук.',
'wrong_wfQuery_params' => 'wfQuery() функцине тĕрĕс мар параметрсем панă<br />
Функци: $1<br />
Ыйту: $2',
'viewsource'           => 'Курăм',
'viewsourcefor'        => '«$1» страници',
'actionthrottled'      => 'Хăвăртлăха чакарнă',
'actionthrottledtext'  => 'Спампа кĕрешнине пула ку ĕçе пĕчĕк вăхăт хушшинче ытла нумай тума чарнă. Темиçе минутран тепре туса пăхма пултаратăр.',
'protectedpagetext'    => 'Ку страницăна тӳрлетме май çук, хӳтĕленĕ.',
'viewsourcetext'       => 'Эсир ку страницăн малтанхи текстне пăхма тата копилеме пултаратăр:',
'protectedinterface'   => "Ку страница çинче MediaWiki'н системлă çырăвĕ вырнаçнă, ăна проект администраторĕсем çеç улăштарма пултараççĕ.",
'sqlhidden'            => '(SQL ыйтăва пытарнă)',
'namespaceprotected'   => 'Сирĕн «$1» ят уçлăхĕнчи статьясене тӳрлетмелли май çук..',
'customcssjsprotected' => 'Ку страницăра тепĕр хутшăнаканăн ятарлă ĕнерлевĕсем вырнаçнă, çавна май эсир ăна тӳрлетейместĕр.',
'ns-specialprotected'  => '«{{ns:special}}» ят уçлăхĕнчи страницăсене эсир тӳрлетейместĕр.',
'titleprotected'       => "Ку ятлă страницăна хатĕрлессине [[Хутшăнакан:$1|$1]] хутшăнакан чарса хунă.
Çак сăлтава кăтартнă: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Ĕнерлев йăнăшĕ. Вирус сканерĕ паллă мар: <i>$1</i>',
'virus-scanfailed'     => 'скенерланă чухнехи йăнăш (код $1)',
'virus-unknownscanner' => 'паллă мар антивирус:',

# Login and logout pages
'logouttitle'                => 'Сайтран тух',
'logouttext'                 => 'Эсир палласа илмен хутшăнакан евĕр ĕçлетĕр. Сайт сире ятпа мар, IP-адрес урлă пĕлет. Эсир анонимла, е малтанхи евĕрлĕ çĕнĕ сеанс уçса, е  урăх ятпа ĕçлеме пултаратăр. Хăш-пĕр страницăсем эсир сайта кĕнĕ пек курăнма пултараççĕ, ăна тӳрлетмешкĕн браузер кэшне çĕнетĕр.',
'welcomecreation'            => '== Ырă сунса кĕтетпĕр, $1! ==
Эсир пирĕн патăмăрта çырăнтăр.
[[Special:Preferences|Сайт ĕнĕрлевĕсене хăвсамăра кирлĕ пек]] лартма ан манса кайăр.',
'loginpagetitle'             => 'Сайта кĕр',
'yourname'                   => 'Сирĕн ят',
'yourpassword'               => 'Вăрттăн сăмах:',
'yourpasswordagain'          => 'Вăрттăн сăмах тепре çырăр:',
'remembermypassword'         => 'Ку компьютер çинче мана астуса хăвармалла',
'yourdomainname'             => 'Сирĕн домен',
'loginproblem'               => '<span style="color:red">Хутшăнакана палаймарăмăр.</span>',
'login'                      => 'Сайта кĕр',
'nav-login-createaccount'    => 'Сайта кĕр / регистрацилен',
'loginprompt'                => 'Сайта кĕрес тесен сирĕн «cookies»-па усă курма ирĕк памалла.',
'userlogin'                  => 'Сайта кĕр',
'logout'                     => 'Сайтран тухас',
'userlogout'                 => 'сайтран тух',
'notloggedin'                => 'Эсир сайта кĕмен',
'nologin'                    => 'Эсир халĕ те регистрациленмен-и? $1.',
'nologinlink'                => 'Çĕнĕ хутшăнакана регистрацилесси',
'createaccount'              => 'Çĕнĕ хутшăнакана регистрацилесси',
'gotaccount'                 => 'Эсир регистрациленсе-и? $1.',
'gotaccountlink'             => 'Сайта кĕр',
'createaccountmail'          => 'эл. почта тăрăх',
'badretype'                  => 'Эсир кăтартнă парольсем пĕр пек мар.',
'userexists'                 => 'Эсир усă курас теекен ята йышăннă. Тархасшăн, урăх ят суйласа илĕр.',
'youremail'                  => 'Электронлă почта:',
'username'                   => 'Хутшăнакан ячĕ:',
'uid'                        => 'Хутшăнакан идентификаторĕ:',
'yourrealname'               => 'Сирĕн чăн ят (*)',
'yourlanguage'               => 'Интерфейс чĕлхи:',
'yourvariant'                => 'Чĕлхе варианчĕ',
'yournick'                   => 'Сирĕн ят (алă пусма усă курăнĕ):',
'badsig'                     => 'Алă пуснинче йăнăш пур. HTML тэгĕсене тĕрĕслĕр.',
'badsiglength'               => 'Алă пусни ытла вăрăм, $1 символтан кĕскерех пулмалла.',
'email'                      => 'Эл. почта',
'loginerror'                 => 'Хутшăнакана палласа илеймерĕмĕр',
'prefs-help-email'           => 'Электронлă почта (вăл кирлисем шутне кĕмест пулин те) ытти хутшăнакансене сирĕнпе ун урлă çыхăнма май парать. Çыхăну тытнă вăхăтра ыттисем сирĕн адреса пĕлеймеççĕ.',
'prefs-help-email-required'  => 'Электронлă почтăн адресне кăтартмалла.',
'noname'                     => 'Эсир килĕшӳллĕ хутшăнаканăн ятне кăтартман.',
'loginsuccesstitle'          => 'Сайта кĕтĕмĕр',
'loginsuccess'               => 'Халĕ эсир $1 ятпа ĕçлетĕр.',
'nosuchuser'                 => '$1 ятлă хутшăнакан çук.
Çырнă ята тепĕр хут тĕрĕслĕр, е аяларах вырнаçнă формăна усă курса çĕнĕ хутшăнакана регистрацилĕр.',
'nosuchusershort'            => '$1 ятлă хутшăнакан çук. Ятне епле çырнине тĕрĕслĕр.',
'nouserspecified'            => 'Сирĕн хутшăнаканăн ятне каламалла.',
'wrongpassword'              => 'Эсир кăтартнă вăрттăн сăмах тĕрĕс мар. Урăххине кăтартăр.',
'wrongpasswordempty'         => 'Пушă мар пароль çырăр тархасшăн.',
'mailmypassword'             => 'Çĕнĕ вăрттăн сăмаха ярса ил',
'passwordremindertitle'      => '{{grammar:genitive|{{SITENAME}}}} хутшăнаканăн вăрттăн сăмахне асаилтересси',
'noemail'                    => '$1 ятлă хутшăнаканăн электронлă адресне кăтартман.',
'passwordsent'               => 'Çĕнĕ вăрттăн сăмаха $1 хутшăнакан кăтартнă эл. почтăн адресĕпе ятăмар.

Вăрттăн сăмаха илнĕ хыççăн, тархасшăн, сайта çĕнĕрен кĕрĕр.',
'blocked-mailpassword'       => 'Ку IP-адреслисене статьясене тӳрлетме чарнă. Вăрттăн сăмаха аса илмелли функципе те усă кураймăр.',
'mailerror'                  => 'Çыру яраймарăмăр, йăнăш тухрĕ: $1',
'acct_creation_throttle_hit' => 'Сирĕн $1 хутшăнакан пур ĕнтĕ, урăх кĕртейместĕр.',
'emailauthenticated'         => 'Сирĕн электронлă адреса çирĕплетнĕ $1.',
'emailconfirmlink'           => 'Хăвăр эл. почтăн адресне çирĕплетĕр',
'accountcreated'             => 'Сире хутшăнакансем шутне кĕртрĕмĕр',
'accountcreatedtext'         => 'Хутшăнакансем шутне $1 ятлă çĕнĕ хутшăнакана кĕртрĕмĕр.',
'createaccount-title'        => '{{SITENAME}}: хутшăнакана хушасси',
'loginlanguagelabel'         => 'Чĕлхе: $1',

# Password reset dialog
'resetpass'         => 'Хутшăнаканăн вăрттăн сăмахне çĕнетесси',
'resetpass_header'  => 'Пароле пăрахăçла',
'resetpass_submit'  => 'Вăрттăн сăмаха лартса сайта кĕр',
'resetpass_missing' => 'Формăра даннăйсем çук.',

# Edit page toolbar
'bold_sample'    => 'Çурмахулăм текст',
'bold_tip'       => 'Çурмахулăм текст',
'italic_sample'  => 'Тайлăк текст',
'italic_tip'     => 'Тайлăк текст',
'link_sample'    => 'Ссылкăн ячĕ',
'link_tip'       => 'Шалти ссылка',
'extlink_sample' => 'http://www.example.com ссылкăн ячĕ',
'extlink_tip'    => 'Тулаш ссылки (http:// префикс çинчен ан манăр)',
'headline_tip'   => 'Иккĕмĕш шайри ят',
'math_sample'    => 'Формулăна кунта кĕртĕр',
'math_tip'       => 'Математика формули (LaTeX форматпа)',
'nowiki_sample'  => 'Кунта фотматламан текста кĕртĕр.',
'nowiki_tip'     => 'Вики-ĕрешлевне пăхмалла мар',
'image_tip'      => 'Кĕртсе лартнă ӳкерчĕк',
'media_tip'      => 'Медиа-файл çине каçă',
'sig_tip'        => 'Сирĕн алă пусни тата вăхăт',
'hr_tip'         => 'Горизонтальлĕ йĕр (сахалтарах усă курăр)',

# Edit pages
'summary'                => 'Улшăнусен кĕске ăнлантарăвĕ',
'subject'                => 'Тема',
'minoredit'              => 'Кунта пěчěк улшăнусем кăна кěртнě',
'watchthis'              => 'Ку страницăна кěртекен  улшăнусем  хыççăн сăнамалла',
'savearticle'            => 'Страницăна çырса хур',
'preview'                => 'Епле курăнĕ',
'showpreview'            => 'Малтанлăхи курăну',
'showlivepreview'        => 'Епле курăнассине хăвăрт пахасси',
'showdiff'               => 'Кĕртнĕ улшăнусем',
'anoneditwarning'        => "'''Асăрхăр''': Эсир сайта хăвăр çинчен пĕлтермен, çавăнпа та ку страницăна улăштарнин журналне сирĕн IP-адреса çырса хума тивĕ.",
'missingcommenttext'     => 'Аяларах, тархасшăн, хăвар пĕлтерĕве çырăр.',
'summary-preview'        => 'Ăнлантару çапла пулĕ',
'subject-preview'        => 'Статья ячĕ çапла пулĕ',
'blockedtitle'           => 'Хутшăнакана ĕçлеме чарнă',
'blockedtext'            => "<big>'''Ку аккаунта е IP-адреса ĕçлеме чарнă.'''</big>

$1 администратор ĕçлеме чарнă. Сăлтавĕ çакă: ''«$2»''.

* Чарнă вăхăт: $8
* Çак вăхăтчен чарнă: $6
* Çакна чарнă: $7

$1 хутшăнакан патне, е ытти [[{{MediaWiki:Grouppage-sysop}}|администраторсем]] патне эсир çыру ярса меншĕн ĕçлеме чарнине сӳтсе явма пултаратăр.

Асăрхар: [[Special:Preferences|хăвăрăн ĕнерлевсенче]] эл. почта адресне çирĕплетмен пулсан, е сайта кĕмен пулсан — администратор патне çыру яраймастăр. Администратор сире çыру яма чарнă пулсан — ку хутĕнче те çыру яраймăр.

Сирĕн IP-адрес — $3, чару идентификаторĕ — #$5. Çырусенче, тархасшăн, вĕсене пĕлтерĕр.",
'blockednoreason'        => 'сăлтавне пĕлтермен',
'blockedoriginalsource'  => 'Аяларах «$1» страницăн текстне куратăр.',
'blockededitsource'      => "Аяларах эсир «$1» страницăра '''хăвсамăр тунă''' улшăнусене куратăр.",
'whitelistedittitle'     => 'Улăштарма сирĕн авторизациленмелле',
'loginreqtitle'          => 'Сайта кĕмелле',
'loginreqlink'           => 'сайта кĕр',
'loginreqpagetext'       => 'Ытти страницăсене курмашкăн сирĕн $1.',
'accmailtitle'           => 'Пароле леçрĕмĕр.',
'accmailtext'            => '$1 вăрттăн сăмахне кунта леçрĕмĕр: $2.',
'newarticle'             => '(Çĕнни)',
'newarticletext'         => 'Ссылка урлă эсир халлĕхе çук статья çине куçрăр.
Çĕнĕ статьяна кĕртес тесен аяларах вырнаçнă чӳречере текста çырăр.
(тĕплĕнрех пĕлес тесен [[Help:Пулăшу|пулăшу страниципе]] паллашăр).
Енчен те эсир кунта йăнăшпа лекнĕ пулсан — сирĕн браузерăн «Каялла» кнопка çине пусăр.',
'usercsspreview'         => "'''Ан манăр, эсир сирĕн css файл епле пулассине çеç куратăр, ăна халлĕхе çырса хуман!'''",
'userjspreview'          => "'''Астăвăр, ку сирĕн javascript-файлăн малтанхи курăмĕ кăна, ăна хальлĕхе çырса хуман!'''",
'updated'                => '(Çĕнелнĕ)',
'note'                   => '<strong>Ăнлантаркăч:</strong>',
'previewnote'            => '<strong>Ку страницăна халлěхе çырса хуман. Эсир ку страницă мěнле пулассине кăна куратăр!</strong>',
'previewconflict'        => 'Çӳлти чӳречере эсир халĕ çырса хурсан текст епле курăннине куратăр.',
'editing'                => '$1 тӳрлетни',
'editingsection'         => '$1 тӳрлетни (статья разделě)',
'editingcomment'         => '$1 тӳрлетни (кӗске анлантарӑвӗ)',
'editconflict'           => 'Тӳрлетнĕ вăхăтра тавлашу тухрĕ: $1',
'yourtext'               => 'Сирĕн текст',
'storedversion'          => 'Астуса хăварнă верси',
'yourdiff'               => 'Уйрăмлăхсем',
'copyrightwarning'       => 'Эсир кĕртнĕ, е улăштарнă пур статьясен тексчĕсем $2 ($1 пăхăр) лизензине пăхăнаççĕ. Енчен те эсир кĕртнĕ текста пурте усă курма, тӳрлетсе улăштарма пултарнипе килĕшместĕр пулсан кунта ăна ан кĕртĕр. <br /> Çавăн пекех эсир кĕртнĕ текстăн е авторĕ, е ăна ирĕклĕ усă курма юракан çăлкуçсенчен илнине çирĕплететĕр. <br /> <strong>АВТОР ПРАВИСЕМПЕ ХӲТĔЛЕНĔ МАТЕРИАЛСЕНЕ АН КĔРТĔР!</strong>',
'protectedpagewarning'   => '<strong>АСĂРХАТТАРНИ: ку страницăна улшăнусем кĕртессинчен хӳтĕленĕ, ăна тӳрлетме администраторсем кăна пултараççĕ.</strong>',
'titleprotectedwarning'  => '<strong>Асăрхаттару. Ку страницăпа ĕçлеме чарнă, ăна хăш-пĕр хутшăнакан кăна хатерлеме пултарать.</strong>',
'templatesused'          => 'Ку страница çинче усă курнă шаблонсем:',
'templatesusedpreview'   => 'Пăхакан страница çинче усă курнă шаблонсем:',
'templatesusedsection'   => 'Ку пайра усă курнă шаблонсем:',
'template-protected'     => '(сыхланă)',
'template-semiprotected' => '(пĕр пайне сыхланă)',
'nocreatetitle'          => 'Страницăсене хатĕрлессине чакарнă',

# Parser/template warnings
'expensive-parserfunction-category'      => 'Кунта эсир чылай ресурс ыйтакан функцисемпе нумай ĕçлекен страницăсене куратăр',
'post-expand-template-argument-category' => 'Шаблон аргуменчĕсене сиктерсе хăварнă страницăсем',

# "Undo" feature
'undo-norev'   => 'Ку тӳрлетĕве пăрахăçлама май çук — вăл е пулман та, е ăна кăларса пăрахнă.',
'undo-summary' => '$1 хутшăнакан [[Special:Contributions/$2|$2]] ([[User_talk:$2|сӳтсе яв]]) кĕртнĕ № улшăнăва тавăрнă',

# Account creation failure
'cantcreateaccounttitle' => 'Хутшăнакана кĕртме май çук',

# History pages
'viewpagelogs'        => 'Ку страницăн журналĕсене пăхасси',
'nohistory'           => 'Ку страницăн улшăнусен журналĕ çук.',
'revnotfound'         => 'Верси тупăнмарĕ',
'currentrev'          => 'Хальхи верси',
'revisionasof'        => '$1 верси',
'revision-info'       => '$1 версийĕ; $2',
'previousrevision'    => '&larr;Малтанхи верси',
'nextrevision'        => 'Çěнěрех верси→',
'currentrevisionlink' => 'хальхи верси çине куç',
'cur'                 => 'хальхи',
'next'                => 'тепěр',
'last'                => 'малт.',
'page_first'          => 'пĕрремĕш',
'page_last'           => 'юлашки',
'deletedrev'          => '[кăларса пăрахнă]',
'histfirst'           => 'Пĕрремĕш',
'histlast'            => 'Юлашки',
'historysize'         => '$1 байт',
'historyempty'        => '(пушă)',

# Revision feed
'history-feed-title'       => 'Тӳрлетӳсен историйĕ',
'history-feed-description' => 'Ку страницăна викире улаштарнин историйĕ',

# Revision deletion
'rev-deleted-comment'    => '(комментарине кăларса пăрахнă)',
'rev-deleted-user'       => '(автор ятне хуратнă)',
'rev-deleted-event'      => '(кăларса пăрахнă)',
'rev-delundel'           => 'кăтарт/пытар',
'revdelete-legend'       => 'Чару ларт:',
'revdelete-hide-text'    => 'Страницăн çак верси текстне пытар',
'revdelete-hide-comment' => 'Комментарине пытар',
'revdelete-hide-user'    => 'Автор ятне пытар',
'revdelete-success'      => 'Версин курăмлăхне улăштартăмăр.',
'pagehist'               => 'Страница историйĕ',
'deletedhist'            => 'Кăларса пăрахнисен историйĕ',
'revdelete-content'      => 'ăшлăхĕ',
'revdelete-uname'        => 'Хутшăнакан ячĕ',
'revdelete-hid'          => '$1 пытарнă',
'revdelete-unhid'        => '$1 курăмлă тунă',

# Suppression log
'suppressionlog' => 'Пытару журналĕ',

# Diffs
'difference' => '(Версисем хушшинчи улшăнусем)',
'lineno'     => '$1-мĕш йĕрке:',
'editundo'   => 'пăрахăçла',

# Search results
'searchresults'      => 'Тупрăмăр',
'notitlematches'     => 'Статьясен ячĕсем пĕр пек мар',
'textmatches'        => 'Статьясенчи текст пĕрпеклĕхĕ',
'prevn'              => 'малтанхи $1',
'nextn'              => 'малалли $1',
'showingresults'     => 'Аяларах эсир <b>#$2</b> пуçласа кăтартнă <b>$1</b> йĕркене куратăр.',
'powersearch'        => 'Анлă шырав',
'powersearch-legend' => 'Анлă шырав',

# Preferences page
'preferences'        => 'Ĕнерлевсем',
'mypreferences'      => 'Ман ĕнерлевсем',
'prefs-edits'        => 'Тӳрлетӳсен шучĕ:',
'prefsnologin'       => 'Эсир сайта кĕмен',
'qbsettings'         => 'Навигаци хăми',
'changepassword'     => 'Пароле улăштар',
'skin'               => 'Ерешленӳ',
'dateformat'         => 'Дата формачĕ',
'datedefault'        => 'Палăртман чухнехи',
'math_syntax_error'  => 'синтаксис йăнăшĕ',
'prefs-rc'           => 'Юлашки улшăнусен страници',
'prefs-watchlist'    => 'Сăнав списокĕ',
'prefs-misc'         => 'Ытти ĕнерлевсем',
'saveprefs'          => 'Çырса хур',
'oldpassword'        => 'Кивě пароль',
'newpassword'        => 'Çěнě пароль',
'textboxsize'        => 'Тӳрлетни',
'rows'               => 'Йěркесем',
'columns'            => 'Юпасем:',
'searchresultshead'  => 'Шыраса тупрăмăр',
'recentchangesdays'  => 'Çак кунсен хушшинчи юлашки улшăнусене кăтартмалла:',
'recentchangescount' => 'Списокра çакăн чухлĕ тӳрлетӳ кăтартмалла:',
'savedprefs'         => 'Сирĕн ĕнерлевсене сыхласа хăвартăмăр',
'timezonelegend'     => 'Сехет поясĕ',
'localtime'          => 'Вырăнти вăхăт',
'servertime'         => 'Сервер вăхăчě',
'files'              => 'Файлсем',

# User rights
'userrights'               => 'Хутшăнакансен прависемпе ĕçлесси', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Хутшăнакансен ушкăнĕсемпе ĕçлесси',
'userrights-user-editname' => 'Хутшăнакан ятне кăтартăр:',
'editinguser'              => "тӳрлетни '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Хутшăнакансен ушкăнĕсене улăштар',
'saveusergroups'           => 'Хутшăнаканăн ушкăнĕсем астуса юл',
'userrights-groupsmember'  => 'Çак ушкăнсене кĕрет:',
'userrights-reason'        => 'Улăштару сăлтавĕ:',

# Groups
'group'               => 'Ушкăн:',
'group-autoconfirmed' => 'Автоподтверждённые участники',
'group-bot'           => 'Ботсем',
'group-sysop'         => 'Администраторсем',
'group-bureaucrat'    => 'Бюрократсем',
'group-suppress'      => 'Ревизоры',
'group-all'           => '(пурте)',

'group-autoconfirmed-member' => 'автоподтверждённый участник',
'group-bot-member'           => 'бот',
'group-sysop-member'         => 'администратор',
'group-bureaucrat-member'    => 'бюрократ',
'group-suppress-member'      => 'Ревизор',

'grouppage-autoconfirmed' => '{{ns:project}}:Автоподтверждённые участники',
'grouppage-bot'           => '{{ns:project}}:Ботсем',
'grouppage-sysop'         => '{{ns:project}}:Администраторсем',
'grouppage-bureaucrat'    => '{{ns:project}}:Бюрократсем',
'grouppage-suppress'      => '{{ns:project}}:Ревизоры',

# User rights log
'rightslogtext' => 'Ку пользовательсен прависене улăштарниссен журналě',
'rightsnone'    => '(çук)',

# Recent changes
'recentchanges'     => 'Юлашки улшăнусем',
'rcnote'            => 'Юлашки <strong>$2</strong> кун хушшинчи <strong>$1</strong> улшăнусем. Халě пěтěмпе  <strong>{{NUMBEROFARTICLES}}</strong> статья.',
'rclistfrom'        => 'Юлашки улшăнусене $1 вăхăтран пуçласа кăтартнă',
'rcshowhideminor'   => 'пĕчĕк тӳрлетӳсене $1',
'rcshowhidebots'    => 'ботсене $1',
'rcshowhideliu'     => 'сайта кĕнĕскерсене $1',
'rcshowhideanons'   => 'ячĕсене палăртманскерсене $1',
'rcshowhidepatr'    => 'тĕрĕсленĕ тӳрлетӳсене $1',
'rcshowhidemine'    => 'хăвăн тӳрлетӳсене $1',
'rclinks'           => 'Юлашки $2 кун хушшинче тунă $1 улшăнусене кăтартмалла<br />$3',
'diff'              => 'танл.',
'hist'              => 'истори',
'hide'              => 'кăтартмалла мар',
'show'              => 'кăтартмалла',
'minoreditletter'   => 'п',
'newpageletter'     => 'Ç',
'rc_categories_any' => 'Кашни',
'newsectionsummary' => '/* $1 */ Çĕнĕ тема',

# Recent changes linked
'recentchangeslinked' => 'Çыхăннă улшăнусем',

# Upload
'upload'             => 'Файла кĕртесси',
'uploadbtn'          => 'Файла кĕрт',
'uploadnologin'      => 'Эсир сайта кĕмен.',
'uploadnologintext'  => 'Файла кĕртес умĕн сирĕн малтан [[Special:UserLogin|сайта кĕмелле]].',
'uploaderror'        => 'Файла кĕртне чухне йăнăш пулчĕ',
'uploadlog'          => 'Файлсене кĕртнин логĕ',
'uploadlogpage'      => 'Файлсене кĕртнине кăтартакан журнал',
'uploadlogpagetext'  => 'Аяларах эсир юлашкинчен кĕртнĕ файлсене куратăр.
Пур çĕрте те сервер вăхăтне (Гринвич тăрăх, UTC) кăтартнă.',
'filedesc'           => 'Кĕскен ăнлантарни',
'fileuploadsummary'  => 'Кĕске ăнлантару:',
'filestatus'         => 'Усă курмалли майсем',
'filesource'         => 'Çăлкуç',
'uploadedfiles'      => 'Кĕртнĕ файлсем',
'ignorewarning'      => 'Асăрхаттарусене пăхмасăр файла çав-çавах çырса хумалла.',
'ignorewarnings'     => 'Асăрхаттарусене шута илмелле мар',
'badfilename'        => 'Файл ятне $1 çине улăштарнă',
'filetype-badmime'   => '"$1" MIME-тĕслĕ файлсене кĕртейместпĕр.',
'filetype-missing'   => 'Файлăн хушма ячĕ тупăнмарĕ (тĕслĕхрен, «.jpg»).',
'largefileserver'    => 'Файл пысăкăшĕ юрăхлинчен пысăкрах (пĕчĕклетĕр).',
'successfulupload'   => 'Файла ăнăçлă тултартăмăр',
'uploadwarning'      => 'Асăрхаттару',
'savefile'           => 'Файла çырса хур',
'uploadedimage'      => '«[[$1]]» кĕртрĕмĕр',
'uploaddisabled'     => 'Каçарăр та сайта халĕ нимĕн те кĕртме юрамаст.',
'uploaddisabledtext' => 'Ку вики-сайтра файлсене кĕртме чарнă.',
'uploadvirus'        => 'Файл ăшĕнче вирус пур! $1 пăхăр.',
'sourcefilename'     => 'Файлăн чăн ячĕ',
'watchthisupload'    => 'Ку файла сăнав списокне кĕртмелле',
'filewasdeleted'     => 'Ку ятла файл малтан пулнă, анчах та ăна кăларса пăрахнă. Тархасшăн, $1 кĕртес умĕн тĕрĕслĕр.',

'upload-proto-error' => 'Йăнăш протокол',
'upload-file-error'  => 'Шалти йăнăш',
'upload-misc-error'  => 'Файла кĕртнĕ чухне паллă мар йăнăш пулчĕ',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error28' => 'Файла кĕртме уйăрнă вăхăт иртсе кайрĕ',

'nolicense'          => 'Çук',
'upload_source_url'  => ' (тĕрĕс, пурте курма пултаракан интертетел адресĕ)',
'upload_source_file' => ' (сирĕн компьютер çинчи файл)',

# Special:ImageList
'imagelist-summary'     => 'Ку ятарлă страницăра эсир пур кĕртнĕ файлсене куратăр.
Нумай пулмасть кĕртнисем çулте вырнаçнă.
Юпа тăррине пуссан йĕркелӳ майĕ улшăнĕ.',
'imagelist_search_for'  => 'Ӳкерчĕк ячĕ тăрăх шырани:',
'imagelist'             => 'Ӳкерчěксен списокě',
'imagelist_name'        => 'Файл ячĕ',
'imagelist_user'        => 'Хутшăнакан',
'imagelist_size'        => 'Виçи',
'imagelist_description' => 'Ăнлантаркăч',

# Image description page
'filehist'                  => 'Файл историйĕ',
'imagelinks'                => 'Ссылкăсем',
'linkstoimage'              => 'Çак страницăсем ку файл çине кăтартаççĕ:',
'nolinkstoimage'            => 'Ку файл çине кăтартакан страницăсем çук.',
'shareduploadwiki'          => 'Тĕплĕнрех информацине $1 синче тупма пулать.',
'noimage'                   => 'Ун пек ятлă файл çук, эсир ăна $1.',
'noimage-linktext'          => 'кĕртме пултаратăр',
'uploadnewversion-linktext' => 'Ку файлăн çĕнĕ версине кĕрт',

# File reversion
'filerevert' => 'Кивĕ $1 версине тавăр',

# File deletion
'filedelete'                  => '$1 кăларса пăрахасси',
'filedelete-legend'           => 'Файла кăларса пăрах',
'filedelete-intro'            => "Эсир '''[[Media:$1|$1]]''' кăларса пăрахатăр.",
'filedelete-submit'           => 'Кăларса пăрах',
'filedelete-success'          => "'''$1''' кăларса пăрахрăмăр.",
'filedelete-success-old'      => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\'-ăн $3, $2 вăхăтри версине кăларса пăрахнă.</span>',
'filedelete-iscurrent'        => 'Эсир ку файлăн юлашки версине кăларса пăрахасшăн. Тархасшăн, файла малтан кивĕ версинчен пĕрин патне те пулин тавăрăр.',
'filedelete-otherreason'      => 'Урăх сăлтав:',
'filedelete-reason-otherlist' => 'Урăх сăлтав',

# MIME search
'mimesearch' => 'MIME тăрăх шырани',

# Unwatched pages
'unwatchedpages' => 'Никам та сăнаман страницăсем',

# List redirects
'listredirects' => 'Куçарусен списокĕ',

# Unused templates
'unusedtemplates'     => 'Усă курман шаблонсем',
'unusedtemplatestext' => 'Ку страница çинче страницăсенче усă курман «Шаблон» ятлă ятсен уçлăхне шутне кĕрекен страницăсене куратăр.',
'unusedtemplateswlh'  => 'ытти ссылкăсем',

# Random page
'randompage' => 'Ăнсăртран илнě страницă',

# Random redirect
'randomredirect' => 'Ăнсăртран илнĕ куçару',

# Statistics
'sitestats'              => 'Сайт статистики',
'userstats'              => 'Хутшăнакансен статистики',
'statistics-mostpopular' => 'Чи нумай пăхакан страницăсем',

'disambiguations'      => 'Нумай пĕлтерĕшлĕ статьясене кăтартакан страницăсем',
'disambiguationspage'  => 'Template:Disambig',
'disambiguations-text' => "Çак статьясем '''нумай пĕлтерĕшле страницăсем'''çине куçараççĕ.
Унта куçарас вырăнне вĕсем кирлĕ страницăсем çине куçармалла пулĕ.<br />
Енчен те страница çинче [[MediaWiki:Disambiguationspage]] страницăра кăтартнă шаблон ятне вырнаçтарнă пулсан вăл нумай пĕлтерĕшлĕ страница шутланать.",

'doubleredirects' => 'Икĕ хут куçаракансем',

'brokenredirects'        => 'Татăк куçару страницисем',
'brokenredirectstext'    => 'Ку куçару страницисем çук страницăна куçараççĕ:',
'brokenredirects-edit'   => '(тӳрлет)',
'brokenredirects-delete' => '(кăларса пăрах)',

'fewestrevisions' => 'Сахал тӳрлетнĕ статьясем',

# Miscellaneous special pages
'nviews'                  => '$1 хут пăхнă',
'specialpage-empty'       => 'Ку страница пушă',
'lonelypages'             => 'Тăлăх страницăсем',
'uncategorizedpages'      => 'Каталогсăр страницăсем',
'uncategorizedcategories' => 'Каталога кĕртмен категорисем',
'uncategorizedimages'     => 'Категорисĕр ӳкерчĕксем',
'uncategorizedtemplates'  => 'Категорисĕр шаблонсем',
'unusedcategories'        => 'Усă курман категорисем',
'unusedimages'            => 'Усă курман файлсем',
'wantedcategories'        => 'Хатĕрлемелли категорисем',
'wantedpages'             => 'Хатĕрлемелли страницăсем',
'mostlinked'              => 'Ку страницăсем çине чи нумай ссылка лартнă',
'mostlinkedcategories'    => 'Ку категорисем чи нумай ссылкăсем лартнă',
'mostlinkedtemplates'     => 'Чи анлă усă куракан шаблонсем',
'mostcategories'          => 'Чи нумай категорине кĕртнĕ страницăсем',
'mostimages'              => 'Чи анлă усă куракан ӳкерчĕксем',
'mostrevisions'           => 'Чи нумай тӳрлетнĕ страницăсем',
'prefixindex'             => 'Сăмах пуçламăшĕсен кăтартмăшĕ',
'shortpages'              => 'Кĕске статьясем',
'longpages'               => 'Вăрăм страницăсем',
'deadendpages'            => 'Ниăçта та урăх ертмен страницăсем',
'protectedpages'          => 'хӳтĕленĕ страницăсем',
'protectedtitles'         => 'Юраман ятсем',
'listusers'               => 'Хутшăнакансен списокĕ',
'newpages'                => 'Çěнě страницăсем',
'newpages-username'       => 'Хутшăнакан:',
'ancientpages'            => 'Чи кивĕ статьясем',
'move'                    => 'Ятне улăштар',
'movethispage'            => 'Ку страницăн ятне улăштар',
'unusedcategoriestext'    => 'Çак категори страницисем çинче ытти категорисемпе статьясем çук.',
'notargettitle'           => 'Тĕллевне кăтартман',

# Book sources
'booksources'    => 'Кĕнекесен çăлкуçĕсем',
'booksources-go' => 'Шыра',

# Special:Log
'specialloguserlabel' => 'Хутшăнакан:',
'log'                 => 'Журналсем',
'all-logs-page'       => 'Журналсем',
'log-search-submit'   => 'Туп',

# Special:AllPages
'allpages'         => 'Пěтěм страницăсем',
'alphaindexline'   => '$1-$2',
'nextpage'         => 'Тепěр страницă ($1)',
'prevpage'         => 'Малтанхи страница ($1)',
'allarticles'      => 'Пĕтĕм статьясем',
'allinnamespace'   => 'Пĕтĕм статьясем («$1» ят уçлăхĕ)',
'allpagesprev'     => 'Малтанхисем',
'allpagesnext'     => 'Тепěр',
'allpagessubmit'   => 'Ту',
'allpagesprefix'   => 'Çак йĕрекесенчен пуçланакансен страницăсене шыра:',
'allpagesbadtitle' => 'Страницăн ячĕ тĕрĕс мар. Ятĕнче е интервики, е чĕлхе префиксĕ е усă курма юраман символсем пур.',
'allpages-bad-ns'  => '{{SITENAME}}-ра «$1» ят уçлăхĕ çук.',

# Special:Categories
'categories'                    => 'Категорисем',
'categoriespagetext'            => 'Викинче çак категорисем пур.',
'special-categories-sort-count' => 'шучĕ тăрăх йĕркеле',
'special-categories-sort-abc'   => 'алфавит тăрăх йĕркеле',

# Special:ListUsers
'listusers-submit'   => 'Кăтарт',
'listusers-noresult' => 'Хутшăнакансем тупăнмарĕç.',

# Special:ListGroupRights
'listgrouprights-group'    => 'Ушкăн',
'listgrouprights-helppage' => 'Help:Ушкăн прависем',

# E-mail user
'emailuser'       => 'Хутшăнакана çыру яр... (Письмо участнику)',
'usermailererror' => 'Электронлă почта урлă пĕлтерӳ янă чухне йăнăш тухрĕ:',
'noemailtitle'    => 'Электронлă почта адресĕ çук',
'noemailtext'     => 'Ку хутшăнакан электронлă адресне кăтартман е ытти хутшăнакансенчен çыру илесшĕн мар.',
'emailfrom'       => 'Камран',
'emailto'         => 'Кам патне',
'emailsubject'    => 'Çыру теми',
'emailsend'       => 'Яр',
'emailccme'       => 'Çыру копине ман пата ямалла',
'emailsent'       => 'Çырăва леçрĕмĕр',
'emailsenttext'   => 'Сирĕн электронлă çырăва леçрĕмĕр.',

# Watchlist
'watchlist'            => 'Пăхса тăракан страницăсем списокĕ',
'mywatchlist'          => 'Ман сăнав списокĕ',
'nowatchlist'          => 'Эсир пăхса тăракан страницăсен списокĕ пушă.',
'watchnologin'         => 'Системăна хăвăр ята каламалла',
'watchnologintext'     => 'Сăнав списокне улăштармашкăн сирĕн системăна [[Special:UserLogin|хăвăр ята каламалла]].',
'addedwatch'           => 'Сăнамалли шутне кĕртрĕмĕр',
'addedwatchtext'       => '«[[:$1]]» страницăна сирĕн [[Special:Watchlist|сăнав списока]] хушрăмăр. Малашне [[Special:RecentChanges|юлашки улшăнусене кăтартакан страницăра]] унпа çыхăннă улшăнусене хылăм шрифтпа курăнĕç.
Енчен те сирĕн ку страницăна сăнас кăмăл пĕтсен, «ан сăна» пускăч çине пусăр.',
'removedwatch'         => 'Сăнамалли списокран кăларса пăрахнă',
'removedwatchtext'     => '«[[:$1]]» страницăна сирĕн сăнамалли списокран кăларса пăрахнă.',
'watch'                => 'Сăна',
'watchthispage'        => 'Ку страницăна сăнаса тăр',
'unwatch'              => 'ан сăна',
'unwatchthispage'      => 'Сăнама пăрах',
'notanarticle'         => 'Ку статья мар',
'watchlist-show-bots'  => 'Ботсен ĕçне кăтарт',
'watchlist-hide-bots'  => 'Ботсен ĕçне пытар',
'watchlist-show-own'   => 'Ман тӳрлетӳсене кăтарт',
'watchlist-hide-own'   => 'Ман тӳрлетӳсене пытар',
'watchlist-show-minor' => 'Пĕчĕк тӳрлетӳсене кăтарт',
'watchlist-hide-minor' => 'Пĕчĕк тӳрлетӳсене пытар',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Сăнамаллисем шутне хушасси…',
'unwatching' => 'Сăнав списокĕнчен кăларса пăрахасси…',

'enotif_reset'       => 'Пур страницăсене те пăхнă пек палăрт',
'enotif_newpagetext' => 'Ку çĕнĕ страница',
'changed'            => 'улăштарнă',
'created'            => 'хатĕрленĕ',
'enotif_anon_editor' => '$1 анонимлă хутшăнакан',

# Delete/protect/revert
'deletepage'                  => 'Кăларса парахнă статьясем',
'confirm'                     => 'Çирĕплетни',
'excontent'                   => 'ăшлăхĕ: «$1»',
'excontentauthor'             => 'ăшлăхĕ: «$1» ([[Special:Contributions/$2|$2]] пĕччен кăна улшăнусем кĕртнĕ)',
'exblank'                     => 'страница пушă пулнă',
'actioncomplete'              => 'Турăмăр',
'deletedtext'                 => '«$1» кăларса парахрăмăр.
Юлашки кăларса пăрахнă статьясен списокне курмашкăн кунта пăхăр: $2.',
'suppressedarticle'           => '«[[$1]]» пытарнă',
'dellogpage'                  => 'Кăларса пăрахнисем',
'dellogpagetext'              => 'Аяларах эсир юлашки кăларса пăрахнă статьясене куратăр.',
'deletionlog'                 => 'кăларса пăрахнисем',
'reverted'                    => 'Малтанхи версине тавăрнă',
'deletecomment'               => 'Кăларса пăрахнин сăлтавĕ',
'rollback'                    => 'Тÿрлетÿсене каялла куçарасси',
'rollback_short'              => 'Каялла куçарасси',
'rollbacklink'                => 'каялла тавăр',
'rollbackfailed'              => 'Каялла куçарнă çухна йăнăш тухнă',
'protectlogpage'              => 'Хӳтĕлев журналĕ',
'protectlogtext'              => 'Аяларах эсир статьясене хӳтĕленин тата хӳтĕлев паллине пăрахаçланин журналне куратăр.',
'protectedarticle'            => '«[[$1]]» страницăна хӳтĕлерĕмĕр',
'unprotectedarticle'          => '«[[$1]]» страницăн хӳтĕлевне пăрахăçланă',
'protectcomment'              => 'Страницăна хӳтĕленин сăлтавĕ',
'protectexpiry'               => 'Пĕтет:',
'protect-level-autoconfirmed' => 'Статьяна çĕнĕрен регистрациленнĕ, е пачах та регистрациленменнисенчен хӳтĕле',
'protect-level-sysop'         => 'Администраторсем çеç',

# Undelete
'undelete'               => 'Кăларса пăрахнă страницăсене пăх',
'viewdeletedpage'        => 'Кăларса пăрахнă страницăсене пăх',
'undeleterevisions'      => 'Архивра пурĕ $1 верси',
'undeletebtn'            => 'Каялла тавăр!',
'undeletereset'          => 'Тасат',
'undeletedarticle'       => '«[[$1]]» каялла тавăрнă',
'undeletedrevisions'     => '$1 кăларса пăрахнă тӳрлетӳсене каялла тавăрнă',
'undelete-search-box'    => 'Кăларса пăрахнă страницăсен хушшинчи шырав',
'undelete-search-submit' => 'Шыра',

# Namespace form on various pages
'blanknamespace' => '(Тĕп)',

# Contributions
'contributions' => 'Хутшăнакан ӳсĕмĕ',
'mycontris'     => 'Манăн ӳсĕм',
'contribsub2'   => '$1 ӳсĕмĕ ($2)',
'uctop'         => ' (пуçламăш)',

'sp-contributions-blocklog' => 'Чарса лартнисен журналĕ',
'sp-contributions-search'   => 'Тӳпе шыравĕ',
'sp-contributions-submit'   => 'Шырас',

# What links here
'whatlinkshere'       => 'Кунта килекен ссылкăсем',
'linklistsub'         => '(ссылкăсен списокĕ)',
'linkshere'           => "Çак страницăсем '''[[:$1]]''' çине илсе килеççĕ:",
'nolinkshere'         => "'''[[:$1]]''' страница çине ытти страницăсенчен килме пулмасть.",
'whatlinkshere-links' => '← ссылкăсем',

# Block/unblock
'blockip'                  => 'Хăтшăнакана ĕçлеме чар',
'ipbreason'                => 'Сăлтавĕ',
'ipbother'                 => 'Урăх вăхăт:',
'ipbotheroption'           => 'урăххи',
'badipaddress'             => 'IP-адрес формачĕ тĕрĕс мар, е ку ятлă хутшăнакан кунта çук.',
'blockipsuccesssub'        => 'Ĕçлеме чартăмăр',
'blockipsuccesstext'       => '[[Special:Contributions/$1|«$1»]] ĕçлеме чарнă.
<br />[[Special:IPBlockList|ĕçлеме чарнă IP-адрессен списокне]] пăх.',
'unblockip'                => 'IP-адреса блокировкăран калар',
'unblocked-id'             => '$1 чарса лартнине ирĕке кăларнă',
'ipblocklist'              => 'Чарнă IP-адрессемпе хутшăнакансен списокĕ',
'ipblocklist-username'     => 'Хутшăнакан ячĕ, е IP-адрес:',
'ipblocklist-submit'       => 'Туп',
'ipblocklist-empty'        => 'Чару списокĕ пушă.',
'blocklink'                => 'ĕçлеме чар',
'unblocklink'              => 'блокировкăран кăлар',
'contribslink'             => 'ӳсĕмĕ',
'blocklogpage'             => 'Ĕçлеме чарнисен журналĕ',
'unblocklogentry'          => '«$1» блокировкăран кăларнă',
'block-log-flags-anononly' => 'анонимлă хутшăнакансем кăна',
'block-log-flags-nocreate' => 'хутшăнакансене регистрациленме чарнă',
'block-log-flags-noemail'  => 'çыру яма чарнă',

# Move page
'move-page-legend'        => 'Страницăна куçарнă',
'movearticle'             => 'Страницăн ятне улăштар',
'newtitle'                => 'Çĕнĕ ят',
'move-watch'              => 'Ку страницăна сăнамаллисем шутне хуш',
'movepagebtn'             => 'Страницăн ятне улăштар',
'pagemovedsub'            => 'Куçарас ěç тěрěс иртрě',
'articleexists'           => 'Ку ятлă статья е пур, е ун пек ят пама юрамасть.
Тархасшăн, статьяна урăх ят парăр.',
'talkexists'              => "'''Страницăн ятне улăштартăмăр, анчах та сӳтсе явмалли страницăн ятне улăштараймарăмар — вăл ятлă страницăна маларах хатĕрленĕ. Тархасшăн, вĕсене хăвăр тĕллĕн çыпăçтарăр.'''",
'movetalk'                => 'Статьяна сӳтсе явнă страницăн ятне те улăштармалла',
'1movedto2'               => '$1 $2 çине куçарнă',
'1movedto2_redir'         => '«[[$1]]» статья ятне маларах куçаракан страница пулнă «[[$2]]» çине улăштарнă',
'movelogpage'             => 'Статьясен ятне улăштарнине кăтартакан журнал',
'movereason'              => 'Сăлтавĕ',
'delete_and_move'         => 'Кăларса пăрахса куçарасси',
'delete_and_move_text'    => '==Кăларса пăрахмалла==
[[:$1|«$1»]] ятлă страница пур. Урăх ят парас тесе ăна кăларса пăрахмалла-и?',
'delete_and_move_confirm' => 'Ку страницăна чăнах та кăларса пăрахмалла',
'delete_and_move_reason'  => 'Урăх ят памашкăн кăларса парахнă',

# Export
'export'           => 'Статьясен экспорчĕ',
'export-submit'    => 'Экспортла',
'export-addcat'    => 'Хуш',
'export-templates' => 'Шаблонсене ĕçлеттер',

# Namespace 8 related
'allmessages'         => 'Система пĕлтерĕвĕсем',
'allmessagesname'     => 'Пĕлтерӳ',
'allmessagescurrent'  => 'Хальхи текст',
'allmessagesmodified' => 'Улăштарнисене кăна кăтартмалла',

# Thumbnails
'thumbnail-more'           => 'Пысăклатмалли',
'filemissing'              => 'Файл тупăнмарĕ',
'thumbnail_error'          => 'Пĕчĕк ӳкерчĕке тăваймарăмăр: $1',
'thumbnail_invalid_params' => 'Пĕчĕк ӳкерчĕкĕн параметрĕ йăнăш',

# Special:Import
'import'                     => 'Страницăсене импортласси',
'importinterwiki'            => 'Вики хушшинчи импорт',
'import-interwiki-submit'    => 'Импортирла',
'import-interwiki-namespace' => 'Страницăсене çак ят уçлăхне вырнаçтар:',
'importnopages'              => 'Импортламалли страницăсем çук.',
'importnotext'               => 'Тексчĕ çук',
'importnofile'               => 'Импортламалли файла тиемен',
'import-noarticle'           => 'Импортламалли страница çук!',

# Import log
'importlogpage'          => 'Импорт журналĕ',
'import-logentry-upload' => '«[[$1]]» — файлтан импортла',

# Tooltip help for the actions
'tooltip-pt-userpage'    => 'Пользователь страници',
'tooltip-pt-mytalk'      => 'Ман канашлу страници',
'tooltip-pt-preferences' => 'Настройкӑсем',
'tooltip-ca-talk'        => 'Статьяна сӳтсе явасси',
'tooltip-ca-edit'        => 'Эсир ку страницӑна тӳрлетме пултаратӑр. Тархасшӑн ҫырса хӑваричен страницӑ мӗнле пулассине пӑхӑр.',
'tooltip-ca-addsection'  => 'Кӗске ӑнлантару хушма пултаратӑр.',
'tooltip-ca-viewsource'  => 'Ку страницӑна эсир улӑштарма пултараймастӑр. Ӑна мӗнле ҫырнине кӑна пӑхма пултаратӑр.',
'tooltip-ca-protect'     => 'Улӑшратусенчен сыхласси',
'tooltip-ca-delete'      => 'Страницӑна кӑларса пӑрахмалли',
'tooltip-ca-move'        => 'Страницӑна урӑх ҫӗре куҫарасси',
'tooltip-ca-watch'       => 'Ку страницӑ хыҫҫӑн сӑнама пуҫласси',
'tooltip-ca-unwatch'     => 'Ку страницӑ хыҫҫӑн урӑх сӑнамалла мар',
'tooltip-search'         => 'Шырав',
'tooltip-p-logo'         => 'Тӗп страницӑ',
'tooltip-save'           => 'Тӳрлетӳсене астуса хăвармалла',
'tooltip-watch'          => 'Çак страницăна пăхса тăмаллисем шутне хуш',

# Attribution
'anonymous'        => '{{GRAMMAR:genitive|{{SITENAME}}}} анонимлă хутшăнакансем',
'lastmodifiedatby' => 'Ку страницăна юлашки хут $2, $1 вăхăтра $3 хутшăнакан улăштарнă.', # $1 date, $2 time, $3 user
'othercontribs'    => '$1 ĕçĕ çинче никĕсленнĕ.',
'others'           => 'ыттисем',
'creditspage'      => 'Пархатарлăх',

# Spam protection
'spambot_username' => 'MediaWiki спамран тасатни',

# Info page
'infosubtitle'   => 'Страница çинчен',
'numedits'       => 'Улшăнусен шучĕ (статьясем): $1',
'numtalkedits'   => 'Улшăнусен шучĕ (сӳтсе явмалли страница): $1',
'numwatchers'    => 'Сăнакансен шучĕ: $1',
'numtalkauthors' => 'Тĕрлĕ авторсен шучĕ (çӳтсе явмалли страница): $1',

# Math options
'mw_math_png'    => 'Яланах PNG хатĕрлемелле',
'mw_math_simple' => 'Ахаль чухне HTML, урăхла чухне PNG',
'mw_math_html'   => 'май пулсан HTML, май çук пулсан — PNG',
'mw_math_modern' => 'Хальхи браузерсемпе усă курмашкăн сĕннĕ пек',

# Patrolling
'markaspatrolleddiff'   => 'Тĕрĕсленĕ тесе палăрт',
'markaspatrolledtext'   => 'Ку статьяна тĕрĕсленĕ тесе палăртмалла',
'markedaspatrolled'     => 'Тĕрĕсленĕ пек палăртнă',
'markedaspatrolledtext' => 'Суйланă версине тĕрĕсленĕ пек палăртнă.',

# Image deletion
'deletedrevision'       => '$1 кивĕ версине кăларса парахнă.',
'filedeleteerror-short' => 'Файла кăларса пăрахнă чухне йăнăш тухрĕ: $1',

# Browsing diffs
'nextdiff' => 'Малаллахи тӳрлетӳ →',

# Media information
'file-info'      => '(файл пысăкăшĕ: $1, MIME-тĕсĕ: $2)',
'file-info-size' => '($1 × $2 пиксел, файл пысăкăше: $3, MIME-тĕсĕ: $4)',
'file-nohires'   => '<small>Пысăкрах калăпăшли çук.</small>',

# Special:NewImages
'newimages'         => 'Çĕнĕ файлсен галерейи',
'newimages-summary' => 'Ку ятарлă страницăра эсир нумай пулмасть кĕртнĕ файлсене куратăр',
'noimages'          => 'Ӳкерчĕксем çук.',
'ilsubmit'          => 'Шырамалла',
'bydate'            => 'дата тăрăх',

# EXIF tags
'exif-exifversion'     => 'Exif версийĕ',
'exif-flashpixversion' => 'Ĕçлеме пултаракан FlashPix версийĕ',
'exif-gpsversionid'    => 'GPS-информаци блокĕн версийĕ',

'exif-lightsource-0' => 'Паллă мар',

'exif-scenecapturetype-0' => 'Стандартлă',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'км/сех',

# External editor support
'edit-externally' => 'Ку файла тулашри программăна усă курса тӳрлет',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'пурте',
'imagelistall'     => 'Пурте',
'watchlistall2'    => 'пурте',
'namespacesall'    => 'пурте',

# E-mail address confirmation
'confirmemail_sent'     => 'Çирĕплетмелли ыйтуллă çырăва ятăмăр.',
'confirmemail_success'  => 'Сирĕн электронлă почтăн адресне çирĕплентĕ.',
'confirmemail_loggedin' => 'Сирĕн электронлă почтăн адресне çирĕплетрĕмĕр.',

# Scary transclusion
'scarytranscludefailed'  => '[Шел те, $1 шаблонпа усă кураймарăмăр]',
'scarytranscludetoolong' => '[Питĕ шел, URL ытла вăрăм]',

# Trackbacks
'trackbackremove' => ' ([$1 кăларса пăрах])',

# Delete conflict
'deletedwhileediting' => 'Асăрхăр: эсир тӳрлетнĕ вăхăтра ку страницăна кăларса парахнă!',
'recreate'            => 'Çĕнĕрен пуçла',

# HTML dump
'redirectingto' => '[[:$1]] çине куçаракан страница …',

# AJAX search
'searchcontaining' => 'Ăшĕсенче «$1» пур статьясене шыратпăр.',
'searchnamed'      => '$1 ятлă статьясене шыратпăр',
'articletitles'    => '«$1»-ран пуçланакан статьясем',

# Multipage image navigation
'imgmultipageprev' => '← малтанхи страница',
'imgmultipagenext' => 'тепĕр страница →',
'imgmultigo'       => 'Куç!',

# Table pager
'table_pager_next'         => 'Тепĕр страница',
'table_pager_prev'         => 'Малтанхи страница',
'table_pager_first'        => 'Пĕрремĕш страница',
'table_pager_last'         => 'Юлашки страница',
'table_pager_limit'        => 'Страница çинче $1 кăтарт',
'table_pager_limit_submit' => 'Ту',
'table_pager_empty'        => 'Тупăнмарĕ',

# Auto-summaries
'autosumm-blank'   => 'Статьяна тĕппипех кăларса пăрахнă',
'autosumm-replace' => 'Страницăн ăшлăхне «$1» çине улăштарнă',
'autoredircomment' => '[[$1]] çине куçарни',
'autosumm-new'     => 'Çĕнни: $1',

# Live preview
'livepreview-loading' => 'Тултаратпăр…',
'livepreview-ready'   => 'Тултаратпăр… Пулчĕ!',

# Watchlist editor
'watchlistedit-noitems' => 'Сирĕн сăнав списокĕ пушă.',

# Watchlist editing tools
'watchlisttools-view' => 'Ку тӳрлетӳпе çыхăннăскерсем',

# Special:Version
'version' => 'MediaWiki версийĕ', # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath'        => 'Файл çулĕ',
'filepath-submit' => 'Çул',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Пĕр пек файлсен шыравĕ',
'fileduplicatesearch-legend'   => 'Дубликатсен шыравĕ',
'fileduplicatesearch-filename' => 'Файл ячĕ:',
'fileduplicatesearch-submit'   => 'Туп',

# Special:SpecialPages
'specialpages'                   => 'Ятарлă страницăсем',
'specialpages-group-maintenance' => 'Техника обслуживанийĕн отчечĕсем',
'specialpages-group-other'       => 'Ытти ятарлă страницăсем',
'specialpages-group-login'       => 'Регистрациленесси',
'specialpages-group-changes'     => 'Юлашки улшăнусемпе журналсем',
'specialpages-group-media'       => 'Медиа-материалсемпе тултарăшсем',
'specialpages-group-users'       => 'Хутшăнакансем тата правасем',
'specialpages-group-highuse'     => 'Нумай усă куракан страницăсем',

);
